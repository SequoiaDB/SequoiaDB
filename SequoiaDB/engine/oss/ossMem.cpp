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

   Source File Name = ossMem.cpp

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
#include "ossMem.hpp"
#include "ossMem.c"

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL ) || defined ( SDB_SHELL )

#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "filenames.hpp"
#include "ossVer.h"
#include <set>
#include <map>

#include "../bson/bson.h"

/*
   _ossMemInfoAssit define
*/
struct _ossMemInfoAssit
{
   UINT32   _file ;
   UINT32   _line ;
   UINT64   _size ;
   UINT32   _debugSize ;

   _ossMemInfoAssit( const void* p )
   {
      const CHAR *pAddr = ( const CHAR * )p ;

      _file = *(UINT32*)( pAddr + OSS_MEM_HEAD_FILEOFFSET ) ;
      _line = *(UINT32*)( pAddr + OSS_MEM_HEAD_LINEOFFSET ) ;
      _size = *(UINT64*)( pAddr + OSS_MEM_HEAD_SIZEOFFSET ) ;
      _debugSize = *(UINT32*)( pAddr + OSS_MEM_HEAD_DEBUGOFFSET ) ;
   }
} ;
typedef _ossMemInfoAssit ossMemInfoAssit ;

/*
   _ossMemTrackItem define
*/
struct _ossMemTrackItem
{
   const CHAR* _p ;
   const CHAR* _pInfo ;
   UINT64      _fileline ;
   UINT64      _size ;
   UINT64      _time ;
   UINT32      _tid ;

   _ossMemTrackItem( void *p = NULL )
   {
      _p = ( const CHAR* )p ;
      _pInfo = NULL ;
      _fileline = 0 ;
      _size = 0 ;
      _time = 0 ;
      _tid = 0 ;
   }
   _ossMemTrackItem( void *p, UINT32 file, UINT32 line, UINT64 size, const CHAR *pInfo )
   {
      _p = ( const CHAR* )p ;
      _pInfo = pInfo ;
      _fileline = ((UINT64)file << 32) | (UINT64)line ;
      _size = size ;
      _time = 0 ;
      _tid = 0 ;
   }

   UINT32 getFile() const
   {
      return (UINT32)( _fileline >> 32 ) ;
   }

   UINT32 getLine() const
   {
      return (UINT32)_fileline ;
   }

   const CHAR* getInfo() const
   {
      return _pInfo ;
   }

   bool operator< ( const _ossMemTrackItem &rhs ) const
   {
      return _p < rhs._p ? true : false ;
   }
   bool operator== ( const _ossMemTrackItem &rhs ) const
   {
      return _p == rhs._p ? true : false ;
   }
} ;
typedef _ossMemTrackItem ossMemTrackItem ;

/*
   _ossMemStatItem define
*/
struct _ossMemStatItem
{
   UINT64      _times ;
   UINT64      _totalSize ;
   UINT64      _freeInTC ;
   UINT64      _freeInTCTimes ;
   const CHAR* _pInfo ;

   _ossMemStatItem()
   {
      _times = 0 ;
      _totalSize = 0 ;
      _freeInTC = 0 ;
      _freeInTCTimes = 0 ;
      _pInfo = NULL ;
   }

   bool operator< ( const _ossMemStatItem &rhs ) const
   {
      /// reverse
      return _times > rhs._times ? true : false ;
   }
} ;
typedef _ossMemStatItem ossMemStatItem ;

/// Item title and format
#define OSS_MEM_DUMP_ITEM_TITLE \
   "        ID              Address           Size    FillSize                                          File      Line       TID    Time"

#define OSS_MEM_DUMP_ITER_FOMART \
   "%10ld   %18p     %10ld  %10ld    %30s(%10u)    %6u"

#define OSS_MEM_DUMP_ITEM_FORMAT_TID_TIME \
   "    %6u    %s"

#define OSS_MEM_DUMP_ERRORSTR " ****(has error)"

/// Stat title and format

#define OSS_MEM_DUMP_STAT_TITLE \
   "        ID                            File              :   Line ----            Count           TotalSize"

#define OSS_MEM_DUMP_STAT_FORMAT \
   "%10ld    %30s(%10u): %6u ---- %16lu    %16ld"

typedef std::set<ossMemTrackItem>               OSS_MEM_TRACKMAP ;
typedef OSS_MEM_TRACKMAP::iterator              OSS_MEM_TRACKMAP_IT ;
typedef std::map<UINT64,ossMemStatItem>         OSS_MEM_STATMAP ;
typedef OSS_MEM_STATMAP::iterator               OSS_MEM_STATMAP_IT ;
typedef OSS_MEM_STATMAP::const_iterator         OSS_MEM_STATMAP_CIT ;
typedef std::multimap<ossMemStatItem, UINT64>   OSS_MEM_SORTMAP ;
typedef OSS_MEM_SORTMAP::iterator               OSS_MEM_SORTMAP_IT ;
typedef OSS_MEM_SORTMAP::const_iterator         OSS_MEM_SORTMAP_CIT ;

#define OSS_MEM_TRACKCB_NAME_SZ     ( 64 )
#define OSS_MEM_TRACEDUMP_TM_BUF    ( 64 )
#define OSSMEMTRACEDUMPBUFSZ        ( 1024 )

/*
   _ossMemTrackCB define
*/
class _ossMemTrackCB
{
   private:
      ossSpinRecursiveXLatch     _memTrackMutex ;
      OSS_MEM_TRACKMAP           _memTrackMap ;
      OSS_MEM_STATMAP            _memStatMap ;
      CHAR                       _name[ OSS_MEM_TRACKCB_NAME_SZ + 1 ] ;
      UINT64                     _resetTime ;

      /// buf
      CHAR                       _linebuff[ OSSMEMTRACEDUMPBUFSZ + 1 ] ;
      CHAR                       _timebuff[ OSS_MEM_TRACEDUMP_TM_BUF ] ;

      // stat
      UINT64                     _lastDumpTime ;
      UINT64                     _lastTrackNum ;
      UINT64                     _lastTrackSize ;
      UINT64                     _lastStatTimes ;
      UINT64                     _lastStatSize ;

   protected:
      virtual BOOLEAN   verifyItem( const ossMemTrackItem &item ) const
      {
         if ( !ossMemVerify( ( void* )( item._p + OSS_MEM_HEADSZ ) ) )
         {
            return FALSE ;
         }

         /// check size
         ossMemInfoAssit assit( ( const void* )item._p ) ;
         if ( assit._size != item._size  )
         {
#if defined (SDB_ENGINE)
            PD_LOG ( PDSEVERE, "memory size(%lld) is not the same with "
                     "track item(%lld)",
                     assit._size, item._size ) ;
#endif
            return FALSE ;
         }
         else if ( assit._file != item.getFile() ||
                   assit._line != item.getLine() )
         {
#if defined (SDB_ENGINE)
            PD_LOG ( PDSEVERE, "memory file(%d) or line(%d) is not the "
                     "same with track item's file(%d) or line(%d)",
                     assit._file, assit._line,
                     item.getFile(), item.getLine() ) ;
#endif
            return FALSE ;
         }

         return TRUE ;
      }

      virtual UINT64    getFillSize( const ossMemTrackItem &item ) const
      {
         ossMemInfoAssit assit( (const void*)item._p ) ;
         return OSS_MEM_HEADSZ + ( assit._debugSize << 1 ) ;
      }

      virtual UINT32    traceItemExpand( const ossMemTrackItem &item,
                                         CHAR *pBuff, UINT32 buffSize ) const
      {
         return 0 ;
      }

      virtual BOOLEAN   isFreeInTC( const ossMemTrackItem &item ) const
      {
         return FALSE ;
      }

   protected:
      void memTraceItem( UINT64 index,
                         const ossMemTrackItem &item,
                         ossPrimitiveFileOp &trapFile,
                         BOOLEAN &isError,
                         BOOLEAN &freeInTc )
      {
         UINT32 len = 0 ;

         if ( ossMemDebugVerify )
         {
            isError = verifyItem( item ) ? FALSE : TRUE ;
         }
         else
         {
            isError = FALSE ;
         }

         freeInTc = isFreeInTC( item ) ;

         if ( isError || ossMemDebugDetail )
         {
            len = ossSnprintf( _linebuff, sizeof( _linebuff),
                               OSS_MEM_DUMP_ITER_FOMART,
                               index, item._p, item._size,
                               getFillSize( item ),
                               autoGetFileName( item.getFile() ).c_str(),
                               item.getFile(),
                               item.getLine() ) ;

            if ( 0 != item._tid || 0 != item._time )
            {
               ossTimestamp tm ;
               tm.time = item._time / 1000000 ;
               tm.microtm = item._time % 1000000 ;
               ossTimestampToString( tm, _timebuff ) ;

               len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                                   OSS_MEM_DUMP_ITEM_FORMAT_TID_TIME,
                                   item._tid, _timebuff ) ;
            }

            len += traceItemExpand( item, _linebuff + len,
                                    sizeof( _linebuff ) - len ) ;

            /// add error
            if ( isError )
            {
               len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                                   OSS_MEM_DUMP_ERRORSTR ) ;
            }

            len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                                "\n" ) ;

            trapFile.Write( _linebuff, len ) ;
         }
      }

      void memTraceStatItem( UINT64 index,
                             UINT64 fileline,
                             const ossMemStatItem &item,
                             ossPrimitiveFileOp &trapFile )
      {
         UINT32 len = 0 ;
         UINT32 file = (UINT32)( fileline >> 32 ) ;
         UINT32 line = (UINT32)fileline ;

         len = ossSnprintf( _linebuff, sizeof( _linebuff ),
                            OSS_MEM_DUMP_STAT_FORMAT,
                            index,
                            autoGetFileName( file ).c_str(),
                            file, line,
                            item._times, item._totalSize ) ;

         if ( item._freeInTC > 0 )
         {
            len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                               " (FreeInTC: %llu)",
                               item._freeInTC ) ;
         }

         if ( item._pInfo )
         {
            len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                               " (%s)", item._pInfo ) ;
         }

         len += ossSnprintf( _linebuff + len, sizeof( _linebuff ) - len,
                             OSS_NEWLINE ) ;

         trapFile.Write( _linebuff, len ) ;
      }

      void addStat( OSS_MEM_STATMAP &mapStat,
                    UINT64 fileline,
                    UINT64 size,
                    BOOLEAN isFreeInTc,
                    const CHAR *pInfo )
      {
         try
         {
            ossMemStatItem &item = mapStat[ fileline ] ;
            ++item._times ;
            item._totalSize += size ;
            item._pInfo = pInfo ;

            if ( isFreeInTc )
            {
               item._freeInTC += size ;
               ++item._freeInTCTimes ;
            }            
         }
         catch ( ... )
         {
         }
      }

      void memStatMap2SortMap( const OSS_MEM_STATMAP &mapStat,
                               OSS_MEM_SORTMAP &mapSort )
      {
         try
         {
            OSS_MEM_STATMAP_CIT cit = mapStat.begin() ;
            while ( cit != mapStat.end() )
            {
               mapSort.insert( OSS_MEM_SORTMAP::value_type( cit->second,
                                                            cit->first ) ) ;
               ++cit ;
            }
         }
         catch ( ... )
         {
         }
      }

      void memTrace( ossPrimitiveFileOp &trapFile,
                     const OSS_MEM_TRACKMAP &mapTrack,
                     OSS_MEM_STATMAP &mapStat,
                     UINT64 *pTotalSize = NULL )
      {
         OSS_MEM_TRACKMAP_IT it ;
         UINT64 totalSize = 0 ;
         UINT64 index = 0 ;
         UINT64 totalError = 0 ;
         BOOLEAN isError = FALSE ;
         BOOLEAN isFreeInTc = FALSE ;

         totalError = 0 ;

         /// write begin
         ossSnprintf( _linebuff, sizeof( _linebuff ),
                      "\n\n-------- Memory Information List( Run Time ) --------\n" ) ;
         trapFile.Write( _linebuff ) ;

         /// write title
         trapFile.Write( OSS_MEM_DUMP_ITEM_TITLE ) ;
         trapFile.Write( "\n\n" ) ;

         /// write item
         it = mapTrack.begin() ;
         while ( it != mapTrack.end() )
         {
            const ossMemTrackItem &item = *it ;
            totalSize += item._size ;
            memTraceItem( ++index, item, trapFile, isError, isFreeInTc ) ;
            if ( isError )
            {
               ++totalError ;
            }
            /// insert to map stat
            addStat( mapStat, item._fileline, item._size,
                     isFreeInTc, item._pInfo ) ;
            ++it ;
         }

         /// write result
         ossSnprintf( _linebuff, sizeof( _linebuff ),
                      "\n\n"
                      "+++++++++++++++++++\n"
                      "TotalSize  : %llu\n"
                      "TotalNum   : %llu\n"
                      "TotalError : %llu\n"
                      "+++++++++++++++++++\n",
                      totalSize, index, totalError ) ;
         trapFile.Write( _linebuff ) ;

         if ( pTotalSize )
         {
            *pTotalSize = totalSize ;
         }
      }

      void memStatTrace( ossPrimitiveFileOp &trapFile,
                         const OSS_MEM_SORTMAP &mapSort,
                         const CHAR *prefix,
                         UINT64 *pTotalSize = NULL,
                         UINT64 *pTotalTimes = NULL )
      {
         OSS_MEM_SORTMAP_CIT cit ;
         UINT64 totalSize = 0 ;
         UINT64 totalFreeInTC = 0 ;
         UINT64 totalTimes = 0 ;
         UINT64 freeInTCTimes = 0 ;         
         
         UINT64 index = 0 ;

         if ( !prefix )
         {
            prefix = "Unknown" ;
         }

         /// write begin
         ossSnprintf( _linebuff, sizeof( _linebuff ),
                      "\n\n-------- Memory Information Stat( %s ) --------\n",
                      prefix ) ;
         trapFile.Write( _linebuff ) ;

         /// write title
         trapFile.Write( OSS_MEM_DUMP_STAT_TITLE ) ;
         trapFile.Write( "\n\n" ) ;

         /// write item
         cit = mapSort.begin() ;
         while ( cit != mapSort.end() )
         {
            totalSize += cit->first._totalSize ;
            totalFreeInTC += cit->first._freeInTC ;
            totalTimes += cit->first._times ;
            freeInTCTimes += cit->first._freeInTCTimes ;
            memTraceStatItem( ++index, cit->second, cit->first, trapFile ) ;
            ++cit ;
         }

         /// write result
         ossSnprintf( _linebuff, sizeof( _linebuff ),
                      "\n\n"
                      "+++++++++++++++++++\n"
                      "TotalSize     : %llu\n"
                      "TotalNum      : %llu\n"
                      "FreeInTCSize  : %llu\n"
                      "FreeInTCNum   : %llu\n"
                      "StatNum       : %llu\n"
                      "+++++++++++++++++++\n",
                      totalSize, totalTimes,
                      totalFreeInTC, freeInTCTimes,
                      index ) ;
         trapFile.Write( _linebuff ) ;

         if ( pTotalSize )
         {
            *pTotalSize = totalSize ;
         }
         if ( pTotalTimes )
         {
            *pTotalTimes = totalTimes ;
         }
      }

   public:
      _ossMemTrackCB ( const CHAR *name )
      {
         ossMemset( _name, 0, sizeof( _name ) ) ;
         ossStrncpy( _name, name, OSS_MEM_TRACKCB_NAME_SZ ) ;

         ossMemset( _linebuff, 0, sizeof( _linebuff ) ) ;
         ossMemset( _timebuff, 0, sizeof( _timebuff ) ) ;
         _resetTime = ossGetCurrentMicroseconds() ;

         _lastDumpTime = _resetTime ;
         _lastTrackNum = 0 ;
         _lastTrackSize = 0 ;
         _lastStatTimes = 0 ;
         _lastStatSize = 0 ;
      }
      virtual ~_ossMemTrackCB ()
      {
      }

      void reset()
      {
         ossSignalShield shield ;
         shield.doNothing() ;

         _memTrackMutex.get() ;

         _memTrackMap.clear() ;
         _memStatMap.clear() ;
         _resetTime = ossGetCurrentMicroseconds() ;

         _lastDumpTime = _resetTime ;
         _lastTrackNum = 0 ;
         _lastTrackSize = 0 ;
         _lastStatTimes = 0 ;
         _lastStatSize = 0 ;

         _memTrackMutex.release() ;
      }

      const CHAR* getName() const { return _name ; }

      void memTrack ( void *p, UINT32 file, UINT32 line, UINT64 size, const CHAR *pInfo = NULL )
      {
         ossSignalShield shield ;
         shield.doNothing() ;

         ossMemTrackItem item( p, file, line, size, pInfo ) ;
         item._tid = ossGetCurrentThreadID() ;
         item._time = ossGetCurrentMicroseconds() ;

         _memTrackMutex.get() ;

         /// add stat
         addStat( _memStatMap, item._fileline, item._size,
                  FALSE, item._pInfo ) ;
         /// add to item
         try
         {
            _memTrackMap.insert( item ) ;
         }
         catch ( ... )
         {
         }
         _memTrackMutex.release() ;
      }

      void memUnTrack ( void *p )
      {
         ossSignalShield shield ;
         shield.doNothing() ;

         ossMemTrackItem item( p ) ;

         _memTrackMutex.get() ;
         try
         {
            _memTrackMap.erase( item ) ;
         }
         catch ( ... )
         {
         }
         _memTrackMutex.release() ;
      }

      void memTrace( ossPrimitiveFileOp &trapFile )
      {
         UINT32 len = 0 ;
         OSS_MEM_SORTMAP mapSort ;
         OSS_MEM_STATMAP mapLocalStat ;
         UINT64 beginTime = 0 ;
         UINT64 endTime = 0 ;
         ossTimestamp tm ;
         CHAR beginTimebuff[ OSS_MEM_TRACEDUMP_TM_BUF ] = { 0 } ;
         CHAR versionTxt[ 100 ] = { 0 } ;

         UINT64 trackNum = 0 ;
         UINT64 trackSize = 0 ;
         UINT64 statTimes = 0 ;
         UINT64 statSize = 0 ;

         ossSignalShield shield ;
         shield.doNothing() ;

         beginTime = ossGetCurrentMicroseconds() ;

         tm.time = beginTime / 1000000 ;
         tm.microtm = beginTime % 1000000 ;
         ossTimestampToString( tm, beginTimebuff ) ;

         /// header info
         len = ossSnprintf( _linebuff, sizeof( _linebuff ),
                            "====> Memory( %s ) dump begin( %s ) ====>\n",
                            getName(), beginTimebuff ) ;
         trapFile.Write( _linebuff, len ) ;

         /// guard
         {
            ossScopedLock lock( &_memTrackMutex ) ;

            tm.time = _resetTime / 1000000 ;
            tm.microtm = _resetTime % 1000000 ;
            ossTimestampToString( tm, _timebuff ) ;

            /// dump global stat info
            memStatMap2SortMap( _memStatMap, mapSort ) ;
            memStatTrace( trapFile, mapSort, "Global", &statSize, &statTimes ) ;
            mapSort.clear() ;

            /// dump item info
            memTrace( trapFile, _memTrackMap, mapLocalStat, &trackSize ) ;

            trackNum = ( UINT64 )_memTrackMap.size() ;
         }

         /// dump local stat info
         memStatMap2SortMap( mapLocalStat, mapSort ) ;
         memStatTrace( trapFile, mapSort, "Run Time" ) ;
         mapSort.clear() ;

         endTime = ossGetCurrentMicroseconds() ;

         ossSprintVersion( "V", versionTxt, sizeof( versionTxt ), FALSE ) ;

         /// dump reset time
         len = ossSnprintf( _linebuff, sizeof( _linebuff ),
                            "\n"
                            " Version            : %s\n"
                            " Reset Time         : %s\n"
                            " Dump Time          : %s\n"
                            " Dump Interval      : %lld (secs)\n"
                            " Dump Cost Time     : %lld (secs)\n"
                            " Global Count Inc   : %lld\n"
                            " Global Size Inc    : %lld\n"
                            " Run Time Count Inc : %lld\n"
                            " Run Time Size Inc  : %lld\n",
                            versionTxt,
                            _timebuff,
                            beginTimebuff,
                            ( beginTime - _lastDumpTime ) / 1000000L,
                            ( endTime - beginTime ) / 1000000L,
                            ( statTimes - _lastStatTimes ),
                            ( statSize - _lastStatSize ),
                            ( trackNum - _lastTrackNum ),
                            ( trackSize - _lastTrackSize ) ) ;
         trapFile.Write( _linebuff, len ) ;

         /// result info
         len = ossSnprintf( _linebuff, sizeof( _linebuff ),
                            "\n<==== Memory( %s ) dump end <====\n\n",
                            getName() ) ;
         trapFile.Write( _linebuff, len ) ;

         /// set last info
         _lastDumpTime = beginTime ;
         _lastStatTimes = statTimes ;
         _lastStatSize = statSize ;
         _lastTrackNum = trackNum ;
         _lastTrackSize = trackSize ;
      }

} ;
typedef _ossMemTrackCB ossMemTrackCB ;

static ossMemTrackCB gMemTrackCB( "OSS_MEMORY" ) ;

void ossMemTrack ( void *p )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_OSSMALLOC )
   {
      ossMemInfoAssit assit( p ) ;
      gMemTrackCB.memTrack( p, assit._file, assit._line, assit._size ) ;
   }
}

void ossMemUnTrack ( void *p )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_OSSMALLOC )
   {
      gMemTrackCB.memUnTrack( p ) ;
   }
}

OSS_POOL_MEMCHECK_FUNC ossPoolMemCheckFunc = NULL ;
OSS_POOL_MEMINFO_FUNC  ossPoolMemInfoFunc = NULL ;
OSS_POOL_MEMUSERPTR_FUNC ossPoolMemUserPtrFunc = NULL ;

void ossSetPoolMemcheckFunc( OSS_POOL_MEMCHECK_FUNC pFunc )
{
   ossPoolMemCheckFunc = pFunc ;
}

void ossSetPoolMemInfoFunc( OSS_POOL_MEMINFO_FUNC pFunc )
{
   ossPoolMemInfoFunc = pFunc ;
}

void ossSetPoolMemUserPtrFunc( OSS_POOL_MEMUSERPTR_FUNC pFunc )
{
   ossPoolMemUserPtrFunc = pFunc ;
}

/*
   _ossPoolMemTrackCB define
*/
class _ossPoolMemTrackCB : public _ossMemTrackCB
{
   public:
      _ossPoolMemTrackCB( const CHAR *name ) : _ossMemTrackCB( name ) {}
      virtual ~_ossPoolMemTrackCB() {}

   protected:
      virtual void*     getRealPtr( void *p ) const
      {
         return p ;
      }
      virtual void*     getUserPtr( void *p ) const
      {
         if ( ossPoolMemUserPtrFunc )
         {
            return ossPoolMemUserPtrFunc( p ) ;
         }
         return NULL ;
      }

   protected:
      virtual BOOLEAN   verifyItem( const ossMemTrackItem &item ) const
      {
         void *pRealPtr = getRealPtr( (void*)item._p ) ;

         if ( !pRealPtr )
         {
            return TRUE ;
         }

         if ( ossPoolMemCheckFunc )
         {
            if ( !ossPoolMemCheckFunc( pRealPtr ) )
            {
               return FALSE ;
            }
         }

         if ( ossPoolMemInfoFunc )
         {
            UINT64 realSize = 0 ;
            INT32 pool = 0 ;
            INT32 index = 0 ;

            ossPoolMemInfoFunc( pRealPtr, realSize, pool, index, NULL ) ;

            if ( item._size >= realSize )
            {
#if defined (SDB_ENGINE)
               PD_LOG ( PDSEVERE, "memory real size(%lld) is less than item's "
                        "size(%lld)",
                        realSize, item._size ) ;
#endif
               return FALSE ;
            }
         }

         return TRUE ;
      }

      virtual UINT64    getFillSize( const ossMemTrackItem &item ) const
      {
         void *pRealPtr = getRealPtr( (void*)item._p ) ;

         if ( pRealPtr && ossPoolMemInfoFunc )
         {
            UINT64 realSize = 0 ;
            INT32 pool = 0 ;
            INT32 index = 0 ;

            ossPoolMemInfoFunc( pRealPtr, realSize, pool, index, NULL ) ;

            return realSize - item._size ;
         }
         return 0 ;
      }

      virtual BOOLEAN   isFreeInTC( const ossMemTrackItem &item ) const
      {
         void *pRealPtr = getRealPtr( (void*)item._p ) ;
         BOOLEAN freeInTc = FALSE ;

         if ( pRealPtr && ossPoolMemInfoFunc )
         {
            UINT64 realSize = 0 ;
            INT32 pool = 0 ;
            INT32 index = 0 ;

            ossPoolMemInfoFunc( pRealPtr, realSize, pool, index, &freeInTc ) ;
         }

         return freeInTc ;
      }

      virtual UINT32    traceItemExpand( const ossMemTrackItem &item,
                                         CHAR *pBuff, UINT32 buffSize ) const
      {
         UINT32 len = 0 ;
         BOOLEAN freeInTc = FALSE ;
         void *pRealPtr = getRealPtr( (void*)item._p ) ;
         void *pUser = getUserPtr( (void*)item._p ) ;

         if ( pRealPtr && ossPoolMemInfoFunc )
         {
            UINT64 realSize = 0 ;
            INT32 pool = 0 ;
            INT32 index = 0 ;

            ossPoolMemInfoFunc( pRealPtr, realSize, pool, index, &freeInTc ) ;

            len += ossSnprintf( pBuff, buffSize, " (Pool:%d, Idx:%d)",
                                pool, index ) ;
         }

         if ( pUser )
         {
            /// check free
            if ( freeInTc )
            {
               len += ossSnprintf( pBuff + len, buffSize - len,
                                   " (Free in TC)" ) ;
            }
            /// For bson
            else if ( item._pInfo &&
                      0 == ossStrcmp( item._pInfo, BSON_INFO_STR ) &&
                      *(UINT32*)pUser != 0 &&
                      *(UINT32*)((CHAR*)pUser+sizeof(UINT32)) != 0 )
            {
               try
               {
                  bson::BSONObj obj( (CHAR*)pUser + sizeof( UINT32 ) ) ;
                  /// print bson info
                  len += ossSnprintf( pBuff + len, buffSize - len, " (%d) (%s)",
                                      obj.objsize(),
                                      obj.toString(false, false, true).c_str() ) ;
               }
               catch( std::exception & )
               {
               }
            }
            /// Pr
            else if ( item._pInfo )
            {
               /// add info
               len += ossSnprintf( pBuff + len, buffSize - len, " (%s)",
                                   item._pInfo ) ;
            }
         }
         else if ( item._pInfo )
         {
            /// add info
            len += ossSnprintf( pBuff + len, buffSize - len, " (%s)",
                                item._pInfo ) ;
         }

         return len ;
      }

} ;
typedef _ossPoolMemTrackCB ossPoolMemTrackCB ;

static ossPoolMemTrackCB gPoolMemTrackCB( "POOL_MEMORY" ) ;

void ossPoolMemTrack( void *p, UINT64 userSize, UINT32 file, UINT32 line, const CHAR *pInfo )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_POOLALLOC )
   {
      gPoolMemTrackCB.memTrack( p, file, line, userSize, pInfo ) ;
   }
}

void ossPoolMemUnTrack( void *p )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_POOLALLOC )
   {
      gPoolMemTrackCB.memUnTrack( p ) ;
   }
}

OSS_TC_MEMREALPTR_FUNC ossTCMemRealPtrFunc = NULL ;

void ossSetTCMemRealPtrFunc( OSS_TC_MEMREALPTR_FUNC pFunc )
{
   ossTCMemRealPtrFunc = pFunc ;
}

/*
   _ossThreadMemTrackCB define
*/
class _ossThreadMemTrackCB : public _ossPoolMemTrackCB
{
   public:
      _ossThreadMemTrackCB( const CHAR *name ) : _ossPoolMemTrackCB( name ) {}
      virtual ~_ossThreadMemTrackCB() {}

   protected:
      virtual void*     getRealPtr( void *p ) const
      {
         if ( ossTCMemRealPtrFunc )
         {
            return ossTCMemRealPtrFunc( p ) ;
         }
         return NULL ;
      }
      virtual void*     getUserPtr( void *p ) const
      {
         return p ;
      }
      virtual BOOLEAN   isFreeInTC( const ossMemTrackItem &item ) const
      {
         return FALSE ;
      }
} ;
typedef _ossThreadMemTrackCB ossThreadMemTrackCB ;

ossThreadMemTrackCB gThreadMemTrackCB( "THREAD_CACHE" ) ;

void ossThreadMemTrack( void *p, UINT64 userSize, UINT32 file, UINT32 line, const CHAR *pInfo )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_THREADALLOC )
   {
      gThreadMemTrackCB.memTrack( p, file, line, userSize, pInfo ) ;
   }
}

void ossThreadMemUnTrack( void *p )
{
   if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_THREADALLOC )
   {
      gThreadMemTrackCB.memUnTrack( p ) ;
   }
}

// dump memory info into file
INT32 ossMemTraceAFile ( ossMemTrackCB *pTrackCB,
                         const CHAR *pPath,
                         const CHAR *pFileName )
{
   ossPrimitiveFileOp trapFile ;
   CHAR fileName [ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;

   // build trace file name
   len = ossSnprintf ( fileName, sizeof(fileName), "%s%s%u%s",
                       pPath, OSS_PRIMITIVE_FILE_SEP,
                       ossGetCurrentProcessID(),
                       pFileName ) ;
   if ( len >= sizeof( fileName ) )
   {
      rc = SDB_INVALIDPATH ;
      // file path invalid
      goto error ;
   }

   // open file
   rc = trapFile.Open ( fileName ) ;

   // dump into trap file
   if ( trapFile.isValid () )
   {
      trapFile.seekToEnd () ;
      pTrackCB->memTrace( trapFile ) ;
   }

done :
   trapFile.Close () ;
   return rc ;
error :
   goto done ;
}

// dump mallinfo to file
INT32 ossMemDumpMallinfo( const CHAR *pPath, const CHAR *pFileName )
{
#ifdef _WINDOWS
   return SDB_OK ;
#else
   ossPrimitiveFileOp trapFile ;
   CHAR fileName [ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   ossTimestamp tm ;
   CHAR  linebuff[ OSSMEMTRACEDUMPBUFSZ + 1 ] = { 0 } ;
   CHAR  beginTimebuff[ OSS_MEM_TRACEDUMP_TM_BUF ] = { 0 } ;
   struct mallinfo mi ;

   ossSignalShield shield ;
   shield.doNothing() ;

   // build trace file name
   len = ossSnprintf ( fileName, sizeof(fileName), "%s%s%u%s",
                       pPath, OSS_PRIMITIVE_FILE_SEP,
                       ossGetCurrentProcessID(),
                       pFileName ) ;
   if ( len >= sizeof( fileName ) )
   {
      rc = SDB_INVALIDPATH ;
      // file path invalid
      goto error ;
   }

   beginTime = ossGetCurrentMicroseconds() ;
   tm.time = beginTime / 1000000 ;
   tm.microtm = beginTime % 1000000 ;
   ossTimestampToString( tm, beginTimebuff ) ;

   // open file
   rc = trapFile.Open ( fileName ) ;

   if ( trapFile.isValid() )
   {
      trapFile.seekToEnd () ;

      mi = mallinfo() ;

      /// header info
      len = ossSnprintf( linebuff, sizeof( linebuff ),
                         "====> Memory( SYS mallinfo ) dump begin( %s ) ====>" OSS_NEWLINE,
                         beginTimebuff ) ;
      trapFile.Write( linebuff, len ) ;

      /// mallinfo
      len = ossSnprintf( linebuff, sizeof( linebuff ),
                         "      Total non-mmapped bytes (arena) : %u" OSS_NEWLINE
                         "           # of free chunks (ordblks) : %u" OSS_NEWLINE
                         "    # of free fastbin blocks (smblks) : %u" OSS_NEWLINE
                         "          # of mapped regions (hblks) : %u" OSS_NEWLINE
                         "     Bytes in mapped regions (hblkhd) : %u" OSS_NEWLINE
                         " Max. total allocated space (usmblks) : %u" OSS_NEWLINE
                         "Free bytes held in fastbins (fsmblks) : %u" OSS_NEWLINE
                         "     Total allocated space (uordblks) : %u" OSS_NEWLINE
                         "          Total free space (fordblks) : %u" OSS_NEWLINE
                         "  Topmost releasable block (keepcost) : %u" OSS_NEWLINE,
                         mi.arena,
                         mi.ordblks,
                         mi.smblks,
                         mi.hblks,
                         mi.hblkhd,
                         mi.usmblks,
                         mi.fsmblks,
                         mi.uordblks,
                         mi.fordblks,
                         mi.keepcost ) ;
      trapFile.Write( linebuff, len ) ;

      /// tail info
      endTime = ossGetCurrentMicroseconds() ;
      len = ossSnprintf( linebuff, sizeof( linebuff ),
                         OSS_NEWLINE
                         " Dump Time          : %s" OSS_NEWLINE
                         " Dump Cost Time     : %lld (microsec)" OSS_NEWLINE
                         "<==== Memory( SYS mallinfo ) dump end <====" OSS_NEWLINE OSS_NEWLINE,
                         beginTimebuff,
                         ( endTime - beginTime ) ) ;
      trapFile.Write( linebuff, len ) ;
   }

done :
   trapFile.Close () ;
   return rc ;
error :
   goto done ;
#endif
}

INT32 ossMemDumpMallocInfo( const CHAR *pPath, const CHAR *pFileName )
{
#ifdef _WINDOWS
   return SDB_OK ;
#else
   CHAR fileName [ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   UINT64 beginTime = 0 ;
   ossTimestamp tm ;
   CHAR  beginTimebuff[ OSS_MEM_TRACEDUMP_TM_BUF ] = { 0 } ;
   FILE *fp = NULL ;

   ossSignalShield shield ;
   shield.doNothing() ;

   beginTime = ossGetCurrentMicroseconds() ;
   tm.time = beginTime / 1000000 ;
   tm.microtm = beginTime % 1000000 ;
   ossTimestampToString( tm, beginTimebuff ) ;

   // build trace file name
   len = ossSnprintf ( fileName, sizeof(fileName), "%s%s%u_%s%s",
                       pPath, OSS_PRIMITIVE_FILE_SEP,
                       ossGetCurrentProcessID(),
                       beginTimebuff,
                       pFileName ) ;
   if ( len >= sizeof( fileName ) )
   {
      rc = SDB_INVALIDPATH ;
      // file path invalid
      goto error ;
   }

   // open file
   fp = fopen( fileName, "w+" ) ;
   if ( NULL != fp )
   {
      /// mallinfo
      malloc_info( 0, fp ) ;
   }

done :
   if ( fp )
   {
      fclose( fp ) ;
      fp = NULL ;
   }
   return rc ;
error :
   goto done ;
#endif
}

INT32 ossMemTrace ( const CHAR *pPath )
{
   INT32 rc = SDB_OK ;

   if ( ossMemDebugEnabled )
   {
      /// trace memdump file
      if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_OSSMALLOC )
      {
         rc = ossMemTraceAFile( &gMemTrackCB, pPath, SDB_OSS_MEMDUMPNAME ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      /// trace pooldump file
      if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_POOLALLOC )
      {
         rc = ossMemTraceAFile( &gPoolMemTrackCB, pPath, SDB_POOL_MEMDUMPNAME ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      /// trace threaddump file
      if ( ossMemDebugMask & OSS_MEMDEBUG_MASK_THREADALLOC )
      {
         rc = ossMemTraceAFile( &gThreadMemTrackCB, pPath, SDB_TC_MEMDUMPNAME ) ;
         if ( rc )
         {
            goto error ;
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 ossMemDump( const CHAR *pPath )
{
   /// dump mallinfo
   ossMemDumpMallinfo( pPath, SDB_SYS_MEMDUMPNAME ) ;
   ossMemDumpMallocInfo( pPath, SDB_SYS_MEMINFONAME ) ;

   return SDB_OK ;
}

void ossOnMemConfigChange( BOOLEAN debugEnable,
                           UINT32  memDebugSize,
                           BOOLEAN memDebugVerify,
                           BOOLEAN memDebugDetail,
                           UINT32  memDebugMask )
{
   if ( debugEnable != ossMemDebugEnabled )
   {
      gMemTrackCB.reset() ;
      gPoolMemTrackCB.reset() ;
      gThreadMemTrackCB.reset() ;
   }
   else
   {
      if ( ( ossMemDebugMask & OSS_MEMDEBUG_MASK_OSSMALLOC ) ^
           ( memDebugMask & OSS_MEMDEBUG_MASK_OSSMALLOC ) )
      {
         gMemTrackCB.reset() ;
      }

      if ( ( ossMemDebugMask & OSS_MEMDEBUG_MASK_POOLALLOC ) ^
           ( memDebugMask & OSS_MEMDEBUG_MASK_POOLALLOC) )
      {
         gPoolMemTrackCB.reset() ;
      }

      if ( ( ossMemDebugMask & OSS_MEMDEBUG_MASK_THREADALLOC ) ^
           ( memDebugMask & OSS_MEMDEBUG_MASK_THREADALLOC ) )
      {
         gThreadMemTrackCB.reset() ;
      }
   }

   ossEnableMemDebug( debugEnable, memDebugSize, memDebugVerify,
                      memDebugDetail, memDebugMask ) ;
}

BOOLEAN _ossString2MemDebugMask( const CHAR *pStr, UINT32 &mask )
{
   BOOLEAN result = TRUE ;

   while ( pStr && ' ' == *pStr )
   {
      ++pStr ;
   }

   if ( 0 == ossStrcasecmp( pStr, OSS_MEMDEBUG_MASK_OSSMALLOC_STR ) )
   {
      mask |= OSS_MEMDEBUG_MASK_OSSMALLOC ;
   }
   else if ( 0 == ossStrcasecmp( pStr, OSS_MEMDEBUG_MASK_POOLALLOC_STR ) )
   {
      mask |= OSS_MEMDEBUG_MASK_POOLALLOC ;
   }
   else if ( 0 == ossStrcasecmp( pStr, OSS_MEMDEBUG_MASK_THREADALLOC_STR ) )
   {
      mask |= OSS_MEMDEBUG_MASK_THREADALLOC ;
   }
   else if ( 0 == ossStrcasecmp( pStr, OSS_MEMDEBUG_MASK_ALL_STR ) )
   {
      mask |= OSS_MEMDEBUG_MASK_ALL ;
   }
   else
   {
      result = FALSE ;
   }

   return result ;
}

BOOLEAN ossString2MemDebugMask( const CHAR * pStr, UINT32 &mask )
{
   BOOLEAN result = TRUE ;
   const CHAR *p = pStr ;
   CHAR *p1 = NULL ;

   while( result && p && *p )
   {
      p1 = (CHAR*)ossStrchr( p, '|' ) ;
      if ( p1 )
      {
         *p1 = 0 ;
      }
      result = _ossString2MemDebugMask( p, mask ) ;
      if ( p1 )
      {
         *p1 = '|' ;
      }
      p = p1 ? p1 + 1 : NULL ;
   }

   return result ;
}

#endif

