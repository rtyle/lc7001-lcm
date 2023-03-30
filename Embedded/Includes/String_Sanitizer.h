/*
 * String_Sanitizer.h
 *
 *  Created on: Mar 7, 2019
 *      Author: Legrand
 */

#ifndef STRING_SANITIZER_H_
#define STRING_SANITIZER_H_

// Enable conversion of illegal ASCII characters to legal 2-byte Unicode characters.
#define STRING_SANITIZER_MULTIBYTE_EXPANSION 0

#include "includes.h"

char sanitizedNameString[MAX_SIZE_NAME_STRING];

void SanitizeString(char*, char*, uint16_t);
void SanitizeJsonStringObject(json_t*);
char* TaskSanitizeString(char*);
json_t * Sanitized_json_string(char * stringptr);

#endif /* STRING_SANITIZER_H_ */
