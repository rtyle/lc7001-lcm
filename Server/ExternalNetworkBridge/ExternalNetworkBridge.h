#ifndef _EXTERNAL_NETWORK_BRIDGE_H_
#define _EXTERNAL_NETWORK_BRIDGE_H_

#include <thread>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <sqlite3.h>
#include "LCMSocketHandler.h"
#include "AppSocketHandler.h"
#include "RegistrationSocketHandler.h"
#include "Publisher.h"
#include "Subscriber.h"

typedef struct socketHandlerStruct
{
   std::shared_ptr<LCMSocketHandler> lcmSocket;
   std::vector<std::shared_ptr<AppSocketHandler>> appSockets;
} socketHandlerStruct;

class ExternalNetworkBridge : public Subscriber
{
   public:
      ExternalNetworkBridge(const int &appPort, const int &lcmPort, const int &registrationPort, const std::string &databaseFilename, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename);
      ExternalNetworkBridge(const int &appPort, const int &lcmPort, const int &registrationPort, const std::string &databaseFilename);
      virtual ~ExternalNetworkBridge();
      void start();
      void stop();

      void printLCMTable();
      void printAppTable();

      virtual void update(Publisher *who, std::shared_ptr<PublishObject> obj);

   private:
      // Port number to listen for app connections
      int appPort = -1;

      // Port number to listen for LCM connections
      int lcmPort = -1;

      // Port number to listen for registration connections
      int registrationPort = -1;

      // Database Handle
      sqlite3 *db;

      // App Listener Thread
      int appSocket;
      std::thread appThread;
      virtual void runAppThread();

      // LCM Listener Thread
      int lcmSocket;
      std::thread lcmThread;
      virtual void runLCMThread();

      // Registration Thread
      int registrationSocket;
      std::thread registrationThread;
      virtual void runRegistrationThread();

      // Clean up thread
      bool cleanupThreadRunning;
      std::condition_variable cleanupThreadCondition;
      std::mutex cleanupThreadMutex;
      std::thread cleanupThread;
      virtual void runCleanupThread();

      // Registration sockets connected
      std::map<int, std::shared_ptr<RegistrationSocketHandler>> registrationHandlers;
      std::mutex registrationHandlerMutex;

      // App sockets connected
      std::map<int, std::shared_ptr<AppSocketHandler>> appHandlers;
      std::mutex appHandlerMutex;

      // LCM sockets connected
      std::map<int, std::shared_ptr<LCMSocketHandler>> lcmHandlers;
      std::mutex lcmHandlerMutex;

      // Bridge Map
      std::map<uint64_t, socketHandlerStruct> bridgeMap;
      std::mutex bridgeMapMutex;

      static const int LCM_DISCONNECT_TIMEOUT_SECONDS = 12;

      // SSL Filenames. If not set, bridge will default to non-SSL connections
      std::string localCertificateFilename;
      std::string localKeyFilename;
      std::string remoteCertificateFilename;
};

#endif
