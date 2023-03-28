/**
 * Copyright (c) 2020 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/*!
 * @brief     Simple, efficient, PCG configuration routine
 *
 *   This PCG driver supports:
 *    - Multiple input clock configurations
 *    - External frame synchronization
 *
 * @file      pcg_simple.c
 * @version   1.0.0
 * @copyright 2020 Analog Devices, Inc.  All rights reserved.
 *
*/
#include <assert.h>

#include <sys/adi_core.h>
#include <sru.h>

#include "pcg_simple.h"

static void pcg_route_clk( PCG_RESOURCE pcg, uint32_t pin ) {

    switch (pcg) {
        case PCG_A:
            switch(pin) {
                case 1: SRU(DAI0_PB01_O, PCG0_EXTCLKA_I); break;
                case 2: SRU(DAI0_PB02_O, PCG0_EXTCLKA_I); break;
                case 3: SRU(DAI0_PB03_O, PCG0_EXTCLKA_I); break;
                case 4: SRU(DAI0_PB04_O, PCG0_EXTCLKA_I); break;
                case 5: SRU(DAI0_PB05_O, PCG0_EXTCLKA_I); break;
                case 6: SRU(DAI0_PB06_O, PCG0_EXTCLKA_I); break;
                case 7: SRU(DAI0_PB07_O, PCG0_EXTCLKA_I); break;
                case 8: SRU(DAI0_PB08_O, PCG0_EXTCLKA_I); break;
                case 9: SRU(DAI0_PB09_O, PCG0_EXTCLKA_I); break;
                case 10: SRU(DAI0_PB10_O, PCG0_EXTCLKA_I); break;
                case 11: SRU(DAI0_PB11_O, PCG0_EXTCLKA_I); break;
                case 12: SRU(DAI0_PB12_O, PCG0_EXTCLKA_I); break;
                case 13: SRU(DAI0_PB13_O, PCG0_EXTCLKA_I); break;
                case 14: SRU(DAI0_PB14_O, PCG0_EXTCLKA_I); break;
                case 15: SRU(DAI0_PB15_O, PCG0_EXTCLKA_I); break;
                case 16: SRU(DAI0_PB16_O, PCG0_EXTCLKA_I); break;
                case 17: SRU(DAI0_PB17_O, PCG0_EXTCLKA_I); break;
                case 18: SRU(DAI0_PB18_O, PCG0_EXTCLKA_I); break;
                case 19: SRU(DAI0_PB19_O, PCG0_EXTCLKA_I); break;
                case 20: SRU(DAI0_PB20_O, PCG0_EXTCLKA_I); break;
                default: assert(0);
            }
            break;
        case PCG_B:
            switch(pin) {
                case 1: SRU(DAI0_PB01_O, PCG0_EXTCLKB_I); break;
                case 2: SRU(DAI0_PB02_O, PCG0_EXTCLKB_I); break;
                case 3: SRU(DAI0_PB03_O, PCG0_EXTCLKB_I); break;
                case 4: SRU(DAI0_PB04_O, PCG0_EXTCLKB_I); break;
                case 5: SRU(DAI0_PB05_O, PCG0_EXTCLKB_I); break;
                case 6: SRU(DAI0_PB06_O, PCG0_EXTCLKB_I); break;
                case 7: SRU(DAI0_PB07_O, PCG0_EXTCLKB_I); break;
                case 8: SRU(DAI0_PB08_O, PCG0_EXTCLKB_I); break;
                case 9: SRU(DAI0_PB09_O, PCG0_EXTCLKB_I); break;
                case 10: SRU(DAI0_PB10_O, PCG0_EXTCLKB_I); break;
                case 11: SRU(DAI0_PB11_O, PCG0_EXTCLKB_I); break;
                case 12: SRU(DAI0_PB12_O, PCG0_EXTCLKB_I); break;
                case 13: SRU(DAI0_PB13_O, PCG0_EXTCLKB_I); break;
                case 14: SRU(DAI0_PB14_O, PCG0_EXTCLKB_I); break;
                case 15: SRU(DAI0_PB15_O, PCG0_EXTCLKB_I); break;
                case 16: SRU(DAI0_PB16_O, PCG0_EXTCLKB_I); break;
                case 17: SRU(DAI0_PB17_O, PCG0_EXTCLKB_I); break;
                case 18: SRU(DAI0_PB18_O, PCG0_EXTCLKB_I); break;
                case 19: SRU(DAI0_PB19_O, PCG0_EXTCLKB_I); break;
                case 20: SRU(DAI0_PB20_O, PCG0_EXTCLKB_I); break;
                default: assert(0);
            }
            break;
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
        case PCG_C:
            switch(pin) {
                case 1: SRU2(DAI1_PB01_O, PCG0_EXTCLKC_I); break;
                case 2: SRU2(DAI1_PB02_O, PCG0_EXTCLKC_I); break;
                case 3: SRU2(DAI1_PB03_O, PCG0_EXTCLKC_I); break;
                case 4: SRU2(DAI1_PB04_O, PCG0_EXTCLKC_I); break;
                case 5: SRU2(DAI1_PB05_O, PCG0_EXTCLKC_I); break;
                case 6: SRU2(DAI1_PB06_O, PCG0_EXTCLKC_I); break;
                case 7: SRU2(DAI1_PB07_O, PCG0_EXTCLKC_I); break;
                case 8: SRU2(DAI1_PB08_O, PCG0_EXTCLKC_I); break;
                case 9: SRU2(DAI1_PB09_O, PCG0_EXTCLKC_I); break;
                case 10: SRU2(DAI1_PB10_O, PCG0_EXTCLKC_I); break;
                case 11: SRU2(DAI1_PB11_O, PCG0_EXTCLKC_I); break;
                case 12: SRU2(DAI1_PB12_O, PCG0_EXTCLKC_I); break;
                case 13: SRU2(DAI1_PB13_O, PCG0_EXTCLKC_I); break;
                case 14: SRU2(DAI1_PB14_O, PCG0_EXTCLKC_I); break;
                case 15: SRU2(DAI1_PB15_O, PCG0_EXTCLKC_I); break;
                case 16: SRU2(DAI1_PB16_O, PCG0_EXTCLKC_I); break;
                case 17: SRU2(DAI1_PB17_O, PCG0_EXTCLKC_I); break;
                case 18: SRU2(DAI1_PB18_O, PCG0_EXTCLKC_I); break;
                case 19: SRU2(DAI1_PB19_O, PCG0_EXTCLKC_I); break;
                case 20: SRU2(DAI1_PB20_O, PCG0_EXTCLKC_I); break;
                default: assert(0);
            }
            break;
        case PCG_D:
            switch(pin) {
                case 1: SRU2(DAI1_PB01_O, PCG0_EXTCLKD_I); break;
                case 2: SRU2(DAI1_PB02_O, PCG0_EXTCLKD_I); break;
                case 3: SRU2(DAI1_PB03_O, PCG0_EXTCLKD_I); break;
                case 4: SRU2(DAI1_PB04_O, PCG0_EXTCLKD_I); break;
                case 5: SRU2(DAI1_PB05_O, PCG0_EXTCLKD_I); break;
                case 6: SRU2(DAI1_PB06_O, PCG0_EXTCLKD_I); break;
                case 7: SRU2(DAI1_PB07_O, PCG0_EXTCLKD_I); break;
                case 8: SRU2(DAI1_PB08_O, PCG0_EXTCLKD_I); break;
                case 9: SRU2(DAI1_PB09_O, PCG0_EXTCLKD_I); break;
                case 10: SRU2(DAI1_PB10_O, PCG0_EXTCLKD_I); break;
                case 11: SRU2(DAI1_PB11_O, PCG0_EXTCLKD_I); break;
                case 12: SRU2(DAI1_PB12_O, PCG0_EXTCLKD_I); break;
                case 13: SRU2(DAI1_PB13_O, PCG0_EXTCLKD_I); break;
                case 14: SRU2(DAI1_PB14_O, PCG0_EXTCLKD_I); break;
                case 15: SRU2(DAI1_PB15_O, PCG0_EXTCLKD_I); break;
                case 16: SRU2(DAI1_PB16_O, PCG0_EXTCLKD_I); break;
                case 17: SRU2(DAI1_PB17_O, PCG0_EXTCLKD_I); break;
                case 18: SRU2(DAI1_PB18_O, PCG0_EXTCLKD_I); break;
                case 19: SRU2(DAI1_PB19_O, PCG0_EXTCLKD_I); break;
                case 20: SRU2(DAI1_PB20_O, PCG0_EXTCLKD_I); break;
                default: assert(0);
            }
            break;
#endif
        default:
            assert(0);
    }
}

static void pcg_route_fs( PCG_RESOURCE pcg, uint32_t pin ) {

    switch (pcg) {
        case PCG_A:
            switch(pin) {
                case 1: SRU(DAI0_PB01_O, PCG0_SYNC_CLKA_I); break;
                case 2: SRU(DAI0_PB02_O, PCG0_SYNC_CLKA_I); break;
                case 3: SRU(DAI0_PB03_O, PCG0_SYNC_CLKA_I); break;
                case 4: SRU(DAI0_PB04_O, PCG0_SYNC_CLKA_I); break;
                case 5: SRU(DAI0_PB05_O, PCG0_SYNC_CLKA_I); break;
                case 6: SRU(DAI0_PB06_O, PCG0_SYNC_CLKA_I); break;
                case 7: SRU(DAI0_PB07_O, PCG0_SYNC_CLKA_I); break;
                case 8: SRU(DAI0_PB08_O, PCG0_SYNC_CLKA_I); break;
                case 9: SRU(DAI0_PB09_O, PCG0_SYNC_CLKA_I); break;
                case 10: SRU(DAI0_PB10_O, PCG0_SYNC_CLKA_I); break;
                case 11: SRU(DAI0_PB11_O, PCG0_SYNC_CLKA_I); break;
                case 12: SRU(DAI0_PB12_O, PCG0_SYNC_CLKA_I); break;
                case 13: SRU(DAI0_PB13_O, PCG0_SYNC_CLKA_I); break;
                case 14: SRU(DAI0_PB14_O, PCG0_SYNC_CLKA_I); break;
                case 15: SRU(DAI0_PB15_O, PCG0_SYNC_CLKA_I); break;
                case 16: SRU(DAI0_PB16_O, PCG0_SYNC_CLKA_I); break;
                case 17: SRU(DAI0_PB17_O, PCG0_SYNC_CLKA_I); break;
                case 18: SRU(DAI0_PB18_O, PCG0_SYNC_CLKA_I); break;
                case 19: SRU(DAI0_PB19_O, PCG0_SYNC_CLKA_I); break;
                case 20: SRU(DAI0_PB20_O, PCG0_SYNC_CLKA_I); break;
                default: assert(0);
            }
            break;
        case PCG_B:
            switch(pin) {
                case 1: SRU(DAI0_PB01_O, PCG0_SYNC_CLKB_I); break;
                case 2: SRU(DAI0_PB02_O, PCG0_SYNC_CLKB_I); break;
                case 3: SRU(DAI0_PB03_O, PCG0_SYNC_CLKB_I); break;
                case 4: SRU(DAI0_PB04_O, PCG0_SYNC_CLKB_I); break;
                case 5: SRU(DAI0_PB05_O, PCG0_SYNC_CLKB_I); break;
                case 6: SRU(DAI0_PB06_O, PCG0_SYNC_CLKB_I); break;
                case 7: SRU(DAI0_PB07_O, PCG0_SYNC_CLKB_I); break;
                case 8: SRU(DAI0_PB08_O, PCG0_SYNC_CLKB_I); break;
                case 9: SRU(DAI0_PB09_O, PCG0_SYNC_CLKB_I); break;
                case 10: SRU(DAI0_PB10_O, PCG0_SYNC_CLKB_I); break;
                case 11: SRU(DAI0_PB11_O, PCG0_SYNC_CLKB_I); break;
                case 12: SRU(DAI0_PB12_O, PCG0_SYNC_CLKB_I); break;
                case 13: SRU(DAI0_PB13_O, PCG0_SYNC_CLKB_I); break;
                case 14: SRU(DAI0_PB14_O, PCG0_SYNC_CLKB_I); break;
                case 15: SRU(DAI0_PB15_O, PCG0_SYNC_CLKB_I); break;
                case 16: SRU(DAI0_PB16_O, PCG0_SYNC_CLKB_I); break;
                case 17: SRU(DAI0_PB17_O, PCG0_SYNC_CLKB_I); break;
                case 18: SRU(DAI0_PB18_O, PCG0_SYNC_CLKB_I); break;
                case 19: SRU(DAI0_PB19_O, PCG0_SYNC_CLKB_I); break;
                case 20: SRU(DAI0_PB20_O, PCG0_SYNC_CLKB_I); break;
                default: assert(0);
            }
            break;
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)

        case PCG_C:
            switch(pin) {
                case 1: SRU2(DAI1_PB01_O, PCG0_SYNC_CLKC_I); break;
                case 2: SRU2(DAI1_PB02_O, PCG0_SYNC_CLKC_I); break;
                case 3: SRU2(DAI1_PB03_O, PCG0_SYNC_CLKC_I); break;
                case 4: SRU2(DAI1_PB04_O, PCG0_SYNC_CLKC_I); break;
                case 5: SRU2(DAI1_PB05_O, PCG0_SYNC_CLKC_I); break;
                case 6: SRU2(DAI1_PB06_O, PCG0_SYNC_CLKC_I); break;
                case 7: SRU2(DAI1_PB07_O, PCG0_SYNC_CLKC_I); break;
                case 8: SRU2(DAI1_PB08_O, PCG0_SYNC_CLKC_I); break;
                case 9: SRU2(DAI1_PB09_O, PCG0_SYNC_CLKC_I); break;
                case 10: SRU2(DAI1_PB10_O, PCG0_SYNC_CLKC_I); break;
                case 11: SRU2(DAI1_PB11_O, PCG0_SYNC_CLKC_I); break;
                case 12: SRU2(DAI1_PB12_O, PCG0_SYNC_CLKC_I); break;
                case 13: SRU2(DAI1_PB13_O, PCG0_SYNC_CLKC_I); break;
                case 14: SRU2(DAI1_PB14_O, PCG0_SYNC_CLKC_I); break;
                case 15: SRU2(DAI1_PB15_O, PCG0_SYNC_CLKC_I); break;
                case 16: SRU2(DAI1_PB16_O, PCG0_SYNC_CLKC_I); break;
                case 17: SRU2(DAI1_PB17_O, PCG0_SYNC_CLKC_I); break;
                case 18: SRU2(DAI1_PB18_O, PCG0_SYNC_CLKC_I); break;
                case 19: SRU2(DAI1_PB19_O, PCG0_SYNC_CLKC_I); break;
                case 20: SRU2(DAI1_PB20_O, PCG0_SYNC_CLKC_I); break;
                default: assert(0);
            }
            break;
        case PCG_D:
            switch(pin) {
                case 1: SRU2(DAI1_PB01_O, PCG0_SYNC_CLKD_I); break;
                case 2: SRU2(DAI1_PB02_O, PCG0_SYNC_CLKD_I); break;
                case 3: SRU2(DAI1_PB03_O, PCG0_SYNC_CLKD_I); break;
                case 4: SRU2(DAI1_PB04_O, PCG0_SYNC_CLKD_I); break;
                case 5: SRU2(DAI1_PB05_O, PCG0_SYNC_CLKD_I); break;
                case 6: SRU2(DAI1_PB06_O, PCG0_SYNC_CLKD_I); break;
                case 7: SRU2(DAI1_PB07_O, PCG0_SYNC_CLKD_I); break;
                case 8: SRU2(DAI1_PB08_O, PCG0_SYNC_CLKD_I); break;
                case 9: SRU2(DAI1_PB09_O, PCG0_SYNC_CLKD_I); break;
                case 10: SRU2(DAI1_PB10_O, PCG0_SYNC_CLKD_I); break;
                case 11: SRU2(DAI1_PB11_O, PCG0_SYNC_CLKD_I); break;
                case 12: SRU2(DAI1_PB12_O, PCG0_SYNC_CLKD_I); break;
                case 13: SRU2(DAI1_PB13_O, PCG0_SYNC_CLKD_I); break;
                case 14: SRU2(DAI1_PB14_O, PCG0_SYNC_CLKD_I); break;
                case 15: SRU2(DAI1_PB15_O, PCG0_SYNC_CLKD_I); break;
                case 16: SRU2(DAI1_PB16_O, PCG0_SYNC_CLKD_I); break;
                case 17: SRU2(DAI1_PB17_O, PCG0_SYNC_CLKD_I); break;
                case 18: SRU2(DAI1_PB18_O, PCG0_SYNC_CLKD_I); break;
                case 19: SRU2(DAI1_PB19_O, PCG0_SYNC_CLKD_I); break;
                case 20: SRU2(DAI1_PB20_O, PCG0_SYNC_CLKD_I); break;
                default: assert(0);
            }
            break;
#endif
        default:
            assert(0);
    }
}

// Configure PCG
PCG_SIMPLE_RESULT pcg_open( PCG_SIMPLE_CONFIG * config) {

    assert(config);

    uint32_t reg_sync=0, reg_ctrl1=0, reg_ctrl0=0, reg_ctrl0_2;

    switch (config->clk_src) {
        case PCG_SRC_DAI_PIN:
            pcg_route_clk(config->pcg, config->clk_in_dai_pin);
            reg_ctrl1 |= BITM_PCG_CTLA1_CLKSRC;
            break;
        case PCG_SRC_CLKIN:
            break;
        case PCG_SRC_SCLK_PLL:
            reg_sync |= BITM_PCG_SYNC1_CLKASRC;
            reg_sync |= BITM_PCG_SYNC1_FSASRC;
            break;
        default:
            break;
    }

    if (config->sync_to_fs) {
        reg_ctrl1 |= BITM_PCG_CTLA1_FSSRC;
        reg_sync |= BITM_PCG_SYNC1_FSA;

        pcg_route_fs( config->pcg, config->fs_in_dai_pin );
    }

    uint32_t clock_divide = config->bitclk_div;
    uint32_t lr_divide = config->lrclk_clocks_per_frame * clock_divide;

    reg_ctrl1 |= (clock_divide << BITP_PCG_CTLA1_CLKDIV);
    reg_ctrl0 |= (lr_divide << BITP_PCG_CTLA0_FSDIV);

    uint32_t lr_phase_lo = 0; //(lr_divide/2 + 1) & 0x3FF;
    uint32_t lr_phase_high = 0; //(lr_divide/2 + 1) >> 10;

    reg_ctrl1 |= (lr_phase_lo) << BITP_PCG_CTLA1_FSPHASELO; // Shift LR clock to transition on falling edge
    reg_ctrl0_2 = reg_ctrl0 | (lr_phase_high) << BITP_PCG_CTLA0_FSPHASEHI;

#if 0
    // FIXME to work with enable
    reg_ctrl0_2 = reg_ctrl0_2 | BITM_PCG_CTLA0_CLKEN | BITM_PCG_CTLA0_FSEN;
#endif

    switch (config->pcg) {
        case PCG_A:
            *pREG_PCG0_CTLA0 = reg_ctrl0;
            *pREG_PCG0_CTLA1 = reg_ctrl1;
            *pREG_PCG0_SYNC1 = reg_sync;
            *pREG_PCG0_CTLA0 = reg_ctrl0_2;
            break;
        case PCG_B:
            *pREG_PCG0_CTLB0 = reg_ctrl0;
            *pREG_PCG0_CTLB1 = reg_ctrl1;
            *pREG_PCG0_SYNC1 = reg_sync << 16;
            *pREG_PCG0_CTLB0 = reg_ctrl0_2;
            break;
#if defined(__ADSPSC589_FAMILY__) || defined(__ADSP21569_FAMILY__) || \
    defined(__ADSPSC594_FAMILY__)
        case PCG_C:
            *pREG_PCG0_SYNC2 = reg_sync;
            *pREG_PCG0_CTLC1 = reg_ctrl1;
            *pREG_PCG0_CTLC0 = reg_ctrl0;
            *pREG_PCG0_CTLC0 = reg_ctrl0_2;
            break;
        case PCG_D:
            *pREG_PCG0_CTLD0 = reg_ctrl0;
            *pREG_PCG0_CTLD1 = reg_ctrl1;
            *pREG_PCG0_SYNC2 = reg_sync << 16;
            *pREG_PCG0_CTLD0 = reg_ctrl0_2;
            break;
#endif
        default:
            assert(0);
    }

    return PCG_SIMPLE_SUCCESS;
}

PCG_SIMPLE_RESULT pcg_enable( PCG_RESOURCE pcg, bool enable)
{
    volatile uint32_t *pREG_PCG_CTL0;

    switch (pcg) {
    case PCG_A:
        pREG_PCG_CTL0 = pREG_PCG0_CTLA0;
        break;
    case PCG_B:
        pREG_PCG_CTL0 = pREG_PCG0_CTLB0;
        break;
    case PCG_C:
        pREG_PCG_CTL0 = pREG_PCG0_CTLC0;
        break;
    case PCG_D:
        pREG_PCG_CTL0 = pREG_PCG0_CTLD0;
        break;
    default:
        assert(0);
    }

    uint32_t ctl0 = *pREG_PCG_CTL0;
    if (enable)
        ctl0 |= BITM_PCG_CTLA0_CLKEN | BITM_PCG_CTLA0_FSEN;
    else
        ctl0 &= ~(BITM_PCG_CTLA0_CLKEN | BITM_PCG_CTLA0_FSEN);
    *pREG_PCG_CTL0 = ctl0;

    return PCG_SIMPLE_SUCCESS;
}
