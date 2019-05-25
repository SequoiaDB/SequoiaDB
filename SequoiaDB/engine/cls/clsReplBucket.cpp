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

   Source File Name = clsReplBucket.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsReplBucket.hpp"
#include "dpsLogRecord.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "../bson/lib/md5.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

using namespace bson ;

namespace engine
{

   #define CLS_BUCKET_LEN_PTR( pData ) \
      (UINT32*)((CHAR*)pData+((dpsLogRecordHeader*)pData)->_length)

   #define CLS_BUCKET_NEXT_PTR( pData ) \
      (ossValuePtr*)((CHAR*)pData+((dpsLogRecordHeader*)pData)->_length+sizeof(UINT32))

   #define CLS_BUCKET_SET_NEXT( pData, pNext ) \
      *CLS_BUCKET_NEXT_PTR( pData ) = (ossValuePtr)pNext

   #define CLS_BUCKET_GET_NEXT( pData ) \
      (CHAR*)(*CLS_BUCKET_NEXT_PTR( pData ))

   #define CLS_BUCKET_GET_LEN( pData ) \
      *CLS_BUCKET_LEN_PTR( pData )

   #define CLS_BUCKET_SET_LEN( pData, len ) \
      *CLS_BUCKET_LEN_PTR( pData ) = len

   #define CLS_BUCKET_NEW_LEN( len ) \
      ( len + sizeof(UINT32) + sizeof(ossValuePtr) )

   #define CLS_REPLSYNC_ONCE_NUM             (5)
#if defined OSS_ARCH_64
   #define CLS_REPL_BUCKET_MAX_MEM_POOL      (5*1024)          // MB
#elif defined OSS_ARCH_32
   #define CLS_REPL_BUCKET_MAX_MEM_POOL      (512)             // MB
#endif

   #define CLS_REPL_MAX_ROLLBACK_TIMES       ( 10 )
   #define CLS_REPL_RETRY_INTERVAL           ( 100 )

   /*
      Tool functions
   */
   const CHAR* clsGetReplBucketStatusDesp( INT32 status )
   {
      switch ( status )
      {
         case CLS_BUCKET_CLOSED :
            return "CLOSED" ;
         case CLS_BUCKET_NORMAL :
            return "NORMAL" ;
         case CLS_BUCKET_WAIT_ROLLBACK :
            return "WAITROLLBACK" ;
         case CLS_BUCKET_ROLLBACKING :
            return "ROLLBACKING" ;
         default :
            break ;
      }
      return "UNKNOWN" ;
   }

   /*
      _clsBucketUnit implement
   */
   _clsBucketUnit::_clsBucketUnit ()
   {
      _pDataHeader   = NULL ;
      _pDataTail     = NULL ;
      _number        = 0 ;
      _attachIn      = FALSE ;
      _inQue         = FALSE ;
   }

   _clsBucketUnit::~_clsBucketUnit ()
   {
      SDB_ASSERT( 0 == _number, "Must be empty" ) ;
   }

   void _clsBucketUnit::push( CHAR *pData, UINT32 len )
   {
      dpsLogRecordHeader *header = ( dpsLogRecordHeader* )pData ;

      SDB_ASSERT( pData && len > sizeof(CHAR*), "pData can't be NULL" ) ;
      SDB_ASSERT( header->_length + sizeof(CHAR*) <= len, "len error" ) ;

      if ( 0 == _number )
      {
         _pDataHeader = pData ;
         _pDataTail   = pData ;
      }
      else
      {
         CLS_BUCKET_SET_NEXT( _pDataTail, pData ) ;
         _pDataTail = pData ;
      }

      CLS_BUCKET_SET_NEXT( pData, NULL ) ;
      CLS_BUCKET_SET_LEN( pData, len ) ;
      ++_number ;
   }

   BOOLEAN _clsBucketUnit::pop( CHAR **ppData, UINT32 &len )
   {
      BOOLEAN ret = FALSE ;
      CHAR *next = NULL ;

      if ( 0 == _number )
      {
         goto done ;
      }

      *ppData = _pDataHeader ;
      len = CLS_BUCKET_GET_LEN( _pDataHeader ) ;
      next = CLS_BUCKET_GET_NEXT( _pDataHeader ) ;
      --_number ;
      ret = TRUE ;

      if ( 0 == _number )
      {
         _pDataHeader = NULL ;
         _pDataTail   = NULL ;
      }
      else
      {
         _pDataHeader = next ;
      }

   done:
      return ret ;
   }

   /*
      _clsBucket implement
   */
   _clsBucket::_clsBucket ()
   :_totalCount( 0 ), _idleUnitCount( 0 ), _allCount( 0 ),
    _curAgentNum( 0 ), _idleAgentNum( 0 )
   {
      _pDPSCB     = NULL ;
      _pMonDBCB   = NULL ;
      _bucketSize = 0 ;
      _bitSize    = 0 ;
      _status     = CLS_BUCKET_CLOSED ;
      _replayer   = NULL ;
      _maxReplSync= 0 ;
      _maxSubmitOffset = 0 ;
      _submitRC   = SDB_OK ;

      _emptyEvent.signal() ;
      _allEmptyEvent.signal() ;
   }

   _clsBucket::~_clsBucket ()
   {
      CHAR *pData = NULL ;
      UINT32 len = 0 ;
      clsBucketUnit *pUnit = NULL ;

      vector< clsBucketUnit* >::iterator it = _dataBucket.begin() ;
      while ( it != _dataBucket.end() )
      {
         pUnit = *it ;

         while ( pUnit->pop( &pData, len ) )
         {
            _memPool.release( pData, len ) ;
         }
         ++it ;
         SDB_OSS_DEL ( pUnit ) ;
      }
      _dataBucket.clear() ;

      vector< ossSpinXLatch* >::iterator itLatch = _latchBucket.begin() ;
      while ( itLatch != _latchBucket.end() )
      {
         SDB_OSS_DEL *itLatch ;
         ++itLatch ;
      }
      _latchBucket.clear() ;

      _memPool.final() ;

      if ( _replayer )
      {
         SDB_OSS_DEL _replayer ;
         _replayer = NULL ;
      }
   }

   BSONObj _clsBucket::toBson ()
   {
      INT32 completeMapSize = 0 ;
      DPS_LSN_OFFSET firstLSNOffset = DPS_INVALID_LSN_OFFSET ;
      BSONObjBuilder builder ;

      builder.append( "Status", clsGetReplBucketStatusDesp( _status ) ) ;
      builder.append( "MaxReplSync", (INT32)_maxReplSync ) ;
      builder.append( "BucketSize", (INT32)_bucketSize ) ;
      builder.append( "IdleUnitCount", (INT32)idleUnitCount() ) ;
      builder.append( "CurAgentNum", (INT32)curAgentNum() ) ;
      builder.append( "IdleAgentNum", (INT32)idleAgentNum() ) ;
      builder.append( "BucketRecordNum", (INT32)bucketSize() ) ;
      builder.append( "AllRecordNum", (INT32)size() ) ;
      builder.append( "ExpectLSN", (INT64)_expectLSN.offset ) ;
      builder.append( "MaxSubmitOffset", (INT64)_maxSubmitOffset ) ;

      _bucketLatch.get() ;
      completeMapSize = (INT32)_completeMap.size() ;
      if ( completeMapSize > 0 )
      {
         firstLSNOffset = _completeMap.begin()->first ;
      }
      _bucketLatch.release() ;

      builder.append( "CompleteMapSize", completeMapSize ) ;
      builder.append( "CompleteFirstLSN", (INT64)firstLSNOffset ) ;

      return builder.obj() ;
   }

   void _clsBucket::enforceMaxReplSync( UINT32 maxReplSync )
   {
      if ( 0 != _maxReplSync )
      {
         waitEmpty() ;
      }
      else
      {
         reset() ;
      }
      _maxReplSync = maxReplSync ;
   }

   INT32 _clsBucket::init()
   {
      INT32 rc = SDB_OK ;
      UINT32 index = 0 ;
      clsBucketUnit *pBucket = NULL ;
      ossSpinXLatch *pLatch  = NULL ;
      _pDPSCB                = pmdGetKRCB()->getDPSCB() ;
      _pMonDBCB              = pmdGetKRCB()->getMonDBCB() ;
      _maxReplSync           = pmdGetOptionCB()->maxReplSync() ;
      _bucketSize            = pmdGetOptionCB()->replBucketSize() ;

      _replayer              = SDB_OSS_NEW clsReplayer() ;
      if ( !_replayer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc memory" ) ;
         goto error ;
      }

      if ( !ossIsPowerOf2( _bucketSize, &_bitSize ) )
      {
         PD_LOG( PDERROR, "Repl bucket size must be the power of 2, value[%u] "
                 "is invalid", _bucketSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _memPool.initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Init mem pool failed, rc: %d", rc ) ;

      while ( index < _bucketSize )
      {
         pBucket = SDB_OSS_NEW clsBucketUnit() ;
         if ( !pBucket )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc memory for bukcet unit" ) ;
            goto error ;
         }
         _dataBucket.push_back( pBucket ) ;
         pBucket = NULL ;

         pLatch = SDB_OSS_NEW ossSpinXLatch() ;
         if ( !pLatch )
         {
            PD_LOG( PDERROR, "Failed to alloc memory for latch" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _latchBucket.push_back( pLatch ) ;
         pLatch = NULL ;

         ++index ;
      }

      _emptyEvent.signal() ;
      _allEmptyEvent.signal() ;
      _status = CLS_BUCKET_NORMAL ;
      _submitRC = SDB_OK ;

   done:
      return rc ;
   error:
      _bitSize = 0 ;
      _bucketSize = 0 ;
      goto done ;
   }

   void _clsBucket::fini ()
   {
      _memPool.final() ;
   }

   void _clsBucket::reset ( BOOLEAN setExpect )
   {
      if ( 0 != size() )
      {
         PD_LOG( PDWARNING, "Bucket[%s] size is not 0",
                 toBson().toString().c_str() ) ;
      }

      _status = CLS_BUCKET_NORMAL ;
      _submitRC = SDB_OK ;
      if ( setExpect )
      {
         _expectLSN = _pDPSCB->expectLsn() ;
      }
      else
      {
         _expectLSN.offset = DPS_INVALID_LSN_OFFSET ;
         _expectLSN.version = DPS_INVALID_LSN_VERSION ;
      }
      _memPool.clear() ;
   }

   void _clsBucket::close ()
   {
      _status = CLS_BUCKET_CLOSED ;
   }

   UINT32 _clsBucket::calcIndex( const CHAR * pData, UINT32 len )
   {
      if ( 0 == _bitSize )
      {
         return 0 ;
      }

      md5::md5digest digest ;
      md5::md5( pData, len, digest ) ;
      UINT32 hashValue = 0 ;
      UINT32 i = 0 ;
      while ( i++ < 4 )
      {
         hashValue |= ( (UINT32)digest[i-1] << ( 32 - 8 * i ) ) ;
      }
      return (UINT32)( hashValue >> ( 32 - _bitSize ) ) ;
   }

   INT32 _clsBucket::pushData( UINT32 index, CHAR * pData, UINT32 len )
   {
      INT32 rc = SDB_OK ;

      if ( DPS_INVALID_LSN_OFFSET == _expectLSN.offset )
      {
         _expectLSN = _pDPSCB->expectLsn() ;
      }

      if ( CLS_BUCKET_NORMAL != _status )
      {
         PD_LOG( PDERROR, "Bucket status is %d[%s], can't push data",
                 _status, clsGetReplBucketStatusDesp( (INT32)_status ) ) ;
         rc = _submitRC ;
         if ( SDB_OK == rc )
         {
            rc = SDB_CLS_REPLAY_LOG_FAILED ;
         }
      }
      else
      {
         rc = _pushData( index, pData, len, TRUE, TRUE ) ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__PUSHDATA, "_clsBucket::_pushData" )
   INT32 _clsBucket::_pushData( UINT32 index, CHAR * pData, UINT32 len,
                                BOOLEAN incAllCount, BOOLEAN newMem )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET__PUSHDATA ) ;
      CHAR *pNewData = NULL ;
      UINT32 newLen = 0 ;
      static const UINT64 sMaxMemSize =
         (UINT64)CLS_REPL_BUCKET_MAX_MEM_POOL << 20 ;

      if ( index >= _bucketSize )
      {
         PD_LOG( PDERROR, "UnitID[%u] is more than bucket size[%u]",
                 index, _bucketSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( newMem )
      {
         while ( _memPool.totalSize() > sMaxMemSize )
         {
            if ( SDB_OK == _emptyEvent.wait( 10 ) )
            {
               break ;
            }
         }
         pNewData = _memPool.alloc( CLS_BUCKET_NEW_LEN( len ), newLen ) ;
         if ( !pNewData )
         {
            PD_LOG( PDERROR, "Failed to alloc memory for log data, len: %d",
                    len ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemcpy( pNewData, pData, len ) ;
      }
      else
      {
         pNewData = pData ;
         newLen   = len ;
      }

      _latchBucket[ index ]->get() ;

      _idleUnitCount.inc() ;

      if ( 0 == curAgentNum() || ( !_dataBucket[ index ]->isAttached() &&
           idleAgentNum() < idleUnitCount() &&
           curAgentNum() < maxReplSync() ) )
      {
         /*PD_LOG( PDEVENT, "CurAgentNum: %u, IdleAgentNum: %u, "
                 "TotalCount: %u, AllCount: %u, IdleUnitCount: %u, "
                 "index: %u, nty que size: %u, index size: %u", curAgentNum(),
                 idleAgentNum(), _totalCount.fetch(), size(), idleUnitCount(),
                 index, _ntyQueue.size(), _dataBucket[ index ]->size() ) ;*/
         INT32 rcTmp = startReplSyncJob( NULL, this, 120*OSS_ONE_SEC ) ;
         if ( SDB_OK == rcTmp )
         {
            incCurAgent() ;
            incIdleAgent() ;
         }
         else if ( SDB_QUIESCED == rcTmp )
         {
            rc = rcTmp ;
            goto error ;
         }
         else if ( _curAgentNum.compare( 0 ) )
         {
            rc = rcTmp ;
            PD_LOG( PDSEVERE, "Start repl-sync session failed, rc: %d."
                    "The node can't to sync data from other node, need to "
                    "restart", rc ) ;
            PMD_RESTART_DB( rc ) ;
            goto error ;
         }
      }

      _dataBucket[ index ]->push( pNewData, newLen ) ;
      _counterLock.lock_w() ;
      _totalCount.inc() ;
      _emptyEvent.reset() ;
      if ( incAllCount )
      {
         _allCount.inc() ;
         _allEmptyEvent.reset() ;
      }

      _counterLock.release_w() ;

      if ( !_dataBucket[ index ]->isAttached() &&
           !_dataBucket[ index ]->isInQue() )
      {
         _ntyQueue.push( index ) ;
         _dataBucket[ index ]->pushToQue() ;
      }
      else
      {
         _idleUnitCount.dec() ;
      }

      _latchBucket[ index ]->release() ;

   done:
      PD_TRACE_EXITRC( SDB__CLSBUCKET__PUSHDATA, rc ) ;
      return rc ;
   error:
      if ( pNewData && newMem )
      {
         _memPool.release( pNewData, newLen ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_POPDATA, "_clsBucket::popData" )
   BOOLEAN _clsBucket::popData( UINT32 index, CHAR ** ppData, UINT32 &len )
   {
      BOOLEAN ret = FALSE ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_POPDATA ) ;

      SDB_ASSERT( index < _bucketSize, "Index must less than bucket size" ) ;
      if ( index >= _bucketSize )
      {
         goto error ;
      }

      _latchBucket[ index ]->get() ;

      SDB_ASSERT ( _dataBucket[ index ]->isAttached(),
                   "Must attach in first" ) ;

      while ( TRUE )
      {
         ret = _dataBucket[ index ]->pop( ppData, len ) ;
         if ( ret )
         {
            if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
            {
               _totalCount.dec () ;
               _allCount.dec() ;
               _memPool.release( *ppData, len ) ;
               continue ;
            }
         }
         break ;
      }

      _latchBucket[ index ]->release() ;

   done:
      PD_TRACE_EXIT( SDB__CLSBUCKET_POPDATA ) ;
      return ret ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_WAITQUEEMPTY, "_clsBucket::waitQueEmpty" )
   INT32 _clsBucket::waitQueEmpty( INT64 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITQUEEMPTY ) ;
      rc = _emptyEvent.wait( millisec ) ;
      PD_TRACE_EXITRC( SDB__CLSBUCKET_WAITQUEEMPTY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_WAITEMPTY, "_clsBucket::waitEmpty" )
   INT32 _clsBucket::waitEmpty( INT64 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITEMPTY ) ;
      rc = _allEmptyEvent.wait( millisec ) ;
      PD_TRACE_EXITRC( SDB__CLSBUCKET_WAITEMPTY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_WAITSUBMIT, "_clsBucket::waitSubmit" )
   INT32 _clsBucket::waitSubmit( INT64 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITSUBMIT ) ;
      rc = _submitEvent.wait( millisec ) ;
      PD_TRACE_EXITRC( SDB__CLSBUCKET_WAITSUBMIT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_WAITANDROLLBACK, "_clsBucket::waitEmptyAndRollback" )
   INT32 _clsBucket::waitEmptyAndRollback( UINT32 *pNum )
   {
      INT32 rc = SDB_OK ;
      UINT32 num = 0 ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITANDROLLBACK ) ;

      _emptyEvent.wait() ;
      if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
      {
         rc = _submitRC ? _submitRC : SDB_CLS_REPLAY_LOG_FAILED ;
         _doRollback( num ) ;
         _emptyEvent.wait() ;
         _status = CLS_BUCKET_NORMAL ;
         _submitRC = SDB_OK ;
      }
      else
      {
         _allEmptyEvent.wait() ;
      }

      if ( pNum )
      {
         *pNum = num ;
      }

      PD_TRACE_EXITRC( SDB__CLSBUCKET_WAITANDROLLBACK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_WAITWITHCHECK, "_clsBucket::waitEmptyWithCheck" )
   INT32 _clsBucket::waitEmptyWithCheck()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITWITHCHECK ) ;

      _emptyEvent.wait() ;

      if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
      {
         rc = _submitRC ? _submitRC : SDB_CLS_REPLAY_LOG_FAILED ;
      }
      else
      {
         _allEmptyEvent.wait() ;
      }

      PD_TRACE_EXITRC( SDB__CLSBUCKET_WAITWITHCHECK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__DOROLLBACK, "_clsBucket::_doRollback" )
   INT32 _clsBucket::_doRollback ( UINT32 &num )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET__DOROLLBACK ) ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;

      map< DPS_LSN_OFFSET, clsCompleteInfo >::reverse_iterator rit ;

      if ( CLS_BUCKET_WAIT_ROLLBACK != getStatus() )
      {
         goto done ;
      }
      _emptyEvent.wait() ;
      _status = CLS_BUCKET_ROLLBACKING ;
      _bucketLatch.get() ;
      rit = _completeMap.rbegin() ;
      while ( rit != _completeMap.rend() )
      {
         ++num ;
         clsCompleteInfo &info = rit->second ;

         if ( pTransCB && pTransCB->isTransOn() &&
              !pTransCB->isNeedSyncTrans() )
         {
            dpsLogRecord record ;
            record.load( info._pData ) ;
            if ( !pTransCB->rollbackTransInfoFromLog( record ) )
            {
               pTransCB->setIsNeedSyncTrans( TRUE ) ;
            }
         }

         rc = _pushData( info._unitID, info._pData, info._len, FALSE, FALSE ) ;
         if ( rc )
         {
            SDB_ASSERT( SDB_OK == rc, "Push complete log to rollback failed" ) ;
            _allCount.dec() ;
            _memPool.release( info._pData, info._len ) ;
         }
         ++rit ;
      }
      _completeMap.clear() ;
      _bucketLatch.release() ;

   done:
      PD_TRACE_EXITRC( SDB__CLSBUCKET__DOROLLBACK, rc ) ;
      return rc ;
   }

   UINT32 _clsBucket::size ()
   {
      return _allCount.fetch() ;
   }

   BOOLEAN _clsBucket::isEmpty ()
   {
      return 0 == size() ? TRUE : FALSE ;
   }

   UINT32 _clsBucket::bucketSize ()
   {
      return _totalCount.fetch() ;
   }

   UINT32 _clsBucket::idleUnitCount ()
   {
      return _idleUnitCount.fetch() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_BEGINUNIT, "_clsBucket::beginUnit" )
   INT32 _clsBucket::beginUnit( pmdEDUCB * cb, UINT32 & unitID,
                                INT64 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_BEGINUNIT ) ;
      INT64 timeout = 0 ;

      while ( TRUE )
      {
         if ( CLS_BUCKET_CLOSED == _status )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( !_ntyQueue.timed_wait_and_pop( unitID, OSS_ONE_SEC ) )
         {
            if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
            {
               timeout = 0 ;
            }
            else
            {
               timeout += OSS_ONE_SEC ;
            }

            if ( millisec > 0 && timeout >= millisec )
            {
               rc = SDB_TIMEOUT ;
               goto error ;
            }
            continue ;
         }

         _latchBucket[ unitID ]->get() ;
         if ( _dataBucket[ unitID ]->isAttached() )
         {
            _latchBucket[ unitID ]->release() ;
            continue ;
         }
         _idleUnitCount.dec() ;
         decIdelAgent() ;
         _dataBucket[ unitID ]->attach() ;

         _latchBucket[ unitID ]->release() ;

         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSBUCKET_BEGINUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_ENDUNIT, "_clsBucket::endUnit" )
   INT32 _clsBucket::endUnit( pmdEDUCB * cb, UINT32 unitID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_ENDUNIT ) ;

      SDB_ASSERT( unitID < _bucketSize, "unitID must less bucket size" ) ;
      if ( unitID >= _bucketSize )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      incIdleAgent() ;
      _latchBucket[ unitID ]->get() ;

      SDB_ASSERT( _dataBucket[ unitID ]->isAttached(), "Must attach in unit" ) ;

      _dataBucket[ unitID ]->dettach() ;

      if ( !_dataBucket[ unitID ]->isEmpty() )
      {
         _ntyQueue.push( unitID ) ;
         _dataBucket[ unitID ]->pushToQue() ;

         _idleUnitCount.inc() ;
      }
      _latchBucket[ unitID ]->release() ;

      _counterLock.lock_r() ;
      if ( _totalCount.compare( 0 ) )
      {
         _emptyEvent.signalAll() ;
      }
      if ( _allCount.compare( 0 ) )
      {
         _allEmptyEvent.signalAll() ;
      }
      _counterLock.release_r() ;

   done:
      PD_TRACE_EXITRC( SDB__CLSBUCKET_ENDUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsBucket::_incCount( const CHAR * pData )
   {
      dpsLogRecordHeader *pHeader = (dpsLogRecordHeader*)pData ;

      switch ( pHeader->_type )
      {
         case LOG_TYPE_DATA_INSERT :
            _pMonDBCB->monOperationCountInc ( MON_INSERT_REPL ) ;
            break ;
         case LOG_TYPE_DATA_UPDATE :
            _pMonDBCB->monOperationCountInc ( MON_UPDATE_REPL ) ;
            break ;
         case LOG_TYPE_DATA_DELETE :
            _pMonDBCB->monOperationCountInc ( MON_DELETE_REPL ) ;
            break ;
         default :
            break ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_SUBMITDATA, "_clsBucket::submitData" )
   INT32 _clsBucket::submitData( UINT32 unitID, _pmdEDUCB *cb,
                                 CHAR * pData, UINT32 len,
                                 CLS_SUBMIT_RESULT &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_SUBMITDATA ) ;
      dpsLogRecordHeader *pHeader = (dpsLogRecordHeader*)pData ;

      if ( CLS_BUCKET_ROLLBACKING != _status )
      {
         rc = _replayer->replay( pHeader, cb, FALSE ) ;
         if ( rc )
         {
            SDB_ASSERT( SDB_OOM == rc || SDB_NOSPC == rc,
                        "Unexpect error occured" ) ;
            if ( CLS_BUCKET_WAIT_ROLLBACK != _status )
            {
               _status = CLS_BUCKET_WAIT_ROLLBACK ;
               _submitRC = rc ;
            }
            _allCount.dec() ;
            _memPool.release( pData, len ) ;
         }
         else
         {
            _submitResult( pHeader->_lsn, pHeader->_version, pHeader->_length,
                           pData, len, unitID, result ) ;
         }
      }
      else
      {
         UINT32 retryTimes = 0 ;
         while( TRUE )
         {
            rc = _replayer->rollback( pHeader, cb ) ;
            if ( ( SDB_OOM == rc || SDB_NOSPC == rc ) &&
                 ++retryTimes < CLS_REPL_MAX_ROLLBACK_TIMES )
            {
               ossSleep( CLS_REPL_RETRY_INTERVAL ) ;
               continue ;
            }
            SDB_ASSERT( SDB_OK == rc, "Rollback dps log failed" ) ;
            break ;
         }
         _allCount.dec() ;
         _memPool.release( pData, len ) ;
      }

      _totalCount.dec () ;

      PD_TRACE_EXITRC( SDB__CLSBUCKET_SUBMITDATA, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__SUBMITRESULT, "_clsBucket::_submitResult" )
   void _clsBucket::_submitResult( DPS_LSN_OFFSET offset, DPS_LSN_VER version,
                                   UINT32 lsnLen, CHAR *pData, UINT32 len,
                                   UINT32 unitID, CLS_SUBMIT_RESULT &result )
   {
      PD_TRACE_ENTRY( SDB__CLSBUCKET__SUBMITRESULT ) ;
      clsCompleteInfo info ;
      info._len      = len ;
      info._pData    = pData ;
      info._unitID   = unitID ;

      BOOLEAN releaseMem = FALSE ;
      _bucketLatch.get() ;

      _incCount( pData ) ;

      if ( _expectLSN.compareOffset( offset ) >= 0 )
      {
         SDB_ASSERT( 0 == _expectLSN.compareOffset( offset ),
                     "expect lsn is error" ) ;

         if ( 0 == _expectLSN.compareOffset( offset ) )
         {
            _expectLSN.version = version ;
            _expectLSN.offset += lsnLen ;
         }
         result = CLS_SUBMIT_EQ_EXPECT ;
         releaseMem = TRUE ;

         map< UINT64, clsCompleteInfo >::iterator it = _completeMap.begin() ;
         while ( it != _completeMap.end() )
         {
            clsCompleteInfo &tmpInfo = it->second ;
            if ( _expectLSN.compareOffset( it->first ) >= 0 )
            {
               if ( 0 == _expectLSN.compareOffset( it->first ) )
               {
                  _expectLSN.version =
                     ((dpsLogRecordHeader*)tmpInfo._pData)->_version ;
                  _expectLSN.offset +=
                     ((dpsLogRecordHeader*)tmpInfo._pData)->_length ;
               }
               _memPool.release( tmpInfo._pData, tmpInfo._len ) ;
               _completeMap.erase( it++ ) ;
               _allCount.dec() ;
               continue ;
            }
            break ;
         }
         _submitEvent.signal() ;
         goto done ;
      }

      if ( offset > _maxSubmitOffset )
      {
         result = CLS_SUBMIT_GT_MAX ;
         _maxSubmitOffset = offset ;
      }
      else
      {
         result = CLS_SUBMIT_LT_MAX ;
      }

      if ( !(_completeMap.insert( std::make_pair( offset, info ) ) ).second )
      {
         SDB_ASSERT( FALSE, "System error, dps log exist" ) ;
         releaseMem = TRUE ;
         goto done ;
      }

   done:
      _bucketLatch.release() ;
      if ( releaseMem )
      {
         _memPool.release( pData, len ) ;
         _allCount.dec() ;
      }
      PD_TRACE_EXIT( SDB__CLSBUCKET__SUBMITRESULT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_FORCECOMPLETE, "_clsBucket::forceCompleteAll" )
   INT32 _clsBucket::forceCompleteAll ()
   {
      PD_TRACE_ENTRY( SDB__CLSBUCKET_FORCECOMPLETE ) ;
      map< UINT64, clsCompleteInfo >::iterator it ;

      _bucketLatch.get() ;

      if ( !_curAgentNum.compare( 0 ) )
      {
         goto done ;
      }

      PD_LOG( PDWARNING, "Repl bucket begin to force complete, expect lsn: "
              "[%d,%lld], Status:%d[%s]",
              _expectLSN.version, _expectLSN.offset,
              _status, clsGetReplBucketStatusDesp( (INT32)_status ) ) ;

      it = _completeMap.begin() ;
      while ( it != _completeMap.end() )
      {
         clsCompleteInfo &tmpInfo = it->second ;
         dpsLogRecordHeader *pHeader = (dpsLogRecordHeader*)( tmpInfo._pData) ;

         if ( CLS_BUCKET_NORMAL == _status )
         {
            _expectLSN.offset = pHeader->_lsn + pHeader->_length ;
            _expectLSN.version = pHeader->_version ;
         }

         PD_LOG( PDWARNING, "Repl bucket forced complete lsn: [%d,%lld], "
                 "len: %d", pHeader->_version, pHeader->_lsn,
                 pHeader->_length ) ;

         _memPool.release( tmpInfo._pData, tmpInfo._len ) ;
         _allCount.dec() ;
         ++it ;
      }
      _completeMap.clear() ;
      _submitEvent.signal() ;

      _allEmptyEvent.signalAll() ;
      _emptyEvent.signalAll() ;

      PD_LOG( PDWARNING, "Repl bucket end to force complete, expect lsn: "
              "[%d,%lld], Status:%d[%s]",
              _expectLSN.version, _expectLSN.offset,
              _status, clsGetReplBucketStatusDesp( (INT32)_status ) ) ;

   done:
      _bucketLatch.release() ;
      PD_TRACE_EXIT( SDB__CLSBUCKET_FORCECOMPLETE ) ;
      return SDB_OK ;
   }

   /*
      _clsBucketSyncJob implement
   */
   _clsBucketSyncJob::_clsBucketSyncJob ( clsBucket *pBucket, INT32 timeout )
   {
      _pBucket = pBucket ;
      _timeout = timeout ;
   }

   _clsBucketSyncJob::~_clsBucketSyncJob ()
   {
      _pBucket = NULL ;
   }

   RTN_JOB_TYPE _clsBucketSyncJob::type () const
   {
      return RTN_JOB_REPLSYNC ;
   }

   const CHAR *_clsBucketSyncJob::name () const
   {
      return "Job[ReplSync]" ;
   }

   BOOLEAN _clsBucketSyncJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKETSYNCJOB_DOIT, "_clsBucketSyncJob::doit" )
   INT32 _clsBucketSyncJob::doit ()
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSBUCKETSYNCJOB_DOIT ) ;
      pmdKRCB *krcb           = pmdGetKRCB() ;
      pmdEDUMgr *eduMgr       = krcb->getEDUMgr() ;

      UINT32 unitID           = 0 ;
      CHAR *pData             = NULL ;
      UINT32 len              = 0 ;
      UINT32 number           = 0 ;
      CLS_SUBMIT_RESULT res   = CLS_SUBMIT_EQ_EXPECT ;

      while ( TRUE )
      {
         eduMgr->waitEDU( eduCB() ) ;

         rc = _pBucket->beginUnit( eduCB(), unitID, _timeout ) ;
         if ( rc )
         {
            break ;
         }
         eduMgr->activateEDU( eduCB() ) ;

         number = 0 ;
         while ( _pBucket->popData( unitID, &pData, len ) )
         {
            ++number ;
            eduCB()->incEventCount() ;

            _pBucket->submitData( unitID, eduCB(), pData, len, res ) ;

            if ( _pBucket->idleUnitCount() > 1 && CLS_SUBMIT_EQ_EXPECT != res )
            {
               if ( CLS_SUBMIT_GT_MAX == res || number > CLS_REPLSYNC_ONCE_NUM )
               {
                  break ;
               }
            }
         }

         _pBucket->endUnit( eduCB(), unitID ) ;
      }

      _pBucket->decIdelAgent() ;
      _pBucket->decCurAgent() ;

      if ( _pBucket->curAgentNum() == 0 )
      {
         if ( _pBucket->idleUnitCount() > 0 && PMD_IS_DB_UP() )
         {
            rc = startReplSyncJob( NULL, _pBucket, 60*OSS_ONE_SEC ) ;
            if ( SDB_OK == rc )
            {
               _pBucket->incCurAgent() ;
               _pBucket->incIdleAgent() ;
            }
         }
         else
         {
            if ( 0 != _pBucket->size() )
            {
               PD_LOG( PDERROR, "Repl bucket info has error: %s",
                       _pBucket->toBson().toString().c_str() ) ;

               _pBucket->forceCompleteAll() ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__CLSBUCKETSYNCJOB_DOIT ) ;
      return SDB_OK ;
   }

   /*
      Functions
   */
   INT32 startReplSyncJob( EDUID * pEDUID, clsBucket * pBucket, INT32 timeout )
   {
      INT32 rc                = SDB_OK ;
      clsBucketSyncJob * pJob = NULL ;

      pJob = SDB_OSS_NEW clsBucketSyncJob ( pBucket, timeout ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

