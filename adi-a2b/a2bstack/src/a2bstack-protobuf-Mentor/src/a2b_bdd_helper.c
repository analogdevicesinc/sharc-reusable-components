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
 * \author: Mentor Graphics, Embedded Software Division
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

#include "a2b_bdd_helper.h"

/*======================= D E F I N E S ===========================*/


/*======================= L O C A L  P R O T O T Y P E S  =========*/


/*======================= D A T A  ================================*/


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
            ecb->baseEcb.masterNodeInfo.vendorId = 
                                            bdd->nodes[0].nodeDescr.vendor;
            ecb->baseEcb.masterNodeInfo.productId = 
                                            bdd->nodes[0].nodeDescr.product;
            ecb->baseEcb.masterNodeInfo.version = 
                                            bdd->nodes[0].nodeDescr.version;
        }

        ecb->baseEcb.i2cMasterAddr = bdd->masterAddr;
    }

} /* a2b_bddPalInit */
