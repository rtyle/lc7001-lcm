#include "LCMTestClient.h"
#include "sysinfo.h"
#include "Debug.h"

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

LCMTestClient::LCMTestClient(const uint64_t &deviceId) :
   TestClient(deviceId)
{
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool LCMTestClient::handleSystemInfo(json_t * systemInfoObject)
{
   logVerbose("Enter");

   // Convert the device ID into a MAC address to report
   char MACAddressString[18];
   snprintf(MACAddressString, sizeof(MACAddressString), 
         "%02X:%02X:%02X:%02X:%02X:%02X", 
         static_cast<unsigned int>((deviceId >> 40) & 0xFF),
         static_cast<unsigned int>((deviceId >> 32) & 0xFF),
         static_cast<unsigned int>((deviceId >> 24) & 0xFF),
         static_cast<unsigned int>((deviceId >> 16) & 0xFF),
         static_cast<unsigned int>((deviceId >> 8) & 0xFF),
         static_cast<unsigned int>(deviceId & 0xFF));

   // Create the system info response and send it back
   json_t * responseObject = json_object();
   json_object_set_new(responseObject, "ID", json_integer(0));
   json_object_set_new(responseObject, "Service", json_string("SystemInfo"));
   json_object_set_new(responseObject, "Model", json_string(MODEL_STRING));
   json_object_set_new(responseObject, "FirmwareVersion", json_string(SYSTEM_INFO_FIRMWARE_VERSION));
   json_object_set_new(responseObject, "FirmwareDate", json_string(SYSTEM_INFO_FIRMWARE_DATE));
   json_object_set_new(responseObject, "FirmwareBranch", json_string(SYSTEM_INFO_FIRMWARE_BRANCH));
   json_object_set_new(responseObject, "MACAddress", json_string(MACAddressString));
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

