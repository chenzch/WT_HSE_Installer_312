/* Copyright 2023 NXP */
/* License: BSD 3-clause
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the
       names of its contributors may be used to endorse or promote products
       derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * main implementation: use this 'C' sample to create your own application
 *
 */
#include "S32K312.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmsis_gcc.h>

#include <hse_interface.h>

#include <S32K312_MU.h>
#include <S32K312_MC_ME.h>

#define UTEST_BASE_ADDRESS             (0x1B000000UL)
#define HSE_STATUS_INIT_OK 	            ((hseStatus_t)1U << 8U)
#define MU_IS_BIT_SET(value, i)        (0U != ((value) & (0x1U << i)))

const MU_MemMapPtr pMU[MU_INSTANCE_COUNT] = IP_MU_BASE_PTRS;

extern uint32_t __HSE_BIN_START;

typedef void (*pfAsyncCallback_t) (hseSrvResponse_t status, void* pArg);

static bool checkHseFwFeatureFlagEnabled(void);
static uint16_t HSE_MU_GetHseStatus( uint8_t u8MuInstance );
static hseSrvResponse_t HSE_GetVersion_Example( hseAttrFwVersion_t *pHseFwVersion );
static hseSrvResponse_t TrigUpdateHSEFW(uint32_t storeHseFWAddr, hseAccessMode_t updateMode);
static hseSrvResponse_t GetAttr ( hseAttrId_t attrId, uint32_t attrLen, void *pAttr );
static hseSrvResponse_t HSE_Send(uint8_t u8MuInstance, uint8_t u8MuChannel, hseSrvDescriptor_t* pHseSrvDesc);
static hseSrvResponse_t HSE_MU_ReceiveResponseBlocking( uint8_t u8MuInstance, uint8_t u8Channel );
static bool HSE_MU_IsResponseReady(uint8_t u8MuInstance, uint8_t u8Channel);
static uint32_t HSE_MU_ReceiveResponse(uint8_t u8MuInstance, uint8_t u8Channel);
static void Reset(void);

static hseSrvDescriptor_t hseSrvDescMu0[HSE_NUM_OF_CHANNELS_PER_MU];
static hseSrvDescriptor_t hseSrvDescMu1[HSE_NUM_OF_CHANNELS_PER_MU];

hseSrvDescriptor_t* const gHseSrvDesc[MU_INSTANCE_COUNT] =
{
    hseSrvDescMu0,
    hseSrvDescMu1,
};

typedef struct
{
	hseSrvResponse_t response;                 /**< @brief    Service response written in the callback (in case of sync with interrupt mode). */
} hseCallbackInfo_t;

static uint8_t volatile hseBusyChannel[MU_INSTANCE_COUNT][HSE_NUM_OF_CHANNELS_PER_MU];
static volatile hseCallbackInfo_t hseCallbackInfo[MU_INSTANCE_COUNT][HSE_NUM_OF_CHANNELS_PER_MU];

int main(void) {

	/* Status variable for flash interface */
	hseSrvResponse_t hseSrvResponse;
	hseAttrFwVersion_t gHseFwVersion = {0U};
    uint32_t WaitCount = 8000000;

    while(0 == (IP_MC_ME->PRTN0_CORE2_STAT & MC_ME_PRTN0_CORE0_STAT_WFI_MASK))
    {

    }

    /* Check if HSE FW usage flag is already enabled. Otherwise program the flag */
    if(false == checkHseFwFeatureFlagEnabled())
    {
#if 0
    	  /* unlock UTEST data flash sector */
    	  PFLASH_Unlock (PFLASH_BL5, PFLASH_SS0, PFLASH_S0);

    	  /* Write in UtestSector using main interface */
    	  status = FLASH_Write ((uint32_t*)UTEST_BASE_ADDRESS,
    			  	  	  	  	  	  	   hseFwFeatureFlagEnabledValue,
    	                                   sizeof(hseFwFeatureFlagEnabledValue));
#endif

    	  while(1)
    	  {
    		  /*HSE usage not enable & have not HSE FW*/
    	  }
    }

    /* Wait for HSE to initialize(read status bits) after installation */
    while((HSE_STATUS_INIT_OK & HSE_MU_GetHseStatus(0)) == 0)
    {
        if (--WaitCount == 0) {
            Reset();
        }
    }

    (void)HSE_GetVersion_Example(&gHseFwVersion);
    if((gHseFwVersion.majorVersion != 1) && (gHseFwVersion.minorVersion != 1))
    {
    	hseSrvResponse = TrigUpdateHSEFW((uint32_t)&__HSE_BIN_START, HSE_ACCESS_MODE_ONE_PASS);
    	if (HSE_SRV_RSP_OK == hseSrvResponse) {
    		for (;;);
    	}
    }

    for (;;);

    return 0;
}

/******************************************************************************
 * Function:    checkHseFwFeatureFlagEnabled
 * Description: Verifies whether hse fw feature flag is already enabled or not
 *****************************************************************************/
bool checkHseFwFeatureFlagEnabled(void)
{
    uint64_t default_val = 0xFFFFFFFFFFFFFFFFUL;
    uint64_t hsefwfeatureflag = *(uint64_t*)(UTEST_BASE_ADDRESS);

    //check the default value
    if(0 != memcmp((void *)&hsefwfeatureflag, (void *)&default_val, 0x8U)) {
        return true;
    }
    else {
        return false;
    }
}

/* Base addresses for each MU instance */
const uint32_t u32BaseAddr[MU_INSTANCE_COUNT] = IP_MU_BASE_ADDRS;

uint16_t HSE_MU_GetHseStatus(uint8_t u8MuInstance)
{
    uint32_t u32FlagStatusReg = 0UL;

    /* Get MU_FSR (MU Flag Status register) and returns HSE status (16 MSB) */
    u32FlagStatusReg = pMU[u8MuInstance]->FSR;
    u32FlagStatusReg = ((u32FlagStatusReg & 0xFFFF0000UL) >> 16U);

    return (uint16_t)u32FlagStatusReg;
}


/******************************************************************************
 * Function:    HSE_GetVersion_Example
 * Description: Example of HSE service - get FW version
******************************************************************************/
hseSrvResponse_t HSE_GetVersion_Example( hseAttrFwVersion_t *pHseFwVersion )
{
    hseSrvResponse_t srvResponse;
    srvResponse = GetAttr(
            HSE_FW_VERSION_ATTR_ID,
            sizeof(hseAttrFwVersion_t),
            pHseFwVersion );
    if (HSE_SRV_RSP_OK != srvResponse) {
    	for(;;);
    }
    return srvResponse;
}

static const uint8_t u8MuInstance = 0;
static const uint8_t u8MuChannel = 0;

hseSrvResponse_t TrigUpdateHSEFW(uint32_t storeHseFWAddr,
								 hseAccessMode_t updateMode)
{
    hseSrvResponse_t hseSrvResponse = HSE_SRV_RSP_GENERAL_ERROR;
    hseSrvDescriptor_t* pHseSrvDesc = NULL;

    pHseSrvDesc = &gHseSrvDesc[u8MuInstance][u8MuChannel];
    memset(pHseSrvDesc, 0, sizeof(hseSrvDescriptor_t));

	pHseSrvDesc->srvId = HSE_SRV_ID_FIRMWARE_UPDATE;
    pHseSrvDesc->hseSrv.firmwareUpdateReq.accessMode   = updateMode;
    pHseSrvDesc->hseSrv.firmwareUpdateReq.pInFwFile  = storeHseFWAddr;

    hseSrvResponse = HSE_Send(u8MuInstance, u8MuChannel, pHseSrvDesc);
    return hseSrvResponse;
}

hseSrvResponse_t GetAttr
(
    hseAttrId_t attrId,
    uint32_t attrLen,
    void *pAttr
)
{
    hseSrvResponse_t hseSrvResponse = HSE_SRV_RSP_GENERAL_ERROR;
    hseSrvDescriptor_t* pHseSrvDesc = &gHseSrvDesc[u8MuInstance][u8MuChannel];

    memset(pHseSrvDesc, 0, sizeof(hseSrvDescriptor_t));

    pHseSrvDesc->srvId                      = HSE_SRV_ID_GET_ATTR;
    pHseSrvDesc->hseSrv.getAttrReq.attrId   = attrId;
    pHseSrvDesc->hseSrv.getAttrReq.attrLen  = attrLen;
    pHseSrvDesc->hseSrv.getAttrReq.pAttr    = (HOST_ADDR)pAttr;

    hseSrvResponse = HSE_Send(u8MuInstance, u8MuChannel, pHseSrvDesc);
    return hseSrvResponse;
}

hseSrvResponse_t HSE_Send(uint8_t u8MuInstance, uint8_t u8MuChannel, hseSrvDescriptor_t* pHseSrvDesc)
{
    hseSrvResponse_t srvResponse;

    /* The MU instance and channel assumed to be in range */
    /* Assuming the channel is free */
	/*This logic is added to avoid the syncronization issue. HOst should send the next command
	when receive buffer is full and channel busy status is clear */

    /* Send the request sync/async */
	/* Check whther interrupts are enabled */
    pMU[u8MuInstance]->TR[u8MuChannel] = (uintptr_t )pHseSrvDesc;
    srvResponse = HSE_MU_ReceiveResponseBlocking(u8MuInstance, u8MuChannel);

    return srvResponse;
}

hseSrvResponse_t HSE_MU_ReceiveResponseBlocking( uint8_t u8MuInstance, uint8_t u8Channel )
{
	#define HSE_MU_TIMEOUT_DURATION ((uint32_t)0xFFFFFFFFUL)
	#define HSE_SRV_RSP_GENERAL_ERROR ((hseSrvResponse_t)0x33D6D4F1UL)

    uint32_t u32TimeOutCount = HSE_MU_TIMEOUT_DURATION;
    hseSrvResponse_t u32HseMuResponse = HSE_SRV_RSP_GENERAL_ERROR;

    /* Wait until HSE process the request and send the response */
    while((0UL < u32TimeOutCount) && (false ==  HSE_MU_IsResponseReady(u8MuInstance, u8Channel)))
    {
        u32TimeOutCount--;
    }

    /* Response received if TIMEOUT did not occur */
    if(u32TimeOutCount > 0UL)
    {
        /* Get HSE response */
        u32HseMuResponse = HSE_MU_ReceiveResponse(u8MuInstance, u8Channel);
        while (
            (0U != ((pMU[u8MuInstance]->FSR) & (0x1U << (u8Channel))))
        ) {

        }
        // while(MU_IS_BIT_SET(pMU[u8MuInstance]->FSR, u8Channel)) {};
    }

    return u32HseMuResponse;
}

bool HSE_MU_IsResponseReady(uint8_t u8MuInstance, uint8_t u8Channel)
{
    if ((0U != ((pMU[u8MuInstance]->RSR) & (0x1U << (u8Channel))))) {
        return true;
    } else {
        return false;
    }
    // return (MU_IS_BIT_SET(pMU[u8MuInstance]->RSR, u8Channel));
}

uint32_t HSE_MU_ReceiveResponse(uint8_t u8MuInstance, uint8_t u8Channel)
{
    uint32_t hseSrvResponse = pMU[u8MuInstance]->RR[u8Channel];
    return hseSrvResponse;
}

void Reset(void) {

#define MC_ME_CTL_KEY_DIRECT_KEY_U32                    ((uint32_t)0x00005AF0U)
#define MC_ME_CTL_KEY_INVERTED_KEY_U32                  ((uint32_t)0x0000A50FU)

	IP_MC_ME->MODE_CONF = MC_ME_MODE_CONF_FUNC_RST(1);
	IP_MC_ME->MODE_UPD = MC_ME_MODE_UPD_MODE_UPD(1);
    IP_MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY(MC_ME_CTL_KEY_DIRECT_KEY_U32);
    IP_MC_ME->CTL_KEY = MC_ME_CTL_KEY_KEY(MC_ME_CTL_KEY_INVERTED_KEY_U32);
}
