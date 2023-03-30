#include "Debug.h"
#include "SSLClient.h"
#include <getopt.h>
#include <sstream>

#define DEFAULT_PORT 1979
#define DEFAULT_HOSTNAME "127.0.0.1"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void printUsage()
{
   std::cout << "Usage: ./SSLClient -h [server ip] -p [port] -c [localCert.pem] -k [localKey.pem] -r [remoteCert.pem]" << std::endl;
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

   logInfo("Creating the Client connecting to %s:%d", hostname.c_str(), port);
   SSLClient client(localCertificateFile, localKeyFile, remoteCertificateFile);
   if(client.connectToHost(hostname, port))
   {
      logInfo("Successfully connected to the server");
   }
   else
   {
      logError("Failed to connect to the server");
   }

   bool done = false;
   while(!done)
   {
      std::string cmd;
      std::cout << "SSL Client: " << std::flush;
      std::getline(std::cin, cmd);

      if(!cmd.empty())
      {
         if(cmd == "quit")
         {
            client.disconnect();
            done = true;
         }
         else if((cmd == "?") || (cmd == "help"))
         {
            std::cout << "Available commands: quit, help, ?" << std::endl;
         }
         else
         {
            std::vector<uint8_t> dataBytes(cmd.begin(), cmd.end());

            // Send the data to the server
            if(!client.send(dataBytes))
            {
               logError("Failed to send data to the server");
               done = true;
            }
         }
      }
   }

   logInfo("Exiting");
   return 0;
}
