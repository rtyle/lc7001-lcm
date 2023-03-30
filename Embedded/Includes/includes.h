/*
 * includes.h
 *
 *  Created on: Jun 17, 2013
 *      Author: Legrand
 */

#ifndef INCLUDES_H_
#define INCLUDES_H_

//-----------------------------------------------------------------------------
#ifndef AMDBUILD
#include <mqx.h>
#include <bsp.h>
#include <lwevent.h>
#include <timer.h>
#include <event.h>
#include <message.h>
#include <mutex.h>
#include <stdlib.h>
#include <ipcfg.h>
#if MQX_KERNEL_LOGGING
#include <klog.h>
#endif
#else
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif // AMDBUILD

#include "rflc_scenes.h"
#include "defines.h"
#include "sysinfo.h"
#include "onq_endian.h"
#include "onq_standard_types.h"
#include "jansson.h"
#include "typedefs.h"
#include "globals.h"

#include "config.h"

//-----------------------------------------------------------------------------

//#include "jansson.h"

//-----------------------------------------------------------------------------

#include "RTCS.h"

#include "Crc.h"
#include "Flash.h"
#include "Zones.h"
#include "Scenes.h"
#include "SceneController.h"
#include "RFM100_Tasks.h"
#include "Socket_Task.h"
#include "I2C.h"
#include "Event_Task.h"
#include "CC1110_Programmer.h"

//-----------------------------------------------------------------------------

#endif /* INCLUDES_H_ */
