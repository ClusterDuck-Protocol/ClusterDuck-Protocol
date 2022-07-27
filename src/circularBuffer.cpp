#include "include/circularBuffer.h"
#include "DuckLogger.h"

//when initializing the buffer, the buffer created will be one larger than the provided size
//one slot in the buffer is used as a waste slot
CircularBuffer::CircularBuffer(int size)
{
    buffer = new CdpPacket[size + 1];
    head = 0;
    buffer_end = size + 1;
    tail = 0;
    count = 0;
}
void CircularBuffer::push(CdpPacket msg)
{
    buffer[head] = msg;
    head = head+ 1;
    if(head == buffer_end) head = 0;
    if(head == tail) tail+= 1;
    if(tail == buffer_end) tail = 0;
}
int CircularBuffer::getHead(){
    return head;
}
int CircularBuffer::getTail(){
    return tail;
}
int CircularBuffer::getBufferEnd(){
    return buffer_end;
}

CdpPacket CircularBuffer::getMessage(int index)
{
    return buffer[index];
}

int CircularBuffer::findMuid(std::vector<byte> muid){
    for (int i = 0; i < (head); i++) {
        std::string log1(muid.begin(), muid.end());
        std::string log2(buffer[i].muid.begin(), buffer[i].muid.end());
        if(buffer[i].muid == muid){
            return i;
        }
    }
    return -1;
}

int CircularBuffer::updateMuid(std::vector<byte> oldMuid, std::vector<byte> newMuid){
    int packetIndex = findMuid(oldMuid);
    buffer[packetIndex].muid = newMuid;
    return 1;
}

int CircularBuffer::ackMessage(std::vector<byte> muid){
    int packetIndex = findMuid(muid);
    buffer[packetIndex].acked = true;
    return 1;
}

CircularBuffer::~CircularBuffer()
{
    delete [] buffer;
}


