/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = impWorker.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_WORKER_HPP_
#define IMP_WORKER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /* POSIX */
#include <pthread.h>
#endif

namespace import
{
   class WorkerArgs: public SDBObject
   {
   private:
      // disallow copy and assign
      WorkerArgs(const WorkerArgs&);
      void operator=(const WorkerArgs&);
   protected:
      WorkerArgs() {}
   public:
      virtual ~WorkerArgs() {}
   };

   typedef void (*WorkerRoutine)(WorkerArgs*);

   struct WorkerThread: public SDBObject
   {
#ifdef _WINDOWS
      HANDLE         thread;
#else /* POSIX */
      pthread_t      thread;
#endif
      WorkerRoutine  routine;
      WorkerArgs*    args;

      WorkerThread(WorkerRoutine routine, WorkerArgs* args)
      {
         ossMemset(&thread, 0, sizeof(thread));
         this->routine = routine;
         this->args = args;
      }
   };

   class Worker: public SDBObject
   {
   public:
      Worker(WorkerRoutine routine, WorkerArgs* args, BOOLEAN managed = FALSE);
      ~Worker();
      INT32 start();
      INT32 waitStop();

   private:
      WorkerThread   _thread;
      BOOLEAN        _started;
      BOOLEAN        _managed; // true if manage the args memory
   };

}

#endif /* IMP_WORKER_HPP_ */
