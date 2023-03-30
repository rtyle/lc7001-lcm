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
 * $FileName: HVAC_Task.c$
 * $Version : 3.8.24.0$
 * $Date    : Sep-20-2011$
 *
 * Comments:
 *
 *   
 *
 *END************************************************************************/

#include "includes.h"
#include "Eliot_REST_Task.h"
#ifndef AMDBUILD

#if ! BSPCFG_ENABLE_IO_SUBSYSTEM
#error This application requires BSPCFG_ENABLE_IO_SUBSYSTEM defined non-zero in user_config.h. Please recompile BSP with this option.
#endif

#ifndef BSP_DEFAULT_IO_CHANNEL_DEFINED
#error This application requires BSP_DEFAULT_IO_CHANNEL to be not NULL. Please set corresponding BSPCFG_ENABLE_TTYx to non-zero in user_config.h and recompile BSP with this option.
#endif

#if DEMOCFG_ENABLE_KLOG
#if !MQX_KERNEL_LOGGING
#warning Need MQX_KERNEL_LOGGING enabled in kernel (user_config.h and user_config.cw)
#endif
#if !defined(DEMOCFG_KLOG_ADDR) || !defined(DEMOCFG_KLOG_SIZE)
#warning Need klog address and size
#endif
#endif

#ifdef MESSAGE_EXCHANGE_EXAMPLE
#define SERVER_QUEUE 8
#define CLIENT_QUEUE_BASE 9
#endif

//=================
// Debug settings
//=================
#define DEBUG_RFM100_ANY            0 // check this in as 0
#define DEBUG_RFM100                (1 && DEBUG_RFM100_ANY)
#define DEBUG_RFM100_ERROR          (1 && DEBUG_RFM100_ANY)
#define DEBUG_RFM100_MSGQ           (0 && DEBUG_RFM100_ANY) // CAVEAT slows down LCM detection by App
#define DEBUG_ZONEARRAY_MUTEX       (0 && DEBUG_RFM100_ANY)
#define DEBUG_QUBE_MUTEX            (0 && DEBUG_RFM100_ANY)
#define DEBUG_QUBE_MUTEX_MAX_MS          10
#define DEBUG_ZONEARRAY_MUTEX_MAX_MS     10

#if DEBUG_RFM100_ANY
#define DEBUGBUF                              debugBuf
#define SIZEOF_DEBUGBUF                       sizeof(debugBuf)
#define DEBUG_BEGIN_TICK         debugLocalStartTick
#define DEBUG_END_TICK           debugLocalEndTick
#define DEBUG_TICK_OVERFLOW      debugLocalOverflow
#define DEBUG_TICK_OVERFLOW_MS   debugLocalOverflowMs
// declare these locally: MQX_TICK_STRUCT            debugLocalStartTick;
// declare these locally: MQX_TICK_STRUCT            debugLocalEndTick;
// declare these locally: bool                       debugLocalOverflow;
// declare these locally: uint32_t                   debugLocalOverflowMs;
#else
char nonDebugDummyBuf[2];
#define DEBUGBUF                              nonDebugDummyBuf
#define SIZEOF_DEBUGBUF                       sizeof(nonDebugDummyBuf)
MQX_TICK_STRUCT                  nonDebugStartTick;
MQX_TICK_STRUCT                  nonDebugEndTick;
bool                             nonDebugOverflow;
uint32_t                         nonDebugOverflowMs;
#define DEBUG_BEGIN_TICK         nonDebugStartTick
#define DEBUG_END_TICK           nonDebugEndTick
#define DEBUG_TICK_OVERFLOW      nonDebugOverflow
#define DEBUG_TICK_OVERFLOW_MS   nonDebugOverflowMs
#endif

#define DBGPRINT_INTSTRSTR if (DEBUG_RFM100) broadcastDebug
#define DBGPRINT_INTSTRSTR_ERROR if (DEBUG_RFM100_ERROR) broadcastDebug
#define DBGPRINT_INTSTRSTR_MSGQ if (DEBUG_RFM100_MSGQ) broadcastDebug
#define DBGPRINT_INTSTRSTR_ZONEARRAY_MUTEX if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug
#define DBGPRINT_INTSTRSTR_QUBE_MUTEX if (DEBUG_QUBE_MUTEX) broadcastDebug
#define DBGSNPRINTF if (DEBUG_RFM100) snprintf

#define DEBUG_QUBE_MUTEX_START if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_QUBE_MUTEX_GOT   if (DEBUG_QUBE_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_QUBE_MUTEX_IS_OVERFLOW (DEBUG_QUBE_MUTEX && (DEBUG_QUBE_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW if (DEBUG_QUBE_MUTEX && DEBUG_QUBE_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"qubeSocketMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_QUBE_MUTEX
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  DEBUG_QUBE_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_QUBE_MUTEX_OVERFLOW; if (DEBUG_QUBE_MUTEX && DEBUG_TICK_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW  if (DEBUG_QUBE_MUTEX) broadcastDebug // optimizes away, no warnings 
#endif

#define DEBUG_ZONEARRAY_MUTEX_START if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_BEGIN_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_GOT   if (DEBUG_ZONEARRAY_MUTEX) _time_get_ticks(&DEBUG_END_TICK) 
#define DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW (DEBUG_ZONEARRAY_MUTEX && (DEBUG_ZONEARRAY_MUTEX_MAX_MS < (DEBUG_TICK_OVERFLOW_MS = (uint32_t) _time_diff_milliseconds(&DEBUG_END_TICK, &DEBUG_BEGIN_TICK, &DEBUG_TICK_OVERFLOW))))   
#define DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW if (DEBUG_ZONEARRAY_MUTEX && DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneArrayMutex %dms at %s:%d",DEBUG_TICK_OVERFLOW_MS, __PRETTY_FUNCTION__, __LINE__); else DEBUGBUF[0] = 0; 
#if DEBUG_ZONEARRAY_MUTEX
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  DEBUG_ZONEARRAY_MUTEX_GOT; DBGSNPRINT_MS_STR_IF_ZONEARRAY_MUTEX_OVERFLOW; if (DEBUG_ZONEARRAY_MUTEX_IS_OVERFLOW) broadcastDebug 
#else
#define DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW  if (DEBUG_ZONEARRAY_MUTEX) broadcastDebug 
#endif
//================================================================================================


//bool_t UTIL_Internal_QLinkTransmitPacket(const QLink_address * destination,
//                                          byte_t type,
//                                           byte_t instance,
//                                            byte_t * data,
//                                             byte_t dataSize);

#define MAX_NUMBER_OF_SENT_MESSAGES 100
#define SENT_MESSAGE_PURGE_SECONDS  5
sent_message sentMessages[MAX_NUMBER_OF_SENT_MESSAGES];
byte_t sentMessageReadIndex;
byte_t sentMessageWriteIndex;
MUTEX_STRUCT SentMessageArrayMutex;

extern uint_32 updateSock;

unsigned char BlinkCountDigitIndex[3];
unsigned char BlinkDisplayDigitCount[3];
unsigned char BlinkDisplayDigitCountDestination[3];
unsigned char BlinkDisplayLEDState[3];
onqtimer_t BlinkDisplayTimer[3];

volatile dword_t resetSocketFlag = false;
dword_t tempSocketIndex;
extern byte_t globalJsonSocketIndex;

extern byte_t   lastLowBatteryBitArray[ZONE_BIT_ARRAY_BYTES];
extern byte_t   zoneChangedThisMinuteBitArray[ZONE_BIT_ARRAY_BYTES];

_queue_id qlinkSendMessageAckQID;

void InitializeGPIO(void);

MQX_FILE_PTR RFM100UARTHandle;

byte_t receivedPacket[64];
volatile byte_t receivedPacketIndex = 0;
volatile byte_t currentReceiveState = IDLE;

static byte_t oldSequenceNumber = 0;
static byte_t newSequenceNumber = 0;

uint_32 receiveAckCounter = 0;

bool_t spareS3Input_pressed = false;
bool_t spareS4Input_pressed = false;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int_32 GetTickCount(void)
{
   static boolean firstTimeFlag = TRUE;
   static MQX_TICK_STRUCT lastTickCount;
   static MQX_TICK_STRUCT newTickCount;
   static int_32 lastTicksDelta;
   bool overflowFlag;
   int_32 ticksDelta;
   int_32 returnTicks;

   if (firstTimeFlag)
   {
      firstTimeFlag = FALSE;
      lastTicksDelta = 0;
      _time_get_elapsed_ticks(&lastTickCount);
      return 0;
   }

   _time_get_elapsed_ticks(&newTickCount);
   ticksDelta = _time_diff_milliseconds(&newTickCount, &lastTickCount, &overflowFlag);
   returnTicks = (ticksDelta - lastTicksDelta);
   lastTicksDelta = ticksDelta;
   if (ticksDelta > 100000)
   {
      lastTickCount = newTickCount;
      lastTicksDelta = 0;
   }

   if (returnTicks < 0)
   {
      returnTicks = 0;
   }

   return returnTicks;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void InitializeGPIO(void)
{

   //-----------------------------------------------------------------------------
   // outputs
   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&ledD7Blue,
                               (GPIO_PORT_E | GPIO_PIN28),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&ledD7Blue, BSP_LED4_MUX_GPIO);
   /*Turn off Led */
   lwgpio_set_value(&ledD7Blue, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&led1Green,
                               (GPIO_PORT_B | GPIO_PIN22),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&led1Green, BSP_LED4_MUX_GPIO);
   /*Turn off LED */
   lwgpio_set_value(&led1Green, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&led1Red,
                               (GPIO_PORT_B | GPIO_PIN23),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&led1Red, BSP_LED4_MUX_GPIO);
   /*Turn off LED */
   lwgpio_set_value(&led1Red, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&led2Green,
                               (GPIO_PORT_B | GPIO_PIN21),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&led2Green, BSP_LED4_MUX_GPIO);
   /*Turn off LED */
   lwgpio_set_value(&led2Green, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&led2Red,
                               (GPIO_PORT_B | GPIO_PIN20),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&led2Red, BSP_LED4_MUX_GPIO);
   /*Turn off Led */
   lwgpio_set_value(&led2Red, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&jsonActivityLED,
                               (GPIO_PORT_A | GPIO_PIN19),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&jsonActivityLED, BSP_LED4_MUX_GPIO);
   lwgpio_set_value(&jsonActivityLED, LWGPIO_VALUE_HIGH); // turn off json activity led

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&OUT0Output,
                               (GPIO_PORT_C | GPIO_PIN17),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&OUT0Output, BSP_LED4_MUX_GPIO);
   // Turn off OUT0
   lwgpio_set_value(&OUT0Output, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&OUT1Output,
                               (GPIO_PORT_C | GPIO_PIN19),
                                LWGPIO_DIR_OUTPUT,
                                 LWGPIO_VALUE_NOCHANGE);

   lwgpio_set_functionality(&OUT1Output, BSP_LED4_MUX_GPIO);
   // Turn off OUT1
   lwgpio_set_value(&OUT1Output, LWGPIO_VALUE_HIGH);

   //-----------------------------------------------------------------------------
   // inputs
   //-----------------------------------------------------------------------------
   // PORTA0 - Verbose debugging jumper input
   (void) InitializeGPIOPoint(&debugJumperInput,
                               (GPIO_PORT_A | GPIO_PIN0),
                                LWGPIO_DIR_INPUT,
                                 LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&debugJumperInput, LWGPIO_MUX_A0_GPIO);
   lwgpio_set_attribute(&debugJumperInput, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&IN0Input,
                               (GPIO_PORT_C | GPIO_PIN16),
                                LWGPIO_DIR_INPUT,
                                 LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&IN0Input, BSP_LED4_MUX_GPIO);
   lwgpio_set_attribute(&IN0Input, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   //-----------------------------------------------------------------------------

   (void) InitializeGPIOPoint(&IN1Input,
                               (GPIO_PORT_C | GPIO_PIN18),
                                LWGPIO_DIR_INPUT,
                                 LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&IN1Input, BSP_LED4_MUX_GPIO);
   lwgpio_set_attribute(&IN1Input, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   //-----------------------------------------------------------------------------
   
   (void) InitializeGPIOPoint(&factoryResetInput,
                                (GPIO_PORT_B | GPIO_PIN6),
                                 LWGPIO_DIR_INPUT,
                                  LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&factoryResetInput, BSP_LED4_MUX_GPIO);
   lwgpio_set_attribute(&factoryResetInput, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   //-----------------------------------------------------------------------------
   
   (void) InitializeGPIOPoint(&spareS3Input,
                                (GPIO_PORT_B | GPIO_PIN7),
                                 LWGPIO_DIR_INPUT,
                                  LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&spareS3Input, BSP_LED4_MUX_GPIO);
   lwgpio_set_attribute(&spareS3Input, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   //-----------------------------------------------------------------------------
   
   (void) InitializeGPIOPoint(&spareS4Input,
                                (GPIO_PORT_B | GPIO_PIN8),
                                 LWGPIO_DIR_INPUT,
                                  LWGPIO_VALUE_NOCHANGE);
   lwgpio_set_functionality(&spareS4Input, BSP_LED4_MUX_GPIO);
   lwgpio_set_attribute(&spareS4Input, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef DIAGNOSTICS_HEARTBEAT
unsigned char FindFirstDigit(unsigned long BlinkDisplayCount)
{
   unsigned char BlinkDigitCountIndex = 0;

   if (BlinkDisplayCount == 0)
   {
      return 0;
   }

   while (BlinkDisplayCount)
   {
      BlinkDisplayCount /= 10;
      BlinkDigitCountIndex++;
   }

   return (BlinkDigitCountIndex - 1);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

unsigned char GetDigit(unsigned long BlinkDisplayCount, unsigned char BlinkDigitCountIndex)
{
   while (BlinkDigitCountIndex)
   {
      BlinkDisplayCount /= 10;
      BlinkDigitCountIndex--;
   }

   if ((BlinkDisplayCount % 10) == 0)
   {
      return 10;
   }

   return (BlinkDisplayCount % 10);

}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleSocketPingTimers(onqtimer_t tickCount)
{
   byte_t socketIndex;

   _mutex_lock(&JSONTransmitTimerMutex);
   for (socketIndex = 0; socketIndex < MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS; socketIndex++)
   {
      if (jsonSockets[socketIndex].JSONSocketConnection)
      {
         jsonSocketTimer[socketIndex] += tickCount;
      }
   }
   _mutex_unlock(&JSONTransmitTimerMutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void HandleCloudSocketPingTimer(onqtimer_t tickCount)
{
   timeSincePingReceived += tickCount;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t GetNumItemsInSentMessageQueue()
{
   byte_t numItemsInQueue = 0;

   if(sentMessageWriteIndex >= sentMessageReadIndex)
   {
      numItemsInQueue = (sentMessageWriteIndex - sentMessageReadIndex);
   }
   else
   {
      numItemsInQueue = MAX_NUMBER_OF_SENT_MESSAGES - (sentMessageReadIndex - sentMessageWriteIndex);
   }

   // Ensure the number of items in the queue is a valid number
   if((numItemsInQueue < 0) ||
      (numItemsInQueue > MAX_NUMBER_OF_SENT_MESSAGES))
   {
      // Invalid number. Set the number of items in the queue to 0
      numItemsInQueue = 0;

      // Reset the read and write indexes
      sentMessageWriteIndex = 0;
      sentMessageReadIndex = 0;
   }

   return numItemsInQueue;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void ClearMessagesFromSentQueue()
{
   // Get the current time from the real time clock
   TIME_STRUCT currentTime;
   _time_get(&currentTime);

   // Clear out any messages that are older than the message purge time
   byte_t numItemsToRead = GetNumItemsInSentMessageQueue();

   // Iterate over the array moving the read index past old items that can be overwritten
   byte_t itemsRead = 0;
   byte_t currentIndex = sentMessageReadIndex;

   while(itemsRead < numItemsToRead)
   {
      if((currentTime.SECONDS - sentMessages[currentIndex].timeSent.SECONDS) > SENT_MESSAGE_PURGE_SECONDS)
      {
         // Set the read index to one past the current index
         sentMessageReadIndex = (sentMessageReadIndex+1) % MAX_NUMBER_OF_SENT_MESSAGES;
         
         // Increment the current index
         currentIndex = (currentIndex+1) % MAX_NUMBER_OF_SENT_MESSAGES;

         // Increment the items read count
         itemsRead++;
      }
      else
      {
         // The time was less than the purge time. Safe to break out of the 
         // loop because everything after will have a smaller time difference
         itemsRead = numItemsToRead;
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void AddToSentMessageQueue(zone_properties * zoneProperties)
{
   // Add mutex protection
   _mutex_lock(&SentMessageArrayMutex);
   
   // Clear any old messages from the sent queue
   ClearMessagesFromSentQueue();

   // Determine if there is any space available in the sent message queue
   byte_t numItemsInQueue = GetNumItemsInSentMessageQueue();

   if(numItemsInQueue < MAX_NUMBER_OF_SENT_MESSAGES)
   {
      // Get the current time from the real time clock
      TIME_STRUCT currentTime;
      _time_get(&currentTime);

      // Add the message to the sent messages queue
      sentMessages[sentMessageWriteIndex].timeSent = currentTime;
      
      // if we have a ptr to the zone properties
      if (zoneProperties) 
      {
         sentMessages[sentMessageWriteIndex].groupId = zoneProperties->groupId;
         sentMessages[sentMessageWriteIndex].buildingId = zoneProperties->buildingId;
         sentMessages[sentMessageWriteIndex].houseId = zoneProperties->houseId;
         sentMessages[sentMessageWriteIndex].powerLevel = zoneProperties->powerLevel;
      }
      // Increment the write index
      sentMessageWriteIndex = (sentMessageWriteIndex+1) % MAX_NUMBER_OF_SENT_MESSAGES;
   }

   // Unlock the mutex
   _mutex_unlock(&SentMessageArrayMutex);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t IsMessageARepeat(byte_t buildingId, byte_t houseId, word_t groupId, byte_t powerLevel, byte_t timeToLive)
{
   bool_t messageRepeated = false;

   if(timeToLive < 2)
   {
      // Add mutex protection
      _mutex_lock(&SentMessageArrayMutex);

      // Get the number of items to compare from the sent message array
      byte_t numItemsToRead = GetNumItemsInSentMessageQueue();
      
      // Iterate over the array moving the read index past old items that can be overwritten
      byte_t itemsRead = 0;
      byte_t currentIndex = sentMessageReadIndex;

      while(itemsRead < numItemsToRead)
      {
         // Determine if the parameters match a message in the sent message queue
         if((sentMessages[currentIndex].buildingId == buildingId) &&
            (sentMessages[currentIndex].houseId == houseId) &&
            (sentMessages[currentIndex].groupId == groupId) &&
            (sentMessages[currentIndex].powerLevel == powerLevel))
         {
            // Parameters match a message in the sent queue. Set the repeated flag to true
            messageRepeated = true;

            // Break out of the loop
            itemsRead = numItemsToRead;
         }
         else
         {
            // Increment the current index
            currentIndex = (currentIndex+1) % MAX_NUMBER_OF_SENT_MESSAGES;

            // Increment the items read count
            itemsRead++;
         }
      }

      // Unlock the mutex
      _mutex_unlock(&SentMessageArrayMutex);
   }

   return messageRepeated;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int_32 RFM100_Send(byte_t * outBytes, byte_t length)
{
   int_32 bytesSent;

   bytesSent = write(RFM100UARTHandle, outBytes, length);

   return bytesSent;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RS232Receive_Task(uint_32 param)
{
   byte_t inByte;
   int_32 numberOfBytesReceived;
   RS232_RECEIVE_TASK_MESSAGE_PTR RS232ReceiveTaskMessagePtr;

   while (TRUE)
   {
      numberOfBytesReceived = read(RFM100UARTHandle, &inByte, 1); // blocking operation
      if (numberOfBytesReceived)
      { // got a byte, send it via the message queue
         if (RS232ReceiveTaskMessagePoolID)
         { // message pool is available
            RS232ReceiveTaskMessagePtr = (RS232_RECEIVE_TASK_MESSAGE_PTR) _msg_alloc(RS232ReceiveTaskMessagePoolID);
            if (RS232ReceiveTaskMessagePtr)
            {
               RS232ReceiveTaskMessagePtr->HEADER.SOURCE_QID = 0; //not relevant
               RS232ReceiveTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RS232_RECEIVE_TASK_MESSAGE_QUEUE);
               RS232ReceiveTaskMessagePtr->HEADER.SIZE = sizeof(RS232_RECEIVE_TASK_MESSAGE);

               RS232ReceiveTaskMessagePtr->receivedByte = inByte;

               _msgq_send(RS232ReceiveTaskMessagePtr);
            }
            else
            {
               rs232ReceiveTaskMsgAllocErr++;
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// BuildReceivedPacket
//
// Takes a single received byte and uses it to build the received packet.
// Maintains the receive packet state machine.
//
// params:  inByte - the received byte
//
// returns: true - complete packet has been received
//          false -no packet received
//
//-----------------------------------------------------------------------------

byte_t BuildReceivedPacket(byte_t inByte)
{
   static byte_t recvdLen = 0;

   //save the byte into the packet
   receivedPacket[receivedPacketIndex] = inByte;
   receivedPacketIndex++;

   if (receivedPacketIndex >= sizeof(receivedPacket))
   {
      currentReceiveState = IDLE;
      return false;
   }

   switch (currentReceiveState)
   {
      case IDLE:
         // if start byte
         if ((inByte == (byte_t) START_BYTE))
         {
            currentReceiveState = RECEIVING_RF100_LENGTH;
         }
         break;
      case RECEIVING_RF100_LENGTH:
         recvdLen = inByte + 1;

         if (recvdLen > sizeof(receivedPacket))
         {
            currentReceiveState = IDLE;
         }
         else
         {
            currentReceiveState = RECEIVING_RF100_RSSI;
         }
         break;
      case RECEIVING_RF100_RSSI:
         currentReceiveState = RECEIVING_RF100_PAYLOAD;
         break;

      case RECEIVING_RF100_PAYLOAD:

         // if we are done getting a packet, process it, and set state back to idle
         //    receivedPacketIndex is greater than length plus the length byte
         if (receivedPacketIndex > (recvdLen + 1))
         {
            currentReceiveState = IDLE;
            return true; // we've received a valid packet
         }
         break;
   }

   return false; // don't have a valid packet yet
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// ProcessReceivedPacket
//
// Validate the received packet, updates any data structures and creates and
// sends json broadcast.  Handles new house id if it's ready for that.
// Important that no blocking operations take place.
//
// packet is in the following global array:
//   receivedPacket[]
//       byte 0 - start byte - 0xb2
//       byte 1 - length - number of bytes after this, including CRC
//       byte 2 - rssi
//       byte 3 - command byte -- FIRST BYTE OF THE TOP DOG MESSAGE
//       byte 4-6 - first address
//       byte 7 - x payload
//                first byte contains payload length and function code
//       last byte(s) - crc - see below
// 
// example ramp TOPDOG_F1:
//   0       1      2      3      4      5      6      7      8      9     10     11
// <0xb2> <0x09> <0x30> <0x2a> <0x23> <0x4a> <0xf8> <0x85> <0xff> <0x32> <0x00> <0xxx>
//  hdr    len    rssi   hdr   -----  address -----   PL1    PL2    PL3    PL4    crc
//
// note that crc is calculated from buffer[3] for a length of (len - 1)
// the packet crc sits at buffer[len + 2]
//
//
// example ramp TOPDOG_F2:
//   0       1      2      3      4      5      6      7      8      9     10     11     12
// <0xb2> <0x0a> <0x30> <0xca> <0x23> <0x4a> <0xf8> <0x85> <0xff> <0x32> <0x00> <0xcb> <0x34>
//  hdr    len    rssi   hdr   -----  address -----   PL1    PL2    PL3    PL4    crc   ~crc
//
// note that crc is calculated from buffer[3] for a length of (len - 2)
// note that crc sits at buffer[len + 1] and ~crc at buffer[len + 2]
//
// params:  none
//
// returns: none
//
//-----------------------------------------------------------------------------

void ProcessReceivedPacket(void)
{
   byte_t packetHeader;
   byte_t packetLength;
   byte_t packetRSSI;
   byte_t topdogFamily;
   byte_t topdogAddressMode;
   byte_t timeToLive;
   byte_t crc;
   byte_t slowDeactivateMode;
   bool_t validPacketFlag = false;
#if DEBUG_RFM100_ANY
   char                       debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif

   packetHeader = receivedPacket[0];
   packetLength = receivedPacket[1];
   packetRSSI = receivedPacket[2];

   // parse out specifics from topdog header byte
   topdogFamily = (receivedPacket[3] & 0xe0) >> 5;
   timeToLive = (receivedPacket[3] & 0x03);

   if (topdogFamily == TOPDOG_F1)
   {
      crc = UpdateCRC8(0, &receivedPacket[3], (packetLength - 1)); // single byte crc
      if (crc == receivedPacket[packetLength + 2])
      {
         validPacketFlag = true;
      }
   }
   else if ((topdogFamily == TOPDOG_F2) || (topdogFamily == TOPDOG_CADIR))
   {
      crc = UpdateCRC8(0, &receivedPacket[3], (packetLength - 2)); // two byte crc
      if ((receivedPacket[packetLength + 1] == crc) && (receivedPacket[packetLength + 2] == (byte_t) (~crc)))
      {
         validPacketFlag = true;
      }
   }
   // else any other family type leaves validPacketFlag as false

   if (validPacketFlag)
   {
      if (manufacturingTestModeFlag)
      {  // we're in manufacturing test mode, so issue a broadcast json packet for this rf packet
         json_t * broadcastObject;
         json_t * packetDataObject;
         byte_t   packetDataIndex;

         broadcastObject = json_object();
         if (broadcastObject)
         {
            packetDataObject = json_array();
            if (packetDataObject)
            {
               json_object_set_new(broadcastObject, "ID", json_integer(0));
               json_object_set_new(broadcastObject, "Service", json_string("MTMReceivedRFPacket"));
               json_object_set_new(broadcastObject, "SignalStrength", json_integer(packetRSSI));
               json_object_set_new(broadcastObject, "PacketHeader", json_integer(packetHeader));
      
               for (packetDataIndex = 0; packetDataIndex < packetLength; packetDataIndex++)
               {
                  json_array_append_new(packetDataObject, json_integer(receivedPacket[3 + packetDataIndex]));
               }
               json_object_set_new(broadcastObject, "PacketData", packetDataObject);
               
               //broadcast MTMReceivedRFPacket
               json_object_set_new(broadcastObject, "Status", json_string("Success"));
               SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
            }
            json_decref(broadcastObject);
         }
         return;
      }
      
      topdogAddressMode = (receivedPacket[3] & 0x18) >> 3;
      // don't worry about retransmit delay or time-to-live
      if ((topdogAddressMode == TDAM_BROADCAST) || (topdogAddressMode == TDAM_ANON_MULTICAST) || (topdogAddressMode == TDAM_MULTICAST))
      { // these types of address mode contain topdog addresses as the next three bytes
         if (!(receivedPacket[5] & 0x10))
         { // this bit indicates a room address - we only want to process group addresses
            word_t groupId;
            word_t cmd;
            byte_t buildingId;
            byte_t houseId;
            byte_t payloadLength;
            byte_t packIdx;
            byte_t targetValue;
            byte_t bankIndex;
            byte_t chgUporDown;
            bool_t sceneState;
 
            buildingId = (receivedPacket[4] & 0xe0) >> 5;
            houseId = (receivedPacket[4] & 0x1f) << 3;      // first five bits of house id are bit4-bit0 in this byte
            houseId |= (receivedPacket[5] & 0xe0) >> 5;     // last three bits of house id are bit7-bit5 in this byte
            HighByteOfWord(groupId) = receivedPacket[5] & 0x0f;   // first four bits of group id are bit3-bit0 in this byte
            LowByteOfWord(groupId) = receivedPacket[6];           // last 8 bits of group id are bit7-bit0 in this byte

            // groupId = (word_t) (receivedPacket[5] & 0x0f) << 8;
            // groupId |= (receivedPacket[6] & 0xff);

            packIdx = 7; // starting position for command
            payloadLength = receivedPacket[packIdx] >> 6;
            cmd = receivedPacket[packIdx++] & 0x3f;
            if (cmd >= 0x30) // we have a two byte command
            {
               cmd <<= 8;
               cmd |= receivedPacket[packIdx++]; // read the next byte, which will have the actual cmd
            }

            // Note: packIdx is now pointing to the parameters of the message from the command
            
            switch (cmd)
            {
               case RAMP:
               case CANCEL_RAMP:
                  // normalize to 1-100
                  targetValue = (byte_t) (((((word_t) receivedPacket[packIdx]) * 100) + (MAX_TD_POWER >> 1)) / MAX_TD_POWER); 

                  if(!IsMessageARepeat(buildingId, houseId, groupId, targetValue, timeToLive))
                  {
                     DEBUG_ZONEARRAY_MUTEX_START;
                     _mutex_lock(&ZoneArrayMutex);
                     DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessReceivedPacket before ZoneArrayMutex HandleRampReceived",DEBUGBUF);
                     DEBUG_ZONEARRAY_MUTEX_START;
                     HandleRampReceived(buildingId, houseId, groupId, targetValue, receivedPacket[packIdx + 2]);
                     //                                                         level                        type
                     DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW ProcessReceivedPacket after HandleRampReceived",DEBUGBUF);
                     _mutex_unlock(&ZoneArrayMutex);
                  }
                  break;
               case STATUS:
                  // normalize to 1-100
                  targetValue = (byte_t) (((((word_t) receivedPacket[packIdx]) * 100) + (MAX_TD_POWER >> 1)) / MAX_TD_POWER); 

                  HandleStatusReceived(buildingId, houseId, groupId, receivedPacket[packIdx + 1], targetValue);
                  //                                                         level                        type 
                  break;
               case SCENE_CONTROLLER_MASTER_OFF:
                  newSequenceNumber = receivedPacket[packIdx+1];

                  if ((newSequenceNumber != oldSequenceNumber) ||
                      (newSequenceNumber == 0))
                  {
                     if (receivedPacket[packIdx + 2])
                     {
                        slowDeactivateMode = SLOW_DEACTIVATION_RAMP_VALUE;
                     }
                     else
                     {
                        slowDeactivateMode = 0;
                     }
                     
                     HandleSceneControllerMasterOff(buildingId, houseId, groupId, receivedPacket[packIdx + 2], slowDeactivateMode);
                  }
                  break;
               case SCENE_REQUESTED:
                  bankIndex = receivedPacket[packIdx + 1] & 0x3f;
                  newSequenceNumber = receivedPacket[packIdx+3];
                  
                  if ( (receivedPacket[packIdx + 1] & 0x40) )
                  {
                     sceneState = TRUE;
                  }
                  else
                  {
                     sceneState = FALSE;
                  }
                  
                  if (receivedPacket[packIdx + 1] & 0x80)
                  {
                     slowDeactivateMode = SLOW_DEACTIVATION_RAMP_VALUE;
                  }
                  else
                  {
                     slowDeactivateMode = 0;
                  }
                  if ((newSequenceNumber != oldSequenceNumber) ||
                      (newSequenceNumber == 0))
                  {
                     oldSequenceNumber = newSequenceNumber;
                     HandleSceneControllerSceneRequest(buildingId, houseId, groupId, bankIndex, sceneState, receivedPacket[packIdx + 2], slowDeactivateMode);
                     //                                                                                             device type
                  }

                  break;
               case SCENE_ADJUST:
                  newSequenceNumber = receivedPacket[packIdx + 3];
                  chgUporDown = (receivedPacket[packIdx + 1] & 0x80) >> 7;
                  if ((newSequenceNumber != oldSequenceNumber) ||
                      (newSequenceNumber == 0))
                  {
                     oldSequenceNumber = newSequenceNumber;
                     HandleSceneControllerSceneAdjust(buildingId, houseId, groupId, receivedPacket[packIdx], PERCENT_CHANGE_FOR_ADJUST, chgUporDown, 0);
                     //                                                                    scene index
                  }
                  break;
 
            }

         } // if (!(receivedPacket[5] & 0x10))
           // not a room address

      } // if ((topdogAddressMode == TDAM_BROADCAST) || (topdogAddressMode == TDAM_ANON_MULTICAST) || (topdogAddressMode == TDAM_MULTICAST))

   } // if (validPacketFlag)
     // good checksum
}


//-----------------------------------------------------------------------------

void PublishAckToQueue(byte_t ackByte)
{
#if DEBUG_RFM100
   char debugBuf[100];
#endif
   if (RFM100TransmitAckMessagePoolID)
   { // message pool is available
      RFM100_TRANSMIT_ACK_MESSAGE_PTR RFM100TransmitAckMessagePtr;
   
      RFM100TransmitAckMessagePtr = (RFM100_TRANSMIT_ACK_MESSAGE_PTR) _msg_alloc(RFM100TransmitAckMessagePoolID);
      if (RFM100TransmitAckMessagePtr)
      {
         RFM100TransmitAckMessagePtr->HEADER.SOURCE_QID = 0; //not relevant
         RFM100TransmitAckMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RFM100_TRANSMIT_ACK_MESSAGE_QUEUE);
         RFM100TransmitAckMessagePtr->HEADER.SIZE = sizeof(RFM100_TRANSMIT_ACK_MESSAGE);
   
         RFM100TransmitAckMessagePtr->ackByte = ackByte;
   
         _msgq_send(RFM100TransmitAckMessagePtr);
         receiveAckCounter++;
         DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"PublishAckToQueue:%d receiveAckCounter=%d ackByte=0x02X", __LINE__, receiveAckCounter, RFM100TransmitAckMessagePtr->ackByte);
         DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"PublishAckToQueue",DEBUGBUF);
      }
      else
      {
         rfm100TransmitTaskMsgAllocErr++;
      }
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RFM100Receive_Task(uint_32 param)
{
   RS232_RECEIVE_TASK_MESSAGE_PTR   RS232ReceiveTaskMessagePtr;
   _mqx_uint                        result;
   _queue_id                        RS232ReceiveTaskMessageQID;
   byte_t                           inByte;
   byte_t                           gotAPacketFlag = false;
#if DEBUG_RFM100
   char debugBuf[100];
#endif

   // Open a message queue for receiving the RS232 bytes queue:
   RS232ReceiveTaskMessageQID = _msgq_open(RS232_RECEIVE_TASK_MESSAGE_QUEUE, 0);
   // Create a message pool:
   RS232ReceiveTaskMessagePoolID = _msgpool_create(sizeof(RS232_RECEIVE_TASK_MESSAGE), NUMBER_OF_RS232_RECEIVE_MESSAGES_IN_POOL, 0, 0);

   _task_create(0, RS232_RECEIVE_TASK, 0);

   while (TRUE)
   {

      if (currentReceiveState == IDLE)
      {  // in idle state, wait forever for a character
         RS232ReceiveTaskMessagePtr = _msgq_receive(RS232ReceiveTaskMessageQID, 0);
         receivedPacketIndex = 0;
      }
      else
      {  // if we're in the middle of receiving a message from the RFM100, only wait for a short time before declaring a timeout
         RS232ReceiveTaskMessagePtr = _msgq_receive(RS232ReceiveTaskMessageQID, MILLISECOND_DELAY_FOR_RFM100_RECEIVE_ABORT);
      }

      if (!RS232ReceiveTaskMessagePtr)
      {  // timeout out - reset to idle state
         currentReceiveState = IDLE;
         DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Receive_Task currentReceiveState=IDLE","");
      }
      else // we got a byte
      {

         inByte = RS232ReceiveTaskMessagePtr->receivedByte;
         _msg_free(RS232ReceiveTaskMessagePtr);

         DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Receive_Task:%d receiveAckCounter=%d currentReceiveState=%d inByte=0x%02X", __LINE__, receiveAckCounter, currentReceiveState, inByte);
         DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
         
         if ((currentReceiveState == IDLE) &&
               ((inByte == RFM100_ACK_BYTE) || (inByte == RFM100_NAK_BYTE)))  // ACKs from RF100
         {
            PublishAckToQueue(inByte);
         }
         else
         {
            gotAPacketFlag = BuildReceivedPacket(inByte);
            DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Receive_Task:%d receiveAckCounter=%d gotAPacketFlag=%d", __LINE__, receiveAckCounter, gotAPacketFlag);
            DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
            if (gotAPacketFlag)
            {
               ProcessReceivedPacket();
            }
         }

      }

   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//   config_RFM100()
//
//    This sends configuration data to the top dog RF device, that sends 
//   raw data to the top dog devices via RF.  
//   
//
//-----------------------------------------------------------------------------
bool_t config_RFM100()
{
   int_32 bytesSent = 0;
   byte_t config[6];

   // Removed - seems to do nothing
   // RESET the device 
   //   config[0] = 0x80;     // Write command
   //   config[1] = 0x18;     // Register to start writing
   //   config[2] = 0x01;     // Length
   //   config[3] = 0x00;     // reset cmd to status reg 0
   //   ix = RS232_Send(config, 6);
   //   _time_delay(1000);

   // Configure the device options
   config[0] = 0x80; // Write command
   config[1] = 0x04; // Register to start writing
   config[2] = 0x03; // Length
   config[3] = 0x10; // Opt reg 1 - Control/Config -
   //   Bit 4 = 0 - Disable as transmit completion control char C5
   config[4] = 0x73; // Opt Reg 2 - I2C Config - Default values for I2C Setup register
   config[5] = 0xAB; // Opt Reg 3 - 1010 1011
   //   Default values for UART Setup register
   //   Bit 0 - enable UART
   //   Bit 1 - enable Auto clr DSR
   //   Bit 2 - disable ASCII Convert
   //   Bit 3 - enable auto transmit
   //   Bit 4-5 - 10 -  auto transmit of correct packet
   //   Bit 6-7 - 10 - 38.4kbaud to top dog

   bytesSent = RFM100_Send(config, 6);
   if (bytesSent != 6) // if send failed,  return true for indication
   {
      return true;
   }

   _time_delay(20);

#define RFM100_TXMODE_TBFF_SHIFT   6
#define RFM100_TXMODE_LRBFF_SHIFT  5
#define RFM100_TXMODE_LRTBF_SHIFT  4
#define RFM100_TXMODE_FTS_SHIFT    2
#define RFM100_TXMODE_TTL_SHIFT    0
   
   config[0] = 0x80; // Write command
   config[1] = 0xB0; // Register to start writing
   config[2] = 0x01; // Length
   config[3] = (0 << RFM100_TXMODE_TBFF_SHIFT) |
               (0 << RFM100_TXMODE_LRBFF_SHIFT) |
               (0 << RFM100_TXMODE_LRTBF_SHIFT) |
               (1 << RFM100_TXMODE_FTS_SHIFT) |    // additional frames to send - so 2 indicates 3 total frames
               (2 << RFM100_TXMODE_TTL_SHIFT);

   bytesSent = RFM100_Send(config, 4);
   if (bytesSent != 4) // if send failed,  return true for indication
   {
      return true;
   }

   _time_delay(20);

   return false;  // no errors
}

//-------------------------------------------------------------
//  GetFuncByte()
//   given a top dog device structure and a cmd, this will
// return the function byte for a top dog message.
// Bits 0-4 = cmd
// Bits 5-7 = length  (note 6 bytes is max size) 
//                    (length includes this byte)
//
//-------------------------------------------------------------

byte_t GetFuncByte(word_t cmd)
{
   // Len is in upper 3 bits, bottom 5 bits are the cmd
   // Encoded length is actual length / 2
   // 1 byte = 0x00
   // 2 bytes = 0x01
   // 4 bytes = 0x10
   // 6 bytes = 0x11

   byte_t encodedLen = 0;

   if (cmd == RAMP)  // payload len is 4 bytes
   {
      encodedLen = 0x80;  // ramp cmd has 4 bytes
      return (encodedLen | cmd);
   }
   else
   // note, this is a 2 byte cmd
   //   only return second byte
   {
      encodedLen = 0x00;  // ramp cmd has 4 bytes
      return (encodedLen | cmd);

   }

   return cmd;

}  // end of GetFuncByte

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void FlushMessageQID(_queue_id QID)
{
   pointer msgPtr;

   do
   {
      msgPtr = _msgq_poll(QID);
      if (msgPtr)
      {
         _msg_free(msgPtr);
      }
   } while (msgPtr);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RFM100Transmit_Task(uint_32 param)
{
   uint_32                          serialParam;
   RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr;
   RFM100_TRANSMIT_ACK_MESSAGE_PTR  RFM100TransmitAckMessagePtr;
   _queue_id                        RFM100TransmitTaskMessageQID;
   _queue_id                        RFM100TransmitAckMessageQID;
   _mqx_uint                        result;
   byte_t                           retryCount;
   bool_t                           successFlag;
   bool_t                           rfMessageSentFlag;
#if DEBUG_RFM100
   char debugBuf[100];
#endif
   
   RFM100UARTHandle = fopen("ittyb:", (pointer) (IO_SERIAL_RAW_IO));
   if (RFM100UARTHandle)
   {
      serialParam = TOP_DOG_BAUD_RATE;
      ioctl(RFM100UARTHandle, IO_IOCTL_SERIAL_SET_BAUD, &serialParam);

      serialParam = IO_SERIAL_STOP_BITS_1;
      ioctl(RFM100UARTHandle, IO_IOCTL_SERIAL_SET_STOP_BITS, &serialParam);

      serialParam = IO_SERIAL_PARITY_NONE;
      ioctl(RFM100UARTHandle, IO_IOCTL_SERIAL_SET_PARITY, &serialParam);

      serialParam = 8;
      ioctl(RFM100UARTHandle, IO_IOCTL_SERIAL_SET_DATA_BITS, &serialParam);

      config_RFM100();

      _task_create(0, RFM100_RECEIVE_TASK, 0);  // should be ready to receive packets
   }

   // Open a message queue for receiving transmit packets:
   RFM100TransmitTaskMessageQID = _msgq_open(RFM100_TRANSMIT_TASK_MESSAGE_QUEUE, 0);
   // Create a message pool:
   RFM100TransmitTaskMessagePoolID = _msgpool_create(sizeof(RFM100_TRANSMIT_TASK_MESSAGE), NUMBER_OF_RFM100_TRANSMIT_MESSAGES_IN_POOL, 0, 0);

   // Open a message queue for receiving the rfm100 ack/nak's:
   RFM100TransmitAckMessageQID = _msgq_open(RFM100_TRANSMIT_ACK_MESSAGE_QUEUE, 0);
   // Create a message pool:
   RFM100TransmitAckMessagePoolID = _msgpool_create(sizeof(RFM100_TRANSMIT_ACK_MESSAGE), NUMBER_OF_RFM100_TRANSMIT_ACK_MESSAGES_IN_POOL, 0, 0);

   while (TRUE)
   {
      // pend forever on a message to transmit
      RFM100TransmitTaskMessagePtr = _msgq_receive(RFM100TransmitTaskMessageQID, 0);

      // get message from queue
      if (RFM100TransmitTaskMessagePtr)
      {
         
         retryCount = 0;
         successFlag = false;
         while ((retryCount < NUMBER_OF_RFM100_TRANSMIT_ATTEMPTS) && (!successFlag))
         {
            FlushMessageQID(RFM100TransmitAckMessageQID);
            if ((!retryCount) && (RFM100TransmitTaskMessagePtr->sceneId == (MAX_NUMBER_OF_SCENES + 1)))
            {
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task calling HandleTransmitTaskMessage false","");
               rfMessageSentFlag = HandleTransmitTaskMessage(RFM100TransmitTaskMessagePtr, false);
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Transmit_Task:%d retryCount=%d rfMessageSentFlag=%d from HandleTransmitTaskMessage", __LINE__, retryCount, rfMessageSentFlag);
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
            }
            else
            {  // if this is a retry, force the rf message since we would otherwise skip transmitting it.
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task calling HandleTransmitTaskMessage true","");
               rfMessageSentFlag = HandleTransmitTaskMessage(RFM100TransmitTaskMessagePtr, true);
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Transmit_Task:%d retryCount=%d rfMessageSentFlag=%d from HandleTransmitTaskMessage", __LINE__, retryCount, rfMessageSentFlag);
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
            }

            if (rfMessageSentFlag)
            {  // we only need to wait on an ack/nak if we actually sent a packet
               RFM100TransmitAckMessagePtr = _msgq_receive(RFM100TransmitAckMessageQID, RFM100_TRANSMIT_ACK_TIMEOUT);
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Transmit_Task:%d retryCount=%d 0x%04X = _msgq_receive(RFM100TransmitAckMessageQID, RFM100_TRANSMIT_ACK_TIMEOUT);", __LINE__, retryCount, (int) RFM100TransmitAckMessagePtr );
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
               if (RFM100TransmitAckMessagePtr)
               {
                  DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Transmit_Task:%d retryCount=%d 0x%04X = RFM100TransmitAckMessagePtr->ackByte", __LINE__, retryCount, (unsigned int) RFM100TransmitAckMessagePtr->ackByte );
                  DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
                  if (RFM100TransmitAckMessagePtr->ackByte == RFM100_ACK_BYTE)
                  {
                     successFlag = true;
                  }
                  if (receiveAckCounter > 0)
                     receiveAckCounter --;
                  _msg_free(RFM100TransmitAckMessagePtr);
               }
            }
            else
            {  // no rf message sent, so force successful
               successFlag = true;
               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"RFM100Transmit_Task:%d no rf message sent, so force successful", __LINE__);
               DBGPRINT_INTSTRSTR_MSGQ(__LINE__,"RFM100Transmit_Task",DEBUGBUF);
            }
            
            retryCount++;
            
            if (!successFlag)
            {  // slight delay before retry
               _time_delay(100);
            }
         }
         
         _msg_free(RFM100TransmitTaskMessagePtr);
                  
         if (rfMessageSentFlag)
         {  // only delay if we actually sent an rf packet.
            _time_delay(250);
         }

      }  // if (RFM100TransmitTaskMessagePtr)

   }  // while true

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void EventController_Task(uint_32 param)
{  // formerly qlink task - probably has a lot of stuff we no longer need 
   dword_t tickCount;
   onqtimer_t led1GreenUpdateTimer;
   onqtimer_t factoryResetTimer = 0;
   onqtimer_t toggleCloudTimer = 0;
   onqtimer_t togglePasswordTimer = PASSWORD_RESET_TIMER;
   bool_t resetDeviceFlag = FALSE;
   word_t counter = 0;
   byte_t ledIndex;
   MUTEX_ATTR_STRUCT mutexattr;
   int_32 result;
   char versionHash[20];

   _task_create(0, RFM100_TRANSMIT_TASK, 0);

   InitializeGPIO();

   // Turn on the Power LED
   SetGPIOOutput(POWER_LED, LED_ON);

   sscanf(SYSTEM_INFO_FIRMWARE_VERSION, "%d.%d.%d-%d-%s",
           &currentFirmwareVersion[0],
            &currentFirmwareVersion[1],
             &currentFirmwareVersion[2],
              &currentFirmwareVersion[3],
               versionHash);

   _mutatr_init(&mutexattr);
   _mutex_init(&JSONTransmitTimerMutex, &mutexattr);

   _mutex_init(&ZoneArrayMutex, &mutexattr);

   _mutex_init(&ScenePropMutex, &mutexattr);

   _mutex_init(&TaskHeartbeatMutex, &mutexattr);
      
   _mutex_init(&SentMessageArrayMutex, &mutexattr);

#ifdef SHADE_CONTROL_ADDED
   _mutex_init(&qubeSocketMutex, &mutexattr);
#endif
   
   // Initialize the system to defaults before attempting to load from flash
   InitializeSystemProperties();
   
   InitializeZoneArray();
   InitializeSceneArray();
   InitializeSceneControllerArray();
   global_houseID = 0;

   InitializeFlash();

   LoadSystemPropertiesFromFlash();
   
   // Make sure we reset the addALight flag on startup.  Otherwise it can linger
   // and make things not work (e.g. scenes).
   systemProperties.addALight = false;

   LoadZonesFromFlash();

   LoadScenesFromFlash();

   LoadSceneControllersFromFlash();

#ifdef SHADE_CONTROL_ADDED
   ReadAllShadeIdsFromFlash();
#endif

   // Zones and Scene Controllers were loaded from flash. Set the house ID led
   if(global_houseID == 0)
   {
      // Turn on the house ID not set LED
      SetGPIOOutput(HOUSE_ID_NOT_SET, LED_ON);

      // Set the configured flag to false so that the Add a Light screen
      // pops up when connecting
      systemProperties.configured = false;
   }
   else
   {
      // Turn off the house ID not set LED
      SetGPIOOutput(HOUSE_ID_NOT_SET, LED_OFF);

      // Set the configured flag to true so that the Add a Light screen
      // does not pop up when connecting for the first time
      systemProperties.configured = true;
   }

//#define DEBUG_ZONE_FULL
   
   byte_t zoneIndex;
   zone_properties   zoneProperties;

#ifdef SHADE_CONTROL_ADDED
   // Migrate the device type of any shade groups that were created under firmware version 3.0.x
   for (zoneIndex = 0; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
       if (GetZoneBit(shadeInfoStruct.zoneIsShadeGroupBitArray, zoneIndex))
       {
          if (GetZoneProperties(zoneIndex, &zoneProperties) && zoneProperties.deviceType != SHADE_GROUP_TYPE)
          {
              // Perform the migration.  Keep zoneIsShadeGroupBitArray as-is in case we need to know
              // that this zone was created under firmware version 3.0.x
              zoneProperties.deviceType = SHADE_GROUP_TYPE;
              SetZoneProperties(zoneIndex, &zoneProperties);
          }
       }
   }
#endif
   
#ifdef DEBUG_ZONE_FULL
   for (zoneIndex = 1; zoneIndex < MAX_NUMBER_OF_ZONES; zoneIndex++)
   {
      // Initialize the volatile data
      zoneProperties.AppContextId = 0;
      zoneProperties.state = TRUE;
      zoneProperties.targetState = FALSE;
      zoneProperties.powerLevel = 100;
      zoneProperties.targetPowerLevel = 100;
      snprintf (zoneProperties.ZoneNameString ,  20, "Zone %d", zoneIndex);
      
      // Update the zone properties array with any changes
       SetZoneProperties(zoneIndex, &zoneProperties);
   
   }
   
#endif

   // Create the socket task now that the data is loaded
   _task_create(0, SOCKET_TASK, 0);

   // initialize IO before starting this task

   GetTickCount();
   led1GreenUpdateTimer = 0;
   _time_delay(100);

   jsonActivityTimer = 0;
   restartSystemTimer = 0;
   flashSaveTimer = 0;
   checkForUpdatesTimer = INITIAL_UPDATE_CHECK_TIME;
   checkWatchdogCountersTimer = WATCHDOG_TIME;
   refreshTokenTimer = 0;
   clearQubeAddressTimer = 0;

   for (ledIndex = 0; ledIndex < 3; ledIndex++)
   {
      BlinkDisplayCount[ledIndex] = 0;
      BlinkDisplayFlag[ledIndex] = false;
      BlinkDisplayTimer[ledIndex] = 0;
      BlinkDisplayLEDState[ledIndex] = true;
      BlinkDisplayDigitCountDestination[ledIndex] = 0;
      BlinkCountDigitIndex[ledIndex] = 0;
   }
   BlinkDisplayFlag[0] = true;
   BlinkDisplayCount[0] = currentFirmwareVersion[2];  // minor rev number
   BlinkDisplayFlag[1] = true;

   // Init all watchdog task counters to 0
   unsigned long watchdog_previous_task_counters[NUM_TASKS_PLUS_ONE];
   memset(watchdog_previous_task_counters, 0, sizeof(watchdog_previous_task_counters));
   memset(watchdog_task_counters, 0, sizeof(watchdog_task_counters));

   while (TRUE)
   {

      tickCount = GetTickCount();
      if (tickCount)
      {
         HandleSocketPingTimers(tickCount);
         HandleCloudSocketPingTimer(tickCount);

         if (resetSocketFlag)
         {
            result = shutdown(updateSock, FLAG_ABORT_CONNECTION);
            if (result != RTCS_OK)
            {
               printf("\nError, shutdown() failed with error code %lx", result);
            }

            updateSock = 0;
            resetSocketFlag = false;
         }  // if (resetSocketFlag)

         if (refreshTokenTimer)
         {
            if (refreshTokenTimer > tickCount)
            {
               refreshTokenTimer -= tickCount;
            }
            else
            {
               refreshTokenFlag = true;
               refreshTokenFailCount = 0;
               refreshTokenTimer = 0;
            }
         }
         
         if (flashSaveTimer)
         {
            if (flashSaveTimer > tickCount)
            {
               flashSaveTimer -= tickCount;
            }
            else
            {
               SaveSystemPropertiesToFlash();
               flashSaveTimer = 0;
            }
         }

         if (checkForUpdatesTimer)
         {
            if (checkForUpdatesTimer > tickCount)
            {
               checkForUpdatesTimer -= tickCount;
            }
            else
            {
               SendUpdateCommand(UPDATE_COMMAND_CHECK_FOR_UPDATE, NULL, NULL);
               checkForUpdatesTimer = 0;
            }
         }

         // Check the watchdog timer
         if (checkWatchdogCountersTimer)
         {
            if (checkWatchdogCountersTimer > tickCount)
            {
               checkWatchdogCountersTimer -= tickCount;
            }
            else
            {
                // It's time to check all watchdog counters that are in-use (nonzero means in-use)
                for (int i = 0; i < NUM_TASKS_PLUS_ONE; i++)
                {
                    if (watchdog_task_counters[i] != 0)
                    {
                        if (watchdog_previous_task_counters[i] == watchdog_task_counters[i])
                        {
                            // The task in question has stalled - reboot the system
                            broadcastDebug(i, "Watchdog:", "Task has stalled - REBOOT");

                            _time_delay(1000);
                            SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA) | SCB_AIRCR_SYSRESETREQ_MASK;
                        }

                        watchdog_previous_task_counters[i] = watchdog_task_counters[i];
                    }
                }

                broadcastDebug(0, "Watchdog:", "All task counters are normal");

                // Reset the watchdog timer
                checkWatchdogCountersTimer = WATCHDOG_TIME;
            }
         }

         // handle the restart system timer
         if (restartSystemTimer)
         {
            if (restartSystemTimer > tickCount)
            {
               restartSystemTimer -= tickCount;
            }
            else
            {
               SetGPIOOutput(ledD7Blue, LED_ON);
               if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
               {
                   led1Green_default_state = LED_ON;
                   SetGPIOOutput(led1Green, led1Green_default_state);
               }
               SetGPIOOutput(led1Red, LED_OFF);
               SetGPIOOutput(led2Green, LED_ON);
               SetGPIOOutput(led2Red, LED_OFF);
               /* request software reset */SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA) | SCB_AIRCR_SYSRESETREQ_MASK;
               /* wait for reset to occur */
               while (1)
                  ;
            }
         }

         // handle the json activity LED timer
         if (jsonActivityTimer > tickCount)
         {
            jsonActivityTimer -= tickCount;
         }
         else
         {
            lwgpio_set_value(&jsonActivityLED, LWGPIO_VALUE_HIGH);
         }
         
         if (clearQubeAddressTimer)
         {
            if (clearQubeAddressTimer > tickCount)
            {
               clearQubeAddressTimer -= tickCount;
            }
            else
            {
                clearQubeAddressTimer = 0;
            }
         }

#ifdef DIAGNOSTICS_HEARTBEAT
         
         if (led1Green_function != LED1_GREEN_FUNCTION_DEFAULT)
         {
             if (led1GreenUpdateTimer > tickCount)
             {
                 // Count down the timer
                 led1GreenUpdateTimer -= tickCount;
             }
             else
             {
                 
                 switch (led1Green_function)
                 {
                     case LED1_GREEN_FUNCTION_FREE_MEMORY:
                        BlinkDisplayCount[1] = _mem_get_free() / 10000;
                        break;

                     case LED1_GREEN_FUNCTION_TASK_HEARTBEAT_BITMASK:
                        _mutex_lock(&TaskHeartbeatMutex);
                        BlinkDisplayCount[1] = task_heartbeat_bitmask;
                        task_heartbeat_bitmask = 0;
                        _mutex_unlock(&TaskHeartbeatMutex);
                        break;
                 }

                 // Add a digit to the front that indicates which debug value is flashing
                 unsigned long multiplier = 10;

                 while (multiplier < BlinkDisplayCount[1] && multiplier < 10000000000)
                 {
                     multiplier *= 10;
                 }
                 
                 BlinkDisplayCount[1] += (multiplier * led1Green_function);

                 // Update the value every so often
                 led1GreenUpdateTimer = GREEN_LED1_UPDATE_TIMER;
             }
         }
         
         for (ledIndex = 0; ledIndex < 3; ledIndex++)
         {
            if (BlinkDisplayFlag[ledIndex])
            {
               if (BlinkDisplayTimer[ledIndex] > tickCount)
               {
                  BlinkDisplayTimer[ledIndex] -= tickCount;
               }
               else
               {
                  if (BlinkDisplayLEDState[ledIndex])
                  {  // we were on, so turn off
                     switch (ledIndex)
                     {
                        case 0:
                           SetGPIOOutput(ledD7Blue, LED_OFF);
                           break;
                        case 1:
                           if (led1Green_function != LED1_GREEN_FUNCTION_DEFAULT)
                              SetGPIOOutput(led1Green, LED_OFF);
                           break;
                        case 2:
                           //SetGPIOOutput(led2Red, LED_OFF);
                           break;
                     }
                     BlinkDisplayLEDState[ledIndex] = false;
                     BlinkDisplayDigitCount[ledIndex]++;
                     if (BlinkDisplayDigitCount[ledIndex] >= BlinkDisplayDigitCountDestination[ledIndex])
                     {  // we've finished flashing the count, delay long before restarting sequence
                        BlinkDisplayDigitCount[ledIndex] = 0;
                        if (BlinkCountDigitIndex[ledIndex] == 0)
                        {  // we were at the last digit, so delay 2 seconds
                           BlinkDisplayTimer[ledIndex] = 2000;
                           BlinkCountDigitIndex[ledIndex] = FindFirstDigit(BlinkDisplayCount[ledIndex]);
                           BlinkDisplayDigitCountDestination[ledIndex] = GetDigit(BlinkDisplayCount[ledIndex], BlinkCountDigitIndex[ledIndex]);
                        }
                        else
                        {  // we finished this digit, get the next digit and delay one second
                           BlinkCountDigitIndex[ledIndex]--;
                           BlinkDisplayDigitCountDestination[ledIndex] = GetDigit(BlinkDisplayCount[ledIndex], BlinkCountDigitIndex[ledIndex]);
                           BlinkDisplayTimer[ledIndex] = 1000;
                        }
                     }
                     else
                     {
                        if (BlinkDisplayDigitCountDestination[ledIndex] == 10)
                        {
                           BlinkDisplayTimer[ledIndex] = 62;
                        }
                        else
                        {
                           BlinkDisplayTimer[ledIndex] = 250;
                        }
                     }
                  }
                  else
                  {  // we were off, so turn on
                     switch (ledIndex)
                     {
                        case 0:
                           SetGPIOOutput(ledD7Blue, LED_ON);
                           break;
                        case 1:
                           if (led1Green_function != LED1_GREEN_FUNCTION_DEFAULT)
                               SetGPIOOutput(led1Green, LED_ON);
                           break;
                        case 2:
                           //SetGPIOOutput(led2Red, LED_ON);
                           break;
                     }
                     BlinkDisplayLEDState[ledIndex] = true;
                     if (BlinkDisplayDigitCountDestination[ledIndex] == 10)
                     {
                        BlinkDisplayTimer[ledIndex] = 62;
                     }
                     else
                     {
                        BlinkDisplayTimer[ledIndex] = 250;
                     }
                  }
               }

            }  // if (BlinkDisplayFlag[ledIndex])

         }  //  for (ledIndex = 0; ledIndex < 3; ledIndex++)
#endif
         if (manufacturingTestModeFlag)
         {
            HandleMTMInputSense(tickCount);
         }
         else
         {
            // Handle the SW1 button.
            // Pressing the SW1 button cycles the green LED behavior through the available functions
            // Holding it for 15 seconds resets the password
            if(lwgpio_get_value(&spareS3Input) == LWGPIO_VALUE_LOW)
            {
               if (togglePasswordTimer)
               {  // timer is active
                  if (togglePasswordTimer > tickCount)
                  {  // countdown timer not yet satisfied
                     togglePasswordTimer -= tickCount;
                  }
                  else
                  {  // countdown timer satisfied - reset password, disable timer until button released
                     broadcastDebug(12345, "USER PASSWORD RESET", "");
                     JSA_ClearUserKeyInFlash();
                     togglePasswordTimer = 0;
                  }
               }

                if (!spareS3Input_pressed)
                {
                    // Clear the LED of previous value
                    SetGPIOOutput(led1Green, LED_OFF);
                    
                    // Cycle to the next value
                    if (led1Green_function == (LED1_GREEN_FUNCTION_TOTAL - 1))
                        led1Green_function = LED1_GREEN_FUNCTION_DEFAULT;
                    else
                        led1Green_function++;
                    
                    if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
                        SetGPIOOutput(led1Green, led1Green_default_state);
                    
                    spareS3Input_pressed = true;
                    led1GreenUpdateTimer = 0;
                }
            }
            else
            {
                spareS3Input_pressed = false;
                togglePasswordTimer = PASSWORD_RESET_TIMER;
            }
            
            // Handle the SW2 button.
            // Pressing the SW2 disconnects us from Eliot cloud temporarily
            if(lwgpio_get_value(&spareS4Input) == LWGPIO_VALUE_LOW)
            {
                toggleCloudTimer += tickCount;

                if(toggleCloudTimer >= TOGGLE_CLOUD_TIMER)
                {
                    // Invert cloud state
                    cloudState ^= true;

                    // Turn on red LED if cloud state is off
                    SetGPIOOutput(led1Green, LED_OFF);
                    SetGPIOOutput(led1Red, cloudState? LED_OFF : LED_ON);
                    toggleCloudTimer = 0;
                }
            }
            else
            {
                spareS4Input_pressed = false;
                toggleCloudTimer = 0;
            }

            // Handle the factory reset button
            if(lwgpio_get_value(&factoryResetInput) == LWGPIO_VALUE_LOW)
            {
               // Factory Reset button is pushed
               // Add the tick count to the timer
               factoryResetTimer += tickCount;
   
               if(factoryResetTimer >= FACTORY_RESET_TIMER)
               {
                  SetGPIOOutput(led1Red, LED_ON);
                  if (led1Green_function == LED1_GREEN_FUNCTION_DEFAULT)
                  {
                     led1Green_default_state = LED_OFF;
                     SetGPIOOutput(led1Green, led1Green_default_state);
                  }
                  SetGPIOOutput(led2Red, LED_OFF);
                  SetGPIOOutput(led2Green, LED_OFF);
                  resetDeviceFlag = true;
               }
               else
               {
                  SetGPIOOutput(led1Green, LED_OFF);
                  // Turn the LED on and off every quarter second
                  if((((factoryResetTimer / 250) * 250) % 500) == 0)
                  {
                     SetGPIOOutput(led1Red, LED_ON);
                  }
                  else
                  {
                     SetGPIOOutput(led1Red, LED_OFF);
                  }
               }
            }
            else // Reset button is not held
            {
               // On release do the factory reset if the flag is set
               if(resetDeviceFlag)
               {
                  // Clear Flash and restart the system
                  resetDeviceFlag = 0;
                  HandleClearFlash();
                  _time_delay(1000); // Stall any further LED control until system reset
               }

               // Clear the factory reset timer and turn off the led
               factoryResetTimer = 0;

               #if(0)
               SetGPIOOutput(led1Red, LED_OFF); // Pre-Eliot L2 LED control
               #else
               Eliot_LedSet(); // Eliot connection state on L2 when reset is not held
               #endif
            }
         }
      }
      else
      {
         counter++;
         _time_delay(1);
      }
   }

}

#else
void AddToSentMessageQueue(zone_properties * zoneProperties)
{
}

#endif // #ifndef AMDBUILD


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t SendRampCommandToTransmitTask(byte_t zoneIndex, byte_t sceneId, byte_t scenePercent, uint8_t priority)
{
   zone_properties zoneProperties;
   bool_t errorFlag = false;
   RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr;

   // build message for queue
   
      RFM100TransmitTaskMessagePtr = (RFM100_TRANSMIT_TASK_MESSAGE_PTR) _msg_alloc(RFM100TransmitTaskMessagePoolID);
      if (RFM100TransmitTaskMessagePtr)
      {
         // Get the zone properties for the zone Index
         if (GetZoneProperties(zoneIndex, &zoneProperties))
         {
            RFM100TransmitTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
            RFM100TransmitTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RFM100_TRANSMIT_TASK_MESSAGE_QUEUE);
            RFM100TransmitTaskMessagePtr->HEADER.SIZE = sizeof(RFM100_TRANSMIT_TASK_MESSAGE);
   
            RFM100TransmitTaskMessagePtr->command = RAMP;
            // fill in the other parameters
            RFM100TransmitTaskMessagePtr->groupId = zoneProperties.groupId;
            RFM100TransmitTaskMessagePtr->buildingId = zoneProperties.buildingId;
            RFM100TransmitTaskMessagePtr->houseId = zoneProperties.houseId;
            RFM100TransmitTaskMessagePtr->sceneId = sceneId;
            RFM100TransmitTaskMessagePtr->scenePercent = scenePercent;
            
            // send message and report (success=true)/(fail=false)
            
            errorFlag = !_msgq_send_priority(RFM100TransmitTaskMessagePtr, priority);
         }
         else
         {
            // Unable to get the zone properties for the zone index
            errorFlag = true;
         }
      }
      else
      {
         errorFlag = true;
         rfm100TransmitTaskMsgAllocErr++;
      }
   
   return errorFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  if the passed in scene id matches any button scene id of the passed in 
// scene controller, build a message to transmit a Scene Executed command
// to the scene controller
// 
//-----------------------------------------------------------------------------

bool_t SendSceneExecutedToTransmitTask(scene_controller_properties *sceneControllerProperties, byte_t sceneId, byte_t avgLevel, bool_t state)
{
   bool_t errorFlag = FALSE;
   bool_t found = FALSE;
   RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr;
 
   // build message for queue
   RFM100TransmitTaskMessagePtr = (RFM100_TRANSMIT_TASK_MESSAGE_PTR) _msg_alloc(RFM100TransmitTaskMessagePoolID);

   if (RFM100TransmitTaskMessagePtr)
   {
      // check the pointer passed in
      if (sceneControllerProperties)
      {
         found = FALSE;
         RFM100TransmitTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
         RFM100TransmitTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RFM100_TRANSMIT_TASK_MESSAGE_QUEUE);
         RFM100TransmitTaskMessagePtr->HEADER.SIZE = sizeof(RFM100_TRANSMIT_TASK_MESSAGE);
      
         RFM100TransmitTaskMessagePtr->command = SCENE_EXECUTED;
         // fill in the other parameters
         RFM100TransmitTaskMessagePtr->groupId =    sceneControllerProperties->groupId;
         RFM100TransmitTaskMessagePtr->buildingId = sceneControllerProperties->buildingId;
         RFM100TransmitTaskMessagePtr->houseId =    sceneControllerProperties->houseId;
         RFM100TransmitTaskMessagePtr->sceneId =    sceneId;
         RFM100TransmitTaskMessagePtr->avgLevel =   avgLevel;
         RFM100TransmitTaskMessagePtr->state =   state;
       
         // send message and report (success=true)/(fail=false)
         errorFlag = !_msgq_send_priority(RFM100TransmitTaskMessagePtr, RFM100_MESSAGE_LOW_PRIORITY);

      }
      else
      {
         // Unable to get the properties for the scene controller index
         errorFlag = true;
      }
   }
   else
   {
      errorFlag = true;
      rfm100TransmitTaskMsgAllocErr++;
   }

   return errorFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Assign a scene to a specified button in a scene controller
// 
//-----------------------------------------------------------------------------

bool_t SendAssignSceneCommandToTransmitTask(scene_controller_properties *sceneControllerProperties, byte_t buttonIdx, byte_t avgLevel)
{
   bool_t errorFlag = false;
   RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr;

   // build message for queue
   RFM100TransmitTaskMessagePtr = (RFM100_TRANSMIT_TASK_MESSAGE_PTR) _msg_alloc(RFM100TransmitTaskMessagePoolID);
   if (RFM100TransmitTaskMessagePtr)
   {
      // Get the scene controller properties for the sctl Index passed in
      if (sceneControllerProperties)
      {
         RFM100TransmitTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
         RFM100TransmitTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RFM100_TRANSMIT_TASK_MESSAGE_QUEUE);
         RFM100TransmitTaskMessagePtr->HEADER.SIZE = sizeof(RFM100_TRANSMIT_TASK_MESSAGE);

         RFM100TransmitTaskMessagePtr->command = ASSIGN_SCENE;
         // fill in the other parameters

         RFM100TransmitTaskMessagePtr->groupId =    sceneControllerProperties->groupId;
         RFM100TransmitTaskMessagePtr->buildingId = sceneControllerProperties->buildingId;
         RFM100TransmitTaskMessagePtr->houseId =    sceneControllerProperties->houseId;
         RFM100TransmitTaskMessagePtr->sceneId =    sceneControllerProperties->bank[buttonIdx].sceneId;
         RFM100TransmitTaskMessagePtr->buttonIndex =   buttonIdx;
         RFM100TransmitTaskMessagePtr->toggled = sceneControllerProperties->bank[buttonIdx].toggled;
         RFM100TransmitTaskMessagePtr->toggleable = sceneControllerProperties->bank[buttonIdx].toggleable;
         RFM100TransmitTaskMessagePtr->avgLevel =   avgLevel;
 
         // send message and report (success=true)/(fail=false)
         errorFlag = !_msgq_send_priority(RFM100TransmitTaskMessagePtr, RFM100_MESSAGE_LOW_PRIORITY);
      }
      else
      {
         // Unable to get the properties for the scene controller
         errorFlag = true;
      }
   }
   else
   {
      errorFlag = true;
      rfm100TransmitTaskMsgAllocErr++;
   }

   return errorFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Scene Controller Settings to a scene controller
// 
//-----------------------------------------------------------------------------

bool_t SendSceneSettingsCommandToTransmitTask(scene_controller_properties *sceneControllerProperties, bool_t nightMode, bool_t clearScene)
{
   bool_t errorFlag = false;
   RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr;

   // build message for queue
   RFM100TransmitTaskMessagePtr = (RFM100_TRANSMIT_TASK_MESSAGE_PTR) _msg_alloc(RFM100TransmitTaskMessagePoolID);
   if (RFM100TransmitTaskMessagePtr)
   {
      // Get the scene controller properties for the sctl Index passed in
      if (sceneControllerProperties)
      {
         RFM100TransmitTaskMessagePtr->HEADER.SOURCE_QID = 0;  //not relevant
         RFM100TransmitTaskMessagePtr->HEADER.TARGET_QID = _msgq_get_id(0, RFM100_TRANSMIT_TASK_MESSAGE_QUEUE);
         RFM100TransmitTaskMessagePtr->HEADER.SIZE = sizeof(RFM100_TRANSMIT_TASK_MESSAGE);

         RFM100TransmitTaskMessagePtr->command = SCENE_CTL_SETTING;
         // fill in the other parameters

         RFM100TransmitTaskMessagePtr->groupId =    sceneControllerProperties->groupId;
         RFM100TransmitTaskMessagePtr->buildingId = sceneControllerProperties->buildingId;
         RFM100TransmitTaskMessagePtr->houseId =    sceneControllerProperties->houseId;
         if (clearScene)
         {
            RFM100TransmitTaskMessagePtr->value =   0x02;
         }
         else
         {
            RFM100TransmitTaskMessagePtr->value = nightMode;
         }
 
         // send message and report (success=true)/(fail=false)
         errorFlag = !_msgq_send_priority(RFM100TransmitTaskMessagePtr, RFM100_MESSAGE_LOW_PRIORITY);
      }
      else
      {
         // Unable to get the properties for the scene controller
         errorFlag = true;
      }
   }
   else
   {
      errorFlag = true;
      rfm100TransmitTaskMsgAllocErr++;
   }

   return errorFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  BuildRamp()
//    given a ptr to a buffer, this builds a ramp command based on the 
// zone Index into the zone array.  This will return length of the buffer.
//
//-----------------------------------------------------------------------------

byte_t BuildRamp(byte_t* payload, byte_t zoneIdx)
{
   zone_properties zoneProperties;
   byte_t payldCt = 0;
   byte_t crcValue = 0;

   if(GetZoneProperties(zoneIdx, &zoneProperties))
   {
      payload[payldCt++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0
      payload[payldCt++] = 0xb4;  // Header
      payload[payldCt++] = 0x00;  // Length Byte - just set for now, but go back when we have the final buffer size

      // TOP DOG HEADER BYTE:
      //  Bits 0-1   = TTL (time to live)
      //  Bits 2     = Retransmit delay
      //  Bits 3-4   = Address mode
      //               00 - Broadcast
      //               01 - Anonomous Multicast
      //               10 - Unicast
      //               11 - Multicast
      //  Bits 5-7    = Family Type
      //               001 - Topdog F1
      //               110 - Topdog F2
      //
      payload[payldCt++] = (TOPDOG_F2 << 5) | (TDAM_ANON_MULTICAST << 3) | SHORT_RANDOM_TIME | TTL_TWO;
      // Address:
      //     byte 0 =  7    6    5    4    3    2    1    0
      //               |-Build ID-|  |house ID - up 5 bits|
      //
      //     byte 1 =  7    6    5    4    3    2    1    0
      //               |---HID---|        |--group up 4 bits|
      //
      //     byte 2 =  7    6    5    4    3    2    1    0
      //             |------------   groupID --------------|
      //
      payload[payldCt] = (byte_t) ((zoneProperties.buildingId << 5) & 0xe0);  // 0x57;      // First Address - byte 0  
      payload[payldCt++] |= (byte_t) ((zoneProperties.houseId >> 3) & 0x1f);  // First Address - byte 0  

      payload[payldCt] = (byte_t) ((zoneProperties.houseId << 5) & 0xe0);
      payload[payldCt++] |= (byte_t) ((zoneProperties.groupId >> 8) & 0x0f);  //0x64;     // First Address - byte 1 

      payload[payldCt++] = (byte_t) (zoneProperties.groupId & 0x0ff);  // 0x90;     // First Address - byte 2  

      payload[payldCt++] = RAMP_FUNCTION_BYTE;

      if (!zoneProperties.targetState)  // if state is off, then set power level to 0
      {
         payload[payldCt++] = 0;
      }
      else
      {
         payload[payldCt++] = (byte_t) (((((word_t) zoneProperties.powerLevel) * MAX_TD_POWER) + 50) / 100);
      }
      payload[payldCt++] = zoneProperties.rampRate;
      payload[payldCt++] = zoneProperties.deviceType;

      crcValue = UpdateCRC8(0, &payload[3], payldCt - 3);

      payload[payldCt++] = crcValue;
      payload[payldCt++] = ~crcValue;

      // now, add the length into the packet, don't include the first 3 bytes
      payload[2] = payldCt - 3;

      // Add this message to the sent message queue
      AddToSentMessageQueue(&zoneProperties);
   }

   return payldCt;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  BuildRampAll()
//
// Builds and sends the multicast command to ramp all lights
//-----------------------------------------------------------------------------
void BuildRampAll(byte_t buildingId, byte_t roomId, byte_t houseId, byte_t powerLevel)
{
   byte_t payldCt = 0;
   byte_t crcValue = 0;
   byte_t payload[32];

   payload[payldCt++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0
   payload[payldCt++] = 0xB4;  // Header
   payload[payldCt++] = 0x00;  // Length Byte - just set for now, but go back when we have the final buffer size

   // TOP DOG HEADER BYTE:
   //  Bits 0-1   = TTL (time to live)
   //  Bits 2     = Retransmit delay
   //  Bits 3-4   = Address mode
   //               00 - Broadcast
   //               01 - Anonymous Multicast
   //               10 - Unicast
   //               11 - Multicast
   //  Bits 5-7    = Family Type
   //               001 - Topdog F1
   //               110 - Topdog F2
   //
   payload[payldCt++] = (TOPDOG_F2 << 5) | (TDAM_ANON_MULTICAST << 3) | SHORT_RANDOM_TIME | TTL_TWO;
   //
   // Address:
   //     byte 0 =  7    6    5    4    3    2    1    0
   //               |-Build ID-|  |house ID - up 5 bits|
   //
   //     byte 1 =  7    6    5    4    3    2    1    0
   //               |---HID---|   [1]   |----Room ID---|
   //
   //     byte 2 =  7    6    5    4    3    2    1    0
   //               |-Room ID-|    |------Preset-------|
   //

   payload[payldCt] = (byte_t) ((buildingId << 5) & 0xE0);  // First Address - byte 0  
   payload[payldCt++] |= (byte_t) ((houseId >> 3) & 0x1F);  // First Address - byte 0  

   payload[payldCt] = (byte_t) ((houseId << 5) & 0xE0);
   payload[payldCt] |= 0x10; // R/G bit (0==Group, 1==Room)
   payload[payldCt++] |= (byte_t) ((roomId >> 3) & 0x07);  // First Address - byte 1

   payload[payldCt++] = (byte_t) ((roomId << 5) & 0xE0);   // First Address - byte 2  

   payload[payldCt++] = RAMP_FUNCTION_BYTE;

   payload[payldCt++] = (byte_t) (((((word_t) powerLevel) * MAX_TD_POWER) + 50) / 100);

   payload[payldCt++] = 50;
   payload[payldCt++] = DIMMER_TYPE;

   crcValue = UpdateCRC8(0, &payload[3], payldCt - 3);

   payload[payldCt++] = crcValue;
   payload[payldCt++] = ~crcValue;

   // now, add the length into the packet, don't include the first 3 bytes
   payload[2] = payldCt - 3;

   //Send Message
   (void) RFM100_Send(payload, payldCt);

   char dbgprint[64];
   int x=0;
   for(x=0; x<payldCt; x++)
   {
      sprintf(dbgprint + x*2, "%02X", payload[x]);
   }
   broadcastDebug(payldCt, "__RFM100PKT__", dbgprint);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  BuildAssignScene()
//    given a ptr to a buffer, this builds a Assign Scene command based on the 
// scene controller Index into the scene controller array. 
//    This will return length of the buffer.
//
//-----------------------------------------------------------------------------

byte_t BuildAssignScene(byte_t* payload, word_t groupId, 
      byte_t buildingId, 
      byte_t houseId,
      byte_t sceneId, 
      byte_t buttonIndex, 
      bool_t toggled,  
      bool_t toggleable,   // toggleable
      byte_t avgLevel)
{

   byte_t payldCt = 0;
   byte_t crcValue = 0;


      payload[payldCt++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0
      payload[payldCt++] = 0xb4;  // Header
      payload[payldCt++] = 0x00;  // Length Byte - just set for now, but go back when we have the final buffer size

      // TOP DOG HEADER BYTE:
      //  Bits 0-1   = TTL (time to live)
      //  Bits 2     = Retransmit delay
      //  Bits 3-4   = Address mode
      //               00 - Broadcast
      //               01 - Anonomous Multicast
      //               10 - Unicast
      //               11 - Multicast
      //  Bits 5-7    = Family Type
      //               001 - Topdog F1
      //               110 - Topdog F2
      //
      payload[payldCt++] = (TOPDOG_F2 << 5) | (TDAM_ANON_MULTICAST << 3) | SHORT_RANDOM_TIME | TTL_TWO;
      // Address:
      //     byte 0 =  7    6    5    4    3    2    1    0
      //               |-Build ID-|  |house ID - up 5 bits|
      //
      //     byte 1 =  7    6    5    4    3    2    1    0
      //               |---HID---|        |--group up 4 bits|
      //
      //     byte 2 =  7    6    5    4    3    2    1    0
      //             |------------   groupID --------------|
      //
      payload[payldCt] = (byte_t) ((buildingId << 5) & 0xe0);  // 0x57;      // First Address - byte 0  
      payload[payldCt++] |= (byte_t) ((houseId >> 3) & 0x1f);  // First Address - byte 0  

      payload[payldCt] = (byte_t) ((houseId << 5) & 0xe0);
      payload[payldCt++] |= (byte_t) ((groupId >> 8) & 0x0f);  //0x64;     // First Address - byte 1 

      payload[payldCt++] = (byte_t) (groupId & 0x0ff);  // 0x90;     // First Address - byte 2  

      payload[payldCt++] = ASSIGN_SCENE_FUNCTION_BYTE;

      // byte 1: scene index for the button
      payload[payldCt++] = sceneId;
      
      // Byte 2: Bits0-5-Button Index, Bit 6:state, Bit 7:toggleable
      payload[payldCt++] = (byte_t)(buttonIndex & 0x3f) | 
                           (byte_t)((toggled << 6) & 0x40) | 
                           (byte_t)((toggleable << 7) & 0x80);
      
      payload[payldCt++] = avgLevel;  // average level of all loads for the scene associated with this bank

      crcValue = UpdateCRC8(0, &payload[3], payldCt - 3);

      payload[payldCt++] = crcValue;
      payload[payldCt++] = ~crcValue;

      // now, add the length into the packet, don't include the first 3 bytes
      payload[2] = payldCt - 3;

 
   return payldCt;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  BuildSceneExecute()
//    given a ptr to a buffer, this builds a SceneExecute command based on the 
// sctl Index into the scene controller array.  This will return length of the buffer.
//
//-----------------------------------------------------------------------------

byte_t BuildSceneExecute(byte_t* payload, byte_t sceneControllerIndex, byte_t sceneId, bool_t state, byte_t avgLevel, uint16_t groupId)
{
   scene_controller_properties sctlProperties;
   byte_t payldCt = 0;
   byte_t crcValue = 0;

   if(GetSceneControllerProperties(sceneControllerIndex, &sctlProperties))
   {
      payload[payldCt++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0
      payload[payldCt++] = 0xb4;  // Header
      payload[payldCt++] = 0x00;  // Length Byte - just set for now, but go back when we have the final buffer size

      // TOP DOG HEADER BYTE:
      //  Bits 0-1   = TTL (time to live)
      //  Bits 2     = Retransmit delay
      //  Bits 3-4   = Address mode
      //               00 - Broadcast
      //               01 - Anonomous Multicast
      //               10 - Unicast
      //               11 - Multicast
      //  Bits 5-7    = Family Type
      //               001 - Topdog F1
      //               110 - Topdog F2
      //
      payload[payldCt++] = (TOPDOG_F2 << 5) | (TDAM_ANON_MULTICAST << 3) | SHORT_RANDOM_TIME | TTL_TWO;
      // Address:
      //     byte 0 =  7    6    5    4    3    2    1    0
      //               |-Build ID-|  |house ID - up 5 bits|
      //
      //     byte 1 =  7    6    5    4    3    2    1    0
      //               |---HID---|        |--group up 4 bits|
      //
      //     byte 2 =  7    6    5    4    3    2    1    0
      //             |------------   groupID --------------|
      //
      payload[payldCt] = (byte_t) ((sctlProperties.buildingId << 5) & 0xe0);  // 0x57;      // First Address - byte 0  
      payload[payldCt++] |= (byte_t) ((sctlProperties.houseId >> 3) & 0x1f);  // First Address - byte 0  

      payload[payldCt] = (byte_t) ((sctlProperties.houseId << 5) & 0xe0);
      
      payload[payldCt++] |= (byte_t) ((groupId >> 8) & 0x0f);     

      payload[payldCt++] = (byte_t) (groupId & 0x0ff);           

      payload[payldCt++] = SCENE_EXECUTED_FUNCTION_BYTE;

      // byte 1: scene index 
      payload[payldCt++] = sceneId; 
      
      // Byte 2: state is active
      if (state)
      {
         payload[payldCt++] = 0x40;
      }
      else
      {
         payload[payldCt++] = 0;
      }
     // payload[payldCt++] = (byte_t)((sctlProperties.bank[buttonIndex].toggled << 6) & 0x40); 
       
      payload[payldCt++] = avgLevel;  // average level of all loads

      crcValue = UpdateCRC8(0, &payload[3], payldCt - 3);

      payload[payldCt++] = crcValue;
      payload[payldCt++] = ~crcValue;

      // now, add the length into the packet, don't include the first 3 bytes
      payload[2] = payldCt - 3;

   }

   return payldCt;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  BuildSceneSettings()
//    given a ptr to a buffer, this builds a Scene Settings command based on the 
// sctl Index into the scene controller array.  This will return length of the buffer.
//
//-----------------------------------------------------------------------------

byte_t BuildSceneSettings(byte_t* payload, byte_t mode, word_t groupId, byte_t buildingId, byte_t houseId)
{
   scene_controller_properties sctlProperties;
   byte_t payldCt = 0;
   byte_t crcValue = 0;

 
   payload[payldCt++] = 0x86;  // Command 0x86: set bit TxF and TxB in cmd reg 0
   payload[payldCt++] = 0xb4;  // Header
   payload[payldCt++] = 0x00;  // Length Byte - just set for now, but go back when we have the final buffer size

   // TOP DOG HEADER BYTE:
   //  Bits 0-1   = TTL (time to live)
   //  Bits 2     = Retransmit delay
   //  Bits 3-4   = Address mode
   //               00 - Broadcast
   //               01 - Anonomous Multicast
   //               10 - Unicast
   //               11 - Multicast
   //  Bits 5-7    = Family Type
   //               001 - Topdog F1
   //               110 - Topdog F2
   //
   payload[payldCt++] = (TOPDOG_F2 << 5) | (TDAM_ANON_MULTICAST << 3) | SHORT_RANDOM_TIME | TTL_TWO;
   // Address:
   //     byte 0 =  7    6    5    4    3    2    1    0
   //               |-Build ID-|  |house ID - up 5 bits|
   //
   //     byte 1 =  7    6    5    4    3    2    1    0
   //               |---HID---|        |--group up 4 bits|
   //
   //     byte 2 =  7    6    5    4    3    2    1    0
   //              |------------   groupID --------------|
   //
   payload[payldCt] = (byte_t) ((buildingId << 5) & 0xe0);  // 0x57;      // First Address - byte 0  
   payload[payldCt++] |= (byte_t) ((houseId >> 3) & 0x1f);  // First Address - byte 0  

   payload[payldCt] = (byte_t) ((houseId << 5) & 0xe0);
      
   payload[payldCt++] |= (byte_t) ((groupId >> 8) & 0x0f);   
   payload[payldCt++] = (byte_t) (groupId & 0x0ff);   

   payload[payldCt++] = SCENE_CTL_SET_FUNCTION_BYTE;

   // byte 1: 1:On, for mode
   payload[payldCt++] = mode;
      
   // Byte 2: not used
   payload[payldCt++] = 0;
      
   payload[payldCt++] = 0;  

   crcValue = UpdateCRC8(0, &payload[3], payldCt - 3);

   payload[payldCt++] = crcValue;
   payload[payldCt++] = ~crcValue;

   // now, add the length into the packet, don't include the first 3 bytes
   payload[2] = payldCt - 3;


   return payldCt;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t HandleTransmitTaskMessage(RFM100_TRANSMIT_TASK_MESSAGE_PTR RFM100TransmitTaskMessagePtr, bool_t forceSendFlag)
{
   zone_properties zoneProperties;
   scene_controller_properties sceneControllerProperties; 
   byte_t processFlag = false;
   byte_t broadcastFlag = false;
   byte_t xmitBuffer[32];
   byte_t xmitLen;
   byte_t zoneIdx;
   byte_t sctlIdx;
   byte_t buttonIdx;
   byte_t sceneId;
   byte_t avgLevel;
   byte_t mode;
   bool_t rfMessageSentFlag = false;
#if DEBUG_RFM100_ANY
   char debugBuf[100];
   MQX_TICK_STRUCT            debugLocalStartTick;
   MQX_TICK_STRUCT            debugLocalEndTick;
   bool                       debugLocalOverflow;
   uint32_t                   debugLocalOverflowMs;
#endif
   
   // if message is a ramp command
   if (RFM100TransmitTaskMessagePtr->command == RAMP)
   {
      processFlag = true;
      broadcastFlag = false;

      // mutex lock (we will use this mutex for anything related to zoneArray)
      DEBUG_ZONEARRAY_MUTEX_START;
      _mutex_lock(&ZoneArrayMutex);
      DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage before ZoneArrayMutex",DEBUGBUF);
      DEBUG_ZONEARRAY_MUTEX_START;

      zoneIdx = GetZoneIndex(RFM100TransmitTaskMessagePtr->groupId, RFM100TransmitTaskMessagePtr->buildingId, RFM100TransmitTaskMessagePtr->houseId);

      //    if we can find this zone in our zone array
      if (zoneIdx < MAX_NUMBER_OF_ZONES)
      {
         if (GetZoneProperties(zoneIdx, &zoneProperties))
         {
            //       if currentState matches targetState
            if (zoneProperties.state == zoneProperties.targetState)
            {
               // if targetState is ON
               if (zoneProperties.targetState == true)
               {
                  // if currentPowerLevel matches targetPowerLevel
                  if (zoneProperties.powerLevel == zoneProperties.targetPowerLevel)
                  {
                     DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneIndex %d at powerLevel %d", zoneIdx, zoneProperties.powerLevel);
                     DBGPRINT_INTSTRSTR(__LINE__,"HandleTransmitTaskMessage next=ON powerLevel matches targetPowerLevel", DEBUGBUF);
                     processFlag = false;
                  }
                  else
                  {
                     DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneIndex %d powerLevel %d != target %d", zoneIdx, zoneProperties.powerLevel, zoneProperties.targetPowerLevel);
                     DBGPRINT_INTSTRSTR(__LINE__,"HandleTransmitTaskMessage next=ON powerLevel not at targetPowerLevel", DEBUGBUF);
                  }
               }  // if (zoneProperties.targetState == true)
               else  // next state is off
               {
                  // No need to send a ramp command when the target state and the 
                  // current state are both false. However, we still need to broadcast
                  // the level to the app if the target level and current level differ
                  processFlag = false;

                  if (zoneProperties.powerLevel != zoneProperties.targetPowerLevel)
                  {
                     DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneIndex %d powerLevel %d != target %d", zoneIdx, zoneProperties.powerLevel, zoneProperties.targetPowerLevel);
                     DBGPRINT_INTSTRSTR(__LINE__,"HandleTransmitTaskMessage next=OFF powerLevel not at targetPowerLevel", DEBUGBUF);
                     zoneProperties.powerLevel = zoneProperties.targetPowerLevel;
                     broadcastFlag = true;
                     SetZoneBit(zoneChangedThisMinuteBitArray,zoneIdx);
#ifdef SHADE_CONTROL_ADDED
                     // regardless of on/off state, for a shade (which is acting as a dimmer), change its level.
                     if (zoneProperties.deviceType == SHADE_TYPE || zoneProperties.deviceType == SHADE_GROUP_TYPE)
                     {
                        processFlag = true;
                        
                     }
#endif
                  }
               }
            }  // if (zoneProperties.state == zoneProperties.targetState)

            if (processFlag || broadcastFlag)
            {
               zoneProperties.powerLevel = zoneProperties.targetPowerLevel;
               zoneProperties.state = zoneProperties.targetState;

               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneIndex %d processFlag=%d broadcastFlag=%d powerLevel=%d state=%d", zoneIdx, processFlag, broadcastFlag, zoneProperties.powerLevel, zoneProperties.state );
               DBGPRINT_INTSTRSTR(__LINE__,"HandleTransmitTaskMessage sending updates", DEBUGBUF);

               // Set the zone properties with the updated levels and states
               SetZoneProperties(zoneIdx, &zoneProperties);

               json_t * broadcastObject;
               json_t * propObject;

               broadcastObject = json_object();
               if (broadcastObject)
               {
                  BuildZonePropertiesObject(broadcastObject, zoneIdx, (PWR_BOOL_PROPERTIES_BITMASK | PWR_LVL_PROPERTIES_BITMASK));
                  json_object_set_new(broadcastObject, "ID", json_integer(0));
                  json_object_set_new(broadcastObject, "Service", json_string("ZonePropertiesChanged"));
   
                  if (zoneProperties.AppContextId)
                  {
                     // Only send the app context ID back if it is non-zero
                     json_object_set_new(broadcastObject, "AppContextId", json_integer(zoneProperties.AppContextId));
                  }
   
                  //broadcast zonePropertiesChanged with created PropertyList
                  json_object_set_new(broadcastObject, "Status", json_string("Success"));
                  SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                  json_decref(broadcastObject);
               }

               DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"Calling Eliot_QueueZoneChanged for zone index %d", zoneIdx);
               DBGPRINT_INTSTRSTR(__LINE__,"HandleTargetValue",DEBUGBUF);

               // Send the zone property changed to the Samsung Task
               Eliot_QueueZoneChanged(zoneIdx);

            }  // if (processFlag || broadcastFlag)

            if ((processFlag) || (forceSendFlag))
            {
#ifdef SHADE_CONTROL_ADDED
               if (zoneProperties.deviceType == SHADE_TYPE || zoneProperties.deviceType == SHADE_GROUP_TYPE)
               {
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after SAMI_SendZoneStatus",DEBUGBUF);
                  _mutex_unlock(&ZoneArrayMutex);

                  DEBUG_QUBE_MUTEX_START;
                  _mutex_lock(&qubeSocketMutex);
                  DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage before qubeSocketMutex",DEBUGBUF);
                  DEBUG_QUBE_MUTEX_START;
                  if (qubeAddress)
                  {
                     DBGSNPRINTF(DEBUGBUF,SIZEOF_DEBUGBUF,"zoneIndex=%d powerLevel=%d", zoneIdx, zoneProperties.powerLevel );
                     DBGPRINT_INTSTRSTR(__LINE__,"HandleTransmitTaskMessage SendShadeCommand", DEBUGBUF);
                     SendShadeCommand(zoneIdx, zoneProperties.powerLevel);
                  }
                  else
                  {
                     DBGPRINT_INTSTRSTR_ERROR(zoneIdx,"HandleTransmitTaskMessage fail to SendShadeCommand, no qubeAddress", "");                                          
                  }
                  DBGPRINT_INTSTRSTR_IF_QUBE_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after SendShadeCommand",DEBUGBUF);
                  _mutex_unlock(&qubeSocketMutex);

                  rfMessageSentFlag = true;
               }
               else
#endif
               {
                  xmitLen = BuildRamp(xmitBuffer, zoneIdx);
                  _mutex_unlock(&ZoneArrayMutex);
                  DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after ZoneArrayMutex",DEBUGBUF);
                  (void) RFM100_Send(xmitBuffer, xmitLen);
                  rfMessageSentFlag = true;
               }
            }
            else
            {
               _mutex_unlock(&ZoneArrayMutex);
               DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after ZoneArrayMutex",DEBUGBUF);
            }

            // Determine if we need to broadcast the scene finished
            if (RFM100TransmitTaskMessagePtr->sceneId < MAX_NUMBER_OF_SCENES)
            {
               _mutex_lock(&ScenePropMutex);
               scene_properties sceneProperties;
               if(GetSceneProperties(RFM100TransmitTaskMessagePtr->sceneId, &sceneProperties))
               {
                  if((sceneProperties.running == true) &&
                     (RFM100TransmitTaskMessagePtr->scenePercent == 100))
                  {
                     json_t * broadcastObject = json_object();
                     if (broadcastObject)
                     {
                        json_t * propObject = json_object();
                        if (propObject)
                        {
                           // Set the scene to not running
                           sceneProperties.running = false;
      
                           // Set the scene properties
                           SetSceneProperties(RFM100TransmitTaskMessagePtr->sceneId, &sceneProperties);
                           // Create the broadcast object to let users know that the scene is complete
                           json_object_set_new(broadcastObject, "ID", json_integer(0));
                           json_object_set_new(broadcastObject, "Service", json_string("ScenePropertiesChanged"));
                           json_object_set_new(broadcastObject, "SID", json_integer(RFM100TransmitTaskMessagePtr->sceneId));
      
                           if (zoneProperties.AppContextId)
                           {
                              // Only send the app context ID back if it is non-zero
                              json_object_set_new(broadcastObject, "AppContextId", json_integer(zoneProperties.AppContextId));
                           }
      
                           json_object_set_new(broadcastObject, "Status", json_string("Success"));
      
                           json_object_set_new(propObject, "Running", json_false());
                           json_object_set_new(broadcastObject, "PropertyList", propObject);
                           SendJSONPacket(MAX_NUMBER_OF_INCOMING_JSON_CONNECTIONS, broadcastObject);
                        }
                        json_decref(broadcastObject);
                     }
                     
                     // send a message to any scene controller that has this scene attached to a button 
                     //     that the scene has been activated
                  }
               }

               _mutex_unlock(&ScenePropMutex);
            }

         } // if (GetZoneProperties(zoneIdx, &zoneProperties))
         else
         {
            _mutex_unlock(&ZoneArrayMutex);
            DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after ZoneArrayMutex",DEBUGBUF);
         }
      }  // if (zoneIdx < MAX_NUMBER_OF_ZONES)
      else
      {
         _mutex_unlock(&ZoneArrayMutex);
         DBGPRINT_INTSTRSTR_IF_ZONEARRAY_MUTEX_OVERFLOW(__LINE__,"MUTEX_OVERFLOW HandleTransmitTaskMessage after ZoneArrayMutex",DEBUGBUF);
      }

   }  // if (RFM100TransmitTaskMessagePtr->command == RAMP)
   else if (RFM100TransmitTaskMessagePtr->command == ASSIGN_SCENE)
   {
      // handle building and sending Assign_Scene to SCTL
        xmitLen = BuildAssignScene(xmitBuffer, 
                                 RFM100TransmitTaskMessagePtr->groupId, 
                                 RFM100TransmitTaskMessagePtr->buildingId, 
                                 RFM100TransmitTaskMessagePtr->houseId,
                                 RFM100TransmitTaskMessagePtr->sceneId, 
                                 RFM100TransmitTaskMessagePtr->buttonIndex, 
                                 RFM100TransmitTaskMessagePtr->toggled,   // toggled
                                 RFM100TransmitTaskMessagePtr->toggleable,   // toggleable
                                 RFM100TransmitTaskMessagePtr->avgLevel);
      (void) RFM100_Send(xmitBuffer, xmitLen);
      rfMessageSentFlag = true;
   }
   else if (RFM100TransmitTaskMessagePtr->command == SCENE_EXECUTED)
   {
      // handle building and sending Scene_Executed to SCTL
      sctlIdx =  GetSceneControllerIndex(RFM100TransmitTaskMessagePtr->groupId, RFM100TransmitTaskMessagePtr->buildingId, RFM100TransmitTaskMessagePtr->houseId);
      avgLevel = RFM100TransmitTaskMessagePtr->avgLevel;
      bool_t state = RFM100TransmitTaskMessagePtr->state;
      sceneId = RFM100TransmitTaskMessagePtr->sceneId;
      xmitLen = BuildSceneExecute(xmitBuffer, sctlIdx, sceneId, state, avgLevel, 0);
      (void) RFM100_Send(xmitBuffer, xmitLen);
      rfMessageSentFlag = true;
   }
   else if (RFM100TransmitTaskMessagePtr->command == SCENE_CTL_SETTING)
   {
      // handle building and sending Scene_Ctl_Setting to SCTL
      mode = RFM100TransmitTaskMessagePtr->value;
      xmitLen = BuildSceneSettings(xmitBuffer, mode, RFM100TransmitTaskMessagePtr->groupId, RFM100TransmitTaskMessagePtr->buildingId, RFM100TransmitTaskMessagePtr->houseId);
      (void) RFM100_Send(xmitBuffer, xmitLen);
      rfMessageSentFlag = true;
     
   }
     
   
   return rfMessageSentFlag;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void InitializeSystemProperties(void)
{
   systemProperties.locationInfoString[0] = 0;
   systemProperties.addALight = false;
   systemProperties.gmtOffsetSeconds = 0;
   systemProperties.effectiveGmtOffsetSeconds = 0;
   systemProperties.useDaylightSaving = true;
   systemProperties.latitude.degrees = 0;
   systemProperties.latitude.minutes = 0;
   systemProperties.latitude.seconds = 0;
   systemProperties.longitude.degrees = 0;
   systemProperties.longitude.minutes = 0;
   systemProperties.longitude.seconds = 0;
   systemProperties.configured = false;
   systemProperties.addASceneController = false;
   systemShadeSupportProperties.addAShade = false;
   systemTimeValid = false;
  
   // Since the GMT offset and effective GMT offset are equal
   // we are not in DST
   inDST = FALSE;
 
   // Initialize the system time flag to invalid
   systemTimeValid = false;
}
