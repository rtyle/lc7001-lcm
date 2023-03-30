#ifndef __SCENECTRL_H__
#define __SCENECTRL_H__

#if (__cplusplus)
extern "C" {
#endif

void InitializeSceneControllerArray();
bool_t AnySceneControllersInArray();
byte_t GetAvailableSceneControllerIndex();
byte_t GetSceneControllerIndex(word_t groupId, byte_t buildingId, byte_t houseId);
byte_t FindGroupIDInSceneControllerArray(word_t groupWanted);
bool_t GetSceneControllerProperties(byte_t sceneIndex, scene_controller_properties *scenePropertiesPtr);
bool_t SetSceneControllerProperties(byte_t sceneIndex, scene_controller_properties *scenePropertiesPtr);
uint8_t LoadSceneControllersFromFlash();
#if (__cplusplus)
}
#endif

#endif // __SCENECTRL_H__
