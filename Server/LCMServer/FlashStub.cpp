#include "FlashStub.h"
#include "Debug.h"
#include <fstream>
#include <sstream>
#include <vector>

using namespace legrand::rflc::scenes;

static FlashStub *flashStubInstance;

extern "C"
{
#include "globals.h"
#include "defines.h"
#include "Zones.h"
#include "Scenes.h"
#include "SceneController.h"
#include "swap_demo.h"

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   nonvolatile_zone_properties * GetNVZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData)
   {
      nonvolatile_zone_properties *nvZonePropertiesPtr = NULL;

      if(flashStubInstance)
      {
         nvZonePropertiesPtr = flashStubInstance->getNonvolatileZonePropertiesPointer(zoneIndex, validateZoneData);
      }

      return nvZonePropertiesPtr;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   uint32_t WriteZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_zone_properties * nvZonePropertiesPtr)
   {
      uint32_t returnCode = -1;

      if(flashStubInstance)
      {
         logDebug("Writing Zone Properties to flash");
         if(!flashStubInstance->insertDataInZoneArray(zoneIndex, nvZonePropertiesPtr))
         {
            logError("Error inserting data to the zone array");
         }
         else
         {
            if(!flashStubInstance->appendZoneToFile(zoneIndex))
            {
               logError("Error appending the zone to the file");
            }
            else
            {
               returnCode = FTFx_OK;
            }
         }
      }

      return returnCode;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void ReadAllZonePropertiesFromFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize)
   {
      if(flashStubInstance)
      {
         logInfo("Reading all Zone Properties from flash");
         flashStubInstance->copyDataFromZoneArray(nvZoneArray, arraySize);
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void WriteAllZonePropertiesToFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize)
   {
      if(flashStubInstance)
      {
         logInfo("Writing all zone properties to flash");
         flashStubInstance->copyDataToZoneArray(nvZoneArray, arraySize);

         if(!flashStubInstance->saveAllZonesToFile())
         {
            logError("Error saving zone data to file");
         }
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void EraseZonesFromFlash()
   {
      if(flashStubInstance)
      {
         logInfo("Erasing zones from flash");
         flashStubInstance->eraseZoneFile();
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   nonvolatile_samsung_zone_properties * GetNVSamsungZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData)
   {
      nonvolatile_samsung_zone_properties *nvSamsungZonePropertiesPtr = NULL;

      if(flashStubInstance)
      {
         nvSamsungZonePropertiesPtr = flashStubInstance->getNonvolatileSamsungZonePropertiesPointer(zoneIndex, validateZoneData);
      }

      return nvSamsungZonePropertiesPtr;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   uint32_t WriteSamsungZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_samsung_zone_properties * nvSamsungZonePropertiesPtr)
   {
      uint32_t returnCode = -1;

      if(flashStubInstance)
      {
         logInfo("Writing Samsung Zone Properties to flash %s", nvSamsungZonePropertiesPtr->SamsungDeviceId);
         if(!flashStubInstance->insertDataInSamsungZonePropertiesArray(zoneIndex, nvSamsungZonePropertiesPtr))
         {
            logError("Error inserting data to the samsung zone properties array");
         }
         else
         {
            if(!flashStubInstance->appendSamsungZonePropertiesToFile(zoneIndex))
            {
               logError("Error appending the samsung zone property to the file");
            }
            else
            {
               returnCode = FTFx_OK;
            }
         }
      }

      return returnCode;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   bool_t ReadSamsungZonePropertiesFromFlashSector(byte_t zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZoneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Reading Samsung Zone Properties from flash ");
         flashStubInstance->copyDataFromSamsungZonePropertiesArray(zoneIndex, nvSamsungZoneArray, arraySize);
      }

      return error;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   bool_t WriteSamsungZonePropertiesToFlashSector(byte_t zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZoneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Writing all Samsung Zone properties to flash for sector");
         error = flashStubInstance->copyDataToSamsungZonePropertiesArray(zoneIndex, nvSamsungZoneArray, arraySize);

         if(!flashStubInstance->saveAllSamsungZonePropertiesToFile())
         {
            logError("Error saving Samsung Zone properties data to file");
            error = true;
         }
      }

      return error;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void EraseSamsungZonePropertiesFromFlash(byte_t zoneIndex)
   {
      logInfo("Enter");
      if(flashStubInstance)
      {
         logInfo("Erasing Samsung Zone properties from flash");
         flashStubInstance->eraseSamsungZonePropertiesFile(zoneIndex);
      }
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------

   nonvolatile_scene_properties * GetNVScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData)
   {
      nonvolatile_scene_properties *nvScenePropertiesPtr = NULL;

      if(flashStubInstance)
      {
         nvScenePropertiesPtr = flashStubInstance->getNonvolatileScenePropertiesPointer(sceneIndex, validateSceneData);
      }

      return nvScenePropertiesPtr;
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------

   uint32_t WriteScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_scene_properties * nvScenePropertiesPtr)
   {
      uint32_t returnCode = -1;

      if(flashStubInstance)
      {
         logDebug("Writing Scene Properties to flash");
         if(!flashStubInstance->insertDataInSceneArray(sceneIndex, nvScenePropertiesPtr))
         {
            logError("Error inserting data to the scene array");
         }
         else
         {
            if(!flashStubInstance->appendSceneToFile(sceneIndex))
            {
               logError("Error appending the scene to the file");
            }
            else
            {
               returnCode = FTFx_OK;
            }
         }
      }

      return returnCode;
   }

   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------
   //--------------------------------------------------------------------------

   bool_t ReadScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Reading all Scene Properties from flash for sector");
         error = flashStubInstance->copyDataFromSceneArray(sceneIndex, nvSceneArray, arraySize);
      }

      return error;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   bool_t WriteScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Writing all scene properties to flash for sector");
         error = flashStubInstance->copyDataToSceneArray(sceneIndex, nvSceneArray, arraySize);

         if(!flashStubInstance->saveAllScenesToFile())
         {
            logError("Error saving scene data to file");
            error = true;
         }
      }

      return error;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void EraseScenesFromFlash(byte_t sceneIndex)
   {
      if(flashStubInstance)
      {
         flashStubInstance->eraseSceneFile(sceneIndex);
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   nonvolatile_samsung_scene_properties * GetNVSamsungScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData)
   {
      nonvolatile_samsung_scene_properties *nvSamsungScenePropertiesPtr = NULL;

      if(flashStubInstance)
      {
         nvSamsungScenePropertiesPtr = flashStubInstance->getNonvolatileSamsungScenePropertiesPointer(sceneIndex, validateSceneData);
      }

      return nvSamsungScenePropertiesPtr;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   uint32_t WriteSamsungScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_samsung_scene_properties * nvSamsungScenePropertiesPtr)
   {
      uint32_t returnCode = -1;

      if(flashStubInstance)
      {
         logInfo("Writing Samsung Scene Properties to flash %s", nvSamsungScenePropertiesPtr->SamsungDeviceId);
         if(!flashStubInstance->insertDataInSamsungScenePropertiesArray(sceneIndex, nvSamsungScenePropertiesPtr))
         {
            logError("Error inserting data to the samsung scene properties array");
         }
         else
         {
            if(!flashStubInstance->appendSamsungScenePropertiesToFile(sceneIndex))
            {
               logError("Error appending the samsung scene property to the file");
            }
            else
            {
               returnCode = FTFx_OK;
            }
         }
      }

      return returnCode;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   bool_t ReadSamsungScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_samsung_scene_properties nvSamsungSceneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Reading Samsung Scene Properties from flash ");
         flashStubInstance->copyDataFromSamsungScenePropertiesArray(sceneIndex, nvSamsungSceneArray, arraySize);
      }

      return error;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   bool_t WriteSamsungScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_samsung_scene_properties nvSamsungSceneArray[], size_t arraySize)
   {
      bool_t error = false;

      if(flashStubInstance)
      {
         logInfo("Writing all Samsung Scene properties to flash for sector");
         error = flashStubInstance->copyDataToSamsungScenePropertiesArray(sceneIndex, nvSamsungSceneArray, arraySize);

         if(!flashStubInstance->saveAllSamsungScenePropertiesToFile())
         {
            logError("Error saving Samsung Scene properties data to file");
            error = true;
         }
      }

      return error;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void EraseSamsungScenePropertiesFromFlash(byte_t sceneIndex)
   {
      logInfo("Enter");
      if(flashStubInstance)
      {
         logInfo("Erasing Samsung Scene properties from flash");
         flashStubInstance->eraseSamsungScenePropertiesFile(sceneIndex);
      }
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void ClearFlash()
   {
      if(flashStubInstance)
      {
         flashStubInstance->eraseSceneFile(0);
         flashStubInstance->eraseZoneFile();
         flashStubInstance->eraseSceneControllerFile();
         flashStubInstance->eraseSamsungZonePropertiesFile(0);
         flashStubInstance->eraseSamsungScenePropertiesFile(0);
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   nonvolatile_scene_controller_properties * GetNVSceneControllerPropertiesPtr(byte_t sceneControllerIndex, bool_t validateSceneControllerData)
   {
      nonvolatile_scene_controller_properties *nvSceneControllerPropertiesPtr = NULL;

      if(flashStubInstance)
      {
         nvSceneControllerPropertiesPtr = flashStubInstance->getNonvolatileSceneControllerPropertiesPointer(sceneControllerIndex, validateSceneControllerData);
      }
      
      return nvSceneControllerPropertiesPtr;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   uint32_t WriteSceneControllerPropertiesToFlash(byte_t sceneControllerIndex, nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr)
   {
      uint32_t returnCode = -1;

      if(flashStubInstance)
      {
         logDebug("Writing Scene Controller Properties to flash");
         if(!flashStubInstance->insertDataInSceneControllerArray(sceneControllerIndex, nvSceneControllerPropertiesPtr))
         {
            logError("Error inserting data to the scene controller array");
         }
         else
         {
            if(!flashStubInstance->appendSceneControllerToFile(sceneControllerIndex))
            {
               logError("Error appending the scene controller to the file");
            }
            else
            {
               returnCode = FTFx_OK;
            }
         }
      }

      return returnCode;
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void ReadAllSceneControllerPropertiesFromFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize)
   {
      if(flashStubInstance)
      {
         logInfo("Reading all Scene Controller Properties from flash");
         flashStubInstance->copyDataFromSceneControllerArray(nvSceneControllerArray, arraySize);
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void WriteAllSceneControllerPropertiesToFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize)
   {
      if(flashStubInstance)
      {
         logInfo("Writing all scene controller properties to flash");
         flashStubInstance->copyDataToSceneControllerArray(nvSceneControllerArray, arraySize);

         if(!flashStubInstance->saveAllSceneControllersToFile())
         {
            logError("Error saving scene controller data to file");
         }
      }
   }

   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------
   //---------------------------------------------------------------------------

   void EraseSceneControllersFromFlash()
   {
      if(flashStubInstance)
      {
         logInfo("Erasing scene controllers from flash");
         flashStubInstance->eraseSceneControllerFile();
      }
   }

}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

FlashStub::FlashStub(const std::string &_zoneFilename, const std::string &_sceneFilename, const std::string &_sceneControllerFilename, const std::string &_samsungZonePropertiesFilename, const std::string &_samsungScenePropertiesFilename) :
   zoneFilename(_zoneFilename),
   samsungZonePropertiesFilename(_samsungZonePropertiesFilename),
   sceneFilename(_sceneFilename),
   samsungScenePropertiesFilename(_samsungScenePropertiesFilename),
   sceneControllerFilename(_sceneControllerFilename)
{
   // Set the flash stub instance to this class
   flashStubInstance = this;

   // Initialize the zone data to all 0xFF until it can be loaded from the file
   memset(nvSamsungZoneProperties, 0xFF, sizeof(nvSamsungZoneProperties));
   loadAllSamsungZonePropertiesFromFile();
   saveAllSamsungZonePropertiesToFile();

   memset(nvZoneProperties, 0xFF, sizeof(nvZoneProperties));
   loadAllZonesFromFile();
   saveAllZonesToFile();

   // Initialize the scene data to all 0xFF until it can be loaded from file
   memset(nvSamsungSceneProperties, 0xFF, sizeof(nvSamsungSceneProperties));
   loadAllSamsungScenePropertiesFromFile();
   saveAllSamsungScenePropertiesToFile();

   memset(nvSceneProperties, 0xFF, sizeof(nvSceneProperties));
   loadAllScenesFromFile();
   saveAllScenesToFile();

   // Initialize the scene controller data to all 0xFF until it can be loaded from file
   memset(nvSceneControllerProperties, 0xFF, sizeof(nvSceneControllerProperties));
   loadAllSceneControllersFromFile();
   saveAllSceneControllersToFile();

}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

FlashStub::~FlashStub()
{
   flashStubInstance = NULL;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadAllZonesFromFile()
{
   int numZonesLoaded = 0;

   if(!zoneFilename.empty())
   {
      logInfo("Loading Zones from File (\"%s\")", zoneFilename.c_str());

      std::ifstream infile;
      infile.open(zoneFilename, std::ifstream::in);

      if(infile.is_open())
      {
         logInfo("File was able to be opened");

         // Parse Data from file and save zone data to array
         int lineIndex = 0;
         std::string zoneLine;
         while(std::getline(infile, zoneLine))
         {
            // Parse each tab delimited line
            std::stringstream ss(zoneLine);
            std::string token;
            std::vector<std::string> tokens;
            while(std::getline(ss, token, '\t'))
            {
               tokens.push_back(token);
            }

            if(tokens.size() == 8)
            {
               try
               {
                  // Extract the zone data from the line in the file
                  bool error = false;

                  // Parse the zone index and verify
                  int zoneIndex = std::stoi(tokens[0]);
                  if((zoneIndex < 0) || (zoneIndex > MAX_NUMBER_OF_ZONES))
                  {
                     logError("Invalid Zone index for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the building ID and verify
                  int buildingId = std::stoi(tokens[1]);
                  if((buildingId < 0) || (buildingId > 0xFF))
                  {
                     logError("Invalid building ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the house ID and verify
                  int houseId = std::stoi(tokens[2]);

                  if((houseId < 0) || (houseId > 0xFF))
                  {
                     logError("Invalid house ID for line %d", lineIndex);
                     error = true;
                  }
                  else if(global_houseID == 0)
                  {
                     // Set the global house ID to the first house ID parsed
                     global_houseID = houseId;
                  }
                  else if(houseId != global_houseID)
                  {
                     logError("House ID %d on line %d does not match global house ID %d", houseId, lineIndex, global_houseID);
                     error = true;
                  }

                  // Parse the group ID and verify
                  int groupId = std::stoi(tokens[3]);
                  if((groupId < 0) || (groupId > 0xFFFF))
                  {
                     logError("Invalid group ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the room ID and verify
                  int roomId = std::stoi(tokens[4]);
                  if((roomId < 0) || (roomId > 0xFF))
                  {
                     logError("Invalid room ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the device type and verify
                  Dev_Type deviceType;
                  int typeValue = std::stoi(tokens[5]);
                  switch(typeValue)
                  {
                     case DIMMER_TYPE:
                        deviceType = DIMMER_TYPE;
                        break;
                     case SWITCH_TYPE:
                        deviceType = SWITCH_TYPE;
                        break;
                     case FAN_CONTROLLER_TYPE:
                        deviceType = FAN_CONTROLLER_TYPE;
                        break;
                     case NETWORK_REMOTE_TYPE:
                        deviceType = NETWORK_REMOTE_TYPE;
                        break;
                     case SCENE_CONTROLLER_4_BUTTON_TYPE:
						 deviceType = SCENE_CONTROLLER_4_BUTTON_TYPE;
                        break;
                    default:
                        logError("Unknown device type for line %d", lineIndex);
                        error = true;
                  }

                  // Parse the ramp rate and verify
                  int rampRate = std::stoi(tokens[6]);
                  if((rampRate < 0) || (rampRate > 0xFF))
                  {
                     logError("Invalid ramp rate for line %d", lineIndex);
                     error = true;
                  }

                  if(!error)
                  {
                     logInfo("No Errors. Setting the zone properties, %d: numZonesLoaded:%d,  lineIndex:%d", zoneIndex, numZonesLoaded+1, lineIndex+1);
                     zone_properties zoneProperties;
                     zoneProperties.groupId = groupId;
                     zoneProperties.rampRate = rampRate;
                     zoneProperties.deviceType = deviceType;
                     zoneProperties.buildingId = buildingId;
                     zoneProperties.houseId = houseId;
                     zoneProperties.roomId = roomId;

                     // Set the volatile zone properties to their defaults
                     zoneProperties.state = false;
                     zoneProperties.targetState = false;
                     zoneProperties.powerLevel = 100;
                     zoneProperties.targetPowerLevel = 100;
                     zoneProperties.AppContextId = 0;
                     snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "%s", tokens[7].c_str());
                     snprintf(zoneProperties.SamsungDeviceId, sizeof(zoneProperties.SamsungDeviceId), "%s", nvSamsungZoneProperties[zoneIndex].SamsungDeviceId);

                     SetZoneProperties(zoneIndex, &zoneProperties) ;

                     numZonesLoaded++;
                  }
               }
               catch(std::invalid_argument)
               {
                  logError("Invalid argument in line %d", lineIndex);
               }
               catch(std::out_of_range)
               {
                  logError("Argument out of range in line %d", lineIndex);
               }
            }
            else
            {
               logError("Line %d missing tokens. %d expected, %d parsed.", lineIndex, 8, static_cast<int>(tokens.size()));
            }

            lineIndex++;
         } 
      }
      else
      {
         logError("Could not open file provided. Loading defaults and saving to file");
         numZonesLoaded = loadDefaultZones();
      } 

      infile.close();
   }
   else
   {
      logInfo("No Zone file provided, Loading the default zones");
      numZonesLoaded = loadDefaultZones();
   }
   logInfo("numZonesLoaded:%d",numZonesLoaded);
   logInfo("********************************************************");

   return numZonesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::saveAllZonesToFile()
{
   bool zonesSaved = false;
   
   if(!zoneFilename.empty())
   {
      logInfo("Saving Zones to File (\"%s\")", zoneFilename.c_str());

      // Prevent concurrent access to the zone file
      std::lock_guard<std::mutex> lock(zoneFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(zoneFilename, std::ofstream::out | std::ofstream::trunc);

      if(outfile.is_open())
      {
         nonvolatile_zone_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile zone array and save any valid zones
         // to the file separated by tabs
         for(byte_t zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
         {
            if(memcmp(&nvZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0)
            {
               // Zone is different than the default. Save it to the file
               outfile << static_cast<int>(zoneIndex) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].buildingId) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].houseId) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].groupId) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].roomId) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].deviceType) << "\t"
                       << static_cast<int>(nvZoneProperties[zoneIndex].rampRate) << "\t"
                       << nvZoneProperties[zoneIndex].ZoneNameString << std::endl; 
            }
         }

         // Close the file
         outfile.close();

         zonesSaved = true;
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Zone file provided, Unable to save the zones to a file");
   }
 
   return zonesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::appendZoneToFile(const byte_t &zoneIndex)
{
   bool zoneSaved = false;

   if(!zoneFilename.empty())
   {
      // Prevent concurrent access to the zone file
      std::lock_guard<std::mutex> lock(zoneFileMutex);

      // Open the file for appending to the end of the file
      std::ofstream outfile;
      outfile.open(zoneFilename, std::ios_base::out | std::ios_base::app);

      if(outfile.is_open())
      {
         nonvolatile_zone_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile zone array and save any valid zones
         // to the file separated by tabs
         if(memcmp(&nvZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0)
         {
            // Zone is different than the default. Save it to the file
            outfile << static_cast<int>(zoneIndex) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].buildingId) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].houseId) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].groupId) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].roomId) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].deviceType) << "\t"
               << static_cast<int>(nvZoneProperties[zoneIndex].rampRate) << "\t"
               << nvZoneProperties[zoneIndex].ZoneNameString << std::endl; 

            zoneSaved = true;
         }
         else
         {
            logError("Not appending zone to file. Data is uninitialized");
         }

         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Zone file provided, Unable to save the zone to a file");
   }

   return zoneSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::eraseZoneFile()
{
   if(!zoneFilename.empty())
   {
      logInfo("Erasing Zone File (\"%s\")", zoneFilename.c_str());

      // Initialize the zone data to all 0xFF until it can be loaded from the file
      memset(nvZoneProperties, 0xFF, sizeof(nvZoneProperties));

      // Prevent concurrent access to the zone file
      std::lock_guard<std::mutex> lock(zoneFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(zoneFilename, std::ios_base::out | std::ios_base::trunc);

      if(outfile.is_open())
      {
         // File was successfully opened and cleared
         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file. Might not be erased");
      }
   }
   else
   {
      logError("No Zone file provided, Unable to erase the zone file");
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

nonvolatile_zone_properties * FlashStub::getNonvolatileZonePropertiesPointer(const byte_t &zoneIndex, const bool &validateZoneData)
{
   nonvolatile_zone_properties * nvZonePropertiesPtr = NULL;
   nonvolatile_zone_properties uninitialized;
   memset(&uninitialized, 0xFF, sizeof(uninitialized));

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      if((!validateZoneData) ||
         (memcmp(&nvZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0))
      {
         nvZonePropertiesPtr = &nvZoneProperties[zoneIndex];
      }
   }

   return nvZonePropertiesPtr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::insertDataInZoneArray(const byte_t &zoneIndex, nonvolatile_zone_properties * nvZonePropertiesPtr)
{
   bool zoneDataAdded = false;

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      nvZoneProperties[zoneIndex].groupId = nvZonePropertiesPtr->groupId;
      nvZoneProperties[zoneIndex].deviceType = nvZonePropertiesPtr->deviceType;
      nvZoneProperties[zoneIndex].rampRate = nvZonePropertiesPtr->rampRate;
      nvZoneProperties[zoneIndex].buildingId = nvZonePropertiesPtr->buildingId;
      nvZoneProperties[zoneIndex].houseId = nvZonePropertiesPtr->houseId;
      nvZoneProperties[zoneIndex].roomId = nvZonePropertiesPtr->roomId;
      snprintf(nvZoneProperties[zoneIndex].ZoneNameString, sizeof(nvZoneProperties[zoneIndex].ZoneNameString), "%s", nvZonePropertiesPtr->ZoneNameString);
      zoneDataAdded = true;
   }

   return zoneDataAdded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::copyDataFromZoneArray(nonvolatile_zone_properties nvZoneArray[], const size_t &arraySize)
{
   memcpy(nvZoneArray, nvZoneProperties, sizeof(nvZoneProperties));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::copyDataToZoneArray(nonvolatile_zone_properties nvZoneArray[], const size_t &arraySize)
{
   memcpy(nvZoneProperties, nvZoneArray, sizeof(nvZoneProperties));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadAllSamsungZonePropertiesFromFile()
{
   int numPropertiesLoaded = 0;

   if(!samsungZonePropertiesFilename.empty())
   {
      logInfo("Loading Samsung Zone Properties from File (\"%s\")", samsungZonePropertiesFilename.c_str());

      std::ifstream infile;
      infile.open(samsungZonePropertiesFilename, std::ifstream::in);

      if(infile.is_open())
      {
         logInfo("File was able to be opened");

         // Parse Data from file and save zone data to array
         int lineIndex = 0;
         std::string zoneLine;
         while(std::getline(infile, zoneLine))
         {
            // Parse each tab delimited line
            std::stringstream ss(zoneLine);
            std::string token;
            std::vector<std::string> tokens;
            while(std::getline(ss, token, '\t'))
            {
               tokens.push_back(token);
            }

            if(tokens.size() == 2)
            {
               try
               {
                  // Extract the zone data from the line in the file
                  bool error = false;

                  // Parse the zone index and verify
                  int zoneIndex = std::stoi(tokens[0]);
                  if((zoneIndex < 0) || (zoneIndex > MAX_NUMBER_OF_ZONES))
                  {
                     logError("Invalid Zone index for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the samsung device ID
                  std::string samsungDeviceId = tokens[1];

                  logInfo("Saving Data %d: %s\n", zoneIndex, samsungDeviceId.c_str());

                  if(!error)
                  {
                     logDebug("No Errors. Setting the properties, numPropertiesLoaded:%d,  lineIndex:%d", numPropertiesLoaded+1, lineIndex+1);

                     nonvolatile_samsung_zone_properties nvSamsungZoneProperties;
                     snprintf(nvSamsungZoneProperties.SamsungDeviceId, sizeof(nvSamsungZoneProperties.SamsungDeviceId), "%s", samsungDeviceId.c_str());

                     insertDataInSamsungZonePropertiesArray(zoneIndex, &nvSamsungZoneProperties);
                     numPropertiesLoaded++;
                  }
               }
               catch(std::invalid_argument)
               {
                  logError("Invalid argument in line %d", lineIndex);
               }
               catch(std::out_of_range)
               {
                  logError("Argument out of range in line %d", lineIndex);
               }
            }
            else
            {
               logError("Line %d missing tokens. %d expected, %d parsed.", lineIndex, 2, static_cast<int>(tokens.size()));
            }

            lineIndex++;
         } 
      }
      else
      {
         logError("Could not open file provided. Not loading any samsung zone properties");
         numPropertiesLoaded = 0;
      } 

      infile.close();
   }
   else
   {
      logInfo("No Properties file provided");
      numPropertiesLoaded = 0;
   }
   logInfo("numSamsungZonePropertiesLoaded:%d",numPropertiesLoaded);
   logInfo("********************************************************");

   return numPropertiesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::saveAllSamsungZonePropertiesToFile()
{
   bool propertiesSaved = false;
   
   if(!samsungZonePropertiesFilename.empty())
   {
      logInfo("Saving Samsung Zone Properties to File (\"%s\")", samsungZonePropertiesFilename.c_str());

      // Prevent concurrent access to the samsung properties file
      std::lock_guard<std::mutex> lock(samsungZonePropertiesFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(samsungZonePropertiesFilename, std::ofstream::out | std::ofstream::trunc);

      if(outfile.is_open())
      {
         nonvolatile_samsung_zone_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile zone array and save any valid zones
         // to the file separated by tabs
         for(byte_t zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
         {
            if(memcmp(&nvSamsungZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0)
            {
               // Zone is different than the default. Save it to the file
               outfile << static_cast<int>(zoneIndex) << "\t"
                       << nvSamsungZoneProperties[zoneIndex].SamsungDeviceId << std::endl;
            }
         }

         // Close the file
         outfile.close();

         propertiesSaved = true;
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No samsung zone properties file provided, Unable to save the properties to a file");
   }
 
   return propertiesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::appendSamsungZonePropertiesToFile(const byte_t &zoneIndex)
{
   bool propertiesSaved = false;

   if(!samsungZonePropertiesFilename.empty())
   {
      // Prevent concurrent access to the samsung zone properties file
      std::lock_guard<std::mutex> lock(samsungZonePropertiesFileMutex);

      // Open the file for appending to the end of the file
      std::ofstream outfile;
      outfile.open(samsungZonePropertiesFilename, std::ios_base::out | std::ios_base::app);

      if(outfile.is_open())
      {
         nonvolatile_samsung_zone_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile array and save any valid properties
         // to the file separated by tabs
         if(memcmp(&nvSamsungZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0)
         {
            // Zone is different than the default. Save it to the file
            outfile << static_cast<int>(zoneIndex) << "\t"
               << nvSamsungZoneProperties[zoneIndex].SamsungDeviceId << std::endl; 

            propertiesSaved = true;
         }
         else
         {
            logError("Not appending property to file. Data is uninitialized");
         }

         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No file provided, Unable to save the property to a file");
   }

   return propertiesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::eraseSamsungZonePropertiesFile(const byte_t &zoneIndex)
{
   if(!samsungZonePropertiesFilename.empty())
   {
      logInfo("Erasing Samsung Zone Properties File (\"%s\")", samsungZonePropertiesFilename.c_str());

      // Initialize the zone data to all 0xFF until it can be loaded from the file
      memset(nvSamsungZoneProperties, 0xFF, sizeof(nvSamsungZoneProperties));

      // Prevent concurrent access to the samsung zone properties file
      std::lock_guard<std::mutex> lock(samsungZonePropertiesFileMutex);

      byte_t sectorIndex = (zoneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR);
      logInfo("Erasing samsung zone properties from array at index %d", sectorIndex);

      // Initialize the samsung zone properties data to all 0xFF until it can be loaded from the file
      memset(&nvSamsungZoneProperties[sectorIndex * NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR], 0xFF, NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_zone_properties));

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(samsungZonePropertiesFilename, std::ios_base::out | std::ios_base::trunc);

      if(outfile.is_open())
      {
         // File was successfully opened and cleared
         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file. Might not be erased");
      }
   }
   else
   {
      logError("No file provided, Unable to erase the samsung zone properties file");
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

nonvolatile_samsung_zone_properties * FlashStub::getNonvolatileSamsungZonePropertiesPointer(const byte_t &zoneIndex, const bool &validateZoneData)
{
   nonvolatile_samsung_zone_properties * nvSamsungZonePropertiesPtr = NULL;
   nonvolatile_samsung_zone_properties uninitialized;
   memset(&uninitialized, 0xFF, sizeof(uninitialized));

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      if((!validateZoneData) ||
         (memcmp(&nvSamsungZoneProperties[zoneIndex], &uninitialized, sizeof(uninitialized)) != 0))
      {
         nvSamsungZonePropertiesPtr = &nvSamsungZoneProperties[zoneIndex];
      }
   }

   return nvSamsungZonePropertiesPtr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::insertDataInSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties * nvSamsungZonePropertiesPtr)
{
   bool samsungZonePropertyDataAdded = false;

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      snprintf(nvSamsungZoneProperties[zoneIndex].SamsungDeviceId, sizeof(nvSamsungZoneProperties[zoneIndex].SamsungDeviceId), "%s", nvSamsungZonePropertiesPtr->SamsungDeviceId);
      samsungZonePropertyDataAdded = true;
   }

   return samsungZonePropertyDataAdded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataFromSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZoneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = zoneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR;
      memcpy(nvSamsungZoneArray, &nvSamsungZoneProperties[sectorIndex * NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR], NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_zone_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataToSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZoneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = (zoneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR);
      memcpy(&nvSamsungZoneProperties[sectorIndex * NUM_SCENES_IN_FLASH_SECTOR], nvSamsungZoneArray, NUM_SCENES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_zone_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadAllScenesFromFile()
{
   int numScenesLoaded = 0;

   if(!sceneFilename.empty())
   {
      logInfo("Loading Scenes from File (\"%s\")", sceneFilename.c_str());

      std::ifstream infile;
      infile.open(sceneFilename, std::ifstream::in);

      if(infile.is_open())
      {
         logInfo("File was able to be open");

         // Parse Data from file and save scene data to array
         int lineIndex = 0;
         std::string sceneLine;
         while(std::getline(infile, sceneLine))
         {
            // Parse each tab delimited line
            std::stringstream ss(sceneLine);
            std::string token;
            std::vector<std::string> tokens;
            while(std::getline(ss, token, '\t'))
            {
               tokens.push_back(token);
            }

            if(tokens.size() == 15)
            {
               try
               {
                  // Extract the scene data from the line in the file
                  bool error = false;

                  // Parse the scene index
                  int sceneIndex = std::stoi(tokens[0]);
                  if((sceneIndex < 0) || (sceneIndex > MAX_NUMBER_OF_SCENES))
                  {
                     logError("Invalid scene index for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the number of zones in the scene
                  int numZonesInScene = std::stoi(tokens[1]);
                  if((numZonesInScene < 0) || (numZonesInScene > MAX_ZONES_IN_SCENE))
                  {
                     logError("Invalid number of zones for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the trigger time
                  unsigned int triggerTime = static_cast<unsigned int>(std::stoi(tokens[2]));

                  // Parse the frequency type
                  FrequencyType frequencyType;
                  int typeValue = std::stoi(tokens[3]);
                  switch(typeValue)
                  {
                     case NONE:
                        frequencyType = NONE;
                        break;
                     case ONCE:
                        frequencyType = ONCE;
                        break;
                     case EVERY_WEEK:
                        frequencyType = EVERY_WEEK;
                        break;
                     default:
                        logError("Unknown frequency type for line %d", lineIndex);
                        error = true;
                  }

                  // Parse the trigger type
                  TriggerType triggerType;
                  typeValue = std::stoi(tokens[4]);
                  switch(typeValue)
                  {
                     case REGULAR_TIME:
                        triggerType = REGULAR_TIME;
                        break;
                     case SUNRISE:
                        triggerType = SUNRISE;
                        break;
                     case SUNSET:
                        triggerType = SUNSET;
                        break;
                     default:
                        logError("Unknown trigger type for line %d", lineIndex);
                        error = true;
                  }

                  // Parse the day bits
                  DailySchedule dayBits;
                  dayBits.days = 0;
                  int sunday = std::stoi(tokens[5]);
                  int monday = std::stoi(tokens[6]);
                  int tuesday = std::stoi(tokens[7]);
                  int wednesday = std::stoi(tokens[8]);
                  int thursday = std::stoi(tokens[9]);
                  int friday = std::stoi(tokens[10]);
                  int saturday = std::stoi(tokens[11]);
                  dayBits.DayBits.sunday = (sunday != 0);
                  dayBits.DayBits.monday = (monday != 0);
                  dayBits.DayBits.tuesday = (tuesday != 0);
                  dayBits.DayBits.wednesday = (wednesday != 0);
                  dayBits.DayBits.thursday = (thursday != 0);
                  dayBits.DayBits.friday = (friday != 0);
                  dayBits.DayBits.saturday = (saturday != 0);

                  // Parse the delta
                  int delta = std::stoi(tokens[12]);

                  // Parse the skip next flag
                  int skipNextVal = std::stoi(tokens[13]);
                  bool skipNext = (skipNextVal != 0);

                  // Parse the scene name
                  std::string sceneName = tokens[14];

                  // Parse the zone data
                  zone_in_scene zoneData[MAX_ZONES_IN_SCENE];
                  if(!error)
                  {
                     logDebug("No Errors for the scene. Get the zone data");

                     for(int zoneIndex = 0; (zoneIndex < numZonesInScene) && (!error); zoneIndex++)
                     {
                        std::string zoneLine;
                        zone_in_scene zoneInScene;

                        if(std::getline(infile, zoneLine))
                        {
                           // Parse each tab delimited line
                           std::stringstream ss(zoneLine);
                           std::string zoneToken;
                           std::vector<std::string> zoneTokens;
                           while(std::getline(ss, zoneToken, '\t'))
                           {
                              zoneTokens.push_back(zoneToken);
                           }

                           if(zoneTokens.size() == 4)
                           {
                              // Parse the zone data and verify that it is valid
                              int zoneId = std::stoi(zoneTokens[0]);
                              if((zoneId < 0) || (zoneId > MAX_NUMBER_OF_ZONES))
                              {
                                 logError("Invalid Zone index");
                                 error = true;
                              }
                              else
                              {
                                 // Verify zone exists and is valid
                                 zone_properties zoneProperties;
                                 if(GetZoneProperties(zoneId, &zoneProperties))
                                 {
                                    if(!zoneProperties.ZoneNameString[0])
                                    {
                                       logError("Trying to add an invalid zone %d to scene %d", zoneId, sceneIndex);
                                       error = true;
                                    }
                                    else
                                    {
                                       logDebug("Zone %d in scene %d is valid", zoneId, sceneIndex);
                                    }
                                 }
                                 else
                                 {
                                    logError("Could not get zone properties for zone id %d in scene %d", zoneId, sceneIndex);
                                    error = true;
                                 }
                              }

                              int powerLevel = std::stoi(zoneTokens[1]);
                              if((powerLevel < 0) || (powerLevel > 100))
                              {
                                 logError("Invalid power level");
                                 error = true;
                              }

                              int rampRate = std::stoi(zoneTokens[2]);
                              if((rampRate < 0) || (rampRate > 0xFF))
                              {
                                 logError("Invalid ramp rate");
                                 error = true;
                              }

                              int stateVal = std::stoi(zoneTokens[3]);
                              bool state = (stateVal != 0);

                              zoneInScene.zoneId = zoneId;
                              zoneInScene.powerLevel = powerLevel;
                              zoneInScene.rampRate = rampRate;
                              zoneInScene.state = state;

                              zoneData[zoneIndex] = zoneInScene;
                           }
                           else
                           {
                              logError("Zone missing tokens. %d expected, %d parsed.", 4, static_cast<int>(zoneTokens.size()));
                              error = true;
                           }
                        }
                        else
                        {
                           logError("Failed to get zone data for the scene");
                           error = true;
                        }
                     }
                  }

                  scene_properties sceneProperties;
                  if(!error)
                  {
                     logDebug("No Errors getting the zone data. Set the scene properties");

                     // Set the nonvolatile scene properties
                     sceneProperties.numZonesInScene = numZonesInScene;
                     for(int zoneIndex = 0; zoneIndex < numZonesInScene; zoneIndex++)
                     {
                        sceneProperties.zoneData[zoneIndex].zoneId = zoneData[zoneIndex].zoneId;
                        sceneProperties.zoneData[zoneIndex].rampRate = zoneData[zoneIndex].rampRate;
                        sceneProperties.zoneData[zoneIndex].powerLevel = zoneData[zoneIndex].powerLevel;
                        sceneProperties.zoneData[zoneIndex].state = zoneData[zoneIndex].state;
                     }
                     sceneProperties.nextTriggerTime = triggerTime;
                     sceneProperties.freq = frequencyType;
                     sceneProperties.trigger = triggerType;
                     sceneProperties.dayBitMask = dayBits;
                     sceneProperties.delta = delta;
                     sceneProperties.skipNext = skipNext;
                     snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "%s", sceneName.c_str());
                     snprintf(sceneProperties.SamsungDeviceId, sizeof(sceneProperties.SamsungDeviceId), "%s", nvSamsungSceneProperties[sceneIndex].SamsungDeviceId);

                     // Set the volatile scene properties to their defaults
                     sceneProperties.running = false;

                     // Call set scene properties to save the volatile properties
                     SetSceneProperties(sceneIndex, &sceneProperties);

                     numScenesLoaded++;
                  }
                  else
                  {
                     logError("No loading scene data because of invalid zones. Erase the scene");
                     memset(sceneProperties.SceneNameString, 0x00, sizeof(sceneProperties.SceneNameString));
                     SetSceneProperties(sceneIndex, &sceneProperties);
                  }
               }
               catch(std::invalid_argument)
               {
                  logError("Invalid argument in line %d", lineIndex);
               }
               catch(std::out_of_range)
               {
                  logError("Argument out of range in line %d", lineIndex);
               }
            }
            else
            {
               logError("Line %d missing tokens. %d expected, %d parsed.", lineIndex, 15, static_cast<int>(tokens.size()));
            }

            lineIndex++;
         }

         infile.close();

      }
      else
      {
         logError("Could not open file provided. Loading defaults and saving to file");
         numScenesLoaded = loadDefaultScenes();
      }
   }
   else
   {
      logInfo("No Zone file provided, Loading the default scenes");
      numScenesLoaded = loadDefaultScenes();
   }
   logInfo("numScenesLoaded: %d", numScenesLoaded);
   return numScenesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::saveAllScenesToFile()
{
   bool scenesSaved = false;

   if(!sceneFilename.empty())
   {
      logInfo("Saving Scenes to File (\"%s\")", sceneFilename.c_str());

      // Prevent concurrent access to the scene file
      std::lock_guard<std::mutex> lock(sceneFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(sceneFilename, std::ofstream::out | std::ofstream::trunc);

      if(outfile.is_open())
      {
         nonvolatile_scene_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile scene array and save any valid scenes
         // to the file separated by tabs
         for(byte_t sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
         {
            if(memcmp(&nvSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0)
            {
               // Scene is different than the default. Save it to the file
               outfile << static_cast<int>(sceneIndex) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].numZonesInScene) << "\t"
                       << static_cast<unsigned int>(nvSceneProperties[sceneIndex].nextTriggerTime) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].freq) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].trigger) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.sunday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.monday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.tuesday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.wednesday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.thursday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.friday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.saturday) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].delta) << "\t"
                       << static_cast<int>(nvSceneProperties[sceneIndex].skipNext) << "\t"
                       << nvSceneProperties[sceneIndex].SceneNameString << std::endl; 

               // Save the zone data for the scene
               for(int zoneIndex = 0; zoneIndex < nvSceneProperties[sceneIndex].numZonesInScene; zoneIndex++)
               {
                  outfile << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].zoneId) << "\t"
                          << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].powerLevel) << "\t"
                          << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].rampRate) << "\t"
                          << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].state) << std::endl;
               }
            }
         }

         // Close the file
         outfile.close();

         scenesSaved = true;
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Zone file provided, Unable to save the zones to a file");
   }

   return scenesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::appendSceneToFile(const byte_t &sceneIndex)
{
   bool sceneSaved = false;

   if(!sceneFilename.empty())
   {
      logDebug("Saving Scene %d to File (\"%s\")", sceneIndex, sceneFilename.c_str());

      // Prevent concurrent access to the zone file
      std::lock_guard<std::mutex> lock(sceneFileMutex);

      // Open the file for appending to the end of the file
      std::ofstream outfile;
      outfile.open(sceneFilename, std::ios_base::out | std::ios_base::app);

      if(outfile.is_open())
      {
         nonvolatile_scene_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile scene array and save any valid scenes
         // to the file separated by tabs
         if(memcmp(&nvSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0)
         {
            // Scene is different than the default. Save it to the file
            outfile << static_cast<int>(sceneIndex) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].numZonesInScene) << "\t"
               << static_cast<unsigned int>(nvSceneProperties[sceneIndex].nextTriggerTime) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].freq) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].trigger) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.sunday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.monday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.tuesday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.wednesday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.thursday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.friday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].dayBitMask.DayBits.saturday) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].delta) << "\t"
               << static_cast<int>(nvSceneProperties[sceneIndex].skipNext) << "\t"
               << nvSceneProperties[sceneIndex].SceneNameString << std::endl; 

            // Save the zone data for the scene
            for(int zoneIndex = 0; zoneIndex < nvSceneProperties[sceneIndex].numZonesInScene; zoneIndex++)
            {
               outfile << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].zoneId) << "\t"
                  << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].powerLevel) << "\t"
                  << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].rampRate) << "\t"
                  << static_cast<int>(nvSceneProperties[sceneIndex].zoneData[zoneIndex].state) << std::endl;
            }

            sceneSaved = true;
         }
         else
         {
            logError("Not appending scene to file. Data is uninitialized");
         }

         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Scene file provided, Unable to save the scene to a file");
   }

   return sceneSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::eraseSceneFile(const byte_t &sceneIndex)
{
   if(!sceneFilename.empty())
   {
      logInfo("Erasing Scene File (\"%s\")", sceneFilename.c_str());

      // Prevent concurrent access to the scene file
      std::lock_guard<std::mutex> lock(sceneFileMutex);

      byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
      logInfo("Erasing scenes from array at index %d", sectorIndex);

      // Initialize the scene data to all 0xFF until it can be loaded from the file
      memset(&nvSceneProperties[sectorIndex * NUM_SCENES_IN_FLASH_SECTOR], 0xFF, NUM_SCENES_IN_FLASH_SECTOR * sizeof(nonvolatile_scene_properties));

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(sceneFilename, std::ios_base::out | std::ios_base::trunc);

      if(outfile.is_open())
      {
         // File was successfully opened and cleared
         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file. Might not be erased");
      }
   }
   else
   {
      logError("No Scene file provided, Unable to erase the scene file");
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

nonvolatile_scene_properties * FlashStub::getNonvolatileScenePropertiesPointer(const byte_t &sceneIndex, const bool &validateSceneData)
{
   nonvolatile_scene_properties * nvScenePropertiesPtr = NULL;
   nonvolatile_zone_properties uninitialized;
   memset(&uninitialized, 0xFF, sizeof(uninitialized));

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      if((!validateSceneData) ||
         (memcmp(&nvSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0))
      {
         nvScenePropertiesPtr = &nvSceneProperties[sceneIndex];
      }
   }

   return nvScenePropertiesPtr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::insertDataInSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties * nvScenePropertiesPtr)
{
   bool sceneDataAdded = false;

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      nvSceneProperties[sceneIndex].numZonesInScene = nvScenePropertiesPtr->numZonesInScene;
      nvSceneProperties[sceneIndex].nextTriggerTime = nvScenePropertiesPtr->nextTriggerTime;
      nvSceneProperties[sceneIndex].freq = nvScenePropertiesPtr->freq;
      nvSceneProperties[sceneIndex].trigger = nvScenePropertiesPtr->trigger;
      nvSceneProperties[sceneIndex].dayBitMask.days = nvScenePropertiesPtr->dayBitMask.days;
      nvSceneProperties[sceneIndex].delta = nvScenePropertiesPtr->delta;
      nvSceneProperties[sceneIndex].skipNext = nvScenePropertiesPtr->skipNext;
      snprintf(nvSceneProperties[sceneIndex].SceneNameString, sizeof(nvSceneProperties[sceneIndex].SceneNameString), "%s", nvScenePropertiesPtr->SceneNameString);

      // Save the zone data for the scene
      for(int zoneIndex = 0; zoneIndex < nvSceneProperties[sceneIndex].numZonesInScene; zoneIndex++)
      {
         nvSceneProperties[sceneIndex].zoneData[zoneIndex].zoneId = nvScenePropertiesPtr->zoneData[zoneIndex].zoneId;
         nvSceneProperties[sceneIndex].zoneData[zoneIndex].powerLevel = nvScenePropertiesPtr->zoneData[zoneIndex].powerLevel;
         nvSceneProperties[sceneIndex].zoneData[zoneIndex].rampRate = nvScenePropertiesPtr->zoneData[zoneIndex].rampRate;
         nvSceneProperties[sceneIndex].zoneData[zoneIndex].state = nvScenePropertiesPtr->zoneData[zoneIndex].state;
      }

      sceneDataAdded = true;
   }

   return sceneDataAdded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataFromSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties nvSceneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SCENES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
      memcpy(nvSceneArray, &nvSceneProperties[sectorIndex * NUM_SCENES_IN_FLASH_SECTOR], NUM_SCENES_IN_FLASH_SECTOR * sizeof(nonvolatile_scene_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataToSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties nvSceneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SCENES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
      memcpy(&nvSceneProperties[sectorIndex * NUM_SCENES_IN_FLASH_SECTOR], nvSceneArray, NUM_SCENES_IN_FLASH_SECTOR * sizeof(nonvolatile_scene_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadAllSamsungScenePropertiesFromFile()
{
   int numPropertiesLoaded = 0;

   if(!samsungScenePropertiesFilename.empty())
   {
      logInfo("Loading Samsung Scene Properties from File (\"%s\")", samsungScenePropertiesFilename.c_str());

      std::ifstream infile;
      infile.open(samsungScenePropertiesFilename, std::ifstream::in);

      if(infile.is_open())
      {
         logInfo("File was able to be opened");

         // Parse Data from file and save scene data to array
         int lineIndex = 0;
         std::string sceneLine;
         while(std::getline(infile, sceneLine))
         {
            // Parse each tab delimited line
            std::stringstream ss(sceneLine);
            std::string token;
            std::vector<std::string> tokens;
            while(std::getline(ss, token, '\t'))
            {
               tokens.push_back(token);
            }

            if(tokens.size() == 2)
            {
               try
               {
                  // Extract the scene data from the line in the file
                  bool error = false;

                  // Parse the scene index and verify
                  int sceneIndex = std::stoi(tokens[0]);
                  if((sceneIndex < 0) || (sceneIndex > MAX_NUMBER_OF_SCENES))
                  {
                     logError("Invalid Scene index for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the samsung device ID
                  std::string samsungDeviceId = tokens[1];

                  logInfo("Saving Data %d: %s\n", sceneIndex, samsungDeviceId.c_str());

                  if(!error)
                  {
                     logDebug("No Errors. Setting the properties, numPropertiesLoaded:%d,  lineIndex:%d", numPropertiesLoaded+1, lineIndex+1);

                     nonvolatile_samsung_scene_properties nvSamsungSceneProperties;
                     snprintf(nvSamsungSceneProperties.SamsungDeviceId, sizeof(nvSamsungSceneProperties.SamsungDeviceId), "%s", samsungDeviceId.c_str());

                     insertDataInSamsungScenePropertiesArray(sceneIndex, &nvSamsungSceneProperties);
                     numPropertiesLoaded++;
                  }
               }
               catch(std::invalid_argument)
               {
                  logError("Invalid argument in line %d", lineIndex);
               }
               catch(std::out_of_range)
               {
                  logError("Argument out of range in line %d", lineIndex);
               }
            }
            else
            {
               logError("Line %d missing tokens. %d expected, %d parsed.", lineIndex, 2, static_cast<int>(tokens.size()));
            }

            lineIndex++;
         } 
      }
      else
      {
         logError("Could not open file provided. Not loading any samsung scene properties");
         numPropertiesLoaded = 0;
      } 

      infile.close();
   }
   else
   {
      logInfo("No Properties file provided");
      numPropertiesLoaded = 0;
   }
   logInfo("numSamsungScenePropertiesLoaded:%d",numPropertiesLoaded);
   logInfo("********************************************************");

   return numPropertiesLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::saveAllSamsungScenePropertiesToFile()
{
   bool propertiesSaved = false;
   
   if(!samsungScenePropertiesFilename.empty())
   {
      logInfo("Saving Samsung Scene Properties to File (\"%s\")", samsungScenePropertiesFilename.c_str());

      // Prevent concurrent access to the samsung properties file
      std::lock_guard<std::mutex> lock(samsungScenePropertiesFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(samsungScenePropertiesFilename, std::ofstream::out | std::ofstream::trunc);

      if(outfile.is_open())
      {
         nonvolatile_samsung_scene_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile scene array and save any valid scenes
         // to the file separated by tabs
         for(byte_t sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
         {
            if(memcmp(&nvSamsungSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0)
            {
               // Scene is different than the default. Save it to the file
               outfile << static_cast<int>(sceneIndex) << "\t"
                       << nvSamsungSceneProperties[sceneIndex].SamsungDeviceId << std::endl;
            }
         }

         // Close the file
         outfile.close();

         propertiesSaved = true;
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No samsung scene properties file provided, Unable to save the properties to a file");
   }
 
   return propertiesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::appendSamsungScenePropertiesToFile(const byte_t &sceneIndex)
{
   bool propertiesSaved = false;

   if(!samsungScenePropertiesFilename.empty())
   {
      // Prevent concurrent access to the samsung scene properties file
      std::lock_guard<std::mutex> lock(samsungScenePropertiesFileMutex);

      // Open the file for appending to the end of the file
      std::ofstream outfile;
      outfile.open(samsungScenePropertiesFilename, std::ios_base::out | std::ios_base::app);

      if(outfile.is_open())
      {
         nonvolatile_samsung_scene_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile array and save any valid properties
         // to the file separated by tabs
         if(memcmp(&nvSamsungSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0)
         {
            // Scene is different than the default. Save it to the file
            outfile << static_cast<int>(sceneIndex) << "\t"
               << nvSamsungSceneProperties[sceneIndex].SamsungDeviceId << std::endl; 

            propertiesSaved = true;
         }
         else
         {
            logError("Not appending property to file. Data is uninitialized");
         }

         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No file provided, Unable to save the property to a file");
   }

   return propertiesSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::eraseSamsungScenePropertiesFile(const byte_t &sceneIndex)
{
   if(!samsungScenePropertiesFilename.empty())
   {
      logInfo("Erasing Samsung Scene Properties File (\"%s\")", samsungScenePropertiesFilename.c_str());

      // Initialize the scene data to all 0xFF until it can be loaded from the file
      memset(nvSamsungSceneProperties, 0xFF, sizeof(nvSamsungSceneProperties));

      // Prevent concurrent access to the samsung scene properties file
      std::lock_guard<std::mutex> lock(samsungScenePropertiesFileMutex);

      byte_t sectorIndex = (sceneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR);
      logInfo("Erasing samsung scene properties from array at index %d", sectorIndex);

      // Initialize the samsung scene properties data to all 0xFF until it can be loaded from the file
      memset(&nvSamsungSceneProperties[sectorIndex * NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR], 0xFF, NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_scene_properties));

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(samsungScenePropertiesFilename, std::ios_base::out | std::ios_base::trunc);

      if(outfile.is_open())
      {
         // File was successfully opened and cleared
         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file. Might not be erased");
      }
   }
   else
   {
      logError("No file provided, Unable to erase the samsung scene properties file");
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

nonvolatile_samsung_scene_properties * FlashStub::getNonvolatileSamsungScenePropertiesPointer(const byte_t &sceneIndex, const bool &validateSceneData)
{
   nonvolatile_samsung_scene_properties * nvSamsungScenePropertiesPtr = NULL;
   nonvolatile_samsung_scene_properties uninitialized;
   memset(&uninitialized, 0xFF, sizeof(uninitialized));

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      if((!validateSceneData) ||
         (memcmp(&nvSamsungSceneProperties[sceneIndex], &uninitialized, sizeof(uninitialized)) != 0))
      {
         nvSamsungScenePropertiesPtr = &nvSamsungSceneProperties[sceneIndex];
      }
   }

   return nvSamsungScenePropertiesPtr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::insertDataInSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties * nvSamsungScenePropertiesPtr)
{
   bool samsungScenePropertyDataAdded = false;

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      snprintf(nvSamsungSceneProperties[sceneIndex].SamsungDeviceId, sizeof(nvSamsungSceneProperties[sceneIndex].SamsungDeviceId), "%s", nvSamsungScenePropertiesPtr->SamsungDeviceId);
      samsungScenePropertyDataAdded = true;
   }

   return samsungScenePropertyDataAdded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataFromSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties nvSamsungSceneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = sceneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR;
      memcpy(nvSamsungSceneArray, &nvSamsungSceneProperties[sectorIndex * NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR], NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_scene_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::copyDataToSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties nvSamsungSceneArray[], const size_t &arraySize)
{
   bool error = false;

   if(arraySize != NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR)
   {
      logError("Array size is not valid");
      error = true;
   }
   else
   {
      byte_t sectorIndex = (sceneIndex / NUM_SAMSUNG_PROPERTIES_IN_FLASH_SECTOR);
      memcpy(&nvSamsungSceneProperties[sectorIndex * NUM_SCENES_IN_FLASH_SECTOR], nvSamsungSceneArray, NUM_SCENES_IN_FLASH_SECTOR * sizeof(nonvolatile_samsung_scene_properties));
   }

   return error;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadAllSceneControllersFromFile()
{
   int numSceneControllersLoaded = 0;

   if(!sceneControllerFilename.empty())
   {
      logInfo("Loading Scene Controllers from File (\"%s\")", sceneControllerFilename.c_str());

      std::ifstream infile;
      infile.open(sceneControllerFilename, std::ifstream::in);

      if(infile.is_open())
      {
         logInfo("File was able to be open");

         // Parse Data from file and save scene controller data to array
         int lineIndex = 0;
         std::string sceneControllerLine;
         while(std::getline(infile, sceneControllerLine))
         {
            // Parse each tab delimited line
            std::stringstream ss(sceneControllerLine);
            std::string token;
            std::vector<std::string> tokens;
            while(std::getline(ss, token, '\t'))
            {
               tokens.push_back(token);
            }

            if(tokens.size() == 8)
            {
               try
               {
                  // Extract the scene controller data from the line in the file
                  bool error = false;

                  // Parse the scene controller index and verify
                  int sceneControllerIndex = std::stoi(tokens[0]);
                  if((sceneControllerIndex < 0) || (sceneControllerIndex > MAX_NUMBER_OF_SCENE_CONTROLLERS))
                  {
                     logError("Invalid Scene Controller index for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the building ID and verify
                  int buildingId = std::stoi(tokens[1]);
                  if((buildingId < 0) || (buildingId > 0xFF))
                  {
                     logError("Invalid building ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the house ID and verify
                  int houseId = std::stoi(tokens[2]);

                  if((houseId < 0) || (houseId > 0xFF))
                  {
                     logError("Invalid house ID for line %d", lineIndex);
                     error = true;
                  }
                  else if(global_houseID == 0)
                  {
                     // Set the global house ID to the first house ID parsed
                     global_houseID = houseId;
                  }
                  else if(houseId != global_houseID)
                  {
                     logError("House ID %d on line %d does not match global house ID %d", houseId, lineIndex, global_houseID);
                     error = true;
                  }

                  // Parse the group ID and verify
                  int groupId = std::stoi(tokens[3]);
                  if((groupId < 0) || (groupId > 0xFFFF))
                  {
                     logError("Invalid group ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the room ID and verify
                  int roomId = std::stoi(tokens[4]);
                  if((roomId < 0) || (roomId > 0xFF))
                  {
                     logError("Invalid room ID for line %d", lineIndex);
                     error = true;
                  }

                  // Parse the device type and verify
                  Dev_Type deviceType;
                  int typeValue = std::stoi(tokens[5]);
                  switch(typeValue)
                  {
                     case SCENE_CONTROLLER_4_BUTTON_TYPE:
						 deviceType = SCENE_CONTROLLER_4_BUTTON_TYPE;
                        break;
                     default:
                        logError("Invalid device type for line %d", lineIndex);
                        error = true;
                  }

                  // Parse the night mode and verify
                  int nightMode = std::stoi(tokens[6]);
                  if((nightMode != 0) && (nightMode != 1))
                  {
                     logError("Invalid night mode value %d for line %d", nightMode, lineIndex);
                     error = true;
                  }

                  // Parse the bank data
                  scene_controller_bank bankData[MAX_SCENES_IN_SCENE_CONTROLLER];
                  if(!error)
                  {
                     logDebug("No Errors for the scene controller. Get the bank data");

                     for(int bankIndex = 0; (bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER) && (!error); bankIndex++)
                     {
                        std::string bankLine;
                        scene_controller_bank bank;

                        if(std::getline(infile, bankLine))
                        {
                           // Parse each tab delimited line
                           std::stringstream ss(bankLine);
                           std::string bankToken;
                           std::vector<std::string> bankTokens;
                           while(std::getline(ss, bankToken, '\t'))
                           {
                              bankTokens.push_back(bankToken);
                           }

                           if(bankTokens.size() == 3)
                           {
                              // Parse the bank data and verify that it is valid
                              int sceneId = std::stoi(bankTokens[0]);
                              if(sceneId > MAX_NUMBER_OF_SCENES)
                              {
                                 logError("Invalid Scene index");
                                 error = true;
                              }
                              else
                              {
                                 // Verify scene exists and is valid
                                 if(sceneId > 0)
                                 {
                                    scene_properties sceneProperties;
                                    if(GetSceneProperties(sceneId, &sceneProperties))
                                    {
                                       if(!sceneProperties.SceneNameString[0])
                                       {
                                          logError("Trying to add an invalid scene %d to scene controller %d", sceneId, sceneControllerIndex);
                                          error = true;
                                       }
                                       else
                                       {
                                          logDebug("Scene %d in scene controller %d is valid", sceneId, sceneControllerIndex);
                                       }
                                    }
                                    else
                                    {
                                       logError("Could not get scene properties for scene id %d in scene controller %d", sceneId, sceneControllerIndex);
                                       error = true;
                                    }
                                 }
                              }

                              int toggleable = std::stoi(bankTokens[1]);
                              if((toggleable != 0) && (toggleable != 1))
                              {
                                 logError("Invalid toggleable value");
                                 error = true;
                              }

                              int toggled = std::stoi(bankTokens[2]);
                              if((toggled != 0) && (toggled != 1))
                              {
                                 logError("Invalid toggled value");
                                 error = true;
                              }

                              bank.sceneId = sceneId;
                              bank.toggleable = toggleable;
                              bank.toggled = toggled;

                              bankData[bankIndex] = bank;
                           }
                           else
                           {
                              logError("Bank missing tokens. %d expected, %d parsed.", 3, static_cast<int>(bankTokens.size()));
                              error = true;
                           }
                        }
                        else
                        {
                           logError("Failed to get bank data for the scene controller");
                           error = true;
                        }
                     }
                  }

                  if(!error)
                  {
                     logDebug("No Errors. Setting the scene controller properties");
                     scene_controller_properties sceneControllerProperties;
                     for(int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
                     {
                        sceneControllerProperties.bank[bankIndex].sceneId = bankData[bankIndex].sceneId;
                        sceneControllerProperties.bank[bankIndex].toggleable = bankData[bankIndex].toggleable;
                        sceneControllerProperties.bank[bankIndex].toggled = bankData[bankIndex].toggled;
                     }
                     sceneControllerProperties.groupId = groupId;
                     sceneControllerProperties.nightMode = nightMode;
                     sceneControllerProperties.deviceType = deviceType;
                     sceneControllerProperties.buildingId = buildingId;
                     sceneControllerProperties.houseId = houseId;
                     sceneControllerProperties.roomId = roomId;
                     snprintf(sceneControllerProperties.SceneControllerNameString, sizeof(sceneControllerProperties.SceneControllerNameString), "%s", tokens[7].c_str());
                     SetSceneControllerProperties(sceneControllerIndex, &sceneControllerProperties);

                     numSceneControllersLoaded++;
                  }
               }
               catch(std::invalid_argument)
               {
                  logError("Invalid argument in line %d", lineIndex);
               }
               catch(std::out_of_range)
               {
                  logError("Argument out of range in line %d", lineIndex);
               }
            }
            else
            {
               logError("Line %d missing tokens. %d expected, %d parsed.", lineIndex, 8, static_cast<int>(tokens.size()));
            }

            lineIndex++;
         }

         infile.close();

      }
      else
      {
         logError("Could not open file provided. Loading defaults and saving to file");
         numSceneControllersLoaded = loadDefaultSceneControllers();
      }
   }
   else
   {
      logInfo("No Scene Controller file provided, Loading the default scene controllers");
      numSceneControllersLoaded = loadDefaultSceneControllers();
   }
   logInfo("numSceneControllersLoaded:%d",numSceneControllersLoaded);
   return numSceneControllersLoaded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::saveAllSceneControllersToFile()
{
   bool sceneControllersSaved = false;

   if(!sceneControllerFilename.empty())
   {
      logInfo("Saving Scene Controllers to File (\"%s\")", sceneControllerFilename.c_str());

      // Prevent concurrent access to the scene controller file
      std::lock_guard<std::mutex> lock(sceneControllerFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(sceneControllerFilename, std::ofstream::out | std::ofstream::trunc);

      if(outfile.is_open())
      {
         nonvolatile_scene_controller_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile scene controller array and save any valid
         // scene controllers to the file separated by tabs
         for(byte_t sceneControllerIndex = 0; sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS; sceneControllerIndex++)
         {
            if(memcmp(&nvSceneControllerProperties[sceneControllerIndex], &uninitialized, sizeof(uninitialized)) != 0)
            {
               // Scene controller is different than the default. Save it to the file
               outfile << static_cast<int>(sceneControllerIndex) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].buildingId) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].houseId) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].groupId) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].roomId) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].deviceType) << "\t"
                       << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].nightMode) << "\t"
                       << nvSceneControllerProperties[sceneControllerIndex].SceneControllerNameString << std::endl; 

               // Save the scene data for the scene controller
               for(int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
               {
                  outfile << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].sceneId) << "\t"
                     << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggleable) << "\t"
                     << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggled) << std::endl;
               }

            }
         }

         // Close the file
         outfile.close();

         sceneControllersSaved = true;
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Scene Controller file provided, Unable to save the scene controllers to a file");
   }

   return sceneControllersSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::appendSceneControllerToFile(const byte_t &sceneControllerIndex)
{
   bool sceneControllerSaved = false;

   if(!sceneControllerFilename.empty())
   {
      logDebug("Saving Scene Controller %d to File (\"%s\")", sceneControllerIndex, sceneControllerFilename.c_str());

      // Prevent concurrent access to the scene controller file
      std::lock_guard<std::mutex> lock(sceneControllerFileMutex);

      // Open the file for appending to the end of the file
      std::ofstream outfile;
      outfile.open(sceneControllerFilename, std::ios_base::out | std::ios_base::app);

      if(outfile.is_open())
      {
         nonvolatile_scene_controller_properties uninitialized;
         memset(&uninitialized, 0xFF, sizeof(uninitialized));

         // Iterate over the nonvolatile zone array and save any valid scene controllers to the file separated by tabs
         if(memcmp(&nvSceneControllerProperties[sceneControllerIndex], &uninitialized, sizeof(uninitialized)) != 0)
         {
            // Scene controller is different than the default. Save it to the file
            outfile << static_cast<int>(sceneControllerIndex) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].buildingId) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].houseId) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].groupId) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].roomId) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].deviceType) << "\t"
               << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].nightMode) << "\t"
               << nvSceneControllerProperties[sceneControllerIndex].SceneControllerNameString << std::endl; 

            // Save the scene data for the scene controller
            for(int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
            {
               outfile << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].sceneId) << "\t"
                  << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggleable) << "\t"
                  << static_cast<int>(nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggled) << std::endl;
            }

            sceneControllerSaved = true;
         }
         else
         {
            logError("Not appending scene controller to file. Data is uninitialized");
         }

         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file for writing");
      }
   }
   else
   {
      logError("No Scene Controller file provided, Unable to save the scene controller to a file");
   }

   return sceneControllerSaved;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::eraseSceneControllerFile()
{
   if(!sceneControllerFilename.empty())
   {
      logInfo("Erasing Scene Controller File (\"%s\")", sceneControllerFilename.c_str());

      // Initialize the scene controller data to all 0xFF until it can be loaded from the file
      memset(nvSceneControllerProperties, 0xFF, sizeof(nvSceneControllerProperties));

      // Prevent concurrent access to the scene controller file
      std::lock_guard<std::mutex> lock(sceneControllerFileMutex);

      // Open the file for writing and clear any data that was previously saved
      std::ofstream outfile;
      outfile.open(sceneControllerFilename, std::ios_base::out | std::ios_base::trunc);

      if(outfile.is_open())
      {
         // File was successfully opened and cleared
         // Close the file
         outfile.close();
      }
      else
      {
         logError("Could not open the file. Might not be erased");
      }
   }
   else
   {
      logError("No Scene controller file provided, Unable to erase the scene controller file");
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

nonvolatile_scene_controller_properties * FlashStub::getNonvolatileSceneControllerPropertiesPointer(const byte_t &sceneControllerIndex, const bool &validateSceneControllerData)
{
   nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr = NULL;
   nonvolatile_scene_controller_properties uninitialized;
   memset(&uninitialized, 0xFF, sizeof(uninitialized));

   if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
   {
      if((!validateSceneControllerData) ||
         (memcmp(&nvSceneControllerProperties[sceneControllerIndex], &uninitialized, sizeof(uninitialized)) != 0))
      {
         nvSceneControllerPropertiesPtr = &nvSceneControllerProperties[sceneControllerIndex];
      }
   }

   return nvSceneControllerPropertiesPtr;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

bool FlashStub::insertDataInSceneControllerArray(const byte_t &sceneControllerIndex, nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr)
{
   bool sceneControllerDataAdded = false;

   if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
   {
      nvSceneControllerProperties[sceneControllerIndex].groupId = nvSceneControllerPropertiesPtr->groupId;
      nvSceneControllerProperties[sceneControllerIndex].deviceType = nvSceneControllerPropertiesPtr->deviceType;
      nvSceneControllerProperties[sceneControllerIndex].nightMode = nvSceneControllerPropertiesPtr->nightMode;
      nvSceneControllerProperties[sceneControllerIndex].buildingId = nvSceneControllerPropertiesPtr->buildingId;
      nvSceneControllerProperties[sceneControllerIndex].houseId = nvSceneControllerPropertiesPtr->houseId;
      nvSceneControllerProperties[sceneControllerIndex].roomId = nvSceneControllerPropertiesPtr->roomId;
      snprintf(nvSceneControllerProperties[sceneControllerIndex].SceneControllerNameString, sizeof(nvSceneControllerProperties[sceneControllerIndex].SceneControllerNameString), "%s", nvSceneControllerPropertiesPtr->SceneControllerNameString);

      for(int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
      {
         nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].sceneId = nvSceneControllerPropertiesPtr->bank[bankIndex].sceneId;
         nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggleable = nvSceneControllerPropertiesPtr->bank[bankIndex].toggleable;
         nvSceneControllerProperties[sceneControllerIndex].bank[bankIndex].toggled = nvSceneControllerPropertiesPtr->bank[bankIndex].toggled;
      }

      sceneControllerDataAdded = true;
   }

   return sceneControllerDataAdded;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::copyDataFromSceneControllerArray(nonvolatile_scene_controller_properties nvSceneControllerArray[], const size_t &arraySize)
{
   memcpy(nvSceneControllerArray, nvSceneControllerProperties, sizeof(nvSceneControllerProperties));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

void FlashStub::copyDataToSceneControllerArray(nonvolatile_scene_controller_properties nvSceneControllerArray[], const size_t &arraySize)
{
   memcpy(nvSceneControllerProperties, nvSceneControllerArray, sizeof(nvSceneControllerProperties));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int FlashStub::loadDefaultZones()
{
   int numZonesLoaded = 0;

   // Set the global house ID if it is not already set
   if(global_houseID == 0)
   {
      global_houseID = 0x10;
   }

   for(int zoneIndex = 0; zoneIndex < 10; zoneIndex++)
   {
      zone_properties zoneProperties;
      zoneProperties.buildingId = 1;
      zoneProperties.houseId = global_houseID;
      zoneProperties.roomId = 0;
      zoneProperties.groupId = zoneIndex + 1;
      zoneProperties.rampRate = 100;
      zoneProperties.deviceType = DIMMER_TYPE;
      zoneProperties.SamsungDeviceId[0] = 0x00;

      if(zoneIndex == 0)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Kitchen");
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
      }
      else if(zoneIndex == 1)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Living Room");
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.state = TRUE;
         zoneProperties.targetState = TRUE;
      }
      else if(zoneIndex == 2)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Hall Light");
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.state = TRUE;
         zoneProperties.targetState = TRUE;
      }
      else if(zoneIndex == 3)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Kids Bedroom");
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.state = TRUE;
         zoneProperties.targetState = TRUE;
      }
      else if(zoneIndex == 4)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Bathroom");
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.state = TRUE;
         zoneProperties.targetState = TRUE;
      }
      else if(zoneIndex == 5)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Master Bedroom");
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
      }
      else if(zoneIndex == 6)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Front Porch");
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
      }
      else if(zoneIndex == 7)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Back Door");
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
      }
      else if(zoneIndex == 8)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Stairs");
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
      }
      else if(zoneIndex == 9)
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Desk Light");
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.deviceType = SWITCH_TYPE;
      }
      else 
      {
         snprintf(zoneProperties.ZoneNameString, sizeof(zoneProperties.ZoneNameString), "Zone %d", zoneIndex);
         zoneProperties.state = FALSE;
         zoneProperties.targetState = FALSE;
         zoneProperties.powerLevel = 100;
         zoneProperties.targetPowerLevel = 100;
         zoneProperties.deviceType = SWITCH_TYPE;
      }


      // Set the zone properties
      SetZoneProperties(zoneIndex, &zoneProperties);
      numZonesLoaded++;
   }

   return numZonesLoaded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int FlashStub::loadDefaultScenes()
{
   int numScenesLoaded = 0;

   for(int sceneIndex = 0; sceneIndex < 4; sceneIndex++)
   {
      scene_properties sceneProperties;

      if(sceneIndex == 0)
      {
         snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "Wake Up");
         sceneProperties.nextTriggerTime = 1426746000;  // 3-20-2015 6:20
         sceneProperties.freq = EVERY_WEEK;
         sceneProperties.trigger = SUNRISE;
         sceneProperties.delta = 30;
         sceneProperties.running = false;
         sceneProperties.dayBitMask.days = 0;
         sceneProperties.dayBitMask.DayBits.monday = 1;
         sceneProperties.dayBitMask.DayBits.tuesday = 1;
         sceneProperties.dayBitMask.DayBits.wednesday = 1;
         sceneProperties.dayBitMask.DayBits.thursday = 1;
         sceneProperties.dayBitMask.DayBits.friday = 1;
      }
      else if(sceneIndex == 1)
      {
         snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "Dinner");
         sceneProperties.nextTriggerTime = 1429813800;  // 4-23-2015 18:30
         sceneProperties.freq = EVERY_WEEK;
         sceneProperties.trigger = REGULAR_TIME;
         sceneProperties.delta = 0;
         sceneProperties.running = false;
         sceneProperties.dayBitMask.days = 0;
         sceneProperties.dayBitMask.DayBits.sunday = 1;
         sceneProperties.dayBitMask.DayBits.monday = 1;
         sceneProperties.dayBitMask.DayBits.tuesday = 1;
         sceneProperties.dayBitMask.DayBits.wednesday = 1;
         sceneProperties.dayBitMask.DayBits.thursday = 1;
         sceneProperties.dayBitMask.DayBits.friday = 1;
         sceneProperties.dayBitMask.DayBits.saturday = 1;
      }
      else if(sceneIndex == 2)
      {
         snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "Go to Bed");
         sceneProperties.nextTriggerTime = 1429826400;  // 4-23-2015 22:00
         sceneProperties.freq = EVERY_WEEK;
         sceneProperties.trigger = REGULAR_TIME;
         sceneProperties.delta = 0;
         sceneProperties.running = false;
         sceneProperties.dayBitMask.days = 0;
         sceneProperties.dayBitMask.DayBits.sunday = 1;
         sceneProperties.dayBitMask.DayBits.monday = 1;
         sceneProperties.dayBitMask.DayBits.tuesday = 1;
         sceneProperties.dayBitMask.DayBits.wednesday = 1;
         sceneProperties.dayBitMask.DayBits.thursday = 1;
      }
      else if(sceneIndex == 3)
      {
         snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "Movie");
         sceneProperties.nextTriggerTime = 1429807500;  // 4-25-2015 16:45
         sceneProperties.freq = EVERY_WEEK;
         sceneProperties.trigger = SUNSET;
         sceneProperties.delta = 90;
         sceneProperties.running = false;
         sceneProperties.dayBitMask.days = 0;
         sceneProperties.dayBitMask.DayBits.sunday = 1;
         sceneProperties.dayBitMask.DayBits.monday = 1;
         sceneProperties.dayBitMask.DayBits.tuesday = 1;
         sceneProperties.dayBitMask.DayBits.wednesday = 1;
         sceneProperties.dayBitMask.DayBits.thursday = 1;
         sceneProperties.dayBitMask.DayBits.friday = 1;
         sceneProperties.dayBitMask.DayBits.saturday = 1;
      }
      else 
      {
         snprintf(sceneProperties.SceneNameString, sizeof(sceneProperties.SceneNameString), "Scene %d", sceneIndex);
         sceneProperties.nextTriggerTime = 1429807500;  // 4-25-2015 16:45
         sceneProperties.freq = EVERY_WEEK;
         sceneProperties.trigger = SUNSET;
         sceneProperties.delta = 90;
         sceneProperties.running = false;
         sceneProperties.dayBitMask.days = 0;
         sceneProperties.dayBitMask.DayBits.sunday = 1;
         sceneProperties.dayBitMask.DayBits.monday = 1;
         sceneProperties.dayBitMask.DayBits.tuesday = 1;
         sceneProperties.dayBitMask.DayBits.wednesday = 1;
         sceneProperties.dayBitMask.DayBits.thursday = 1;
         sceneProperties.dayBitMask.DayBits.friday = 1;
         sceneProperties.dayBitMask.DayBits.saturday = 1;
      }

      sceneProperties.skipNext = false;
      sceneProperties.running = false;

      // Set the zone data for the scenes
      sceneProperties.numZonesInScene = 4;
      bool zoneError = false;
      for(int zoneIndex = 0; (zoneIndex < sceneProperties.numZonesInScene) && (!zoneError); zoneIndex++)
      {         
         byte_t zoneId = sceneIndex + zoneIndex;
         zone_properties zoneProperties;

         sceneProperties.zoneData[zoneIndex].zoneId = zoneId;
         sceneProperties.zoneData[zoneIndex].powerLevel = (sceneIndex + 1) * 2;
         sceneProperties.zoneData[zoneIndex].rampRate = 100;
         sceneProperties.zoneData[zoneIndex].state = TRUE;

         if(GetZoneProperties(zoneId, &zoneProperties))
         {
            if(!zoneProperties.ZoneNameString[0])
            {
               logError("Trying to add an invalid zone %d to scene %d", zoneId, sceneIndex);
               zoneError = true;
            }
            else
            {
               logDebug("Zone %d in scene %d is valid", zoneId, sceneIndex);
            }
         }
         else
         {
            logError("Could not get zone properties for zone id %d in scene %d", zoneId, sceneIndex);
            zoneError = true;
         }
      }

      // Set the scene properties if there were no errors
      if(!zoneError)
      {
         SetSceneProperties(sceneIndex, &sceneProperties);
         numScenesLoaded++;
      }
      else
      {
         logError("No loading scene data because of invalid zones. Erase the scene");
         memset(sceneProperties.SceneNameString, 0x00, sizeof(sceneProperties.SceneNameString));
         SetSceneProperties(sceneIndex, &sceneProperties);
      }
   }

   return numScenesLoaded;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int FlashStub::loadDefaultSceneControllers()
{
   int numSceneControllersLoaded = 0;

   // Set the global house ID if it is not already set
   if(global_houseID == 0)
   {
      global_houseID = 0x10;
   }
	
   scene_controller_properties sceneControllerProperties;
  
   for (int ix=0; ix < 4; ix++)
   {
      sceneControllerProperties.buildingId = 1;
      sceneControllerProperties.houseId = global_houseID;
      sceneControllerProperties.roomId = 0;
      sceneControllerProperties.groupId = 100;
      sceneControllerProperties.nightMode = 0;
	  sceneControllerProperties.deviceType = SCENE_CONTROLLER_4_BUTTON_TYPE;
      snprintf(sceneControllerProperties.SceneControllerNameString, sizeof(sceneControllerProperties.SceneControllerNameString), "SC %d", ix);

      for(int bankIndex = 0; bankIndex < MAX_SCENES_IN_SCENE_CONTROLLER; bankIndex++)
      {
         if(bankIndex == 0)
         {
            sceneControllerProperties.bank[bankIndex].sceneId = 0;
         }
         else
         {
            sceneControllerProperties.bank[bankIndex].sceneId = -1;
         }

         sceneControllerProperties.bank[bankIndex].toggleable = 1;
         sceneControllerProperties.bank[bankIndex].toggled = 0;
      }

      // Set the scene controller properties
      SetSceneControllerProperties(ix, &sceneControllerProperties);
      numSceneControllersLoaded++;
   }

   return numSceneControllersLoaded;
}
