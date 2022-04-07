#include "adi_gemac_proc_int.h"
#include "adi_phy_int.h"

#undef __PHY_DP8386X__
#undef __PHY_DP83848__

#ifdef GEMAC_SUPPORT_EMAC0

#if EMAC0_PHY_NUM_DEV >= 1
#if EMAC0_PHY0_DEV == PHY_DEV_DP8386X
#define __PHY_DP8386X__
#elif EMAC0_PHY0_DEV == PHY_DEV_DP83848
#define __PHY_DP83848__
#else
#error "EMAC0 PHY0 Not configured"
#endif
#endif

#if EMAC0_PHY_NUM_DEV >= 2
#if EMAC0_PHY1_DEV == PHY_DEV_DP8386X
#define __PHY_DP8386X__
#elif EMAC0_PHY1_DEV == PHY_DEV_DP83848
#define __PHY_DP83848__
#else
#error "EMAC0 PHY1 Not configured"
#endif
#endif


#endif /* GEMAC_SUPPORT_EMAC0 */


#ifdef GEMAC_SUPPORT_EMAC1

#if EMAC1_PHY_NUM_DEV >= 1
#if EMAC1_PHY0_DEV == PHY_DEV_DP8386X
#define __PHY_DP8386X__
#elif EMAC1_PHY0_DEV == PHY_DEV_DP83848
#define __PHY_DP83848__
#else
#error "EMAC1 PHY0 Not configured"
#endif
#endif

#if EMAC1_PHY_NUM_DEV >= 2
#if EMAC1_PHY1_DEV == PHY_DEV_DP8386X
#define __PHY_DP8386X__
#elif EMAC1_PHY1_DEV == PHY_DEV_DP83848
#define __PHY_DP83848__
#else
#error "EMAC1 PHY1 Not configured"
#endif
#endif


#endif /* GEMAC_SUPPORT_EMAC1 */


#ifdef __PHY_DP8386X__
#include "adi_dp8386x_phy.h"
#endif

#ifdef __PHY_DP83848__
#include "adi_dp83848_phy_int.h"
#endif

#if ((!defined(GEMAC_SUPPORT_EMAC0)) && (defined(GEMAC_SUPPORT_EMAC1)))
#error "Invalid Configuration"
#endif

ADI_PHY_DEVICE PhyDevice[EMAC_NUM_DEV] = {




#ifdef GEMAC_SUPPORT_EMAC0

#if EMAC0_PHY_NUM_DEV > 0
#if EMAC0_PHY_DEV == 0
    {
    	EMAC0_PHY0_ADDR,
#if (EMAC0_PHY0_DEV == PHY_DEV_DP8386X)
        dp8386x_phy_init,
        dp8386x_phy_uninit,
        dp8386x_phy_get_status,
#elif (EMAC0_PHY0_DEV == PHY_DEV_DP83848)
        dp83848_phy_init,
        dp83848_phy_uninit,
        dp83848_phy_get_status,
#else
#error "Unknown PHY"
#endif
        NULL,
        NULL,
		0u,
		0u
    },
#else
#error "Not Yet Supported"
#endif
#endif /* EMAC0_PHY_NUM_DEV > 0 */


#endif




#ifdef GEMAC_SUPPORT_EMAC1

#if EMAC1_PHY_NUM_DEV > 0
#if EMAC1_PHY_DEV == 0
    {
    	EMAC1_PHY0_ADDR,
#if (EMAC1_PHY0_DEV == PHY_DEV_DP8386X)
        dp8386x_phy_init,
        dp8386x_phy_uninit,
        dp8386x_phy_get_status,
#elif (EMAC1_PHY0_DEV == PHY_DEV_DP83848)
        dp83848_phy_init,
        dp83848_phy_uninit,
        dp83848_phy_get_status,
#else
#error "Unknown PHY"
#endif
        NULL,
        NULL,
		1u,
		0u
    },
#else
#error "Not Yet Supported"
#endif
#else
	{
	    0,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    NULL,
	    1,
	    0
	},
#endif /* EMAC1_PHY_NUM_DEV > 0 */


#endif


};

