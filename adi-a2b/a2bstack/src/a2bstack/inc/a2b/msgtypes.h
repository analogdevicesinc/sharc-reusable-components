/*=============================================================================
 *
 * Project: a2bstack
 *
 * Copyright (c) 2015 - Analog Devices Inc. All Rights Reserved.
 * This software is subject to the terms and conditions of the license set 
 * forth in the project LICENSE file. Downloading, reproducing, distributing or 
 * otherwise using the software constitutes acceptance of the license. The 
 * software may not be used except as expressly authorized under the license.
 *
 *=============================================================================
 *
 * \file:   msgtypes.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the public A2B stack message types.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_msgtypes              Message Defintions
 *  
 * This defines the public A2B stack message types.
 *
 * \{ */
/*============================================================================*/

#ifndef A2B_MSGTYPES_H_
#define A2B_MSGTYPES_H_

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "defs.h"
#ifdef A2B_FEATURE_COMM_CH
#include "adi_a2b_commch_engine.h"
#endif	/* A2B_FEATURE_COMM_CH */

/*======================= D E F I N E S ===========================*/

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgtypes_types        Message Types
 *  
 * Current Message Types
 *
 * \{ */
/*----------------------------------------------------------------------------*/
#define A2B_MSG_FIRST       (0u) /*!< Min message type (for range checking)  */

/** Unknown message type, typically to indicate error */
#define A2B_MSG_UNKNOWN     (A2B_MSG_FIRST)

#define A2B_MSG_REQUEST     (1u) /*!< Directed request */
#define A2B_MSG_NOTIFY      (2u) /*!< Notification message */

#define A2B_MSG_MAX         (3u) /*!< Max message type (for range checking)  */

/** \} -- a2bstack_msgtypes_types */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgtypes_msgreq       Request Message Commands
 *  
 * Current Request Message Commands
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/** Min message request command (for range checking)  */
#define A2B_MSGREQ_FIRST                    (0u)
 
/** Unknown message request command, typically to indicate error */ 
#define A2B_MSGREQ_UNKNOWN                  (A2B_MSGREQ_FIRST)
                                           
/** Reset the A2B network */               
#define A2B_MSGREQ_NET_RESET                (1u)
                                           
/** Start A2B network discovery */         
#define A2B_MSGREQ_NET_DISCOVERY            (2u)

/** Start A2B network discovery in diagmode */
#define A2B_MSGREQ_NET_DISCOVERY_DIAGMODE   (3u)

/** Request directed to a slave plugin to complete
 * any necessary initialization of peripherals
 * attached to the slave node. This request is made
 * during node discovery but *prior* to global audio
 * configuration being completed and audio enabled on
 * the network. The message request payload contains the
 * a2b_PluginInit structure with the *req* field initialized.
 * The response to the request includes and indication of
 * whether or not the peripheral initialization was successful.
 * It is returned in the *req* field of the a2b_PluginInit
 * structure stored in the payload section of the returned message.
 */
#define A2B_MSGREQ_PLUGIN_PERIPH_INIT       (4u)

/** Request directed to a slave plugin to de-initialize
 * any peripherals attached to the slave node. The response
 * to the request includes an indication of whether the
 * de-initialization was successful (or not). It is returned in the
 * a2b_PluginDeinit structure stored in the response message's
 * payload area.
 */
#define A2B_MSGREQ_PLUGIN_PERIPH_DEINIT     (5u)

/** Request directed to a master or slave plugin for version
 * and build information about the plugin itself.
 */
#define A2B_MSGREQ_PLUGIN_VERSION           (6u)

/** Initiate BERT request to master plugin  */
#define A2B_MSGREQ_NET_BERT_START           (7u)

/** Update BERT request to master plugin */
#define A2B_MSGREQ_NET_BERT_UPDATE          (8u)

/** Stop BERT request to master plugin */
#define A2B_MSGREQ_NET_BERT_STOP            (9u)

/** Disable Line Diagnostics request to master plugin */
#define A2B_MSGREQ_NET_DISBALE_LINEDIAG		(10u)

/** Message Transmission request to master plugin using communication channel */
#define A2B_MSGREQ_COMMCH_SEND_MSG		    (11u)

/** Transmission request to master plugin over mailbox to a particular slave node */
#define A2B_MSGREQ_SEND_MBOX_DATA		    (12u)

/** Max message request command (for range checking) */
#define A2B_MSGREQ_MAX                      (13u)

/** Arbitrary custom command.  Anything beyond this
  *  value is considered a custom command.
  */
#define A2B_MSGREQ_CUSTOM                  (100u)

/** \} -- a2bstack_msgtypes_msgreq */

/*----------------------------------------------------------------------------*/
/**
 * \defgroup a2bstack_msgtypes_msgnotify    Notify Message Commands
 *  
 * Current Notify Message Commands
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/** Min message notify command (for range checking) */
#define A2B_MSGNOTIFY_FIRST                (0u)

/** Notify type used when a plugin triggers a GPIO interrupt. Causes an
 * interrupt notification to be emitted.
 */
#define A2B_MSGNOTIFY_GPIO_INTERRUPT       (1u)

/** Notify type used when a plugin sends a power fault
 * notification.
 */
#define A2B_MSGNOTIFY_POWER_FAULT          (2u)

/** Notify type used when the stack detects <em>any</em> interrupt and
 * emits a notification. This also includes any GPIO related interrupts.
 */
#define A2B_MSGNOTIFY_INTERRUPT            (3u)

/** Notification that is emitted at the end of discovery whether it resulted
 * in success or failure. See a2b_DiscoveryStatus for an indication of the
 * payload contents.
 */
#define A2B_MSGNOTIFY_DISCOVERY_DONE       (4u)


/** Notification that is emitted to indicate a new event related to mailbox communication
 *  which includes status of transmission and any new data received over mailbox
 *  See a2b_MailboxEventInfo for an indication of the
 * payload contents.
 */
#define A2B_MSGNOTIFY_MAILBOX_EVENT       (5u)


/** Notification that is emitted to indicate a new event related to communication channel
 *  which includes status of transmission and any new message received
 *  See a2b_DiscoveryStatus for an indication of the
 * payload contents.
 */
#define A2B_MSGNOTIFY_COMMCH_EVENT       (6u)


/** Max message notify command (for range checking) */
#define A2B_MSGNOTIFY_MAX                  (7u)

/** Arbitrary custom command.  Anything at or beyond this
 *  value is considered a custom command.
 */
#define A2B_MSGNOTIFY_CUSTOM               (100u)

/** \} -- a2bstack_msgtypes_msgnotify */

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct _bdd_Network;


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_msgtypes_req      Request Messages
 *  
 * These typedefs for all system request messages.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/** 
 * \name Supporting Types
 *  
 * These typedefs used for some requests used within 
 * some system request messages--not request definitions. 
 *  
 * \{ */
/*----------------------------------------------------------------------------*/

 /**
  * Enumeration for Mailbox Event
  */
 typedef enum A2B_MAILBOX_EVENT_TYPE {
	A2B_MBOX_TX_DONE,
	A2B_MBOX_TX_TIMEOUT,
	A2B_MBOX_TX_IO_ERROR,
	A2B_MBOX_RX_DATA,
	A2B_MBOX_RX_IO_ERROR
 } A2B_MAILBOX_EVENT_TYPE;


/** Information about a single stream.  This MUST match the
 *  BDD defintion of bdd_Stream.  We want to avoid making the
 *  BDD header public to avoid confusion; thus this definition.
 */
typedef struct a2b_StreamInfo
{
    /** Stream Name */
    a2b_Char        name[16u];

    a2b_UInt32      sampleRate;
    a2b_UInt32      sampleRateMultiplier;

    /** Number of channels within the stream */
    a2b_UInt32      numChans;

} a2b_StreamInfo;

typedef struct a2b_TdmSettings
{
    a2b_UInt16      networkSampleRate;    /*!< 48000, 44100 */

    /*!< #A2B_REG_I2SRATE relevant settings: 1, 2, 4 (0 == N/A) */
    a2b_UInt8       sampleRateMultiplier;

    /* A2B_REG_I2SGCFG relevant settings (FS and BCLK related)
     * common to all RX and TX pins
     */
    /** TDM Mode: 2,4,8,16,32 */
    a2b_UInt8       tdmMode;

    /** 32 | 16 (bits) */
    a2b_UInt8       slotSize;

    /** Sync type: FALSE = single cycle, TRUE = half sample */
    a2b_Bool        halfCycle;

    /** Early Sync: FALSE = same cycle, TRUE = previous cycle */
    a2b_Bool        prevCycle;

    /** Invert Sync: FALSE = rising edge, TRUE = falling edge */
    a2b_Bool        fallingEdge;
    
    /* A2B_REG_I2SCFG, A2B_REG_I2STXOFFSET, A2B_REG_I2SRXOFFSET
     * relevent settings. Independent RX and TX settings
     * rx and tx definitions are relative to the AD2410
     *   i.e. rx = Data from the node towards the bus (AD2410 input)
     *        tx = Data from the bus towards the node (AD2410 output)
     */
    struct {
        a2b_UInt8       pinEnabled;     /*!< bitwise pin enable flags */
        a2b_Bool        interleave;     /*!< data is interleaved */
        a2b_Bool        invertBclk;     /*!< bit clock is inverted */
        a2b_UInt8       offset;
    } rx;

    struct {
        a2b_UInt8       pinEnabled;     /*!< bitwise pin enable flags */
        a2b_Bool        interleave;     /*!< data is interleaved */
        a2b_Bool        invertBclk;     /*!< bit clock is inverted */
        a2b_UInt8       offset;
        a2b_Bool        triStateBefore;
        a2b_Bool        triStateAfter;
    } tx;

    /** 
     *  \{
     *  Static list of ordered streams as seen by this node.
     *  The first NULL pointer is the end of the list.
     */
    const a2b_StreamInfo*  downstream[32u];
    const a2b_StreamInfo*  upstream[32u];
    /** \} */

    /** 
     *  \{
     *  This is the number of broadcast streams at the
     *  head of the up/downstream list.
     */
    a2b_UInt32      downstreamBcastCnt;         
    a2b_UInt32      upstreamBcastCnt;
    /** \} */

} a2b_TdmSettings;

/** \} */


/** Payload data for A2B_MSGREQ_NET_DISCOVERY within an a2bMsg request */
typedef struct a2b_NetDiscovery
{
    /** Input (request) parameters */
    struct {
        /** BDD file loaded into a binary form.  This MUST be a static
         *  reference that is available for the life of the stack.  This
         *  pointer is copied and used within the master plugin.
         */
        const struct _bdd_Network* bdd;

        /** EEPROM/Peripheral Pkg loaded into a binary form */
        const a2b_Byte*       periphPkg;

        /** Length/Size of the periphPkg */
        a2b_UInt32      pkgLen;

        /** Deinit all plugins prior to discovery */
        a2b_Bool        deinitFirst;

    } req;

    /** Output (response) parameters */
    struct {
        /** The overall status of the discovery process */
        a2b_HResult     status;

        /** Number of nodes discovered.  In the case of an
         *  error this indicates how far the process got
         *  before the error/fault/etc.
         */
        a2b_UInt32      numNodes;

		/*** Node Info for the last discovered node
		*/
		a2b_NodeInfo	oLastNodeInfo;
    } resp;

} a2b_NetDiscovery;

/** Payload data for a A2B_MSGREQ_PLUGIN_PERIPH_INIT within an a2bMsg request */
typedef struct a2b_PluginInit
{
    /** Input (request) parameters */
    struct {
        
        /** TDM settings specific to the node being initialized.
         *  This is defined as this to allow us to use a single
         *  malloc'ed struct in the stack and not bloating all
         *  messages.  Users of this data MUST use the data
         *  before the release or copy any required data so
         *  it can be used later.
         */
        a2b_TdmSettings     *tdmSettings;

        const void *pNodePeriDeviceConfig;

    } req;

    /** Output (response) parameters */
    struct {
        a2b_HResult         status;
    } resp;

} a2b_PluginInit;

/** Payload data for an A2B_MSGREQ_PLUGIN_PERIPH_DEINIT message within a
 * a2bMsg message.
 */
typedef struct a2b_PluginDeinit
{
    /** No input (request) parameters. **/

    /** Output (response) parameters */
    struct {
        a2b_HResult status;
    } resp;
} a2b_PluginDeinit;

/** Payload data for a A2B_MSGREQ_PLUGIN_VERSION within an a2bMsg request */
typedef struct a2b_PluginVerInfo
{
    /** Output (response) parameters */
    struct {
        /* Major, minor, and release version of plugin */
        a2b_UInt32      majorVer;
        a2b_UInt32      minorVer;
        a2b_UInt32      relVer;

        /** Pointer to plugin allocated/owned build description string */
        const a2b_Char* buildInfo;
    } resp;
} a2b_PluginVerInfo;

/** Payload data for an A2B_MSGREQ_NET_BERT_START message within a
 * a2bMsg message.
 */
typedef struct a2b_PluginBERTStart
{
	/** Input (request) parameters */
	    struct {
	        /** BERT Config Buffer Ptr*/
	        a2b_UInt32* pBertConfigBuf;

	        /** BERT Handler Ptr */
	        a2b_UInt32*   pBertHandle;

	    } req;
    /** Output (response) parameters */
} a2b_PluginBERTStart;

#ifdef A2B_FEATURE_COMM_CH
/** Payload data for an A2B_MSGREQ_COMMCH_SEND_MSG message within a
 * a2bMsg message.
 */
typedef struct a2b_CommChTxInfo
{
	/** Input (request) parameters */
	    struct {
	        /** Transmission Message Buffer Ptr*/
	    	a2b_CommChMsg* pTxMsg;

	        /** Destination Slave Node Address */
	    	a2b_Int16   nSlvNodeAddr;

	    } req;
    /** Output (response) parameters */
} a2b_CommChTxInfo;
#endif	/* A2B_FEATURE_COMM_CH */

/** Payload data for an A2B_MSGREQ_SEND_MBOX_DATA message within a
 * a2bMsg message.
 */
typedef struct a2b_MailboxTxInfo
{
	/** Input (request) parameters */
	    struct {
	        /** Data Buffer Ptr */
	    	a2b_UInt8* 	pwBuf;

	    	 /** Data Size */
	    	a2b_UInt8 	nDataSz;

	        /** Destination Slave Node Address */
	    	a2b_Int16   nSlvNodeAddr;

	    	/** Mailbox number */
	    	a2b_UInt8	nMbox;

	     } req;
    /** Output (response) parameters */
} a2b_MailboxTxInfo;


/** \} -- a2bstack_msgtypes_req */


/*----------------------------------------------------------------------------*/
/** 
 * \defgroup a2bstack_msgtypes_notify   Notification Messages
 *  
 * These typedefs for all system notification messages.
 *
 * \{ */
/*----------------------------------------------------------------------------*/

typedef struct a2b_PowerFault
{
    /** The status of the power fault diagnosis.
     * The actual diagnosis may fail even though a power fault
     * has been detected. The localization of the fault location is
     * unreliable in this case.
     */
    a2b_HResult     status;

    /** The reported interrupt type (per A2B_ENUM_INTTYPE_PWRERR_XXX
     * enumeration defined in a2b/regdefs.h).
     */
    a2b_UInt8       intrType;

    /** The A2B node address where it's believed the fault occurred */
    a2b_Int16       faultNode;
} a2b_PowerFault;


/**
 * This is the notification payload for both A2B_MSGNOTIFY_GPIO_INTERRUPT and
 * A2B_MSGNOTIFY_INTERRUPT notifications.
 */
typedef struct a2b_Interrupt
{
    /** The reported interrupt type (per A2B_ENUM_INTTYPE_PWRERR_XXX
     * enumeration defined in a2b/regdefs.h).
     */
    a2b_UInt8       intrType;

    /**
     * The A2B node address reporting the interrupt.
     */
    a2b_Int16       nodeAddr;
} a2b_Interrupt;


/**
 * This is the notification payload for the A2B_MSGNOTIFY_DISCOVERY_DONE
 * notification.
 */
/* Output (response) parameters */
typedef struct a2b_DiscoveryStatus{
    /** The overall status of the discovery process */
    a2b_HResult     status;

    /** Number of nodes discovered.  In the case of an
     *  error this indicates how far the process got
     *  before the error/fault/etc.
     */
    a2b_UInt32      numNodes;
} a2b_DiscoveryStatus;

/**
 * This is the notification payload for A2B_MSGNOTIFY_MAILBOX_EVENT  notification.
 */
/* Output (response) parameters */
typedef struct a2b_MailboxEventInfo{

	/** Mailbox  Event Type */
	A2B_MAILBOX_EVENT_TYPE     eEvent;

    /** Slave Node Id
     * In case of reception event , slave node from which data received
     * In case of transmission event , slave node to which data transmitted
     */
    a2b_UInt16      nNodeId;

    /** Pointer to the Read Buffer in case of message reception event
     * */
    a2b_UInt8* prBuf;

    /** Data Size*/
    a2b_UInt8 nDataSz;

    /** The handle of Communication channel instance that should process this event */
    a2b_Handle commChHnd;
} a2b_MailboxEventInfo;

#ifdef A2B_FEATURE_COMM_CH
/**
 * This is the notification payload for A2B_MSGNOTIFY_COMMCH_EVENT  notification.
 */
/* Output (response) parameters */
typedef struct a2b_CommChEventInfo{

	/** Communication Channel Event Type */
	A2B_COMMCH_EVENT     eEvent;

    /** Slave Node Id
     * In case of reception event , slave node from which message received
     * In case of transmission event , slave node to which message transmitted
     */
    a2b_UInt16      nNodeId;

    /** Pointer to the Read Buffer in case of message reception event
     * */
    a2b_CommChMsg *pRxMsg;

} a2b_CommChEventInfo;
#endif	/* A2B_FEATURE_COMM_CH */

/** \} -- a2bstack_msgtypes_notify */


/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_msgtypes */

#endif /* A2B_MSGTYPES_H_ */
