#ifndef __ZONES_H__
#define __ZONES_H__

#if (__cplusplus)
extern "C" {
#endif

void InitializeZoneArray();
bool_t AnyZonesInArray(void);
byte_t GetAvailableZoneIndex();
byte_t GetZoneIndex(word_t groupId, byte_t buildingId, byte_t houseId);
bool_t GetZoneProperties(byte_t zoneIndex, zone_properties *zonePropertiesPtr);
bool_t SetZoneProperties(byte_t zoneIndex, zone_properties *zonePropertiesPtr);
uint8_t LoadZonesFromFlash();
uint8_t GetZoneIndexFromDeviceId(const char * deviceId);

#if (__cplusplus)
}
#endif

#endif // __ZONES_H__
