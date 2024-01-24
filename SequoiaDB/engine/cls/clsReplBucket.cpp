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
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "utilBsonHash.hpp"

using namespace bson ;

namespace engine
{

   // data
   // length
   // parallel type
   // CL name hash
   // CL unique ID
   // wait LSN
   // next pointer
   #define CLS_BUCKET_NEW_LEN( len ) \
      ( len + sizeof( UINT32 ) + \
              sizeof( CLS_PARALLA_TYPE ) + \
              sizeof( UINT32 ) + \
              sizeof( utilCLUniqueID ) + \
              sizeof( DPS_LSN_OFFSET ) + \
              sizeof( ossValuePtr ) )

   #define CLS_BUCKET_ORG_LEN( len ) \
      ( len - sizeof( UINT32 ) - \
              sizeof( CLS_PARALLA_TYPE ) - \
              sizeof( UINT32 ) - \
              sizeof( utilCLUniqueID ) - \
              sizeof( DPS_LSN_OFFSET ) - \
              sizeof( ossValuePtr ) )

   #define CLS_BUCKET_LEN_PTR( pData ) \
      (UINT32 *)( (CHAR *)pData + ( (dpsLogRecordHeader *)pData )->_length )

   #define CLS_BUCKET_PARALLATYPE_PTR( pData ) \
      (CLS_PARALLA_TYPE *)( (CHAR *)( CLS_BUCKET_LEN_PTR( pData ) ) + \
                            sizeof( UINT32 ) )

   #define CLS_BUCKET_CLHASH_PTR( pData ) \
      (UINT32 *)( (CHAR *)( CLS_BUCKET_PARALLATYPE_PTR( pData ) ) + \
                  sizeof( CLS_PARALLA_TYPE ) )

   #define CLS_BUCKET_CLUNIQUEID_PTR( pData ) \
      (utilCLUniqueID *)( (CHAR *)( CLS_BUCKET_CLHASH_PTR( pData ) ) + \
                          sizeof( UINT32 ) )

   #define CLS_BUCKET_WAITLSN_PTR( pData ) \
      (DPS_LSN_OFFSET *)( (CHAR *)( CLS_BUCKET_CLUNIQUEID_PTR( pData ) ) + \
                          sizeof( utilCLUniqueID ) )

   #define CLS_BUCKET_NEXT_PTR( pData ) \
      (ossValuePtr *)( (CHAR *)( CLS_BUCKET_WAITLSN_PTR( pData ) ) + \
                       sizeof( DPS_LSN_OFFSET ) )

   #define CLS_BUCKET_GET_LEN( pData ) \
      *CLS_BUCKET_LEN_PTR( pData )

   #define CLS_BUCKET_SET_LEN( pData, len ) \
      *CLS_BUCKET_LEN_PTR( pData ) = len

   #define CLS_BUCKET_GET_PARALLATYPE( pData ) \
      *CLS_BUCKET_PARALLATYPE_PTR( pData )

   #define CLS_BUCKET_SET_PARALLATYPE( pData, type ) \
      *CLS_BUCKET_PARALLATYPE_PTR( pData ) = type

   #define CLS_BUCKET_GET_CLHASH( pData ) \
      *CLS_BUCKET_CLHASH_PTR( pData )

   #define CLS_BUCKET_SET_CLHASH( pData, hash ) \
      *CLS_BUCKET_CLHASH_PTR( pData ) = hash

   #define CLS_BUCKET_GET_CLUNIQUEID( pData ) \
      *CLS_BUCKET_CLUNIQUEID_PTR( pData )

   #define CLS_BUCKET_SET_CLUNIQUEID( pData, clUniqueID ) \
      *CLS_BUCKET_CLUNIQUEID_PTR( pData ) = clUniqueID

   #define CLS_BUCKET_GET_WAITLSN( pData ) \
      *CLS_BUCKET_WAITLSN_PTR( pData )

   #define CLS_BUCKET_SET_WAITLSN( pData, lsn ) \
      *CLS_BUCKET_WAITLSN_PTR( pData ) = lsn ;

   #define CLS_BUCKET_SET_NEXT( pData, pNext ) \
      *CLS_BUCKET_NEXT_PTR( pData ) = (ossValuePtr)pNext

   #define CLS_BUCKET_GET_NEXT( pData ) \
      (CHAR*)(*CLS_BUCKET_NEXT_PTR( pData ))

   #define CLS_REPLSYNC_ONCE_NUM             (5)
#if defined OSS_ARCH_64
   #define CLS_REPL_BUCKET_MAX_MEM_POOL      (2*1024)          // MB
#elif defined OSS_ARCH_32
   #define CLS_REPL_BUCKET_MAX_MEM_POOL      (512)             // MB
#endif

   #define CLS_REPL_MAX_ROLLBACK_TIMES       ( 10 )
   #define CLS_REPL_RETRY_INTERVAL           ( 100 )

   // LSN array for hash keys of the last replayed unique indexes will be
   // combined with collection hash value and re-hash to 0 - 65535
   #define CLS_UNQIDX_HASH_SIZE  ( 65536 )
   #define CLS_UNQIDX_HASH_MOD   ( (UINT16)( 0xFFFF ) )

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

   void _clsBucketUnit::push( CHAR *pData )
   {
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

   BOOLEAN _clsBucketUnit::pop()
   {
      BOOLEAN ret = FALSE ;
      CHAR *next = NULL ;

      if ( 0 == _number )
      {
         goto done ;
      }

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

   BOOLEAN _clsBucketUnit::front( CHAR **ppData, UINT32 &len )
   {
      BOOLEAN ret = FALSE ;

      if ( 0 == _number )
      {
         goto done ;
      }

      *ppData = _pDataHeader ;
      len = CLS_BUCKET_GET_LEN( _pDataHeader ) ;

      ret = TRUE ;

   done:
      return ret ;
   }

   /*
      _clsBucket implement
   */
   _clsBucket::_clsBucket ()
   :_totalCount( 0 ), _idleUnitCount( 0 ), _allCount( 0 ),
    _curAgentNum( 0 ), _idleAgentNum( 0 ), _waitAgentNum( 0 ),
    _lastUnqIdxSize( 0 ),
    _lastNewUnqIdxLSN( NULL ),
    _lastNewUnqIdxBkt( NULL ),
    _lastOldUnqIdxLSN( NULL ),
    _lastOldUnqIdxBkt( NULL ),
    _unqIdxBitmap( 0 ),
    _lastExpectLSN( DPS_INVALID_LSN_OFFSET )
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
      _ntyQueue   = NULL ;

      _emptyEvent.signal() ;
      _allEmptyEvent.signal() ;

      _pendingCLUniqueID = UTIL_UNIQUEID_NULL ;
      _lastIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
      _lastNIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
      _replayEventHandler = NULL ;
   }

   _clsBucket::~_clsBucket ()
   {
      CHAR *pData = NULL ;
      UINT32 len = 0 ;
      clsBucketUnit *pUnit = NULL ;

      // release all memory and data bucket
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

      // release all latch lock
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

      // scoped lock to avoid exception
      {
         ossScopedLock lock( &_bucketLatch, SHARED ) ;
         completeMapSize = (INT32)_completeMap.size() ;
         if ( completeMapSize > 0 )
         {
            firstLSNOffset = _completeMap.begin()->first ;
         }
      }

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
      if ( 0 == _maxReplSync || 0 == maxReplSync )
      {
         reset() ;
      }
      _maxReplSync = maxReplSync ;
   }

   INT32 _clsBucket::init( clsReplayEventHandler *handler )
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

      _replayEventHandler = handler ;

      if ( !ossIsPowerOf2( _bucketSize, &_bitSize ) )
      {
         PD_LOG( PDERROR, "Repl bucket size must be the power of 2, value[%u] "
                 "is invalid", _bucketSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // init mem
      rc = _memPool.initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Init mem pool failed, rc: %d", rc ) ;

      // create data bucket and latch
      while ( index < _bucketSize )
      {
         // memory will be freed in destructor
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

      PD_CHECK( _queueBuffer.initBuffer( _bucketSize ),
                SDB_OOM, error, PDERROR, "Failed to allocate queue buffer "
                "for notify queue [%u]", _bucketSize ) ;
      _ntyQueue =
            SDB_OSS_NEW CLS_BUCKET_QUEUE(
                  CLS_BUCKET_QUEUE_CONTAINER( &_queueBuffer ) ) ;
      PD_CHECK( NULL != _ntyQueue, SDB_OOM, error, PDERROR,
                "Failed to allocate notify queue" ) ;

      _lastNewUnqIdxLSN =
            (DPS_LSN_OFFSET *)( SDB_OSS_MALLOC( sizeof( DPS_LSN_OFFSET ) *
                                                CLS_UNQIDX_HASH_SIZE ) ) ;
      PD_CHECK( NULL != _lastNewUnqIdxLSN, SDB_OOM, error, PDERROR,
                "Failed allocate array for last LSN for new unique index "
                "hash values" ) ;
      _lastNewUnqIdxBkt =
            (INT16 *)( SDB_OSS_MALLOC( sizeof( INT16 ) *
                                       CLS_UNQIDX_HASH_SIZE ) ) ;
      PD_CHECK( NULL != _lastNewUnqIdxBkt, SDB_OOM, error, PDERROR,
                "Failed allocate array for last bucket for new unique index "
                "hash values" ) ;
      _lastOldUnqIdxLSN =
            (DPS_LSN_OFFSET *)( SDB_OSS_MALLOC( sizeof( DPS_LSN_OFFSET ) *
                                                CLS_UNQIDX_HASH_SIZE ) ) ;
      PD_CHECK( NULL != _lastOldUnqIdxLSN, SDB_OOM, error, PDERROR,
                "Failed allocate array for last LSN for old unique index "
                "hash values" ) ;
      _lastOldUnqIdxBkt =
            (INT16 *)( SDB_OSS_MALLOC( sizeof( INT16 ) *
                                       CLS_UNQIDX_HASH_SIZE ) ) ;
      PD_CHECK( NULL != _lastOldUnqIdxBkt, SDB_OOM, error, PDERROR,
                "Failed allocate array for last bucket for old unique index "
                "hash values" ) ;

      _unqIdxBitmap.resize( CLS_UNQIDX_HASH_SIZE ) ;
      PD_CHECK( CLS_UNQIDX_HASH_SIZE == _unqIdxBitmap.getSize(),
                SDB_OOM, error, PDERROR, "Failed to allocate bitmap for "
                "unique index hash values" ) ;

      _lastUnqIdxSize = CLS_UNQIDX_HASH_SIZE ;

      _waitAgentNum.init( 0 ) ;
      _emptyEvent.signal() ;
      _allEmptyEvent.signal() ;
      _status = CLS_BUCKET_NORMAL ;
      _submitRC = SDB_OK ;

      _pendingCLUniqueID = UTIL_UNIQUEID_NULL ;

      initUnqIdxLSN() ;

   done:
      return rc ;
   error:
      _bitSize = 0 ;
      _bucketSize = 0 ;
      goto done ;
   }

   void _clsBucket::fini ()
   {
      SAFE_OSS_FREE( _lastNewUnqIdxLSN ) ;
      SAFE_OSS_FREE( _lastNewUnqIdxBkt ) ;
      SAFE_OSS_FREE( _lastOldUnqIdxLSN ) ;
      SAFE_OSS_FREE( _lastOldUnqIdxBkt ) ;
      _lastUnqIdxSize = 0 ;

      if ( NULL != _ntyQueue )
      {
         SAFE_OSS_DELETE( _ntyQueue ) ;
      }
      _queueBuffer.finiBuffer() ;

      _memPool.final() ;

      _replayEventHandler = NULL ;
   }

   void _clsBucket::reset ( BOOLEAN setExpect )
   {
      if ( 0 != size() )
      {
         PD_LOG( PDWARNING, "Bucket[%s] size is not 0",
                 toBson().toString().c_str() ) ;
      }

      _waitAgentNum.init( 0 ) ;
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

      _pendingCLUniqueID = UTIL_UNIQUEID_NULL ;

      resetUnqIdxLSN( FALSE ) ;
   }

   void _clsBucket::close ()
   {
      _status = CLS_BUCKET_CLOSED ;
   }

   UINT32 _clsBucket::calcIndex( UINT32 hashValue )
   {
      return (UINT32)( hashValue >> ( 32 - _bitSize ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_PUSHWAIT, "_clsBucket::pushWait" )
   INT32 _clsBucket::pushWait( UINT32 index, DPS_LSN_OFFSET waitLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET_PUSHWAIT ) ;

      dpsLogRecordHeader header ;
      header._type = LOG_TYPE_DUMMY ;
      header._length = sizeof( dpsLogRecordHeader ) ;

      clsReplayInfo info ;
      info._pData = (CHAR *)( &header ) ;
      info._len = CLS_BUCKET_NEW_LEN( header._length ) ;
      info._waitLSN = waitLSN ;

      rc = _checkAndPushData( index, info ) ;

      PD_TRACE_EXITRC( SDB__CLSBUCKET_PUSHWAIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_PUSHDATA, "_clsBucket::pushData" )
   INT32 _clsBucket::pushData( UINT32 index, CHAR *pData, UINT32 len,
                               CLS_PARALLA_TYPE parallaType,
                               UINT32 clHash, utilCLUniqueID clUniqueID,
                               DPS_LSN_OFFSET waitLSN )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET_PUSHDATA ) ;

      clsReplayInfo info ;
      info._pData = pData ;
      info._len = CLS_BUCKET_NEW_LEN( len ) ;
      info._clHash = clHash ;
      info._clUniqueID = clUniqueID ;
      info._parallaType = parallaType ;
      info._waitLSN = waitLSN ;

      rc = _checkAndPushData( index, info ) ;

      PD_TRACE_EXITRC( SDB__CLSBUCKET_PUSHDATA, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__CHECKANDPUSHDATA, "_clsBucket::_checkAndPushData" )
   INT32 _clsBucket::_checkAndPushData( UINT32 index, clsReplayInfo &info )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET__CHECKANDPUSHDATA ) ;

      // only use offset
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
         rc = _pushData( index, info, TRUE, TRUE ) ;
      }

      PD_TRACE_EXITRC( SDB__CLSBUCKET__CHECKANDPUSHDATA, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__PUSHDATA, "_clsBucket::_pushData" )
   INT32 _clsBucket::_pushData( UINT32 index, clsReplayInfo &info,
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
         // prepare new data
         pNewData = _memPool.alloc( info._len, newLen ) ;
         if ( !pNewData )
         {
            PD_LOG( PDERROR, "Failed to alloc memory for log data, len: %d",
                    info._len ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         // copy data
         ossMemcpy( pNewData, info._pData, CLS_BUCKET_ORG_LEN( info._len ) ) ;

         CLS_BUCKET_SET_LEN( pNewData, newLen ) ;
         CLS_BUCKET_SET_PARALLATYPE( pNewData, info._parallaType ) ;
         CLS_BUCKET_SET_CLHASH( pNewData, info._clHash ) ;
         CLS_BUCKET_SET_CLUNIQUEID( pNewData, info._clUniqueID ) ;
         CLS_BUCKET_SET_WAITLSN( pNewData, info._waitLSN ) ;
      }
      else
      {
         pNewData = info._pData ;
         newLen   = info._len ;
      }

      // scoped lock to avoid exception
      {
         ossScopedLock bucketLock( _latchBucket[ index ] ) ;

         _idleUnitCount.inc() ;

         // start replsync job
         if ( 0 == curAgentNum() || ( !_dataBucket[ index ]->isAttached() &&
              idleAgentNum() < idleUnitCount() &&
              curAgentNum() < maxReplSync() ) )
         {
            /*PD_LOG( PDEVENT, "CurAgentNum: %u, IdleAgentNum: %u, "
                    "TotalCount: %u, AllCount: %u, IdleUnitCount: %u, "
                    "index: %u, nty que size: %u, index size: %u",
                    curAgentNum(), idleAgentNum(), _totalCount.fetch(),
                    size(), idleUnitCount(), index, _ntyQueue.size(),
                    _dataBucket[ index ]->size() ) ;*/
            INT32 rcTmp = startReplSyncJob( NULL, this, 120*OSS_ONE_SEC ) ;
            if ( SDB_OK == rcTmp )
            {
               incCurAgent() ;
               incIdleAgent() ;
            }
            else if ( SDB_QUIESCED == rcTmp )
            {
               /// DB Shutdown
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

         _dataBucket[ index ]->push( pNewData ) ;

         {
            ossScopedRWLock counterLock( &_counterLock, EXCLUSIVE ) ;
            _totalCount.inc() ;
            _emptyEvent.reset() ;
            if ( incAllCount )
            {
               _allCount.inc() ;
               _allEmptyEvent.reset() ;
            }
         }

         // no cb attach in and no push to que, need to push to nty quque
         if ( !_dataBucket[ index ]->isAttached() &&
              !_dataBucket[ index ]->isInQue() )
         {
            _ntyQueue->push( index ) ;
            _dataBucket[ index ]->pushToQue() ;
         }
         else
         {
            _idleUnitCount.dec() ;
         }
      }

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
   BOOLEAN _clsBucket::popData( UINT32 index, clsReplayInfo &info )
   {
      BOOLEAN ret = FALSE ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET_POPDATA ) ;

      CHAR *pData = NULL ;
      UINT32 len = 0 ;
      DPS_LSN_OFFSET waitOffset = DPS_INVALID_LSN_OFFSET ;

      SDB_ASSERT( index < _bucketSize, "Index must less than bucket size" ) ;
      if ( index >= _bucketSize )
      {
         goto error ;
      }

      // scoped lock to avoid exception
      {
         ossScopedLock bucketLock( _latchBucket[ index ] ) ;

         // must attach in first
         SDB_ASSERT ( _dataBucket[ index ]->isAttached(),
                      "Must attach in first" ) ;

         while ( TRUE )
         {
            ret = _dataBucket[ index ]->front( &pData, len ) ;
            if ( ret )
            {
               if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
               {
                  // error happened, ignore remain data
                  _dataBucket[ index ]->pop() ;
                  _totalCount.dec () ;
                  _allCount.dec() ;
                  _memPool.release( pData, len ) ;
                  continue ;
               }

               waitOffset = CLS_BUCKET_GET_WAITLSN( pData ) ;
               if ( DPS_INVALID_LSN_OFFSET != waitOffset &&
                    CLS_BUCKET_NORMAL == _status )
               {
                  // need wait completed for given offset
                  if ( _checkCompleted( waitOffset ) )
                  {
                     // wait done, pop from bucket and release
                     PD_LOG( PDDEBUG, "Bucket [%u]: wait for LSN [%llu] done",
                             index, waitOffset ) ;

                     _dataBucket[ index ]->pop() ;

                     if ( LOG_TYPE_DUMMY ==
                           ( (dpsLogRecordHeader *)pData )->_type )
                     {
                        // dummy record, no need to process further
                        // skip it
                        _totalCount.dec() ;
                        _allCount.dec() ;
                        _memPool.release( pData, len ) ;
                        continue ;
                     }

                     // otherwise, record should be replayed
                     // need further process
                  }
                  else
                  {
                     // wait failed, should not pop this record
                     ret = FALSE ;
                  }
               }
               else
               {
                  // not wait operator, pop here
                  _dataBucket[ index ]->pop() ;
               }

               if ( ret )
               {
                  // record popped for further process
                  info._pData = pData ;
                  info._len = len ;
                  info._unitID = index ;
                  info._parallaType = CLS_BUCKET_GET_PARALLATYPE( pData ) ;
                  info._clHash = CLS_BUCKET_GET_CLHASH( pData ) ;
                  info._clUniqueID = CLS_BUCKET_GET_CLUNIQUEID( pData ) ;
               }
            }
            break ;
         }
      }

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
   INT32 _clsBucket::waitEmptyAndRollback( UINT32 *pNum,
                                           DPS_LSN *pCompleteLsn )
   {
      INT32 rc = SDB_OK ;
      UINT32 num = 0 ;
      PD_TRACE_ENTRY( SDB__CLSBUCKET_WAITANDROLLBACK ) ;

      _emptyEvent.wait() ;

      if ( CLS_BUCKET_WAIT_ROLLBACK == _status )
      {
         rc = _submitRC ? _submitRC : SDB_CLS_REPLAY_LOG_FAILED ;

         _doRollback( num ) ;

         // wait
         _emptyEvent.wait() ;

         // only set pending for duplicated key
         if ( SDB_IXM_DUP_KEY == rc )
         {
            PD_LOG( PDEVENT, "Failed to replay in parallel, "
                    "need resolve pending, expect LSN [%llu]",
                    _expectLSN.offset ) ;
            if ( UTIL_UNIQUEID_NULL != _pendingCLUniqueID )
            {
               setPending( _pendingCLUniqueID, _expectLSN.offset ) ;
            }
         }
         else
         {
            _pendingCLUniqueID = UTIL_UNIQUEID_NULL ;
         }

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
      if ( pCompleteLsn )
      {
         *pCompleteLsn = _expectLSN ;
      }

      // parallel replay stopped
      // we are rolling back, LSN will move backwards
      // so we need to enforce reset index LSNs
      resetUnqIdxLSN( TRUE ) ;

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

      CLS_COMP_MAP::reverse_iterator rit ;

      if ( CLS_BUCKET_WAIT_ROLLBACK != getStatus() )
      {
         goto done ;
      }
      // wait for empty
      _emptyEvent.wait() ;
      // set status
      _status = CLS_BUCKET_ROLLBACKING ;

      // scoped lock to avoid exception
      {
         ossScopedLock lock( &_bucketLatch ) ;

         // push complete queue to bucket
         rit = _completeMap.rbegin() ;
         while ( rit != _completeMap.rend() )
         {
            ++num ;
            clsReplayInfo &info = rit->second ;
            UINT32 unitID = info._unitID ;

            // for record parallel mode, we use collection parallel mode
            // to rollback INSERT/UPDATE/DELETE
            if ( CLS_PARALLA_REC == info._parallaType )
            {
               dpsLogRecordHeader *header =
                                    ( dpsLogRecordHeader * )( info._pData ) ;
               if ( header->_type == LOG_TYPE_DATA_INSERT ||
                    header->_type == LOG_TYPE_DATA_UPDATE ||
                    header->_type == LOG_TYPE_DATA_DELETE )
               {
                  unitID = calcIndex( info._clHash ) ;
               }
            }

            rc = _pushData( unitID, info, FALSE, FALSE ) ;
            if ( rc )
            {
               SDB_ASSERT( SDB_OK == rc, "Push complete log to "
                           "rollback failed" ) ;
               _allCount.dec() ;
               _memPool.release( info._pData, info._len ) ;
            }
            ++rit ;
         }
         _completeMap.clear() ;
      }

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

         if ( !_ntyQueue->timed_wait_and_pop( unitID, OSS_ONE_SEC ) )
         {
            /// when in wait rollback, unit can't quit by timeout
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

         // scoped lock to avlid exception
         {
            ossScopedLock bucketLock( _latchBucket[ unitID ] ) ;

            // if has some other attach in, wait next
            if ( _dataBucket[ unitID ]->isAttached() )
            {
               _latchBucket[ unitID ]->release() ;
               continue ;
            }
            _idleUnitCount.dec() ;
            decIdelAgent() ;
            // set attach
            _dataBucket[ unitID ]->attach() ;
         }

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

      BOOLEAN pushedBack = FALSE ;

      SDB_ASSERT( unitID < _bucketSize, "unitID must less bucket size" ) ;
      if ( unitID >= _bucketSize )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      incIdleAgent() ;

      // scoped lock to avoid exception
      {
         ossScopedLock bucketLock( _latchBucket[ unitID ] ) ;
         SDB_ASSERT( _dataBucket[ unitID ]->isAttached(),
                     "Must attach in unit" ) ;

         _dataBucket[ unitID ]->dettach() ;

         if ( !_dataBucket[ unitID ]->isEmpty() )
         {
            _ntyQueue->push( unitID ) ;
            _dataBucket[ unitID ]->pushToQue() ;

            _idleUnitCount.inc() ;

            pushedBack = TRUE ;
         }
      }

      // scoped lock to avoid exception
      // if the unit is pushed back, it means the unit is not empty
      // so the counters can not be 0
      if ( !pushedBack )
      {
         ossScopedRWLock counterLock( &_counterLock, SHARED ) ;
         if ( _totalCount.compare( 0 ) )
         {
            _emptyEvent.signalAll() ;
         }
         if ( _allCount.compare( 0 ) )
         {
            _allEmptyEvent.signalAll() ;
         }
      }

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

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__REPLAY, "_clsBucket::_replay" )
   INT32 _clsBucket::_replay( UINT32 unitID,
                              pmdEDUCB *cb,
                              clsReplayInfo &info,
                              CLS_SUBMIT_RESULT &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET__REPLAY ) ;

      dpsLogRecordHeader *header = (dpsLogRecordHeader *)info._pData ;

      SDB_ASSERT( NULL != header, "record is invalid" ) ;

      BOOLEAN canRetry = TRUE ;
      BOOLEAN isWaiting = FALSE ;

      // in record parallel mode, need capture duplicated key issues
      // NOTE: record parallel mode may cause duplicated key issue, in both
      //       replay and rollback phases which will cause some records are
      //       left, so we need to ignore those duplicated keys in collection
      //       parallel mode
      BOOLEAN ignoreDupKey =
                  ( CLS_PARALLA_REC == info._parallaType ) ? FALSE : TRUE ;

   retry:
      try
      {
         rc = _replayer->replay( header, cb, FALSE, ignoreDupKey, &info._dataExInfo ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to replay lsn [%llu] because of exception:%s",
                 header->_lsn, e.what() ) ;
      }
      
      if ( rc )
      {
         PD_LOG( PDERROR, "Replay LSN [%llu] encountered error: %d",
                 header->_lsn, rc ) ;
         SDB_ASSERT( ( SDB_OOM == rc ||
                       SDB_NOSPC == rc ||
                       SDB_CLS_FULL_SYNC == rc ||
                       ( SDB_IXM_DUP_KEY == rc && !ignoreDupKey ) ),
                     "Unexpected error occurred" ) ;

         if ( CLS_BUCKET_NORMAL == _status &&
              SDB_IXM_DUP_KEY == rc &&
              !ignoreDupKey &&
              canRetry )
         {
            // found a duplicated key issue
            // wait complete LSN to reach this record, and retry
            if ( !isWaiting )
            {
               _waitAgentNum.inc() ;
               isWaiting = TRUE ;

               PD_LOG( PDDEBUG, "Bucket [%u]: failed to replay lsn [%llu], "
                       "wait to resolve duplicated key, %u threads waiting",
                       unitID, header->_lsn, _waitAgentNum.fetch() ) ;
            }
            // NOTE: all running threads could not be waiting
            if ( _waitAgentNum.fetch() < _curAgentNum.fetch() )
            {
               // not all agents are waiting, so this agent could wait
               waitSubmit( CLS_REPL_RETRY_INTERVAL ) ;
               if ( completeLSN().compareOffset( header->_lsn ) >= 0 )
               {
                  // when meets expect LSN, do the last retry
                  canRetry = FALSE ;
               }
               goto retry ;
            }
            else
            {
               // all agents are waiting, stop waiting
               PD_LOG( PDDEBUG, "Bucket [%u]: too many waiting threads, "
                       "stop waiting", unitID ) ;
            }
         }
         if ( CLS_BUCKET_WAIT_ROLLBACK != _status )
         {
            _status = CLS_BUCKET_WAIT_ROLLBACK ;
            _submitRC = rc ;
            if ( SDB_IXM_DUP_KEY == rc )
            {
               // set pending collection unique ID
               _pendingCLUniqueID = info._clUniqueID ;
               PD_LOG( PDEVENT, "Bucket [%u]: failed to replay lsn [%llu], "
                       "need resolve duplicated key", unitID, header->_lsn ) ;
            }
         }
         _allCount.dec() ;
         _memPool.release( info._pData, info._len ) ;
      }
      else
      {
         _submitResult( header->_lsn, header->_version,
                        header->_length, info, unitID, result ) ;
      }

      if ( isWaiting )
      {
         _waitAgentNum.dec() ;
         isWaiting = FALSE ;
      }

      PD_TRACE_EXITRC( SDB__CLSBUCKET__REPLAY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__ROLLBACK, "_clsBucket::_rollback" )
   INT32 _clsBucket::_rollback( UINT32 unitID,
                                pmdEDUCB *cb,
                                clsReplayInfo &info )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET__ROLLBACK ) ;

      dpsLogRecordHeader *header = (dpsLogRecordHeader *)info._pData ;

      SDB_ASSERT( NULL != header, "record is invalid" ) ;

      PD_LOG( PDDEBUG, "Bucket [%u]: start to rollback lsn [%llu]",
              unitID, header->_lsn ) ;

      UINT32 retryTimes = 0 ;
      while( TRUE )
      {
         rc = _replayer->rollback( header, cb ) ;
         // in few cases, we could retry later
         // OOM or No space
         if ( ( SDB_OOM == rc ||
                SDB_NOSPC == rc ||
                SDB_DB_FULLSYNC == rc ) &&
                ++retryTimes < CLS_REPL_MAX_ROLLBACK_TIMES )
         {
            ossSleep( CLS_REPL_RETRY_INTERVAL ) ;
            continue ;
         }
         else if ( SDB_RTN_AUTOINDEXID_IS_FALSE == rc )
         {
            // id index is missing, we should start full sync
            PD_LOG( PDERROR, "Failed to rollback lsn [%llu] need restart to "
                    "full sync, rc: %d", header->_lsn, rc ) ;
            PMD_RESTART_DB_FULLSYNC( rc ) ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to rollback lsn [%llu], rc: %d",
                    header->_lsn, rc ) ;
            SDB_ASSERT( SDB_OK == rc, "Rollback dps log failed" ) ;
         }
         break ;
      }

      _allCount.dec() ;
      _memPool.release( info._pData, info._len ) ;

      PD_TRACE_EXITRC( SDB__CLSBUCKET__ROLLBACK, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_SUBMITDATA, "_clsBucket::submitData" )
   INT32 _clsBucket::submitData( UINT32 unitID, _pmdEDUCB *cb,
                                 clsReplayInfo &info,
                                 CLS_SUBMIT_RESULT &result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSBUCKET_SUBMITDATA ) ;

      if ( CLS_BUCKET_ROLLBACKING != _status )
      {
         rc = _replay( unitID, cb, info, result ) ;
      }
      else
      {
         rc = _rollback( unitID, cb, info ) ;
      }

      _totalCount.dec () ;

      PD_TRACE_EXITRC( SDB__CLSBUCKET_SUBMITDATA, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET__SUBMITRESULT, "_clsBucket::_submitResult" )
   void _clsBucket::_submitResult( DPS_LSN_OFFSET offset, DPS_LSN_VER version,
                                   UINT32 lsnLen, clsReplayInfo &info,
                                   UINT32 unitID, CLS_SUBMIT_RESULT &result )
   {
      PD_TRACE_ENTRY( SDB__CLSBUCKET__SUBMITRESULT ) ;

      info._unitID   = unitID ;

      BOOLEAN releaseMem = FALSE ;

      // scoped lock to avoid exception
      ossScopedLock lock( &_bucketLatch ) ;

      // increase repl counter
      _incCount( info._pData ) ;

      // the first one
      if ( _expectLSN.compareOffset( offset ) >= 0 )
      {
         SDB_ASSERT( 0 == _expectLSN.compareOffset( offset ),
                     "expect lsn is error" ) ;

         if ( 0 == _expectLSN.compareOffset( offset ) )
         {
            _expectLSN.version = version ;
            _expectLSN.offset += lsnLen ;
            if ( info._dataExInfo._isValid && NULL != _replayEventHandler )
            {
               _replayEventHandler->onReplayLog( info._dataExInfo._csLID,
                                                 info._dataExInfo._clLID,
                                                 info._dataExInfo._extID,
                                                 info._dataExInfo._extOffset,
                                                 info._dataExInfo._lobOid,
                                                 info._dataExInfo._lobSequence,
                                                 offset ) ;
            }
         }
         result = CLS_SUBMIT_EQ_EXPECT ;
         releaseMem = TRUE ;

         CLS_COMP_MAP::iterator it = _completeMap.begin() ;
         while ( it != _completeMap.end() )
         {
            clsReplayInfo &tmpInfo = it->second ;
            if ( _expectLSN.compareOffset( it->first ) >= 0 )
            {
               if ( 0 == _expectLSN.compareOffset( it->first ) )
               {
                  _expectLSN.version =
                     ((dpsLogRecordHeader*)tmpInfo._pData)->_version ;
                  _expectLSN.offset +=
                     ((dpsLogRecordHeader*)tmpInfo._pData)->_length ;
               }
               // notify to lsn full sync source session
               if ( tmpInfo._dataExInfo._isValid && NULL != _replayEventHandler )
               {
                  _replayEventHandler->onReplayLog( tmpInfo._dataExInfo._csLID,
                                                    tmpInfo._dataExInfo._clLID,
                                                    tmpInfo._dataExInfo._extID,
                                                    tmpInfo._dataExInfo._extOffset,
                                                    tmpInfo._dataExInfo._lobOid,
                                                    tmpInfo._dataExInfo._lobSequence,
                                                    it->first ) ;
               }
               _memPool.release( tmpInfo._pData, tmpInfo._len ) ;
               _completeMap.erase( it++ ) ;
               _allCount.dec() ;
               continue ;
            }
            break ;
         }
         _submitEvent.signalAll() ;
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

      // not expect, insert to map
      if ( !(_completeMap.insert( std::make_pair( offset, info ) ) ).second )
      {
         SDB_ASSERT( FALSE, "System error, dps log exist" ) ;
         releaseMem = TRUE ;
         goto done ;
      }

   done:
      if ( releaseMem )
      {
         _memPool.release( info._pData, info._len ) ;
         _allCount.dec() ;
      }
      PD_TRACE_EXIT( SDB__CLSBUCKET__SUBMITRESULT ) ;
      return ;
   }

   BOOLEAN _clsBucket::hasPending()
   {
      return ( UTIL_UNIQUEID_NULL != _pendingCLUniqueID ) ;
   }

   DPS_LSN _clsBucket::completeLSN ( BOOLEAN withRetEvent )
   {
      ossScopedLock lock( &_bucketLatch, SHARED ) ;
      if ( withRetEvent )
      {
         _submitEvent.reset() ;
      }
      return _expectLSN ;
   }

   DPS_LSN _clsBucket::fastCompleteLSN( UINT32 retryTimes,
                                        BOOLEAN *pDirty )
   {
      UINT32 i = 0 ;
      DPS_LSN expectLsn ;

      while( i++ < retryTimes )
      {
         if ( _bucketLatch.try_get_shared() )
         {
            expectLsn = _expectLSN ;
            _bucketLatch.release_shared() ;
            if ( pDirty )
            {
               *pDirty = FALSE ;
            }
            return expectLsn ;
         }
         _submitEvent.wait( 1 ) ;
      }

      expectLsn = _expectLSN ;
      if ( pDirty )
      {
         *pDirty = TRUE ;
      }
      return expectLsn ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSBUCKET_FORCECOMPLETE, "_clsBucket::forceCompleteAll" )
   INT32 _clsBucket::forceCompleteAll ()
   {
      PD_TRACE_ENTRY( SDB__CLSBUCKET_FORCECOMPLETE ) ;
      CLS_COMP_MAP::iterator it ;

      ossScopedLock lock( &_bucketLatch ) ;

      // if has agent process, do nothing
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
         clsReplayInfo &tmpInfo = it->second ;
         dpsLogRecordHeader *pHeader = (dpsLogRecordHeader*)( tmpInfo._pData) ;

         /// Only normal to change _expectLSN
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
      _submitEvent.signalAll() ;

      _allEmptyEvent.signalAll() ;
      _emptyEvent.signalAll() ;

      PD_LOG( PDWARNING, "Repl bucket end to force complete, expect lsn: "
              "[%d,%lld], Status:%d[%s]",
              _expectLSN.version, _expectLSN.offset,
              _status, clsGetReplBucketStatusDesp( (INT32)_status ) ) ;

   done:
      PD_TRACE_EXIT( SDB__CLSBUCKET_FORCECOMPLETE ) ;
      return SDB_OK ;
   }

   clsCLParallaInfo *_clsBucket::getOrCreateInfo( const CHAR *collection,
                                                  utilCLUniqueID clUID )
   {
      clsCLParallaInfo *info = NULL ;

      try
      {
#if defined(_DEBUG)
         MAP_CL_PARALLAINFO::iterator iter = _mapParallaInfo.find( clUID ) ;
         if ( iter == _mapParallaInfo.end() )
         {
            info = ( &_mapParallaInfo[ clUID ] ) ;
            info->setCollection( collection ) ;
         }
         else
         {
            info = ( &_mapParallaInfo[ clUID ] ) ;
         }
#else
         info = ( &_mapParallaInfo[ clUID ] ) ;
#endif
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return info ;
   }

   void _clsBucket::setPending( utilCLUniqueID clUID, DPS_LSN_OFFSET lsn )
   {
      MAP_CL_PARALLAINFO::iterator iter = _mapParallaInfo.find( clUID ) ;
      if ( iter != _mapParallaInfo.end() )
      {
         iter->second.setPending( lsn ) ;
      }
   }

   void _clsBucket::clearParallaInfo()
   {
      resetUnqIdxLSN( FALSE ) ;
      _mapParallaInfo.clear() ;
   }

   INT32 _clsBucket::waitForLSN( DPS_LSN_OFFSET lsn )
   {
      INT32 rc = SDB_OK ;

      DPS_LSN curLSN = completeLSN( TRUE ) ;
      while ( curLSN.compareOffset( lsn ) <= 0 )
      {
         if ( CLS_BUCKET_NORMAL != getStatus() ||
              bucketSize() == 0 )
         {
            rc = waitEmptyWithCheck() ;
            break ;
         }
         waitSubmit( OSS_ONE_SEC ) ;
         curLSN = completeLSN( TRUE ) ;
      }

      return rc ;
   }

   INT32 _clsBucket::waitForIDLSNComp( DPS_LSN_OFFSET nidRecLSN  )
   {
      INT32 rc = SDB_OK ;

      // no ID record parallel LSN is set, OK to return
      if ( DPS_INVALID_LSN_OFFSET == _lastIDRecParaLSN )
      {
         _lastNIDRecParaLSN = nidRecLSN ;
         goto done ;
      }

      // wait LSN to be completed
      rc = waitForLSN( _lastIDRecParaLSN ) ;
      if ( SDB_OK == rc )
      {
         // last ID record parallel LSN is completed, we could skip next time
         _lastIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
         _lastNIDRecParaLSN = nidRecLSN ;
      }

   done:
      return rc ;
   }

   INT32 _clsBucket::waitForNIDLSNComp( DPS_LSN_OFFSET idRecLSN  )
   {
      INT32 rc = SDB_OK ;

      // no non-ID record parallel LSN is set, OK to return
      if ( DPS_INVALID_LSN_OFFSET == _lastNIDRecParaLSN )
      {
         _lastIDRecParaLSN = idRecLSN ;
         goto done ;
      }

      // wait LSN to be completed
      rc = waitForLSN( _lastNIDRecParaLSN ) ;
      if ( SDB_OK == rc )
      {
         // last non-ID record parallel LSN is completed
         // we could skip next time
         _lastNIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
         _lastIDRecParaLSN = idRecLSN ;
      }

   done:
      return rc ;
   }

   void _clsBucket::initUnqIdxLSN()
   {
      for ( UINT32 i = 0 ; i < _lastUnqIdxSize ; ++ i )
      {
         _lastNewUnqIdxLSN[ i ] = DPS_INVALID_LSN_OFFSET ;
         _lastNewUnqIdxBkt[ i ] = -1 ;
         _lastOldUnqIdxLSN[ i ] = DPS_INVALID_LSN_OFFSET ;
         _lastOldUnqIdxBkt[ i ] = -1 ;
      }
      _lastIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
      _lastNIDRecParaLSN = DPS_INVALID_LSN_OFFSET ;
      _lastExpectLSN = DPS_INVALID_LSN_OFFSET ;
   }

   void _clsBucket::resetUnqIdxLSN( BOOLEAN isEnforced )
   {
      if ( _lastUnqIdxSize > 0 &&
           ( isEnforced ||
             DPS_INVALID_LSN_OFFSET != _lastExpectLSN ) )
      {
         initUnqIdxLSN() ;
      }
   }

   DPS_LSN_OFFSET _clsBucket::checkUnqIdxWaitLSN(
                                    dpsUnqIdxHashArray &newUnqIdxHashArray,
                                    dpsUnqIdxHashArray &oldUnqIdxHashArray,
                                    DPS_LSN_OFFSET currentLSN,
                                    UINT32 clHash,
                                    UINT32 bucketID )
   {
      DPS_LSN_OFFSET waitLSN = DPS_INVALID_LSN_OFFSET ;

      // find the maximum LSN with the same hash values replayed
      // by the previous records
      // WARNING: should be called by dispatch thread of clsReplayer
      if ( !newUnqIdxHashArray.empty() )
      {
         _checkUnqIdxWaitLSN( newUnqIdxHashArray,
                              currentLSN,
                              clHash,
                              bucketID,
                              waitLSN,
                              _unqIdxBitmap,
                              _lastOldUnqIdxLSN,
                              _lastOldUnqIdxBkt ) ;
      }
      if ( !oldUnqIdxHashArray.empty() )
      {
         _checkUnqIdxWaitLSN( oldUnqIdxHashArray,
                              currentLSN,
                              clHash,
                              bucketID,
                              waitLSN,
                              _unqIdxBitmap,
                              _lastNewUnqIdxLSN,
                              _lastNewUnqIdxBkt ) ;
      }
      if ( newUnqIdxHashArray.getCurSize() > 0 )
      {
         _saveUnqIdxWaitLSN( newUnqIdxHashArray, currentLSN, bucketID,
                             _lastNewUnqIdxLSN, _lastNewUnqIdxBkt ) ;
      }
      if ( oldUnqIdxHashArray.getCurSize() > 0 )
      {
         _saveUnqIdxWaitLSN( oldUnqIdxHashArray, currentLSN, bucketID,
                             _lastOldUnqIdxLSN, _lastOldUnqIdxBkt ) ;
      }

      if ( DPS_INVALID_LSN_OFFSET != waitLSN )
      {
         // check if wait LSN already completed
         if ( DPS_INVALID_LSN_OFFSET != _lastExpectLSN &&
              waitLSN < _lastExpectLSN )
         {
            waitLSN =  DPS_INVALID_LSN_OFFSET ;
         }
         else
         {
            _lastExpectLSN = completeLSN( FALSE ).offset ;
            if ( DPS_INVALID_LSN_OFFSET != _lastExpectLSN  &&
                 waitLSN < _lastExpectLSN )
            {
               waitLSN = DPS_INVALID_LSN_OFFSET ;
            }
         }
      }

      return waitLSN ;
   }

   void _clsBucket::_checkUnqIdxWaitLSN( dpsUnqIdxHashArray &unqIdxHashArray,
                                         DPS_LSN_OFFSET currentLSN,
                                         UINT32 clHash,
                                         UINT32 bucketID,
                                         DPS_LSN_OFFSET &waitLSN,
                                         utilBitmap &unqIdxBitmap,
                                         DPS_LSN_OFFSET *checkLSN,
                                         INT16 *checkBucket )
   {
      SDB_ASSERT( NULL != checkLSN, "check LSN is invalid" ) ;
      SDB_ASSERT( NULL != checkBucket, "check bucket is invalid" ) ;

      UINT32 index = 0 ;

      unqIdxBitmap.resetBitmap() ;

      for ( index = 0 ; index < unqIdxHashArray.size() ; ++ index )
      {
         UINT16 hashValue = unqIdxHashArray[ index ] ;
         if ( DPS_UNQIDX_INVALID_HASH != hashValue )
         {
            UINT32 reHashValue =
                  BSON_HASHER::hashCombine( clHash, (UINT32)hashValue ) ;
            reHashValue &= CLS_UNQIDX_HASH_MOD ;
            unqIdxHashArray[ index ] = (UINT16)reHashValue ;

            if ( !unqIdxBitmap.testBit( reHashValue ) )
            {
               // find for maximum wait LSN for all hash values
               // NOTE: if last value is pushed to the same bucket,
               //       no need to wait
               INT16 checkBucketID = checkBucket[ reHashValue ] ;
               DPS_LSN_OFFSET checkOffset = checkLSN[ reHashValue ] ;
               if ( (INT16)bucketID != checkBucketID &&
                    DPS_INVALID_LSN_OFFSET != checkOffset &&
                    ( DPS_INVALID_LSN_OFFSET == waitLSN ||
                      waitLSN < checkOffset ) )
               {
                  waitLSN = checkOffset ;
               }
               unqIdxBitmap.setBit( reHashValue ) ;
            }
         }
         else
         {
            break ;
         }
      }

      unqIdxHashArray.setCurSize( index ) ;
   }

   void _clsBucket::_saveUnqIdxWaitLSN( dpsUnqIdxHashArray &unqIdxHashArray,
                                        DPS_LSN_OFFSET currentLSN,
                                        UINT32 bucketID,
                                        DPS_LSN_OFFSET *saveLSN,
                                        INT16 *saveBucket )
   {
      for ( UINT32 index = 0 ;
            index < unqIdxHashArray.getCurSize() ;
            ++ index )
      {
         UINT16 hashValue = unqIdxHashArray[ index ] ;
         saveLSN[ hashValue ] = currentLSN ;
         saveBucket[ hashValue ] = (INT16)bucketID ;
      }
   }

   /*
      _clsBucketSyncJob implement
   */
   _clsBucketSyncJob::_clsBucketSyncJob ( clsBucket *pBucket, INT32 timeout )
   {
      _pBucket = pBucket ;
      _timeout = timeout ;
      _hasEndUnit = TRUE ;
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
      UINT32 number           = 0 ;
      CLS_SUBMIT_RESULT res   = CLS_SUBMIT_EQ_EXPECT ;

      while ( TRUE )
      {
         clsReplayInfo info ;

         eduMgr->waitEDU( eduCB() ) ;

         rc = _pBucket->beginUnit( eduCB(), unitID, _timeout ) ;
         if ( rc )
         {
            break ;
         }
         _hasEndUnit = FALSE ;
         eduMgr->activateEDU( eduCB() ) ;

         number = 0 ;
         while ( _pBucket->popData( unitID, info ) )
         {
            ++number ;
            eduCB()->incEventCount() ;

            _pBucket->submitData( unitID, eduCB(), info, res ) ;

            if ( _pBucket->idleUnitCount() > 1 && CLS_SUBMIT_EQ_EXPECT != res )
            {
               if ( CLS_SUBMIT_GT_MAX == res || number > CLS_REPLSYNC_ONCE_NUM )
               {
                  break ;
               }
            }
         }

         _pBucket->endUnit( eduCB(), unitID ) ;
         _hasEndUnit = TRUE ;
      }

      PD_TRACE_EXIT( SDB__CLSBUCKETSYNCJOB_DOIT ) ;
      return SDB_OK ;
   }

   void _clsBucketSyncJob::_onAttach()
   {
   }

   void _clsBucketSyncJob::_onDetach()
   {
      if ( _pBucket )
      {
         if ( _hasEndUnit )
         {
            /// when doit occur exception, endUnit is not called.
            /// In this case, do not dec idle agent
            _pBucket->decIdelAgent() ;
         }
         _pBucket->decCurAgent() ;

         if ( _pBucket->curAgentNum() == 0 )
         {
            BOOLEAN needForce = FALSE ;

            if ( _pBucket->idleUnitCount() > 0 && PMD_IS_DB_UP() )
            {
               if ( SDB_OK == startReplSyncJob( NULL, _pBucket,
                                                60*OSS_ONE_SEC ) )
               {
                  _pBucket->incCurAgent() ;
                  _pBucket->incIdleAgent() ;
               }
               else
               {
                  needForce = TRUE ;
               }
            }
            else
            {
               needForce = TRUE ;
            }

            if ( needForce && 0 != _pBucket->size() )
            {
               PD_LOG( PDERROR, "Repl bucket info has error: %s",
                       _pBucket->toBson().toString().c_str() ) ;

               _pBucket->forceCompleteAll() ;
            }
         }
      }
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

