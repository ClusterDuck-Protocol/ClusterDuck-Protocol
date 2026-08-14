#ifndef DUCK_H
#define DUCK_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../utils/DuckError.h"
#include "../include/cdpcfg.h"
#include "../radio/DuckLoRa.h"
#include "DuckTypes.h"
#include "../utils/DuckUtils.h"
#include <cassert>
#include <unordered_map>
#include "../CdpPacket.h"
#include "../DuckEsp.h"
#include "../wifi/DuckWifiNone.h"
#include "../routing/DuckRouter.h"
#include "../routing/RouteJSON.h"
#include "../security/DuckIdentity.h"
#include "../security/DuckCrypto.h"
#include "../security/OpenDmsConfig.h"

#define NET_JOIN_DELAY 15000L

// Build-time default for encryptionEnabled_ (see setUplinkEncryptionEnabled()).
// Off by default: sketches must opt in, either via this build flag
// (-DDUCK_CRYPTO_DEFAULT_ENABLED=1 in platformio.ini) or at runtime.
#ifndef DUCK_CRYPTO_DEFAULT_ENABLED
#define DUCK_CRYPTO_DEFAULT_ENABLED 0
#endif

//templated class to require some radio capability
template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class Duck {
  public:
    virtual ~Duck(){
    };

    /**
     * @brief Duck main running loop
     */
    void run(){
      duckRadio.serviceInterruptFlags();
      Duck::logIfLowMemory();
      opendmsconfig::checkSerialProvisioning();
      if(router.getNetworkState() == NetworkState::PUBLIC) {
        if(duckRadio.getReceiveFlag()){
          handleReceivedPacket();
        }
      } else {
        if(this->getType() == DuckType::DETECTOR){
          loginfo_ln("Detector duck -- bypassing network search.");
          router.setNetworkState(NetworkState::PUBLIC);
        } else{
            attemptNetworkJoin();
            if(router.getNetworkState() == NetworkState::SEARCHING && (millis() > (NET_JOIN_DELAY * 5 + 5000L))){
              loginfo_ln("No existing network found, creating new CDP network...");
              router.setNetworkState(NetworkState::PUBLIC);
            }
        }
      }

    }

    int setupWithDefaults() {
      this->setupSerial(115200);
      int err = duckidentity::begin();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR setupWithDefaults failed to initialize DuckIdentity rc = %d", err);
        return err;
      }
      opendmsconfig::begin();
      // Overwrite the constructor-supplied, human-readable name-derived DUID
      // with the crypto-identity-derived DUID (SHA256(pubkey) truncated to
      // DUID_LENGTH bytes), so mesh addressing is self-certifying.
      duckidentity::getDuid(this->duid.data());
      err = this->setupLoRaRadio();
      if (err != DUCK_ERR_NONE) {
      logerr_ln("ERROR setupWithDefaults rc = %d",err); 
      }
      return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a std::string representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendData(uint8_t topic, const std::string data, const std::array<uint8_t,8> targetDevice = PAPADUCK_DUID) {
      int err = DUCK_ERR_NONE;
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }
      if(router.getNetworkState() == NetworkState::PUBLIC){
        std::vector<uint8_t> app_data;
        app_data.insert(app_data.end(), data.begin(), data.end());
        CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());

        std::optional<Duid> nextHop = router.getBestNextHop(txPacket.dduid);
        if(nextHop.has_value() || txPacket.dduid == PAPADUCK_DUID || txPacket.dduid == BROADCAST_DUID){
          router.getFilter().assignUniqueMessageId(txPacket);
          err = sendToRadio(txPacket);
        } else {
            if((millis() - this->lastRreqTime) > 30000){
              loginfo_ln("[DUCK] Destination not in table, sending new RREQ.");
              RouteJSON rreqDoc = RouteJSON(txPacket.dduid, this->duid);
              rreqDoc.addToPath(this->duid);
              sendRouteRequest(txPacket.dduid, rreqDoc);
              this->lastRreqTime = millis();
            }
        }
      }
        return err;
    }

    /**
     * @brief Send data to the CDP network mesh
     * @param topic the message topic
     * @param data a vector of bytes representing the data to send
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
    */
    int sendData(uint8_t topic, const uint8_t* data, int length, const std::array<uint8_t,8> targetDevice = PAPADUCK_DUID) {
      int err = DUCK_ERR_NONE;
      if (topic < reservedTopic::max_reserved) {
        logerr_ln("ERROR send data failed, topic is reserved.");
        return DUCKPACKET_ERR_TOPIC_INVALID;
      }

      if(router.getNetworkState() == NetworkState::PUBLIC){
        std::vector<uint8_t> app_data;
        app_data.insert(app_data.end(), &data[0], &data[length]);
        CdpPacket txPacket = CdpPacket(targetDevice, topic, app_data, this->duid, this->getType());

        std::optional<Duid> nextHop = router.getBestNextHop(txPacket.dduid);
        if(nextHop.has_value() || txPacket.dduid == PAPADUCK_DUID || txPacket.dduid == BROADCAST_DUID){
          router.getFilter().assignUniqueMessageId(txPacket);
          err = sendToRadio(txPacket);
        } else {
            if((millis() - this->lastRreqTime) > 30000){
              loginfo_ln("[DUCK] Destination not in table, sending new RREQ.");
              RouteJSON rreqDoc = RouteJSON(txPacket.dduid, this->duid);
              rreqDoc.addToPath(this->duid);
              sendRouteRequest(txPacket.dduid, rreqDoc);
              this->lastRreqTime = millis();
            }
        }
      }
      return err;
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
     * @brief Announce this Duck's own long-term X25519 public key so peers
     * can learn it and use sendEncryptedData()/decrypt encrypted_data
     * packets addressed to this Duck. Opt-in: nothing calls this
     * automatically. TOFU on the receiving end -- there is no signature or
     * verification beyond "first announcement seen for this SDUID wins".
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int announceIdentity(Duid targetDevice = BROADCAST_DUID){
      std::vector<uint8_t> data(duckidentity::getPublicKey(), duckidentity::getPublicKey() + duckidentity::PUBLIC_KEY_LENGTH);
      int err = sendReservedTopicData(targetDevice, reservedTopic::identity_announce, data);
      if (err != DUCK_ERR_NONE){
        logerr_ln("ERR: failed to send identity_announce");
      }
      return err;
    }

    /**
     * @brief Send data end-to-end encrypted to a specific peer Duck (session
     * mode, static-static X25519 ECDH between the two Ducks' long-term
     * identities). The peer's public key must already be known -- see
     * announceIdentity()/learnPeerIdentity() -- otherwise this returns
     * DUCK_ERR_CRYPTO_ECDH_FAILED without sending anything. Opt-in: existing
     * sendData() calls are completely unaffected.
     * @param topic the application-level topic (embedded in the encrypted
     * plaintext, NOT sent in cleartext -- the on-air topic is always
     * reservedTopic::encrypted_data).
     * @param data the plaintext application data to encrypt and send.
     * @param targetDevice the peer Duck's DUID.
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendEncryptedData(uint8_t topic, const std::string data, Duid targetDevice){
      std::optional<std::array<uint8_t, duckcrypto::PUBLIC_KEY_LENGTH>> peerKey = getPeerIdentity(targetDevice);
      if (!peerKey.has_value()) {
        logerr_ln("ERR: sendEncryptedData no known public key for peer, send announceIdentity() first");
        return DUCK_ERR_CRYPTO_ECDH_FAILED;
      }
      std::vector<uint8_t> plaintext;
      plaintext.reserve(1 + data.size());
      plaintext.push_back(topic);
      plaintext.insert(plaintext.end(), data.begin(), data.end());

      std::vector<uint8_t> onAirData(duckcrypto::NONCE_LENGTH + plaintext.size() + duckcrypto::TAG_LENGTH);
      uint8_t* nonceOut = onAirData.data();
      uint8_t* ciphertextOut = onAirData.data() + duckcrypto::NONCE_LENGTH;
      uint8_t* tagOut = onAirData.data() + duckcrypto::NONCE_LENGTH + plaintext.size();

      std::array<uint8_t, HEADER_AAD_LENGTH> aad = buildHeaderAad(this->duid, targetDevice, reservedTopic::encrypted_data);
      int rc = duckcrypto::encryptWithPeer(peerKey.value().data(), aad.data(), aad.size(),
                                            plaintext.data(), plaintext.size(),
                                            nonceOut, ciphertextOut, tagOut);
      if (rc != DUCK_ERR_NONE) {
        logerr_ln("ERR: sendEncryptedData encryptWithPeer failed rc = %d", rc);
        return rc;
      }

      CdpPacket txPacket = CdpPacket(targetDevice, reservedTopic::encrypted_data, onAirData, this->duid, this->getType());
      return routeAndSend(txPacket);
    }

    /**
     * @brief Send data one-way sealed to OpenDMS's pinned static public key
     * (src/security/OpenDmsConfig.h). Returns DUCK_ERR_CRYPTO_ECDH_FAILED
     * without sending if OpenDMS's public key has not been configured
     * (still the all-zero placeholder). Opt-in: existing sendData() calls
     * are completely unaffected -- callers must explicitly use this method
     * to get sealed uplink traffic.
     * @param topic the application-level topic (embedded in the encrypted
     * plaintext, NOT sent in cleartext -- the on-air topic is always
     * reservedTopic::sealed_uplink).
     * @param data the plaintext application data to seal and send.
     * @param targetDevice the device UID to receive the message (default is all papa devices)
     * @returns DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendSealedData(uint8_t topic, const std::string data, const std::array<uint8_t,8> targetDevice = PAPADUCK_DUID){
      if (!opendmsconfig::isConfigured()) {
        logerr_ln("ERR: sendSealedData OpenDMS static public key is not configured");
        return DUCK_ERR_CRYPTO_ECDH_FAILED;
      }
      std::vector<uint8_t> plaintext;
      plaintext.reserve(1 + data.size());
      plaintext.push_back(topic);
      plaintext.insert(plaintext.end(), data.begin(), data.end());

      std::vector<uint8_t> onAirData(duckcrypto::PUBLIC_KEY_LENGTH + duckcrypto::NONCE_LENGTH + plaintext.size() + duckcrypto::TAG_LENGTH);
      uint8_t* ephemeralPubOut = onAirData.data();
      uint8_t* nonceOut = onAirData.data() + duckcrypto::PUBLIC_KEY_LENGTH;
      uint8_t* ciphertextOut = onAirData.data() + duckcrypto::PUBLIC_KEY_LENGTH + duckcrypto::NONCE_LENGTH;
      uint8_t* tagOut = onAirData.data() + duckcrypto::PUBLIC_KEY_LENGTH + duckcrypto::NONCE_LENGTH + plaintext.size();

      std::array<uint8_t, HEADER_AAD_LENGTH> aad = buildHeaderAad(this->duid, targetDevice, reservedTopic::sealed_uplink);
      int rc = duckcrypto::sealToStatic(opendmsconfig::OPENDMS_STATIC_PUBLIC_KEY, aad.data(), aad.size(),
                                         plaintext.data(), plaintext.size(),
                                         ephemeralPubOut, nonceOut, ciphertextOut, tagOut);
      if (rc != DUCK_ERR_NONE) {
        logerr_ln("ERR: sendSealedData sealToStatic failed rc = %d", rc);
        return rc;
      }

      CdpPacket txPacket = CdpPacket(targetDevice, reservedTopic::sealed_uplink, onAirData, this->duid, this->getType());
      return routeAndSend(txPacket);
    }

    /**
     * @brief Enable or disable this Duck's uplink-encryption preference at
     * runtime. This is a plain preference flag read via
     * isUplinkEncryptionEnabled() -- it does NOT change what sendData()
     * does. Sketches decide whether to call sendData() or
     * sendSealedData()/sendEncryptedData() by checking
     * isUplinkEncryptionEnabled() themselves. Defaults to
     * DUCK_CRYPTO_DEFAULT_ENABLED (off unless a build flag overrides it),
     * and can be overridden either direction at runtime regardless of the
     * build-time default.
     */
    void setUplinkEncryptionEnabled(bool enabled) {
      encryptionEnabled_ = enabled;
    }

    /**
     * @brief Whether this Duck currently prefers encrypted sends. See
     * setUplinkEncryptionEnabled().
     */
    bool isUplinkEncryptionEnabled() const {
      return encryptionEnabled_;
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
      std::copy(name.begin(), name.end(),duid.begin());
    }

    RadioType duckRadio;
    WifiCapability duckWifi;
    static constexpr int MEMORY_LOW_THRESHOLD = PACKET_LENGTH + sizeof(CdpPacket);
    std::array<uint8_t,8> duid;
    DuckRouter router;

    /// See setUplinkEncryptionEnabled()/isUplinkEncryptionEnabled(). Seeded
    /// from the DUCK_CRYPTO_DEFAULT_ENABLED build flag (off by default);
    /// freely overridable at runtime either direction.
    bool encryptionEnabled_ = DUCK_CRYPTO_DEFAULT_ENABLED;

    /// Maximum number of peer public keys cached via identity_announce.
    /// Bounds RAM use; once full, new peers are simply not learned (no
    /// eviction) until a reboot. Small mesh deployments only.
    static constexpr size_t MAX_PEER_IDENTITIES = 16;
    std::unordered_map<std::string, std::array<uint8_t, duckcrypto::PUBLIC_KEY_LENGTH>> peerIdentities;

    /**
     * @brief Record a peer's long-term public key, learned via a received
     * identity_announce packet. TOFU: first announcement seen for a given
     * DUID is trusted and kept; later announcements from the same DUID are
     * ignored (does not overwrite), since accepting a later "announcement"
     * blindly would let an attacker who spoofs an SDUID silently swap out
     * an already-trusted peer's key.
     */
    void learnPeerIdentity(Duid peerDuid, const uint8_t* pubKey){
      // NOTE: uses raw-byte string construction, NOT duckutils::toString() --
      // that helper collapses any non-printable byte to the literal string
      // "ERROR: Non-printable character", which would collide every
      // hash-derived (non-printable) DUID into one bucket. std::string can
      // safely hold arbitrary bytes including embedded NULs.
      std::string key(peerDuid.begin(), peerDuid.end());
      if (peerIdentities.count(key)) {
        return;
      }
      if (peerIdentities.size() >= MAX_PEER_IDENTITIES) {
        logerr_ln("learnPeerIdentity: peer identity cache full, dropping announce from %s", key.c_str());
        return;
      }
      std::array<uint8_t, duckcrypto::PUBLIC_KEY_LENGTH> copy;
      std::copy(pubKey, pubKey + duckcrypto::PUBLIC_KEY_LENGTH, copy.begin());
      peerIdentities[key] = copy;
      loginfo_ln("learnPeerIdentity: cached public key for peer %s", key.c_str());
    }

    /**
     * @brief Look up a previously-learned peer's public key.
     * @returns the peer's public key, or std::nullopt if no
     * identity_announce has been received from that DUID yet.
     */
    std::optional<std::array<uint8_t, duckcrypto::PUBLIC_KEY_LENGTH>> getPeerIdentity(Duid peerDuid){
      auto it = peerIdentities.find(std::string(peerDuid.begin(), peerDuid.end()));
      if (it == peerIdentities.end()) {
        return std::nullopt;
      }
      return it->second;
    }

    /// Length of the header-binding AAD built by buildHeaderAad(): sduid(8)
    /// || dduid(8) || wire topic(1).
    static constexpr size_t HEADER_AAD_LENGTH = 8 + 8 + 1;

    /**
     * @brief Build the additional-authenticated-data bytes used to bind an
     * encrypted/sealed packet's cleartext CDP header to its ciphertext, so
     * a relay can't splice a captured ciphertext onto a different sender,
     * recipient, or topic and have it still authenticate.
     *
     * Deliberately excludes muid, hopCount, and dcrc: muid is assigned by
     * the router *after* the packet (and therefore the ciphertext) is
     * built, and hopCount/dcrc mutate on every relay hop -- binding either
     * would make a legitimately multi-hop-relayed packet fail to decrypt
     * at its final destination. sduid/dduid/topic are fixed by the
     * original sender and never rewritten in transit, so they're safe (and
     * sufficient) to bind. Both the encrypt and decrypt side MUST build
     * this identically or authentication will fail.
     */
    static std::array<uint8_t, HEADER_AAD_LENGTH> buildHeaderAad(const Duid& sduid, const Duid& dduid, uint8_t topic){
      std::array<uint8_t, HEADER_AAD_LENGTH> aad;
      std::copy(sduid.begin(), sduid.end(), aad.begin());
      std::copy(dduid.begin(), dduid.end(), aad.begin() + sduid.size());
      aad[sduid.size() + dduid.size()] = topic;
      return aad;
    }

    /**
     * @brief Route and transmit a pre-built packet using the same
     * best-next-hop / RREQ logic as sendData(). Shared by
     * sendEncryptedData()/sendSealedData().
     */
    int routeAndSend(CdpPacket& txPacket){
      int err = DUCK_ERR_NONE;
      if (router.getNetworkState() != NetworkState::PUBLIC) {
        return err;
      }
      std::optional<Duid> nextHop = router.getBestNextHop(txPacket.dduid);
      if (nextHop.has_value() || txPacket.dduid == PAPADUCK_DUID || txPacket.dduid == BROADCAST_DUID) {
        router.getFilter().assignUniqueMessageId(txPacket);
        err = sendToRadio(txPacket);
      } else {
        if ((millis() - this->lastRreqTime) > 30000) {
          loginfo_ln("[DUCK] Destination not in table, sending new RREQ.");
          RouteJSON rreqDoc = RouteJSON(txPacket.dduid, this->duid);
          rreqDoc.addToPath(this->duid);
          sendRouteRequest(txPacket.dduid, rreqDoc);
          this->lastRreqTime = millis();
        }
      }
      return err;
    }

    /**
     * @brief Attempt to decrypt a received reservedTopic::encrypted_data
     * packet using the sender's cached public key (learned via a prior
     * identity_announce). On success, returns the original app-level
     * topic and decrypted payload so the caller can dispatch/deliver it
     * as if it had been received via plain sendData(). Returns
     * std::nullopt if the sender's key isn't known, the data is malformed,
     * or authentication fails (message must be discarded either way).
     */
    std::optional<std::pair<uint8_t, std::vector<uint8_t>>> tryDecryptEncryptedData(const CdpPacket& rxPacket){
      std::optional<std::array<uint8_t, duckcrypto::PUBLIC_KEY_LENGTH>> peerKey = getPeerIdentity(rxPacket.sduid);
      if (!peerKey.has_value()) {
        logerr_ln("tryDecryptEncryptedData: no known public key for sender, dropping.");
        return std::nullopt;
      }
      if (rxPacket.data.size() < duckcrypto::NONCE_LENGTH + 1 + duckcrypto::TAG_LENGTH) {
        logerr_ln("tryDecryptEncryptedData: data too short (%d bytes), dropping.", (int)rxPacket.data.size());
        return std::nullopt;
      }
      size_t ciphertextLen = rxPacket.data.size() - duckcrypto::NONCE_LENGTH - duckcrypto::TAG_LENGTH;
      const uint8_t* nonce = rxPacket.data.data();
      const uint8_t* ciphertext = rxPacket.data.data() + duckcrypto::NONCE_LENGTH;
      const uint8_t* tag = rxPacket.data.data() + duckcrypto::NONCE_LENGTH + ciphertextLen;
      std::vector<uint8_t> plaintext(ciphertextLen);
      std::array<uint8_t, HEADER_AAD_LENGTH> aad = buildHeaderAad(rxPacket.sduid, rxPacket.dduid, rxPacket.topic);
      int rc = duckcrypto::decryptFromPeer(peerKey.value().data(), nonce, aad.data(), aad.size(),
                                            ciphertext, ciphertextLen, tag, plaintext.data());
      if (rc != DUCK_ERR_NONE) {
        logerr_ln("tryDecryptEncryptedData: decrypt failed rc = %d, dropping.", rc);
        return std::nullopt;
      }
      uint8_t topic = plaintext.front();
      std::vector<uint8_t> payload(plaintext.begin() + 1, plaintext.end());
      return std::make_pair(topic, payload);
    }

    
    /**
     * @brief Duck-type specific handler for different packet topics
     */ 
    virtual void handleReceivedPacket() = 0;

    int broadcastPacket(CdpPacket& packet){
      bool alreadySeen = router.getFilter().bloom_check(packet.muid.data(), MUID_LENGTH);
      int err = DUCK_ERR_NONE;
      if(alreadySeen){
        logdbg_ln("broadcastPacket: Packet already seen. No relay.");
      } else{
        packet.hopCount++;
        err = sendToRadio(packet);
      }
      return err;
    }
    //this->dduid == BROADCAST_DUID || this->dduid == PAPADUCK_DUID

    int forwardPacket(CdpPacket& packet){
      //next node checks if it has the destination in its table
      //if in table, and ttl hasn't expired, forward the packet
      //if in the table and ttl has expired, send a rreq and wait for response before sending?<--- do this later?
      //if the duck can't find the destination in its routing table then it just doesn't send
      int err = DUCK_ERR_NONE;
      std::optional<Duid> nextHop = router.getBestNextHop(packet.dduid);
      if(nextHop.has_value() || packet.dduid == PAPADUCK_DUID){ //do we need to make sure this duck isn't a papa?
        err = broadcastPacket(packet);
        if (err != DUCK_ERR_NONE) {
            logerr_ln("====> ERROR forwardPacket failed. rc = %d",err);
        } else {
            loginfo_ln("forwardPacket: packet RELAY DONE");
        }
      } else{
        std::string strDuid(packet.dduid.begin(), packet.dduid.end());
        logdbg_ln("no entry for this id, skipping relay DDuid: %s", strDuid.c_str());
      }
      return err;
    }

    unsigned long lastRreqTime = 0L;

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
      std::optional<CdpPacket> cdpNode = checkForNetworks();
      if(cdpNode.has_value()){
        //add an entry for the nearest neighbor, next hop is itself
        router.insertIntoRoutingTable(cdpNode->sduid, cdpNode->sduid, this->getSignalScore()); //should signal score be stored on cdp packet?
        router.setNetworkState(NetworkState::PUBLIC);
      } else {
        if((millis() - this->lastRreqTime) > NET_JOIN_DELAY){
          RouteJSON rreqDoc = RouteJSON(BROADCAST_DUID, this->duid);
          rreqDoc.addToPath(this->duid);
          sendRouteRequest(BROADCAST_DUID, rreqDoc);
          loginfo_ln("searching for networks....");
          lastRreqTime = millis();
        }
      }
    };

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
     * SNR_MIN = -11.5 dB
     * SNR_MAX = 11.5 dB
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

    /**
     * @brief Read packets from CDP nodes responding to our network join request
     * @returns Optional<CdpPacket> if network join response is found, nullopt if not 
     */
    std::optional<CdpPacket> checkForNetworks(){ 
      std::optional<CdpPacket> result;

      if (duckRadio.getReceiveFlag()){
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
          logerr_ln("ERROR failed to get data from DuckRadio.");
          result = std::nullopt;
        } else{
          CdpPacket rxPacket(rxData.value());
          if((rxPacket.topic == reservedTopic::rrep) && (rxPacket.dduid == this->duid)){ //should all packets without valid crc immediately be discarded at a lower level?
            result = std::optional<CdpPacket>{rxPacket}; 
          } else {
            result = std::nullopt;
          }
        }
      } else {
        result = std::nullopt;
      }
      return result;
    }

    /**
     * @brief Control access for public APIs to send certain reserved topic types
     *
     * @param targetDevice device uid to send to
     * @param topic reserved topic of data
     * @param data byte vector representing data to send
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     */
    int sendReservedTopicData(Duid targetDevice, reservedTopic topic, std::vector<uint8_t> data){
      int err = DUCK_ERR_NONE;
      if((router.getNetworkState() == NetworkState::PUBLIC) || ((router.getNetworkState() == NetworkState::SEARCHING) && (topic == reservedTopic::rreq))){
        CdpPacket txPacket = CdpPacket(targetDevice, topic, data, this->duid, this->getType());
        router.getFilter().assignUniqueMessageId(txPacket);
        err = txPacket.prepareForSending();
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR Failed to build packet, err = " + err);
          return err;
        }
        err = duckRadio.sendData(txPacket.asBytes());
        if (err != DUCK_ERR_NONE) {
          logerr_ln("ERROR Lora sendData failed, err = %d", err);
        }
      } 
      return err;
    }

    /**
     * @brief Passes data to radiolib for rx
     *
     * @param txPacket CdpPacket to be sent
     * @return DUCK_ERR_NONE if the data was sent successfully, an error code otherwise.
     *
     * @note **Uplink channel spreading**
     *
     * When this duck is the *originator* of a Papa-bound packet
     * (dduid == PAPADUCK_DUID && sduid == this->duid), the radio is
     * temporarily switched to a randomly selected channel from
     * CDPCFG_UPLINK_CHANNEL_POOL before transmission. This spreads TX
     * load across all 8 SX1302 demodulators and reduces last-hop
     * collisions at the gateway.
     *
     * Relayed packets (sduid != this->duid) are always sent on the shared
     * mesh channel (922.8 MHz, CDPCFG_RF_LORA_FREQ) regardless of their
     * destination, including when the destination is PapaDuck. This is
     * intentional: intermediate MamaDucks must be able to hear and forward
     * packets on the common channel. PapaDuck's SX1302 receives on all 8
     * channels simultaneously, so it will receive relayed packets on
     * 922.8 MHz without any issue.
     */
    int sendToRadio(CdpPacket& txPacket) {
      int err = txPacket.prepareForSending();
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Failed to build ping packet, err = " + err);
        return err;
      }

      router.getFilter().bloom_add(txPacket.muid.data(), MUID_LENGTH);

      bool isUplinkToPapa = (txPacket.dduid == PAPADUCK_DUID) &&
                             (txPacket.sduid == this->duid);
      if (isUplinkToPapa) {
        float uplinkFreq = duckRadio.getRandomUplinkChannel();
        duckRadio.setUplinkFrequency(uplinkFreq);
      }

      err = duckRadio.sendData(txPacket.asBytes());
      if (err != DUCK_ERR_NONE) {
        logerr_ln("ERROR Lora sendData failed, err = %d", err);
      }
     
      return err;
    }
};

#endif
