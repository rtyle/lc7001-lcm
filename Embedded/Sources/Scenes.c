#include "includes.h"
#include "swap_demo.h"

volatile_scene_properties            sceneArray[MAX_NUMBER_OF_SCENES];
nonvolatile_scene_properties         nvSceneArray[NUM_SCENES_IN_FLASH_SECTOR];
nonvolatile_eliot_scene_properties nvEliotSceneArray[NUM_SCENES_IN_FLASH_SECTOR];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// InitializeSceneArray
//
// Initializes the scene array to use the default values
//
//------------------------------------------------------------------------------

void InitializeSceneArray(void)
{
   // This is the default data. This will be overwritten by non-volatile data
   byte_t sceneIndex;

   for (sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
   {
      // Initialize the volatile data
      sceneArray[sceneIndex].running = FALSE;
      sceneArray[sceneIndex].nvSceneProperties = NULL;
      sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetAvailableSceneIndex
//
// Returns the scene index for the next available slot or MAX_NUMBER_OF_SCENES
// if there are no available slots
//
//------------------------------------------------------------------------------
byte_t GetAvailableSceneIndex()
{
   byte_t sceneIndex;

   for (sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
   {
      if ((sceneArray[sceneIndex].nvSceneProperties) &&
          (sceneArray[sceneIndex].nvSceneProperties->SceneNameString[0] == 0))
      {
         break;
      }
      else if (!sceneArray[sceneIndex].nvSceneProperties)
      {
         break;
      }
   }

   return sceneIndex;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// GetSceneProperties
// 
// Returns TRUE and the scene properties for the provided index
// Returns FALSE on error
//
// -----------------------------------------------------------------------------

bool_t GetSceneProperties(byte_t sceneIndex, scene_properties *scenePropertiesPtr)
{
   bool_t propertiesReturned = FALSE;

   if((sceneIndex >= 0) &&
      (sceneIndex < MAX_NUMBER_OF_SCENES) &&
      (sceneArray[sceneIndex].nvSceneProperties))
   {
      scenePropertiesPtr->running = sceneArray[sceneIndex].running;
      for(int zoneIndex = 0; zoneIndex < sceneArray[sceneIndex].nvSceneProperties->numZonesInScene; zoneIndex++)
      {
         scenePropertiesPtr->zoneData[zoneIndex].zoneId = sceneArray[sceneIndex].nvSceneProperties->zoneData[zoneIndex].zoneId;
         scenePropertiesPtr->zoneData[zoneIndex].powerLevel = sceneArray[sceneIndex].nvSceneProperties->zoneData[zoneIndex].powerLevel;
         scenePropertiesPtr->zoneData[zoneIndex].rampRate = sceneArray[sceneIndex].nvSceneProperties->zoneData[zoneIndex].rampRate;
         scenePropertiesPtr->zoneData[zoneIndex].state = sceneArray[sceneIndex].nvSceneProperties->zoneData[zoneIndex].state;
      }
      scenePropertiesPtr->numZonesInScene = sceneArray[sceneIndex].nvSceneProperties->numZonesInScene;
      scenePropertiesPtr->nextTriggerTime = sceneArray[sceneIndex].nvSceneProperties->nextTriggerTime;
      scenePropertiesPtr->freq = sceneArray[sceneIndex].nvSceneProperties->freq;
      scenePropertiesPtr->trigger = sceneArray[sceneIndex].nvSceneProperties->trigger;
      scenePropertiesPtr->dayBitMask.days = sceneArray[sceneIndex].nvSceneProperties->dayBitMask.days;
      scenePropertiesPtr->delta = sceneArray[sceneIndex].nvSceneProperties->delta;
      scenePropertiesPtr->skipNext = sceneArray[sceneIndex].nvSceneProperties->skipNext;
      snprintf(scenePropertiesPtr->SceneNameString, sizeof(scenePropertiesPtr->SceneNameString), "%s", sceneArray[sceneIndex].nvSceneProperties->SceneNameString);

      if(sceneArray[sceneIndex].nvEliotSceneProperties)
      {
         snprintf(scenePropertiesPtr->EliotDeviceId, sizeof(scenePropertiesPtr->EliotDeviceId), "%s", sceneArray[sceneIndex].nvEliotSceneProperties->EliotDeviceId);
      }
      else
      {
         memset(scenePropertiesPtr->EliotDeviceId, 0x00, sizeof(scenePropertiesPtr->EliotDeviceId));
      }

      propertiesReturned = TRUE;
   }

   return propertiesReturned;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// SetSceneProperties
//
// Returns TRUE if the properties were able to be set for the provided index
// Returns FALSE on error
// -----------------------------------------------------------------------------

bool_t SetSceneProperties(byte_t sceneIndex, scene_properties *scenePropertiesPtr)
{
   bool_t propertiesSet = FALSE;

   if((sceneIndex >= 0) &&
      (sceneIndex < MAX_NUMBER_OF_SCENES))
   {
      // Get a pointer to the non volatile scene properties for this index
      nonvolatile_scene_properties * nvScenePropertiesPtr = GetNVScenePropertiesPtr(sceneIndex, false);
   
      // Get a pointer to the eliot scene properties for this index
      nonvolatile_eliot_scene_properties *nvEliotScenePropertiesPtr = GetNVEliotScenePropertiesPtr(sceneIndex, false);

      if(nvScenePropertiesPtr && nvEliotScenePropertiesPtr)
      {
         sceneArray[sceneIndex].running = scenePropertiesPtr->running;
         sceneArray[sceneIndex].nvSceneProperties = nvScenePropertiesPtr;
         sceneArray[sceneIndex].nvEliotSceneProperties = nvEliotScenePropertiesPtr;

         // Check if this scene is initialized
         // Iterate over all of the bytes in memory and check if they are all
         // set to 0xFF. If so, the memory is not initialized. If a byte other
         // than 0xFF is found, the memory has been set to something else and
         // further checks will be needed
         // Equivalent to the following
         //    nonvolatile_zone_properties uninitialized;
         //    memset(&uninitialized, 0xFF, sizeof(uninitialized));
         //    if(memcmp(nvZonePropertiesPtr, &uninitialized, sizeof(uninitialized)) == 0)
         bool_t memoryInitialized = FALSE;
         byte_t * memoryByte = (byte_t *)nvScenePropertiesPtr;

         for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_scene_properties)) && (!memoryInitialized); byteIndex++, memoryByte++)
         {
            if(*memoryByte != 0xFF)
            {
               memoryInitialized = TRUE;
            }
         }

         if(!memoryInitialized)
         {
            // Scene is not initialized. No need to erase flash, we
            // can just write the bytes to flash at the index
            
            if(scenePropertiesPtr->SceneNameString[0])
            {
               nonvolatile_scene_properties nvSceneProperties;
               nvSceneProperties.numZonesInScene = scenePropertiesPtr->numZonesInScene;
               for(int zoneIndex = 0; zoneIndex < scenePropertiesPtr->numZonesInScene; zoneIndex++)
               {
                  nvSceneProperties.zoneData[zoneIndex].zoneId = scenePropertiesPtr->zoneData[zoneIndex].zoneId;
                  nvSceneProperties.zoneData[zoneIndex].powerLevel = scenePropertiesPtr->zoneData[zoneIndex].powerLevel;
                  nvSceneProperties.zoneData[zoneIndex].rampRate = scenePropertiesPtr->zoneData[zoneIndex].rampRate;
                  nvSceneProperties.zoneData[zoneIndex].state = scenePropertiesPtr->zoneData[zoneIndex].state;
               }
               nvSceneProperties.nextTriggerTime = scenePropertiesPtr->nextTriggerTime;
               nvSceneProperties.freq = scenePropertiesPtr->freq;
               nvSceneProperties.trigger = scenePropertiesPtr->trigger;
               nvSceneProperties.dayBitMask.days = scenePropertiesPtr->dayBitMask.days;
               nvSceneProperties.delta = scenePropertiesPtr->delta;
               nvSceneProperties.skipNext = scenePropertiesPtr->skipNext;
               snprintf(nvSceneProperties.SceneNameString, sizeof(nvSceneProperties.SceneNameString), "%s", scenePropertiesPtr->SceneNameString);
               if(WriteScenePropertiesToFlash(sceneIndex, &nvSceneProperties) != FTFx_OK)
               {
                  // Error writing the scene properties to flash. Set the scene as invalid
                  sceneArray[sceneIndex].nvSceneProperties = NULL;
               }

               memoryByte = (byte_t *)nvEliotScenePropertiesPtr;
               for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_eliot_scene_properties)) && (!memoryInitialized); byteIndex++, memoryByte++)
               {
                  if(*memoryByte != 0xFF)
                  {
                     memoryInitialized = TRUE;
                  }
               }

               if(!memoryInitialized)
               {
                  if(scenePropertiesPtr->EliotDeviceId[0])
                  {
                     nonvolatile_eliot_scene_properties nvEliotSceneProperties;

                     snprintf(nvEliotSceneProperties.EliotDeviceId, sizeof(nvEliotSceneProperties.EliotDeviceId), "%s", scenePropertiesPtr->EliotDeviceId);

                     if(WriteEliotScenePropertiesToFlash(sceneIndex, &nvEliotSceneProperties) != FTFx_OK)
                     {
                        // Error writing the eliot properties to flash. Set the pointer to null
                        sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
                     }
                  }
                  else
                  {
                     // Eliot Scene device ID string is NULL so this id was deleted
                     sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
                  }
               }
               else
               {
                  sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
               }
            }
            else
            {
               // Scene name string is NULL so the scene was deleted
               // Set the non volatile scene properties pointer to NULL so
               // GetSceneProperties will return invalid data for this scene
               sceneArray[sceneIndex].nvSceneProperties = NULL;
               sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
            }
         }
         else
         {
            // Scene was already initialized. Check if any of the data is
            // different and needs to be saved
            bool_t zoneDataIsDifferent = false;

            if(scenePropertiesPtr->numZonesInScene != nvScenePropertiesPtr->numZonesInScene)
            {
               zoneDataIsDifferent = true;
            }

            for(int zoneIndex = 0; (zoneIndex < scenePropertiesPtr->numZonesInScene) && !zoneDataIsDifferent; zoneIndex++)
            {
               if((scenePropertiesPtr->zoneData[zoneIndex].zoneId != nvScenePropertiesPtr->zoneData[zoneIndex].zoneId) ||
                     (scenePropertiesPtr->zoneData[zoneIndex].powerLevel != nvScenePropertiesPtr->zoneData[zoneIndex].powerLevel) ||
                     (scenePropertiesPtr->zoneData[zoneIndex].rampRate != nvScenePropertiesPtr->zoneData[zoneIndex].rampRate) ||
                     (scenePropertiesPtr->zoneData[zoneIndex].state != nvScenePropertiesPtr->zoneData[zoneIndex].state))
               {
                  zoneDataIsDifferent = true;
               }
            }

            if((zoneDataIsDifferent) ||
               (scenePropertiesPtr->nextTriggerTime != nvScenePropertiesPtr->nextTriggerTime) ||
               (scenePropertiesPtr->freq != nvScenePropertiesPtr->freq) ||
               (scenePropertiesPtr->trigger != nvScenePropertiesPtr->trigger) ||
               (scenePropertiesPtr->dayBitMask.days != nvScenePropertiesPtr->dayBitMask.days) ||
               (scenePropertiesPtr->delta != nvScenePropertiesPtr->delta) ||
               (scenePropertiesPtr->skipNext != nvScenePropertiesPtr->skipNext) ||
               (strncmp(scenePropertiesPtr->SceneNameString, nvScenePropertiesPtr->SceneNameString, sizeof(scenePropertiesPtr->SceneNameString)) != 0))
            {
               // Calculate the scene index in the sector
               byte_t sceneIndexInSector = (sceneIndex % NUM_SCENES_IN_FLASH_SECTOR);

               // Save data from flash for all scenes in a sector to an array in RAM
               ReadScenePropertiesFromFlashSector(sceneIndex, nvSceneArray, NUM_SCENES_IN_FLASH_SECTOR);

               if(scenePropertiesPtr->SceneNameString[0])
               {
                  // Replace data with the current information for this scene index in the sector
                  nvSceneArray[sceneIndexInSector].numZonesInScene = scenePropertiesPtr->numZonesInScene;
                  for(int zoneIndex = 0; zoneIndex < scenePropertiesPtr->numZonesInScene; zoneIndex++)
                  {
                     nvSceneArray[sceneIndexInSector].zoneData[zoneIndex].zoneId = scenePropertiesPtr->zoneData[zoneIndex].zoneId;
                     nvSceneArray[sceneIndexInSector].zoneData[zoneIndex].powerLevel = scenePropertiesPtr->zoneData[zoneIndex].powerLevel;
                     nvSceneArray[sceneIndexInSector].zoneData[zoneIndex].rampRate = scenePropertiesPtr->zoneData[zoneIndex].rampRate;
                     nvSceneArray[sceneIndexInSector].zoneData[zoneIndex].state = scenePropertiesPtr->zoneData[zoneIndex].state;
                  }
                  nvSceneArray[sceneIndexInSector].nextTriggerTime = scenePropertiesPtr->nextTriggerTime;
                  nvSceneArray[sceneIndexInSector].freq = scenePropertiesPtr->freq;
                  nvSceneArray[sceneIndexInSector].trigger = scenePropertiesPtr->trigger;
                  nvSceneArray[sceneIndexInSector].dayBitMask.days = scenePropertiesPtr->dayBitMask.days;
                  nvSceneArray[sceneIndexInSector].delta = scenePropertiesPtr->delta;
                  nvSceneArray[sceneIndexInSector].skipNext = scenePropertiesPtr->skipNext;
                  snprintf(nvSceneArray[sceneIndexInSector].SceneNameString, sizeof(nvSceneArray[sceneIndexInSector].SceneNameString), "%s", scenePropertiesPtr->SceneNameString);
               }
               else
               {
                  // Scene name string is NULL so this scene was deleted. Clear the data
                  memset(&nvSceneArray[sceneIndexInSector], 0xFF, sizeof(nvSceneArray[sceneIndexInSector]));

                  // Set the non volatile scene properties pointer to NULL so GetSceneProperties will return invalid data for this scene
                  sceneArray[sceneIndex].nvSceneProperties = NULL;
               }

               // Erase the entire sector of flash for all scenes in a sector
               EraseScenesFromFlash(sceneIndex);

               // Write updated scene data to flash for all scenes in a sector
               WriteScenePropertiesToFlashSector(sceneIndex, nvSceneArray, NUM_SCENES_IN_FLASH_SECTOR);
            }
            // else scene is identical. No need to save
            
            // Determine if the Eliot Scene Properties are identical or need to be saved
            if(strncmp(scenePropertiesPtr->EliotDeviceId, nvEliotScenePropertiesPtr->EliotDeviceId, sizeof(scenePropertiesPtr->EliotDeviceId)) != 0)
            {
               // Calculate the scene index in the sector
               byte_t sceneIndexInSector = (sceneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

               // Save data from flash for all eliot properties to an array in RAM
               ReadEliotScenePropertiesFromFlashSector(sceneIndex, nvEliotSceneArray, NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

               if(scenePropertiesPtr->EliotDeviceId[0])
               {
                  // Replace data with the current information for this scene index
                  snprintf(nvEliotSceneArray[sceneIndexInSector].EliotDeviceId, sizeof(nvEliotSceneArray[sceneIndexInSector].EliotDeviceId), "%s", scenePropertiesPtr->EliotDeviceId);
               }
               else
               {
                  // Eliot Scene Device ID is NULL so this scene was deleted. Clear the data
                  memset(&nvEliotSceneArray[sceneIndexInSector], 0xFF, sizeof(nvEliotSceneArray[sceneIndexInSector]));

                  // Set the non volatile scene properties pointer to NULL
                  sceneArray[sceneIndex].nvEliotSceneProperties = NULL;
               }

               // Erase the entire sector of flash for all scenes in the sector
               EraseEliotScenePropertiesFromFlash(sceneIndex);

               // Write updated eliot properties to flash for all scenes in the sector
               WriteEliotScenePropertiesToFlashSector(sceneIndex, nvEliotSceneArray, NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
            }
            // else scene is identical. No need to save
         }

         propertiesSet = true;
      }
   }

   return propertiesSet;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

uint8_t LoadScenesFromFlash()
{
   uint8_t numScenesLoaded = 0;
   nonvolatile_scene_properties * nvScenePropertiesPtr;
   nonvolatile_eliot_scene_properties *nvEliotScenePropertiesPtr;

   for(byte_t sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
   {
      nvScenePropertiesPtr = GetNVScenePropertiesPtr(sceneIndex, true);
      nvEliotScenePropertiesPtr = GetNVEliotScenePropertiesPtr(sceneIndex, true);

      if(nvScenePropertiesPtr)
      {
         sceneArray[sceneIndex].nvSceneProperties = nvScenePropertiesPtr;
         sceneArray[sceneIndex].nvEliotSceneProperties = nvEliotScenePropertiesPtr;
         numScenesLoaded++;
      }
   }

   return numScenesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

uint8_t GetSceneIndexFromDeviceId(const char * deviceId)
{
   uint8_t sceneIndex = 0;
   nonvolatile_eliot_scene_properties * nvEliotScenePropertiesPtr;

   for (sceneIndex = 0;  sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
   {
      nvEliotScenePropertiesPtr = GetNVEliotScenePropertiesPtr(sceneIndex, true);

      if (nvEliotScenePropertiesPtr)
      {
         if (!strcmp(nvEliotScenePropertiesPtr->EliotDeviceId, deviceId))
         {
            return sceneIndex;
         }
      }
   }

   return sceneIndex;
   
}
