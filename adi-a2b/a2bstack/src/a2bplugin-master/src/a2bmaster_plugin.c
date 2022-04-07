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
 * \file:   plugin.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  The implementation of an A2B stack master plugin.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/pluginapi.h"
#include "a2b/error.h"
#include "a2b/conf.h"
#include "a2b/defs.h"
#include "a2b/util.h"
#include "a2b/msg.h"
#include "a2b/trace.h"
#include "a2b/interrupt.h"
#include "a2b/i2c.h"
#include "a2b/timer.h"
#include "a2b/msgrtr.h"
#include "a2bplugin-master/plugin.h"
#include "discovery.h"
#include "pwrdiag.h"
#include "periphcfg.h"
#include "verinfo.h"
#include "a2b/stackctxmailbox.h"

/*======================= D E F I N E S ===========================*/

/*======================= L O C A L  P R O T O T Y P E S  =========*/
static a2b_HResult a2b_applyBdd(
    a2b_Plugin*         plugin,
    struct a2b_Msg*     msg);
#ifdef A2B_FEATURE_COMM_CH
static a2b_HResult
a2b_mailBoxi2cWrite(a2b_Plugin*  plugin,a2b_UInt8* pWriteBuf,
		a2b_UInt8 nDataSize, a2b_Int16 nSlvNodeAddr, a2b_UInt8 nMBoxNo);
static a2b_HResult
a2b_mailBoxi2cRead(a2b_Plugin*  plugin,a2b_UInt8* pReadBuf,
		a2b_UInt8 nDataSize, a2b_Int16 nSlvNodeAddr,  a2b_UInt8 nMBoxNo);
static a2b_Bool
a2b_mailboxStartTimer
    (
    a2b_Plugin*     plugin,
	a2b_UInt16 	nCommChInstNo
    );
static void
a2b_onMboxTxResponseTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    );
static void a2b_CommChCallBk(void* pCbParam, a2b_CommChMsg *pRxMsg, A2B_COMMCH_EVENT eEventType, a2b_Int8 nNodeAddr);
static void a2b_SendMboxEventNotification(a2b_Plugin* plugin, A2B_MAILBOX_EVENT_TYPE eMboxEvent, a2b_Int16 NodeAddr,  a2b_UInt8* prBuf,   a2b_UInt8 nDataSz);
static void a2b_SendCommChEventNotification(a2b_Plugin* plugin, A2B_COMMCH_EVENT eCommChEvent, a2b_Int16 NodeAddr,  a2b_CommChMsg * pMsg);
static void a2b_pluginCommChInit(struct a2b_StackContext* ctx, a2b_Plugin* plugin);
static void a2b_CommChAssignInstToSlvNodes( a2b_Plugin* plugin);
static void a2b_pluginCommChDeInit(a2b_Plugin* plugin);
#endif /* A2B_FEATURE_COMM_CH */
static a2b_Plugin* a2b_pluginFind(a2b_Handle  hnd);
static a2b_Handle a2b_pluginOpen(struct a2b_StackContext* ctx,
    const a2b_NodeSignature*    nodeSig);
static a2b_HResult a2b_pluginClose(a2b_Handle  hnd);
static a2b_Int32 a2b_pluginExecute(struct a2b_Msg*  msg,
    a2b_Handle pluginHnd,
    struct a2b_StackContext* ctx);
static void a2b_pluginInterrupt(struct a2b_StackContext* ctx,
    a2b_Handle                  hnd,
    a2b_UInt8                   intrSrc,
    a2b_UInt8                   intrType);


/*======================= D A T A  ================================*/

static a2b_Plugin gsPlugins[A2B_CONF_MAX_NUM_MASTER_NODES];

/*======================= C O D E =================================*/




/*!****************************************************************************
*
*  \b              a2b_applyBdd
*
*  Apply the BDD based on a loaded INIT payload.
*
*  \param          [in]    plugin   The master plugin.
*  \param          [in]    msg      The network discovery message.
*
*  \pre            None
*
*  \post           None
*
*  \return         A status code that can be checked with the A2B_SUCCEEDED()
*                  or A2B_FAILED() for success or failure of the function.
*
******************************************************************************/
static a2b_HResult
a2b_applyBdd
    (
    a2b_Plugin*         plugin,
    struct a2b_Msg*     msg
    )
{
    a2b_HResult                 status = A2B_RESULT_SUCCESS;
    a2b_NetDiscovery*           discPayload;

    discPayload = (a2b_NetDiscovery*)a2b_msgGetPayload(msg);

    if (( A2B_NULL == discPayload ) ||
        ( A2B_NULL == discPayload->req.bdd ))
    {
        return A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                A2B_EC_INVALID_PARAMETER);
    }

    /* Keep a reference to the peripheral package */
    plugin->periphCfg.pkgCfg = discPayload->req.periphPkg;
    plugin->periphCfg.pkgLen = discPayload->req.pkgLen;

    plugin->bddLoaded = A2B_TRUE;
    /* NOTE: We DO NOT make a copy of the data.  It is expected
     *       that the application will not free the BDD--MUST be
     *       available for the life of the stack.
     */
    plugin->bdd = discPayload->req.bdd;



#ifdef A2B_FEATURE_TRACE
    /* Only for debug */
    if ( plugin->overrides[0] & A2B_MPLUGIN_IGN_EEPROM )
    {
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "a2b_applyBdd: Override for Ignore EEPROM"));
    }
    else if ( plugin->overrides[0] & A2B_MPLUGIN_EEPROM_VER_ONLY )
    {
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "a2b_applyBdd: Override for EEPROM Version Only"));
    }
    else
    {
        /* Completing the control statement */
    }
    if ( plugin->overrides[0] & A2B_MPLUGIN_IGN_PERIPH_ERR )
    {
        A2B_TRACE0((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                     "a2b_applyBdd: Override for Ignore Peripheral Error"));
    }
#endif

    return status;

} /* a2b_applyBdd */

#ifdef A2B_FEATURE_COMM_CH
/*!****************************************************************************
*
*  \b               a2b_mailBoxi2cWrite
*
*  The function for sending mailbox data to remote slave node.
*
*  \param           [in]    plugin       Handle to plugin structure
*
*  \param           [in]    pWriteBuf    Pointer to the Data Buffer to be sent out
*
*  \param           [in]    nDataSize    Number of data bytes
*
*  \param           [in]    nSlvNodeAddr Slave Node Address
*
*  \param           [in]    nMBoxNo 	 Mailbox Number (0/1)
*
*  \pre             None
*
*  \post            None
*
*  \return           Success or Error
*
******************************************************************************/
static a2b_HResult
a2b_mailBoxi2cWrite(a2b_Plugin*  plugin,a2b_UInt8* pWriteBuf,
		a2b_UInt8 nDataSize, a2b_Int16 nSlvNodeAddr, a2b_UInt8 nMBoxNo)
{
	a2b_UInt8 wBuf[5];
	a2b_UInt32 nIndex;
	a2b_HResult status = 0u;

	if(nMBoxNo == 0u)
	{
		wBuf[0] = A2B_REG_MBOX0B0;
	}
	else /*if(nMBoxNo == 1) */
	{
		wBuf[0] = A2B_REG_MBOX0B1;
	}
	for(nIndex = 0u; nIndex < nDataSize; nIndex++)
	{
		wBuf[nIndex+1u] = pWriteBuf[nIndex];
	}
	status = a2b_i2cSlaveWrite(plugin->ctx, nSlvNodeAddr, ((a2b_UInt16)nDataSize+(a2b_UInt16)1u), &wBuf[0]);
	if(A2B_FAILED(status))
	{
		A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
					"Exit: %s execute(): Mailbox Write Failed",
					A2B_MPLUGIN_PLUGIN_NAME));
	}

	return (status);
}

/*!****************************************************************************
*
*  \b               a2b_mailBoxi2cRead
*
*  The function for reading mailbox data from remote slave node.
*
*
*  \param           [in]    plugin       Handle to plugin structure
*
*  \param           [in]    pReadBuf    Pointer to  Data Buffer where data read is to be stored
*
*  \param           [in]    nDataSize    Number of data bytes to be read
*
*  \param           [in]    nSlvNodeAddr Slave Node Address
*
*  \param           [in]    nMBoxNo 	 Mailbox Number (0/1)
*
*  \pre             None
*
*  \post            None
*
*  \return           Success or Error
*
******************************************************************************/
static a2b_HResult
a2b_mailBoxi2cRead(a2b_Plugin*  plugin,a2b_UInt8* pReadBuf,
		a2b_UInt8 nDataSize, a2b_Int16 nSlvNodeAddr,  a2b_UInt8 nMBoxNo)
{
	a2b_UInt8 wBuf;
	a2b_HResult status = 0u;

	if(nMBoxNo == 0u)
	{
		wBuf = A2B_REG_MBOX0B0;
	}
	else /*if(nMBoxNo == 1) */
	{
		wBuf = A2B_REG_MBOX1B0;
	}
	status = a2b_i2cSlaveWriteRead(plugin->ctx, nSlvNodeAddr, 1u, &wBuf, (a2b_UInt16)nDataSize, pReadBuf);
	if(A2B_FAILED(status))
	{
		A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
					"Exit: %s execute(): Mailbox Write Failed",
					A2B_MPLUGIN_PLUGIN_NAME));
	}

	return (status);

}

/*!****************************************************************************
*
*  \b               a2b_GetCommChInstIdForSlvNode
*
*  The function is used for getting the index of the communication channel instance
*  handling a given Slave Node.
*
*
*  \param           [in]    plugin       Handle to plugin structure
*
*  \param           [in]    nSlvNodeAddr Slave Node Address
*
*  \param           [out]   nIndexOut    Communication Channel Instance Value returned
*
*  \pre             None
*
*  \post            None
*
*  \return          A2B_TRUE on Success
*  					A2B_FALSE on Failure
*
******************************************************************************/
a2b_Bool
a2b_GetCommChInstIdForSlvNode(a2b_Plugin*  plugin, a2b_Int16 nSlvNodeAddr, a2b_UInt16* nIndexOut)
{
	a2b_Bool bRet = A2B_FALSE;
	a2b_UInt16 nIndex = 0u;

	/* Locate the comm ch instance id for this slave node */
	for(nIndex = 0u; nIndex < A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES; nIndex++)
	{
		if(plugin->commCh.commChSlvNodeIds[nIndex] == nSlvNodeAddr)
		{
			*nIndexOut = nIndex;
			bRet = A2B_TRUE;
			break;
		}
	}
	return (bRet);
}


/*!****************************************************************************
*
*  \b              a2b_mailboxStartTimer
*
*  Generate/Start the mailbox tx response timeout timer
*
*  \param          [in]    plugin			Master Plugin
*  \param          [in]    nCommChInstNo	Instance Id of Communication channel
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
static a2b_Bool
a2b_mailboxStartTimer
    (
    a2b_Plugin*     plugin,
	a2b_UInt16 	nCommChInstNo
    )
{
    /* Default is for the discovery timer */
    a2b_UInt32 delay = A2B_MAILBOX_TX_RESPONSE_TIMEOUT_IN_MS;
    a2b_TimerFunc timerFunc = &a2b_onMboxTxResponseTimeout;

    /* Stop the previously running timer */
    a2b_timerStop( plugin->commCh.mboxTimeoutTimer[nCommChInstNo] );

    /* Single shot timer */
    a2b_timerSet( plugin->commCh.mboxTimeoutTimer[nCommChInstNo], delay, 0u );
    a2b_timerSetHandler(plugin->commCh.mboxTimeoutTimer[nCommChInstNo], timerFunc);
    a2b_timerSetData(plugin->commCh.mboxTimeoutTimer[nCommChInstNo], plugin);
    a2b_timerStart( plugin->commCh.mboxTimeoutTimer[nCommChInstNo] );

    return A2B_TRUE;

} /* a2b_mailboxStartTimer */

/*!****************************************************************************
*
*  \b              a2b_onMboxTxResponseTimeout
*
*  Handle the timeout waiting for mailbox empty interrupt from slave node on a
*  transmission
*
*  \param          [in]    timer		Timer Handle
*  \param          [in]    userData		User Data
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void
a2b_onMboxTxResponseTimeout
    (
    struct a2b_Timer *timer,
    a2b_Handle userData
    )
{
	a2b_UInt16			nCommChInstNo = 0u, nIndex;
	a2b_Int16			NodeAddr;
	a2b_Plugin*			plugin = (a2b_Plugin*)userData;
	A2B_COMMCH_RET		nCommChRet;

	/* Find communication channel instance corresponding to  the timer */
	for (nIndex = 0u; nIndex < A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES; nIndex++)
	{
		if(timer == plugin->commCh.mboxTimeoutTimer[nIndex])
		{
			nCommChInstNo = nIndex;
			break;
		}
	}
	NodeAddr = plugin->commCh.commChSlvNodeIds[nCommChInstNo];
	a2b_SendMboxEventNotification(plugin, A2B_MBOX_TX_TIMEOUT,  NodeAddr, A2B_NULL, 0u );
	nCommChRet = adi_a2b_CommChMstrTick(plugin->commCh.commChHnd[nCommChInstNo]);
	if(nCommChRet == A2B_COMMCH_FAILED)
	{
		  A2B_TRACE1((plugin->ctx,
					(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
					"a2b_onMboxTxResponseTimeout: failed to tick Communication channel"
					"Node Address: 0x%lX", NodeAddr));
	}

}

/*!****************************************************************************
*
*  \b              a2b_CommChCallBk
*
*  Handle the callback from the Communication channel on a new event
*
*  \param          [in]    pCbParam	    	Master Call Back Param
*  \param          [in]    pMsg				Pointer to Message
*  \param          [in]    eEventType		Event Type
*  \param          [in]    nNodeAddr		Node Address
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_CommChCallBk(void* pCbParam, a2b_CommChMsg *pMsg, A2B_COMMCH_EVENT eEventType, a2b_Int8 nNodeAddr)
{

	struct a2b_Msg*     		pCommChnotifyMsg;
	a2b_CommChEventInfo*		pCommChEventInfo;
	a2b_Plugin* plugin = (a2b_Plugin*)pCbParam;
	a2b_HResult                 status;
	a2b_Bool					bRet;

	/* Handling internally if Message ID < 0xA  */
	if(pMsg->nMsgId >= A2B_COMMCH_MSG_CUSTOM_START)
	{
		/* New application event .Notify event to higher layer
		 * */
		a2b_SendCommChEventNotification(plugin, eEventType, nNodeAddr, pMsg);
	}
	else
	{
#ifdef A2B_FEATURE_COMM_CH
#ifdef ADI_BDD
		bRet = adi_a2b_MstrPluginCommChStatusCb(pCbParam, pMsg, eEventType, nNodeAddr);
		if(bRet == A2B_FALSE)
		{
			A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR), "Exit: %s: Communication channel Event Processing by Plugin failed", A2B_MPLUGIN_PLUGIN_NAME));
		}
#endif
#endif
	}

}
/*!****************************************************************************
*
*  \b              a2b_SendMboxEventNotification
*
*  Send a mailbox event notification of type A2B_MSGNOTIFY_MAILBOX_EVENT
*
*  \param          [in]    plugin	    Master Call Back Param
*  \param          [in]    eMboxEvent	Event Type
*  \param          [in]    NodeAddr		Node Address
*  \param          [in]    prBuf		Read Buffer in case of reception event
*  \param      	   [in]    nDataSz		Size of data in buffer in case of reception event
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_SendMboxEventNotification(a2b_Plugin* plugin, A2B_MAILBOX_EVENT_TYPE eMboxEvent, a2b_Int16 NodeAddr,  a2b_UInt8* prBuf,   a2b_UInt8 nDataSz)
{

	struct a2b_Msg*     		pMboxnotifyMsg;
	a2b_MailboxEventInfo* 		pMboxEventInfo;
	a2b_UInt16 					nCommChInstNo;
	a2b_HResult                 status;
	a2b_Bool					bRet;

	/* Alloc notification message */
	pMboxnotifyMsg 				= a2b_msgAlloc(plugin->ctx, A2B_MSG_NOTIFY, A2B_MSGNOTIFY_MAILBOX_EVENT);
	pMboxEventInfo              = (a2b_MailboxEventInfo*)a2b_msgGetPayload(pMboxnotifyMsg);
	pMboxEventInfo->eEvent      = eMboxEvent;
	pMboxEventInfo->nNodeId     = (a2b_UInt16) NodeAddr;
	if(eMboxEvent == A2B_MBOX_RX_DATA )
	{
		pMboxEventInfo->prBuf   = prBuf;
		pMboxEventInfo->nDataSz = nDataSz;
	}
	bRet = a2b_GetCommChInstIdForSlvNode(plugin, NodeAddr, &nCommChInstNo);
	if(bRet == A2B_TRUE)
	{
		pMboxEventInfo->commChHnd = plugin->commCh.commChHnd[nCommChInstNo] ;
		/* Make best effort delivery of notification */
		status = a2b_msgRtrNotify(pMboxnotifyMsg);
		if ( A2B_FAILED(status) )
		{
			 A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
								"Exit: %s:  Mailbox Event  Notification failed",
								A2B_MPLUGIN_PLUGIN_NAME));
		}

		(void)a2b_msgUnref(pMboxnotifyMsg);
	}
	else
	{
		 A2B_TRACE1((plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
										"Exit: %s:  Mailbox Event  Notification failed:Communication channel Instance for slave node address not available",
										A2B_MPLUGIN_PLUGIN_NAME));
	}
}


/*!****************************************************************************
*
*  \b              a2b_SendCommChEventNotification
*
*  Send a communication channel event notification of type A2B_COMMCH_EVENT
*
*  \param          [in]    plugin	    Master Call Back Param
*  \param          [in]    eCommChEvent	Event Type
*  \param          [in]    NodeAddr		Node Address
*  \param          [in]    pMsg			Communication Channel message
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_SendCommChEventNotification(a2b_Plugin* plugin, A2B_COMMCH_EVENT eCommChEvent, a2b_Int16 NodeAddr,  a2b_CommChMsg * pMsg)
{

	struct a2b_Msg*     		pCommChnotifyMsg;
	a2b_CommChEventInfo*		pCommChEventInfo;
	a2b_UInt16 					nCommChInstNo;
	a2b_HResult                 status;

	/* Alloc Communication channel event notification message */
	pCommChnotifyMsg 			=  a2b_msgAlloc(plugin->ctx, A2B_MSG_NOTIFY, A2B_MSGNOTIFY_COMMCH_EVENT);
	pCommChEventInfo            =  (a2b_CommChEventInfo*)a2b_msgGetPayload(pCommChnotifyMsg);
	pCommChEventInfo->eEvent    =  eCommChEvent;
	pCommChEventInfo->nNodeId   =  (a2b_UInt16) (a2b_UInt16) NodeAddr;
	pCommChEventInfo->pRxMsg 	=  pMsg;
	/* Make best effort delivery of notification */
	status = a2b_msgRtrNotify(pCommChnotifyMsg);
	if ( A2B_FAILED(status) )
	{
		 A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
							"Exit: %s: Communication channel Event Notify failed",
							A2B_MPLUGIN_PLUGIN_NAME));
	}

	(void)a2b_msgUnref(pCommChnotifyMsg);
}

/*!****************************************************************************
*
*  \b              a2b_pluginCommChInit
*
*  Initialize the communication channel for the plugin
*
*  \param          [in]    ctx	    stack context
*  \param          [in]    plugin	plugin
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_pluginCommChInit(struct a2b_StackContext* ctx, a2b_Plugin* plugin)
{
    a2b_UInt16 nIndex;
    a2b_CommChMstrConfig oCommChConfig;

    /* Create the mailbox for the master plugin - communication channel
     * messaging.  This allows Node authentication using communication
     * channel during discovery. Priority is highest as authentication
     * should delay discovery as less as possible
     */
    plugin->commCh.commChMboxHnd = a2b_stackCtxMailboxAlloc(
                                                        plugin->ctx,
                                                        A2B_JOB_PRIO0 );
    if ( A2B_NULL == plugin->commCh.commChMboxHnd )
    {
        A2B_TRACE1( (plugin->ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                 	 "Exit: %s: a2b_pluginCommChInit: failed to create communication channel mailbox",
				 	 A2B_MPLUGIN_PLUGIN_NAME));
    }

    /* Assign memory from global memory reserved */
	oCommChConfig.pMem = &plugin->commCh.gCommChMem[0];
	/* Callback parameter is master plugin instance */
	oCommChConfig.pCbParam 	= plugin;
	oCommChConfig.pfStatCb 	= &a2b_CommChCallBk;
	oCommChConfig.ctx		= ctx;
	oCommChConfig.mboxHnd   = plugin->commCh.commChMboxHnd;

	/* Open the Communication channel */
	adi_a2b_CommChMstrOpen(ctx);

	/* Create the instances. Instance to Node Id mapping is done
	 * according to bdd configuration supplied
	 * during start of discovery            	 *
	 * */
	for(nIndex = 0u ; nIndex < A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES; nIndex ++)
	{
		plugin->commCh.commChHnd[nIndex] 			= adi_a2b_CommChMstrCreate(&oCommChConfig);
		/* Set to invalid value initially */
		plugin->commCh.commChSlvNodeIds[nIndex] 	=  A2B_NODEADDR_MASTER;
		plugin->commCh.mboxTimeoutTimer[nIndex] 	= a2b_timerAlloc(ctx, A2B_NULL, plugin);
		oCommChConfig.pMem 				    = ((a2b_UInt8*)oCommChConfig.pMem) + sizeof(a2b_CommChMstrInfo);
	}
}

/*!****************************************************************************
*
*  \b              a2b_pluginCommChDeInit
*
*  DeInitialize the communication channel for the plugin
*
*  \param          [in]    plugin	plugin
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_pluginCommChDeInit(a2b_Plugin* plugin)
{
	a2b_UInt16 nIndex;

	/* Free Comm Job
	*/
	(void)a2b_stackCtxMailboxFree(plugin->ctx, plugin->commCh.commChMboxHnd);


	/* Close the Communication channel */
	adi_a2b_CommChMstrClose(plugin->ctx);

	/* Destroy timer instances
	 */
	for (nIndex = 0u; nIndex < A2B_CONF_COMMCH_MAX_NO_OF_SMART_SLAVES; nIndex++)
	{

		/* Unref timer*/
		(void)a2b_timerUnref(plugin->commCh.mboxTimeoutTimer[nIndex]);

	}
}

/*!****************************************************************************
*
*  \b              a2b_CommChAssignInstToSlvNodes
*
*  Assigns the various communication channel instances to slave nodes based on
*  their addresses. The valid slave nodes and their addresses are determined
*  from network BDD information.
*
*  \param          [in]    plugin	plugin
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
static void a2b_CommChAssignInstToSlvNodes( a2b_Plugin* plugin)
{
	a2b_UInt16					nIdIndex;
	a2b_UInt16					nbddIndex;
	nIdIndex = 0u;

	/* Map the communication channel instances to the
	 * Nodes which have authorization via communication
	 * channel enabled
	 *  */
	for(nbddIndex = 0u ; nbddIndex < plugin->bdd->nodes_count; nbddIndex ++)
	{
		if(((a2b_UInt8)plugin->bdd->nodes[nbddIndex].mbox.mbox0ctl && A2B_BITM_MBOX0CTL_MB0EN) ||
		   ((a2b_UInt8)plugin->bdd->nodes[nbddIndex].mbox.mbox1ctl && A2B_BITM_MBOX1CTL_MB1EN)	)
		{
			/* Communication channel Authorization is only for slave nodes */
			if( nbddIndex != 0u)
			{
				/* Assign the id , bdd includes the master node so reduce by 1 */
				plugin->commCh.commChSlvNodeIds[nIdIndex] = (a2b_Int16)nbddIndex-1;
				nIdIndex++;
			}
		}
	}
}
#endif /* A2B_FEATURE_COMM_CH */

/*!****************************************************************************
*
*  \b              a2b_pluginFind
*
*  Lookup the plugin based on a handle
*
*  \param          [in]    hnd
*
*  \pre            None
*
*  \post           None
*
*  \return         [add here]
*
******************************************************************************/
static a2b_Plugin*
a2b_pluginFind
    (
    a2b_Handle  hnd
    )
{
    a2b_UInt32 idx;
    a2b_Plugin* plugin = A2B_NULL;
    for ( idx = 0u; idx < (a2b_UInt32)A2B_ARRAY_SIZE(gsPlugins); ++idx )
    {
        if ( hnd == (a2b_Handle)&gsPlugins[idx] )
        {
            if ( gsPlugins[idx].inUse )
            {
                plugin = &gsPlugins[idx];
            }
            break;
        }
    }

    return plugin;

} /* a2b_pluginFind */


/*!****************************************************************************
*
*  \b              a2b_pluginOpen
*
*  Called to see if the plugin handles a specific node.
*
*  During discovery we scan through the plugins list trying to open each
*  one giving the nodeInfo and nodeAddr of the discovered node. If the
*  plugin can manage this node then a valid (instantiated) handle is
*  returned, else it returns null and the next plugin is tried. Once a
*  non-null handle is returned it is assumed this is the managing plugin.
*  If no plugin handles a node then it's assumed to be a very dumb node.
*
*  \param          [in]    ctx         A2B stack context
*  \param          [in]    nodeSig     node signature (version/product/etc)
*
*  \pre            None
*
*  \post           The returned handle will be returned on the close() and
*                  is available on all messages to the plugin, use
*                  msg.a2b_msgGetPluginHdl() to get the handle.
*
*  \return         NULL=error, plugin does NOT handle the nodeInfo/nodeAddr
*                  NON-NULL=nodeInfo/nodeAddr handled by this plugin
*
******************************************************************************/
static a2b_Handle
a2b_pluginOpen
    (
    struct a2b_StackContext*    ctx,
    const a2b_NodeSignature*    nodeSig
    )
{
    a2b_Plugin* plugin = A2B_NULL;
    struct a2b_Timer* timer = A2B_NULL;
    a2b_UInt32  idx;
    a2b_Int16 nodeAddr = A2B_NODEADDR_NOTUSED;
    static a2b_Bool  bPluginInit = A2B_FALSE;

    if ( A2B_NULL != nodeSig )
    {
        nodeAddr = nodeSig->nodeAddr;
    }
    else
    {
    	return A2B_NULL;
    }

    if ( nodeAddr != A2B_NODEADDR_MASTER )
    {
        return A2B_NULL;
    }

    if ( !bPluginInit )
    {
        bPluginInit = A2B_TRUE;
        for ( idx = 0u; idx < (a2b_UInt32)A2B_ARRAY_SIZE(gsPlugins); ++idx )
        {
            (void)a2b_memset(&gsPlugins[idx], 0, sizeof(gsPlugins[idx]));
            gsPlugins[idx].ctx         = A2B_NULL;
            gsPlugins[idx].inUse       = A2B_FALSE;
            gsPlugins[idx].bddLoaded   = A2B_FALSE;
            gsPlugins[idx].timer       = A2B_NULL;

            a2b_pwrDiagInit(&gsPlugins[idx]);
        }
    }

    if ( (A2B_NULL != ctx) /*&& (A2B_NULL != nodeSig)*/ )
    {
        /* Look for an available master plugin control block */
        for ( idx = 0u; idx < (a2b_UInt32)A2B_ARRAY_SIZE(gsPlugins); ++idx )
        {
            if ( !gsPlugins[idx].inUse )
            {
                timer = a2b_timerAlloc(ctx, A2B_NULL, &gsPlugins[idx]);
                if ( A2B_NULL == timer )
                {
                    /* Bail out since we couldn't allocate a timer */
                    A2B_TRACE0((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                         "a2b_pluginOpen: failed to allocate a timer"));
                }
                else
                {
                    plugin = &gsPlugins[idx];
                    (void)a2b_memset(plugin, 0, sizeof(*plugin));
                    plugin->timer      		 = timer;
					plugin->ctx        		 = ctx;
                    plugin->nodeSig     	 = *nodeSig;
                    plugin->inUse       	 = A2B_TRUE;
                    plugin->discovery.inDiscovery = A2B_FALSE;
                    a2b_pwrDiagInit(plugin);

#ifdef A2B_FEATURE_COMM_CH
                    a2b_pluginCommChInit(ctx, plugin);
#endif /* A2B_FEATURE_COMM_CH */
                }
                break;
            }
        }
    }
    return plugin;

} /* a2b_pluginOpen */

/*!****************************************************************************
*
*  \b              a2b_pluginClose
*
*  Called to close the plugin
*
*  \param          [in]    hnd
*
*  \pre            None
*
*  \post           None
*
*  \return         Success or Error
*
******************************************************************************/
static a2b_HResult
a2b_pluginClose
    (
    a2b_Handle  hnd
    )
{
    a2b_Plugin* plugin;
    a2b_HResult status = A2B_MAKE_HRESULT(A2B_SEV_FAILURE, A2B_FAC_PLUGIN,
                                          A2B_PLUGIN_EC_BAD_ARG);

    plugin = a2b_pluginFind(hnd);
    if ( A2B_NULL != plugin )
    {
#ifdef A2B_FEATURE_COMM_CH
		a2b_pluginCommChDeInit(plugin);
#endif /* A2B_FEATURE_COMM_CH */

        (void)a2b_timerUnref(plugin->timer);
        plugin->ctx         = A2B_NULL;
        plugin->inUse       = A2B_FALSE;
        plugin->bddLoaded   = A2B_FALSE;
        (void)a2b_memset(&plugin->discovery, 0, sizeof(a2b_PluginDiscovery));

        status = A2B_RESULT_SUCCESS;
    }

    return status;

} /* a2b_pluginClose */


/*!****************************************************************************
*
*  \b              a2b_pluginExecute
*
*  Called when a job needs executing by this plugin.
*
*  \param          [in]    msg          A2B message
*  \param          [in]    pluginHnd    plugin handler
*  \param          [in]    ctx          A2B stack context
*
*  \pre            None
*
*  \post           None
*
*  \return         A2B_EXEC_COMPLETE == Execution is now complete
*                  A2B_EXEC_SCHEDULE == Execution is unfinished - schedule again
*                  A2B_EXEC_SUSPEND  == Execution is unfinished - suspend
*                                       scheduling until a later event
*
******************************************************************************/
static a2b_Int32
a2b_pluginExecute
    (
    struct a2b_Msg*             msg,
    a2b_Handle                  pluginHnd,
    struct a2b_StackContext*    ctx
    )
{
    a2b_UInt8                   wBuf[4];
    a2b_HResult                 status;
    a2b_Int32                   ret    = A2B_EXEC_COMPLETE;
    a2b_Plugin*                 plugin = (a2b_Plugin*)pluginHnd;
    a2b_UInt32                  cmd    = a2b_msgGetCmd(msg);
    a2b_UInt32*                 payload;
    a2b_NetDiscovery*           netDisc;
#ifdef A2B_RUN_BIT_ERROR_TEST
    a2b_PluginBERTStart*        pBertMsg;
#endif
#ifdef A2B_FEATURE_COMM_CH
    a2b_MailboxTxInfo*    		pMboxTxInfo;
    a2b_UInt16 					nCommChInstNo;
    a2b_CommChTxInfo*			pCommChTxInfo;
    A2B_COMMCH_RET				nCommChRet;
    a2b_Bool 					bRet;
 #endif

    if ( (plugin == A2B_NULL) || (A2B_NULL == msg) )
    {
        A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                    "Exit: %s execute(): Internal error",
                    A2B_MPLUGIN_PLUGIN_NAME));
        return A2B_EXEC_COMPLETE;
    }

    A2B_TRACE2((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                "Enter: %s execute(%ld):", A2B_MPLUGIN_PLUGIN_NAME, &cmd));

    switch ( cmd )
    {
#ifdef A2B_FEATURE_EEPROM_PROCESSING
        case A2B_MPLUGIN_START_PERIPH_CFG:
            ret = a2b_periphCfgStartProcessing( msg );
            break;

        case A2B_MPLUGIN_CONT_PERIPH_CFG:
            ret = a2b_periphCfgProcessing( plugin, (a2b_Int16)a2b_msgGetTid(msg)); /* TODO - chk */
            break;
#endif /* A2B_FEATURE_EEPROM_PROCESSING */

        case A2B_MSGREQ_NET_RESET:

            wBuf[0] = A2B_REG_CONTROL;
            wBuf[1] = A2B_ENUM_CONTROL_RESET_PE;
            (void)a2b_i2cMasterWrite( ctx, 2u, &wBuf);

            /* According to the "AD2410 Transceiver Programming
             * Ref Rev 0.3 Aug 2014", pg 2.3 Section "Master Node
             * Bring Up" we need a >25ms timeout after this reset.
             */
            /* USE: A2B_SW_RESET_DELAY */

            break;

        case A2B_MPLUGIN_DSCVRY_OVERRIDES:
            payload = (a2b_UInt32*)a2b_msgGetPayload( msg );
            /* Copy the override setting for later use */
            plugin->overrides[0u] = payload[0u];
            plugin->overrides[1u] = payload[1u];
            break;

        case A2B_MSGREQ_NET_DISCOVERY:
            status  = a2b_applyBdd( plugin, msg );
            netDisc = (a2b_NetDiscovery*)a2b_msgGetPayload(msg);

            if ( A2B_SUCCEEDED(status) )
            {
#ifdef A2B_FEATURE_COMM_CH
            	a2b_CommChAssignInstToSlvNodes(plugin);
#endif
                /* Start discovery */
                ret = a2b_dscvryStart( plugin, netDisc->req.deinitFirst );
            }
            else
            {

                netDisc->resp.status = status;
                netDisc->resp.numNodes = 0u;
            }
            break;

        case A2B_MSGREQ_NET_DISCOVERY_DIAGMODE:
            /* TODO maybe? */
            break;

        case A2B_MSGREQ_PLUGIN_VERSION:
            a2b_mstr_getVerInfo((struct a2b_PluginVerInfo*)a2b_msgGetPayload(msg));
            break;
#ifdef A2B_RUN_BIT_ERROR_TEST
        case A2B_MSGREQ_NET_BERT_START:
        	pBertMsg = (a2b_PluginBERTStart*)a2b_msgGetPayload(msg);
        	plugin->pBertHandler = (ADI_A2B_BERT_HANDLER*)pBertMsg->req.pBertHandle;
        	adi_a2b_BertIntiation(pBertMsg->req.pBertConfigBuf , plugin);
        	break;
        case A2B_MSGREQ_NET_BERT_UPDATE:
        	adi_a2b_BertUpdate(plugin);
        	break;
        case A2B_MSGREQ_NET_BERT_STOP:
        	adi_a2b_BertStop(plugin);
        	break;
#endif
#ifdef DISABLE_PWRDIAG
        case A2B_MSGREQ_NET_DISBALE_LINEDIAG:
        	payload = (a2b_UInt32*)a2b_msgGetPayload(msg);
        	plugin->bDisablePwrDiag = (a2b_Bool)payload[0u];
        	break;
#endif

#ifdef A2B_FEATURE_COMM_CH
        /* Handle message transmission  request using Communication channel */
        case A2B_MSGREQ_COMMCH_SEND_MSG:
        	/* Get Message Payload information*/
        	pCommChTxInfo = (a2b_CommChTxInfo*)a2b_msgGetPayload(msg);
        	bRet = a2b_GetCommChInstIdForSlvNode(plugin, pCommChTxInfo->req.nSlvNodeAddr, &nCommChInstNo);
        	if(bRet == A2B_TRUE)
        	{
				nCommChRet = adi_a2b_CommChMstrTxMsg(plugin->commCh.commChHnd[nCommChInstNo], pCommChTxInfo->req.pTxMsg, (a2b_Int8)pCommChTxInfo->req.nSlvNodeAddr);
				if(nCommChRet == A2B_COMMCH_FAILED)
				{
					/* Transmission request over Communication channel failed
					 * Notify error event to higher layer
					 * */
					a2b_SendCommChEventNotification(plugin, A2B_COMMCH_EVENT_FAILURE, pCommChTxInfo->req.nSlvNodeAddr,pCommChTxInfo->req.pTxMsg);
					A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
							"Exit: %s execute(): Communication Channel Send Message failed",
							A2B_MPLUGIN_PLUGIN_NAME));
				}
        	}
        	else
        	{
        		/*No communication channel instance mapped for the slave node
        		 * Notify error event to higher layer
        		 * */
        		a2b_SendCommChEventNotification(plugin, A2B_COMMCH_EVENT_FAILURE, pCommChTxInfo->req.nSlvNodeAddr,pCommChTxInfo->req.pTxMsg);
				A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
							"Exit: %s execute(): Communication Channel : Instance for slave node address not available",
							A2B_MPLUGIN_PLUGIN_NAME));
        	}
        	/* job is complete . let other jobs run */
        	ret = A2B_EXEC_COMPLETE;
        	break;

        /* Handle data transmission request over mailbox to slave node */
        case A2B_MSGREQ_SEND_MBOX_DATA:

        	/* Get Mailbox Payload information and write to remote slave node over I2C */
        	pMboxTxInfo = (a2b_MailboxTxInfo*)a2b_msgGetPayload(msg);
        	status = a2b_mailBoxi2cWrite(plugin, pMboxTxInfo->req.pwBuf, pMboxTxInfo->req.nDataSz,pMboxTxInfo->req.nSlvNodeAddr, pMboxTxInfo->req.nMbox);

        	/* If Mailbox write itself failed notify Communication channel instance of the failure  */
        	if(A2B_FAILED(status))
        	{
        		a2b_SendMboxEventNotification(plugin, A2B_MBOX_TX_IO_ERROR,  pMboxTxInfo->req.nSlvNodeAddr, A2B_NULL, 0u );
        	}
        	else
        	{
        		bRet = a2b_GetCommChInstIdForSlvNode(plugin, pMboxTxInfo->req.nSlvNodeAddr, &nCommChInstNo);
        		if(bRet == A2B_TRUE)
        		{
					/* Start the timer to check for timeout on acknowledgment from slave */
					bRet = a2b_mailboxStartTimer(plugin, nCommChInstNo);
        		}
        		else
        		{
        			A2B_TRACE1((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
							"Exit: %s execute(): Communication Channel : Instance for slave node address not available",
							A2B_MPLUGIN_PLUGIN_NAME));
        		}

        	}
        	/* job is complete . let other jobs run */
        	ret = A2B_EXEC_COMPLETE;
        	break;
#endif
        default:
            A2B_TRACE2((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_WARN),
                        "%s execute(%ld): Unhandled command",
                        A2B_MPLUGIN_PLUGIN_NAME, &cmd));
            break;
    }

    A2B_TRACE3((ctx, (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_TRACE1),
                "Exit: %s execute(%ld): 0x%lX", A2B_MPLUGIN_PLUGIN_NAME,
                &cmd, &ret));

    return ret;

} /* a2b_pluginExecute */


/*!****************************************************************************
*
*  \b              a2b_pluginInterrupt
*
*  Called to process an interrupt for the plugin.
*
*  \param          [in]    ctx       A2B stack context
*  \param          [in]    hnd       Plugin handler
*  \param          [in]    intrSrc   interrupt source
*  \param          [in]    intrType  interrupt type (A2B_ENUM_INTTYPE_*)
*
*  \pre            Not called from the JobExecutor
*
*  \post           None
*
*  \return         None
*
*******************************************************************************/
static void
a2b_pluginInterrupt
    (
    struct a2b_StackContext*    ctx,
    a2b_Handle                  hnd,
    a2b_UInt8                   intrSrc,
    a2b_UInt8                   intrType
    )
{
    a2b_HResult result;
    a2b_Plugin* plugin = a2b_pluginFind(hnd);
#ifdef A2B_FEATURE_COMM_CH
    a2b_UInt16 					nCommChInstNo;
    a2b_UInt8 					readBuf[ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE], nTempVar;
	a2b_Int16                   nNodeAddr;
	A2B_COMMCH_RET				nCommChRet;
	a2b_Bool					bRet;
	nTempVar 	= intrSrc & (a2b_UInt8)A2B_BITM_INTSRC_INODE;
	nNodeAddr 	= (intrSrc & A2B_BITM_INTSRC_MSTINT) ? A2B_NODEADDR_MASTER : (a2b_Int16)(nTempVar);
#endif
    A2B_UNUSED(ctx);

    if ( A2B_NULL != plugin )
    {
        switch ( intrType )
        {
            case A2B_ENUM_INTTYPE_DSCDONE:
                /* If we're diagnosing a power fault */
                if ( a2b_pwrDiagIsActive(plugin) )
                {
                    a2b_pwrDiagDiagnose(plugin, intrSrc, intrType);
                }
                else if ( plugin->discovery.inDiscovery )
                {
                    (void)a2b_dscvryNodeDiscovered(plugin);
                }
                else
                {
                	/* Completing the control statement */
                }
                break;

            case A2B_ENUM_INTTYPE_PWRERR_CS_GND:
            case A2B_ENUM_INTTYPE_PWRERR_CS_VBAT:
            case A2B_ENUM_INTTYPE_PWRERR_CS:
            case A2B_ENUM_INTTYPE_PWRERR_CDISC:
            case A2B_ENUM_INTTYPE_PWRERR_CREV:
            case A2B_ENUM_INTTYPE_PWRERR_FAULT:
            case A2B_ENUM_INTTYPE_PWRERR_NLS_GND:
            case A2B_ENUM_INTTYPE_PWRERR_NLS_VBAT:
            case A2B_ENUM_INTTYPE_STRTUP_ERR_RTF:
#ifdef DISABLE_PWRDIAG
            	if(!plugin->bDisablePwrDiag)
            	{
#endif
                /* If we're not diagnosing a power fault yet then ... */
                if ( !a2b_pwrDiagIsActive(plugin) )
                {
                    /* If the fault happened during node discovery */
                    if ( plugin->discovery.inDiscovery )
                    {
                        /* Stop any active timers. Discovery is
                         * effectively aborted at this point but we won't
                         * reply to the app until we know more about the fault.
                         */
                        a2b_timerStop(plugin->timer);
                        /* Make sure the message schedule suspends processing
                         * more messages until the diagnosis is done.
                         */
                        a2b_msgRtrExecUpdate(plugin->ctx, A2B_MSG_MAILBOX,
                                           A2B_EXEC_SUSPEND);
                    }

                    result = a2b_pwrDiagStart(plugin, intrSrc, intrType);
                    if ( A2B_FAILED(result) )
                    {
                        A2B_TRACE1((ctx,
                                (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
                                "a2b_pluginInterrupt: failed to start "
                                "power diags: 0x%lX", &result));
                        /* If we're still in discovery then end it */
                        if ( plugin->discovery.inDiscovery )
                        {
                            a2b_dscvryEnd(plugin, (a2b_UInt32)A2B_EC_DISCOVERY_PWR_FAULT);
                        }
                    }
                    else
                    {
                        /* We've started (and perhaps already finished)
                         * the power fault diagnostics.
                         */
                        A2B_TRACE2((ctx,
                                (A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_INFO),
                                "a2b_pluginInterrupt: starting power "
                                "fault diagnosis: "
                                "src=0x%bX type=%bd", &intrSrc, &intrType));
                    }
                }
                /* Else we're in the middle of power fault diagnosis */
                else
                {
                    /* Continue with the diagnosis */
                    a2b_pwrDiagDiagnose(plugin, intrSrc, intrType);
                }

#ifdef DISABLE_PWRDIAG
            	}
#endif
                break;
			case A2B_ENUM_INTTYPE_SRFERR:
			case A2B_ENUM_INTTYPE_IRPT_MSG_ERR:
			case A2B_ENUM_INTTYPE_BECOVF:
#ifdef DISABLE_PWRDIAG
            	if(!plugin->bDisablePwrDiag)
            	{
#endif
					if (!plugin->discovery.inDiscovery)
					{
						result = a2b_pwrDiagStart(plugin, intrSrc, intrType);
					}
#ifdef DISABLE_PWRDIAG
            	}
#endif
				break;
			case A2B_ENUM_INTTYPE_MSTR_RUNNING:
#ifdef DISABLE_PWRDIAG
            	if(!plugin->bDisablePwrDiag)
            	{
#endif
// 23Mar2021 (KCF) - This interrupt should be  ignored due to the way the application state machine operates
#if 0
					/* Master running interrupt happens before discovery starts. Hence power
					diagnostics to be done for cases only after discovery is done. */
					if((plugin->discovery.dscNumNodes!=0u) && (!plugin->discovery.inDiscovery) && (!plugin->pwrDiag.hasFault))
					{
						result = a2b_pwrDiagStart(plugin, intrSrc, intrType);
					}
#endif
#ifdef DISABLE_PWRDIAG
            	}
#endif
			break;
#ifdef A2B_FEATURE_COMM_CH
			case A2B_ENUM_INTTYPE_MBOX0_FULL:
			/* Not Handled as Default Tx mailbox is Mailbox No 1  */
			break;
			case A2B_ENUM_INTTYPE_MBOX0_EMPTY:

				bRet = a2b_GetCommChInstIdForSlvNode(plugin, nNodeAddr, &nCommChInstNo);
			    if(bRet == A2B_TRUE)
			    {
					/* Stop the previously running mailbox timeout timer */
					a2b_timerStop( plugin->commCh.mboxTimeoutTimer[nCommChInstNo] );
					a2b_SendMboxEventNotification(plugin, A2B_MBOX_TX_DONE,  nNodeAddr, A2B_NULL, 0u );
					nCommChRet = adi_a2b_CommChMstrTick(plugin->commCh.commChHnd[nCommChInstNo]);
					if(nCommChRet == A2B_COMMCH_FAILED)
					{
						  A2B_TRACE1((ctx,
									(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
									"a2b_pluginInterrupt:Mbox0 Empty: failed to tick Communication channel"
									"Node Address: 0x%lX", nNodeAddr));
					}
			    }
			    else
			    {
					  A2B_TRACE1((ctx,
								(A2B_TRC_DOM_PLUGIN | A2B_TRC_LVL_ERROR),
								"a2b_pluginInterrupt: Mbox0 Empty: Communication channel Instance for slave node address not available "
								"Node Address: 0x%lX", nNodeAddr));
			    }

			break;

			case A2B_ENUM_INTTYPE_MBOX1_FULL:
				result = a2b_mailBoxi2cRead( plugin, &readBuf[0], ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE, nNodeAddr, 1u );
				/* If Mailbox read itself failed notify Comm ch instance of the failure  */
				if(A2B_FAILED(result))
				{
					a2b_SendMboxEventNotification(plugin, A2B_MBOX_RX_IO_ERROR,  nNodeAddr, A2B_NULL, 0u );
				}
				/* Notify the new message reception event */
				else
				{
					a2b_SendMboxEventNotification(plugin, A2B_MBOX_RX_DATA,  nNodeAddr, &readBuf[0], ADI_A2B_COMMCH_ENG_MAX_MBOX_DATA_SIZE );
				}
			break;

			case A2B_ENUM_INTTYPE_MBOX1_EMPTY:
			/* Not Handled as Default Rx mailbox is Mailbox No 0  */
			break;
#endif
		    default:
                break;
        }
    }

} /* a2b_pluginInterrupt */


/*!****************************************************************************
*
*  \b              A2B_MASTER_PLUGIN_INIT
*
*  Called by PAL to init the plugin
*
*  \param          [in]    api
*
*  \pre            None
*
*  \post           None
*
*  \return         TRUE=success, FALSE=NULL api
*
******************************************************************************/
A2B_DSO_PUBLIC a2b_Bool
A2B_MASTER_PLUGIN_INIT
    (
    struct a2b_PluginApi*   api
    )
{
    a2b_Bool status = A2B_FALSE;

    if ( A2B_NULL != api )
    {
        api->open      = &a2b_pluginOpen;
        api->close     = &a2b_pluginClose;
        api->execute   = &a2b_pluginExecute;
        api->interrupt = &a2b_pluginInterrupt;

        (void)a2b_strncpy(api->name, A2B_MPLUGIN_PLUGIN_NAME,
                    (a2b_Size)A2B_ARRAY_SIZE(api->name) - 1u);
        api->name[A2B_ARRAY_SIZE(api->name)-1u] = '\0';

        status = A2B_TRUE;
    }

    return status;
} /* A2B_MASTER_PLUGIN_INIT */
