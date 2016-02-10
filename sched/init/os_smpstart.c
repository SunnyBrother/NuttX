/****************************************************************************
 * sched/init/os_smpstart.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sched.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>

# include "init/init.h"

#ifdef CONFIG_SMP

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: os_idletask
 *
 * Description:
 *   This is the common IDLE task for CPUs 1 through (CONFIG_SMP_NCPUS-1).
 *   It is equivalent to the CPU 0 IDLE logic in os_start.c
 *
 * Input Parameters:
 *   Standard task arguments.
 *
 * Returned Value:
 *   This function does not return.
 *
 ****************************************************************************/

int os_idletask(int argc, FAR char *argv[])
{
  sdbg("CPU%d: Beginning Idle Loop\n");
  for (; ; )
    {
      /* Perform garbage collection (if it is not being done by the worker
       * thread).  This cleans-up memory de-allocations that were queued
       * because they could not be freed in that execution context (for
       * example, if the memory was freed from an interrupt handler).
       */

#ifndef CONFIG_SCHED_WORKQUEUE
      /* We must have exclusive access to the memory manager to do this
       * BUT the idle task cannot wait on a semaphore.  So we only do
       * the cleanup now if we can get the semaphore -- this should be
       * possible because if the IDLE thread is running, no other task is!
       *
       * WARNING: This logic could have undesirable side-effects if priority
       * inheritance is enabled.  Imaginee the possible issues if the
       * priority of the IDLE thread were to get boosted!  Moral: If you
       * use priority inheritance, then you should also enable the work
       * queue so that is done in a safer context.
       */

      if (kmm_trysemaphore() == 0)
        {
          sched_garbagecollection();
          kmm_givesemaphore();
        }
#endif

      /* Perform any processor-specific idle state operations */

      up_idle();
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: os_smpstart
 *
 * Description:
 *   In an SMP configution, only one CPU is initially active (CPU 0). System
 *   initialization occurs on that single thread. At the completion of the
 *   initialization of the OS, just before beginning normal multitasking,
 *   the additional CPUs would be started by calling this function.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero on success; a negater errno value on failure.
 *
 ****************************************************************************/

int os_smpstart(void)
{
  int ret;
  int i;

  for (i = 1; i < CONFIG_SMP_NCPUS; i++)
    {
      ret = up_cpustart(i, os_idletask);
#ifdef CONFIG_DEBUG
      if (ret < 0)
        {
          sdbg("ERROR: Failed to start CPU%d: %d\n", i, ret);
          return ret;
        }
#else
      UNUSED(ret);
#endif
    }

  return OK;
}

#endif /* CONFIG_SMP */