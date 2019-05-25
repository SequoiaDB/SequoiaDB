/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = impWorker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impWorker.hpp"
#include "pd.hpp"

namespace import
{

#ifdef _WINDOWS

   static UINT32 __stdcall _threadMain(void* arg)
   {
      WorkerThread* self;

      SDB_ASSERT(NULL != arg, "arg can't be NULL");

      self = (WorkerThread*)arg;
      try
      {
         self->routine(self->args);
      }
      catch(std::exception &e)
      {
         PD_LOG(PDERROR, "unexpected err happened:%s", e.what());
      }
      return SDB_OK;
   }

   static INT32 _threadCreate(WorkerThread* thread)
   {
      SDB_ASSERT(NULL != thread, "thread can't be NULL");
      SDB_ASSERT(NULL != thread->routine, "routine can't be NULL");

      thread->thread = (HANDLE)_beginthreadex(NULL, 0,
                                              _threadMain, thread,
                                              0, NULL);
      if (NULL == thread->thread)
      {
         PD_LOG(PDERROR, "failed to create thread");
         return SDB_SYS;
      }

      return SDB_OK;
   }

   static INT32 _threadJoin(WorkerThread* thread)
   {
      DWORD rc;
      BOOL brc;

      SDB_ASSERT(NULL != thread, "thread can't be NULL");

      rc = WaitForSingleObject (thread->thread, INFINITE);
      if (WAIT_FAILED == rc)
      {
         PD_LOG(PDERROR, "failed to wait for thread, rc=%d", rc);
         return SDB_SYS;
      }

      brc = CloseHandle(thread->thread);
      if (0 == brc)
      {
         PD_LOG(PDERROR, "failed to close thread");
         return SDB_SYS;
      }

      return SDB_OK;
   }

#else /* POSIX */

#include <signal.h>

   static void* _threadMain(void* arg)
   {
      WorkerThread* self;
      sigset_t sigset;
      INT32 ret;

      SDB_ASSERT(NULL != arg, "arg can't be NULL");

      self = (WorkerThread*)arg;

      /*  No signals should be processed by this thread.
          All the signals should be delivered to main
          thread, not to worker threads. */
      ret = sigfillset(&sigset);
      SDB_ASSERT(ret == 0, "");

      ret = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
      SDB_ASSERT(ret == 0, "");

      try
      {
         self->routine(self->args);
      }
      catch(std::exception &e)
      {
         PD_LOG(PDERROR, "unexpected err happened:%s", e.what());
      }
      return NULL;
   }

   static INT32 _threadCreate(WorkerThread* thread)
   {
      INT32 ret;

      SDB_ASSERT(NULL != thread, "thread can't be NULL");
      SDB_ASSERT(NULL != thread->routine, "routine can't be NULL");

      ret = pthread_create(&thread->thread, NULL, _threadMain, thread);
      if (0 != ret)
      {
         PD_LOG(PDERROR, "failed to create thread");
         return SDB_SYS;
      }

      return SDB_OK;
   }

   static INT32 _threadJoin(WorkerThread* thread)
   {
      INT32 ret;

      SDB_ASSERT(NULL != thread, "thread can't be NULL");

      ret = pthread_join(thread->thread, NULL);
      if (0 != ret)
      {
         PD_LOG(PDERROR, "failed to join thread");
         return SDB_SYS;
      }

      return SDB_OK;
   }

#endif

   Worker::Worker(WorkerRoutine routine, WorkerArgs* args, BOOLEAN managed)
   : _thread(routine, args),
     _managed(managed)
   {
      _started = FALSE;
   }

   Worker::~Worker()
   {
      if (_managed)
      {
         SAFE_OSS_DELETE(_thread.args);
      }
   }

   INT32 Worker::start()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_started, "worker already started");

      rc = _threadCreate(&_thread);
      if (SDB_OK == rc)
      {
         _started = TRUE;
      }

      return rc;
   }

   INT32 Worker::waitStop()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_started, "worker didn't start");

      rc = _threadJoin(&_thread);
      if (SDB_OK == rc)
      {
         _started = FALSE;
      }

      return rc;
   }
}
