#include "EventTaskStub.h"
#include "Debug.h"
#include "Event_Task.h"

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

EventTaskStub::EventTaskStub()
{
   logVerbose("Enter");
   logDebug("Starting the event task thread");
   running = true;
   eventTaskThread = std::move(std::thread(&EventTaskStub::RunEventTaskThread, this));
   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

EventTaskStub::~EventTaskStub()
{
   logVerbose("Enter");

   running = false;
   eventTaskCondition.notify_one();
   alignCondition.notify_one();

   if(eventTaskThread.joinable())
   {
      eventTaskThread.join();
   }

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EventTaskStub::AlignToMinuteBoundary()
{
   logInfo("Aligning to minute boundary");

   // Sleep the required time
   std::unique_lock<std::mutex> lock(alignMutex);
   alignCondition.wait_for(lock, std::chrono::milliseconds(getMillisecondsUntilNextMinute()));

   logInfo("Aligned. Notify thread to wake up");
   eventTaskCondition.notify_one();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EventTaskStub::RunEventTaskThread()
{
   logVerbose("Enter");
   logDebug("Event Task Thread running");

   while(running)
   {
      if(running)
      {
         logInfo("Running the event task");

         // Call the Event Task function
         HandleMinuteChange();
      }

      logInfo("Sleep until the next minute");

      std::unique_lock<std::mutex> lock(eventTaskMutex);
      eventTaskCondition.wait_for(lock, std::chrono::milliseconds(getMillisecondsUntilNextMinute()), [this](){ return (running == false); });

      logInfo("Woke Up");
      IncrementMinuteCounter();
   }

   logInfo("Event Task Thread exiting");

   logVerbose("Exit");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int EventTaskStub::getMillisecondsUntilNextMinute()
{
   TIME_STRUCT currentTime;
   _time_get(&currentTime);

   DATE_STRUCT currentDate;
   _time_to_date(&currentTime, &currentDate);

   return ((60 - currentDate.SECOND) * 1000);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
