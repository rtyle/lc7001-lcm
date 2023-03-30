#ifndef ONQ_STANDARD_TYPES_H
#define ONQ_STANDARD_TYPES_H

//-----------------------------------------------------------------------------

typedef unsigned char      byte_t;
typedef unsigned short     word_t;
typedef unsigned long      dword_t;

typedef char               signed_byte_t;
typedef short              signed_word_t;
typedef long               signed_dword_t;

typedef volatile unsigned short onqtimer_t;
typedef volatile unsigned long large_timer_t;

typedef unsigned char      bool_t;

typedef byte_t *           far_byte_ptr;
typedef word_t *           far_word_ptr;
typedef dword_t *          far_dword_ptr;

typedef const byte_t *     far_const_byte_ptr;
typedef const word_t *     far_const_word_ptr;
typedef const dword_t *    far_const_dword_ptr;

typedef byte_t *           byte_ptr;
typedef word_t *           word_ptr;
typedef dword_t *          dword_ptr;

typedef const byte_t *     const_byte_ptr;
typedef const word_t *     const_word_ptr;
typedef const dword_t *    const_dword_ptr;

//-----------------------------------------------------------------------------

#endif
