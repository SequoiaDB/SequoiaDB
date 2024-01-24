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

   Source File Name = dmsTaskStatus.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who      Description
   ====== ========= =======  ==============================================
          2019/8/23 Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsTaskStatus.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include "msgDef.hpp"

namespace engine
{
   const CHAR* dmsTaskTypeStr( DMS_TASK_TYPE taskType )
   {
      switch( taskType )
      {
         case DMS_TASK_SPLIT :
            return VALUE_NAME_SPLIT ;
         case DMS_TASK_SEQUENCE :
            return VALUE_NAME_ALTERSEQUENCE ;
         case DMS_TASK_CREATE_IDX :
            return VALUE_NAME_CREATEIDX ;
         case DMS_TASK_DROP_IDX :
            return VALUE_NAME_DROPIDX ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* dmsTaskStatusStr( DMS_TASK_STATUS taskStatus )
   {
      switch( taskStatus )
      {
         case DMS_TASK_STATUS_READY :
            return VALUE_NAME_READY ;
         case DMS_TASK_STATUS_RUN :
            return VALUE_NAME_RUNNING ;
         case DMS_TASK_STATUS_PAUSE :
            return VALUE_NAME_PAUSE ;
         case DMS_TASK_STATUS_CANCELED :
            return VALUE_NAME_CANCELED ;
         case DMS_TASK_STATUS_META :
            return VALUE_NAME_CHGMETA ;
         case DMS_TASK_STATUS_CLEANUP :
            return VALUE_NAME_CLEANUP ;
         case DMS_TASK_STATUS_ROLLBACK :
            return VALUE_NAME_ROLLBACK ;
         case DMS_TASK_STATUS_FINISH :
            return VALUE_NAME_FINISH ;
         case DMS_TASK_STATUS_END :
            return VALUE_NAME_END ;
         default :
            break ;
      }
      return "Unknown" ;
   }

   const CHAR* dmsOpInfoStr( DMS_OPINFO_TYPE infoType )
   {
      switch( infoType )
      {
         case OPINFO_SCAN_DATA :
            return DMS_OPINFO_STR_SCANDATA ;
         case OPINFO_SORT_DATA :
            return DMS_OPINFO_STR_SORTDATA ;
         case OPINFO_INSERT_KEY :
            return DMS_OPINFO_STR_INSERTKEY ;
         case OPINFO_SCAN_THEN_INSERT :
            return DMS_OPINFO_STR_SCANTHENINSERT ;
         default :
            break ;
      }
      return "" ;
   }

   #define DMS_TASK_PROGRESS_100     ( 100 )      // 100%
   #define DMS_TASK_EXPIRED_TIME     ( 1800 )     // second, 30 min

   _dmsIdxTaskStatus::_dmsIdxTaskStatus( DMS_TASK_TYPE taskType,
                                         UINT64 taskID,
                                         UINT32 locationID,
                                         UINT64 mainTaskID )
   :_dmsTaskStatus( taskID ),
    _taskType( taskType ),
    _locationID( locationID ),
    _mainTaskID( mainTaskID ),
    _taskStatus( DMS_TASK_STATUS_READY ),
    _clUniqueID( UTIL_UNIQUEID_NULL ),
    _sortBufSize( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ),
    _retryCnt( 0 ),
    _resultCode( SDB_OK ),
    _opInfo( OPINFO_UNKNOWN ),
    _isInitialized( FALSE ),
    _pauseReport( FALSE ),
    _hasSetDef( FALSE ),
    _isGlobalIdx( FALSE ),
    _totalRecNum( 0 ),
    _pcsedRecNum( 0 ),
    _pcsRecNumLastTime( 0 ),
    _progress( 0 ),
    _speed( 0 ),
    _timeSpent( 0.0 ),
    _timeLeft( 0.0 )
   {
      if ( DMS_IS_DUMMY_CATTASKID( taskID ) )
      {
         _hasCatalogTask = FALSE ;
      }
      else
      {
         _hasCatalogTask = TRUE ;
      }
      ossMemset( _clFullName, 0, DMS_COLLECTION_FULL_NAME_SZ + 1 ) ;
      ossMemset( _indexName,  0, IXM_INDEX_NAME_SIZE + 1 ) ;
      ossGetCurrentTime( _beginTimestamp ) ;
      _calculateTimestamp = _beginTimestamp ;
   }

   _dmsIdxTaskStatus& _dmsIdxTaskStatus::operator=( const _dmsIdxTaskStatus &rhs )
   {
      _taskType = rhs._taskType ;
      _taskID = rhs._taskID ;
      _locationID = rhs._locationID ;
      _mainTaskID = rhs._mainTaskID ;
      _hasCatalogTask = rhs._hasCatalogTask ;
      _pauseReport = rhs._pauseReport ;
      _taskStatus = rhs._taskStatus ;
      ossStrncpy( _clFullName, rhs._clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clUniqueID = rhs._clUniqueID ;
      _indexDef = rhs._indexDef.getOwned() ;
      ossStrncpy( _indexName, rhs._indexName, IXM_INDEX_NAME_SIZE ) ;
      _sortBufSize = rhs._sortBufSize ;
      _isGlobalIdx = rhs._isGlobalIdx ;
      _totalRecNum = rhs._totalRecNum ;
      _pcsedRecNum = rhs._pcsedRecNum ;
      _pcsRecNumLastTime = rhs._pcsRecNumLastTime ;
      _calculateTimestamp = rhs._calculateTimestamp ;
      _beginTimestamp = rhs._beginTimestamp ;
      _endTimestamp = rhs._endTimestamp ;
      _retryCnt = rhs._retryCnt ;
      _resultCode = rhs._resultCode ;
      _opInfo = rhs._opInfo ;
      _progress = rhs._progress ;
      _speed = rhs._speed ;
      _timeSpent = rhs._timeSpent ;
      _timeLeft = rhs._timeLeft ;
      _isInitialized = rhs._isInitialized ;
      return *this ;
   }

   void _dmsIdxTaskStatus::setStatus( DMS_TASK_STATUS status )
   {
      _taskStatus = status ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_INIT, "_dmsIdxTaskStatus::init" )
   INT32 _dmsIdxTaskStatus::init( const CHAR* collectionName,
                                  const BSONObj& index,
                                  INT32 sortBufSize,
                                  utilCLUniqueID clUniqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_INIT ) ;

      if ( _isInitialized )
      {
         goto done ; ;
      }

      // colection
      ossStrncpy( _clFullName, collectionName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clUniqueID = clUniqID ;

      // index definition
      try
      {
         if ( index.hasField( IXM_NAME_FIELD ) )
         {
            _indexDef = index.getOwned() ;
            _hasSetDef = TRUE ;

            // create index, idxDef: { "name": "aIdx", "key": { "a": 1 }, ... }
            BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_NAME ) ;
            PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                       "Field[%s] invalid in index def[%s]",
                       IXM_FIELD_NAME_NAME, _indexDef.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;

            _isGlobalIdx = _indexDef[ IXM_GLOBAL_FIELD ].trueValue() ;
         }
         else
         {
            // drop index: { "": "aIdx" }
            BSONElement ele = index.firstElement() ;
            PD_CHECK ( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                       "Field[\"\"] invalid in index obj[%s]",
                       index.toString().c_str() ) ;
            ossStrncpy( _indexName, ele.valuestr(), IXM_INDEX_NAME_SIZE ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      // sort buffer size
      if ( sortBufSize >= 0 )
      {
         _sortBufSize = sortBufSize ;
      }

      _isInitialized = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB_DMSIDXTASKSTAT_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_TOBSON, "_dmsIdxTaskStatus::toBSON" )
   BSONObj _dmsIdxTaskStatus::toBSON( UINT32 mask )
   {
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_TOBSON ) ;

      ossScopedLock lock( &_latch, SHARED ) ;

      BSONObjBuilder builder ;

      if ( DMS_TASK_MASK_GROUPNAME & mask )
      {
         builder.append( FIELD_NAME_GROUPNAME, pmdGetKRCB()->getGroupName() ) ;
      }
      if ( DMS_TASK_MASK_MAINTASKID & mask )
      {
         if ( DMS_INVALID_TASKID != _mainTaskID )
         {
            builder.append( FIELD_NAME_MAIN_TASKID, (INT64)_mainTaskID ) ;
         }
      }
      if ( DMS_TASK_MASK_TASKID & mask )
      {
         builder.append( FIELD_NAME_TASKID, (INT64)_taskID ) ;
      }
      if ( DMS_TASK_MASK_STATUS & mask )
      {
         builder.append( FIELD_NAME_STATUS, _taskStatus ) ;
         builder.append( FIELD_NAME_STATUSDESC,
                         dmsTaskStatusStr( _taskStatus ) ) ;
      }
      if ( DMS_TASK_MASK_TASKTYPE & mask )
      {
         builder.append( FIELD_NAME_TASKTYPE, _taskType ) ;
         builder.append( FIELD_NAME_TASKTYPEDESC,
                         dmsTaskTypeStr( _taskType ) ) ;
      }
      if ( DMS_TASK_MASK_CLNAME & mask )
      {
         builder.append( FIELD_NAME_NAME, _clFullName ) ;
         // If collection unique id is not set, we don't display UniqueID field.
         if ( _clUniqueID != UTIL_UNIQUEID_NULL )
         {
            builder.append( FIELD_NAME_UNIQUEID, (INT64)_clUniqueID ) ;
         }
      }
      if ( DMS_TASK_MASK_IDXNAME & mask )
      {
         builder.append( FIELD_NAME_INDEXNAME, _indexName ) ;
      }
      if ( DMS_TASK_MASK_IDXDEF & mask )
      {
         if ( _hasSetDef )
         {
            builder.append( IXM_FIELD_NAME_INDEX_DEF, _indexDef ) ;
         }
      }
      if ( DMS_TASK_MASK_SORTBUFSZ & mask )
      {
         if ( DMS_TASK_CREATE_IDX == _taskType )
         {
            builder.append( IXM_FIELD_NAME_SORT_BUFFER_SIZE, _sortBufSize ) ;
         }
      }
      if ( DMS_TASK_MASK_RESULTCODE & mask )
      {
         builder.append( FIELD_NAME_RESULTCODE, _resultCode ) ;
         builder.append( FIELD_NAME_RESULTCODEDESC,
                         DMS_TASK_STATUS_FINISH == _taskStatus ?
                         getErrDesp( _resultCode ) : "" ) ;
      }
      if ( DMS_TASK_MASK_RESULTINFO & mask )
      {
         builder.append( FIELD_NAME_RESULTINFO, _resultInfo ) ;
      }
      if ( DMS_TASK_MASK_OPINFO & mask )
      {
         builder.append( FIELD_NAME_OPINFO, dmsOpInfoStr( _opInfo ) ) ;
      }
      if ( DMS_TASK_MASK_RETRYCNT & mask )
      {
         builder.append( FIELD_NAME_RETRY_COUNT, _retryCnt ) ;
      }
      if ( DMS_TASK_MASK_PROGRESS & mask )
      {
         builder.append( FIELD_NAME_PROGRESS, _progress ) ;
      }
      if ( DMS_TASK_MASK_SPEED & mask )
      {
         builder.append( FIELD_NAME_SPEED, (INT64)_speed ) ;
      }
      if ( DMS_TASK_MASK_TIMESPENT & mask )
      {
         builder.append( FIELD_NAME_TIMESPENT, _timeSpent ) ;
      }
      if ( DMS_TASK_MASK_TIMELEFT & mask )
      {
         builder.append( FIELD_NAME_TIMELEFT, _timeLeft ) ;
      }

      if ( DMS_TASK_MASK_TOTALSZ & mask )
      {
         builder.append( FIELD_NAME_TOTAL_RECORDS,
                         (INT64)(_totalRecNum.fetch()) ) ;
      }
      if ( DMS_TASK_MASK_PROCESSSZ & mask )
      {
         builder.append( FIELD_NAME_PROCESSED_RECORDS,
                         (INT64)(_pcsedRecNum.fetch()) ) ;
      }

      PD_TRACE_EXIT( SDB_DMSIDXTASKSTAT_TOBSON ) ;
      return builder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSIDXTASKSTAT_UPPGS, "_dmsIdxTaskStatus::updateProgress" )
   void _dmsIdxTaskStatus::updateProgress()
   {
      PD_TRACE_ENTRY( SDB_DMSIDXTASKSTAT_UPPGS ) ;

      ossScopedLock lock( &_latch, EXCLUSIVE ) ;

      BOOLEAN isFirstCalculate = _pcsRecNumLastTime == 0 ? TRUE : FALSE ;
      ossTimestamp currentTimestamp ;
      ossGetCurrentTime( currentTimestamp ) ;

      UINT64 curTime = currentTimestamp.time * 1000000 +
                         currentTimestamp.microtm  ;
      UINT64 calTime = _calculateTimestamp.time * 1000000 +
                       _calculateTimestamp.microtm  ;
      FLOAT64 calTimeInterval = ( curTime - calTime ) / 1000000.0 ;

      UINT64 totalRecNum = _totalRecNum.fetch() ;
      UINT64 pcsedRecNum = _pcsedRecNum.fetch() ;

      // If it is first time, we should calculate. If task has finished, we
      // only calculate ONE time. If calculate frequently, we just skip.
      if ( DMS_TASK_STATUS_FINISH == status() )
      {
         if ( DMS_TASK_PROGRESS_100 == _progress )
         {
            goto done ;
         }
      }
      else if ( !isFirstCalculate )
      {
         if ( currentTimestamp.time - _calculateTimestamp.time < 5 )
         {
            goto done ;
         }
      }

      if ( DMS_TASK_STATUS_FINISH == status() )
      {
         _progress = DMS_TASK_PROGRESS_100 ;
         _timeLeft = 0.0 ;

         if ( calTimeInterval != 0.0 )
         {
            _speed = ( pcsedRecNum - _pcsRecNumLastTime ) / calTimeInterval ;
         }
         else
         {
            _speed = 0 ;
         }

         UINT64 beginTime = _beginTimestamp.time * 1000000 +
                            _beginTimestamp.microtm  ;
         UINT64 endTime = _endTimestamp.time * 1000000 +
                          _endTimestamp.microtm  ;
         _timeSpent = ( endTime - beginTime ) / 1000000.0 ;
      }
      else
      {
         // progress
         if ( totalRecNum != 0 )
         {
            FLOAT64 percentage = (FLOAT64)pcsedRecNum / totalRecNum ;
            _progress = percentage * 100 ;
         }
         else
         {
            _progress = 0 ;
         }
         if ( DMS_TASK_PROGRESS_100 == _progress )
         {
            // the task is not finished yet.
            _progress = 99 ;
         }

         // time spent
         UINT64 beginMs = _beginTimestamp.time * 1000000 +
                            _beginTimestamp.microtm  ;
         UINT64 currentMs = currentTimestamp.time * 1000000 +
                              currentTimestamp.microtm  ;
         _timeSpent = ( currentMs - beginMs ) / 1000000.0 ;

         // speed
         if ( calTimeInterval != 0.0 )
         {
            _speed = ( pcsedRecNum - _pcsRecNumLastTime ) / calTimeInterval ;
         }
         else
         {
            _speed = 0 ;
         }

         // time left
         if ( _speed != 0 )
         {
            _timeLeft = ( totalRecNum - pcsedRecNum ) / _speed ;
         }
         else if ( _progress != 0 )
         {
            // timeSpent   timeLeft
            // --------- = --------------
            // progress    100 - progress
            _timeLeft = _timeSpent / _progress *
                        ( DMS_TASK_PROGRESS_100 - _progress ) ;
         }
         else
         {
            _timeLeft = 0.0 ;
         }
      }

      _calculateTimestamp = currentTimestamp ;
      _pcsRecNumLastTime = pcsedRecNum ;

   done :
      PD_TRACE_EXIT( SDB_DMSIDXTASKSTAT_UPPGS ) ;
      return ;
   }

   const CHAR* _dmsIdxTaskStatus::collectionName() const
   {
      // Not under the protection of latch
      return _clFullName ;
   }

   void _dmsIdxTaskStatus::collectionName( CHAR* name, INT32 size ) const
   {
      SDB_ASSERT( name, "point can't be null") ;
      SDB_ASSERT( size > DMS_COLLECTION_FULL_NAME_SZ, "size is too small") ;

      ossScopedLock lock( &_nameLatch, SHARED ) ;
      ossStrncpy( name, _clFullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      name[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;
   }

   void _dmsIdxTaskStatus::collectionRename( const CHAR* newCLName )
   {
      ossScopedLock lock( &_nameLatch, EXCLUSIVE ) ;
      ossStrncpy( _clFullName, newCLName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;
   }

   void _dmsIdxTaskStatus::setTotalRecNum( UINT64 num )
   {
      _totalRecNum.swap( num ) ;
   }

   void _dmsIdxTaskStatus::incPcsedRecNum( UINT64 delta )
   {
      _pcsedRecNum.add( delta ) ;
   }

   void _dmsIdxTaskStatus::resetPcsedRecNum()
   {
      _pcsedRecNum.swap( 0 ) ;
   }

   void _dmsIdxTaskStatus::incRetryCnt()
   {
      _retryCnt++ ;
   }

   void _dmsIdxTaskStatus::_buildResultInfo( INT32 resultCode,
                                             const CHAR* resultDetail,
                                             utilWriteResult* wResultDetail )
   {
      if ( resultCode == SDB_OK )
      {
         return ;
      }

      try
      {
         if ( resultDetail )
         {
            _resultInfo = BSON( FIELD_NAME_DETAIL << resultDetail ) ;
         }
         else if ( wResultDetail )
         {
            _resultInfo = wResultDetail->toBSON().getOwned() ;
         }
         if ( _resultInfo.isEmpty() )
         {
            if ( SDB_TASK_HAS_CANCELED == _resultCode ||
                 SDB_TASK_ROLLBACK == _resultCode )
            {
               if ( DMS_TASK_CREATE_IDX == _taskType &&
                    SDB_IXM_NOTEXIST == resultCode )
               {
                  // Index not found during rollback, it is normal
               }
               else
               {
                  _resultInfo = BSON( FIELD_NAME_INTERNAL_RESULTCODE <<
                                      resultCode ) ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
   }

   void _dmsIdxTaskStatus::setStatus2Finish( INT32 resultCode,
                                             const CHAR* resultDetail,
                                             utilWriteResult* wResultDetail )
   {
      if ( DMS_TASK_STATUS_CANCELED == _taskStatus )
      {
         _resultCode = SDB_TASK_HAS_CANCELED ;
      }
      else if ( DMS_TASK_STATUS_ROLLBACK == _taskStatus )
      {
         _resultCode = SDB_TASK_ROLLBACK ;
      }
      else
      {
         _resultCode = resultCode ;
      }

      resetOpInfo() ;

      {
         ossScopedLock lock( &_latch, EXCLUSIVE ) ;

         _buildResultInfo( resultCode, resultDetail, wResultDetail ) ;

         ossGetCurrentTime( _endTimestamp ) ;
         _taskStatus = DMS_TASK_STATUS_FINISH ; // set before updateProgress()
      }
      updateProgress() ;
   }

   const ossTimestamp& _dmsIdxTaskStatus::beginTimestamp() const
   {
      return _beginTimestamp ;
   }

   const ossTimestamp& _dmsIdxTaskStatus::endTimestamp() const
   {
      return _endTimestamp ;
   }

   BOOLEAN _dmsIdxTaskStatus::hasTaskInCatalog()
   {
      return _hasCatalogTask ;
   }

   void _dmsIdxTaskStatus::setHasTaskInCatalog( BOOLEAN has )
   {
      _hasCatalogTask = has ;
   }

   void _dmsIdxTaskStatus::setOpInfo( DMS_OPINFO_TYPE infoType )
   {
      _opInfo = infoType ;
   }

   void _dmsIdxTaskStatus::resetOpInfo()
   {
      _opInfo = OPINFO_UNKNOWN ;
   }

   BOOLEAN _dmsIdxTaskStatus::isGlobalIdx() const
   {
      return _isGlobalIdx ;
   }

   INT32 _dmsIdxTaskStatus::globalIdxCL( const CHAR *&clName,
                                         UINT64 &clUniqID ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele = _indexDef.getField( IXM_FIELD_NAME_GLOBAL_OPTION ) ;
         if ( Object == ele.type() )
         {
            BSONObj option = ele.embeddedObject() ;

            ele = option.getField( FIELD_NAME_COLLECTION ) ;
            if ( String == ele.type() )
            {
               clName = ele.valuestr() ;
            }

            ele = option.getField( FIELD_NAME_CL_UNIQUEID ) ;
            if ( ele.isNumber() )
            {
               clUniqID = ele.numberLong() ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      return rc ;
   }

   const static BSONObj emptyObj ;

   const BSONObj& _dmsIdxTaskStatus::indexDef() const
   {
      if ( _hasSetDef )
      {
         return _indexDef ;
      }
      else
      {
         return emptyObj ; ;
      }
   }

   INT32 _dmsIdxTaskStatus::setIndexDef( const BSONObj &indexDef )
   {
      INT32 rc = SDB_OK ;

      if ( !_hasSetDef )
      {
         try
         {
            _indexDef = indexDef.getOwned() ;
            _hasSetDef = TRUE ;
            _isGlobalIdx = _indexDef[ IXM_GLOBAL_FIELD ].trueValue() ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTASKMGR_CRT, "_dmsTaskStatusMgr::createIdxItem" )
   INT32 _dmsTaskStatusMgr::createIdxItem( DMS_TASK_TYPE type,
                                           dmsIdxTaskStatusPtr& statusPtr,
                                           UINT64 taskID,
                                           UINT32 locationID,
                                           UINT64 mainTaskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTASKMGR_CRT ) ;
      SDB_ASSERT( type == DMS_TASK_CREATE_IDX || type == DMS_TASK_DROP_IDX,
                  "Invalid task type" ) ;

      // When it is a standalone node, or data.createIndex(xxx), there is no
      // catalog task id on standalone node. When taskID is invalid, we should
      // generate a dummy task id.
      if ( DMS_INVALID_TASKID == taskID )
      {
         ossScopedLock lock ( &_hwmLatch, EXCLUSIVE ) ;

         // Dummy task id is in range of ( min, max ]
         if ( _dummyTaskHWM < DMS_DUMMY_CATTASKID_MIN ||
              _dummyTaskHWM >= DMS_DUMMY_CATTASKID_MAX )
         {
            _dummyTaskHWM = DMS_DUMMY_CATTASKID_MIN ;
         }
         _dummyTaskHWM++ ;
         taskID = _dummyTaskHWM ;
      }

      // new status
      _dmsIdxTaskStatus* pItem = SDB_OSS_NEW _dmsIdxTaskStatus( type,
                                                                taskID,
                                                                locationID,
                                                                mainTaskID ) ;
      if ( NULL == pItem )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Fail to allocate memory" ) ;
         goto error ;
      }

      // add to map
      try
      {
         ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

         MAP_IDSTATUS_IT it = _mapStatus.find( taskID ) ;
         if ( it == _mapStatus.end() )
         {
            try
            {
               statusPtr = dmsIdxTaskStatusPtr( pItem ) ;
            }
            catch( std::exception &e )
            {  
               pItem = NULL ;
               rc = ossException2RC( &e ) ;
               PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
            }
            // shared_ptr takes over pItem's memory
            pItem = NULL ;
            _mapStatus[taskID] = statusPtr ;
         }
         else
         {
            statusPtr = boost::dynamic_pointer_cast<dmsIdxTaskStatus>(it->second) ;
            statusPtr->setLocationID( locationID ) ;
            SDB_OSS_DEL pItem ;
            pItem = NULL ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_DMSTASKMGR_CRT, rc ) ;
      return rc ;
   error:
      if ( pItem )
      {
         SDB_OSS_DEL pItem ;
      }
      goto done ;
   }

   BOOLEAN _dmsTaskStatusMgr::findItem( UINT64 taskID,
                                        dmsTaskStatusPtr& statusPtr )
   {
      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      MAP_IDSTATUS_IT it = _mapStatus.find( taskID ) ;
      if ( it == _mapStatus.end() )
      {
         return FALSE ;
      }
      else
      {
         statusPtr = it->second ;
         return TRUE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTASKMGR_DUMP, "_dmsTaskStatusMgr::dumpInfo" )
   INT32 _dmsTaskStatusMgr::dumpInfo( ossPoolMap<UINT64, BSONObj>& statusMap )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTASKMGR_DUMP ) ;

      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      MAP_IDSTATUS_IT it ;
      for ( it = _mapStatus.begin() ; it != _mapStatus.end() ; ++it )
      {
         dmsTaskStatusPtr statusPtr = it->second ;
         try
         {
            statusMap.insert( std::pair<UINT64, BSONObj>( it->first,
                              statusPtr->toBSON( DMS_TASK_MASK_SNAPSHOT ) ) ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB_DMSTASKMGR_DUMP ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTASKMGR_DUMPREP, "_dmsTaskStatusMgr::dumpForReport" )
   INT32 _dmsTaskStatusMgr::dumpForReport( ossPoolMap<UINT64, BSONObj>& statusMap )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_DMSTASKMGR_DUMPREP ) ;

      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      MAP_IDSTATUS_IT it ;
      for ( it = _mapStatus.begin() ; it != _mapStatus.end() ; ++it )
      {
         dmsTaskStatusPtr statusPtr = it->second ;

         if ( DMS_TASK_STATUS_READY == statusPtr->status() ||
              !statusPtr->hasTaskInCatalog() ||
              statusPtr->pauseReport() )
         {
            // If this task hasn't run yet, or this task doesn't have a
            // corresponding task on catalog, we needn't report its progress
            // to catalog.
            continue ;
         }

         statusPtr->updateProgress() ;

         try
         {
            statusMap.insert( std::pair<UINT64, BSONObj>( it->first,
                              statusPtr->toBSON( DMS_TASK_MASK_REPORT ) ) ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB_DMSTASKMGR_DUMPREP ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsTaskStatusMgr::hasTaskToReport()
   {
      ossScopedLock lock ( &_mapLatch, SHARED ) ;

      for ( MAP_IDSTATUS_IT it = _mapStatus.begin() ;
            it != _mapStatus.end() ; ++it )
      {
         // DON'T check READY status like dumpForReport(). Because if status is
         // READY, it will soon switch to RUN status, and need to be reported.
         if ( it->second->hasTaskInCatalog() )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSTASKMGR_CLEAN, "_dmsTaskStatusMgr::cleanOutOfDate" )
   void _dmsTaskStatusMgr::cleanOutOfDate( BOOLEAN isPrimary )
   {
      PD_TRACE_ENTRY( SDB_DMSTASKMGR_CLEAN ) ;

      /*
      * The primary node may be switched during index creation, so we need to
      * known current node is primary or slave.
      */
      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      MAP_IDSTATUS_IT it = _mapStatus.begin() ;
      while ( it != _mapStatus.end() )
      {
         dmsTaskStatusPtr statPtr = it->second ;
         if ( isPrimary && statPtr->hasTaskInCatalog() )
         {
            // Clean it after the corresponding catalog task was finished.
            // When catalog task is canceled or rollback, current local task
            // need to rollback.
            it++ ;
            continue ;
         }
         if ( DMS_TASK_STATUS_FINISH == statPtr->status() )
         {
            ossTimestamp curTS ;
            ossGetCurrentTime( curTS ) ;
            if ( curTS.time - statPtr->endTimestamp().time >
                                                    DMS_TASK_EXPIRED_TIME )
            {
               PD_LOG( PDINFO, "Clean up expired task[%s]",
                       statPtr->toBSON().toString().c_str() ) ;
               _mapStatus.erase( it++ ) ;
               continue ;
            }
         }
         it++ ;
      }

      PD_TRACE_EXIT( SDB_DMSTASKMGR_CLEAN ) ;
   }

   void _dmsTaskStatusMgr::renameCS( const CHAR* oldCSName,
                                     const CHAR* newCSName )
   {
      INT32 oldCSLen = ossStrlen( oldCSName ) ;

      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      for ( MAP_IDSTATUS_IT it = _mapStatus.begin() ;
            it != _mapStatus.end() ; ++it )
      {
         dmsTaskStatusPtr statPtr = it->second ;
         const CHAR* clFullName = statPtr->collectionName() ;

         // if it is in the collection space
         if ( 0 == ossStrncmp( clFullName, oldCSName, oldCSLen ) &&
              clFullName[oldCSLen] == '.' )
         {
            // build new collection name
            CHAR newCLFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
            ossStrcpy( newCLFullName, newCSName ) ;
            ossStrncat( newCLFullName, clFullName + oldCSLen,
                        ossStrlen( clFullName ) - oldCSLen ) ;
            // rename
            statPtr->collectionRename( newCLFullName ) ;
         }
      }
   }

   void _dmsTaskStatusMgr::renameCL( const CHAR* oldCLName,
                                     const CHAR* newCLName )
   {
      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      for ( MAP_IDSTATUS_IT it = _mapStatus.begin() ;
            it != _mapStatus.end() ; ++it )
      {
         dmsTaskStatusPtr statPtr = it->second ;
         if ( 0 == ossStrcmp( statPtr->collectionName(), oldCLName ) )
         {
            statPtr->collectionRename( newCLName ) ;
         }
      }
   }

   void _dmsTaskStatusMgr::dropCS( const CHAR* csName )
   {
      INT32 csLen = ossStrlen( csName ) ;

      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      MAP_IDSTATUS_IT it = _mapStatus.begin() ;
      while ( it != _mapStatus.end() )
      {
         const CHAR* clFullName = it->second->collectionName() ;

         // if it is in the collection space
         if ( 0 == ossStrncmp( clFullName, csName, csLen ) &&
              clFullName[csLen] == '.' )
         {
            it->second->setStatus( DMS_TASK_STATUS_CANCELED ) ;
            _mapStatus.erase( it++ ) ;
         }
         else
         {
            it++ ;
         }
      }
   }

   void _dmsTaskStatusMgr::dropCL( const CHAR* collection )
   {
      ossScopedLock lock ( &_mapLatch, EXCLUSIVE ) ;

      MAP_IDSTATUS_IT it = _mapStatus.begin() ;
      while ( it != _mapStatus.end() )
      {
         if ( 0 == ossStrcmp( it->second->collectionName(), collection ) )
         {
            it->second->setStatus( DMS_TASK_STATUS_CANCELED ) ;
            _mapStatus.erase( it++ ) ;
         }
         else
         {
            it++ ;
         }
      }
   }
}

