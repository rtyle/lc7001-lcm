#include "RFM100Stub.h"
#include "Debug.h"
#include "RFM100_Tasks.h"
#include "MQXWrapper.h"

extern "C"
{
#include "defines.h"
#include "sysinfo.h"
#include "globals.h"
#include "typedefs.h"
#include "Socket_Task.h"
#include "RFM100_Tasks.h"
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

RFM100Stub::RFM100Stub(std::shared_ptr<std::mutex> mutexPtr) :
   messageIndex(0),
   priorityMutexPtr(mutexPtr)
{
   logVerbose("Enter");

   // Create the message pool queue
   RFM100TransmitTaskMessagePoolID = _msgpool_create(sizeof(RFM100_TRANSMIT_TASK_MESSAGE), NUMBER_OF_RFM100_TRANSMIT_MESSAGES_IN_POOL, 0, 0);

   running = true;
   logDebug("Starting the transmit thread");
   transmitThread = std::move(std::thread(&RFM100Stub::RunTransmitThread, this));
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

RFM100Stub::~RFM100Stub()
{
   logVerbose("Enter");

   running = false;
   transmitMessageQueueCondition.notify_one();

   if(transmitThread.joinable())
   {
      transmitThread.join();
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void RFM100Stub::AddMessageToQueue(RFM100_TRANSMIT_TASK_MESSAGE_PTR messagePtr, uint8_t priority)
{
   logVerbose("Enter");

   logDebug("Adding message to queue %p", messagePtr);
   // Protect the queue with a mutex
   std::lock_guard<std::mutex> lock(transmitMessageQueueMutex);
   RFM100Message msg;
   msg.transmitTaskMessagePtr = messagePtr;
   msg.priority = priority;
   msg.index = messageIndex++;
   transmitMessageQueue.push(msg);
   transmitMessageQueueCondition.notify_one();

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void RFM100Stub::RunTransmitThread()
{
   logVerbose("Enter");
   logDebug("Transmit Thread running");

   while(running)
   {
      logDebug("Waiting on condition");
      {
         std::unique_lock<std::mutex> lock(transmitMessageQueueMutex);
         transmitMessageQueueCondition.wait(lock, [this](){return transmitMessageQueue.size() > 0;});
         logDebug("Condition triggered");
      }

      if(priorityMutexPtr->try_lock())
      {
         // Only pull the message from the queue if we were able to obtain the
         // priority lock
         if(running)
         {
            // Get the latest message from the deque
            RFM100Message message = transmitMessageQueue.top();
            transmitMessageQueue.pop();

            logDebug("Handling message %p", message.transmitTaskMessagePtr);
            HandleTransmitTaskMessage(message.transmitTaskMessagePtr, false);
            free(message.transmitTaskMessagePtr);
         }

         // Unlock the mutex
         priorityMutexPtr->unlock();
      }
   }

   logDebug("Transmit Thread exiting");

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
