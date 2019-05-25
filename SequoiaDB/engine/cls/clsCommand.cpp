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

using namespace bson ;

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
         PD_CHECK ( !beSplitKey.eoo() && beSplitKey.type() == Object,
                    SDB_INVALIDARG, error, PDERROR,
                    "Invalid split key: %s",
                    beSplitKey.toString().c_str() ) ;
         _splitKey = beSplitKey.embeddedObject () ;
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

      rc = sdbGetClsCB()->startTaskCheck ( match ) ;
      PD_TRACE_EXITRC ( SDB__CLSSPLIT_DOIT, rc ) ;
      return rc ;
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
      INT32 rc = sdbGetShardCB()->syncUpdateCatalog( _subCLName ) ;
      if ( rc )
      {
         catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
         pCatAgent->lock_w() ;
         pCatAgent->clear( _subCLName ) ;
         pCatAgent->release_w() ;
      }

      rtnCB->getAPM()->invalidateCLPlans( _collectionName ) ;

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
      catAgent *pCatAgent = sdbGetShardCB()->getCataAgent() ;
      INT32 rc = sdbGetShardCB()->syncUpdateCatalog( _subCLName ) ;
      if ( rc )
      {
         pCatAgent->lock_w() ;
         pCatAgent->clear( _subCLName ) ;
         pCatAgent->release_w() ;
      }

      pCatAgent->lock_w() ;
      pCatAgent->clear( _collectionName ) ;
      pCatAgent->release_w() ;

      rtnCB->getAPM()->invalidateCLPlans( _collectionName ) ;

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

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnReelect)
   _rtnReelect::_rtnReelect()
   :_timeout( 30 ),
    _level( CLS_REELECTION_LEVEL_3 )
   {

   }

   _rtnReelect::~_rtnReelect()
   {

   }

   INT32 _rtnReelect::spaceNode()
   {
      return CMD_SPACE_NODE_DATA | CMD_SPACE_NODE_CATA  ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREELECT_INIT, "_rtnReelect::init" )
   INT32 _rtnReelect::init ( INT32 flags, INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pMatcherBuff,
                             const CHAR *pSelectBuff,
                             const CHAR *pOrderByBuff,
                             const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREELECT_INIT ) ;
      BSONObj obj ;
      try
      {
         obj = BSONObj( pMatcherBuff ) ;
         BSONElement e = obj.getField( FIELD_NAME_REELECTION_TIMEOUT ) ;
         if ( !e.eoo() )
         {
            if ( !e.isNumber() )
            {
               PD_LOG( PDERROR, "invalid reelection msg:%s",
                       obj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            _timeout = e.Number() ;
         }

         e = obj.getField( FIELD_NAME_REELECTION_LEVEL ) ;
         if ( !e.eoo() )
         {
            if ( !e.isNumber() )
            {
               PD_LOG( PDERROR, "invalid reelection msg:%s",
                       obj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            _level = ( CLS_REELECTION_LEVEL )((INT32)e.Number()) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNREELECT_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNREELECT_DOIT, "_rtnReelect::doit" )
   INT32 _rtnReelect::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                            _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                            INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNREELECT_DOIT ) ;
      replCB *repl = sdbGetReplCB() ;

      rc = repl->reelect( _level, _timeout, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to reelect:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNREELECT_DOIT, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNFORCESTEPUP_INIT, "_rtnForceStepUp::init" )
   INT32 _rtnForceStepUp::init( INT32 flags, INT64 numToSkip,
                                INT64 numToReturn,
                                const CHAR *pMatcherBuff,
                                const CHAR *pSelectBuff,
                                const CHAR *pOrderByBuff,
                                const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNFORCESTEPUP_INIT ) ;
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
      PD_TRACE_EXITRC( SDB__RTNFORCESTEPUP_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNFORCESTEPUP_DOIT, "_rtnForceStepUp::doit" )
   INT32 _rtnForceStepUp::doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNFORCESTEPUP_DOIT ) ;
      replCB *repl = sdbGetReplCB() ;
      rc = repl->stepUp( _seconds, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to step up:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNFORCESTEPUP_DOIT, rc ) ;
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
}

