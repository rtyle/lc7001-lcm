// JSR v1.1.1 - A stream-oriented JSON reader - Legrand 2019

// v1.1.1
// Add object_count to count all objects and sub-objects
// Add top_level_object_count to count top level objects
// Add array_depth to indicate when an object array has closed

// v1.1.0
//  Improve handling of nested objects
//  Add handling of ints and bools
//  Add JSR_PAIR_TRANSIENT pair option
//  Add JSR_OPTION_MATCH_COUNT_RECURSE option

#ifndef JSR_H_
#define JSR_H_

#include <stdint.h>
#include "string.h"

// Compile options
#define JSR_OPTION_ENABLE_SAFETIES		1		// Enable error checking
#define JSR_OPTION_PRINTF_DEBUG			0		// Printf debugging for development
#define JSR_MAX_DEPTH					12		// Maximum depth for match counting

// Bits defined for JSR_ENV.state - Not for external use
#define JSR_STATE_IN_QUOTES				0x80	// Set when the parser is reading within quotes
#define JSR_STATE_COLON 				0x40	// Set when a colon is encountered
#define JSR_STATE_HTTP_BODY				0x20	// Set when an '\r\n\r\n' string is encountered

// Bits defined for JSR_ENV.error - Non-fatal errors indicate a potentially undesirable condition
#define JSR_ERROR_VALUE_OVERFLOW 		0x80	// Set when JSR_ENV.value_max_size is too low for the content
#define JSR_ERROR_KEY_OVERFLOW 			0x40	// Set when JSR_ENV.key_max_size is too low for the content
#define JSR_ERROR_EMPTY_PAIR 			0x20	// Set when a pair has options==0 
#define JSR_ERROR_ZERO_LENGTH_DEST		0x10	// Destination field has zero length specified
// Bits defined for JSR_ENV.error - Fatal errors indicate a serious problem
#define JSR_ERROR_NULL_PAIR_PTR	 		0x08	// Set when pairs==0 
#define JSR_ERROR_NULL_KEY_PTR			0x04	// Set when key_str==0
#define JSR_ERROR_NULL_VALUE_PTR		0x02	// Set when value_str==0

// jsr_read() return values
#define JSR_RET_OK 						0x00	// Normal return value
#define JSR_RET_FATAL_ERROR				0x80	// Fatal error - see JSR_ENV.error for details
#define JSR_RET_END_OBJECT 				0x40	// End of object has been encountered - More calls required to

// Bits defined for JS_ENV.option
#define JSR_OPTION_SKIP_HTTP_HEADER		0x80	// When set, input stream is ignored until \r\n\r\n is read
// With match count recurse enabled, a parent's match count is incremented by matches within child objects
// in addition to the parent's own matches.
#define JSR_OPTION_MATCH_COUNT_RECURSE	0x40	// Add an object's match count to its parent's.

// Bits defined for JSR_PAIR.option
#define JSR_PAIR_KEY 					0x01	// Set if the pair defines a key
#define JSR_PAIR_VALUE 					0x02	// Set if the pair defines a value
#define JSR_PAIR_BOTH 					0x03	// Set if the pair defines both a value and key
#define JSR_PAIR_NEITHER				0x00	// Pairs with this value skipped over
#define JSR_PAIR_TRANSIENT				0x80	// Captured variable is cleared when a new object begins at the same depth

// Constants
#define JSR_HTTP_HEADER_DELIMITER		('\r'<<24 | '\n'<<16 | '\r'<<8 | '\n')

#if JSR_OPTION_PRINTF_DEBUG
#include <stdio.h>
#endif

typedef struct JSR_PAIR
{
	char* key_str;			// Pointer to char[] buffer for the key string
	char* value_str;		// Pointer to char[] buffer for the value string
	uint32_t key_max_length;		// Capacity of the key buffer - optional when key_str is supplied
	uint32_t value_max_length;		// Capacity of the value buffer - optional when value_str is supplied
	uint8_t option;			// Defines whether key, value or both fields are supplied
	uint8_t depth;			// The depth at which data was captured
} JSR_PAIR;


typedef struct JSR_ENV
{
	char* key_str;			// Pointer to temporary char[] buffer for key parsing
	char* value_str;		// Pointer to temporary char[] buffer for value parsing
	uint32_t key_max_length;		// Key string capacity
	uint32_t value_max_length;		// Value string capacity
	uint32_t write_idx ;			// Indexes writes to key and value strings
	uint32_t read_idx;			// Indexes reads from the stream buffer
	uint8_t option;
	uint8_t state;
	uint8_t error;
	uint32_t depth;				// Tracks tree depth of the current object
	uint8_t match_threshold;	// Matched pair threshold within an object where jsr_read() will halt
	uint8_t match_min_depth;	// Don't evaluate match_count above this level
	uint8_t match_count[JSR_MAX_DEPTH];		// Tracks matches within an object
	uint8_t pair_count;			// Number of pair elements to consider
	uint32_t fifo4;			// 4-byte character FIFO implemented as uint32
	uint32_t object_count;
	uint32_t top_level_object_count;
	uint32_t array_depth;
	struct JSR_PAIR *pairs;	// Pointer to array of pair structures for comparison
} JSR_ENV;


void jsr_env_init(JSR_ENV* env);
void jsr_env_reset(JSR_ENV* env);
uint8_t jsr_read(JSR_ENV* env, char* buffer, uint32_t size);


#endif /* JSR_H_ */
