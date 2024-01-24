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

   Source File Name = coordRemoteSession.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordRemoteSession.hpp"
#include "msgMessageFormat.hpp"
#include "coordCommon.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dpsUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtnRemoteMessenger.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordSessionPropSite implement
   */
   _coordSessionPropSite::_coordSessionPropSite ()
   : _rtnSessionProperty(),
     _mapLastNodes(),
     _pEDUCB( NULL ),
     _pSite( NULL ),
     _writeTransNodeNum( 0 )
   {
   }

   _coordSessionPropSite::~_coordSessionPropSite()
   {
      SDB_ASSERT( NULL == _pEDUCB, "EDU must be NULL" ) ;
   }

   void _coordSessionPropSite::clear()
   {
      _mapLastNodes.clear() ;
   }

   INT32 _coordSessionPropSite::addLastNode( const MsgRouteID &nodeID,
                                             BOOLEAN primaryRequest )
   {
      INT32 rc = SDB_OK ;

      try
      {
         MAP_GROUP_2_NODE_IT iter =
                              _mapLastNodes.find( nodeID.columns.groupID ) ;
         if ( iter != _mapLastNodes.end() && !primaryRequest )
         {
            // found group
            coordLastNodeStatus &lastNodeStatus = iter->second ;
            if ( lastNodeStatus._nodeID.value != nodeID.value )
            {
               // not the same node, need update node ID
               lastNodeStatus._nodeID.value = nodeID.value ;
               // new node is used, save current tick, then make it
               // timeout after a preferred period
               lastNodeStatus._addTick = pmdGetDBTick() ;
            }
         }
         else
         {
            // if not found or primary required (write request),
            // add last node directly
            coordLastNodeStatus lastNodeStatus ;
            // set node ID
            lastNodeStatus._nodeID.value = nodeID.value ;
            // new node is used, save current tick, then make it
            // timeout after a preferred period
            lastNodeStatus._addTick = pmdGetDBTick() ;
            // save to the last node map
            _mapLastNodes[ nodeID.columns.groupID ] = lastNodeStatus ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save the last node for group [%u] "
                 "with node [%u], error: %s", nodeID.columns.groupID,
                 nodeID.columns.nodeID, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   UINT64 _coordSessionPropSite::getLastNode( UINT32 groupID )
   {
      MsgRouteID nodeID ;
      MAP_GROUP_2_NODE_IT iter = _mapLastNodes.find( groupID ) ;

      nodeID.value = MSG_INVALID_ROUTEID ;

      if ( iter != _mapLastNodes.end() )
      {
         nodeID.value = iter->second._nodeID.value ;
         // check if timeout against preferred period
         // - if period is zero, always timeout
         // - if added before a period (in second), make it timeout
         // NOTE: if period is -1, always not timeout
         if ( ( _instanceOption.getPreferedPeriod() == 0 ) ||
              ( _instanceOption.getPreferedPeriod() > 0 &&
                pmdGetTickSpanTime( iter->second._addTick ) >
                      ( (UINT64)( _instanceOption.getPreferedPeriod() ) *
                        OSS_ONE_SEC ) ) )
         {
            nodeID.value = MSG_INVALID_ROUTEID ;
            // remote node now
            _mapLastNodes.erase( iter ) ;
         }
      }

      return nodeID.value ;
   }

   BOOLEAN _coordSessionPropSite::existNode( UINT32 groupID ) const
   {
      MAP_GROUP_2_NODE::const_iterator cit = _mapLastNodes.find( groupID ) ;
      if ( cit != _mapLastNodes.end() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _coordSessionPropSite::delLastNode( UINT32 groupID )
   {
      _mapLastNodes.erase( groupID ) ;
   }

   void _coordSessionPropSite::delLastNode( const MsgRouteID &nodeID )
   {
      MAP_GROUP_2_NODE_IT it = _mapLastNodes.find( nodeID.columns.groupID ) ;
      if ( it != _mapLastNodes.end() &&
           it->second._nodeID.value == nodeID.value )
      {
         _mapLastNodes.erase( it ) ;
      }
   }

   void _coordSessionPropSite::setEduCB( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
   }

   void _coordSessionPropSite::setSite( pmdRemoteSessionSite *pSite )
   {
      _pSite = pSite ;
   }

   void _coordSessionPropSite::_onSetInstance ()
   {
      clear() ;
   }

   void _coordSessionPropSite::_toBson( BSONObjBuilder &builder ) const
   {
      _pEDUCB->getTransExecutor()->toBson( builder ) ;

      if ( _pEDUCB->getSource() )
      {
         try
         {
            builder.append( FIELD_NAME_SOURCE, _pEDUCB->getSource() ) ;
         }
         catch( std::exception &e )
         {
            /// ignore
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         }
      }
   }

   INT32 _coordSessionPropSite::_checkTransConf( const _dpsTransConfItem *pTransConf )
   {
      INT32 rc = SDB_OK ;
      UINT32 mask = pTransConf->getTransConfMask() ;

      /// When in transaction, can only update transtimeout
      if ( _pEDUCB->isTransaction() )
      {
         OSS_BIT_CLEAR( mask, TRANS_CONF_MASK_TIMEOUT ) ;

         if ( 0 != mask )
         {
            PD_LOG_MSG( PDERROR, "In transaction can only update %s",
                        FIELD_NAME_TRANS_TIMEOUT ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else if ( !sdbGetTransCB()->isTransOn() &&
                OSS_BIT_TEST( mask, TRANS_CONF_MASK_AUTOCOMMIT ) &&
                pTransConf->isTransAutoCommit() )
      {
         rc = SDB_DPS_TRANS_DIABLED ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordSessionPropSite::_updateTransConf( const _dpsTransConfItem *pTransConf )
   {
      _pEDUCB->updateTransConfByMask( *pTransConf ) ;
   }

   void _coordSessionPropSite::_updateSource( const CHAR *pSource )
   {
      _pEDUCB->setSource( pSource ) ;
   }

   BOOLEAN _coordSessionPropSite::isTransNode( const MsgRouteID &routeID ) const
   {
      BOOLEAN found = FALSE ;
      MAP_TRANS_NODES_CIT cit ;

      cit = _mapTransNodes.find( routeID.columns.groupID ) ;
      if ( cit != _mapTransNodes.end() &&
           cit->second._nodeID.value == routeID.value )
      {
         found = TRUE ;
      }

      return found ;
   }

   BOOLEAN _coordSessionPropSite::getTransNodeRouteID( UINT32 groupID,
                                                       MsgRouteID &routeID ) const
   {
      BOOLEAN found = FALSE ;
      MAP_TRANS_NODES_CIT cit ;
      routeID.value = 0 ;

      cit = _mapTransNodes.find( groupID ) ;
      if ( cit != _mapTransNodes.end() )
      {
         routeID = cit->second._nodeID ;
         found = TRUE ;
      }

      return found ;
   }

   BOOLEAN _coordSessionPropSite::hasTransNode( UINT32 groupID ) const
   {
      if ( _mapTransNodes.find( groupID ) != _mapTransNodes.end() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordSessionPropSite::isTransNodeEmpty() const
   {
      return _mapTransNodes.empty() ;
   }

   BOOLEAN _coordSessionPropSite::checkAndUpdateNode( const MsgRouteID &routeID,
                                                      BOOLEAN doWrite )
   {
      MAP_TRANS_NODES_IT it = _mapTransNodes.find( routeID.columns.groupID ) ;
      if ( it != _mapTransNodes.end() )
      {
         SDB_ASSERT( it->second._nodeID.value == routeID.value,
                     "NodeID should be the same" ) ;
         if ( doWrite && !it->second._hasWritten &&
              it->second._nodeID.value == routeID.value )
         {
            it->second._hasWritten = doWrite ;
            ++_writeTransNodeNum ;
         }
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordSessionPropSite::delTransNode( const MsgRouteID &routeID )
   {
      MAP_TRANS_NODES_IT it = _mapTransNodes.find( routeID.columns.groupID ) ;
      if ( it != _mapTransNodes.end() &&
           it->second._nodeID.value == routeID.value )
      {
         if ( it->second._hasWritten )
         {
            --_writeTransNodeNum ;
         }
         _mapTransNodes.erase( it ) ;
         return TRUE ;
      }
      return FALSE ;
   }

   void _coordSessionPropSite::addTransNode( const MsgRouteID &routeID,
                                             BOOLEAN isWrite )
   {
      coordTransNodeStatus &node = _mapTransNodes[ routeID.columns.groupID ] ;

      /// already exist
      if ( MSG_INVALID_ROUTEID != node._nodeID.value && node._hasWritten )
      {
         --_writeTransNodeNum ;
      }

      node._nodeID.value = routeID.value ;
      node._hasWritten = isWrite ;
      if ( isWrite )
      {
         ++_writeTransNodeNum ;
      }
   }

   UINT32 _coordSessionPropSite::getTransNodeSize() const
   {
      return _mapTransNodes.size() ;
   }

   UINT32 _coordSessionPropSite::getWriteTransNodeSize() const
   {
      return _writeTransNodeNum ;
   }

   UINT32 _coordSessionPropSite::dumpTransNode( SET_ROUTEID &setID ) const
   {
      MAP_TRANS_NODES_CIT cit = _mapTransNodes.begin() ;
      while( cit != _mapTransNodes.end() )
      {
         setID.insert( cit->second._nodeID.value ) ;
         ++cit ;
      }
      return _mapTransNodes.size() ;
   }

   const _coordSessionPropSite::MAP_TRANS_NODES*
      _coordSessionPropSite::getTransNodeMap() const
   {
      return &_mapTransNodes ;
   }

   INT32 _coordSessionPropSite::beginTrans( _pmdEDUCB *cb,
                                            BOOLEAN isAutoCommit )
   {
      INT32 rc = SDB_OK ;

      if ( !cb->isTransaction() )
      {
         SDB_ASSERT( _mapTransNodes.empty(), "Trans node is not empty" ) ;

         dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
         DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;

         if ( !pTransCB->isTransOn() )
         {
            rc = SDB_DPS_TRANS_DIABLED ;
         }
         else
         {
            /// alloc trans id
            transID = pTransCB->allocTransID( isAutoCommit ) ;
            /// clear first op
            DPS_TRANS_CLEAR_FIRSTOP( transID ) ;

            /// set trans id
            cb->setTransID( transID ) ;

            _mapTransNodes.clear() ;
            _writeTransNodeNum = 0 ;

            PD_LOG( PDINFO, "Begin transaction(ID:%s, IDAttr:%s)",
                    dpsTransIDToString( transID ).c_str(),
                    dpsTransIDAttrToString( transID ).c_str() ) ;
         }
      }

      return rc ;
   }

   void _coordSessionPropSite::endTrans( _pmdEDUCB *cb )
   {
      cb->setTransID( DPS_INVALID_TRANS_ID ) ;
      _mapTransNodes.clear() ;
      _writeTransNodeNum = 0 ;
   }

   /*
      _coordSessionPropMgr implement
   */
   _coordSessionPropMgr::_coordSessionPropMgr()
   : _mapProps()
   {
      _useOwnQueue = FALSE ;
   }

   _coordSessionPropMgr::_coordSessionPropMgr( BOOLEAN useOwnQueen )
   : _mapProps()
   {
      _useOwnQueue = useOwnQueen ;
   }

   _coordSessionPropMgr::~_coordSessionPropMgr()
   {
      SDB_ASSERT( 0 == _mapProps.size(), "Props must be empty" ) ;
   }

   void _coordSessionPropMgr::onRegister( _pmdRemoteSessionSite *pSite,
                                          _pmdEDUCB *cb )
   {
      coordSessionPropSite &propSite = _mapProps[ cb->getTID() ] ;
      propSite.setInstanceOption( _instanceOption ) ;
      propSite.setOperationTimeout( _operationTimeout ) ;
      propSite.setEduCB( cb ) ;
      propSite.setSite( pSite ) ;
      pSite->setUserData( (UINT64)&propSite ) ;

      if ( _useOwnQueue )
      {
         _rtnRemoteSiteHandle *handler = SDB_OSS_NEW _rtnRemoteSiteHandle() ;
         if ( NULL != handler )
         {
            pSite->setHandle( handler ) ;
         }
      }
   }

   void _coordSessionPropMgr::onUnreg( _pmdRemoteSessionSite *pSite,
                                       _pmdEDUCB *cb )
   {
      coordSessionPropSite *pPropSite = NULL ;
      pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;
      if ( pPropSite )
      {
         pPropSite->setEduCB( NULL ) ;
         pPropSite->setSite( NULL ) ;
         pSite->setUserData( 0 ) ;
      }
      _mapProps.erase( cb->getTID() ) ;
      if ( _useOwnQueue )
      {
         rtnRemoteSiteHandle *handler =
                                    (rtnRemoteSiteHandle *)pSite->getHandle() ;
         SAFE_OSS_DELETE( handler ) ;
         pSite->setHandle( NULL ) ;
      }
   }

   coordSessionPropSite* _coordSessionPropMgr::getSite( _pmdEDUCB *cb )
   {
      _pmdRemoteSessionSite *pSite = NULL ;
      coordSessionPropSite *propSite = NULL ;

      pSite = ( _pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( pSite )
      {
         propSite = ( coordSessionPropSite* )pSite->getUserData() ;
      }

      return propSite ;
   }

   #define COORD_GROUP_SEL_INVALID        ( -1 )
   #define COORD_GROUP_SEL_NONE           ( -2 )

   /*
      _coordGroupSel implement
   */
   _coordGroupSel::_coordGroupSel()
   {
      _pResource     = NULL ;
      _pPropSite     = NULL ;
      _primary       = FALSE ;
      _svcType       = MSG_ROUTE_SHARD_SERVCIE ;
      _hasUpdate     = FALSE ;
      _pos           = COORD_GROUP_SEL_INVALID ;
      _selTimes      = 0 ;
      _ignoredNum    = 0 ;
      _lastNodeID.value = MSG_INVALID_ROUTEID ;
   }

   _coordGroupSel::~_coordGroupSel()
   {
   }

   void _coordGroupSel::init( coordResource *pResource,
                              coordSessionPropSite *pPropSite,
                              BOOLEAN primary,
                              MSG_ROUTE_SERVICE_TYPE svcType )
   {
      _pResource        = pResource ;
      _pPropSite        = pPropSite ;
      _primary          = primary ;
      _svcType          = svcType ;
   }

   void _coordGroupSel::setPrimary( BOOLEAN primary )
   {
      _primary          = primary ;
   }

   BOOLEAN _coordGroupSel::isPrimary() const
   {
      return _primary ;
   }

   BOOLEAN _coordGroupSel::isRequiredPrimary() const
   {
      if ( _pPropSite && _pPropSite->isMasterRequired() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordGroupSel::isRequiredSecondary() const
   {
      if ( _pPropSite && _pPropSite->isSlaveRequired() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordGroupSel::isPreferredPrimary() const
   {
      if ( _pPropSite && _pPropSite->isMasterPreferred() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordGroupSel::isPreferredSecondary() const
   {
      if ( _pPropSite && _pPropSite->isSlavePreferred() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordGroupSel::existLastNode( UINT32 groupID ) const
   {
      if ( _pPropSite && _pPropSite->existNode( groupID ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _coordGroupSel::setServiceType( MSG_ROUTE_SERVICE_TYPE svcType )
   {
      _svcType          = svcType ;
   }

   void _coordGroupSel::_resetStatus()
   {
      _pos = COORD_GROUP_SEL_INVALID ;
      _selTimes = 0 ;
      _ignoredNum = 0 ;
      _hasUpdate = FALSE ;
      _lastNodeID.value = MSG_INVALID_ROUTEID ;
      _selectedPositions.clear() ;
   }

   void _coordGroupSel::addGroupPtr2Map( CoordGroupInfoPtr &groupPtr )
   {
      _mapGroupPtr[ groupPtr->groupID() ] = groupPtr ;
   }

   BOOLEAN _coordGroupSel::getGroupPtrFromMap( UINT32 groupID,
                                               CoordGroupInfoPtr &groupPtr )
   {
      BOOLEAN bGet = FALSE ;
      CoordGroupMap::iterator it = _mapGroupPtr.find( groupID ) ;
      if ( it != _mapGroupPtr.end() )
      {
         groupPtr = it->second ;
         bGet = TRUE ;
      }
      return bGet ;
   }

   INT32 _coordGroupSel::selBegin( UINT32 groupID, MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;

      _resetStatus() ;

      if ( !getGroupPtrFromMap( groupID, _groupPtr ) )
      {
         rc = _pResource->getGroupInfo( groupID, _groupPtr ) ;
         if ( rc && !_hasUpdate )
         {
            _hasUpdate = TRUE ;
            rc = _pResource->updateGroupInfo( groupID, _groupPtr,
                                              _pPropSite->getEDUCB() ) ;
         }
         if ( rc )
         {
            if ( SDB_CAT_NO_ADDR_LIST != rc )
            {
               PD_LOG( PDERROR, "Get or update group[%u] info failed, rc: %d",
                       groupID, rc ) ;
            }
            goto error ;
         }
         addGroupPtr2Map( _groupPtr ) ;
      }

      /// In transaction, need to use the trans node
      _pPropSite->getTransNodeRouteID( groupID, nodeID ) ;
      if ( MSG_INVALID_ROUTEID != nodeID.value &&
           _svcType == nodeID.columns.serviceID )
      {
         PD_LOG( PDDEBUG, "Select node: %s", routeID2String( nodeID ).c_str() ) ;
         goto done ;
      }
      else if ( _primary && !SDB_IS_DSID( groupID ) )
      {
         rc = _selPrimaryBegin( nodeID ) ;
      }
      else
      {
         rc = _selOtherBegin( nodeID ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSel::_selPrimaryBegin( MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;

      nodeID = _groupPtr->primary( _svcType ) ;
      if ( MSG_INVALID_ROUTEID == nodeID.value && !_hasUpdate )
      {
         _hasUpdate = TRUE ;
         UINT32 groupID = _groupPtr->groupID() ;
         rc = _pResource->updateGroupInfo( groupID, _groupPtr,
                                           _pPropSite->getEDUCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update group[%u] info failed, rc: %d",
                    groupID, rc ) ;
            goto error ;
         }
         nodeID = _groupPtr->primary( _svcType ) ;
      }

      if ( MSG_INVALID_ROUTEID == nodeID.value )
      {
         /// when no primary node, select any one
         rc = _selOtherBegin( nodeID ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         _lastNodeID.value = nodeID.value ;
         _pos = COORD_GROUP_SEL_NONE ;
         PD_LOG( PDDEBUG, "Select node: %s", routeID2String( nodeID ).c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSel::_selOtherBegin( MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;
      UINT32 groupID = _groupPtr->groupID() ;
      clsGroupItem *groupItem = NULL ;
      UINT32 nodeNum = 0 ;

      nodeID.value = _pPropSite->getLastNode( groupID ) ;
      // last node is valid and in group info and meets the constraints
      if ( MSG_INVALID_ROUTEID != nodeID.value &&
           COORD_GROUP_SEL_INVALID == _pos )
      {
         if ( _groupPtr->nodePos( nodeID.columns.nodeID ) >= 0 &&
              _meetPreferConstraint( nodeID ) &&
              _svcType == nodeID.columns.serviceID )
         {
            _pos = COORD_GROUP_SEL_NONE ;
            PD_LOG( PDDEBUG, "Select node: %s",
                    routeID2String( nodeID ).c_str() ) ;
            goto done ;
         }
         else
         {
            _pPropSite->delLastNode( groupID ) ;
         }
      }

      groupItem = _groupPtr.get() ;
      nodeNum = groupItem->nodeCount() ;
      if ( nodeNum <= 0 && !_hasUpdate )
      {
         _hasUpdate = TRUE ;
         rc = _pResource->updateGroupInfo( groupID, _groupPtr,
                                           _pPropSite->getEDUCB() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update group[%u] info failed, rc: %d",
                    groupID, rc ) ;
            goto error ;
         }
         groupItem = _groupPtr.get() ;
         nodeNum = groupItem->nodeCount() ;
      }
      if ( nodeNum <= 0 )
      {
         rc = SDB_CLS_EMPTY_GROUP ;
         PD_LOG( PDERROR, "Group[%u] is empty", groupID ) ;
         goto error ;
      }

      rc = _calcBeginPos( groupItem, _pPropSite->getInstanceOption(),
                          ossRand(), _pos ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate begin position for "
                   "group [%u], rc: %d", groupID, rc ) ;
      if ( _pos < 0 )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
         PD_LOG_MSG( PDERROR, "No preferred instance found in group [%s]",
                     _groupPtr->groupName().c_str() ) ;
         goto error ;
      }
      rc = _nextPos( _groupPtr, _pPropSite->getInstanceOption(),
                     _selTimes, _pos, nodeID ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         _lastNodeID.value = nodeID.value ;
         PD_LOG( PDDEBUG, "Select node: %s", routeID2String( nodeID ).c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordGroupSel::_meetPreferConstraint( const MsgRouteID &nodeID ) const
   {
      BOOLEAN met = FALSE ;
      PMD_PREFER_CONSTRAINT constraint =
            _pPropSite->getInstanceOption().getPreferredConstraint() ;
      UINT16 primaryID = _groupPtr->primary().columns.nodeID ;

      switch ( constraint )
      {
         case PMD_PREFER_CONSTRAINT_PRY_ONLY:
            met = ( nodeID.columns.nodeID == primaryID ) ? TRUE : FALSE ;
            break ;
         case PMD_PREFER_CONSTRAINT_SND_ONLY:
            met = ( nodeID.columns.nodeID != primaryID ) ? TRUE : FALSE ;
            break ;
         case PMD_PREFER_CONSTRAINT_NONE:
         default:
            met = TRUE ;
            break ;
      }

      return met ;
   }

   INT32 _coordGroupSel::selNext( MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;

      if ( COORD_GROUP_SEL_INVALID == _pos )
      {
         rc = SDB_CLS_NODE_BSFAULT ;
         goto error ;
      }

      if ( COORD_GROUP_SEL_NONE == _pos )
      {
         rc = _selOtherBegin( nodeID ) ;
      }
      else
      {
         rc = _nextPos( _groupPtr, _pPropSite->getInstanceOption(),
                        _selTimes, _pos, nodeID ) ;
      }
      if ( rc )
      {
         goto error ;
      }
      else
      {
         _lastNodeID.value = nodeID.value ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSel::_calcBeginPos( clsGroupItem *pGroupItem,
                                        const rtnInstanceOption & instanceOption,
                                        UINT32 random,
                                        INT32 &pos )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN selected = FALSE ;
      UINT32 primaryPos = pGroupItem->getPrimaryPos() ;

      pos = 0 ;

      if ( !SDB_IS_DSID( pGroupItem->groupID() ) &&
           instanceOption.hasCommonInstance() )
      {
         const VEC_NODE_INFO * nodes = pGroupItem->getNodes() ;
         SDB_ASSERT( NULL != nodes, "node list is invalid" ) ;
         rc = _selectPositions( *nodes, primaryPos, instanceOption,
                                _selectedPositions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to select position for group [%u], "
                      "rc: %d", pGroupItem->groupID(), rc ) ;

         if ( !_selectedPositions.empty() )
         {
            pos = _selectedPositions.front() ;
            _selectedPositions.pop_front() ;
            selected = TRUE ;
         }
         else if ( instanceOption.isPreferredStrict() )
         {
            pos = COORD_GROUP_SEL_INVALID ;
            selected = TRUE ;
         }
      }
      else if ( SDB_IS_DSID( pGroupItem->groupID() ) ||
                instanceOption.isSlavePreferred() )
      {
         const VEC_NODE_INFO * nodes = pGroupItem->getNodes() ;
         SDB_ASSERT( NULL != nodes, "node list is invalid" ) ;
         rc = _selectSlavePreferred( *nodes, primaryPos, _selectedPositions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to select slave position for "
                      "group [%u], rc: %d", pGroupItem->groupID(), rc ) ;

         if ( !_selectedPositions.empty() )
         {
            pos = _selectedPositions.front() ;
            _selectedPositions.pop_front() ;
            selected = TRUE ;
         }
      }

      if ( !selected )
      {
         UINT32 nodeCount = pGroupItem->nodeCount() ;
         BOOLEAN isSlavePreferred = FALSE ;

         if ( instanceOption.isMasterPreferred() )
         {
            // if there is no primary, then go on to get random node
            if ( CLS_RG_NODE_POS_INVALID != primaryPos )
            {
               selected = TRUE ;
               pos = (INT32)primaryPos ;
            }
            else
            {
               pos = random ;
            }
         }
         else if ( instanceOption.isSlavePreferred() )
         {
            isSlavePreferred = TRUE ;
            pos = random ;
         }
         else if ( instanceOption.getSpecialInstance() ==
                   PMD_PREFER_INSTANCE_TYPE_ANYONE ||
                   instanceOption.getSpecialInstance() ==
                   PMD_PREFER_INSTANCE_TYPE_ANYONE_SND )
         {
            pos = random ;
         }
         else
         {
            if ( 1 == instanceOption.getInstanceList().size() )
            {
               pos = instanceOption.getInstanceList().front() - 1 ;
            }
            else
            {
               pos = random ;
            }
         }

         if ( !selected && nodeCount > 0 )
         {
            // Round up the position to number of nodes
            pos = pos % nodeCount ;
            if ( isSlavePreferred && (UINT32)pos == primaryPos )
            {
               // Move one position back for slave preferred but primary has
               // been chosen
               pos = ( pos + 1 ) % nodeCount ;
            }
         }
      }

   done:
      return rc ;

   error :
      goto done ;
   }

   INT32 _coordGroupSel::_nextPos( CoordGroupInfoPtr &groupPtr,
                                   const rtnInstanceOption & instanceOption,
                                   UINT32 &selTimes,
                                   INT32 &pos,
                                   MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;
      INT32 status = NET_NODE_STAT_UNKNOWN ;
      UINT32 tmpPos = 0 ;
      BOOLEAN foundNode = FALSE ;
      clsGroupItem *pGroupItem = groupPtr.get() ;
      UINT32 groupID = pGroupItem->groupID() ;
      BOOLEAN isSlavePreferred = FALSE ;
      UINT32 nodeCount = pGroupItem->nodeCount() ;

      while( selTimes < nodeCount )
      {
         tmpPos = pos ;
         if ( selTimes > 0 )
         {
            if ( !_selectedPositions.empty() )
            {
               tmpPos = _selectedPositions.front() ;
               _selectedPositions.pop_front() ;
            }
            else if ( instanceOption.isPreferredStrict() )
            {
               break ;
            }
            else
            {
               // Only when choose from group will consider move position
               // for slave preferred option
               // The selected positions have been considered for slave
               // preferred option
               isSlavePreferred = instanceOption.isSlavePreferred() ;
               tmpPos = ( tmpPos + 1 ) % nodeCount ;
            }
         }

         if ( isSlavePreferred )
         {
            // Slave is preferred, avoid to use the primary node
            UINT32 primaryPos = pGroupItem->getPrimaryPos() ;
            if ( CLS_RG_NODE_POS_INVALID != primaryPos &&
                 selTimes + 1 == nodeCount )
            {
               tmpPos = primaryPos ;
            }
            else if ( tmpPos == primaryPos )
            {
               tmpPos = ( tmpPos + 1 ) % nodeCount ;
            }
         }

         pos = tmpPos ;
         ++selTimes ;

         rc = pGroupItem->getNodeID( pos, nodeID, _svcType ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = pGroupItem->getNodeInfo( pos, status ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( NET_NODE_STAT_NORMAL == status )
         {
            foundNode = TRUE ;
            break ;
         }
         ++_ignoredNum ;
      }

      if ( !foundNode )
      {
         rc = SDB_CLS_NODE_BSFAULT ;

         if ( !_hasUpdate )
         {
            _hasUpdate = TRUE ;
            rc = _pResource->updateGroupInfo( groupID, groupPtr,
                                              _pPropSite->getEDUCB() ) ;
            if ( rc )
            {
               if ( SDB_CLS_GRP_NOT_EXIST != rc && _ignoredNum > 0 )
               {
                  PD_LOG( PDWARNING, "Update group[%u] info failed, rc: %d",
                          groupID, rc ) ;
                  groupPtr->clearNodesStat() ;
                  rc = SDB_OK ;
               }
               else
               {
                  PD_LOG( PDERROR, "Update group[%u] info failed, rc: %d",
                          groupID, rc ) ;
               }
            }
         }

         if ( SDB_OK == rc )
         {
            _resetStatus() ;
            /// need set update to TRUE
            _hasUpdate = TRUE ;
            rc = _selOtherBegin( nodeID ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSel::_selectPositions ( const VEC_NODE_INFO & groupNodes,
                                            UINT32 primaryPos,
                                            const rtnInstanceOption & instanceOption,
                                            COORD_POS_LIST & selectedPositions )
   {
      INT32 rc = SDB_OK ;

      PMD_PREFER_INSTANCE_MODE mode = instanceOption.getPreferredMode() ;
      const RTN_INSTANCE_LIST & instanceList = instanceOption.getInstanceList() ;
      COORD_POS_ARRAY tempPositions ;
      COORD_POS_ARRAY orderTemp ;
      UINT8 unselectMask = 0xFF ;
      UINT32 nodeCount = groupNodes.size() ;
      BOOLEAN foundPrimary = FALSE ;
      BOOLEAN primaryFirst = ( instanceOption.getSpecialInstance() ==
                               PMD_PREFER_INSTANCE_TYPE_MASTER ||
                               instanceOption.getPreferredConstraint() ==
                               PMD_PREFER_CONSTRAINT_PRY_ONLY ) ;
      BOOLEAN primaryLast = ( instanceOption.getSpecialInstance() ==
                              PMD_PREFER_INSTANCE_TYPE_SLAVE ||
                              instanceOption.getPreferredConstraint() ==
                              PMD_PREFER_CONSTRAINT_SND_ONLY ) ;

      selectedPositions.clear() ;

      // no need to get select positions in below cases
      // - no nodes in this group
      // - instance list is empty
      if ( nodeCount == 0 || instanceList.empty() )
      {
         goto done ;
      }

      // iterate each instance to match nodes in current group
      for ( RTN_INSTANCE_LIST::const_iterator instIter = instanceList.begin() ;
            instIter != instanceList.end() ;
            instIter ++ )
      {
         PMD_PREFER_INSTANCE_TYPE instance = ( PMD_PREFER_INSTANCE_TYPE )( *instIter ) ;
         if ( instance > PMD_PREFER_INSTANCE_TYPE_MIN &&
              instance < PMD_PREFER_INSTANCE_TYPE_MAX )
         {
            UINT8 pos = 0 ;
            for ( VEC_NODE_INFO::const_iterator nodeIter = groupNodes.begin() ;
                  nodeIter != groupNodes.end() ;
                  nodeIter ++, pos ++ )
            {
               if ( nodeIter->_instanceID == (UINT8)instance )
               {
                  // check if current node is primary
                  if ( primaryPos == pos )
                  {
                     foundPrimary = TRUE ;
                     if ( primaryFirst || primaryLast )
                     {
                        // primary is required in the first or the last position
                        // skip to process other nodes
                        // will add primary later
                        continue ;
                     }
                  }

                  // if this is not primary, or we don't care about primary
                  // save to result
                  if ( PMD_PREFER_INSTANCE_MODE_ORDERED == mode )
                  {
                     // the preferred mode is ordered, save the position
                     // into candidate positions which will be shuffled after
                     // the end of the loop
                     try
                     {
                        orderTemp.append( pos ) ;
                     }
                     catch ( exception &e )
                     {
                        PD_LOG( PDERROR, "Failed to add selected position, "
                                "error: %s", e.what() ) ;
                        rc = SDB_SYS ;
                        goto error ;
                     }
                  }
                  else
                  {
                     // the preferred mode is random, save the position
                     // into candidate positions which will be shuffled later
                     try
                     {
                        tempPositions.append( pos ) ;
                     }
                     catch ( exception &e )
                     {
                        PD_LOG( PDERROR, "Failed to add selected position, "
                                "error: %s", e.what() ) ;
                        rc = SDB_SYS ;
                        goto error ;
                     }
                  }

                  OSS_BIT_CLEAR( unselectMask, 1 << pos ) ;
               }
            }
            // shuffle candidate positions of slave nodes which have same instanceid
            // into selected positions
            // NOTE: the preferred mode is ordered in this case
            if ( !orderTemp.empty() )
            {
               SDB_ASSERT( PMD_PREFER_INSTANCE_MODE_ORDERED == mode,
                           "should be ordered mode" ) ;
               rc = _shufflePositions( orderTemp, selectedPositions ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to shuffle positions, rc: %d",
                            rc ) ;
            }
         }
      }

      if ( !tempPositions.empty() )
      {
         // shuffle candidate positions into selected positions
         // NOTE: the preferred mode is random in this case
         SDB_ASSERT( PMD_PREFER_INSTANCE_MODE_RANDOM == mode,
                     "should be random mode" ) ;
         rc = _shufflePositions( tempPositions, selectedPositions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shuffle positions, rc: %d",
                      rc ) ;
      }

      if ( foundPrimary )
      {
         // primary is found in preferred instance
         if ( primaryFirst )
         {
            // preferred is master, put the primary at the first of
            // selected positions
            try
            {
               selectedPositions.push_front( primaryPos ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add selected position, "
                       "error: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else if ( primaryLast )
         {
            // preferred is slave, put the primary at the last of selected
            // positions
            try
            {
               selectedPositions.push_back( primaryPos ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add selected position, "
                       "error: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         OSS_BIT_CLEAR( unselectMask, 1 << primaryPos ) ;
      }

      // if not strict preferred mode, push the unselected positions in the
      // end of selected positions
      // - if some of them are selected already, we could put remaining nodes
      //   as later choice
      // - if master or slave is preferred, we need to put the remaining nods
      //   into master or slave preferred order
      if ( !instanceOption.isPreferredStrict() &&
           ( !selectedPositions.empty() ||
             instanceOption.isMasterPreferred() ||
             instanceOption.isSlavePreferred() ) )
      {
         foundPrimary = FALSE ;
         tempPositions.clear() ;
         for ( UINT32 pos = 0 ; pos < nodeCount ; ++ pos )
         {
            if ( OSS_BIT_TEST( unselectMask, 1 << pos ) )
            {
               if ( pos == primaryPos )
               {
                  // slave/master is preferred, and primary should be added later
                  if ( instanceOption.isMasterPreferred() ||
                       instanceOption.isSlavePreferred() )
                  {
                     foundPrimary = TRUE ;
                     continue ;
                  }
               }
               try
               {
                  tempPositions.append( (UINT8)pos ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to add selected position, "
                          "error: %s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            }
         }

         if ( foundPrimary && instanceOption.isMasterPreferred() )
         {
            // add primary to the beginning of remaining nodes
            try
            {
               selectedPositions.push_back( primaryPos ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add selected position, "
                       "error: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( !tempPositions.empty() )
         {
            // shuffle candidate positions into selected positions
            // NOTE: the preferred mode is random in this case
            rc = _shufflePositions( tempPositions, selectedPositions ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to shuffle positions, rc: %d",
                         rc ) ;
         }

         if ( foundPrimary && instanceOption.isSlavePreferred() )
         {
            // add primary to the last
            try
            {
               selectedPositions.push_back( primaryPos ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add selected position, "
                       "error: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }

#ifdef _DEBUG
      if ( selectedPositions.empty() )
      {
         PD_LOG( PDDEBUG, "Got no selected node positions" ) ;
      }
      else
      {
         StringBuilder ss ;
         for ( COORD_POS_LIST::iterator iter = selectedPositions.begin() ;
               iter != selectedPositions.end() ;
               iter ++ )
         {
            if ( iter != selectedPositions.begin() )
            {
               ss << ", " ;
            }
            ss << ( *iter ) ;
         }
         PD_LOG( PDDEBUG, "Got selected node positions : [ %s ]",
                 ss.str().c_str() ) ;
      }
#endif

   done:
      return rc ;

   error:
      selectedPositions.clear() ;
      goto done ;
   }

   INT32 _coordGroupSel::_selectSlavePreferred ( const VEC_NODE_INFO & groupNodes,
                                                 UINT32 primaryPos,
                                                 COORD_POS_LIST & selectedPositions )
   {
      INT32 rc = SDB_OK ;

      COORD_POS_ARRAY tempPositions ;
      UINT8 pos = 0 ;

      selectedPositions.clear() ;

      // add all slave nodes into candidate positions
      for ( VEC_NODE_INFO::const_iterator nodeIter = groupNodes.begin() ;
            nodeIter != groupNodes.end() ;
            nodeIter ++, pos ++ )
      {
         if ( primaryPos != pos )
         {
            try
            {
               tempPositions.append( pos ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to add selected position, "
                       "error: %s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }

      // shuffle candidate positions
      if ( !tempPositions.empty() )
      {
         rc = _shufflePositions( tempPositions, selectedPositions ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shuffle positions, rc: %d",
                      rc ) ;
      }

      if ( CLS_RG_NODE_POS_INVALID != primaryPos )
      {
         // add primary to the last of selected positions
         try
         {
            selectedPositions.push_back( primaryPos ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add selected position, "
                    "error: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

#ifdef _DEBUG
      if ( selectedPositions.empty() )
      {
         PD_LOG( PDDEBUG, "Got no selected node positions" ) ;
      }
      else
      {
         StringBuilder ss ;
         for ( COORD_POS_LIST::iterator iter = selectedPositions.begin() ;
               iter != selectedPositions.end() ;
               iter ++ )
         {
            if ( iter != selectedPositions.begin() )
            {
               ss << ", " ;
            }
            ss << ( *iter ) ;
         }
         PD_LOG( PDDEBUG, "Got selected node positions : [ %s ]",
                 ss.str().c_str() ) ;
      }
#endif

   done:
      return rc ;

   error:
      selectedPositions.clear() ;
      goto done ;
   }

   INT32 _coordGroupSel::_savePositions( COORD_POS_ARRAY &positionArray,
                                         COORD_POS_LIST &positionList )
   {
      INT32 rc = SDB_OK ;

      COORD_POS_ARRAY::iterator posIter( positionArray ) ;
      UINT8 tmpPos = 0 ;
      while ( posIter.next( tmpPos ) )
      {
         try
         {
            positionList.push_back( tmpPos ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add selected position, "
                    "error: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      positionArray.clear() ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _coordGroupSel::_shufflePositions ( COORD_POS_ARRAY & positionArray,
                                             COORD_POS_LIST & positionList )
   {
      INT32 rc = SDB_OK ;
      if ( 0 == positionArray.size() )
      {
         goto done ;
      }
      if ( 1 == positionArray.size() )
      {
         try
         {
            positionList.push_back( positionArray[0] ) ;
         }
         catch ( exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Failed to add selected position, error: %s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
         positionArray.clear() ;
         goto done ;
      }

      for ( UINT32 i = 0 ; i < positionArray.size() ; i ++ )
      {
         UINT32 random = ossRand() % positionArray.size() ;
         if ( i != random )
         {
            UINT8 tmp = positionArray[ i ] ;
            positionArray[ i ] = positionArray[ random ] ;
            positionArray[ random ] = tmp  ;
         }
      }

      rc = _savePositions( positionArray, positionList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save selected positions, "
                   "rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _coordGroupSel::selDone()
   {
      if ( MSG_INVALID_ROUTEID != _lastNodeID.value )
      {
         // save to the last node, could be re-use for the next query
         _pPropSite->addLastNode( _lastNodeID, _primary ) ;
      }
      _resetStatus() ;
   }

   void _coordGroupSel::updateStat( const MsgRouteID &nodeID, INT32 rc )
   {
      if ( MSG_INVALID_ROUTEID != nodeID.value )
      {
         if ( SDB_OK != rc && _pPropSite )
         {
            _pPropSite->delLastNode( nodeID ) ;
         }

         if( _groupPtr.get() )
         {
            _groupPtr->updateNodeStat( nodeID.columns.nodeID,
                                       netResult2Status( rc ) ) ;
         }
      }
   }

   /*
      _coordCataSel implement
   */
   _coordCataSel::_coordCataSel()
   {
      _pResource = NULL ;
      _hasUpdate = FALSE ;
   }

   _coordCataSel::~_coordCataSel()
   {
   }

   INT32 _coordCataSel::updateCataInfo( const CHAR *pCollectionName,
                                        _pmdEDUCB *cb,
                                        BOOLEAN isRoot )
   {
      INT32 rc = SDB_OK ;

      if ( !pCollectionName || !(*pCollectionName) )
      {
         if ( _cataPtr.get() )
         {
            pCollectionName = _cataPtr->getName() ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Collection name is null or empty" ) ;
            goto error ;
         }
      }

      _hasUpdate = TRUE ;
      rc = _pResource->updateCataInfo( pCollectionName, _cataPtr, cb ) ;
      if ( rc )
      {
         /// Restore the rc when changed by updateCataInfo
         if ( isRoot && SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
         {
            rc = SDB_DMS_NOTEXIST ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCataSel::bind( coordResource *pResource,
                              const CHAR *pCollectionName,
                              _pmdEDUCB *cb,
                              BOOLEAN forceUpdate,
                              BOOLEAN isRoot )
   {
      INT32 rc = SDB_OK ;

      _pResource = pResource ;

      if ( !forceUpdate )
      {
         rc = _pResource->getCataInfo( pCollectionName, _cataPtr ) ;
      }
      else
      {
         _hasUpdate = FALSE ;
         rc = SDB_DMS_NOTEXIST ;
      }
      if ( rc && !_hasUpdate )
      {
         rc = updateCataInfo( pCollectionName, cb, isRoot ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCataSel::bind( coordResource *pResource,
                              const CoordCataInfoPtr &cataPtr,
                              BOOLEAN hasUpdated )
   {
      _pResource = pResource ;
      _cataPtr = cataPtr ;
      _hasUpdate = hasUpdated ;

      return SDB_OK ;
   }

   void _coordCataSel::clear()
   {
      _hasUpdate = FALSE ;
      _cataPtr = CoordCataInfoPtr() ;
      _mapGrp2subs.clear() ;
   }

   void _coordCataSel::setUpdated( BOOLEAN updated )
   {
      _hasUpdate = updated ;
   }

   BOOLEAN _coordCataSel::hasUpdated() const
   {
      return _hasUpdate ;
   }

   CoordCataInfoPtr& _coordCataSel::getCataPtr()
   {
      return _cataPtr ;
   }

   CoordGroupSubCLMap& _coordCataSel::getGroup2SubsMap()
   {
      return _mapGrp2subs ;
   }

   INT32 _coordCataSel::getLobGroupLst( _pmdEDUCB *cb,
                                        const CoordGroupList &exceptGrpLst,
                                        CoordGroupList &groupLst,
                                        const BSONObj *pQuery )
   {
      INT32 rc = SDB_OK ;

      _mapGrp2subs.clear() ;

      if ( !_cataPtr->isMainCL() )
      {
         _cataPtr->getGroupLst( groupLst ) ;
         if ( groupLst.size() <= 0 )
         {
            if ( pQuery )
            {
               PD_LOG( PDWARNING, "Failed to get groups for obj[%s] from "
                       "catalog info[%s]", pQuery->toString().c_str(),
                       _cataPtr->getCatalogSet()->toCataInfoBson(
                       ).toString().c_str() ) ;
            }
         }
         else
         {
            //don't resend to the node which reply ok
            CoordGroupList::const_iterator iter = exceptGrpLst.begin();
            while( iter != exceptGrpLst.end() )
            {
               groupLst.erase( iter->first ) ;
               ++iter ;
            }
         }
      }
      else
      {
         // main-collection
         CLS_SUBCL_LIST subCLLst ;
         CLS_SUBCL_LIST_IT iterCL ;

      retry:
         rc = _cataPtr->findLobSubCLNamesByMatcher( pQuery, subCLLst ) ;
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         if ( 0 == subCLLst.size() && !hasUpdated() )
         {
            if ( SDB_OK == updateCataInfo( NULL, cb ) )
            {
               goto retry ;
            }
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            static const CoordGroupList s_emptyGrpLst ;
            CoordGroupList subGrpLst ;
            CoordGroupList::iterator subGrpItr ;
            _coordCataSel subSel ;

            rc = subSel.bind( _pResource, (*iterCL).c_str(), cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get collectoin[%s]'s catalog failed, rc: %d",
                       (*iterCL).c_str(), rc ) ;
               goto error ;
            }

            rc = subSel.getLobGroupLst( cb, s_emptyGrpLst, subGrpLst, pQuery ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get sub-collection[%s]'s group list failed, "
                       "rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }

            subGrpItr = subGrpLst.begin() ;
            while ( subGrpItr != subGrpLst.end() )
            {
               if ( exceptGrpLst.end() ==
                    exceptGrpLst.find( subGrpItr->first ) )
               {
                  _mapGrp2subs[ subGrpItr->first ].push_back( *iterCL ) ;
                  groupLst[ subGrpItr->first ] = subGrpItr->second ;
               }
               ++subGrpItr ;
            }
            ++iterCL ;
         } /// end while
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCataSel::getGroupLst( _pmdEDUCB *cb,
                                     const CoordGroupList &exceptGrpLst,
                                     CoordGroupList &groupLst,
                                     const BSONObj *pQuery )
   {
      INT32 rc = SDB_OK ;

      _mapGrp2subs.clear() ;

      if ( !_cataPtr->isMainCL() )
      {
         // normal collection or sub-collection
         if ( NULL == pQuery || pQuery->isEmpty() )
         {
            _cataPtr->getGroupLst( groupLst ) ;
         }
         else
         {
            rc = _cataPtr->getGroupByMatcher( *pQuery, groupLst ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get group by matcher failed, rc: %d", rc ) ;
               goto error ;
            }
         }

         if ( groupLst.size() <= 0 )
         {
            if ( pQuery )
            {
               PD_LOG( PDWARNING, "Failed to get groups for obj[%s] from "
                       "catalog info[%s]", pQuery->toString().c_str(),
                       _cataPtr->getCatalogSet()->toCataInfoBson(
                       ).toString().c_str() ) ;
            }
         }
         else
         {
            //don't resend to the node which reply ok
            CoordGroupList::const_iterator iter = exceptGrpLst.begin();
            while( iter != exceptGrpLst.end() )
            {
               groupLst.erase( iter->first ) ;
               ++iter ;
            }
         }
      }
      else
      {
         // main-collection
         CLS_SUBCL_LIST subCLLst ;
         CLS_SUBCL_LIST_IT iterCL ;

      retry:
         if ( NULL == pQuery || pQuery->isEmpty() )
         {
            rc = _cataPtr->getSubCLList( subCLLst ) ;
         }
         else
         {
            rc = _cataPtr->getMatchSubCLs( *pQuery, subCLLst ) ;
         }
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         if ( 0 == subCLLst.size() && !hasUpdated() )
         {
            if ( SDB_OK == updateCataInfo( NULL, cb ) )
            {
               goto retry ;
            }
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            static const CoordGroupList s_emptyGrpLst ;
            CoordGroupList subGrpLst ;
            CoordGroupList::iterator subGrpItr ;
            _coordCataSel subSel ;

            // Some collections may be using the same data source.
            rc = subSel.bind( _pResource, (*iterCL).c_str(), cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get collectoin[%s]'s catalog failed, rc: %d",
                       (*iterCL).c_str(), rc ) ;
               goto error ;
            }

            rc = subSel.getGroupLst( cb, s_emptyGrpLst, subGrpLst, pQuery ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get sub-collection[%s]'s group list failed, "
                       "rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }

            subGrpItr = subGrpLst.begin() ;
            while ( subGrpItr != subGrpLst.end() )
            {
               if ( exceptGrpLst.end() ==
                    exceptGrpLst.find( subGrpItr->first ) )
               {
                  _mapGrp2subs[ subGrpItr->first ].push_back( *iterCL ) ;
                  groupLst[ subGrpItr->first ] = subGrpItr->second ;
               }
               ++subGrpItr ;
            }
            ++iterCL ;
         } /// end while
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordGroupSessionCtrl implement
   */
   _coordGroupSessionCtrl::_coordGroupSessionCtrl()
   {
      _retryTimes = 0 ;
      _maxRetryTimes = COORD_OPR_MAX_RETRY_TIMES_DFT ;
      _pResource = NULL ;
      _pPropSite = NULL ;
      _pGroupSel = NULL ;
      _pRemoteHandle = NULL ;
   }

   _coordGroupSessionCtrl::~_coordGroupSessionCtrl()
   {
   }

   void _coordGroupSessionCtrl::setMaxRetryTimes( UINT32 maxRetryTimes )
   {
      _maxRetryTimes = maxRetryTimes ;
   }

   BOOLEAN _coordGroupSessionCtrl::_canRetry() const
   {
      return _retryTimes < _maxRetryTimes ? TRUE : FALSE ;
   }

   void _coordGroupSessionCtrl::init( coordResource *pResource,
                                      coordSessionPropSite *pPropSite,
                                      coordGroupSel *pGroupSel,
                                      IRemoteSessionHandler *pRemoteHandle )
   {
      _pResource = pResource ;
      _pPropSite = pPropSite ;
      _pGroupSel = pGroupSel ;
      _pRemoteHandle = pRemoteHandle ;
   }

   void _coordGroupSessionCtrl::incRetry()
   {
      ++_retryTimes ;
   }

   void _coordGroupSessionCtrl::resetRetry()
   {
      _retryTimes = 0 ;
   }

   UINT32 _coordGroupSessionCtrl::getRetryTimes() const
   {
      return _retryTimes ;
   }

   BOOLEAN _coordGroupSessionCtrl::canRetry( INT32 flag,
                                             const MsgRouteID &nodeID,
                                             UINT32 newPrimaryID,
                                             BOOLEAN isReadCmd,
                                             BOOLEAN canUpdate )
   {
      BOOLEAN bRetry = FALSE ;
      CoordGroupInfoPtr groupPtr ;
      _pmdEDUCB *cb = NULL ;

      cb = _pPropSite->getEDUCB() ;

      if ( coordCheckNodeReplyFlag( flag ) )
      {
         /// remove last node
         _pPropSite->delLastNode( nodeID ) ;
      }

      if ( !_pGroupSel ||
           !_pGroupSel->getGroupPtrFromMap( nodeID.columns.groupID,
                                            groupPtr ) )
      {
         goto done ;
      }

      bRetry = _canRetry() ;

      if ( SDB_CLS_NOT_PRIMARY == flag && 0 != newPrimaryID )
      {
         INT32 preStat = NET_NODE_STAT_NORMAL ;
         MsgRouteID primaryNodeID ;
         primaryNodeID.value = nodeID.value ;
         primaryNodeID.columns.nodeID = newPrimaryID ;

         if ( SDB_OK == groupPtr->updatePrimary( primaryNodeID,
                                                 TRUE, &preStat ) )
         {
            /// when primay's crash has not discoverd by other nodes,
            /// new primary's nodeid may still be old one.
            /// To avoid send msg to crashed node frequently,
            /// sleep some times.
            if ( NET_NODE_STAT_NORMAL != preStat )
            {
               groupPtr->cancelPrimary() ;
               PD_LOG( PDWARNING, "Primary node[%d.%d] is crashed, sleep "
                       "%d seconds", primaryNodeID.columns.groupID,
                       primaryNodeID.columns.nodeID,
                       NET_NODE_FAULTUP_MIN_TIME ) ;
               ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
            }
            goto done ;
         }
      }
      else if ( SDB_CLS_NOT_SECONDARY == flag )
      {
         // the node reports not secondary, which means it is
         // primary node, so update primary node directly
         groupPtr->updatePrimary( nodeID, TRUE ) ;
         goto done ;
      }

      if ( !bRetry )
      {
         goto done ;
      }

      // start-from field is not set by reply, but we could guess the node
      // status by return flag
      if ( SDB_CLS_NOT_PRIMARY == flag || SDB_INVALID_ROUTEID == flag )
      {
         if ( groupPtr.get() && SDB_CLS_NOT_PRIMARY == flag )
         {
            // report it is not primary
            groupPtr->updatePrimary( nodeID, FALSE ) ;
         }

         if ( canUpdate || SDB_INVALID_ROUTEID == flag )
         {
            UINT32 groupID = nodeID.columns.groupID ;
            INT32 rc = _pResource->updateGroupInfo( groupID, groupPtr, cb ) ;
            if ( SDB_OK == rc )
            {
               _pGroupSel->addGroupPtr2Map( groupPtr ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Update group info[%u] from remote failed, "
                       "rc: %d", groupID, rc ) ;
               bRetry = FALSE ;
            }
         }
      }
      else if ( SDB_CLS_NODE_NOT_ENOUGH == flag )
      {
         /// do nothing
      }
      else if ( SDB_COORD_REMOTE_DISC == flag )
      {
         // [SDB_COORD_REMOTE_DISC] can't use in write command,
         // because when some insert/update opr do partibal,
         // if retry, data will repeat. The code can't update status,
         // because it maybe occured in long time ago
         if ( !isReadCmd )
         {
            bRetry = FALSE ;
         }
         if( groupPtr.get() )
         {
            groupPtr->updateNodeStat( nodeID.columns.nodeID,
                                      netResult2Status( flag ) ) ;
         }
      }
      else if ( SDB_CLS_FULL_SYNC == flag ||
                SDB_RTN_IN_REBUILD == flag ||
                SDB_DATABASE_DOWN == flag ||
                SDB_CLS_DATA_NOT_SYNC == flag ||
                SDB_CLS_NODE_IN_MAINTENANCE == flag )
      {
         if( groupPtr.get() )
         {
            groupPtr->updateNodeStat( nodeID.columns.nodeID,
                                      netResult2Status( flag ) ) ;
         }

         if ( !isReadCmd && SDB_DATABASE_DOWN == flag )
         {
            PD_LOG( PDWARNING, "Node[%d.%d] is doing shutdown, sleep "
                    "%d seconds", nodeID.columns.groupID,
                    nodeID.columns.nodeID,
                    NET_NODE_FAULTUP_MIN_TIME ) ;
            ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC ) ;
         }
         else if ( SDB_CLS_NODE_IN_MAINTENANCE == flag )
         {
            if ( _maxRetryTimes < COORD_OPR_MAX_RETRY_TIMES )
            {
               setMaxRetryTimes( _maxRetryTimes + 1 ) ;
            }
         }
      }
      else if ( ( SDB_UNKNOWN_MESSAGE == flag ||
                  SDB_CLS_UNKNOW_MSG == flag ) &&
                 _pRemoteHandle )
      {
         _pRemoteHandle->setUserData( coordRemoteHandlerBase::INIT_V0 ) ;
      }
      else
      {
         bRetry = FALSE ;
      }

   done:
      return bRetry ;
   }

   BOOLEAN _coordGroupSessionCtrl::canRetry( INT32 flag,
                                             coordCataSel &cataSel,
                                             BOOLEAN canUpdate )
   {
      BOOLEAN bRetry = FALSE ;
      _pmdEDUCB *cb = _pPropSite->getEDUCB() ;

      if ( _canRetry() && coordCataCheckFlag( flag ) )
      {
         bRetry = TRUE ;

         if ( canUpdate && SDB_OK != cataSel.updateCataInfo( NULL, cb ) )
         {
            bRetry = FALSE ;
         }
      }

      return bRetry ;
   }

   /*
      _coordGroupSession implement
   */
   _coordGroupSession::_coordGroupSession()
   {
      _pSite      = NULL ;
      _pPropSite  = NULL ;
      _pSession   = NULL ;
      _pGroupHandle = NULL ;
      _timeout    = 0 ;
   }

   _coordGroupSession::~_coordGroupSession()
   {
      release() ;

      _pSite      = NULL ;
      _pPropSite  = NULL ;
      _pGroupHandle = NULL ;
   }

   INT64 _coordGroupSession::getTimeout() const
   {
      return _timeout ;
   }

   INT32 _coordGroupSession::init( coordResource *pResource,
                                   _pmdEDUCB *cb,
                                   INT64 timeout,
                                   IRemoteSessionHandler *pHandle,
                                   IGroupSessionHandler *pGroupHandle )
   {
      INT32 rc = SDB_OK ;
      _timeout = timeout ;

      if ( !pResource || !cb )
      {
         SDB_ASSERT( FALSE, "Invalid arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( _pSession )
      {
         SDB_ASSERT( FALSE, "Already init" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _pGroupHandle = pGroupHandle ;
      _pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( !_pSite )
      {
         PD_LOG( PDERROR, "Session[%s] is not registered",
                 cb->toString().c_str() ) ;
         rc = SDB_SYS ;
         SDB_ASSERT( FALSE, "Session is not registered" ) ;
         goto error ;
      }
      _pPropSite = ( coordSessionPropSite* )_pSite->getUserData() ;
      if ( !_pPropSite )
      {
         rc = SDB_SYS ;
         SDB_ASSERT( FALSE, "Prop site can't be NULL" ) ;
         goto error ;
      }

      if ( 0 == _timeout )
      {
         _timeout = _pPropSite->getOperationTimeout() ;
      }
      if ( !pHandle )
      {
         pHandle = &_baseHandle ;
      }

      _groupSel.init( pResource, _pPropSite ) ;
      _groupCtrl.init( pResource, _pPropSite, &_groupSel, pHandle ) ;

      _pSession = _pSite->addSession( _timeout, pHandle ) ;
      if ( !_pSession )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _pSession->setUserData( (UINT64)this ) ;

      // check if has events which requires immediate response in the session
      _pSite->checkImmediateRespEvents( pHandle ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordGroupSession::release()
   {
      if ( _pSession )
      {
         _pSite->removeSession( _pSession ) ;
         _pSession = NULL ;
      }
   }

   void _coordGroupSession::clear()
   {
      if ( _pSession )
      {
         _pSession->clearSubSession() ;
      }
   }

   void _coordGroupSession::resetSubSession()
   {
      if ( _pSession )
      {
         _pSession->resetAllSubSession() ;
      }
   }

   pmdRemoteSession* _coordGroupSession::getSession()
   {
      return _pSession ;
   }

   coordGroupSel* _coordGroupSession::getGroupSel()
   {
      return &_groupSel ;
   }

   coordGroupSessionCtrl* _coordGroupSession::getGroupCtrl()
   {
      return &_groupCtrl ;
   }

   coordRemoteHandlerBase* _coordGroupSession::getBaseHandle()
   {
      return &_baseHandle ;
   }

   coordSessionPropSite* _coordGroupSession::getPropSite()
   {
      return _pPropSite ;
   }

   INT32 _coordGroupSession::sendMsg( MsgHeader *pSrcMsg,
                                      UINT32 groupID,
                                      const netIOVec *pIov,
                                      pmdSubSession **ppSub )
   {
      return _sendMsg( pSrcMsg, groupID, pIov, ppSub ) ;
   }

   INT32 _coordGroupSession::sendMsg( MsgHeader *pSrcMsg,
                                      CoordGroupList &grpLst,
                                      const netIOVec *pIov )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList::iterator it ;

      it = grpLst.begin() ;
      while( it != grpLst.end() )
      {
         rc = _sendMsg( pSrcMsg, it->first, pIov ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send msg[%s] to group[%u] failed, rc:%d",
                    msg2String( pSrcMsg ).c_str(), it->first, rc ) ;
            goto error ;
         }
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSession::sendMsg( MsgHeader *pSrcMsg,
                                      CoordGroupList &grpLst,
                                      const GROUP_2_IOVEC &iov )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList::iterator it ;
      GROUP_2_IOVEC::const_iterator itIO ;
      const netIOVec *pCommonIO = NULL ;
      const netIOVec *pIOVec = NULL ;

      // find common iovec
      itIO = iov.find( 0 ) ; // group id is 0 for common iovec
      if ( iov.end() != itIO )
      {
         pCommonIO = &( itIO->second ) ;
      }

      it = grpLst.begin() ;
      while( it != grpLst.end() )
      {
         itIO = iov.find( it->first ) ;
         if ( iov.end() != itIO )
         {
            pIOVec = &( itIO->second ) ;
         }
         else if ( pCommonIO )
         {
            pIOVec = pCommonIO ;
         }
         else
         {
            PD_LOG( PDERROR, "Can't find the group[%u]'s iovec datas",
                    it->first ) ;
            rc = SDB_SYS ;
            SDB_ASSERT( FALSE, "Group iovec is null" ) ;
            goto error ;
         }

         rc = _sendMsg( pSrcMsg, it->first, pIOVec ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send msg[%s] to group[%u] failed, rc:%d",
                    msg2String( pSrcMsg ).c_str(), it->first, rc ) ;
            goto error ;
         }
         ++it ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordGroupSession::_sendMsg( MsgHeader *pSrcMsg,
                                       UINT32 groupID,
                                       const netIOVec *pIov,
                                       pmdSubSession **ppSub )
   {
      INT32 rc = SDB_OK ;
      pmdSubSession *pSub = NULL ;
      MsgRouteID nodeID ;
      MsgRouteID oldNodeID ;

      rc = _groupSel.selBegin( groupID, nodeID ) ;
      if ( rc )
      {
         goto error ;
      }

      while( TRUE )
      {
         pSub = _pSession->addSubSession( nodeID.value ) ;
         if ( !pSub )
         {
            PD_LOG( PDERROR, "Add sub[%s] to session[%llu] failed",
                    routeID2String( nodeID ).c_str(),
                    _pSession->sessionID() ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         pSub->setReqMsg( pSrcMsg, PMD_EDU_MEM_NONE ) ;
         if ( pIov )
         {
            pSub->addIODatas( *pIov ) ;
         }

         if ( _pGroupHandle )
         {
            _pGroupHandle->prepareForSend( groupID, pSub, &_groupSel,
                                           &_groupCtrl ) ;
         }

         rc = _pSession->sendMsg( pSub ) ;
         if ( SDB_OK == rc )
         {
            if ( ppSub )
            {
               *ppSub = pSub ;
            }
            _groupSel.selDone() ;
            break ;
         }

         /// save old node id
         oldNodeID.value = nodeID.value ;
         /// update node stat
         _groupSel.updateStat( nodeID, rc ) ;

         /// get next node
         /// when all node failed, need to check error filter out
         if ( pSub->canSwitchOtherNode( rc ) )
         {
            rc = _groupSel.selNext( nodeID ) ;
         }
         if ( rc )
         {
            /// when ignore the error, sub session is null,
            /// can't be set to ppSub
            if ( !ppSub && pSub->canErrFilterOut( rc ) )
            {
               rc = SDB_OK ;
            }

            /// remove the old node
            _pSession->delSubSession( oldNodeID.value ) ;

            if ( rc )
            {
               goto error ;
            }
            break ;
         }

         /// remove the old node
         _pSession->delSubSession( oldNodeID.value ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

