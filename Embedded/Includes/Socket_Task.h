/*
* Socket_Task.h
*
*  Created on: Jun 17, 2013
*      Author: Legrand
*/

#ifndef SOCKET_TASK_H_
#define SOCKET_TASK_H_

#ifndef AMDBUILD
void JSONAccept_Task(uint_32 param);

void JSONTransmit_Task(uint_32 param);

void Socket_Task(uint_32 param);

void FirmwareUpdate_Task(uint_32 param);

int_32 Socket_send(int sockfd, void *buf, size_t len, int flags, unsigned long* errorCounter, const char *caller);

bool_t SendUpdateCommand(byte_t commandIndex, json_t * hostObj, json_t * branchObj);

void MDNS_Task(uint_32 param);

extern firmware_info updateFirmwareInfo;

#define SB327_INCEPTION_YEAR 2020

#else

// Add extern C when building for C++
#if (__cplusplus)
extern "C" 
{
#endif
void NotDefined(json_t *root, json_t *responseObject);

// Linux specific defines to remove compilation of embedded functions
#define HandleManufacturingTestMode(root, response, broadcast) NotDefined(root, response)
#define HandleMTMSendRFPacket(root, response) NotDefined(root, response)
#define HandleMTMSetOutput(root, response) NotDefined(root, response)
#define HandleMTMSendRS485String(root, response) NotDefined(root, response)
#define HandleMTMSetMACAddress(root, response) NotDefined(root, response)
#define HandleMTMCheckFlash(root, response) NotDefined(root, response)

#define HandleCheckForUpdate() NotDefined(NULL, NULL)
#define HandleInstallUpdate(response) NotDefined(NULL, response)
#define HandleFirmwareUpdate(root, response) NotDefined(root, response)
#define HandleSystemRestart() NotDefined(NULL, NULL)
#define HandlePing() NotDefined(NULL, NULL)
#define JSONAccept_Task(param) NotDefined(NULL, NULL)
#define JSONTransmit_Task(param) NotDefined(NULL, NULL)
#define Socket_Task(param) NotDefined(NULL, NULL)
#define FirmwareUpdate_Task(param) NotDefined(NULL, NULL)
#define SendUpdateCommand(cmdIndx, hostObj, branchObj) NotDefined(NULL, NULL)
#define MDNS_TASK(param) NotDefined(NULL, NULL)

#if (__cplusplus)
}
#endif
#endif

// Shared functions between embedded and the server processes
// Add extern C when building for C++
#if (__cplusplus)
extern "C" {
#endif

void SendJSONPacket(byte_t socketIndex, json_t * packetToSend);

json_t * BuildErrorResponseBadJsonFormat(json_error_t *error);
void BuildErrorResponse(json_t *responseObject, const char *errorText, uint32_t errorCode);
void BuildErrorResponseNoID(json_t *responseObject, const char *errorText, uint32_t errorCode);

void ProcessJsonService(json_t *root, json_t *service,
      json_t *responseObject, json_t ** jsonBroadcastObjectPtr);
json_t * ProcessJsonPacket(json_t * root, json_t ** jsonBroadcastObjectPtr);

// Zones 
void HandleListZones(json_t *responseObject);
void HandleReportZoneProperties(json_t * root, json_t * responseObject);
void HandleSetZoneProperties(json_t *root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
bool_t HandleIndividualProperty(const char * keyPtr,
                                 json_t * valueObject,
                                  zone_properties * zoneProperties,
                                   json_t * responseObject,
                                    dword_t * propertyIndexPtr,
                                     bool_t * sendToTransmitQueueFlagPtr,
                                      bool_t * broadcastFlagPtr,
                                       bool_t * nameChangeFlagPtr,
                                        uint_8 setFlag);

void BuildZonePropertiesObject(json_t * reportPropertiesObject, 
      byte_t zoneIdxWanted, dword_t propertyBitmask);
void HandleDeleteZone(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);

// Scenes
void HandleListScenes(json_t * responseObject);
void HandleReportSceneProperties(json_t * root, json_t * responseObject);
void HandleSetSceneProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
void HandleRunScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
bool_t HandleIndividualSceneProperty(const char * keyPtr,
                                      json_t * valueObject,
                                       scene_properties * scenePropertiesPtr,
                                        json_t * responseObject,
                                         bool_t * nameChangeFlagPtr,
                                          uint_8 setFlag);
void HandleCreateScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
void HandleDeleteScene(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);

// System Info
void BroadcastSystemInfo();
void HandleSystemInfo(json_t * responseObject);
void HandleReportSystemProperties(json_t * responseObject);
void BuildSystemPropertiesObject(json_t * reportPropertiesObject, dword_t propertyBitmask);
void HandleSetSystemProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
bool_t HandleIndividualSystemProperty(const char * keyPtr, json_t * valueObject,
      json_t * responseObject, uint_8 setFlag);
bool_t ValidateLocationProperty(json_t * valueObject, json_t * responseObject, bool_t isLatitude);
void SendBroadcastMessageForMTMCheckFlashResults(bool_t errorFlag);

// Ramp commands
void BroadcastZoneAdded(byte_t zoneID);
void HandleTargetValue(word_t groupID, byte_t targetValue, byte_t zoneIdx);
void HandleTargetValueAll(byte_t targetValue);
void HandleRampReceived(byte_t buildingId, byte_t houseId, word_t groupId, byte_t targetLevel, Dev_Type deviceType);
void HandleTriggerRampCommand(json_t *root, json_t *response);
void HandleTriggerSceneControllerCommand(json_t *root, json_t *response);
void HandleTriggerRampAllCommand(json_t *root, json_t *responseObject);

// Status commands
void HandleStatusReceived(byte_t buildingId, byte_t houseId, word_t groupId, byte_t targetLevel, byte_t deviceType);

void ExecuteScene(byte_t sceneIndex, scene_properties * scenePropertiesPtr, uint32_t appContextId, bool_t activate, byte_t value, byte_t change, byte_t rampRate);

// Scene Controller Functions
void HandleListSceneControllers(json_t * responseObject);
void HandleReportSceneControllerProperties(json_t * root, json_t * responseObject);
void HandleSetSceneControllerProperties(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
void HandleDeleteSceneController(json_t * root, json_t * responseObject, json_t ** jsonBroadcastObjectPtr);
void HandleSceneControllerCommandReceived( byte_t sceneControllerIndex, byte_t buildingId, byte_t houseId, word_t groupId, byte_t bankIndex);
void HandleSceneControllerMasterOff(byte_t buildingId, byte_t houseId, word_t groupId, byte_t deviceType, byte_t rampRate);
void HandleSceneControllerSceneRequest(byte_t buildingId, byte_t houseId, word_t groupId, byte_t bankIndex, byte_t state, byte_t deviceType, byte_t rampeRate);
void HandleSceneControllerSceneAdjust(byte_t buildingId, byte_t houseId, word_t groupId, byte sceneIdx, byte_t value, byte_t change, byte_t rampeRate);
byte_t FindNextEmptyInSceneControllerArray(void);

// Apple watch expanded commands
void HandleListZonesExpanded(json_t * responseObject);
void HandleListScenesExpanded(json_t * responseObject);
// Factory Reset
void HandleClearFlash();

// manufacturing test mode input check;
void HandleMTMInputSense(onqtimer_t tickCount);
// Qmotion Shade functions
void TryToFindQmotion(void);
void GetQmotionDevicesInfo(void);
void SendShadeCommand(uint8_t zoneIndex, uint8_t percentOpen);
void QueryQmotionStatus(void);

// Password Functions
void JSA_ClearUserKeyInFlash();

#if (__cplusplus)
}
#endif

#endif /* SOCKET_TASK_H_ */
