/*
 * File:		swap_demo.h
 * Purpose:		Main process
 *
 */

#ifndef _SWAPDEMO_H_
#define _SWAPDEMO_H_

#include "SSD_Types.h"
#include "SSD_FTFx.h"
#include "SSD_FTFx_Internal.h"

#ifdef K60DN512 
#define FLASH_SWAP_INDICATOR_ADDR  0x3F800  //last sector of lower block

#define PFLASH_BLOCK0_BASE  0x00000000
#define PFLASH_BLOCK1_BASE  0x00040000
#define PFLASH_BLOCK_SIZE  0x00040000

#define READ_NORMAL_MARGIN        0x00
#define READ_USER_MARGIN          0x01
#define READ_FACTORY_MARGIN       0x02

#define EE_ENABLE                 0x00
#define RAM_ENABLE                0xFF
#define DEBUGENABLE               0x00
#define PFLASH_IFR_ADDR           0x00000000
#define DFLASH_IFR_ADDR           0x00800000

#define SECTOR_SIZE               0x00000800 /* 2 KB size */
#define BUFFER_SIZE_BYTE          0x10

/* FTFL module base */
#define FTFx_REG_BASE           0x40020000
#define DFLASH_IFR_BASE         0xFFFFFFFF      /* unused */
#define PFLASH_BLOCK_BASE       0x00000000
#define DEFLASH_BLOCK_BASE      0xFFFFFFFF      /* There is not DFlash */
#define EERAM_BLOCK_BASE        0x14000000

#define PBLOCK_SIZE             0x00080000      /* 512 KB size */
#define EERAM_BLOCK_SIZE        0x00001000      /* 4 KB size */

#else

#define FLASH_SWAP_INDICATOR_ADDR  0x7F000  //last sector of lower block

#define PFLASH_BLOCK0_BASE  0x00000000
#define PFLASH_BLOCK1_BASE  0x00080000
#define PFLASH_BLOCK_SIZE  0x00080000

#define READ_NORMAL_MARGIN        0x00
#define READ_USER_MARGIN          0x01
#define READ_FACTORY_MARGIN       0x02

#define EE_ENABLE                 0x00
#define RAM_ENABLE                0xFF
#define DEBUGENABLE               0x00
#define PFLASH_IFR_ADDR           0x00000000
#define DFLASH_IFR_ADDR           0x00800000

#define SECTOR_SIZE               0x00001000 /* 4 KB size */
#define BUFFER_SIZE_BYTE          0x10

/* FTFL module base */
#define FTFx_REG_BASE           0x40020000
#define DFLASH_IFR_BASE         0xFFFFFFFF      /* unused */
#define PFLASH_BLOCK_BASE       0x00000000
#define DEFLASH_BLOCK_BASE      0xFFFFFFFF      /* There is not DFlash */
#define EERAM_BLOCK_BASE        0x14000000

#define PBLOCK_SIZE             0x00100000      /* 1024 KB size */
#define EERAM_BLOCK_SIZE        0x00001000      /* 4 KB size */

#endif

#define pFlashInit                      (FlashInit)
#define pDEFlashPartition               (DEFlashPartition)
#define pSetEEEEnable                   (SetEEEEnable)
#define pPFlashSetProtection            (PFlashSetProtection)
#define pPFlashGetProtection            (PFlashGetProtection)
#define pFlashVerifySection             (FlashVerifySection)
#define pFlashVerifyBlock               (FlashVerifyBlock)
#define pFlashVerifyAllBlock            (FlashVerifyAllBlock)
#define pFlashSetInterruptEnable        (FlashSetInterruptEnable)
#define pFlashSecurityBypass            (FlashSecurityBypass)
#define pFlashReadResource              (FlashReadResource)
#define pFlashReadOnce                  (FlashReadOnce)
#define pFlashProgramSection            (FlashProgramSection)
#define pFlashProgramOnce               (FlashProgramOnce)
#define pFlashProgramLongword           (FlashProgramLongword)
#define pFlashProgramCheck              (FlashProgramCheck)
#define pFlashGetSecurityState          (FlashGetSecurityState)
#define pFlashGetInterruptEnable        (FlashGetInterruptEnable)
#define pFlashEraseSuspend              (FlashEraseSuspend)
#define pFlashEraseBlock                (FlashEraseBlock)
#define pFlashEraseAllBlock             (FlashEraseAllBlock)
#define pFlashCommandSequence           (FlashCommandSequence)
#define pFlashCheckSum                  (FlashCheckSum)
#define pEERAMSetProtection             (EERAMSetProtection)
#define pEERAMGetProtection             (EERAMGetProtection)
#define pEEEWrite                       (EEEWrite)
#define pDFlashSetProtection            (DFlashSetProtection)
#define pDFlashGetProtection            (DFlashGetProtection)
#define pFlashEraseResume               (FlashEraseResume)
#define pFlashEraseSector               (FlashEraseSector)
#define pPFlashGetSwapStatus            (PFlashGetSwapStatus)
#define pPFlashSwap                     (PFlashSwap)

void ErrorTrap(UINT32 returnCode);

#endif /* _ISRDEMO_H_ */
