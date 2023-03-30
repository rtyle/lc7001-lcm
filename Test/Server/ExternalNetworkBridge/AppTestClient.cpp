#include "AppTestClient.h"
#include "Debug.h"
#include <sstream>

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

AppTestClient::AppTestClient(const uint64_t &deviceId) :
   TestClient(deviceId)
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool AppTestClient::handleReportAppID(json_t * appIdObject)
{
   logVerbose("Enter");

   // Create the app ID response and send it back
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(0));
   json_object_set_new(responseObject, "Service", json_string("ReportAppID"));
   json_object_set_new(responseObject, "AppID", json_integer(deviceId));
   json_object_set_new(responseObject, "Status", json_string("Success"));

   // Send the JSON message
   bool messageSent = sendJsonPacket(responseObject);

   // Free the memory of the JSON object
   json_decref(responseObject);

   logVerbose("Exit %d", messageSent);
   return messageSent;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

