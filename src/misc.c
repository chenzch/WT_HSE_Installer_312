//---------------------------------------------------------------------------------------------------------------------
//
// WTMEC CORPORATION CONFIDENTIAL
// ________________________________
//
// [2024] Wtmec Corporation
// All Rights Reserved.
//
// NOTICE: This is an unpublished work of authorship, which contains trade secrets.
// Wtmec Corporation owns all rights to this work and intends to maintain it in confidence to
// preserve its trade secret status. Wtmec Corporation reserves the right, under the copyright
// laws of the United States or those of any other country that may have jurisdiction, to protect
// this work as an unpublished work, in the event of an inadvertent or deliberate unauthorized
// publication. Wtmec Corporation also reserves its rights under all copyright laws to protect
// this work as a published work, when appropriate. Those having access to this work may not copy
// it, use it, modify it, or disclose the information contained in it without the written
// authorization of Wtmec Corporation.
//
//---------------------------------------------------------------------------------------------------------------------
#include "device.h"
#include "misc.h"

#define UTEST_BASE_ADDR (0x1B000000UL)

void FunctionalReset(void) {

    __asm("dsb");
    __asm("isb");

	IP_MC_ME->MODE_CONF = MC_ME_MODE_CONF_FUNC_RST(1);
	IP_MC_ME->MODE_UPD = MC_ME_MODE_UPD_MODE_UPD(1);
    IP_MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY((uint32_t)0x00005AF0U);
    IP_MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY((uint32_t)0x0000A50FU);
}

bool checkHseFwFeatureFlagEnabled(void) {
    uint64_t default_val = 0xFFFFFFFFFFFFFFFFUL;
    uint64_t hsefwfeatureflag = *(uint64_t*)UTEST_BASE_ADDR;

    //check the default value
    if(0 != memcmp((void *)&hsefwfeatureflag, (void *)&default_val, 0x8U)) {
        return true;
    } else {
        return false;
    }
}

bool EnableHseFeature(void) {
    register uint8_t domain_id = IP_XRDC->HWCFG1 & 0xFF;

    uint32_t status;

    /* unlock UTEST data flash sector */
    IP_PFLASH->PFCBLKU_SPELOCK[0] &= ~1U;

    /* entry semaphore loop */
    do {
        IP_PFLASH->PFCPGM_PEADR_L = (uint32_t)UTEST_BASE_ADDR;
    } while(((IP_FLASH->MCR & FLASH_MCR_PEID_MASK) >> FLASH_MCR_PEID_SHIFT) != domain_id);

    /* clear any pending program & erase errors                           */
    IP_FLASH->MCRS = FLASH_MCRS_PEP_MASK | FLASH_MCRS_PES_MASK;

    IP_FLASH->DATA[0] = 0xDDCCBBAA;
    IP_FLASH->DATA[1] = 0xAABBCCDD;

    IP_FLASH->MCR |= FLASH_MCR_PGM_MASK;             /* initiate sequence             */
    IP_FLASH->MCR |= FLASH_MCR_EHV_MASK;             /* enable high voltage           */
    while(!(IP_FLASH->MCRS & FLASH_MCRS_DONE_MASK)); /* wait until MCRS[DONE]=1       */
    IP_FLASH->MCR &= ~FLASH_MCR_EHV_MASK;            /* disable high voltage          */
    status = IP_FLASH->MCRS;                         /* read main interface status    */
    IP_FLASH->MCR &= ~FLASH_MCR_PGM_MASK;            /* close sequence                */

    if (status & FLASH_MCRS_PEG_MASK) {
        return true;
    } else {
        return false;
    }
}

bool IsPOR(void) {
    if (IP_MC_RGM->DES & MC_RGM_DES_F_POR_MASK) {
    	IP_MC_RGM->DES = MC_RGM_DES_F_POR_MASK;
        return true;
    } else {
        return false;
    }
}

bool CheckSBAF(uint8_t socType) {
/*
 * 05 - S32K344, S32K324, S32K314
 * 0C - S32K311, S32K310
 * 0D - S32K312, S32K342, S32K322, S32K341
 * 0E - S32K358, S32K348, S32K338, S32K328, S32K336, S32K356
 * 0F - S32K396, S32K376, S32K394, S32K374
 * 10 - S32K388
 */
    bool ret = false;
    uint64_t SBAFVer = *(uint64_t*)0x4039C020;
    switch (socType)
    {
    case 0x05: // S32K344, S32K324, S32K314
        if (
            (SBAFVer == (0x0004090000000500UL)) ||
            (SBAFVer == (0x03000A0000000500UL))
        ) {
            ret = true;
        }
        break;
    case 0x0D: // S32K312, S32K342, S32K322, S32K341
        if (
            (SBAFVer == (0x0000080000000D00UL)) ||
            (SBAFVer == (0x0100090000000D00UL))
        ) {
            ret = true;
        }
        break;
    case 0x0E: // S32K358
        if (
            (SBAFVer == (0x03000C0000000E00UL))
        ) {
            ret = true;
        }
        break;
    default:
        break;
    }
    return ret;
}
