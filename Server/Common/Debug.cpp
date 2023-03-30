#include "Debug.h"
#include <stdlib.h>
#include <time.h>

namespace debugPrint 
{
   namespace debugPrint_internal 
   {
      static char defaultDebugLevel = INFO;
   }

   using namespace debugPrint_internal;
   bool shouldPrint(char messageLevel)
   {
      bool print = false;

      if(messageLevel >= getDebugLevel())
      {
         print = true;
      }
      return print;
   }

   long getDebugLevel()
   {
      // Read the environment variable for the debug level
      char *debugLevelStr;
      debugLevelStr = getenv("DEBUG_PRINT_LEVEL");

      long debugLevel = defaultDebugLevel;
      if(debugLevelStr != NULL)
      {
         debugLevel = strtol(debugLevelStr, NULL, 10);
      }
   
      return debugLevel;
   }

   std::string getTime()
   {
      time_t now = time(nullptr);
      time(&now);
      struct tm* timeinfo = localtime(&now);

      char buffer[80];
      strftime(buffer, 80, "%T", timeinfo);
      std::string result(buffer);
      return result;
   }
}
