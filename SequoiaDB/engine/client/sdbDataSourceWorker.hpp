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

   Source File Name = sdbDataSourceWorker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/8/2016    LXJ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDBDATASOURCE_WORKER_HPP_
#define SDBDATASOURCE_WORKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /*POSIX*/
#include <pthread.h>
#endif

namespace sdbclient
{
   typedef void (*workerFunc)( void* ) ;

   struct sdbDSWorkThread
   {
      #ifdef _WINDOWS
      HANDLE         thread ;
      #else /*POSIX*/
      pthread_t      thread ;
      #endif

      workerFunc     func ;
      void*          args ;

      sdbDSWorkThread( workerFunc func, void* args )
      {
         ossMemset( &thread, 0, sizeof(thread) ) ;
         this->func = func ;
         this->args = args ;
      }

   } ;

   class sdbDSWorker : public SDBObject
   {
   public:
      sdbDSWorker( workerFunc func, void* args ) ;
      ~sdbDSWorker() ;
      INT32 start() ;
      INT32 waitStop() ;

   private:
      sdbDSWorkThread         _thread ;
      BOOLEAN                 _started ;
   } ;
}

#endif

