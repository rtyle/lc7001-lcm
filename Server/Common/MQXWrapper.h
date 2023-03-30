#ifndef _MQX_WRAPPER_H_
#define _MQX_WRAPPER_H_

#ifdef AMDBUILD
#if (__cplusplus)
extern "C"
{
#endif

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#define _PTR_        *
#define TRUE         true
#define FALSE        false
#define LWSEM_STRUCT    int
#define MUTEX_STRUCT    pthread_mutex_t
#define MAX_HOSTNAMESIZE   64
#define RTCS_OK 0
#define RTCS_ERROR -1
#define RTCS_SOCKET_ERROR -1
#define FLAG_CLOSE_TX SHUT_RDWR
#define RTCS_ERROR_BASE (0x00001000ul)
#define RTCSERR_TIMEOUT (RTCS_ERROR_BASE|0x103)
#define RTCSERR_INVALID_PARAMETER (RTCS_ERROR_BASE|0x105)

   typedef struct message_header_struct
   {
      int SIZE;
      int TARGET_QID;
      int SOURCE_QID;
      unsigned char CONTROL;
      unsigned char RESERVED;
   } MESSAGE_HEADER_STRUCT, * MESSAGE_HEADER_STRUCT_PTR;

   typedef int _pool_id;
   typedef int _timer_id;
   typedef unsigned char _enet_address[6];

   typedef uint8_t      boolean;
   typedef int8_t       int_8;
   typedef uint8_t      uint_8;
   typedef uint8_t      byte;
   typedef uint8_t      byte_t;
   typedef uint8_t      uchar;
   typedef int16_t      int_16;
   typedef uint16_t     uint_16;
   typedef int32_t      int_32;
   typedef uint32_t     uint_32;
   typedef uint32_t     uint32;
   typedef void *       pointer;
   typedef uint32_t     _ip_address;

   // TIME_STRUCT is defined in MQX/mqx/source/include/mqx.h but needs to be 
   // defined here for the PC Build
   typedef struct time_struct
   {
      uint32_t SECONDS;
      uint32_t MILLISECONDS;
   } TIME_STRUCT, * TIME_STRUCT_PTR;

   // DATE_STRUCT is defined in MQX/mqx/source/include/mqx.h but needs to be
   // defined here for the PC Build
   typedef struct date_struct
   {
      int16_t YEAR;
      int16_t MONTH;
      int16_t DAY;
      int16_t HOUR;
      int16_t MINUTE;
      int16_t SECOND;
      int16_t MILLISEC;
      int16_t WDAY;
      int16_t YDAY;
   } DATE_STRUCT, * DATE_STRUCT_PTR;

   typedef struct lwgpio_struct
   {
      uint32_t *pcr_reg;
      uint32_t *iomuxc_reg;
      uint32_t *gpio_ptr;
      uint32_t pinmask;
      uint32_t flags;
   } LWGPIO_STRUCT, * LWGPIO_STRUCT_PTR;

   typedef enum {
      LWGPIO_VALUE_LOW,
      LWGPIO_VALUE_HIGH,
      LWGPIO_VALUE_NOCHANGE
   } LWGPIO_VALUE;

   // -------------------------------------------------------------------------
   // The following functions need to be redefined for the Linux build to use
   // the appropriate native Linux OS calls
   // -------------------------------------------------------------------------

   // Time Functions
   void _time_get(TIME_STRUCT * mqxTime);
   void _time_set(TIME_STRUCT * mqxTime);
   void _time_delay(int milliseconds);
   boolean _time_to_date(TIME_STRUCT * mqxTime, DATE_STRUCT * mqxDate);
   boolean _time_from_date(DATE_STRUCT * mqxDate, TIME_STRUCT * mqxTime);
   uint32_t SNTP_oneshot(_ip_address destination, uint32_t timeout);

   // Mutex functions
   void _mutex_lock(pthread_mutex_t *mutex);
   void _mutex_unlock(pthread_mutex_t *mutex);

   // Queue functions
   int  _msgpool_create(uint16_t messageSize, uint16_t numMessages, uint16_t growNumber, uint16_t growLimit);
   void * _msg_alloc(int _poolId);
   int _msgq_get_id(int flags, int queueId);
   void _msg_free(void *);

   // Network Functions
   boolean RTCS_resolve_ip_address(const char * name, _ip_address * ipAddress, char * ipName, uint32_t ipNameSize);

   // Memory Functions
   void * _mem_alloc_system(uint32_t size);

   // LED Functions
   void lwgpio_set_value(LWGPIO_STRUCT_PTR, LWGPIO_VALUE);

#if (__cplusplus)
}

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
class Clock
{
   public:
      virtual ~Clock();
      static Clock* getClock();
      void setTime(TIME_STRUCT * time);
      void getTime(TIME_STRUCT * time);

   private:
      Clock();
      static std::shared_ptr<Clock> clockInstance;
      uint32_t seconds;

      std::thread oneSecondThread;
      std::mutex oneSecondMutex;
      std::condition_variable oneSecondCondition;
      bool running;
      void runOneSecondThread();
};

#endif // __cplusplus

#endif // AMDBUILD

#endif // _MQX_FOR_LINUX_H_
