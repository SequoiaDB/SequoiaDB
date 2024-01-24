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

   Source File Name = catContextNode.cpp

   Descriptive Name = Runtime Context of Catalog

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime Context of Catalog
   helper functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catContextNode.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"
#include "utilCommon.hpp"
#include "catLocation.hpp"

#define CAT_PORT_STR_SZ 10

using namespace bson ;

namespace engine
{
   /*
    * _catCtxNodeBase implement
    */
   _catCtxNodeBase::_catCtxNodeBase ( INT64 contextID, UINT64 eduID )
   : _catContextBase( contextID, eduID ),
     _groupID( CAT_INVALID_GROUPID )
   {
      _isLocalConnection = FALSE ;
   }

   _catCtxNodeBase::~_catCtxNodeBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXNODE_INITQUERY, "_catCtxNodeBase::_initQuery" )
   INT32 _catCtxNodeBase::_initQuery ( const NET_HANDLE &handle,
                                       MsgHeader *pMsg,
                                       const CHAR *pQuery,
                                       const CHAR *pHint,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXNODE_INITQUERY ) ;

      NET_EH eh ;

      rc = _catContextBase::_initQuery( handle, pMsg, pQuery, pHint, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to init query, rc: %d",
                   rc ) ;

      eh = _pCatCB->netWork()->getFrame()->getEventHandle( handle ) ;
      PD_CHECK( eh.get(),
                SDB_NETWORK_CLOSE, error, PDERROR,
                "Failed to get network event handle" ) ;
      _isLocalConnection = eh->isLocalConnection() ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXNODE_INITQUERY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXNODE_COUNTNODE, "_catCtxNodeBase::_countNodes" )
   INT32 _catCtxNodeBase::_countNodes ( const CHAR *pCollection,
                                        const BSONObj &matcher,
                                        UINT64 &count,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXNODE_COUNTNODE ) ;

      INT64 totalCount = 0;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;
      rtnQueryOptions options ;
      options.setCLFullName( pCollection ) ;
      options.setQuery( matcher ) ;
      rc = rtnGetCount( options, _pDmsCB, cb, pRtnCB, &totalCount ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get count on collection [%s] with [%s], rc: %d",
                    pCollection,
                    matcher.toString().c_str(), rc ) ;

      SDB_ASSERT( totalCount >= 0,
                  "totalCount must be greater than or equal 0" ) ;

      count = static_cast<UINT64>( totalCount ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXNODE_COUNTNODE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXNODE__BUILDP1REPLY, "_catCtxNodeBase::_buildP1Reply" )
   INT32 _catCtxNodeBase::_buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCTXNODE__BUILDP1REPLY ) ;

      try
      {
         builder.appendElements( _boTarget ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build phase 1 reply, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXNODE__BUILDP1REPLY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
    * _catCtxActiveGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxActiveGrp, RTN_CONTEXT_CAT_ACTIVE_GROUP,
                          "CAT_ACTIVE_GROUP" )

   _catCtxActiveGrp::_catCtxActiveGrp ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = FALSE ;
   }

   _catCtxActiveGrp::~_catCtxActiveGrp ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXACTIVEGRP_PARSEQUERY, "_catCtxActiveGrp::_parseQuery" )
   INT32 _catCtxActiveGrp::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXACTIVEGRP_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_ACTIVE_GROUP_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_GROUPNAME_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXACTIVEGRP_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXACTIVEGRP_CHECK_INT, "_catCtxActiveGrp::_checkInternal" )
   INT32 _catCtxActiveGrp::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXACTIVEGRP_CHECK_INT ) ;

      BSONObj boNodeList ;
      INT32 groupStatus ;

      rc = catGetGroupObj( _targetName.c_str(), FALSE, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = rtnGetIntElement( _boTarget, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUPID_NAME, rc ) ;

      rc = rtnGetArrayElement( _boTarget, CAT_GROUP_NAME, boNodeList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      PD_CHECK( !boNodeList.isEmpty(),
                SDB_CLS_EMPTY_GROUP, error, PDERROR,
                "Failed to active group, can't active empty-group" ) ;

      rc = rtnGetIntElement( _boTarget, CAT_GROUP_STATUS, groupStatus ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUP_STATUS, rc ) ;

      // if catalog group or status is already active
      PD_CHECK( CATALOG_GROUPID != _groupID && SDB_CAT_GRP_ACTIVE != groupStatus,
                SDB_OK, done, PDWARNING,
                "Group [%s] is already active", _targetName.c_str() ) ;

      // lock group
      PD_CHECK( _lockMgr.tryLockGroup( _targetName, SHARED ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock group [%s]", _targetName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXACTIVEGRP_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXACTIVEGRP_EXECUTE_INT, "_catCtxActiveGrp::_executeInternal" )
   INT32 _catCtxActiveGrp::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXACTIVEGRP_EXECUTE_INT ) ;

      BSONObj boStatus = BSON( CAT_GROUP_STATUS << SDB_CAT_GRP_ACTIVE ) ;
      BSONObj boUpdater = BSON( "$set" << boStatus ) ;
      BSONObj boMatcher = BSON( CAT_GROUPNAME_NAME << _targetName ) ;
      BSONObj boHint;

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION,
                      boMatcher, boUpdater, boHint, FLG_UPDATE_UPSERT,
                      cb, _pDmsCB, _pDpsCB, w );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update catalog info of group[%s], rc=%d",
                   _targetName.c_str(), rc ) ;

      _pCatCB->activeGroup( _groupID ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXACTIVEGRP_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
    * _catCtxShutdownGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxShutdownGrp, RTN_CONTEXT_CAT_SHUTDOWN_GROUP,
                          "CAT_SHUTDOWN_GROUP" )

   _catCtxShutdownGrp::_catCtxShutdownGrp ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeOnP1 = TRUE ;
      _needRollback = FALSE ;
   }

   _catCtxShutdownGrp::~_catCtxShutdownGrp ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXSHUTDOWNGRP_PARSEQUERY, "_catCtxShutdownGrp::_parseQuery" )
   INT32 _catCtxShutdownGrp::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXSHUTDOWNGRP_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_SHUTDOWN_GROUP_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_GROUPNAME_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXSHUTDOWNGRP_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXSHUTDOWNGRP_CHECK_INT, "_catCtxShutdownGrp::_checkInternal" )
   INT32 _catCtxShutdownGrp::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXSHUTDOWNGRP_CHECK_INT ) ;

      BSONObj boNodeList ;

      rc = catGetGroupObj( _targetName.c_str(), FALSE, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      // lock group
      PD_CHECK( _lockMgr.tryLockGroup( _targetName, SHARED ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock group [%s]", _targetName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXSHUTDOWNGRP_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXSHUTDOWNGRP_EXECUTE_INT, "_catCtxShutdownGrp::_executeInternal" )
   INT32 _catCtxShutdownGrp::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXSHUTDOWNGRP_EXECUTE_INT ) ;

      PD_TRACE_EXIT ( SDB_CATCTXSHUTDOWNGRP_EXECUTE_INT ) ;

      return SDB_OK ;
   }

   /*
    * _catCtxRemoveGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxRemoveGrp, RTN_CONTEXT_CAT_REMOVE_GROUP,
                          "CAT_REMOVE_GROUP" )

   _catCtxRemoveGrp::_catCtxRemoveGrp ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxRemoveGrp::~_catCtxRemoveGrp ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMGRP_PARSEQUERY, "_catCtxRemoveGrp::_parseQuery" )
   INT32 _catCtxRemoveGrp::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMGRP_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_RM_GROUP_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_GROUPNAME_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMGRP_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMGRP_CHECK_INT, "_catCtxRemoveGrp::_checkInternal" )
   INT32 _catCtxRemoveGrp::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMGRP_CHECK_INT ) ;

      BSONObj boNodeList ;

      // check name is valid
      if ( 0 != _targetName.compare( COORD_GROUPNAME ) &&
           0 != _targetName.compare( CATALOG_GROUPNAME ) &&
           0 != _targetName.compare( SPARE_GROUPNAME ) )
      {
         rc = catGroupNameValidate( _targetName.c_str(), FALSE ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Group name [%s] is invalid",
                      _targetName.c_str() ) ;
      }

      rc = catGetGroupObj( _targetName.c_str(), FALSE, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      PD_CHECK( !_pCatCB->isImageEnabled() ||
                !_pCatCB->getCatDCMgr()->groupInImage( _targetName ),
                SDB_CAT_GROUP_HAS_IMAGE, error, PDERROR,
                "Can't remove group [%s] when group has image and image is enable",
                _targetName.c_str() ) ;

      rc = rtnGetIntElement( _boTarget, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field[%s], rc: %d",
                   CAT_GROUPID_NAME, rc ) ;

      if ( CATALOG_GROUPID == _groupID )
      {
         INT64 count = 0 ;
         rc = catGroupCount( count, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count groups, rc: %d", rc ) ;

         PD_CHECK( count <= 1,
                   SDB_CATA_RM_CATA_FORBIDDEN, error, PDERROR,
                   "Can not remove catalog group when has other group exist (%lld)",
                   count ) ;
      }
      else
      {
         UINT64 count = 0 ;
         /// hard code.
         BSONObj matcher = BSON( FIELD_NAME_CATALOGINFO".GroupID" << _groupID ) ;

         /// confirm that there is no collections in this group.
         rc = _countNodes( CAT_COLLECTION_INFO_COLLECTION, matcher, count, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count collection: %s, match: %s, rc: %d",
                      CAT_COLLECTION_INFO_COLLECTION,
                      matcher.toString().c_str(), rc ) ;
         PD_CHECK( count == 0,
                   SDB_CAT_RM_GRP_FORBIDDEN, error, PDERROR,
                   "Can not remove a group with data in it" ) ;

         /// confirm that there is no task in this group.
         matcher = BSON( FIELD_NAME_TARGETID << _groupID ) ;
         rc = _countNodes( CAT_TASK_INFO_COLLECTION, matcher, count, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count collection: %s, match: %s, rc: %d",
                      CAT_TASK_INFO_COLLECTION,
                      matcher.toString().c_str(), rc ) ;
         PD_CHECK( count == 0,
                   SDB_CAT_RM_GRP_FORBIDDEN, error, PDERROR,
                   "Can not remove a group with task in it" ) ;
      }

      // lock group
      PD_CHECK( _lockMgr.tryLockGroup( _targetName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock group [%s]", _targetName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMGRP_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMGRP_EXECUTE_INT, "_catCtxRemoveGrp::_executeInternal" )
   INT32 _catCtxRemoveGrp::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMGRP_EXECUTE_INT ) ;

      BOOLEAN isDeleted = FALSE ;
      BSONObj matcher ;
      BSONObj dummy ;

      if ( CATALOG_GROUPID == _groupID )
      {
         // Will be shutdown anyway
         goto done ;
      }

      // remove group
      _pCatCB->removeGroupID( _groupID ) ;
      isDeleted = TRUE ;

      // remove from all domain
      rc = catDelGroupFromDomain( NULL, _targetName.c_str(), _groupID,
                                  cb, _pDmsCB, _pDpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove group [%s] from domain, rc: %d",
                   _targetName.c_str(), rc ) ;

      // Remove group from group mode
      rc = catDelGroupFromGrpMode( _groupID, w, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove group [%s] from group mode, rc: %d",
                   _targetName.c_str(), rc ) ;

      matcher = BSON( FIELD_NAME_GROUPNAME << _targetName ) ;
      rc = rtnDelete( CAT_NODE_INFO_COLLECTION, matcher, dummy, 0,
                      cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove info of group [%s] from collection [%s], "
                   "rc: %d",
                   _targetName.c_str(), CAT_NODE_INFO_COLLECTION, rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMGRP_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      if ( isDeleted )
      {
         _pCatCB->insertGroupID ( _groupID, _targetName, TRUE ) ;
      }
      goto done ;
   }

   /*
    * _catCtxAlterGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxAlterGrp, RTN_CONTEXT_CAT_ALTER_GROUP, "CAT_ALTER_GROUP" )

   _catCtxAlterGrp::_catCtxAlterGrp( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID ),
     _pActionName( NULL ),
     _pCatNodeMgr( _pCatCB->getCatNodeMgr() )
   {
      _executeOnP1 = FALSE ;
      _needRollback = FALSE ;
   }

   _catCtxAlterGrp::~_catCtxAlterGrp()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION (  SDB_CATCTXALTERGROUP_OPEN, "_catCtxAlterGrp::open" )
   INT32 _catCtxAlterGrp::open( MSG_TYPE cmdType,
                                const BSONObj &queryObj,
                                const BSONObj &hintObj,
                                rtnContextBuf &buffObj,
                                _pmdEDUCB *cb )
   {
      SDB_ASSERT ( _status == CAT_CONTEXT_NEW, "Wrong catalog status before opening" ) ;

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(  SDB_CATCTXALTERGROUP_OPEN ) ;

      _cmdType = cmdType ;

      try
      {
         _boQuery = queryObj.getOwned() ;
         _boHint = hintObj.getOwned() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occured: %s", e.what() ) ;
         goto error ;
      }

      rc = _open( buffObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC(  SDB_CATCTXALTERGROUP_OPEN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_PARSEQUERY, "_catCtxAlterGrp::_parseQuery" )
   INT32 _catCtxAlterGrp::_parseQuery( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXALTERGRP_PARSEQUERY ) ;

      SDB_ASSERT( MSG_BS_QUERY_REQ == _cmdType, "Wrong command type" ) ;

      try
      {
         BSONElement ele ;

         // Get GroupID from hint obj
         rc = rtnGetIntElement( _boHint, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from hint object: %s, rc: %d",
                      CAT_GROUPID_NAME, _boHint.toPoolString().c_str(), rc ) ;

         // Get action name
         rc = rtnGetStringElement( _boQuery, FIELD_NAME_ACTION, &_pActionName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from query object: %s, rc: %d",
                      FIELD_NAME_ACTION, _boQuery.toPoolString().c_str(), rc ) ;

         // Get option
         if ( 0 == ossStrcmp( SDB_ALTER_GROUP_SET_ACTIVE_LOCATION, _pActionName ) ||
              0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) ||
              0 == ossStrcmp( SDB_ALTER_GROUP_START_MAINTENANCE_MODE, _pActionName ) ||
              0 == ossStrcmp( SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE, _pActionName ) ||
              0 == ossStrcmp( SDB_ALTER_GROUP_SET_ATTR, _pActionName ) )
         {
            rc = rtnGetObjElement( _boQuery, FIELD_NAME_OPTIONS, _option ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from query object: %s, rc: %d",
                         FIELD_NAME_OPTIONS, _boQuery.toPoolString().c_str(), rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERGRP_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_CHECK_INT, "_catCtxAlterGrp::_checkInternal" )
   INT32 _catCtxAlterGrp::_checkInternal( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXALTERGRP_CHECK_INT ) ;

      BSONObjIterator itr ;

      // Get group obj by group id
      rc = catGetGroupObj( _groupID, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group [%u] obj, rc: %d", _groupID, rc ) ;

      // Get group name
      rc = rtnGetSTDStringElement( _boTarget, FIELD_NAME_GROUPNAME, _targetName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group name, rc: %d", rc ) ;

      // Lock group
      PD_CHECK( _lockMgr.tryLockGroup( _targetName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock group [%s]", _targetName.c_str() ) ;

      try
      {
         // Match Action
         if ( 0 == ossStrcmp( SDB_ALTER_GROUP_SET_ACTIVE_LOCATION, _pActionName ) )
         {
            rc = _setActiveLocation() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set active location, rc: %d", rc ) ;
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_SET_ATTR, _pActionName ) )
         {
            // Parse options
            itr = BSONObjIterator( _option ) ;
            while ( itr.more() )
            {
               BSONElement optionEle = itr.next() ;
               // Match ActiveLocation
               if ( 0 == ossStrcmp( CAT_ACTIVE_LOCATION_NAME, optionEle.fieldName() ) )
               {
                  rc = _setActiveLocation() ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to set active location in group[%s], rc: %d",
                               _targetName.c_str(),  rc ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Invalid alter group option[%s]", optionEle.fieldName() ) ;
                  goto error ;
               }
            }
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
         {
            rc = _checkCriticalMode( TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check critical mode, rc: %d", rc ) ;
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_STOP_CRITICAL_MODE, _pActionName ) )
         {
            rc = _checkCriticalMode( FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check critical mode, rc: %d", rc ) ;

            _executeOnP1 = TRUE ;
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_MAINTENANCE_MODE, _pActionName ) )
         {
            rc = _checkMaintenanceMode( TRUE) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check maintenance mode, rc: %d", rc ) ;

            _executeOnP1 = TRUE ;
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE, _pActionName ) )
         {
            rc = _checkMaintenanceMode( FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check maintenance mode, rc: %d", rc ) ;

            _executeOnP1 = TRUE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to alter group, received unknown action[%s]",
                    _pActionName ) ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERGRP_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_EXECUTE_INT, "_catCtxAlterGrp::_executeInternal" )
   INT32 _catCtxAlterGrp::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCTXALTERGRP_EXECUTE_INT ) ;

      if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
      {
         rc = _startCriticalMode() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start critical mode, rc: %d", rc ) ;
      }
      else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_STOP_CRITICAL_MODE, _pActionName ) )
      {
         rc = _stopCriticalMode() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to stop critical mode, rc: %d", rc ) ;
      }
      else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_MAINTENANCE_MODE, _pActionName ) )
      {
         rc = _startMaintenanceMode() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start maintenance mode, rc: %d", rc ) ;
      }
      else if ( 0 == ossStrcmp( SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE, _pActionName ) )
      {
         rc = _stopMaintenanceMode() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to stop maintenance mode, rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXALTERGRP_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP__BUILDP1REPLY, "_catCtxAlterGrp::_buildP1Reply" )
   INT32 _catCtxAlterGrp::_buildP1Reply( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP__BUILDP1REPLY ) ;

      try
      {
         // In alter group command, the return msg of all actions should contain groupID
         builder.append( FIELD_NAME_GROUPID, _groupID ) ;

         // If the action is start critical mode, locationID or nodeID should also be returned
         if ( 0 == ossStrcmp( SDB_ALTER_GROUP_START_CRITICAL_MODE, _pActionName ) )
         {
            if ( CAT_INVALID_NODEID != _grpMode.grpModeInfo[0].nodeID )
            {
               builder.append( CAT_NODEID_NAME, _grpMode.grpModeInfo[0].nodeID ) ;
            }
            else if ( CAT_INVALID_LOCATIONID != _grpMode.grpModeInfo[0].locationID )
            {
               builder.append( CAT_LOCATIONID_NAME, _grpMode.grpModeInfo[0].locationID ) ;
            }
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP__BUILDP1REPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_SETACTIVELOC, "_catCtxAlterGrp::_setActiveLocation" )
   INT32 _catCtxAlterGrp::_setActiveLocation()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_SETACTIVELOC ) ;

      BSONElement optionEle ;
      ossPoolString newActLoc ;
      ossPoolString oldActLoc ;

      // Get new ActiveLocation, this field should not be empty
      optionEle = _option.getField( CAT_ACTIVE_LOCATION_NAME ) ;
      if ( optionEle.eoo() || String != optionEle.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDWARNING, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
         goto error ;
      }
      // optionEle.valuestrsize include the length of '\0'
      if ( MSG_LOCATION_NAMESZ < optionEle.valuestrsize() - 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
         goto error ;
      }
      newActLoc = optionEle.valuestrsafe() ;

      // Check and get active location
      rc = catCheckAndGetActiveLocation( _boTarget, _groupID, newActLoc, oldActLoc ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check active location, rc: %d", rc ) ;

      // Compare oldLocation and newLocation
      if ( oldActLoc == newActLoc )
      {
         PD_LOG( PDDEBUG, "The old and new ActiveLocation are same, do nothing" ) ;
         goto done ;
      }

      // Set new ActiveLocation
      if ( ! newActLoc.empty() )
      {
         rc = _pCatNodeMgr->setActiveLocation( _groupID, newActLoc ) ;
      }
      // Remove old ActiveLocation
      else
      {
         rc = _pCatNodeMgr->removeActiveLocation( _groupID ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to set active location, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_SETACTIVELOC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_CHECK_CRITICAL_MODE, "_catCtxAlterGrp::_checkCriticalMode" )
   INT32 _catCtxAlterGrp::_checkCriticalMode( BOOLEAN isStartMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_CHECK_CRITICAL_MODE ) ;

      BOOLEAN isCataGroup = FALSE ;
      BSONElement grpModeEle ;

      // Init _grpMode member
      _grpMode.groupID = _groupID ;
      _grpMode.mode = CLS_GROUP_MODE_CRITICAL ;

      // Check the groupID, only the node in cata and data group can start/stop critical mode
      if ( CATALOG_GROUPID == _groupID )
      {
         isCataGroup = TRUE ;
      }
      else if ( DATA_GROUP_ID_BEGIN > _groupID || DATA_GROUP_ID_END < _groupID )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "Group[%u] doesn't support to %s critical mode",
                     _groupID, isStartMode ? "start" : "stop" ) ;
         goto error ;
      }

      // Check if this group is in maintenance mode
      grpModeEle = _boTarget.getField( CAT_GROUP_MODE_NAME ) ;
      if ( ! grpModeEle.eoo() )
      {
         if ( String != grpModeEle.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDWARNING, "Failed to get the field[%s], type[%d] is not String",
                    CAT_GROUP_MODE_NAME, grpModeEle.type() ) ;
            goto error ;
         }
         else if ( 0 == ossStrcmp( CAT_MAINTENANCE_MODE_NAME, grpModeEle.valuestrsafe() ) )
         {
            rc = SDB_OPERATION_CONFLICT ;
            PD_LOG_MSG( PDERROR, "Failed to %s critical mode in group[%u], "
                        "maintenance mode is operating", isStartMode ? "start" : "stop", _groupID ) ;
            goto error ;
         }
      }

      // If the command is stop critical mode, do nothing
      if ( ! isStartMode )
      {
         goto done ;
      }

      rc = catParseGroupModeInfo( _option, _boTarget, _groupID, TRUE, _grpMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group mode info[%s], rc: %d",
                   _option.toPoolString().c_str(), rc ) ;

      // If we start critical mode in cata group, we must ensure that cata primary is in effective nodes
      if ( isCataGroup )
      {
         const clsGrpModeItem& grpModeItem = _grpMode.grpModeInfo[0] ;

         if ( pmdGetNodeID().columns.nodeID != grpModeItem.nodeID &&
              ( grpModeItem.location.empty() ||
                0 != ossStrcmp( pmdGetLocation(), grpModeItem.location.c_str() ) ) )
         {
            rc = SDB_OPERATION_CONFLICT ;
            PD_LOG_MSG( PDERROR, "Catalog group's primary is not in effective nodes "
                        "of critical mode" ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_CHECK_CRITICAL_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_START_CRITICAL_MODE, "_catCtxAlterGrp::_startCriticalMode" )
   INT32 _catCtxAlterGrp::_startCriticalMode()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_START_CRITICAL_MODE ) ;

      if ( CATALOG_GROUPID == _groupID )
      {
         catSetSyncW( 1 ) ;
      }

      rc = _pCatNodeMgr->startGrpMode( _grpMode, _targetName, _boTarget ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start critical mode, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_START_CRITICAL_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_STOP_CRITICAL_MODE, "_catCtxAlterGrp::_stopCriticalMode" )
   INT32 _catCtxAlterGrp::_stopCriticalMode()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_STOP_CRITICAL_MODE ) ;

      if ( CATALOG_GROUPID == _groupID )
      {
         catSetSyncW( 1 ) ;
      }

      rc = _pCatNodeMgr->stopGrpMode( _grpMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to stop critical mode, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_STOP_CRITICAL_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_CHECK_MAINTENANCE_MODE, "_catCtxAlterGrp::_checkMaintenanceMode" )
   INT32 _catCtxAlterGrp::_checkMaintenanceMode( BOOLEAN isStartMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_CHECK_MAINTENANCE_MODE ) ;

      BSONElement grpModeEle, primaryEle ;

      // Init _grpMode member
      _grpMode.groupID = _groupID ;
      _grpMode.mode = CLS_GROUP_MODE_MAINTENANCE ;

      // Check the groupID, only the node in cata and data group can start/stop maintenance mode
      if ( CATALOG_GROUPID != _groupID &&
           ( DATA_GROUP_ID_BEGIN > _groupID || DATA_GROUP_ID_END < _groupID ) )
      {
         rc = SDB_OPERATION_CONFLICT ;
         PD_LOG_MSG( PDERROR, "Group[%u] doesn't support to %s maintenance mode",
                     _groupID, isStartMode ? "start" : "stop" ) ;
         goto error ;
      }

      // Check if this group is in critical mode
      grpModeEle = _boTarget.getField( CAT_GROUP_MODE_NAME ) ;
      if ( ! grpModeEle.eoo() )
      {
         if ( String != grpModeEle.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDWARNING, "Failed to get the field[%s], type[%d] is not String",
                    CAT_GROUP_MODE_NAME, grpModeEle.type() ) ;
            goto error ;
         }
         else if ( 0 == ossStrcmp( CAT_CRITICAL_MODE_NAME, grpModeEle.valuestrsafe() ) )
         {
            rc = SDB_OPERATION_CONFLICT ;
            PD_LOG_MSG( PDERROR, "Failed to %s maintenance mode in group[%u], "
                        "critical mode is operating", isStartMode ? "start" : "stop", _groupID ) ;
            goto error ;
         }
      }
      // If the command is stop maintenance mode, do nothing
      else if ( ! isStartMode )
      {
         goto done ;
      }

      // If the command is stop maintenance mode and options is empty, it means stop all maintenance mode
      if ( ! isStartMode && _option.isEmpty() )
      {
         goto done ;
      }

      rc = catParseGroupModeInfo( _option, _boTarget, _groupID, isStartMode, _grpMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group mode info[%s], rc: %d",
                   _option.toPoolString().c_str(), rc ) ;

      // If the group mode parameter is location, cover to nodes
      if ( CAT_INVALID_LOCATIONID != _grpMode.grpModeInfo[0].locationID )
      {
         clsGrpModeItem item = _grpMode.grpModeInfo[0] ;
         _grpMode.grpModeInfo.clear() ;

         BSONObjIterator nodeItr ;
         BSONObj nodeListObj ;
         ossPoolString tmpHostName ;
         ossPoolString tmpSvcName ;

         rc = rtnGetArrayElement( _boTarget, CAT_GROUP_NAME, nodeListObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d", CAT_GROUP_NAME, rc ) ;

         // Parse nodeList array
         nodeItr = BSONObjIterator( nodeListObj ) ;
         while ( nodeItr.more() )
         {
            clsGrpModeItem tmpItem ;
            BSONObj boNode = nodeItr.next().embeddedObject() ;

            BSONElement beLocation = boNode.getField( FIELD_NAME_LOCATION ) ;
            if ( beLocation.eoo() )
            {
               continue ;
            }
            else if ( String != beLocation.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_LOCATION, beLocation.type() ) ;
               goto error ;
            }
            else if ( 0 != ossStrcmp( beLocation.valuestrsafe(), item.location.c_str() ) )
            {
               continue ;
            }

            BSONElement beHost = boNode.getField( FIELD_NAME_HOST ) ;
            if ( beHost.eoo() || String != beHost.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_HOST, beHost.type() ) ;
               goto error ;
            }
            tmpHostName = beHost.valuestrsafe() ;

            BSONElement beService = boNode.getField( FIELD_NAME_SERVICE ) ;
            if ( beService.eoo() || Array != beService.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_SERVICE, beService.type() ) ;
               goto error ;
            }
            tmpSvcName = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE ) ;

            BSONElement beNodeID = boNode.getField( FIELD_NAME_NODEID ) ;
            if ( beNodeID.eoo() || ! beNodeID.isNumber() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_NODEID, beService.type() ) ;
               goto error ;
            }
            tmpItem.nodeID = beNodeID.numberInt() ;

            tmpItem.nodeName = tmpHostName + ":" + tmpSvcName ;
            tmpItem.minKeepTime = item.minKeepTime ;
            tmpItem.maxKeepTime = item.maxKeepTime ;
            tmpItem.updateTime = item.updateTime ;

            _grpMode.grpModeInfo.push_back( tmpItem ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_CHECK_MAINTENANCE_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_START_MAINTENANCE_MODE, "_catCtxAlterGrp::_startMaintenanceMode" )
   INT32 _catCtxAlterGrp::_startMaintenanceMode()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_START_MAINTENANCE_MODE ) ;

      rc = _pCatNodeMgr->startGrpMode( _grpMode, _targetName, _boTarget ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start maintenance mode, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_START_MAINTENANCE_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXALTERGRP_STOP_MAINTENANCE_MODE, "_catCtxAlterGrp::_stopMaintenanceMode" )
   INT32 _catCtxAlterGrp::_stopMaintenanceMode()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCTXALTERGRP_STOP_MAINTENANCE_MODE ) ;

      rc = _pCatNodeMgr->stopGrpMode( _grpMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to stop maintenance mode, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCTXALTERGRP_STOP_MAINTENANCE_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
    * _catCtxCreateNode implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxCreateNode, RTN_CONTEXT_CAT_CREATE_NODE,
                          "CAT_CREATE_NODE" )

   _catCtxCreateNode::_catCtxCreateNode ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeOnP1 = FALSE ;
      _needRollback = TRUE ;
      _nodeID = CAT_INVALID_NODEID ;
      _nodeStatus = SDB_CAT_GRP_DEACTIVE ;
      _nodeRole = SDB_ROLE_MAX ;
      _groupRole = SDB_ROLE_DATA ;
      _instanceID = NODE_INSTANCE_ID_UNKNOWN ;
   }

   _catCtxCreateNode::~_catCtxCreateNode ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_PARSEQUERY, "_catCtxCreateNode::_parseQuery" )
   INT32 _catCtxCreateNode::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_CREATE_NODE_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         const CHAR *roleName ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_GROUPNAME_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_HOST_FIELD_NAME, _hostName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      FIELD_NAME_HOST, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_DBPATH, _dbPath ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s], rc: %d",
                      PMD_OPTION_DBPATH, rc ) ;

         rc = rtnGetStringElement( _boQuery, PMD_OPTION_ROLE, &roleName ) ;
         if ( SDB_OK == rc )
         {
            _nodeRole = utilGetRoleEnum( roleName ) ;
            PD_LOG( PDDEBUG, "Got node role [%d]", _nodeRole ) ;
         }
         else
         {
            rc = SDB_OK ;
            _nodeRole = SDB_ROLE_MAX ;
         }

         rc = rtnGetIntElement( _boQuery, PMD_OPTION_INSTANCE_ID,
                                (INT32 &)_instanceID ) ;
         if ( SDB_OK == rc )
         {
            PD_CHECK( utilCheckInstanceID( _instanceID, TRUE ),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to check field [%s], "
                      "should be %d, or between %d to %d",
                      PMD_OPTION_INSTANCE_ID, NODE_INSTANCE_ID_UNKNOWN,
                      NODE_INSTANCE_ID_MIN + 1, NODE_INSTANCE_ID_MAX - 1 ) ;
            PD_LOG( PDDEBUG, "Got instance ID [%u]", _instanceID ) ;
         }
         else
         {
            rc = SDB_OK ;
            _instanceID = NODE_INSTANCE_ID_UNKNOWN ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_CHECK_INT, "_catCtxCreateNode::_checkInternal" )
   INT32 _catCtxCreateNode::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_CHECK_INT ) ;

      BSONObj boNodeList ;
      BOOLEAN isLocalHost = FALSE ;
      BOOLEAN isValid = FALSE ;
      UINT16 svcPort = 0 ;
      BOOLEAN svcExist = FALSE ;

      rc = catGetGroupObj( _targetName.c_str(), FALSE, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = rtnGetIntElement( _boTarget, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUPID_NAME, rc ) ;

      rc = rtnGetIntElement( _boTarget, CAT_ROLE_NAME, _groupRole ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_ROLE_NAME, rc ) ;

      rc = rtnGetArrayElement( _boTarget, CAT_GROUP_NAME, boNodeList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      // coord group not limited
      if ( COORD_GROUPID != _groupID )
      {
         PD_CHECK( boNodeList.nFields() < CLS_REPLSET_MAX_NODE_SIZE,
                   SDB_DMS_REACHED_MAX_NODES, error, PDERROR,
                   "Reached the maximum number of nodes!" ) ;
      }

      // check if 'localhost' or '127.0.0.1' is used
      if ( 0 == _hostName.compare( OSS_LOCALHOST ) ||
           0 == _hostName.compare( OSS_LOOPBACK_IP ) )
      {
         // add localhost, coord must be the same with catalog
         if ( !_isLocalConnection )
         {
            rc = SDB_CAT_NOT_LOCALCONN ;
            goto error ;
         }
         isLocalHost = TRUE ;
      }

      rc = _checkLocalHost( isLocalHost, isValid, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get localhost existing info, rc: %d", rc ) ;

      PD_CHECK( isValid,
                SDB_CAT_LOCALHOST_CONFLICT, error, PDERROR,
                "'localhost' and '127.0.0.1' cannot be used mixed with "
                "other hostname and IP address" );

      rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_SVCNAME, _localSvc ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   PMD_OPTION_SVCNAME, rc ) ;

      // check local service whether exist or not
      rc = catServiceCheck( _hostName.c_str(), _localSvc.c_str(), svcExist, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check local service [%s], rc: %d",
                   _localSvc.c_str(), rc ) ;
      PD_CHECK( !svcExist,
                SDBCM_NODE_EXISTED, error, PDERROR,
                "Local service [%s] conflict", _localSvc.c_str() ) ;

      ossSocket::getPort( _localSvc.c_str(), svcPort ) ;
      PD_CHECK( 0 != svcPort,
                SDB_INVALIDARG, error, PDERROR,
                "Local service [%s] is invalid, translate to port 0",
                _localSvc.c_str() ) ;

      rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_REPLNAME, _replSvc ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         _replSvc = _getServiceName( svcPort, MSG_ROUTE_REPL_SERVICE ) ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   PMD_OPTION_REPLNAME, rc ) ;

      rc = catServiceCheck( _hostName.c_str(), _replSvc.c_str(), svcExist, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check repl service [%s], rc: %d",
                   _replSvc.c_str(), rc ) ;
      PD_CHECK( !svcExist,
                SDB_CM_CONFIG_CONFLICTS, error, PDERROR,
                "Repl service [%s] conflict", _replSvc.c_str() ) ;

      // shard service
      rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_SHARDNAME, _shardSvc ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         _shardSvc = _getServiceName( svcPort, MSG_ROUTE_SHARD_SERVCIE ) ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   PMD_OPTION_SHARDNAME, rc ) ;

      rc = catServiceCheck( _hostName.c_str(), _shardSvc.c_str(), svcExist, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check shard service [%s], rc: %d",
                   _shardSvc.c_str(), rc ) ;
      PD_CHECK( !svcExist,
                SDB_CM_CONFIG_CONFLICTS, error, PDERROR,
                "Shard service [%s] conflict", _shardSvc.c_str() ) ;

      if ( SDB_ROLE_MAX == _nodeRole )
      {
         // Role of node is not specified, use the role of group
         _nodeRole = _groupRole ;
      }
      else
      {
         // Role of node is specified, should be the same with the group
         PD_CHECK( _nodeRole == _groupRole,
                   SDB_CM_CONFIG_CONFLICTS, error, PDERROR,
                   "Role of node [%d] conflicts with role of group [%d]",
                   _nodeRole, _groupRole ) ;
      }

      if ( SDB_ROLE_CATALOG == _nodeRole )
      {
         rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_CATANAME, _cataSvc ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            _cataSvc = _getServiceName( svcPort, MSG_ROUTE_CAT_SERVICE ) ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get the field [%s], rc: %d",
                      PMD_OPTION_CATANAME, rc ) ;

         rc = catServiceCheck( _hostName.c_str(), _cataSvc.c_str(), svcExist, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check cata service [%s], rc: %d",
                      _cataSvc.c_str(), rc ) ;
         PD_CHECK( !svcExist,
                   SDB_CM_CONFIG_CONFLICTS, error, PDERROR,
                   "Cata service [%s] conflict", _cataSvc.c_str() ) ;
      }

      if ( 0 == _targetName.compare( CATALOG_GROUPNAME ) ||
           0 == _targetName.compare( COORD_GROUPNAME ) )
      {
         _nodeID = _pCatCB->allocSystemNodeID() ;
         _nodeStatus = SDB_CAT_GRP_ACTIVE ;
      }
      else
      {
         _nodeID = _pCatCB->allocNodeID();
      }

      PD_CHECK( CAT_INVALID_NODEID != _nodeID,
                SDB_SYS, error, PDERROR,
                "Failed to allocate node id, maybe node is full" ) ;

      // lock node
      _nodeName = _hostName + ":" + _localSvc ;
      PD_CHECK( _lockMgr.tryLockNode( _targetName, _nodeName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock node [%s] on group [%s]",
                _nodeName.c_str(), _targetName.c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_EXECUTE_INT, "_catCtxCreateNode::_executeInternal" )
   INT32 _catCtxCreateNode::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_EXECUTE_INT ) ;

      rc = catCreateNodeStep( _targetName, _hostName, _dbPath, _instanceID,
                              _localSvc, _replSvc, _shardSvc, _cataSvc,
                              _nodeRole, _nodeID, _nodeStatus,
                              cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to create node [%s] on group [%s], rc: %d",
                    _targetName.c_str(), _nodeName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_ROLLBACK_INT, "_catCtxCreateNode::_rollbackInternal" )
   INT32 _catCtxCreateNode::_rollbackInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_ROLLBACK_INT ) ;

      // do not need to wait
      w = 1 ;
      catSetSyncW( 1 ) ;

      rc = catRemoveNodeStep ( _targetName, _nodeID, cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to rollback create node [%s] on group [%s], rc: %d",
                    _targetName.c_str(), _nodeName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE_ROLLBACK_INT, rc ) ;
      if ( CAT_INVALID_NODEID != _nodeID )
      {
         // TODO: use _pCatCB->releaseNode()
         _pCatCB->releaseNodeID( _nodeID ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE__BUILDP2REPLY, "_catCtxCreateNode::_buildP2Reply" )
   INT32 _catCtxCreateNode::_buildP2Reply( bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(SDB_CATCTXCREATENODE__BUILDP2REPLY) ;
      catBuildNewNode( _hostName, _dbPath, _instanceID, _localSvc, _replSvc, _shardSvc, _cataSvc,
                       _nodeRole, _nodeID, _nodeStatus, builder ) ;

      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE__BUILDP2REPLY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_CHECKLOCALHOST, "_catCtxCreateNode::_checkLocalHost" )
   INT32 _catCtxCreateNode::_checkLocalHost( BOOLEAN isLocalHost,
                                             BOOLEAN &isValid,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_CHECKLOCALHOST ) ;

      BSONObj matcher ;
      UINT64 count = 0 ;

      rc = _countNodes( CAT_NODE_INFO_COLLECTION, matcher, count, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get number of nodes" ) ;

      if ( 0 == count )
      {
         // There is no group, so it can be used no matter localhost or not.
         isValid = TRUE;
         goto done;
      }

      // check whether 'localhost' or '127.0.0.1' is used
      // {"Group.HostName":{"$in":["localhost","127.0.0.1"]}}
      matcher = BSON(
            CAT_GROUP_NAME "." CAT_HOST_FIELD_NAME <<
            BSON ( "$in" << BSON_ARRAY( OSS_LOCALHOST << OSS_LOOPBACK_IP ) ) ) ;
      rc = _countNodes( CAT_NODE_INFO_COLLECTION, matcher, count, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get number of nodes" ) ;

      // if count == 0, then no node uses 'localhost' or '127.0.0.1',
      // so localhost cannot be used.
      // otherwise (count > 0), localhost can be used.
      isValid = isLocalHost ^ ( 0 == count) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXCREATENODE_CHECKLOCALHOST, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   std::string _catCtxCreateNode::_getServiceName ( UINT16 localPort,
                                                    MSG_ROUTE_SERVICE_TYPE type )
   {
      CHAR szPort[ CAT_PORT_STR_SZ + 1 ] = {0};
      UINT16 port = localPort + type ;
      ossItoa( port, szPort, CAT_PORT_STR_SZ ) ;
      return string( szPort ) ;
   }

   /*
    * _catCtxRemoveNode implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxRemoveNode, RTN_CONTEXT_CAT_REMOVE_NODE,
                          "CAT_REMOVE_NODE" )

   _catCtxRemoveNode::_catCtxRemoveNode ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeOnP1 = FALSE ;
      _needRollback = TRUE ;
      _nodeCount = 0 ;
      _nodeID = CAT_INVALID_NODEID ;
      _forced = FALSE ;
      _needDeactive = FALSE ;
   }

   _catCtxRemoveNode::~_catCtxRemoveNode ()
   {
      _onCtxDelete() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_PARSEQUERY, "_catCtxRemoveNode::_parseQuery" )
   INT32 _catCtxRemoveNode::_parseQuery ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_PARSEQUERY ) ;

      SDB_ASSERT( MSG_CAT_DEL_NODE_REQ == _cmdType,
                  "Wrong command type" ) ;

      try
      {
         rc = rtnGetSTDStringElement( _boQuery, CAT_GROUPNAME_NAME, _targetName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      CAT_GROUPNAME_NAME, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, CAT_HOST_FIELD_NAME, _hostName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s], rc: %d",
                      FIELD_NAME_HOST, rc ) ;

         rc = rtnGetSTDStringElement( _boQuery, PMD_OPTION_SVCNAME, _localSvc ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s], rc: %d",
                      PMD_OPTION_DBPATH, rc ) ;

         rc = rtnGetBooleanElement( _boQuery, FIELD_NAME_ENFORCED1, _forced ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = rtnGetBooleanElement( _boQuery, FIELD_NAME_ENFORCED, _forced ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
               _forced = FALSE ;
            }
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] or field[%s], rc: %d",
                      FIELD_NAME_ENFORCED1, FIELD_NAME_ENFORCED, rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMNODE_PARSEQUERY, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_CHECK_INT, "_catCtxRemoveNode::_checkInternal" )
   INT32 _catCtxRemoveNode::_checkInternal ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_CHECK_INT ) ;

      BSONObj boNodeList ;
      BOOLEAN lockGroup = FALSE ;
      INT32 groupRole = SDB_ROLE_DATA ;
      UINT64 nodeNum = 0 ;
      BOOLEAN isSpareGroup = FALSE ;
      BSONObj matcher ;

      rc = catGetGroupObj( _targetName.c_str(), FALSE, _boTarget, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group [%s], rc: %d",
                   _targetName.c_str(), rc ) ;

      rc = rtnGetIntElement( _boTarget, CAT_GROUPID_NAME, (INT32 &)_groupID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUPID_NAME, rc ) ;

      rc = rtnGetArrayElement( _boTarget, CAT_GROUP_NAME, boNodeList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      rc = rtnGetIntElement( _boTarget, CAT_ROLE_NAME, groupRole ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get field [%s], rc: %d",
                   CAT_ROLE_NAME, rc ) ;

      isSpareGroup = ( _targetName.compare( SPARE_GROUPNAME ) == 0 ) ;

      rc = _getRemovedGroupsObj( boNodeList, _nodeID, _location ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get removed node ID" ) ;

      if ( SDB_ROLE_CATALOG == groupRole )
      {
         MsgRouteID localID = pmdGetNodeID() ;
         if ( _nodeID == localID.columns.nodeID )
         {
            rc = SDB_CATA_RM_CATA_FORBIDDEN ;
            PD_LOG( PDERROR,
                    "Can not remove primary catalog. reelect first" ) ;
            goto error ;
         }
      }

      if ( !_forced && SDB_ROLE_COORD != groupRole && !isSpareGroup )
      {
         UINT16 primaryID = CAT_INVALID_NODEID ;
         rc = rtnGetIntElement( _boTarget, FIELD_NAME_PRIMARY,
                                (INT32 &)primaryID ) ;
         if ( SDB_OK == rc )
         {
            if ( primaryID == _nodeID )
            {
               PD_LOG( PDERROR,
                        "Can not remove primary node of group [%s]",
                       _boTarget.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_CATA_RM_NODE_FORBIDDEN ;
               goto error ;
            }
         }
         PD_LOG( PDWARNING,
                 "Can not find primary node of group [%s]",
                 _boTarget.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_OK ;
      }

      _nodeCount = boNodeList.nFields() ;

      // node num judge
      if ( !isSpareGroup && 1 == _nodeCount && !_forced )
      {
         rc = SDB_CATA_RM_NODE_FORBIDDEN ;
         PD_LOG( PDERROR,
                 "Can not remove node when group [%s] is only one node",
                 _boTarget.toString().c_str() ) ;
         goto error ;
      }

      // Forced to remove last data node, should deactive the group
      if ( _forced &&  1 == _nodeCount && !isSpareGroup &&
           SDB_ROLE_DATA == groupRole )
      {
         try
         {
             matcher = BSON( FIELD_NAME_CATALOGINFO".GroupID" << _groupID ) ;
         }
         catch ( std::exception &e )
         {
             rc = ossException2RC( &e ) ;
             PD_LOG ( PDERROR, "Failed to build BSONObj, exception: %s, rc: %d",
                      e.what(), rc ) ;
             goto error ;
         }
         rc = _countNodes( CAT_COLLECTION_INFO_COLLECTION, matcher, nodeNum, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count collection: %s, match: %s, rc: %d",
                      CAT_COLLECTION_INFO_COLLECTION,
                      matcher.toPoolString().c_str(), rc ) ;
         PD_CHECK( nodeNum == 0,
                   SDB_CATA_RM_NODE_FORBIDDEN, error, PDERROR,
                   "Unable to remove the last node or primary with data in a group" ) ;

         /// confirm that no there is no task.
         try
         {
            matcher = BSON( FIELD_NAME_TARGETID << _groupID ) ;
         }
         catch ( std::exception &e )
         {
             rc = ossException2RC( &e ) ;
             PD_LOG ( PDERROR, "Failed to build BSONObj, exception: %s, rc: %d",
                      e.what(), rc ) ;
             goto error ;
         }
         rc = _countNodes( CAT_TASK_INFO_COLLECTION, matcher, nodeNum, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count collection: %s, match: %s, rc: %d",
                      CAT_TASK_INFO_COLLECTION,
                      matcher.toPoolString().c_str(), rc ) ;
         PD_CHECK( nodeNum == 0,
                   SDB_CATA_RM_NODE_FORBIDDEN, error, PDERROR,
                   "Can not remove last node with task in it" ) ;
         lockGroup = TRUE ;
         _needDeactive = TRUE ;
      }

      // check if 'localhost' or '127.0.0.1' is used
      if ( 0 == _hostName.compare( OSS_LOCALHOST ) ||
           0 == _hostName.compare( OSS_LOOPBACK_IP ) )
      {
         // add localhost, coord must be the same with catalog
         if ( !_isLocalConnection )
         {
            rc = SDB_CAT_NOT_LOCALCONN ;
            goto error ;
         }
      }

      // lock node
      _nodeName = _hostName + ":" + _localSvc ;

      if ( lockGroup )
      {
         PD_CHECK( _lockMgr.tryLockGroup( _targetName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock group [%s]", _targetName.c_str() ) ;
      }
      else
      {
         PD_CHECK( _lockMgr.tryLockNode( _targetName, _nodeName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock node [%s] on group [%s]",
                   _nodeName.c_str(), _targetName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMNODE_CHECK_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_EXECUTE_INT, "_catCtxRemoveNode::_executeInternal" )
   INT32 _catCtxRemoveNode::_executeInternal ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_EXECUTE_INT ) ;

      replCB *pReplCB = pmdGetKRCB()->getClsCB()->getReplCB() ;

      if ( !_needDeactive )
      {
         // Re-check again for parallel removing nodes from the same group
         // Note: if need deactive, group has been locked exclusively, no
         // need to re-check
         _lockMgr.unlockObjects() ;

         rc = _checkInternal( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed in catContext [%lld]: "
                      "failed to check context internal, rc: %d",
                      contextID(), rc ) ;
      }

      // For below cases, we don't wait sync
      // 1. forced remove-node command
      // 2. during forced reelect
      if ( _forced || pReplCB->isInStepUp() )
      {
         w = 1 ;
         catSetSyncW( 1 ) ;
      }
      else if ( 0 == _targetName.compare( CATALOG_GROUPNAME ) )
      {
         INT16 tmpW = 1 ;
         if ( _nodeCount > 0 )
         {
            // Reduce sync w at the end of command,
            // since we are removing a Catalog node
            tmpW = (INT16)( ( _nodeCount - 1 ) / 2 + 1 ) ;
         }
         if ( w > tmpW )
         {
            w = tmpW ;
         }
         catSetSyncW( tmpW ) ;
      }

      rc = catRemoveNodeStep( _targetName, _nodeID, cb, _pDmsCB, _pDpsCB, w ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to remove node [%s] from group [%s], rc: %d",
                    _nodeName.c_str(), _targetName.c_str(), rc ) ;

      // release node and location
      _pCatCB->releaseNode( _nodeID, _location ) ;

      if ( _needDeactive )
      {
         rc = _deactiveGroup( cb, w ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to deactive group [%s], rc: %d",
                       _targetName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMNODE_EXECUTE_INT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_GETRMGRPOBJ, "_catCtxRemoveNode::_getRemovedGroupsObj" )
   INT32 _catCtxRemoveNode::_getRemovedGroupsObj ( const BSONObj &boNodeList,
                                                   UINT16 &removeNodeID,
                                                   ossPoolString &location )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_GETRMGRPOBJ ) ;

      string tmpHostName ;
      BOOLEAN exist = FALSE ;
      string tmpSvcName ;

      BSONObjIterator i( boNodeList ) ;
      while ( i.more() )
      {
         BSONElement beNode = i.next() ;
         BSONObj boNode = beNode.embeddedObject() ;

         rc = rtnGetSTDStringElement( boNode, FIELD_NAME_HOST, tmpHostName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field [%s] from [%s], rc: %d",
                      FIELD_NAME_HOST, boNode.toString().c_str(), rc ) ;

         BSONElement beService = boNode.getField( FIELD_NAME_SERVICE ) ;
         if ( beService.eoo() || Array != beService.type() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR,
                    "Failed to get field [%s], field type: %d",
                    FIELD_NAME_SERVICE, beService.type() ) ;
            goto error ;
         }
         tmpSvcName = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE ) ;

         // hostname not same
         if ( 0 == _hostName.compare( tmpHostName ) &&
              0 == _localSvc.compare( tmpSvcName ) )
         {
            exist = TRUE ;
            rc = rtnGetIntElement( boNode, FIELD_NAME_NODEID,
                                   (INT32 &)removeNodeID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s], rc: %d",
                         FIELD_NAME_NODEID, rc ) ;
            
            // Some nodes may not have location, so ignore the SDB_FIELD_NOT_EXIST
            rc = rtnGetPoolStringElement( boNode, FIELD_NAME_LOCATION, location ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         FIELD_NAME_LOCATION, rc ) ;
            break ;
         }
      }

      PD_CHECK( exist, SDB_CLS_NODE_NOT_EXIST, error, PDERROR,
                "Remove node [%s:%s] is not exist in group[%s]",
                _hostName.c_str(), _localSvc.c_str(),
                boNodeList.toString().c_str() ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMNODE_GETRMGRPOBJ, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_DEACTIVEGRP, "_catCtxRemoveNode::_deactiveGroup" )
   INT32 _catCtxRemoveNode::_deactiveGroup ( _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_DEACTIVEGRP ) ;

      BSONObj boStatus = BSON( CAT_GROUP_STATUS << SDB_CAT_GRP_DEACTIVE ) ;
      BSONObj boUpdater = BSON( "$set" << boStatus ) ;
      BSONObj boMatcher = BSON( CAT_GROUPNAME_NAME << _targetName ) ;
      BSONObj boHint;

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION,
                      boMatcher, boUpdater, boHint, FLG_UPDATE_UPSERT,
                      cb, _pDmsCB, _pDpsCB, w );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update catalog info of group[%s], rc=%d",
                   _targetName.c_str(), rc ) ;

      _pCatCB->deactiveGroup( _groupID ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCTXRMNODE_DEACTIVEGRP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}
