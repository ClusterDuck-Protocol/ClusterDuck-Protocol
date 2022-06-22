#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <CdpPacket.h>

class CircularBuffer{
private:
    int head;
    int tail;
    int count;
    int buffer_end;
    CdpPacket* buffer;
public:
    CircularBuffer(int size);

    ~CircularBuffer();

   /**
   * @brief push message into buffer
   *
   * @param msg the packet to add to the buffer
   */
    void push(CdpPacket msg);

   /**
   * @brief retrieve head of the circular buffer
   * 
   * @returns index of the head element
   */
    int getHead();

   /**
   * @brief retrieve tail of the circular buffer
   * 
   * @returns index of the tail element
   */
    int getTail();

   /**
   * @brief retrieve the end of the circular buffer
   * 
   * @returns index of the 
   */
    int getBufferEnd();

   /**
   * @brief retrieve a specific message from the buffer
   *
   * @param index the index of the message to retrieve
   * 
   * @returns the message packet at the provided index
   */
    CdpPacket getMessage(int index);

    int findMuid(std::vector<byte> muid);

};

#endif