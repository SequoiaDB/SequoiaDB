/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordDataSource.cpp

   Descriptive Name = Coordinator data source.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordDataSource.hpp"
#include "coordSdbDataSource.hpp"
#include "coordRemoteMsgHandler.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "netRouteAgent.hpp"
#include "pmdEDU.hpp"
#include "coordFactory.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "coordCB.hpp"
#include "coordDSMsgConvertor.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_DS_NET_HANDLE_BEGINID             ( 0x80000000 )

   /*
      _coordDataSourceMgr implement
   */
   _coordDataSourceMgr::_coordDataSourceMgr()
   {
      _pDSAgent = NULL ;
      _pDSMsgHandler = NULL ;
      _pDSMsgConvertor = NULL ;
   }

   _coordDataSourceMgr::~_coordDataSourceMgr()
   {
      fini() ;
   }

   INT32 _coordDataSourceMgr::init( pmdRemoteSessionMgr *pRemoteMgr )
   {
      INT32 rc = SDB_OK ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;

      _pDSMsgHandler = SDB_OSS_NEW _coordDataSourceMsgHandler( pRemoteMgr,
                                                               this ) ;
      if ( !_pDSMsgHandler )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate data source message handler failed" ) ;
         goto error ;
      }

      _pDSAgent = SDB_OSS_NEW _netRouteAgent( _pDSMsgHandler,
                                              COORD_DS_NET_HANDLE_BEGINID ) ;
      if ( !_pDSAgent )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate data source route agent failed" ) ;
         goto error ;
      }

      /// set net info
      _pDSAgent->getFrame()->setBeatInfo( optCB->getOprTimeout() ) ;
      _pDSAgent->getFrame()->setMaxSockPerNode(
         optCB->maxSockPerNode() ) ;
      _pDSAgent->getFrame()->setMaxSockPerThread(
         optCB->maxSockPerThread() ) ;
      _pDSAgent->getFrame()->setMaxThreadNum(
         optCB->maxSockThread() ) ;

      _pDSMsgConvertor = SDB_OSS_NEW _coordDSMsgConvertor() ;
      if ( !_pDSMsgConvertor )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allcoate data source message convertor failed" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordDataSourceMgr::fini()
   {
      clear() ;

      if ( _pDSMsgConvertor )
      {
         SDB_OSS_DEL _pDSMsgConvertor ;
         _pDSMsgConvertor = NULL ;
      }

      if ( _pDSAgent )
      {
         SDB_OSS_DEL _pDSAgent ;
         _pDSAgent = NULL ;
      }

      if ( _pDSMsgHandler )
      {
         SDB_OSS_DEL _pDSMsgHandler ;
         _pDSMsgHandler = NULL ;
      }
   }

   INT32 _coordDataSourceMgr::active()
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr* pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // 1. start net work
      rc = pEDUMgr->startEDU ( EDU_TYPE_COORD_DS_NETWORK, (void*)_pDSAgent,
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start data source network edu, "
                   "rc: %d", rc ) ;
      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait DSNetwork active failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordDataSourceMgr::deactive()
   {
      if ( _pDSAgent )
      {
         _pDSAgent->stop() ;
      }
      return SDB_OK ;
   }

   void _coordDataSourceMgr::onConfigChange()
   {
      pmdOptionsCB * optCB = pmdGetOptionCB() ;

      if ( _pDSAgent )
      {
         UINT32 oprtimeout = optCB->getOprTimeout() ;
         _pDSAgent->getFrame()->setBeatInfo( oprtimeout ) ;
         _pDSAgent->getFrame()->setMaxSockPerNode(
            optCB->maxSockPerNode() ) ;
         _pDSAgent->getFrame()->setMaxSockPerThread(
            optCB->maxSockPerThread() ) ;
         _pDSAgent->getFrame()->setMaxThreadNum(
            optCB->maxSockThread() ) ;
      }
   }

   void _coordDataSourceMgr::checkSubSession( const MsgRouteID &routeID,
                                              netRouteAgent **ppAgent,
                                              IRemoteMsgConvertor **ppMsgConvertor,
                                              BOOLEAN &isExternConn )
   {
      if ( SDB_IS_DSID( routeID.columns.groupID ) )
      {
         *ppAgent = _pDSAgent ;
         *ppMsgConvertor = _pDSMsgConvertor ;
         isExternConn = TRUE ;
      }
      else
      {
         *ppAgent = NULL ;
         *ppMsgConvertor = NULL ;
         isExternConn = FALSE ;
      }
   }

   BOOLEAN _coordDataSourceMgr::canSwitchOtherNode( INT32 err )
   {
      if ( SDB_COORD_DATASOURCE_PERM_DENIED == err ||
           SDB_COORD_DATASOURCE_TRANS_FORBIDDEN == err ||
           SDB_OPERATION_INCOMPATIBLE == err ||
           SDB_AUTH_AUTHORITY_FORBIDDEN == err ||
           SDB_DMS_NOTEXIST == err ||
           SDB_DMS_CS_NOTEXIST == err )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _coordDataSourceMgr::canErrFilterOut( _pmdSubSession *pSub,
                                                 const MsgHeader *pOrgMsg,
                                                 INT32 err )
   {
      BOOLEAN canFilterOut = FALSE ;

      if ( SDB_IS_DSID( pSub->getNodeID().columns.groupID ) )
      {
         pmdEDUCB *cb = NULL ;
         UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;

         CoordDataSourcePtr dsPtr ;
         BOOLEAN ignore = FALSE ;
         UINT32 oprMask = 0 ;
         INT32 errMask = DS_ERR_FILTER_NONE ;

         if ( !_isErrFilterable( err ) )
         {
            goto done ;
         }
         else if ( coordIsLobMsg( pSub->getReqMsg()->opCode ) )
         {
            goto done ;
         }

         cb = pSub->parent()->getEDUCB() ;
         dsID = SDB_GROUPID_2_DSID( pSub->getNodeID().columns.groupID ) ;

         if ( SDB_OK != _pDSMsgConvertor->filter( pSub, cb, ignore, oprMask ) )
         {
            goto done ;
         }
         else if ( ignore )
         {
            canFilterOut = TRUE ;
            goto done ;
         }

         if ( SDB_OK != getOrUpdateDataSource( dsID, dsPtr, cb ) )
         {
            goto done ;
         }

         errMask = dsPtr->getErrFilterMask() ;

         if ( !( errMask & DS_ERR_FILTER_READ ) && SDB_IS_READ( oprMask ) )
         {
            goto done ;
         }
         else if ( !( errMask & DS_ERR_FILTER_WRITE ) &&
                   SDB_IS_WRITE( oprMask ) )
         {
            goto done ;
         }
         else if ( DS_ERR_FILTER_NONE == errMask )
         {
            goto done ;
         }

         canFilterOut = TRUE ;
      }

   done:
      return canFilterOut ;
   }

   void _coordDataSourceMgr::removeDataSource( const CHAR *name )
   {
      if ( name )
      {
         CoordDataSourcePtr ptr ;

         ossScopedRWLock lock( &_metaLatch, EXCLUSIVE ) ;
         DATASOURCE_MAP_CITR itr = _dataSources.find( name ) ;
         if ( itr != _dataSources.end() )
         {
            ptr = itr->second ;
            _dataSourceIDMap.erase( ptr->getID() ) ;
            _dataSources.erase( name ) ;
            PD_LOG( PDDEBUG, "Remove cache of data source[%s]", name ) ;
         }
      }
   }

   void _coordDataSourceMgr::removeDataSource( UTIL_DS_UID id )
   {
      CoordDataSourcePtr ptr ;

      ossScopedRWLock lock( &_metaLatch, EXCLUSIVE ) ;
      DATASOURCE_ID_MAP_CITR itr = _dataSourceIDMap.find( id ) ;
      if ( itr != _dataSourceIDMap.end() )
      {
         ptr = itr->second ;

         PD_LOG( PDDEBUG, "Remove cache of data source[%s]", ptr->getName() ) ;

         _dataSources.erase( ptr->getName() ) ;
         _dataSourceIDMap.erase( id ) ;
      }
   }

   BOOLEAN _coordDataSourceMgr::hasDataSource( UTIL_DS_UID dataSourceID )
   {
      ossScopedRWLock lock( &_metaLatch, SHARED ) ;
      return _dataSourceIDMap.find( dataSourceID ) != _dataSourceIDMap.end() ;
   }

   INT32 _coordDataSourceMgr::getDataSource( const CHAR *name,
                                             CoordDataSourcePtr &dsPtr )
   {
      INT32 rc = SDB_CAT_DATASOURCE_EXIST ;
      if ( name )
      {
         ossScopedRWLock lock( &_metaLatch, SHARED ) ;
         DATASOURCE_MAP_CITR itr = _dataSources.find( name ) ;
         if ( itr != _dataSources.end() )
         {
            dsPtr = itr->second ;
            rc = SDB_OK ;
         }
      }

      return rc ;
   }

   INT32 _coordDataSourceMgr::getDataSource( UTIL_DS_UID dataSourceID,
                                             CoordDataSourcePtr &dsPtr )
   {
      INT32 rc = SDB_CAT_DATASOURCE_EXIST ;

      ossScopedRWLock lock( &_metaLatch, SHARED ) ;
      DATASOURCE_ID_MAP_CITR itr = _dataSourceIDMap.find( dataSourceID ) ;
      if ( _dataSourceIDMap.end() != itr )
      {
         dsPtr = itr->second ;
         rc = SDB_OK ;
      }

      return rc ;
   }

   INT32 _coordDataSourceMgr::updateDataSource( UTIL_DS_UID id,
                                                CoordDataSourcePtr &dsPtr,
                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj condition = BSON( FIELD_NAME_ID << id ) ;
         rc = _updateDataSourceInfo( condition, cb, dsPtr ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update data source[%u] failed, rc: %d",
                    id, rc ) ;
            if ( SDB_CAT_DATASOURCE_NOTEXIST == rc )
            {
               removeDataSource( id ) ;
            }
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordDataSourceMgr::updateDataSource( const CHAR *name,
                                                CoordDataSourcePtr &dsPtr,
                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj condition = BSON( FIELD_NAME_NAME << name ) ;
         rc = _updateDataSourceInfo( condition, cb, dsPtr ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update data source[%s] failed, rc: %d",
                    name, rc ) ;
            if ( SDB_CAT_DATASOURCE_NOTEXIST == rc )
            {
               removeDataSource( name ) ;
            }
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordDataSourceMgr::getOrUpdateDataSource( UTIL_DS_UID id,
                                                     CoordDataSourcePtr &dsPtr,
                                                     _pmdEDUCB *cb )
   {
      INT32 rc = getDataSource( id, dsPtr ) ;
      if ( rc )
      {
         rc = updateDataSource( id, dsPtr, cb ) ;
      }
      return rc ;
   }

   INT32 _coordDataSourceMgr::getOrUpdateDataSource( const CHAR *name,
                                                     CoordDataSourcePtr &dsPtr,
                                                     _pmdEDUCB *cb )
   {
      INT32 rc = getDataSource( name, dsPtr ) ;
      if ( rc )
      {
         rc = updateDataSource( name, dsPtr, cb ) ;
      }
      return rc ;
   }

   void _coordDataSourceMgr::clear()
   {
      ossScopedRWLock lock( &_metaLatch, EXCLUSIVE ) ;

      _dataSources.clear() ;
      _dataSourceIDMap.clear() ;
   }

   void _coordDataSourceMgr::invalidate()
   {
      clear() ;
   }

   INT32 _coordDataSourceMgr::_updateRouteInfo( const CoordGroupInfoPtr &groupPtr )
   {
      INT32 rc = SDB_OK ;

      string host ;
      string service ;
      MsgRouteID routeID ;
      routeID.value = MSG_INVALID_ROUTEID ;

      UINT32 index = 0 ;
      clsGroupItem *groupItem = groupPtr.get() ;
      while ( SDB_OK == groupItem->getNodeInfo( index++, routeID, host,
                                                service,
                                                COORD_DS_ROUTE_SERVCIE ) )
      {
         rc = _pDSAgent->updateRoute( routeID, host.c_str(),
                                      service.c_str() ) ;
         if ( rc != SDB_OK )
         {
            if ( SDB_NET_UPDATE_EXISTING_NODE == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG ( PDERROR, "Update route[%s] failed, rc: %d",
                        routeID2String( routeID ).c_str(), rc ) ;
               break ;
            }
         }
      }

      return rc ;
   }


   INT32 _coordDataSourceMgr::_createDataSource( const CHAR *type,
                                                 IDataSource **dataSource )
   {
      INT32 rc = SDB_OK ;
      IDataSource* newDataSource = NULL ;

      if ( 0 == ossStrcasecmp( SDB_DATASOURCE_TYPE_NAME, type ) )
      {
         newDataSource = SDB_OSS_NEW coordSdbDataSource() ;
         if ( !newDataSource )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Allocate Sequoiadb DataSource failed" ) ;
            goto error ;
         }
      }

      *dataSource = newDataSource ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordDataSourceMgr::_updateDataSource( const BSONObj &object,
                                                 CoordDataSourcePtr &dsPtr )
   {
      INT32 rc = SDB_OK ;
      IDataSource *dataSource = NULL ;
      const CHAR *type = NULL ;

      try
      {
         BSONElement e = object.getField( FIELD_NAME_TYPE ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "Data source field[%s] is not string",
                    FIELD_NAME_TYPE ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         type = e.valuestr() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = _createDataSource( type, &dataSource ) ;
      if ( rc )
      {
         goto error ;
      }
      dsPtr = CoordDataSourcePtr( dataSource ) ;

      /// init
      rc = dataSource->init( object ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init datasource by object[%s] failed, rc: %d",
                 object.toString().c_str(), rc ) ;
         goto error ;
      }

      /// add to map
      try
      {
         /// update group info
         rc = _updateRouteInfo( dataSource->getGroupInfo() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update route info failed, rc: %d", rc ) ;
            goto error ;
         }

         ossScopedRWLock lock( &_metaLatch, EXCLUSIVE ) ;

         /// must remove by name, because use operator[] when conflict, the
         /// key is old, but the value is new. So the key will be free
         _dataSources.erase( dataSource->getName() ) ;
         /// the add
         _dataSources[ dataSource->getName() ] = dsPtr ;
         _dataSourceIDMap[ dataSource->getID() ] = dsPtr ;
      }
      catch( std::exception &e )
      {
         /// remove
         {
            ossScopedRWLock lock( &_metaLatch, EXCLUSIVE ) ;
            _dataSources.erase( dataSource->getName() ) ;
            _dataSourceIDMap.erase( dataSource->getID() ) ;
         }

         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;

         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _coordDataSourceMgr::_updateDataSourceInfo( const BSONObj &matcher,
                                                     pmdEDUCB *cb,
                                                     CoordDataSourcePtr &dsPtr )
   {
      INT32 rc = SDB_OK ;
      coordCommandFactory *cmdFactory = coordGetFactory() ;
      coordOperator *opr = NULL ;
      CHAR *msgBuff = NULL ;
      INT32 bufSize = 0 ;
      INT64 contextID = -1 ;
      rtnContextBuf buf ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      coordResource *pResource = sdbGetCoordCB()->getResource() ;

      rc = cmdFactory->create( CMD_NAME_LIST_DATASOURCES, opr ) ;
      PD_RC_CHECK( rc, PDERROR, "Create operator[%s] failed[%d]",
                   CMD_NAME_LIST_DATASOURCES, rc ) ;
      rc = opr->init( pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize operator[%s] failed[%d]",
                   opr->getName(), rc ) ;

      try
      {
         // Query the data source info by a 'list datasources' command.
         rc = msgBuildQueryMsg( &msgBuff, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_LIST_DATASOURCES,
                                FLG_QUERY_WITH_RETURNDATA,
                                0, 0, 1, &matcher, NULL, NULL, NULL, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build query message failed[%d]", rc ) ;

         rc = opr->execute( (MsgHeader *)msgBuff, cb, contextID, &buf ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute operator[%s] failed[%d]",
                      opr->getName(), rc ) ;

         rc = rtnGetMore( contextID, 1, buf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CAT_DATASOURCE_NOTEXIST ;
            PD_LOG( PDERROR, "Data source[%s] dose not exist[%d]",
                    matcher.toString().c_str(), rc ) ;
            contextID = -1 ;
            goto error ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get more record from context[%lld] failed[%d]",
                    contextID, rc ) ;
            contextID = -1 ;
            goto error ;
         }

         /// update datasource info
         {
            BSONObj obj( buf.data() ) ;
            rc = _updateDataSource( obj, dsPtr ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( opr )
      {
         cmdFactory->release( opr ) ;
      }
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      if ( msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordDataSourceMgr::_isErrFilterable( INT32 err )
   {
      switch ( err )
      {
         case SDB_COORD_DATASOURCE_PERM_DENIED:
         case SDB_COORD_DATASOURCE_TRANS_FORBIDDEN:
         case SDB_OPERATION_INCOMPATIBLE:
         case SDB_AUTH_AUTHORITY_FORBIDDEN:
            return FALSE ;
         default:
            return TRUE ;
      }
   }
}
