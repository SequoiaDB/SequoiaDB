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
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_OPR_MAX_RETRY_TIMES_DFT        ( 3 )

   /*
      _coordSessionPropSite implement
   */
   _coordSessionPropSite::_coordSessionPropSite ()
   : _rtnSessionProperty(),
     _mapLastNodes(),
     _pEDUCB( NULL )
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

   void _coordSessionPropSite::addLastNode( UINT32 groupID, UINT64 nodeID )
   {
      _mapLastNodes[ groupID ] = nodeID ;
   }

   UINT64 _coordSessionPropSite::getLastNode( UINT32 groupID ) const
   {
      MAP_GROUP_2_NODE::const_iterator cit = _mapLastNodes.find( groupID ) ;
      if ( cit != _mapLastNodes.end() )
      {
         return cit->second ;
      }
      return MSG_INVALID_ROUTEID ;
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

   void _coordSessionPropSite::delLastNode( UINT32 groupID, UINT64 nodeID )
   {
      MAP_GROUP_2_NODE_IT it = _mapLastNodes.find( groupID ) ;
      if ( it != _mapLastNodes.end() &&
           ( it->second >> 16 ) == ( nodeID >> 16 ) )
      {
         _mapLastNodes.erase( it ) ;
      }
   }

   void _coordSessionPropSite::setEduCB( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
   }

   void _coordSessionPropSite::_onSetInstance ()
   {
      clear() ;
   }

   /*
      _coordSessionPropMgr implement
   */
   _coordSessionPropMgr::_coordSessionPropMgr()
   : _mapProps()
   {
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
      pSite->setUserData( (UINT64)&propSite ) ;
   }

   void _coordSessionPropMgr::onUnreg( _pmdRemoteSessionSite *pSite,
                                       _pmdEDUCB *cb )
   {
      coordSessionPropSite *pPropSite = NULL ;
      pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;
      if ( pPropSite )
      {
         pPropSite->setEduCB( NULL ) ;
         pSite->setUserData( 0 ) ;
      }
      _mapProps.erase( cb->getTID() ) ;
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

   BOOLEAN _coordGroupSel::isPreferedPrimary() const
   {
      if ( _pPropSite && _pPropSite->isMasterPreferred() )
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

      _pPropSite->getEDUCB()->getTransNodeRouteID( groupID, nodeID ) ;
      if ( MSG_INVALID_ROUTEID != nodeID.value &&
           _svcType == nodeID.columns.serviceID )
      {
         PD_LOG( PDDEBUG, "Select node: %s", routeID2String( nodeID ).c_str() ) ;
         goto done ;
      }
      else if ( _primary )
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
      if ( MSG_INVALID_ROUTEID != nodeID.value &&
           COORD_GROUP_SEL_INVALID == _pos )
      {
         if ( _groupPtr->nodePos( nodeID.columns.nodeID ) >= 0 )
         {
            nodeID.columns.serviceID = _svcType ;
            _pos = COORD_GROUP_SEL_NONE ;
            PD_LOG( PDDEBUG, "Select node: %s", routeID2String( nodeID ).c_str() ) ;
            goto done ;
         }
         _pPropSite->delLastNode( groupID ) ;
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

      _pos = _calcBeginPos( groupItem, _pPropSite->getInstanceOption(),
                            ossRand() ) ;
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
                                        UINT32 random )
   {
      UINT32 pos = 0 ;
      BOOLEAN selected = FALSE ;
      UINT32 primaryPos = pGroupItem->getPrimaryPos() ;

      if ( instanceOption.hasCommonInstance() )
      {
         const VEC_NODE_INFO * nodes = pGroupItem->getNodes() ;
         SDB_ASSERT( NULL != nodes, "node list is invalid" ) ;
         _selectPositions( *nodes, primaryPos, instanceOption, random,
                           _selectedPositions ) ;

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
         switch ( instanceOption.getSpecialInstance() )
         {
            case PREFER_INSTANCE_TYPE_MASTER :
            case PREFER_INSTANCE_TYPE_MASTER_SND :
            {
               pos = pGroupItem->getPrimaryPos() ;
               if ( CLS_RG_NODE_POS_INVALID != pos )
               {
                  selected = TRUE ;
               }
               else
               {
                  pos = random ;
               }
               break ;
            }
            case PREFER_INSTANCE_TYPE_SLAVE :
            case PREFER_INSTANCE_TYPE_SLAVE_SND :
            {
               isSlavePreferred = TRUE ;
               pos = random ;
               break ;
            }
            case PREFER_INSTANCE_TYPE_ANYONE :
            case PREFER_INSTANCE_TYPE_ANYONE_SND :
            {
               pos = random ;
               break ;
            }
            default :
            {
               if ( 1 == instanceOption.getInstanceList().size() )
               {
                  pos = instanceOption.getInstanceList().front() - 1 ;
               }
               else
               {
                  pos = random ;
               }
               break ;
            }
         }

         if ( !selected && nodeCount > 0 )
         {
            pos = pos % nodeCount ;
            if ( isSlavePreferred && pos == primaryPos )
            {
               pos = ( pos + 1 ) % nodeCount ;
            }
         }
      }

      return (INT32)pos ;
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
            if ( _selectedPositions.empty() )
            {
               isSlavePreferred = instanceOption.isSlavePerferred() ;
               tmpPos = ( tmpPos + 1 ) % nodeCount ;
            }
            else
            {
               tmpPos = _selectedPositions.front() ;
               _selectedPositions.pop_front() ;
            }
         }

         if ( isSlavePreferred )
         {
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
               if ( _ignoredNum > 0 )
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

   void _coordGroupSel::_selectPositions ( const VEC_NODE_INFO & groupNodes,
                                           UINT32 primaryPos,
                                           const rtnInstanceOption & instanceOption,
                                           UINT32 random,
                                           COORD_POS_LIST & selectedPositions )
   {
      RTN_PREFER_INSTANCE_MODE mode = instanceOption.getPreferredMode() ;
      const RTN_INSTANCE_LIST & instanceList = instanceOption.getInstanceList() ;
      COORD_POS_ARRAY tempPositions ;
      UINT8 unselectMask = 0xFF ;
      UINT32 nodeCount = groupNodes.size() ;
      BOOLEAN foundPrimary = FALSE ;
      BOOLEAN primaryFirst = ( instanceOption.getSpecialInstance() == PREFER_INSTANCE_TYPE_MASTER ) ;
      BOOLEAN primaryLast = ( instanceOption.getSpecialInstance() == PREFER_INSTANCE_TYPE_SLAVE ) ;

      selectedPositions.clear() ;

      if ( nodeCount == 0 || instanceList.empty() )
      {
         goto done ;
      }

      for ( RTN_INSTANCE_LIST::const_iterator instIter = instanceList.begin() ;
            instIter != instanceList.end() ;
            instIter ++ )
      {
         RTN_PREFER_INSTANCE_TYPE instance = (RTN_PREFER_INSTANCE_TYPE)(*instIter) ;
         if ( instance > PREFER_INSTANCE_TYPE_MIN &&
              instance < PREFER_INSTANCE_TYPE_MAX )
         {
            UINT8 pos = 0 ;
            for ( VEC_NODE_INFO::const_iterator nodeIter = groupNodes.begin() ;
                  nodeIter != groupNodes.end() ;
                  nodeIter ++, pos ++ )
            {
               if ( nodeIter->_instanceID == (UINT8)instance )
               {
                  if ( primaryPos == pos )
                  {
                     if ( !primaryFirst && !primaryLast )
                     {
                        tempPositions.append( pos ) ;
                     }
                     foundPrimary = TRUE ;
                  }
                  else
                  {
                     tempPositions.append( pos ) ;
                  }
                  OSS_BIT_CLEAR( unselectMask, 1 << pos ) ;
               }
            }
            if ( !tempPositions.empty() &&
                 PREFER_INSTANCE_MODE_ORDERED == mode )
            {
               _shufflePositions( tempPositions, selectedPositions ) ;
            }
         }
      }

      if ( !tempPositions.empty() )
      {
         _shufflePositions( tempPositions, selectedPositions ) ;
      }

      if ( foundPrimary )
      {
         if ( primaryFirst )
         {
            selectedPositions.push_front( primaryPos ) ;
         }
         else if ( primaryLast )
         {
            selectedPositions.push_back( primaryPos ) ;
         }
      }
      else if ( CLS_RG_NODE_POS_INVALID != primaryPos &&
                !selectedPositions.empty() &&
                ( instanceOption.getSpecialInstance() == PREFER_INSTANCE_TYPE_MASTER ||
                  instanceOption.getSpecialInstance() == PREFER_INSTANCE_TYPE_MASTER_SND ) )
      {
         selectedPositions.push_back( primaryPos ) ;
         OSS_BIT_CLEAR( unselectMask, 1 << primaryPos ) ;
      }

      if ( !selectedPositions.empty() )
      {
         UINT8 tmpPos = (UINT8)random ;
         for ( UINT32 i = 0 ; i < nodeCount ; i ++ )
         {
            tmpPos = ( tmpPos + 1 ) % nodeCount ;
            if ( OSS_BIT_TEST( unselectMask, 1 << tmpPos ) )
            {
               selectedPositions.push_back( (UINT8)tmpPos ) ;
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

   done :
      return ;
   }

   void _coordGroupSel::_shufflePositions ( COORD_POS_ARRAY & positionArray,
                                            COORD_POS_LIST & positionList )
   {
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

      COORD_POS_ARRAY::iterator posIter( positionArray ) ;
      UINT8 tmpPos = 0 ;
      while ( posIter.next( tmpPos ) )
      {
         positionList.push_back( tmpPos ) ;
      }

      positionArray.clear() ;
   }

   void _coordGroupSel::selDone()
   {
      if ( MSG_INVALID_ROUTEID != _lastNodeID.value )
      {
         _pPropSite->addLastNode( _lastNodeID.columns.groupID,
                                  _lastNodeID.value ) ;
      }
      _resetStatus() ;
   }

   void _coordGroupSel::updateStat( const MsgRouteID &nodeID, INT32 rc )
   {
      if ( MSG_INVALID_ROUTEID != nodeID.value )
      {
         if ( SDB_OK != rc && _pPropSite )
         {
            _pPropSite->delLastNode( nodeID.columns.groupID,
                                     nodeID.value ) ;
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

   INT32 _coordCataSel::getGroupLst( _pmdEDUCB *cb,
                                     const CoordGroupList &exceptGrpLst,
                                     CoordGroupList &groupLst,
                                     const BSONObj *pQuery )
   {
      INT32 rc = SDB_OK ;

      _mapGrp2subs.clear() ;

      if ( !_cataPtr->isMainCL() )
      {
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
         vector< string > subCLLst ;
         vector< string >::iterator iterCL ;

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
                                      coordGroupSel *pGroupSel )
   {
      _pResource = pResource ;
      _pPropSite = pPropSite ;
      _pGroupSel = pGroupSel ;
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
         _pPropSite->delLastNode( nodeID.columns.groupID, nodeID.value ) ;
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

      if ( !bRetry )
      {
         goto done ;
      }

      if ( SDB_CLS_NOT_PRIMARY == flag )
      {
         if ( groupPtr.get() )
         {
            groupPtr->updatePrimary( nodeID, FALSE ) ;
         }

         if ( canUpdate )
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
      else if ( ( isReadCmd && SDB_COORD_REMOTE_DISC == flag ) ||
                SDB_CLS_NODE_NOT_ENOUGH == flag )
      {
      }
      else if ( SDB_CLS_FULL_SYNC == flag || SDB_RTN_IN_REBUILD == flag )
      {
         if( groupPtr.get() )
         {
            groupPtr->updateNodeStat( nodeID.columns.nodeID,
                                      netResult2Status( flag ) ) ;
         }
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
      _groupSel.init( pResource, _pPropSite ) ;
      _groupCtrl.init( pResource, _pPropSite, &_groupSel ) ;

      if ( 0 == _timeout )
      {
         _timeout = _pPropSite->getOperationTimeout() ;
      }
      if ( !pHandle )
      {
         pHandle = &_baseHandle ;
      }
      _pSession = _pSite->addSession( _timeout, pHandle ) ;
      if ( !_pSession )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _pSession->setUserData( (UINT64)this ) ;

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
            _pGroupHandle->prepareForSend( pSub, &_groupSel, &_groupCtrl ) ;
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
         _pSession->delSubSession( nodeID.value ) ;
         _groupSel.updateStat( nodeID, rc ) ;
         if ( SDB_OK != _groupSel.selNext( nodeID ) )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

