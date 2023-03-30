#ifndef _LCMSERVER_H_
#define _LCMSERVER_H_

#include <thread>
#include <string>
#include <vector>
#include "jansson.h"
#include "defines.h"
#include "typedefs.h"
#include "AvahiClient.h"
#include <mutex>
#include <condition_variable>

class LCMServer
{
   private:
      class LCMClientSocketHandler
      {
         public:
            LCMClientSocketHandler(std::shared_ptr<std::mutex> priorityMutexPtr, 
                  const int &handle = -1, LCMServer *server = NULL);
            virtual ~LCMClientSocketHandler();

            void setSocketHandle(const int &handle);
            void startReceiving();
            void stopReceiving();
            void startPinging(const std::chrono::seconds &dwellTimeSeconds);
            void stopPinging();
            bool SendJSONPacket(json_t *jsonObject, bool notifyPing = true);

         private:
            int socketHandle;
            LCMServer *server;

            // Thread that receives JSON packets from the client
            std::thread receiveThread;
            virtual void runReceiveThread();

            // Thread that sends ping data to the client
            std::thread pingThread;
            std::condition_variable pingCondition;
            std::mutex pingMutex;
            bool pinging;
            bool messageSent;
            virtual void runPingThread(const std::chrono::seconds &dwellTimeSeconds);

            // Methods that send data to the client
            bool sendPing();

            // JSON Message Handlers
            void handleJsonString(const std::string &jsonString);

            // Priority mutex pointer
            std::shared_ptr<std::mutex> priorityMutexPtr;
      };

   public:
      LCMServer(const int &port, std::shared_ptr<std::mutex> priorityMutexPtr);
      virtual ~LCMServer();
      void startLCMServer();
      void stopLCMServer();
      void sendJSONBroadcast(json_t *jsonBroadcastObject);

   private:
      // Port number to listen
      int port = -1;

      // LCM Server Thread
      std::thread serverThread;
      virtual void runServerThread();
      int socketDesc;

      // LCM Client Socket Handlers
      std::mutex clientHandlerMutex;
      std::vector<LCMClientSocketHandler *> clientSocketHandlers;

      // Avahi Client
      AvahiClient *client;

      // Priority Mutex Pointer
      std::shared_ptr<std::mutex> priorityMutexPtr;
};

#endif
