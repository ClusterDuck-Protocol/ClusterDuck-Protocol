//wrap std::queue with push and pop functions that enforce max size, max size should bebadded to cdp cfg
#pragma once

#include "../utils/DuckLogger.h"
#include <queue>
#include <algorithm>
#include <optional>
#include "../CdpPacket.h"

class SizedQueue{
  public:
  SizedQueue(size_t maxSize = CDPCFG_MAX_QUEUE_SIZE): maxSize(maxSize) {}
  ~SizedQueue() = default; 

  bool enqueue(const CdpPacket& packet){
    if (packetQueue.size() < maxSize){
      packetQueue.push(packet);
      ++accepted;
      highWater = std::max(highWater, packetQueue.size());
    }else{
      loginfo_ln("[ROUTER] packet queue max size exceeded");
      ++dropped;
      return false;
    }
    logdbg_ln("[ROUTER] queue size: %zu", packetQueue.size());
    return true;
  }

  std::optional<CdpPacket> dequeue(){
    if(!packetQueue.empty()){
      CdpPacket packet = packetQueue.front();
      packetQueue.pop();
      return packet;
    } else{
      return std::nullopt;
    }
  }

  void clear(){
    cleared += packetQueue.size();
    packetQueue = std::queue<CdpPacket>{};
  }

  const CdpPacket* front() const { return packetQueue.empty() ? nullptr : &packetQueue.front(); }
  size_t size() const { return packetQueue.size(); }
  size_t highWater = 0;
  uint32_t accepted = 0, dropped = 0, cleared = 0;
  
private:
  size_t maxSize;
  std::queue<CdpPacket> packetQueue;
};
