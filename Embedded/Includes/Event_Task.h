/*
 * Event_Task.h
 *
 *  Created on: Mar 31, 2015
 *      Author: Legrand
 */

#ifndef EVENT_TASK_H_
#define EVENT_TASK_H_

#ifndef AMDBUILD

// Embedded Code Only
void Event_Timer_Task();

#endif

#if (__cplusplus)
extern "C"
{
#endif

void broadcastDebug(int num, const char *serviceStr, const char *dataStr);
void broadcastMemoryUse(void);
void broadcastDiagnostics(void);

// Shared between PC and Embedded Build
void broadcastNewSceneTriggerTime(int sceneIndex, scene_properties * scenePropertiesPtr);
void IncrementMinuteCounter();
void HandleMinuteChange();
void AlignToMinuteBoundary();
boolean getLocalTime(uint32_t timeout);
void adjustTriggers(int32_t adjustment);
void checkScenes(TIME_STRUCT  rtcTime);
void calculateNewTrigger(scene_properties * scenePropertiesPtr, TIME_STRUCT rtcTime); 
boolean getNextSunTimes(DATE_STRUCT myCurrentDate, boolean sunriseFlag, DATE_STRUCT *newDate);
boolean isItDST(DATE_STRUCT myTime);
void checkForDST(DATE_STRUCT myTime);

#if (__cplusplus)
}
#endif

#endif /* EVENT_TASK_H_ */
