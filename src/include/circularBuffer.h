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

    /**
     * @brief gets the index in the buffer of the message with the provided muid
     * 
     * @param muid the muid of the message to check for
     * 
     * @returns the index of the message, or -1 if it does not exist in the buffer
     */

    int findMuid(std::vector<byte> muid);

    /**
     * @brief replaces the muid of a message in the buffer, used for message resending
     * 
     * @param oldMuid the muid of the original message that you want to update
     * 
     * @param newMuid the new muid to assign the message
     * 
     * @returns 1 if the muid was updated successfully, 0 if failed 
     */

    int updateMuid(std::vector<byte> oldMuid, std::vector<byte> newMuid);

    /**
     * @brief sets the acked field on the packet to true
     * 
     * @param muid the muid of the message that was acked
     * 
     * @returns 1 if the acked field was set successfully, 0 if failed
     */

    int ackMessage(std::vector<byte> muid);

};

#endif