#ifndef MAMADUCK_H
#define MAMADUCK_H

#include <set>
#include "Duck.h"
#include "../utils/MemoryFree.h"
#include "../security/DuckCrypto.h"
#include "../security/OpenDmsConfig.h"
#include "../security/SecurityEventCounters.h"

template <typename WifiCapability = DuckWifiNone, typename RadioType = DuckLoRa>
class MamaDuck : public Duck<WifiCapability, RadioType> {
public:
    using Duck<WifiCapability, RadioType>::Duck;

    MamaDuck(std::string name = "MAMA0001") : Duck<WifiCapability, RadioType>(std::move(name)) {}

    ~MamaDuck() {};

    using rxDoneCallback = void (*)(CdpPacket data);
    /**
     * @brief Register callback for handling data received from duck devices
     * 
     * The callback will be invoked if the packet needs to be relayed (i.e not seen before)
     * @param cb a callback to handle data received by the papa duck
     */
    void onReceiveDuckData(rxDoneCallback cb) { this->recvDataCallback = cb; }

    /** Skip the RREQ discovery phase and go operational immediately. */
    void goPublic() { this->router.setNetworkState(NetworkState::PUBLIC); }

    /**
     * @brief Get the DuckType
     * 
     * @returns the duck type defined as DuckType
     */
    DuckType getType() {return DuckType::MAMA;}

private :
    WifiCapability duckWifi;
    rxDoneCallback recvDataCallback;

    /**
     * @brief Peers we've replied to with our own announceIdentity() after
     * receiving a directed identity_announce FROM them. See the
     * identity_announce case in ifNotBroadcast() below: without this
     * reciprocal reply, TOFU key exchange is one-directional -- a peer
     * that announces its identity to us (e.g. because it's about to send
     * us an encrypted MTALK) lets US learn ITS key, but gives it no way
     * to learn OURS, so anything WE try to encrypt back to it keeps
     * failing with "no known public key for peer" until that peer's own
     * independent periodic/boot broadcast happens to reach us. Replying
     * once per peer closes that gap within one round trip. Bounded by
     * only recording each peer once (insert-if-absent) to avoid an
     * infinite announce/reply/announce/reply loop between two Ducks.
     */
    std::set<Duid> repliedIdentityTo_;

    /**
     * @brief Consecutive encrypted_cmd decrypt failures since the last
     * success (or the last re-announce). See its use in ifNotBroadcast()'s
     * topics::encrypted_cmd case: repeated failures while OpenDMS is
     * configured are more likely key/identity drift (e.g. a stale TOFU
     * entry on OpenDMS's side) than an ongoing attack, so once this
     * crosses ENCRYPTED_CMD_FAILURE_REANNOUNCE_THRESHOLD, this Duck
     * re-broadcasts its own identity so OpenDMS can re-learn/re-confirm
     * the correct key rather than silently dropping every command
     * forever. Does not loosen any verification -- the failing packet is
     * still dropped either way.
     */
    uint8_t consecutiveEncryptedCmdFailures_ = 0;
    static constexpr uint8_t ENCRYPTED_CMD_FAILURE_REANNOUNCE_THRESHOLD = 3;

    /**
     * @brief Handles any packets received by the duck. Overrides the pure virtual function in Duck base class.
     * Could be a RREQ, RREP, PING, PONG or DATA packet on its associated topic.
     *
     */
    void handleReceivedPacket() override {
        loginfo_ln("====> handleReceivedPacket: START");

        int err;
        std::optional<std::vector<uint8_t>> rxData = this->duckRadio.readReceivedData();
        if (!rxData) {
        logerr_ln("ERROR failed to get data from DuckRadio.");
        return;
        }
        CdpPacket rxPacket(rxData.value());
        logdbg_ln("Got data from radio. size: %d",rxPacket.size());

        // recvDataCallback(rxPacket); crashes the duck if callback body not defined in sketch
        
        //Check if Duck is desitination for this packet before relaying
        std::string rxDduid(rxPacket.dduid.begin(), rxPacket.dduid.end());
        std::string myDuid(this->duid.begin(), this->duid.end());
        loginfo_ln("handleReceivedPacket: dduid=%s myDuid=%s topic=%d", rxDduid.c_str(), myDuid.c_str(), (int)rxPacket.topic);

        if (duckutils::isEqual(BROADCAST_DUID, rxPacket.dduid)) {
            ifBroadcast(rxPacket, err);
        } else if(duckutils::isEqual(this->duid, rxPacket.dduid)) { //Target device check
            loginfo_ln("handleReceivedPacket: packet is FOR ME, delivering to sketch");
            ifNotBroadcast(rxPacket, err);
        } else { //If it's meant for a specific target but not this one
            loginfo_ln("handleReceivedPacket: packet NOT for me, relaying");
            ifNotBroadcast(rxPacket, err, true);
        }
        this->router.getFilter().bloom_add(rxPacket.muid.data(), MUID_LENGTH);
    }

    void ifBroadcast(CdpPacket rxPacket, int err) {
        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                if(rxPacket.hopCount <= 0){
                    loginfo_ln("RREQ received from %s. Sending Response!", rxPacket.sduid.data());
                    RouteJSON rrepDoc = RouteJSON(rxPacket.sduid, this->duid);
                    rrepDoc.addToPath(this->duid);
                    this->sendRouteResponse(rxPacket.sduid, rrepDoc.asString());
                    // Update routing table with signal info
                    this->router.insertIntoRoutingTable(rxPacket.sduid, rxPacket.sduid, this->getSignalScore()); //can only be one hop away
                }
                break;
            }
            case reservedTopic::ping:
                loginfo_ln("PING received. Notifying sketch first, then sending PONG.");
                // Call sketch BEFORE sendPong so the sketch can broadcast its
                // own GPS packet.  That GPS packet reaches the pinging duck
                // before the PONG, so the very first CDK:SEEN already includes
                // GPS coordinates.
                if (recvDataCallback) recvDataCallback(rxPacket);
                err = this->sendPong();
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("ERROR failed to send pong message. rc = %d",err);
                }
                break;
            case reservedTopic::cmd:
                loginfo_ln("Command received");

                err = this->broadcastPacket(rxPacket);
                
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                break;
            case topics::identity_announce:
                loginfo_ln("Identity announce received (broadcast)");
                this->learnPeerIdentity(rxPacket.sduid, rxPacket.data);
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay identity_announce. rc = %d",err);
                }
                break;
            case topics::group_broadcast: {
                // Relay the ciphertext as-received first -- downstream Ducks
                // need the original bytes to decrypt it themselves, even if
                // this Duck can't (e.g. no group key configured, or a
                // different deployment's key).
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay group_broadcast. rc = %d", err);
                }
                std::optional<std::pair<uint8_t, std::vector<uint8_t>>> decrypted = this->tryDecryptGroupData(rxPacket);
                if (decrypted.has_value()) {
                    loginfo_ln("group_broadcast decrypted OK (%d bytes), delivering to sketch.", (int)decrypted->second.size());
                    rxPacket.topic = decrypted->first;
                    rxPacket.data = decrypted->second;
                    rxPacket.wasAuthenticated = true;
                    if (recvDataCallback) {
                        recvDataCallback(rxPacket);
                    }
                }
                break;
            }
            default:
                err = this->broadcastPacket(rxPacket);
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                // Also deliver to the sketch — broadcast data (e.g. emergency
                // broadcast topic 24) must reach handleDuckData on this duck.
                if (recvDataCallback) {
                    recvDataCallback(rxPacket);
                }
        }
    }

    void ifNotBroadcast(CdpPacket rxPacket, int err, bool relay = false) {

        switch(rxPacket.topic) {
            case reservedTopic::rreq: {
                RouteJSON rreqDoc = RouteJSON(rxPacket.data);
                //route requests are just forwarded so we can use the sduid as the origin
                std::optional<Duid> last = rreqDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                if(!relay) {
                    loginfo_ln("handleReceivedPacket: Sending RREP");
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.convertReqToRep());
                    this->sendRouteResponse(lastInPath, rreqDoc.asString());
                } else {
                    rxPacket.data = duckutils::stringToByteVector(rreqDoc.addToPath(this->duid)); //why is this different from stringToArray
                    err = this->forwardPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay RREQ. rc = %d",err);
                    } else {
                        loginfo_ln("handleReceivedPacket: RREQ packet RELAY DONE");
                    }
                }
            }
            break;
          
            case reservedTopic::rrep: {
                //we still need to recieve rreps in case of ttl expiry
                RouteJSON rrepDoc = RouteJSON(rxPacket.data);
                std::optional<Duid> last = rrepDoc.getlastInPath();
                Duid lastInPath = last.has_value() ? last.value() : rxPacket.sduid;
                loginfo_ln("Received Route Response from DUID: %s", rxPacket.sduid.data(), rxPacket.sduid.size());

                std::optional<Duid> nextHop = this->router.getBestNextHop(rrepDoc.getDestination());
                if((rrepDoc.getDestination() != this->duid) && (nextHop.has_value()) && (nextHop.value() !=  rxPacket.sduid)){
                    rrepDoc.popFromPath();
                    rrepDoc.addToPath(this->duid);
                    //route responses need a way to keep tray of who relayed the packet, but a response needs to be directed and not broadly relayed
                    this->sendRouteResponse(rrepDoc.getDestination(), rrepDoc.asString()); //so here the "relaying" duck is known from sduid
                    this->router.insertIntoRoutingTable(rxPacket.sduid, lastInPath, this->getSignalScore());
                } else {
                    //destination = sender of the rrep -> the last hop to current duck
                    this->router.insertIntoRoutingTable(rrepDoc.getOrigin(), lastInPath, this->getSignalScore());
                }
            }
                break;
            case reservedTopic::ping:
                loginfo_ln("PING received. Notifying sketch first, then sending PONG.");
                // Call sketch BEFORE sendPong so the sketch can broadcast its
                // own GPS packet.  That GPS packet reaches the pinging duck
                // before the PONG, so the very first CDK:SEEN already includes
                // GPS coordinates.
                if (recvDataCallback) recvDataCallback(rxPacket);
                err = this->sendPong();
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("ERROR failed to send pong message. rc = %d",err);
                }
                break;
            case reservedTopic::pong:
                loginfo_ln("PONG received from nearby duck.");
                // Deliver to sketch so it can emit CDK:SEEN for peer discovery
                // when the app triggers a CDK:SCAN → duck.sendPing() sweep.
                if (recvDataCallback) recvDataCallback(rxPacket);
                break;
            case reservedTopic::cmd:
                loginfo_ln("Command received");

                err = this->broadcastPacket(rxPacket);
                
                if (err != DUCK_ERR_NONE) {
                    logerr_ln("====> ERROR handleReceivedPacket failed to relay. rc = %d",err);
                } else {
                    loginfo_ln("handleReceivedPacket: packet RELAY DONE");
                }
                break;
            case topics::encrypted_cmd: {
                if (relay) {
                    // Not addressed to us -- blind relay only. We can't
                    // decrypt traffic meant for a different Duck's identity,
                    // and must never try (session key derivation uses OUR
                    // own private key).
                    err = this->forwardPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay encrypted_cmd. rc = %d", err);
                    }
                    break;
                }
                if (!opendmsconfig::isConfigured()) {
                    logerr_ln("encrypted_cmd received but OpenDMS static public key is not configured, dropping.");
                    break;
                }
                if (rxPacket.data.size() < duckcrypto::NONCE_LENGTH + duckcrypto::TAG_LENGTH) {
                    logerr_ln("encrypted_cmd received but data too short (%d bytes), dropping.", (int)rxPacket.data.size());
                    break;
                }
                size_t ciphertextLen = rxPacket.data.size() - duckcrypto::NONCE_LENGTH - duckcrypto::TAG_LENGTH;
                const uint8_t* nonce = rxPacket.data.data();
                const uint8_t* ciphertext = rxPacket.data.data() + duckcrypto::NONCE_LENGTH;
                const uint8_t* tag = rxPacket.data.data() + duckcrypto::NONCE_LENGTH + ciphertextLen;
                std::vector<uint8_t> plaintext(ciphertextLen);
                // encrypted_cmd is by definition always sent BY OpenDMS --
                // only OpenDMS holds the matching static keypair that makes
                // decryptFromPeer(OPENDMS_STATIC_PUBLIC_KEY, ...) succeed --
                // so bind the AAD to the fixed PAPADUCK_DUID placeholder
                // rather than rxPacket.sduid (the transient real DUID of
                // whichever hub happened to relay this packet on-air, which
                // OpenDMS cannot predict and which has no bearing on the
                // security semantics). This mirrors sealed_uplink's already-
                // established convention of using PAPADUCK_DUID wherever
                // OpenDMS -- which has no DUID of its own -- needs to be
                // referenced as a packet's logical sender or recipient.
                std::array<uint8_t, Duck<WifiCapability, RadioType>::HEADER_AAD_LENGTH> aad = this->buildHeaderAad(PAPADUCK_DUID, rxPacket.dduid, rxPacket.topic);
                int rc = duckcrypto::decryptFromPeer(opendmsconfig::OPENDMS_STATIC_PUBLIC_KEY,
                                                      nonce, aad.data(), aad.size(),
                                                      ciphertext, ciphertextLen, tag,
                                                      plaintext.data());
                if (rc != DUCK_ERR_NONE) {
                    logerr_ln("encrypted_cmd decrypt failed rc = %d, dropping (auth failed or corrupt).", rc);
                    securityevents::recordEncryptedCmdRejected();
                    if (++consecutiveEncryptedCmdFailures_ >= ENCRYPTED_CMD_FAILURE_REANNOUNCE_THRESHOLD) {
                        logerr_ln("encrypted_cmd: %u consecutive decrypt failures, re-announcing identity in case of key drift.",
                                  (unsigned)consecutiveEncryptedCmdFailures_);
                        this->announceIdentity();
                        consecutiveEncryptedCmdFailures_ = 0;
                    }
                    break;
                }
                consecutiveEncryptedCmdFailures_ = 0;
                loginfo_ln("encrypted_cmd decrypted OK (%d bytes), delivering to sketch.", (int)plaintext.size());
                rxPacket.data = plaintext;
                rxPacket.wasAuthenticated = true;
                if (recvDataCallback) {
                    recvDataCallback(rxPacket);
                }
                break;
            }
            case topics::identity_announce:
                // Directed identity_announce (rare -- usually broadcast, see
                // ifBroadcast above). Learn it regardless of whether it was
                // addressed to us; relay onward if we're not the target.
                this->learnPeerIdentity(rxPacket.sduid, rxPacket.data);
                if (relay) {
                    err = this->forwardPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay identity_announce. rc = %d", err);
                    }
                } else if (this->isMamaLinkEncryptionEnabled() && repliedIdentityTo_.insert(rxPacket.sduid).second) {
                    // Addressed directly to us: this peer wants to talk to
                    // us and just gave us its key -- reply with ours so it
                    // can decrypt/authenticate what we send back, instead of
                    // waiting on its own periodic/boot broadcast to reach us
                    // independently. See repliedIdentityTo_'s doc comment.
                    // MTALK encryption is permanent (isMamaLinkEncryptionEnabled()
                    // always true), so this reply always fires regardless of
                    // the operator's uplink-encryption preference.
                    this->announceIdentity(rxPacket.sduid);
                }
                break;
            case topics::encrypted_data: {
                if (relay) {
                    // Not addressed to us -- blind relay only, same reasoning
                    // as encrypted_cmd: session key derivation uses OUR own
                    // private key, so we can never decrypt traffic meant for
                    // a different Duck's identity.
                    err = this->forwardPacket(rxPacket);
                    if (err != DUCK_ERR_NONE) {
                        logerr_ln("====> ERROR handleReceivedPacket failed to relay encrypted_data. rc = %d", err);
                    }
                    break;
                }
                std::optional<std::pair<uint8_t, std::vector<uint8_t>>> decrypted = this->tryDecryptEncryptedData(rxPacket);
                if (!decrypted.has_value()) {
                    // tryDecryptEncryptedData already logged the specific reason.
                    break;
                }
                loginfo_ln("encrypted_data decrypted OK (%d bytes), delivering to sketch.", (int)decrypted->second.size());
                rxPacket.topic = decrypted->first;
                rxPacket.data = decrypted->second;
                rxPacket.wasAuthenticated = true;
                if (recvDataCallback) {
                    recvDataCallback(rxPacket);
                }
                break;
            }
            default:
                loginfo_ln("ifNotBroadcast: default topic=%d relay=%d cbSet=%d", (int)rxPacket.topic, (int)relay, (recvDataCallback != nullptr ? 1 : 0));
                if(relay){
                    this->forwardPacket(rxPacket);
                    // Also notify the sketch for relay packets so it can emit CDK:SEEN
                    // frames for all overheard ducks (peer discovery in the mobile app).
                    if (recvDataCallback) recvDataCallback(rxPacket);
                } else if (recvDataCallback) {
                    // Packet is directly addressed to this duck — deliver to sketch
                    recvDataCallback(rxPacket);
                } else {
                    loginfo_ln("ifNotBroadcast: recvDataCallback is NULL, packet dropped!");
                }
        }
    }

};

#endif