#include "Debug.h"
#include "ExternalNetworkBridge.h"
#include <iostream>
#include <sstream>
#include <getopt.h>
#include <sysinfo.h>

#define DEFAULT_BRIDGE_LCM_PORT 2525
#define DEFAULT_BRIDGE_APP_PORT 2526
#define DEFAULT_REGISTRATION_PORT 2527

void printUsage()
{
   std::cout << "Usage: ./ExternalNetworkBridge -l [LCMPort] -a [appPort] -r [registrationPort] -d [databaseFile]" << std::endl;
}

int main(int argc, char* argv[])
{
   logInfo("Running the External Network Bridge version %s", SYSTEM_INFO_FIRMWARE_VERSION);

   int appPort = DEFAULT_BRIDGE_APP_PORT;
   int LCMPort = DEFAULT_BRIDGE_LCM_PORT;
   int registrationPort = DEFAULT_REGISTRATION_PORT;

   std::string databaseFile = "default.db";

   int option = 0;
   while ((option = getopt(argc, argv, "a:l:r:d:")) != -1)
   {
      switch (option)
      {
         case 'a':
            {
               std::istringstream ss(optarg);
               if(!(ss >> appPort))
               {
                  logError("Invalid app port number %s. Using default %d", 
                        optarg, DEFAULT_BRIDGE_APP_PORT);
                  appPort = DEFAULT_BRIDGE_APP_PORT;
               }
               break;
            }
         case 'l':
            {
               std::istringstream ss(optarg);
               if(!(ss >> LCMPort))
               {
                  logError("Invalid LCM port number %s. Using default %d", 
                        optarg, DEFAULT_BRIDGE_LCM_PORT);
                  LCMPort = DEFAULT_BRIDGE_LCM_PORT;
               }
               break;
            }
         case 'r':
            {
               std::istringstream ss(optarg);
               if(!(ss >> registrationPort))
               {
                  logError("Invalid registration port %s. Using default %d", 
                        optarg, DEFAULT_REGISTRATION_PORT);
                  LCMPort = DEFAULT_REGISTRATION_PORT;
               }
               break;
            }
         case 'd':
            {
               databaseFile = optarg;
               break;
            }
         default:
            printUsage();
            exit(-1);
      }
   }

   logInfo("Creating Bridge using ports: App(%d), LCM(%d), Registration(%d)", appPort, LCMPort, registrationPort);
   logInfo("Creating Bridge using database: %s", databaseFile.c_str());
   ExternalNetworkBridge bridge(appPort, LCMPort, registrationPort, databaseFile);
   bridge.start();

   bool done = false;
   while(!done)
   {
      std::string cmd;
      std::cout << "Network Bridge: " << std::flush;
      std::getline(std::cin, cmd);

      if(!cmd.empty())
      {
         if(cmd == "quit" || cmd == "exit")
         {
            bridge.stop();
            done = true;
         }
         else if((cmd == "?") || (cmd == "help"))
         {
            std::cout << "Available commands: quit, exit, lcm, app, clear, help, ?" << std::endl;
         }
         else if(cmd == "lcm")
         {
            bridge.printLCMTable();
         }
         else if(cmd == "app")
         {
            bridge.printAppTable();
         }
         else if(cmd == "clear")
         {
            system("clear");
         }
         else
         {
            std::cout << "Unrecognized command: " << cmd << std::endl;
         }
      }
   }

   logInfo("Exiting");
   return 0;
}
