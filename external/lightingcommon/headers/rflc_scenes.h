//
//  scenes.m
//
//  Copyright (c) 2015 Legrand. All rights reserved.
//

#ifndef __rflc_scenes_h__
#define __rflc_scenes_h__

#ifdef __cplusplus
namespace legrand
{
namespace rflc
{
namespace scenes
{
#endif // __cplusplus

   typedef enum
   {
      NONE        = 0,
      ONCE        = 1,
      EVERY_WEEK  = 2,
   } FrequencyType;
   
   typedef enum
   {
      REGULAR_TIME   = 0,
      SUNRISE        = 1,
      SUNSET         = 2,
   } TriggerType;
   
   typedef union
   {
      struct {
         unsigned char sunday:      1;
         unsigned char monday:      1;
         unsigned char tuesday:     1;
         unsigned char wednesday:   1;
         unsigned char thursday:    1;
         unsigned char friday:      1;
         unsigned char saturday:    1;
         unsigned char spare7:      1;
      } DayBits;
   
      unsigned char days;
   } DailySchedule;

#ifdef __cplusplus
} // scenes
} // rflc
} // legrand
#endif // __cplusplus

#endif // __rflc_scenes_h__
