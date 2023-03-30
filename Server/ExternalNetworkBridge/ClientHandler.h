#ifndef _CLIENT_HANDLER_H
#define _CLIENT_HANDLER_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <jansson.h>
#include <openssl/ssl.h>

class ClientHandler : public std::enable_shared_from_this<ClientHandler>
{
   public:
      ClientHandler(const int &clientSocketHandle, const struct sockaddr_in &clientAddress, const uint64_t &handlerId, const std::string &localCertificateFilename, const std::string &localKeyFilename, const std::string &remoteCertificateFilename);
      virtual ~ClientHandler();

      bool isConnected();
      uint64_t getHandlerId();
      uint64_t getDeviceId();
      virtual std::string getDeviceIdStr() = 0;
      std::string getClientIPAddressString();

      int getClientSocketHandle();
      SSL * getClientSSLHandle();
      bool addBridgedSocketHandle(const int &socketHandle);
      bool addBridgedSocketHandle(SSL *sslHandle);
      void removeBridgedSocketHandle(const int &socketHandle);
      void removeBridgedSocketHandle(SSL *sslHandle);

      bool startPinging();
      bool stopPinging();
      void startReceiving();
      void stopReceiving();

   protected:
      uint64_t deviceId;
      uint64_t handlerId;
      int clientSocketHandle;
      virtual bool sendDeviceIdRequest() = 0;
      virtual void publishDeviceId() = 0;
      
      // SSL handle to use for communication
      SSL *sslHandle;

   private:
      bool connected;
      std::vector<int> bridgedSocketHandles;
      std::vector<SSL*> bridgedSSLHandles;
      std::string ipAddressString;

      // Thread that receives data from the client socket and forwards it
      // to any bridged sockets
      std::thread receiveThread;
      static const int MAX_MESSAGE_SIZE = 1024;
      void runReceiveThread();
      bool handleJsonPacket(const std::string &josnString);
      bool handleSystemInfo(json_t *systemInfoObject);
      bool handleReportAppID(json_t *reportAppIDObject);

      // Thread that continually sends pings to the client socket
      bool pinging;
      std::thread pingThread;
      void runPingThread();
      std::mutex pingMutex;
      std::condition_variable pingCondition;

      // Accept the socket. Either SSL or regular
      SSL_CTX *ctx;
      bool acceptSocket();
};

#endif
