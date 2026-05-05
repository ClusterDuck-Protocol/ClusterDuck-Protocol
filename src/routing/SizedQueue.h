//wrap std::queue with push and pop functions that enforce max size, max size should bebadded to cdp cfg
#include "../utils/DuckLogger.h";
#include "../CdpPacket. h"

class SizedQueue{

}

public:
  SizedQueue::SizedQueue(int maxSize = CDPCFG_MAX_QUEUE_SIZE): this->maxSize;
  SizedQueue::~SizedQueue()

  SizedQueue::enqueue(CdpPacket packet){
    if (packetQueue.size( )> maxSize){
      packetQueue.enqueue(packet);
    }else{
     loginfo_ln("[ROUTER] rx packet queue max size exceeded");
    }
    loginfo_ln(maxSize);
  }
  CdpPacket SizedQueue::dequeue(CdpPacket packet){
    if(!packetQueue.empty()){
      return packetQueue.dequeue(packet);
    }
  }
private:
  int MAX_QUEUE_SIZE;
  std::queue<CdpPacket> packetQueue;