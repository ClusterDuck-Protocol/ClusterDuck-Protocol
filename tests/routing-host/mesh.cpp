#include <cassert>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <tuple>
#include "CdpPacket.h"

class FakeRadio {
public:
    std::deque<std::vector<uint8_t>> inbox;
    inline static std::function<void(FakeRadio*, const std::vector<uint8_t>&)> deliver;
    uint32_t done = 0;
    static void onInterrupt() {}
    inline static const LoRaConfigParams defaultRadioParams{915, 22, 125, 7, 0, onInterrupt};
    // TX power range, like RadioLib's checkOutputPower(). By default an SX1276
    // on PA_BOOST: 2 to 17 or 20 dBm. With sx1262 set: -9 to 22 dBm. Setup
    // fails on any other value, as on the real radio. With looseCheck set,
    // checkTxPower() passes all of -9 to 22 dBm but setup still fails: a check
    // that disagrees with setup. With fault set, setup always fails.
    bool sx1262 = false, looseCheck = false, fault = false;
    int setupPower = 0;
    unsigned setups = 0;
    bool takes(int power) const {
        return sx1262 ? power >= -9 && power <= 22 : (power >= 2 && power <= 17) || power == 20;
    }
    int checkTxPower(int power) const {
        const bool ok = looseCheck ? power >= -9 && power <= 22 : takes(power);
        return ok ? DUCK_ERR_NONE : DUCK_ERR_INVALID_ARGUMENT;
    }
    int setupRadio(const LoRaConfigParams& config) {
        setupPower = config.txPower;
        ++setups;
        if (fault) return DUCKLORA_ERR_BEGIN;
        return takes(config.txPower) ? DUCK_ERR_NONE : DUCKLORA_ERR_SETUP;
    }
    void serviceInterruptFlags() {}
    bool getReceiveFlag() { return !inbox.empty(); }
    std::optional<std::vector<uint8_t>> readReceivedData() {
        if (inbox.empty()) return std::nullopt;
        auto data = inbox.front(); inbox.pop_front(); return data;
    }
    int sendData(std::vector<uint8_t> data) { ++done; deliver(this, data); return 0; }
    int startReceive() { return 0; }
    uint32_t getTxDoneCount() const { return done; }
    uint32_t getTxTimeoutCount() const { return 0; }
    float getRSSI() { return -50; }
    float getSNR() { return 12; }
    bool transmitting = false;
    unsigned sleeps = 0;
    bool isBusy() const { return transmitting || !inbox.empty(); }
    int sleep() { ++sleeps; return 0; }
};

#include "Ducks/MamaDuck.h"
#include "Ducks/PapaDuck.h"
#include "Ducks/DuckLink.h"

// Ducks read and set the wall clock through duckclock. Setting the real clock
// needs root, so the host tests keep a fake one.
namespace duckclock {
time_t hostNow = 0;
unsigned hostSets = 0;
time_t now() { return hostNow; }
int set(uint32_t epoch) { hostNow = epoch; ++hostSets; return 0; }
}

Duid id(const std::string& name) { assert(name.size() == 8); Duid value{}; std::copy(name.begin(), name.end(), value.begin()); return value; }
using Mama = MamaDuck<DuckWifiNone, FakeRadio>;
using Papa = PapaDuck<DuckWifiNone, FakeRadio>;
using Link = DuckLink<DuckWifiNone, FakeRadio>;
template<class Base> struct Node : Base {
    explicit Node(std::string name) : Base(name) { this->onReceiveDuckData(receive); }
    FakeRadio& radio() { return this->duckRadio; }
    DuckRouter& routes() { return this->router; }
    inline static std::vector<CdpPacket> received;
    static void receive(CdpPacket packet) { received.push_back(packet); }
};

struct Mesh {
    Node<Mama> a{"NODE0001"}, b{"NODE0002"}, c{"NODE0003"};
    Node<Papa> p{"GATE0001"};
    std::vector<FakeRadio*> radios{&a.radio(), &b.radio(), &c.radio(), &p.radio()};
    bool links[4][4]{};
    struct Sent { size_t sender; CdpPacket packet; };
    std::vector<Sent> sent;
    Mesh(bool complete) {
        hostMillis = 0;
        Node<Mama>::received.clear(); Node<Papa>::received.clear();
        for (size_t i = 0; i < 4; ++i) for (size_t j = 0; j < 4; ++j)
            links[i][j] = i != j && (complete || i + 1 == j || j + 1 == i);
        FakeRadio::deliver = [this](FakeRadio* from, const std::vector<uint8_t>& bytes) {
            const size_t sender = std::find(radios.begin(), radios.end(), from) - radios.begin();
            CdpPacket packet(bytes);
            sent.push_back({sender, packet});
            if (sender == radios.size()) return; // a node outside the mesh: nobody hears it
            for (size_t j = 0; j < radios.size(); ++j) if (links[sender][j]) radios[j]->inbox.push_back(bytes);
        };
        assert(a.setupWithDefaults() == 0); assert(b.setupWithDefaults() == 0);
        assert(c.setupWithDefaults() == 0); assert(p.setupWithDefaults() == 0);
    }
    void until(uint32_t end) {
        for (; hostMillis < end; hostMillis += 100) { a.run(); b.run(); c.run(); p.run(); }
    }
    size_t count(uint8_t topic, size_t from = 0) const {
        return std::count_if(sent.begin() + from, sent.end(), [topic](const auto& item) { return item.packet.topic == topic; });
    }
};

JsonDocument parse(const std::string& text) { JsonDocument doc; assert(!deserializeJson(doc, text)); return doc; }

// A packet as it arrives over the air, from GATE0001.
std::vector<uint8_t> packetBytes(uint8_t topic, const std::string& json, Muid muid,
                                 Duid target = BROADCAST_DUID, DuckType sender = DuckType::PAPA) {
    CdpPacket packet(target, topic, duckutils::stringToByteVector(json), id("GATE0001"), sender);
    packet.muid = muid;
    assert(packet.prepareForSending() == 0);
    return packet.asBytes();
}

void timeSyncChecks() {
    constexpr bool applies = CDPCFG_REMOTE_TIME_SYNC;
    constexpr uint32_t epoch = 1790000000; // 2026-09-21
    Mesh mesh(true);
    mesh.until(120000);
    Node<Link> link{"LINK0001"}; // outside the mesh: fed directly, heard by nobody
    assert(link.setupWithDefaults() == 0);

    // The duplicate filter can report a false positive, and a duck drops that
    // packet as a repeat. Give each test packet a MUID no duck has seen yet.
    uint8_t serial = 0;
    auto freshMuid = [&] {
        for (;;) {
            const Muid muid{'T','E','S',serial++};
            bool seen = false;
            for (DuckRouter* router : {&mesh.a.routes(), &mesh.b.routes(), &mesh.c.routes(),
                                       &mesh.p.routes(), &link.routes()})
                if (router->getFilter().bloom_check(const_cast<uint8_t*>(muid.data()), MUID_LENGTH)) seen = true;
            if (!seen) return muid;
        }
    };
    auto resetClock = [] { duckclock::hostNow = 0; duckclock::hostSets = 0; };
    // Hand a packet to one duck alone and let it handle it.
    auto feed = [](auto& node, const std::vector<uint8_t>& bytes) {
        node.radio().inbox.push_back(bytes);
        for (int i = 0; i < 3; ++i) node.run();
    };
    auto settle = [&] { mesh.until(hostMillis + 30000); };
    auto relayedByA = [&](const Muid& muid) {
        return std::count_if(mesh.sent.begin(), mesh.sent.end(), [&muid](const auto& item) {
            return item.sender == 0 && item.packet.muid == muid;
        });
    };

    resetClock();
    assert(!mesh.a.hasValidTime());
    duckclock::hostNow = CDPCFG_TIME_MIN_EPOCH - 1; assert(!mesh.a.hasValidTime());
    duckclock::hostNow = CDPCFG_TIME_MIN_EPOCH; assert(mesh.a.hasValidTime());

    // PapaDuck broadcast: each MamaDuck hears three copies (the PapaDuck's and
    // two relays), applies the first and relays once, flag or no flag.
    resetClock();
    assert(mesh.p.sendTimeSync(CDPCFG_TIME_MIN_EPOCH - 1) == DUCK_ERR_INVALID_ARGUMENT);
    const size_t start = mesh.sent.size();
    assert(mesh.p.sendTimeSync(epoch) == DUCK_ERR_NONE);
    settle();
    assert(duckclock::hostSets == (applies ? 3u : 0u));
    assert(mesh.a.hasValidTime() == applies);
    assert(mesh.count(topics::cmd_time, start) == 4);

    // A later copy of the same message is ignored, even after the clock moved on.
    resetClock();
    const Muid repeated = freshMuid();
    const auto copy = packetBytes(topics::cmd_time, "{\"epoch\":1790000100}", repeated);
    feed(mesh.a, copy);
    assert(duckclock::hostSets == (applies ? 1u : 0u));
    if (applies) assert(duckclock::hostNow == 1790000100);
    duckclock::hostNow = 1790000999;
    feed(mesh.a, copy);
    assert(duckclock::hostSets == (applies ? 1u : 0u) && duckclock::hostNow == 1790000999);
    settle();
    assert(relayedByA(repeated) == 1);

    // Rejected, but still relayed: a MamaDuck sender, an epoch before the
    // minimum, no epoch, a string or negative epoch, and a JSON parse error.
    const std::vector<std::pair<std::string, DuckType>> rejected = {
        {"{\"epoch\":1790000000}", DuckType::MAMA},
        {"{\"epoch\":" + std::to_string(CDPCFG_TIME_MIN_EPOCH - 1) + "}", DuckType::PAPA},
        {"{\"time\":1790000000}", DuckType::PAPA},
        {"{\"epoch\":\"1790000000\"}", DuckType::PAPA},
        {"{\"epoch\":-1}", DuckType::PAPA},
        {"{\"epoch\":17900", DuckType::PAPA},
    };
    for (const auto& [json, sender] : rejected) {
        resetClock();
        const Muid muid = freshMuid();
        feed(mesh.a, packetBytes(topics::cmd_time, json, muid, BROADCAST_DUID, sender));
        assert(duckclock::hostSets == 0);
        settle();
        assert(relayedByA(muid) == 1);
    }

    // Addressed to NODE0001: applied there and not relayed.
    resetClock();
    const Muid toA = freshMuid();
    feed(mesh.a, packetBytes(topics::cmd_time, "{\"epoch\":1790000200}", toA, id("NODE0001")));
    assert(duckclock::hostSets == (applies ? 1u : 0u));
    settle();
    assert(relayedByA(toA) == 0);
    // Addressed to NODE0002: NODE0001 passes it on, and only NODE0002 applies it.
    resetClock();
    const Muid toB = freshMuid();
    feed(mesh.a, packetBytes(topics::cmd_time, "{\"epoch\":1790000300}", toB, id("NODE0002")));
    assert(duckclock::hostSets == 0);
    settle();
    assert(relayedByA(toB) == 1);
    assert(duckclock::hostSets == (applies ? 1u : 0u));

    // TX power is a separate flag: stored only with CDPCFG_REMOTE_TX_POWER,
    // relayed either way, and a command for another duck is passed on.
    const Muid power = freshMuid(), powerToB = freshMuid();
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":20}", power));
    assert(mesh.a.eeprom_preferences.getInt("txPwr", -1) == (CDPCFG_REMOTE_TX_POWER ? 20 : -1));
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":17}", powerToB, id("NODE0002")));
    assert(mesh.a.eeprom_preferences.getInt("txPwr", -1) == (CDPCFG_REMOTE_TX_POWER ? 20 : -1));
    settle();
    assert(relayedByA(power) == 1 && relayedByA(powerToB) == 1);

    // A DuckLink applies a broadcast or a command addressed to it, once each.
    resetClock();
    const auto linkCopy = packetBytes(topics::cmd_time, "{\"epoch\":1790000400}", freshMuid());
    feed(link, linkCopy);
    feed(link, linkCopy);
    assert(duckclock::hostSets == (applies ? 1u : 0u));
    feed(link, packetBytes(topics::cmd_time, "{\"epoch\":1790000500}", freshMuid(), id("LINK0001")));
    feed(link, packetBytes(topics::cmd_time, "{\"epoch\":1790000600}", freshMuid(), id("NODE0002")));
    assert(duckclock::hostSets == (applies ? 2u : 0u));
    if (applies) assert(duckclock::hostNow == 1790000500);
    std::cout << "time sync: " << (applies ? "applied once per message, from a PapaDuck only"
                                            : "not applied (flag off)")
              << "; MamaDucks relay every cmd_time\n";
}

// cmd_tx allows 14 to 22 dBm, but an SX1276 only takes 14 to 17 or 20. A
// stored 18, 19, 21 or 22 used to make radio setup fail at every boot.
void txPowerChecks() {
    constexpr bool applies = CDPCFG_REMOTE_TX_POWER;
    Mesh mesh(true);   // SX1276-like radios
    mesh.until(120000);
    // Outside the mesh: fed directly, heard by nobody.
    Node<Link> link{"LINK0001"};
    Node<Mama> wide{"NODE0005"};
    wide.radio().sx1262 = true;
    assert(link.setupWithDefaults() == 0 && wide.setupWithDefaults() == 0);

    uint8_t serial = 0;
    auto freshMuid = [&] {
        for (;;) {
            const Muid muid{'P','W','R',serial++};
            bool seen = false;
            for (DuckRouter* router : {&mesh.a.routes(), &mesh.b.routes(), &mesh.c.routes(),
                                       &mesh.p.routes(), &link.routes(), &wide.routes()})
                if (router->getFilter().bloom_check(const_cast<uint8_t*>(muid.data()), MUID_LENGTH)) seen = true;
            if (!seen) return muid;
        }
    };
    auto feed = [](auto& node, const std::vector<uint8_t>& bytes) {
        node.radio().inbox.push_back(bytes);
        for (int i = 0; i < 3; ++i) node.run();
    };
    auto stored = [](auto& node) { return node.eeprom_preferences.getInt("txPwr", -1); };
    auto relayedByA = [&](const Muid& muid) {
        return std::count_if(mesh.sent.begin(), mesh.sent.end(), [&muid](const auto& item) {
            return item.sender == 0 && item.packet.muid == muid;
        });
    };
    // Report every failed check, then fail once at the end.
    unsigned failures = 0;
    auto check = [&failures](bool ok, const char* what) {
        if (!ok) { ++failures; std::cout << "FAIL tx power: " << what << '\n'; }
    };

    // Commands. A value the radio can't use is not stored, but a MamaDuck still
    // relays it: other ducks may have a radio that takes it.
    const Muid p18 = freshMuid(), p20 = freshMuid(), p19 = freshMuid(), p17 = freshMuid(),
               p23 = freshMuid(), p13 = freshMuid();
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":18}", p18));
    check(stored(mesh.a) == -1, "MamaDuck stored 18 dBm, which its SX1276 rejects");
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":20}", p20));
    check(stored(mesh.a) == (applies ? 20 : -1), "MamaDuck didn't store 20 dBm");
    int before = stored(mesh.a);
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":19}", p19, id("NODE0001")));
    check(stored(mesh.a) == before, "MamaDuck stored 19 dBm, addressed to it, which its SX1276 rejects");
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":17}", p17, id("NODE0001")));
    check(stored(mesh.a) == (applies ? 17 : -1), "MamaDuck didn't store 17 dBm addressed to it");
    before = stored(mesh.a);
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":23}", p23));
    check(stored(mesh.a) == before, "MamaDuck stored 23 dBm, outside 14 to 22");
    // The radio takes 13 dBm, but the command doesn't allow it.
    feed(mesh.a, packetBytes(topics::cmd_tx, "{\"txPwr\":13}", p13));
    check(stored(mesh.a) == before, "MamaDuck stored 13 dBm, outside 14 to 22");
    check(mesh.a.radio().setupPower == CDPCFG_RF_LORA_TXPOW, "TX power changed before the next boot");
    mesh.until(hostMillis + 30000);
    check(relayedByA(p18) == 1 && relayedByA(p20) == 1, "a broadcast cmd_tx was not relayed once");
    check(relayedByA(p19) == 0 && relayedByA(p17) == 0, "a cmd_tx addressed to the MamaDuck was relayed");
    check(relayedByA(p23) == (applies ? 0 : 1) && relayedByA(p13) == (applies ? 0 : 1),
          "a cmd_tx outside 14 to 22 dBm was relayed by a duck that acts on cmd_tx");
    feed(link, packetBytes(topics::cmd_tx, "{\"txPwr\":21}", freshMuid()));
    check(stored(link) == -1, "DuckLink stored 21 dBm, which its SX1276 rejects");
    feed(link, packetBytes(topics::cmd_tx, "{\"txPwr\":17}", freshMuid()));
    check(stored(link) == (applies ? 17 : -1), "DuckLink didn't store 17 dBm");
    feed(link, packetBytes(topics::cmd_tx, "{\"txPwr\":13}", freshMuid()));
    check(stored(link) == (applies ? 17 : -1), "DuckLink stored 13 dBm, outside 14 to 22");
    // A DuckLink ignores a cmd_tx addressed to it, as before.
    feed(link, packetBytes(topics::cmd_tx, "{\"txPwr\":20}", freshMuid(), id("LINK0001")));
    check(stored(link) == (applies ? 17 : -1), "DuckLink stored a cmd_tx addressed to it");
    feed(wide, packetBytes(topics::cmd_tx, "{\"txPwr\":22}", freshMuid()));
    check(stored(wide) == (applies ? 22 : -1), "SX1262 MamaDuck didn't store 22 dBm");
    check(link.radio().setupPower == CDPCFG_RF_LORA_TXPOW && wide.radio().setupPower == CDPCFG_RF_LORA_TXPOW,
          "TX power changed before the next boot (DuckLink or SX1262 MamaDuck)");

    // Boot with a stored value. One outside 14 to 22 dBm, or one the radio
    // can't use, is removed and the default used. If setup still fails with a
    // stored value other than the default (looseCheck), setup runs once more
    // with the default. A radio fault leaves the stored value alone. With the
    // setting off the stored value is ignored and left alone, as before. Every
    // duck type boots through Duck::setupWithDefaults().
    constexpr int dflt = CDPCFG_RF_LORA_TXPOW;
    struct Boot {
        int value; bool sx1262, looseCheck, fault; int power; bool kept; unsigned setups; const char* what;
    };
    const Boot boots[] = {
        {18, false, false, false, dflt, !applies, 1, "stored 18 dBm on an SX1276"},
        {22, false, false, false, dflt, !applies, 1, "stored 22 dBm on an SX1276"},
        {30, false, false, false, dflt, !applies, 1, "stored 30 dBm"},
        {10, false, false, false, dflt, !applies, 1, "stored 10 dBm on an SX1276 (the radio takes it)"},
        {-5, true, false, false, dflt, !applies, 1, "stored -5 dBm on an SX1262 (the radio takes it)"},
        {20, false, false, false, applies ? 20 : dflt, true, 1, "stored 20 dBm on an SX1276"},
        {22, true, false, false, applies ? 22 : dflt, true, 1, "stored 22 dBm on an SX1262"},
        {18, false, true, false, dflt, !applies, applies ? 2u : 1u, "stored 18 dBm, check passes, setup fails"},
        {20, false, false, true, dflt, true, applies ? 2u : 1u, "stored 20 dBm, radio fault"},
        {dflt, false, false, true, dflt, true, 1, "stored default TX power, radio fault"},
    };
    auto bootWith = [&check](auto& node, const Boot& boot, const std::string& what) {
        node.radio().sx1262 = boot.sx1262;
        node.radio().looseCheck = boot.looseCheck;
        node.radio().fault = boot.fault;
        node.eeprom_preferences.putInt("txPwr", boot.value);
        const int rc = node.setupWithDefaults();
        if (boot.fault) check(rc != DUCK_ERR_NONE, (what + ": setup didn't fail").c_str());
        else check(rc == DUCK_ERR_NONE, (what + ": setup failed").c_str());
        check(node.radio().setupPower == boot.power, (what + ": wrong TX power at setup").c_str());
        check(node.radio().setups == boot.setups, (what + ": wrong number of setup attempts").c_str());
        check(node.eeprom_preferences.isKey("txPwr") == boot.kept,
              (what + (boot.kept ? ": removed" : ": not removed")).c_str());
    };
    for (const Boot& boot : boots) {
        Node<Mama> mama{"NODE0006"};
        Node<Link> leaf{"LINK0002"};
        Node<Papa> papa{"GATE0002"};
        bootWith(mama, boot, std::string(boot.what) + ", MamaDuck");
        bootWith(leaf, boot, std::string(boot.what) + ", DuckLink");
        bootWith(papa, boot, std::string(boot.what) + ", PapaDuck");
    }
    assert(failures == 0);
    std::cout << "tx power: " << (applies ? "stored only if this duck's radio takes it; a bad stored value is "
                                            "removed at boot"
                                          : "not applied (flag off); stored values ignored")
              << "; MamaDucks relay cmd_tx for other ducks\n";
}

void sleepChecks() {
    Mesh mesh(true);
    mesh.until(120000);
    assert(mesh.a.txIdle());
    assert(mesh.a.sendData(topics::sensor, "{\"c\":3}") == 0);
    assert(!mesh.a.txIdle()); // queued, not sent yet
    const uint32_t deadline = hostMillis + 30000;
    while (!mesh.a.txIdle() && hostMillis < deadline) mesh.until(hostMillis + 100);
    assert(mesh.a.txIdle());
    assert(std::any_of(Node<Papa>::received.begin(), Node<Papa>::received.end(), [](const auto& packet) {
        return packet.topic == topics::sensor && packet.sduid == id("NODE0001");
    }));
    mesh.a.radio().transmitting = true;
    assert(!mesh.a.txIdle()); // radio mid-transmission
    mesh.a.radio().transmitting = false;
    mesh.a.radio().inbox.push_back(packetBytes(topics::sensor, "{}", {'S','L','P','1'}));
    assert(!mesh.a.txIdle()); // unread packet
    mesh.a.radio().inbox.clear();
    assert(mesh.a.prepareForSleep() == DUCK_ERR_NONE);
    assert(mesh.a.radio().sleeps == 1);
    assert(mesh.a.sendData(topics::sensor, "{\"c\":4}") == DUCK_ERR_NOT_SUPPORTED);
    std::cout << "sleep hooks: txIdle follows the queues and radio; prepareForSleep stops the duck\n";
}
void routerChecks() {
    hostMillis = 100;
    const SignalScore strong{0.95f, 0.75f, 0.85f}, weak{0.5f, 0.5f, 0.5f};
    DuckRouter router;
    router.insertIntoRoutingTable(id("NODE0001"), id("NODE0001"), strong);
    router.insertIntoRoutingTable(id("NODE0001"), id("NODE0002"), weak);
    auto doc = parse(*router.getEntriesFor(PAPADUCK_DUID, id("SELF0001")));
    assert(doc["n"].size() == 1); // indirect update did not erase direct measurement
    router.insertIntoRoutingTable(id("TARGET01"), id("NODE0001"), strong);
    hostMillis += CDPCFG_ROUTE_TTL_MS - 1;
    router.insertIntoRoutingTable(id("TARGET01"), id("NODE0002"), weak);
    ++hostMillis;
    assert(router.getBestNextHop(id("TARGET01")) == id("NODE0002"));
    assert(router.directNeighborCount() == 0);
    for (size_t count : {0, 1, 2, 3, 6, 7, 40}) {
        DuckRouter peers;
        for (size_t i = 0; i < count; ++i) {
            char peer[9]; snprintf(peer, sizeof(peer), "PEER%04u", unsigned(i));
            peers.insertIntoRoutingTable(id(peer), id(peer), strong);
            peers.insertIntoRoutingTable(id(peer), id(peer), strong); // same millisecond, no duplicate
        }
        const auto payload = *peers.getEntriesFor(PAPADUCK_DUID, id("SELF0001"));
        assert(payload.size() <= MAX_DATA_LENGTH);
        auto detail = parse(payload);
        if (count == 3) {
            std::cout << "SIGNAL_FIXTURE=" << payload << '\n';
            CdpPacket fixture(PAPADUCK_DUID, topics::sig, duckutils::stringToByteVector(payload), id("SELF0001"), DuckType::MAMA);
            fixture.muid = {'F','I','X','1'};
            assert(fixture.prepareForSending() == 0);
            auto bytes = fixture.asBytes();
            std::cout << "PACKET_FIXTURE=" << duckutils::convertToHex(bytes.data(), bytes.size()) << '\n';
        }
        assert(detail["o"].as<size_t>() == std::min(count, size_t(CDPCFG_MAX_DIRECT_NEIGHBORS)));
        assert(detail["i"].as<size_t>() == detail["n"].size());
        if (count <= 3) assert(detail["n"].size() == count);
        assert(detail["tr"].as<bool>() == (detail["o"].as<size_t>() > detail["n"].size()));
        std::cout << "neighbors=" << count << " included=" << detail["n"].size() << " bytes=" << payload.size() << '\n';
    }
    // Forward-only culling across >= four destinations and uint32 rollover.
    hostMillis = UINT32_MAX - 100;
    DuckRouter rollover;
    for (int i = 0; i < 5; ++i) {
        char peer[9]; snprintf(peer, sizeof(peer), "PEER%04u", unsigned(i));
        rollover.insertIntoRoutingTable(id(peer), id(peer), strong);
    }
    hostMillis = 200;
    assert(rollover.getBestNextHop(id("PEER0001")));
    hostMillis += CDPCFG_ROUTE_TTL_MS;
    rollover.cullRoutingTable();
    assert(rollover.directNeighborCount() == 0);
    // Length/type checks, duplicate path nodes, malformed and long payloads.
    for (std::string text : {"{}", "{\"origin\":\"bad\",\"destination\":\"NODE0001\",\"path\":[]}",
        "{\"origin\":\"NODE0001\",\"destination\":\"NODE0002\",\"path\":[\"NODE0001\",\"NODE0001\"]}"})
        assert(!RouteJSON::fromPacketData(duckutils::stringToByteVector(text)));
    RouteJSON broadcast(BROADCAST_DUID, id("NODE0001")); broadcast.addToPath(id("NODE0001"));
    assert(RouteJSON::fromPacketData(duckutils::stringToByteVector(broadcast.asString()))->getDestination() == BROADCAST_DUID);
    SizedQueue queue(1);
    CdpPacket packet(PAPADUCK_DUID, topics::health, {1}, id("NODE0001"), DuckType::MAMA);
    packet.hopCount = 0;
    packet.data.assign(228, 1); assert(packet.prepareForSending() == 0);
    packet.data.assign(229, 1); assert(packet.prepareForSending() == DUCKPACKET_ERR_SIZE_INVALID);
    packet.data.assign(1, 1);
    assert(queue.enqueue(packet)); assert(!queue.enqueue(packet)); assert(queue.dropped == 1);
    packet.data.assign(256, 1); assert(packet.prepareForSending() == DUCKPACKET_ERR_SIZE_INVALID);
    packet.data.assign(1, 1); packet.hopCount = MAX_HOPS + 1;
    assert(packet.prepareForSending() == DUCKPACKET_ERR_MAX_HOPS);
}
void toStringChecks() {
    // Printable ASCII stays as it is; any other byte becomes two hex digits.
    const Duid raw{'N','O','D','E', 0x0A, 0x00, 0xFF, '1'};
    assert(duckutils::toString(raw) == "NODE0A00FF1");
    assert(duckutils::toString(std::vector<uint8_t>(raw.begin(), raw.end())) == "NODE0A00FF1");
    assert(duckutils::toString(std::string("NODE0001")) == "NODE0001");
    assert(duckutils::toString(std::vector<char>{'\x80', 'A'}) == "80A");
    // The pointer overload feeds the "Rx packet:" log line: same escaping, and
    // a 0x00 byte must not cut the text short.
    std::array<uint8_t, 8> rawCopy{};
    std::copy(raw.begin(), raw.end(), rawCopy.begin());
    assert(duckutils::toString(rawCopy.data(), int(rawCopy.size())) == "NODE0A00FF1");
    // How that line starts for a broadcast: sender, destination, MUID, topic,
    // duck type and hop count.
    std::vector<uint8_t> header{'W','F','7','U','D','7','0','Z'};
    header.insert(header.end(), BROADCAST_DUID.begin(), BROADCAST_DUID.end());
    header.insert(header.end(), {'K','A','K','M', 0x06, 0x01, 0x00});
    assert(header.size() == DATA_CRC_POS);
    assert(duckutils::toString(header.data(), int(header.size())) == "WF7UD70ZFFFFFFFFFFFFFFFFKAKM060100");
    // The data CRC and the payload follow on the same line. A JSON payload is
    // printable, so it prints unchanged, even after a hop count of 0x00.
    std::vector<uint8_t> rxPacket = header;
    rxPacket.insert(rxPacket.end(), {0x12, 0x00, 0xAB, 'z'});
    const std::string payload = "{\"v\":3,\"boot\":1}";
    rxPacket.insert(rxPacket.end(), payload.begin(), payload.end());
    assert(duckutils::toString(rxPacket.data(), int(rxPacket.size())) ==
           "WF7UD70ZFFFFFFFFFFFFFFFFKAKM060100" "1200ABz" + payload);
    // The routing table and route JSON keep the raw eight bytes.
    assert(duckutils::duidToString(raw).size() == DUID_LENGTH);
    std::cout << "toString: printable bytes kept, others written as hex\n";
}

// The duplicate filter names this class a friend, so the tests can reach inside.
class BloomFilterTester {
public:
    static void setSeeds(BloomFilter& filter, uint32_t first, uint32_t second) {
        filter.Seeds[0] = int(first); filter.Seeds[1] = int(second);
    }
    static int active(const BloomFilter& filter) { return filter.activeFilter; }
    static bool empty(const BloomFilter& filter, int which) {
        const unsigned int* words = which == 1 ? filter.filter1 : filter.filter2;
        return std::all_of(words, words + filter.numSectors, [](unsigned int word) { return word == 0; });
    }
    static uint32_t hash(const std::string& data, uint32_t seed) {
        return BloomFilter::murmur3Hash(reinterpret_cast<const unsigned char*>(data.data()), int(seed), int(data.size()));
    }
    // The two bit positions a MUID sets.
    static std::pair<unsigned, unsigned> bits(BloomFilter& filter, Muid muid) {
        std::unique_ptr<unsigned int[]> results(new unsigned int[filter.numHashes]);
        filter.set_hash_results(muid.data(), MUID_LENGTH, results);
        return {results[0], results[1]};
    }
};

void bloomChecks() {
    // MurmurHash3_x86_32 reference values.
    assert(BloomFilterTester::hash("", 1) == 0x514E28B7);
    assert(BloomFilterTester::hash(std::string("\x21\x43\x65\x87", 4), 0x5082EDEE) == 0x2362F9DE);
    assert(BloomFilterTester::hash("Hello, world!", 0x9747B28C) == 0x24884CBA);

    // MUIDs as ducks make them: four characters from 0-9 and A-Z. Not from the
    // fake random(), which only ever gives 9^4 different MUIDs.
    std::mt19937 rng(528);
    auto newMuid = [&] {
        Muid muid;
        for (auto& c : muid) c = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rng() % 36];
        return muid;
    };
    auto add = [](BloomFilter& filter, Muid muid) { filter.bloom_add(muid.data(), MUID_LENGTH); };
    auto seen = [](BloomFilter& filter, Muid muid) { return filter.bloom_check(muid.data(), MUID_LENGTH) == 1; };

    // Every DEFAULT_MAX_MESSAGES the filter swaps halves and clears all words
    // of the half it reuses, not 312 / 32 of them. A MUID is remembered for at
    // least the next DEFAULT_MAX_MESSAGES.
    {
        BloomFilter filter;
        for (int swap = 1; swap <= 4; ++swap) {
            const Muid first = newMuid();
            add(filter, first);
            for (int i = 1; i < DEFAULT_MAX_MESSAGES; ++i) { add(filter, newMuid()); assert(seen(filter, first)); }
            assert(seen(filter, first));
            const int active = BloomFilterTester::active(filter);
            assert(active == (swap % 2 ? 2 : 1) && BloomFilterTester::empty(filter, active));
        }
    }
    // The two bits are independent: the distance between them takes many
    // values. With the seeded djb2 hash it was nearly constant.
    {
        BloomFilter filter;
        const unsigned size = filter.get_numSectors() * filter.get_bitsPerSector();
        std::set<unsigned> distances;
        for (int i = 0; i < 1000; ++i) {
            const auto [a, b] = BloomFilterTester::bits(filter, newMuid());
            distances.insert((b + size - a) % size);
        }
        assert(distances.size() > 800);
    }
    // Collisions depend on the seeds. With djb2, a filter holding only "05FM"
    // reported "CMDD" as seen for every seed pair.
    for (int i = 0; i < 100; ++i) {
        BloomFilter filter;
        BloomFilterTester::setSeeds(filter, rng(), rng());
        add(filter, {'0','5','F','M'});
        assert(!seen(filter, {'C','M','D','D'}));
    }
    // False positives over 200 swaps of new MUIDs, with the seeds a duck picks
    // and with fixed ones, including adjacent seeds. Two independent hashes give
    // about 0.05%; the djb2 filter gave 1.5%, and clearing 9 words per swap 60%.
    {
        unsigned checked = 0, falsePositives = 0;
        for (int round = 0; round < 4; ++round) {
            BloomFilter filter;
            if (round == 1) BloomFilterTester::setSeeds(filter, 12345, 12346);
            if (round > 1) BloomFilterTester::setSeeds(filter, rng(), rng());
            std::set<Muid> used;
            while (used.size() < 20000) {
                const Muid muid = newMuid();
                if (!used.insert(muid).second) continue;
                ++checked;
                if (seen(filter, muid)) ++falsePositives;
                add(filter, muid);
            }
        }
        std::cout << "duplicate filter: " << falsePositives << " false positives in " << checked
                  << " new MUIDs (" << 100.0 * falsePositives / checked << "%)\n";
        assert(falsePositives * 1000 < checked); // under 0.1%
    }
}

int main() {
    std::cout << "remote commands: time sync=" << CDPCFG_REMOTE_TIME_SYNC
              << " tx power=" << CDPCFG_REMOTE_TX_POWER << '\n';
    routerChecks();
    toStringChecks();
    {
        Mesh mesh(true);
        mesh.until(120000);
        size_t start = mesh.sent.size();
        mesh.until(720000);
        const auto requests = mesh.count(reservedTopic::rreq, start);
        const auto responses = mesh.count(reservedTopic::rrep, start);
        std::cout << "complete mesh / 10 quiet minutes: RREQ=" << requests << " RREP=" << responses << '\n';
        assert(requests <= 6 && responses <= 18);
        assert(mesh.a.routes().getBestNextHop(PAPADUCK_DUID) == id("GATE0001"));
        assert(mesh.a.routes().directNeighborCount() == 3);
        for (uint8_t page = 0; page < 4; ++page) {
            const std::string payload = mesh.a.routingDiagnostics(page);
            assert(!payload.empty() && payload.size() <= MAX_DATA_LENGTH);
            auto doc = parse(payload); assert(doc["p"] == page);
            // Prove the page budget even at maximum 32-bit counter values.
            for (auto pair : doc.as<JsonObject>()) {
                std::string key = pair.key().c_str();
                if (key == "v" || key == "p") continue;
                if (pair.value().is<JsonArray>()) for (auto value : pair.value().as<JsonArray>()) value.set(UINT32_MAX);
                else if (pair.value().is<uint32_t>()) pair.value().set(UINT32_MAX);
            }
            assert(measureJson(doc) <= MAX_DATA_LENGTH);
        }
        CdpPacket duplicate(PAPADUCK_DUID, topics::sensor, {1}, id("NODE0001"), DuckType::MAMA);
        duplicate.muid = {'D','U','P','1'}; assert(duplicate.prepareForSending() == 0);
        const auto receiptCount = Node<Papa>::received.size();
        mesh.p.radio().inbox.push_back(duplicate.asBytes());
        mesh.p.radio().inbox.push_back(duplicate.asBytes());
        mesh.p.run(); mesh.p.run();
        assert(Node<Papa>::received.size() == receiptCount + 2); // MQTT duplicate policy preserved
        // A full data queue is an explicit failure, not false success.
        unsigned accepted = 0;
        for (unsigned i = 0; i < 20; ++i)
            if (mesh.a.sendData(topics::sensor, "{\"c\":1}") == 0) ++accepted;
        assert(accepted == CDPCFG_MAX_QUEUE_SIZE);
    }
    {
        Mesh mesh(false);
        mesh.until(120000);
        assert(mesh.a.sendData(topics::sensor, "{\"c\":1}") == 0);
        mesh.until(210000);
        assert(mesh.a.routes().getBestNextHop(PAPADUCK_DUID) == id("NODE0002"));
        assert(mesh.a.routes().directNeighborCount() == 1);
        assert(std::any_of(Node<Papa>::received.begin(), Node<Papa>::received.end(), [](const auto& packet) {
            return packet.topic == topics::sensor && packet.sduid == id("NODE0001");
        }));
        std::map<std::tuple<Duid,Muid>, std::set<int>> replies;
        for (const auto& item : mesh.sent) if (item.packet.topic == reservedTopic::rrep)
            replies[{item.packet.sduid, item.packet.muid}].insert(item.packet.hopCount);
        bool threeHops = false;
        for (const auto& [key, hops] : replies) if (hops == std::set<int>{0,1,2}) threeHops = true;
        assert(threeHops); // a reply traversed all three links without a new ID
        std::cout << "three-link discovery: gateway route converged; reply identity preserved\n";
        // Gateway loss, quiet late arrival and route expiry/recovery.
        mesh.links[2][3] = mesh.links[3][2] = false;
        mesh.until(2400000);
        assert(!mesh.a.routes().getBestNextHop(PAPADUCK_DUID));
        assert(mesh.a.sendData(topics::health, "{\"x\":1}") == 0); // standalone still accepts
        mesh.links[2][3] = mesh.links[3][2] = true;
        mesh.until(3100000);
        assert(mesh.a.routes().getBestNextHop(PAPADUCK_DUID));
        std::cout << "gateway loss/expiry/late return: recovered without a membership handshake\n";
        // A deep sleep reboot loses ordinary RAM, routing and duplicate history.
        mesh.b.~Node<Mama>(); new (&mesh.b) Node<Mama>("NODE0002");
        assert(mesh.b.setupWithDefaults() == 0);
        assert(mesh.b.sendData(topics::health, "{\"wake\":1}") == 0);
        mesh.until(3200000);
        assert(mesh.b.routes().getBestNextHop(PAPADUCK_DUID));
        std::cout << "relay reboot/wake: accepts immediately and rediscovers gateway\n";
    }
    {
        Mesh mesh(true);
        assert(mesh.a.sendData(topics::health, "{\"early\":1}") == 0);
        mesh.a.run();
        auto before = parse(mesh.a.routingDiagnostics(1));
        assert(before["q"][2].as<unsigned>() == 1);
        hostMillis = 2000; mesh.a.run();
        auto after = parse(mesh.a.routingDiagnostics(1));
        assert(after["q"][2].as<unsigned>() >= 1);
        assert(mesh.sent.empty()); // request was not dequeued before pacing allowed TX
        mesh.until(120000);
        assert(std::any_of(Node<Papa>::received.begin(), Node<Papa>::received.end(), [](const auto& packet) {
            return packet.topic == topics::health && packet.sduid == id("NODE0001");
        }));
        std::cout << "startup send and request pacing: accepted packet reaches gateway\n";
    }
    {
        // 5.0 MamaDucks only relay data addressed to 00000000, so data for the
        // PapaDuck must keep that header address. Route JSON uses the text alias.
        Mesh mesh(true);
        mesh.until(120000);
        const size_t start = mesh.sent.size();
        assert(mesh.a.sendData(topics::sensor, "{\"c\":2}") == 0);
        mesh.until(130000);
        bool zeroHeader = false;
        for (size_t i = start; i < mesh.sent.size(); ++i)
            if (mesh.sent[i].packet.topic == topics::sensor && mesh.sent[i].packet.sduid == id("NODE0001"))
                zeroHeader = mesh.sent[i].packet.dduid == Duid{};
        assert(zeroHeader);
        RouteJSON toPapa(PAPADUCK_DUID, id("NODE0001"));
        toPapa.addToPath(id("NODE0001"));
        const std::string text = toPapa.asString();
        assert(text.find("\"destination\":\"11111111\"") != std::string::npos);
        assert(text.find('\0') == std::string::npos);
        assert(RouteJSON::fromPacketData(duckutils::stringToByteVector(text))->getDestination() == PAPADUCK_DUID);
        // Phoenix 2026 firmware addressed the PapaDuck as 11111111 in the header.
        CdpPacket legacy(PAPADUCK_ALIAS_DUID, topics::sensor, {1}, id("NODE0003"), DuckType::MAMA);
        legacy.muid = {'L','E','G','1'}; assert(legacy.prepareForSending() == 0);
        const auto receipts = Node<Papa>::received.size();
        mesh.p.radio().inbox.push_back(legacy.asBytes());
        for (int i = 0; i < 3; ++i) mesh.p.run();
        assert(Node<Papa>::received.size() == receipts + 1);
        assert(Node<Papa>::received.back().dduid == PAPADUCK_DUID);
        std::cout << "5.0 compatibility: header 00000000, route JSON 11111111, legacy header accepted\n";
    }
    {
        // The PapaDuck is the destination for health, so it hands its own to
        // the application instead of the radio.
        Mesh mesh(true);
        mesh.until(CDPCFG_HEALTH_INTERVAL_MS + 1000);
        const auto own = std::find_if(Node<Papa>::received.begin(), Node<Papa>::received.end(), [](const auto& packet) {
            return packet.topic == topics::health && packet.sduid == id("GATE0001");
        });
        assert(own != Node<Papa>::received.end());
        assert(own->dduid == PAPADUCK_DUID && own->hopCount == 0 && own->duckType == DuckType::PAPA);
        assert(parse(std::string(own->data.begin(), own->data.end()))["v"] == 4);
        assert(std::none_of(mesh.sent.begin(), mesh.sent.end(), [](const auto& item) {
            return item.sender == 3 && item.packet.topic == topics::health;
        }));
        assert(std::any_of(Node<Papa>::received.begin(), Node<Papa>::received.end(), [](const auto& packet) {
            return packet.topic == topics::health && packet.sduid == id("NODE0001");
        }));
        std::cout << "PapaDuck health: handed to the application, not transmitted\n";
    }
    timeSyncChecks();
    txPowerChecks();
    sleepChecks();
    bloomChecks();
    std::cout << "PASS\n";
}
