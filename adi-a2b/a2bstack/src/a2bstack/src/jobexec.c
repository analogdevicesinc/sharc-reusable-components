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
 * \file:   jobexec.c
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This implementation of the job executor.
 *
 *=============================================================================
 */

/*======================= I N C L U D E S =========================*/

#include "a2b/macros.h"
#include "a2b/conf.h"
#include "a2b/defs.h"
#include "a2b/trace.h"
#include "jobexec.h"
#include "job.h"
#include "stackctx.h"
#include "stack_priv.h"
#include "utilmacros.h"

/*======================= D E F I N E S ===========================*/

#define A2B_MAX_SCHED_MASK_VALUE    (0x1FFu)

/*======================= L O C A L  P R O T O T Y P E S  =========*/

/*======================= D A T A  ================================*/

/*======================= C O D E =================================*/

/*!**************************************************************************** 
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecAlloc
*
*  Allocates a Job Executor from the system. Returns A2B_NULL if resources
*  could not be allocated or an instance otherwise.
*
*  \param          [in]    ctx      A2B stack context
*
*  \pre            None
*
*  \post           None
*
*  \return         A pointer to a Job Executor or A2B_NULL if no resources are
 *                 available.
*
******************************************************************************/
A2B_DSO_LOCAL struct a2b_JobExecutor*
a2b_jobExecAlloc
    (
    struct a2b_StackContext*    ctx
    )
{
    a2b_JobExecutor* jobExec = A2B_NULL;

    if ( A2B_NULL != ctx )
    {
        jobExec = (a2b_JobExecutor*)A2B_MALLOC(ctx->stk, sizeof(*jobExec));
        if ( A2B_NULL != jobExec )
        {
            jobExec->schedMask = 1u;
            SLIST_INIT(&jobExec->listHead);
            jobExec->ctx = ctx;
        }
    }

    return jobExec;

} /* a2b_jobExecAlloc */


/*!**************************************************************************** 
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecFree
*
*  Frees a Job Executor and returns its resources to the system.
*
*  \param          [in]    exec     The Job Executor instance to free.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_jobExecFree
    (
    struct a2b_JobExecutor* exec
    )
{
    a2b_JobQueue* jobQ;

    if ( A2B_NULL != exec )
    {
        /* Free up any queues associated with the executor */
        while ( !SLIST_EMPTY(&exec->listHead) )
        {
            jobQ = SLIST_FIRST(&exec->listHead);
            while ( 0u != a2b_jobExecUnrefQueue(jobQ) )
            {
            }
        }

        A2B_FREE(exec->ctx->stk, exec);
    }

} /* a2b_jobExecFree */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecAllocQueue
*
*  Allocates a job queue from the Job Executor. Each allocated job queue
*  has an associated priority with a range from 0 - 4 where zero (0) is
*  the highest priority and four (4) the lowest. The priority controls how
*  frequently jobs from the given queue are scheduled during system "tick"
*  processing. Jobs in the highest priority queue are executed on every tick.
*  Allocated queues are returned with a reference count equal to one (1).
*
*  \param          [in]    exec         The Job Executor to allocate a job
*                                       queue from.
* 
*  \param          [in]    priority     The priority associated with the queue. 
*                                       The priority ranges from 0 - 4 with 0
*                                       having the highest priorityand 4 the
*                                       lowest.
*
*  \pre            None
*
*  \post           None
*
*  \return         A new job queue or A2B_NULL if the allocation has failed.
*
******************************************************************************/
A2B_DSO_LOCAL struct a2b_JobQueue*
a2b_jobExecAllocQueue
    (
    struct a2b_JobExecutor* exec,
    a2b_JobPriority         priority
    )
{
    static const a2b_UInt16 gsPriorityMasks[] = {
                                        0x01FFu, /*!< 111111111 - Highest priority */
                                        0x00AAu, /*!< 010101010 */
                                        0x0049u, /*!< 001001001 */
                                        0x0022u, /*!< 000100010 */
                                        0x0010u  /*!< 000010000 - Lowest priority */
                                        };
    a2b_JobQueue* queue = A2B_NULL;

    if ( (a2b_UInt32)priority < (a2b_UInt32)A2B_ARRAY_SIZE(gsPriorityMasks) )
    {
        queue = A2B_MALLOC(exec->ctx->stk, sizeof(*queue));
        if ( A2B_NULL != queue )
        {
            queue->priorityMask = gsPriorityMasks[priority];
            queue->action = A2B_EXEC_SUSPEND;
            queue->executor = exec;
            SIMPLEQ_INIT(&queue->qHead);
            SLIST_INSERT_HEAD(&exec->listHead, queue, link);
            queue->refCnt = 1u;
        }
    }

    return queue;

} /* a2b_jobExecAllocQueue */


/*!**************************************************************************** 
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecFlushQueue
*
*  Flushes a queue of any outstanding jobs by calling the 'onComplete'
*  function indicating the job was cancelled and then calling the job's
*  destructor.
*
*  \param          [in]    q        The queue to flush of jobs.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_jobExecFlushQueue
    (
    a2b_JobQueue*   q
    )
{
    a2b_Job* job;

    if ( A2B_NULL != q )
    {
        /* Destroy any jobs remaining in this list */
        while ( !SIMPLEQ_EMPTY(&q->qHead) )
        {
            job = SIMPLEQ_FIRST(&q->qHead);
			SIMPLEQ_REMOVE_HEAD(&q->qHead, link);
            /* Let the owner know the job was cancelled */
            if ( A2B_NULL != job->onComplete )
            {
                job->onComplete(job, A2B_TRUE /* job cancelled */);
            }

            if ( job->destroy != A2B_NULL )
            {
                job->destroy(job);
            }

            
        }
    }
} /* a2b_jobExecFlushQueue */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecRefQueue
*
*  Increments the reference count of the queue.
*
*  \param          [in]    q    The queue to increment the reference count.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_jobExecRefQueue
    (
    a2b_JobQueue*   q
    )
{
    if ( A2B_NULL != q )
    {
        q->refCnt += 1u;
    }
} /* a2b_jobExecRefQueue */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecUnrefQueue
*
*  Unreferences the job queue from the Job Executor. When the reference
*  count goes to zero the queue is freed.
*
*  \param          [in]    q        The queue to unreference.
*
*  \pre            None
*
*  \post           The job queue should no longer be used after being
*                  unreferenced.
*
*  \return         The reference count of the queue. When zero (0) it has
*                  been freed.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_UInt32
a2b_jobExecUnrefQueue
    (
        struct a2b_JobQueue*   q
    )
{
    a2b_JobExecutor* exec;
    a2b_UInt32  refCnt = 0u;

    if ( A2B_NULL != q )
    {
        A2B_TRACE2((q->executor->ctx, (A2B_TRC_DOM_STACK | A2B_TRC_LVL_TRACE1),
                    "Enter: a2b_jobExecUnrefQueue(0x%p) : RefCnt=%bd", q,
                    &q->refCnt));


        exec = q->executor;
        if ( 0u == q->refCnt )
        {
            A2B_TRACE1((exec->ctx, (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR),
                        "Queue (0x%p) is already unreferenced", q));
        }
        else
        {
            q->refCnt--;
            refCnt = q->refCnt;

            A2B_TRACE2((exec->ctx, (A2B_TRC_DOM_STACK | A2B_TRC_LVL_TRACE1),
                        "Exit: a2b_jobExecUnrefQueue(0x%p) : RefCnt=%bd",
                        q, &q->refCnt));

            if ( 0u == q->refCnt )
            {
                a2b_jobExecFlushQueue(q);

                /*
                 * Need to unlink the queue from the executor.
                 */

                SLIST_REMOVE(&exec->listHead, q, a2b_JobQueue, link);

                /* Free the queue itself. */
                A2B_FREE(exec->ctx->stk, q);
            }
        }
    }

    return refCnt;

} /* a2b_jobExecUnrefQueue */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecSubmit
*
*  Submits a job to the specified job queue. The job is added to the end
*  of the queue. It is assumed the job queue "owns" the job after it's been
*  successfully submitted.
*
*  \param          [in]    q        The job queue to receive the job.
* 
*  \param          [in]    job      The job to submit.
*
*  \pre            None
*
*  \post           If submitted successfully, the queue now "owns" the job.
*
*  \return         Returns A2B_TRUE if submitted successfully and A2B_FALSE
*                  otherwise.
*
******************************************************************************/
A2B_DSO_LOCAL a2b_Bool
a2b_jobExecSubmit
    (
    struct a2b_JobQueue*    q,
    struct a2b_Job*         job
    )
{
    a2b_Bool submitted = A2B_FALSE;
    if ( (A2B_NULL != q) && (A2B_NULL != job) )
    {
        if ( SIMPLEQ_EMPTY(&q->qHead) )
        {
            /* Make sure it's scheduled to be processed again */
            q->action = A2B_EXEC_SCHEDULE;
        }
        /* Insert the job at the end of the queue */
        SIMPLEQ_INSERT_TAIL(&q->qHead, job, link);
        submitted = A2B_TRUE;
    }

    return submitted;

} /* a2b_jobExecSubmit */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecSchedule
*
*  This function should be called periodically to schedule jobs to be
*  executed. Based on the priority of individual queues, the Job Executor
*  will execute one job at the head of each ready queue. Jobs indicating
*  they are finished processing are finalized and deallocated during the
*  scheduling process.
*
*  \param          [in]    exec     The Job Executor to schedule.
*
*  \pre            None
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_jobExecSchedule
    (
    struct a2b_JobExecutor* exec
    )
{
    a2b_JobQueue* q;
    a2b_JobQueue* nextQ;
    a2b_Job* job;
    a2b_Int32 action;

    if ( A2B_NULL != exec )
    {
        q = SLIST_FIRST(&exec->listHead);
        while ( q != SLIST_END(&exec->listHead) )
        {
            /* Add a reference to the queue in case it's
             * unreferenced as part of the job processing.
             */
            a2b_jobExecRefQueue(q);

            /* If it's the queue's turn to run then ... */
            if ( (exec->schedMask & q->priorityMask) &&
                ( A2B_EXEC_SCHEDULE == q->action) )
            {
                /* If the queue is not empty then ... */
                if ( !SIMPLEQ_EMPTY(&q->qHead) )
                {
                    /* Grab the job at the head of the queue */
                    job = SIMPLEQ_FIRST(&q->qHead);

                    /* If the job provided an execute method then ... */
                    if ( A2B_NULL != job->execute )
                    {
                        action = job->execute(job);
                        a2b_jobExecUpdate(q, action);
                    }
                }
            }

            /* Advance to the next job queue */
            nextQ = SLIST_NEXT(q, link);
            /* The queue could be free here */
            (void)a2b_jobExecUnrefQueue(q);
            q = nextQ;
        }

        /* Advance the scheduling mask to the next position */
        exec->schedMask = exec->schedMask << 1;
        if ( exec->schedMask > A2B_MAX_SCHED_MASK_VALUE )
        {
            /* Reset the mask */
            exec->schedMask = 1u;
        }
    }
} /* a2b_jobExecSchedule */


/*!****************************************************************************
*  \ingroup        a2bstack_jobexec_priv
*  \private
* 
*  \b              a2b_jobExecUpdate
*
*  This function updates the action the scheduler should take with regards
*  to the active job at the "head" (or front) of the job queue. The active
*  job can be marked as being complete, scheduled to be executed again on
*  the next scheduled time slice, or suspended from "running" until some
*  event triggers it to be re-activated.
*
*  \param          [in]    q        The job queue to change/update the
*                                   execution action.
* 
*  \param          [in]    action   The action to effect the active job which 
*                                   is at the head/front of the job queue. It
*                                   can be marked as being complete,
*                                   rescheduled to run again, or be suspended
*                                   from running until an external event causes
*                                   the action to be updated again.
*
*  \pre            There is an active job on the queue who's scheduled action
*                  is to be updated.
*
*  \post           None
*
*  \return         None
*
******************************************************************************/
A2B_DSO_LOCAL void
a2b_jobExecUpdate
    (
    a2b_JobQueue*           q,
    a2b_Int32               action
    )
{
    a2b_Job* job;

    if ( A2B_NULL != q )
    {
        /* Add a reference in case a callback tries to free the queue
         * we're working on.
         */
        a2b_jobExecRefQueue(q);

        switch ( action )
        {
            case A2B_EXEC_COMPLETE:
                if ( !SIMPLEQ_EMPTY(&q->qHead) )
                {
                    /* Reference the active job at the head of the queue */
                    job = SIMPLEQ_FIRST(&q->qHead);

                    /* Remove the job from the front of the queue */
                    SIMPLEQ_REMOVE_HEAD(&q->qHead, link);

                    /* Let the owner handle the completion event */
                    if ( A2B_NULL != job->onComplete )
                    {
                        job->onComplete(job, A2B_FALSE /* not cancelled */);
                    }

                    /* We no longer need the job. Destroy/free the
                     * resource if a destructor is provided.
                     */
                    if ( A2B_NULL != job->destroy )
                    {
                        job->destroy(job);
                    }
                }

                /* If there are more jobs to process then
                 * re-schedule the queue, otherwise, suspend processing
                 * of this queue until new jobs are submitted.
                 */
                if ( !SIMPLEQ_EMPTY(&q->qHead) )
                {
                    q->action = A2B_EXEC_SCHEDULE;
                }
                else
                {
                    q->action = A2B_EXEC_SUSPEND;
                }
                break;

            case A2B_EXEC_SCHEDULE:
            case A2B_EXEC_SUSPEND:
                q->action = action;
                break;

            default:
                /* Nothing to do - should never get here */
                A2B_TRACE1((q->executor->ctx,
                            (A2B_TRC_DOM_STACK | A2B_TRC_LVL_ERROR),
                            "Unknown execution action = %d", &action));
                break;
        }

        /* Release the queue reference - it could be freed here */
        (void)a2b_jobExecUnrefQueue(q);
    }
} /* a2b_jobExecUpdate */
