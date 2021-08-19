/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = clsShardMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsShardMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "msgAuth.hpp"
#include "msgCatalog.hpp"
#include "pmdController.hpp"
#include "../bson/bson.h"
#include "rtnQueryOptions.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsAdapterJob.hpp"
#include "rtnExtDataHandler.hpp"

using namespace bson ;

namespace engine
{

   #define CLS_CATA_RETRY_MAX_TIMES          ( 4 )

   struct _hostAndPort
   {
      std::string _host ;
      std::string _svc ;
      MsgRouteID  _nodeID ;
      INT32       _status ;
      INT32       _result ;

      _hostAndPort( std::string host, std::string svc, MsgRouteID nodeID )
      {
         _host = host ;
         _svc = svc ;
         _nodeID.value = nodeID.value ;
         _status = NET_NODE_STAT_NORMAL ;
         _result = SDB_OK ;
      }
      _hostAndPort()
      {
         _nodeID.value = MSG_INVALID_ROUTEID ;
         _status = NET_NODE_STAT_NORMAL ;
         _result = SDB_OK ;
      }
      BOOLEAN isNormal() const
      {
         return ( NET_NODE_STAT_NORMAL == _status ? TRUE : FALSE ) ;
      }
   } ;

   static INT32 _clsSelectNodes( clsGroupItem *pItem,
                                 BOOLEAN primary,
                                 MSG_ROUTE_SERVICE_TYPE type,
                                 std::vector< _hostAndPort > &hosts )
   {
      _hostAndPort hostItem ;
      INT32 tmpPrimary = pItem->getPrimaryPos() ;

      hosts.clear() ;

      // detect if we want to send to catalog primary
      if ( primary &&
           SDB_OK == pItem->getNodeInfo( tmpPrimary,
                                         hostItem._nodeID,
                                         hostItem._host,
                                         hostItem._svc,
                                         type ) &&
           SDB_OK == pItem->getNodeInfo( tmpPrimary,
                                         hostItem._status ) &&
           hostItem.isNormal() )
      {
         hosts.push_back( hostItem ) ;
      }
      // if we want to broadcast, we push everything in catalog list
      else
      {
         for ( UINT32 pos = 0 ; pos < pItem->nodeCount() ; ++pos )
         {
            if ( SDB_OK == pItem->getNodeInfo( pos, hostItem._nodeID,
                                               hostItem._host, hostItem._svc,
                                               type ) &&
                 SDB_OK == pItem->getNodeInfo( pos, hostItem._status ) &&
                 hostItem.isNormal() )
            {
               hosts.push_back( hostItem ) ;
            }
         }
      }

      return hosts.size() > 0 ? SDB_OK : SDB_CLS_NODE_NOT_EXIST ;
   }

   void _clsUpdateNodeStatus( clsGroupItem *pItem,
                              std::vector< _hostAndPort > &hosts )
   {
      for ( UINT32 i = 0 ; i < hosts.size() ; ++i )
      {
         _hostAndPort &info = hosts[ i ] ;
         if ( info._result )
         {
            pItem->updateNodeStat( info._nodeID.columns.nodeID,
                                   netResult2Status( info._result ) ) ;
         }
      }
   }

   /*
      _clsFreezingWindow implement
   */
   _clsFreezingWindow::_clsFreezingWindow()
   {
      _clCount = 0 ;
   }

   _clsFreezingWindow::~_clsFreezingWindow()
   {
   }

   INT32 _clsFreezingWindow::registerCL ( const CHAR *pName, UINT64 &opID )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      if ( !pName || !*pName )
      {
         rc = SDB_INVALIDARG ;
         SDB_ASSERT( FALSE, "Name is invalid" ) ;
      }
      else
      {
         try
         {
            ossPoolString name( pName ) ;
            rc = _registerCLInternal( name, opID ) ;
         }
         catch( std::exception &e )
         {
            /// ossPoolString maybe throw exception when out of memory
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::registerCS( const CHAR *pName,
                                         UINT64 &opID )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      if ( !pName || !*pName || ossStrchr( pName, '.' ) )
      {
         rc = SDB_INVALIDARG ;
         SDB_ASSERT( FALSE, "Name is invalid" ) ;
      }
      else
      {
         try
         {
            ossPoolString name( pName ) ;
            rc = _registerCSInternal( name, opID ) ;
         }
         catch( std::exception &e )
         {
            /// ossPoolString maybe throw exception when out of memory
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::registerWhole( UINT64 &opID  )
   {
      INT32 rc = SDB_OK ;
      UINT64 oldOpID = opID ;

      ossScopedLock lock( &_latch ) ;

      /// first increase _clCount. Because waitForOpr will use _clCount without
      /// latch. If don't increase first, the case will occur:
      /// 1. split thread has run to opID = pmdAcquireGlobalID() ;
      /// 2. write thread run to pmdAcquireGlobalID(), then run to waitForOpr::
      ///    if ( isWrite && _clCount > 0 )
      /// 3. the split thread will not wait write thread, so some data will lost
      ++_clCount ;

      // operator ID is not given, acquire one for it
      // Must be acquired inside latch, which could avoid other operators to
      // acquire operator ID and pass the checking between acquiring and
      // registering
      if ( 0 == opID )
      {
         opID = pmdAcquireGlobalID() ;
      }

      rc = _regWholeInternal( opID ) ;

      /// last decrease _clCount
      --_clCount ;

      if ( rc && opID != oldOpID )
      {
         /// restore opID when failed
         opID = oldOpID ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::_regWholeInternal( UINT64 opID )
   {
      INT32 rc = SDB_OK ;

      if ( _setWholeID.empty() )
      {
         ++_clCount ;
      }

      try
      {
         _setWholeID.insert( opID ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         --_clCount ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::_registerCLInternal ( const ossPoolString &name,
                                                   UINT64 opID )
   {
      INT32 rc = SDB_OK ;
      MAP_WINDOW::iterator it = _mapWindow.find( name ) ;

      if ( _mapWindow.end() == it )
      {
         ++_clCount ;

         try
         {
            OP_SET newOpSet ;
            newOpSet.insert( opID ) ;

            _mapWindow[ name ] = newOpSet ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            --_clCount ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }
      else
      {
         try
         {
            it->second.insert( opID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return rc ;
   }

   INT32 _clsFreezingWindow::_registerCSInternal( const ossPoolString &name,
                                                  UINT64 opID )
   {
      INT32 rc = SDB_OK ;
      MAP_CS_WINDOW::iterator it = _mapCSWindow.find( name ) ;

      if ( _mapCSWindow.end() == it )
      {
         ++_clCount ;

         try
         {
            OP_SET newOpSet ;
            newOpSet.insert( opID ) ;

            _mapCSWindow[ name ] = newOpSet ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            --_clCount ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }
      else
      {
         try
         {
            it->second.insert( opID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      return rc ;
   }

   void _clsFreezingWindow::unregisterCL ( const CHAR *pName, UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      if ( pName && *pName )
      {
         try
         {
            ossPoolString name( pName ) ;
            _unregisterCLInternal( name, opID ) ;
         }
         catch( ... )
         {
            _unregisterCLByIter( pName, opID ) ;
         }
      }

      _event.signalAll() ;
   }

   void _clsFreezingWindow::unregisterCS( const CHAR *pName, UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      if ( pName && *pName )
      {
         try
         {
            ossPoolString name( pName ) ;
            _unregisterCSInternal( name, opID ) ;
         }
         catch( ... )
         {
            _unregisterCSByIter( pName, opID ) ;
         }
      }

      _event.signalAll() ;
   }

   void _clsFreezingWindow::unregisterWhole( UINT64 opID )
   {
      ossScopedLock lock( &_latch ) ;

      _unregWholeInternal( opID ) ;

      _event.signalAll() ;
   }

   void _clsFreezingWindow::_unregisterCLByIter( const CHAR *pName,
                                                 UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.begin() ;

      while ( it != _mapWindow.end() )
      {
         if ( 0 == ossStrcmp( it->first.c_str(), pName ) )
         {
            it->second.erase( opID ) ;

            if ( it->second.empty() )
            {
               _mapWindow.erase( it ) ;
               --_clCount ;
            }
            break ;
         }
         ++it ;
      }
   }

   void _clsFreezingWindow::_unregisterCSByIter( const CHAR *pName,
                                                 UINT64 opID )
   {
      MAP_CS_WINDOW::iterator it = _mapCSWindow.begin() ;

      while ( it != _mapCSWindow.end() )
      {
         if ( 0 == ossStrcmp( it->first._name.c_str(), pName ) )
         {
            it->second.erase( opID ) ;

            if ( it->second.empty() )
            {
               _mapCSWindow.erase( it ) ;
               --_clCount ;
            }
            break ;
         }
         ++it ;
      }
   }

   void _clsFreezingWindow::unregisterAll()
   {
      ossScopedLock lock( &_latch ) ;

      _setWholeID.clear() ;
      _mapWindow.clear() ;
      _mapCSWindow.clear() ;
      _clCount = 0 ;
   }

   void _clsFreezingWindow::_unregWholeInternal( UINT64 opID )
   {
      _setWholeID.erase( opID ) ;
      if ( _setWholeID.empty() )
      {
         --_clCount ;
      }
   }

   void _clsFreezingWindow::_unregisterCLInternal ( const ossPoolString &name,
                                                    UINT64 opID )
   {
      MAP_WINDOW::iterator it = _mapWindow.find( name ) ;

      if ( _mapWindow.end() != it )
      {
         it->second.erase( opID ) ;

         if ( it->second.empty() )
         {
            _mapWindow.erase( it ) ;
            --_clCount ;
         }
      }
   }

   void _clsFreezingWindow::_unregisterCSInternal( const ossPoolString &name,
                                                   UINT64 opID )
   {
      MAP_CS_WINDOW::iterator it = _mapCSWindow.find( name ) ;

      if ( _mapCSWindow.end() != it )
      {
         it->second.erase( opID ) ;

         if ( it->second.empty() )
         {
            _mapCSWindow.erase( it ) ;
            --_clCount ;
         }
      }
   }

   void _clsFreezingWindow::_blockCheck( const CHAR *pName,
                                         const _clsFreezingWindow::OP_SET &setID,
                                         UINT64 testOPID,
                                         UINT64 testTransOPID,
                                         _pmdEDUCB *cb,
                                         BOOLEAN &result,
                                         BOOLEAN &forceEnd )
   {
      OP_SET::const_iterator cit = setID.begin() ;

      while ( cit != setID.end () )
      {
         if ( *cit == testOPID )
         {
            // Self
            result = FALSE ;
            forceEnd = TRUE ;
            break ;
         }
         else if ( *cit < testOPID  )
         {
            // Should not break, we need to test if testOpID matches
            // the remaining blocking op IDs which may be the blocking op
            // itself
            if ( 0 == testTransOPID || *cit < testTransOPID )
            {
               result = TRUE ;
            }
            else if ( 0 != *pName ) /// Not whole
            {
               if ( NULL == ossStrchr( pName, '.' ) ) /// CS
               {
                  SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
                  dmsStorageUnitID suID = DMS_INVALID_CS ;
                  _dmsStorageUnit *su = NULL ;

                  if ( SDB_OK == dmsCB->nameToSUAndLock( pName, suID, &su ) )
                  {
                     dpsTransLockId lockID( su->LogicalCSID(),
                                            DMS_INVALID_MBID,
                                            NULL ) ;
                     if ( cb->getTransExecutor()->countLock( lockID ) <= 0 )
                     {
                        result = TRUE ;
                     }

                     dmsCB->suUnlock( suID ) ;
                  }
                  else
                  {
                     result = TRUE ;
                  }
               }
               else  /// CL
               {
                  SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
                  _dmsStorageUnit *su = NULL ;
                  const CHAR *pShortName = NULL ;
                  dmsStorageUnitID suID = DMS_INVALID_CS ;
                  UINT16 collectionID = DMS_INVALID_MBID ;

                  if ( SDB_OK == rtnResolveCollectionNameAndLock( pName, dmsCB,
                                                                  &su,
                                                                  &pShortName,
                                                                  suID ) )
                  {
                     if ( SDB_OK == su->data()->findCollection( pShortName,
                                                                collectionID ) )
                     {
                        dpsTransLockId lockID( su->LogicalCSID(),
                                               collectionID,
                                               NULL ) ;
                        if ( cb->getTransExecutor()->countLock( lockID ) <= 0 )
                        {
                           result = TRUE ;
                        }
                     }
                     else
                     {
                        result = TRUE ;
                     }

                     dmsCB->suUnlock( suID ) ;
                  }
                  else
                  {
                     result = TRUE ;
                  }
               }
            }
         }
         ++cit ;
      }
   }

   BOOLEAN _clsFreezingWindow::needBlockOpr( const ossPoolString &name,
                                             UINT64 testOpID,
                                             UINT64 testTransOpID,
                                             _pmdEDUCB *cb )
   {
      MAP_WINDOW::iterator it ;
      MAP_CS_WINDOW::iterator itCS ;
      BOOLEAN needBlock = FALSE ;
      BOOLEAN forceEnd = FALSE ;

      ossScopedLock lock( &_latch ) ;

      /// whole block check
      if ( !_setWholeID.empty() )
      {
         _blockCheck( "", _setWholeID, testOpID,
                      testTransOpID, cb, needBlock, forceEnd ) ;
         if ( forceEnd )
         {
            goto done ;
         }
      }

      /// cs block check
      if ( !_mapCSWindow.empty() &&
           _mapCSWindow.end() != ( itCS = _mapCSWindow.find( name ) ) )
      {
         _blockCheck( itCS->first._name.c_str(), itCS->second, testOpID,
                      testTransOpID, cb, needBlock, forceEnd ) ;
         if ( forceEnd )
         {
            goto done ;
         }
      }

      /// cl block check
      if ( !_mapWindow.empty() &&
           _mapWindow.end() != ( it = _mapWindow.find( name ) ) )
      {
         _blockCheck( it->first.c_str(), it->second, testOpID,
                      testTransOpID, cb, needBlock, forceEnd ) ;
      }

   done:
      return needBlock ;
   }

   INT32 _clsFreezingWindow::waitForOpr( const CHAR *pName,
                                         _pmdEDUCB *cb,
                                         BOOLEAN isWrite )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasBlock = FALSE ;

      if ( isWrite && _clCount > 0 )
      {
         try
         {
            ossPoolString clName( pName ) ;
            BOOLEAN needBlock = TRUE ;
            MAP_WINDOW::iterator it ;
            UINT64 opID = cb->getWritingID() ;
            UINT64 transOpID = cb->getTransWritingID() ;

            while( needBlock )
            {
               if ( cb->isInterrupted() )
               {
                  rc = SDB_APP_INTERRUPT ;
                  break ;
               }

               needBlock = needBlockOpr( clName, opID, transOpID, cb ) ;
               if ( needBlock )
               {
                  if ( !hasBlock )
                  {
                     cb->setBlock( EDU_BLOCK_FREEZING_WND, "" ) ;
                     cb->printInfo( EDU_INFO_DOING,
                                    "Waiting for freezing window(Name:%s)",
                                    pName ) ;
                     hasBlock = TRUE ;
                  }

                  _event.reset() ;
                  _event.wait( OSS_ONE_SEC ) ;
               }
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         }
      }

      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      return rc ;
   }

   BEGIN_OBJ_MSG_MAP(_clsShardMgr, _pmdObjBase)
      ON_MSG ( MSG_CAT_CATGRP_RES, _onCatCatGroupRes )
      ON_MSG ( MSG_CAT_NODEGRP_RES, _onCatGroupRes )
      ON_MSG ( MSG_CAT_QUERY_CATALOG_RSP, _onCatalogReqMsg )
      ON_MSG ( MSG_CAT_QUERY_SPACEINFO_RSP, _onQueryCSInfoRsp )
      ON_MSG ( MSG_COM_REMOTE_DISC, _onHandleClose )
      ON_MSG ( MSG_AUTH_VERIFY_REQ, _onAuthReqMsg )
      ON_MSG ( MSG_SEADPT_UPDATE_IDXINFO_REQ, _onTextIdxInfoReqMsg )
      ON_MSG ( MSG_CLS_TRANS_CHECK_REQ, _onTransCheckReqMsg )
   END_OBJ_MSG_MAP()

   _clsShardMgr::_clsShardMgr ( _netRouteAgent *rtAgent )
   :_cataGrpItem( CATALOG_GROUPID )
   {
      _pNetRtAgent = rtAgent ;
      _requestID = 0 ;
      _pCatAgent = NULL ;
      _pNodeMgrAgent = NULL ;
      _pFreezingWindow = NULL ;
      _pDCMgr = NULL ;
      _pGTSAgent = NULL ;

      _catVerion = 0 ;
      _nodeID.value = 0 ;
      _upCatHandle = NET_INVALID_HANDLE ;
      _remoteEndpointHandle = NET_INVALID_HANDLE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_DECONSTRUCTOR, "_clsShardMgr::~_clsShardMgr" )
   _clsShardMgr::~_clsShardMgr()
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_DECONSTRUCTOR );
      _pNetRtAgent = NULL ;

      SAFE_DELETE ( _pCatAgent ) ;
      SAFE_DELETE ( _pNodeMgrAgent ) ;
      SAFE_DELETE ( _pFreezingWindow ) ;
      SAFE_DELETE ( _pDCMgr ) ;
      SAFE_DELETE ( _pGTSAgent ) ;

      //release event
      MAP_CAT_EVENT_IT it = _mapSyncCatEvent.begin () ;
      while ( it != _mapSyncCatEvent.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _mapSyncCatEvent.clear () ;

      MAP_NM_EVENT_IT itNM = _mapSyncNMEvent.begin () ;
      while ( itNM != _mapSyncNMEvent.end() )
      {
         SDB_OSS_DEL itNM->second ;
         ++itNM ;
      }
      _mapSyncNMEvent.clear () ;
      PD_TRACE_EXIT ( SDB__CLSSHDMGR_DECONSTRUCTOR );
   }

   void _clsShardMgr::setNodeID ( const MsgRouteID & nodeID )
   {
      _nodeID = nodeID ;
      _nodeID.columns.serviceID = 0 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_INIT, "_clsShardMgr::initialize" )
   INT32 _clsShardMgr::initialize()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_INIT );
      UINT32 catGID = CATALOG_GROUPID ;
      // Make sure the node IDs are invalid
      UINT16 catNID = DATA_NODE_ID_END + 1 ;
      MsgRouteID id ;
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      // catAddrs is pointing to option control block
      vector< _pmdAddrPair > catAddrs = optCB->catAddrs() ;
      _netFrame *pNetFrame = NULL ;

      if ( !_pNetRtAgent )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "network runtime agent can't be NULL, rc = %d",
                  rc ) ;
         goto error ;
      }
      pNetFrame = _pNetRtAgent->getFrame() ;
      /// register shard net agent to net monitor for catalog connections
      pNetFrame->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;
      pNetFrame->setMaxSockPerNode( pmdGetOptionCB()->maxSockPerNode() ) ;
      pNetFrame->setMaxSockPerThread( pmdGetOptionCB()->maxSockPerThread() ) ;
      pNetFrame->setMaxThreadNum( pmdGetOptionCB()->maxSockThread() ) ;

      sdbGetPMDController()->registerNet( pNetFrame,
                                          MSG_ROUTE_CAT_SERVICE ) ;

      // init param, get configured catalog address
      for ( UINT32 i = 0 ; i < catAddrs.size() ; ++i )
      {
         if ( 0 == catAddrs[i]._host[ 0 ] )
         {
            break ;
         }
         id.columns.groupID = catGID ;
         id.columns.nodeID = catNID++ ;
         id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         setCatlogInfo( id, catAddrs[i]._host, catAddrs[i]._service ) ;
         _pNetRtAgent->updateRoute( id, catAddrs[i]._host,
                                    catAddrs[i]._service ) ;
      }

      if ( _mapNodes.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Catalog information was not properly configured, "
                  "rc = %d", rc ) ;
         goto error ;
      }

      rc = _cataGrpItem.updateNodes( _mapNodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Update catalog group info failed, rc: %d",
                   rc ) ;

      SAFE_NEW_GOTO_ERROR  ( _pCatAgent, _clsCatalogAgent ) ;
      SAFE_NEW_GOTO_ERROR  ( _pNodeMgrAgent, _clsNodeMgrAgent ) ;
      SAFE_NEW_GOTO_ERROR  ( _pFreezingWindow, _clsFreezingWindow ) ;
      SAFE_NEW_GOTO_ERROR  ( _pDCMgr, _clsDCMgr ) ;
      SAFE_NEW_GOTO_ERROR1 ( _pGTSAgent, _clsGTSAgent, this ) ;

      rc = _pDCMgr->initialize() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init datacenter manager failed, rc: %d", rc ) ;
         goto error ;
      }

      if ( pmdGetKRCB()->getTransCB() &&
           SDB_ROLE_DATA == pmdGetKRCB()->getDBRole() )
      {
         pmdGetKRCB()->getTransCB()->setEventHandler( _pGTSAgent ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::active ()
   {
      return SDB_OK ;
   }

   INT32 _clsShardMgr::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _clsShardMgr::final ()
   {
      if ( _pNetRtAgent )
      {
         sdbGetPMDController()->unregNet( _pNetRtAgent->getFrame() ) ;
      }

      if ( pmdGetKRCB()->getTransCB() &&
           SDB_ROLE_DATA == pmdGetKRCB()->getDBRole() )
      {
         pmdGetKRCB()->getTransCB()->setEventHandler( NULL ) ;
      }

      return SDB_OK ;
   }

   void _clsShardMgr::onConfigChange ()
   {
      if ( _pNetRtAgent )
      {
         _netFrame *pNetFrame = _pNetRtAgent->getFrame() ;
         pNetFrame->setBeatInfo( pmdGetOptionCB()->getOprTimeout() ) ;
         pNetFrame->setMaxSockPerNode( pmdGetOptionCB()->maxSockPerNode() ) ;
         pNetFrame->setMaxSockPerThread( pmdGetOptionCB()->maxSockPerThread() ) ;
         pNetFrame->setMaxThreadNum( pmdGetOptionCB()->maxSockThread() ) ;
      }
   }

   void _clsShardMgr::ntyPrimaryChange( BOOLEAN primary,
                                        SDB_EVENT_OCCUR_TYPE type )
   {
      if ( primary && SDB_EVT_OCCUR_BEFORE == type )
      {
         // clear catalog info
         _pCatAgent->lock_w() ;
         _pCatAgent->clearAll() ;
         _pCatAgent->release_w() ;

         // Clear statistics
         pmdGetKRCB()->getDMSCB()->clearSUCaches( DMS_EVENT_MASK_ALL ) ;
      }
      else if ( primary && SDB_EVT_OCCUR_AFTER == type )
      {
         /// TODO: For multi datacenter, when node change primary can't
         /// write dps log.
         /// So this will cause stat cache is not the same between new
         /// primary node and other secondary nodes
         // sdbGetClsCB()->invalidateStatistics() ;
      }
   }

   void _clsShardMgr::attachCB( _pmdEDUCB * cb )
   {
      sdbGetClsCB()->attachCB( cb ) ;
   }

   void _clsShardMgr::detachCB( _pmdEDUCB * cb )
   {
      sdbGetClsCB()->detachCB( cb ) ;
   }

   void _clsShardMgr::onTimer ( UINT32 timerID, UINT32 interval )
   {
   }

   NodeID _clsShardMgr::nodeID () const
   {
      return _nodeID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SETCATINFO, "_clsShardMgr::setCatlogInfo" )
   void _clsShardMgr::setCatlogInfo ( const NodeID & id,
                                      const std::string& host,
                                      const std::string& service )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SETCATINFO );
      _netRouteNode info ;
      info._id.value = id.value ;
      info._id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
      ossStrncpy( info._host, host.c_str(), OSS_MAX_HOSTNAME ) ;
      info._service[ MSG_ROUTE_CAT_SERVICE ] = service ;
      _mapNodes[ info._id.value ] = info ;

      PD_TRACE_EXIT ( SDB__CLSSHDMGR_SETCATINFO );
   }

   catAgent* _clsShardMgr::getCataAgent ()
   {
      return _pCatAgent ;
   }

   nodeMgrAgent* _clsShardMgr::getNodeMgrAgent ()
   {
      return _pNodeMgrAgent ;
   }

   clsFreezingWindow* _clsShardMgr::getFreezingWindow()
   {
      return _pFreezingWindow ;
   }

   clsDCMgr* _clsShardMgr::getDCMgr()
   {
      return _pDCMgr ;
   }

   clsGTSAgent* _clsShardMgr::getGTSAgent()
   {
      return _pGTSAgent ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCSND, "_clsShardMgr::syncSend" )
   INT32 _clsShardMgr::syncSend( MsgHeader * msg, UINT32 groupID,
                                 BOOLEAN primary, MsgHeader **ppRecvMsg,
                                 INT64 millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCSND ) ;
      INT32 rc = SDB_OK ;
      std::vector< _hostAndPort > hosts ;
      BOOLEAN hasUpdateGroup = FALSE ;

   retry:
      // if we are sending to catalog group
      if ( CATALOG_GROUPID == groupID )
      {
         ossScopedLock lock ( &_shardLatch, SHARED ) ;
         rc = _clsSelectNodes( &_cataGrpItem, primary,
                               MSG_ROUTE_CAT_SERVICE,
                               hosts ) ;
      }
      // if we are sending to user group
      else
      {
         clsGroupItem *item = NULL ;
         rc = getAndLockGroupItem( groupID, &item, TRUE, CLS_SHARD_TIMEOUT,
                                   &hasUpdateGroup ) ;
         if ( SDB_OK == rc )
         {
            rc = _clsSelectNodes( item, primary, MSG_ROUTE_SHARD_SERVCIE,
                                  hosts ) ;
            unlockGroupItem( item ) ;
         }
      }

      /// if we didn't find anything and we have not tried to refresh catalog
      if ( SDB_CLS_NODE_NOT_EXIST == rc && !hasUpdateGroup )
      {
         goto update_group ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to find nodes for sync send, "
                   "group id = %d, rc = %d", groupID, rc ) ;

      // now let's send the information
      {
         UINT32 msgLength = 0 ;
         INT32 receivedLen = 0 ;
         INT32 sentLen = 0 ;
         CHAR* buff = NULL ;
         UINT16 port = 0 ;
         // randomly pickup a starting position
         UINT32 pos = ossRand() % hosts.size() ;

         for ( UINT32 i = 0 ; i < hosts.size() ; ++i )
         {
            // convert from service port to real port
            _hostAndPort &tmpInfo = hosts[pos] ;
            pos = ( pos + 1 ) % hosts.size() ;
            rc = ossGetPort( tmpInfo._svc.c_str(), port ) ;
            PD_RC_CHECK( rc, PDERROR, "Invalid svcname: %s",
                         tmpInfo._svc.c_str() ) ;

            // use millisecond
            // establish a socket connection, will be closed by end of
            // the scope
            ossSocket tmpSocket ( tmpInfo._host.c_str(), port, millisec ) ;
            rc = tmpSocket.initSocket() ;
            PD_RC_CHECK( rc, PDERROR, "Init socket %s:%d failed, rc:%d",
                         tmpInfo._host.c_str(), port, rc ) ;

            rc = tmpSocket.connect() ;
            // if we are not able to connect to a node, let's skip and retry
            // next one
            if ( rc )
            {
               PD_LOG( PDWARNING, "Connect to %s:%d failed, rc:%d",
                       tmpInfo._host.c_str(), port, rc ) ;
               tmpInfo._result = rc ;
               continue ;
            }

            // send msg, if we can connect to the node but failed to send
            // let's skip and retry
            rc = tmpSocket.send( (const CHAR *)msg, msg->messageLength,
                                 sentLen, millisec ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Send messge to %s:%d failed, rc:%d",
                       tmpInfo._host.c_str(), port, rc ) ;
               tmpInfo._result = rc ;
               continue ;
            }

            // recieve msg, do not loop and retry
            rc = tmpSocket.recv( (CHAR*)&msgLength, sizeof(INT32), receivedLen,
                                 millisec ) ;
            PD_RC_CHECK( rc, PDERROR, "Recieve msg length failed, rc: %d",
                         rc ) ;

            if ( msgLength < sizeof(INT32) || msgLength > SDB_MAX_MSG_LENGTH )
            {
               PD_LOG ( PDERROR, "Recieve msg length[%d] error", msgLength ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // buff is freed outside the function
            buff = (CHAR*)SDB_OSS_MALLOC( msgLength + 1 ) ;
            if ( !buff )
            {
               rc = SDB_OOM ;
               PD_LOG ( PDERROR, "Failed to allocate memory for %d bytes",
                        msgLength + 1 ) ;
               goto error ;
            }
            *(INT32*)buff = msgLength ;
            // do not loop and retry, simply return error message when we failed to
            // recv, including timeout, because this is internal communication and
            // we should control the timeout value
            rc = tmpSocket.recv( &buff[sizeof(INT32)], msgLength-sizeof(INT32),
                                 receivedLen,
                                 millisec ) ;
            if ( rc )
            {
               SDB_OSS_FREE( buff ) ;
               PD_LOG ( PDERROR, "Recieve response message failed, rc: %d", rc ) ;
               goto error ;
            }
            // Once we received something, we just break out the loop
            // so no memory leak here
            *ppRecvMsg = (MsgHeader*)buff ;
            break ;
         }
      }

      /// send all node failed
      if ( rc && !hasUpdateGroup )
      {
         goto update_group ;
      }

   done:
      /// update node status
      if ( CATALOG_GROUPID == groupID )
      {
         ossScopedLock lock ( &_shardLatch, SHARED ) ;
         _clsUpdateNodeStatus( &_cataGrpItem, hosts ) ;
      }
      // if we are sending to user group
      else
      {
         clsGroupItem *item = NULL ;
         INT32 rcTmp = getAndLockGroupItem( groupID, &item, FALSE,
                                            CLS_SHARD_TIMEOUT, NULL ) ;
         if ( SDB_OK == rcTmp )
         {
            _clsUpdateNodeStatus( item, hosts ) ;
            unlockGroupItem( item ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCSND, rc );
      return rc ;
   error:
      goto done ;
   update_group:
      if ( !hasUpdateGroup )
      {
         hasUpdateGroup = TRUE ;
         // need to update
         if ( CATALOG_GROUPID == groupID )
         {
            rc = updateCatGroup( millisec ) ;
         }
         else
         {
            rc = syncUpdateGroupInfo( groupID, millisec ) ;
         }

         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         PD_LOG( PDERROR, "Update group[%d] info failed, rc: %d",
                 groupID, rc ) ;
      }
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SND2CAT, "_clsShardMgr::sendToCatlog" )
   INT32 _clsShardMgr::sendToCatlog ( MsgHeader *msg,
                                      NET_HANDLE *pHandle,
                                      INT64 upCataMillsec,
                                      BOOLEAN canUpCataGrp )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SND2CAT ) ;

      MsgRouteID nodeID ;
      INT32 status = 0 ;
      UINT32 tmpPos = CLS_RG_NODE_POS_INVALID ;
      BOOLEAN hasLock = FALSE ;
      BOOLEAN hasUpdateGrp = FALSE ;

   retry:
      _shardLatch.get_shared() ;
      hasLock = TRUE ;

      // sanity check
      if ( !_pNetRtAgent || 0 == _cataGrpItem.nodeCount() )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Either network runtime agent does not exist, "
                  "or catalog list is empty, rc = %d", rc ) ;
         goto error ;
      }

      // if we know the catalog primary node, let's try to send
      tmpPos = _cataGrpItem.getPrimaryPos() ;
      if ( SDB_OK == _cataGrpItem.getNodeID( tmpPos, nodeID,
                                             MSG_ROUTE_CAT_SERVICE ) &&
           SDB_OK == _cataGrpItem.getNodeInfo( tmpPos, status ) &&
           NET_NODE_STAT_NORMAL == status )
      {
         rc = _pNetRtAgent->syncSend ( nodeID, (void*)msg, pHandle ) ;
         if ( rc != SDB_OK )
         {
            string hostName ;
            string svcName ;
            _cataGrpItem.getNodeInfo( nodeID, hostName, svcName ) ;
            PD_LOG ( PDWARNING, "Send message to primary catalog[%s:%s, "
                     "NodeID:%u] failed[rc:%d]", hostName.c_str(),
                     svcName.c_str(), nodeID.columns.nodeID, rc ) ;
            /// update node status
            _cataGrpItem.updateNodeStat( nodeID.columns.nodeID,
                                         netResult2Status( rc ) ) ;
         }
         else
         {
            goto done ;
         }
      }

      // we want send to any one normal node
      {
         tmpPos = ossRand() % _cataGrpItem.nodeCount() ;
         rc = SDB_CLS_NODE_NOT_EXIST ;

         for ( UINT32 times = 0 ; times < _cataGrpItem.nodeCount() ; ++times )
         {
            if ( SDB_OK == _cataGrpItem.getNodeID( tmpPos, nodeID,
                                                   MSG_ROUTE_CAT_SERVICE ) &&
                 SDB_OK == _cataGrpItem.getNodeInfo( tmpPos, status ) &&
                 NET_NODE_STAT_NORMAL == status )
            {
               rc = _pNetRtAgent->syncSend ( nodeID, (void*)msg, pHandle ) ;
               if ( SDB_OK == rc )
               {
                  goto done ;
               }
               else
               {
                  string hostName ;
                  string svcName ;
                  _cataGrpItem.getNodeInfo( nodeID, hostName, svcName ) ;
                  PD_LOG ( PDWARNING, "Send message to catlog[%s:%s, "
                           "NodeID: %u] failed[rc:%d]", hostName.c_str(),
                           svcName.c_str(), nodeID.columns.nodeID, rc ) ;
                  /// updata node status
                  _cataGrpItem.updateNodeStat( nodeID.columns.nodeID,
                                               netResult2Status( rc ) ) ;
               }
            }
            tmpPos = ( tmpPos + 1 ) % _cataGrpItem.nodeCount() ;
         } /// end for
      }

      _shardLatch.release_shared() ;
      hasLock = FALSE ;
      /// send to all failed, update catalog and retry
      if ( canUpCataGrp && !hasUpdateGrp )
      {
         hasUpdateGrp = TRUE ;
         rc = updateCatGroup( upCataMillsec ) ;
         if ( SDB_OK == rc )
         {
            goto retry ;
         }
         PD_LOG( PDERROR, "Update catalog group info failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( hasLock )
      {
         _shardLatch.release_shared() ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SND2CAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // update cached catalog group information
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPDCATGRP, "_clsShardMgr::updateCatGroup" )
   INT32 _clsShardMgr::updateCatGroup ( INT64 millsec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPDCATGRP ) ;
      INT32 rc = SDB_OK ;
      UINT32 times = 0 ;

      _shardLatch.get_shared() ;
      /// clear all node status
      _cataGrpItem.clearNodesStat() ;

      _shardLatch.release_shared() ;

      // build catalog group request
      MsgCatCatGroupReq req ;
      req.header.opCode = MSG_CAT_CATGRP_REQ ;
      req.id.value = 0 ;
      req.id.columns.groupID = CATALOG_GROUPID ;

      if ( millsec > 0 )
      {
         // use request id to store tid
         req.header.requestID = (UINT64)ossGetCurrentThreadID() ;
         _upCatEvent.reset() ;
      }

   retry:
      ++times ;
      //send to a catalog
      rc = sendToCatlog( ( MsgHeader*)&req, &_upCatHandle, 0, FALSE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to sync send to catalog, rc = %d", rc ) ;
         goto error ;
      }

      if ( millsec > 0 )
      {
         INT32 result = 0 ;
         rc = _upCatEvent.wait( millsec, &result ) ;
         if ( SDB_OK == rc )
         {
            rc = result ;
         }
         _upCatHandle = NET_INVALID_HANDLE ;

         if ( rc )
         {
            if ( SDB_NETWORK_CLOSE == rc &&
                 times < CLS_CATA_RETRY_MAX_TIMES )
            {
               goto retry ;
            }
            PD_LOG( PDERROR, "Update catalog group info failed, rc: %d", rc ) ;
            /// don't goto error
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPDCATGRP, rc );
      return rc ;
   error :
      goto done ;
   }

   // drop all data cs
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_CLRALLDATA, "_clsShardMgr::clearAllData" )
   INT32 _clsShardMgr::clearAllData ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_CLRALLDATA );
      _SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      MON_CS_LIST csList ;

      PD_LOG ( PDEVENT, "Clear all dms data" ) ;

      //dump all collectionspace
      dmsCB->dumpInfo( csList, TRUE ) ;
      MON_CS_LIST::const_iterator it = csList.begin() ;
      // for now we don't need to latch dms, since it's only used by
      // full sync, and in that case there's no application able to modify
      // collection space information
      while ( it != csList.end() )
      {
         // drop all collection spaces
         const _monCollectionSpace &cs = *it ;
         rc = rtnDropCollectionSpaceCommand ( cs._name, NULL, dmsCB, NULL,
                                              TRUE ) ;
         if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
         {
            PD_LOG ( PDERROR, "Clear collectionspace[%s] failed[rc:%d]",
               cs._name, rc ) ;
            break ;
         }
         PD_LOG ( PDDEBUG, "Clear collectionspace[%s] succeed", cs._name ) ;
         ++it ;
      }

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_CLRALLDATA, rc );
      return rc ;
   }

   INT32 _clsShardMgr::syncUpdateCatalog ( const CHAR *pCollectionName,
                                           const INT64 millsec )
   {
      return syncUpdateCatalog( UTIL_UNIQUEID_NULL, pCollectionName,
                                millsec ) ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCUPDCAT, "_clsShardMgr::syncUpdateCatalog" )
   INT32 _clsShardMgr::syncUpdateCatalog ( utilCLUniqueID clUniqueID,
                                           const CHAR *pCollectionName,
                                           const INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCUPDCAT );
      BOOLEAN send = FALSE ;
      clsEventItem *pEventInfo = NULL ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

      if ( !pCollectionName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "collection name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      _catLatch.get() ;
      // look for sync event from cache
      // memory will be released in this function, when there's no thread
      // wait for the event
      pEventInfo = _findCatSyncEvent( pCollectionName, clUniqueID, TRUE ) ;
      if ( !pEventInfo )
      {
         _catLatch.release () ;
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate memory for event info, "
                  "rc = %d", rc ) ;
         goto error ;
      }

      // First judge the request is send or not
      if ( FALSE == pEventInfo->send )
      {
         // if the event has not been sent, let's send to catalog
         rc = _sendCatalogReq ( pCollectionName, clUniqueID, 0,
                                &(pEventInfo->netHandle), millsec ) ;
         if ( SDB_OK == rc )
         {
            pEventInfo->send = TRUE ;
            send = TRUE ;
            pEventInfo->waitNum++ ;
            pEventInfo->requestID = _requestID ;
            pEventInfo->event.reset () ;
         }
      }
      else
      {
         // if the event is already sent, let's increase the wait counter
         // note this counter must be protected within catLatch
         pEventInfo->waitNum++ ;
      }
      _catLatch.release () ;

      // we wait for the event if we didn't send anything, or the sent
      // complete successfully
      if ( SDB_OK == rc )
      {
         // wait until event is acknowledged
         INT32 result = SDB_OK ;
         rc = pEventInfo->event.wait ( millsec, &result ) ;
         if ( SDB_OK == rc )
         {
            /// result > 0, means not primary but has already update primary
            rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
         }

         if ( SDB_NET_CANNOT_CONNECT == rc )
         {
            /// the node is crashed, sleep some seconds
            PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                    "other nodes not aware, sleep %d seconds",
                    NET_NODE_FAULTUP_MIN_TIME ) ;
            ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_NETWORK_CLOSE == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = result > 0 ? TRUE : FALSE ;
         }
         else if ( rc && SDB_DMS_NOTEXIST != rc )
         {
            PD_LOG( PDERROR, "Update catalog[%s] failed, rc: %d",
                    pCollectionName, rc ) ;
            /// don't goto error
         }

         // if send=TRUE, must reset send flag
         _catLatch.get () ;
         // decrease the wait number, this must be protected within catLatch
         pEventInfo->waitNum-- ;

         if ( send )
         {
            pEventInfo->send = FALSE ;
         }

         // clear event info if there's no thread wait for it
         if ( 0 == pEventInfo->waitNum )
         {
            pEventInfo->event.reset () ;

            //release the event info
            SDB_OSS_DEL pEventInfo ;
            pEventInfo = NULL ;
            if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
            {
               _mapSyncCLIDEvent.erase ( clUniqueID ) ;
            }
            else
            {
               _mapSyncCatEvent.erase ( pCollectionName ) ;
            }
         }

         _catLatch.release () ;
      }

      /// if need retry, update catalog and retry
      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( millsec ) )
         {
            goto retry ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCUPDCAT, rc );
      return rc ;
   error:
      goto done ;
   }

   // Update information for any group
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_SYNCUPDGPINFO, "_clsShardMgr::syncUpdateGroupInfo" )
   INT32 _clsShardMgr::syncUpdateGroupInfo ( UINT32 groupID, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_SYNCUPDGPINFO );
      BOOLEAN send = FALSE ;
      clsEventItem *pEventInfo = NULL ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      _catLatch.get() ;

      // let's try to create or find existing sync event for a given group
      // memory will be released in this function if there's no other threads
      // wait for the event
      pEventInfo = _findNMSyncEvent( groupID, TRUE ) ;
      if ( !pEventInfo )
      {
         _catLatch.release () ;
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate event info for group %d, "
                  "rc = %d", groupID, rc ) ;
         goto error ;
      }

      //First judge the request is send or not
      if ( FALSE == pEventInfo->send )
      {
         // send group request to catalog ONLY if it's not been sent yet
         rc = _sendGroupReq( groupID, 0, &(pEventInfo->netHandle),
                             millsec ) ;
         // if it's successfully sent, let's mark and wait
         if ( SDB_OK == rc )
         {
            pEventInfo->send = TRUE ;
            send = TRUE ;
            pEventInfo->waitNum++ ;
            pEventInfo->requestID = _requestID ;
            pEventInfo->event.reset () ;
         }
      }
      else
      {
         pEventInfo->waitNum++ ;
      }

      _catLatch.release () ;

      // wait only when someone else is already sent the request
      // or the sent from current thread success
      if ( SDB_OK == rc )
      {
         INT32 result = SDB_OK ;
         rc = pEventInfo->event.wait ( millsec, &result ) ;

         if ( SDB_OK == rc )
         {
            /// result > 0, means not primary but has already update primary
            rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
         }

         if ( SDB_NET_CANNOT_CONNECT == rc )
         {
            /// the node is crashed, sleep some seconds
            PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                    "other nodes not aware, sleep %d seconds",
                    NET_NODE_FAULTUP_MIN_TIME ) ;
            ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_NETWORK_CLOSE == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = FALSE ;
         }
         else if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            needRetry = TRUE ;
            hasUpCataGrp = result > 0 ? TRUE : FALSE ;
         }
         else if ( rc && SDB_CLS_GRP_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "Update group info[%d] failed, rc: %d",
                    groupID, rc ) ;
            /// don't goto error
         }

         // if send=TRUE, must reset send flag
         _catLatch.get () ;
         pEventInfo->waitNum-- ;

         if ( send )
         {
            pEventInfo->send = FALSE ;
         }

         // clear memory resource when there's no other threads
         // wait for the event
         if ( 0 == pEventInfo->waitNum )
         {
            pEventInfo->event.reset () ;

            //release the event info
            SDB_OSS_DEL pEventInfo ;
            pEventInfo = NULL ;
            _mapSyncNMEvent.erase ( groupID ) ;
         }

         _catLatch.release () ;
      }

      /// if need retry, update catalog and retry
      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( millsec ) )
         {
            goto retry ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_SYNCUPDGPINFO,  rc );
      return rc ;
   error:
      goto done ;
   }

   // set or unset a given node id as catalog primary
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPDPRM, "_clsShardMgr::updatePrimary" )
   INT32 _clsShardMgr::updatePrimary ( const NodeID & id, BOOLEAN primary )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPDPRM ) ;
      INT32 rc = SDB_OK ;
      NodeID tmpID ;
      tmpID.value = id.value ;
      tmpID.columns.groupID = CATALOG_GROUPID ;

      _shardLatch.get_shared() ;
      rc = _cataGrpItem.updatePrimary( tmpID, primary ) ;
      _shardLatch.release_shared() ;

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPDPRM, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_UPPRM_BYREPLY, "_clsShardMgr::updatePrimaryByReply" )
   INT32 _clsShardMgr::updatePrimaryByReply( MsgHeader *pMsg, UINT32 groupID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_UPPRM_BYREPLY ) ;
      INT32 rc = MSG_GET_INNER_REPLY_RC( pMsg ) ;
      UINT32 startFrom = MSG_GET_INNER_REPLY_STARTFROM( pMsg ) ;

      if ( IS_REPLY_TYPE( pMsg->opCode ) &&
           SDB_CLS_NOT_PRIMARY == rc &&
           0 != startFrom )
      {
         INT32 preStat = NET_NODE_STAT_NORMAL ;
         NodeID primaryNode ;
         primaryNode.columns.nodeID = startFrom ;
         primaryNode.columns.groupID = groupID ;
         if ( CATALOG_GROUPID == groupID )
         {
            primaryNode.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;

            _shardLatch.get_shared() ;
            rc = _cataGrpItem.updatePrimary( primaryNode, TRUE, &preStat ) ;
            if ( NET_NODE_STAT_NORMAL != preStat )
            {
               _cataGrpItem.cancelPrimary() ;
               rc = SDB_NET_CANNOT_CONNECT ;
            }
            _shardLatch.release_shared() ;
         }
         else
         {
            primaryNode.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;

            clsGroupItem *pGroupItem = NULL ;
            rc = getAndLockGroupItem( groupID, &pGroupItem, FALSE ) ;
            if ( SDB_OK == rc )
            {
               rc = pGroupItem->updatePrimary( primaryNode, TRUE, &preStat ) ;
               if ( NET_NODE_STAT_NORMAL != preStat )
               {
                  pGroupItem->cancelPrimary() ;
                  rc = SDB_NET_CANNOT_CONNECT ;
               }
               unlockGroupItem( pGroupItem ) ;
            }
         }

         if ( NET_NODE_STAT_NORMAL != preStat )
         {
            PD_LOG( PDWARNING, "Group(%d) primary node[%d] is crashed",
                    groupID, startFrom ) ;
         }
         else if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Update group(%d) primary node to [%d] "
                    "by reply message", groupID, startFrom ) ;
         }
      }

      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_UPPRM_BYREPLY, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SNDGPREQ, "_clsShardMgr::_sendGroupReq" )
   INT32 _clsShardMgr::_sendGroupReq ( UINT32 groupID, UINT64 requestID,
                                       NET_HANDLE *pHandle, INT64 millsec )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SNDGPREQ );
      _MsgCatGroupReq msg ;
      msg.header.opCode = MSG_CAT_NODEGRP_REQ ;

      // set to default request id if we didn't specify any
      if ( 0 == requestID )
      {
         requestID = ++_requestID ;
      }
      msg.header.requestID = requestID ;
      msg.id.columns.groupID = groupID ;

      INT32 rc = sendToCatlog( (MsgHeader *)&msg, pHandle, millsec, TRUE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send to catalog, rc = %d", rc ) ;
         goto error ;
      }

      PD_LOG ( PDDEBUG, "send group req[id: %d, requestID: %lld, rc: %d]",
               groupID, _requestID, rc ) ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SNDGPREQ, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDCATAQUERYREQ, "_clsShardMgr::_sendCataQueryReq" )
   INT32 _clsShardMgr::_sendCataQueryReq( INT32 queryType,
                                          const BSONObj &query,
                                          UINT64 requestID,
                                          NET_HANDLE *pHandle,
                                          INT64 millsec )
   {
      INT32 rc        = SDB_OK ;
      CHAR *pBuffer   = NULL ;
      INT32 buffSize  = 0 ;
      MsgHeader * msg = NULL ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SENDCATAQUERYREQ ) ;

      if ( 0 == requestID )
      {
         requestID = ++_requestID ;
      }

      // pBuffer is freed by end of the function
      rc = msgBuildQueryMsg ( &pBuffer, &buffSize, "CAT", 0, requestID, 0,
                              -1, &query, NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to build query msg, rc = %d", rc ) ;
         goto error ;
      }

      msg = (MsgHeader *) pBuffer ;
      msg->opCode = queryType ;
      msg->TID = 0 ;
      msg->routeID.value = 0 ;
      //send message
      rc = sendToCatlog ( msg, pHandle, millsec, TRUE ) ;
      if ( rc )
      {
         // we don't want the error flush diaglog when catalog is offline
         PD_LOG ( PDDEBUG, "Failed to send message to catalog, rc = %d", rc ) ;
         goto error ;
      }

   done:
      if ( pBuffer )
      {
         SDB_OSS_FREE ( pBuffer ) ;
         pBuffer = NULL ;
      }
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SENDCATAQUERYREQ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SNDCATREQ, "_clsShardMgr::_sendCatalogReq" )
   INT32 _clsShardMgr::_sendCatalogReq ( const CHAR *pCollectionName,
                                         utilCLUniqueID clUniqueID,
                                         UINT64 requestID,
                                         NET_HANDLE *pHandle,
                                         INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SNDCATREQ );
      BSONObj query ;
      // sanity check
      if ( !pCollectionName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "collection name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

      // build BSON object
      try
      {
         if ( ! UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            query = BSON ( CAT_COLLECTION_NAME << pCollectionName ) ;
         }
         else
         {
            query = BSON ( CAT_CL_UNIQUEID << (INT64)clUniqueID ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception when creating query: %s",
                  e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // attempt to send to catalog
      rc = _sendCataQueryReq( MSG_CAT_QUERY_CATALOG_REQ, query, requestID,
                              pHandle, millsec ) ;
      if ( SDB_OK != rc )
      {
         // use debug level since we don't want this message flush
         // diaglog when catalog is offline
         PD_LOG ( PDDEBUG, "send catelog req[name: %s, requestID: %lld, "
                  "rc: %d]", pCollectionName, requestID, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SNDCATREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDCSINFOREQ, "_clsShardMgr::_sendCSInfoReq" )
   INT32 _clsShardMgr::_sendCSInfoReq( const CHAR * pCSName,
                                       utilCSUniqueID csUniqueID,
                                       UINT64 requestID,
                                       NET_HANDLE *pHandle, INT64 millsec )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__SENDCSINFOREQ ) ;
      // sanity check
      if ( !pCSName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "cs name can't be NULL, rc = %d", rc ) ;
         goto error ;
      }

      // build query
      try
      {
         if ( ! UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
         {
            query = BSON ( CAT_COLLECTION_SPACE_NAME << pCSName ) ;
         }
         else
         {
            // During the upgrade process, it may happen that data is new,
            // while catalog is still old. In this case, if data only sends id,
            // catalog will not be able to handle it.
            query = BSON ( CAT_COLLECTION_SPACE_NAME << pCSName <<
                           CAT_CS_UNIQUEID << (INT64)csUniqueID ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception when creating query: %s",
                  e.what () ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // send catalog query request to catalog
      rc = _sendCataQueryReq( MSG_CAT_QUERY_SPACEINFO_REQ, query, requestID,
                              pHandle, millsec ) ;
      if ( SDB_OK != rc )
      {
         // we use debug level here since we don't want this message
         // flush diaglog if catalog is offline
         PD_LOG ( PDDEBUG,
                  "send collection space req[name: %s, id: %u,requestID: "
                  "%lld, rc: %d]", pCSName, csUniqueID, requestID, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__SENDCSINFOREQ, rc );
      return rc ;
   error:
      goto done ;
   }

   // get catalog group response information
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATGPRES, "_clsShardMgr::_onCatCatGroupRes" )
   INT32 _clsShardMgr::_onCatCatGroupRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATGPRES );
      UINT32 version = 0 ;
      MsgRouteID primaryNode ;
      UINT32 primary = 0 ;
      std::string groupName ;
      map<UINT64, _netRouteNode> mapNodes ;
      MsgCatCatGroupRes *res = (MsgCatCatGroupRes*)msg ;

      // sanity check, make sure the response is OKAY
      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(msg) )
      {
         PD_LOG ( PDERROR, "Update catalog group info failed[rc: %d]",
                  MSG_GET_INNER_REPLY_RC(msg) ) ;
         rc = MSG_GET_INNER_REPLY_RC(msg) ;
         goto error ;
      }

      // parse the returned result
      rc = msgParseCatGroupRes ( res, version, groupName, mapNodes, &primary ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Parse MsgCatCatGroupRes failed[rc: %d]", rc ) ;
         goto error ;
      }
      primaryNode.columns.groupID = CATALOG_GROUPID ;

      //update to shard net agent
      if ( 0 == version || version != _catVerion )
      {
         _shardLatch.get () ;

         pmdOptionsCB *optCB = pmdGetOptionCB() ;
         string oldCfg, newCfg ;
         // remember the old info
         optCB->toString( oldCfg ) ;
         MAP_ROUTE_NODE oldCatNodes = _mapNodes ;
         NodeID oldID ;

         _catVerion = version ;
         _mapNodes.clear() ;
         optCB->clearCatAddr() ;
         map<UINT64, _netRouteNode>::iterator it = mapNodes.begin() ;
         // iterate for each nodes in catalog list
         while ( it != mapNodes.end() )
         {
            clsNodeItem &nodeItem = it->second ;
            setCatlogInfo( nodeItem._id,
                           nodeItem._host,
                           nodeItem._service[MSG_ROUTE_CAT_SERVICE] ) ;
            optCB->setCatAddr( nodeItem._host,
                               nodeItem._service[
                               MSG_ROUTE_CAT_SERVICE].c_str() ) ;
            ++it ;
         }

         /// update catalog group info
         _cataGrpItem.updateNodes( mapNodes ) ;

         /// update catalog net info
         it = _mapNodes.begin() ;
         while ( it != _mapNodes.end() )
         {
            clsNodeItem &nodeItem = it->second ;
            // try to find old catalog information by id
            if ( SDB_OK == _findCatNodeID ( oldCatNodes, nodeItem._host,
                                            nodeItem._service[
                                            MSG_ROUTE_CAT_SERVICE],
                                            oldID ) )
            {
               // if any field changed
               if ( oldID.value != nodeItem._id.value )
               {
                  // update network runtime component
                  _pNetRtAgent->updateRoute ( oldID, nodeItem._id ) ;
                  PD_LOG ( PDDEBUG, "Update catalog node[%u:%u] to [%u:%u]",
                           oldID.columns.groupID, oldID.columns.nodeID,
                           nodeItem._id.columns.groupID,
                           nodeItem._id.columns.nodeID ) ;
               }
            }
            else
            {
               // if another to find by old id, let's simply update using the
               // new info
               _pNetRtAgent->updateRoute ( nodeItem._id,
                                           nodeItem._host,
                                           nodeItem._service[
                                           MSG_ROUTE_CAT_SERVICE].c_str() ) ;
               PD_LOG ( PDDEBUG, "Update catalog node[%u:%u] to %s:%s",
                        nodeItem._id.columns.groupID,
                        nodeItem._id.columns.nodeID,
                        nodeItem._host,
                        nodeItem._service[MSG_ROUTE_CAT_SERVICE].c_str() ) ;
            }
            // note we do not remove oldID that no longer appears in new config
            // total refresh must be done by restarting database
            ++it ;
         }
         // convert new optcb to string
         optCB->toString( newCfg ) ;
         // if old and new are different, let's flush
         if ( oldCfg != newCfg )
         {
            // refresh to config file
            optCB->reflush2File() ;
         }

         _shardLatch.release () ;
      }

      //update primary
      if ( primary != 0 )
      {
         primaryNode.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
         primaryNode.columns.nodeID = primary ;
         rc = updatePrimary ( primaryNode, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to update primary, rc = %d", rc ) ;
            goto error ;
         }
         else
         {
            PD_LOG( PDEVENT, "Update catalog group primary node to [%d] by "
                    "group info", primary ) ;
         }
      }

   done:
      // signal all threads that's wait for catalog update
      _upCatEvent.signalAll( rc ) ;
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__ONCATGPRES, rc );
      return rc ;
   error:
      goto done ;
   }

   // event of data group update
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATGRPRES, "_clsShardMgr::_onCatGroupRes" )
   INT32 _clsShardMgr::_onCatGroupRes ( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATGRPRES );
      clsEventItem *pEventInfo = NULL ;

      PD_LOG ( PDDEBUG, "Recieve catalog group res[requestID:%lld,flag:%d]",
               msg->requestID, MSG_GET_INNER_REPLY_RC(msg) ) ;

      ossScopedLock lock ( &_catLatch ) ;

      // find a request, we don't create new memory if such event doesn't exist
      // for async call we use 0 as requestID, so we may not get this event
      // in this case if we get success result we have to get the event based on
      // group id
      pEventInfo = _findNMSyncEvent( msg->requestID ) ;

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC(msg) )
      {
         // for error handling
         // 1) group not exist -> we have to clear local cache
         // 2) not primary -> ignore or resend request
         // 3) others report error
         rc = MSG_GET_INNER_REPLY_RC(msg) ;
         // if we are able to find the event
         if ( pEventInfo )
         {
            // let's check if response shows unable to find group
            if ( SDB_CLS_GRP_NOT_EXIST == rc ||
                 SDB_DMS_EOC == rc )
            {
               // in that case, let's clear local group cache information
               _pNodeMgrAgent->lock_w() ;
               _pNodeMgrAgent->clearGroup( pEventInfo->groupID ) ;
               _pNodeMgrAgent->release_w() ;
               pEventInfo->event.signalAll( SDB_CLS_GRP_NOT_EXIST ) ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               INT32 rcTmp = updatePrimaryByReply( msg ) ;
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  /// the node is crashed
                  pEventInfo->event.signalAll ( rcTmp ) ;
               }
               else if ( SDB_OK == rcTmp )
               {
                  pEventInfo->event.signalAll ( 1 ) ;
               }
               else
               {
                  pEventInfo->event.signalAll ( rc ) ;
               }
               rc = SDB_OK ;
            }
            else
            {
               // error handling
               PD_LOG ( PDERROR, "Update group[%d] failed[rc:%d]",
                        pEventInfo->groupID, rc ) ;
               pEventInfo->event.signalAll( rc ) ;
            }
         }
      }
      else
      {
         // if successful returned
         _pNodeMgrAgent->lock_w() ;
         const CHAR* objdata = MSG_GET_INNER_REPLY_DATA(msg) ;
         UINT32 length = msg->messageLength -
                         MSG_GET_INNER_REPLY_HEADER_LEN(msg) ;
         UINT32 groupID = 0 ;

         rc = _pNodeMgrAgent->updateGroupInfo( objdata, length, &groupID ) ;
         PD_LOG ( ( SDB_OK == rc ? PDEVENT : PDERROR ),
                  "Update group[groupID:%u, rc: %d]", groupID, rc ) ;

         //udpate node info to netAgent
         clsGroupItem* groupItem = NULL ;
         if ( SDB_OK == rc )
         {
            groupItem = _pNodeMgrAgent->groupItem( groupID ) ;
            // if the event does not exist, we look for the event based
            // on group id
            // this may happen in async request where requestID = 0
            // in this case the previous call will not be able to get anything
            // so we have to get the wait event here
            if ( !pEventInfo )
            {
               pEventInfo = _findNMSyncEvent( groupID, FALSE ) ;
            }
         }
         // if we do have group information in cache, let's update them
         if ( groupItem )
         {
            UINT32 indexPos = 0 ;
            MsgRouteID nodeID ;
            std::string hostName ;
            std::string service ;

            while ( SDB_OK == groupItem->getNodeInfo( indexPos, nodeID,
                                                      hostName, service,
                                                    MSG_ROUTE_SHARD_SERVCIE ) )
            {
               _pNetRtAgent->updateRoute( nodeID, hostName.c_str(),
                                          service.c_str() ) ;
               ++indexPos ;
            }
         }
         _pNodeMgrAgent->release_w() ;

         // if we have event registered, let's signal to all waiters
         if ( pEventInfo )
         {
            pEventInfo->event.signalAll( rc ) ;
         }
      }

      PD_TRACE_EXITRC (SDB__CLSSHDMGR__ONCATGRPRES, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONCATREQMSG, "_clsShardMgr::_onCatalogReqMsg" )
   INT32 _clsShardMgr::_onCatalogReqMsg ( NET_HANDLE handle, MsgHeader* msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__ONCATREQMSG );
      MsgCatQueryCatRsp *res   = ( MsgCatQueryCatRsp*)msg ;
      PD_LOG ( PDDEBUG, "Recieve catalog response[requestID: %lld, flag: %d]",
               msg->requestID, res->flags ) ;

      INT32 flag               = 0 ;
      INT64 contextID          = -1 ;
      INT32 startFrom          = 0 ;
      INT32 numReturned        = 0 ;
      vector < BSONObj > objList ;
      UINT32 groupID           = nodeID().columns.groupID ;
      INT32 rc                 = SDB_OK ;
      clsEventItem *pEventInfo = NULL ;

      ossScopedLock lock ( &_catLatch ) ;

      if ( SDB_OK != res->flags )
      {
         rc = SDB_CLS_UPDATE_CAT_FAILED ;

         //need to found event info by request id
         // note this request id could be 0 if it's async call
         // in that case we'll get pEventInfo=NULL
         // and that is absolutely expected
         pEventInfo = _findCatSyncEvent ( msg->requestID ) ;
         if ( pEventInfo )
         {
            //the catalog info is delete, so will delete the local item
            if ( SDB_DMS_EOC == res->flags ||
                 SDB_DMS_NOTEXIST == res->flags )
            {
               _pCatAgent->lock_w () ;
               rc = _pCatAgent->clear ( pEventInfo->name.c_str() ) ;
               _pCatAgent->release_w () ;
               pEventInfo->event.signalAll ( SDB_DMS_NOTEXIST ) ;
            }
            //not primary node, should update catalog group info, and send again
            else if ( SDB_CLS_NOT_PRIMARY == res->flags )
            {
               INT32 rcTmp = updatePrimaryByReply( msg ) ;
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  /// the node is crashed
                  pEventInfo->event.signalAll ( rcTmp ) ;
               }
               else if ( SDB_OK == rcTmp )
               {
                  pEventInfo->event.signalAll ( 1 ) ;
               }
               else
               {
                  pEventInfo->event.signalAll ( rc ) ;
               }
               rc = SDB_OK ;
            }
            //update catalog failed
            else
            {
               PD_LOG ( PDERROR, "Update catalog[%s] failed[response: %d]",
                        pEventInfo->name.c_str(), res->flags ) ;
               pEventInfo->event.signalAll ( res->flags ) ;
            }
         }
      }
      else
      {
         _clsCatalogSet *catSet = NULL ;
         INT32 version = 0 ;
         UINT32 groupCount = 0 ;
         const CHAR *pCLType = "normal" ;
         utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
         string collectionName ;

         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to extract reply msg, rc = %d", rc ) ;
            goto error ;
         }

         SDB_ASSERT ( numReturned == 1 && objList.size() == 1,
                      "Collection catalog item num must be 1" ) ;

         _pCatAgent->lock_w () ;
         rc = _pCatAgent->updateCatalog ( 0, groupID, objList[0].objdata(),
                                          objList[0].objsize(), &catSet ) ;
         // check rc first
         if ( SDB_OK != rc )
         {
            // release lock before go away
            _pCatAgent->release_w() ;
            PD_LOG( PDERROR, "failed to update catalog:%d", rc ) ;
            goto error ;
         }
         // get attributes from catalog set
         if ( catSet )
         {
            version = catSet->getVersion() ;
            groupCount = catSet->groupCount() ;
            collectionName = catSet->nameStr() ;
            clUniqueID = catSet->clUniqueID() ;
            if ( catSet->isMainCL() )
            {
               pCLType = "main" ;
            }
            else if ( !catSet->getMainCLName().empty() )
            {
               pCLType = "sub" ;
            }
         }
         _pCatAgent->release_w () ;

         PD_LOG ( PDEVENT,
                  "Update catalog[name: %s, id: %llu, version:%u, type: %s, "
                  "group count: %d, rc: %d]", collectionName.c_str(),
                  clUniqueID, version, pCLType, groupCount, rc ) ;

         //signal collection info event, since the previous pEVentInfo
         //could be NULL ( if it's async call, requestID is 0 ), we'll have
         //to check for pEventInfo here again
         pEventInfo = _findCatSyncEvent( collectionName.c_str(), clUniqueID ) ;
         if ( !pEventInfo )
         {
            // we have already looked up by name and id, find out nothing
            goto done ;
         }
         if ( pEventInfo->requestID == msg->requestID )
         {
            pEventInfo->event.signalAll ( rc ) ;
            goto done ;
         }

         if ( ! UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            // we have already looked up by name, so just goto done.
            goto done ;
         }
         pEventInfo = _findCatSyncEvent( collectionName.c_str() ) ;
         if ( pEventInfo && pEventInfo->requestID == msg->requestID )
         {
            pEventInfo->event.signalAll ( rc ) ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__ONCATREQMSG, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATSYNCEV, "_clsShardMgr::_findCatSyncEvent" )
   clsEventItem *_clsShardMgr::_findCatSyncEvent ( const CHAR * pCollectionName,
                                                   utilCLUniqueID clUniqueID,
                                                   BOOLEAN bCreate )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATSYNCEV );
      SDB_ASSERT ( pCollectionName , "Collection name can't be NULL" ) ;

      clsEventItem *pEventInfo = NULL ;
      MAP_CLID_EVENT_IT it1 ;
      MAP_CAT_EVENT_IT it2 ;

      /// search from id-map, if find out nothing, then search from name-map
      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         it1 = _mapSyncCLIDEvent.find ( clUniqueID ) ;
         if ( it1 != _mapSyncCLIDEvent.end() )
         {
            pEventInfo = it1->second ;
            goto done ;
         }
      }

      it2 = _mapSyncCatEvent.find ( pCollectionName ) ;
      if ( it2 != _mapSyncCatEvent.end() )
      {
         pEventInfo = it2->second ;
         goto done ;
      }

      /// If it has clUniqueID, insert new event item to id-map.
      /// If it hasn't clUniqueID, insert to name-map. Just only insert one map.
      if ( bCreate )
      {
         pEventInfo = SDB_OSS_NEW _clsEventItem ;
         pEventInfo->name = pCollectionName ;
         pEventInfo->clUniqueID = clUniqueID ;
         if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID) )
         {
            _mapSyncCLIDEvent[ clUniqueID ] = pEventInfo ;
         }
         else
         {
            _mapSyncCatEvent[ pCollectionName ] = pEventInfo ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDCATSYNCEV );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATSYNCEVN, "_clsShardMgr::_findCatSyncEvent" )
   clsEventItem *_clsShardMgr::_findCatSyncEvent ( UINT64 requestID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATSYNCEVN );
      clsEventItem *pEventInfo = NULL ;
      MAP_CAT_EVENT_IT it ;
      MAP_CLID_EVENT_IT idit ;

      it = _mapSyncCatEvent.begin() ;
      while ( it != _mapSyncCatEvent.end() )
      {
         pEventInfo = it->second ;
         if ( pEventInfo->requestID == requestID )
         {
            goto done ;
         }
         ++it ;
      }

      idit = _mapSyncCLIDEvent.begin() ;
      while ( idit != _mapSyncCLIDEvent.end() )
      {
         pEventInfo = idit->second ;
         if ( pEventInfo->requestID == requestID )
         {
            goto done ;
         }
         ++idit ;
      }

      pEventInfo = NULL ;
   done :
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDCATSYNCEVN );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDNMSYNCEV, "_clsShardMgr::_findNMSyncEvent" )
   clsEventItem* _clsShardMgr::_findNMSyncEvent( UINT32 groupID,
                                                 BOOLEAN bCreate )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDNMSYNCEV );
      clsEventItem *pEventInfo = NULL ;
      MAP_NM_EVENT_IT it = _mapSyncNMEvent.find ( groupID ) ;
      if ( it != _mapSyncNMEvent.end() )
      {
         pEventInfo = it->second ;
         goto done ;
      }

      if ( !bCreate )
      {
         goto done ;
      }

      //create new event info
      pEventInfo = SDB_OSS_NEW _clsEventItem ;
      pEventInfo->groupID = groupID ;
      //add to map
      _mapSyncNMEvent[groupID] = pEventInfo ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDNMSYNCEV );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDNMSYNCEVN, "_clsShardMgr::_findNMSyncEvent" )
   clsEventItem* _clsShardMgr::_findNMSyncEvent ( UINT64 requestID )
   {
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDNMSYNCEVN );
      clsEventItem *pEventInfo = NULL ;
      MAP_NM_EVENT_IT it = _mapSyncNMEvent.begin() ;
      while ( it != _mapSyncNMEvent.end() )
      {
         pEventInfo = it->second ;
         if ( pEventInfo->requestID == requestID )
         {
            goto done ;
         }
         ++it ;
      }
      pEventInfo = NULL ;
   done :
      PD_TRACE_EXIT ( SDB__CLSSHDMGR__FNDNMSYNCEVN );
      return pEventInfo ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__FNDCATNODEID, "_clsShardMgr::_findCatNodeID" )
   INT32 _clsShardMgr::_findCatNodeID ( MAP_ROUTE_NODE &catNodes,
                                        const CHAR *hostName,
                                        const std::string & service,
                                        NodeID & id )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR__FNDCATNODEID );
      MAP_ROUTE_NODE_IT it = catNodes.begin() ;
      while ( it != catNodes.end() )
      {
         const clsNodeItem &nodeItem = it->second ;
         if ( 0 == ossStrcmp( nodeItem._host, hostName ) &&
              nodeItem._service[ MSG_ROUTE_CAT_SERVICE ] == service )
         {
            id.value = nodeItem._id.value ;
            goto done ;
         }
         ++it ;
      }
      rc = SDB_CLS_NODE_NOT_EXIST ;
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR__FNDCATNODEID, rc );
      return rc ;
   }

   /**
    * Get and LOCK catalog cache, in case that someone clear this catalog cache.
    * Used it with function unlockCataSet().
    *
    * @param(i) name          -- collection name
    * @param(i) ppSet         -- point to catalog cache
    * @param(i) noWithUpdate  -- whether update catalog if not found
    * @param(i) waitMillSec   -- wait time for updating
    * @param(o) pUpdated      -- whether catalog cache has been updated
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_GETANDLOCKCATSET, "_clsShardMgr::getAndLockCataSet" )
   INT32 _clsShardMgr::getAndLockCataSet( const CHAR * name,
                                          clsCatalogSet **ppSet,
                                          BOOLEAN noWithUpdate,
                                          INT64 waitMillSec,
                                          BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_GETANDLOCKCATSET );
      // sanity check
      SDB_ASSERT ( ppSet && name,
                   "ppSet and name can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pCatAgent->lock_r() ;
         *ppSet = _pCatAgent->collectionSet( name ) ;
         // if we can't find the name and request to update catalog
         // we'll call syncUpdateCatalog and refind again
         if ( !(*ppSet) && noWithUpdate )
         {
            _pCatAgent->release_r() ;
            // request to update catalog
            rc = syncUpdateCatalog( name, waitMillSec ) ;
            if ( rc )
            {
               // if we can't find the collection and not able to update
               // catalog, we'll return the error of synUpdateCatalog
               // call
               PD_LOG ( PDERROR, "Failed to sync update catalog, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            // we don't want to update again
            noWithUpdate = FALSE ;
            continue ;
         }
         // if still not able to find it
         if ( !(*ppSet) )
         {
            _pCatAgent->release_r() ;
            rc = SDB_CLS_NO_CATALOG_INFO ;
         }
         break ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_GETANDLOCKCATSET, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsShardMgr::unlockCataSet( clsCatalogSet * catSet )
   {
      if ( catSet )
      {
         _pCatAgent->release_r() ;
      }
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_GETNLCKGPITEM, "_clsShardMgr::getAndLockGroupItem" )
   INT32 _clsShardMgr::getAndLockGroupItem( UINT32 id, clsGroupItem **ppItem,
                                            BOOLEAN noWithUpdate,
                                            INT64 waitMillSec,
                                            BOOLEAN * pUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSHDMGR_GETNLCKGPITEM );
      SDB_ASSERT ( ppItem, "ppItem can't be NULL" ) ;

      while ( SDB_OK == rc )
      {
         _pNodeMgrAgent->lock_r() ;
         *ppItem = _pNodeMgrAgent->groupItem( id ) ;
         // can we find the group from local cache?
         if ( !(*ppItem) && noWithUpdate )
         {
            _pNodeMgrAgent->release_r() ;
            // if we can't find such group and is okay to update cache
            // we'll update cache from catalog
            rc = syncUpdateGroupInfo( id, waitMillSec ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to sync update group info, rc = %d",
                        rc ) ;
               goto error ;
            }
            if ( pUpdated )
            {
               *pUpdated = TRUE ;
            }
            // only update it once
            noWithUpdate = FALSE ;
            continue ;
         }
         // if we are not able to find one, let's return group not found
         if ( !(*ppItem) )
         {
            _pNodeMgrAgent->release_r() ;
            rc = SDB_CLS_NO_GROUP_INFO ;
         }
         break ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__CLSSHDMGR_GETNLCKGPITEM, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _clsShardMgr::unlockGroupItem( clsGroupItem * item )
   {
      if ( item )
      {
         _pNodeMgrAgent->release_r() ;
      }
      return SDB_OK ;
   }

   /*
    if csUniqueID != 0, means it is a input parameter
    if csUniqueID == 0, means it is a output parameter
    */
   INT32 _clsShardMgr::rGetCSInfo( const CHAR * csName,
                                   utilCSUniqueID &csUniqueID,
                                   UINT32 *pageSize,
                                   UINT32 *lobPageSize,
                                   DMS_STORAGE_TYPE *type,
                                   BSONObj *clInfo,
                                   string *newCSName,
                                   INT64 waitMillSec )
   {
      INT32 rc = SDB_OK ;
      clsCSEventItem *item = NULL ;
      UINT64 requestID = 0 ;
      INT32 result = 0 ;
      UINT32 retryTimes = 0 ;
      BOOLEAN needRetry = FALSE ;
      BOOLEAN hasUpCataGrp = FALSE ;

      SDB_ASSERT ( csName, "collection space name can't be NULL" ) ;
      if ( !csName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // memory will be freed by end of the function
      item = SDB_OSS_NEW clsCSEventItem() ;
      if ( NULL == item )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory failed" ) ;
         goto error ;
      }
      item->csName = csName ;

      _catLatch.get() ;
      requestID = ++_requestID ;
      _mapSyncCSEvent[ requestID ] = item ;
      _catLatch.release() ;

   retry:
      ++retryTimes ;
      needRetry = FALSE ;
      // send request
      rc = _sendCSInfoReq( csName, csUniqueID, requestID, &(item->netHandle),
                           waitMillSec ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to send cs info request, rc = %d", rc ) ;
         goto error ;
      }

      // wait for response
      result = SDB_OK ;
      rc = item->event.wait( waitMillSec, &result ) ;
      if ( SDB_OK == rc )
      {
         /// result > 0, means not primary but has already update primary
         rc = result <= 0 ? result : SDB_CLS_NOT_PRIMARY ;
      }

      if ( SDB_NET_CANNOT_CONNECT == rc )
      {
         /// the node is crashed, sleep some seconds
         PD_LOG( PDWARNING, "Catalog group primary node is crashed but "
                 "other nodes not aware, sleep %d seconds",
                 NET_NODE_FAULTUP_MIN_TIME ) ;
         ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
         needRetry = TRUE ;
         hasUpCataGrp = FALSE ;
      }
      else if ( SDB_NETWORK_CLOSE == rc )
      {
         needRetry = TRUE ;
         hasUpCataGrp = FALSE ;
      }
      else if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         needRetry = TRUE ;
         hasUpCataGrp = result > 0 ? TRUE : FALSE ;
      }

      /// if need retry, update catalog and retry
      if ( needRetry && retryTimes < CLS_CATA_RETRY_MAX_TIMES )
      {
         if ( hasUpCataGrp || SDB_OK == updateCatGroup( waitMillSec ) )
         {
            goto retry ;
         }
      }

      // sanity chekc for result
      PD_RC_CHECK( rc, PDWARNING, "Get collection space[%s] info failed, "
                   "rc: %d", csName, rc ) ;
      if ( newCSName )
      {
         *newCSName = item->csName ;
      }
      if ( UTIL_UNIQUEID_NULL == csUniqueID )
      {
         csUniqueID = item->csUniqueID ;
      }
      if ( pageSize )
      {
         *pageSize = item->pageSize ;
      }
      if ( lobPageSize )
      {
         *lobPageSize = item->lobPageSize ;
      }
      if ( type )
      {
         *type = item->type ;
      }
      if ( clInfo )
      {
         *clInfo = item->clInfo.getOwned() ;
      }

   done:
      _catLatch.get() ;
      _mapSyncCSEvent.erase( requestID ) ;
      _catLatch.release() ;

      if ( item )
      {
         SDB_OSS_DEL item ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::updateDCBaseInfo()
   {
      INT32 rc = SDB_OK ;
      rtnQueryOptions queryOpt ;
      CHAR *pBuff = NULL ;
      INT32 bufSize = 0 ;
      MsgHeader *pRecvMsg = NULL ;
      MsgOpReply *pReply = NULL ;
      const UINT32 maxRetryTimes = 3 ;
      UINT32 retryTimes = 0 ;

      queryOpt.setCLFullName( CAT_SYSDCBASE_COLLECTION_NAME ) ;
      queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;
      queryOpt.setQuery( BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ) ;

      rc = queryOpt.toQueryMsg( &pBuff, bufSize, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build query message failed, rc: %d", rc ) ;
         goto error ;
      }

      while( retryTimes++ < maxRetryTimes )
      {
         /// send message
         rc = syncSend( ( MsgHeader* )pBuff, CATALOG_GROUPID, TRUE,
                        &pRecvMsg ) ;
         if ( rc )
         {
            rc = syncSend( ( MsgHeader* )pBuff, CATALOG_GROUPID, FALSE,
                           &pRecvMsg ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         /// extrace reply
         pReply = ( MsgOpReply* )pRecvMsg ;
         rc = pReply->flags ;
         SDB_ASSERT( pReply->contextID == -1, "Context id must be -1" ) ;

         if ( SDB_CLS_NOT_PRIMARY == rc )
         {
            INT32 rcTmp = SDB_OK ;
            rcTmp = updatePrimaryByReply( pRecvMsg ) ;

            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               /// the node is crashed, sleep some seconds
               PD_LOG( PDWARNING, "Catalog group primary node is crashed "
                       "but other nodes not aware, sleep %d seconds",
                       NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }

            if ( rcTmp )
            {
               updateCatGroup( OSS_ONE_SEC ) ;
            }

            SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
            pRecvMsg = NULL ;
            continue ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Update datacenter base info failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         else if ( 1 == pReply->numReturned &&
                   pReply->header.messageLength >
                   (INT32)sizeof( MsgOpReply ) + 5 )
         {
            clsDCBaseInfo *pInfo = _pDCMgr->getDCBaseInfo() ;
            try
            {
               BSONObj obj( ( CHAR* )pRecvMsg + sizeof( MsgOpReply ) ) ;
               rc = _pDCMgr->updateDCBaseInfo( obj ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Update datacenter base info failed, "
                          "rc: %d", rc ) ;
                  goto error ;
               }
               pmdGetKRCB()->setDBReadonly( pInfo->isReadonly() ) ;
               pmdGetKRCB()->setDBDeactivated( !pInfo->isActivated() ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "No data center info" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         /// quit
         break ;
      }

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      if ( pRecvMsg )
      {
         SDB_OSS_FREE( ( CHAR* )pRecvMsg ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::_onQueryCSInfoRsp( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      clsCSEventItem *csItem = NULL ;
      MAP_CS_EVENT_IT it ;
      BSONElement ele ;

      MsgOpReply *res = ( MsgOpReply* )msg ;

      PD_LOG ( PDDEBUG, "Recieve collecton space query response[requestID: "
               "%lld, flag: %d]", msg->requestID, res->flags ) ;

      INT32 flag = 0 ;
      INT64 contextID = -1 ;
      INT32 startFrom = 0 ;
      INT32 numReturned = 0 ;
      vector < BSONObj > objList ;

      ossScopedLock lock ( &_catLatch ) ;

      // based on the requestid, try to find the event
      // there's no async operation for querycs, so we must have request id here
      it = _mapSyncCSEvent.find( msg->requestID ) ;
      if ( it == _mapSyncCSEvent.end() )
      {
         // not found, timeout
         goto done ;
      }

      csItem = it->second ;

      if ( SDB_OK != res->flags )
      {
         rc = res->flags ;

         //not primary node, should send again
         if ( SDB_CLS_NOT_PRIMARY == res->flags )
         {
            INT32 rcTmp = updatePrimaryByReply( msg ) ;
            if ( SDB_NET_CANNOT_CONNECT == rcTmp )
            {
               /// the node is crashed
               csItem->event.signalAll ( rcTmp ) ;
            }
            else if ( SDB_OK == rcTmp )
            {
               csItem->event.signalAll ( 1 ) ;
            }
            else
            {
               csItem->event.signalAll ( rc ) ;
            }
            rc = SDB_OK ;
            goto done ;
         }
         else
         {
            PD_LOG ( PDERROR, "Query collection space[%s] info failed, rc: %d",
                     csItem->csName.c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         // if the response is okay, let's extract reply
         rc = msgExtractReply ( (CHAR *)msg, &flag, &contextID, &startFrom,
                                &numReturned, objList ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         SDB_ASSERT ( numReturned == 1 && objList.size() == 1,
                      "Collection space item num must be 1" ) ;

         try
         {
            //signal collection info event
            ele = objList[0].getField ( CAT_PAGE_SIZE_NAME ) ;
            if ( ele.isNumber() )
            {
               csItem->pageSize = (UINT32)ele.numberInt() ;
            }
            else
            {
               csItem->pageSize = DMS_PAGE_SIZE_DFT ;
            }

            ele = objList[0].getField( CAT_LOB_PAGE_SZ_NAME ) ;
            if ( ele.isNumber() )
            {
               csItem->lobPageSize = (UINT32)ele.numberInt() ;
            }
            else
            {
               csItem->lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
            }

            ele = objList[0].getField( CAT_TYPE_NAME ) ;
            if ( ele.isNumber() &&
                 (INT32)DMS_STORAGE_CAPPED == ele.numberInt() )
            {
               csItem->type = DMS_STORAGE_CAPPED ;
            }
            else
            {
               csItem->type = DMS_STORAGE_NORMAL ;
            }

            ele = objList[0].getField( CAT_COLLECTION_SPACE_NAME ) ;
            if ( ele.type() == String )
            {
               csItem->csName = ele.str() ;
            }

            ele = objList[0].getField( CAT_CS_UNIQUEID ) ;
            if ( ele.eoo() )
            {
               // it is ok, catalog hasn't been upgraded to new version.
               csItem->csUniqueID = UTIL_UNIQUEID_NULL ;
            }
            else if ( ele.isNumber() )
            {
               csItem->csUniqueID = ( utilCSUniqueID ) ele.numberInt() ;
            }
            else
            {
               csItem->csUniqueID = UTIL_UNIQUEID_NULL ;
            }

            // eg:
            // { "Collection": [ { "Name": "bar1", "UniqueID": 2667174690817 } ,
            //                   { "Name": "bar2", "UniqueID": 2667174690818 } ] }
            ele = objList[0].getField( CAT_COLLECTION ) ;
            if ( Array == ele.type() )
            {
               csItem->clInfo = ele.embeddedObject().getOwned() ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      csItem->event.signalAll( rc ) ;

   done:
      return rc ;
   error:
      if ( csItem )
      {
         csItem->event.signalAll( rc ) ;
      }
      goto done ;
   }

   INT32 _clsShardMgr::_onHandleClose( NET_HANDLE handle, MsgHeader *msg )
   {
      ossScopedLock lock( &_catLatch ) ;

      MAP_CAT_EVENT::iterator itCat = _mapSyncCatEvent.begin() ;
      while( itCat != _mapSyncCatEvent.end() )
      {
         clsEventItem *pItem = itCat->second ;
         ++itCat ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      MAP_NM_EVENT::iterator itNode = _mapSyncNMEvent.begin() ;
      while( itNode != _mapSyncNMEvent.end() )
      {
         clsEventItem *pItem = itNode->second ;
         ++itNode ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      MAP_CS_EVENT::iterator itCS = _mapSyncCSEvent.begin() ;
      while( itCS != _mapSyncCSEvent.end() )
      {
         clsCSEventItem *pItem = itCS->second ;
         ++itCS ;
         if ( handle == pItem->netHandle )
         {
            pItem->event.signalAll( SDB_NETWORK_CLOSE ) ;
         }
      }

      if ( handle == _upCatHandle )
      {
         _upCatEvent.signalAll( SDB_NETWORK_CLOSE ) ;
      }

      if ( handle == _remoteEndpointHandle )
      {
         rtnRemoteMessenger *
            messenger = pmdGetKRCB()->getRTNCB()->getRemoteMessenger() ;
         SDB_ASSERT( messenger, "Remote messenger is NULL" ) ;
         messenger->onDisconnect() ;
         _remoteEndpointHandle = NET_INVALID_HANDLE ;
      }

      return SDB_OK ;
   }

   // In one case we need to handle the auth request message for shard session,
   // that is when it's from search engine adapter. We do not really
   // authenticate, but to get the adapter service information from the message.
   // For any other auth request, just return OK.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONAUTHREQMSG, "_clsShardMgr::_onAuthReqMsg" )
   INT32 _clsShardMgr::_onAuthReqMsg( NET_HANDLE handle, MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__ONAUTHREQMSG ) ;
      BSONObj myInfoObj ;
      MsgAuthReply *reply = NULL ;
      INT32 replySize = 0 ;

      // If the adapter handler is valid, there is some valid connection between
      // this node and the adapter. In this case, no new connections with a
      // different handle is allowed.
      // If it's the current valid connection, it's the adapter to try to
      // register again. This will happen when the catalogue primary changed,
      // and the adapter wants to know the new primary node.
      if ( NET_INVALID_HANDLE != _remoteEndpointHandle &&
           handle != _remoteEndpointHandle )
      {
         // Another different adapter. Reject!
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Remote endpoint is ready now. Reject new "
                          "registration" ) ;
         goto error ;
      }

      rc = _genAuthReplyInfo( myInfoObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Generate auth reply information failed[%d]",
                   rc ) ;

      if ( NET_INVALID_HANDLE == _remoteEndpointHandle )
      {
         BSONObj bodyObj ;
         rc = extractAuthMsg( msg, bodyObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract auth message failed[%d]", rc ) ;
         rc = _updateRemoteEndpointInfo( handle, bodyObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Update remote endpoint information by "
                                   "registration information failed[%d]",
                      rc ) ;
      }

   done:
      replySize = sizeof( MsgAuthReply ) ;
      if ( SDB_OK == rc && !myInfoObj.isEmpty())
      {
         replySize += ossRoundUpToMultipleX( myInfoObj.objsize(), 4 ) ;
      }
      reply = ( MsgAuthReply * )SDB_OSS_MALLOC( replySize ) ;
      if ( !reply )
      {
         PD_LOG( PDERROR, "Allocate memory for reply message failed, size: %d",
                 replySize ) ;
         rc = SDB_OOM ;
      }
      else
      {
         reply->header.messageLength = replySize ;
         reply->header.opCode = MSG_AUTH_VERIFY_RES ;
         reply->header.TID = msg->TID ;
         reply->header.routeID.value = 0 ;
         reply->header.requestID = msg->requestID ;
         reply->flags = rc ;
         reply->startFrom = 0 ;
         if ( SDB_OK == rc && !myInfoObj.isEmpty() )
         {
            reply->numReturned = 1 ;
            ossMemcpy( (CHAR *)reply + sizeof( MsgAuthReply ),
                       myInfoObj.objdata(), myInfoObj.objsize() ) ;
         }
         else
         {
            reply->numReturned = 0 ;
         }

         rc = _sendToRemoteEndpoint( handle, (MsgHeader *)reply ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send message to search engine adapter "
                    "failed[ %d ]", rc ) ;
         }
      }

      if ( reply )
      {
         SDB_OSS_FREE( reply ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__ONAUTHREQMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsShardMgr::_onTransCheckReqMsg( NET_HANDLE handle, MsgHeader *msg )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      dpsTransCB *transCB = krcb->getTransCB() ;
      replCB *pReplCB = krcb->getClsCB()->getReplCB() ;
      BSONObj retObj ;
      MsgOpReply reply ;

      reply.header.opCode = MAKE_REPLY_TYPE( msg->opCode ) ;
      reply.header.messageLength = sizeof( MsgOpReply ) ;
      reply.header.requestID = msg->requestID ;
      reply.header.routeID.value = MSG_INVALID_ROUTEID ;
      reply.header.TID = msg->TID ;
      reply.contextID = -1 ;

      /// check primary
      if ( !pReplCB->primaryIsMe() )
      {
         reply.startFrom = pReplCB->getPrimary().columns.nodeID ;
         reply.flags = SDB_CLS_NOT_PRIMARY ;

         retObj = utilGetErrorBson( reply.flags, NULL, NULL ) ;
      }
      else
      {
         INT32 checkRC = SDB_OK ;
         DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET ;

         MsgClsTransCheckReq *pReq = ( MsgClsTransCheckReq* )msg ;
         INT32 status = transCB->checkTransStatus( pReq->transID, lsn ) ;

         // for wait-commit status, we need to make sure pre-commit log
         // is replicated to at least one other replicate node ( group with
         // multiple nodes )
         if ( DPS_TRANS_WAIT_COMMIT == status &&
              DPS_INVALID_LSN_OFFSET != lsn &&
              pReplCB->groupSize() > 1 )
         {
            // just wait for one replica node in this special case
            checkRC = pReplCB->sync( lsn, pmdGetThreadEDUCB(), 2, 10 ) ;
            if ( SDB_OK != checkRC )
            {
               PD_LOG( PDWARNING, "Failed to check sync for transaction "
                       "[%llu] lsn [%llu], rc: %d", pReq->transID, lsn, checkRC ) ;
               checkRC = SDB_CLS_WAIT_SYNC_FAILED ;
            }
            reply.flags = checkRC ;
         }
         else if ( DPS_TRANS_DOING == status )
         {
            // the status is still doing, it might be processing
            // pre-commit/rollback messages
            // tell the requester to wait
            reply.flags = SDB_RTN_EXIST_INDOUBT_TRANS ;
         }
         else
         {
            reply.flags = SDB_OK ;
         }

         retObj = BSON( FIELD_NAME_TRANSACTION_ID << (INT64)pReq->transID <<
                        FIELD_NAME_STATUS << status ) ;
      }

      reply.header.messageLength += retObj.objsize() ;
      reply.numReturned = 1 ;

      /// send reply
      rc = _pNetRtAgent->syncSend( handle, (MsgHeader*)&reply,
                                   (void*)retObj.objdata(),
                                   retObj.objsize() ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Send reply failed, rc: %d", rc ) ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG, "_clsShardMgr::_onTextIdxInfoReqMsg" )
   INT32 _clsShardMgr::_onTextIdxInfoReqMsg ( NET_HANDLE handle,
                                              MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG ) ;

      INT64 peerVersion = -1 ;
      INT64 localVersion = -1 ;

      SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

      BSONObj requestObject ;

      try
      {
         CHAR * requestBody = (CHAR *)msg + sizeof( MsgHeader ) ;
         requestObject = BSONObj( requestBody ) ;

         BSONElement verEle = requestObject.getField( FIELD_NAME_VERSION ) ;
         PD_CHECK( !verEle.eoo(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse request message, text index version dose "
                   "not exist" ) ;
         PD_CHECK( NumberLong == verEle.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse request message, text index version type "
                   "is wrong" ) ;
         peerVersion = verEle.numberLong() ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Failed to parse request message, unexpected err: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // If the versions are not identical, dump the text indices information,
      // and send back to search engine adapter, together with the new version.
      // As after every restart, the version is set to -1 both on engine and
      // adapter, but actually there may have some text indices. So we increase
      // the local version by 1, to force dump the index information.
      localVersion = rtnCB->getTextIdxVersion() ;
      if ( RTN_INIT_TEXT_INDEX_VERSION == localVersion )
      {
         if ( FALSE == rtnCB->updateTextIdxVersion( RTN_INIT_TEXT_INDEX_VERSION,
                                                    peerVersion + 1 ) )
         {
            PD_LOG( PDWARNING, "Failed to initialize text index version, "
                    "some one else has updated" ) ;
         }
         localVersion = rtnCB->getTextIdxVersion() ;
         PD_CHECK( RTN_INIT_TEXT_INDEX_VERSION != localVersion,
                   SDB_SYS, error, PDERROR, "Failed to initialize text "
                   "index version" ) ;
      }

      PD_LOG( PDINFO, "Requesting text index information: peer version: %lld "
              "local version: %lld]", peerVersion, localVersion ) ;

      if ( localVersion == peerVersion )
      {
         // same version, no need to dump text index information
         BSONObjBuilder replyBuilder ;
         BSONObj replyObject ;

         try
         {
            replyBuilder.appendBool( FIELD_NAME_IS_PRIMARY, pmdIsPrimary() ) ;
            replyBuilder.append( FIELD_NAME_VERSION, localVersion ) ;
            replyObject = replyBuilder.done() ;
         }
         catch ( std::exception & e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to build reply message, exception: %s",
                    e.what() ) ;
            goto error ;
         }

         rc = replyToRemoteEndpoint( handle, msg, replyObject ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to send reply message to "
                      "handle: %u groupID: %u nodeID: %u serviceID: %u",
                      handle, msg->routeID.columns.groupID,
                      msg->routeID.columns.nodeID,
                      msg->routeID.columns.serviceID ) ;
      }
      else
      {
         // need dump text index information, push to background job to
         // avoid blocking clsMgr main thread
         rc = clsStartAdapterDumpTextIndexJob( msg, requestObject, peerVersion,
                                               localVersion, handle ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start adapter job to dump text "
                      "index, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__ONTEXTIDXINFOREQMSG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__SENDTOREMOTEENDPOINT, "_clsShardMgr::_sendToRemoteEndpoint" )
   INT32 _clsShardMgr::_sendToRemoteEndpoint ( NET_HANDLE handle,
                                               MsgHeader * msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__SENDTOREMOTEENDPOINT ) ;
      BOOLEAN hasLock = FALSE ;

      _shardLatch.get_shared() ;
      hasLock = TRUE ;

      rc = _pNetRtAgent->syncSend( handle, (void *)msg ) ;
      PD_RC_CHECK( rc, PDERROR, "Send message to search engine adapter "
                   "failed[ %d ]", rc ) ;

   done:
      if ( hasLock )
      {
         _shardLatch.release_shared() ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__SENDTOREMOTEENDPOINT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR_REPLYTOREMOTEENDPOINT, "_clsShardMgr::replyToRemoteEndpoint" )
   INT32 _clsShardMgr::replyToRemoteEndpoint ( NET_HANDLE handle,
                                               MsgHeader * request,
                                               const BSONObj & replyObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CLSSHDMGR_REPLYTOREMOTEENDPOINT ) ;

      SDB_ASSERT( NULL != request, "request message is invaid" ) ;

      MsgOpReply * replyMessage = NULL ;
      INT32 replySize = 0 ;

      replySize = sizeof( MsgOpReply ) ;
      if ( !replyObject.isEmpty() )
      {
         replySize += replyObject.objsize() ;
      }
      replyMessage = (MsgOpReply *)SDB_THREAD_ALLOC( replySize ) ;
      PD_CHECK( NULL != replyMessage, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for reply message, size: %d",
                replySize ) ;

      replyMessage->header.messageLength = replySize ;
      replyMessage->header.opCode = MAKE_REPLY_TYPE( request->opCode ) ;
      replyMessage->header.TID = request->TID ;
      replyMessage->header.routeID.value = 0 ;
      replyMessage->header.requestID = request->requestID ;
      replyMessage->flags = SDB_OK ;
      replyMessage->startFrom = 0 ;
      replyMessage->numReturned = rc ? -1 : 1 ;
      if ( SDB_OK == rc )
      {
         ossMemcpy( (CHAR *)replyMessage + sizeof( MsgOpReply ),
                    replyObject.objdata(), replyObject.objsize() ) ;
      }

      rc = _sendToRemoteEndpoint( handle, (MsgHeader *)replyMessage ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to send message to "
                   "handle: %u groupID: %u nodeID: %u serviceID: %u",
                   handle, request->routeID.columns.groupID,
                   request->routeID.columns.nodeID,
                   request->routeID.columns.serviceID ) ;
      PD_LOG( PDDEBUG, "Send message to handle: %u groupID: %u "
              "nodeID: %u serviceID: %u", handle,
              request->routeID.columns.groupID,
              request->routeID.columns.nodeID,
              request->routeID.columns.serviceID ) ;

   done :
      if ( NULL != replyMessage )
      {
         SDB_THREAD_FREE( replyMessage ) ;
      }
      PD_TRACE_EXITRC( SDB__CLSSHDMGR_REPLYTOREMOTEENDPOINT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__UPDATEREMOTEENDPOINTINFO, "_clsShardMgr::_updateRemoteEndpointInfo" )
   INT32 _clsShardMgr::_updateRemoteEndpointInfo( NET_HANDLE handle,
                                                  const BSONObj &regInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__UPDATEREMOTEENDPOINTINFO ) ;
      MsgRouteID remoteID ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnRemoteMessenger *messenger = NULL ;

      // The remote messenger will be initialized and activated when the adapter
      // registers for the first time.
      messenger = rtnCB->getRemoteMessenger() ;
      if ( !messenger )
      {
         rc = rtnCB->prepareRemoteMessenger() ;
         PD_RC_CHECK( rc, PDERROR, "Prepare remote messenger failed[%d]", rc ) ;
         messenger = rtnCB->getRemoteMessenger() ;
      }

      try
      {
         // Add the route of search engine adapter into rtnCB's net route agent.
         const CHAR *peerHost = regInfo.getStringField( FIELD_NAME_HOST ) ;
         const CHAR *peerSvc =
               regInfo.getStringField( FIELD_NAME_SERVICE_NAME ) ;

         BSONElement ele = regInfo.getField( FIELD_NAME_GROUPID ) ;
         if ( !ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Group id field type error, type[%d]",
                    ele.type() ) ;
            goto error ;
         }
         remoteID.columns.groupID = ele.numberLong() ;

         ele = regInfo.getField( FIELD_NAME_NODEID ) ;
         if ( NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Node id field type error, type[%d]",
                    ele.type() ) ;
            goto error ;
         }
         remoteID.columns.nodeID = ele.numberInt() ;

         ele = regInfo.getField( FIELD_NAME_SERVICE_TYPE ) ;
         if ( NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Service type field type error, type[%d]",
                    ele.type() ) ;
            goto error ;
         }
         remoteID.columns.serviceID = ele.numberInt() ;

         if ( MSG_INVALID_ROUTEID == remoteID.value )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Route id is invalid" ) ;
            goto error ;
         }

         // Update both the route agent of rtn and shard.
         rc = messenger->setTarget( remoteID, peerHost, peerSvc ) ;
         PD_RC_CHECK( rc, PDERROR, "Add remote target failed[%d]", rc ) ;

         // Set the local id of route agent in rtn.
         rc = messenger->setLocalID( _nodeID ) ;
         PD_RC_CHECK( rc, PDERROR, "Set local id failed[%d]", rc ) ;

         rc = _pNetRtAgent->updateRoute( remoteID, peerHost, peerSvc ) ;
         if ( rc && SDB_NET_UPDATE_EXISTING_NODE != rc )
         {
            PD_LOG( PDERROR, "Update route failed[%d], host[%s], "
                             "service[%s]", rc, peerHost, peerSvc ) ;
            goto error ;
         }
         _remoteEndpointHandle = handle ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__UPDATEREMOTEENDPOINTINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSHDMGR__GENAUTHREPLYINFO, "_clsShardMgr::_genAuthReplyInfo" )
   INT32 _clsShardMgr::_genAuthReplyInfo( BSONObj &replyInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSHDMGR__GENAUTHREPLYINFO ) ;
      BSONObjBuilder builder ;
      CHAR groupName[ OSS_MAX_GROUPNAME_SIZE + 1 ] = { 0 } ;

      pmdGetKRCB()->getGroupName( groupName, OSS_MAX_GROUPNAME_SIZE + 1 ) ;
      if ( 0 == ossStrlen( groupName ) )
      {
         rc = SDB_REPL_GROUP_NOT_ACTIVE ;
         goto error ;
      }

      try
      {
         // Need to reply with following information:
         // Whether master node?
         // Group name( used as type of index in ES )
         // Catalog information( Currently only send the catalog primary node
         // information ). The search engine adapter needs the catalog
         // information to get collection version when query.
         builder.appendBool( FIELD_NAME_IS_PRIMARY, pmdIsPrimary() ) ;
         builder.append( FIELD_NAME_GROUPNAME, groupName ) ;

         if ( _cataGrpItem.nodeCount() > 0 )
         {
            BSONObj cataInfo = _buildCataGroupInfo() ;
            builder.append( FIELD_NAME_CATALOGINFO, cataInfo ) ;
         }

         replyInfo = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSSHDMGR__GENAUTHREPLYINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj _clsShardMgr::_buildCataGroupInfo()
   {
      BSONObj obj ;

      if ( _cataGrpItem.nodeCount() > 0 )
      {
         clsNodeItem *primary =
               _cataGrpItem.nodeItemByPos( _cataGrpItem.getPrimaryPos() ) ;
         try
         {
            BSONObjBuilder builder( 1024 ) ;
            builder.append( CAT_GROUPID_NAME, CATALOG_GROUPID ) ;
            builder.append( CAT_GROUPNAME_NAME, CATALOG_GROUPNAME ) ;
            builder.append(CAT_ROLE_NAME, SDB_ROLE_CATALOG ) ;
            builder.append( FIELD_NAME_SECRETID, (INT32) 0 ) ;
            builder.append( CAT_VERSION_NAME, (INT32)1 ) ;
            if ( primary )
            {
               builder.append( CAT_PRIMARY_NAME,
                               (INT32)primary->_id.columns.nodeID ) ;
            }

            BSONArrayBuilder arrGroup( builder.subarrayStart( CAT_GROUP_NAME ) ) ;

            for ( UINT32 i = 0; i < _cataGrpItem.nodeCount(); ++i )
            {
               BSONObjBuilder node( arrGroup.subobjStart() ) ;
               clsNodeItem *nodeItem = _cataGrpItem.nodeItemByPos( i ) ;
               node.append( CAT_NODEID_NAME,
                            (INT32)nodeItem->_id.columns.nodeID ) ;
               node.append( CAT_HOST_FIELD_NAME, nodeItem->_host ) ;

               INT32 status = 0 ;
               _cataGrpItem.getNodeInfo( i, status ) ;
               node.append( CAT_STATUS_NAME, (INT32)status ) ;
               // Service:[{},{}...]
               BSONArrayBuilder arrSvc( node.subarrayStart(
                                         CAT_SERVICE_FIELD_NAME ) ) ;
               BSONObjBuilder svc( arrSvc.subobjStart() ) ;
               svc.append( CAT_SERVICE_TYPE_FIELD_NAME,
                           (INT32)MSG_ROUTE_CAT_SERVICE ) ;
               svc.append( CAT_SERVICE_NAME_FIELD_NAME,
                           nodeItem->_service[ MSG_ROUTE_CAT_SERVICE ] ) ;
               svc.done() ;
               arrSvc.done() ;
               node.done() ;
            }
            arrGroup.done() ;
            obj = builder.obj() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         }
      }

      return obj ;
   }

   INT64 _clsShardMgr::netIn()
   {
      return _pNetRtAgent->netIn() ;
   }

   INT64 _clsShardMgr::netOut()
   {
      return _pNetRtAgent->netOut() ;
   }

}

