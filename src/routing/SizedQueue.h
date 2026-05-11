//wrap std::queue with push and pop functions that enforce max size, max size should bebadded to cdp cfg
#include "../utils/DuckLogger.h"
#include <queue>
#include "../CdpPacket.h"

class SizedQueue{
  public:
  SizedQueue(int maxSize = CDPCFG_MAX_QUEUE_SIZE): maxSize(maxSize) {}
  ~SizedQueue() = default; 

  void enqueue(CdpPacket packet){ //pass copy or reference here?
    if (packetQueue.size() < maxSize){
      packetQueue.push(packet);
    }else{
     loginfo_ln("[ROUTER] rx packet queue max size exceeded");
    }
    loginfo_ln("[ROUTER] queue size: %d", maxSize);
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
  
private:
  int maxSize;
  std::queue<CdpPacket> packetQueue;
};