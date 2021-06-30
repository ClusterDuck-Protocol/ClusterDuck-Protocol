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

  handleOtaUpdate();
  if (getReceiveFlag()) {
    duckutils::setInterrupt(false);
    setReceiveFlag(false);

    handleReceivedPacket();
    rxPacket->reset();
    
    duckutils::setInterrupt(true);
    startReceive();
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

    if (rxPacket->getTopic() == reservedTopic::receipt) {
      CdpPacket packet = CdpPacket(rxPacket->getBuffer());

      if (duckutils::isEqual(duid, packet.dduid)) {
        if (lastMessageMuid.size() != MUID_LENGTH) {
          // This may indicate a duplicate DUID (if this MamaDuck hasn't
          // previously sent any packets on the network) or may indicate that
          // this MamaDuck has recently rebooted (and a previous message is
          // being ack'd).
          logwarn("handleReceivedPacket: received receipt with matching DUID "
            + duckutils::toString(duid)
            + " but no receipt is expected. Not relaying.");
        } else if(duckutils::isEqual(lastMessageMuid, packet.data)) {
          loginfo("handleReceivedPacket: matched receipt. Not relaying. Matching DDUID "
            + duckutils::toString(duid) + " and receipt-MUID "
            + duckutils::toString(lastMessageMuid));
          lastMessageReceipt = true;
        } else {
          // This may indicate a duplicate DUID on the network or it may be a
          // receipt for a previous message.
          logwarn("handleReceivedPacket: received receipt with matching DUID "
            + duckutils::toString(duid) + " but receipt-MUID "
            + duckutils::toString(packet.data)
            + " does not match. Expected receipt for MUID "
            + duckutils::toString(lastMessageMuid)
            + ". Not relaying.");
        }

        // Returning here prevents anything else from happening.
        // TODO[Rory Olsen: 2021-06-23]: The application may need to know about
        //   receipts. I recommend a callback specifically for receipts, or
        //   similar.
        return;
      } else {
        loginfo("handleReceivedPacket: relaying receipt for DDUID "
            + duckutils::toString(packet.dduid)
            + " and receipt-MUID "
            + duckutils::toString(packet.muid));
      }
    }

    err = duckRadio.relayPacket(rxPacket);
    if (err != DUCK_ERR_NONE) {
      logerr("====> ERROR handleReceivedPacket failed to relay. rc = " + String(err));
    } else {
      loginfo("handleReceivedPacket: packet RELAY DONE");
    }
  }
}

bool MamaDuck::getDetectState() { return duckutils::getDetectState(); }
