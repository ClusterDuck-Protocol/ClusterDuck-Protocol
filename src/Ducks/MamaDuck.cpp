#include "../MamaDuck.h"
#include "../MemoryFree.h"

int MamaDuck::setupWithDefaults(std::vector<byte> deviceId, String ssid, String password) {
  
  int err = Duck::setupWithDefaults(deviceId, ssid, password);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupRadio();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWifi("DuckLink");
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupDns();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupWebServer(true);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }

  err = setupOTA();
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR setupWithDefaults rc = " + String(err));
    return err;
  }
  duckutils::getTimer().every(CDPCFG_MILLIS_ALIVE, imAlive);

  return DUCK_ERR_NONE;
}

void MamaDuck::run() {
  duckRadio.serviceInterruptFlags();

  handleOtaUpdate();
  if (DuckRadio::getReceiveFlag()) {
    handleReceivedPacket();
    rxPacket->reset();
  }
  processPortalRequest();
}

void MamaDuck::handleReceivedPacket() {

  std::vector<byte> data;
  bool relay = false;
  
  loginfo("====> handleReceivedPacket: START");

  int err = duckRadio.readReceivedData(&data);
  if (err != DUCK_ERR_NONE) {
    logerr("ERROR failed to get data from DuckRadio. rc = "+ String(err));
    return;
  }
  logdbg("Got data from radio, prepare for relay. size: "+ String(data.size()));

  relay = rxPacket->prepareForRelaying(duid, data);
  if (relay) {
    loginfo("handleReceivedPacket: packet RELAY START");
    // NOTE:
    // Ducks will only handle received message one at a time, so there is a chance the
    // packet being sent below will never be received, especially if the cluster is small
    // there are not many alternative paths to reach other mama ducks that could relay the packet.
    if (rxPacket->getTopic() == reservedTopic::ping) {
      err = sendPong();
      if (err != DUCK_ERR_NONE) {
        logerr("ERROR failed to send pong message. rc = " + String(err));
      }
      return;
    }

    if (rxPacket->getTopic() == reservedTopic::ack) {
      CdpPacket packet = CdpPacket(rxPacket->getBuffer());
      handleAck(packet);
    }

    err = duckRadio.relayPacket(rxPacket);
    if (err != DUCK_ERR_NONE) {
      logerr("====> ERROR handleReceivedPacket failed to relay. rc = " + String(err));
    } else {
      loginfo("handleReceivedPacket: packet RELAY DONE");
    }
  }
}

void MamaDuck::handleAck(const CdpPacket & packet) {
  if (duckutils::isEqual(duid, packet.dduid)
    || duckutils::isEqual(BROADCAST_DUID, packet.dduid)
  ) {
    if (lastMessageMuid.size() == MUID_LENGTH) {
      const byte numPairs = packet.data[0];
      static const int NUM_PAIRS_LENGTH = 1;
      static const int PAIR_LENGTH = DUID_LENGTH + MUID_LENGTH;
      for (int i = 0; i < numPairs; i++) {
        int pairOffset = NUM_PAIRS_LENGTH + i*PAIR_LENGTH;
        std::vector<byte>::const_iterator duidOffset = packet.data.begin() + pairOffset;
        std::vector<byte>::const_iterator muidOffset = packet.data.begin() + pairOffset + DUID_LENGTH;
        if (std::equal(duid.begin(), duid.end(), duidOffset)
          && std::equal(lastMessageMuid.begin(), lastMessageMuid.end(), muidOffset)
        ) {
          loginfo("handleReceivedPacket: matched ack-MUID "
            + duckutils::toString(lastMessageMuid));
          lastMessageAck = true;
          break;
        }
      }
    }

    // TODO[Rory Olsen: 2021-06-23]: The application may need to know about
    //   acks. I recommend a callback specifically for acks, or
    //   similar.
  }
}

bool MamaDuck::getDetectState() { return duckutils::getDetectState(); }
