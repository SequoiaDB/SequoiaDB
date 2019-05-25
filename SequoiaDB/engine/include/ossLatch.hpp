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

   Source File Name = ossLatch.hpp

   Descriptive Name = Operating System Services Latch Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for latch operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SPINLOCK_HPP_
#define OSS_SPINLOCK_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"

#if defined (SDB_ENGINE)
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#endif //SDB_ENGINE

#if defined (_WINDOWS)
#include <WinBase.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif //_WINDOWS

#include "ossAtomicBase.hpp"

typedef SINT32 ossLockType ;
typedef volatile ossLockType ossLock ;
#define OSS_LOCK_LOCKED   1
#define OSS_LOCK_UNLOCKED 0

#define ossLockPeek(pLock) *(volatile const ossLock *)pLock
/*
static OSS_INLINE ossLockType ossLockPeek( volatile const ossLock * const pLock )
{
   return *pLock ;
}*/

#define ossLockGetStatus(pLock) ossAtomicFetch32( ( volatile SINT32 * )pLock )
/*
static OSS_INLINE ossLockType ossLockGetStatus(volatile const ossLock * const pLock)
{
   return ossAtomicFetch32( ( volatile SINT32 * )pLock ) ;
}*/

#define ossLockTestGet(pLock) ossCompareAndSwap32( pLock, OSS_LOCK_UNLOCKED, \
                                                   OSS_LOCK_LOCKED )
/*
static OSS_INLINE BOOLEAN ossLockTestGet( volatile ossLock * const  pLock )
{
   return ( ossCompareAndSwap32( pLock, OSS_LOCK_UNLOCKED, OSS_LOCK_LOCKED ) ) ;
}*/

#define ossLockInit(pLock) *(volatile ossLock *)pLock=OSS_LOCK_UNLOCKED
/*
static OSS_INLINE void ossLockInit( volatile ossLock * const pLock )
{
   *pLock = OSS_LOCK_UNLOCKED ;
}*/

#define ossLockRelease(pLock) \
        {\
           ossCompilerFence() ;\
           ossAtomicExchange32((volatile ossLock *)pLock, OSS_LOCK_UNLOCKED );\
        }
/*
static OSS_INLINE void ossLockRelease( volatile ossLock * const pLock )
{
   ossCompilerFence() ;
   ossAtomicExchange32( pLock, OSS_LOCK_UNLOCKED ) ;
}*/

#define ossWait(x) \
        {\
           UINT32 __i = 0 ;\
           do\
           {\
              ossYield() ;\
              __i++;\
           }while(__i<x);\
        }
/*
static OSS_INLINE void ossWait( UINT32 x )
{
   UINT32 i = 0 ;
   do
   {
      ossYield() ;
      i++ ;
   } while ( i < x ) ;
}*/

#define ossLockGet(pLock) \
        {\
           while(!ossLockTestGet((volatile ossLock*)pLock))\
           {\
              ossWait( 1 );\
           }\
        }
/*
static OSS_INLINE void ossLockGet( volatile ossLock * const pLock )
{
   while( ! ossLockTestGet( pLock ) )
   {
   }
}*/

#if defined (_LINUX) || defined (_AIX)
static OSS_INLINE void ossLockGet8 ( volatile CHAR * const pLock )
{
   if ( !*pLock && !ossAtomicExchange8 ( pLock, OSS_LOCK_LOCKED ) )
      return ;
   for ( int i=0; i<1000; i++ )
   {
      if ( !ossAtomicExchange8 ( pLock, OSS_LOCK_LOCKED ) )
         return ;
      ossYield () ;
   }
   while ( ossAtomicExchange8 ( pLock, OSS_LOCK_LOCKED ) )
      usleep ( 1000 ) ;
}

static OSS_INLINE void ossLockRelease8 ( volatile CHAR * const pLock )
{
   ossCompilerFence() ;
   ossAtomicExchange8( pLock, OSS_LOCK_UNLOCKED ) ;
}
#endif
/*
 * Interface for read/write lock
 */
class ossSLatch : public SDBObject
{
public:
   virtual ~ossSLatch (){} ;
   /*
    * Exclusive lock
    * Wait until lock is get
    */
   virtual void get () = 0 ;
   /*
    * Unlock exclusive lock
    */
   virtual void release () = 0 ;
   /*
    * shared lock
    * Wait until lock is get
    */
   virtual void get_shared () = 0 ;
   /*
    * unlock shared lock
    */
   virtual void release_shared () = 0 ;
   /*
    * get exclusive lock if possible
    * return TRUE if exc lock is get
    * return FALSE if it can't get
    */
   virtual BOOLEAN try_get () = 0 ;
   /*
    * get shared lock if possible
    * return TRUE if shared lock is get
    * return FALSE if it can't get
    */
   virtual BOOLEAN try_get_shared () = 0 ;
} ;

/*
 * Interface for simple (exclusive) lock
 */
class ossXLatch : public SDBObject
{
public :
   virtual ~ossXLatch (){} ;
   /*
    * lock
    * Wait until lock is get
    */
   virtual void get ()= 0 ;
   /*
    * Unlock lock
    */
   virtual void release () = 0 ;
   /*
    * get lock if possible
    * return TRUE if lock is get
    * return FALSE if it can't get
    */
   virtual BOOLEAN try_get() = 0 ;
} ;

class _ossAtomicXLatch : public ossXLatch
{
private :
   ossLock lock ;
public :
   _ossAtomicXLatch ()
   {
      ossLockInit( &lock ) ;
   }
   ~_ossAtomicXLatch ()
   {
   }

   BOOLEAN try_get ()
   {
      return ( ossLockTestGet( &lock ) ) ;
   }

   void get()
   {
      ossLockGet( &lock ) ;
   }

   void release()
   {
      ossLockRelease( &lock ) ;
   }
} ;
typedef class _ossAtomicXLatch ossAtomicXLatch ;

/*
   _ossSpinXLatch define
*/
class _ossSpinXLatch : public ossXLatch
{
#if defined(_WIN32)
private :
   HANDLE _lock ;
public:
   _ossSpinXLatch ()
   {
      _lock = CreateEvent( NULL, FALSE, TRUE, NULL ) ;
      SDB_ASSERT( _lock, "CreateEvent failed" ) ;
   }
   ~_ossSpinXLatch ()
   {
      CloseHandle( _lock ) ;
   }
   void get ()
   {
      SDB_ASSERT( WAIT_OBJECT_0 == WaitForSingleObject( _lock, INFINITE ),
                  "Wait Single Object failed" ) ;
   }
   void release ()
   {
      SetEvent( _lock ) ;
   }
   BOOLEAN try_get ()
   {
      return ( WAIT_OBJECT_0 == WaitForSingleObject( _lock, 0 ) ) ? TRUE : FALSE ;
   }
/*#elif defined (__USE_XOPEN2K)
private :
   pthread_spinlock_t _lock ;
public :
   _ossSpinXLatch ()
   {
      pthread_spin_init ( &_lock , 0 ) ;
   }
   ~_ossSpinXLatch ()
   {
      pthread_spin_destroy ( &_lock ) ;
   }
   void get()
   {
      if ( pthread_spin_trylock ( &_lock ) == 0 )
         return ;
      for ( int i=0; i<1000; i++ )
      {
         if ( pthread_spin_trylock ( &_lock ) == 0 )
            return ;
         ossYield() ;
      }
      for ( int i=0; i<1000; i++ )
      {
         if ( pthread_spin_trylock ( &_lock ) == 0 )
            return ;
         pthread_yield () ;
      }
      while ( pthread_spin_trylock( &_lock ) != 0 )
         usleep ( 1000 ) ;
   }
   void release ()
   {
      pthread_spin_unlock ( &_lock ) ;
   }
   BOOLEAN try_get ()
   {
      return pthread_spin_trylock ( &_lock ) == 0 ;
   }
#elif  defined (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
private :
   volatile int _lock;
public :
   _ossSpinXLatch ()
   {
      _lock = 0 ;
   }
   ~_ossSpinXLatch (){}
   void get ()
   {
      if( !_lock && !__sync_lock_test_and_set ( &_lock, 1 ) )
         return ;
      for ( int i=0; i<1000; i++ )
      {
         if ( !__sync_lock_test_and_set ( &_lock, 1 ) )
            return ;
         ossYield () ;
      }
      while ( __sync_lock_test_and_set(&_lock, 1) )
         usleep ( 1000 ) ;
   }
   void release()
   {
      __sync_lock_release ( &_lock ) ;
   }
   BOOLEAN try_get()
   {
      return ( !_lock && !__sync_lock_test_and_set ( &_lock, 1 ) ) ;
   }*/
#else
private :
   pthread_mutex_t _lock ;
public :
   _ossSpinXLatch ()
   {
      SDB_ASSERT ( pthread_mutex_init ( &_lock, 0 ) == 0,
                   "Failed to init mutex" ) ;
   }
   ~_ossSpinXLatch()
   {
      SDB_ASSERT ( pthread_mutex_destroy ( &_lock ) == 0,
                   "Failed to destroy mutex" ) ;
   }
   void get ()
   {
      SDB_ASSERT ( pthread_mutex_lock ( &_lock ) == 0,
                   "Failed to lock mutex" ) ;
   }
   void release ()
   {
      SDB_ASSERT ( pthread_mutex_unlock ( &_lock ) == 0,
                   "Failed to unlock mutex" ) ;
   }
   BOOLEAN try_get ()
   {
      return ( pthread_mutex_trylock ( &_lock ) == 0 ) ;
   }
#endif
} ;
typedef class _ossSpinXLatch ossSpinXLatch ;

class _ossSpinRecursiveXLatch : public ossXLatch
{
#if defined(_WIN32)
private:
   CRITICAL_SECTION _cs ;
public:
   _ossSpinRecursiveXLatch()
   {
      InitializeCriticalSection( &_cs ) ;
   }

   ~_ossSpinRecursiveXLatch()
   {
      DeleteCriticalSection( &_cs ) ;
   }

   void get()
   {
      EnterCriticalSection( &_cs ) ;
   }

   void release()
   {
      LeaveCriticalSection( &_cs ) ;
   }

   BOOLEAN try_get()
   {
      return TryEnterCriticalSection( &_cs ) ;
   }
#else
private:
   pthread_mutex_t _lock ;
public:
   _ossSpinRecursiveXLatch()
   {
      pthread_mutexattr_t _attr ;
      SDB_ASSERT( pthread_mutexattr_init( &_attr ) == 0,
                  "Failed to init mutex attribute" ) ;
      SDB_ASSERT( pthread_mutexattr_settype( &_attr,
                                             PTHREAD_MUTEX_RECURSIVE ) == 0,
                  "Failed to set mutex type" ) ;

      SDB_ASSERT( pthread_mutex_init( &_lock, &_attr ) == 0,
                  "Failed to init mutex" ) ;
   }

   ~_ossSpinRecursiveXLatch()
   {
      SDB_ASSERT( pthread_mutex_destroy( &_lock ) == 0,
                  "Failed to destroy mutex" ) ;
   }

   void get()
   {
      SDB_ASSERT( pthread_mutex_lock( &_lock ) == 0,
                  "Failed to lock mutex" ) ;
   }

   void release()
   {
      SDB_ASSERT( pthread_mutex_unlock( &_lock ) == 0,
                  "Failed to unlock mutex" ) ;
   }

   BOOLEAN try_get()
   {
      return ( pthread_mutex_trylock( &_lock ) == 0 ) ;
   }
#endif
} ;
typedef _ossSpinRecursiveXLatch ossSpinRecursiveXLatch ;

#if defined (_WINDOWS)
/*
   _ossSRWLock define
*/
class _ossSRWLock
{
   enum OSS_SRWLOCK_TYPE
   {
      SRW_LOCK_NONE     = 0,
      SRW_LOCK_SHARED,
      SRW_LOCK_EXCLUSIVE
   } ;

   public:
      _ossSRWLock()
      {
         _sharedNum = 0 ;
         _exclusiveNum = 0 ;
         _lockType = SRW_LOCK_NONE ;

         _mutex = ::CreateMutex( NULL, FALSE, NULL ) ;
         _sharedEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL ) ;
         _exclusiveEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL ) ;
      }
      ~_ossSRWLock()
      {
         ::CloseHandle( _mutex ) ;
         ::CloseHandle( _sharedEvent ) ;
         ::CloseHandle( _exclusiveEvent ) ;
      }

      INT32    acquireShared( INT64 millisec = -1 )
      {
         ::WaitForSingleObject( _mutex, INFINITE ) ;
         ++_sharedNum ;

         if ( SRW_LOCK_EXCLUSIVE == _lockType )
         {
            DWORD retCode = 0 ;
            retCode = ::SignalObjectAndWait( _mutex, _sharedEvent,
                                             millisec < 0 ? INFINITE : millisec,
                                             FALSE ) ;
            if ( WAIT_OBJECT_0 == retCode )
            {
               return SDB_OK ;
            }
            else if ( WAIT_TIMEOUT == retCode )
            {
               return SDB_TIMEOUT ;
            }
            else
            {
               return SDB_SYS ;
            }
         }

         _lockType = SRW_LOCK_SHARED ;
         ::ReleaseMutex( _mutex ) ;

         return SDB_OK ;
      }
      void     releaseShared()
      {
         SDB_ASSERT( _lockType == SRW_LOCK_SHARED && _sharedNum > 0,
                     "No shared lock" ) ;

         ::WaitForSingleObject( _mutex, INFINITE ) ;
         --_sharedNum ;

         if ( _sharedNum == 0 )
         {
            if ( _exclusiveNum > 0 )
            {
               _lockType = SRW_LOCK_EXCLUSIVE ;
               ::SetEvent( _exclusiveEvent ) ;
            }
            else
            {
               _lockType = SRW_LOCK_NONE ;
            }
         }

         ::ReleaseMutex( _mutex ) ;
      }

      INT32    acquire( INT64 millisec = -1 )
      {
         ::WaitForSingleObject( _mutex, INFINITE );
         ++_exclusiveNum ;

         if ( SRW_LOCK_NONE != _lockType )
         {
            DWORD retCode = 0 ;
            retCode = ::SignalObjectAndWait( _mutex, _exclusiveEvent,
                                             millisec < 0 ? INFINITE : millisec,
                                             FALSE ) ;
            if ( WAIT_OBJECT_0 == retCode )
            {
               return SDB_OK ;
            }
            else if ( WAIT_TIMEOUT == retCode )
            {
               return SDB_TIMEOUT ;
            }
            else
            {
               return SDB_SYS ;
            }
         }

         _lockType = SRW_LOCK_EXCLUSIVE ;
         ::ReleaseMutex( _mutex ) ;

         return SDB_OK ;
      }
      void     release()
      {
         SDB_ASSERT( _lockType == SRW_LOCK_EXCLUSIVE && _exclusiveNum > 0,
                     "No exclusive lock" ) ;

         ::WaitForSingleObject( _mutex, INFINITE ) ;
         --_exclusiveNum ;

         if ( _exclusiveNum > 0 )
         {
            ::SetEvent( _exclusiveEvent ) ;
         }
         else if ( _sharedNum > 0 )
         {
            _lockType = SRW_LOCK_SHARED ;
            ::PulseEvent( _sharedEvent ) ;
         }
         else
         {
            _lockType = SRW_LOCK_NONE ;
         }

         ::ReleaseMutex( _mutex ) ;
      }

   private:
      HANDLE               _mutex ;
      HANDLE               _sharedEvent ;
      HANDLE               _exclusiveEvent ;
      volatile INT32       _sharedNum ;
      volatile INT32       _exclusiveNum ;
      volatile INT32       _lockType ;
} ;
typedef _ossSRWLock ossSRWLock ;

#endif // _WINDOWS

/*
   _ossSpinSLatch define
*/
class _ossSpinSLatch : public ossSLatch
{
#if defined (_WINDOWS)

#if defined (USE_SRW)
private :
   SRWLOCK _lock ;
public:
   _ossSpinSLatch ()
   {
      InitializeSRWLock ( &_lock ) ;
   }
   ~_ossSpinSLatch() {} ;
   void get ()
   {
      AcquireSRWLockExclusive ( &_lock ) ;
   }
   void release ()
   {
      ReleaseSRWLockExclusive ( &_lock ) ;
   }
   void get_shared()
   {
      AcquireSRWLockShared ( &_lock ) ;
   }
   void release_shared ()
   {
      ReleaseSRWLockShared ( &_lock ) ;
   }
   BOOLEAN try_get_shared ()
   {
      return TryAcquireSRWLockShared ( &_lock ) ;
   }
   BOOLEAN try_get ()
   {
      return TryAcquireSRWLockExclusive ( &_lock ) ;
   }
#else
private :
   ossSRWLock _lock ;
public:
   _ossSpinSLatch ()
   {
   }
   ~_ossSpinSLatch() {} ;
   void get ()
   {
      _lock.acquire( -1 ) ;
   }
   void release ()
   {
      _lock.release() ;
   }
   void get_shared()
   {
      _lock.acquireShared( -1 ) ;
   }
   void release_shared ()
   {
      _lock.releaseShared() ;
   }
   BOOLEAN try_get_shared ()
   {
      return SDB_OK == _lock.acquireShared( 0 ) ? TRUE : FALSE ;
   }
   BOOLEAN try_get ()
   {
      return SDB_OK == _lock.acquire( 0 ) ? TRUE : FALSE ;
   }
#endif //USE_SRW

#elif defined (SDB_ENGINE)
private :
   boost::shared_mutex _lock ;
public :
   _ossSpinSLatch ()
   {
   }
   ~_ossSpinSLatch()
   {
   }
   void get ()
   {
      try
      {
         _lock.lock() ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch get failed" ) ;
      }
   }
   void release ()
   {
      try
      {
         _lock.unlock() ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch release failed" ) ;
      }
   }
   void get_shared ()
   {
      try
      {
         _lock.lock_shared () ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch get shared failed" ) ;
      }
   }
   void release_shared ()
   {
      try
      {
         _lock.unlock_shared() ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch release shared failed" ) ;
      }
   }
   BOOLEAN try_get ()
   {
      try
      {
         return _lock.try_lock_for( boost::chrono::milliseconds ( 0 ) ) ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch try get failed" ) ;
      }
      return FALSE ;
   }
   BOOLEAN try_get_shared()
   {
      try
      {
         return _lock.try_lock_shared_for( boost::chrono::milliseconds ( 0 ) ) ;
      }
      catch(...)
      {
         SDB_ASSERT ( FALSE, "SLatch try get shared failed" ) ;
      }
      return FALSE ;
   }

#else
private :
   pthread_rwlock_t  _lock ;
public :
   _ossSpinSLatch ()
   {
      SDB_ASSERT( 0 == pthread_rwlock_init( &_lock, NULL ),
                  "init rwlock failed" ) ;
   }
   ~_ossSpinSLatch()
   {
      SDB_ASSERT( 0 == pthread_rwlock_destroy( &_lock ),
                  "destory rwlock failed" ) ;
   }
   void get ()
   {
      INT32 rc = pthread_rwlock_wrlock( &_lock ) ;
      SDB_ASSERT( 0 == rc, "write rwlock failed" ) ;
   }
   void release ()
   {
      INT32 rc = pthread_rwlock_unlock( &_lock ) ;
      SDB_ASSERT( 0 == rc, "release write rwlock failed" ) ;
   }
   void get_shared ()
   {
      INT32 rc = pthread_rwlock_rdlock( &_lock ) ;
      SDB_ASSERT( 0 == rc, "read rwlock failed" ) ;
   }
   void release_shared ()
   {
      INT32 rc = pthread_rwlock_unlock( &_lock ) ;
      SDB_ASSERT( 0 == rc, "release read rwlock failed" ) ;
   }
   BOOLEAN try_get ()
   {
      INT32 rc = pthread_rwlock_trywrlock( &_lock ) ;
      return 0 == rc ? TRUE : FALSE ;
   }
   BOOLEAN try_get_shared()
   {
      INT32 rc = pthread_rwlock_tryrdlock( &_lock ) ;
      return 0 == rc ? TRUE : FALSE ;
   }
#endif
} ;
typedef class _ossSpinSLatch ossSpinSLatch ;

enum OSS_LATCH_MODE
{
   SHARED ,
   EXCLUSIVE
} ;
void ossLatch ( ossSLatch *latch, OSS_LATCH_MODE mode ) ;
void ossLatch ( ossXLatch *latch ) ;
void ossUnlatch ( ossSLatch *latch, OSS_LATCH_MODE mode ) ;
void ossUnlatch ( ossXLatch *latch ) ;
BOOLEAN ossTestAndLatch ( ossSLatch *latch, OSS_LATCH_MODE mode ) ;
BOOLEAN ossTestAndLatch ( ossXLatch *latch ) ;

class _ossScopedLock
{
private :
   ossSLatch *_slatch ;
   ossXLatch *_xlatch ;
   OSS_LATCH_MODE _mode ;
public :
   _ossScopedLock ( ossSLatch *latch ) :
         _slatch ( NULL ), _xlatch ( NULL ), _mode ( EXCLUSIVE )
   {
      if ( latch )
      {
         _slatch = latch ;
         _mode = EXCLUSIVE ;
         _xlatch = NULL ;
         _slatch->get () ;
      }
   }
   _ossScopedLock ( ossSLatch *latch, OSS_LATCH_MODE mode) :
         _slatch ( NULL ), _xlatch ( NULL ), _mode ( EXCLUSIVE )
   {
      if ( latch )
      {
         _slatch = latch ;
         _mode = mode ;
         _xlatch = NULL ;
         if ( mode == EXCLUSIVE )
            _slatch->get () ;
         else
            _slatch->get_shared () ;
      }
   }
   _ossScopedLock ( ossXLatch *latch ) :
         _slatch ( NULL ), _xlatch ( NULL ), _mode ( EXCLUSIVE )
   {
      if ( latch )
      {
         _xlatch = latch ;
         _slatch = NULL ;
         _xlatch->get () ;
      }
   }
   ~_ossScopedLock ()
   {
      if ( _slatch )
         ( _mode == EXCLUSIVE ) ? _slatch->release() :
                                  _slatch->release_shared() ;
      else if ( _xlatch )
         _xlatch->release () ;
   }
} ;
typedef class _ossScopedLock ossScopedLock;

#endif //OSS_SPINLOCK_HPP_
