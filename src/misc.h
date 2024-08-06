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
#if !defined(MISC_H)
#define MISC_H (1)

#if defined(__cplusplus)
extern "C" {
#endif

void FunctionalReset(void);

void WaitForWFI(void);

bool checkHseFwFeatureFlagEnabled(void);

bool EnableHseFeature(void);

bool IsPOR(void);

extern uint32_t __SRAM_STATUS_START;

#define RAM_STATUS_UNKNWON  (0x0)
#define RAM_STATUS_UTEST_OK (0x1)
#define RAM_STATUS_FW_INIT_OK    (0x2)
#define RAM_STATUS_START_MU_INSTALL (0x3)

typedef struct union {
    uint64_t raw[2];
    struct data {
        uint32_t status;
    };
} RAM_Status, *LPRAM_Status;

#define Status_Data (((*LPRAM_Status)&__SRAM_STATUS_START)->data)

#if defined(__cplusplus)
}
#endif

#endif // MISC_H
