#include "includes.h"
#include "swap_demo.h"

volatile_zone_properties            zoneArray[MAX_NUMBER_OF_ZONES];
nonvolatile_zone_properties         nvZoneArray[MAX_NUMBER_OF_ZONES];
nonvolatile_eliot_zone_properties nvEliotZoneArray[MAX_NUMBER_OF_ZONES];

// these volatile bit arrays are memset to 0 at startup
byte_t   lastLowBatteryBitArray[ZONE_BIT_ARRAY_BYTES];
byte_t   zoneChangedThisMinuteBitArray[ZONE_BIT_ARRAY_BYTES];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// InitializeZoneArray
//
// Sets the zone array to use the default values
//
//------------------------------------------------------------------------------

void InitializeZoneArray(void)
{
   // This is the default data. This will be overwritten by non-volatile data
   byte_t zoneIndex;

   for (zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      // Initialize the volatile data
      zoneArray[zoneIndex].AppContextId = 0;
      zoneArray[zoneIndex].state = FALSE;
      zoneArray[zoneIndex].targetState = FALSE;
      zoneArray[zoneIndex].powerLevel = 100;
      zoneArray[zoneIndex].targetPowerLevel = 100;
      zoneArray[zoneIndex].nvZoneProperties = NULL;
      zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
      
#ifdef SHADE_CONTROL_ADDED
      memset(shadeInfoStruct.shadeInfo[zoneIndex].idString, 0, sizeof(shadeInfoStruct.shadeInfo[zoneIndex].idString));
#endif

   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  anyZonesInArray()
//
//  returns True if there are any zones in the array
//
//------------------------------------------------------------------------------

bool_t AnyZonesInArray()
{
   bool_t zoneFound = FALSE;

   for (int zoneIndex = 0; (zoneIndex < MAX_NUMBER_OF_ZONES) && (!zoneFound); zoneIndex++)
   {
      if ((zoneArray[zoneIndex].nvZoneProperties) &&
          (zoneArray[zoneIndex].nvZoneProperties->ZoneNameString[0] != 0) &&
          (zoneArray[zoneIndex].nvZoneProperties->deviceType != SHADE_TYPE) &&
          (zoneArray[zoneIndex].nvZoneProperties->deviceType != SHADE_GROUP_TYPE))
      {
         zoneFound = TRUE;
      }
   }

   return zoneFound;

}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetAvailableZoneIndex
//
// Returns the zone index for the next available slot or MAX_NUMBER_OF_ZONES
// if there are no available slots
//
//------------------------------------------------------------------------------
byte_t GetAvailableZoneIndex()
{
   byte_t zoneIndex;

   for (zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      if ((zoneArray[zoneIndex].nvZoneProperties) &&
          (zoneArray[zoneIndex].nvZoneProperties->ZoneNameString[0] == 0))
      {
         break;
      }
      else if (!zoneArray[zoneIndex].nvZoneProperties)
      {
         break;
      }
   }

   return zoneIndex;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetZoneIndex
//
// Returns the zone index that corresponds to the provided group, building,
// and house Ids or MAX_NUMBER_OF_ZONES if no zone is found 
//
//------------------------------------------------------------------------------

byte_t GetZoneIndex(word_t groupId, byte_t buildingId, byte_t houseId)
{
   byte_t zoneIndex;

   for (zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      if (zoneArray[zoneIndex].nvZoneProperties)
      {
         if ((zoneArray[zoneIndex].nvZoneProperties->ZoneNameString[0] != 0) && 
             (zoneArray[zoneIndex].nvZoneProperties->groupId == groupId) &&
             (zoneArray[zoneIndex].nvZoneProperties->buildingId == buildingId) &&
             (zoneArray[zoneIndex].nvZoneProperties->houseId == houseId))
         {
            break;
         }
      }
   }

   return zoneIndex;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetZoneProperties
// 
// Returns TRUE and the zone properties for the provided index
// Returns FALSE on error
//
// -----------------------------------------------------------------------------

bool_t GetZoneProperties(byte_t zoneIndex, zone_properties *zonePropertiesPtr)
{
   bool_t propertiesReturned = FALSE;

   if((zoneIndex >= 0) &&
      (zoneIndex < MAX_NUMBER_OF_ZONES) &&
      (zoneArray[zoneIndex].nvZoneProperties))
   {
      zonePropertiesPtr->AppContextId = zoneArray[zoneIndex].AppContextId;
      zonePropertiesPtr->groupId = zoneArray[zoneIndex].nvZoneProperties->groupId;
      zonePropertiesPtr->state = zoneArray[zoneIndex].state;
      zonePropertiesPtr->targetState = zoneArray[zoneIndex].targetState;
      zonePropertiesPtr->powerLevel = zoneArray[zoneIndex].powerLevel;
      zonePropertiesPtr->targetPowerLevel = zoneArray[zoneIndex].targetPowerLevel;
      zonePropertiesPtr->rampRate = zoneArray[zoneIndex].nvZoneProperties->rampRate;
      zonePropertiesPtr->deviceType = zoneArray[zoneIndex].nvZoneProperties->deviceType;
      zonePropertiesPtr->buildingId = zoneArray[zoneIndex].nvZoneProperties->buildingId;
      zonePropertiesPtr->houseId = zoneArray[zoneIndex].nvZoneProperties->houseId;
      zonePropertiesPtr->roomId = zoneArray[zoneIndex].nvZoneProperties->roomId;
      snprintf(zonePropertiesPtr->ZoneNameString, sizeof(zonePropertiesPtr->ZoneNameString), "%s", zoneArray[zoneIndex].nvZoneProperties->ZoneNameString);

      if(zoneArray[zoneIndex].nvEliotZoneProperties)
      {
         snprintf(zonePropertiesPtr->EliotDeviceId, sizeof(zonePropertiesPtr->EliotDeviceId), "%s", zoneArray[zoneIndex].nvEliotZoneProperties->EliotDeviceId);
      }
      else
      {
         memset(zonePropertiesPtr->EliotDeviceId, 0x00, sizeof(zonePropertiesPtr->EliotDeviceId));
      }

      propertiesReturned = TRUE;
   }

   return propertiesReturned;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// SetZoneProperties
//
// Returns TRUE if the properties were able to be set for the provided index
// Returns FALSE on error
// -----------------------------------------------------------------------------

bool_t SetZoneProperties(byte_t zoneIndex, zone_properties *zonePropertiesPtr)
{
   bool_t propertiesSet = FALSE;

   if((zoneIndex >= 0) &&
      (zoneIndex < MAX_NUMBER_OF_ZONES))
   {
      // Get a pointer to the non volatile zone properties for this index
      nonvolatile_zone_properties * nvZonePropertiesPtr = GetNVZonePropertiesPtr(zoneIndex, false);
   
      // Get a pointer to the eliot properties for this index
      nonvolatile_eliot_zone_properties *nvEliotZonePropertiesPtr = GetNVEliotZonePropertiesPtr(zoneIndex, false);

      if(nvZonePropertiesPtr && nvEliotZonePropertiesPtr)
      {
         zoneArray[zoneIndex].AppContextId = zonePropertiesPtr->AppContextId;
         zoneArray[zoneIndex].state = zonePropertiesPtr->state;
         zoneArray[zoneIndex].targetState = zonePropertiesPtr->targetState;
         zoneArray[zoneIndex].powerLevel = zonePropertiesPtr->powerLevel;
         zoneArray[zoneIndex].targetPowerLevel = zonePropertiesPtr->targetPowerLevel;
         zoneArray[zoneIndex].nvZoneProperties = nvZonePropertiesPtr;
         zoneArray[zoneIndex].nvEliotZoneProperties = nvEliotZonePropertiesPtr;

         // Check if this zone is initialized
         // Iterate over all of the bytes in memory and check if they are all
         // set to 0xFF. If so, the memory is not initialized. If a byte other
         // than 0xFF is found, the memory has been set to something else and
         // further checks will be needed
         // Equivalent to the following
         //    nonvolatile_zone_properties uninitialized;
         //    memset(&uninitialized, 0xFF, sizeof(uninitialized));
         //    if(memcmp(nvZonePropertiesPtr, &uninitialized, sizeof(uninitialized)) == 0)
         bool_t memoryInitialized = FALSE;
         byte_t * memoryByte = (byte_t *)nvZonePropertiesPtr;

         for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_zone_properties)) && (!memoryInitialized); byteIndex++, memoryByte++)
         {
            if(*memoryByte != 0xFF)
            {
               memoryInitialized = TRUE;
            }
         }

         if(!memoryInitialized)
         {
            // Zone is not initialized. No need to erase flash, we
            // can just write the bytes to flash at the index

            if(zonePropertiesPtr->ZoneNameString[0])
            {
               nonvolatile_zone_properties nvZoneProperties;
               nvZoneProperties.groupId = zonePropertiesPtr->groupId;
               nvZoneProperties.deviceType = zonePropertiesPtr->deviceType;
               nvZoneProperties.rampRate = zonePropertiesPtr->rampRate;
               nvZoneProperties.buildingId = zonePropertiesPtr->buildingId;
               nvZoneProperties.houseId = zonePropertiesPtr->houseId;
               nvZoneProperties.roomId = zonePropertiesPtr->roomId;
               snprintf(nvZoneProperties.ZoneNameString, sizeof(nvZoneProperties.ZoneNameString), "%s", zonePropertiesPtr->ZoneNameString);

               if(WriteZonePropertiesToFlash(zoneIndex, &nvZoneProperties) != FTFx_OK)
               {
                  // Error writing the zone properties to flash. Set the zone as invalid
                  zoneArray[zoneIndex].nvZoneProperties = NULL;
               }

               memoryByte = (byte_t *)nvEliotZonePropertiesPtr;
               for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_eliot_zone_properties)) && (!memoryInitialized); byteIndex++, memoryByte++)
               {
                  if(*memoryByte != 0xFF)
                  {
                     memoryInitialized = TRUE;
                  }
               }

               if(!memoryInitialized)
               {
                  if(zonePropertiesPtr->EliotDeviceId[0])
                  {
                     nonvolatile_eliot_zone_properties nvEliotZoneProperties;
                     snprintf(nvEliotZoneProperties.EliotDeviceId, sizeof(nvEliotZoneProperties.EliotDeviceId), "%s", zonePropertiesPtr->EliotDeviceId);

                     if(WriteEliotZonePropertiesToFlash(zoneIndex, &nvEliotZoneProperties) != FTFx_OK)
                     {
                        // Error writing the eliot properties to flash. Set the pointer to null
                        zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
                     }
                  }
                  else
                  {
                     // Eliot Zone device ID string is NULL so this id was deleted
                     zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
                  }
               }
               else
               {
                  zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
               }
            }
            else
            {
               // Zone name string is NULL so this zone was deleted
               // Set the non volatile zone properties pointer to NULL so 
               // GetZoneProperties will return invalid data for this zone
               zoneArray[zoneIndex].nvZoneProperties = NULL;
               zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
            }
         }
         else
         {
            // Zone was already initialized. Check if any of the data is
            // different and needs to be saved
            if((zonePropertiesPtr->groupId != nvZonePropertiesPtr->groupId) ||
               (zonePropertiesPtr->deviceType != nvZonePropertiesPtr->deviceType) ||
               (zonePropertiesPtr->rampRate != nvZonePropertiesPtr->rampRate) ||
               (zonePropertiesPtr->buildingId != nvZonePropertiesPtr->buildingId) ||
               (zonePropertiesPtr->houseId != nvZonePropertiesPtr->houseId) ||
               (zonePropertiesPtr->roomId != nvZonePropertiesPtr->roomId) ||
               (strncmp(zonePropertiesPtr->ZoneNameString, nvZonePropertiesPtr->ZoneNameString, sizeof(zonePropertiesPtr->ZoneNameString)) != 0))
            {
               // Save data from flash for all zones to an array in RAM
               ReadAllZonePropertiesFromFlash(nvZoneArray, MAX_NUMBER_OF_ZONES);

               if(zonePropertiesPtr->ZoneNameString[0])
               {
                  // Replace data with the current information for this zone index
                  nvZoneArray[zoneIndex].groupId = zonePropertiesPtr->groupId;
                  nvZoneArray[zoneIndex].rampRate = zonePropertiesPtr->rampRate;
                  nvZoneArray[zoneIndex].deviceType = zonePropertiesPtr->deviceType;
                  nvZoneArray[zoneIndex].buildingId = zonePropertiesPtr->buildingId;
                  nvZoneArray[zoneIndex].houseId = zonePropertiesPtr->houseId;
                  nvZoneArray[zoneIndex].roomId = zonePropertiesPtr->roomId;
                  snprintf(nvZoneArray[zoneIndex].ZoneNameString, sizeof(nvZoneArray[zoneIndex].ZoneNameString), "%s", zonePropertiesPtr->ZoneNameString);
               }
               else
               {
                  // Zone name string is NULL so this zone was deleted. Clear the data
                  memset(&nvZoneArray[zoneIndex], 0xFF, sizeof(nvZoneArray[zoneIndex]));

                  // Set the non volatile zone properties pointer to NULL so GetZoneProperties will return invalid data for this zone
                  zoneArray[zoneIndex].nvZoneProperties = NULL;
               }

               // Erase the entire sector of flash for all zones
               EraseZonesFromFlash();

               // Write updated zone data to flash for all zones
               WriteAllZonePropertiesToFlash(nvZoneArray, MAX_NUMBER_OF_ZONES);
            }
            // else zone is identical. No need to save
            
            // Determine if the Eliot Zone Properties are identical or need to be saved
            if(strncmp(zonePropertiesPtr->EliotDeviceId, nvEliotZonePropertiesPtr->EliotDeviceId, sizeof(zonePropertiesPtr->EliotDeviceId)) != 0)
            {
               // Calculate the zone index in the sector
               byte_t zoneIndexInSector = (zoneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

               // Save data from flash for all eliot properties to an array in RAM
               ReadEliotZonePropertiesFromFlashSector(zoneIndex, nvEliotZoneArray, NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

               if(zonePropertiesPtr->EliotDeviceId[0])
               {
                  // Replace data with the current information for this zone index
                  snprintf(nvEliotZoneArray[zoneIndexInSector].EliotDeviceId, sizeof(nvEliotZoneArray[zoneIndexInSector].EliotDeviceId), "%s", zonePropertiesPtr->EliotDeviceId);
               }
               else
               {
                  // Eliot Zone Device ID is NULL so this zone was deleted. Clear the data
                  memset(&nvEliotZoneArray[zoneIndexInSector], 0xFF, sizeof(nvEliotZoneArray[zoneIndexInSector]));

                  // Set the non volatile zone properties pointer to NULL
                  zoneArray[zoneIndex].nvEliotZoneProperties = NULL;
               }

               // Erase the entire sector of flash for all zones in the sector
               EraseEliotZonePropertiesFromFlash(zoneIndex);

               // Write updated eliot properties to flash for all zones in the sector
               WriteEliotZonePropertiesToFlashSector(zoneIndex, nvEliotZoneArray, NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
            }
            // else zone is identical. No need to save
         }

         propertiesSet = true;
      }
   }

   return propertiesSet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

uint8_t LoadZonesFromFlash()
{
   uint8_t numZonesLoaded = 0;
   nonvolatile_zone_properties *nvZonePropertiesPtr;
   nonvolatile_eliot_zone_properties *nvEliotZonePropertiesPtr;

   for(byte_t zoneIndex = 0;  zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      nvZonePropertiesPtr = GetNVZonePropertiesPtr(zoneIndex, true);
      nvEliotZonePropertiesPtr = GetNVEliotZonePropertiesPtr(zoneIndex, true);

      if(nvZonePropertiesPtr)
      {
         if(global_houseID == 0)
         {
            global_houseID = nvZonePropertiesPtr->houseId;
         }

         zoneArray[zoneIndex].nvZoneProperties = nvZonePropertiesPtr;
         zoneArray[zoneIndex].nvEliotZoneProperties = nvEliotZonePropertiesPtr;
         numZonesLoaded++;
      }
   }

   return numZonesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

uint8_t GetZoneIndexFromDeviceId(const char * deviceId)
{
   uint8_t zoneIndex = 0;
   nonvolatile_eliot_zone_properties * nvEliotZonePropertiesPtr;

   for (zoneIndex = 0;  zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      nvEliotZonePropertiesPtr = GetNVEliotZonePropertiesPtr(zoneIndex, true);

      if (nvEliotZonePropertiesPtr)
      {
         if (!strcmp(nvEliotZonePropertiesPtr->EliotDeviceId, deviceId))
         {
            return zoneIndex;
         }
      }
   }

   return zoneIndex;
   
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
