#ifndef _AVAHI_CLIENT_H_
#define _AVAHI_CLIENT_H_

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <string>
#include <thread>

class AvahiClient
{
   public:
      AvahiClient();
      virtual ~AvahiClient();

   private:
      static std::string mdnsName;
      static AvahiSimplePoll *poll;
      static AvahiEntryGroup *group;

      std::thread avahiThread;
      virtual void runAvahiThread();
      static void avahiCallback(AvahiClient *client, AvahiClientState state,
            AVAHI_GCC_UNUSED void* userdata);
      static void createAvahiNames(AvahiClient *client);
      static void entryGroupCallback(AvahiEntryGroup *g, 
            AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata);

};

#endif
