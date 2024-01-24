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
#include <assert.h>
#include <iostream>
using std::cerr ;
using std::endl ;

namespace import
{

#ifdef _WINDOWS

   static UINT32 __stdcall _threadMain(void* arg)
   {
      WorkerThread* self;

      assert(NULL != arg);

      self = (WorkerThread*)arg;
      try
      {
         self->routine(self->args);
      }
      catch(std::exception &e)
      {
         cerr<<"unexpected err happened: "<<e.what()<<endl ;
      }
      return SDB_OK;
   }

   static INT32 _threadCreate(WorkerThread* thread)
   {
      assert(NULL != thread);
      assert(NULL != thread->routine);

      thread->thread = (HANDLE)_beginthreadex(NULL, 0,
                                              _threadMain, thread,
                                              0, NULL);
      if (NULL == thread->thread)
      {
         cerr<<"failed to create thread"<<endl ;
         return SDB_SYS;
      }

      return SDB_OK;
   }

   static INT32 _threadJoin(WorkerThread* thread)
   {
      DWORD rc;
      BOOL brc;

      assert(NULL != thread);

      rc = WaitForSingleObject (thread->thread, INFINITE);
      if (WAIT_FAILED == rc)
      {
         cerr<<"failed to wait for thread, rc="<<rc<<endl ;
         return SDB_SYS;
      }

      brc = CloseHandle(thread->thread);
      if (0 == brc)
      {
         cerr<<"failed to close thread"<<endl ;
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

      assert(NULL != arg);

      self = (WorkerThread*)arg;

      /*  No signals should be processed by this thread.
          All the signals should be delivered to main
          thread, not to worker threads. */
      ret = sigfillset(&sigset);
      assert(ret == 0);

      ret = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
      assert(ret == 0);

      try
      {
         self->routine(self->args);
      }
      catch(std::exception &e)
      {
         cerr<<"unexpected err happened: "<<e.what()<<endl;
      }
      return NULL;
   }

   static INT32 _threadCreate(WorkerThread* thread)
   {
      INT32 ret;

      assert(NULL != thread);
      assert(NULL != thread->routine);

      ret = pthread_create(&thread->thread, NULL, _threadMain, thread);
      if (0 != ret)
      {
         cerr<<"failed to create thread"<<endl;
         return SDB_SYS;
      }

      return SDB_OK;
   }

   static INT32 _threadJoin(WorkerThread* thread)
   {
      INT32 ret;

      assert(NULL != thread);

      ret = pthread_join(thread->thread, NULL);
      if (0 != ret)
      {
         cerr<<"failed to join thread"<<endl;
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
         delete _thread.args ;
      }
   }

   INT32 Worker::start()
   {
      INT32 rc = SDB_OK;
	  
      assert(!_started);

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

      assert(_started);

      rc = _threadJoin(&_thread);
      if (SDB_OK == rc)
      {
         _started = FALSE;
      }

      return rc;
   }
}
