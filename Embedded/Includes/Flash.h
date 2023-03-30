#ifndef _FLASH_H_
#define _FLASH_H_

#define DEBUG_FLASH 0
#define DBGPRINT_INTSTRSTR if (DEBUG_FLASH) broadcastDebug
#define DBGPRINT_ERRORS_INTSTRSTR if (DEBUG_FLASH) broadcastDebug

void InitializeFlash();
byte_t WhichFlashBlockToUse();
uint32_t InstallFirmwareToFlash();
uint32_t ClearFlash();

bool_t SwapCallback(uint8_t currentSwapMode);
bool_t HandleS19RecordString(char * S19RecordString, byte_t * S19RecordBytes, byte_t S19RecordLength, char * errorString, size_t errorStringLen);

// System Properties
uint8_t LoadSystemPropertiesFromFlash();
uint32_t SaveSystemPropertiesToFlash();

// Zones
nonvolatile_zone_properties * GetNVZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData);
uint32_t WriteZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_zone_properties * nvZonePropertiesPtr);
void ReadAllZonePropertiesFromFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize);
void WriteAllZonePropertiesToFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize);
void EraseZonesFromFlash();

// Eliot Zone Properties
nonvolatile_eliot_zone_properties * GetNVEliotZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData);
uint32_t WriteEliotZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_eliot_zone_properties * nvEliotZonePropertiesPtr);
bool_t ReadEliotZonePropertiesFromFlashSector(byte_t zoneIndex, nonvolatile_eliot_zone_properties nvEliotZoneArray[], size_t arraySize);
bool_t WriteEliotZonePropertiesToFlashSector(byte_t zoneIndex, nonvolatile_eliot_zone_properties nvEliotZoneArray[], size_t arraySize);
void EraseEliotZonePropertiesFromFlash(byte_t zoneIndex);

// Scenes
nonvolatile_scene_properties * GetNVScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData);
uint32_t WriteScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_scene_properties * scenePropertiesPtr);
bool_t ReadScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize);
bool_t WriteScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize);
void EraseScenesFromFlash(byte_t sceneIndex);

// Eliot Scene Properties
nonvolatile_eliot_scene_properties * GetNVEliotScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData);
uint32_t WriteEliotScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_eliot_scene_properties * nvEliotScenePropertiesPtr);
bool_t ReadEliotScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_eliot_scene_properties nvEliotSceneArray[], size_t arraySize);
bool_t WriteEliotScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_eliot_scene_properties nvEliotSceneArray[], size_t arraySize);
void EraseEliotScenePropertiesFromFlash(byte_t sceneIndex);

// Scene Controllers
nonvolatile_scene_controller_properties * GetNVSceneControllerPropertiesPtr(byte_t sceneControllerIndex, bool_t validateSceneControllerData);
uint32_t WriteSceneControllerPropertiesToFlash(byte_t sceneControllerIndex, nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr);
void ReadAllSceneControllerPropertiesFromFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize);
void WriteAllSceneControllerPropertiesToFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize);
void EraseSceneControllersFromFlash();

// Shades
#ifdef SHADE_CONTROL_ADDED
void ReadAllShadeIdsFromFlash(void);
uint32_t WriteAllShadeIdsToFlash(void);
#endif

// Initial checksum value
#define FLASH_CHECKSUM_INIT 0x7777

// Banked flash routine #defines
#define FBK_BANK_SIZE          4096
#define FBK_INFO_SIZE          8              // Size of accounting information at the end of the bank
#define FBK_BLOCK_SIZE         128            // This must be larger or equal to FBK_INFO_SIZE and a power of 2
#define FBK_BLOCK_MASK         FBK_BLOCK_SIZE-1
#define FBK_BANK_SELECT_MASK   0x01           // Limit bank selection to 0 and 1

// Describes a single 4KB bank of flash
typedef struct FBK_Bank_Info
{
       char data[FBK_BANK_SIZE - FBK_INFO_SIZE];  // Data payload
       uint16_t series;                // Incremented on each write
       uint16_t check_word;    // On a valid block, equal to series ^ 0xFFFF
} FBK_Bank_Info;


// Flash bank context - describes one pair of 4KB banks
typedef struct FBK_Context
{
       uint8_t active_bank;    // Bank being read
       uint8_t flags;
       FBK_Bank_Info *(bank_info[2]);
} FBK_Context;


uint32_t FBK_WriteFlash(FBK_Context *ctx, char* src, uint32_t dst, uint32_t len);
uint32_t FBK_Init(FBK_Context *ctx);
char* FBK_Address(FBK_Context *ctx, uint32_t offset);


#endif // _FLASH_H_
