#ifndef I2C_H
#define I2C_H

void I2C_Initialize(void);

void I2C_StartBit(void);

byte_t I2C_SendByte(byte_t outByte);

byte_t I2C_ReceiveByte(void);

void I2C_StopBit(void);

#endif
