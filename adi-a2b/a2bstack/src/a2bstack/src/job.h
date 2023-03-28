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
 * \file:   job.h
 * \author: Mentor Graphics, Embedded Software Division
 * \brief:  This defines the interface for a job.
 *
 *=============================================================================
 */

#ifndef A2B_JOB_H_
#define A2B_JOB_H_

/*======================= I N C L U D E S =========================*/

#include "queue.h"

/*======================= D E F I N E S ===========================*/

/*======================= D A T A T Y P E S =======================*/

A2B_BEGIN_DECLS

/* Forward declarations */
struct a2b_JobQueue;


/**
 * The definition of a "job" that can be executed by a Job Executor. This
 * structure defines a header that should be included as the first
 * field of a custom job. It allows jobs to be chained together (in a job
 * queue) and provides the methods the Job Executor will call to act on the job.
 */
typedef struct a2b_Job
{
      /** Used to link multiple jobs together in a larger job queue */
      SIMPLEQ_ENTRY(a2b_Job) link;
    
      /**
       * A custom job should initialize the following function pointers
       * with an appropriate implementation.
       */
    
      /**
       * Executes the job. The return value indicates whether the job is
       * complete, if it should be rescheduled, or execution suspended.
       */
      a2b_Int32 (* execute)(struct a2b_Job* job);
    
      /** 
       * Called when execution is complete (e.g. execute() returns
       * #A2B_EXEC_COMPLETE). The 'isCancelled' flag indicates whether
       * the job was cancelled prematurely (either before running or
       * in the middle of running).
       */
      void (* onComplete)(struct a2b_Job* job, a2b_Bool isCancelled);
    
      /**
       * Called to release any resources associated with the job. The job
       * will not be accessed by the Job Executor after this call.
       */
      void (* destroy)(struct a2b_Job* job);

} a2b_Job;


/*======================= P U B L I C  P R O T O T Y P E S ========*/


A2B_END_DECLS

/*======================= D A T A =================================*/


#endif /* A2B_JOB_H_ */
