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
CircularBuffer::~CircularBuffer()
{
    delete [] buffer;
}


