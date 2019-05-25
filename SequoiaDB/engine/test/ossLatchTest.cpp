/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "core.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "boost/thread.hpp"
#if defined (_LINUX)
#include <unistd.h>
#endif
#include <stdio.h>
#define MAXTHREADS 1024
UINT32  TESTTHREADS = 30 ;
UINT32  LOOPNUM     = 100 ;
UINT32  SLEEPTIME   = 500 ;
boost::thread *threadList[MAXTHREADS];
#if defined (_LINUX)
struct lockedobject3
{
   int count0;
   int count1;
   volatile char _latch ;
} ;
#endif
struct lockedobject
{
   int count0;
   int count1;
   ossSpinXLatch _latch;
};

struct lockedobject1
{
   int count0;
   int count1;
   ossSpinSLatch _latch;
};

struct lockedobject2
{
   int count0;
   int count1;
   ossAtomicXLatch _latch;
} ;

typedef struct lockedobject LockedObject;
typedef struct lockedobject1 LockedObject1;
typedef struct lockedobject2 LockedObject2;
#if defined (_LINUX)
typedef struct lockedobject3 LockedObject3;
#endif
LockedObject obj;
LockedObject1 obj1;
LockedObject2 obj2;
#if defined (_LINUX)
LockedObject3 obj3;
#endif
INT32 print_function_nolatch(void *ptr)
{
   int thread_id=*(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      obj.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj.count1++;
      if ( obj.count0 != obj.count1)
         printf("thread %d: %d, %d\n", thread_id, obj.count0, obj.count1);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}

INT32 print_function_latch(void *ptr)
{
   int thread_id=*(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      ossLatch(&obj._latch);
      obj.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj.count1++;
      if ( obj.count0 != obj.count1)
         printf("thread %d: %d, %d\n", thread_id, obj.count0, obj.count1);
      ossUnlatch(&obj._latch);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}

INT32 print_function_latch_shared(void *ptr)
{
   int thread_id=*(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      ossLatch(&obj1._latch, SHARED);
      obj1.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj1.count1++;
      if ( obj1.count0 != obj1.count1)
         printf("thread %d: %d, %d\n", thread_id, obj1.count0, obj1.count1);
      ossUnlatch(&obj1._latch,SHARED);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}

INT32 print_function_latch_exclusive(void *ptr)
{
   int thread_id=*(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      ossLatch(&obj1._latch, EXCLUSIVE);
      obj1.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj1.count1++;
      if ( obj1.count0 != obj1.count1)
         printf("thread %d: %d, %d\n", thread_id, obj1.count0, obj1.count1);
      ossUnlatch(&obj1._latch, EXCLUSIVE);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}

INT32 print_function_atomic_latch (void *ptr)
{
   int thread_id=*(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      ossLatch(&obj2._latch);
      obj2.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj2.count1++;
      if ( obj2.count0 != obj2.count1)
         printf("thread %d: %d, %d\n", thread_id, obj2.count0, obj2.count1);
      ossUnlatch(&obj2._latch);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}
#if defined (_LINUX)
INT32 print_function_exchange_latch ( void *ptr)
{
   int thread_id = *(int*)ptr;
   for(unsigned int i=0; i<LOOPNUM; i++)
   {
      ossLockGet8 ( &obj3._latch);
      obj3.count0++;
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
      obj3.count1++;
      if ( obj3.count0 != obj3.count1)
         printf("thread %d: %d, %d\n", thread_id, obj3.count0, obj3.count1);
      ossLockRelease8 ( &obj3._latch);
      if (SLEEPTIME)
         ossSleepmicros(SLEEPTIME);
   }
   return 0 ;
}
#endif

typedef INT32 (*threadFunc)(void*ptr) ;

void RunThreads ( threadFunc F, char *pDescription )
{
   int thread_id[MAXTHREADS];
   printf("%s", pDescription);
   getchar();
   for(unsigned int i=0; i<TESTTHREADS; i++)
   {
      thread_id[i]=i;
      threadList[i]=new boost::thread ( F,
                        (void*)&(thread_id[i]));
   }
   for(unsigned int i=0; i<TESTTHREADS; i++)
   {
      threadList[i]->join();
      delete(threadList[i]);
   }
}
int main(int argc, char** argv)
{
   if ( argc != 1 && argc != 3 && argc !=5 && argc!=7 )
   {
      printf("Syntax: %s [-t numThreads] [-l numLoop] [-s sleepTime]\n",
            (char*)argv[0] ) ;
      return 0;
   }
   for (int i=1; i<(argc>6?6:argc); i++)
   {
      if ( ossStrncmp((char*)argv[i],"-t",strlen("-t"))==0 )
      {
         TESTTHREADS=atoi((char*)argv[++i]);
         if(TESTTHREADS<0 || TESTTHREADS>MAXTHREADS)
         {
            printf("num threads must be within 0 to %d\n",MAXTHREADS);
            return 0;
         }
      }
      else if (ossStrncmp((char*)argv[i],"-l",strlen("-l"))==0 )
      {
         LOOPNUM=atoi((char*)argv[++i]);
      }
      else if (ossStrncmp((char*)argv[i],"-s",strlen("-s"))==0 )
      {
         SLEEPTIME=atoi((char*)argv[++i]);
      }
      else
      {
         printf("Syntax: %s [-t numThreads] [-l numLoop] [-s sleepTime]\n",
               (char*)argv[0] ) ;
         return 0;
      }
   }
   printf("sleeptime = %d\n", SLEEPTIME);
   printf("numThread = %d\n", TESTTHREADS);
   printf("loopNum   = %d\n", LOOPNUM);
   obj.count0=0;
   obj.count1=0;
   obj1.count0=0;
   obj1.count0=0;
   obj2.count1=0;
   obj2.count1=0;
#if defined (_LINUX)
   obj3.count0=0;
   obj3.count1=0;
#endif
   boost::posix_time::ptime t1 ;
   boost::posix_time::ptime t2 ;
   boost::posix_time::time_duration diff ;
   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_nolatch,
      "No Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n",(long long)diff.total_milliseconds()) ;

   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_latch,
      "Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n", (long long)diff.total_milliseconds()) ;

   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_latch_shared,
      "Shared Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n", (long long)diff.total_milliseconds()) ;

   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_latch_exclusive,
      "Exclusive Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n", (long long)diff.total_milliseconds()) ;

   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_atomic_latch,
      "Atomic Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n", (long long)diff.total_milliseconds()) ;
#if defined (_LINUX)
   t1 = boost::posix_time::microsec_clock::local_time() ;
   RunThreads ( print_function_exchange_latch,
      "Exchange Latch Test\nPress any key to start" ) ;
   t2 = boost::posix_time::microsec_clock::local_time() ;
   diff = t2-t1 ;
   printf ( "Takes %lld ms\n", (long long)diff.total_milliseconds()) ;
#endif
   return 0;
}
