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

   Source File Name = catDCManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =     XJH Opt

*******************************************************************************/

#include "catCommon.hpp"
#include "msgCatalog.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "catDCManager.hpp"
#include "clsDCMgr.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "catLocation.hpp"

using namespace bson ;

namespace engine
{

   #define CAT_DC_BLOCK_DFT_SIZE          ( 4096 )

   /*
      _catDCManager implement
   */
   _catDCManager::_catDCManager()
   :_mb( CAT_DC_BLOCK_DFT_SIZE )
   {
      _pDmsCB = NULL ;
      _pDpsCB = NULL ;
      _pRtnCB = NULL ;
      _pCatCB = NULL ;
      _pEduCB = NULL ;
      _pDCMgr = NULL ;
      _pDCBaseInfo = NULL ;
      _isWritedCmd = FALSE ;
      _pLogMgr = FALSE ;
      _isActived = FALSE ;
   }

   _catDCManager::~_catDCManager()
   {
   }

   INT32 _catDCManager::init()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb     = pmdGetKRCB() ;
      _pDmsCB           = krcb->getDMSCB();
      _pDpsCB           = krcb->getDPSCB();
      _pRtnCB           = krcb->getRTNCB();
      _pCatCB           = krcb->getCATLOGUECB();

      _pDCMgr           = SDB_OSS_NEW clsDCMgr() ;
      if ( !_pDCMgr )
      {
         PD_LOG( PDERROR, "Alloc dc manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _pDCBaseInfo = _pDCMgr->getDCBaseInfo() ;

      _pLogMgr          = SDB_OSS_NEW catDCLogMgr() ;
      if ( !_pLogMgr )
      {
         PD_LOG( PDERROR, "Alloc cat dc log manager failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = _pLogMgr->init() ;
      PD_RC_CHECK( rc, PDERROR, "Init system log manager failed, rc: %d",
                   rc ) ;

      _pCatCB->regEventHandler( this ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::fini ()
   {
      // Check the pointer in case that it is not initialized
      // before unregister the handler
      if ( _pCatCB )
      {
         _pCatCB->unregEventHandler( this ) ;
      }
      _pDCBaseInfo = NULL ;
      if ( _pDCMgr )
      {
         SDB_OSS_DEL _pDCMgr ;
         _pDCMgr = NULL ;
      }
      if ( _pLogMgr )
      {
         SDB_OSS_DEL _pLogMgr ;
         _pLogMgr = NULL ;
      }
      return SDB_OK ;
   }

   void _catDCManager::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;
      _pLogMgr->attachCB( cb ) ;

      /// ignore result
      _mapData2DCMgr( _pDCMgr ) ;
   }

   void _catDCManager::detachCB( pmdEDUCB * cb )
   {
      _pLogMgr->detachCB( cb ) ;
      _pEduCB = NULL ;
   }

   INT32 _catDCManager::updateGlobalAddr()
   {
      // not primary
      if ( !pmdIsPrimary() )
      {
         return SDB_CLS_NOT_PRIMARY ;
      }
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      return catUpdateBaseInfoAddr( pmdGetOptionCB()->getCatAddr().c_str(),
                                    TRUE, cb, 1 ) ;
   }

   BOOLEAN _catDCManager::isDCActivated() const
   {
      if ( _pDCBaseInfo )
      {
         return _pDCBaseInfo->isActivated() ;
      }
      return FALSE ;
   }

   BOOLEAN _catDCManager::isDCReadonly() const
   {
      if ( _pDCBaseInfo )
      {
         return _pDCBaseInfo->isReadonly() ;
      }
      return TRUE ;
   }

   BOOLEAN _catDCManager::isImageEnabled() const
   {
      if ( _pDCBaseInfo )
      {
         return _pDCBaseInfo->imageIsEnabled() ;
      }
      return FALSE ;
   }

   BOOLEAN _catDCManager::groupInImage( const string &groupName )
   {
      if ( _pDCBaseInfo )
      {
         if ( _pDCBaseInfo->getImageGroups()->find( groupName ) !=
              _pDCBaseInfo->getImageGroups()->end() )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _catDCManager::groupInImage( UINT32 groupID )
   {
      return groupInImage( _pCatCB->groupID2Name( groupID ) ) ;
   }

   INT32 _catDCManager::onBeginCommand ( MsgHeader *pMsg )
   {
      setWritedCommand( FALSE ) ;
      _lsn = _pDpsCB->expectLsn() ;

      return SDB_OK ;
   }

   INT32 _catDCManager::onEndCommand ( MsgHeader *pMsg, INT32 result )
   {
      INT32 rc = SDB_OK ;
      DPS_LSN expectLSN = _pDpsCB->expectLsn() ;

      if ( !_isActived )
      {
         goto done ;
      }

      // read lsn to expect lsn
      while( !expectLSN.invalid() &&
             _lsn.compareOffset( expectLSN.offset ) < 0 )
      {
         _mb.clear() ;
         rc = _pDpsCB->search( _lsn, &_mb, DPS_SEARCH_ALL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Search dps log[%d.%lld] failed, expect "
                    "lsn[%d.%lld], rc: %d",
                    _lsn.version, _lsn.offset, expectLSN.version,
                    expectLSN.offset, rc ) ;
            goto shutdown ;
         }
         else
         {
            dpsLogRecordHeader *header = ( dpsLogRecordHeader* )_mb.offset(0) ;
            _lsn.offset += header->_length ;
            _lsn.version = header->_version ;

            // save to log
            rc = _pLogMgr->saveSysLog( header ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Save system log[%d.%lld] failed, rc: %d",
                       header->_version, header->_lsn, rc ) ;
               goto shutdown ;
            }
         }
      }

   done:
      return rc ;
   shutdown:
      PD_LOG( PDSEVERE, "Stop program because save system log, rc: %d", rc ) ;
      PMD_RESTART_DB( rc ) ;
      goto done ;
   }

   INT32 _catDCManager::getCATVersion( UINT32 &version )
   {
      INT32 rc = SDB_OK ;

      // update catalog cache
      rc = updateDCCache() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update DC cache, rc: %d", rc ) ;

      version = _pDCBaseInfo->getCATVersion() ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _catDCManager::setCATVersion( UINT32 version )
   {
      INT32 rc = SDB_OK ;

      BSONObj matcher, updator, dummy ;

      try
      {
         matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
         updator = BSON( "$set" <<
                         BSON( FIELD_NAME_CAT_VERSION << (INT32)version ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher and updator, "
                 "occur exception %s", e.what() ) ;
      }

      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      dummy, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection [%s], rc: %d",
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      // update catalog cache
      updateDCCache() ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _catDCManager::active()
   {
      INT32 rc = SDB_OK;

      // update global info
      rc = _updateGlobalInfo() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update global info, rc: %d", rc ) ;

      // update imange info
      rc = _updateImageInfo() ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Update image info failed, rc: %d", rc ) ;
         // when update image info failed, ignore
         rc = SDB_OK ;
      }

      // update dc base info
      rc = _mapData2DCMgr( _pDCMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to map dc base info, rc: %d", rc ) ;

      // restore log manager
      rc = _pLogMgr->restore() ;
      PD_RC_CHECK( rc, PDERROR, "Restore system log failed, rc: %d", rc ) ;

      _isActived = TRUE ;

   done :
      return rc ;
   error :
      PD_LOG( PDSEVERE, "Stop program because of active dc manager failed, "
              "rc: %d", rc ) ;
      PMD_RESTART_DB( rc ) ;
      goto done ;
   }

   INT32 _catDCManager::deactive()
   {
      _isActived = FALSE ;
      return SDB_OK ;
   }

   INT32 _catDCManager::processMsg( const NET_HANDLE &handle,
                                    MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;

      switch ( pMsg->opCode )
      {
      // command message entry, should dispatch in the entry function
      case MSG_CAT_ALTER_IMAGE_REQ :
         rc = processCommandMsg( handle, pMsg, TRUE ) ;
         break ;

      default :
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG( PDWARNING, "Received unknown message (opCode: [%d]%u )",
                    IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode) ) ;
            break;
      }
      return rc ;
   }

   INT32 _catDCManager::processCommandMsg( const NET_HANDLE &handle,
                                           MsgHeader *pMsg,
                                           BOOLEAN writable )
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      MsgOpReply replyHeader ;
      rtnContextBuf ctxBuff ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;

      // init reply msg
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      _fillRspHeader( &(replyHeader.header), &(pQueryReq->header) ) ;

      // extract msg
      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG ( PDWARNING, "Service deactive but received command: %s, "
                     "opCode: %d, rc: %d", pCMDName,
                     pQueryReq->header.opCode, rc ) ;
            goto error ;
         }
      }

      // the second dispatch msg
      switch ( pQueryReq->header.opCode )
      {
         case MSG_CAT_ALTER_IMAGE_REQ :
            rc = processCmdAlterImage( handle, pQuery, ctxBuff ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Recieved unknow command: %s, opCode: %d",
                    pCMDName, pQueryReq->header.opCode ) ;
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Process command[%s] failed, opCode: %d, "
                   "rc: %d", pCMDName, pQueryReq->header.opCode, rc ) ;

   done:
      // send reply
      if ( !_pCatCB->isDelayed() )
      {
         if ( 0 == ctxBuff.size() )
         {
            rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
         }
         else
         {
            replyHeader.header.messageLength += ctxBuff.size() ;
            replyHeader.numReturned = ctxBuff.recordNum() ;
            rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                     (void *)ctxBuff.data(), ctxBuff.size() ) ;
         }
      }
      return rc ;
   error:
      replyHeader.flags = rc ;
      if( SDB_CLS_NOT_PRIMARY == rc )
      {
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done ;
   }

   INT32 _catDCManager::processCmdAlterImage( const NET_HANDLE &handle,
                                              const CHAR *pQuery,
                                              rtnContextBuf &ctxBuff )
   {
      INT32 rc = SDB_OK ;
      clsDCMgr dcMgr ;
      BSONObjBuilder retObjBuilder ;

      try
      {
         const CHAR *pAction = NULL ;
         BSONObj objQuery( pQuery ) ;
         BSONElement e = objQuery.getField( FIELD_NAME_ACTION ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "The field[%s] is not valid in command[%d]'s "
                    "param[%s]", FIELD_NAME_ACTION, MSG_CAT_ALTER_IMAGE_REQ,
                    objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pAction = e.valuestr() ;

         rc = _mapData2DCMgr( &dcMgr ) ;
         PD_RC_CHECK( rc, PDERROR, "Map dc base data to dc manager failed, "
                      "rc: %d", rc ) ;

         if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_CREATE ) )
         {
            rc = processCmdCreateImage( handle, &dcMgr, objQuery,
                                        retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_REMOVE ) )
         {
            rc = processCmdRemoveImage( handle, &dcMgr, objQuery,
                                        retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_ATTACH ) )
         {
            rc = processCmdAttachImage( handle, &dcMgr, objQuery,
                                        retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_DETACH ) )
         {
            rc = processCmdDetachImage( handle, &dcMgr, objQuery,
                                        retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_ENABLE ) )
         {
            rc = processCmdEnableImage( handle, &dcMgr, objQuery,
                                        retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_DISABLE ) )
         {
            rc = processCmdDisableImage( handle, &dcMgr, objQuery,
                                         retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_ACTIVATE ) )
         {
            rc = processCmdActivate( handle, &dcMgr, objQuery,
                                     retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_DEACTIVATE ) )
         {
            rc = processCmdDeactivate( handle, &dcMgr, objQuery,
                                       retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_ENABLE_READONLY ) )
         {
            rc = processCmdEnableReadonly( handle, &dcMgr,
                                           objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_DISABLE_READONLY ) )
         {
            rc = processCmdDisableReadonly( handle, &dcMgr,
                                            objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_SET_ACTIVE_LOCATION ) )
         {
            rc = processCmdSetActiveLocation( handle, &dcMgr,
                                              objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction,
                                       CMD_VALUE_NAME_SET_LOCATION ) )
         {
            rc = processCmdSetLocation( handle, &dcMgr,
                                        objQuery, retObjBuilder ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_START_MAINTENANCE_MODE ) )
         {
            rc = processCmdAlterMaintenanceMode( handle, &dcMgr,
                                                 objQuery, retObjBuilder, TRUE ) ;
         }
         else if ( 0 == ossStrcasecmp( pAction, CMD_VALUE_NAME_STOP_MAINTENANCE_MODE ) )
         {
            rc = processCmdAlterMaintenanceMode( handle, &dcMgr,
                                                 objQuery, retObjBuilder, FALSE ) ;
         }
         else
         {
            PD_LOG( PDERROR, "The value[%s] of field[%s] is not valid "
                    "in command[%d]'s param[%s]", pAction, FIELD_NAME_ACTION,
                    MSG_CAT_ALTER_IMAGE_REQ, objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // need to update dc base info
         _mapData2DCMgr( _pDCMgr ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Parse command[%d]'s param occur exception: %s",
                 MSG_CAT_ALTER_IMAGE_REQ, e.what() ) ;
         goto error ;
      }

      if ( SDB_OK == rc )
      {
         BSONObj retObj = retObjBuilder.obj() ;
         if ( retObj.isEmpty() )
         {
            BSONObjBuilder tmpBuild ;
            vector< string > tmpGroup ;
            _pCatCB->makeGroupsObj( tmpBuild, tmpGroup ) ;
            retObj = tmpBuild.obj() ;
         }
         // get return groups
         ctxBuff = rtnContextBuf( retObj ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdCreateImage( const NET_HANDLE &handle,
                                               _clsDCMgr *pDCMgr,
                                               const BSONObj &objQuery,
                                               BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      const CHAR *clusterName = NULL ;
      const CHAR *businessName = NULL ;
      string address ;

      BSONElement eleAddr = objQuery.getFieldDotted(
         FIELD_NAME_OPTIONS "." FIELD_NAME_ADDRESS ) ;

      if ( !pBaseInfo->hasImage() )
      {
         if ( String != eleAddr.type() ||
              0 == ossStrlen( eleAddr.valuestr() ) )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_OPTIONS "." FIELD_NAME_ADDRESS,
                    objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         rc = pDCMgr->setImageCatAddr( eleAddr.valuestr() ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse image catalog address failed, "
                      "rc: %d", rc ) ;
      }
      else if ( !eleAddr.eoo() )
      {
         if ( String != eleAddr.type() )
         {
            PD_LOG( PDERROR, "Param[%s] is invalid in obj[%s]",
                    FIELD_NAME_OPTIONS "." FIELD_NAME_ADDRESS,
                    objQuery.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( 0 != ossStrlen( eleAddr.valuestr() ) )
         {
            rc = SDB_CAT_IMAGE_IS_CONFIGURED ;
            goto error ;
         }
      }

      // update image catalog
      rc = pDCMgr->updateImageCataGroup( _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Update image catalog group failed, "
                   "rc: %d", rc ) ;
      address = pDCMgr->getImageCatAddr() ;
      // check the address is self cluster
      if ( _isAddrConflict( address, pmdGetOptionCB()->catAddrs() ) )
      {
         rc = SDB_CAT_IMAGE_ADDR_CONFLICT ;
         goto error ;
      }

      // update image dc base info
      rc = pDCMgr->updateImageDCBaseInfo( _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Update image dc base info failed, "
                   "rc: %d", rc ) ;
      clusterName = pDCMgr->getImageDCBaseInfo( _pEduCB,
                              FALSE )->getClusterName() ;
      businessName = pDCMgr->getImageDCBaseInfo( _pEduCB,
                              FALSE )->getBusinessName() ;

      // update info to collection
      {
         BSONObjBuilder builder ;
         if ( clusterName )
         {
            builder.append( FIELD_NAME_IMAGE "." FIELD_NAME_CLUSTERNAME,
                            clusterName ) ;
         }
         if ( businessName )
         {
            builder.append( FIELD_NAME_IMAGE "." FIELD_NAME_BUSINESSNAME,
                            businessName ) ;
         }
         if ( !address.empty() )
         {
            builder.append( FIELD_NAME_IMAGE "." FIELD_NAME_ADDRESS,
                            address ) ;
         }
         BSONObj updator = BSON( "$set" << builder.obj() ) ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                 CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObj hint ;
         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         hint, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize(),
                         NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                      "rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdRemoveImage( const NET_HANDLE &handle,
                                               _clsDCMgr *pDCMgr,
                                               const BSONObj &objQuery,
                                               BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      // not image
      if ( !pBaseInfo->hasImage() )
      {
         rc = SDB_CAT_IMAGE_NOT_CONFIG ;
         goto error ;
      }

      // send to all groups
      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      // update info to collection
      {
         BSONObj updator = BSON( "$unset" << BSON( FIELD_NAME_IMAGE << 0 ) ) ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                 CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObj hint ;
         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         hint, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize(),
                         NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                      "rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdAttachImage( const NET_HANDLE &handle,
                                               _clsDCMgr *pDCMgr,
                                               const BSONObj &objQuery,
                                               BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      nodeMgrAgent *pNodeAgent = NULL ;

      vector< string > vecSourceGrp ;
      BOOLEAN added = FALSE ;
      BSONElement eleGroups ;
      BSONObj objGroups ;

      if ( !pBaseInfo->hasImage() )
      {
         rc = SDB_CAT_IMAGE_NOT_CONFIG ;
         goto error ;
      }

      // update image groups
      rc = pDCMgr->updateImageAllGroups( _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Update image dc groups failed, rc: %d", rc ) ;

      pNodeAgent = pDCMgr->getImageNodeMgrAgent() ;

      // analysis groups
      eleGroups = objQuery.getFieldDotted(
         FIELD_NAME_OPTIONS "." FIELD_NAME_GROUPS ) ;
      if ( Array == eleGroups.type() )
      {
         objGroups = eleGroups.embeddedObject() ;
      }
      else if ( !eleGroups.eoo() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                 FIELD_NAME_OPTIONS "." FIELD_NAME_GROUPS,
                 objQuery.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // if objGroups is empty, will map all groups by the same name
      if ( objGroups.isEmpty() || 0 == objGroups.nFields() )
      {
         UINT32 tmpID = 0 ;
         vector< string > allGroups ;
         _pCatCB->getGroupsName( allGroups ) ;
         for ( UINT32 i = 0 ; i < allGroups.size() ; ++i )
         {
            pBaseInfo->addGroup( allGroups[ i ], allGroups[ i ], &added ) ;
            PD_RC_CHECK( rc, PDERROR, "Add group[%s:%s] failed when attach "
                         "image, rc: %d", allGroups[ i ].c_str(),
                         allGroups[ i ].c_str(), rc ) ;
            if ( added )
            {
               // check image group whether exist
               if ( SDB_OK != pNodeAgent->groupName2ID( allGroups[i].c_str(),
                                                        tmpID ) )
               {
                  PD_LOG( PDERROR, "Image group[%s] is not exist",
                          allGroups[i].c_str() ) ;
                  rc = SDB_CLS_GRP_NOT_EXIST ;
                  goto error ;
               }
               vecSourceGrp.push_back( allGroups[ i ] ) ;
            }
         }
      }
      else
      {
         map< string, string > mapAddGrps ;
         map< string, string >::iterator it ;
         rc = pBaseInfo->addGroups( objGroups, &mapAddGrps ) ;
         PD_RC_CHECK( rc, PDERROR, "Add groups[%s] failed when attach "
                      "image, rc: %d", objGroups.toString().c_str(), rc ) ;
         rc = _checkGroupsValid( mapAddGrps, pNodeAgent ) ;
         PD_RC_CHECK( rc, PDERROR, "Groups[%s] is not all valid, rc: %d",
                      objGroups.toString().c_str(), rc ) ;
         it = mapAddGrps.begin() ;
         while ( it != mapAddGrps.end() )
         {
            vecSourceGrp.push_back( it->first ) ;
            ++it ;
         }
      }

      // add catalog group
      pBaseInfo->addGroup( CATALOG_GROUPNAME, CATALOG_GROUPNAME, &added ) ;
      if ( added )
      {
         vecSourceGrp.push_back( CATALOG_GROUPNAME ) ;
      }

      // construct return object
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecSourceGrp ) ;
      PD_RC_CHECK( rc, PDERROR, "Make groups obj failed, rc: %d", rc ) ;

      // update info to collection
      {
         BSONObjBuilder builder ;
         _dcBaseInfoGroups2Obj( pBaseInfo, builder,
                                FIELD_NAME_IMAGE "." FIELD_NAME_GROUPS ) ;
         BSONObj updator = BSON( "$set" << builder.obj() ) ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                 CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObj hint ;
         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         hint, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize(),
                         NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                      "rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDetachImage( const NET_HANDLE &handle,
                                               _clsDCMgr *pDCMgr,
                                               const BSONObj &objQuery,
                                               BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      BSONElement eleGroups ;
      BSONObj objGroups ;
      vector< string > vecGroups ;

      // detach only when disable
      if ( pBaseInfo->imageIsEnabled() )
      {
         rc = SDB_CAT_IMAGE_IS_ENABLED ;
         goto error ;
      }
      // not image
      else if ( !pBaseInfo->hasImage() )
      {
         rc = SDB_CAT_IMAGE_NOT_CONFIG ;
         goto error ;
      }

      // analysis groups
      eleGroups = objQuery.getField( FIELD_NAME_OPTIONS "." FIELD_NAME_GROUPS ) ;
      if ( Array == eleGroups.type() )
      {
         objGroups = eleGroups.embeddedObject() ;
      }
      else if ( !eleGroups.eoo() )
      {
         PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                 FIELD_NAME_OPTIONS "." FIELD_NAME_GROUPS,
                 objQuery.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // if objGroups is empty, will detach all groups
      if ( objGroups.isEmpty() || 0 == objGroups.nFields() )
      {
         map<string, string> *pImgGrps = pBaseInfo->getImageGroups() ;
         map<string, string>::iterator it = pImgGrps->begin() ;
         while ( it != pImgGrps->end() )
         {
            vecGroups.push_back( it->first ) ;
            ++it ;
         }
         pImgGrps->clear() ;
      }
      else
      {
         map< string, string > mapDelGrps ;
         map< string, string >::iterator it ;
         rc = pBaseInfo->delGroups( objGroups, &mapDelGrps ) ;
         PD_RC_CHECK( rc, PDERROR, "Del groups[%s] failed when detach "
                      "image, rc: %d", objGroups.toString().c_str(), rc ) ;
         it = mapDelGrps.begin() ;
         while ( it != mapDelGrps.end() )
         {
            vecGroups.push_back( it->first ) ;
            ++it ;
         }
      }

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      // update info to collection
      {
         BSONObjBuilder builder ;
         _dcBaseInfoGroups2Obj( pBaseInfo, builder,
                                FIELD_NAME_IMAGE "." FIELD_NAME_GROUPS ) ;
         BSONObj updator = BSON( "$set" << builder.obj() ) ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                 CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObj hint ;
         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         hint, 0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize(),
                         NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                      "rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdEnableImage( const NET_HANDLE &handle,
                                               _clsDCMgr *pDCMgr,
                                               const BSONObj &objQuery,
                                               BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > allGroups ;

      // is already enable
      if ( pBaseInfo->imageIsEnabled() )
      {
         goto done ;
      }
      else if ( !pBaseInfo->hasImage() )
      {
         rc = SDB_CAT_IMAGE_NOT_CONFIG ;
         goto error ;
      }

      // check is dual writable
      if ( !pBaseInfo->isReadonly() )
      {
         // check image wether is non-readonly
         rc = pDCMgr->updateImageDCBaseInfo( _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Update image dc base info failed, rc: %d",
                      rc ) ;
         if ( !pDCMgr->getImageDCBaseInfo( _pEduCB, FALSE )->isReadonly() )
         {
            rc = SDB_CAT_DUAL_WRITABLE ;
            goto error ;
         }
      }

      // check image' all groups has image
      rc = pDCMgr->updateImageAllGroups( _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Update image all groups failed, rc: %d",
                   rc ) ;
      pDCMgr->getImageNodeMgrAgent()->getGroupsName( allGroups ) ;
      for ( UINT32 i = 0 ; i < allGroups.size() ; ++i )
      {
         if ( pBaseInfo->getRImageGroups()->find( allGroups[i] ) ==
              pBaseInfo->getRImageGroups()->end() )
         {
            PD_LOG( PDERROR, "Image group[%s] does not have source group",
                    allGroups[i].c_str() ) ;
            rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
            goto error ;
         }
      }

      // check dc all groups has image
      _pCatCB->getGroupsName( allGroups ) ;
      allGroups.push_back( CATALOG_GROUPNAME ) ;
      for( UINT32 i = 0 ; i < allGroups.size() ; ++i )
      {
         if ( pBaseInfo->getImageGroups()->find( allGroups[i] ) ==
              pBaseInfo->getImageGroups()->end() )
         {
            PD_LOG( PDERROR, "Group[%s] does not have image group",
                    allGroups[i].c_str() ) ;
            rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
            goto error ;
         }
      }

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      // update "enable" to collection
      rc = catEnableImage( TRUE, _pEduCB, _majoritySize(), _pDmsCB, _pDpsCB ) ;
      if ( rc )
      {
         // rollback
         catEnableImage( FALSE, _pEduCB, 1, _pDmsCB, _pDpsCB ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDisableImage( const NET_HANDLE &handle,
                                                _clsDCMgr *pDCMgr,
                                                const BSONObj &objQuery,
                                                BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;

      if ( !pBaseInfo->hasImage() )
      {
         rc = SDB_CAT_IMAGE_NOT_CONFIG ;
         goto error ;
      }
      else if ( !pBaseInfo->imageIsEnabled() )
      {
         goto done ;
      }
      else
      {
         vector< string > vecGroups ;
         map<string, string> *pMapGrps = pBaseInfo->getImageGroups() ;
         map<string, string>::iterator it = pMapGrps->begin() ;
         while( it != pMapGrps->end() )
         {
            vecGroups.push_back( it->first ) ;
            ++it ;
         }
         // make return obj
         rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                      rc ) ;

         // update to collection
         rc = catEnableImage( FALSE, _pEduCB, _majoritySize(), _pDmsCB,
                              _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catEnableImage( TRUE, _pEduCB, 1, _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdActivate( const NET_HANDLE &handle,
                                            _clsDCMgr *pDCMgr,
                                            const BSONObj &objQuery,
                                            BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( !pBaseInfo->isActivated() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_ACTIVATED, TRUE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_ACTIVATED, FALSE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDeactivate( const NET_HANDLE &handle,
                                              _clsDCMgr *pDCMgr,
                                              const BSONObj &objQuery,
                                              BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( pBaseInfo->isActivated() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_ACTIVATED, FALSE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_ACTIVATED, TRUE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdEnableReadonly( const NET_HANDLE &handle,
                                                  _clsDCMgr *pDCMgr,
                                                  const BSONObj &objQuery,
                                                  BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( !pBaseInfo->isReadonly() )
      {
         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_READONLY, TRUE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_READONLY, FALSE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdDisableReadonly( const NET_HANDLE &handle,
                                                   _clsDCMgr *pDCMgr,
                                                   const BSONObj &objQuery,
                                                   BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      clsDCBaseInfo *pBaseInfo = pDCMgr->getDCBaseInfo() ;
      vector< string > vecGroups ;

      _pCatCB->getGroupsName( vecGroups ) ;
      vecGroups.push_back( CATALOG_GROUPNAME ) ;

      // make return obj
      rc = _pCatCB->makeGroupsObj( retObjBuilder, vecGroups ) ;
      PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d",
                   rc ) ;

      if ( pBaseInfo->isReadonly() )
      {
         if ( pBaseInfo->hasImage() && pBaseInfo->imageIsEnabled() )
         {
            if ( SDB_OK == pDCMgr->updateImageDCBaseInfo( _pEduCB ) )
            {
               // if can update image's dc base info, need to check wether it
               // is readonly
               if ( !pDCMgr->getImageDCBaseInfo( _pEduCB, FALSE )->isReadonly() )
               {
                  rc = SDB_CAT_DUAL_WRITABLE ;
                  goto error ;
               }
            }
         }

         // update to collection
         rc = catUpdateDCStatus( FIELD_NAME_READONLY, FALSE, _pEduCB,
                                 _majoritySize(), _pDmsCB, _pDpsCB ) ;
         if ( rc )
         {
            // rollback
            catUpdateDCStatus( FIELD_NAME_READONLY, TRUE, _pEduCB, 1,
                               _pDmsCB, _pDpsCB ) ;
            goto error ;
         }
      }

      // force to secondary to reelect, then the primary node can resume
      // active works did not finish in read-only mode
      _pCatCB->setNeedForceSecondary( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdSetActiveLocation( const NET_HANDLE &handle,
                                                     _clsDCMgr *pDCMgr,
                                                     const BSONObj &objQuery,
                                                     BSONObjBuilder &retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      ossPoolString newActLoc ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get new ActiveLocation, this field should not be empty
         ele = options.getField( CAT_ACTIVE_LOCATION_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
            goto error ;
         }
         // ele.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < ele.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         newActLoc = ele.valuestrsafe() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString oldActLoc ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            // Check and get active location
            rc = catCheckAndGetActiveLocation( groupObj, groupID, newActLoc, oldActLoc ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get and check active location, rc: %d", rc ) ;
               continue ;
            }

            // Compare oldLocation and newLocation
            if ( oldActLoc == newActLoc )
            {
               PD_LOG( PDDEBUG, "The old and new ActiveLocation are same, do nothing" ) ;
               continue ;
            }

            // Set new ActiveLocation
            if ( ! newActLoc.empty() )
            {
               rc = pCatNodeMgr->setActiveLocation( groupID, newActLoc ) ;
            }
            // Remove old ActiveLocation
            else
            {
               rc = pCatNodeMgr->removeActiveLocation( groupID ) ;
            }
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to set active location, rc: %d", rc ) ;
               continue ;
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdSetLocation( const NET_HANDLE & handle,
                                               _clsDCMgr * pDCMgr,
                                               const BSONObj & objQuery,
                                               BSONObjBuilder & retObjBuilder )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      ossPoolString newLoc ;
      ossPoolString hostName ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get new Location, this field should not be empty
         ele = options.getField( CAT_LOCATION_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_LOCATION_NAME ) ;
            goto error ;
         }
         // ele.valuestrsize include the length of '\0'
         if ( MSG_LOCATION_NAMESZ < ele.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         newLoc = ele.valuestrsafe() ;

         ele = options.getField( CAT_HOST_FIELD_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Failed to get the field[%s]", CAT_HOST_FIELD_NAME ) ;
            goto error ;
         }
         hostName = ele.valuestrsafe() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;
         allGroups.push_back( COORD_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString oldActLoc ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            rc = pCatNodeMgr->setGroupLocation( groupObj, groupID, newLoc, hostName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to set group location, rc: %d", rc  ) ;
               continue ;
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::processCmdAlterMaintenanceMode( const NET_HANDLE &handle,
                                                        _clsDCMgr *pDCMgr,
                                                        const BSONObj &objQuery,
                                                        BSONObjBuilder &retObjBuilder,
                                                        const BOOLEAN &isStartMode )
   {
      INT32 rc = SDB_OK ;
      BSONObj options ;
      BSONElement ele ;
      CAT_GROUP_LIST allGroups ;
      CAT_GROUP_LIST failedGroups ;
      catNodeManager* pCatNodeMgr = _pCatCB->getCatNodeMgr() ;

      try
      {
         // Get options
         ele = objQuery.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, objQuery.toPoolString().c_str() ) ;
            goto error ;
         }
         options = ele.embeddedObject() ;

         // Get all groups
         _pCatCB->getGroupsID( allGroups, FALSE ) ;
         allGroups.push_back( CATALOG_GROUPID ) ;

         CAT_GROUP_LIST::const_iterator itr = allGroups.begin() ;
         while ( allGroups.end() != itr )
         {
            BSONObj groupObj ;
            string groupName ;
            ossPoolString hostName ;
            clsGrpModeItem item ;
            clsGroupMode groupMode ;
            catCtxLockMgr lockMgr ;
            UINT32 groupID = *itr++ ;

            // Get group obj by group id
            rc = catGetGroupObj( groupID, groupObj, _pEduCB ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] obj, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Check group mode info
            rc = _checkMaintenanceMode( options, groupObj, groupID,
                                        isStartMode, groupMode, &hostName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to check group[%u] mode info, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Get group name
            rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, groupName ) ;
            if ( SDB_OK != rc )
            {
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to get group[%u] name, rc: %d", groupID, rc ) ;
               continue ;
            }

            // Lock group
            if ( ! lockMgr.tryLockGroup( groupName, EXCLUSIVE ) )
            {
               rc = SDB_LOCK_FAILED ;
               failedGroups.push_back( groupID ) ;
               PD_LOG( PDERROR, "Failed to lock group [%s], rc: %d", groupName.c_str(), rc ) ;
               continue ;
            }

            if ( isStartMode )
            {
               rc = pCatNodeMgr->startGrpMode( groupMode, groupName, groupObj ) ;
               if ( SDB_OK != rc )
               {
                  failedGroups.push_back( groupID ) ;
                  PD_LOG( PDERROR, "Failed to start maintenance mode on group[%u], rc: %d", groupID, rc ) ;
                  continue ;
               }
            }
            else
            {
               rc = pCatNodeMgr->stopGrpMode( groupMode ) ; 
               if ( SDB_OK != rc )
               {
                  failedGroups.push_back( groupID ) ;
                  PD_LOG( PDERROR, "Failed to stop maintenance mode on group[%u], rc: %d", groupID, rc ) ;
                  continue ;
               }
            }
         }

         rc = _pCatCB->makeGroupsObj( retObjBuilder, allGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

         rc = _pCatCB->makeFailedGroupsObj( retObjBuilder, failedGroups ) ;
         PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _catDCManager::_fillRspHeader( MsgHeader * rspMsg,
                                       const MsgHeader * reqMsg )
   {
      rspMsg->opCode = MAKE_REPLY_TYPE( reqMsg->opCode ) ;
      rspMsg->requestID = reqMsg->requestID ;
      rspMsg->routeID.value = 0 ;
      rspMsg->TID = reqMsg->TID ;
      rspMsg->globalID = reqMsg->globalID ;
   }

   INT32 _catDCManager::_mapData2DCMgr( _clsDCMgr *pDCMgr )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      BSONObj infoObj ;

      rc = pDCMgr->initialize() ;
      PD_RC_CHECK( rc, PDERROR, "Init dc manager failed, rc: %d", rc ) ;

      // get data
      rc = catCheckBaseInfoExist( CAT_BASE_TYPE_GLOBAL_STR, exist,
                                  infoObj, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check dc base info exist failed, "
                   "rc: %d", rc ) ;

      if ( exist )
      {
         rc = pDCMgr->updateDCBaseInfo( infoObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Update dc base info failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _catDCManager::_dcBaseInfoGroups2Obj( _clsDCBaseInfo *pInfo,
                                              BSONObjBuilder &builder,
                                              const CHAR *pFieldName )
   {
      BSONArrayBuilder arrayBuild( builder.subarrayStart( pFieldName ) ) ;
      map<string, string> *pGroups = pInfo->getImageGroups() ;
      map<string, string>::iterator it = pGroups->begin() ;
      while ( it != pGroups->end() )
      {
         arrayBuild.append( BSON_ARRAY( it->first << it->second ) ) ;
         ++it ;
      }
      arrayBuild.doneFast() ;
   }

   BOOLEAN _catDCManager::_isAddrConflict( const string &addr,
                                           const vector< pmdAddrPair > &dstAddr )
   {
      BOOLEAN conflict = FALSE ;
      vector< pmdAddrPair > vecAddr ;
      pmdOptionsCB option ;
      option.parseAddressLine( addr.c_str(), vecAddr ) ;

      for ( UINT32 i = 0 ; i < vecAddr.size() ; ++i )
      {
         pmdAddrPair &item = vecAddr[ i ] ;
         for ( UINT32 j = 0 ; j < dstAddr.size() ; ++j )
         {
            const pmdAddrPair &self = dstAddr[ j ] ;

            if ( 0 == ossStrcmp( item._host, self._host ) &&
                 0 == ossStrcmp( item._service, self._service ) )
            {
               conflict = TRUE ;
               goto done ;
            }
         }
      }

   done:
      return conflict ;
   }

   INT32 _catDCManager::_checkGroupsValid( map< string, string > &mapGroups,
                                           nodeMgrAgent *pNodeAgent )
   {
      INT32 rc = SDB_OK ;
      UINT32 groupID = CAT_INVALID_GROUPID ;
      map< string, string >::iterator it = mapGroups.begin() ;
      while( it != mapGroups.end() )
      {
         groupID = _pCatCB->groupName2ID( it->first ) ;
         if ( CAT_INVALID_GROUPID == groupID )
         {
            PD_LOG( PDERROR, "Group[%s] is not exist", it->first.c_str() ) ;
            rc = SDB_CLS_GRP_NOT_EXIST ;
            break ;
         }
         else if ( COORD_GROUPID == groupID )
         {
            PD_LOG( PDERROR, "Coord group[%s] can not image",
                    it->first.c_str() ) ;
            rc = SDB_CAT_IS_NOT_DATAGROUP ;
            break ;
         }
         else if ( SDB_OK != pNodeAgent->groupName2ID( it->second.c_str(),
                                                       groupID ) )
         {
            PD_LOG( PDERROR, "Image group[%s] is not exist",
                    it->second.c_str() ) ;
            rc = SDB_CLS_GRP_NOT_EXIST ;
            break ;
         }
         ++it ;
      }
      return rc ;
   }

   INT16 _catDCManager::_majoritySize()
   {
      return _pCatCB->majoritySize() ;
   }

   INT32 _catDCManager::_updateGlobalInfo()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      BSONObj infoObj ;
      pmdOptionsCB *option = pmdGetOptionCB() ;

      string clusterName ;
      string businessName ;
      option->getFieldStr( PMD_OPTION_CLUSTER_NAME, clusterName, "" ) ;
      option->getFieldStr( PMD_OPTION_BUSINESS_NAME, businessName, "" ) ;

      rc = catCheckBaseInfoExist( CAT_BASE_TYPE_GLOBAL_STR, exist,
                                  infoObj, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Check dc base info exist failed, "
                   "rc: %d", rc ) ;

      if ( !exist )
      {
         // if the global info not exist, need to create
         infoObj = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR <<
                         FIELD_NAME_DATACENTER << BSON(
                           FIELD_NAME_CLUSTERNAME << clusterName <<
                           FIELD_NAME_BUSINESSNAME << businessName <<
                           FIELD_NAME_ADDRESS << option->getCatAddr() ) <<
                         FIELD_NAME_ACTIVATED << true <<
                         FIELD_NAME_READONLY << false <<
                         FIELD_NAME_CSUNIQUEHWM << 0 <<
                         FIELD_NAME_TASKHWM << 0 <<
                         FIELD_NAME_CAT_VERSION << CATALOG_VERSION_CUR <<
                         FIELD_NAME_RECYCLEBIN <<
                         BSON( FIELD_NAME_ENABLE <<
                                     (bool)( UTIL_RECYCLEBIN_DFT_ENABLE ) <<
                               FIELD_NAME_RECYCLEIDHWM << (INT64)0 <<
                               FIELD_NAME_EXPIRETIME <<
                                     UTIL_RECYCLEBIN_DFT_EXPIRETIME <<
                               FIELD_NAME_MAXITEMNUM <<
                                     UTIL_RECYCLEBIN_DFT_MAXITEMNUM <<
                               FIELD_NAME_MAXVERNUM <<
                                     UTIL_RECYCLEBIN_DFT_MAXVERNUM <<
                               FIELD_NAME_AUTODROP <<
                                     (bool)( UTIL_RECYCLEBIN_DFT_AUTODROP ) ) ) ;
         rc = rtnInsert( CAT_SYSDCBASE_COLLECTION_NAME, infoObj, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Insert global info[%s] to collection[%s] "
                      "failed, rc: %d", infoObj.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }
      else
      {
         utilUpdateResult upResult ;
         string tmpClsName ;
         string tmpBusName ;
         clsDCBaseInfo dcBaseInfo ;

         // update dc base info
         rc = dcBaseInfo.updateFromBSON( infoObj, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse dc base info[%s] failed, rc: %d",
                      infoObj.toString().c_str() ) ;

         if ( !dcBaseInfo.isReadonly() )
         {
            tmpClsName = dcBaseInfo.getClusterName() ;
            tmpBusName = dcBaseInfo.getBusinessName() ;

            if ( clusterName != tmpClsName || businessName != tmpBusName )
            {
               PD_LOG( PDEVENT, "Cluster name[%s] or business name[%s] has "
                       "changed to %s:%s", tmpClsName.c_str(), tmpBusName.c_str(),
                       clusterName.c_str(), businessName.c_str() ) ;
               BSONObj updator = BSON( "$set" << BSON(
                 FIELD_NAME_DATACENTER "." FIELD_NAME_CLUSTERNAME << clusterName <<
                 FIELD_NAME_DATACENTER "." FIELD_NAME_BUSINESSNAME << businessName )
                                      ) ;
               BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                       CAT_BASE_TYPE_GLOBAL_STR ) ;
               rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                               BSONObj(), 0, _pEduCB, _pDmsCB, _pDpsCB, 1,
                               &upResult ) ;
               PD_RC_CHECK( rc, PDERROR, "Update global info[%s] failed, rc: %d",
                            updator.toString().c_str(), rc ) ;
               if ( upResult.updateNum() <= 0 )
               {
                  PD_LOG( PDERROR, "Not found global info, matcher: %s",
                          matcher.toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            }

            // add recycle bin if not exists
            if ( !infoObj.hasField( FIELD_NAME_RECYCLEBIN ) )
            {
               BSONObj updator =
                     BSON( "$set" <<
                           BSON( FIELD_NAME_RECYCLEBIN <<
                                 BSON( FIELD_NAME_ENABLE <<
                                          (bool)( UTIL_RECYCLEBIN_DFT_ENABLE ) <<
                                       FIELD_NAME_RECYCLEIDHWM << (INT64)0 <<
                                       FIELD_NAME_EXPIRETIME <<
                                             UTIL_RECYCLEBIN_DFT_EXPIRETIME <<
                                       FIELD_NAME_MAXITEMNUM <<
                                             UTIL_RECYCLEBIN_DFT_MAXITEMNUM <<
                                       FIELD_NAME_MAXVERNUM <<
                                             UTIL_RECYCLEBIN_DFT_MAXVERNUM <<
                                       FIELD_NAME_AUTODROP <<
                                          (bool)( UTIL_RECYCLEBIN_DFT_AUTODROP ) ) ) ) ;

               BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                       CAT_BASE_TYPE_GLOBAL_STR ) ;
               rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                               BSONObj(), 0, _pEduCB, _pDmsCB, _pDpsCB, 1,
                               &upResult ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to update recycle info [%s], "
                            "rc: %d", updator.toString().c_str(), rc ) ;
               PD_CHECK( upResult.updateNum() > 0, SDB_SYS, error, PDERROR,
                         "Not found global info, matcher: %s",
                         matcher.toString().c_str() ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_updateImageInfo()
   {
      INT32 rc = SDB_OK ;
      clsDCMgr dcMgr ;
      clsDCBaseInfo *pBaseInfo = NULL ;
      clsDCBaseInfo *pImageBaseInfo = NULL ;

      rc = _mapData2DCMgr( &dcMgr ) ;
      PD_RC_CHECK( rc, PDERROR, "Map dc base data to dc manager failed, "
                   "rc: %d", rc ) ;

      pBaseInfo = dcMgr.getDCBaseInfo() ;

      // if has image, need to update image's cluster name, business name and
      // cat address
      // because catalog is a single thread, so don't lock_r
      if ( pBaseInfo->hasImage() )
      {
         BSONObjBuilder builder ;
         BSONObj newObj ;
         string catAddr ;
         const CHAR *imageClsName = NULL ;
         const CHAR *imageBsName = NULL ;

         rc = dcMgr.updateImageCataGroup( _pEduCB ) ;
         PD_RC_CHECK( rc, PDWARNING, "Update image catagroup failed, rc: %d",
                      rc ) ;

         // if catalog address has update, need to reflush
         catAddr = dcMgr.getImageCatAddr() ;
         if ( 0 != ossStrcmp( catAddr.c_str(),
                              pBaseInfo->getImageAddress() ) )
         {
            rc = catUpdateBaseInfoAddr( catAddr.c_str(), FALSE, _pEduCB, 1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Update image address to dc base info "
                         "failed, rc: %d", rc ) ;
         }

         // if image clustername or businessname has update, need to reflush
         rc = dcMgr.updateImageDCBaseInfo( _pEduCB ) ;
         PD_RC_CHECK( rc, PDWARNING, "Update image dc base info failed, "
                      "rc: %d", rc ) ;
         pImageBaseInfo = dcMgr.getImageDCBaseInfo( _pEduCB, FALSE ) ;
         imageClsName = pImageBaseInfo->getClusterName() ;
         imageBsName = pImageBaseInfo->getBusinessName() ;

         if ( 0 != ossStrcmp( pBaseInfo->getImageClusterName(),
                              imageClsName ) )
         {
            builder.append( FIELD_NAME_IMAGE "." FIELD_NAME_CLUSTERNAME,
                            imageClsName ) ;
         }
         if ( 0 != ossStrcmp( pBaseInfo->getImageBusinessName(),
                              imageBsName ) )
         {
            builder.append( FIELD_NAME_IMAGE "." FIELD_NAME_BUSINESSNAME,
                            imageBsName ) ;
         }
         newObj = builder.obj() ;
         if ( !newObj.isEmpty() )
         {
            BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                                    CAT_BASE_TYPE_GLOBAL_STR ) ;
            BSONObj updator = BSON( "$set" << newObj ) ;
            BSONObj hint ;
            rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher,
                            updator, hint, 0, _pEduCB, _pDmsCB, _pDpsCB,
                            1, NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] "
                         "failed, rc: %d", updator.toString().c_str(),
                         CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_checkMaintenanceMode( const BSONObj &option,
                                               const BSONObj &groupObj,
                                               const UINT32 &groupID,
                                               const BOOLEAN &isStartMode,
                                               clsGroupMode &groupMode,
                                               ossPoolString *pHostName )
   {
      INT32 rc = SDB_OK ;

      BSONElement grpModeEle, primaryEle ;

      // Init groupMode
      groupMode.groupID = groupID ;
      groupMode.mode = CLS_GROUP_MODE_MAINTENANCE ;

      // Check if this group is in critical mode
      grpModeEle = groupObj.getField( CAT_GROUP_MODE_NAME ) ;
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
                        "critical mode is operating", isStartMode ? "start" : "stop", groupID ) ;
            goto error ;
         }
      }
      // If the command is stop maintenance mode, do nothing
      else if ( ! isStartMode )
      {
         goto done ;
      }

      // If the command is stop maintenance mode and options is empty, it means stop all maintenance mode
      if ( ! isStartMode && option.isEmpty() )
      {
         goto done ;
      }

      rc = catParseGroupModeInfo( option, groupObj, groupID, isStartMode, groupMode, pHostName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse group mode info[%s], rc: %d",
                   option.toPoolString().c_str(), rc ) ;

      rc = _buildGroupModeInfo( groupObj, *pHostName, groupMode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build group[%u] mode info, rc: %d", groupID, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catDCManager::_buildGroupModeInfo( const BSONObj &groupObj,
                                             const ossPoolString &hostName,
                                             clsGroupMode &groupMode )
   {
      INT32 rc = SDB_OK ;

      clsGrpModeItem item = groupMode.grpModeInfo[0] ;
      groupMode.grpModeInfo.clear() ;

      BSONObjIterator nodeItr ;
      BSONObj nodeListObj ;
      ossPoolString tmpHostName ;
      ossPoolString tmpSvcName ;

      rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, nodeListObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d", CAT_GROUP_NAME, rc ) ;

      // Parse nodeList array
      nodeItr = BSONObjIterator( nodeListObj ) ;
      while ( nodeItr.more() )
      {
         clsGrpModeItem tmpItem ;
         BSONObj boNode = nodeItr.next().embeddedObject() ;

         if ( ! item.location.empty() )
         {
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

         }
         else if ( ! hostName.empty() )
         {
            BSONElement beHostName = boNode.getField( FIELD_NAME_HOST ) ;
            if ( beHostName.eoo() )
            {
               continue ;
            }
            else if ( String != beHostName.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                       FIELD_NAME_HOST, beHostName.type() ) ;
               goto error ;
            }
            else if ( 0 != ossStrcmp( beHostName.valuestrsafe(), hostName.c_str() ) )
            {
               continue ;
            }
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

         groupMode.grpModeInfo.push_back( tmpItem ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}
