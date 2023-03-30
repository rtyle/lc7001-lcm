/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
* Copyright (c) 2004-2008 Embedded Access Inc.;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: Tasks.c$
* $Version : 3.7.9.0$
* $Date    : Jan-17-2011$
*
* Comments:
*
*   
*
*END************************************************************************/

#define MAIN
#include "includes.h"
#include "Eliot_REST_Task.h"
#include "Eliot_MQTT_Task.h"

// MQX task initialization information

const TASK_TEMPLATE_STRUCT MQX_template_list[] =
{
//  Task Index,               Function,               Stack,  Priority, Name,                   Attributes,             Param,  Time Slice */

#if DEMOCFG_ENABLE_SERIAL_SHELL
//   {SHELL_TASK,             Shell_Task,             2500,   14,      "Shell",                 MQX_AUTO_START_TASK,    0,      0           },
#endif

   #if DEMOCFG_ENABLE_AUTO_LOGGING
   {LOGGING_TASK,             Logging_task,           2500,   11,      "Logging",               0,                      0,      0           },
#endif

   {RS232_RECEIVE_TASK,       RS232Receive_Task,      1000,   12,      "RS232Receive",          0,                      0,      0           },

   {RFM100_RECEIVE_TASK,      RFM100Receive_Task,     3000,   13,      "RFM100Receive",         0,                      0,      0           },

   {RFM100_TRANSMIT_TASK,     RFM100Transmit_Task,    3000,   14,      "RFM100Transmit",        0,                      0,      0           },

   {EVENT_CONTROLLER_TASK,    EventController_Task,   3000,   15,      "EventController",       MQX_AUTO_START_TASK,    0,      0           },

   {SOCKET_TASK,              Socket_Task,            4000,   18,      "Socket",                0,                      0,      0           },

   {JSON_ACCEPT_TASK,         JSONAccept_Task,        1000,   16,      "JsonAccept",            0,                      0,      0           },

   {WATCH_ACCEPT_TASK,        JSONAccept_Task,        1000,   16,      "WatchAccept",           0,                      0,      0           },

   {FIRMWARE_UPDATE_TASK,     FirmwareUpdate_Task,    2500,   18,      "FirmwareUpdate",        0,                      0,      0           },

   {MDNS_TASK,                MDNS_Task,              2000,   10,      "MDNS",                  0,                      0,      0           },

   {JSON_TRANSMIT_TASK,       JSONTransmit_Task,      2000,   18,      "JSONTranmsit",          0,                      0,      0           },
 
   {EVENT_TIMER_TASK,         Event_Timer_Task,       4000,   18,      "Event_Timer",           0,                      0,      0           },

   {ELIOT_MQTT_TASK,          Eliot_MQTT_Task,        14000,  18,      "ELIOT_MQTT",            0,                      0,      0           },

   {ELIOT_REST_TASK,          Eliot_REST_Task,        14000,  18,      "ELIOT_REST",            0,                      0,      0           },
   
   {0}

};



 
/* EOF */
