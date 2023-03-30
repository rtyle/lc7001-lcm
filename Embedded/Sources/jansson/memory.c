/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2011-2012 Basile Starynkevitch <basile@starynkevitch.net>
 *
 * Jansson is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#ifndef AMDBUILD
#include <mqx.h>
#include <bsp.h>
#else
#include <stdio.h>
#include <stdint.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "jansson.h"
#include "jansson_private.h"

unsigned long malloc_count;
unsigned long free_count;
unsigned long malloc_fail_count;
//extern _lwmem_pool_id JSONPoolID;


//#define MATTS_BASE_LIBS_MEMORY_ALLOCATION_DIAGNOSTICS

#ifdef MATTS_BASE_LIBS_MEMORY_ALLOCATION_DIAGNOSTICS

#define MAX_NUMBER_OF_MALLOCS 1000

typedef struct
{
   unsigned char usedFlag;
   void * address;
} mallocEntry;

mallocEntry mallocArray[MAX_NUMBER_OF_MALLOCS];
unsigned long totalMallocs = 0;
unsigned long maxTotalMallocs = 0;

#endif

void *jsonp_malloc(size_t size)
{
   void * returnPtr;
   volatile unsigned long i = size;
   char test[60];
   
   if (!size)
   {
      return NULL;
   }

#ifndef AMDBUILD
   snprintf(test, sizeof(test), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);  
   snprintf(test, sizeof(test), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);  
   snprintf(test, sizeof(test), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);  
#endif
   i++;

#ifndef AMDBUILD
   returnPtr = _lwmem_alloc_system(size);
#else
   returnPtr = malloc(size);
#endif
 
   if (!returnPtr)
   {
      malloc_fail_count++;
      return NULL;
   }
   malloc_count++;
   
#if 0
   i++;
   snprintf(test, sizeof(test), "Static RAM Usage:   %d Bytes\r\n", (uint32_t)_mqx_get_initialization()->START_OF_KERNEL_MEMORY - 0x1FFF0000);  
   snprintf(test, sizeof(test), "  Peak RAM Usage:   %d Bytes(%d%%)\r\n", (uint32_t)_mem_get_highwater() - 0x1FFF0000, ((uint32_t)_mem_get_highwater() - 0x1FFF0000)/0x51E);  
   snprintf(test, sizeof(test), "Current RAM Free:   %d Bytes(%d%%)\r\n", _mem_get_free(), _mem_get_free()/0x51E);  
#endif
    


#ifdef MATTS_BASE_LIBS_MEMORY_ALLOCATION_DIAGNOSTICS
   while ((i < MAX_NUMBER_OF_MALLOCS) && (mallocArray[i].usedFlag))
   {
      i++;
   }
   if (i < MAX_NUMBER_OF_MALLOCS)
   {
      mallocArray[i].usedFlag = 1;
      mallocArray[i].address = returnPtr;
      totalMallocs++;
      if (totalMallocs > maxTotalMallocs)
      {
         maxTotalMallocs = totalMallocs;
      }
   }
#endif

   return returnPtr;
//    return (*do_malloc)(size);
}

void jsonp_free(void *ptr)
{
   unsigned long i = 0;
   
   if (!ptr)
   {
      return;
   }
   free_count++;
   
#ifdef MATTS_BASE_LIBS_MEMORY_ALLOCATION_DIAGNOSTICS
   while ((i < MAX_NUMBER_OF_MALLOCS) && (mallocArray[i].address != ptr))
   {
      i++;
   }
   if (i < MAX_NUMBER_OF_MALLOCS)
   {
      mallocArray[i].usedFlag = 0;
      mallocArray[i].address = 0;
      totalMallocs--;
   }
#endif

#ifndef AMDBUILD
   _mem_free(ptr);
#else
   free(ptr);
#endif
}

char *jsonp_strdup(const char *str)
{
    char *new_str;

    new_str = jsonp_malloc(strlen(str) + 1);
    if(!new_str)
        return NULL;

    strcpy(new_str, str);
    return new_str;
}

//void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn)
//{
//    do_malloc = malloc_fn;
//    do_free = free_fn;
//}
