/*******************************************************************************

   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
      sdbDSWorker( workerFunc func, void* args, BOOLEAN managed = FALSE ) ;
      ~sdbDSWorker() ;
      INT32 start() ;
      INT32 waitStop() ;

   private:
      sdbDSWorkThread         _thread ;
      BOOLEAN                 _started ;
      BOOLEAN                 _managed ;
   } ;
}

#endif

