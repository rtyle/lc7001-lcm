#ifndef _CRC_H_
#define _CRC_H_

byte_t UpdateCRC8(byte_t currentCRC, byte_t *buffer, dword_t size);
word_t UpdateCRC16(word_t currentCRC, byte_t *buffer, dword_t size);

#endif
