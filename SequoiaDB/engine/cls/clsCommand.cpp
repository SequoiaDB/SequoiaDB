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

   Source File Name = clsCommand.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/27/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsCommand.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtn.hpp"
#include "rtnContextAlter.hpp"
#include "msgMessageFormat.hpp"

using namespace bson ;

// Default size of capped collection for text index. The unit is MB. So its 30G.
#define TEXT_INDEX_DATA_BUFF_DEFAULT_SIZE  ( 30 * 1024 )

namespace engine
{

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSplit)
   _rtnSplit::_rtnSplit ()
   {
      ossMemset ( _szCollection, 0, sizeof(_szCollection) ) ;
      ossMemset ( _szTargetName, 0, sizeof(_szTargetName) ) ;
      ossMemset ( _szSourceName, 0, sizeof(_szSourceName) ) ;
      _percent = 0.0 ;
   }

   INT32 _rtnSplit::spaceService ()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   _rtnSplit::~_rtnSplit ()
   {
   }

   const CHAR *_rtnSplit::name ()
   {
      return NAME_SPLIT ;
   }

   RTN_COMMAND_TYPE _rtnSplit::type ()
   {
      return CMD_SPLIT ;
   }

   BOOLEAN _rtnSplit::writable ()
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLIT_INIT, "_rtnSplit::init" )
   INT32 _rtnSplit::init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR * pMatcherBuff,
                           const CHAR * pSelectBuff,
                           const CHAR * pOrderByBuff,
                           const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLIT_INIT ) ;
      const CHAR *pCollectionName = NULL ;
      const CHAR *pTargetName     = NULL ;
      const CHAR *pSourceName     = NULL ;

      try
      {
         BSONObj boRequest ( pMatcherBuff ) ;
         BSONElement beName       = boRequest.getField ( CAT_COLLECTION_NAME ) ;
         BSONElement beTarget     = boRequest.getField ( CAT_TARGET_NAME ) ;
         BSONElement beSplitKey   = boRequest.getField ( CAT_SPLITVALUE_NAME ) ;
         BSONElement beSource     = boRequest.getField ( CAT_SOURCE_NAME ) ;
         BSONElement bePercent    = boRequest.getField ( CAT_SPLITPERCENT_NAME ) ;

         // validate collection name and read
         PD_CHECK ( !beName.eoo() && beName.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Invalid collection name: %s", beName.toString().c_str() ) ;
         pCollectionName = beName.valuestr() ;
         PD_CHECK ( ossStrlen ( pCollectionName ) <
                       DMS_COLLECTION_SPACE_NAME_SZ +
                       DMS_COLLECTION_NAME_SZ + 1,
                    SDB_INVALIDARG, error, PDERROR,
                    "Collection name is too long: %s", pCollectionName ) ;
         ossStrncpy ( _szCollection, pCollectionName,
                         DMS_COLLECTION_SPACE_NAME_SZ +
                          DMS_COLLECTION_NAME_SZ + 1 ) ;
         // validate target name and read
         PD_CHECK ( !beTarget.eoo() && beTarget.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Invalid target group name: %s",
                    beTarget.toString().c_str() ) ;
         pTargetName = beTarget.valuestr() ;
         PD_CHECK ( ossStrlen ( pTargetName ) < OP_MAXNAMELENGTH,
                    SDB_INVALIDARG, error, PDERROR,
                    "target group name is too long: %s",
                    pTargetName ) ;
         ossStrncpy ( _szTargetName, pTargetName, OP_MAXNAMELENGTH ) ;
         // validate source name and read
         PD_CHECK ( !beSource.eoo() && beSource.type() == String,
                    SDB_INVALIDARG, error, PDERROR,
                    "Invalid source group name: %s",
                    beSource.toString().c_str() ) ;
         pSourceName = beSource.valuestr() ;
         PD_CHECK ( ossStrlen ( pSourceName ) < OP_MAXNAMELENGTH,
                    SDB_INVALIDARG, error, PDERROR,
                    "source group name is too long: %s",
                    pSourceName ) ;
         ossStrncpy ( _szSourceName, pSourceName, OP_MAXNAMELENGTH ) ;
         // read split key
         PD_CHECK ( !beSplitKey.eoo() && beSplitKey.type() == Object,
                    SDB_INVALIDARG, error, PDERROR,
                    "Invalid split key: %s",
                    beSplitKey.toString().c_str() ) ;
         _splitKey = beSplitKey.embeddedObject () ;
         // percent
         _percent = bePercent.numberDouble() ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception handled when parsing split request: %s",
                       e.what() ) ;
      }
      PD_TRACE4 ( SDB__CLSSPLIT_INIT,
                  PD_PACK_STRING ( pCollectionName ),
                  PD_PACK_STRING ( pTargetName ),
                  PD_PACK_STRING ( pSourceName ),
                  PD_PACK_BSON ( _splitKey ) ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSPLIT_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLIT_DOIT, "_rtnSplit::doit" )
   INT32 _rtnSplit::doit ( _pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                           _SDB_RTNCB * rtnCB, _dpsLogWrapper * dpsCB,
                           INT16 w, INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLIT_DOIT ) ;
      BSONObjBuilder matchBuilder ;

      matchBuilder.append( CAT_TASKTYPE_NAME, CLS_TASK_SPLIT ) ;
      matchBuilder.append( CAT_COLLECTION_NAME, _szCollection ) ;
      matchBuilder.append( CAT_TARGET_NAME, _szTargetName ) ;
      matchBuilder.append( CAT_SOURCE_NAME, _szSourceName ) ;
      if ( _splitKey.isEmpty() )
      {
         matchBuilder.append( CAT_SPLITPERCENT_NAME, _percent  ) ;
      }
      else
      {
         matchBuilder.append( CAT_SPLITVALUE_NAME, _splitKey ) ;
      }
      BSONObj match = matchBuilder.obj() ;

      rc = sdbGetClsCB()->startTaskCheck ( match, TRUE ) ;
      PD_TRACE_EXITRC ( SDB__CLSSPLIT_DOIT, rc ) ;
      return rc ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCreateIndex)
   _rtnCreateIndex::_rtnCreateIndex ()
   : _collectionName ( NULL ),
     _indexName( NULL ),
     _sortBufSize ( SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ),
     _textIdx( FALSE ),
     _isGlobal( FALSE ),
     _taskID( CLS_INVALID_TASKID ),
     _isAsync( FALSE ),
     _isStandaloneIdx( FALSE )
   {
   }

   _rtnCreateIndex::~_rtnCreateIndex ()
   {
   }

   const CHAR *_rtnCreateIndex::name ()
   {
      return NAME_CREATE_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnCreateIndex::type ()
   {
      return CMD_CREATE_INDEX ;
   }

   BOOLEAN _rtnCreateIndex::writable ()
   {
      return _isStandaloneIdx ? FALSE : TRUE ; ;
   }

   const CHAR *_rtnCreateIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCREATEINDEX_INIT, "_rtnCreateIndex::init" )
   INT32 _rtnCreateIndex::init ( INT32 flags, INT64 numToSkip,
                                 INT64 numToReturn,
                                 const CHAR * pMatcherBuff,
                                 const CHAR * pSelectBuff,
                                 const CHAR * pOrderByBuff,
                                 const CHAR * pHintBuff)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCREATEINDEX_INIT ) ;
      BSONObj arg ( pMatcherBuff ) ;
      BSONObj hint ( pHintBuff ) ;
      BOOLEAN hasSortBufSz = FALSE ;

      // get collectio name
      rc = rtnGetStringElement( arg, FIELD_NAME_COLLECTION, &_collectionName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get string [collection] " ) ;
         goto error ;
      }

      // get index definition
      rc = rtnGetObjElement( arg, FIELD_NAME_INDEX, _index ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get object [Index] " ) ;
         goto error ;
      }

      rc = rtnCheckAndConvertIndexDef( _index ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to convert index definition: %s",
                  _index.toString().c_str() ) ;
         goto error ;
      }

      rc = _validateDef( _index ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to validate index definition: %s, rc: %d",
                  _index.toString().c_str(), rc ) ;
         goto error ;
      }

      // get global
      rc = rtnGetBooleanElement( _index, IXM_FIELD_NAME_GLOBAL,
                                 _isGlobal ) ;
      rc = SDB_FIELD_NOT_EXIST == rc ? SDB_OK : rc ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field(%s):index=%s,rc=%d",
                   IXM_FIELD_NAME_GLOBAL, _index.toString().c_str(), rc ) ;

      // get standalone
      rc = rtnGetBooleanElement( _index, IXM_FIELD_NAME_STANDALONE,
                                 _isStandaloneIdx ) ;
      rc = SDB_FIELD_NOT_EXIST == rc ? SDB_OK : rc ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field(%s):index=%s,rc=%d",
                   IXM_FIELD_NAME_STANDALONE, _index.toString().c_str(), rc ) ;

      // get index name
      rc = rtnGetStringElement( _index, IXM_FIELD_NAME_NAME, &_indexName ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get string [name] " ) ;
         goto error ;
      }

      // get sort buffer size
      if ( arg.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         hasSortBufSz = TRUE ;
         rc = rtnGetIntElement( arg, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufSize ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "%s should be number",
                        IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
            goto error ;
         }
      }
      else if ( hint.hasField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
      {
         hasSortBufSz = TRUE ;
         rc = rtnGetIntElement( hint, IXM_FIELD_NAME_SORT_BUFFER_SIZE,
                                _sortBufSize ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "%s should be number",
                        IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
            goto error ;
         }
      }
      if ( _sortBufSize < 0 )
      {
         PD_LOG_MSG( PDERROR, "'%s' invalid: %d",
                     IXM_FIELD_NAME_SORT_BUFFER_SIZE, _sortBufSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !hasSortBufSz )
      {
         // For text index, the "sort buffer size" is actually used as the 'Size'
         // option for the corresponding capped collection.
         if ( _textIdx )
         {
            _sortBufSize = TEXT_INDEX_DATA_BUFF_DEFAULT_SIZE ;
         }
      }

      // get task id
      rc = rtnGetNumberLongElement( hint, FIELD_NAME_TASKID, (INT64&)_taskID ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get field[%s] from hint[%s]",
                  FIELD_NAME_TASKID, hint.toString().c_str() ) ;
         goto error ;
      }

      // get async
      rc = rtnGetBooleanElement( arg, FIELD_NAME_ASYNC, _isAsync ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get field[%s] from match[%s]",
                  FIELD_NAME_ASYNC, arg.toString().c_str() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSCREATEINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCREATEINDEX_DOIT, "_rtnCreateIndex::doit" )
   INT32 _rtnCreateIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                                 SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                                 INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCREATEINDEX_DOIT ) ;

      // standalone/ data node does not support aync
      if ( _isAsync && ( CMD_SPACE_SERVICE_SHARD != getFromService() ) )
      {
         rc = SDB_OPERATION_INCOMPATIBLE ;
         PD_LOG_MSG( PDERROR, "Async is only supported in cluster" ) ;
         goto error ;
      }

      // Currently only support text index in cluster.
      if ( _textIdx && ( CMD_SPACE_SERVICE_SHARD != getFromService() ) )
      {
         PD_LOG_MSG( PDERROR, "Text index is only supported in cluster" ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      // Currently only support global index in cluster.
      if ( _isGlobal && ( CMD_SPACE_SERVICE_SHARD != getFromService() ) )
      {
         PD_LOG_MSG( PDERROR, "Global index is only supported in cluster" ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      /// create consistent index by coord
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() && !_isStandaloneIdx )
      {
         SDB_ASSERT( _taskID != CLS_INVALID_TASKID, "task id is invalid" ) ;

         rc = sdbGetClsCB()->startIdxTaskCheck( _taskID, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to start task check, rc: %d",
                      rc ) ;
      }
      /// create standalone index by coord, or
      /// create index by data or standalone node
      else
      {
         if ( CMD_SPACE_SERVICE_SHARD == getFromService() && _isStandaloneIdx )
         {
            dpsCB = NULL ;
         }

         BOOLEAN sysCall = pmdGetOptionCB()->authEnabled() ? FALSE : TRUE ;
         dmsTaskStatusMgr *pStatMgr = rtnCB->getTaskStatusMgr() ;
         dmsIdxTaskStatusPtr statusPtr ;

         rc = pStatMgr->createIdxItem( DMS_TASK_CREATE_IDX, statusPtr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create task status, rc: %d",
                      rc ) ;

         rc = statusPtr->init( _collectionName, _index, _sortBufSize ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initialize task status, rc: %d",
                      rc ) ;

         statusPtr->setStatus( DMS_TASK_STATUS_RUN ) ;

         rc = rtnCreateIndexCommand( _collectionName, _index,
                                     cb, dmsCB, dpsCB, sysCall, _sortBufSize,
                                     &_writeResult, statusPtr.get() ) ;
         statusPtr->setStatus2Finish( rc, cb ? cb->getInfo(EDU_INFO_ERROR) :
                                               NULL, &_writeResult ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create index[%s] for collection[%s], rc: %d",
                      _indexName, _collectionName, rc ) ;
      }

   done:
      // audit
      if ( SDB_OK == rc && CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc,
                           "IndexDef:%s, SortBuffSize:%d, Async:%s",
                           _index.toString().c_str(), _sortBufSize,
                           _isAsync ? "true" : "false" ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSCREATEINDEX_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Check if there is mixed use of normal index and text index.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCREATEINDEX__VALIDATEDEF, "_rtnCreateIndex::_validateDef" )
   INT32 _rtnCreateIndex::_validateDef( const BSONObj &index )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSCREATEINDEX__VALIDATEDEF ) ;

      BOOLEAN hasText = FALSE ;
      const string textFieldVal = "text" ;
      BSONObj idxDef = index.getObjectField( IXM_FIELD_NAME_KEY ) ;
      BSONObjIterator itr( idxDef ) ;

      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;
         if ( ele.eoo() )
         {
            PD_LOG( PDERROR, "Index definition ended unexpected. "
                    "Definition: %s", idxDef.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( String == ele.type() && textFieldVal == ele.String() )
         {
            hasText = TRUE ;
         }
         else
         {
            if ( hasText )
            {
               PD_LOG( PDERROR, "Text index can only contain fields specified "
                       "as text. Definition: %s", idxDef.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }

      _textIdx = hasText ;

   done:
      PD_TRACE_EXITRC( SDB__CLSCREATEINDEX__VALIDATEDEF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnDropIndex)
   _rtnDropIndex::_rtnDropIndex ()
   :_collectionName ( NULL ),
    _indexName( NULL ),
    _taskID( CLS_INVALID_TASKID ),
    _isAsync( FALSE ),
    _isStandaloneIdx( FALSE )
   {
   }

   _rtnDropIndex::~_rtnDropIndex ()
   {
   }

   const CHAR *_rtnDropIndex::name ()
   {
      return NAME_DROP_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnDropIndex::type ()
   {
      return CMD_DROP_INDEX ;
   }

   const CHAR *_rtnDropIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   BOOLEAN _rtnDropIndex::writable ()
   {
      return _isStandaloneIdx ? FALSE : TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDROPINDEX_INIT, "_rtnDropIndex::init" )
   INT32 _rtnDropIndex::init ( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR *pMatcherBuff,
                               const CHAR *pSelectBuff,
                               const CHAR *pOrderByBuff,
                               const CHAR *pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__CLSDROPINDEX_INIT ) ;

      BSONElement ele ;
      BSONObj matcher( pMatcherBuff ) ;
      BSONObj hint( pHintBuff ) ;

      // get collection
      INT32 rc = rtnGetStringElement ( matcher, FIELD_NAME_COLLECTION,
                                       &_collectionName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get string[collection]" ) ;
         goto error ;
      }

      // get index
      rc = rtnGetObjElement ( matcher, FIELD_NAME_INDEX, _index ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to get index object " ) ;
         goto error ;
      }

      ele = _index.firstElement() ;
      if ( ele.type() != String )
      {
         PD_LOG ( PDERROR, "Invalid index obj[%s]",
                  _index.toString().c_str() ) ;
         goto error ;
      }
      _indexName = ele.valuestr() ;

      // get async
      rc = rtnGetBooleanElement( matcher, FIELD_NAME_ASYNC, _isAsync ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get field[%s] from matcher[%s]",
                  FIELD_NAME_ASYNC, matcher.toString().c_str() ) ;
         goto error ;
      }

      // get task id
      rc = rtnGetNumberLongElement( hint, FIELD_NAME_TASKID,
                                    (INT64&)_taskID ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get field[%s] from hint[%s]",
                  FIELD_NAME_TASKID, hint.toString().c_str() ) ;
         goto error ;
      }

      // get standalone
      rc = rtnGetBooleanElement( hint, FIELD_NAME_STANDALONE,
                                 _isStandaloneIdx ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get field[%s] from hint[%s]",
                  FIELD_NAME_STANDALONE, hint.toString().c_str() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSDROPINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDROPINDEX_DOIT, "_rtnDropIndex::doit" )
   INT32 _rtnDropIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                               SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                               INT16 w , INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDROPINDEX_DOIT ) ;

      // standalone/ data node does not support aync
      if ( _isAsync && ( CMD_SPACE_SERVICE_SHARD != getFromService() ) )
      {
         PD_LOG_MSG( PDERROR, "Async is only supported in cluster" ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      /// drop consistent index by coord
      if ( CMD_SPACE_SERVICE_SHARD == getFromService() && !_isStandaloneIdx )
      {
         SDB_ASSERT( _taskID != CLS_INVALID_TASKID, "task id is invalid" ) ;

         rc = sdbGetClsCB()->startIdxTaskCheck( _taskID, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to start task check, rc: %d",
                      rc ) ;
      }
      /// drop standalone index by coord, or
      /// drop index by data or standalone node
      else
      {
         BOOLEAN onlyStandalone = FALSE ;
         if ( CMD_SPACE_SERVICE_SHARD == getFromService() && _isStandaloneIdx )
         {
            dpsCB = NULL ;
            onlyStandalone = TRUE ;
         }

         BOOLEAN sysCall = pmdGetOptionCB()->authEnabled() ? FALSE : TRUE ;
         BSONElement indexEle = _index.firstElement() ;
         dmsTaskStatusMgr *pStatMgr = rtnCB->getTaskStatusMgr() ;
         dmsIdxTaskStatusPtr statusPtr ;

         rc = pStatMgr->createIdxItem( DMS_TASK_DROP_IDX, statusPtr ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to create task status, rc: %d",
                      rc ) ;

         rc = statusPtr->init( _collectionName, BSON( "" << _indexName ) ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initialize task status, rc: %d",
                      rc ) ;

         statusPtr->setStatus( DMS_TASK_STATUS_RUN ) ;

         rc = rtnDropIndexCommand( _collectionName, indexEle,
                                   cb, dmsCB, dpsCB, sysCall,
                                   statusPtr.get(), onlyStandalone ) ;
         statusPtr->setStatus2Finish( rc, cb ? cb->getInfo(EDU_INFO_ERROR) :
                                               NULL ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to drop index[%s] for collection[%s], rc: %d",
                      _indexName, _collectionName, rc ) ;
      }

   done:
      // audit
      if ( SDB_OK == rc && CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), AUDIT_OBJ_CL,
                           _collectionName, rc, "IndexDef:%s, Async:%s",
                           _index.toString().c_str(),
                           _isAsync ? "true" : "false" ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSDROPINDEX_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCopyIndex)
   _rtnCopyIndex::_rtnCopyIndex () :_collectionName ( NULL )
   {
   }

   _rtnCopyIndex::~_rtnCopyIndex ()
   {
   }

   const CHAR *_rtnCopyIndex::name ()
   {
      return NAME_COPY_INDEX ;
   }

   RTN_COMMAND_TYPE _rtnCopyIndex::type ()
   {
      return CMD_COPY_INDEX ;
   }

   const CHAR *_rtnCopyIndex::collectionFullName ()
   {
      return _collectionName ;
   }

   BOOLEAN _rtnCopyIndex::writable ()
   {
      return TRUE ;
   }

   INT32 _rtnCopyIndex::spaceNode ()
   {
      return CMD_SPACE_NODE_DATA ;
   }

   INT32 _rtnCopyIndex::spaceService ()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCYINDEX_INIT, "_rtnCopyIndex::init" )
   INT32 _rtnCopyIndex::init ( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR *pMatcherBuff,
                               const CHAR *pSelectBuff,
                               const CHAR *pOrderByBuff,
                               const CHAR *pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__CLSCYINDEX_INIT ) ;

      INT32 rc = SDB_OK ;

      BSONObj matcher( pMatcherBuff ) ;

      rc = rtnGetStringElement( matcher, FIELD_NAME_NAME, &_collectionName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s] from matcher[%s], rc: %d",
                   FIELD_NAME_NAME, matcher.toString().c_str(), rc ) ;
      // extract other field in copyIndexOnMainCL()

   done:
      PD_TRACE_EXITRC ( SDB__CLSCYINDEX_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCopyIndex::doit ( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                               SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                               INT16 w , INT64 *pContextID )
   {
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnCancelTask)
   INT32 _rtnCancelTask::spaceNode ()
   {
      return CMD_SPACE_NODE_ALL & ( ~CMD_SPACE_NODE_STANDALONE ) ;
   }

   INT32 _rtnCancelTask::init( INT32 flags, INT64 numToSkip,
                               INT64 numToReturn,
                               const CHAR * pMatcherBuff,
                               const CHAR * pSelectBuff,
                               const CHAR * pOrderByBuff,
                               const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement ele = matcher.getField( CAT_TASKID_NAME ) ;
         if ( ele.eoo() || !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Field[%s] type[%d] is error", CAT_TASKID_NAME,
                    ele.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _taskID = (UINT64)ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCancelTask::doit( pmdEDUCB * cb, SDB_DMSCB * dmsCB,
                               SDB_RTNCB * rtnCB, SDB_DPSCB * dpsCB,
                               INT16 w, INT64 *pContextID )
   {
      PD_LOG( PDEVENT, "Stop task[ID=%lld]", _taskID ) ;
      return sdbGetClsCB()->stopTask( _taskID ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnLinkCollection)
   _rtnLinkCollection::_rtnLinkCollection()
   {
      _collectionName = NULL ;
      _subCLName      = NULL ;
   }

   _rtnLinkCollection::~_rtnLinkCollection()
   {
   }

   INT32 _rtnLinkCollection::spaceService ()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   const CHAR * _rtnLinkCollection::name ()
   {
      return NAME_LINK_COLLECTION;
   }

   RTN_COMMAND_TYPE _rtnLinkCollection::type ()
   {
      return CMD_LINK_COLLECTION ;
   }

   const CHAR *_rtnLinkCollection::collectionFullName ()
   {
      return _collectionName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSLINKCL_INIT, "_rtnLinkCollection::init" )
   INT32 _rtnLinkCollection::init ( INT32 flags, INT64 numToSkip,
                                    INT64 numToReturn,
                                    const CHAR * pMatcherBuff,
                                    const CHAR * pSelectBuff,
                                    const CHAR * pOrderByBuff,
                                    const CHAR * pHintBuff )
   {
      PD_TRACE_ENTRY ( SDB__CLSLINKCL_INIT ) ;
      INT32 rc = SDB_OK;
      try
      {
         BSONObj arg ( pMatcherBuff ) ;
         rc = rtnGetStringElement ( arg, FIELD_NAME_NAME,
                                    &_collectionName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to get string[%s], rc: %d",
                     FIELD_NAME_NAME, rc ) ;
            goto error ;
         }
         rc = rtnGetStringElement( arg, FIELD_NAME_SUBCLNAME,
                                   &_subCLName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get string [%s], rc: %d",
                    FIELD_NAME_SUBCLNAME, rc ) ;
            goto error ;
         }

      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSLINKCL_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLinkCollection::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                    INT16 w, INT64 *pContextID )
   {
      /// need to update sub collection catalog info
      INT32 rc = sdbGetShardCB()->syncUpdateCatalog( _subCLName ) ;
      if ( rc )
      {
         catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
         pCatAgent->lock_w() ;
         pCatAgent->clear( _subCLName ) ;
         pCatAgent->release_w() ;
      }

      // Clear cached main-collection plans
      rtnCB->getAPM()->invalidateCLPlans( _collectionName ) ;

      // Tell secondary nodes to clear catalog and plan caches
      sdbGetClsCB()->invalidateCache ( _collectionName,
                                       DPS_LOG_INVALIDCATA_TYPE_CATA |
                                       DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      sdbGetClsCB()->invalidateCata( _subCLName ) ;
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnUnlinkCollection)
   _rtnUnlinkCollection::_rtnUnlinkCollection()
   {
   }

   _rtnUnlinkCollection::~_rtnUnlinkCollection()
   {
   }

   const CHAR * _rtnUnlinkCollection::name ()
   {
      return NAME_UNLINK_COLLECTION ;
   }

   RTN_COMMAND_TYPE _rtnUnlinkCollection::type ()
   {
      return CMD_UNLINK_COLLECTION ;
   }

   INT32 _rtnUnlinkCollection::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                      _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                      INT16 w, INT64 *pContextID )
   {
      /// need to update sub collection catalog info
      catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
      INT32 rc = sdbGetShardCB()->syncUpdateCatalog( _subCLName ) ;
      if ( rc )
      {
         pCatAgent->lock_w() ;
         pCatAgent->clear( _subCLName ) ;
         pCatAgent->release_w() ;
      }

      /// clear main catalog info
      pCatAgent->lock_w() ;
      pCatAgent->clear( _collectionName ) ;
      pCatAgent->release_w() ;

      // Clear cached main-collection plans
      rtnCB->getAPM()->invalidateCLPlans( _collectionName ) ;

      // Tell secondary nodes to clear catalog and plan caches
      sdbGetClsCB()->invalidateCache( _collectionName,
                                      DPS_LOG_INVALIDCATA_TYPE_CATA |
                                      DPS_LOG_INVALIDCATA_TYPE_PLAN ) ;
      sdbGetClsCB()->invalidateCata( _subCLName ) ;
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnInvalidateCache)
   _rtnInvalidateCache::_rtnInvalidateCache()
   {

   }

   _rtnInvalidateCache::~_rtnInvalidateCache()
   {

   }

   INT32 _rtnInvalidateCache::spaceNode()
   {
      return CMD_SPACE_NODE_DATA | CMD_SPACE_NODE_CATA  ;
   }

   INT32 _rtnInvalidateCache::init ( INT32 flags,
                                     INT64 numToSkip,
                                     INT64 numToReturn,
                                     const CHAR *pMatcherBuff,
                                     const CHAR *pSelectBuff,
                                     const CHAR *pOrderByBuff,
                                     const CHAR *pHintBuff )
   {
      return SDB_OK ;
   }

   INT32 _rtnInvalidateCache::doit ( _pmdEDUCB *cb,
                                     SDB_DMSCB *dmsCB,
                                     _SDB_RTNCB *rtnCB,
                                     _dpsLogWrapper *dpsCB,
                                     INT16 w,
                                     INT64 *pContextID )
   {
      sdbGetShardCB()->getCataAgent()->lock_w() ;
      sdbGetShardCB()->getCataAgent()->clearAll() ;
      sdbGetShardCB()->getCataAgent()->release_w() ;

      sdbGetShardCB()->getNodeMgrAgent()->lock_w() ;
      sdbGetShardCB()->getNodeMgrAgent()->clearAll() ;
      sdbGetShardCB()->getNodeMgrAgent()->release_w() ;

      return  SDB_OK ;
   }

   _rtnReelectBase::_rtnReelectBase()
   :_timeout( CLS_REELECT_COMMAND_TIMEOUT_DFT ),
    _level( CLS_REELECTION_LEVEL_3 )
   {
      _nodeID = 0 ;
      _isDestNotify = FALSE ;
   }

   _rtnReelectBase::~_rtnReelectBase()
   {

   }

   INT32 _rtnReelectBase::spaceNode()
   {
      return CMD_SPACE_NODE_DATA | CMD_SPACE_NODE_CATA  ;
   }

   INT32 _rtnReelectBase::spaceService()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREELECT_PARSEARGS, "_rtnReelectBase::_parseReelectArgs" )
   INT32 _rtnReelectBase::_parseReelectArgs( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECT_PARSEARGS ) ;

      BSONElement e ;
      
      e = obj.getField( FIELD_NAME_REELECTION_TIMEOUT ) ;
      if ( !e.eoo() )
      {
         if ( !e.isNumber() )
         {
            PD_LOG( PDERROR, "Param[%s] is not number in object[%s]",
                    FIELD_NAME_REELECTION_TIMEOUT,
                    obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _timeout = e.numberInt() ;
      }

      e = obj.getField( FIELD_NAME_REELECTION_LEVEL ) ;
      if ( !e.eoo() )
      {
         if ( !e.isNumber() )
         {
            PD_LOG( PDERROR, "Param[%s] is not number in object[%s]",
                    FIELD_NAME_REELECTION_LEVEL,
                    obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _level = ( CLS_REELECTION_LEVEL )((INT32)e.numberInt()) ;
      }

      e = obj.getField( FIELD_NAME_NODEID ) ;
      if ( !e.eoo() )
      {
         if ( !e.isNumber() )
         {
            PD_LOG( PDERROR, "Param[%s] is not number in object[%s]",
                    FIELD_NAME_NODEID,
                    obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _nodeID = (UINT16)e.numberInt() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECT_PARSEARGS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   #define CLS_REELECT_WAIT_TIME          ( 10000 )       /// 10 secs
   #define CLS_REELECT_WAIT_INTERVAL      ( 100 )         /// 100 ms
   #define CLS_REELECT_SW_TIMEOUT         ( 10000 )       /// 10 secs

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnReelectGroup )
   _rtnReelectGroup::_rtnReelectGroup()
   {

   }

   _rtnReelectGroup::~_rtnReelectGroup()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREELECTGROUP_INIT, "_rtnReelectGroup::init" )
   INT32 _rtnReelectGroup::init ( INT32 flags, INT64 numToSkip,
                                  INT64 numToReturn,
                                  const CHAR *pMatcherBuff,
                                  const CHAR *pSelectBuff,
                                  const CHAR *pOrderByBuff,
                                  const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTGROUP_INIT ) ;

      try
      {
         BSONObj obj = BSONObj( pMatcherBuff ) ;

         if ( obj.isEmpty() )
         {
            _isDestNotify = TRUE ;
            // goto done ;
         }
         else
         {
            rc = _parseReelectArgs( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pasrse reelect info,rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTGROUP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREELECTGROUP_DOIT, "_rtnReelectGroup::doit" )
   INT32 _rtnReelectGroup::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTGROUP_DOIT ) ;
      replCB *repl = sdbGetReplCB() ;

      if ( _isDestNotify )
      {
         UINT32 timeout = 0 ;
         _clsVoteMachine* vote = repl->voteMachine( FALSE ) ;

         /// Wait repl down group info
         while ( !vote->isInit() && timeout < CLS_REELECT_WAIT_TIME )
         {
            ossSleep( CLS_REELECT_WAIT_INTERVAL ) ;
            timeout += CLS_REELECT_WAIT_INTERVAL ;
         }

         vote->setShadowWeight( CLS_ELECTION_WEIGHT_MAX, CLS_REELECT_SW_TIMEOUT ) ;
         vote->setElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;

         /// When in CLS_ELECTION_STATUS_SILENCE, need to force to secondary
         if ( vote->isStatus( CLS_ELECTION_STATUS_SILENCE ) )
         {
            vote->force( CLS_ELECTION_STATUS_SEC ) ;
         }
      }
      else
      {
         rc = repl->reelect( _level, _timeout, cb, _nodeID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Replica Group: Failed to reelect:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTGROUP_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnReelectLocation )
   _rtnReelectLocation::_rtnReelectLocation()
    :_pLocation( NULL )
   {

   }

   _rtnReelectLocation::~_rtnReelectLocation()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREELECTLOCATION_INIT, "_rtnReelectLocation::init" )
   INT32 _rtnReelectLocation::init ( INT32 flags, INT64 numToSkip,
                                     INT64 numToReturn,
                                     const CHAR *pMatcherBuff,
                                     const CHAR *pSelectBuff,
                                     const CHAR *pOrderByBuff,
                                     const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTLOCATION_INIT ) ;

      try
      {
         BSONElement e ;

         BSONObj queryObj = BSONObj( pMatcherBuff ) ;
         BSONObj hintObj = BSONObj( pHintBuff ) ;

         // Get location info
         e = queryObj.getField( FIELD_NAME_LOCATION ) ;
         if ( e.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Location is null" ) ;
            goto error ;
         }
         if ( String != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Location type[%d] is not string", e.type() ) ;
            goto error ;
         }
         _pLocation = e.valuestrsafe() ;

         // Identify if it's destination node
         e = hintObj.getField( FIELD_NAME_ISDESTINATION ) ;
         if ( !e.eoo() )
         {
            if ( !e.isBoolean() )
            {
               PD_LOG( PDERROR, "Param[%s] is not boolean in object[%s]",
                       FIELD_NAME_ISDESTINATION,
                       hintObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            _isDestNotify = e.boolean() ;
         }

         if ( ! _isDestNotify )
         {
            rc = _parseReelectArgs( queryObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pasrse reelect info,rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTLOCATION_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREELECTLOCATION_DOIT, "_rtnReelectLocation::doit" )
   INT32 _rtnReelectLocation::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                    _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                    INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTLOCATION_DOIT ) ;
      replCB *repl = sdbGetReplCB() ;

      // Check location info
      const CHAR* localLocation = pmdGetLocation() ;
      if ( '\0' == localLocation[0] || NULL == _pLocation ||
           0 != ossStrcmp( _pLocation, localLocation ) )
      {
         rc = SDB_INVALID_ROUTEID ;
         PD_LOG( PDERROR, "Location Set: The location[%s] of reelect "
                 "doesn't match local location[%s]", _pLocation ? _pLocation : "null",
                 localLocation ? localLocation : "null" ) ;
         goto error ;
      }

      if ( _isDestNotify )
      {
         UINT32 timeout = 0 ;
         _clsVoteMachine* vote = repl->voteMachine( TRUE ) ;

         /// Wait repl down group info
         while ( !vote->isInit() && timeout < CLS_REELECT_WAIT_TIME )
         {
            ossSleep( CLS_REELECT_WAIT_INTERVAL ) ;
            timeout += CLS_REELECT_WAIT_INTERVAL ;
         }

         vote->setShadowWeight( CLS_ELECTION_WEIGHT_MAX, CLS_REELECT_SW_TIMEOUT ) ;
         vote->setElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;

         /// When in CLS_ELECTION_STATUS_SILENCE, need to force to secondary
         if ( vote->isStatus( CLS_ELECTION_STATUS_SILENCE ) )
         {
            vote->force( CLS_ELECTION_STATUS_SEC ) ;
         }
      }
      else
      {
         rc = repl->locationReelect( _level, _timeout, cb, _nodeID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Location Set: Failed to reelect:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTLOCATION_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnForceStepUp )
   INT32 _rtnForceStepUp::spaceNode()
   {
      return CMD_SPACE_NODE_CATA ;
   }

   INT32 _rtnForceStepUp::spaceService()
   {
      return CMD_SPACE_SERVICE_LOCAL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFORCESTEPUP_INIT, "_rtnForceStepUp::init" )
   INT32 _rtnForceStepUp::init( INT32 flags, INT64 numToSkip,
                                INT64 numToReturn,
                                const CHAR *pMatcherBuff,
                                const CHAR *pSelectBuff,
                                const CHAR *pOrderByBuff,
                                const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSFORCESTEPUP_INIT ) ;
      BSONObj options ;
      try
      {
         options = BSONObj( pMatcherBuff ).copy() ;
         BSONElement e = options.getField( FIELD_NAME_FORCE_STEP_UP_TIME ) ;
         if ( e.isNumber() )
         {
            _seconds = e.Number() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSFORCESTEPUP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFORCESTEPUP_DOIT, "_rtnForceStepUp::doit" )
   INT32 _rtnForceStepUp::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSFORCESTEPUP_DOIT ) ;
      replCB *repl = sdbGetReplCB() ;
      rc = repl->stepUp( _seconds, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to step up:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSFORCESTEPUP_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _clsAlterDC )
   _clsAlterDC::_clsAlterDC()
   {
      _pAction = "" ;
   }

   _clsAlterDC::~_clsAlterDC()
   {
   }

   INT32 _clsAlterDC::spaceService ()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   BOOLEAN _clsAlterDC::writable()
   {
      if ( !_pAction )
      {
         return FALSE ;
      }
      else if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_ENABLE_READONLY,
                                    _pAction ) ||
                0 == ossStrcasecmp( CMD_VALUE_NAME_DISABLE_READONLY,
                                    _pAction ) ||
                0 == ossStrcasecmp( CMD_VALUE_NAME_ACTIVATE, _pAction ) ||
                0 == ossStrcasecmp( CMD_VALUE_NAME_DEACTIVATE, _pAction ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   INT32 _clsAlterDC::init( INT32 flags, INT64 numToSkip,
                            INT64 numToReturn, const CHAR *pMatcherBuff,
                            const CHAR *pSelectBuff,
                            const CHAR *pOrderByBuff,
                            const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj query( pMatcherBuff ) ;
         BSONElement eleAction = query.getField( FIELD_NAME_ACTION ) ;
         if ( String != eleAction.type() )
         {
            PD_LOG( PDERROR, "The field[%s] is not valid in command[%s]'s "
                    "param[%s]", FIELD_NAME_ACTION, NAME_ALTER_DC,
                    query.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _pAction = eleAction.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse params occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsAlterDC::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                            _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                            INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      clsCB *pClsCB = pmdGetKRCB()->getClsCB() ;
      clsDCBaseInfo *pInfo = NULL ;

      if ( !pClsCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      pInfo = pClsCB->getShardCB()->getDCMgr()->getDCBaseInfo() ;

      if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_ENABLE_READONLY,
                               _pAction ) )
      {
         pInfo->setReadonly( TRUE ) ;
         pmdGetKRCB()->setDBReadonly( TRUE ) ;
      }
      else if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_DISABLE_READONLY,
                                    _pAction ) )
      {
         pInfo->setReadonly( FALSE ) ;
         pmdGetKRCB()->setDBReadonly( FALSE ) ;
      }
      else if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_ACTIVATE, _pAction ) )
      {
         pInfo->setAcitvated( TRUE ) ;
         pmdGetKRCB()->setDBDeactivated( FALSE ) ;
      }
      else if ( 0 == ossStrcasecmp( CMD_VALUE_NAME_DEACTIVATE, _pAction ) )
      {
         pInfo->setAcitvated( FALSE ) ;
         pmdGetKRCB()->setDBDeactivated( TRUE ) ;
      }
      else
      {
         rc = pClsCB->getShardCB()->updateDCBaseInfo() ;
         if ( rc )
         {
            goto error ;
         }
      }

      /// when is active or disable readonly
      if ( pClsCB->isPrimary() &&
           ( 0 == ossStrcasecmp( CMD_VALUE_NAME_DISABLE_READONLY,
                                 _pAction ) ||
             0 == ossStrcasecmp( CMD_VALUE_NAME_ACTIVATE, _pAction ) ) )
      {
         pClsCB->getReplCB()->voteMachine()->force( CLS_ELECTION_STATUS_SEC ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _rtnAlterCommand implement
    */
   _rtnAlterCommand::_rtnAlterCommand ()
   : _rtnCommand(),
     _rtnAlterJobHolder()
   {
   }

   _rtnAlterCommand::~_rtnAlterCommand ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCMD_INIT, "_rtnAlterCommand::init" )
   INT32 _rtnAlterCommand::init ( INT32 flags,
                                  INT64 numToSkip,
                                  INT64 numToReturn,
                                  const CHAR * pMatcherBuff,
                                  const CHAR * pSelectBuff,
                                  const CHAR * pOrderByBuff,
                                  const CHAR * pHintBuff )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSALTERCMD_INIT ) ;

      try
      {
         BSONObj alterObject( pMatcherBuff ) ;

         rc = createAlterJob() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create alter job, rc: %d", rc ) ;

         rc = _alterJob->initialize( NULL, _getObjectType(), alterObject ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to initialize alter job, rc: %d", rc ) ;
      }
      catch ( exception & e )
      {
         PD_LOG( PDERROR, "unexpected error happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCMD_INIT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCMD_DOIT, "_rtnAlterCommand::doit" )
   INT32 _rtnAlterCommand::doit ( _pmdEDUCB * cb,
                                  _SDB_DMSCB * dmsCB,
                                  _SDB_RTNCB * rtnCB,
                                  _dpsLogWrapper * dpsCB,
                                  INT16 w,
                                  INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSALTERCMD_DOIT ) ;

      const CHAR * objectName = _alterJob->getObjectName() ;
      const rtnAlterOptions * options = _alterJob->getOptions() ;
      const _rtnAlterInfo * alterInfo = _alterJob->getAlterInfo() ;
      const RTN_ALTER_TASK_LIST & alterTasks = _alterJob->getAlterTasks() ;

      if ( alterTasks.empty() )
      {
         goto done ;
      }

      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
               iter != alterTasks.end() ;
               ++ iter )
         {
            rtnAlterTask * task = ( *iter ) ;

            if ( task->testFlags( RTN_ALTER_TASK_FLAG_SHARDONLY ) )
            {
               PD_LOG( PDWARNING,
                       "Failed to execute task [%s]: the request should "
                       "from SHARD port", task->getActionName() ) ;
               if ( options->isIgnoreException() )
               {
                  continue ;
               }
               else
               {
                  rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
                  goto error ;
               }
            }

            rc = _executeTask ( objectName, task, alterInfo, options,
                                cb, dmsCB, rtnCB, dpsCB, w ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to run alter task [%s], rc: %d",
                       task->getActionName(), rc ) ;
               if ( options->isIgnoreException() )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               else
               {
                  goto error ;
               }
            }
         }

         if ( SDB_OK != _alterJob->getParseRC() )
         {
            // Report the parse error
            rc = _alterJob->getParseRC() ;
            goto error ;
         }

         /// AUDIT
         PD_AUDIT_COMMAND( AUDIT_DDL, name(), _getAuditType(), objectName, rc,
                           "Option:%s",
                           _alterJob->getJobObject().toString().c_str() ) ;
      }
      else
      {
         const rtnAlterTask * task = NULL ;
         PD_CHECK( 1 == alterTasks.size(), SDB_OPTION_NOT_SUPPORT,
                   error, PDERROR, "Failed to execute alter job: "
                   "should have only one task" ) ;

         task = alterTasks.front() ;
         if ( task->testFlags( RTN_ALTER_TASK_FLAG_3PHASE ) )
         {
            rc = _openContext( cb, rtnCB, pContextID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to open context, alter "
                         "collection space, rc: %d", rc ) ;
         }
         else
         {
            rc = _executeTask( objectName, task, alterInfo, options,
                               cb, dmsCB, rtnCB, dpsCB, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to execute alter task [%s] on [%s], "
                         "rc: %d", task->getActionName(), objectName, rc ) ;
         }

         if ( CMD_ALTER_COLLECTION == type() )
         {
            catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
            pCatAgent->lock_w () ;
            pCatAgent->clear ( collectionFullName() ) ;
            pCatAgent->release_w () ;

            sdbGetClsCB()->invalidateCata( collectionFullName() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCMD_DOIT, rc ) ;
      return rc ;

   error :
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

   /*
      _rtnAlterCollectionSpace implement
    */
   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAlterCollectionSpace )

   _rtnAlterCollectionSpace::_rtnAlterCollectionSpace ()
   : _rtnAlterCommand()
   {
   }

   _rtnAlterCollectionSpace::~_rtnAlterCollectionSpace ()
   {
   }

   const CHAR* _rtnAlterCollectionSpace::spaceName()
   {
      return ( NULL != _alterJob &&
               RTN_ALTER_COLLECTION_SPACE == _alterJob->getObjectType() ) ?
             _alterJob->getObjectName() : NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCOLLECTIONSPACE__EXECTASK, "_rtnAlterCollectionSpace::_executeTask" )
   INT32 _rtnAlterCollectionSpace::_executeTask ( const CHAR * collectionSpace,
                                                  const rtnAlterTask * task,
                                                  const rtnAlterInfo * alterInfo,
                                                  const rtnAlterOptions * options,
                                                  _pmdEDUCB * cb,
                                                  _SDB_DMSCB * dmsCB,
                                                  _SDB_RTNCB * rtnCB,
                                                  _dpsLogWrapper * dpsCB,
                                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSALTERCOLLECTIONSPACE__EXECTASK ) ;

      SDB_ASSERT( NULL != collectionSpace, "collection space is invalid" ) ;
      SDB_ASSERT( NULL != task, "task is invalid" ) ;

      rc = rtnAlterCollectionSpace( collectionSpace, task, alterInfo, options,
                                    cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute task [%s] on collection "
                   "space [%s], rc: %d", task->getActionName(),
                   collectionSpace, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCOLLECTIONSPACE__EXECTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCOLLECTIONSPACE__OPENCONTEXT, "_rtnAlterCollectionSpace::_openContext" )
   INT32 _rtnAlterCollectionSpace::_openContext ( _pmdEDUCB * cb,
                                                  _SDB_RTNCB * rtnCB,
                                                  INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSALTERCOLLECTIONSPACE__OPENCONTEXT ) ;

      rtnContextAlterCS::sharePtr context ;
      rc = rtnCB->contextNew( RTN_CONTEXT_ALTERCS, context,
                              *pContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, rc: %d", rc ) ;

      rc = context->open( (*this), cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, alter "
                   "collection space, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCOLLECTIONSPACE__OPENCONTEXT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _rtnAlterCollection implement
    */
   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAlterCollection )

   _rtnAlterCollection::_rtnAlterCollection ()
   : _rtnAlterCommand()
   {
   }

   _rtnAlterCollection::~_rtnAlterCollection()
   {

   }

   const CHAR *_rtnAlterCollection::collectionFullName()
   {
      return ( NULL != _alterJob &&
               RTN_ALTER_COLLECTION == _alterJob->getObjectType() ) ?
             _alterJob->getObjectName() : NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCOLLECTION__EXECTASK, "_rtnAlterCollection::_executeTask" )
   INT32 _rtnAlterCollection::_executeTask ( const CHAR * collection,
                                             const rtnAlterTask * task,
                                             const rtnAlterInfo * alterInfo,
                                             const rtnAlterOptions * options,
                                             _pmdEDUCB * cb,
                                             _SDB_DMSCB * dmsCB,
                                             _SDB_RTNCB * rtnCB,
                                             _dpsLogWrapper * dpsCB,
                                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSALTERCOLLECTION__EXECTASK ) ;

      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;
      SDB_ASSERT( NULL != task, "task is invalid" ) ;

      rc = rtnAlterCollection( collection, task, alterInfo, options,
                               cb, dpsCB, &_writeResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute task [%s] on collection "
                   "[%s], rc: %d", task->getActionName(), collection, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCOLLECTION__EXECTASK, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__CLSALTERCOLLECTION__OPENCONTEXT, "_rtnAlterCollection::_openContext" )
   INT32 _rtnAlterCollection::_openContext ( _pmdEDUCB * cb,
                                             _SDB_RTNCB * rtnCB,
                                             INT64 * pContextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSALTERCOLLECTION__OPENCONTEXT ) ;

      rtnContextAlterCL::sharePtr context ;
      rc = rtnCB->contextNew( RTN_CONTEXT_ALTERCL, context,
                              *pContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create context, rc: %d", rc ) ;

      rc = context->open( (*this), cb, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, alter "
                   "collection space, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__CLSALTERCOLLECTION__OPENCONTEXT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnAlterGroup )
   _rtnAlterGroup::_rtnAlterGroup()
   : _groupID( INVALID_GROUPID ),
     _pActionName( NULL ),
     _enforced( FALSE )
   {
   }

   _rtnAlterGroup::~_rtnAlterGroup()
   {
   }

   INT32 _rtnAlterGroup::spaceNode()
   {
      return CMD_SPACE_NODE_DATA | CMD_SPACE_NODE_CATA  ;
   }

   INT32 _rtnAlterGroup::spaceService()
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   const CHAR *_rtnAlterGroup::name ()
   {
      return NAME_ALTER_GROUP ;
   }

   RTN_COMMAND_TYPE _rtnAlterGroup::type ()
   {
      return CMD_ALTER_GROUP ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSALTERGROUP__PARSEOPTIONS, "_rtnAlterGroup::_parseOptions" )
   INT32 _rtnAlterGroup::_parseOptions()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSALTERGROUP__PARSEOPTIONS ) ;

      try
      {
         // In start critical mode action, we don't need to validate the parameter,
         // because they have been validated in cata node
         if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
         {
            BSONElement optionEle ;
            clsGrpModeItem tmpGrpModeItem ;
            const CLS_LOC_INFO_MAP &locMap = sdbGetReplCB()->getLocInfoMap() ;

            // Init _grpMode member
            _grpMode.groupID = _groupID ;
            _grpMode.mode = CLS_GROUP_MODE_CRITICAL ;

            // If nodeName and location are both in options, we just use nodeName
            if ( _option.hasField( FIELD_NAME_NODE_NAME ) )
            {
               optionEle = _option.getField( FIELD_NAME_NODE_NAME ) ;
               tmpGrpModeItem.nodeName = optionEle.valuestrsafe() ;
               tmpGrpModeItem.nodeID = sdbGetClsCB()->getNodeID().columns.nodeID ;
            }
            else if ( _option.hasField( CAT_LOCATION_NAME ) )
            {
               optionEle = _option.getField( CAT_LOCATION_NAME ) ;
               tmpGrpModeItem.location = optionEle.valuestrsafe() ;

               // Assign locationID
               CLS_LOC_INFO_MAP::const_iterator locItr = locMap.begin() ;
               while ( locMap.end() != locItr )
               {
                  if ( 0 == ossStrcmp( optionEle.valuestrsafe(),
                                       locItr->second._location.c_str() ) )
                  {
                     tmpGrpModeItem.locationID = locItr->second._locationID ;
                     break ;
                  }
                  ++locItr ;
               }
            }

            // Get Enforced field
            if ( _option.hasField( FIELD_NAME_ENFORCED1 ) )
            {
               optionEle = _option.getField( FIELD_NAME_ENFORCED1 ) ;
               _enforced = optionEle.booleanSafe() ;
            }

            // We don't to need to save minKeepTime and maxKeepTime, because these two parameters
            // is useless in data node this start critical mode command

            _grpMode.grpModeInfo.push_back( tmpGrpModeItem ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to alter group, received unknown action[%s]",
                    _pActionName ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSALTERGROUP__PARSEOPTIONS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSALTERGROUP_INIT, "_rtnAlterGroup::init" )
   INT32 _rtnAlterGroup::init ( INT32 flags, INT64 numToSkip,
                                INT64 numToReturn,
                                const CHAR *pMatcherBuff,
                                const CHAR *pSelectBuff,
                                const CHAR *pOrderByBuff,
                                const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSALTERGROUP_INIT ) ;

      try
      {
         BSONObj queryObj = BSONObj( pMatcherBuff ) ;
         BSONObj hintObj = BSONObj( pHintBuff ) ;

         // Get GroupID from hint obj
         rc = rtnGetIntElement( hintObj, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from hint object: %s, rc: %d",
                      CAT_GROUPID_NAME, hintObj.toPoolString().c_str(), rc ) ;

         // Get action name
         rc = rtnGetStringElement( queryObj, FIELD_NAME_ACTION, &_pActionName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from query object: %s, rc: %d",
                      FIELD_NAME_ACTION, queryObj.toPoolString().c_str(), rc ) ;

         // Get option
         if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
         {
            rc = rtnGetObjElement( queryObj, FIELD_NAME_OPTIONS, _option ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from query object: %s, rc: %d",
                         FIELD_NAME_OPTIONS, queryObj.toPoolString().c_str(), rc ) ;
         }

         rc = _parseOptions() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse options [%s], actionName: [%s] rc: %d",
                      _option.toPoolString().c_str(), _pActionName, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSALTERGROUP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSALTERGROUP_DOIT, "_rtnAlterGroup::doit" )
   INT32 _rtnAlterGroup::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                               _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                               INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSALTERGROUP_DOIT ) ;

      if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
      {
         _clsVoteMachine* vote = sdbGetReplCB()->voteMachine( FALSE ) ;

         rc = vote->setGrpMode( _grpMode, CLS_REELECT_COMMAND_TIMEOUT_DFT * 1000, TRUE, _enforced ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to do actionName: [%s] rc: %d", _pActionName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CLSALTERGROUP_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

