#ifndef _EVENT_TASK_STUB_H_
#define _EVENT_TASK_STUB_H_

#include "LCMServer.h"

class EventTaskStub
{
   public:
      // Constructor
      EventTaskStub();

      // Destructor
      virtual ~EventTaskStub();

      // Aligns the event task thread to run on minute boundaries
      void AlignToMinuteBoundary();

   private:
      // Event Task Thread
      std::thread eventTaskThread;
      std::condition_variable eventTaskCondition;
      std::mutex eventTaskMutex;
      std::condition_variable alignCondition;
      std::mutex alignMutex;
      bool running;
      void RunEventTaskThread();
      int getMillisecondsUntilNextMinute();
};

#endif
