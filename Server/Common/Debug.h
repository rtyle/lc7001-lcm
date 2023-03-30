#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <iostream>
#include <cstring>

namespace debugPrint {
   enum DebugLevel {
      VERBOSE = 0,
      DEBUG = 1,
      INFO = 2,
      WARN = 3,
      ERROR = 4,
      FATAL = 5
   };

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define logMessage(LEVEL, MESSAGE, ...) \
   std::cout << "LOG." << static_cast<int>(LEVEL) << "  " << \
   debugPrint::getTime() << " [" << FILENAME \
   << "::" << __FUNCTION__ << "(" << __LINE__ << ")]: "; \
   printf(MESSAGE, ##__VA_ARGS__); \
   std::cout << std::endl;

   // Convenience method to log a verbose message
#define logVerbose(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::VERBOSE)) \
      {\
         logMessage(debugPrint::VERBOSE, __VA_ARGS__); \
      }\
   }while(false);

   // Convenience method to log a debug message
#define logDebug(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::DEBUG)) \
      {\
         logMessage(debugPrint::DEBUG, __VA_ARGS__); \
      }\
   }while(false);

   // Convenience method to log an info message
#define logInfo(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::INFO)) \
      {\
         logMessage(debugPrint::INFO, __VA_ARGS__); \
      }\
   }while(false);

   // Convenience method to log a warning message
#define logWarning(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::WARN)) \
      {\
         logMessage(debugPrint::WARN, __VA_ARGS__); \
      }\
   }while(false);

   // Convenience method to log an error message
#define logError(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::ERROR)) \
      {\
         logMessage(debugPrint::ERROR, __VA_ARGS__); \
      }\
   }while(false);

   // Convenience method to log a fatal message
#define logFatal(...) \
   do {\
      if(debugPrint::shouldPrint(debugPrint::FATAL)) \
      {\
         logMessage(debugPrint::FATAL, __VA_ARGS__); \
      }\
   }while(false);

   long getDebugLevel();
   bool shouldPrint(char messageLevel);
   std::string getTime();
}

#endif
