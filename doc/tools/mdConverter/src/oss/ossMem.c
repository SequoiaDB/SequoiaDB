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

   Source File Name = ossMem.c

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.h"
#include "ossMem.h"
#include "ossUtil.h"
#if defined (_LINUX) || defined (_AIX)
#include <stdlib.h>
#elif defined (_WINDOWS)
#include <malloc.h>
#endif

// in engine we always compile in C++, so we can include pd.hpp
#if defined (SDB_ENGINE)
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ossTrace.h"
#else
#define SDB__OSSMEMALLOC
#define SDB__OSSMEMREALLOC
#define SDB__OSSMEMFREE
#define PD_TRACE_ENTRY(x)
#define PD_TRACE_EXIT(x)
#endif

BOOLEAN ossMemDebugEnabled = FALSE ;
UINT32 ossMemDebugSize = 0 ;

#define OSS_MEM_HEADSZ 32

#define OSS_MEM_HEAD_EYECATCHER1SIZE  sizeof(UINT32)
#define OSS_MEM_HEAD_FREEDSIZE sizeof(UINT32)
#define OSS_MEM_HEAD_SIZESIZE    sizeof(UINT64)
#define OSS_MEM_HEAD_DEBUGSIZE   sizeof(UINT32)
#define OSS_MEM_HEAD_FILESIZE    sizeof(UINT32)
#define OSS_MEM_HEAD_LINESIZE    sizeof(UINT32)
#define OSS_MEM_HEAD_KEYECATCHER2SIZE sizeof(UINT32)

#define OSS_MEM_HEAD_EYECATCHER1OFFSET 0
#define OSS_MEM_HEAD_FREEDOFFSET \
      OSS_MEM_HEAD_EYECATCHER1OFFSET + OSS_MEM_HEAD_EYECATCHER1SIZE
#define OSS_MEM_HEAD_SIZEOFFSET \
      OSS_MEM_HEAD_FREEDOFFSET + OSS_MEM_HEAD_FREEDSIZE
#define OSS_MEM_HEAD_DEBUGOFFSET   \
      OSS_MEM_HEAD_SIZEOFFSET + OSS_MEM_HEAD_SIZESIZE
#define OSS_MEM_HEAD_FILEOFFSET    \
      OSS_MEM_HEAD_DEBUGOFFSET + OSS_MEM_HEAD_DEBUGSIZE
#define OSS_MEM_HEAD_LINEOFFSET    \
      OSS_MEM_HEAD_FILEOFFSET + OSS_MEM_HEAD_FILESIZE
#define OSS_MEM_HEAD_EYECATCHER2OFFSET \
      OSS_MEM_HEAD_LINEOFFSET + OSS_MEM_HEAD_LINESIZE

#define SDB_MEMDEBUG_ENDPOS     sizeof(CHAR)

#if defined (_LINUX) || defined (_AIX)
#define OSS_MEM_MAX_SZ 4294967295ll
#elif defined (_WINDOWS)
#define OSS_MEM_MAX_SZ 4294967295LL
#endif

// in C version, we don't do anything
// in C++, we keep track of all memory allocations from ossMemAlloc
#if !defined (__cplusplus) || !defined (SDB_ENGINE)
void ossMemTrack ( void *p ) {}
void ossMemUnTrack ( void *p ) {}
void ossMemTrace ( const CHAR *pPath ) {}
#endif

static BOOLEAN ossMemSanityCheck ( void *p )
{
   CHAR *headerMem = NULL ;
   if ( !p )
      return TRUE ;
   headerMem = ((CHAR*)p) - OSS_MEM_HEADSZ ;
   return ( *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER1OFFSET) ==
            SDB_MEMHEAD_EYECATCHER1 ) &&
          ( *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER2OFFSET) ==
            SDB_MEMHEAD_EYECATCHER2 ) ;
}
// this function verify whether a given pointer is at starting of a memory block
// and not overflowed
// basically we just move back ossMemDebugSize bytes and make sure it's all with
// right guardian bytes
// and then move back size bytes and make sure it's followed by GUARDEND and
// GUARDSTOP
static BOOLEAN ossMemVerify ( void *p )
{
   CHAR *headerMem  = NULL ;
   UINT32 debugSize = 0 ;
   CHAR *pStart     = NULL ;
   UINT32 i         = 0 ;
   UINT64 size      = 0 ;
   CHAR *pEnd       = NULL ;
   // return TRUE for NULL
   if ( !p )
      return TRUE ;
   // get header
   headerMem = ((CHAR*)p) - OSS_MEM_HEADSZ ;
   // sanity check eye catcher1
   if ( *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER1OFFSET) !=
        SDB_MEMHEAD_EYECATCHER1 )
   {
#if defined (SDB_ENGINE)
      PD_LOG ( PDSEVERE, "eye catcher 1 doesn't match: %u",
               *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER1OFFSET) ) ;
#endif
      return FALSE ;
   }
   // sanity check eye catcher2
   if ( *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER2OFFSET) !=
        SDB_MEMHEAD_EYECATCHER2 )
   {
#if defined (SDB_ENGINE)
      PD_LOG ( PDSEVERE, "eye catcher 2 doesn't match: %u",
               *(UINT32*)(headerMem+OSS_MEM_HEAD_EYECATCHER2OFFSET) ) ;
#endif
      return FALSE ;
   }
   // sanity check isFree
   if ( *(UINT32*)(headerMem+OSS_MEM_HEAD_FREEDOFFSET) != 0 )
   {
#if defined (SDB_ENGINE)
      PD_LOG ( PDSEVERE, "memory is already freed: %u",
               *(UINT32*)(headerMem+OSS_MEM_HEAD_FREEDOFFSET) ) ;
#endif
      return FALSE ;
   }
   // get debug size
   debugSize = *(UINT32*)(headerMem+OSS_MEM_HEAD_DEBUGOFFSET) ;
   // do not verify if we don't have debug segment
   if ( 0 == debugSize )
      return TRUE ;
   // debug size sanity check
   if ( debugSize > SDB_MEMDEBUG_MAXGUARDSIZE ||
        debugSize < SDB_MEMDEBUG_MINGUARDSIZE ||
        debugSize > ossMemDebugSize )
   {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "debug size is not valid: %d", debugSize ) ;
#endif
      return FALSE ;
   }
   // get real memory segment start point
   pStart = headerMem - debugSize ;
   // make sure all guardian start got SDB_MEMDEBUG_GUARDSTART
   for ( i = 0; i < debugSize; ++i )
   {
      if ( *(pStart + i) != SDB_MEMDEBUG_GUARDSTART )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "unexpected start byte read from 0x%p",
                  *(pStart + i)) ;
#endif
         return FALSE ;
      }
   }
   // then check the size
   size = *(UINT64*)(headerMem+OSS_MEM_HEAD_SIZEOFFSET) ;
   // then jump over size
   pEnd = ((CHAR*)p)+size ;
   // check end byte
   if ( *pEnd != SDB_MEMDEBUG_GUARDEND )
   {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "unexpected end byte read from 0x%p",
                  *pEnd) ;
#endif
      return FALSE ;
   }
   // make sure all guardian stop got SDB_MEMDEBUG_GUARDSTOP
   for ( i = SDB_MEMDEBUG_ENDPOS; i < debugSize; ++i )
   {
      if ( *(pEnd+i) != SDB_MEMDEBUG_GUARDSTOP )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "unexpected stop byte read from 0x%p",
                  *(pEnd+i)) ;
#endif
         return FALSE ;
      }
   }
   return TRUE ;
}

static void ossMemFixHead ( CHAR *p,
                            size_t datasize, UINT32 debugSize,
                            const CHAR *file, UINT32 line )
{
   *(UINT32*)(p+OSS_MEM_HEAD_EYECATCHER1OFFSET) = SDB_MEMHEAD_EYECATCHER1 ;
   *(UINT64*)(p+OSS_MEM_HEAD_SIZEOFFSET)    = datasize ;
   *(UINT32*)(p+OSS_MEM_HEAD_DEBUGOFFSET)   = debugSize ;
   *(UINT32*)(p+OSS_MEM_HEAD_LINEOFFSET)    = line ;
   *(UINT32*)(p+OSS_MEM_HEAD_EYECATCHER2OFFSET) = SDB_MEMHEAD_EYECATCHER2 ;
   *(UINT32*)(p+OSS_MEM_HEAD_FREEDOFFSET)   = 0 ;
   // if memory debug is enabled, let's keep track of all memory allocations
   if ( ossMemDebugEnabled )
      ossMemTrack ( p ) ;
}

// allocate extra header only, do not allocate memory debug section
static void *ossMemAlloc1 ( size_t size, const CHAR* file, UINT32 line )
{
   CHAR *p = NULL ;
   UINT64 totalSize = size + OSS_MEM_HEADSZ ;
   p = (CHAR*)malloc ( totalSize ) ;
   if ( !p )
      return NULL ;
   ossMemFixHead ( p, size, 0, file, line ) ;
   return ((CHAR*)p)+OSS_MEM_HEADSZ ;
}

// allocate extra header + debug segment
static void *ossMemAlloc2 ( size_t size, const CHAR* file, UINT32 line )
{
   CHAR *p          = NULL ;
   CHAR *expMem     = NULL ;
   UINT32 debugSize = ossMemDebugSize ;
   UINT32 endSize   = 0 ;
   UINT64 totalSize = 0 ;
   debugSize = OSS_MIN ( debugSize, SDB_MEMDEBUG_MAXGUARDSIZE ) ;
   debugSize = OSS_MAX ( debugSize, SDB_MEMDEBUG_MINGUARDSIZE ) ;
   // calculate how many bytes to fill up guardstop bytes
   // need to subtract one byte for GUARDEND
   endSize = debugSize - SDB_MEMDEBUG_ENDPOS ;
   // add size with 2 debug size, for begin and end
   totalSize = size + OSS_MEM_HEADSZ + ( debugSize<<1 ) ;

   // actually allocate memory
   p = (CHAR*)malloc ( totalSize ) ;
   // if allocation failed, return NULL
   if ( !p )
      return NULL ;
   // initialize head
   ossMemFixHead ( p + debugSize, size, debugSize, file, line ) ;
   // expected memory is starting from p + ossMemDebugSize
   expMem = p + debugSize + OSS_MEM_HEADSZ ;
   // init START part
   // fill start guardian bytes
   ossMemset ( p, SDB_MEMDEBUG_GUARDSTART, debugSize ) ;

   // STOP PART
   // fill stop gardian bytes
   // first we set 1 byte to indicate normal end
   *(expMem + size) = SDB_MEMDEBUG_GUARDEND ;
   // then we fill up stop guardian bytes
   ossMemset ( expMem+size+SDB_MEMDEBUG_ENDPOS,
               SDB_MEMDEBUG_GUARDSTOP,
               endSize ) ;
   // put 0 into the memory block
   ossMemset ( expMem, 0, size ) ;
   // return the pointer where actual data supposed to start
   return expMem ;
}

void ossEnableMemDebug( BOOLEAN debugEnable, UINT32 memDebugSize )
{
   ossMemDebugEnabled   = debugEnable ;
   ossMemDebugSize      = memDebugSize ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMEMALLOC, "ossMemAlloc" )
void* ossMemAlloc ( size_t size, const CHAR* file, UINT32 line )
{
//   PD_TRACE_ENTRY ( SDB__OSSMEMALLOC ) ;
   void *p = NULL ;
   if ( size == 0 )
      p = NULL ;
   else if ( !ossMemDebugEnabled || !ossMemDebugSize )
      p = ossMemAlloc1 ( size, file, line ) ;
   else
      p = ossMemAlloc2 ( size, file, line ) ;
//   PD_TRACE_EXIT ( SDB__OSSMEMALLOC ) ;
   return p ;
}

// reallocate memory with header + debug section
static void *ossMemRealloc2 ( void* pOld, size_t size,
                              const CHAR* file, UINT32 line )
{
   // get original pointer
   CHAR *p          = NULL ;
   CHAR *expMem     = NULL ;
   CHAR *headerMem  = NULL ;
   // for old memory block size
   UINT64 oldSize   = 0 ;
   UINT32 debugSize = ossMemDebugSize ;
   UINT32 endSize   = 0 ;
   UINT64 totalSize = 0 ;
   // for the difference between old and new memory size
   UINT64 diffSize = 0 ;
   debugSize = OSS_MIN ( debugSize, SDB_MEMDEBUG_MAXGUARDSIZE ) ;
   debugSize = OSS_MAX ( debugSize, SDB_MEMDEBUG_MINGUARDSIZE ) ;

   if ( pOld )
   {
      BOOLEAN checkHead = ossMemVerify ( pOld ) ;
      if ( !checkHead )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "Invalid memory verified at 0x%p", pOld ) ;
#endif
         ossPanic () ;
      }
      // if memory check passes, let's calculate the real memory starting point
      headerMem = ((CHAR*)pOld) - OSS_MEM_HEADSZ ;
      // get the debug size for this segment, note if the debugSize doens't
      // match the current ossMemDebugSize, we do not change the current
      // debugSize because it's realloc
      debugSize = *(UINT32*)(headerMem+OSS_MEM_HEAD_DEBUGOFFSET) ;
      // get the original memory buffer size
      oldSize = *(UINT64*)(headerMem+OSS_MEM_HEAD_SIZEOFFSET) ;
      // set the old-buffer as free
      *(UINT32*)(headerMem+OSS_MEM_HEAD_FREEDOFFSET) = 1 ;
      p = headerMem - debugSize ;
   }
   // get the difference between old and new memory size
   if ( size > oldSize )
      diffSize = size - oldSize ;
   // calculate how many bytes to fill up guardstop bytes
   // need to subtract one byte for GUARDEND
   endSize = debugSize - SDB_MEMDEBUG_ENDPOS ;
   // get total number of bytes including guardian page
   totalSize = size + OSS_MEM_HEADSZ + ( debugSize<<1 ) ;
   // start reallocate memory
   p = (CHAR*)realloc ( p, totalSize ) ;
   if ( !p )
      return NULL ;

   // initialize header
   ossMemFixHead ( p + debugSize, size, debugSize, file, line ) ;
   // expected memory is starting from p + ossMemDebugSize
   expMem = p + OSS_MEM_HEADSZ + debugSize ;
   // init START part
   // fill start guardian bytes
   ossMemset ( p, SDB_MEMDEBUG_GUARDSTART, debugSize ) ;

   // STOP PART
   // fill stop gardian bytes
   // first we set 1 byte to indicate normal end
   *(expMem + size) = SDB_MEMDEBUG_GUARDEND ;
   // then we fill up stop guardian bytes
   ossMemset ( expMem+size+SDB_MEMDEBUG_ENDPOS,
               SDB_MEMDEBUG_GUARDSTOP,
               endSize ) ;
   // if the new memory size is greater than old one, let's initialize 0 to the
   // newly added part
   if ( diffSize )
      ossMemset ( expMem + oldSize, 0, diffSize ) ;
   // return the pointer where actual data supposed to start
   return expMem ;
}

// reallocate memory with header only
static void *ossMemRealloc1 ( void* pOld, size_t size,
                              const CHAR* file, UINT32 line )
{
   CHAR *p = NULL ;
   UINT64 totalSize = size + OSS_MEM_HEADSZ ;
   if ( pOld )
   {
      BOOLEAN checkHead = ossMemSanityCheck ( pOld ) ;
      if ( !checkHead )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "Invalid memory verified at 0x%p", pOld ) ;
#endif
         ossPanic () ;
      }
      p = ((CHAR*)pOld - OSS_MEM_HEADSZ ) ;
      if ( *(UINT32*)( p + OSS_MEM_HEAD_DEBUGOFFSET ) != 0 )
         return ossMemRealloc2 ( pOld, size, file, line ) ;
      *(UINT32*)(p+OSS_MEM_HEAD_FREEDOFFSET) = 1 ;
   }
   p = (CHAR*)realloc ( p, totalSize ) ;
   if ( !p )
      return NULL ;
   ossMemFixHead ( p, size, 0, file, line ) ;
   return ((CHAR*)p)+OSS_MEM_HEADSZ ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMEMREALLOC, "ossMemRealloc" )
void* ossMemRealloc ( void* pOld, size_t size,
                      const CHAR* file, UINT32 line )
{
//   PD_TRACE_ENTRY ( SDB__OSSMEMREALLOC ) ;
   void *p = NULL ;
   if ( size == 0 )
      p = NULL ;
   else if ( !ossMemDebugEnabled || !ossMemDebugSize )
      p = ossMemRealloc1 ( pOld, size, file, line ) ;
   else
      p = ossMemRealloc2 ( pOld, size, file, line ) ;
//   PD_TRACE_EXIT ( SDB__OSSMEMREALLOC ) ;
   return p ;
}

void ossMemFree2 ( void *p )
{
   if ( p )
   {
      CHAR *headerMem   = NULL ;
      UINT32 debugSize  = 0 ;
      CHAR *pStart      = NULL ;
      BOOLEAN checkHead = ossMemVerify ( p ) ;
      if ( !checkHead )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "Invalid memory verified at 0x%p", p ) ;
#endif
         ossPanic () ;
      }
      headerMem = ((CHAR*)p) - OSS_MEM_HEADSZ ;
      debugSize = *(UINT32*)(headerMem+OSS_MEM_HEAD_DEBUGOFFSET) ;
      // mark the memory is freed
      *(UINT32*)(headerMem+OSS_MEM_HEAD_FREEDOFFSET)   = 1 ;
      pStart = headerMem - debugSize ;
      free ( pStart ) ;
      // remove memory if we enable memory debug
      if ( ossMemDebugEnabled )
         ossMemUnTrack ( pStart ) ;
   }
}

void ossMemFree1 ( void *p )
{
   if ( p )
   {
      CHAR *pStart = NULL ;
      BOOLEAN checkHead = ossMemSanityCheck ( p ) ;
      if ( !checkHead )
      {
#if defined (SDB_ENGINE)
         PD_LOG ( PDSEVERE, "Invalid memory verified at %p", p ) ;
#endif
         ossPanic () ;
      }
      pStart = ((CHAR*)p) - OSS_MEM_HEADSZ ;
      if ( *(UINT32*)(pStart + OSS_MEM_HEAD_DEBUGOFFSET ) != 0 )
         ossMemFree2 ( p ) ;
      else
      {
         *(UINT32*)((CHAR*)pStart+OSS_MEM_HEAD_FREEDOFFSET)   = 1 ;
         free ( pStart ) ;
         // remove memory if we enable memory debug
         if ( ossMemDebugEnabled )
            ossMemUnTrack ( pStart ) ;
      }
   }
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMEMFREE, "ossMemFree" )
void ossMemFree ( void *p )
{
//   PD_TRACE_ENTRY ( SDB__OSSMEMFREE ) ;
   if ( !ossMemDebugEnabled || !ossMemDebugSize )
      ossMemFree1 ( p ) ;
   else
      ossMemFree2 ( p ) ;
//   PD_TRACE_EXIT ( SDB__OSSMEMFREE ) ;
}

void *ossAlignedAlloc( UINT32 alignment, UINT32 size )
{
#if defined (_LINUX) || defined (_AIX)
   void *ptr = NULL ;
   /// returns zero on success, or one of the 
   ///  error values listed in the next section on failure.
   /// Note that errno is not set.
   INT32 rc = SDB_OK ;
   rc = posix_memalign( &ptr, alignment, size ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return ptr ;
error:
   if ( NULL != ptr )
   {
      SDB_OSS_ORIGINAL_FREE( ptr ) ;
      ptr = NULL ;
   }
   goto done ;
#elif defined (_WINDOWS)
   return _aligned_malloc( size, alignment ) ;
#else
   return NULL ;
#endif   
}

