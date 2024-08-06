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
#include "device.h"
#include "misc.h"
#include "mu.h"


int main(void) {

	/* Status variable for flash interface */
	hseAttrFwVersion_t gHseFwVersion = {0U};

    WaitForWFI();

    if (IsPOR()) {
        ((*LPRAM_Status)&__SRAM_STATUS_START)->raw[0] = 0;
        ((*LPRAM_Status)&__SRAM_STATUS_START)->raw[1] = 0;
        Status_Data.Data = RAM_STATUS_UNKNWON;
    }

    switch (Status_Data.Data) {
        case RAM_STATUS_UNKNWON:
            if(checkHseFwFeatureFlagEnabled() || EnableHseFeature()) {
                Status_Data.Data = RAM_STATUS_UTEST_OK;
            }
            FunctionalReset();
            break;
        case RAM_STATUS_UTEST_OK:
            {
                uint32_t WaitCount = 8000000U;
                while(!MU_GetHseStatus()) {
                    if (--WaitCount == 0) {
                        Status_Data.Data = RAM_STATUS_START_MU_INSTALL;
                        FunctionalReset();
                    }
                }
                Status_Data.Data = RAM_STATUS_FW_INIT_OK;
            }
            break;
        default:
            Status_Data.Data = RAM_STATUS_UNKNWON;
            FunctionalReset();
            break;
    }

    hseAttrFwVersion_t CurrVersion = { HSE_FW_VERSION };
    if (HSE_SRV_RSP_OK != HSE_GetVersion(&gHseFwVersion)) {
        for(;;);
    }
    if((gHseFwVersion.socTypeId == CurrVersion.socTypeId) &&
       (
        (gHseFwVersion.majorVersion != CurrVersion.majorVersion) ||
        (gHseFwVersion.minorVersion != CurrVersion.minorVersion) ||
        (gHseFwVersion.patchVersion != CurrVersion.patchVersion)
       )
    ) {
    	if (HSE_SRV_RSP_OK == TrigUpdateHSEFW()) {
    	    FunctionalReset();
    	}
    }
    return 0;
}
