/*
 * RFM100_Tasks.h
 *
 *  Created on: Jun 17, 2013
 *      Author: Legrand
 */

#ifndef RFM100_TASKS_H_
#define RFM100_TASKS_H_

//-----------------------------------------------------------------------------

#ifndef AMDBUILD

// Embedded Code Only
void RS232Receive_Task(uint_32 param);

int_32 RFM100_Send(byte_t * outBytes, byte_t length);

void PublishAckToQueue(byte_t ackByte);

void RFM100Receive_Task(uint_32 param);

void RFM100Transmit_Task(uint_32 param);

void EventController_Task(uint_32 param);

#else

#if (__cplusplus)
extern "C"
{
#endif

int _msgq_send_priority(RFM100_TRANSMIT_TASK_MESSAGE_PTR message, uint8_t priority);

int RFM100_Send(byte_t * buffer, int length);

#if (__cplusplus)
}
#endif

#endif

// Code shared between server and embedded code
#if (__cplusplus)
extern "C"
{
#endif

byte_t CalculateCRC(byte_t *buffer, int buffSize);

void InitializeSystemProperties(void);

void InitializeSceneControllerArray(void);

bool_t SendSceneExecutedToTransmitTask(scene_controller_properties *sceneControllerProperties, byte_t sceneId, byte_t avgLevel, bool_t state);

bool_t SendRampCommandToTransmitTask(byte_t zoneIndex, byte_t sceneIndex, byte_t scenePercent, uint8_t priority);

void BuildRampAll(byte_t buildingId, byte_t roomId, byte_t houseId, byte_t powerLevel);

bool_t HandleTransmitTaskMessage(RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr, bool_t forceSendFlag);

bool_t SendAssignSceneCommandToTransmitTask(scene_controller_properties *sceneControllerProperties, byte_t sceneId, byte_t avgLevel);

bool_t SendSceneSettingsCommandToTransmitTask(scene_controller_properties *sceneControllerProperties, bool_t nightMode, bool_t clearScene);

#if (__cplusplus)
}
#endif


//-----------------------------------------------------------------------------

#endif /* RFM100_TASKS_H_ */
