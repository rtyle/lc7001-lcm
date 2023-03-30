#include "SSLClient.h"
#include "Debug.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>
#include <signal.h>
#include "SSLCommon.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLClient::SSLClient(const std::string &localCertFile, const std::string &localKeyFile, const std::string &remoteCertFile) :
   localCertificateFilename(localCertFile),
   localKeyFilename(localKeyFile),
   remoteCertificateFilename(remoteCertFile),
   ctx(NULL),
   sslHandle(NULL)
{
   logVerbose("Enter");

   signal(SIGPIPE, SIG_IGN);

   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

SSLClient::~SSLClient()
{
   logVerbose("Enter");

   disconnect();
   
   // Release the context
   if(sslHandle)
   {
      SSL_free(sslHandle);
      sslHandle = NULL;
   }

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

bool SSLClient::connectToHost(const std::string &hostname, const int &port)
{
   logVerbose("Enter");

   bool connected = false;

   // Create the context
   ctx = SSLCommon::createSSLContext(localCertificateFilename, localKeyFilename, remoteCertificateFilename);

   if(ctx == NULL)
   {
      ERR_print_errors_fp(stderr);
   }
   else
   {
      // Create the server address struct
      struct sockaddr_in serverAddr;
      memset(&serverAddr, 0, sizeof(serverAddr));
      serverAddr.sin_family = AF_INET;
      serverAddr.sin_addr.s_addr = inet_addr(hostname.c_str());
      serverAddr.sin_port = htons(port);

      // Create the socket
      int socketHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
      if(socketHandle == -1)
      {
         logError("Could not create socket");
      }
      else
      {
         logDebug("Socket created %d", socketHandle);

         // Connect to the server
         int connectReturn = connect(socketHandle, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

         if(connectReturn < 0)
         {
            logError("Failed to connect to the server: %s", strerror(errno));

            // Close the socket Handle
            close(socketHandle);
            socketHandle = -1;
         }
         else
         {
            sslHandle = SSL_new(ctx);
            SSL_set_fd(sslHandle, socketHandle);
            if(SSL_connect(sslHandle) == -1)
            {
               logError("Failed to connect to the SSL Socket");
               ERR_print_errors_fp(stderr);

               disconnect();
            }
            else
            {
               logInfo("Successfully connected to the SSL Server");
               logDebug("Connected with %s encryption", SSL_get_cipher(sslHandle));
               // Get any certificates
               X509 *cert = SSL_get_peer_certificate(sslHandle);

               if(cert != NULL)
               {
                  logDebug("Server Certificate Returned");

                  char *line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
                  logDebug("Subject %s", line);
                  free(line);

                  line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
                  logDebug("Issuer %s", line);
                  free(line);

                  X509_free(cert);

                  connected = true;

                  // Start listening on the socket for data
                  logDebug("Start Receiving");
                  receiveThread = std::move(std::thread(&SSLClient::runReceiveThread, this));
               }
               else
               {
                  logError("No Certificates. Disconnecting");
                  disconnect();
               }
            }
         }
      }
   }

   logVerbose("Exit");
   return connected;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool SSLClient::disconnect(const bool &joinReceiveThread)
{
   logVerbose("Enter");

   bool disconnected = false;

   if(sslHandle)
   {
      int socketHandle = SSL_get_fd(sslHandle);
      logDebug("Disconnecting the client socket %d", socketHandle);

      SSL_shutdown(sslHandle);

      // Close the socket
      if(socketHandle != -1)
      {
         logDebug("Shutdown Socket");
         shutdown(socketHandle, SHUT_RDWR);

         logDebug("Closing Socket");
         close(socketHandle);
         socketHandle = -1;
         logDebug("Close Finished");

         if(joinReceiveThread)
         {
            if(receiveThread.joinable())
            {
               logVerbose("Joining the receive thread");
               receiveThread.join();
            }
         }

         logDebug("Disconnected");
         disconnected = true;
      }
   }
   else
   {
      // Already disconnected so return true
      if(joinReceiveThread)
      {
         if(receiveThread.joinable())
         {
            logVerbose("Joining the receive thread");
            receiveThread.join();
         }
      }

      disconnected = true;
   }

   logVerbose("Exit");
   return disconnected;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool SSLClient::send(const std::vector<uint8_t> &data)
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
         disconnect();
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

void SSLClient::runReceiveThread()
{
   logVerbose("Enter");

   char dataBytes[255];
   int readSize = 0;

   while((sslHandle) &&
         ((readSize = SSL_read(sslHandle, dataBytes, sizeof(dataBytes))) > 0))
   {
      // Notify any subscribers
      const std::vector<uint8_t> bytes(dataBytes, dataBytes+readSize);
      auto notifyBytes = std::make_shared<SSLBytes>(bytes);
      publish(notifyBytes);

      logVerbose("Waiting for next bytes");
   }

   if(readSize == 0)
   {
      logInfo("Server Disconnected");
   }
   else if(readSize < 0)
   {
      logError("Receive Failed");
   }

   // Stop receiving and shut down the socket
   disconnect(false);

   logDebug("Receive Thread Exiting");
   logVerbose("Exit");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
