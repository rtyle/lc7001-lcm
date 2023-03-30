#include "MQXWrapper.h"
#include "Debug.h"
#include "onq_standard_types.h"
#include "onq_endian.h"
#include <time.h>
#include <thread>
#include <map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define SNTP_PACKET_SIZE 48
#define   SECS_TO_1970 2208988800L   // from RFC 868 
#define   SECS_TO_2015 3637323830L   // from 1900 to 2015

typedef struct message_pool_struct
{
   uint16_t messageSize;
   uint16_t numMessages;
   uint16_t growNumber;
   uint16_t growLimit;
} message_pool;

static std::map<int, message_pool> messageQueuePoolIDMap;

extern "C"
{
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _time_get(TIME_STRUCT * mqxTime)
   {
      Clock::getClock()->getTime(mqxTime);

      logDebug("Current Time in seconds = %d", mqxTime->SECONDS);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _time_set(TIME_STRUCT * mqxTime)
   {
      Clock::getClock()->setTime(mqxTime);

      logDebug("Current Time in seconds = %d", mqxTime->SECONDS);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _time_delay(int milliseconds)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   boolean _time_to_date(TIME_STRUCT * mqxTime, DATE_STRUCT * mqxDate)
   {
      time_t seconds;
      seconds = mqxTime->SECONDS;

      struct tm * timeStruct = gmtime(&seconds);
      mqxDate->YEAR = timeStruct->tm_year + 1900;
      mqxDate->MONTH = timeStruct->tm_mon + 1;
      mqxDate->DAY = timeStruct->tm_mday;
      mqxDate->HOUR = timeStruct->tm_hour;
      mqxDate->MINUTE = timeStruct->tm_min;
      mqxDate->SECOND = timeStruct->tm_sec;
      mqxDate->MILLISEC = 0;
      mqxDate->WDAY = timeStruct->tm_wday;
      mqxDate->YDAY = timeStruct->tm_yday;

      return true;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   boolean _time_from_date(DATE_STRUCT * mqxDate, TIME_STRUCT * mqxTime)
   {
      struct tm timeStruct;
      timeStruct.tm_year = mqxDate->YEAR - 1900;
      timeStruct.tm_mon = mqxDate->MONTH - 1;
      timeStruct.tm_mday = mqxDate->DAY;
      timeStruct.tm_hour = mqxDate->HOUR;
      timeStruct.tm_min = mqxDate->MINUTE;
      timeStruct.tm_sec = mqxDate->SECOND;
      timeStruct.tm_wday = mqxDate->WDAY;
      timeStruct.tm_yday = mqxDate->YDAY;

      time_t seconds = timegm(&timeStruct);

      boolean converted = false;
      if(seconds > 0)
      {
         converted = true;
         mqxTime->SECONDS = (uint32_t)seconds;
         mqxTime->MILLISECONDS = 0;
      }

      return converted;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   uint32_t SNTP_oneshot(_ip_address destination, uint32_t timeoutMS)
   {
      char sendPkt[750];
      char recvPkt[750*2];
      uint32_t returnValue = RTCS_ERROR;
      
      if(destination != 0)
      {
         // Open a UDP socket to talk to the Time NIST Server
         int timeSock = socket(PF_INET, SOCK_DGRAM, 0);

         if(timeSock != -1)
         {
            // Set the socket options to timeout after the timeout parameter
            struct timeval opt_value;
            opt_value.tv_sec = (timeoutMS / 1000);
            opt_value.tv_usec = (timeoutMS % 1000) * 1000;
            int result = setsockopt(timeSock, SOL_SOCKET, SO_RCVTIMEO, &opt_value, sizeof(opt_value));

            if(result != -1)
            {
               // Set up the remote time server address
               struct sockaddr_in remote_sin;
               memset(&remote_sin, 0, sizeof(remote_sin));
               remote_sin.sin_family = AF_INET;
               remote_sin.sin_port = htons(123);
               remote_sin.sin_addr.s_addr = destination;

               // Build the packet to request the time from the NTP server
               memset(sendPkt, 0, sizeof(sendPkt));
               sendPkt[0] = 0x1b;
               sendPkt[1] = 0x0f;
               sendPkt[2] = 0x08;
               sendPkt[40] = 0x41;
               sendPkt[41] = 0x41;
               sendPkt[42] = 0x41;
               sendPkt[43] = 0x41;
               sendPkt[44] = 0x00;

               // Send the packet to the server
               result = sendto(timeSock, sendPkt, SNTP_PACKET_SIZE, 0, (struct sockaddr *) &remote_sin, sizeof(remote_sin));
               if(result != -1)
               {
                  // Get the response from the server
                  result = recvfrom(timeSock, recvPkt, SNTP_PACKET_SIZE + 16, 0, NULL, NULL);

                  if (result > 0)
                  {
                     // RFC 958 - Network Time Protocol
                     // ---------------------------------------------
                     // Time is returned, 8 bytes for each time
                     //    4 bytes are seconds since 1/1/1900
                     //    4 bytes are fraction part -- 0.2 nanosecond interval
                     // NTP time will rollover 2036 or 2^32 or 136 years
                     // 
                     // 4 Times are returned,
                     //   byte 16: reference time stamp - starts at byte 16 of packet
                     //   byte 24: origin time stamp
                     //   byte 32: receive time stamp
                     //   byte 40: transmit time stamp **** Use this one for LCM time
                     //
                     uint32_t timeRcvd;
                     HighByteOfDword(timeRcvd) = recvPkt[40];
                     UpperMiddleByteOfDword(timeRcvd) = recvPkt[41];
                     LowerMiddleByteOfDword(timeRcvd) = recvPkt[42];
                     LowByteOfDword(timeRcvd) = recvPkt[43];

                     // Do a sanity check on time, has to be at least 2015
                     if (timeRcvd > SECS_TO_2015)
                     {
                        // t4 is time since 1/1/1900, convert to unix epoch time (since 1/1/1970)
                        TIME_STRUCT systemTime;
                        systemTime.SECONDS = (timeRcvd - SECS_TO_1970);
                        systemTime.MILLISECONDS = 0;

                        // set the real time clock with the received time
                        _time_set(&systemTime);

                        returnValue = RTCS_OK;
                     }
                     else
                     {
                        // Time was in the past
                        returnValue = RTCS_ERROR;
                     }
                  } 
                  else 
                  {
                     // Failed to receive bytes from the server
                     returnValue = RTCSERR_TIMEOUT;
                  }
               } 
               else
               {
                  // Failed to send the bytes to the server
                  returnValue = errno;
               }
            } 
            else 
            {
               // Failed to set the socket options
               returnValue = errno;
            }

            // Shutdown and close the socket
            shutdown(timeSock, FLAG_CLOSE_TX);
            close(timeSock);
         } 
         else
         {
            // Failed to open the socket
            returnValue = errno;
         }
      } 
      else 
      {
         // Invalid parameter. Destination was not specified
         returnValue = RTCSERR_INVALID_PARAMETER;
      }

      return returnValue;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _mutex_lock(pthread_mutex_t *mutex)
   {
      logDebug("Locking Mutex %p", mutex);
      pthread_mutex_lock(mutex);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _mutex_unlock(pthread_mutex_t *mutex)
   {
      logDebug("Unlocking Mutex %p", mutex);
      pthread_mutex_unlock(mutex);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   int _msgpool_create(uint16_t messageSize, uint16_t numMessages, uint16_t growNumber, uint16_t growLimit)
   {
      int poolId = -1;

      message_pool messagePool;
      messagePool.messageSize = messageSize;
      messagePool.numMessages = numMessages;
      messagePool.growNumber = growNumber;
      messagePool.growLimit = growLimit;

      if(messageQueuePoolIDMap.empty())
      {
         poolId = 1;
      }
      else
      {
         // Get the maximum value of the pool IDs from the map. 
         // Maps are sorted by keys from smallest to largest so the last
         // element will have the largest key.
         // Add one to get the next pool Id to use
         poolId = messageQueuePoolIDMap.rbegin()->first + 1;
      }

      // Add the structure to the map
      messageQueuePoolIDMap.insert(std::pair<int, message_pool>(poolId, messagePool));

      return poolId;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void * _msg_alloc(int poolId)
   {
      // Find the pool ID in the map
      void * pointer = NULL;

      auto messageQueuePoolIter = messageQueuePoolIDMap.find(poolId);
      if(messageQueuePoolIter != messageQueuePoolIDMap.end())
      {
         pointer = malloc(messageQueuePoolIter->second.messageSize);
      }
      else
      {
         logError("Message Queue Pool ID %d is invalid. Returning null", poolId);
      }

      return pointer;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   int _msgq_get_id(int flags, int queueId)
   {
      return queueId;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   
   void _msg_free(void * pointer)
   {
      free(pointer);
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   boolean RTCS_resolve_ip_address(const char * hostname, _ip_address * ipAddress, char * ipName, uint32_t ipNameSize)
   {
      boolean resolved = false;
      struct hostent *result;

      result = gethostbyname(hostname);
      if(result == NULL)
      {
         logError("Unable to resolve hostname %s", hostname);
      }
      else
      {
         // Copy the network address to the sockaddr_in structure
         memcpy(ipAddress, result->h_addr_list[0], result->h_length);
      }

      return resolved;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void * _mem_alloc_system(uint32_t size)
   {
      void * pointer = malloc(size);
      return pointer;
   }

   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------
   // -------------------------------------------------------------------------

   void lwgpio_set_value(LWGPIO_STRUCT_PTR, LWGPIO_VALUE)
   {
   }
}

std::shared_ptr<Clock> Clock::clockInstance = nullptr;

Clock* Clock::getClock()
{
   if(clockInstance == nullptr)
   {
      clockInstance = std::shared_ptr<Clock>(new Clock());
   }

   return clockInstance.get();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Clock::Clock() :
   seconds(1041480306) // 01/02/03 04:05:06
{
   // Start the one second thread
   running = true;
   oneSecondThread = std::move(std::thread(&Clock::runOneSecondThread, this));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

Clock::~Clock()
{
   running = false;

   if(oneSecondThread.joinable())
   {
      oneSecondThread.join();
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void Clock::setTime(TIME_STRUCT * time)
{
   std::lock_guard<std::mutex> lock(oneSecondMutex);
   seconds = time->SECONDS;
   oneSecondCondition.notify_one();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void Clock::getTime(TIME_STRUCT * time)
{
   time->SECONDS = seconds;
   time->MILLISECONDS = 0;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void Clock::runOneSecondThread()
{
   while(running)
   {
      std::unique_lock<std::mutex> lock(oneSecondMutex);
      
      // Sleep for a second and then increment the seconds timer
      std::cv_status status = oneSecondCondition.wait_for(lock, std::chrono::milliseconds(1000));

      if(status == std::cv_status::timeout)
      {
         seconds++;
      }
      else
      {
         // One second timer was interrupted by a set operation. 
         // Do not increment the timer in this case, but
         // immediately go back to waiting for the condition again
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
