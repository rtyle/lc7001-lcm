#ifndef _RFM100STUB_H_
#define _RFM100STUB_H_

#include "LCMServer.h"
#include <deque>
#include <queue>
#include <memory>

struct RFM100Message
{
   RFM100_TRANSMIT_TASK_MESSAGE_PTR transmitTaskMessagePtr;
   uint8_t priority;
   uint32_t index;
};

struct RFM100MessageComparator
{
   bool operator()(RFM100Message const &lhs, RFM100Message const &rhs)
   {
      if(lhs.priority == rhs.priority)
      {
         // This will put the message with the lowest message index 
         // at the top of the queue if the priorities are equal
         return lhs.index > rhs.index;
      }
      else
      {
         // This will put the message with the highest priority at the top of
         // the queue if the priorities are not equal
         return lhs.priority < rhs.priority;
      }
   }
};

class RFM100Stub
{
   public:
      // Constructor
      RFM100Stub(std::shared_ptr<std::mutex> priorityMutexPtr);

      // Destructor
      virtual ~RFM100Stub();

      // Adds a message to the RFM100 Transmit Queue
      void AddMessageToQueue(RFM100_TRANSMIT_TASK_MESSAGE_PTR messagePtr, uint8_t priority);

   private:
      // Message queue variables
      int messageIndex;
      std::priority_queue<RFM100Message, std::vector<RFM100Message>, RFM100MessageComparator> transmitMessageQueue;
      std::mutex transmitMessageQueueMutex;
      std::condition_variable transmitMessageQueueCondition;

      // Thread that pulls messages from the queue
      std::thread transmitThread;
      bool running;
      void RunTransmitThread();

      // Mutex used to enforce thread priority
      std::shared_ptr<std::mutex> priorityMutexPtr;
};

#endif
