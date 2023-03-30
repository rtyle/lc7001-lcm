#include "includes.h"

#ifndef AMDBUILD

LWGPIO_STRUCT TD_DC;
LWGPIO_STRUCT TD_DD;
LWGPIO_STRUCT TD_RST;

typedef enum
{
   TIMERS_OFF = 0x08,
   DMA_PAUSE = 0x04,
   TIMER_SUSPEND = 0x02,
   SELECT_FLASH_INFO_PAGE = 0x01,
   NO_WRITE_CONFIGURATION = 0x00
} cc1110_write_config;

typedef enum
{
   THIRTY_TWO_KB = 0,
   TWENTY_FOUR_KB = 1,
   SIXTEEN_KB = 2,
   EIGHT_KB = 3,
   FOUR_KB = 4,
   TWO_KB = 5,
   ONE_KB = 6,
   ZERO_BYTES = 7
} cc1110_lock_size;

typedef enum
{
   CHIP_ERASE_DONE = 0x80,
   PCON_IDLE = 0x40,
   CPU_HALTED = 0x20,
   POWER_MODE_0 = 0x10,
   HALT_STATUS = 0x08,
   DEBUG_LOCKED = 0x04,
   OSCILLATOR_STABLE = 0x02,
   STACK_OVERFLOW = 0x01
} cc1110_status;

#define FLASH_WORD_SIZE    2

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void DelayNanoseconds(int32_t delay)
{
   MQX_TICK_STRUCT initialTickCount;
   MQX_TICK_STRUCT currentTickCount;
   int32_t ticksDelta;
   int32_t nanosecondsElapsed = 0;
   bool overflowFlag;

   // Get the initial tick count
   _time_get_elapsed_ticks(&initialTickCount);

   while(nanosecondsElapsed < delay)
   {
      _time_get_elapsed_ticks(&currentTickCount);

      nanosecondsElapsed = _time_diff_nanoseconds(&currentTickCount, &initialTickCount, &overflowFlag);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool InitializeDebugPins()
{
   bool configured;

   configured = InitializeGPIOPoint(&TD_RST,
         (GPIO_PORT_C | GPIO_PIN2),
         LWGPIO_DIR_OUTPUT,
         LWGPIO_VALUE_NOCHANGE);

   configured &= InitializeGPIOPoint(&TD_DC,
         (GPIO_PORT_C | GPIO_PIN5),
         LWGPIO_DIR_OUTPUT,
         LWGPIO_VALUE_NOCHANGE);

   configured &= InitializeGPIOPoint(&TD_DD,
         (GPIO_PORT_C | GPIO_PIN6),
         LWGPIO_DIR_OUTPUT,
         LWGPIO_VALUE_NOCHANGE);

   if(configured)
   {
      // All pins were initialized.
      // Set the functionality and value
      lwgpio_set_functionality(&TD_RST, BSP_LED4_MUX_GPIO);
      lwgpio_set_value(&TD_RST, LWGPIO_VALUE_HIGH);

      lwgpio_set_functionality(&TD_DC, BSP_LED4_MUX_GPIO);
      lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);

      lwgpio_set_functionality(&TD_DD, BSP_LED4_MUX_GPIO);
      lwgpio_set_value(&TD_DD, LWGPIO_VALUE_LOW);
   }

   return configured;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void SendCC1110Byte(byte_t sendByte)
{
   // Configure the data pin to send data
   lwgpio_set_direction(&TD_DD, LWGPIO_DIR_OUTPUT);
   lwgpio_set_value(&TD_DD, LWGPIO_VALUE_LOW);

   byte_t bit;
   LWGPIO_VALUE bitValue;
   for(bit = 0x80; bit; bit >>= 1)
   {
      // Set the clock to high for the next clock period
      lwgpio_set_value(&TD_DC, LWGPIO_VALUE_HIGH);

      // Write out the bit on the data line
      bitValue = (sendByte & bit) ? LWGPIO_VALUE_HIGH : LWGPIO_VALUE_LOW;
      lwgpio_set_value(&TD_DD, bitValue);

      // Set the clock low so the CC1110 samples the data
      lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

byte_t GetCC1110Byte()
{
   byte_t readByte = 0;

   // Configure the data pin to receive data
   lwgpio_set_direction(&TD_DD, LWGPIO_DIR_INPUT);
   for(byte_t loop = 0; loop < 0xFF; loop++);
   //lwgpio_set_attribute(&TD_DD, LWGPIO_ATTR_PULL_UP, LWGPIO_AVAL_ENABLE);

   // Make sure the clock is low before starting
   // CC1110 Data sends data on the positive edge of the clock
   lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);
   
   byte_t bit;
   for(bit = 0x80; bit; bit >>= 1)
   {
      // Set the clock high so we can sample the data
      lwgpio_set_value(&TD_DC, LWGPIO_VALUE_HIGH);

      // Read in the bit from the data line
      if(lwgpio_get_value(&TD_DD) == LWGPIO_VALUE_HIGH)
      {
         readByte |= bit;
      }

      // Set the clock to low for the next clock period
      lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);
   }

   return readByte;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

byte_t SendDebugInstruction(byte_t *parameters, byte_t numParameters)
{
   byte_t returnByte = 0;

   switch(numParameters)
   {
      case 1:
         // Send the debug command header for 1 parameter
         SendCC1110Byte(0x55);
         
         // Send the parameter
         SendCC1110Byte(parameters[0]);

         // Read the return value
         returnByte = GetCC1110Byte();

         break;

      case 2:
         // Send the debug command header for 2 parameters
         SendCC1110Byte(0x56);
         
         // Send the parameters
         SendCC1110Byte(parameters[0]);
         SendCC1110Byte(parameters[1]);

         // Read the return value
         returnByte = GetCC1110Byte();

         break;

      case 3:
         // Send the debug command header for 3 parameters
         SendCC1110Byte(0x57);
         
         // Send the parameter
         SendCC1110Byte(parameters[0]);
         SendCC1110Byte(parameters[1]);
         SendCC1110Byte(parameters[2]);

         // Read the return value
         returnByte = GetCC1110Byte();

         break;

      default:
         returnByte = 0;
   }

   return returnByte;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ReadXData(word_t address, byte_t *data, word_t numBytes)
{
   byte_t parameters[3];

   // MOV DPTR, address
   parameters[0] = 0x90;                     // MOV DPTR
   parameters[1] = HighByteOfWord(address);  // High Byte of Address
   parameters[2] = LowByteOfWord(address);   // Low Byte of Address
   SendDebugInstruction(parameters, 3);

   // Read each byte returned into the array
   for(word_t i = 0; i < numBytes; i++)
   {
      // MOVX A, @DPTR
      parameters[0] = 0xE0;
      *(data + i) = SendDebugInstruction(parameters, 1);

      // INC DPTR
      parameters[0] = 0xA3;
      SendDebugInstruction(parameters, 1);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void WriteXData(word_t address, byte_t *data, word_t numBytes)
{
   byte_t parameters[3];

   // MOV DPTR, address
   parameters[0] = 0x90;                     // MOV DPTR
   parameters[1] = HighByteOfWord(address);  // High Byte of Address
   parameters[2] = LowByteOfWord(address);   // Low Byte of Address
   SendDebugInstruction(parameters, 3);

   // Write the data array to memory
   for(word_t i = 0; i < numBytes; i++)
   {
      // MOV A, data[n]
      parameters[0] = 0x74;
      parameters[1] = *(data + i);
      SendDebugInstruction(parameters, 2);

      // MOVX @DPTR, A
      parameters[0] = 0xF0;
      SendDebugInstruction(parameters, 1);

      // INC DPTR
      parameters[0] = 0xA3;
      SendDebugInstruction(parameters, 1);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void WriteProgramRoutine(word_t address, word_t programAddress, byte_t programSizeBytes)
{
   byte_t programBytes[34];
   word_t adjusted;
   word_t numWords;
   byte_t offset = 0;

   // MOV FADDRH, #imm  (FADDRH bits 5:1 determine the page to use)
   adjusted = programAddress / FLASH_WORD_SIZE;
   programBytes[0] = 0x75;
   programBytes[1] = 0xAD;
   programBytes[2] = HighByteOfWord(adjusted);

   // MOV FADDRL, #00
   programBytes[3] = 0x75;
   programBytes[4] = 0xAC;
   programBytes[5] = LowByteOfWord(adjusted);

   // Initialize the data pointer
   // MOV DPTR, #0f000H
   programBytes[6] = 0x90;
   programBytes[7] = 0xF0;
   programBytes[8] = 0x00;

   // MOV R7, #imm
   numWords = programSizeBytes / FLASH_WORD_SIZE;
   if(numWords > 0xFF)
   {
      programBytes[9] = 0x7F;
      programBytes[10] = HighByteOfWord(numWords);
      offset = 2;
   }

   // MOV R6, #imm
   programBytes[9+offset] = 0x7E;
   programBytes[10+offset] = LowByteOfWord(numWords);

   // MOV FLC, #02H
   programBytes[11+offset] = 0x75;
   programBytes[12+offset] = 0xAE;
   programBytes[13+offset] = 0x02;

   // writeLoop; MOV R5, #imm
   programBytes[14+offset] = 0x7D;
   programBytes[15+offset] = FLASH_WORD_SIZE;

   // writeWordLoop; Move A, @DPTR
   programBytes[16+offset] = 0xE0;

   // INC DPTR
   programBytes[17+offset] = 0xA3;

   // MOV FWDATA, A
   programBytes[18+offset] = 0xF5;
   programBytes[19+offset] = 0xAF;

   // DJNZ R5, writeWordLoop (Wait for completion)
   programBytes[20+offset] = 0xDD;
   programBytes[21+offset] = 0xFA;

   // writeWaitLoop; MOV A, FLC
   programBytes[22+offset] = 0xE5;
   programBytes[23+offset] = 0xAE;

   // JB ACC_SWBSY, writeWaitLoop
   programBytes[24+offset] = 0x20;
   programBytes[25+offset] = 0xE6;
   programBytes[26+offset] = 0xFB;

   // DJNZ R6, writeLoop
   programBytes[27+offset] = 0xDE;
   programBytes[28+offset] = 0xF1;

   if(numWords > 0xFF)
   {
      // DJNZ R7, writeLoop
      programBytes[29+offset] = 0xDF;
      programBytes[30+offset] = 0xEF;
      offset = 4;
   }

   // Done, fake a breakpoint
   programBytes[29+offset] = 0xA5;

   WriteXData(address, programBytes, 30+offset);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_ResumePC(word_t address)
{
   // Sets the program counter to the address provided
   byte_t parameters[3];
   parameters[0] = 0x02;                     // Set PC instruction
   parameters[1] = HighByteOfWord(address);  // Address high byte
   parameters[2] = LowByteOfWord(address);   // Address low byte
   SendDebugInstruction(parameters, 3);

   // Send the resume command
   SendCC1110Byte(0x4C);

   // Get and discard the return value
   GetCC1110Byte();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_EnterDebugMode()
{
   // Debug mode is entered by forcing two rising edge transitions on
   // the clock while the reset line is held low
   // Set the reset line to low
   lwgpio_set_value(&TD_RST, LWGPIO_VALUE_LOW);
   DelayNanoseconds(2000000);

   // Force Two rising edge transitions on the clock line
   lwgpio_set_value(&TD_DC, LWGPIO_VALUE_HIGH);
   lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);
   lwgpio_set_value(&TD_DC, LWGPIO_VALUE_HIGH);
   lwgpio_set_value(&TD_DC, LWGPIO_VALUE_LOW);
   DelayNanoseconds(2000000);

   // Bring the chip out of reset
   lwgpio_set_value(&TD_RST, LWGPIO_VALUE_HIGH);
   DelayNanoseconds(2000000);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_ExitDebugMode()
{
   // Sets the program counter to 0 and resumes execution
   CC1110_ResumePC(0x00);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

byte_t CC1110_ReadStatus()
{
   // Send the command to get the chip status
   SendCC1110Byte(0x34);

   // Read the status byte
   return GetCC1110Byte();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_ChipErase()
{
   byte_t parameter[1];
   byte_t status;

   // Send an initial NOP command to ensure the status byte has been updated
   parameter[0] = 0x00;
   SendDebugInstruction(parameter, 1);

   // Send the command to erase flash memory
   SendCC1110Byte(0x14);

   // Get and discard the return value
   GetCC1110Byte();

   // Wait until the chip erase completes
   do
   {
      status = CC1110_ReadStatus();
   } while(!(status & CHIP_ERASE_DONE));
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_GetChipID(byte_t *chipId, byte_t *revisionNumber)
{
   // Send the command to get the chip ID (0x68)
   SendCC1110Byte(0x68);

   // Get the chip ID
   *chipId = GetCC1110Byte();

   // Get the revision number
   *revisionNumber = GetCC1110Byte();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_WriteConfig(byte_t configByte)
{
   // Write the debug configuration command byte
   SendCC1110Byte(0x1D);

   // Send the configuration data
   SendCC1110Byte(configByte);

   // Read and discard the return value
   GetCC1110Byte();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void CC1110_ReadFlashInformation(bool *bootBlockWritable, bool *debugEnabled, cc1110_lock_size *lockSize)
{
   byte_t parameter[1];
   byte_t status[1];

   // Send an initial NOP command to ensure the status byte has been updated
   parameter[0] = 0x00;
   SendDebugInstruction(parameter, 1);

   // Select the flash information page by writing the config bits to do so
   CC1110_WriteConfig(SELECT_FLASH_INFO_PAGE);

   // Read the bits from the flash information page.
   // The lock bits appear at address 0x00 in the flash information page.
   ReadXData(0x00, status, 1);

   // Return the config to the default
   CC1110_WriteConfig(NO_WRITE_CONFIGURATION);

   // Set the return values based on the status byte read
   *bootBlockWritable = (status[0] & 0x10);
   *debugEnabled = (status[0] & 0x01);
   *lockSize = ((status[0] & 0x0E) >> 1);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool CC1110_WriteFlashMemory(word_t address, byte_t *data, word_t numBytes)
{
   bool error = FALSE;
   byte_t parameters[3];
   byte_t status;

   // The number of bytes to write and the address must be even numbers since
   // the data is written in 2-byte increments
   if(((numBytes % 2) == 0) &&
      ((address % 2) == 0))
   {
      // Write the input data to the XDATA section
      WriteXData(0xF000, data, numBytes);

      // Write the program to the XDATA section that will load the
      // program into code space
      WriteProgramRoutine(0xF400, address, numBytes);

      // MOV MEMCTR, (bank * 16) + 1
      parameters[0] = 0x75;   // MOV
      parameters[1] = 0xC7;   // MEMCTR
      parameters[2] = 0x01;   // (bank * 16) + 1   bank always 0 for cc1110
      SendDebugInstruction(parameters, 3);

      // Resume the PC at the Program Routine Address
      CC1110_ResumePC(0xF400);

      // Wait until the status says the CPU halted
      do
      {
         status = CC1110_ReadStatus();
      } while(!(status & CPU_HALTED));
   }
   else
   {
      error = TRUE;
   }

   return !error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void CC1110_ReadFlashMemory(word_t address, byte_t *data, word_t numBytes)
{
   byte_t parameters[3];

   //MOV MEMCTR, (bank * 16) + 1
   parameters[0] = 0x75;   // MOV
   parameters[1] = 0xC7;   // MEMCTR
   parameters[2] = 0x01;   // (bank * 16) + 1   bank always 0 for cc1110
   SendDebugInstruction(parameters, 3);

   // MOV DPTR, address
   parameters[0] = 0x90;                     // MOV DPTR
   parameters[1] = HighByteOfWord(address);  // Address high byte
   parameters[2] = LowByteOfWord(address);   // Address low byte
   SendDebugInstruction(parameters, 3);

   // Read each byte into the array
   for(word_t i = 0; i < numBytes; i++)
   {
      // CLR A
      parameters[0] = 0xE4;
      SendDebugInstruction(parameters, 1);

      // MOVC A, @A+DPTR
      parameters[0] = 0x93;
      *(data + i) = SendDebugInstruction(parameters, 1);

      // INC DPTR
      parameters[0] = 0xA3;
      SendDebugInstruction(parameters, 1);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool CC1110_EraseFlashPage(word_t address)
{
   bool erased = FALSE;
   byte_t page;
   bool bootBlockWritable = FALSE;
   bool debugEnabled = FALSE;
   cc1110_lock_size lockSize = 0;

   // FADDRH bits 5:1 determine the page to use
   page = ((address >> 8) / FLASH_WORD_SIZE) & 0x7E;

   // Determine if the page could be erased
   CC1110_ReadFlashInformation(&bootBlockWritable, &debugEnabled, &lockSize);

   if(page == 0 && bootBlockWritable == FALSE)
   {
      // The boot block (page 0) is not writable
      erased = FALSE;
   }
   else if(lockSize == THIRTY_TWO_KB)
   {
      // All pages are protected
      erased = FALSE;
   }
   else if((lockSize == TWENTY_FOUR_KB) &&
           (page > 15))
   {
      // The upper 24 KB are protected and the page falls in that region
      erased = FALSE;
   }
   else if((lockSize == SIXTEEN_KB) &&
           (page > 31))
   {
      // The upper 16 KB are protected and the page falls in that region
      erased = FALSE;
   }
   else if((lockSize == EIGHT_KB) &&
           (page > 47))
   {
      // The upper 8 KB are protected and the page falls in that region
      erased = FALSE;
   }
   else if((lockSize == FOUR_KB) &&
           (page > 55))
   {
      // The upper 4 KB are protected and the page falls in that region
      erased = FALSE;
   }
   else if((lockSize == TWO_KB) &&
           (page > 59))
   {
      // The upper 2 KB are protected and the page falls in that region
      erased = FALSE;
   }
   else if((lockSize == ONE_KB) &&
           (page > 62))
   {
      // The last page is protected and the page falls in that region
      erased = FALSE;
   }
   else
   {
      // Create the routine that will erase the bytes
      byte_t routine[16];
      byte_t status;

      // MOV FADDRH, #imm
      routine[0] = 0x75;
      routine[1] = 0xAD;
      routine[2] = page;

      // MOV FADDRL, #00
      routine[3] = 0x75;
      routine[4] = 0xAC;
      routine[5] = 0x00;

      // MOV FLC, #01H    (Erase)
      routine[6] = 0x75;
      routine[7] = 0xAE;
      routine[8] = 0x01;

      // NOP. According to the data sheet, a nop must follow the erase instr
      routine[9] = 0x00;

      // MOV A, FLC
      routine[10] = 0xE5;
      routine[11] = 0xAE;

      // JB ACC_BUSY. Wait until the erase is complete
      routine[12] = 0x20;
      routine[13] = 0xE7;
      routine[14] = 0xFB;

      // Set a breakpoint so we know when the program is finished
      routine[15] = 0xA5;

      // Write the erase routine to RAM
      WriteXData(0xF000, routine, 16);

      // Resume the program counter at the RAM address of the code
      // that erases the flash page
      CC1110_ResumePC(0xF000);

      // Wait until the breakpoint is hit. Once the breakpoint is hit,
      // the status will be CPU_HALTED (0x20)
      do
      {
         status = CC1110_ReadStatus();
      } while(!(status & CPU_HALTED));

      erased = TRUE;
   }

   return erased;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool CC1110_Program()
{
   bool error = FALSE;
   byte_t chipId = 0;
   byte_t revisionNumber = 0;

   // Write data to a flash page
   byte_t programData[128];
   byte_t readData[128];
   byte_t i = 0;
   for(i = 0; i < sizeof(programData); i++)
   {
      programData[i] = i;
   }

   // Set up the GPIO pins for SPI communication
   if(InitializeDebugPins())
   {
      // Enter Debug Mode
      CC1110_EnterDebugMode();

      // Get the chip ID and revision number
      CC1110_GetChipID(&chipId, &revisionNumber);

      // Erase a flash page
      CC1110_EraseFlashPage(0x7800);

      // Write data to that flash page
      CC1110_WriteFlashMemory(0x7800, programData, sizeof(programData));

      // Read data from the flash page
      CC1110_ReadFlashMemory(0x7800, readData, sizeof(readData));

      // Get out of debug mode
      CC1110_ExitDebugMode();
   }
   else
   {
      error = TRUE;
   }

   return !error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#endif
