#include "RegistrationTestClient.h"
#include "Debug.h"
#include <jansson.h>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool RegistrationTestClient::sendRegistrationMessage(const uint64_t &appId, const std::string &lcmId)
{
   logVerbose("Enter");
   bool messageSent = false;

   json_t *jsonObject = json_object();
   if(jsonObject)
   {
      // Create the message
      json_object_set_new(jsonObject, "ID", json_integer(1));
      json_object_set_new(jsonObject, "Service", json_string("AppRegistration"));
      json_object_set_new(jsonObject, "appID", json_integer(appId));
      json_object_set_new(jsonObject, "lcmID", json_string(lcmId.c_str()));

      // Send the message
      messageSent = sendJsonPacket(jsonObject);

      // Clean up memory
      json_decref(jsonObject);
   }

   logVerbose("Exit %d", messageSent);
   return messageSent;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

