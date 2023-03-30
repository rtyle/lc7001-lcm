#include "includes.h"
#include "swap_demo.h"

volatile_scene_controller_properties    sceneControllerArray[MAX_NUMBER_OF_SCENE_CONTROLLERS];
nonvolatile_scene_controller_properties nvSceneControllerArray[MAX_NUMBER_OF_SCENE_CONTROLLERS];

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// InitializeSceneControllerArray
//
// Initializes the scene controller array to use the default values
//
//------------------------------------------------------------------------------

void InitializeSceneControllerArray(void)
{
   byte_t sceneControllerIndex;
   
   for (sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
   {
      sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = NULL;
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  AnySceneControllersInArray()
//
//  returns True if there are any scene controllers in the array
//
//------------------------------------------------------------------------------

bool_t AnySceneControllersInArray()
{
   bool_t sceneControllerFound = FALSE;

   for (int sceneControllerIndex = 0; (sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS) && (!sceneControllerFound); sceneControllerIndex++)
   {
      if ((sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties) &&
          (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->SceneControllerNameString[0] != 0))
      {
         sceneControllerFound = TRUE;
      }
   }

   return sceneControllerFound;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t GetAvailableSceneControllerIndex(void)
{
   byte_t sceneControllerIndex;

   for(sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
   {
      if ((sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties) &&
          (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->SceneControllerNameString[0] == 0))
      {
         break;
      }
      else if (!sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties)
      {
         break;
      }
   }

   return sceneControllerIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetSceneControllerIndex
//
// Returns the Scene Controller index that corresponds to the provided group, building,
// and house Ids or MAX_NUMBER_OF_SCENE_CONTROLLERS if no Scene Controller is found 
//
//------------------------------------------------------------------------------

byte_t GetSceneControllerIndex(word_t groupId, byte_t buildingId, byte_t houseId)
{
   byte_t sceneControllerIndex;

   for (sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
   {
      if (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties)
      {
         if ((sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->SceneControllerNameString[0] != 0) &&
             (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->groupId == groupId) &&
             (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->buildingId == buildingId) &&
             (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->houseId == houseId))
         {
            break;
         }
      }
   }

   return sceneControllerIndex;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GetSceneControllerProperties
// 
// Returns TRUE and the sctl properties for the provided index
// Returns FALSE on error
//
// -----------------------------------------------------------------------------

bool_t GetSceneControllerProperties(byte_t sceneControllerIndex, scene_controller_properties *sceneControllerPropertiesPtr)
{

   bool_t propertiesReturned = FALSE;
   byte_t button;
   
   if((sceneControllerIndex >= 0) &&
      (sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS) &&
      (sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties))
   {
      sceneControllerPropertiesPtr->groupId = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->groupId;
      sceneControllerPropertiesPtr->buildingId = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->buildingId;
      sceneControllerPropertiesPtr->houseId = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->houseId;
      sceneControllerPropertiesPtr->roomId = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->roomId;
      sceneControllerPropertiesPtr->nightMode = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->nightMode;
      sceneControllerPropertiesPtr->deviceType = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->deviceType;
  
      for (button = 0; button < MAX_SCENES_IN_SCENE_CONTROLLER; button++)
      {
         sceneControllerPropertiesPtr->bank[button].sceneId = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->bank[button].sceneId;
         sceneControllerPropertiesPtr->bank[button].toggleable = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->bank[button].toggleable;
         sceneControllerPropertiesPtr->bank[button].toggled = sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->bank[button].toggled;
      }
      snprintf(sceneControllerPropertiesPtr->SceneControllerNameString, sizeof(sceneControllerPropertiesPtr->SceneControllerNameString), "%s", sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties->SceneControllerNameString);
      
      propertiesReturned = TRUE;
   }

   return propertiesReturned;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t SetSceneControllerProperties(byte_t sceneControllerIndex, scene_controller_properties *sceneControllerPropertiesPtr)
{
   byte_t button;
   bool_t propertiesSet = FALSE;
   
   if((sceneControllerIndex >= 0) &&
       (sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS))
    {
       // Get a pointer to the non volatile scene controller properties for this index
       nonvolatile_scene_controller_properties *nvSceneControllerPropertiesPtr = GetNVSceneControllerPropertiesPtr(sceneControllerIndex, false);

       if(nvSceneControllerPropertiesPtr)
       {
          sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = nvSceneControllerPropertiesPtr;

          // Check if this scene controller is initialized
          // Iterate over all of the bytes in memory and check if they are all
          // set to 0xFF. If so, the memory is not initialized. If a byte other
          // than 0xFF is found, the memory has been set to something else and
          // further checks will be needed.
          bool_t memoryInitialized = FALSE;
          byte_t * memoryByte = (byte_t *)nvSceneControllerPropertiesPtr;

          for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_scene_controller_properties)) && (!memoryInitialized); byteIndex++, memoryByte++)
          {
             if(*memoryByte != 0xFF)
             {
                memoryInitialized = TRUE;
             }
          }

          if(!memoryInitialized)
          {
             // Scene controller is not initialized. No need to erase flash,
             // we can just write the bytes to flash at the index
             if(sceneControllerPropertiesPtr->SceneControllerNameString[0])
             {
                nonvolatile_scene_controller_properties nvSceneControllerProperties;
                nvSceneControllerProperties.groupId = sceneControllerPropertiesPtr->groupId;
                nvSceneControllerProperties.buildingId = sceneControllerPropertiesPtr->buildingId;
                nvSceneControllerProperties.houseId = sceneControllerPropertiesPtr->houseId;
                nvSceneControllerProperties.roomId = sceneControllerPropertiesPtr->roomId;
                nvSceneControllerProperties.nightMode = sceneControllerPropertiesPtr->nightMode;
                nvSceneControllerProperties.deviceType = sceneControllerPropertiesPtr->deviceType;

                for(button = 0; button < MAX_SCENES_IN_SCENE_CONTROLLER; button++)
                {
                   nvSceneControllerProperties.bank[button].sceneId = sceneControllerPropertiesPtr->bank[button].sceneId;
                   nvSceneControllerProperties.bank[button].toggleable = sceneControllerPropertiesPtr->bank[button].toggleable;
                   nvSceneControllerProperties.bank[button].toggled = sceneControllerPropertiesPtr->bank[button].toggled;
                }

                snprintf(nvSceneControllerProperties.SceneControllerNameString, sizeof(nvSceneControllerProperties.SceneControllerNameString), "%s", sceneControllerPropertiesPtr->SceneControllerNameString);

                if(WriteSceneControllerPropertiesToFlash(sceneControllerIndex, &nvSceneControllerProperties) != FTFx_OK)
                {
                   // Error writing the scene controller properties to flash. Set the scene controller as invalid
                   sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = NULL;
                }
             }
             else
             {
                // Scene controller name string is NULL so this scene controller was deleted
                // Set the non volatile scene controller properties pointer to NULL so
                // GetSceneControllerProperties will return invalid data for this scene controller
                sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = NULL;
             }
          }
          else
          {
             // Scene controller was already initialized. Check if any of the data is different
             // and needs to be saved
             bool_t bankDifferent = FALSE;
             for(button = 0; (!bankDifferent) && (button < MAX_SCENES_IN_SCENE_CONTROLLER); button++)
             {
                if((sceneControllerPropertiesPtr->bank[button].sceneId != nvSceneControllerPropertiesPtr->bank[button].sceneId) ||
                   (sceneControllerPropertiesPtr->bank[button].toggleable != nvSceneControllerPropertiesPtr->bank[button].toggleable) ||
                   (sceneControllerPropertiesPtr->bank[button].toggled != nvSceneControllerPropertiesPtr->bank[button].toggled) )
                {
                   bankDifferent = TRUE;
                }
             }

             if((bankDifferent) ||
                (sceneControllerPropertiesPtr->groupId != nvSceneControllerPropertiesPtr->groupId) ||
                (sceneControllerPropertiesPtr->buildingId != nvSceneControllerPropertiesPtr->buildingId) ||
                (sceneControllerPropertiesPtr->houseId != nvSceneControllerPropertiesPtr->houseId) ||
                (sceneControllerPropertiesPtr->roomId != nvSceneControllerPropertiesPtr->roomId) ||
                (sceneControllerPropertiesPtr->nightMode != nvSceneControllerPropertiesPtr->nightMode) ||
                (sceneControllerPropertiesPtr->deviceType != nvSceneControllerPropertiesPtr->deviceType) ||
                (strncmp(sceneControllerPropertiesPtr->SceneControllerNameString, nvSceneControllerPropertiesPtr->SceneControllerNameString, sizeof(sceneControllerPropertiesPtr->SceneControllerNameString)) != 0))
             {
                // Save data from flash for all scene controllers to an array in RAM
                ReadAllSceneControllerPropertiesFromFlash(nvSceneControllerArray, MAX_NUMBER_OF_SCENE_CONTROLLERS);

                if(sceneControllerPropertiesPtr->SceneControllerNameString[0])
                {
                   // Replace data with the current information for this scene controller index
                   for(button = 0; button < MAX_SCENES_IN_SCENE_CONTROLLER; button++)
                   {
                      nvSceneControllerArray[sceneControllerIndex].bank[button].sceneId = sceneControllerPropertiesPtr->bank[button].sceneId;
                      nvSceneControllerArray[sceneControllerIndex].bank[button].toggleable = sceneControllerPropertiesPtr->bank[button].toggleable;
                      nvSceneControllerArray[sceneControllerIndex].bank[button].toggled = sceneControllerPropertiesPtr->bank[button].toggled;
                   }

                   nvSceneControllerArray[sceneControllerIndex].groupId = sceneControllerPropertiesPtr->groupId;
                   nvSceneControllerArray[sceneControllerIndex].buildingId = sceneControllerPropertiesPtr->buildingId;
                   nvSceneControllerArray[sceneControllerIndex].houseId = sceneControllerPropertiesPtr->houseId;
                   nvSceneControllerArray[sceneControllerIndex].roomId = sceneControllerPropertiesPtr->roomId;
                   nvSceneControllerArray[sceneControllerIndex].nightMode = sceneControllerPropertiesPtr->nightMode;
                   nvSceneControllerArray[sceneControllerIndex].deviceType = sceneControllerPropertiesPtr->deviceType;
                  snprintf(nvSceneControllerArray[sceneControllerIndex].SceneControllerNameString, sizeof(nvSceneControllerArray[sceneControllerIndex].SceneControllerNameString), "%s", sceneControllerPropertiesPtr->SceneControllerNameString);
                }
                else
                {
                  // Scene Controller name string is NULL so this zone was deleted. Clear the data
                  memset(&nvSceneControllerArray[sceneControllerIndex], 0xFF, sizeof(nvSceneControllerArray[sceneControllerIndex]));

                  // Set the non volatile scene controller properties pointer to NULL so
                  // GetSceneControllerProperties will return invalid data for this index
                  sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = NULL;
                }

                // Erase the entire sector of flash for all scene controllers
                EraseSceneControllersFromFlash();

                // Write the updated scene controller data to flash for all scene controllers
                WriteAllSceneControllerPropertiesToFlash(nvSceneControllerArray, MAX_NUMBER_OF_SCENE_CONTROLLERS);
             }
             // else scene controller is identical. No need to save
          }

          propertiesSet = true;

       }
    }

   return propertiesSet;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint8_t LoadSceneControllersFromFlash()
{
   uint8_t numSceneControllersLoaded = 0;
   nonvolatile_scene_controller_properties *nvSceneControllerPropertiesPtr;

   for(byte_t sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
   {
      nvSceneControllerPropertiesPtr = GetNVSceneControllerPropertiesPtr(sceneControllerIndex, true);

      if(nvSceneControllerPropertiesPtr)
      {
         if(global_houseID == 0)
         {
            global_houseID = nvSceneControllerPropertiesPtr->houseId;
         }

         sceneControllerArray[sceneControllerIndex].nvSceneControllerProperties = nvSceneControllerPropertiesPtr;
         numSceneControllersLoaded++;
      }
   }

   return numSceneControllersLoaded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

