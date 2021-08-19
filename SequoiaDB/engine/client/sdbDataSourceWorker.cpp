/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbDataSourceWorker.cpp

   Descriptive Name = SDB Data Source Worker Source File

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbDataSourceWorker.hpp"
#include "pd.hpp"

namespace sdbclient
{
#ifdef _WINDOWS
   static UINT32 __stdcall _threadMain( void* arg )
   {
      SDB_ASSERT( NULL != arg, "arg can't be NULL" ) ;

      sdbDSWorkThread* self ;
      self = (sdbDSWorkThread*)arg ;
      self->func( self->args ) ;
      return SDB_OK ;
   }

   static INT32 _threadCreate( sdbDSWorkThread* thread )
   {
      SDB_ASSERT( NULL != thread, "thread can't be NULL" ) ;
      SDB_ASSERT( NULL != thread->func, "routine can't be NULL" ) ;

      thread->thread = (HANDLE)_beginthreadex(
         NULL, 0, _threadMain, thread, 0, NULL ) ;
      if (NULL == thread->thread)
         return SDB_SYS ;
      return SDB_OK ;
   }

   static INT32 _threadJoin( sdbDSWorkThread* thread )
   {
      DWORD rc ;
      BOOL brc ;
      
      SDB_ASSERT( NULL != thread, "thread can't be NULL" ) ;

      rc = WaitForSingleObject( thread->thread, 
         INFINITE ) ;
      if ( WAIT_FAILED == rc )
         return SDB_SYS ;
      brc = CloseHandle( thread->thread ) ;
      if ( 0 == brc )
         return SDB_SYS ;
      return SDB_OK ;
   }
#else     /*POSIX*/
#include <signal.h>
   static void* _threadMain( void* arg )
   {
      sdbDSWorkThread* self ;
      sigset_t sigset ;
      INT32 ret ;

      SDB_ASSERT( NULL != arg, "arg can't be NULL" ) ;

      self = (sdbDSWorkThread*)arg ;
      ret = sigfillset( &sigset ) ;
      SDB_ASSERT( ret == 0, "" ) ;
      
      ret = pthread_sigmask( SIG_BLOCK, &sigset, NULL ) ;
      SDB_ASSERT( ret == 0, "" ) ;

      self->func( self->args ) ;
      return NULL ;
   }

   static INT32 _threadCreate( sdbDSWorkThread* thread )
   {
      INT32 ret ;

      SDB_ASSERT( NULL != thread, "thread can't be NULL" ) ;
      SDB_ASSERT( NULL != thread->func, "routine can't be NULL" ) ;


      ret = pthread_create( &thread->thread, NULL, _threadMain, thread ) ;
      if ( 0 != ret )
         return SDB_SYS ;
      return SDB_OK ;
   }

   static INT32 _threadJoin( sdbDSWorkThread* thread )
   {
      INT32 ret ;

      SDB_ASSERT( NULL != thread, "thread can't be NULL" ) ;

      ret = pthread_join( thread->thread, NULL ) ;
      if ( 0 != ret )
         return SDB_SYS ;
      return SDB_OK ;
   }

#endif

   sdbDSWorker::sdbDSWorker( workerFunc func, void* args )
      : _thread( func, args )
   {
      _started = FALSE ;
   }

   sdbDSWorker::~sdbDSWorker()
   {
   }

   INT32 sdbDSWorker::start()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !_started, "worker already started" ) ;

      rc = _threadCreate( &_thread ) ;
      if ( SDB_OK == rc )
      {
         _started = TRUE ;
      }

      return rc ;
   }

   INT32 sdbDSWorker::waitStop()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _started, "worker didn't start" ) ;

      rc = _threadJoin( &_thread ) ;
      if ( SDB_OK == rc )
      {
         _started = FALSE ;
      }

      return rc ;
   }
}
