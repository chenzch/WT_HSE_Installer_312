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
#include "mu.h"

extern uint32_t __HSE_BIN_START;

bool MU_GetHseStatus(void) {
    if (IP_MU_0__MUB->FSR & MU_FSR_F24_MASK) {
        return true;
    } else {
        return false;
    }
}

static hseSrvResponse_t GetAttr(hseAttrId_t attrId, uint32_t attrLen, void *pAttr);
static hseSrvResponse_t HSE_Send(hseSrvDescriptor_t* pHseSrvDesc);
static hseSrvDescriptor_t hseSrvDescMu0;

hseSrvResponse_t TrigUpdateHSEFW(void) {

    hseSrvDescriptor_t* pHseSrvDesc = NULL;

    pHseSrvDesc = &hseSrvDescMu0;
    memset(pHseSrvDesc, 0, sizeof(hseSrvDescriptor_t));

	pHseSrvDesc->srvId = HSE_SRV_ID_FIRMWARE_UPDATE;
    pHseSrvDesc->hseSrv.firmwareUpdateReq.accessMode = HSE_ACCESS_MODE_ONE_PASS;
    pHseSrvDesc->hseSrv.firmwareUpdateReq.pInFwFile  = (uint32_t)&__HSE_BIN_START;

    return HSE_Send(pHseSrvDesc);
}

hseSrvResponse_t HSE_GetVersion(hseAttrFwVersion_t *pHseFwVersion) {
    return GetAttr(HSE_FW_VERSION_ATTR_ID, sizeof(hseAttrFwVersion_t), pHseFwVersion);
}

hseSrvResponse_t HSE_Send(hseSrvDescriptor_t* pHseSrvDesc) {
    uint32_t u32TimeOutCount;

    IP_MU_0__MUB->TR[0] = (uintptr_t)pHseSrvDesc;

    u32TimeOutCount = -1UL;

    /* Wait until HSE process the request and send the response */
    while((0UL < u32TimeOutCount--) && (0 == (IP_MU_0__MUB->RSR & MU_RSR_RF0_MASK)));

    /* Response received if TIMEOUT did not occur */
    if(u32TimeOutCount > 0UL)
    {
        /* Get HSE response */
        hseSrvResponse_t u32HseMuResponse = IP_MU_0__MUB->RR[0];
        while (IP_MU_0__MUB->FSR & MU_FSR_F0_MASK);
        return u32HseMuResponse;
    }
    else {
        return HSE_SRV_RSP_GENERAL_ERROR;
    }
}

hseSrvResponse_t GetAttr(hseAttrId_t attrId, uint32_t attrLen, void *pAttr) {
    hseSrvDescriptor_t* pHseSrvDesc = &hseSrvDescMu0;
    memset(pHseSrvDesc, 0, sizeof(hseSrvDescriptor_t));

    pHseSrvDesc->srvId                      = HSE_SRV_ID_GET_ATTR;
    pHseSrvDesc->hseSrv.getAttrReq.attrId   = attrId;
    pHseSrvDesc->hseSrv.getAttrReq.attrLen  = attrLen;
    pHseSrvDesc->hseSrv.getAttrReq.pAttr    = (HOST_ADDR)pAttr;

    return HSE_Send(pHseSrvDesc);
}
