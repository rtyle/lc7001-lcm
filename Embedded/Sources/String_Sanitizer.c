/*
 * String_Sanitizer.c
 *
 *  Created on: Mar 7, 2019
 *      Author: Legrand
 */

#include "String_Sanitizer.h"


void SanitizeString(char* outStringPtr, char* inStringPtr, uint16_t sizeofOutString)
{
   uint16_t outStringByteLength = 0;
   uint16_t utf8_char = 0;

   while ((*inStringPtr) && ((outStringByteLength + 1) < sizeofOutString))
   {
      if ((*inStringPtr & UTF8_2_BYTE_MASK) == UTF8_2_BYTE_TAG)
      {  // two byte character
         // make sure we have both bytes
         if (*(inStringPtr + 1) == 0)
         {  // hit our null before satisfying the two bytes
            *outStringPtr++ = '_';
            inStringPtr++;
         }
         else
         {
            // For now, force all multibyte characters to underscrors
            if (true || (outStringByteLength + 2) >= sizeofOutString)
            {
               *outStringPtr++ = '_';
               outStringByteLength++;
               inStringPtr += 2;
            }
            else
            {
               *outStringPtr++ = *inStringPtr++;
               *outStringPtr++ = *inStringPtr++;
               outStringByteLength += 2;
            }
         }
      }
      else if ((*inStringPtr & UTF8_3_BYTE_MASK) == UTF8_3_BYTE_TAG)
      {  // three byte character
         // make sure we have all bytes
         if (*(inStringPtr + 1) == 0)
         {  // hit our null before satisfying the three bytes
            *outStringPtr++ = '_';
            inStringPtr++;
         }
         else if (*(inStringPtr + 2) == 0)
         {  // hit our null before satisfying the three bytes
            *outStringPtr++ = '_';
            inStringPtr += 2;
         }
         else
         {
            // For now, force all multibyte characters to underscrors
            if (true || (outStringByteLength + 3) >= sizeofOutString)
            {
               *outStringPtr++ = '_';
               outStringByteLength++;
               inStringPtr += 3;
            }
            else
            {
               *outStringPtr++ = *inStringPtr++;
               *outStringPtr++ = *inStringPtr++;
               *outStringPtr++ = *inStringPtr++;
               outStringByteLength += 3;
            }
         }
      }
      else if ((*inStringPtr & UTF8_4_BYTE_MASK) == UTF8_4_BYTE_TAG)
      {  // four byte character
         // make sure we have all bytes
         if (*(inStringPtr + 1) == 0)
         {  // hit our null before satisfying the four bytes
            *outStringPtr++ = '_';
            inStringPtr++;
         }
         else if (*(inStringPtr + 2) == 0)
         {  // hit our null before satisfying the four bytes
            *outStringPtr++ = '_';
            inStringPtr += 2;
         }
         else if (*(inStringPtr + 3) == 0)
         {  // hit our null before satisfying the four bytes
            *outStringPtr++ = '_';
            inStringPtr += 3;
         }
         else
         {  // even if we did have all four bytes of the character,
            // we do not allow 4-byte characters to be used.
            *outStringPtr++ = '_';
            outStringByteLength++;
            inStringPtr += 4;
         }
      }
      else
      {  // single byte character
         switch (*inStringPtr)
         {
            // Legal characters
            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '0' ... '9':
            case '.':
            case '-':
            case '_':
            case ':':
            case ' ':
                *outStringPtr = *inStringPtr;
                break;

            default:
            #if(STRING_SANITIZER_MULTIBYTE_EXPANSION)
            // Illegal character:  Encode the ASCII character as a Latin
            // Unicode character (0x01nn) which Eliot will accept.
            if ((outStringByteLength + 2) >= sizeofOutString)
            {
                // Use an underscore if there are not at least two bytes available.
                *outStringPtr++ = '_';
            }
            else
            {
                // Unicode character:                 MSB                         LSB
                //      0x0100 Code page:   0  0  0  0  0  0  0  1      x  x  x  x  x  x  x  x
                //      ASCII characater:   x  x  x  x  x  x  x  x     b7 b6 b5 b4 b3 b2 b1 b0

                // UTF8 Output:                       MSB                         LSB
                //         UTF8 encoding:   1  1  0  x  x  x  x  x      1  0  x  x  x  x  x  x
                //      0x0100 Code page:   x  x  x  0  0  1  x  x      x  x  x  x  x  x  x  x
                //      ASCII characater:   x  x  x  x  x  x b7 b6      x  x b5 b4 b3 b2 b1 b0

                *outStringPtr++ = 0xC4 | (*inStringPtr >> 6);
                *outStringPtr = 0x80 | (*inStringPtr & 0x3F);

                outStringByteLength++;
                break;
            }
            #else
            // Convert illegal characters to underscores
            *outStringPtr = '_';
            #endif
         }

         outStringPtr++;
         inStringPtr++;
         outStringByteLength++;
      }
   }

   *outStringPtr = 0;

}

void SanitizeJsonStringObject(json_t* stringObject)
{  // takes a json string object and replaces its value with a sanitized string.
   char  newNameString[MAX_SIZE_NAME_STRING];

   SanitizeString(newNameString, (char *) json_string_value(stringObject), sizeof(newNameString));

   json_string_set(stringObject, newNameString);
}

char* TaskSanitizeString(char* stringptr)
{
   SanitizeString(sanitizedNameString, stringptr, sizeof(sanitizedNameString));

   return sanitizedNameString;
}

json_t * Sanitized_json_string(char * stringptr)
{  // takes a string and returns a sanitized json string object
   char  newNameString[MAX_SIZE_NAME_STRING];
   SanitizeString(newNameString, stringptr, sizeof(newNameString));
   return json_string(newNameString);
}

