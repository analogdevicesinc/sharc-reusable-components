/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary & confidential to Analog Devices, Inc.
 * and its licensors. See LICENSE for complete details.
 *
 *=============================================================================
 *
 * \file:   a2b_bdd_helper.c
 * \brief:  The implementation of BDD Helper functions
 *
 *=============================================================================
 */


/*======================= I N C L U D E S =========================*/

/*#define A2B_DUMP_BDD*/

#ifdef A2B_DUMP_BDD
    #include "stddef.h"
    #include "stdio.h"
#endif

#include "a2b/ctypes.h"
#include "a2b_bdd_helper.h"
#include "a2b/regdefs.h"

/*======================= D E F I N E S ===========================*/
/*! Configure only non default values */
#define A2B_CONFIGURE_ONLY_NON_DEFAULT	(1)

/*! Updates the 'has' field */
#define A2B_UPDATE_HAS(x, y, z) x.has_##y = (x.y == z) ? A2B_FALSE : A2B_TRUE
#define A2B_SET_HAS(x, y) x.has_##y =  A2B_TRUE;
#define A2B_CLEAR_HAS(x, y) x.has_##y =  A2B_FALSE;
#define A2B_CONFIG_HAS(x, y, z) x.has_##y =  z;

/*! \addtogroup Network_Configuration
 *  @{
 */

/*! \addtogroup Bus_Configuration  Bus Configuration
* @{
*/
/*======================= L O C A L  P R O T O T Y P E S  =========*/
#ifdef ADI_SIGMASTUDIO_BCF
static void  adi_a2b_ParseMasterNCD(bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg,
	    ADI_A2B_COMMON_CONFIG* pCommon);
static void  adi_a2b_ParseSlaveNCD(	bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg);
static void adi_a2b_ParseMasterPinMux34(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg);
static void adi_a2b_ParseMasterPinMux56(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg);
static void  adi_a2b_ParseSlavePinMux012(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg);
static void adi_a2b_ParseSlavePinMux34(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg);
static void  adi_a2b_ParseSlavePinMux56(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg);
#ifdef ENABLE_AD242x_SUPPORT
static void  adi_a2b_ParseMasterNCD_242x(bdd_Node *pMstrNode,
	ADI_A2B_MASTER_NCD* pMstCfg,
	ADI_A2B_COMMON_CONFIG* pCommon);
static void  adi_a2b_ParseSlaveNCD_242x(bdd_Node *pSlvNode,
	ADI_A2B_SLAVE_NCD* pSlvCfg);
static void  adi_a2b_ParseMasterPinMux12(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg);
static void  adi_a2b_ParseMasterPinMux7(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg);
static void  adi_a2b_ParseSlavePinMux7(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg);

static void adi_a2b_SetforAllReg_242x_master(bdd_Node *pNode);
static void adi_a2b_SetforAllReg_242x_slave(bdd_Node *pNode);
#endif

static void adi_a2b_SetforAllReg_241x_slave(bdd_Node *pNode);
static void adi_a2b_SetforAllReg_241x_master(bdd_Node *pNode);
static void adi_a2b_CheckforDefault(bdd_Node *pNode);
static void adi_a2b_CheckforAutoConfig(bdd_Node *pNode, bool bAutoConfig);

#elif defined (A2B_BCF_FROM_SOC_EEPROM)

a2b_HResult a2b_EepromWriteRead(a2b_Handle hnd, a2b_UInt16 addr, a2b_UInt16 nWrite,
        const a2b_Byte* wBuf, a2b_UInt16 nRead,
        a2b_Byte* rBuf);

#endif


/**
 @}
*/

/**
 @}
*/
/*======================= D A T A  ================================*/

/*****************************************************************************************/
/*!
@brief      This function parses Bus Configuration Data(BCD) to Peripheral Config Table

@param [in] pFrameWorkHandle    Framework configuration pointer
@param [in] pBusDescription     Pointer to bus configuration data

@return     void

*/
/******************************************************************************************/



/*======================= M A C R O S =============================*/


/*======================= C O D E =================================*/

#ifdef A2B_DUMP_BDD
/*!****************************************************************************
*
*  \b              a2b_bddDecode
*
*  Helper routine to decode an A2B BDD.
*
*  \param          [in]    bdd          BDD to dump
*
*  \pre            None
*
*  \post           None
*
*  \return         True = success, False = Failure
*
******************************************************************************/
void
a2b_bddDump
    (
    bdd_Network*    bdd
    )
{
    size_t  idx, idx2;

    #define OPTb( sTitle, val, bHas ) \
        if (bHas) { printf("%s %s\n", sTitle, (val)?"TRUE":"false"); } else { printf("%s NOT SET\n", sTitle); }
    #define OPTs( sTitle, val, bHas ) \
        if (bHas) { printf("%s '%s'\n", sTitle, val); } else { printf("%s NOT SET\n", sTitle); }
    #define OPT8( sTitle, val, bHas ) \
        if (bHas) { printf("%s %02X\n", sTitle, val); } else { printf("%s NOT SET\n", sTitle); }
    #define OPT16( sTitle, val, bHas ) \
        if (bHas) { printf("%s %04X\n", sTitle, val); } else { printf("%s NOT SET\n", sTitle); }
    #define OPT32( sTitle, val, bHas ) \
        if (bHas) { printf("%s %08X\n", sTitle, val); } else { printf("%s NOT SET\n", sTitle); }

    #define REQsz( sTitle, val ) printf("%s %08lX\n", sTitle, val)
    #define REQb( sTitle, val )  printf("%s %s\n", sTitle, (val)?"TRUE":"false")
    #define REQs( sTitle, val )  printf("%s '%s'\n", sTitle, val)
    #define REQ8( sTitle, val )  printf("%s %02X\n", sTitle, val)
    #define REQ16( sTitle, val ) printf("%s %04X\n", sTitle, val)
    #define REQ32( sTitle, val ) printf("%s %08X\n", sTitle, val)

    if ( bdd == A2B_NULL )
    {
        printf("a2b_bddDump: NULL bdd\n");
        return;
    }

    printf("\na2b_bddDump: START ===========================================\n");

    printf("sampleRate: %08X (%d)\n", bdd->sampleRate, bdd->sampleRate);
    REQ32("masterAddr:", bdd->masterAddr);

    printf("metaData:\n");
    REQ32(">    date:        ", bdd->metaData.date);
    REQ32(">    version:     ", bdd->metaData.version);
    OPTs (">    author:      ", bdd->metaData.author, bdd->metaData.has_author);
    OPTs (">    organization:", bdd->metaData.organization, bdd->metaData.has_organization);
    OPTs (">    company:     ", bdd->metaData.company, bdd->metaData.has_company);

    printf("policy:\n");
    switch (bdd->policy.discoveryMode)
    {
    case 0:  printf(">    discoveryMode: SIMPLE\n"); break;
    case 1:  printf(">    discoveryMode: MODIFIED\n"); break;
    case 2:  printf(">    discoveryMode: OPTIMIZED\n"); break;
    case 3:  printf(">    discoveryMode: ADVANCED\n"); break;
    default: printf(">    discoveryMode: UNKNOWN (%d)\n", bdd->policy.discoveryMode); break;
    }
    switch (bdd->policy.cfgMethod)
    {
    case 0:  printf(">    cfgMethod:     AUTO\n"); break;
    case 1:  printf(">    cfgMethod:     BDD\n"); break;
    case 2:  printf(">    cfgMethod:     HYBRID\n"); break;
    default: printf(">    cfgMethod:     UNKNOWN (%d)\n", bdd->policy.cfgMethod); break;
    }
    switch (bdd->policy.cfgPriority)
    {
    case 0:  printf(">    cfgPriority:   AUTO\n"); break;
    case 1:  printf(">    cfgPriority:   BDD\n"); break;
    default: printf(">    cfgPriority:   UNKNOWN (%d)\n", bdd->policy.cfgPriority); break;
    }
    switch (bdd->policy.cfgErrPolicy)
    {
    case 0:  printf(">    cfgErrPolicy:  FATAL\n"); break;
    case 1:  printf(">    cfgErrPolicy:  ERROR\n"); break;
    case 2:  printf(">    cfgErrPolicy:  WARN\n"); break;
    case 3:  printf(">    cfgErrPolicy:  NONE\n"); break;
    default: printf(">    cfgErrPolicy:  UNKNOWN (%d)\n", bdd->policy.cfgErrPolicy); break;
    }

    printf("Streams: (cnt: %d)\n", bdd->streams_count);
    for ( idx = 0; idx < bdd->streams_count; idx++ )
    {
        REQsz(">----idx:                 ", idx);
        REQs (">    name:                ", bdd->streams[idx].name);
        REQ32(">    sampleRate:          ", bdd->streams[idx].sampleRate);
        REQ32(">    sampleRateMultiplier:", bdd->streams[idx].sampleRateMultiplier);
        REQ32(">    numChans:            ", bdd->streams[idx].numChans);

    } /* streams */

    printf("Nodes: (cnt: %d)\n", bdd->nodes_count);
    for ( idx = 0; idx < bdd->nodes_count; idx++ )
    {
        REQsz(">----idx:          ", idx);
        switch (bdd->nodes[idx].nodeType)
        {
        case 0:  printf(">    nodeType: UNKNOWN\n"); break;
        case 1:  printf(">    nodeType: MASTER\n"); break;
        case 2:  printf(">    nodeType: SLAVE\n"); break;
        default: printf(">    nodeType: UNKNOWN (%d)\n", bdd->nodes[idx].nodeType); break;
        }
        printf(">    ctrlRegs:\n");
        OPT32(">        bcdnslots:", bdd->nodes[idx].ctrlRegs.bcdnslots, bdd->nodes[idx].ctrlRegs.has_bcdnslots);
        OPT32(">        ldnslots: ", bdd->nodes[idx].ctrlRegs.ldnslots, bdd->nodes[idx].ctrlRegs.has_ldnslots);
        OPT32(">        lupslots: ", bdd->nodes[idx].ctrlRegs.lupslots, bdd->nodes[idx].ctrlRegs.has_lupslots);
        REQ32(">        dnslots:  ", bdd->nodes[idx].ctrlRegs.dnslots);
        REQ32(">        upslots:  ", bdd->nodes[idx].ctrlRegs.upslots);
        REQ32(">        respcycs: ", bdd->nodes[idx].ctrlRegs.respcycs);
        OPT32(">        slotfmt:  ", bdd->nodes[idx].ctrlRegs.slotfmt, bdd->nodes[idx].ctrlRegs.has_slotfmt);

        if ( !bdd->nodes[idx].has_intRegs )
        {
            printf(">    ctrlRegs: NOT SET\n");
        }
        else
        {
            printf(">    intRegs:\n");
            OPT32(">        intmsk0:", bdd->nodes[idx].intRegs.intmsk0, bdd->nodes[idx].intRegs.has_intmsk0);
            OPT32(">        intmsk1:", bdd->nodes[idx].intRegs.intmsk1, bdd->nodes[idx].intRegs.has_intmsk1);
            OPT32(">        intmsk2:", bdd->nodes[idx].intRegs.intmsk2, bdd->nodes[idx].intRegs.has_intmsk2);
            OPT32(">        becctl: ", bdd->nodes[idx].intRegs.becctl,  bdd->nodes[idx].intRegs.has_becctl);
        }

        if ( !bdd->nodes[idx].has_tuningRegs )
        {
            printf(">    tuningRegs: NOT SET\n");
        }
        else
        {
            printf(">    tuningRegs:\n");
            OPT32(">        vregctl:", bdd->nodes[idx].tuningRegs.vregctl, bdd->nodes[idx].tuningRegs.has_vregctl);
            OPT32(">        txactl: ", bdd->nodes[idx].tuningRegs.txactl, bdd->nodes[idx].tuningRegs.has_txactl);
            OPT32(">        rxactl: ", bdd->nodes[idx].tuningRegs.rxactl, bdd->nodes[idx].tuningRegs.has_rxactl);
            OPT32(">        txbctl: ", bdd->nodes[idx].tuningRegs.txbctl, bdd->nodes[idx].tuningRegs.has_txbctl);
            OPT32(">        rxbctl: ", bdd->nodes[idx].tuningRegs.rxbctl, bdd->nodes[idx].tuningRegs.has_rxbctl);
        }

        printf(">    i2cI2sRegs:\n");
        REQ32(">        i2ccfg:     ", bdd->nodes[idx].i2cI2sRegs.i2ccfg);
        REQ32(">        pllctl:     ", bdd->nodes[idx].i2cI2sRegs.pllctl);
        REQ32(">        i2sgcfg:    ", bdd->nodes[idx].i2cI2sRegs.i2sgcfg);
        REQ32(">        i2scfg:     ", bdd->nodes[idx].i2cI2sRegs.i2scfg);
        OPT32(">        i2srate:    ", bdd->nodes[idx].i2cI2sRegs.i2srate, bdd->nodes[idx].i2cI2sRegs.has_i2srate);
        OPT32(">        i2stxoffset:", bdd->nodes[idx].i2cI2sRegs.i2stxoffset, bdd->nodes[idx].i2cI2sRegs.has_i2stxoffset);
        OPT32(">        i2srxoffset:", bdd->nodes[idx].i2cI2sRegs.i2srxoffset, bdd->nodes[idx].i2cI2sRegs.has_i2srxoffset);
        OPT32(">        syncoffset: ", bdd->nodes[idx].i2cI2sRegs.syncoffset, bdd->nodes[idx].i2cI2sRegs.has_syncoffset);
        REQ32(">        pdmctl:     ", bdd->nodes[idx].i2cI2sRegs.pdmctl);
        REQ32(">        errmgmt:    ", bdd->nodes[idx].i2cI2sRegs.errmgmt);

        printf(">    pinIoRegs:\n");
        OPT32(">        clkcfg: ", bdd->nodes[idx].pinIoRegs.clkcfg, bdd->nodes[idx].pinIoRegs.has_clkcfg);
        REQ32(">        gpiooen:", bdd->nodes[idx].pinIoRegs.gpiooen);
        REQ32(">        gpioien:", bdd->nodes[idx].pinIoRegs.gpioien);
        REQ32(">        pinten: ", bdd->nodes[idx].pinIoRegs.pinten);
        REQ32(">        pintinv:", bdd->nodes[idx].pinIoRegs.pintinv);
        REQ32(">        pincfg: ", bdd->nodes[idx].pinIoRegs.pincfg);

        REQb (">    ignEeprom:      ", bdd->nodes[idx].ignEeprom);
        REQb (">    verifyNodeDescr:", bdd->nodes[idx].verifyNodeDescr);

        printf(">    nodeDescr:\n");
        REQ32(">        vendor: ", bdd->nodes[idx].nodeDescr.vendor);
        REQ32(">        product:", bdd->nodes[idx].nodeDescr.product);
        REQ32(">        version:", bdd->nodes[idx].nodeDescr.version);

        REQ32(">   downstreamBcastCnt:", bdd->nodes[idx].downstreamBcastCnt);
        printf(">   dwnstream: (cnt: %d)\n", bdd->nodes[idx].downstream_count);
        for ( idx2 = 0; idx2 < bdd->nodes[idx].downstream_count; idx2++ )
        {
            printf(">        downstream[%2ld]: %08X\n", idx2, bdd->nodes[idx].downstream[idx2]);
        } /* downstream */

        REQ32(">   upstreamBcastCnt:", bdd->nodes[idx].upstreamBcastCnt);
        printf(">   ustream: (cnt: %d)\n", bdd->nodes[idx].upstream_count);
        for ( idx2 = 0; idx2 < bdd->nodes[idx].upstream_count; idx2++ )
        {
            printf(">        upstream[%2ld]: %08X\n", idx2, bdd->nodes[idx].upstream[idx2]);
        } /* upstream */


    } /* nodes */

    printf("a2b_bddDump: END   ===========================================\n\n");
    fflush( stdout );

} /* a2b_bddDump */
#endif



/*!****************************************************************************
*
*  \b              a2b_bddDecode
*
*  Helper routine to decode an A2B BDD.
*
*  \param          [in]    bddData      pointer to binary BDD
*  \param          [in]    bddLen       length of bddData
*  \param          [out]   bddOut       decoded BDD
*
*  \pre            None
*
*  \post           None
*
*  \return         True = success, False = Failure
*
******************************************************************************/
a2b_Bool
a2b_bddDecode
    (
    const a2b_Byte* bddData,
    a2b_UInt32      bddLen,
    bdd_Network*    bddOut
    )
{
    pb_istream_t    stream;

    if ( ( bddData == A2B_NULL ) || ( bddLen == 0 ) )
    {
        return A2B_FALSE;
    }

    /* Cast away the constant-ness. The Nanopb API needs to be corrected
     * since 'bddData' is never modified in a pb_istream_t (and hence
     * can be treated as const).
     */
    stream = pb_istream_from_buffer( (a2b_Byte*)bddData, bddLen );

    if ( !pb_decode( &stream, bdd_Network_fields, bddOut ) )
    {
        return A2B_FALSE;
    }

    #ifdef A2B_DUMP_BDD
    a2b_bddDump( bddOut );
    #endif

    return A2B_TRUE;

} /* a2b_bddDecode */


/*!****************************************************************************
*
*  \b              a2b_bddPalInit
*
*  Helper routine to initialize some ECB values within the PAL.
*
*  \param          [in]    ecb
*  \param          [in]    bdd      decoded BDD (e.g. from a2b_bddDecode)
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
void
a2b_bddPalInit
    (
    A2B_ECB*            ecb,
    const bdd_Network*  bdd
    )
{
    if ( A2B_NULL != ecb )
    {
        if ( 0 != bdd->nodes_count )
        {
            ecb->baseEcb.masterNodeInfo.vendorId = (a2b_UInt8)
                                            bdd->nodes[0].nodeDescr.vendor;
            ecb->baseEcb.masterNodeInfo.productId = (a2b_UInt8)
                                            bdd->nodes[0].nodeDescr.product;
            ecb->baseEcb.masterNodeInfo.version = (a2b_UInt8)
                                            bdd->nodes[0].nodeDescr.version;
        }

        ecb->baseEcb.i2cMasterAddr = (a2b_UInt16)bdd->masterAddr;
    }

} /* a2b_bddPalInit */

/*! \addtogroup Network_Configuration
 *  @{
 */

/*! \addtogroup Bus_Configuration  Bus Configuration
* @{
*/

#ifdef ADI_SIGMASTUDIO_BCF

/*!****************************************************************************
*
*  \b              adi_a2b_ComprBcfParse_bdd
*
*  Helper routine to decode an A2B BDD.
*
*  \param          [in]    pCmprBusDescription      pointer to compressed BCF
*  \param          [in]    bdd_Graph       BDD destination array
*
*  \pre            None
*
*  \post           None
*
*  \return         True = success, False = Failure
*
******************************************************************************/
void adi_a2b_ComprBcfParse_bdd( ADI_A2B_COMPR_BCD *pCmprBusDescription,
		bdd_Network  *bdd_Graph, a2b_UInt8 nBusIndex )
{
    a2b_UInt8 nIndex1;

	/* Resetting the bdd network */
	memset(bdd_Graph, 0, sizeof(bdd_Network));

	/* Try to load the BDD structure */
	/* The Network Configuration is the binary exported by the Network
	* Configuration tool encoded in a Google Protobuf format.
	*/
	(void)a2b_bddDecode(pCmprBusDescription->apNetworkconfig[nBusIndex]->pgA2bNetwork,pCmprBusDescription->apNetworkconfig[nBusIndex]->gA2bNetworkLen, bdd_Graph);

	for(nIndex1 = 0; nIndex1 < bdd_Graph->nodes_count; nIndex1++)
	{
		/* Disabling verification of Vendor and Product ID, to be in sync with the uncompressed BCF */
		bdd_Graph->nodes[nIndex1].verifyNodeDescr = false;
	}



}

/*!****************************************************************************
*
*  \b              a2b_bcfParse_bdd
*
*  Helper routine to Parse the SigmaStudio BCF file to generate the
*  fields of BDD structure
*
*  \param          [in]    pBusDescription 		Ptr to Bus Description Struct
*  \param          [in]    bdd     			 	decoded BDD (e.g. from a2b_bddDecode)
*  \param          [in]    nBusIndex      		Chain/Bus/network index (To identify each one uniquely)
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
void
a2b_bcfParse_bdd
    (
    ADI_A2B_BCD *pBusDescription,
	bdd_Network  *bdd_Graph,
	a2b_UInt8	  nBusIndex
    )
{
    ADI_A2B_MASTER_SLAVE_CONFIG* pMasterSlaveChain;
    ADI_A2B_COMMON_CONFIG* pCommon;
    ADI_A2B_MASTER_NCD* pMstCfg;
    ADI_A2B_SLAVE_NCD* pSlvCfg;
    bdd_Node *pMstrNode;
    bdd_Node *pSlvNode;
    a2b_UInt8 nIndex1;
    a2b_UInt8 nNumMasternode = pBusDescription->nNumMasterNode;
    a2b_UInt8 nMstrNodeIdx= 0u;
    a2b_UInt8 nPLLCTL = 0u;

    /* Index should be lesser than the number of masters */
    if(nBusIndex >= nNumMasternode )
    {
    	return;
    }
	/* Resetting the bdd network */
	memset(bdd_Graph, 0, sizeof(bdd_Network));

	/* Get master-slave chain pointer */
	pMasterSlaveChain = pBusDescription->apNetworkconfig[nBusIndex];

	/* Pointer to master configuration */
	pMstCfg = pMasterSlaveChain->pMasterConfig;

	/* Common configuration settings  */
	pCommon = &pMasterSlaveChain->sCommonSetting;

	bdd_Graph->nodes_count = (pb_size_t)((a2b_UInt32)pMasterSlaveChain->nNumSlaveNode + 1U);
	bdd_Graph->masterAddr =  (a2b_UInt32) pCommon->nMasterI2CAddr;
	pMstrNode = &(bdd_Graph->nodes[nMstrNodeIdx]);

	/* Mandate Verify Node Descriptor */
	pMstrNode->verifyNodeDescr = true;
	nPLLCTL = pMstCfg->sRegSettings.nPLLCTL;
	adi_a2b_ParseMasterNCD(pMstrNode,pMstCfg,pCommon);

#ifdef ENABLE_AD242x_SUPPORT
	if(A2B_IS_AD242X_CHIP(pMstrNode->nodeDescr.vendor, pMstrNode->nodeDescr.product))
	{
		adi_a2b_ParseMasterNCD_242x(pMstrNode,pMstCfg,pCommon);
		adi_a2b_SetforAllReg_242x_master(pMstrNode);
	}

	if(A2B_IS_AD2428X_CHIP(pMstrNode->nodeDescr.vendor, pMstrNode->nodeDescr.product))
	{
		/* Let us assume that all the parts have common spread settings */
		bdd_Graph->policy.has_common_SSSettings =  1u;
	}
#endif

	if(A2B_IS_AD241X_CHIP(pMstrNode->nodeDescr.vendor, pMstrNode->nodeDescr.product))
	{
		/* Set has field for all the registers */
		adi_a2b_SetforAllReg_241x_master(pMstrNode);
	}
#if A2B_CONFIGURE_ONLY_NON_DEFAULT
		/* Remove if non deafult */
		adi_a2b_CheckforDefault(pMstrNode);
#endif
	/* Loop over number of slaves */
	for(nIndex1 = 0u ; nIndex1 < (a2b_UInt8)pMasterSlaveChain->nNumSlaveNode; nIndex1++)
	{
		pSlvCfg = pMasterSlaveChain->apSlaveConfig[nIndex1];
		pSlvNode = &(bdd_Graph->nodes[1u+nIndex1]);

		/* Mandate Verify Node Descriptor */
		pSlvNode->verifyNodeDescr = true;

		adi_a2b_ParseSlaveNCD(pSlvNode, pSlvCfg);
#ifdef ENABLE_AD242x_SUPPORT
		if(A2B_IS_AD242X_CHIP(pSlvNode->nodeDescr.vendor, pSlvNode->nodeDescr.product))
		{
			adi_a2b_ParseSlaveNCD_242x(pSlvNode,pSlvCfg);
			adi_a2b_SetforAllReg_242x_slave(pSlvNode);
		}
#endif
		/* Enable has for all */
		if(A2B_IS_AD241X_CHIP(pSlvNode->nodeDescr.vendor, pSlvNode->nodeDescr.product))
		{
			adi_a2b_SetforAllReg_241x_slave(pSlvNode);
			bdd_Graph->policy.has_common_SSSettings =  0u;
		}

		/* Prune in case of default */
#if A2B_CONFIGURE_ONLY_NON_DEFAULT
		adi_a2b_CheckforDefault(pSlvNode);
#endif
		/* Clear registers for auto configuration */
		adi_a2b_CheckforAutoConfig(pSlvNode, pSlvCfg->bEnableAutoConfig);

		/* Update common spread settings */
		if(nPLLCTL != pSlvCfg->sRegSettings.nPLLCTL)
		{
			bdd_Graph->policy.has_common_SSSettings =  0u;
		}
		/* if any of the part is not AD2428x series, then reset common spread */
		if(A2B_IS_AD2428X_CHIP(pSlvNode->nodeDescr.vendor, pSlvNode->nodeDescr.product) != (a2b_Bool)1u)
		{
			bdd_Graph->policy.has_common_SSSettings =  0u;
		}

	}

	bdd_Graph->policy.discoveryMode = (bdd_DiscoveryMode)pBusDescription->sTargetProperties.eDiscoveryMode;
	bdd_Graph->policy.cfgPriority = bdd_CONFIG_PRIORITY_BDD;
	/*bdd_Graph->policy.cfgMethod = bdd_CONFIG_METHOD_BDD; AUTO-CONFIG*/
	bdd_Graph->policy.cfgMethod = bdd_CONFIG_METHOD_HYBRID;
	bdd_Graph->policy.cfgErrPolicy = bdd_CONFIG_ERR_POLICY_FATAL;
	bdd_Graph->policy.discoveryStartDelay =  pBusDescription->sTargetProperties.nDiscoveryStartDelay;
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterNCD
*
*  Helper routine to Parse the Master config from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*  \param          [in]    pMstCfg   Ptr to Common Config of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseMasterNCD
(
	bdd_Node *pMstrNode,
	ADI_A2B_MASTER_NCD* pMstCfg,
	ADI_A2B_COMMON_CONFIG* pCommon
)
{

	pMstrNode->verifyNodeDescr = false;
	pMstrNode->ignEeprom = true;
	pMstrNode->nodeType = bdd_NODE_TYPE_MASTER;

	/* Assign ID registers */
	pMstrNode->nodeDescr.product = (a2b_UInt32)pMstCfg->sAuthSettings.nProductID;
	pMstrNode->nodeDescr.vendor = (a2b_UInt32)pMstCfg->sAuthSettings.nVendorID;
	pMstrNode->nodeDescr.version = (a2b_UInt32)pMstCfg->sAuthSettings.nVersionID;

	/* Control registers */
	pMstrNode->ctrlRegs.bcdnslots = 0U;
	pMstrNode->ctrlRegs.ldnslots = 0U;
	pMstrNode->ctrlRegs.lupslots = 0U;

	pMstrNode->ctrlRegs.dnslots = (a2b_UInt32)pMstCfg->sConfigCtrlSettings.nPassDwnSlots;
	pMstrNode->ctrlRegs.upslots = (a2b_UInt32)pMstCfg->sConfigCtrlSettings.nPassUpSlots;
	pMstrNode->ctrlRegs.respcycs = (a2b_UInt32)pMstCfg->sConfigCtrlSettings.nRespCycle;
	pMstrNode->ctrlRegs.slotfmt = (a2b_UInt32)(( pCommon->nDwnSlotSize | pCommon->nUpSlotSize | \
                                             ( pCommon->bDwnstreamCompression << (a2b_UInt8)A2B_BITP_SLOTFMT_DNFP) | \
                                             ( pCommon->bUpstreamCompression << (a2b_UInt8)A2B_BITP_SLOTFMT_UPFP)));
	pMstrNode->ctrlRegs.datctl = (a2b_UInt32)pMstCfg->sConfigCtrlSettings.nDatctrl;

	pMstrNode->upstreamBcastCnt = pMstrNode->downstreamBcastCnt = pMstrNode->ctrlRegs.bcdnslots;
	pMstrNode->upstream_count = (pb_size_t)pMstrNode->ctrlRegs.upslots;
	pMstrNode->downstream_count = (pb_size_t)pMstrNode->ctrlRegs.dnslots;

	/* I2S & PDM registers */
	pMstrNode->i2cI2sRegs.i2ccfg = (a2b_UInt16)(pMstCfg->sConfigCtrlSettings.bI2CEarlyAck << \
			                                                                     (a2b_UInt8)A2B_BITP_I2CCFG_EACK);
	pMstrNode->i2cI2sRegs.pllctl = (pMstCfg->sConfigCtrlSettings.nPLLTimeBase | pMstCfg->sConfigCtrlSettings.nBCLKRate);
	pMstrNode->i2cI2sRegs.i2sgcfg = ( pMstCfg->sI2SSettings.bEarlySync << (a2b_UInt8)A2B_BITP_I2SGCFG_EARLY ) | \
                                    ( pMstCfg->sI2SSettings.nTDMMode | pMstCfg->sI2SSettings.nTDMChSize ) | \
                                    ( pMstCfg->sI2SSettings.nSyncMode | pMstCfg->sI2SSettings.nSyncPolarity << A2B_BITP_I2SGCFG_INV ) ;

	pMstrNode->i2cI2sRegs.i2scfg = ( pMstCfg->sI2SSettings.bRXInterleave << (a2b_UInt8)A2B_BITP_I2SCFG_RX2PINTL ) | \
                                   ( pMstCfg->sI2SSettings.bTXInterleave << (a2b_UInt8)A2B_BITP_I2SCFG_TX2PINTL ) | \
                                   ( pMstCfg->sI2SSettings.nBclkRxPolarity << (a2b_UInt8)A2B_BITP_I2SCFG_RXBCLKINV )| \
                                   ( pMstCfg->sI2SSettings.nBclkTxPolarity << (a2b_UInt8)A2B_BITP_I2SCFG_TXBCLKINV);

	pMstrNode->i2cI2sRegs.i2srate = 0U;
	pMstrNode->i2cI2sRegs.i2stxoffset = (a2b_UInt16)( pMstCfg->sI2SSettings.nTxOffset | \
			                                      (pMstCfg->sI2SSettings.bTriStateAfterTx << (a2b_UInt8)A2B_BITP_I2STXOFFSET_TSAFTER) | \
                                                  (pMstCfg->sI2SSettings.bTriStateBeforeTx << A2B_BITP_I2STXOFFSET_TSBEFORE));
	pMstrNode->i2cI2sRegs.i2srxoffset = pMstCfg->sI2SSettings.nRxOffset;
	pMstrNode->i2cI2sRegs.has_syncoffset = true;
	pMstrNode->i2cI2sRegs.syncoffset = 0U;
	pMstrNode->i2cI2sRegs.pdmctl  = pMstCfg->sRegSettings.nPDMCTL;
	pMstrNode->i2cI2sRegs.errmgmt = pMstCfg->sRegSettings.nERRMGMT;

	/* INT registers */
	pMstrNode->has_intRegs = true;
	pMstrNode->intRegs.becctl = pMstCfg->sRegSettings.nBECCTL;
	pMstrNode->ctrlRegs.control = pMstCfg->sRegSettings.nCONTROL;
	pMstrNode->intRegs.intmsk0 = ( pMstCfg->sInterruptSettings.bReportHDCNTErr << (a2b_UInt8)A2B_BITP_INTPND0_HDCNTERR) | \
                                 ( pMstCfg->sInterruptSettings.bReportDDErr << (a2b_UInt8)A2B_BITP_INTPND0_DDERR) | \
                                 ( pMstCfg->sInterruptSettings.bReportCRCErr << (a2b_UInt8)A2B_BITP_INTPND0_CRCERR) | \
                                 ( pMstCfg->sInterruptSettings.bReportDataParityErr << (a2b_UInt8)A2B_BITP_INTPND0_DPERR) | \
                                 ( pMstCfg->sInterruptSettings.bReportPwrErr << (a2b_UInt8)A2B_BITP_INTPND0_PWRERR ) | \
                                 ( pMstCfg->sInterruptSettings.bReportErrCntOverFlow << (a2b_UInt8)A2B_BITP_INTPND0_BECOVF )| \
                                 ( pMstCfg->sInterruptSettings.bReportSRFMissErr << (a2b_UInt8)A2B_BITP_INTPND0_SRFERR );

	pMstrNode->intRegs.intmsk1 = ( pMstCfg->sInterruptSettings.bReportGPIO3 << (a2b_UInt8)A2B_BITP_INTPND1_IO3PND ) | \
                                 ( pMstCfg->sInterruptSettings.bReportGPIO4 << (a2b_UInt8)A2B_BITP_INTPND1_IO4PND ) | \
                                 ( pMstCfg->sInterruptSettings.bReportGPIO5 << (a2b_UInt8)A2B_BITP_INTPND1_IO5PND ) | \
                                 ( pMstCfg->sInterruptSettings.bReportGPIO6 << (a2b_UInt8)A2B_BITP_INTPND1_IO6PND);

	pMstrNode->intRegs.intmsk2 = ( pMstCfg->sInterruptSettings.bSlaveIntReq  << (a2b_UInt8)A2B_BITP_INTPND2_SLVIRQ ) | \
                                 ( pMstCfg->sInterruptSettings.bReportI2CErr << (a2b_UInt8)A2B_BITP_INTPND2_I2CERR ) | \
                                 ( pMstCfg->sInterruptSettings.bDiscComplete << (a2b_UInt8)A2B_BITP_INTPND2_DSCDONE ) | \
                                 ( pMstCfg->sInterruptSettings.bIntFrameCRCErr << (a2b_UInt8)A2B_BITP_INTPND2_ICRCERR );

	pMstrNode->pinIoRegs.pincfg = (pMstCfg->sGPIOSettings.bHighDriveStrength << (a2b_UInt8)A2B_BITP_PINCFG_DRVSTR );

    /* Parse pin multiplex - 3 and 4 */
	adi_a2b_ParseMasterPinMux34( pMstrNode ,  pMstCfg);
    /* Parse pin multiplex - 5 and 6 */
	adi_a2b_ParseMasterPinMux56( pMstrNode ,  pMstCfg);
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlaveNCD
*
*  Helper routine to Parse the Slave config from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseSlaveNCD
(
	bdd_Node *pSlvNode,
	ADI_A2B_SLAVE_NCD* pSlvCfg
)
{
	pSlvNode->ignEeprom = (pSlvCfg->bEnableAutoConfig)^ 0x1;/*AUTO-CONFIG*/
	pSlvNode->verifyNodeDescr = false;
	pSlvNode->nodeType = bdd_NODE_TYPE_SLAVE;

	/* Assign ID registers */
	pSlvNode->nodeDescr.product = (a2b_UInt32)pSlvCfg->sAuthSettings.nProductID;
	pSlvNode->nodeDescr.vendor = (a2b_UInt32)pSlvCfg->sAuthSettings.nVendorID;
	pSlvNode->nodeDescr.version = (a2b_UInt32)pSlvCfg->sAuthSettings.nVersionID;

	/* Custom Node Id Settings */
	pSlvNode->nodeDescr.oCustomNodeIdSettings.bCustomNodeIdAuth = pSlvCfg->sCustomNodeAuthSettings.bCustomNodeIdAuth;

	if(pSlvNode->nodeDescr.oCustomNodeIdSettings.bCustomNodeIdAuth == ENABLED)
	{
		if(pSlvCfg->sCustomNodeAuthSettings.nReadFrm == A2B_READ_FROM_MEM)
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadFrmMemory	= ENABLED;
		}
		else
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadFrmMemory	= DISABLED;
		}

		if(pSlvCfg->sCustomNodeAuthSettings.nReadFrm == A2B_READ_FROM_GPIO)
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadGpioPins		= ENABLED;
		}
		else
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadGpioPins		= DISABLED;
		}

		if(pSlvCfg->sCustomNodeAuthSettings.nReadFrm == A2B_READ_FROM_COMM_CH)
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh	= ENABLED;
		}
		else
		{
			pSlvNode->nodeDescr.oCustomNodeIdSettings.bReadFrmCommCh	= DISABLED;
		}

		pSlvNode->nodeDescr.oCustomNodeIdSettings.nDeviceAddr		= pSlvCfg->sCustomNodeAuthSettings.nDeviceAddr;
		memcpy(&pSlvNode->nodeDescr.oCustomNodeIdSettings.nNodeId[0], &pSlvCfg->sCustomNodeAuthSettings.nNodeId[0], ADI_A2B_MAX_CUST_NODE_ID_LEN);
		pSlvNode->nodeDescr.oCustomNodeIdSettings.nNodeIdLength		= pSlvCfg->sCustomNodeAuthSettings.nNodeIdLength;
		pSlvNode->nodeDescr.oCustomNodeIdSettings.nReadMemAddrWidth	= pSlvCfg->sCustomNodeAuthSettings.nReadMemAddrWidth;
		pSlvNode->nodeDescr.oCustomNodeIdSettings.nReadMemAddr		= pSlvCfg->sCustomNodeAuthSettings.nReadMemAddr;
		memcpy(&pSlvNode->nodeDescr.oCustomNodeIdSettings.aGpio[0], &pSlvCfg->sCustomNodeAuthSettings.aGpio[0], ADI_A2B_MAX_GPIO_PINS);
		pSlvNode->nodeDescr.oCustomNodeIdSettings.nTimeOut 			=   pSlvCfg->sCustomNodeAuthSettings.nTimeOut;
	}

	/* Control registers */
	pSlvNode->ctrlRegs.bcdnslots = pSlvCfg->sConfigCtrlSettings.nBroadCastSlots;
	pSlvNode->ctrlRegs.ldnslots = pSlvCfg->sConfigCtrlSettings.nLocalDwnSlotsConsume;
	pSlvNode->ctrlRegs.lupslots = pSlvCfg->sConfigCtrlSettings.nLocalUpSlotsContribute;

	pSlvNode->ctrlRegs.dnslots = (a2b_UInt32)pSlvCfg->sConfigCtrlSettings.nPassDwnSlots;
	pSlvNode->ctrlRegs.upslots = (a2b_UInt32)pSlvCfg->sConfigCtrlSettings.nPassUpSlots;
	pSlvNode->ctrlRegs.respcycs = (a2b_UInt32)pSlvCfg->sConfigCtrlSettings.nRespCycle;
	pSlvNode->ctrlRegs.control = (a2b_UInt32)pSlvCfg->sRegSettings.nCONTROL;
	pSlvNode->ctrlRegs.slotfmt = 0u;


	/* I2S & PDM registers */
	pSlvNode->i2cI2sRegs.i2ccfg = (a2b_UInt16)(pSlvCfg->sConfigCtrlSettings.nI2CFrequency) | \
                                              (pSlvCfg->sConfigCtrlSettings.nSuperFrameRate);


	pSlvNode->i2cI2sRegs.pllctl = (pSlvCfg->sRegSettings.nPLLCTL);
	pSlvNode->i2cI2sRegs.i2sgcfg = ( pSlvCfg->sI2SSettings.bEarlySync << (a2b_UInt8)A2B_BITP_I2SGCFG_EARLY ) | \
                                    ( pSlvCfg->sI2SSettings.nTDMMode | pSlvCfg->sI2SSettings.nTDMChSize ) | \
                                    ( pSlvCfg->sI2SSettings.nSyncMode | pSlvCfg->sI2SSettings.nSyncPolarity << A2B_BITP_I2SGCFG_INV ) ;

	pSlvNode->i2cI2sRegs.i2scfg = ( pSlvCfg->sI2SSettings.bRXInterleave << (a2b_UInt8)A2B_BITP_I2SCFG_RX2PINTL ) | \
                                   ( pSlvCfg->sI2SSettings.bTXInterleave << (a2b_UInt8)A2B_BITP_I2SCFG_TX2PINTL ) | \
                                   ( pSlvCfg->sI2SSettings.nBclkRxPolarity << (a2b_UInt8)A2B_BITP_I2SCFG_RXBCLKINV )| \
                                   ( pSlvCfg->sI2SSettings.nBclkTxPolarity << (a2b_UInt8)A2B_BITP_I2SCFG_TXBCLKINV);


	pSlvNode->i2cI2sRegs.i2srate = (pSlvCfg->sI2SSettings.sI2SRateConfig.bReduce << (a2b_UInt8)A2B_BITP_I2SRATE_REDUCE )| \
                                           (pSlvCfg->sI2SSettings.sI2SRateConfig.nSamplingRate);

	pSlvNode->i2cI2sRegs.i2stxoffset = 0u;
	pSlvNode->i2cI2sRegs.i2srxoffset = 0u;
	pSlvNode->i2cI2sRegs.syncoffset = pSlvCfg->sI2SSettings.nSyncOffset;
	pSlvNode->i2cI2sRegs.pdmctl |= ( pSlvCfg->sPDMSettings.bHPFUse << (a2b_UInt8)(A2B_BITP_PDMCTL_HPFEN) ) | \
                                   ( pSlvCfg->sPDMSettings.nHPFCutOff)  | \
            		               ( pSlvCfg->sPDMSettings.nNumSlotsPDM0) | (pSlvCfg->sPDMSettings.nNumSlotsPDM1);
	pSlvNode->i2cI2sRegs.errmgmt = pSlvCfg->sRegSettings.nERRMGMT;

	/* INT registers */
	pSlvNode->has_intRegs = true;
	pSlvNode->intRegs.becctl = pSlvCfg->sRegSettings.nBECCTL;
	pSlvNode->intRegs.intmsk0 = ( pSlvCfg->sInterruptSettings.bReportHDCNTErr << (a2b_UInt8)A2B_BITP_INTPND0_HDCNTERR) | \
                                 ( pSlvCfg->sInterruptSettings.bReportDDErr << (a2b_UInt8)A2B_BITP_INTPND0_DDERR) | \
                                 ( pSlvCfg->sInterruptSettings.bReportCRCErr << (a2b_UInt8)A2B_BITP_INTPND0_CRCERR) | \
                                 ( pSlvCfg->sInterruptSettings.bReportDataParityErr << (a2b_UInt8)A2B_BITP_INTPND0_DPERR) | \
                                 ( pSlvCfg->sInterruptSettings.bReportPwrErr << (a2b_UInt8)A2B_BITP_INTPND0_PWRERR ) | \
                                 ( pSlvCfg->sInterruptSettings.bReportErrCntOverFlow << (a2b_UInt8)A2B_BITP_INTPND0_BECOVF )| \
                                 ( pSlvCfg->sInterruptSettings.bReportSRFMissErr << (a2b_UInt8)A2B_BITP_INTPND0_SRFERR );

	pSlvNode->intRegs.intmsk1 =  ( pSlvCfg->sInterruptSettings.bReportGPIO0 << (a2b_UInt8)A2B_BITP_INTPND1_IO0PND ) | \
			                     ( pSlvCfg->sInterruptSettings.bReportGPIO1 << (a2b_UInt8)A2B_BITP_INTPND1_IO1PND ) | \
			                     ( pSlvCfg->sInterruptSettings.bReportGPIO2 << (a2b_UInt8)A2B_BITP_INTPND1_IO2PND ) | \
			                     ( pSlvCfg->sInterruptSettings.bReportGPIO3 << (a2b_UInt8)A2B_BITP_INTPND1_IO3PND ) | \
                                 ( pSlvCfg->sInterruptSettings.bReportGPIO4 << (a2b_UInt8)A2B_BITP_INTPND1_IO4PND ) | \
                                 ( pSlvCfg->sInterruptSettings.bReportGPIO5 << (a2b_UInt8)A2B_BITP_INTPND1_IO5PND ) | \
                                 ( pSlvCfg->sInterruptSettings.bReportGPIO6 << (a2b_UInt8)A2B_BITP_INTPND1_IO6PND);

	pSlvNode->intRegs.intmsk2 = 0u;

	pSlvNode->pinIoRegs.clkcfg = (pSlvCfg->sI2SSettings.nCodecClkRate << A2B_BITP_CLKCFG_CCLKRATE);

    /* Parsing Pin Mux */
	adi_a2b_ParseSlavePinMux012(pSlvNode ,  pSlvCfg);
	adi_a2b_ParseSlavePinMux34( pSlvNode ,  pSlvCfg);
	adi_a2b_ParseSlavePinMux56( pSlvNode ,  pSlvCfg);

    pSlvNode->pinIoRegs.pincfg = (pSlvCfg->sGPIOSettings.bHighDriveStrength << (a2b_UInt8)A2B_BITP_PINCFG_DRVSTR );

}


/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterPinMux34
*
*  Helper routine to Parse the Master Pin Mux 3&4 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_ParseMasterPinMux34(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg
		)
{
	/* Pin multiplex for GPIO 3*/
	    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO3PinUsage)
	    {
	        case A2B_GPIO_3_INPUT:
	        	pMstrNode->pinIoRegs.gpioien |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO3IEN);
	        	pMstrNode->pinIoRegs.pinten  |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO3Interrupt << A2B_BITP_PINTEN_IO3IE);
	        	pMstrNode->pinIoRegs.pintinv |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO3IntPolarity << A2B_BITP_PINTINV_IO3INV);
	            break;
	        case A2B_GPIO_3_OUTPUT:
	        	pMstrNode->pinIoRegs.gpiooen |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO3OEN);
	        	pMstrNode->pinIoRegs.gpiodat |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO3Val << A2B_BITP_GPIODAT_IO3DAT);
	            break;
	        case A2B_GPIO_3_AS_DTX0:
	        	pMstrNode->i2cI2sRegs.i2scfg |= (1u <<A2B_BITP_I2SCFG_TX0EN);
	            break;
	        /*case A2B_GPIO_3_DISABLE:
	            break;*/
	        default:
	        break;
	    }

	    /* Pin multiplex for GPIO 4*/
	    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO4PinUsage)
	    {
	        case A2B_GPIO_4_INPUT:
	        	pMstrNode->pinIoRegs.gpioien |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO4IEN);
	        	pMstrNode->pinIoRegs.pinten  |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO4Interrupt << A2B_BITP_PINTEN_IO4IE);
	        	pMstrNode->pinIoRegs.pintinv |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO4IntPolarity << A2B_BITP_PINTINV_IO4INV);
	            break;
	        case A2B_GPIO_4_OUTPUT:
	           pMstrNode->pinIoRegs.gpiooen |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO4OEN);
	           pMstrNode->pinIoRegs.gpiodat |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO4Val << A2B_BITP_GPIODAT_IO4DAT);
	           break;
	        case A2B_GPIO_4_AS_DTX1:
	           pMstrNode->i2cI2sRegs.i2scfg |= (1u <<A2B_BITP_I2SCFG_TX1EN);
	           break;
	        /*case A2B_GPIO_4_DISABLE:
	           break;*/
	        default:
	           break;
	       }

}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterPinMux56
*
*  Helper routine to Parse the Master Pin Mux 5&6 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_ParseMasterPinMux56(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg)
{

    /* Pin multiplex for GPIO 5*/
    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO5PinUsage)
    {
    case A2B_GPIO_5_INPUT:
    	pMstrNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO5IEN);
    	pMstrNode->pinIoRegs.pinten        |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO5Interrupt << A2B_BITP_PINTEN_IO5IE);
    	pMstrNode->pinIoRegs.pintinv       |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO5IntPolarity << A2B_BITP_PINTINV_IO5INV);
        break;
    case A2B_GPIO_5_OUTPUT:
    	pMstrNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO5OEN);
    	pMstrNode->pinIoRegs.gpiodat       |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO5Val << A2B_BITP_GPIODAT_IO5DAT);
        break;
    case A2B_GPIO_5_AS_DRX0:
    	pMstrNode->i2cI2sRegs.i2scfg        |= (1u <<A2B_BITP_I2SCFG_RX0EN);
        break;
    /*case A2B_GPIO_5_DISABLE:
        break;*/
    default:
        break;

    }

    /* Pin multiplex for GPIO 6*/
    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO6PinUsage)
    {
    case A2B_GPIO_6_INPUT:
    	pMstrNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO6IEN);
    	pMstrNode->pinIoRegs.pinten        |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO6Interrupt << A2B_BITP_PINTEN_IO6IE);
    	pMstrNode->pinIoRegs.pintinv       |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO6IntPolarity << A2B_BITP_PINTINV_IO6INV);
        break;
    case A2B_GPIO_6_OUTPUT:
    	pMstrNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO6OEN);
    	pMstrNode->pinIoRegs.gpiodat       |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO6Val << A2B_BITP_GPIODAT_IO6DAT);
        break;
    case A2B_GPIO_6_AS_DRX1:
    	pMstrNode->i2cI2sRegs.i2scfg       |= (1u <<A2B_BITP_I2SCFG_RX1EN);
        break;
    /*case A2B_GPIO_6_DISABLE:
        break;*/
    default:
        break;

    }
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlavePinMux012
*
*  Helper routine to Parse the Slave Pin Mux 0,1&2 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseSlavePinMux012(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg)
{

     /* Pin multiplex for GPIO 0*/
    switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO0PinUsage)
    {
        case A2B_GPIO_0_INPUT:
        	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO0IEN);
        	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO0Interrupt << A2B_BITP_PINTEN_IO0IE);
        	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO0IntPolarity << A2B_BITP_PINTINV_IO0INV);
            break;
        case A2B_GPIO_0_OUTPUT:
        	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO0OEN);
        	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO0Val << A2B_BITP_GPIODAT_IO0DAT);
            break;
        /*case A2B_GPIO_0_DISABLE:
            break; */
        default:
        break;

    }

    /* Pin multiplex for GPIO 1*/
    switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO1PinUsage)
    {
        case A2B_GPIO_1_INPUT:
        	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO1IEN);
        	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO1Interrupt << A2B_BITP_PINTEN_IO1IE);
        	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO1IntPolarity << A2B_BITP_PINTINV_IO1INV);
            break;
        case A2B_GPIO_1_OUTPUT:
        	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO1OEN);
        	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO1Val << A2B_BITP_GPIODAT_IO1DAT);
            break;
        case A2B_GPIO_1_AS_CLKOUT:
#ifdef ENABLE_AD242x_SUPPORT
        	if(A2B_IS_AD242X_CHIP(pSlvNode->nodeDescr.vendor, pSlvNode->nodeDescr.product))
        	{
				pSlvNode->pinIoRegs.clk1cfg     |= (1u << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1EN);
        	}
#endif

            break;

        /* case A2B_GPIO_1_DISABLE:
            break; */
        default:
        break;
    }

    /* Pin multiplex for GPIO 2*/
    switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO2PinUsage)
    {
        case A2B_GPIO_2_INPUT:
        	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO2IEN);
        	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO2Interrupt << A2B_BITP_PINTEN_IO2IE);
        	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO2IntPolarity << A2B_BITP_PINTINV_IO2INV);
            break;
        case A2B_GPIO_2_OUTPUT:
        	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO2OEN);
        	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO2Val << A2B_BITP_GPIODAT_IO2DAT);
            break;
        case A2B_GPIO_2_AS_CLKOUT:
#ifdef ENABLE_AD242x_SUPPORT
        	if(A2B_IS_AD242X_CHIP(pSlvNode->nodeDescr.vendor, pSlvNode->nodeDescr.product))
        	{
				pSlvNode->pinIoRegs.clk2cfg     |= (1u << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1EN);
        	}
        	else
#endif
        	{
				pSlvNode->pinIoRegs.clkcfg        |= (1u << (a2b_UInt8)A2B_BITP_CLKCFG_CCLKEN);
        	}
            break;
        /* case A2B_GPIO_2_DISABLE:
            break; */
        default:
        break;

    }
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlavePinMux34
*
*  Helper routine to Parse the Slave Pin Mux 3&4 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_ParseSlavePinMux34(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg)
{
     /* Pin multiplex for GPIO 3*/
    switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO3PinUsage)
    {
        case A2B_GPIO_3_INPUT:
        	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO3IEN);
        	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO3Interrupt << A2B_BITP_PINTEN_IO3IE);
        	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO3IntPolarity << A2B_BITP_PINTINV_IO3INV);
            break;
        case A2B_GPIO_3_OUTPUT:
        	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO3OEN);
        	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO3Val << A2B_BITP_GPIODAT_IO3DAT);
            break;
        case A2B_GPIO_3_AS_DTX0:
        	pSlvNode->i2cI2sRegs.i2scfg      |= ( 1u <<A2B_BITP_I2SCFG_TX0EN);
            break;
    /*    case A2B_GPIO_3_DISABLE:
            break; */
        default:
        break;

    }

   /* Pin multiplex for GPIO 4*/
  switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO4PinUsage)
  {
    case A2B_GPIO_4_INPUT:
    	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO4IEN);
    	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO4Interrupt << A2B_BITP_PINTEN_IO4IE);
    	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO4IntPolarity << A2B_BITP_PINTINV_IO4INV);
        break;
    case A2B_GPIO_4_OUTPUT:
    	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO4OEN);
    	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO4Val << A2B_BITP_GPIODAT_IO4DAT);
        break;
    case A2B_GPIO_4_AS_DTX1:
    	pSlvNode->i2cI2sRegs.i2scfg       |= ( 1u <<A2B_BITP_I2SCFG_TX1EN);
        break;
     /* case A2B_GPIO_4_DISABLE:
        break; */
    default:
        break;
  }


}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlavePinMux56
*
*  Helper routine to Parse the Slave Pin Mux 5&6 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseSlavePinMux56(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg)
{


  /* Pin multiplex for GPIO 5*/
  switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO5PinUsage)
  {
    case A2B_GPIO_5_INPUT:
    	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO5IEN);
    	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO5Interrupt << A2B_BITP_PINTEN_IO5IE);
    	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO5IntPolarity << A2B_BITP_PINTINV_IO5INV);
        break;
    case A2B_GPIO_5_OUTPUT:
    	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO5OEN);
    	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO5Val << A2B_BITP_GPIODAT_IO5DAT);
        break;
    case A2B_GPIO_5_AS_DRX0:
    	pSlvNode->i2cI2sRegs.i2scfg       |= ( 1u <<A2B_BITP_I2SCFG_RX0EN);
        break;
    case A2B_GPIO_5_AS_PDM0:
    	pSlvNode->i2cI2sRegs.pdmctl       |= ( 1u <<A2B_BITP_PDMCTL_PDM0EN);
        break;
    /* case A2B_GPIO_5_DISABLE:
        break; */
    default:
        break;

  }

  /* Pin multiplex for GPIO 6*/
  switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO6PinUsage)
  {
    case A2B_GPIO_6_INPUT:
    	pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO6IEN);
    	pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO6Interrupt << A2B_BITP_PINTEN_IO6IE);
    	pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO6IntPolarity << A2B_BITP_PINTINV_IO6INV);
        break;
    case A2B_GPIO_6_OUTPUT:
    	pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO6OEN);
    	pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO6Val << A2B_BITP_GPIODAT_IO6DAT);
        break;
    case A2B_GPIO_6_AS_DRX1:
    	pSlvNode->i2cI2sRegs.i2scfg        |= ( 1u <<A2B_BITP_I2SCFG_RX1EN);
        break;
    case A2B_GPIO_6_AS_PDM1:
    	pSlvNode->i2cI2sRegs.pdmctl        |= ( 1u <<A2B_BITP_PDMCTL_PDM1EN);
        break;
    /* case A2B_GPIO_6_DISABLE:
        break; */
    default:
        break;

  }
}

#ifdef ENABLE_AD242x_SUPPORT
/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterNCD_242x
*
*  Helper routine to Parse the 242x Master config from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*  \param          [in]    pMstCfg   Ptr to Common Config of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseMasterNCD_242x
(
	bdd_Node *pMstrNode,
	ADI_A2B_MASTER_NCD* pMstCfg,
	ADI_A2B_COMMON_CONFIG* pCommon
)
{
	uint32_t *psGPIODMask[8];
	A2B_GPIOD_PIN_CONFIG asGPIODConfig[8];
	a2b_UInt8 nGPIOIndex;

	/* GPIOD  */
	pMstrNode->has_gpioDist = true;
	pMstrNode->gpioDist.has_gpiod0msk = true;
	pMstrNode->gpioDist.has_gpiod1msk = true;
	pMstrNode->gpioDist.has_gpiod2msk = true;
	pMstrNode->gpioDist.has_gpiod3msk = true;
	pMstrNode->gpioDist.has_gpiod4msk = true;
	pMstrNode->gpioDist.has_gpiod5msk = true;
	pMstrNode->gpioDist.has_gpiod6msk = true;
	pMstrNode->gpioDist.has_gpiod7msk = true;
	pMstrNode->gpioDist.has_gpioden = true;
	pMstrNode->gpioDist.has_gpiodinv = true;

	pMstrNode->gpioDist.gpiodinv = (pMstCfg->sGPIODSettings.sGPIOD1Config.bGPIOSignalInv << 1u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD2Config.bGPIOSignalInv << 2u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD3Config.bGPIOSignalInv << 3u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD4Config.bGPIOSignalInv << 4u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD5Config.bGPIOSignalInv << 5u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD6Config.bGPIOSignalInv << 6u) |\
								   (pMstCfg->sGPIODSettings.sGPIOD7Config.bGPIOSignalInv << 7u);

	pMstrNode->gpioDist.gpioden = (pMstCfg->sGPIODSettings.sGPIOD1Config.bGPIODistance << 1u) |\
			 	 	 	 	 	  (pMstCfg->sGPIODSettings.sGPIOD2Config.bGPIODistance << 2u) |\
								  (pMstCfg->sGPIODSettings.sGPIOD3Config.bGPIODistance << 3u) |\
								  (pMstCfg->sGPIODSettings.sGPIOD4Config.bGPIODistance << 4u) |\
								  (pMstCfg->sGPIODSettings.sGPIOD5Config.bGPIODistance << 5u) |\
								  (pMstCfg->sGPIODSettings.sGPIOD6Config.bGPIODistance << 6u) |\
								  (pMstCfg->sGPIODSettings.sGPIOD7Config.bGPIODistance << 7u);

	/* GPIOD Mask update */
	psGPIODMask[0] = &pMstrNode->gpioDist.gpiod0msk;
	psGPIODMask[1] = &pMstrNode->gpioDist.gpiod1msk;
	psGPIODMask[2] = &pMstrNode->gpioDist.gpiod2msk;
	psGPIODMask[3] = &pMstrNode->gpioDist.gpiod3msk;
	psGPIODMask[4] = &pMstrNode->gpioDist.gpiod4msk;
	psGPIODMask[5] = &pMstrNode->gpioDist.gpiod5msk;
	psGPIODMask[6] = &pMstrNode->gpioDist.gpiod6msk;
	psGPIODMask[7] = &pMstrNode->gpioDist.gpiod7msk;

	asGPIODConfig[1] = pMstCfg->sGPIODSettings.sGPIOD1Config;
	asGPIODConfig[2] = pMstCfg->sGPIODSettings.sGPIOD2Config;
	asGPIODConfig[3] = pMstCfg->sGPIODSettings.sGPIOD3Config;
	asGPIODConfig[4] = pMstCfg->sGPIODSettings.sGPIOD4Config;
	asGPIODConfig[5] = pMstCfg->sGPIODSettings.sGPIOD5Config;
	asGPIODConfig[6] = pMstCfg->sGPIODSettings.sGPIOD6Config;
	asGPIODConfig[7] = pMstCfg->sGPIODSettings.sGPIOD7Config;

	for(nGPIOIndex = 1u; nGPIOIndex < 8u; nGPIOIndex++)
	{


		*(psGPIODMask[nGPIOIndex]) =  (a2b_UInt16) ( (asGPIODConfig[nGPIOIndex].abBusPortMask[0] << 0u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[1] << 1u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[2] << 2u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[3] << 3u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[4] << 4u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[5] << 5u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[6] << 6u) |\
									 (asGPIODConfig[nGPIOIndex].abBusPortMask[7] << 7u));
	}

	 /* I2S & PDM registers */
	pMstrNode->i2cI2sRegs.i2srrate = pCommon->bEnableReduceRate << (a2b_UInt8)(A2B_BITP_I2SRRATE_RBUS ) |   \
									   (pCommon->nSysRateDivFactor << (a2b_UInt8)A2B_BITP_I2SRRATE_RRDIV);

	pMstrNode->i2cI2sRegs.i2srrctl = ( pMstCfg->sI2SSettings.sI2SRateConfig.bRRStrobe << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENSTRB )|\
			                         ( pMstCfg->sI2SSettings.sI2SRateConfig.bRRStrobeDirection << (a2b_UInt8)A2B_BITP_I2SRRCTL_STRBDIR )|\
									 ( pMstCfg->sI2SSettings.sI2SRateConfig.bRRValidBitExtraBit << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENXBIT )|\
									 ( pMstCfg->sI2SSettings.sI2SRateConfig.bRRValidBitLSB << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENVLSB )|\
									 ( pMstCfg->sI2SSettings.sI2SRateConfig.bRRValidBitExtraCh << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENCHAN );

	/* INT registers */
	pMstrNode->intRegs.intmsk1 = ( pMstCfg->sInterruptSettings.bReportGPIO1 << (a2b_UInt8)A2B_BITP_INTPND1_IO1PND ) | \
									 ( pMstCfg->sInterruptSettings.bReportGPIO2 << (a2b_UInt8)A2B_BITP_INTPND1_IO2PND ) | \
									 ( pMstCfg->sInterruptSettings.bReportGPIO3 << (a2b_UInt8)A2B_BITP_INTPND1_IO3PND ) | \
									 ( pMstCfg->sInterruptSettings.bReportGPIO4 << (a2b_UInt8)A2B_BITP_INTPND1_IO4PND ) | \
									 ( pMstCfg->sInterruptSettings.bReportGPIO5 << (a2b_UInt8)A2B_BITP_INTPND1_IO5PND ) | \
									 ( pMstCfg->sInterruptSettings.bReportGPIO6 << (a2b_UInt8)A2B_BITP_INTPND1_IO6PND ) |\
									 ( pMstCfg->sInterruptSettings.bReportGPIO7 << (a2b_UInt8)A2B_BITP_INTPND1_IO7PND );
	pMstrNode->pinIoRegs.pincfg = (pMstCfg->sGPIOSettings.bHighDriveStrength << (a2b_UInt8)A2B_BITP_PINCFG_DRVSTR ) |\
		       	   	   	   	   	   	   	   	   	   	   	   	   (pMstCfg->sGPIOSettings.bIRQInv << (a2b_UInt8)A2B_BITP_PINCFG_IRQINV) |\
															   (pMstCfg->sGPIOSettings.bIRQTriState << (a2b_UInt8)A2B_BITP_PINCFG_IRQTS);

	/*ClockCFg1 & CFG2*/
	pMstrNode->pinIoRegs.clk1cfg |= (pMstCfg->sClkOutSettings.bClk1Div << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1DIV) |\
	          (pMstCfg->sClkOutSettings.bClk1PreDiv << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1PDIV) |\
	          (pMstCfg->sClkOutSettings.bClk1Inv << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1INV);

	pMstrNode->pinIoRegs.has_clk2cfg |= (pMstCfg->sClkOutSettings.bClk2Div << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1DIV) |\
			   	   	   	   	   	        (pMstCfg->sClkOutSettings.bClk2PreDiv << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1PDIV) |\
										(pMstCfg->sClkOutSettings.bClk2Inv << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1INV);

	pMstrNode->i2cI2sRegs.i2sgcfg |= ( pMstCfg->sI2SSettings.bSerialRxOnDTx1 << (a2b_UInt8)A2B_BITP_I2SGCFG_RXONDTX1 );

	pMstrNode->i2cI2sRegs.pdmctl2 = pMstCfg->sRegSettings.nPDMCTL2;
	pMstrNode->i2cI2sRegs.pllctl  = pMstCfg->sRegSettings.nPLLCTL;

	/* Tuning Registers */
	pMstrNode->tuningRegs.txactl = pMstCfg->sRegSettings.nTXACTL;
	pMstrNode->tuningRegs.txbctl = pMstCfg->sRegSettings.nTXBCTL;

	/* Pin Multiplex  */
	adi_a2b_ParseMasterPinMux12(pMstrNode, pMstCfg);
	adi_a2b_ParseMasterPinMux7(pMstrNode, pMstCfg);
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlaveNCD_242x
*
*  Helper routine to Parse the 242x Slave config from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseSlaveNCD_242x
(
	bdd_Node *pSlvNode,
	ADI_A2B_SLAVE_NCD* pSlvCfg
)
{
    uint32_t *psGPIODMask[8];
    A2B_GPIOD_PIN_CONFIG asGPIODConfig[8];
    a2b_UInt8 nGPIOIndex;

    pSlvNode->ctrlRegs.lupslots = pSlvCfg->sConfigCtrlSettings.nLocalUpSlotsContribute;
    pSlvNode->ctrlRegs.dnslots = pSlvCfg->sConfigCtrlSettings.nPassDwnSlots;
    pSlvNode->ctrlRegs.upslots = pSlvCfg->sConfigCtrlSettings.nPassUpSlots;
	pSlvNode->has_slotEnh = true;

    if(pSlvCfg->sConfigCtrlSettings.bUseDwnslotConsumeMasks == 1u)
    {
    	pSlvNode->ctrlRegs.has_ldnslots = true;
    	pSlvNode->ctrlRegs.ldnslots = ((a2b_UInt8)A2B_BITM_LDNSLOTS_DNMASKEN | pSlvCfg->sConfigCtrlSettings.nSlotsforDwnstrmContribute);
    	pSlvNode->slotEnh.dnmask0 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[0] << A2B_BITP_DNMASK0_RXDNSLOT00)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[1] << A2B_BITP_DNMASK0_RXDNSLOT01 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[2] << A2B_BITP_DNMASK0_RXDNSLOT02)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[3] << A2B_BITP_DNMASK0_RXDNSLOT03 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[4] << A2B_BITP_DNMASK0_RXDNSLOT04)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[5] << A2B_BITP_DNMASK0_RXDNSLOT05 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[6] << A2B_BITP_DNMASK0_RXDNSLOT06)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[7] << A2B_BITP_DNMASK0_RXDNSLOT07 );

    	pSlvNode->slotEnh.dnmask1 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[8] << A2B_BITP_DNMASK0_RXDNSLOT00)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[9] << A2B_BITP_DNMASK0_RXDNSLOT01 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[10] << A2B_BITP_DNMASK0_RXDNSLOT02)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[11] << A2B_BITP_DNMASK0_RXDNSLOT03 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[12] << A2B_BITP_DNMASK0_RXDNSLOT04)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[13] << A2B_BITP_DNMASK0_RXDNSLOT05 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[14] << A2B_BITP_DNMASK0_RXDNSLOT06)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[15] << A2B_BITP_DNMASK0_RXDNSLOT07 );

    	pSlvNode->slotEnh.dnmask2 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[16] << A2B_BITP_DNMASK0_RXDNSLOT00)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[17] << A2B_BITP_DNMASK0_RXDNSLOT01 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[18] << A2B_BITP_DNMASK0_RXDNSLOT02)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[19] << A2B_BITP_DNMASK0_RXDNSLOT03 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[20] << A2B_BITP_DNMASK0_RXDNSLOT04)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[21] << A2B_BITP_DNMASK0_RXDNSLOT05 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[22] << A2B_BITP_DNMASK0_RXDNSLOT06)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[23] << A2B_BITP_DNMASK0_RXDNSLOT07 );

    	pSlvNode->slotEnh.dnmask3 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[24] << A2B_BITP_DNMASK0_RXDNSLOT00)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[25] << A2B_BITP_DNMASK0_RXDNSLOT01 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[26] << A2B_BITP_DNMASK0_RXDNSLOT02)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[27] << A2B_BITP_DNMASK0_RXDNSLOT03 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[28] << A2B_BITP_DNMASK0_RXDNSLOT04)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[29] << A2B_BITP_DNMASK0_RXDNSLOT05 )|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[30] << A2B_BITP_DNMASK0_RXDNSLOT06)|\
				   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anDwnstreamConsumeSlots[31] << A2B_BITP_DNMASK0_RXDNSLOT07 );
    }
    else
    {
    	pSlvNode->ctrlRegs.ldnslots = pSlvCfg->sConfigCtrlSettings.nLocalDwnSlotsConsume;
    	pSlvNode->slotEnh.dnmask0 = 0u;
    	pSlvNode->slotEnh.dnmask1 = 0u;
    	pSlvNode->slotEnh.dnmask2 = 0u;
    	pSlvNode->slotEnh.dnmask3 = 0u;
    }


    pSlvNode->slotEnh.dnoffset = pSlvCfg->sConfigCtrlSettings.nOffsetDwnstrmContribute;
    pSlvNode->slotEnh.upoffset = pSlvCfg->sConfigCtrlSettings.nOffsetUpstrmContribute;

    pSlvNode->slotEnh.upmask0 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[0] << A2B_BITP_UPMASK0_RXUPSLOT00)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[1] << A2B_BITP_UPMASK0_RXUPSLOT01 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[2] << A2B_BITP_UPMASK0_RXUPSLOT02)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[3] << A2B_BITP_UPMASK0_RXUPSLOT03 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[4] << A2B_BITP_UPMASK0_RXUPSLOT04)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[5] << A2B_BITP_UPMASK0_RXUPSLOT05 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[6] << A2B_BITP_UPMASK0_RXUPSLOT06)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[7] << A2B_BITP_UPMASK0_RXUPSLOT07 );

    pSlvNode->slotEnh.upmask1 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[8] << A2B_BITP_UPMASK0_RXUPSLOT00)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[9] << A2B_BITP_UPMASK0_RXUPSLOT01 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[10] << A2B_BITP_UPMASK0_RXUPSLOT02)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[11] << A2B_BITP_UPMASK0_RXUPSLOT03 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[12] << A2B_BITP_UPMASK0_RXUPSLOT04)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[13] << A2B_BITP_UPMASK0_RXUPSLOT05 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[14] << A2B_BITP_UPMASK0_RXUPSLOT06)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[15] << A2B_BITP_UPMASK0_RXUPSLOT07 );

    pSlvNode->slotEnh.upmask2 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[16] << A2B_BITP_UPMASK0_RXUPSLOT00)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[17] << A2B_BITP_UPMASK0_RXUPSLOT01 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[18] << A2B_BITP_UPMASK0_RXUPSLOT02)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[19] << A2B_BITP_UPMASK0_RXUPSLOT03 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[20] << A2B_BITP_UPMASK0_RXUPSLOT04)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[21] << A2B_BITP_UPMASK0_RXUPSLOT05 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[22] << A2B_BITP_UPMASK0_RXUPSLOT06)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[23] << A2B_BITP_UPMASK0_RXUPSLOT07 );

    pSlvNode->slotEnh.upmask3 = ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[24] << A2B_BITP_UPMASK0_RXUPSLOT00)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[25] << A2B_BITP_UPMASK0_RXUPSLOT01 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[26] << A2B_BITP_UPMASK0_RXUPSLOT02)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[27] << A2B_BITP_UPMASK0_RXUPSLOT03 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[28] << A2B_BITP_UPMASK0_RXUPSLOT04)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[29] << A2B_BITP_UPMASK0_RXUPSLOT05 )|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[30] << A2B_BITP_UPMASK0_RXUPSLOT06)|\
			   ((a2b_UInt8)pSlvCfg->sConfigCtrlSettings.anUpstreamConsumeSlots[31] << A2B_BITP_UPMASK0_RXUPSLOT07 );

    pSlvNode->i2cI2sRegs.i2srrctl = ( pSlvCfg->sI2SSettings.sI2SRateConfig.bRRStrobe << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENSTRB )|\
			   ( pSlvCfg->sI2SSettings.sI2SRateConfig.bRRStrobeDirection << (a2b_UInt8)A2B_BITP_I2SRRCTL_STRBDIR )|\
			   ( pSlvCfg->sI2SSettings.sI2SRateConfig.bRRValidBitExtraBit << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENXBIT )|\
			   ( pSlvCfg->sI2SSettings.sI2SRateConfig.bRRValidBitLSB << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENVLSB )|\
			   ( pSlvCfg->sI2SSettings.sI2SRateConfig.bRRValidBitExtraCh << (a2b_UInt8)A2B_BITP_I2SRRCTL_ENCHAN );


    pSlvNode->i2cI2sRegs.i2srrsoffs = ( pSlvCfg->sI2SSettings.sI2SRateConfig.nRROffset << (a2b_UInt8)A2B_BITP_I2SRRSOFFS_RRSOFFSET);


    pSlvNode->i2cI2sRegs.i2srate = (pSlvCfg->sI2SSettings.sI2SRateConfig.bReduce << (a2b_UInt8)A2B_BITP_I2SRATE_REDUCE )| \
			   (pSlvCfg->sI2SSettings.sI2SRateConfig.nSamplingRate)| \
			   (pSlvCfg->sI2SSettings.sI2SRateConfig.nRBCLKRate << (a2b_UInt8)A2B_BITP_I2SRATE_BLCKRATE ) |\
			   (pSlvCfg->sI2SSettings.sI2SRateConfig.bShareBusSlot << (a2b_UInt8)A2B_BITP_I2SRATE_SHARE );

    pSlvNode->i2cI2sRegs.pdmctl   |=  ( pSlvCfg->sPDMSettings.bHPFUse << (a2b_UInt8)(A2B_BITP_PDMCTL_HPFEN) ) | \
    						              ( pSlvCfg->sPDMSettings.nPDMRate <<(a2b_UInt8)(A2B_BITP_PDMCTL_PDMRATE)  |
    						                pSlvCfg->sPDMSettings.nNumSlotsPDM0 | pSlvCfg->sPDMSettings.nNumSlotsPDM1);


    pSlvNode->intRegs.intmsk0 = ( pSlvCfg->sInterruptSettings.bReportHDCNTErr       << (a2b_UInt8)A2B_BITP_INTPND0_HDCNTERR) | \
			( pSlvCfg->sInterruptSettings.bReportDDErr          << (a2b_UInt8)A2B_BITP_INTPND0_DDERR) | \
			( pSlvCfg->sInterruptSettings.bReportCRCErr         << (a2b_UInt8)A2B_BITP_INTPND0_CRCERR) | \
			( pSlvCfg->sInterruptSettings.bReportDataParityErr  << (a2b_UInt8)A2B_BITP_INTPND0_DPERR) | \
			( pSlvCfg->sInterruptSettings.bReportPwrErr         << (a2b_UInt8)A2B_BITP_INTPND0_PWRERR ) | \
			( pSlvCfg->sInterruptSettings.bReportErrCntOverFlow << (a2b_UInt8)A2B_BITP_INTPND0_BECOVF ) | \
			( pSlvCfg->sInterruptSettings.bReportSRFMissErr 	<< (a2b_UInt8)A2B_BITP_INTPND0_SRFERR ) |\
			( pSlvCfg->sInterruptSettings.bReportSRFCrcErr 	    << (a2b_UInt8)A2B_BITP_INTPND0_SRFCRCERR );

    pSlvNode->intRegs.intmsk1 =  (pSlvCfg->sInterruptSettings.bReportGPIO0 << (a2b_UInt8)A2B_BITP_INTPND1_IO0PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO1 << (a2b_UInt8)A2B_BITP_INTPND1_IO1PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO2 << (a2b_UInt8)A2B_BITP_INTPND1_IO2PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO3 << (a2b_UInt8)A2B_BITP_INTPND1_IO3PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO4 << (a2b_UInt8)A2B_BITP_INTPND1_IO4PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO5 << (a2b_UInt8)A2B_BITP_INTPND1_IO5PND ) | \
    								( pSlvCfg->sInterruptSettings.bReportGPIO6 << (a2b_UInt8)A2B_BITP_INTPND1_IO6PND ) |\
    								( pSlvCfg->sInterruptSettings.bReportGPIO7 << (a2b_UInt8)A2B_BITP_INTPND1_IO7PND );

    pSlvNode->pinIoRegs.pincfg = (pSlvCfg->sGPIOSettings.bHighDriveStrength << (a2b_UInt8)A2B_BITP_PINCFG_DRVSTR ) |\
			   (pSlvCfg->sGPIOSettings.bIRQInv << (a2b_UInt8)A2B_BITP_PINCFG_IRQINV) |\
			   (pSlvCfg->sGPIOSettings.bIRQTriState << (a2b_UInt8)A2B_BITP_PINCFG_IRQTS);

    /* Parsing Pin Mux */
    adi_a2b_ParseSlavePinMux7( pSlvNode ,  pSlvCfg);

    /* GPIOD  */

    pSlvNode->has_gpioDist = true;
    pSlvNode->gpioDist.gpiodinv = (pSlvCfg->sGPIODSettings.sGPIOD0Config.bGPIOSignalInv << 0u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD1Config.bGPIOSignalInv << 1u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD2Config.bGPIOSignalInv << 2u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD3Config.bGPIOSignalInv << 3u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD4Config.bGPIOSignalInv << 4u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD5Config.bGPIOSignalInv << 5u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD6Config.bGPIOSignalInv << 6u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD7Config.bGPIOSignalInv << 7u);

    pSlvNode->gpioDist.gpioden = (pSlvCfg->sGPIODSettings.sGPIOD0Config.bGPIODistance << 0u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD1Config.bGPIODistance << 1u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD2Config.bGPIODistance << 2u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD3Config.bGPIODistance << 3u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD4Config.bGPIODistance << 4u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD5Config.bGPIODistance << 5u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD6Config.bGPIODistance << 6u) |\
			 (pSlvCfg->sGPIODSettings.sGPIOD7Config.bGPIODistance << 7u);

    /* GPIOD Mask update */
    psGPIODMask[0] = &pSlvNode->gpioDist.gpiod0msk;
    psGPIODMask[1] = &pSlvNode->gpioDist.gpiod1msk;
    psGPIODMask[2] = &pSlvNode->gpioDist.gpiod2msk;
    psGPIODMask[3] = &pSlvNode->gpioDist.gpiod3msk;
    psGPIODMask[4] = &pSlvNode->gpioDist.gpiod4msk;
    psGPIODMask[5] = &pSlvNode->gpioDist.gpiod5msk;
    psGPIODMask[6] = &pSlvNode->gpioDist.gpiod6msk;
    psGPIODMask[7] = &pSlvNode->gpioDist.gpiod7msk;

    asGPIODConfig[0] = pSlvCfg->sGPIODSettings.sGPIOD0Config;
    asGPIODConfig[1] = pSlvCfg->sGPIODSettings.sGPIOD1Config;
    asGPIODConfig[2] = pSlvCfg->sGPIODSettings.sGPIOD2Config;
    asGPIODConfig[3] = pSlvCfg->sGPIODSettings.sGPIOD3Config;
    asGPIODConfig[4] = pSlvCfg->sGPIODSettings.sGPIOD4Config;
    asGPIODConfig[5] = pSlvCfg->sGPIODSettings.sGPIOD5Config;
    asGPIODConfig[6] = pSlvCfg->sGPIODSettings.sGPIOD6Config;
    asGPIODConfig[7] = pSlvCfg->sGPIODSettings.sGPIOD7Config;

    for(nGPIOIndex = 0u; nGPIOIndex < 8u; nGPIOIndex++)
    {

    	*(psGPIODMask[nGPIOIndex]) =  (a2b_UInt16) ( (asGPIODConfig[nGPIOIndex].abBusPortMask[0] << 0u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[1] << 1u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[2] << 2u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[3] << 3u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[4] << 4u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[5] << 5u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[6] << 6u) |\
    								(asGPIODConfig[nGPIOIndex].abBusPortMask[7] << 7u));
    }

    /*ClockCFg1 & CFG2*/
    pSlvNode->pinIoRegs.clk1cfg |= (pSlvCfg->sClkOutSettings.bClk1Div << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1DIV) |\
			                   (pSlvCfg->sClkOutSettings.bClk1PreDiv << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1PDIV) |\
			                       (pSlvCfg->sClkOutSettings.bClk1Inv << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1INV);

    pSlvNode->pinIoRegs.clk2cfg |= 	(pSlvCfg->sClkOutSettings.bClk2Div << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1DIV) |\
			   (pSlvCfg->sClkOutSettings.bClk2PreDiv << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1PDIV) |\
			   (pSlvCfg->sClkOutSettings.bClk2Inv << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1INV);

    pSlvNode->ctrlRegs.suscfg = pSlvCfg->sRegSettings.nSUSCFG;

    pSlvNode->has_mbox = true;
    pSlvNode->mbox.mbox0ctl = pSlvCfg->sRegSettings.nMBOX0CTL;
    pSlvNode->mbox.mbox1ctl = pSlvCfg->sRegSettings.nMBOX1CTL;

	pSlvNode->i2cI2sRegs.i2sgcfg |= ( pSlvCfg->sI2SSettings.bSerialRxOnDTx1 << (a2b_UInt8)A2B_BITP_I2SGCFG_RXONDTX1 );


    pSlvNode->i2cI2sRegs.pdmctl2   |= ( (pSlvCfg->sPDMSettings.bPDMInvClk 			<< (a2b_UInt8)A2B_BITP_PDMCTL2_PDMINVCLK) |
    									(pSlvCfg->sPDMSettings.bPDMAltClk 			<< (a2b_UInt8)A2B_BITP_PDMCTL2_PDMALTCLK) |
										(pSlvCfg->sPDMSettings.bPDM0FallingEdgeFrst << (a2b_UInt8)A2B_BITP_PDMCTL2_PDM0FFRST) |
										(pSlvCfg->sPDMSettings.bPDM1FallingEdgeFrst << (a2b_UInt8)A2B_BITP_PDMCTL2_PDM1FFRST) |
										(pSlvCfg->sPDMSettings.ePDMDestination 		<< (a2b_UInt8)A2B_BITP_PDMCTL2_PDMDEST) );

    pSlvNode->i2cI2sRegs.pllctl  = pSlvCfg->sRegSettings.nPLLCTL;

    pSlvNode->tuningRegs.txactl	   |= (pSlvCfg->sRegSettings.nTXACTL);
    pSlvNode->tuningRegs.txbctl	   |= (pSlvCfg->sRegSettings.nTXBCTL);
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterPinMux12
*
*  Helper routine to Parse the Master Pin Mux 1&2 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseMasterPinMux12(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg)
{
    /* Pin multiplex for GPIO 1*/
    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO1PinUsage)
    {
        case A2B_GPIO_1_INPUT:
        	pMstrNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO1IEN);
        	pMstrNode->pinIoRegs.pinten        |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO1Interrupt << A2B_BITP_PINTEN_IO1IE);
        	pMstrNode->pinIoRegs.pintinv       |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO1IntPolarity << A2B_BITP_PINTINV_IO1INV);
            break;
        case A2B_GPIO_1_OUTPUT:
        	pMstrNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO1OEN);
        	pMstrNode->pinIoRegs.gpiodat       |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO1Val << A2B_BITP_GPIODAT_IO1DAT);
            break;
        case A2B_GPIO_1_AS_CLKOUT:
        	pMstrNode->pinIoRegs.clk1cfg    |= (1u << (a2b_UInt8)A2B_BITP_CLKOUT1_CLK1EN);
            break;
        /* case A2B_GPIO_1_DISABLE:
            break; */
        default:
        break;

    }

    /* Pin multiplex for GPIO 2*/
    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO2PinUsage)
    {
        case A2B_GPIO_2_INPUT:
        	pMstrNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO2IEN);
        	pMstrNode->pinIoRegs.pinten        |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO2Interrupt << A2B_BITP_PINTEN_IO2IE);
        	pMstrNode->pinIoRegs.pintinv       |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO2IntPolarity << A2B_BITP_PINTINV_IO2INV);
            break;
        case A2B_GPIO_2_OUTPUT:
        	pMstrNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO2OEN);
        	pMstrNode->pinIoRegs.gpiodat       |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO2Val << A2B_BITP_GPIODAT_IO2DAT);
            break;
        case A2B_GPIO_2_AS_CLKOUT:
        	pMstrNode->pinIoRegs.clk2cfg     |= (1u << (a2b_UInt8)A2B_BITP_CLKOUT2_CLK1EN);
            break;
        /* case A2B_GPIO_2_DISABLE:
            break; */
        default:
        break;

    }
}


/*!****************************************************************************
*
*  \b              adi_a2b_ParseMasterPinMux7
*
*  Helper routine to Parse the Master Pin Mux 7 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pMstrNode Ptr to Master Node of the BDD struct
*  \param          [in]    pMstCfg   Ptr to Master Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseMasterPinMux7(
		bdd_Node *pMstrNode,
		ADI_A2B_MASTER_NCD* pMstCfg)
{

    switch(pMstCfg->sGPIOSettings.sPinMuxSettings.bGPIO7PinUsage)
    {
 	   case A2B_GPIO_7_INPUT:
 		  pMstrNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO7IEN);
 		  pMstrNode->pinIoRegs.pinten        |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO7Interrupt << A2B_BITP_PINTEN_IO7IE);
 		  pMstrNode->pinIoRegs.pintinv       |= (pMstCfg->sGPIOSettings.sPinIntConfig.bGPIO7IntPolarity << A2B_BITP_PINTINV_IO7INV);
 		   break;
 	   case A2B_GPIO_7_OUTPUT:
 		  pMstrNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO7OEN);
 		  pMstrNode->pinIoRegs.gpiodat       |= (pMstCfg->sGPIOSettings.sOutPinVal.bGPIO7Val << A2B_BITP_GPIODAT_IO7DAT);
 		   break;
 	   /*case A2B_GPIO_7_DISABLE:
 		   break; */
 	   case A2B_GPIO_7_PDMCLK:
 		  pMstrNode->i2cI2sRegs.pdmctl2		 |= (1u << (a2b_UInt8)A2B_BITP_PDMCTL2_PDMALTCLK);
 		   break;
 	   default:
 	   break;

    }
}

/*!****************************************************************************
*
*  \b              adi_a2b_ParseSlavePinMux7
*
*  Helper routine to Parse the Slave Pin Mux 7 from BCF to BDD
*  fields of BDD structure
*
*  \param          [in]    pSlvNode Ptr to Slave Node of the BDD struct
*  \param          [in]    pSlvCfg   Ptr to Slave Node of the BCF struct
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void  adi_a2b_ParseSlavePinMux7(
		bdd_Node *pSlvNode,
		ADI_A2B_SLAVE_NCD* pSlvCfg)
{

   /* Pin multiplex for GPIO 7*/
   switch(pSlvCfg->sGPIOSettings.sPinMuxSettings.bGPIO7PinUsage)
   {
	   case A2B_GPIO_7_INPUT:
		   pSlvNode->pinIoRegs.gpioien       |= (1u << (a2b_UInt8)A2B_BITP_GPIOIEN_IO7IEN);
		   pSlvNode->pinIoRegs.pinten        |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO7Interrupt << A2B_BITP_PINTEN_IO7IE);
		   pSlvNode->pinIoRegs.pintinv       |= (pSlvCfg->sGPIOSettings.sPinIntConfig.bGPIO7IntPolarity << A2B_BITP_PINTINV_IO7INV);
		   break;
	   case A2B_GPIO_7_OUTPUT:
		   pSlvNode->pinIoRegs.gpiooen       |= (1u << (a2b_UInt8)A2B_BITP_GPIOOEN_IO7OEN);
		   pSlvNode->pinIoRegs.gpiodat       |= (pSlvCfg->sGPIOSettings.sOutPinVal.bGPIO7Val << A2B_BITP_GPIODAT_IO7DAT);
		   break;
	   /*case A2B_GPIO_7_DISABLE:
		   break; */
 	   case A2B_GPIO_7_PDMCLK:
 		  pSlvNode->i2cI2sRegs.pdmctl2		 |= (1u << (a2b_UInt8)A2B_BITP_PDMCTL2_PDMALTCLK);
 		  break;
	   default:
	   break;

   }
}
#endif


/*!****************************************************************************
*
*  \b              adi_a2b_ParsePeriCfgFrComBCF
*
*  Helper routine parse peripheral info from compressed BCF
*
*  \param          [in]   pBusDescription      	pointer to bus decription
*  \param          [out]  aPeriDownloadTable    Table tp store peripheral config info
*  \param          [in]   nBusIndex       		Bus/Chain Index
*
*  \pre            None
*
*  \post           None
*
*  \return         True = success, False = Failure
*
******************************************************************************/
void adi_a2b_ParsePeriCfgFrComBCF(ADI_A2B_COMPR_BCD*  pBusDescription, ADI_A2B_NODE_PERICONFIG aPeriDownloadTable[], a2b_UInt8 nBusIndex)
{
	ADI_A2B_COMPR_MASTER_SLAVE_CONFIG* pMasterSlaveChain;
    a2b_UInt8 nIndex1,nIndex2;
    a2b_UInt8 nNumConfig;
    a2b_UInt8 nBranchID = 0U;
    ADI_A2B_NODE_PERICONFIG_DATA *pSlvCfg;

    /* Number of peripheral devices connected to target processor */
    aPeriDownloadTable[0u].nNumConfig = 0u;
    for(nIndex2 = 0u; nIndex2 < pBusDescription->sTargetProperties.nNumPeriDevice; nIndex2++)
    {
        /* Include only if the configuration exists */
        if(pBusDescription->sTargetProperties.apPeriConfig != A2B_NULL)
        {
            /* Number of config */
            nNumConfig = aPeriDownloadTable[0u].nNumConfig;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].bActive = 1u;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nConnectedNodeID       = 0xFFu;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nDeviceAddress         = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->nI2Caddr;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nNumPeriConfigUnit     = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->nNumPeriConfigUnit;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].paPeriConfigUnit       = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->paPeriConfigUnit;
            aPeriDownloadTable[0u].nNumConfig++;
        }
        else
        {
        	aPeriDownloadTable[0u].aDeviceConfig[nIndex2].bActive        = 0u;
        }

    }

    /* Initialize empty fields */
    for(nIndex2 = pBusDescription->sTargetProperties.nNumPeriDevice; nIndex2 < (a2b_UInt8)ADI_A2B_MAX_DEVICES_PER_NODE; nIndex2++  )
    {
    	aPeriDownloadTable[0u].aDeviceConfig[nIndex2].bActive        = 0u;
    }

	pMasterSlaveChain = pBusDescription->apNetworkconfig[nBusIndex];
	nBranchID = nBusIndex;
	for(nIndex1 = 0u ; nIndex1 < (a2b_UInt8)pMasterSlaveChain->nNumSlaveNode; nIndex1++)
	{
		pSlvCfg = pMasterSlaveChain->apNodePericonfig[nIndex1];
		aPeriDownloadTable[nIndex1+1].nNumConfig = 0u;
		/* Peripheral update */
		for(nIndex2 = 0u; nIndex2 < pSlvCfg->nNumPeriDevice; nIndex2++)
		{
			if(pSlvCfg->apNodePeriDeviceConfig[nIndex2]->paPeriConfigUnit != A2B_NULL)
			{
				/* Number of config */
				nNumConfig = aPeriDownloadTable[nIndex1+1].nNumConfig;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        = 1u;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nConnectedNodeID         = nIndex2;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nDeviceAddress           = pSlvCfg->apNodePeriDeviceConfig[nIndex2]->nI2Caddr;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nNumPeriConfigUnit       = pSlvCfg->apNodePeriDeviceConfig[nIndex2]->nNumPeriConfigUnit;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].paPeriConfigUnit         = pSlvCfg->apNodePeriDeviceConfig[nIndex2]->paPeriConfigUnit;
				aPeriDownloadTable[nIndex1+1u].nNumConfig++;
			}
			else
			{
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        = 0u;
			}
		}

		for(nIndex2 = pSlvCfg->nNumPeriDevice; nIndex2 < (a2b_UInt8)ADI_A2B_MAX_DEVICES_PER_NODE; nIndex2++  )
		{
			aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        	= 0u;
		}
	}

}


/*****************************************************************************************/
/*!
@brief      This function parses Bus Configuration Data(BCD) to Peripheral Config Table

@param [in] pBusDescription       Pointer to bus configuration data
@param [in] aPeriDownloadTable    Framework configuration pointer
@param [in] nBusIndex			  Chain/Bus/network index

@return     void

*/
/******************************************************************************************/
void adi_a2b_ParsePeriCfgTable(ADI_A2B_BCD*  pBusDescription, ADI_A2B_NODE_PERICONFIG aPeriDownloadTable[], a2b_UInt8 nBusIndex)
{
    ADI_A2B_MASTER_SLAVE_CONFIG* pMasterSlaveChain;
    ADI_A2B_SLAVE_NCD* pSlvCfg;
    a2b_UInt8 nIndex1,nIndex2;
    a2b_UInt8 nNumConfig;

    /* Number of peripheral devices connected to target processor */
    aPeriDownloadTable[0u].nNumConfig = 0u;

    for(nIndex2 = 0u; nIndex2 < pBusDescription->sTargetProperties.nNumPeriDevice; nIndex2++)
    {
        /* Include only if the configuration exists */
        if(pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->paPeriConfigUnit != A2B_NULL)
        {
            /* Number of config */
            nNumConfig = aPeriDownloadTable[0u].nNumConfig;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].bActive = 1u;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nConnectedNodeID       = 0xFFu;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nDeviceAddress         = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->nI2Caddr;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].nNumPeriConfigUnit     = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->nNumPeriConfigUnit;
            aPeriDownloadTable[0u].aDeviceConfig[nNumConfig].paPeriConfigUnit       = pBusDescription->sTargetProperties.apPeriConfig[nIndex2]->paPeriConfigUnit;
            aPeriDownloadTable[0u].nNumConfig++;
        }
        else
        {
        	aPeriDownloadTable[0u].aDeviceConfig[nIndex2].bActive        = 0u;
        }

    }
    /* Initialize empty fields */
    for(nIndex2 = pBusDescription->sTargetProperties.nNumPeriDevice; nIndex2 < (a2b_UInt8)ADI_A2B_MAX_DEVICES_PER_NODE; nIndex2++  )
    {
    	aPeriDownloadTable[0u].aDeviceConfig[nIndex2].bActive        = 0u;
    }

	pMasterSlaveChain = pBusDescription->apNetworkconfig[nBusIndex];

	for(nIndex1 = 0u ; nIndex1 < (a2b_UInt8)pMasterSlaveChain->nNumSlaveNode; nIndex1++)
	{
		pSlvCfg = pMasterSlaveChain->apSlaveConfig[nIndex1];
		aPeriDownloadTable[nIndex1+1].nNumConfig = 0u;
		/* Peripheral update */
		for(nIndex2 = 0u; nIndex2 < pSlvCfg->nNumPeriDevice; nIndex2++)
		{
			if(pSlvCfg->apPeriConfig[nIndex2]->paPeriConfigUnit != A2B_NULL)
			{
				/* Number of config */
				nNumConfig = aPeriDownloadTable[nIndex1+1].nNumConfig;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        = 1u;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nConnectedNodeID         = pSlvCfg->nNodeID;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nDeviceAddress           = pSlvCfg->apPeriConfig[nIndex2]->nI2Caddr;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].nNumPeriConfigUnit       = pSlvCfg->apPeriConfig[nIndex2]->nNumPeriConfigUnit;
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nNumConfig].paPeriConfigUnit         = pSlvCfg->apPeriConfig[nIndex2]->paPeriConfigUnit;
				aPeriDownloadTable[nIndex1+1u].nNumConfig++;
			}
			else
			{
				aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        = 0u;
			}
		}

		for(nIndex2 = pSlvCfg->nNumPeriDevice; nIndex2 < (a2b_UInt8)ADI_A2B_MAX_DEVICES_PER_NODE; nIndex2++  )
		{
			aPeriDownloadTable[nIndex1+1u].aDeviceConfig[nIndex2].bActive        = 0u;
		}
	}

}


/*!****************************************************************************
*
*  \b              adi_a2b_CheckforDefault
*
*  Helper routine to check if a node register is configured for default value and
*  if so sets the HAS status in the bdd_node structure as 'false' so that stack
*  does not configure the register during discovery.
*
*  \param          [in]    pNode Pointer to Node of the BDD structure
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_CheckforDefault(bdd_Node *pNode)
{

	/* Control registers */
    A2B_UPDATE_HAS(pNode->ctrlRegs, bcdnslots, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, ldnslots, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, lupslots, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, dnslots, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, upslots, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, respcycs, 0x40);
	A2B_UPDATE_HAS(pNode->ctrlRegs, slotfmt, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, suscfg, 0);
	A2B_UPDATE_HAS(pNode->ctrlRegs, datctl, 0);

	/* Interupt registers */
    A2B_UPDATE_HAS(pNode->intRegs, intmsk0,0);
    A2B_UPDATE_HAS(pNode->intRegs, intmsk1,0);
    A2B_UPDATE_HAS(pNode->intRegs, intmsk2,0);
    A2B_UPDATE_HAS(pNode->intRegs, becctl,0);

    /* I2C/i2s Registers */
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2ccfg     ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,pllctl     ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2sgcfg    ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2scfg     ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2srate    ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2stxoffset,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2srxoffset,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,syncoffset ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,pdmctl     ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,errmgmt    ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2srrate   ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2srrctl   ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,i2srrsoffs ,0);
    A2B_UPDATE_HAS(pNode->i2cI2sRegs,pdmctl2    ,0);

    /* PIn IO */
    A2B_UPDATE_HAS(pNode->pinIoRegs,clkcfg , 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,gpiooen, 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,gpioien, 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,pinten,  0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,pintinv, 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,pincfg,  0x01);
    A2B_UPDATE_HAS(pNode->pinIoRegs,gpiodat, 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,clk1cfg, 0);
    A2B_UPDATE_HAS(pNode->pinIoRegs,clk2cfg, 0);

    /* Slot Enahnce */
    A2B_UPDATE_HAS(pNode->slotEnh,upmask0, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,upmask1, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,upmask2, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,upmask3, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,upoffset,0);
    A2B_UPDATE_HAS(pNode->slotEnh,dnmask0, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,dnmask1, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,dnmask2, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,dnmask3, 0);
    A2B_UPDATE_HAS(pNode->slotEnh,dnoffset,0);

    /* GPIO over distance */
    A2B_UPDATE_HAS(pNode->gpioDist,gpioden,  0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod0msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod1msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod2msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod3msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod4msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod5msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod6msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiod7msk,0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpioddat, 0);
    A2B_UPDATE_HAS(pNode->gpioDist,gpiodinv, 0);

    /* Mail Box */
	A2B_UPDATE_HAS(pNode->mbox,mbox0ctl, 0);
	A2B_UPDATE_HAS(pNode->mbox,mbox1ctl, 0);

	/* Tuning Registers */
	A2B_UPDATE_HAS(pNode->tuningRegs,txactl, 0);
	A2B_UPDATE_HAS(pNode->tuningRegs,txbctl, 0);

}

/*!****************************************************************************
*
*  \b              adi_a2b_CheckforAutoConfig
*
*  Helper routine to clear the HAS status of relevant registers if auto configuration
*  via EEPROM is enabled for a Node.
*
*  \param          [in]    pNode 		Pointer to Node of the BDD structure
*  \param          [in]    bAutoConfig  Auto Configuration Flag
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_CheckforAutoConfig(bdd_Node *pNode, bool bAutoConfig)
{
	if(bAutoConfig == true)
	{
		/* Control registers */
		A2B_CLEAR_HAS(pNode->ctrlRegs, bcdnslots);
		A2B_CLEAR_HAS(pNode->ctrlRegs, ldnslots);
		A2B_CLEAR_HAS(pNode->ctrlRegs, lupslots);


		/* I2C/i2s Registers */
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2ccfg     );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,pllctl     );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2sgcfg    );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2scfg     );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2srate    );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2stxoffset);
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,i2srxoffset);
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,syncoffset );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,pdmctl     );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,errmgmt    );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,pdmctl2    );

		/* PIn IO */
		A2B_CLEAR_HAS(pNode->pinIoRegs,gpiodat);
		A2B_CLEAR_HAS(pNode->pinIoRegs,gpiooen);
		A2B_CLEAR_HAS(pNode->pinIoRegs,gpioien);
		A2B_CLEAR_HAS(pNode->pinIoRegs,pinten);
		A2B_CLEAR_HAS(pNode->pinIoRegs,pintinv);
		A2B_CLEAR_HAS(pNode->pinIoRegs,pincfg);

	}


}


/*!****************************************************************************
*
*  \b              adi_a2b_SetforAllReg_242x_master
*
*  Helper routine to consider all the valid registers of AD242x master for configuration
*
*  \param          [in]    pNode Pointer to Master Node of the BDD structure
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_SetforAllReg_242x_master(bdd_Node *pNode)
{
	/* Control registers */
	A2B_SET_HAS(pNode->ctrlRegs, dnslots);
	A2B_SET_HAS(pNode->ctrlRegs, upslots);
	A2B_SET_HAS(pNode->ctrlRegs, respcycs);
	A2B_SET_HAS(pNode->ctrlRegs, slotfmt);
	A2B_SET_HAS(pNode->ctrlRegs, suscfg);
	A2B_SET_HAS(pNode->ctrlRegs, datctl);

	/* Interupt registers */
    A2B_SET_HAS(pNode->intRegs, intmsk0);
    A2B_SET_HAS(pNode->intRegs, intmsk1);
    A2B_SET_HAS(pNode->intRegs, intmsk2);
    A2B_SET_HAS(pNode->intRegs, becctl);

    /* I2C/i2s Registers */
    A2B_SET_HAS(pNode->i2cI2sRegs,i2ccfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2sgcfg    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2scfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srate    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2stxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,syncoffset );
    A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,errmgmt    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srrctl   );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srrsoffs );


    /* PIn IO */
    A2B_SET_HAS(pNode->pinIoRegs,gpiooen);
    A2B_SET_HAS(pNode->pinIoRegs,gpioien);
    A2B_SET_HAS(pNode->pinIoRegs,pinten);
    A2B_SET_HAS(pNode->pinIoRegs,pintinv);
    A2B_SET_HAS(pNode->pinIoRegs,pincfg);
    A2B_SET_HAS(pNode->pinIoRegs,gpiodat);
    A2B_SET_HAS(pNode->pinIoRegs,clk1cfg);
    A2B_SET_HAS(pNode->pinIoRegs,clk2cfg);


    /* GPIO over distance */
    A2B_SET_HAS(pNode->gpioDist,gpioden );
    A2B_SET_HAS(pNode->gpioDist,gpiod0msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod1msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod2msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod3msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod4msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod5msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod6msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod7msk);
    A2B_SET_HAS(pNode->gpioDist,gpioddat);
    A2B_SET_HAS(pNode->gpioDist,gpiodinv);

    /* Mail Box */
	A2B_SET_HAS(pNode->mbox,mbox0ctl);
	A2B_SET_HAS(pNode->mbox,mbox1ctl);

	/* Set only for AD2428 series */
	if( A2B_IS_AD2428X_CHIP(pNode->nodeDescr.vendor, pNode->nodeDescr.product))
	{
		A2B_SET_HAS(pNode->i2cI2sRegs,pllctl     );
		A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl2    );
		/* Tuning Registers */
		A2B_SET_HAS(pNode->tuningRegs,txactl);
		A2B_SET_HAS(pNode->tuningRegs,txbctl);
		A2B_SET_HAS(pNode->ctrlRegs,control );
	}
	else
	{
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,pllctl     );
		A2B_CLEAR_HAS(pNode->i2cI2sRegs,pdmctl2    );
		/* Tuning Registers */
		A2B_CLEAR_HAS(pNode->tuningRegs,txactl);
		A2B_CLEAR_HAS(pNode->tuningRegs,txbctl);
		A2B_CLEAR_HAS(pNode->ctrlRegs,control );

	}

}

/*!****************************************************************************
*
*  \b              adi_a2b_SetforAllReg_242x_slave
*
*  Helper routine to consider all the valid registers of AD241x slave for configuration
*
*  \param          [in]    pNode Pointer to Slave Node of the BDD structure
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_SetforAllReg_242x_slave(bdd_Node *pNode)
{
	/* Control registers */
    A2B_SET_HAS(pNode->ctrlRegs, bcdnslots);
	A2B_SET_HAS(pNode->ctrlRegs, ldnslots);
	A2B_SET_HAS(pNode->ctrlRegs, lupslots);
	A2B_SET_HAS(pNode->ctrlRegs, dnslots);
	A2B_SET_HAS(pNode->ctrlRegs, upslots);
	A2B_SET_HAS(pNode->ctrlRegs, respcycs);
	A2B_SET_HAS(pNode->ctrlRegs, slotfmt);
	A2B_SET_HAS(pNode->ctrlRegs, suscfg);

	/* Interupt registers */
    A2B_SET_HAS(pNode->intRegs, intmsk0);
    A2B_SET_HAS(pNode->intRegs, intmsk1);
    A2B_SET_HAS(pNode->intRegs, intmsk2);
    A2B_SET_HAS(pNode->intRegs, becctl);

    /* I2C/i2s Registers */
    A2B_SET_HAS(pNode->i2cI2sRegs,i2ccfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2sgcfg    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2scfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srate    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2stxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,syncoffset );
    A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,errmgmt    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srrate   );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srrsoffs );


    /* PIn IO */
    A2B_SET_HAS(pNode->pinIoRegs,gpiooen);
    A2B_SET_HAS(pNode->pinIoRegs,gpioien);
    A2B_SET_HAS(pNode->pinIoRegs,pinten);
    A2B_SET_HAS(pNode->pinIoRegs,pintinv);
    A2B_SET_HAS(pNode->pinIoRegs,pincfg);
    A2B_SET_HAS(pNode->pinIoRegs,gpiodat);
    A2B_SET_HAS(pNode->pinIoRegs,clk1cfg);
    A2B_SET_HAS(pNode->pinIoRegs,clk2cfg);

    /* Slot Enahnce */
    A2B_SET_HAS(pNode->slotEnh,upmask0);
    A2B_SET_HAS(pNode->slotEnh,upmask1);
    A2B_SET_HAS(pNode->slotEnh,upmask2);
    A2B_SET_HAS(pNode->slotEnh,upmask3);
    A2B_SET_HAS(pNode->slotEnh,upoffset);
    A2B_SET_HAS(pNode->slotEnh,dnmask0);
    A2B_SET_HAS(pNode->slotEnh,dnmask1);
    A2B_SET_HAS(pNode->slotEnh,dnmask2);
    A2B_SET_HAS(pNode->slotEnh,dnmask3);
    A2B_SET_HAS(pNode->slotEnh,dnoffset);

    /* GPIO over distance */
    A2B_SET_HAS(pNode->gpioDist,gpioden );
    A2B_SET_HAS(pNode->gpioDist,gpiod0msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod1msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod2msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod3msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod4msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod5msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod6msk);
    A2B_SET_HAS(pNode->gpioDist,gpiod7msk);
    A2B_SET_HAS(pNode->gpioDist,gpioddat);
    A2B_SET_HAS(pNode->gpioDist,gpiodinv);

    /* Mail Box */
	A2B_SET_HAS(pNode->mbox,mbox0ctl);
	A2B_SET_HAS(pNode->mbox,mbox1ctl);

	/* Set only for AD2428 series */
	if( A2B_IS_AD2428X_CHIP(pNode->nodeDescr.vendor, pNode->nodeDescr.product))
	{
	    A2B_SET_HAS(pNode->i2cI2sRegs,pllctl     );
	    A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl2    );

		/* Tuning Registers */
		A2B_SET_HAS(pNode->tuningRegs,txactl);
		A2B_SET_HAS(pNode->tuningRegs,txbctl);

		A2B_SET_HAS(pNode->ctrlRegs,control );
	}
	else
	{
	    A2B_CLEAR_HAS(pNode->i2cI2sRegs,pllctl     );
	    A2B_CLEAR_HAS(pNode->i2cI2sRegs,pdmctl2    );

		/* Tuning Registers */
	    A2B_CLEAR_HAS(pNode->tuningRegs,txactl);
	    A2B_CLEAR_HAS(pNode->tuningRegs,txbctl);
	    A2B_CLEAR_HAS(pNode->ctrlRegs,control );
	}

}

/*!****************************************************************************
*
*  \b              adi_a2b_SetforAllReg_241x_slave
*
*  Helper routine to consider all the valid registers of AD241x slave for configuration
*
*  \param          [in]    pNode Pointer to Slave Node of the BDD structure
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_SetforAllReg_241x_slave(bdd_Node *pNode)
{

	/* Control registers */
    A2B_SET_HAS(pNode->ctrlRegs, bcdnslots);
	A2B_SET_HAS(pNode->ctrlRegs, ldnslots);
	A2B_SET_HAS(pNode->ctrlRegs, lupslots);
	A2B_SET_HAS(pNode->ctrlRegs, dnslots);
	A2B_SET_HAS(pNode->ctrlRegs, upslots);
	A2B_SET_HAS(pNode->ctrlRegs, respcycs);
	A2B_SET_HAS(pNode->ctrlRegs, slotfmt);

	/* Interupt registers */
    A2B_SET_HAS(pNode->intRegs, intmsk0);
    A2B_SET_HAS(pNode->intRegs, intmsk1);
    A2B_SET_HAS(pNode->intRegs, intmsk2);
    A2B_SET_HAS(pNode->intRegs, becctl);

    /* I2C/i2s Registers */
    A2B_SET_HAS(pNode->i2cI2sRegs,i2ccfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,pllctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2sgcfg    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2scfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srate    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2stxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,syncoffset );
    A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,errmgmt    );

    /* PIn IO */
    A2B_SET_HAS(pNode->pinIoRegs,clkcfg );
    A2B_SET_HAS(pNode->pinIoRegs,gpiooen);
    A2B_SET_HAS(pNode->pinIoRegs,gpioien);
    A2B_SET_HAS(pNode->pinIoRegs,pinten);
    A2B_SET_HAS(pNode->pinIoRegs,pintinv);
    A2B_SET_HAS(pNode->pinIoRegs,pincfg);
    A2B_SET_HAS(pNode->pinIoRegs,gpiodat);


}


/*!****************************************************************************
*
*  \b              adi_a2b_SetforAllReg_241x_master
*
*  Helper routine to consider all the valid registers of AD241x master for configuration
*
*  \param          [in]    pNode Pointer to Master Node of the BDD structure
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void adi_a2b_SetforAllReg_241x_master(bdd_Node *pNode)
{

	/* Control registers */
	A2B_SET_HAS(pNode->ctrlRegs, dnslots);
	A2B_SET_HAS(pNode->ctrlRegs, upslots);
	A2B_SET_HAS(pNode->ctrlRegs, respcycs);
	A2B_SET_HAS(pNode->ctrlRegs, slotfmt);

	/* Interupt registers */
    A2B_SET_HAS(pNode->intRegs, intmsk0);
    A2B_SET_HAS(pNode->intRegs, intmsk1);
    A2B_SET_HAS(pNode->intRegs, intmsk2);
    A2B_SET_HAS(pNode->intRegs, becctl);

    /* I2C/i2s Registers */
    A2B_SET_HAS(pNode->i2cI2sRegs,i2ccfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,pllctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2sgcfg    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2scfg     );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srate    );
    A2B_SET_HAS(pNode->i2cI2sRegs,i2stxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,i2srxoffset);
    A2B_SET_HAS(pNode->i2cI2sRegs,syncoffset );
    A2B_SET_HAS(pNode->i2cI2sRegs,pdmctl     );
    A2B_SET_HAS(pNode->i2cI2sRegs,errmgmt    );

    /* PIN IO */
    A2B_SET_HAS(pNode->pinIoRegs,clkcfg );
    A2B_SET_HAS(pNode->pinIoRegs,gpiooen);
    A2B_SET_HAS(pNode->pinIoRegs,gpioien);
    A2B_SET_HAS(pNode->pinIoRegs,pinten);
    A2B_SET_HAS(pNode->pinIoRegs,pintinv);
    A2B_SET_HAS(pNode->pinIoRegs,pincfg);
    A2B_SET_HAS(pNode->pinIoRegs,gpiodat);

}
#endif
#ifdef A2B_BCF_FROM_SOC_EEPROM

#define  A2B_LVL0_EEPROM_BYTES		17u
#define  A2B_LVL0_NUM_CHIAN			6u
#define  A2B_LVL0_LVL1_PTR			8u
#define  A2B_LVL1_NUM_SLAVE			6u
/*!****************************************************************************
 *
 *  \b              a2b_get_bddFromEEPROM
 *
 *  Helper routine to Parse the SigmaStudio BCF file to generate the
 *  fields of BDD structure
 *
 *  \param          [in]    ecb      Ptr to Bus Description Struct
 *  \param          [in]    bdd      decoded BDD (e.g. from a2b_bddDecode)
 *
 *  \pre            None
 *
 *  \post           None
 *
 *  \return         None
 *
 ******************************************************************************/
a2b_HResult a2b_get_bddFromEEPROM(A2B_ECB* ecb, bdd_Network *bdd_Graph, a2b_UInt8* pBuff, a2b_UInt8 pPeriBuf[], ADI_A2B_NETWORK_CONFIG* pTgtProp)
{
	a2b_UInt8 wBuf[2] = {0, 0};
	a2b_UInt8 status = 0;
	a2b_UInt8 nNumSlaves;
	a2b_UInt8 nNumChain;
	a2b_UInt16 nBDDLength;
	a2b_UInt8 crc8;
	a2b_UInt16 nChRdIndx, nNodeIdx;
	a2b_UInt16 nPeriConfigLen;

	/* Read Level 0  */
	status = a2b_EepromWriteRead(ecb->palEcb.i2chnd, A2B_I2C_EEPROM_ADDR, 2u, wBuf, A2B_LVL0_EEPROM_BYTES, pBuff);
	crc8 = a2b_crc8(pBuff, 0u, A2B_LVL0_EEPROM_BYTES - 1);

	if(pBuff[A2B_LVL0_EEPROM_BYTES - 1] != crc8)
	{
		/* CRC Fail */
		return A2B_FALSE;
	}

    nNumChain = pBuff[A2B_LVL0_NUM_CHIAN];

    if(nNumChain > 1)
    {
    	/* Not supported */
		return A2B_FALSE;
    }
	/* Read LEVEL 1*/
	A2B_GET_UINT16_BE(nChRdIndx, pBuff, A2B_LVL0_LVL1_PTR);
	A2B_PUT_UINT16_BE(nChRdIndx, wBuf, 0u);

	/* Check CRC */
	status = a2b_EepromWriteRead(ecb->palEcb.i2chnd, A2B_I2C_EEPROM_ADDR, 2u, wBuf, A2B_LVL1_NUM_SLAVE + 1u, pBuff);
	/* Get Number of slaves  */
	nNumSlaves = pBuff[A2B_LVL1_NUM_SLAVE];

	/* Update Target properties */
	pTgtProp->bLineDiagnostics = pBuff[2] & 0x01;
	pTgtProp->bAutoDiscCriticalFault  = (pBuff[2] & 0x02u)>>1u;
	pTgtProp->bAutoRediscOnFault  = (pBuff[2] & 0x04u) >>2u;
	pTgtProp->nAttemptsCriticalFault = pBuff[3];
	pTgtProp->nRediscInterval = pBuff[4];

	nChRdIndx += (A2B_LVL1_NUM_SLAVE + 1u);
	A2B_PUT_UINT16_BE(nChRdIndx, wBuf, 0u);

	/* Total number bytes for peri info */
	nPeriConfigLen = (nNumSlaves + 1u) * 2u;

	/* Read and store Configuration Pointers  */
	status = a2b_EepromWriteRead( ecb->palEcb.i2chnd, A2B_I2C_EEPROM_ADDR, 2u, wBuf, nPeriConfigLen, pPeriBuf);
	nChRdIndx += nPeriConfigLen;
	A2B_PUT_UINT16_BE(nChRdIndx, wBuf, 0u);

	/* Read BDD Length */
	status = a2b_EepromWriteRead(ecb->palEcb.i2chnd, A2B_I2C_EEPROM_ADDR, 2u, wBuf, 2u, pBuff);
	nChRdIndx += 2u;
	A2B_PUT_UINT16_BE(nChRdIndx, wBuf, 0u);

	/* Get BDD length */
	A2B_GET_UINT16_BE(nBDDLength, pBuff, 0u);

	/* Read BDD  */
	status = a2b_EepromWriteRead(ecb->palEcb.i2chnd, A2B_I2C_EEPROM_ADDR, 2u, wBuf, nBDDLength, pBuff);

	/* Decode BDD */
	a2b_bddDecode(pBuff, nBDDLength, bdd_Graph);

	/* Explicitly ensure node level eeprom is not ignored */
	for(nNodeIdx = 0u; nNodeIdx < nNumSlaves + 1; nNodeIdx++)
	{
		bdd_Graph->nodes[nNodeIdx].ignEeprom = 0;
	}

	return status;


}

#endif

/**
 @}
*/

/**
 @}
*/
