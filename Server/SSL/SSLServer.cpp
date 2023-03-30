#include "SSLServer.h"
#include "Debug.h"
#include "SSLCommon.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLServer::SSLServer(const int &listenPort, const std::string &localCertFile, const std::string &localKeyFile, const std::string &remoteCertFile) :
   port(listenPort),
   socketDesc(-1),
   localCertificateFilename(localCertFile),
   localKeyFilename(localKeyFile),
   remoteCertificateFilename(remoteCertFile),
   ctx(NULL)
{
   logVerbose("Enter");

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLServer::~SSLServer()
{
   logVerbose("Enter");

   stop();
   
   // Release the context
   if(ctx)
   {
      SSL_CTX_free(ctx);
      ctx = NULL;
   }

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::start()
{
   logVerbose("Enter");

   ctx = SSLCommon::createSSLContext(localCertificateFilename, localKeyFilename, remoteCertificateFilename);

   if(ctx == NULL)
   {
      ERR_print_errors_fp(stderr);
   }

   serverThread = std::move(std::thread(&SSLServer::runServerThread, this));

   std::unique_lock<std::mutex> lock(serverThreadMutex);
   serverThreadCondition.wait(lock);

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::stop()
{
   logVerbose("Enter");

   logDebug("Waiting for the Server to stop %d", socketDesc);
   // Close the socket
   if(socketDesc != -1)
   {
      logDebug("Shutdown Socket");
      int rv = shutdown(socketDesc, SHUT_RDWR);

      logDebug("Shutdown returned %d. Closing Socket", rv);
      close(socketDesc);
      socketDesc = -1;
      logDebug("Close Finished");
   }

   // Join the server thread
   if(serverThread.joinable())
   {
      serverThread.join();
   }

   // Wait for all of the clients to return
   logDebug("Waiting for Clients to finish %d", static_cast<int>(clientSocketHandlers.size()));
   for(size_t i = 0; i < clientSocketHandlers.size(); i++)
   {
      // Free all the client socket handlers
      delete clientSocketHandlers[i];
   }
   clientSocketHandlers.clear();

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::sendBroadcast(const std::vector<uint8_t> &data)
{
   logVerbose("Enter");

   std::lock_guard<std::mutex> lock(clientHandlerMutex);
   for(auto iter = clientSocketHandlers.begin(); iter != clientSocketHandlers.end();)
   {
      if(!(*iter)->send(data))
      {
         logError("Failed to send data to the client");

         // Failed to send. Remove the socket from the list
         (*iter)->stopReceiving();
         delete *iter;

         // Remove the pointer from the list of client handlers so we
         // don't try to send any packets to them again. This will return
         // the iterator of the next object
         iter = clientSocketHandlers.erase(iter);
      }
      else
      {
         // Send succeeded. Increment the iterator
         ++iter;
      }
   }

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::runServerThread()
{
   logVerbose("Enter");

   logDebug("Running Server Thread!");
   int clientSock;
   int c;
   struct sockaddr_in server;
   struct sockaddr_in client;

   // Create the socket
   socketDesc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(socketDesc == -1)
   {
      // Send the condition that the server thread is running
      {
         std::lock_guard<std::mutex> lock(serverThreadMutex);
      }
      serverThreadCondition.notify_all();

      logError("Could not create socket");
   }
   else
   {
      logDebug("Socket created %d", socketDesc);

      // Set the Socket Option to allow reuse of local addresses
      int optionValue = 1;
      if(setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, (char*)&optionValue, sizeof(optionValue)) < 0)
      {
         logError("Error setting SO_REUSEADDR: %s", strerror(errno));
      }

      // Prepare the sockaddr_in structure
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      server.sin_port = htons(port);

      // Attempt to bind to this socket
      int bindReturn = -1;
      while(bindReturn < 0)
      {
         bindReturn = bind(socketDesc, (sockaddr *)&server, sizeof(server));

         if(bindReturn < 0)
         {
            logError("Bind Failed: %s", strerror(errno));
            logDebug("Sleep for a second and try again");
            std::this_thread::sleep_for(std::chrono::seconds(1));
         }
      }

      logDebug("Bind Complete");

      // Listen
      listen(socketDesc, 3);

      // Accept any incoming connection
      c = sizeof(struct sockaddr_in);

      // Send the condition that the server thread is running
      {
         std::lock_guard<std::mutex> lock(serverThreadMutex);
      }
      serverThreadCondition.notify_all();

      logDebug("Waiting for socket accept");
      while( (clientSock = accept(socketDesc, (sockaddr*)&client, 
                  (socklen_t*)&c)) > 0)
      {
         logDebug("Socket connected. Starting Client Handler");

         SSLClientSocketHandler *clientSocketHandler = new SSLClientSocketHandler(clientSock, ctx, this);
         clientSocketHandler->startReceiving();

         {
            logVerbose("Locking Client Mutex");
            std::lock_guard<std::mutex> lock(clientHandlerMutex);
            clientSocketHandlers.push_back(clientSocketHandler);
            logVerbose("Unlocking Client Mutex");
         }
      }

      logDebug("Accept returned -1");
   }
   
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLServer::SSLClientSocketHandler::SSLClientSocketHandler(const int &handle, SSL_CTX *ctx, Publisher *pub) :
   publisher(pub)
{
   logVerbose("Enter");

   if(ctx)
   {
      sslHandle = SSL_new(ctx);
      SSL_set_fd(sslHandle, handle);
   }

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLServer::SSLClientSocketHandler::~SSLClientSocketHandler()
{
   logVerbose("Enter");

   stopReceiving();

   // Join the receiving thread
   if(receiveThread.joinable())
   {
      logVerbose("Joining the receive thread");
      receiveThread.join();
      logVerbose("Joined");
   }
   
   if(sslHandle)
   {
      SSL_free(sslHandle);
      sslHandle = NULL;
   }

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::SSLClientSocketHandler::startReceiving()
{
   logVerbose("Enter");

   // Start thread to receive data
   logDebug("Start Receiving");
   receiveThread = std::move(std::thread(&SSLServer::SSLClientSocketHandler::runReceiveThread, this));
   
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::SSLClientSocketHandler::stopReceiving()
{
   logVerbose("Enter");

   // Shutdown and close socket
   if(sslHandle)
   {
      logDebug("Shutting down the socket");
      int socketHandle = SSL_get_fd(sslHandle);
      SSL_shutdown(sslHandle);
      shutdown(socketHandle, SHUT_RDWR);
      close(socketHandle);
    }
   
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool SSLServer::SSLClientSocketHandler::send(const std::vector<uint8_t> &data)
{
   logVerbose("Enter");

   bool dataSent = false;

   if(sslHandle == NULL)
   {
      logError("Socket handle is invalid");
   }
   else
   {
      int bytesWritten = SSL_write(sslHandle, reinterpret_cast<const char*>(data.data()), data.size());

      if(bytesWritten == static_cast<int>(data.size()))
      {
         dataSent = true;
      }
      else if(bytesWritten == -1)
      {
         logError("Write Failed");
      }
      else
      {
         logError("Data was not sent successfully %d, %d", bytesWritten, static_cast<int>(data.size()));
      }
   }

   logVerbose("Exit");
   return dataSent;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void SSLServer::SSLClientSocketHandler::runReceiveThread()
{
   logVerbose("Enter");

   char dataBytes[255];
   int readSize = 0;

   if(sslHandle)
   {
      logDebug("Waiting for Bytes");
      if(SSL_accept(sslHandle) == -1)
      {
         logError("Socket Handle is invalid");
         ERR_print_errors_fp(stderr);
      }
      else
      {
         // Get any certificates
         X509 *cert = SSL_get_peer_certificate(sslHandle);

         if(cert != NULL)
         {
            logDebug("Client Certificates");
            char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            logDebug("Subject %s", line);
            free(line);

            line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            logDebug("Issuer %s", line);
            free(line);

            X509_free(cert);
         }
         else
         {
            logError("No Client Certificates. Shutdown the socket");
         }

         while((readSize = SSL_read(sslHandle, dataBytes, sizeof(dataBytes))) > 0)
         {
            if(publisher)
            {
               // Notify any subscribers
               const std::vector<uint8_t> bytes(dataBytes, dataBytes+readSize);
               auto notifyBytes = std::make_shared<SSLBytes>(bytes);
               publisher->publish(notifyBytes);

               std::string byteString(bytes.begin(), bytes.end());
               logDebug("Bytes Received = %s", byteString.c_str());
            }

            logVerbose("Waiting for next set of bytes");
         }

         if(readSize == 0)
         {
            logInfo("Client Disconnected");
         }
         else if(readSize < 0)
         {
            logError("Receive Failed");
         }
      }

      // Stop receiving and shut down the socket
      stopReceiving();
   }
   else
   {
      logError("SSL Handle is NULL");
   }

   logDebug("Receive Thread Exiting");
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
