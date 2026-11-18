#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <Preferences.h>
#include "../utils/DuckError.h"
#include "../include/cdpcfg.h"
#include "../radio/DuckLoRa.h"
#include "DuckTypes.h"
#include "../utils/DuckUtils.h"
#include <cassert>
#include "../CdpPacket.h"
#include "../DuckEsp.h"
#include "../wifi/DuckWifiNone.h"
#include "../routing/DuckRouter.h"
#include "../routing/RouteJSON.h"
#include "../routing/SizedQueue.h"
#include "../utils/MemoryFree.h"
#include "../utils/DuckClock.h"
#include <esp_system.h>
#include <algorithm>

//templated class to require some radio capability
template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class Duck {
  public:
    virtual ~Duck(){
    };
    Preferences eeprom_preferences;
    /**
     * @brief Duck main running loop
     */
    void run(){
      duckRadio.serviceInterruptFlags();
      Duck::logIfLowMemory();
      if (!meshReady) return;
      // Membership is local readiness, never permission to exchange mesh data.
      // Gateway reachability is reported independently by the router.
      if (duckRadio.getReceiveFlag()) queueReceivedPacket();
      if (auto packet = rxQueue.dequeue()) processReceivedPacket(*packet);
      if (initialDiscoveryPending && uint32_t(millis() - setupAt) >= initialDiscoveryDelay) {
        initialDiscoveryPending = false;
        discover(BROADCAST_DUID, 1); // startup, one-hop neighbour discovery
      }
      // Do not dequeue until pacing allows an attempt. Alternate control/data
      // when both are pending, so route discovery cannot starve detections.
      if (uint32_t(millis() - lastPacketTx) >= CDPCFG_MAX_PACKET_SEND_RATE) {
        const bool control = reqQueue.size() && (preferControl || !txQueue.size());
        SizedQueue& queue = control ? reqQueue : txQueue;
        if (const CdpPacket* front = queue.front()) {
          CdpPacket packet = *front;
          lastPacketTx = millis();
          const int err = sendToRadio(packet);
          if (err == DUCKLORA_ERR_TX_BUSY) ++radioBusy;
          else {
            queue.dequeue();
            preferControl = !control;
            if (err == DUCK_ERR_NONE) ++radioStarted;
            else ++radioFailed;
          }
        }
      }
      duckTimer.tick();
    }

    int setupWithDefaults() {
      this->setupSerial(115200);
      if (!eeprom_preferences.begin("duck")) {
        logerr_ln("Failed to initialise EEPROM, aborting duck setup");
        return DUCK_ERR_EEPROM_INIT;
      }

#if CDPCFG_REMOTE_TX_POWER
      // TX power last set by a topics::cmd_tx command, if any. A stored value
      // outside 14 to 22 dBm, or one this radio doesn't take, would make radio
      // setup fail at every boot: remove it and use the default. The check
      // only compares numbers, so it runs before the radio is set up.
      int txPwr = CDPCFG_RF_LORA_TXPOW;
      if (eeprom_preferences.isKey("txPwr")) {
        const int stored = eeprom_preferences.getInt("txPwr", CDPCFG_RF_LORA_TXPOW);
        const char* refused = nullptr;
        if (stored < 14 || stored > 22) {
          refused = "is outside 14 to 22 dBm";
        } else if (duckRadio.checkTxPower(stored) != DUCK_ERR_NONE) {
          refused = "is not supported by this radio";
        }
        if (refused) {
          dropStoredTxPower(stored, refused);
        } else {
          txPwr = stored;
        }
      }
#else
      int txPwr = CDPCFG_RF_LORA_TXPOW;
#endif
      Serial.print("tx power value on startup : ");
      Serial.println(txPwr);

      const LoRaConfigParams radioParam = {
        /* band     = */ CDPCFG_RF_LORA_FREQ,
        /* txPower  = */ static_cast<int8_t>(txPwr),
        /* bw       = */ CDPCFG_RF_LORA_BW,
        /* sf       = */ CDPCFG_RF_LORA_SF,
        /* gain     = */ CDPCFG_RF_LORA_GAIN,
        /* func     = */ duckRadio.onInterrupt
    };

      int err = this->setupLoRaRadio(radioParam);
#if CDPCFG_REMOTE_TX_POWER
      // Not expected after the check above. If setup still fails with a stored
      // power, try once more with the default. setupRadio() starts again from
      // begin(), which resets the radio. Remove the stored value only if that
      // works, so a radio fault doesn't erase it.
      if (err != DUCK_ERR_NONE && txPwr != CDPCFG_RF_LORA_TXPOW) {
        LoRaConfigParams fallback = radioParam;
        fallback.txPower = CDPCFG_RF_LORA_TXPOW;
        err = this->setupLoRaRadio(fallback);
        if (err == DUCK_ERR_NONE) {
          dropStoredTxPower(txPwr, "made radio setup fail");
        }
      }
#endif
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR setupWithDefaults rc = %d",err); 
      } else{
        duckTimer.every(CDPCFG_SIGNAL_INTERVAL_MS, sendSignalData, this);
        duckTimer.every(CDPCFG_ROUTE_KEEPALIVE_MS, sendRouteKeepalive, this);
        duckTimer.every(CDPCFG_HEALTH_INTERVAL_MS, sendRoutingDiagnostics, this);
        meshReady = true;
        setupAt = millis();
        bootEpoch = esp_random();
        initialDiscoveryDelay = 2000UL + esp_random() % 4000UL;
        initialDiscoveryPending = getType() != DuckType::PAPA && getType() != DuckType::DETECTOR;
        router.setNetworkState(NetworkState::PUBLIC);
        loginfo_ln("[ROUTE] open mesh ready; protocol=2 epoch=%lu", (unsigned long)bootEpoch);
      }
      return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a std::string representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE only if accepted into the software TX queue.
     */
    int sendData(uint8_t topic, const std::string data, const Duid targetDevice = PAPADUCK_DUID) {
      return sendData(topic, reinterpret_cast<const uint8_t*>(data.data()), data.size(), targetDevice);
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a vector of bytes representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE only if accepted into the software TX queue.
    */
    int sendData(uint8_t topic, const uint8_t* data, int length, const Duid targetDevice = PAPADUCK_DUID) {
      if (topic < reservedTopic::max_reserved) return DUCKPACKET_ERR_TOPIC_INVALID;
      if (!data || length <= 0 || size_t(length) > MAX_DATA_LENGTH) return DUCKPACKET_ERR_SIZE_INVALID;
      if (!meshReady) return DUCK_ERR_NOT_SUPPORTED;
      CdpPacket packet(targetDevice, topic, std::vector<uint8_t>(data, data + length), duid, getType());
      router.getFilter().assignUniqueMessageId(packet);
      if (!txQueue.enqueue(packet)) return DUCK_ERR_QUEUE_FULL;
      ++appAccepted;
      loginfo_ln("[CDP_DIAG] ACCEPT src=%s mid=%s topic=%u bytes=%u stage=queued",
        duckutils::duidToString(duid).c_str(), duckutils::arrayToHexString(packet.muid).c_str(),
        topic, (unsigned)packet.data.size());
      // Route discovery is advisory for this open mesh. Throttling it must never
      // suppress an application packet or pretend an enqueue succeeded.
      if (targetDevice != BROADCAST_DUID && getType() != DuckType::PAPA &&
          !router.getBestNextHop(targetDevice) &&
          (!hasRequestedRoute || uint32_t(millis() - lastRreqTime) >= 30000UL))
        discover(targetDevice, 2); // missing destination
      return DUCK_ERR_NONE;
    }

    /**
     * @brief sendData that allows sending for reserved topic ping
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPing(Duid targetDevice = BROADCAST_DUID){
      std::vector<uint8_t> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::ping, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send ping");
      }
      return err;
    }

    /**
     * @brief Get the duck's unique ID.
     * 
     * @returns A byte vector representing the duck's unique ID
     */ 
    std::vector<uint8_t> getDuckId() {
      return std::vector<uint8_t>(duid.begin(), duid.end());
    }

    /**
     * @brief Get the duck type.
     * @returns A value representing a DuckType
     */
    virtual DuckType getType() = 0;

    /**
     * @brief Check whether the duck has nothing left to send or process.
     *
     * True when the receive, data and control queues are empty and the radio
     * is not transmitting, holding an unread packet or waiting for an
     * interrupt to be serviced. Before deep sleep, keep calling run() until
     * this is true, or queued packets are lost. Use a timeout: on a busy mesh
     * a relay keeps getting packets to forward, and a packet can arrive at any
     * moment, so the answer only holds until the next run().
     * @returns true if no transmit, receive or control work is pending.
     */
    bool txIdle() const {
      return rxQueue.size() == 0 && txQueue.size() == 0 && reqQueue.size() == 0 &&
        !duckRadio.isBusy();
    }

    /**
     * @brief Put the LoRa radio to sleep, ready for ESP32 deep sleep.
     *
     * Call it once txIdle() is true (anything still queued is dropped), right
     * before esp_deep_sleep_start(). On success the duck stops: run() no
     * longer receives or sends, and sendData() returns DUCK_ERR_NOT_SUPPORTED.
     * Waking from deep sleep is a reboot, so the next boot runs
     * setupWithDefaults() again, which reinitialises the radio, queues, routes
     * and duplicate filter. This is not a light-sleep pause: there is no
     * resume without a reboot.
     * @returns DUCK_ERR_NONE, or the radio's error code, in which case the duck
     * keeps running.
     */
    int prepareForSleep() {
      const int err = duckRadio.sleep();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR prepareForSleep: radio sleep failed. rc = %d", err);
        return err;
      }
      meshReady = false;
      loginfo_ln("[CDP] radio asleep, duck stopped until the next boot");
      return DUCK_ERR_NONE;
    }

    /**
     * @brief Check whether the system clock holds a usable UTC time.
     *
     * The clock can be set by a topics::cmd_time from a PapaDuck (with
     * CDPCFG_REMOTE_TIME_SYNC), or by the sketch itself, e.g. from NTP or GPS.
     * It survives deep sleep and software resets, but not a power loss.
     * @returns true if the clock is at or after CDPCFG_TIME_MIN_EPOCH.
     */
    bool hasValidTime() const {
      return duckclock::now() >= static_cast<time_t>(CDPCFG_TIME_MIN_EPOCH);
    }

    // One readable radio heartbeat. v4 uses camelCase keys; c/fm retain their string types;
    // transmission completion is not a remote delivery acknowledgement.
    std::string healthMessage() {
      JsonDocument doc;
      doc["c"] = std::to_string(counter);
      doc["fm"] = std::to_string(freeMemory());
      doc["boot"] = bootEpoch;
      doc["uptimeS"] = uint32_t(millis()) / 1000;
      doc["neighbors"] = router.directNeighborCount();
      doc["txStarted"] = radioStarted;
      doc["txDone"] = duckRadio.getTxDoneCount();
      doc["txFailed"] = radioFailed;
      doc["txTimeouts"] = duckRadio.getTxTimeoutCount();
      doc["queueDrops"] = uint64_t(rxQueue.dropped) + txQueue.dropped + reqQueue.dropped;
      doc["v"] = 4;
      std::string result;
      serializeJson(doc, result);
      if (result.size() > MAX_DATA_LENGTH) return "";
      return result;
    }

    // Versioned, independently bounded health pages. All counters reset at boot;
    // b identifies that epoch, u is uptime in seconds. null means unavailable.
    // Detailed pages remain available locally; ordinary radio health uses v4.
    std::string routingDiagnostics(uint8_t page) {
      JsonDocument doc;
      doc["v"] = 2;
      doc["p"] = page;
      doc["b"] = bootEpoch;
      doc["u"] = uint32_t(millis()) / 1000;
      if (page == 0) {
        doc["C"] = std::to_string(counter);
        doc["FM"] = std::to_string(freeMemory());
        doc["r"] = int(esp_reset_reason());
        doc["n"] = int(router.getNetworkState());
        const auto hop = router.getBestNextHop(PAPADUCK_DUID);
        const auto age = router.getRouteAge(PAPADUCK_DUID);
        if (getType() == DuckType::PAPA) { doc["gw"] = "self"; doc["ga"] = 0; }
        else if (hop && age) { doc["gw"] = duckutils::duidToString(*hop); doc["ga"] = *age / 1000; }
        else { doc["gw"] = nullptr; doc["ga"] = nullptr; }
        doc["nb"] = router.directNeighborCount();
        doc["j"] = discoveryAttempts;
        doc["jr"] = discoveryReason;
        doc["nt"] = router.transitions;
      } else if (page == 1) {
        auto q = doc["q"].to<JsonArray>();
        q.add(rxQueue.size()); q.add(txQueue.size()); q.add(reqQueue.size());
        auto h = doc["h"].to<JsonArray>();
        h.add(rxQueue.highWater); h.add(txQueue.highWater); h.add(reqQueue.highWater);
        auto d = doc["d"].to<JsonArray>();
        d.add(rxQueue.dropped); d.add(txQueue.dropped); d.add(reqQueue.dropped);
      } else if (page == 2) {
        auto rq = doc["rq"].to<JsonArray>(); for (auto n : requestStats) rq.add(n);
        auto rp = doc["rp"].to<JsonArray>(); for (auto n : responseStats) rp.add(n);
        auto dr = doc["dr"].to<JsonArray>();
        dr.add(controlMalformed); dr.add(hopDropped); dr.add(controlLoopDropped);
      } else if (page == 3) {
        auto tx = doc["tx"].to<JsonArray>();
        tx.add(radioStarted); tx.add(duckRadio.getTxDoneCount()); tx.add(radioFailed);
        tx.add(radioBusy); tx.add(duckRadio.getTxTimeoutCount());
        auto app = doc["app"].to<JsonArray>();
        app.add(appAccepted); app.add(dataForwarded); app.add(dataSuppressed);
        if (router.signalSnapshot) {
          auto signal = doc["sg"].to<JsonArray>();
          signal.add((*router.signalSnapshot)[0]); signal.add((*router.signalSnapshot)[1]);
          signal.add(uint32_t(millis() - (*router.signalSnapshot)[2]) / 1000);
        } else doc["sg"] = nullptr;
        doc["ne"] = router.neighborEvictions;
      }
      std::string result;
      serializeJson(doc, result);
      // Never silently hand an oversized diagnostic packet to the radio.
      if (result.size() > MAX_DATA_LENGTH) return "";
      return result;
    }

    void joinWifiNetwork(std::string ssid = "", std::string password = ""){
      int err = this->duckWifi.joinNetwork(ssid, password);

      // If we fail to connect to WiFi, retry a few times
      if (err == DUCK_INTERNET_ERR_CONNECT) {
        int retry=0;
        while ( err ==  DUCK_INTERNET_ERR_CONNECT && retry < 5 ) {
          Serial.printf("WiFi connection failed, retry connection: %s\n", ssid.c_str());
          delay(5000);
          err = err = this->duckWifi.joinNetwork(ssid, password);
          retry++;
        }  
      }

        if (err == DUCK_INTERNET_ERR_CONNECT) {
          logerr_ln("ERROR wifi setup failed = %d",err);
        }
    }


  protected:
    /**
     * @brief Construct a new Duck object.
     *
     */
    Duck(std::string name){ //check this for correct length
      duid.fill(0);
      std::copy_n(name.begin(), std::min(name.size(), duid.size()), duid.begin());
    }

    RadioType duckRadio;
    WifiCapability duckWifi;
    static constexpr int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);
    Duid duid;
    DuckRouter router;


    
    /**
     * @brief Duck-type specific handler for different packet topics
     */ 
    virtual void handleReceivedPacket(CdpPacket packet) = 0;

    /**
     * @brief Report this duck's own health message (topics::health).
     *
     * Ducks send it over the mesh to the PapaDuck. A PapaDuck is that
     * destination, so it overrides this to hand the message straight to its
     * own application instead of using the radio.
     * @returns DUCK_ERR_NONE if the message was accepted, an error code otherwise.
     */
    virtual int reportHealth(const std::string& message) {
      return sendData(topics::health, message);
    }

    int broadcastPacket(CdpPacket& packet){
      if (packet.hopCount >= MAX_HOPS) { ++hopDropped; return DUCKPACKET_ERR_MAX_HOPS; }
      if (router.getFilter().bloom_check(packet.muid.data(), MUID_LENGTH)) {
        ++dataSuppressed;
        return DUCK_ERR_NONE;
      }
      CdpPacket relay = packet;
      ++relay.hopCount;
      if (!txQueue.enqueue(relay)) return DUCK_ERR_QUEUE_FULL;
      router.getFilter().bloom_add(packet.muid.data(), MUID_LENGTH);
      ++dataForwarded;
      return DUCK_ERR_NONE;
    }

    int forwardPacket(CdpPacket& packet){
      // Bounded duplicate-suppressed forwarding also works without a known
      // destination route, including after sleep or late gateway arrival.
      return broadcastPacket(packet);
    }

#if CDPCFG_REMOTE_TIME_SYNC
    /**
     * @brief Set the clock from a topics::cmd_time packet, {"epoch":<UTC seconds>}.
     *
     * Applied only if all of these hold:
     * - it is the first copy of this message. Later copies arrive after extra
     *   relay delay and would step the clock back, so call this before the
     *   packet is relayed or added to the duplicate filter;
     * - the sender is a PapaDuck;
     * - "epoch" is an integer of at least CDPCFG_TIME_MIN_EPOCH.
     * The caller relays the packet as usual, whether or not it was applied.
     * @returns true if the clock was set.
     */
    bool applyTimeCommand(CdpPacket& packet) {
      if (router.getFilter().bloom_check(packet.muid.data(), MUID_LENGTH)) {
        logdbg_ln("[TIME] cmd_time mid=%s already seen, ignored",
          duckutils::arrayToHexString(packet.muid).c_str());
        return false;
      }
      if (packet.duckType != DuckType::PAPA) {
        logwarn_ln("[TIME] cmd_time from %s rejected: sender type %u is not a PapaDuck",
          duckutils::duidToString(packet.sduid).c_str(), unsigned(packet.duckType));
        return false;
      }
      JsonDocument json;
      const DeserializationError error =
        deserializeJson(json, reinterpret_cast<const char*>(packet.data.data()), packet.data.size());
      if (error) {
        logerr_ln("[TIME] cmd_time from %s rejected: %s",
          duckutils::duidToString(packet.sduid).c_str(), error.c_str());
        return false;
      }
      const JsonVariantConst value = json["epoch"];
      if (!value.is<uint32_t>()) {
        logwarn_ln("[TIME] cmd_time from %s rejected: no integer epoch",
          duckutils::duidToString(packet.sduid).c_str());
        return false;
      }
      const uint32_t epoch = value.as<uint32_t>();
      if (epoch < CDPCFG_TIME_MIN_EPOCH) {
        logwarn_ln("[TIME] cmd_time from %s rejected: epoch %lu is before %lu",
          duckutils::duidToString(packet.sduid).c_str(), (unsigned long)epoch,
          (unsigned long)CDPCFG_TIME_MIN_EPOCH);
        return false;
      }
      if (duckclock::set(epoch) != 0) {
        logerr_ln("[TIME] failed to set the clock to %lu", (unsigned long)epoch);
        return false;
      }
      loginfo_ln("[TIME] clock set to %lu from %s", (unsigned long)epoch,
        duckutils::duidToString(packet.sduid).c_str());
      return true;
    }
#endif

    unsigned long lastRreqTime = 0L;
    unsigned long lastPacketTx = 0L;

    /**
     * @brief Set up USB serial port
     * @param baudRate baud rate  matching serial monitor
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */ 
    int setupSerial(int baudRate) {
      // // This gives us 10 seconds to do a hard reset if the board is in a bad state after power cycle
      // while (!Serial && millis() < 10000);
    
      Serial.begin(baudRate);
      loginfo_ln("setupSerial rc = %d",DUCK_ERR_NONE);
      loginfo_ln("Running CDP Version: %s",duckutils::getCDPVersion().c_str());
      loginfo_ln("CDP source revision: %s", CDPCFG_BUILD_REVISION);
      return DUCK_ERR_NONE;
    }

    /**
     * @brief Configure LoRa radio
     * @param baudRate baud rate  matching serial monitor
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */ 
    int setupLoRaRadio(const LoRaConfigParams& config = RadioType::defaultRadioParams){
      int err = duckRadio.setupRadio(config);

      if (err == DUCKLORA_ERR_BEGIN) {
        logerr_ln("ERROR setupRadio. Starting LoRa Failed. rc = %d",err);
        return err;
      }
      if (err == DUCKLORA_ERR_SETUP) {
        logerr_ln("ERROR setupRadio. Setup LoRa Failed. rc = %d",err);
        return err;
      }
      if (err == DUCKLORA_ERR_RECEIVE) {
        logerr_ln("ERROR setupRadio. Receive LoRa Failed. rc = %d",err);
        return err;
      }
    
      loginfo_ln("setupRadio rc = %d",DUCK_ERR_NONE);
    
      return DUCK_ERR_NONE;
    }

   

    /**
     * @brief Join a visible CDP network if existing
     */ 
    void attemptNetworkJoin(){
      // Compatibility entry point: no public/private membership handshake.
      if (meshReady) router.setNetworkState(NetworkState::PUBLIC);
    }

    /**
     * @brief sendData that allows sending for reserved topic rreq
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendRouteRequest(Duid targetDevice, RouteJSON json){
      std::string strJson = json.asString();
      std::vector<uint8_t> app_data;
      app_data.insert(app_data.end(), strJson.begin(), strJson.end());
      int err = sendReservedTopicData(targetDevice, reservedTopic::rreq, app_data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send rreq");
      }
      return err;
    }

    /**
     * @brief sendData that allows sending for reserved topic rreq
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendRouteResponse(Duid targetDevice, std::string data){
      std::vector<uint8_t> app_data;
      app_data.insert(app_data.end(), data.begin(), data.end());
      int err = sendReservedTopicData(targetDevice, reservedTopic::rrep, app_data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send rrep");
      }
      return err;
    }

    /**
     * @brief Refresh the routing entry for a directly-heard neighbour.
     *
     * Routes are otherwise only inserted on RREQ/RREP, so a duck that is actively
     * sending data still ages out of its neighbours' tables and every later send
     * has to rediscover it with an RREQ. A packet with hopCount 0 has not been
     * relayed, so its originator is provably one hop away and reachable right now.
     */
    void refreshNeighborRoute(const CdpPacket& packet) {
      if (packet.hopCount != 0 || !packet.receivedSignal) return;
      router.insertIntoRoutingTable(packet.sduid, packet.sduid, *packet.receivedSignal, packet.timeReceived);
      if (packet.duckType == DuckType::PAPA)
        router.insertIntoRoutingTable(PAPADUCK_DUID, packet.sduid, *packet.receivedSignal, packet.timeReceived);
    }

    /**
     * @brief Periodically broadcast an RREQ so neighbours refresh our entry in
     * their routing table and their RREPs refresh theirs in ours.
     *
     * Without this a duck that has nothing to say ages out of every table after
     * CDPCFG_ROUTE_TTL_MS even though it is still reachable -- which is why the
     * TTL previously had to be set longer than the slowest telemetry beacon.
     * @returns true so the timer stays armed.
     */
    static bool sendRouteKeepalive(void* p){
      Duck* duck = static_cast<Duck*>(p);
      if (duck->getType() == DuckType::PAPA || duck->getType() == DuckType::DETECTOR) return false;
      if (duck->meshReady) {
        duck->discover(BROADCAST_DUID, 3);
        if (!duck->router.getBestNextHop(PAPADUCK_DUID) &&
            (!duck->hasRequestedRoute || uint32_t(millis() - duck->lastRreqTime) >= 30000UL))
          duck->discover(PAPADUCK_DUID, 4); // late gateway/recovery
      }
      return true;
    }

    /**
     * @brief sendData that allows sending for signal info for DMS mapping on an internal timer
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    static bool sendSignalData(void* p){ //move this to mama/link?
      Duck* duckInstance = static_cast<Duck*>(p);
      if (duckInstance->getType() == DuckType::PAPA) {
        logdbg_ln("[DUCK] Skipping signal info send for PapaDuck.");
        return false;
      } else if (duckInstance->getType() == DuckType::DETECTOR) {
        logdbg_ln("[DUCK] Skipping signal info send for Detector.");
        return false;
      }
      int err;
      duckInstance->router.cullRoutingTable();
      std::optional<std::string> message = duckInstance->router.getEntriesFor(PAPADUCK_DUID, duckInstance->duid);

      if(message.has_value()){
        loginfo_ln("signal data: %s", message.value().c_str());
        err = duckInstance->sendData(topics::sig, message.value());
        if (err == DUCK_ERR_NONE) duckInstance->initialSignalSent = true;
        if (err != DUCK_ERR_NONE) {
          logdbg_ln("[DUCK] signal info for DMS message failed to send.");
        } else {
          logdbg_ln("[DUCK] signal info for DMS message successfully sent.");
        }
      } else { 
        logdbg_ln("[DUCK] No route entry for specified target was found.");
        JsonDocument doc;
        std::string strSourceDuid = duckutils::hexToString(duckutils::duidAsString(duckInstance->duid));
        doc["s"] = strSourceDuid;
        JsonArray neighborsArr = doc["n"].to<JsonArray>();
        doc["v"] = 2;
        std::string jsonString;
        serializeJson(doc, jsonString);
        
        err = duckInstance->sendData(topics::sig, jsonString);
      }
      
      return true;
    }

    /**
     * @brief sendData that allows sending for reserved topic pong
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendPong(Duid targetDevice = BROADCAST_DUID){
      std::vector<uint8_t> data(1, 0);
      int err = sendReservedTopicData(targetDevice, reservedTopic::pong, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send pong");
      }
      return err;
    }

    /**
     * @brief Enable duck radio to start receiving packets from the mesh network
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int startReceive(){
      int err = duckRadio.startReceive();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Restarting Duck...");
        duckesp::restartDuck();
      }
      return err;
    };

    /**
     * @brief Log an error message if the system's memory is too low.
     */
    static void logIfLowMemory() {
      if (duckesp::getMinFreeHeap() < MEMORY_LOW_THRESHOLD
        || duckesp::getMaxAllocHeap() < MEMORY_LOW_THRESHOLD
      ) {
        //logwarn_ln("WARNING heap memory is low");
      }
    }

    /**
     * @brief Calculate a signal score based on the current RSSI and SNR values.
     * The signal score is a value between 0 and 1, where 1 is the best possible
     * signal and 0 is the worst possible signal.
     *
     * The formula used to calculate the signal score is:
     *
     * signalScore = (normalizedRSSI + normalizedSNR) / 2
     *
     * where:
     *
     * normalizedRSSI = (RSSI - RSSI_MIN) / (RSSI_MAX - RSSI_MIN)
     * normalizedSNR = (SNR - SNR_MIN) / (SNR_MAX - SNR_MIN)
     *
     * RSSI_MIN = -131 dBm
     * RSSI_MAX = -20 dBm
     * SNR_MIN = -30 dB
     * SNR_MAX = 30 dB
     *
     * @returns SignalScore struct containing rssi, snr, and signalScore values
     */
    SignalScore getSignalScore(){
      SignalScore signalInfo;
      signalInfo.rssi = (duckRadio.getRSSI() - RSSI_MIN)/(RSSI_MAX-RSSI_MIN);
      signalInfo.snr = (duckRadio.getSNR() - SNR_MIN)/(SNR_MAX-SNR_MIN);
      signalInfo.signalScore = (signalInfo.rssi + signalInfo.snr) / 2.0f;
      return signalInfo;
    }


  private:
    Duck(Duck const&) = delete;
    Duck& operator=(Duck const&) = delete;
    SizedQueue rxQueue;
    SizedQueue txQueue;
    SizedQueue reqQueue;

    bool meshReady = false, initialDiscoveryPending = false, hasRequestedRoute = false;
    bool initialSignalSent = false;
    bool preferControl = true;
    uint32_t setupAt = 0, initialDiscoveryDelay = 0, bootEpoch = 0;
    uint32_t appAccepted = 0, dataForwarded = 0, dataSuppressed = 0;
    uint32_t radioStarted = 0, radioFailed = 0, radioBusy = 0;
    uint32_t controlMalformed = 0, hopDropped = 0, controlLoopDropped = 0;
    uint32_t discoveryAttempts = 0;
    uint8_t discoveryReason = 0, diagnosticPage = 0;
    // originated/enqueued, forwarded/enqueued, duplicate suppressed, dropped.
    std::array<uint32_t, 4> requestStats{}, responseStats{};
    struct ControlSeen {
      Duid source{}; Muid id{}; uint8_t topic = 0; uint32_t at = 0; bool valid = false;
    };
    std::array<ControlSeen, 64> controlHistory{};
    size_t historyIndex = 0;

#if CDPCFG_REMOTE_TX_POWER
    // Boot fallback for a stored topics::cmd_tx value: remove it and log one
    // line. The caller uses CDPCFG_RF_LORA_TXPOW.
    void dropStoredTxPower(int stored, const char* reason) {
      const bool removed = eeprom_preferences.remove("txPwr");
      logerr_ln("Stored tx power %d dBm %s: %s, using %d dBm", stored, reason,
        removed ? "removed" : "could not remove it", CDPCFG_RF_LORA_TXPOW);
    }
#endif

    bool controlSeen(const CdpPacket& packet) const {
      for (const auto& seen : controlHistory)
        if (seen.valid && uint32_t(millis() - seen.at) < CDPCFG_ROUTE_TTL_MS &&
            seen.source == packet.sduid && seen.id == packet.muid && seen.topic == packet.topic)
          return true;
      return false;
    }

    void rememberControl(const CdpPacket& packet) {
      controlHistory[historyIndex] = {packet.sduid, packet.muid, packet.topic, uint32_t(millis()), true};
      historyIndex = (historyIndex + 1) % controlHistory.size();
    }

    int enqueueControl(CdpPacket& packet, bool forwarded) {
      auto& stats = packet.topic == reservedTopic::rreq ? requestStats : responseStats;
      if (packet.data.empty() || packet.data.size() > MAX_DATA_LENGTH) {
        ++stats[3]; return DUCKPACKET_ERR_SIZE_INVALID;
      }
      if (packet.hopCount > MAX_HOPS) { ++stats[3]; ++hopDropped; return DUCKPACKET_ERR_MAX_HOPS; }
      if (!reqQueue.enqueue(packet)) { ++stats[3]; return DUCK_ERR_QUEUE_FULL; }
      ++stats[forwarded ? 1 : 0];
      if (!forwarded) rememberControl(packet);
      loginfo_ln("[ROUTE] enqueue t=%u src=%s mid=%s hop=%u to=%s relay=%u",
        packet.topic, duckutils::duidToString(packet.sduid).c_str(),
        duckutils::arrayToHexString(packet.muid).c_str(), packet.hopCount,
        RouteJSON::toText(packet.dduid).c_str(), forwarded);
      return DUCK_ERR_NONE;
    }

    void discover(Duid target, uint8_t reason) {
      RouteJSON request(target, duid);
      request.addToPath(duid);
      if (sendRouteRequest(target, request) == DUCK_ERR_NONE) {
        ++discoveryAttempts; discoveryReason = reason;
        if (target != BROADCAST_DUID) {
          lastRreqTime = millis();
          hasRequestedRoute = true;
        }
      }
    }

    static bool sendRoutingDiagnostics(void* p) {
      Duck* duck = static_cast<Duck*>(p);
      if (duck->getType() == DuckType::DETECTOR) return false;
      const std::string detail = duck->routingDiagnostics(duck->diagnosticPage);
      loginfo_ln("[CDP_DIAG] health_detail=%s", detail.c_str());
      const std::string message = duck->healthMessage();
      if (duck->reportHealth(message) == DUCK_ERR_NONE) {
        ++duck->counter;
        duck->diagnosticPage = (duck->diagnosticPage + 1) % 4;
      }
      // One early topology snapshot makes a new installation observable without
      // waiting for the first CDPCFG_SIGNAL_INTERVAL_MS.
      if (!duck->initialSignalSent && duck->getType() != DuckType::PAPA && duck->getType() != DuckType::DETECTOR)
        sendSignalData(duck);
      return true;
    }

    void observeVia(const CdpPacket& packet, Duid via) {
      if (!packet.receivedSignal || via == duid || via == PAPADUCK_DUID || via == BROADCAST_DUID) return;
      router.insertIntoRoutingTable(via, via, *packet.receivedSignal, packet.timeReceived);
    }

    bool handleRoutingPacket(CdpPacket packet) {
      if (getType() == DuckType::DETECTOR) return false;
      if (packet.topic != reservedTopic::rreq && packet.topic != reservedTopic::rrep) return false;
      auto& stats = packet.topic == reservedTopic::rreq ? requestStats : responseStats;
      if (packet.hopCount > MAX_HOPS) { ++stats[3]; ++hopDropped; return true; }
      auto parsed = RouteJSON::fromPacketData(packet.data);
      if (!parsed) { ++stats[3]; ++controlMalformed; return true; }
      RouteJSON& route = *parsed;
      if (packet.topic == reservedTopic::rreq) {
        const auto via = route.getlastInPath();
        if (!via || !route.startsAt(packet.sduid) || route.getOrigin() != packet.sduid || route.getDestination() != packet.dduid ||
            route.pathSize() != size_t(packet.hopCount) + 1) {
          ++stats[3]; ++controlMalformed; return true;
        }
        observeVia(packet, *via);
        if (controlSeen(packet)) { ++stats[2]; return true; }
        rememberControl(packet);
        if (route.contains(duid)) { ++stats[3]; ++controlLoopDropped; return true; }
        if (packet.receivedSignal)
          router.insertIntoRoutingTable(route.getOrigin(), *via, *packet.receivedSignal, packet.timeReceived);
        const bool broadcast = packet.dduid == BROADCAST_DUID;
        const bool destination = packet.dduid == duid ||
            (packet.dduid == PAPADUCK_DUID && getType() == DuckType::PAPA);
        if ((broadcast && packet.hopCount == 0) || destination) {
          auto reply = route.responseFrom(getType() == DuckType::PAPA ? PAPADUCK_DUID : duid, packet.muid);
          reply.setVia(duid);
          sendRouteResponse(*via, reply.asString());
        } else if (!broadcast) {
          if (getType() == DuckType::LINK) return true; // leaf, never advertise a transit path
          if (packet.hopCount >= MAX_HOPS) { ++stats[3]; ++hopDropped; return true; }
          route.addToPath(duid);
          packet.data = duckutils::stringToByteVector(route.asString());
          ++packet.hopCount;
          enqueueControl(packet, true);
        }
      } else {
        auto via = route.getVia();
        if (!via && packet.hopCount == 0) via = packet.sduid;
        if (via) observeVia(packet, *via);
        // Overhearing a unicast reply does not authorize regenerating it.
        if (packet.dduid != duid) return true;
        if (controlSeen(packet)) { ++stats[2]; return true; }
        rememberControl(packet);
        const bool legacyDirect = !route.getVia() && packet.hopCount == 0 && route.getDestination() == duid;
        if (!via || (!legacyDirect && (!route.startsAt(route.getDestination()) ||
            route.getlastInPath() != std::optional<Duid>(duid)))) {
          ++stats[3]; ++controlMalformed; return true;
        }
        if (packet.receivedSignal) {
          router.insertIntoRoutingTable(route.getOrigin(), *via, *packet.receivedSignal, packet.timeReceived);
          if (packet.duckType == DuckType::PAPA) {
            router.insertIntoRoutingTable(PAPADUCK_DUID, *via, *packet.receivedSignal, packet.timeReceived);
            router.insertIntoRoutingTable(packet.sduid, *via, *packet.receivedSignal, packet.timeReceived);
          }
        }
        if (route.getDestination() == duid) return true;
        if (getType() == DuckType::LINK) return true;
        route.popFromPath();
        const auto previous = route.getlastInPath();
        if (packet.hopCount >= MAX_HOPS) { ++stats[3]; ++hopDropped; return true; }
        if (!previous || *previous == *via) {
          ++stats[3]; ++controlLoopDropped; return true;
        }
        packet.dduid = *previous;
        route.setVia(duid);
        packet.data = duckutils::stringToByteVector(route.asString());
        ++packet.hopCount;
        // Preserve responder, MUID and accumulated hops through the return path.
        enqueueControl(packet, true);
      }
      return true;
    }

    void processReceivedPacket(CdpPacket& packet) {
      // Phoenix 2026 firmware addressed the PapaDuck as "11111111" in the header.
      if (packet.dduid == PAPADUCK_ALIAS_DUID) packet.dduid = PAPADUCK_DUID;
      if (handleRoutingPacket(packet)) return;
      if (packet.hopCount > MAX_HOPS) { ++hopDropped; return; }
      refreshNeighborRoute(packet);
      handleReceivedPacket(packet);
    }

    //Telemetry
    uint32_t counter = 1;
    Timer<10> duckTimer;

    /** 
     * @brief Read packets from CDP nodes responding to our network join request
     * @returns Optional<CdpPacket> if network join response is found, nullopt if not 
     */
    std::optional<CdpPacket> checkForNetworks(){ return std::nullopt; }

    /**
     * @brief Control access for public APIs to send certain reserved topic types
     *
     * @param targetDevice device uid to send to
     * @param topic reserved topic of data
     * @param data byte vector representing data to send
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendReservedTopicData(Duid targetDevice, reservedTopic topic, std::vector<uint8_t> data){
      if (!meshReady) return DUCK_ERR_NOT_SUPPORTED;
      if (data.empty() || data.size() > MAX_DATA_LENGTH) return DUCKPACKET_ERR_SIZE_INVALID;
      CdpPacket packet(targetDevice, topic, data, duid, getType());
      router.getFilter().assignUniqueMessageId(packet);
      if (topic == reservedTopic::rreq || topic == reservedTopic::rrep)
        return enqueueControl(packet, false);
      return txQueue.enqueue(packet) ? DUCK_ERR_NONE : DUCK_ERR_QUEUE_FULL;
    }

    /**
     * @brief Passes data to radiolib for rx
     *
     * @param txPacket CdpPacket to be sent 
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendToRadio(CdpPacket& txPacket) {
      int err = txPacket.prepareForSending();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Failed to build ping packet: %s, err = %i",getDuckErrorString(err), err);
        return err;
      }

      router.getFilter().bloom_add(txPacket.muid.data(), MUID_LENGTH);

      err = duckRadio.sendData(txPacket.asBytes());
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Lora sendData failed, err = %d", err);
      }
     
      return err;
    }

    void queueReceivedPacket(){
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData.has_value()) {
          logerr_ln("ERROR failed to get data from DuckRadio.");
          return;
        }
        CdpPacket rxPacket(rxData.value());
        logdbg_ln("Got data from radio. size: %d",rxPacket.size());
        rxPacket.timeReceived = millis();
        rxPacket.receivedSignal = getSignalScore();
        rxQueue.enqueue(rxPacket);
        //move handle receieve packet to duck base, turn old handlereceive into route protocol?
    }
    
};

#endif
