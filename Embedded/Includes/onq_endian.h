/*
 * endian.h
 *
 *  Created on: Jun 13, 2013
 *      Author: Legrand
 */

#ifndef ENDIAN_H_
#define ENDIAN_H_

//-----------------------------------------------------------------------------

// Big Endian
//#define HighByteOfWord(WordVariable)            *((byte_t *) &WordVariable)
//#define LowByteOfWord(WordVariable)             *(((byte_t *) &WordVariable) + 1)
//
//#define HighByteOfPtr(PtrVariable)              *((byte_t *) &PtrVariable)
//#define HighWordOfDword(DwordVariable)          *((word_t *) &DwordVariable)
//#define MiddleWordOfDword(DwordVariable)        *((word_t *) (((byte *) &DwordVariable) + 1))
//#define LowWordOfDword(DwordVariable)           *(((word_t *) &DwordVariable) + 1)

//-----------------------------------------------------------------------------

// Little Endian
#define LowByteOfWord(WordVariable)             *((byte_t *) &WordVariable)
#define HighByteOfWord(WordVariable)            *(((byte_t *) &WordVariable) + 1)

#define LowWordOfDword(DwordVariable)           *((word_t *) &DwordVariable)
#define HighWordOfDword(DwordVariable)          *(((word_t *) &DwordVariable) + 1)

//-----------------------------------------------------------------------------

#define HighByteOfDword(DwordVariable)          HighByteOfWord(HighWordOfDword(DwordVariable))
#define UpperMiddleByteOfDword(DwordVariable)   LowByteOfWord(HighWordOfDword(DwordVariable))
#define LowerMiddleByteOfDword(DwordVariable)   HighByteOfWord(LowWordOfDword(DwordVariable))
#define LowByteOfDword(DwordVariable)           LowByteOfWord(LowWordOfDword(DwordVariable))

#endif /* ENDIAN_H_ */
