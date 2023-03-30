#include "includes.h"
#include "swap_demo.h"

#define ZONE_INDEX_OFFSET              0x28
#define ELIOT_PROPERTIES_OFFSET        0x30
#define SCENE_INDEX_OFFSET             0x1C0
#define SCENE_CONTROLLER_INDEX_OFFSET  0x48

FBK_Context eliot_fbk;

/* Flash driver structure */
FLASH_SSD_CONFIG ftfl_cfg = { FTFx_REG_BASE, /* FTFx control register base */
      PFLASH_BLOCK_BASE, /* base address of PFlash block */
      PBLOCK_SIZE, /* size of PFlash block */
      DEFLASH_BLOCK_BASE, /* base address of DFlash block */
      0, /* size of DFlash block */
      EERAM_BLOCK_BASE, /* base address of EERAM block */
      EERAM_BLOCK_SIZE, /* size of EERAM block */
      0, /* size of EEE block */
      DEBUGENABLE, /* background debug mode enable bit */
      NULL_CALLBACK /* pointer to callback function */
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// NON-VOLATILE MEMORY UPPER FLASH BLOCK (0xdb000 - 0xfefff)
//----------------------------------------------------------
// Eliot bank 0 (0xdb000 - 0xdbfff)
//  0xdb000 - 0xdb5ff               Refresh token
//  0xdb600 - 0xdb63f               Primary key
//  0xdb640 - 0xdb66f               Plant ID
//  0xdb670 - 0xdb681               JSON socket user key + checksum
//  0xdb682 - 0xdbff7               Unused
//  0xdbff8 - 0xdbfff               Block info
//----------------------------------------------------------
// Eliot bank 1 (0xdc000 - 0xdcfff)
//  (structure of 0xdb000 - 0xdbfff is mirrored here)
//----------------------------------------------------------
// Zone Array (0xdd000 - 0xddfff)
//  0xdd000 - 0xdd007               Zone[0] CRC
//  0xdd008 - 0xdd027               Zone[0]
//  0xdd028 - 0xdd030               Zone[1] CRC
//  0xdd031 - 0xdd04f               Zone[1]
//  0xdd050 - 0xdd057               Zone[2] CRC
//  0xdd058 - 0xdd077               Zone[2]
//  ...
//  ...
//  0xddf78 - 0xddf80               Zone[99] CRC
//  0xddf81 - 0xddf9f               Zone[99]
//  0xddfa0 - 0xddfff               unused
//----------------------------------------------------------
// Scene Block 0 [Scenes 0-8]
//  0xde000 - 0xde007               Scene[0] CRC
//  0xde008 - 0xde1bf               Scene[0]
//  0xde1c0 - 0xde1c7               Scene[1] CRC
//  0xde1c8 - 0xde37f               Scene[1]
//  ...
//  ...
//  0xdee00 - 0xdee07               Scene[8] CRC
//  0xde008 - 0xdefbf               Scene[8]
//  0xdefc0 - 0xdefff               unused
//----------------------------------------------------------
//  0xdf000 - 0xdffff               Scene Block 1 [Scenes 9-17]
//----------------------------------------------------------
//  0xe0000 - 0xe0fff               Scene Block 2 [Scenes 18-26]
//----------------------------------------------------------
//  0xe1000 - 0xe1fff               Scene Block 3 [Scenes 27-35]
//----------------------------------------------------------
//  0xe2000 - 0xe2fff               Scene Block 4 [Scenes 36-44]
//----------------------------------------------------------
//  0xe3000 - 0xe3fff               Scene Block 5 [Scenes 45-53]
//----------------------------------------------------------
//  0xe4000 - 0xe4fff               Scene Block 6 [Scenes 54-62]
//----------------------------------------------------------
//  0xe5000 - 0xe5fff               Scene Block 7 [Scenes 63-71]
//----------------------------------------------------------
//  0xe6000 - 0xe6fff               Scene Block 8 [Scenes 72-80]
//----------------------------------------------------------
//  0xe7000 - 0xe7fff               Scene Block 9 [Scenes 81-89]
//----------------------------------------------------------
//  0xe8000 - 0xe8fff               Scene Block 10 [Scenes 90-98]
//----------------------------------------------------------
//  0xe9000 - 0xe937f               Scene Block 10 [Scenes 99-100]
//----------------------------------------------------------
//  0xe937f - 0xe9fff               unused
//----------------------------------------------------------
// Scene Controller Array (0xea000 - 0xeafff)
//  0xea000 - 0xea007               SC[0] CRC
//  0xea008 - 0xea047               SC[0]
//  0xea048 - 0xea04f               SC[1] CRC
//  0xea050 - 0xea08f               SC[1]
//  0xea090 - 0xea097               SC[2] CRC
//  0xea098 - 0xea0d7               SC[2]
//  ...
//  ...
//  0xea558 - 0xea55f               SC[19] CRC
//  0xea560 - 0xea59f               SC[19]
//  0xea5a0 - 0xeafff               unused
//----------------------------------------------------------
//  Samsung Zone Device IDs [0-49] (0xeb000 - 0xebfff)
//  0xeb000 - 0xeb007               Zone[0] CRC
//  0xeb008 - 0xeb02F               Zone[0]
//  0xeb030 - 0xeb037               Zone[1] CRC
//  0xeb038 - 0xeb05f               Zone[1]
//  0xeb060 - 0xeb067               Zone[2] CRC
//  0xeb068 - 0xeb08F               Zone[2]
//  ...
//  ...
//  0xeb930 - 0xeb937               Zone[49] CRC
//  0xeb938 - 0xeb95f               Zone[49]
//  0xeb960 - 0xebfff               unused
//----------------------------------------------------------
//  Samsung Zone Device IDs [50-99] (0xec000 - 0xecfff)
//----------------------------------------------------------
//  Samsung Scene IDs [0-49]        (0xed000 - 0xedfff)
//  0xed000 - 0xeb007               Scene[0] CRC
//  0xed008 - 0xeb02F               Scene[0]
//  0xed030 - 0xeb037               Scene[1] CRC
//  0xed038 - 0xeb05f               Scene[1]
//  0xed060 - 0xeb067               Scene[2] CRC
//  0xed068 - 0xeb08F               Scene[2]
//  ...
//  ...
//  0xed930 - 0xeb937               Scene[49] CRC
//  0xed938 - 0xeb95f               Scene[49]
//  0xed960 - 0xebfff               unused
//----------------------------------------------------------
//  Samsung Scene IDs [50-99]       (0xee000 - 0xeefff)
//----------------------------------------------------------
//  0xef000 - 0xeffff               shade id [0-99]
//----------------------------------------------------------
//  0xf0000 - 0xfdfff               unused
//----------------------------------------------------------
//  0xfe000 - 0xfe002               CANARY_BYTE_STRING
//----------------------------------------------------------
//  0xfe003 - 0xfe003               Flash block index - used to determine which block is most recently saved.
//----------------------------------------------------------
//  0xfe004 - 0xfe006               MAC address bytes
//----------------------------------------------------------
//  0xfe007 - 0xfe007               unused
//----------------------------------------------------------
//  0xfe008 - 0xfe008 + flash_sizeof(systemProperties)      System Properties structure
//----------------------------------------------------------
//  0xfe008 +  flash_sizeof(systemProperties) - 0xfefef     unused
//----------------------------------------------------------
//  0xfeff0 - 0xfeff1               flash block crc
//----------------------------------------------------------
//  0xfeff2 - 0xfefff               unused
//----------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// flash_sizeof() 
//    Used to "round-up" the actual storage space to allow the structures to 
//    fill 8-byte segments (the smallest block allowed to be written to flash).
//-----------------------------------------------------------------------------

word_t flash_sizeof(word_t originalSize)
{
   word_t flashSize;

   flashSize = originalSize >> 3;
   if (originalSize & 0x0007)
   {
      // Add 1 to capture any partial longwords
      flashSize++;
   }

   // Multiply by 8 to get back to the actual number of bytes
   // Now 8-byte segment aligned
   flashSize <<= 3;

   return flashSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void InitializeFlash()
{
   uint32_t returnCode;
   uint8_t CurrentSwapMode = 0;
   uint8_t CurrentSwapBlockStatus = 0;
   uint8_t NextSwapBlockStatus = 0;
   uint8_t blockIndex;
   uchar * pFlashMemory;

   if (((FTFE_FCNFG >> 3) & 0x01) == 0x0)
   {
      printf("Program Flash Block 0 located at address 0x0000\n");
      printf("Executing from Program Flash Block 0\n\n");
   }
   else
   {
      printf("Program Flash Block 1 located at address 0x0000\n");
      printf("Executing from Program Flash Block 1\n\n");
   }

   /* Initialize flash driver */
   printf("Initializing Flash Driver: ");
   returnCode = pFlashInit(&ftfl_cfg);
   if (FTFx_OK == returnCode)
   {
      printf("Success!\r\n");
   }
   else
   {
      printf("Failed!\r\n");
   }

   /* Get Status of Swap System */
   printf("Checking Swap System Status: ");
   returnCode = pPFlashGetSwapStatus(&ftfl_cfg, FLASH_SWAP_INDICATOR_ADDR, &CurrentSwapMode, &CurrentSwapBlockStatus, &NextSwapBlockStatus, FlashCommandSequence);
   if (FTFx_OK == returnCode)
   {
      printf("Success!\r\n");
   }
   else
   {
      /* could have mgstat error if swap system had to resolve corrupted swap indicator */
      if ((returnCode & FTFx_ERR_MGSTAT0) == returnCode)
      {
         printf("/******** Possible corruption of flash swap indicators caused by reset during swap command****/\n");
         printf("/******** Perform another swap to clean up ***************************************************/\n");

      }
      else
      {
         printf("Failed!\r\n");
         //         ErrorTrap(returnCode);  
      }
   }

   switch (CurrentSwapMode)
   {
      case FTFx_SWAP_UNINIT:
         /* Before you initialize the swap system you'll be in this mode */
         printf("Swap Mode: Uninitialized\n");
         break;
      case FTFx_SWAP_READY:
         /* Typical mode you should be in if no interruptions to firmware update process*/
         printf("Swap Mode: Ready\n");
         break;
      case FTFx_SWAP_UPDATE:
         /* You might be in this mode if a reset interrupted your firmware update process */
         printf("Swap Mode: Update\n");
         break;
      case FTFx_SWAP_UPDATE_ERASED:
         /* You might be in this mode if a reset interrupted your firmware update process */
         printf("Swap Mode: Update Erased\n");
         break;
      case FTFx_SWAP_COMPLETE:
         /* Should never be in this mode after reset */
         printf("Swap Mode: Complete\n");
         break;
   }

   if (CurrentSwapMode == FTFx_SWAP_UPDATE || CurrentSwapMode == FTFx_SWAP_UPDATE_ERASED)
   {
      printf("/******** Reset during firmware update process may have occurred ********/\n");
      printf("/******** Run the firmware update (swap) process again ******************/\n");
   }

   // determine which block has the data we should load
   blockIndex = WhichFlashBlockToUse();
   if (blockIndex == 1)
   {  
      // use the low block
      pFlashMemory = (unsigned char *) FLASH_LOWER_BLOCK(SYSTEM_PROPERTIES_FLASH_ADDRESS);
      pFlashMemory += 3;
      
      // since we're going to be updating to the high block, be sure to 
      // increment the current index from the lower block so it will take 
      // precedence on the next startup.
      flashBlockIndexForNextWrite = (*pFlashMemory) + 1;
   }
   else if (blockIndex == 2)
   {  
      // use the high block
      pFlashMemory = (unsigned char *) SYSTEM_PROPERTIES_FLASH_ADDRESS;
      pFlashMemory += 3;

      // since we're already using the section we can write to, 
      // no need to increment the index.
      flashBlockIndexForNextWrite = *pFlashMemory;
   }
   else
   {  
      // no valid block, so leave everything as defaults
      flashBlockIndexForNextWrite = 1;
      return;
   }

   // If it was determined that data was used from the the low block,
   // it means that it was left over from a recent flash swap.  
   // Copy everything to the upper block for normal use.
   if (blockIndex == 1)
   {
      // Disable interrupts while writing to flash
      _int_disable();

      returnCode = FlashEraseSector(&ftfl_cfg, LCM_DATA_STARTING_BLOCK, (FLASH_UPPER_BLOCK_END_ADDRESS - LCM_DATA_STARTING_BLOCK),
            FlashCommandSequence);
      returnCode = FlashProgramPhrase(&ftfl_cfg, LCM_DATA_STARTING_BLOCK, (FLASH_UPPER_BLOCK_END_ADDRESS - LCM_DATA_STARTING_BLOCK),
            (LCM_DATA_STARTING_BLOCK & (~0x80000)), FlashCommandSequence);

      // Re-enable interrupts
      _int_enable();
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

byte_t WhichFlashBlockToUse(void)
{
   // 0 = neither
   // 1 = low block
   // 2 = high block
   dword_t crcCalcBytes = 0;
   word_t savedCRCValue;
   word_t calculatedCRCValue;
   int lowBlockIndex = -1;
   int highBlockIndex = -1;
   byte_t blockToUse = 0;

   // get the total number of bytes to use for checksum
   // Canary Bytes
   crcCalcBytes += 4;

   // MAC address bytes
   crcCalcBytes += 4;

   // system_properties structure
   crcCalcBytes += flash_sizeof(sizeof(systemProperties));

   if (!strncmp(CANARY_BYTE_STRING, (const char *) FLASH_LOWER_BLOCK(SYSTEM_PROPERTIES_FLASH_ADDRESS), 3))
   {
      // canary byte is good - check for crc
      calculatedCRCValue = 0xFFFF;
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *) FLASH_LOWER_BLOCK(SYSTEM_PROPERTIES_FLASH_ADDRESS), crcCalcBytes);
      savedCRCValue = *((word_t *) FLASH_LOWER_BLOCK(SYSTEM_PROPERTIES_FLASH_CRC_ADDRESS));
      if (savedCRCValue == calculatedCRCValue)
      {
         lowBlockIndex = *((unsigned char *) (FLASH_LOWER_BLOCK(SYSTEM_PROPERTIES_FLASH_ADDRESS) + 3));
      }
   }
   if (!strncmp(CANARY_BYTE_STRING, (const char *)SYSTEM_PROPERTIES_FLASH_ADDRESS, 3))
   {
      // canary byte is good - check for crc
      calculatedCRCValue = 0xFFFF;
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *) SYSTEM_PROPERTIES_FLASH_ADDRESS, crcCalcBytes);
      savedCRCValue = *((word_t *)SYSTEM_PROPERTIES_FLASH_CRC_ADDRESS);
      if (savedCRCValue == calculatedCRCValue)
      {
         highBlockIndex = *((unsigned char *) (SYSTEM_PROPERTIES_FLASH_ADDRESS + 3));
      }
   }

   if ((lowBlockIndex == -1) && (highBlockIndex == -1))
   {  
      // Neither has the appropriate canary bytes and valid crc's. Return error
      blockToUse = 0;
   }
   else if ((lowBlockIndex == 0) && (highBlockIndex == 0xFF))
   {  
      // Special case where we may have rolled over. Use low block
      blockToUse = 1;
   }
   else if ((highBlockIndex == 0) && (lowBlockIndex == 0xFF))
   {  
      // Special case where we may have rolled over. Use high block
      blockToUse = 2;
   }
   else if (highBlockIndex > lowBlockIndex)
   {
      // Use high block
      blockToUse = 2;
   }
   else
   {
      // Use low block
      blockToUse = 1;
   }

   return blockToUse;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint32_t InstallFirmwareToFlash()
{
   // Call the pFlash swap driver function
   return pPFlashSwap(&ftfl_cfg, FLASH_SWAP_INDICATOR_ADDR, SwapCallback, FlashCommandSequence);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

uint32_t ClearFlash()
{
   uint32_t returnCode;

   // Initialize zones, scenes, scene controllers, and system properties
   InitializeZoneArray();
   InitializeSceneArray();
   InitializeSceneControllerArray();
   InitializeSystemProperties();

   // Disable interrupts while writing to flash
   _int_disable();

   // Erase the flash memory for all of the data
   returnCode = FlashEraseSector(&ftfl_cfg, LCM_DATA_STARTING_BLOCK, (SYSTEM_PROPERTIES_FLASH_ADDRESS - LCM_DATA_STARTING_BLOCK), FlashCommandSequence);
   
   // Re-enable interrupts
   _int_enable();

   // Save the default system properties to flash but keep the existing MAC
   if(returnCode == FTFx_OK)
   {
      returnCode = SaveSystemPropertiesToFlash();
   }

   return returnCode;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/*********************************************************************
 *
 *  Function Name    : SwapCallback
 *  Description      : Copies the contents of the lower p-flash block to the upper p-flash block
 *                     to simulate firwmare update
 *  Arguments        : currentSwapMode
 *  Return Value     : BOOLEAN indicating if swap should continue
 *
 *********************************************************************/
bool_t SwapCallback(uint8_t currentSwapMode)
{
   /* FUNCTION OVERVIEW 
    * This callback function is called by the swap driver function  
    * to report the swap state back to the calling application.  
    * The application can determine whether to continue the swap. 
    * When the swap driver reports that the swap system is in the Update state,
    * you can erase / reprogram the firmware in the upper block. 
    *
    * This example simulates a firmware update by copying the contents of the lower block to the 
    * upper block.  The upper block code represents the new firmware that will replace the old 
    * firmware in the lower block after the swap. 
    *
    */

   uint32_t fsec_unsecure = 0xfffffffe;
   uint32_t block1_fsec_addr = PFLASH_BLOCK1_BASE + 0x0000040C;
   BOOL swapContinue = TRUE; /* swap will continue unless modified */

   switch (currentSwapMode)
   {
      case FTFx_SWAP_UNINIT:
         /* Before you initialize the swap system you'll be in this mode. */
         printf("Swap Mode: Uninitialized\n");

         break;
      case FTFx_SWAP_READY:
         /* Typical mode you should be in before you start the swap process. */
         printf("Swap Mode: Ready\n");

         break;
      case FTFx_SWAP_UPDATE:
         /* Erase the non-active (upper) block to prepare it to be programmed.
          * Alternatively, if swapping back to software that is already in the non-active 
          * block, then do nothing.  The swap software driver will erase the sector with the 
          * swap indicator for you, as required by the swap process.  
          */
         printf("Swap Mode: Update\n");
         printf("Simulating Firmware Update: Erasing \n");
         /* erase upper block to prepare for firmware update */
//            FTFL_erase_block1_function();            
         break;
      case FTFx_SWAP_UPDATE_ERASED:
         /* Ready to program the non-active (upper) block. 
          * or if just swapping back to known good software in non-active block, then do nothing. 
          */
         printf("Swap Mode: Update Erased\n");
         printf("Simulating Firmware Update: Programming \n");
         /* copy active (lower) block to non-active (upper) block - simulates firmware update */
//            FTFL_copy_block_function();
         /* check that the flash security longword is set to "unsecure" in the upper block */
         if (READ32(block1_fsec_addr) != fsec_unsecure)
         {
            printf("Flash security longword in upper block not set to default, flash may be secured if swapped\n");
            printf("cancelling the swap\n");
            swapContinue = FALSE;
         }

         break;
      case FTFx_SWAP_COMPLETE:
         /* Ready to Reset to complete the swap.  */
         printf("Swap Mode: Complete\n");

         break;
   }

   /* report back to the swap driver if it should continue to the next swap mode */
   return swapContinue;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t HandleS19RecordString(char * S19RecordString, byte_t * S19RecordBytes, byte_t S19RecordLength, char * errorString, size_t errorStringLen)
{
   uint32_t rev;
   uint32_t returnCode;
   uint32_t writeAddress;
   uint32_t seconds;
   bool_t errorFlag = true;

   switch (S19RecordString[1] - '0')
   {
      case 0:  // block header
         /* first check if silicon is rev 1.4 or later */
         /* For rev 1.4 (4N30D), REVID = 3 */
         /* check if device supports swap */
         /* note - on kinetis 100Mhz devices, swap is supported on devices with two p-flash blocks and no flexNVM */
         /* so checking if pflash bit is set which indicates the device only has p-flash */
         // Disable interrupts while writing to flash
         _int_disable();

         returnCode = FlashEraseSector(&ftfl_cfg, PFLASH_BLOCK1_BASE, (LCM_DATA_STARTING_BLOCK - PFLASH_BLOCK1_BASE), FlashCommandSequence);

         // Re-enable interrupts
         _int_enable();

         if (returnCode != FTFx_OK)
         {
            snprintf(errorString, errorStringLen, "Failure erasing FLASH.  Code=%d", returnCode);
            break;
         }
         errorFlag = false;
         break;

      case 1:  // data sequence - 2 address bytes
         snprintf(errorString, errorStringLen, "Don't handle 'S1' yet.");
         break;

      case 2:  // data sequence - 3 address bytes
         snprintf(errorString, errorStringLen, "Don't handle 'S2' yet.");
         break;

      case 3:  // data sequence - 4 address bytes
         HighByteOfDword(writeAddress) = S19RecordBytes[0];
         UpperMiddleByteOfDword(writeAddress) = S19RecordBytes[1];
         LowerMiddleByteOfDword(writeAddress) = S19RecordBytes[2];
         LowByteOfDword(writeAddress) = S19RecordBytes[3];
         writeAddress += PFLASH_BLOCK1_BASE;
         if (writeAddress >= LCM_DATA_STARTING_BLOCK)
         {
            snprintf(errorString, errorStringLen, "Address out of range.");
            break;
         }

         // Disable interrupts while writing to flash
         _int_disable();

         returnCode = FlashProgramPhrase(&ftfl_cfg, writeAddress, (S19RecordLength - 4), (uint32_t) &S19RecordBytes[4], FlashCommandSequence);

         // Re-enable interrupts
         _int_enable();

         if (returnCode != FTFx_OK)
         {
            snprintf(errorString, errorStringLen, "Flash write failed.  Address=%d, Code=%d", (writeAddress - PFLASH_BLOCK1_BASE), returnCode);
            break;
         }
         errorFlag = false;
         break;

      case 5:  // record count
         snprintf(errorString, errorStringLen, "Don't handle 'S5' yet.");
         break;

      case 7:  // end of block - 4 address bytes
         // copy over the non-volatile data
         returnCode = SaveSystemPropertiesToFlash();
         if (returnCode != FTFx_OK)
         {
            snprintf(errorString, errorStringLen, "Non-volatile flash write failed.  Code=%d", returnCode);
            break;
         }
         errorFlag = false;
         break;

      case 8:  // end of block - 3 address bytes
         snprintf(errorString, errorStringLen, "Don't handle 'S8' yet.");
         break;

      case 9:  // end of block - 2 address bytes
         snprintf(errorString, errorStringLen, "Don't handle 'S9' yet.");
         break;

      default:  // error
         snprintf(errorString, errorStringLen, "Unrecognized 'S' type.");
         break;
   }

   return errorFlag;

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool_t LoadSystemPropertiesFromFlash()
{
   dword_t crcCalcBytes = 0;
   word_t savedCRCValue;
   word_t calculatedCRCValue;
   bool_t error = FALSE;
   bool_t mustResaveFlashFlag = false;

   // Calculate the total number of bytes to use for checksum
   // 4 Canary Bytes + 4 MAC Address Bytes + sizeof(systemProperties)
   crcCalcBytes = 4 + 4 + flash_sizeof(sizeof(systemProperties));

   // Get the address where the data begins
   uchar * pFlashMemory = (unsigned char *)SYSTEM_PROPERTIES_FLASH_ADDRESS;

   if (!strncmp(CANARY_BYTE_STRING, (const char *)pFlashMemory, 3))
   {
      // canary byte is good - check for crc
      calculatedCRCValue = 0xFFFF;
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, pFlashMemory, crcCalcBytes);
      savedCRCValue = *((word_t *)SYSTEM_PROPERTIES_FLASH_CRC_ADDRESS);
      if (savedCRCValue == calculatedCRCValue)
      {
         // Move the pointer past the canary bytes, to the index value
         pFlashMemory += 3;
         if (*pFlashMemory != flashBlockIndexForNextWrite)
         {
            mustResaveFlashFlag = true;
         }
         
         // Move the pointer past the the index value
         pFlashMemory++;

         // Canary bytes and CRC is valid. Load the data
         // load the MAC Address
         myMACAddress[3] = *pFlashMemory++;
         myMACAddress[4] = *pFlashMemory++;
         myMACAddress[5] = *pFlashMemory++;
         pFlashMemory++;

         // Load the System Properties
         memcpy(&systemProperties, pFlashMemory, sizeof(systemProperties));

         // Set the DST flag based on the GMT offsets
         if(systemProperties.effectiveGmtOffsetSeconds == systemProperties.gmtOffsetSeconds)
         {
            inDST = FALSE;
         }
         else if(systemProperties.effectiveGmtOffsetSeconds == (systemProperties.gmtOffsetSeconds + 3600))
         {
            inDST = TRUE;
         }
         else
         {
            // There was an error and the effective GMT offset is invalid.
            // Set it equal to the gmt offset and set the inDST flag to false
            systemProperties.effectiveGmtOffsetSeconds = systemProperties.gmtOffsetSeconds;
            inDST = FALSE;
            mustResaveFlashFlag = true;
         }
         
         // check if we have valid eliot cloud data
         pFlashMemory = (unsigned char *) (SYSTEM_PROPERTIES_FLASH_ADDRESS + crcCalcBytes);
         
         // Verify that the CRC is valid
         savedCRCValue = *((word_t *) pFlashMemory);

         // Extended system properties starts 8 bytes after the crc
         pFlashMemory += 8;

         calculatedCRCValue = 0xFFFF;
         calculatedCRCValue = UpdateCRC16(calculatedCRCValue, pFlashMemory, sizeof(extendedSystemProperties));
         if (savedCRCValue == calculatedCRCValue)
         {
            // Load the extended system properties
            memcpy(&extendedSystemProperties, pFlashMemory, sizeof(extendedSystemProperties));
         }
         
      }
      else
      {
         // CRC was not valid
         error = TRUE;
      }
   }
   else
   {
      // Canary byte is not valid
      error = TRUE;
   }

   if (mustResaveFlashFlag)
   {
      SaveSystemPropertiesToFlash();
   }
      
   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t SaveSystemPropertiesToFlash()
{
   uint8_t *   pFlashMemory;
   uint_32     returnCode;
   uint_32     destinationAddress = SYSTEM_PROPERTIES_FLASH_ADDRESS;
   uint_16     numberOfBytesToWrite;
   uint_16     calculatedCRCValue;
   uint8_t     titleBlock[8];

   // Disable interrupts while writing to flash
   _int_disable();

   returnCode = FlashEraseSector(&ftfl_cfg, destinationAddress, 0x01000, FlashCommandSequence);

   // Re-enable interrupts
   _int_enable();

   if (returnCode != FTFx_OK)
   {
      returnCode = (10000 + returnCode);
   }
   else
   {
      // Disable interrupts while writing to flash
      _int_disable();

      destinationAddress += 8;  // skip the canary bytes and mac address for now

      numberOfBytesToWrite = flash_sizeof(sizeof(systemProperties));
      // save the systemProperties data
      returnCode = FlashProgramPhrase(&ftfl_cfg,
                                       destinationAddress,
                                        numberOfBytesToWrite,
                                         (uint32_t) &systemProperties,
                                          FlashCommandSequence);
      if (returnCode != FTFx_OK)
      {
         returnCode = (20000 + returnCode);
      }
      else
      {
         destinationAddress += numberOfBytesToWrite;
         // Save the extended settings (its crc will be at the current
         // destinationAddress and the actual structure will be 8 bytes after).
         numberOfBytesToWrite = flash_sizeof(sizeof(extendedSystemProperties));
         returnCode = FlashProgramPhrase(&ftfl_cfg,
                                          (destinationAddress + 8),
                                           numberOfBytesToWrite,
                                            (uint32_t) &extendedSystemProperties,
                                             FlashCommandSequence);
         if (returnCode != FTFx_OK)
         {
            returnCode = (30000 + returnCode);
         }
         else
         {
         
            // save the extended settings crc
            calculatedCRCValue = 0xFFFF;
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *) (destinationAddress + 8), sizeof(extendedSystemProperties));
            memset(&titleBlock[0], 0xFF, sizeof(titleBlock));
            memcpy(&titleBlock[0], &calculatedCRCValue, sizeof(calculatedCRCValue));
            returnCode = FlashProgramPhrase(&ftfl_cfg,
                                             destinationAddress,
                                              8,
                                               (uint32_t) titleBlock,
                                                FlashCommandSequence);
            if (returnCode != FTFx_OK)
            {
               returnCode = (40000 + returnCode);
            }
            else
            {

               // finally, save the canary bytes and mac address
               memcpy(&titleBlock[0], CANARY_BYTE_STRING, 3);
               titleBlock[3] = flashBlockIndexForNextWrite;
               memcpy(&titleBlock[4], &myMACAddress[3], 3);
               titleBlock[7] = 0xFF;
               returnCode = FlashProgramPhrase(&ftfl_cfg,
                                                SYSTEM_PROPERTIES_FLASH_ADDRESS,
                                                 8,
                                                  (uint32_t) titleBlock,
                                                   FlashCommandSequence);
               if (returnCode != FTFx_OK)
               {
                  returnCode = (50000 + returnCode);
               }
               else
               {
                  // one last verification - calculate and save a crc
                  calculatedCRCValue = 0xFFFF;
                  calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *) SYSTEM_PROPERTIES_FLASH_ADDRESS, (flash_sizeof(sizeof(systemProperties) + 8)));
                  memset(&titleBlock[0], 0xFF, sizeof(titleBlock));
                  memcpy(&titleBlock[0], &calculatedCRCValue, sizeof(calculatedCRCValue));
                  returnCode = FlashProgramPhrase(&ftfl_cfg,
                                                   SYSTEM_PROPERTIES_FLASH_CRC_ADDRESS,
                                                    8,
                                                     (uint32_t) titleBlock,
                                                      FlashCommandSequence);
                  if (returnCode != FTFx_OK)
                  {
                     returnCode = (60000 + returnCode);
                  }
               }
            }
         }
      }

      // Re-enable interrupts
      _int_enable();
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

nonvolatile_eliot_zone_properties * GetNVEliotZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData)
{
   // Return the pointer to the data based on the zoneIndex requested. 
   // Verify that the checksum is valid for this zone before returning.
   byte_t * dataPtr = NULL;
   nonvolatile_eliot_zone_properties *nvEliotZonePropertiesPtr = NULL;
   word_t calculatedCRCValue = 0xFFFF;
   word_t storedCRCValue;
   byte_t sectorIndex = (zoneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   byte_t zoneInSectorIndex = (zoneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      dataPtr = (byte_t *) (LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex) + (ELIOT_PROPERTIES_OFFSET * zoneInSectorIndex));

      // Verify that the CRC is valid
      HighByteOfWord(storedCRCValue) = *dataPtr++;
      LowByteOfWord(storedCRCValue) = *dataPtr++;

      // Add 6 to the data Ptr to move past the pad bytes after the CRC
      dataPtr = dataPtr + 6;

      // Calculate the CRC for the zone data
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(nonvolatile_eliot_zone_properties));

      if((storedCRCValue == calculatedCRCValue) ||
         (!validateZoneData))
      {
         if(((dword_t)dataPtr >= LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS) &&
            (((dword_t)dataPtr + sizeof(nonvolatile_eliot_zone_properties)) <= LCM_DATA_ELIOT_ZONE_PROPERTIES_END_ADDRESS))
         {
            // The CRC value matches what is expected and the 
            // address is within the valid range for zones. Return the address
            nvEliotZonePropertiesPtr = (nonvolatile_eliot_zone_properties*)dataPtr;
         }
      }
   }
   
   return nvEliotZonePropertiesPtr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

nonvolatile_zone_properties * GetNVZonePropertiesPtr(byte_t zoneIndex, bool_t validateZoneData)
{
   // Return the pointer to the data based on the zoneIndex requested. 
   // Verify that the checksum is valid for this zone before returning.
   byte_t * dataPtr = NULL;
   nonvolatile_zone_properties *nvZonePropertiesPtr = NULL;
   word_t calculatedCRCValue = 0xFFFF;
   word_t storedCRCValue;

   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      dataPtr = (byte_t *) (LCM_DATA_ZONES_FLASH_START_ADDRESS + (ZONE_INDEX_OFFSET * zoneIndex));

      // Verify that the CRC is valid
      HighByteOfWord(storedCRCValue) = *dataPtr++;
      LowByteOfWord(storedCRCValue) = *dataPtr++;

      // Add 6 to the data Ptr to move past the pad bytes after the CRC
      dataPtr = dataPtr + 6;

      // Calculate the CRC for the zone data
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(nonvolatile_zone_properties));

      if((storedCRCValue == calculatedCRCValue) ||
         (!validateZoneData))
      {
         if(((dword_t)dataPtr >= LCM_DATA_ZONES_FLASH_START_ADDRESS) &&
            (((dword_t)dataPtr + sizeof(nonvolatile_zone_properties)) <= LCM_DATA_ZONES_FLASH_END_ADDRESS))
         {
            // The CRC value matches what is expected and the 
            // address is within the valid range for zones. Return the address
            nvZonePropertiesPtr = (nonvolatile_zone_properties*)dataPtr;
         }
      }
   }
   
   return nvZonePropertiesPtr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t WriteEliotZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_eliot_zone_properties * nvEliotZonePropertiesPtr)
{
   uint32_t returnCode = 0xFFFFFFFF;
   dword_t zoneAddressInFlash;
   dword_t endAddressInFlash;
   dword_t sectorIndex = (zoneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   dword_t zoneInSectorIndex = (zoneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   word_t numberOfBytesToWrite;
   word_t calculatedCRCValue = 0xFFFF;
   byte_t crcArray[8];

   // Write the zone properties to flash for the index. No need to worry
   // about erasing flash in this function, because it is assumed that this
   // function will only be called when flash is already blank
   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      // Only write valid data
      bool_t dataValid = FALSE;
      byte_t * memoryByte = (byte_t *)nvEliotZonePropertiesPtr;

      for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_eliot_zone_properties)) && (!dataValid); byteIndex++, memoryByte++)
      {
         if(*memoryByte != 0xFF)
         {
            dataValid = TRUE;
         }
      }

      if(dataValid)
      {
         zoneAddressInFlash = LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex) + (dword_t)(ELIOT_PROPERTIES_OFFSET * zoneInSectorIndex);

         endAddressInFlash = zoneAddressInFlash + flash_sizeof(sizeof(crcArray)) + flash_sizeof(sizeof(nonvolatile_eliot_zone_properties));

         // Make sure the address falls in the flash block for zones
         if((zoneAddressInFlash >= LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS) &&
            (endAddressInFlash <= LCM_DATA_ELIOT_ZONE_PROPERTIES_END_ADDRESS))
         {
            // Calculate the CRC for the zone data
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *)nvEliotZonePropertiesPtr, sizeof(nonvolatile_eliot_zone_properties));

            // Write the CRC bytes to flash
            memset(crcArray, 0xFF, sizeof(crcArray));
            crcArray[0] = HighByteOfWord(calculatedCRCValue);
            crcArray[1] = LowByteOfWord(calculatedCRCValue);
            numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

            // Disable interrupts while writing to flash
            _int_disable();

            returnCode = FlashProgramPhrase(&ftfl_cfg, zoneAddressInFlash, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

            // Get the size of the flash bytes to write
            numberOfBytesToWrite = flash_sizeof(sizeof(nonvolatile_eliot_zone_properties));
            returnCode = FlashProgramPhrase(&ftfl_cfg, zoneAddressInFlash + 8, numberOfBytesToWrite, (uint32_t)nvEliotZonePropertiesPtr, FlashCommandSequence);

            // Re-enable interrupts
            _int_enable();
         }
         else
         {
            returnCode = FTFx_ERR_RANGE;
         }
      }
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t WriteZonePropertiesToFlash(byte_t zoneIndex, nonvolatile_zone_properties * nvZonePropertiesPtr)
{
   uint32_t returnCode = 0xFFFFFFFF;
   dword_t zoneAddressInFlash;
   dword_t endAddressInFlash;
   word_t numberOfBytesToWrite;
   word_t calculatedCRCValue = 0xFFFF;
   byte_t crcArray[8];

   // Write the zone properties to flash for the index. No need to worry
   // about erasing flash in this function, because it is assumed that this
   // function will only be called when flash is already blank
   if(zoneIndex < MAX_NUMBER_OF_ZONES)
   {
      // Only write valid data
      bool_t dataValid = FALSE;
      byte_t * memoryByte = (byte_t *)nvZonePropertiesPtr;

      for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_zone_properties)) && (!dataValid); byteIndex++, memoryByte++)
      {
         if(*memoryByte != 0xFF)
         {
            dataValid = TRUE;
         }
      }

      if(dataValid)
      {
         zoneAddressInFlash = LCM_DATA_ZONES_FLASH_START_ADDRESS + (dword_t)(ZONE_INDEX_OFFSET * zoneIndex);

         endAddressInFlash = zoneAddressInFlash + flash_sizeof(sizeof(crcArray)) + flash_sizeof(sizeof(nonvolatile_zone_properties));

         // Make sure the address falls in the flash block for zones
         if((zoneAddressInFlash >= LCM_DATA_ZONES_FLASH_START_ADDRESS) &&
            (endAddressInFlash <= LCM_DATA_ZONES_FLASH_END_ADDRESS))
         {
            // Calculate the CRC for the zone data
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *)nvZonePropertiesPtr, sizeof(nonvolatile_zone_properties));

            // Write the CRC bytes to flash
            memset(crcArray, 0xFF, sizeof(crcArray));
            crcArray[0] = HighByteOfWord(calculatedCRCValue);
            crcArray[1] = LowByteOfWord(calculatedCRCValue);
            numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

            // Disable interrupts while writing to flash
            _int_disable();

            returnCode = FlashProgramPhrase(&ftfl_cfg, zoneAddressInFlash, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

            // Get the size of the flash bytes to write
            numberOfBytesToWrite = flash_sizeof(sizeof(nonvolatile_zone_properties));
            returnCode = FlashProgramPhrase(&ftfl_cfg, zoneAddressInFlash + 8, numberOfBytesToWrite, (uint32_t)nvZonePropertiesPtr, FlashCommandSequence);

            // Re-enable interrupts
            _int_enable();
         }
         else
         {
            returnCode = FTFx_ERR_RANGE;
         }
      }
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t ReadEliotZonePropertiesFromFlashSector(byte_t zoneIndex, nonvolatile_eliot_zone_properties nvEliotZoneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)
   {
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (zoneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

      // Get the eliot zone properties for all of the zones in that sector
      for(byte_t zoneInSectorIndex = 0; zoneInSectorIndex < arraySize; zoneInSectorIndex++)
      {
         nonvolatile_eliot_zone_properties * nvEliotZonePropertiesPtr = GetNVEliotZonePropertiesPtr((zoneInSectorIndex + (sectorIndex * NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)), true);

         if(nvEliotZonePropertiesPtr)
         {
            memcpy(&nvEliotZoneArray[zoneInSectorIndex], nvEliotZonePropertiesPtr, sizeof(nonvolatile_eliot_zone_properties));
         }
         else
         {
            memset(&nvEliotZoneArray[zoneInSectorIndex], 0xFF, sizeof(nonvolatile_eliot_zone_properties));
         }
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ReadAllZonePropertiesFromFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize)
{
   // Read all of the zone properties from flash
   for(byte_t zoneIndex = 0; zoneIndex < arraySize; zoneIndex++)
   {
      nonvolatile_zone_properties * nvZonePropertiesPtr = GetNVZonePropertiesPtr(zoneIndex, true);

      if(nvZonePropertiesPtr)
      {
         memcpy(&nvZoneArray[zoneIndex], nvZonePropertiesPtr, sizeof(nonvolatile_zone_properties));
      }
      else
      {
         memset(&nvZoneArray[zoneIndex], 0xFF, sizeof(nonvolatile_zone_properties));
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t WriteEliotZonePropertiesToFlashSector(byte_t zoneIndex, nonvolatile_eliot_zone_properties nvEliotZoneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)
   {
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (zoneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

      // Write the zone properties to flash for all zones in the sector
      for(byte_t zoneInSectorIndex = 0; zoneInSectorIndex < arraySize; zoneInSectorIndex++)
      {
         WriteEliotZonePropertiesToFlash((zoneInSectorIndex + (sectorIndex * NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)), &nvEliotZoneArray[zoneInSectorIndex]);
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void WriteAllZonePropertiesToFlash(nonvolatile_zone_properties nvZoneArray[], size_t arraySize)
{
   // Write the zone properties to flash
   for(byte_t zoneIndex = 0; zoneIndex < arraySize; zoneIndex++)
   {
      WriteZonePropertiesToFlash(zoneIndex, &nvZoneArray[zoneIndex]);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EraseEliotZonePropertiesFromFlash(byte_t zoneIndex)
{
   byte_t sectorIndex = (zoneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   uint32_t address = LCM_DATA_ELIOT_ZONE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex);

   // Erase the zone properties sector
   _int_disable();
   FlashEraseSector(&ftfl_cfg, address, 0x1000, FlashCommandSequence);
   _int_enable();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EraseZonesFromFlash()
{
   // Erase the zone properties sector
   _int_disable();
   FlashEraseSector(&ftfl_cfg, (uint32_t)LCM_DATA_ZONES_FLASH_START_ADDRESS, 0x1000, FlashCommandSequence);
   _int_enable();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

nonvolatile_eliot_scene_properties * GetNVEliotScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData)
{
   // Return the pointer to the data based on the sceneIndex requested. 
   // Verify that the checksum is valid for this scene before returning.
   byte_t * dataPtr = NULL;
   nonvolatile_eliot_scene_properties *nvEliotScenePropertiesPtr = NULL;
   word_t calculatedCRCValue = 0xFFFF;
   word_t storedCRCValue;
   byte_t sectorIndex = (sceneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   byte_t sceneInSectorIndex = (sceneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      dataPtr = (byte_t *) (LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex) + (ELIOT_PROPERTIES_OFFSET * sceneInSectorIndex));

      // Verify that the CRC is valid
      HighByteOfWord(storedCRCValue) = *dataPtr++;
      LowByteOfWord(storedCRCValue) = *dataPtr++;

      // Add 6 to the data Ptr to move past the pad bytes after the CRC
      dataPtr = dataPtr + 6;

      // Calculate the CRC for the scene data
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(nonvolatile_eliot_scene_properties));

      if((storedCRCValue == calculatedCRCValue) ||
         (!validateSceneData))
      {
         if(((dword_t)dataPtr >= LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS) &&
            (((dword_t)dataPtr + sizeof(nonvolatile_eliot_scene_properties)) <= LCM_DATA_ELIOT_SCENE_PROPERTIES_END_ADDRESS))
         {
            // The CRC value matches what is expected and the 
            // address is within the valid range for scenes. Return the address
            nvEliotScenePropertiesPtr = (nonvolatile_eliot_scene_properties*)dataPtr;
         }
      }
   }
   
   return nvEliotScenePropertiesPtr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

nonvolatile_scene_properties * GetNVScenePropertiesPtr(byte_t sceneIndex, bool_t validateSceneData)
{
   size_t nvSceneSize = sizeof(nonvolatile_scene_properties);
   size_t flashSize = flash_sizeof(nvSceneSize);

   // Return the pointer to the data based on the sceneIndex requested. 
   // Verify that the checksum is valid for this scene before returning.
   byte_t * dataPtr = NULL;
   nonvolatile_scene_properties *nvScenePropertiesPtr = NULL;
   word_t calculatedCRCValue = 0xFFFF;
   word_t storedCRCValue;
   byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
   byte_t sceneInSectorIndex = (sceneIndex % NUM_SCENES_IN_FLASH_SECTOR);

   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      // Calculate the scene block based on the index
      dataPtr = (byte_t *) (LCM_DATA_SCENES_FLASH_START_ADDRESS + (0x1000 * sectorIndex) + (SCENE_INDEX_OFFSET * sceneInSectorIndex));

      // Verify that the CRC is valid
      HighByteOfWord(storedCRCValue) = *dataPtr++;
      LowByteOfWord(storedCRCValue) = *dataPtr++;

      // Add 6 to the data Ptr to move past the pad bytes after the CRC
      dataPtr = dataPtr + 6;

      // Calculate the CRC for the scene data
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(nonvolatile_scene_properties));

      if((storedCRCValue == calculatedCRCValue) ||
         (!validateSceneData))
      {
         if(((dword_t)dataPtr >= LCM_DATA_SCENES_FLASH_START_ADDRESS) &&
            (((dword_t)dataPtr + sizeof(nonvolatile_scene_properties)) <= LCM_DATA_SCENES_FLASH_END_ADDRESS))
         {
            // The CRC value matches what is expected and the address
            // is within the valid range for scenes. Return the address
            nvScenePropertiesPtr = (nonvolatile_scene_properties *)dataPtr;
         }
      }
   }
   
   return nvScenePropertiesPtr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t WriteEliotScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_eliot_scene_properties * nvEliotScenePropertiesPtr)
{
   uint32_t returnCode = 0xFFFFFFFF;
   dword_t sceneAddressInFlash;
   dword_t endAddressInFlash;
   dword_t sectorIndex = (sceneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   dword_t sceneInSectorIndex = (sceneIndex % NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   word_t numberOfBytesToWrite;
   word_t calculatedCRCValue = 0xFFFF;
   byte_t crcArray[8];

   // Write the scene properties to flash for the index. No need to worry
   // about erasing flash in this function, because it is assumed that this
   // function will only be called when flash is already blank
   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      // Only write valid data
      bool_t dataValid = FALSE;
      byte_t * memoryByte = (byte_t *)nvEliotScenePropertiesPtr;

      for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_eliot_scene_properties)) && (!dataValid); byteIndex++, memoryByte++)
      {
         if(*memoryByte != 0xFF)
         {
            dataValid = TRUE;
         }
      }

      if(dataValid)
      {
         sceneAddressInFlash = LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex) + (dword_t)(ELIOT_PROPERTIES_OFFSET * sceneInSectorIndex);

         endAddressInFlash = sceneAddressInFlash + flash_sizeof(sizeof(crcArray)) + flash_sizeof(sizeof(nonvolatile_eliot_scene_properties));

         // Make sure the address falls in the flash block for scenes
         if((sceneAddressInFlash >= LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS) &&
            (endAddressInFlash <= LCM_DATA_ELIOT_SCENE_PROPERTIES_END_ADDRESS))
         {
            // Calculate the CRC for the scene data
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *)nvEliotScenePropertiesPtr, sizeof(nonvolatile_eliot_scene_properties));

            // Write the CRC bytes to flash
            memset(crcArray, 0xFF, sizeof(crcArray));
            crcArray[0] = HighByteOfWord(calculatedCRCValue);
            crcArray[1] = LowByteOfWord(calculatedCRCValue);
            numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

            // Disable interrupts while writing to flash
            _int_disable();

            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneAddressInFlash, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

            // Get the size of the flash bytes to write
            numberOfBytesToWrite = flash_sizeof(sizeof(nonvolatile_eliot_scene_properties));
            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneAddressInFlash + 8, numberOfBytesToWrite, (uint32_t)nvEliotScenePropertiesPtr, FlashCommandSequence);

            // Re-enable interrupts
            _int_enable();
         }
         else
         {
            returnCode = FTFx_ERR_RANGE;
         }
      }
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t WriteScenePropertiesToFlash(byte_t sceneIndex, nonvolatile_scene_properties * nvScenePropertiesPtr)
{
   uint32_t returnCode = 0xFFFFFFFF;
   dword_t sceneAddressInFlash;
   dword_t endAddressInFlash;
   dword_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
   dword_t sceneInSectorIndex = (sceneIndex % NUM_SCENES_IN_FLASH_SECTOR);
   word_t numberOfBytesToWrite;
   word_t calculatedCRCValue = 0xFFFF;
   byte_t crcArray[8];

   // Write the scene properties to flash for the index. No need to worry
   // about erasing flash in this function, because it is assumed that this
   // function will only be called when flash is already blank
   if(sceneIndex < MAX_NUMBER_OF_SCENES)
   {
      // Only write valid data
      bool_t dataValid = FALSE;
      byte_t * memoryByte = (byte_t *)nvScenePropertiesPtr;

      for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_scene_properties)) && (!dataValid); byteIndex++, memoryByte++)
      {
         if(*memoryByte != 0xFF)
         {
            dataValid = TRUE;
         }
      }

      if(dataValid)
      {
         sceneAddressInFlash = LCM_DATA_SCENES_FLASH_START_ADDRESS + (0x1000 * sectorIndex) + (dword_t)(SCENE_INDEX_OFFSET * sceneInSectorIndex);

         endAddressInFlash = sceneAddressInFlash + flash_sizeof(sizeof(crcArray)) + flash_sizeof(sizeof(nonvolatile_scene_properties));

         // Make sure the address falls in the flash block for zones
         if((sceneAddressInFlash >= LCM_DATA_SCENES_FLASH_START_ADDRESS) &&
            (endAddressInFlash <= LCM_DATA_SCENES_FLASH_END_ADDRESS))
         {
            // Calculate the CRC for the scene data
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *)nvScenePropertiesPtr, sizeof(nonvolatile_scene_properties));

            // Write the CRC bytes to flash
            memset(crcArray, 0xFF, sizeof(crcArray));
            crcArray[0] = HighByteOfWord(calculatedCRCValue);
            crcArray[1] = LowByteOfWord(calculatedCRCValue);
            numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

            // Disable interrupts while writing to flash
            _int_disable();

            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneAddressInFlash, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

            // Get the size of the flash bytes to write
            numberOfBytesToWrite = flash_sizeof(sizeof(nonvolatile_scene_properties));
            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneAddressInFlash + 8, numberOfBytesToWrite, (uint32_t)nvScenePropertiesPtr, FlashCommandSequence);

            // Re-enable interrupts
            _int_enable();
         }
         else
         {
            returnCode = FTFx_ERR_RANGE;
         }
      }
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t ReadEliotScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_eliot_scene_properties nvEliotSceneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)
   {
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (sceneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

      // Get the eliot scene properties for all of the scenes in that sector
      for(byte_t sceneInSectorIndex = 0; sceneInSectorIndex < arraySize; sceneInSectorIndex++)
      {
         nonvolatile_eliot_scene_properties * nvEliotScenePropertiesPtr = GetNVEliotScenePropertiesPtr((sceneInSectorIndex + (sectorIndex * NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)), true);

         if(nvEliotScenePropertiesPtr)
         {
            memcpy(&nvEliotSceneArray[sceneInSectorIndex], nvEliotScenePropertiesPtr, sizeof(nonvolatile_eliot_scene_properties));
         }
         else
         {
            memset(&nvEliotSceneArray[sceneInSectorIndex], 0xFF, sizeof(nonvolatile_eliot_scene_properties));
         }
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t ReadScenePropertiesFromFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_SCENES_IN_FLASH_SECTOR)
   {
      // Will not get the entire number of scenes in the flash sector
      // return an error
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);

      // Get the scene properties for all of the scenes in that sector
      for(byte_t sceneInSectorIndex = 0; sceneInSectorIndex < NUM_SCENES_IN_FLASH_SECTOR; sceneInSectorIndex++)
      {
         nonvolatile_scene_properties * nvScenePropertiesPtr = GetNVScenePropertiesPtr((sceneInSectorIndex + (sectorIndex * NUM_SCENES_IN_FLASH_SECTOR)), true);

         if(nvScenePropertiesPtr)
         {
            memcpy(&nvSceneArray[sceneInSectorIndex], nvScenePropertiesPtr, sizeof(nonvolatile_scene_properties));
         }
         else
         {
            memset(&nvSceneArray[sceneInSectorIndex], 0xFF, sizeof(nonvolatile_scene_properties));
         }
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t WriteEliotScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_eliot_scene_properties nvEliotSceneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)
   {
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (sceneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);

      // Write the scene properties to flash for all scenes in the sector
      for(byte_t sceneInSectorIndex = 0; sceneInSectorIndex < arraySize; sceneInSectorIndex++)
      {
         WriteEliotScenePropertiesToFlash((sceneInSectorIndex + (sectorIndex * NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR)), &nvEliotSceneArray[sceneInSectorIndex]);
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

bool_t WriteScenePropertiesToFlashSector(byte_t sceneIndex, nonvolatile_scene_properties nvSceneArray[], size_t arraySize)
{
   bool_t error = false;

   if(arraySize != NUM_SCENES_IN_FLASH_SECTOR)
   {
      // Will not write the entire number of scenes in the flash sector
      // return an error
      error = true;
   }
   else
   {
      // Determine the sector to use
      byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);

      // Write the scene properties for all of the scenes in the sector
      for(byte_t sceneInSectorIndex = 0; sceneInSectorIndex < NUM_SCENES_IN_FLASH_SECTOR; sceneInSectorIndex++)
      {
         WriteScenePropertiesToFlash((sceneInSectorIndex + (sectorIndex * NUM_SCENES_IN_FLASH_SECTOR)), &nvSceneArray[sceneInSectorIndex]);
      }
   }

   return error;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EraseEliotScenePropertiesFromFlash(byte_t sceneIndex)
{
   byte_t sectorIndex = (sceneIndex / NUM_ELIOT_PROPERTIES_IN_FLASH_SECTOR);
   uint32_t address = LCM_DATA_ELIOT_SCENE_PROPERTIES_START_ADDRESS + (0x1000 * sectorIndex);

   // Erase the scene properties sector
   _int_disable();
   FlashEraseSector(&ftfl_cfg, address, 0x1000, FlashCommandSequence);
   _int_enable();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EraseScenesFromFlash(byte_t sceneIndex)
{
   byte_t sectorIndex = (sceneIndex / NUM_SCENES_IN_FLASH_SECTOR);
   uint32_t address = LCM_DATA_SCENES_FLASH_START_ADDRESS + (0x1000 * sectorIndex);

   // Erase the scene properties sector containing this scene index
   _int_disable();
   FlashEraseSector(&ftfl_cfg, address, 0x1000, FlashCommandSequence);
   _int_enable();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

nonvolatile_scene_controller_properties * GetNVSceneControllerPropertiesPtr(byte_t sceneControllerIndex, bool_t validateSceneControllerData)
{
   // Return the pointer to the data based on the sceneControllerIndex requested. 
   // Verify that the checksum is valid for this index before returning.
   byte_t * dataPtr = NULL;
   nonvolatile_scene_controller_properties *nvSceneControllerPropertiesPtr = NULL;
   word_t calculatedCRCValue = 0xFFFF;
   word_t storedCRCValue;

   if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
   {
      dataPtr = (byte_t *) (LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS + (SCENE_CONTROLLER_INDEX_OFFSET * sceneControllerIndex));

      // Verify that the CRC is valid
      HighByteOfWord(storedCRCValue) = *dataPtr++;
      LowByteOfWord(storedCRCValue) = *dataPtr++;

      // Add 6 to the data Ptr to move past the pad bytes after the CRC
      dataPtr = dataPtr + 6;

      // Calculate the CRC for the scene controller data
      calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(nonvolatile_scene_controller_properties));

      if((storedCRCValue == calculatedCRCValue) ||
         (!validateSceneControllerData))
      {
         if(((dword_t)dataPtr >= LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS) &&
            (((dword_t)dataPtr + sizeof(nonvolatile_scene_controller_properties)) <= LCM_DATA_SCENE_CONTROLLERS_FLASH_END_ADDRESS))
         {
            // The CRC value matches what is expected and the 
            // address is within the valid range for scene controllers. 
            // Return the address
            nvSceneControllerPropertiesPtr = (nonvolatile_scene_controller_properties*)dataPtr;
         }
      }
   }
   
   return nvSceneControllerPropertiesPtr;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

uint32_t WriteSceneControllerPropertiesToFlash(byte_t sceneControllerIndex, nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr)
{
   uint32_t returnCode = 0xFFFFFFFF;
   dword_t sceneControllerAddressInFlash;
   dword_t endAddressInFlash;
   word_t numberOfBytesToWrite;
   word_t calculatedCRCValue = 0xFFFF;
   byte_t crcArray[8];

   // Write the scene controller properties to flash for the index. No need to worry
   // about erasing flash in this function, because it is assumed that this
   // function will only be called when flash is already blank
   if(sceneControllerIndex < MAX_NUMBER_OF_SCENE_CONTROLLERS)
   {
      // Only write valid data
      bool_t dataValid = FALSE;
      byte_t * memoryByte = (byte_t *)nvSceneControllerPropertiesPtr;

      for(size_t byteIndex = 0; (byteIndex < sizeof(nonvolatile_scene_controller_properties)) && (!dataValid); byteIndex++, memoryByte++)
      {
         if(*memoryByte != 0xFF)
         {
            dataValid = TRUE;
         }
      }

      if(dataValid)
      {
         sceneControllerAddressInFlash = LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS + (dword_t)(SCENE_CONTROLLER_INDEX_OFFSET * sceneControllerIndex);

         endAddressInFlash = sceneControllerAddressInFlash + flash_sizeof(sizeof(crcArray)) + flash_sizeof(sizeof(nonvolatile_scene_controller_properties));

         // Make sure the address falls in the flash block for scene controllers
         if((sceneControllerAddressInFlash >= LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS) &&
            (endAddressInFlash <= LCM_DATA_SCENE_CONTROLLERS_FLASH_END_ADDRESS))
         {
            // Calculate the CRC for the scene controller data
            calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *)nvSceneControllerPropertiesPtr, sizeof(nonvolatile_scene_controller_properties));

            // Write the CRC bytes to flash
            memset(crcArray, 0xFF, sizeof(crcArray));
            crcArray[0] = HighByteOfWord(calculatedCRCValue);
            crcArray[1] = LowByteOfWord(calculatedCRCValue);
            numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

            // Disable interrupts while writing to flash
            _int_disable();

            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneControllerAddressInFlash, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

            // Get the size of the flash bytes to write
            numberOfBytesToWrite = flash_sizeof(sizeof(nonvolatile_scene_controller_properties));
            returnCode = FlashProgramPhrase(&ftfl_cfg, sceneControllerAddressInFlash + 8, numberOfBytesToWrite, (uint32_t)nvSceneControllerPropertiesPtr, FlashCommandSequence);

            // Re-enable interrupts
            _int_enable();
         }
         else
         {
            returnCode = FTFx_ERR_RANGE;
         }
      }
   }

   return returnCode;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void ReadAllSceneControllerPropertiesFromFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize)
{
   // Read all of the scene controller properties from flash
   for(byte_t sceneControllerIndex = 0; sceneControllerIndex < arraySize; sceneControllerIndex++)
   {
      nonvolatile_scene_controller_properties * nvSceneControllerPropertiesPtr = GetNVSceneControllerPropertiesPtr(sceneControllerIndex, true);

      if(nvSceneControllerPropertiesPtr)
      {
         memcpy(&nvSceneControllerArray[sceneControllerIndex], nvSceneControllerPropertiesPtr, sizeof(nonvolatile_scene_controller_properties));
      }
      else
      {
         memset(&nvSceneControllerArray[sceneControllerIndex], 0xFF, sizeof(nonvolatile_scene_controller_properties));
      }
   }
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void WriteAllSceneControllerPropertiesToFlash(nonvolatile_scene_controller_properties nvSceneControllerArray[], size_t arraySize)
{
   // Write the scene controller properties to flash
   for(byte_t sceneControllerIndex = 0; sceneControllerIndex < arraySize; sceneControllerIndex++)
   {
      WriteSceneControllerPropertiesToFlash(sceneControllerIndex, &nvSceneControllerArray[sceneControllerIndex]);
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void EraseSceneControllersFromFlash()
{
   // Erase the scene controller properties sector
   _int_disable();
   FlashEraseSector(&ftfl_cfg, (uint32_t)LCM_DATA_SCENE_CONTROLLERS_FLASH_START_ADDRESS, 0x1000, FlashCommandSequence);
   _int_enable();
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#ifdef SHADE_CONTROL_ADDED

void ReadAllShadeIdsFromFlash(void)
{
   byte_t * dataPtr = NULL;
   word_t   calculatedCRCValue = 0xFFFF;
   word_t   storedCRCValue;

   dataPtr = (byte_t *) LCM_DATA_SHADE_ID_START_ADDRESS;

   // Verify that the CRC is valid
   HighByteOfWord(storedCRCValue) = *dataPtr++;
   LowByteOfWord(storedCRCValue) = *dataPtr++;

   // Add 6 to the data Ptr to move past the pad bytes after the CRC
   dataPtr = dataPtr + 6;

   // Calculate the CRC for the zone data
   calculatedCRCValue = UpdateCRC16(calculatedCRCValue, dataPtr, sizeof(shadeInfoStruct));

   if (storedCRCValue == calculatedCRCValue)
   {
      memcpy(&shadeInfoStruct, (byte_t *) dataPtr, sizeof(shadeInfoStruct));
   }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

uint32_t WriteAllShadeIdsToFlash(void)
{
   uint32_t returnCode = 0xFFFFFFFF;
   byte_t * dataPtr = NULL;
   word_t   calculatedCRCValue = 0xFFFF;
   word_t   numberOfBytesToWrite;
   byte_t   crcArray[8];

   calculatedCRCValue = UpdateCRC16(calculatedCRCValue, (byte_t *) &shadeInfoStruct, sizeof(shadeInfoStruct));

   // Write the CRC bytes to flash
   memset(crcArray, 0xFF, sizeof(crcArray));
   crcArray[0] = HighByteOfWord(calculatedCRCValue);
   crcArray[1] = LowByteOfWord(calculatedCRCValue);
   numberOfBytesToWrite = flash_sizeof(sizeof(crcArray));

   dataPtr = (byte_t *) LCM_DATA_SHADE_ID_START_ADDRESS;

   // Disable interrupts while writing to flash
   _int_disable();

   FlashEraseSector(&ftfl_cfg, (uint32_t) dataPtr, 0x1000, FlashCommandSequence);

   // save the CRC
   returnCode = FlashProgramPhrase(&ftfl_cfg, (uint32_t) dataPtr, numberOfBytesToWrite, (uint32_t) crcArray, FlashCommandSequence);

   // Add 8 to the data Ptr to move past the CRC and the pad bytes after the CRC
   dataPtr = dataPtr + 8;

   // Get the size of the flash bytes to write
   numberOfBytesToWrite = flash_sizeof(sizeof(shadeInfoStruct));
   returnCode = FlashProgramPhrase(&ftfl_cfg, (uint32_t) dataPtr, numberOfBytesToWrite, (uint32_t) &shadeInfoStruct, FlashCommandSequence);

   // Re-enable interrupts
   _int_enable();

   return returnCode;

}

#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


/* The FBK (Flash BanK) group of functions manage alternating banks of flash
 * memory 4KB each in order to overcome block size and alignment restrictions
 * that would normally apply to writing flash memory.  8192B of flash provide
 * 4088B effective storage.
 */


// Read both banks, determine if they're valid and which is newer.
uint32_t FBK_Init(FBK_Context *ctx)
{
       int x=0;
       DBGPRINT_INTSTRSTR(__LINE__, "\nFBK_Init()", "");
       ctx->active_bank = 0;

       for(x=0; x<2; x++)
       {
               if(0xFFFF ^ ctx->bank_info[x]->series ^ ctx->bank_info[x]->check_word)
               {
                       DBGPRINT_INTSTRSTR(x, "FBK_Init()", "Bad check word on bank:");
                       ctx->active_bank = !x;
                       return 0;
               }
               DBGPRINT_INTSTRSTR(ctx->active_bank, "FBK_Init()", "Selected bank:");
       }

       if( (uint16_t)(ctx->bank_info[1]->series - ctx->bank_info[0]->series) == 1)
       {
               // Both banks have a valid signature and
               // bank 1 is newer than bank 0.
               ctx->active_bank = 1;
       }

       DBGPRINT_INTSTRSTR(ctx->active_bank, "FBK_Init()",  "Selected bank:");
       return 0;
}


// Write an array of data to flash.  dst is an offset relative to the start of a
// 4KB bank.  src can point to flash or RAM.  There are no alignment or size
// restrictions other than the destination and length must not overrun the bank:
//   dst+len < 4096
uint32_t FBK_WriteFlash(FBK_Context *ctx, char* src, uint32_t dst, uint32_t len)
{
       int32_t x=0;
       int32_t write_idx=0;
       char* old_block=0;
       char* new_block=0;
       char buffer[FBK_BLOCK_SIZE];
       int32_t block_idx=0;
       FBK_Bank_Info *info_ptr=0;
       uint32_t ret=0;

       if(dst + len >= FBK_BANK_SIZE)
       {
               return -1;
       }

       _mutex_lock(&ZoneArrayMutex);

       old_block = ctx->bank_info[ctx->active_bank]->data;
       new_block = ctx->bank_info[!ctx->active_bank]->data;

       DBGPRINT_INTSTRSTR(!ctx->active_bank, "FBK_WriteFlash()", "Writing to bank:");
       DBGPRINT_INTSTRSTR((int)new_block, "FBK_WriteFlash()", "Erase 4KB block at:");

       _int_disable();
       ret = FlashEraseSector(&ftfl_cfg, (uint32_t)new_block, FBK_BANK_SIZE, FlashCommandSequence);
       _int_enable();
       DBGPRINT_INTSTRSTR(ret, "FBK_WriteFlash()", "FlashEraseSector() returns:");

       while(x<FBK_BANK_SIZE)
       {
               for(block_idx=0; block_idx<FBK_BLOCK_SIZE; block_idx++)
               {
                       if( (x+block_idx)>=dst && (x+block_idx)<(dst+len) )
                       {
                               buffer[block_idx] = *(src++);
                               old_block++;
                       }
                       else
                       {
                               buffer[block_idx] = *(old_block++);
                       }
               }

               x+=FBK_BLOCK_SIZE;

               // Overwrite the buffer contents with FBK_Bank_Info fields
               if(x == FBK_BANK_SIZE)
               {
                       // Align the end of the info_ptr struct with the end of the buffer
                       info_ptr = (FBK_Bank_Info*)(&buffer[FBK_BLOCK_SIZE-FBK_INFO_SIZE] - sizeof(info_ptr->data ));

                       // Advance the series and generate a new check word
                       info_ptr->series = ctx->bank_info[ctx->active_bank]->series + 1;
                       info_ptr->check_word = info_ptr->series ^ 0xFFFF;

                       DBGPRINT_INTSTRSTR(info_ptr->series, "FBK_WriteFlash()", "New block series:");
               }

               _int_disable();
               ret = FlashProgramPhrase(&ftfl_cfg, (uint32_t)new_block, FBK_BLOCK_SIZE, (uint32_t)&buffer, FlashCommandSequence);
               _int_enable();

               if(ret)
               {
                       DBGPRINT_INTSTRSTR(ret, "FBK_WriteFlash()", "FlashProgramPhrase returns:");
               }

               new_block += FBK_BLOCK_SIZE;
       }

       ctx->active_bank = !ctx->active_bank;  // Swap banks

       DBGPRINT_INTSTRSTR(ctx->active_bank, "FBK_WriteFlash()", "Activated bank:");
       DBGPRINT_INTSTRSTR(ctx->bank_info[ctx->active_bank]->series, "FBK_WriteFlash()", "Series:");

       _mutex_unlock(&ZoneArrayMutex);

       return 0;
}



// Given an offset value of 0-4095B, this function returns the corresponding
// address in flash depending on the active bank.
char* FBK_Address(FBK_Context *ctx, uint32_t offset)
{
       char* ret = 0;
       ret = ctx->bank_info[ctx->active_bank & FBK_BANK_SELECT_MASK]->data;
       ret += offset;
       return ret;
}


