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
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "utilCommon.hpp"

#define CAT_PORT_STR_SZ 10

namespace engine
{
   /*
    * _catCtxNodeBase implement
    */
   _catCtxNodeBase::_catCtxNodeBase ( INT64 contextID, UINT64 eduID )
   : _catContextBase( contextID, eduID )
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
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCTXNODE_INITQUERY ) ;

      NET_EH eh ;

      rc = _catContextBase::_initQuery( handle, pMsg, pQuery, cb ) ;
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

   /*
    * _catCtxActiveGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxActiveGrp, RTN_CONTEXT_CAT_ACTIVE_GROUP,
                          "CAT_ACTIVE_GROUP" )

   _catCtxActiveGrp::_catCtxActiveGrp ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
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

      PD_CHECK( CATALOG_GROUPID != _groupID && SDB_CAT_GRP_ACTIVE != groupStatus,
                SDB_OK, done, PDWARNING,
                "Group [%s] is already active", _targetName.c_str() ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXACTIVEGRP_MAKEREPLY, "_catCtxActiveGrp::_makeReply" )
   INT32 _catCtxActiveGrp::_makeReply ( rtnContextBuf &buffObj )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXACTIVEGRP_MAKEREPLY ) ;

      buffObj = rtnContextBuf( _boTarget.getOwned() ) ;

      PD_TRACE_EXIT ( SDB_CATCTXACTIVEGRP_MAKEREPLY ) ;
      return SDB_OK ;
   }

   /*
    * _catCtxShutdownGrp implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxShutdownGrp, RTN_CONTEXT_CAT_SHUTDOWN_GROUP,
                          "CAT_SHUTDOWN_GROUP" )

   _catCtxShutdownGrp::_catCtxShutdownGrp ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeAfterLock = TRUE ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXSHUTDOWNGRP_MAKEREPLY, "_catCtxShutdownGrp::_makeReply" )
   INT32 _catCtxShutdownGrp::_makeReply ( rtnContextBuf &buffObj )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXSHUTDOWNGRP_MAKEREPLY ) ;

      buffObj = rtnContextBuf( _boTarget.getOwned() ) ;

      PD_TRACE_EXIT ( SDB_CATCTXSHUTDOWNGRP_MAKEREPLY ) ;

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
      _executeAfterLock = FALSE ;
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
         BSONObj matcher = BSON( FIELD_NAME_CATALOGINFO".GroupID" << _groupID ) ;

         rc = _countNodes( CAT_COLLECTION_INFO_COLLECTION, matcher, count, cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to count collection: %s, match: %s, rc: %d",
                      CAT_COLLECTION_INFO_COLLECTION,
                      matcher.toString().c_str(), rc ) ;
         PD_CHECK( count == 0,
                   SDB_CAT_RM_GRP_FORBIDDEN, error, PDERROR,
                   "Can not remove a group with data in it" ) ;

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
         goto done ;
      }

      _pCatCB->removeGroupID( _groupID ) ;
      isDeleted = TRUE ;

      rc = catDelGroupFromDomain( NULL, _targetName.c_str(), _groupID,
                                  cb, _pDmsCB, _pDpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove group [%s] from domain, rc: %d",
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMGRP_MAKEREPLY, "_catCtxRemoveGrp::_makeReply" )
   INT32 _catCtxRemoveGrp::_makeReply ( rtnContextBuf &buffObj )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXRMGRP_MAKEREPLY ) ;

      if ( CAT_CONTEXT_READY == _status )
      {
         buffObj = rtnContextBuf( _boTarget.getOwned() ) ;
      }
      else if ( CAT_CONTEXT_END != _status )
      {
         BSONObj dummy ;
         buffObj = rtnContextBuf( dummy.getOwned() ) ;
      }

      PD_TRACE_EXIT ( SDB_CATCTXRMGRP_MAKEREPLY ) ;

      return SDB_OK ;
   }

   /*
    * _catCtxCreateNode implement
    */
   RTN_CTX_AUTO_REGISTER( _catCtxCreateNode, RTN_CONTEXT_CAT_CREATE_NODE,
                          "CAT_CREATE_NODE" )

   _catCtxCreateNode::_catCtxCreateNode ( INT64 contextID, UINT64 eduID )
   : _catCtxNodeBase( contextID, eduID )
   {
      _executeAfterLock = FALSE ;
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

      if ( COORD_GROUPID != _groupID )
      {
         PD_CHECK( boNodeList.nFields() < CLS_REPLSET_MAX_NODE_SIZE,
                   SDB_DMS_REACHED_MAX_NODES, error, PDERROR,
                   "Reached the maximum number of nodes!" ) ;
      }

      if ( 0 == _hostName.compare( OSS_LOCALHOST ) ||
           0 == _hostName.compare( OSS_LOOPBACK_IP ) )
      {
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
         _nodeRole = _groupRole ;
      }
      else
      {
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
         _pCatCB->releaseNodeID( _nodeID ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXCREATENODE_MAKEREPLY, "_catCtxCreateNode::_makeReply" )
   INT32 _catCtxCreateNode::_makeReply ( rtnContextBuf &buffObj )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXCREATENODE_MAKEREPLY ) ;

      BSONObj dummy ;
      buffObj = rtnContextBuf( dummy.getOwned() ) ;

      PD_TRACE_EXIT ( SDB_CATCTXCREATENODE_MAKEREPLY ) ;

      return SDB_OK ;
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
         isValid = TRUE;
         goto done;
      }

      matcher = BSON(
            CAT_GROUP_NAME"."CAT_HOST_FIELD_NAME <<
            BSON ( "$in" << BSON_ARRAY( OSS_LOCALHOST << OSS_LOOPBACK_IP ) ) ) ;
      rc = _countNodes( CAT_NODE_INFO_COLLECTION, matcher, count, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get number of nodes" ) ;

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
      _executeAfterLock = FALSE ;
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

         rc = rtnGetBooleanElement( _boQuery, CMD_NAME_ENFORCED, _forced ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
            _forced = FALSE ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CMD_NAME_ENFORCED, rc ) ;
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

      BOOLEAN isSpareGroup = FALSE ;

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

      rc = _getRemovedGroupsObj( boNodeList, _nodeID ) ;
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

      if ( !isSpareGroup && 1 == _nodeCount && !_forced )
      {
         rc = SDB_CATA_RM_NODE_FORBIDDEN ;
         PD_LOG( PDERROR,
                 "Can not remove node when group [%s] is only one node",
                 _boTarget.toString().c_str() ) ;
         goto error ;
      }

      if ( _forced &&  1 == _nodeCount && !isSpareGroup &&
           SDB_ROLE_DATA == groupRole )
      {
         lockGroup = TRUE ;
         _needDeactive = TRUE ;
      }

      if ( 0 == _hostName.compare( OSS_LOCALHOST ) ||
           0 == _hostName.compare( OSS_LOOPBACK_IP ) )
      {
         if ( !_isLocalConnection )
         {
            rc = SDB_CAT_NOT_LOCALCONN ;
            goto error ;
         }
      }

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

      BSONObjBuilder updateBuilder ;
      BSONObj updator, matcher, dummyObj ;
      BSONArray baNewNodeList ;
      BSONObj boGroup ;

      if ( !_needDeactive )
      {
         _lockMgr.unlockObjects() ;

         rc = _checkInternal( cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed in catContext [%lld]: "
                      "failed to check context internal, rc: %d",
                      contextID(), rc ) ;
      }

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

      _pCatCB->releaseNodeID( _nodeID ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_MAKEREPLY, "_catCtxRemoveNode::_makeReply" )
   INT32 _catCtxRemoveNode::_makeReply ( rtnContextBuf &buffObj )
   {
      PD_TRACE_ENTRY ( SDB_CATCTXRMNODE_MAKEREPLY ) ;

      BSONObj dummy ;
      buffObj = rtnContextBuf( dummy.getOwned() ) ;

      PD_TRACE_EXIT ( SDB_CATCTXRMNODE_MAKEREPLY ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCTXRMNODE_GETRMGRPOBJ, "_catCtxRemoveNode::_getRemovedGroupsObj" )
   INT32 _catCtxRemoveNode::_getRemovedGroupsObj ( const BSONObj &boNodeList,
                                                   UINT16 &removeNodeID )
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

         if ( 0 == _hostName.compare( tmpHostName ) &&
              0 == _localSvc.compare( tmpSvcName ) )
         {
            exist = TRUE ;
            rc = rtnGetIntElement( boNode, FIELD_NAME_NODEID,
                                   (INT32 &)removeNodeID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field [%s], rc: %d",
                         FIELD_NAME_NODEID, rc ) ;
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
