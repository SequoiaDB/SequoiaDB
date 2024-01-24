#ifndef TEST_WORKER_HPP_
#define TEST_WORKER_HPP_

#include <string.h>

#include "core.hpp"
#include "oss.hpp"

#ifdef _WINDOWS
#include <process.h>
#include <Windows.h>
#else /* POSIX */
#include <pthread.h>
#endif

namespace test
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
         memset(&thread, 0, sizeof(thread));
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

#endif /* TEST_WORKER_HPP_ */
