/*
 * Eliot_REST_Task.h
 *
 *  Created on: Mar 7, 2019
 *      Author: Legrand
 */

#ifndef ELIOT_REST_TASK_H_
#define ELIOT_REST_TASK_H_

#include "includes.h"

#define Eliot_QueueAddZone(index)      Eliot_SendTaskMessage(ELIOT_COMMAND_ADD_ZONE, index, NULL);
#define Eliot_QueueAddScene(index)     Eliot_SendTaskMessage(ELIOT_COMMAND_ADD_SCENE, index, NULL);
#define Eliot_QueueZoneChanged(index)  Eliot_SendTaskMessage(ELIOT_COMMAND_ZONE_CHANGED, index, NULL);
#define Eliot_QueueSceneStarted(index) Eliot_SendTaskMessage(ELIOT_COMMAND_SCENE_STARTED, index, NULL);
#define Eliot_QueueRenameZone(index)   Eliot_SendTaskMessage(ELIOT_COMMAND_RENAME_ZONE, index, NULL);
#define Eliot_QueueRenameScene(index)  Eliot_SendTaskMessage(ELIOT_COMMAND_RENAME_SCENE, index, NULL);


void Eliot_QueueSceneFinished(_timer_id, void*, uint32_t, uint32_t);

bool_t Eliot_SendTaskMessage(byte_t, byte_t, json_t*);

void Eliot_REST_Task();
int32_t Eliot_LcmPlantAndModuleInit();
uint32_t Eliot_TimeNow(void);
void Eliot_QueueDeleteDevice(const char*);
void Eliot_LogOut(void);

extern char *eliot_primary_key;
extern char eliot_lcm_plant_id[ELIOT_DEVICE_ID_SIZE];
extern char eliot_lcm_module_id[ELIOT_DEVICE_ID_SIZE];

extern uint32_t eliot_user_token_modified_time;
extern uint32_t eliot_refresh_token_error_time;
extern uint32_t eliot_refresh_token_modified_time;
extern uint32_t eliot_last_quota_exceeded_time;
extern FBK_Context eliot_fbk;


#define ELIOT_USER_TOKEN_LIFESPAN              3240     // 54 minutes
#define ELIOT_REFRESH_TOKEN_LIFESPAN           28800    // 8 hours
#define ELIOT_QUOTA_EXCEEDED_PERIOD            3600     // 1 hour

#define ELIOT_CONNECT_ALT_HOST_TOKEN_REFRESH   0xFFFFFFAA
#define ELIOT_AUTH_PROFILE                     "B2C_1_RFLightingControl-SignUporSignIn"
#define MICROSOFT_TENANT_ID                    "0d8816d5-3e7f-4c86-8229-645137e0f222"
#define ELIOT_TOKEN_REFRESH_HOST               "login.eliotbylegrand.com"
#define ELIOT_TOKEN_REFRESH_URL                "https://"ELIOT_TOKEN_REFRESH_HOST"/"MICROSOFT_TENANT_ID"/"ELIOT_AUTH_PROFILE"/oauth2/v2.0/token"
#define ELIOT_CLIENT_ID                        "570b91b9-7b09-4928-8612-955b3b85790c"
#define ELIOT_LOOP_INTERVAL_MASK               0x01FF
#define ELIOT_JSR_BLOCK_SIZE                   32
#define ELIOT_CONNECT_WITH_RETRY_INITIAL_DELAY 250
#define ELIOT_CONNECT_WITH_RETRY_ATTEMPTS      5

#endif /* ELIOT_REST_TASK_H_ */
