#include "AvahiClient.h"
#include "Debug.h"
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sstream>

std::string AvahiClient::mdnsName = "LCM1.local";
AvahiSimplePoll *AvahiClient::poll = NULL;
AvahiEntryGroup *AvahiClient::group = NULL;
AvahiClient::AvahiClient()
{
   logVerbose("Enter");

   // Start the thread that publishes the mdns name
   avahiThread = std::move(std::thread(&AvahiClient::runAvahiThread, this));

   logVerbose("Exit");
}


AvahiClient::~AvahiClient()
{
   logVerbose("Enter");

   logInfo("Stopping the Avahi Thread");

   // Stop the Avahi Thread
   if(poll)
   {
      avahi_simple_poll_quit(poll);
   }

   if(avahiThread.joinable())
   {
      avahiThread.join();
   }

   // Free the poll
   if(poll)
   {
      avahi_simple_poll_free(poll);
      poll = NULL;
   }

   logVerbose("Exit");
}

void AvahiClient::runAvahiThread()
{
   logVerbose("Enter");

   poll = avahi_simple_poll_new();

   if(poll)
   {
      int error = 0;
      const AvahiPoll *simplePoll = avahi_simple_poll_get(poll);
      AvahiClient *client = avahi_client_new(simplePoll,
            static_cast<AvahiClientFlags>(0), AvahiClient::avahiCallback, NULL, &error);

      if(client != NULL)
      {
         logDebug("Starting the Avahi poll loop");
         avahi_simple_poll_loop(poll);

         logDebug("Poll loop finished. Freeing client");
         avahi_client_free(client);
         client = NULL;
      }
      else
      {
         logError("Failed to start the Avahi client %s", 
               avahi_strerror(error));
      }
   }
   else
   {
      logError("Failed to create the Avahi simple poll object");
   }

   logDebug("Avahi Client Thread finished");

   logVerbose("Exit");
}


void AvahiClient::avahiCallback(AvahiClient *client, AvahiClientState state, 
      AVAHI_GCC_UNUSED void * userdata) 
{
   logVerbose("Enter");

   // Called whenever the client or server state changes
   switch (state) 
   {
      case AVAHI_CLIENT_S_RUNNING:
         createAvahiNames(client);
         break;

      case AVAHI_CLIENT_FAILURE:
         logError("Client failure: %s", 
               avahi_strerror(avahi_client_errno(client)));
         avahi_simple_poll_quit(poll);
         break;

      case AVAHI_CLIENT_S_COLLISION:
      case AVAHI_CLIENT_S_REGISTERING:
         if(group)
         {
            logDebug("Reset Group");
            avahi_entry_group_reset(group);
         }
         break;

      case AVAHI_CLIENT_CONNECTING:
         // do nothing
         break;
   }

   logVerbose("Exit");
}

void AvahiClient::createAvahiNames(AvahiClient *client)
{
   logVerbose("Enter");

   // If this is the first time we're called 
   // Create a new entry group
   if(!group)
   {
      logDebug("Create a new entry group");
      group = avahi_entry_group_new(client, AvahiClient::entryGroupCallback, 
            NULL);
      if(!group)
      {
         logError("avahi_entry_group_new() failed: %s", 
               avahi_strerror(avahi_client_errno(client)));
      }
      else
      {
         // If the group is empty (either because it was just created, 
         // or because it was reset previously, add our entries.
         if(avahi_entry_group_is_empty(group)) 
         {  
            struct ifaddrs *ifAddrStruct=NULL;
            struct ifaddrs *ifa=NULL;
            void *tmpAddrPtr=NULL;
            getifaddrs(&ifAddrStruct);

            char ipAddressStr[INET_ADDRSTRLEN];
            for(ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
            {
               if(ifa->ifa_addr->sa_family == AF_INET &&
                     (ifa->ifa_flags & IFF_LOOPBACK) != IFF_LOOPBACK)
               {
                  tmpAddrPtr =&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
                  inet_ntop(AF_INET, tmpAddrPtr, ipAddressStr, INET_ADDRSTRLEN);
                  logDebug("%s IP Address %s", ifa->ifa_name, ipAddressStr);
               }
            }

            struct in_addr addr;
            logDebug("IP Address = %s", ipAddressStr);
            inet_pton(AF_INET, ipAddressStr, &addr);
            std::stringstream hexEncoded;
            hexEncoded << static_cast<char>(((addr.s_addr) & 0xFF))
               << static_cast<char>((((addr.s_addr)>>8) & 0xFF))
               << static_cast<char>((((addr.s_addr)>>16) & 0xFF))
               << static_cast<char>((((addr.s_addr)>>24) & 0xFF));

            int ret = avahi_entry_group_add_record(group, 
                  AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 
                  (AvahiPublishFlags)(AVAHI_PUBLISH_USE_MULTICAST|AVAHI_PUBLISH_ALLOW_MULTIPLE), 
                  mdnsName.c_str(), AVAHI_DNS_CLASS_IN, AVAHI_DNS_TYPE_A, 
                  AVAHI_DEFAULT_TTL, hexEncoded.str().c_str(), 
                  hexEncoded.str().length()); 

            if(ret < 0)
            {
               logError("Failed to add record [%s]: %s", mdnsName.c_str(), 
                     avahi_strerror(ret));
            }
            else
            {
               logInfo("Published DNS-SD hostname alias [%s] for IP %s", 
                     mdnsName.c_str(), ipAddressStr);

               ret = avahi_entry_group_commit(group);
               if(ret < 0)
               {
                  logError("Failed to commit entry group: %s", 
                        avahi_strerror(ret));
               }
            }
         }
         else
         {
            logDebug("Entries already added");
         }
      }
   }
   else
   {
      logDebug("Group already created");
   }

   logVerbose("Exit");
}


// Called whenever the entry group state changes
void AvahiClient::entryGroupCallback(AvahiEntryGroup *g, 
      AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) 
{
   logVerbose("Enter");

   assert(g == group || group == NULL);
   group = g;

   switch (state) 
   {
      case AVAHI_ENTRY_GROUP_FAILURE :
         logError("Entry group failure: %s", avahi_strerror(
                  avahi_client_errno(avahi_entry_group_get_client(g))));
         avahi_simple_poll_quit(poll);
         break;

      case AVAHI_ENTRY_GROUP_COLLISION : 
      case AVAHI_ENTRY_GROUP_ESTABLISHED :
      case AVAHI_ENTRY_GROUP_UNCOMMITED:
      case AVAHI_ENTRY_GROUP_REGISTERING:
         // empty
         break;
   }

   logVerbose("Exit");
}
