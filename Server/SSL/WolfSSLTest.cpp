#include "Debug.h"
#include <wolfssl/ssl.h>
#include <getopt.h>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT 1979
#define DEFAULT_HOSTNAME "127.0.0.1"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int myVerify(int preverify, WOLFSSL_X509_STORE_CTX* store)
{
   (void)preverify;
   char buffer[80];

   logInfo("In verification callback, error = %d, %s\n",
         store->error, wolfSSL_ERR_error_string(store->error, buffer));

   return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void printUsage()
{
   std::cout << "Usage: ./WolfSSLClient -h [server ip] -p [port] -c [localCert.pem] -k [localKey.pem] -r [remoteCert.pem]" << std::endl;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
   int port = DEFAULT_PORT;
   std::string hostname = DEFAULT_HOSTNAME;
   std::string localCertificateFile;
   std::string localKeyFile;
   std::string remoteCertificateFile;
   char option = 0;

   while ((option = getopt(argc, argv, "h:p:c:k:r:")) != -1)
   {
      switch (option)
      {
         case 'p':
            {
               std::istringstream ss(optarg);
               if(!(ss >> port))
               {
                  logError("Invalid port number %s. Using default %d", optarg, DEFAULT_PORT);
                  port = DEFAULT_PORT;
               }
               break;
            }
         case 'h':
            {
               hostname = optarg;
               break;
            }
         case 'c':
            localCertificateFile = optarg;
            break;
         case 'k':
            localKeyFile = optarg;
            break;
         case 'r':
            remoteCertificateFile = optarg;
            break;
         default:
            {
               logError("Invalid Argument %c", option);
               printUsage();
               exit(-1);
            }
      }
   }

   /* for debug, compile wolfSSL with DEBUG_WOLFSSL defined */
   wolfSSL_Debugging_ON();
   wolfSSL_Init();

   WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
   if (ctx == 0)
   {
      logError("Could not create context");
   }
   else
   {
      wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, myVerify);

      int ret = wolfSSL_CTX_use_certificate_file(ctx, localCertificateFile.c_str(), SSL_FILETYPE_PEM);
      if(ret != SSL_SUCCESS)
      {
         logError("Can't load client cert file, check file");
         exit(-1);
      }

      ret = wolfSSL_CTX_use_PrivateKey_file(ctx, localKeyFile.c_str(), SSL_FILETYPE_PEM);
      if(ret != SSL_SUCCESS)
      {
         logError("Can't load client key file, check file");
         exit(-2);
      }

      ret = wolfSSL_CTX_load_verify_locations(ctx, remoteCertificateFile.c_str(), 0);
      if(ret != SSL_SUCCESS)
      {
         logError("Can't load CA cert file, check file");
         exit(-3);
      }

      /* create socket descriptor */
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if(sockfd == -1)
      {
         logError("Socket creation failed");
         exit(-4);
      }
      else
      {
         logInfo("Socket created successfully");
      }

      struct sockaddr_in	servaddr;
      memset((char*)&servaddr, 0, sizeof(servaddr));
      servaddr.sin_family = AF_INET;
      servaddr.sin_port = htons(port);
      servaddr.sin_addr.s_addr = inet_addr(hostname.c_str());

      ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
      if(ret < 0)
      {
         logError("Connect() failed");
         exit(-5);
      } 
      else 
      {
         logInfo("Connected to %u, port %d.", servaddr.sin_addr.s_addr,
               servaddr.sin_port);
      }

      WOLFSSL *ssl = wolfSSL_new(ctx);
      if(ssl == NULL)
      {
         logError("WolfSSL_new failed");
         exit(-6);
      }

      wolfSSL_set_fd(ssl, sockfd);

      ret = wolfSSL_connect(ssl);
      if(ret != SSL_SUCCESS)
      {
         logError("WolfSSL_connect failed");
         exit(-7);
      }

      bool done = false;
      while(!done)
      {
         std::string cmd;
         std::cout << "WolfSSL Client: " << std::flush;
         std::getline(std::cin, cmd);

         if(!cmd.empty())
         {
            if(cmd == "quit")
            {
               done = true;
            }
            else
            {
               // Send the data to the server
               size_t bytesWritten = wolfSSL_write(ssl, cmd.c_str(), sizeof(cmd.c_str()));

               if(bytesWritten != sizeof(cmd.c_str()))
               {
                  logError("Write failed. Wrote %d of %d", static_cast<int>(bytesWritten), static_cast<int>(sizeof(cmd.c_str())));
                  done = true;
               }
            }
               
         }
      }

      wolfSSL_shutdown(ssl);
      wolfSSL_free(ssl);
      wolfSSL_CTX_free(ctx);
      wolfSSL_Cleanup();
      shutdown(sockfd, SHUT_RDWR);
      close(sockfd);
   }
   logInfo("Exiting");
   return 0;
}
