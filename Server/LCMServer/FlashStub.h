#ifndef _FLASH_STUB_H_
#define _FLASH_STUB_H_

#include <string>

#include "defines.h"
#include "typedefs.h"

class FlashStub
{
   public:
      FlashStub(const std::string &zoneFilename, const std::string &sceneFilename, const std::string &sceneControllerFilename, const std::string &samsungZonePropertiesFilename, const std::string &samsungScenePropertiesFilename);
      virtual ~FlashStub();

      void ClearFlash();

      // Zones
      bool saveAllZonesToFile();
      bool appendZoneToFile(const byte_t &zoneIndex);
      void eraseZoneFile();
      nonvolatile_zone_properties * getNonvolatileZonePropertiesPointer(const byte_t &zoneIndex, const bool &validateZoneData);
      bool insertDataInZoneArray(const byte_t &zoneIndex, nonvolatile_zone_properties * zonePropertiesPtr);
      void copyDataFromZoneArray(nonvolatile_zone_properties nvZoneArray[], const size_t &arraySize);
      void copyDataToZoneArray(nonvolatile_zone_properties nvZoneArray[], const size_t &arraySize);

      // Samsung Zone Properties
      bool saveAllSamsungZonePropertiesToFile();
      bool appendSamsungZonePropertiesToFile(const byte_t &zoneIndex);
      void eraseSamsungZonePropertiesFile(const byte_t &zoneIndex);
      nonvolatile_samsung_zone_properties * getNonvolatileSamsungZonePropertiesPointer(const byte_t &zoneIndex, const bool &validateZoneData);
      bool insertDataInSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties * samsungZonePropertiesPtr);
      bool copyDataFromSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZonePropertiesArray[], const size_t &arraySize);
      bool copyDataToSamsungZonePropertiesArray(const byte_t &zoneIndex, nonvolatile_samsung_zone_properties nvSamsungZonePropertiesArray[], const size_t &arraySize);

      // Scenes
      bool saveAllScenesToFile();
      bool appendSceneToFile(const byte_t &sceneIndex);
      void eraseSceneFile(const byte_t &sceneIndex);
      nonvolatile_scene_properties * getNonvolatileScenePropertiesPointer(const byte_t &sceneIndex, const bool &validateSceneData);
      bool insertDataInSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties * scenePropertiesPtr);
      bool copyDataFromSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties nvSceneArray[], const size_t &arraySize);
      bool copyDataToSceneArray(const byte_t &sceneIndex, nonvolatile_scene_properties nvSceneArray[], const size_t &arraySize);

      // Samsung Scene Properties
      bool saveAllSamsungScenePropertiesToFile();
      bool appendSamsungScenePropertiesToFile(const byte_t &sceneIndex);
      void eraseSamsungScenePropertiesFile(const byte_t &sceneIndex);
      nonvolatile_samsung_scene_properties * getNonvolatileSamsungScenePropertiesPointer(const byte_t &sceneIndex, const bool &validateSceneData);
      bool insertDataInSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties * samsungScenePropertiesPtr);
      bool copyDataFromSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties nvSamsungScenePropertiesArray[], const size_t &arraySize);
      bool copyDataToSamsungScenePropertiesArray(const byte_t &sceneIndex, nonvolatile_samsung_scene_properties nvSamsungScenePropertiesArray[], const size_t &arraySize);

      // Scene Controllers
      bool saveAllSceneControllersToFile();
      bool appendSceneControllerToFile(const byte_t &sceneControllerIndex);
      void eraseSceneControllerFile();
      nonvolatile_scene_controller_properties * getNonvolatileSceneControllerPropertiesPointer(const byte_t &sceneControllerIndex, const bool &validateSceneControllerData);
      bool insertDataInSceneControllerArray(const byte_t &sceneControllerIndex, nonvolatile_scene_controller_properties * sceneControllerPropertiesPtr);
      void copyDataFromSceneControllerArray(nonvolatile_scene_controller_properties nvSceneControllerArray[], const size_t &arraySize);
      void copyDataToSceneControllerArray(nonvolatile_scene_controller_properties nvSceneControllerArray[], const size_t &arraySize);

   private:
      int loadAllZonesFromFile();
      int loadAllSamsungZonePropertiesFromFile();
      int loadAllSamsungScenePropertiesFromFile();
      int loadDefaultZones();
      int loadAllScenesFromFile();
      int loadDefaultScenes();
      int loadAllSceneControllersFromFile();
      int loadDefaultSceneControllers();

      std::string zoneFilename;
      std::mutex zoneFileMutex;
      std::string samsungZonePropertiesFilename;
      std::mutex samsungZonePropertiesFileMutex;
      std::string sceneFilename;
      std::mutex sceneFileMutex;
      std::string samsungScenePropertiesFilename;
      std::mutex samsungScenePropertiesFileMutex;
      std::string sceneControllerFilename;
      std::mutex sceneControllerFileMutex;

      nonvolatile_zone_properties nvZoneProperties[MAX_NUMBER_OF_ZONES];
      nonvolatile_samsung_zone_properties nvSamsungZoneProperties[MAX_NUMBER_OF_ZONES];
      nonvolatile_scene_properties nvSceneProperties[MAX_NUMBER_OF_SCENES];
      nonvolatile_samsung_scene_properties nvSamsungSceneProperties[MAX_NUMBER_OF_SCENES];
      nonvolatile_scene_controller_properties nvSceneControllerProperties[MAX_NUMBER_OF_SCENE_CONTROLLERS];
};

#endif
