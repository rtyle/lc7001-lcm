/*
 * Eliot_Common.c
 *
 *  Created on: Mar 8, 2019
 *      Author: Legrand
 */

#include "Eliot_Common.h"
#include "includes.h"

const char eliot_empty_string[] = "";            // A safe place to point char* strings on init.
ELIOT_ERROR_COUNT eliot_error_count;
const char *err_label[] = ELIOT_ERROR_LABELS;
ELIOT_LED_STATE eliot_led_state = {0,0,0,0,0};
char eliot_user_token[ELIOT_TOKEN_SIZE];
char *eliot_refresh_token = eliot_empty_string;
int last_metrics_scene_index = -1;
int last_metrics_zone_index = -1;


// Reset all error counts to zero
void Eliot_ResetErrorCounters(void)
{
	memset(&eliot_error_count, 0, sizeof(ELIOT_ERROR_COUNT));
	DBGPRINT_INTSTRSTR(__LINE__, "Eliot_ResetErrorCounters()", "");
}


// Print the contents of eliot_error_count to the console as a JSON object.
void Eliot_PrintErrorCounters(void)
{
	uint32_t count;
	uint32_t x = 0;
	json_t * broadcastObject = json_object();

	json_object_set_new(broadcastObject, "Service", json_string("EliotErrors"));
	json_object_set_new(broadcastObject, "CurrentTime", json_integer(Eliot_TimeNow()));

	for(x=0; x<sizeof(ELIOT_ERROR_COUNT)>>2; x++)
	{
		count = *(uint32_t*)((x<<2)+(int)&eliot_error_count);

		if(count)
		{
			json_object_set_new(broadcastObject, err_label[x], json_integer(count));
		}
	}

	SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
	json_delete(broadcastObject);
}


void AdjustDeviceLevel(const char* deviceId, short delta)
{
    _mutex_lock(&ZoneArrayMutex);
    for(uint8_t zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
    {
        zone_properties zoneProperties;
        if(GetZoneProperties(zoneIndex, &zoneProperties))
        {
            if(zoneProperties.ZoneNameString[0] && zoneProperties.EliotDeviceId[0])
            {
                if (strcmp(zoneProperties.EliotDeviceId, deviceId) == 0)
                {
                    short newValue = zoneProperties.targetPowerLevel + delta;
                    if (newValue > 100)
                    {
                        zoneProperties.targetPowerLevel = 100;
                    }
                    else if (newValue < 1)
                    {
                        //If the targetPowerLevel is <1, i.e. 0, turn device off.
                        zoneProperties.targetState = false;
                    }
                    else
                    {
                        zoneProperties.targetPowerLevel = newValue;
                    }

                    SetZoneProperties(zoneIndex, &zoneProperties);

                    // Send the command to the RFM100 transmit task
                    SendRampCommandToTransmitTask(zoneIndex, (MAX_NUMBER_OF_SCENES + 1), 0, RFM100_MESSAGE_HIGH_PRIORITY);

                    if (last_metrics_zone_index == zoneIndex)
                    {
                        // Force this zone's merics to be sent
                        last_metrics_zone_index = -1;
                    }

                    break;
                }
            }
        }
    }
    _mutex_unlock(&ZoneArrayMutex);
}

void SetDeviceState(const char* deviceId, short state, short level)
{
    _mutex_lock(&ZoneArrayMutex);
    for(uint8_t zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
    {
        zone_properties zoneProperties;
        if(GetZoneProperties(zoneIndex, &zoneProperties))
        {
            if(zoneProperties.ZoneNameString[0] && zoneProperties.EliotDeviceId[0])
            {
                if (strcmp(zoneProperties.EliotDeviceId, deviceId) == 0)
                {
                    // Set the target state to the requested state
                    if (state >= 0) {
                        zoneProperties.targetState = state;
                    }

                    if (level >= 0) {
                        zoneProperties.targetPowerLevel = (level & 0xFF);
                    }

                    zoneProperties.rampRate = (50 & 0xFF);

                    // Save the zone properties
                    SetZoneProperties(zoneIndex, &zoneProperties);

                    switch(zoneProperties.deviceType)
                    {
                        case SHADE_GROUP_TYPE:
                        case SHADE_TYPE:
                            SendShadeCommand(zoneIndex, zoneProperties.targetPowerLevel);
                            break;
                            
                        default:
                            // Send the command to the RFM100 transmit task
                            SendRampCommandToTransmitTask(zoneIndex, (MAX_NUMBER_OF_SCENES + 1), 0, RFM100_MESSAGE_HIGH_PRIORITY);
                            break;
                    }

                    if (last_metrics_zone_index == zoneIndex)
                    {
                        // Force this zone's merics to be sent
                        last_metrics_zone_index = -1;
                    }

                    break;
                }
            }
        }
    }
    _mutex_unlock(&ZoneArrayMutex);
}

int FindZoneIndexById(const char* deviceId)
{
    zone_properties zoneProperties;

    for(uint8_t zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
    {
        if(GetZoneProperties(zoneIndex, &zoneProperties))
        {
            if(zoneProperties.ZoneNameString[0] && zoneProperties.EliotDeviceId[0])
            {
                if (strcmp(zoneProperties.EliotDeviceId, deviceId) == 0)
                {
                    return (int)zoneIndex;
                }
            }
        }
    }

    return -1;
}

int FindSceneIndexById(const char* deviceId)
{
    scene_properties sceneProperties;

    for(uint8_t sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
    {
        if(GetSceneProperties(sceneIndex, &sceneProperties))
        {
            if(sceneProperties.SceneNameString[0] && sceneProperties.EliotDeviceId[0])
            {
                if (strcmp(sceneProperties.EliotDeviceId, deviceId) == 0)
                {
                    return (int)sceneIndex;
                }
            }
        }
    }

    return -1;
}

void SetSceneState(const char* deviceId, const bool_t state)
{
   _mutex_lock(&ScenePropMutex);
   for(uint8_t sceneIndex = 0; sceneIndex < MAX_NUMBER_OF_SCENES; sceneIndex++)
   {
      scene_properties sceneProperties;
      if(GetSceneProperties(sceneIndex, &sceneProperties))
      {
         if(sceneProperties.SceneNameString[0] &&
            sceneProperties.EliotDeviceId[0])
         {
            if(!strcmp(sceneProperties.EliotDeviceId, deviceId))
            {
               ExecuteScene(sceneIndex, &sceneProperties, 0 /*appContextId*/, state, 0 /*value*/, 0 /*change*/, 0 /*rampRate*/);

               // Save the scene properties
               SetSceneProperties(sceneIndex, &sceneProperties);

               // Break the loop because the scene was found
	       break;
            }
         }
      }
   }
   _mutex_unlock(&ScenePropMutex);
}


// This function turns off the L2 LED to indicate network activity.  L2
// is restored by Eliot_LedSet().
int32_t Eliot_LedIndicateActivity(void)
{
	if(!eliot_led_state.error_flags)
	{
		eliot_led_state.activity_holdoff = ELIOT_LED_ACTIVITY_DURATION;
		SetGPIOOutput(led1Red, LED_OFF);
		SetGPIOOutput(led1Green, LED_OFF);
	}
	return ELIOT_RET_OK;
}


// Eliot_LedSet() controls the L2 LED output under normal operation.  It's called
// from EventController_Task in RFM100_Tasks.c so that blocking operations in
// REST/MQTT will not stall flashing.  Note: Using broadcastDebug() inside of
// this function will prevent the LCM from booting.
int32_t Eliot_LedSet(void)
{
	TIME_STRUCT mqxTime;
	_time_get(&mqxTime);
	int x=0;

	uint8_t flash_clock = (mqxTime.MILLISECONDS >>8);

	// When error_flags b2:b31 is set, L2 will flash the number of the bit(s) set
	// sequentially, overriding the normal MQTT connection state indication.
	// For example, a value of 0x48 will cause L2 to flash red six times, pause,
	// flash red three more times and repeat the cycle.  Bits 0 and 1 have no
	// effect.

	if(eliot_led_state.error_flags & 0xFFFFFFFC)
	{
		SetGPIOOutput(led1Green, LED_OFF);

		if(flash_clock != eliot_led_state.clock_last)
		{
			eliot_led_state.countdown--;

			if(eliot_led_state.countdown>=0)
			{
				SetGPIOOutput(led1Red, eliot_led_state.countdown & 1);
			}
			else
			{
				SetGPIOOutput(led1Red, LED_OFF);

				if(eliot_led_state.countdown < 0-ELIOT_LED_ERROR_SPACING)
				{
					// Don't use bits 0 or 1
					while(eliot_led_state.active_code > 2)
					{
						eliot_led_state.active_code--;
						if(1 & (eliot_led_state.error_flags >> eliot_led_state.active_code))
						{
							eliot_led_state.countdown = eliot_led_state.active_code << 1;
							break;
						}
					}

					if(eliot_led_state.active_code <= 2)
					{
						eliot_led_state.active_code = 31;
					}
				}
			}

			eliot_led_state.clock_last = flash_clock;
		}
		return ELIOT_RET_OK;
	}

	if(eliot_led_state.activity_holdoff)
	{
		// Green LED has been turned off to indicate network activity.
		// Don't turn it back on until this countdown reaches zero.
		eliot_led_state.activity_holdoff--;
		return ELIOT_RET_OK;
	}

	if(!eliot_refresh_token[0])
	{
		// Off
		SetGPIOOutput(led1Red, LED_OFF);
		SetGPIOOutput(led1Green, LED_OFF);
		return ELIOT_RET_OK;
	}

	if(Eliot_MqttIsConnected())
	{
		// Green
		SetGPIOOutput(led1Red, LED_OFF);
		SetGPIOOutput(led1Green, LED_ON);
	}
	else
	{
		// Flashing amber
		SetGPIOOutput(led1Red, mqxTime.MILLISECONDS & 0x80);
		SetGPIOOutput(led1Green, mqxTime.MILLISECONDS & 0x80);
	}

	return ELIOT_RET_OK;
}

/** 
 * logConnectionSequence()
 * A function to set the Connection Log Sequence Number and broadcast a debug message
 * @param int SequenceNum Current Sequence Number
 * @param const char *dataStr The message to broadcast
**/
void logConnectionSequence(int SequenceNum, const char *dataStr)
{
	connectionSequenceNum = SequenceNum;
	broadcastDebug(connectionSequenceNum, ": Connection Log", dataStr);
}

