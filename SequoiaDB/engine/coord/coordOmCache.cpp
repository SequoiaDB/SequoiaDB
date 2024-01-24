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

   Source File Name = coordOmCache.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordOmCache.hpp"
#include "coordOmStrategyAccessor.hpp"
#include "pd.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordTaskInfoItem implement
   */
   _coordTaskInfoItem::_coordTaskInfoItem()
   {
   }

   _coordTaskInfoItem::~_coordTaskInfoItem()
   {
   }

   INT32 _coordTaskInfoItem::init( INT64 taskID,
                                   const string &taskName )
   {
      INT32 rc = SDB_OK ;
      omTaskStrategyInfo *pInfo = NULL ;

      pInfo = SDB_OSS_NEW omTaskStrategyInfo() ;
      if ( !pInfo )
      {
         PD_LOG( PDERROR, "Allocate task strategy info failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _defaultPtr = omTaskStrategyInfoPtr( pInfo ) ;

      pInfo->setTaskID( taskID ) ;
      pInfo->setTaskName( taskName ) ;
      pInfo->setNice( OM_TASK_STRATEGY_NICE_DFT ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   omTaskStrategyInfoPtr _coordTaskInfoItem::getDefaultStrategyPtr()
   {
      return _defaultPtr ;
   }

   BOOLEAN _coordTaskInfoItem::getStrategyPtrByID( INT64 ruleID,
                                                   omTaskStrategyInfoPtr &ptr )
   {
      MAP_RULE2STRATEGY_IT it ;

      it = _mapStrategy.find( ruleID ) ;
      if ( it != _mapStrategy.end() )
      {
         ptr = it->second ;
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _coordTaskInfoItem::_search( const MMAP_STR2ID &mapInfo,
                                       const string &key,
                                       SET_RULEID &setID ) const
   {
      pair<MMAP_STR2ID_CIT,MMAP_STR2ID_CIT> ret ;
      MMAP_STR2ID_CIT cit ;

      ret = mapInfo.equal_range( key ) ;
      cit = ret.first ;
      while( cit != ret.second )
      {
         setID.insert( cit->second ) ;
         ++cit ;
      }
      return setID.size() ;
   }

   UINT32 _coordTaskInfoItem::searchByUser( const string &userName,
                                            SET_RULEID &setID ) const
   {
      _search( _mapUserName, userName, setID ) ;
      if ( !userName.empty() )
      {
         _search( _mapUserName, "", setID ) ;
      }
      return setID.size() ;
   }

   UINT32 _coordTaskInfoItem::searchByIP( const string &ip,
                                          SET_RULEID &setID ) const
   {
      _search( _mapIP, ip, setID ) ;
      if ( !ip.empty() )
      {
         _search( _mapIP, "", setID ) ;
      }
      return setID.size() ;
   }

   void _coordTaskInfoItem::insertStrategy( omTaskStrategyInfoPtr ptr )
   {
      INT64 ruleID = ptr->getRuleID() ;

      /// UserName
      _mapUserName.insert( pair<string,INT64>( ptr->getUserName(),
                                               ruleID ) ) ;
      /// IP
      if ( 0 == ptr->getIPCount() )
      {
         _mapIP.insert( pair<string,INT64>( "", ruleID ) ) ;
      }
      else
      {
         const SET_IP* pSetIP = ptr->getIPSet() ;
         SET_IP::const_iterator itIP = pSetIP->begin() ;
         while( itIP != pSetIP->end() )
         {
            _mapIP.insert( pair<string,INT64>( *itIP, ruleID ) ) ;
            ++itIP ;
         }
      }

      /// Strategy
      _mapStrategy[ ruleID ] = ptr ;
   }

   /*
      _coordOmStrategyAgent implement
   */
   _coordOmStrategyAgent::_coordOmStrategyAgent()
   {
      _lastVersion = OM_TASK_STRATEGY_INVALID_VER ;
   }

   _coordOmStrategyAgent::~_coordOmStrategyAgent()
   {
   }

   INT32 _coordOmStrategyAgent::init()
   {
      INT32 rc = SDB_OK ;
      omTaskStrategyInfo *pInfo = NULL ;

      pInfo = SDB_OSS_NEW omTaskStrategyInfo() ;
      if ( !pInfo )
      {
         PD_LOG( PDERROR, "Allocate task strategy info failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _defaultPtr = omTaskStrategyInfoPtr( pInfo ) ;

      pInfo->setTaskName( OM_TASK_STRATEGY_TASK_NAME_DFT ) ;
      pInfo->setTaskID( OM_TASK_STRATEGY_TASK_ID_DFT ) ;
      pInfo->setNice( OM_TASK_STRATEGY_NICE_DFT ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordOmStrategyAgent::fini()
   {
      clear() ;
   }

   BOOLEAN _coordOmStrategyAgent::isValid() const
   {
      return _lastVersion == OM_TASK_STRATEGY_INVALID_VER ?
             FALSE : TRUE ;
   }

   INT32 _coordOmStrategyAgent::getLastVersion() const
   {
      return _lastVersion ;
   }

   void _coordOmStrategyAgent::updateLastVersion( INT32 version )
   {
      _lastVersion = version ;
   }

   void _coordOmStrategyAgent::lock( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _latch.get_shared() ;
      }
      else
      {
         _latch.get() ;
      }
   }

   void _coordOmStrategyAgent::unlock( OSS_LATCH_MODE mode )
   {
      if ( SHARED == mode )
      {
         _latch.release_shared() ;
      }
      else
      {
         _latch.release() ;
      }
   }

   void _coordOmStrategyAgent::_clear( BOOLEAN hasLocked, BOOLEAN needNotify )
   {
      MAP_TASK_INFO_IT it ;

      if ( !hasLocked )
      {
         lock( EXCLUSIVE ) ;
      }

      it = _mapTaskInfo.begin() ;
      while( it != _mapTaskInfo.end() )
      {
         SDB_OSS_DEL it->second ;
         ++it ;
      }
      _mapTaskInfo.clear() ;

      _lastVersion = OM_TASK_STRATEGY_INVALID_VER ;

      if ( needNotify )
      {
         _changeEvent.signal() ;
      }

      if ( !hasLocked )
      {
         unlock( EXCLUSIVE ) ;
      }
   }

   void _coordOmStrategyAgent::clear()
   {
      _clear( FALSE, TRUE ) ;
   }

   void _coordOmStrategyAgent::_mergeRuleID( const SET_RULEID &left,
                                             const SET_RULEID &right,
                                             SET_RULEID &result )
   {
      SET_RULEID::const_iterator iterLeft = left.begin() ;
      SET_RULEID::const_iterator iterRight = right.begin() ;

      while ( iterLeft != left.end() && iterRight != right.end() )
      {
         if ( *iterLeft == *iterRight )
         {
            result.insert( *iterLeft ) ;
            ++iterLeft ;
            ++iterRight ;
         }
         else if ( *iterLeft < *iterRight )
         {
            ++iterLeft ;
         }
         else
         {
            ++iterRight ;
         }
      }
   }

   BOOLEAN _coordOmStrategyAgent::_findTaskItem( const string &name,
                                                 coordTaskInfoItem **ppItem )
   {
      MAP_TASK_INFO_IT iterInfo ;

      iterInfo = _mapTaskInfo.find( name ) ;
      if ( iterInfo != _mapTaskInfo.end() )
      {
         if ( ppItem )
         {
            *ppItem = iterInfo->second ;
         }
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _coordOmStrategyAgent::getTaskStrategy( const string &taskName,
                                                 const string &userName,
                                                 const string &ip,
                                                 omTaskStrategyInfoPtr &ptr,
                                                 BOOLEAN hasLocked )
   {
      SET_RULEID setUserNameID ;
      SET_RULEID setIPID ;
      SET_RULEID setMergeID ;
      SET_RULEID_IT iterID ;
      BOOLEAN needRelease = FALSE ;
      INT64 curSortID = OM_TASK_STRATEGY_INVALID_SORTID ;
      coordTaskInfoItem *pTaskInfoItem = NULL ;
      omTaskStrategyInfoPtr tmpPtr ;

      if ( !hasLocked )
      {
         lock( SHARED ) ;
         needRelease = TRUE ;
      }

      if ( !isValid() || taskName.empty() )
      {
         ptr = _defaultPtr ;
         goto done ;
      }

      if ( !_findTaskItem( taskName, &pTaskInfoItem ) )
      {
         ptr = _defaultPtr ;
         goto done ;
      }

      pTaskInfoItem->searchByUser( userName, setUserNameID ) ;
      pTaskInfoItem->searchByIP( ip, setIPID ) ;
      /// merge
      _mergeRuleID( setUserNameID, setIPID, setMergeID ) ;
      /// set strategy default
      ptr = pTaskInfoItem->getDefaultStrategyPtr() ;

      if ( setMergeID.empty() )
      {
         goto done ;
      }

      iterID = setMergeID.begin() ;
      while( iterID != setMergeID.end() )
      {
         if ( pTaskInfoItem->getStrategyPtrByID( *iterID, tmpPtr ) )
         {
            if ( OM_TASK_STRATEGY_INVALID_SORTID == curSortID ||
                 tmpPtr->getSortID() < curSortID )
            {
               ptr = tmpPtr ;
               curSortID = tmpPtr->getSortID() ;
            }
         }
         ++iterID ;
      }

   done:
      if ( needRelease )
      {
         unlock( SHARED ) ;
      }
      return SDB_OK ;
   }

   INT32 _coordOmStrategyAgent::insertTaskInfo(
                                       const vector<omTaskInfoPtr> &vecTaskInfo,
                                       BOOLEAN hasLocked )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needRelease = FALSE ;
      coordTaskInfoItem *pTaskInfoItem = NULL ;

      if ( !hasLocked )
      {
         lock( EXCLUSIVE ) ;
         needRelease = TRUE ;
      }

      for ( UINT32 i = 0 ; i < vecTaskInfo.size() ; ++i )
      {
         const omTaskInfoPtr &ptr = vecTaskInfo[i] ;

         if ( _findTaskItem( ptr->getTaskName(), NULL ) )
         {
            PD_LOG( PDWARNING, "Duplicate task[%s]",
                    ptr->toBSON().toString().c_str() ) ;
            /// ignored
            continue ;
         }

         pTaskInfoItem = SDB_OSS_NEW coordTaskInfoItem() ;
         if ( !pTaskInfoItem )
         {
            PD_LOG( PDERROR, "Allocate task info item failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         rc = pTaskInfoItem->init( ptr->getTaskID(), ptr->getTaskName() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init task info item failed, rc: %d", rc ) ;
            goto error ;
         }
         /// add to map
         _mapTaskInfo[ ptr->getTaskName() ] = pTaskInfoItem ;
         pTaskInfoItem = NULL ;
      }

   done:
      if ( needRelease )
      {
         unlock( EXCLUSIVE ) ;
      }
      if ( pTaskInfoItem )
      {
         SDB_OSS_DEL pTaskInfoItem ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmStrategyAgent::insertStrategyInfo(
                          const vector<omTaskStrategyInfoPtr> &vecStrategyInfo,
                          BOOLEAN hasLocked )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needRelease = FALSE ;
      coordTaskInfoItem *pTaskInfoItem = NULL ;

      if ( !hasLocked )
      {
         lock( EXCLUSIVE ) ;
         needRelease = TRUE ;
      }

      for ( UINT32 i = 0 ; i < vecStrategyInfo.size() ; ++i )
      {
         const omTaskStrategyInfoPtr &ptr = vecStrategyInfo[i] ;

         if ( !_findTaskItem( ptr->getTaskName(), &pTaskInfoItem ) )
         {
            /// The task is disable, ignored
            continue ;
         }

         pTaskInfoItem->insertStrategy( ptr ) ;
      }

      if ( needRelease )
      {
         unlock( EXCLUSIVE ) ;
      }
      return rc ;
   }

   INT32 _coordOmStrategyAgent::waitChange( INT64 millisec )
   {
      return _changeEvent.wait( millisec ) ;
   }

   INT32 _coordOmStrategyAgent::update( _pmdEDUCB *cb,
                                        INT64 timeout )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasLock = FALSE ;
      coordOmStrategyAccessor accessor( timeout ) ;

      omStrategyMetaInfo metaInfo ;
      vector< omTaskInfoPtr > vecTaskInfo ;
      vector< omTaskStrategyInfoPtr > vecStrategyInfo ;

      lock( EXCLUSIVE ) ;
      hasLock = TRUE ;

      rc = accessor.getMetaInfoFromOm( metaInfo, cb, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Get meta info from om failed, rc: %d", rc ) ;
         goto error ;
      }

      /// check version
      if ( getLastVersion() == metaInfo.getVersion() )
      {
         goto done ;
      }

      /// First clear
      _clear( TRUE, FALSE ) ;

      /// Then update task info
      rc = accessor.getTaskInfoFromOm( vecTaskInfo, cb, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get task info from om failed, rc: %d", rc ) ;
         goto error ;
      }
      rc = insertTaskInfo( vecTaskInfo, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Insert task info failed, rc: %d", rc ) ;
         goto error ;
      }
      /// Then update strategy info
      rc = accessor.getStrategyInfoFromOm( vecStrategyInfo, cb, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get strategy info from om failed, rc: %d", rc ) ;
         goto error ;
      }
      rc = insertStrategyInfo( vecStrategyInfo, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Insert strategy info failed, rc: %d", rc ) ;
         goto error ;
      }
      /// Last update version
      updateLastVersion( metaInfo.getVersion() ) ;

   done:
      if ( hasLock )
      {
         unlock( EXCLUSIVE ) ;
      }
      return rc ;
   error:
      goto done ;
   }

}

