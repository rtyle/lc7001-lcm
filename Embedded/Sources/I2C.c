//-----------------------------------------------------------------------------
// I2C.c
//-----------------------------------------------------------------------------

#include "includes.h"

LWGPIO_STRUCT I2CSCL;
LWGPIO_STRUCT I2CSDA;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define HC908_I2C_WRITE    lwgpio_set_direction(&I2CSDA, LWGPIO_DIR_OUTPUT)
#define HC908_I2C_READ     lwgpio_set_direction(&I2CSDA, LWGPIO_DIR_INPUT)
#ifndef K64
#define I2C_DELAY_NEEDED
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void I2C_Initialize(void)
{
   bool_t OutputPort;

   OutputPort = InitializeGPIOPoint(&I2CSCL,
                                     (GPIO_PORT_B | GPIO_PIN2),
                                      LWGPIO_DIR_OUTPUT,
                                       LWGPIO_ATTR_PULL_UP);
   if (!OutputPort)
   {
//      printf("Initializing LWGPIO for I2CSCL failed.\n");
   }
   lwgpio_set_functionality(&I2CSCL, BSP_LED1_MUX_GPIO);

   OutputPort = InitializeGPIOPoint(&I2CSDA,
                                     (GPIO_PORT_B | GPIO_PIN3),
                                      LWGPIO_DIR_OUTPUT,
                                       LWGPIO_ATTR_PULL_UP);
   if (!OutputPort)
   {
//      printf("Initializing LWGPIO for I2CSDA failed.\n");
   }
   lwgpio_set_functionality(&I2CSDA, BSP_LED1_MUX_GPIO);

   HC908_I2C_WRITE;

//   HC908_I2C_REG_CLOCK  = 1;
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);
//   HC908_I2C_REG_DATA   = 1;
   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_HIGH);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void I2C_StartBit(void)
{

   ////////////////////////////////////////////////////////////////////////////
   // MTM :
   // since SDA is left in the '1' state by the stop bit,
   // we need to bring it to '0' to signify a start bit.
   // Note that SCL remains in the '1' state.
   ////////////////////////////////////////////////////////////////////////////

   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_LOW);

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_LOW);

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t I2C_SendByte(byte_t outByte)
{
   byte_t bitMask;
   byte_t isSuccessful;
   LWGPIO_VALUE inputBit;

   ////////////////////////////////////////////////////////////////////////////
   // DAK : Taken from MTM code-base i2c.c
   //       Modified for style and abstraction
   ////////////////////////////////////////////////////////////////////////////

   bitMask = 0x80;

   while (bitMask != 0)
   {
      // Test bit
      if ((outByte & bitMask) != 0)
      {
         // Set data high
         lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_HIGH);
      }
      else
      {
         // Set data low
         lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_LOW);
      }

#ifdef I2C_DELAY_NEEDED
      _time_delay(1);
#endif

      // Pull control line high to clock bit
      lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);

#ifdef I2C_DELAY_NEEDED
      _time_delay(1);
#endif

      // Pull control line low for next bit
      lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_LOW);

      // Shift over to next bit
      bitMask >>= 1;
   }

   // Wait for acknowledgement

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   // Switch to reading
   HC908_I2C_READ;

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   // The data line should have been pulled low if acknowledged

   inputBit = lwgpio_get_value(&I2CSDA);
   if (inputBit != LWGPIO_VALUE_LOW)
   {
      isSuccessful = false;
   }
   else
   {
      isSuccessful = true;
   }

   // Clock in a bit
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   // Switch back to writing
   HC908_I2C_WRITE;

   // Prepare for stop bit or next byte
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_LOW);

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   return isSuccessful;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t I2C_ReceiveByte(void)
{
   byte_t inByte;
   byte_t whichBit;
   LWGPIO_VALUE inputBit;

   ////////////////////////////////////////////////////////////////////////////
   // DAK : Taken from MTM code-base i2c.c
   //       Modified for style and abstraction
   ////////////////////////////////////////////////////////////////////////////

   // Make sure we're in reading mode
   HC908_I2C_READ;

   whichBit = 8;
   inByte   = 0;

   while (whichBit != 0)
   {
      // Pull control line high
      lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
      _time_delay(1);
#endif

      // Prepare incoming byte by shifting to proper position
      inByte <<= 1;

      // Decrement our bit number to give the data line some time to settle
      --whichBit;

      // Add bit to the result
      inputBit = lwgpio_get_value(&I2CSDA);
      if (inputBit == LWGPIO_VALUE_HIGH)
      {
         inByte |= 0x01;
      }

      // Prepare for next bit
      lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_LOW);

      // Wait
#ifdef I2C_DELAY_NEEDED
      _time_delay(1);
#endif
   }

   // Switch to writing
   HC908_I2C_WRITE;
   // Send an not-acknowledgement bit

   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_HIGH);

#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif
   
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   return inByte;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void I2C_StopBit(void)
{
   ////////////////////////////////////////////////////////////////////////////
   // DAK : Taken from MTM code-base i2c.c
   //       Modified for style and abstraction
   ////////////////////////////////////////////////////////////////////////////

   // Pull down the data line so we can transition it when the clock goes high
   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_LOW);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   // Pull control line high to begin stop bit sequence
   lwgpio_set_value(&I2CSCL, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif

   // Data bit transition indicates stop bit
   lwgpio_set_value(&I2CSDA, LWGPIO_VALUE_HIGH);
#ifdef I2C_DELAY_NEEDED
   _time_delay(1);
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef USING_I2C_DEVICE_DRIVERS
struct STRU_I2C_BUFFER
{
   char dst_addr;
   char reg_addr;
   char data[100]; // Here is the buffer for sending or 
                // receiving data
};

struct STRU_I2C_BUFFER i2c_buf_rx;
struct STRU_I2C_BUFFER i2c_buf_tx;
   // I never got this to work - I kept it in case we ever investigate further - Matt
   file_iic0 = fopen("ii2c0:", NULL);
   if (file_iic0 == NULL)
   {
      printf("\nOpen the IIC0 driver failed!!!\n");
      return;
   }

//    param = 100000;
   if (I2C_OK == ioctl (file_iic0, IO_IOCTL_I2C_SET_BAUD, &param))
   {
      printf ("OK\n");
   }
   else
   {
      printf ("ERROR\n");
   }

   printf ("Set master mode ... ");
   if (I2C_OK == ioctl (file_iic0, IO_IOCTL_I2C_SET_MASTER_MODE, NULL))
   {
      printf ("OK\n");
   }
   else
   {
      printf ("ERROR\n");
   }

   param = 0x60;
   printf ("Set station address to 0x%02x ... ", param);
   if (I2C_OK == ioctl (file_iic0, IO_IOCTL_I2C_SET_STATION_ADDRESS, &param))
   {
      printf ("OK\n");
   }
   else
   {
      printf ("ERROR\n");
   }

   param = 0x50;
   printf ("Set destination address to 0x%02x ... ", param);
   if (I2C_OK == ioctl (file_iic0, IO_IOCTL_I2C_SET_DESTINATION_ADDRESS, &param))
   {
      printf ("OK\n");
   }
   else
   {
      printf ("ERROR\n");
   }
    
#if 0
   buffer[0] = 0;
   buffer[1] = 0;
   len = fwrite (buffer, 1, 1, file_iic0);
   if (len < 1)
   {
      printf("send failed, len = %d \n", len);
   }
   else
   {
      printf("send ok, len = %d \n", len);
   }
#endif
    
   i2c_buf_tx.dst_addr = 0xA0; // The I2C slave address 
   i2c_buf_tx.reg_addr = 0;    // You may view this as a register 
                               // address or a command to slave.
   i2c_buf_rx.dst_addr = 0xA0; // The same as above, for receiving 
 
   i2c_buf_rx.reg_addr = 0;    // The same as above, for receiving 
  

   i2c_buf_tx.data[0] = 0x00;
    
   i2c_ptr = _bsp_get_i2c_base_address (0);
    
   len = fwrite(&i2c_buf_tx, 1, 2, file_iic0);
   if (len < 1)
   {
      printf("send failed, len = %d \n", len);
   }
   else
   {
      printf("send ok, len = %d \n", len);
   }

#if 0
   while(1)
   {
      printf("---------------------\n");
      // send 4 bytes to slave
      len = fwrite(&i2c_buf_tx, 1, 4, file_iic0);
      if( len < 4 )
         printf("send failed, len = %d \n", len);
      else
         printf("send ok, len = %d \n", len);

      // clear receiving buffer
      memset(i2c_buf_rx.data, 0, 4);
      // receive 4 bytes from slave
      len = fread(&i2c_buf_rx, 1, 4, file_iic0);
      printf("get i2c data, len = %x\n", len);
      for(i=0; i<len; i++)
         printf("%x \n", i2c_buf_rx.data[i]);
      _time_delay(10);
   }
#endif
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

