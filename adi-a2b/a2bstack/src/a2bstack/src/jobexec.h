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
 * \file:   jobexec.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the job executor interface.
 *
 *=============================================================================
 */

/*============================================================================*/
/** 
 * \defgroup a2bstack_jobexec           Job Executor Module 
 *  
 * This defines the job executor interface. 
 *  
 * \{ */
/** 
 * \defgroup a2bstack_jobexec_priv      \<Private\>
 * \private 
 *
 * This defines job executor API's that are private to the stack. 
 *  
 * \{ */
/*============================================================================*/

#ifndef A2B_JOBEXEC_H_
#define A2B_JOBEXEC_H_

/*======================= I N C L U D E S =========================*/
#include "a2b/macros.h"
#include "a2b/ctypes.h"
#include "queue.h"

/*======================= D E F I N E S ===========================*/


/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_Job;
struct a2b_JobExecutor;
struct a2b_JobQueue;
struct a2b_StackContext;

/**
 * Holds a sequence of jobs to be executed.
 */
typedef struct a2b_JobQueue
{
    SLIST_ENTRY(a2b_JobQueue)               link;

    /** This allows the jobQueue to exist on a second linked list */
    SLIST_ENTRY(a2b_JobQueue)               link2;

    SIMPLEQ_HEAD(a2b_JobHead, a2b_Job)      qHead;
    a2b_UInt16                              priorityMask;
    a2b_Int32                               action;
    struct a2b_JobExecutor*                 executor;
    a2b_UInt32                              refCnt;
} a2b_JobQueue;

/**
 * Manages a list of job queues and executes jobs from the queues.
 */
typedef struct a2b_JobExecutor
{
    SLIST_HEAD(a2b_JobQHead, a2b_JobQueue)  listHead;
    a2b_UInt16                              schedMask;
    struct a2b_StackContext*                ctx;
} a2b_JobExecutor;


/*======================= P U B L I C  P R O T O T Y P E S ========*/

/**
 * Define the API for the Job Executor services
 */


A2B_EXPORT A2B_DSO_LOCAL struct a2b_JobExecutor* a2b_jobExecAlloc(
                                        struct a2b_StackContext* ctx);

A2B_EXPORT A2B_DSO_LOCAL void a2b_jobExecFree(
                                        struct a2b_JobExecutor* exec);

A2B_EXPORT A2B_DSO_LOCAL struct a2b_JobQueue* a2b_jobExecAllocQueue(
                                        struct a2b_JobExecutor* exec,
                                        a2b_JobPriority priority);

A2B_EXPORT A2B_DSO_LOCAL void a2b_jobExecFlushQueue(a2b_JobQueue* q);
A2B_EXPORT A2B_DSO_LOCAL void a2b_jobExecRefQueue(a2b_JobQueue* q);
A2B_EXPORT A2B_DSO_LOCAL a2b_UInt32 a2b_jobExecUnrefQueue(
                                        struct a2b_JobQueue* q);

A2B_EXPORT A2B_DSO_LOCAL a2b_Bool a2b_jobExecSubmit(
                                        struct a2b_JobQueue* q,
                                        struct a2b_Job* job);

A2B_EXPORT A2B_DSO_LOCAL void a2b_jobExecSchedule(
                                        struct a2b_JobExecutor* exec);

A2B_EXPORT A2B_DSO_LOCAL void a2b_jobExecUpdate(a2b_JobQueue* q,
                                        a2b_Int32 action);

A2B_END_DECLS

/*======================= D A T A =================================*/

/** \} -- a2bstack_jobexec_priv */

/** \} -- a2bstack_jobexec */

#endif /* A2B_JOBEXEC_H_ */
