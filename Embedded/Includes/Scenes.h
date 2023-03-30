#ifndef __SCENES_H__
#define __SCENES_H__

#if (__cplusplus)
extern "C" {
#endif

void InitializeSceneArray();
byte_t GetAvailableSceneIndex();
bool_t GetSceneProperties(byte_t sceneIndex, scene_properties *scenePropertiesPtr);
bool_t SetSceneProperties(byte_t sceneIndex, scene_properties *scenePropertiesPtr);
uint8_t LoadScenesFromFlash();
uint8_t GetSceneIndexFromDeviceId(const char * deviceId);

#if (__cplusplus)
}
#endif

#endif // __SCENES_H__
