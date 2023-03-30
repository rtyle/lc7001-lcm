#include "Debug.h"
#include "LCMServer.h"
#include "RFM100Stub.h"
#include "EventTaskStub.h"
#include "sysinfo.h"
#include "time.h"
#include <sstream>
#include <iostream>
#include <getopt.h>
#include "FlashStub.h"

#define MAIN

extern "C"
{
#include "defines.h"
#include "sysinfo.h"
#include "globals.h"
#include "typedefs.h"
#include "Socket_Task.h"
#include "RFM100_Tasks.h"
#include "Event_Task.h"
}

LCMServer *server = NULL;
RFM100Stub *rfm100 = NULL;
EventTaskStub *eventTask = NULL;

// Functions that are called from the C code
// TODO: Move these to a separate file
extern "C"
{
   extern pthread_mutex_t ZoneArrayMutex;
   extern pthread_mutex_t ScenePropMutex;

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void SendJSONPacket(byte_t socketIndex, json_t *packetToSend)
   {
      logVerbose("Send Json Packet");

      if(socketIndex == MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS)
      {
         if(server)
         {
            logDebug("Sending a broadcast packet");
            server->sendJSONBroadcast(packetToSend);
         }
         else
         {
            logError("Server pointer is invalid");
         }
      }
      else
      {
         logError("Only handles broadcast calls at this time");
      }
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   int _msgq_send_priority(RFM100_TRANSMIT_TASK_MESSAGE_PTR message, uint8_t priority)
   {
      logDebug("Adding message to the transmit queue");

      if(rfm100)
      {
         rfm100->AddMessageToQueue(message, priority);
      }
      return 1;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   int RFM100_Send(byte_t *buffer, int transmitLength)
   {
      // Simulate time it takes to send a message and receive the
      // acknowledgement over the RF
      _time_delay(500);
      return transmitLength;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void AlignToMinuteBoundary()
   {
      if(eventTask)
      {
         eventTask->AlignToMinuteBoundary();
      }
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
}

void printUsage()
{
   std::cout << "Usage: ./LCMServer -p [port] -z [zoneFile] -s [sceneFile] -c [sceneControllerFile]" << std::endl;
}

int main(int argc, char* argv[])
{
   // Initialize the mutex
   if(pthread_mutex_init(&ZoneArrayMutex, NULL) != 0)
   {
      logError("Unable to initialize the mutex. Exiting");
      return 1;
   }

   if(pthread_mutex_init(&ScenePropMutex, NULL) != 0)
   {
      logError("Unable to initialize the mutex. Exiting");
      return 1;
   }

   logDebug("ZoneArrayMutex = %p", &ZoneArrayMutex);
   logDebug("ScenePropMutex = %p", &ScenePropMutex);

   logInfo("Running LCM version %s", SYSTEM_INFO_FIRMWARE_VERSION);
   int port = -1;
   std::string zoneFilename = "Zones.txt";
   std::string sceneFilename = "Scenes.txt";
   std::string sceneControllerFilename = "SceneControllers.txt";
   std::string samsungZonePropertiesFilename = "SamsungZoneProperties.txt";
   std::string samsungScenePropertiesFilename = "SamsungSceneProperties.txt";

   int option = 0;
   int optionIndex = 0;
   static struct option long_options[] =
   {
      {"samsungZones", required_argument, 0, 0},
      {"samsungScenes", required_argument, 0, 0},
      {0,0,0,0}
   };

   while ((option = getopt_long(argc, argv, "p:z:s:c:", long_options, &optionIndex)) != -1)
   {
      switch (option)
      {
         case 0:
            if(!strcmp(long_options[optionIndex].name, "samsungZones"))
            {
               samsungZonePropertiesFilename = optarg;
            }
            else if(!strcmp(long_options[optionIndex].name, "samsungScenes"))
            {
               samsungScenePropertiesFilename = optarg;
            }
            break;
         case 'p':
            {
               std::istringstream ss(optarg);
               if(!(ss >> port))
               {
                  logError("Invalid port number %s. Using default %d", optarg,
                        LISTEN_PORT);
                  port = LISTEN_PORT;
               }
               break;
            }
         case 'z':
            zoneFilename = optarg;
            break;
         case 's':
            sceneFilename = optarg;
            break;
         case 'c':
            sceneControllerFilename = optarg;
            break;
         default:
            printUsage();
            exit(-1);
      }
   }

   if(port == -1)
   {
      logDebug("Using Default port %d", LISTEN_PORT);
      port = LISTEN_PORT;
   }
   else
   {
      logDebug("Using port number from command line %d", port);
   }
   
   std::shared_ptr<std::mutex> priorityMutexPtr(new std::mutex());

   logInfo("Creating the Flash Stub");
   FlashStub flash(zoneFilename, sceneFilename, sceneControllerFilename, samsungZonePropertiesFilename, samsungScenePropertiesFilename);

   logInfo("Creating Server listening on port %d", port);
   server = new LCMServer(port, priorityMutexPtr);
   server->startLCMServer();

   logInfo("Creating the RFM 100 Stub");
   rfm100 = new RFM100Stub(priorityMutexPtr);

   logInfo("Creating the Event Task Stub");
   eventTask = new EventTaskStub();

   bool done = false;
   while(!done)
   {
      std::string cmd;
      std::cout << "LCM Server: " << std::flush;
      std::getline(std::cin, cmd);

      if(!cmd.empty())
      {
         if(cmd == "quit")
         {
            server->stopLCMServer();
            delete eventTask;
            done = true;
         }
         else if((cmd == "?") || (cmd == "help"))
         {
            std::cout << "Available commands: quit, help, ?" << std::endl;
         }
         else
         {
            std::cout << "Unrecognized command: " << cmd << std::endl;
         }
      }
   }

   // Destroy the mutex
   pthread_mutex_destroy(&ZoneArrayMutex);
   pthread_mutex_destroy(&ScenePropMutex);

   return 0;
}
