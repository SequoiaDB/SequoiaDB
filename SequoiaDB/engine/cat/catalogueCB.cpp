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

   Source File Name = catalogueCB.cpp

   Descriptive Name = Catalog Control Block

   When/how to use: this program may be used in catalog component for control
   block initialization and common functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "catalogueCB.hpp"
#include "catCommon.hpp"
#include "msgCatalog.hpp"
#include "clsMgr.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pmd.hpp"
#include "pmdController.hpp"
#include "utilLightJobBase.hpp"
#include "IDataSource.hpp"
#include <stdlib.h>

using namespace bson ;

namespace engine
{

   #define CAT_WAIT_EDU_ATTACH_TIMEOUT       ( 60 * OSS_ONE_SEC )

   #define CAT_TASK_RETRY_INTERVAL           ( 10000000 ) // 10s

   /*
      _catClearTaskJob define and implement
    */
   class _catClearTaskJob : public _utilLightJob
   {
      public :
         _catClearTaskJob ( CLS_TASK_TYPE taskType)
         : _utilLightJob(),
           _taskType( taskType )
         {
         }

         virtual ~_catClearTaskJob ()
         {
         }

         virtual const CHAR * name() const
         {
            return "ClearTaskJob" ;
         }

         virtual INT32 doit( IExecutor *pExe,
                             UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime )
         {
            sleepTime = CAT_TASK_RETRY_INTERVAL ;
            result = UTIL_LJOB_DO_FINISH ;

            // This is an async task, check primary first
            BOOLEAN isDelay = FALSE ;
            INT32 rc = sdbGetCatalogueCB()->primaryCheck( (pmdEDUCB *)pExe,
                                                          FALSE,
                                                          isDelay ) ;
            if ( SDB_DPS_TRANS_DOING_ROLLBACK == rc )
            {
               // let's retry after one second
               PD_LOG( PDWARNING, "Failed to check primary during rollback, "
                       "will retry" ) ;
               result = UTIL_LJOB_DO_CONT ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               PD_LOG( PDINFO, "It is not primary, exit job[%s]", name() ) ;
            }
            else if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to check primary, rc: %d", rc ) ;
            }
            else
            {
               rc = catRemoveTasksByType( _taskType, (pmdEDUCB *)pExe, 1 ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "Failed to clear sequence tasks, rc: %d",
                          rc ) ;
               }
               else
               {
                  PD_LOG( PDINFO, "Clear sequence tasks done" ) ;
               }
            }

            return SDB_OK ;
         }

      protected :
         CLS_TASK_TYPE _taskType ;
   } ;

   typedef class _catClearTaskJob catClearTaskJob ;

   static void _catStartClearTaskJob ( CLS_TASK_TYPE taskType )
   {
      catClearTaskJob * job = SDB_OSS_NEW catClearTaskJob( taskType ) ;
      if ( NULL == job )
      {
         PD_LOG( PDWARNING, "Failed to allocate catClearTaskJob[Type: %d]",
                 taskType ) ;
      }
      else
      {
         INT32 rc = job->submit( TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to submit catClearTaskJob[Type: %d], "
                    "rc: %d", taskType, rc ) ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Submit catClearTaskJob[Type: %d] done",
                    taskType ) ;
         }
      }
   }

   /*
      _catCleanupExpiredTaskJob define and implement
    */
   #define CAT_TASK_EXPIRED_TIME     ( 1800 )     // second, 30 min
   #define CAT_CLEANUP_TASK_INTERVAL ( 600 * 1000000 ) // microsecond, 10 min

   class _catCleanupExpiredTaskJob : public _utilLightJob
   {
      public :
         _catCleanupExpiredTaskJob() : _utilLightJob() {}

         virtual ~_catCleanupExpiredTaskJob () {}

         virtual const CHAR* name() const
         {
            return "Catalog-Cleanup-Expired-Task-Job" ;
         }

         virtual INT32 doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime )
         {
            sleepTime = CAT_CLEANUP_TASK_INTERVAL ;
            result = UTIL_LJOB_DO_CONT ;

            // This is an async task, check primary first
            BOOLEAN isDelay = FALSE ;
            INT32 rc = sdbGetCatalogueCB()->primaryCheck( (pmdEDUCB *)pExe,
                                                          FALSE, isDelay ) ;
            if ( SDB_DPS_TRANS_DOING_ROLLBACK == rc )
            {
               // let's retry after one second
               PD_LOG( PDINFO, "Check primary during rollback, will retry" ) ;
               sleepTime = CAT_TASK_RETRY_INTERVAL ;
               result = UTIL_LJOB_DO_CONT ;
            }
            else if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               PD_LOG( PDINFO, "It is not primary, exit job[%s]", name() ) ;
               result = UTIL_LJOB_DO_FINISH ;
            }
            else if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to check primary, rc: %d", rc ) ;
            }
            else
            {
               rc = catRemoveExpiredTasks( (pmdEDUCB *)pExe, 1,
                                           CAT_TASK_EXPIRED_TIME ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Failed to cleanup expired tasks, rc: %d",
                          rc ) ;
               }
               else
               {
                  PD_LOG( PDINFO, "Job[%s] done", name() ) ;
               }
            }

            return SDB_OK ;
         }

   } ;

   typedef class _catCleanupExpiredTaskJob catCleanupExpiredTaskJob ;

   static void _catStartCleanupExpiredTaskJob()
   {
      _catCleanupExpiredTaskJob *job = SDB_OSS_NEW _catCleanupExpiredTaskJob() ;
      if ( NULL == job )
      {
         PD_LOG( PDWARNING, "Failed to allocate catCleanupExpiredTaskJob" ) ;
      }
      else
      {
         INT32 rc = job->submit( TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to submit job[%s], rc: %d",
                    job->name(), rc ) ;
         }
         else
         {
            PD_LOG( PDDEBUG, "Submit job[%s] done", job->name() ) ;
         }
      }
   }

   sdbCatalogueCB::sdbCatalogueCB()
   {
      _routeID.value       = 0;
      _strHostName         = "";
      _strCatServiceName   = "";
      _pNetWork            = NULL;
      _iCurNodeId          = CAT_DATA_NODE_ID_BEGIN;
      _iCurGrpId           = CAT_DATA_GROUP_ID_BEGIN;
      _curSysNodeId        = SYS_NODE_ID_BEGIN;
      _nextLocId           = CAT_NODE_LOCATION_ID_BEGIN ;
      _primaryID.value     = MSG_INVALID_ROUTEID ;
      _isActived           = FALSE ;
      _needForceSecondary  = FALSE ;
      _nodeInfoChanged     = FALSE ;
      _inPacketLevel       = 0 ;
   }

   sdbCatalogueCB::~sdbCatalogueCB()
   {
   }

   INT16 sdbCatalogueCB::majoritySize( BOOLEAN needWaitSync )
   {
      // For sub-command inside transaction, do not wait for replicas
      // For ending transaction of commands, wait for majority number of replicas

      INT16 ret = 1 ;

      INT16 w = sdbGetReplCB()->majoritySize() ;

      if ( needWaitSync )
      {
         ret = w ;
      }

      return ret ;
   }

   INT32 sdbCatalogueCB::primaryCheck( _pmdEDUCB *cb, BOOLEAN canDelay,
                                       BOOLEAN &isDelay )
   {
      pmdKRCB *pKRCB = pmdGetKRCB() ;
      replCB *pRepl = pKRCB->getClsCB()->getReplCB() ;
      INT32 rc = SDB_OK ;
      isDelay = FALSE ;

      if ( pRepl->primaryIsMe() &&
           ( _isActived || pKRCB->isDBReadonly() ||
             pKRCB->isDBDeactivated() ) )
      {
         /// check rollback
         if ( pmdGetKRCB()->getTransCB()->isDoRollback() )
         {
            if ( !canDelay || !delayCurOperation() )
            {
               rc = SDB_DPS_TRANS_DOING_ROLLBACK ;
            }
         }
         goto done ;
      }
      rc = SDB_CLS_NOT_PRIMARY ;

      // if know primary exist( and not self ) or no majority size,
      // return at now, otherwise, need to wait some time
      if ( MSG_INVALID_ROUTEID !=
           ( _primaryID.value = pRepl->getPrimary().value ) &&
             ! pRepl->primaryIsMe() && pRepl->isSendNormal( _primaryID.value ) )
      {
         goto error ;
      }
      else if ( ! pRepl->isMajorityAlive() )
      {
         goto error ;
      }
      else if ( canDelay && delayCurOperation() )
      {
         isDelay = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   UINT32 sdbCatalogueCB::setTimer( UINT32 milliSec )
   {
      UINT32 id = NET_INVALID_TIMER_ID ;
      if ( _pNetWork )
      {
         _pNetWork->addTimer( milliSec, &_catMainCtrl, id ) ;
      }
      return id ;
   }

   void sdbCatalogueCB::killTimer( UINT32 timerID )
   {
      if ( _pNetWork )
      {
         _pNetWork->removeTimer( timerID ) ;
      }
   }

   BOOLEAN sdbCatalogueCB::delayCurOperation()
   {
      return _catMainCtrl.delayCurOperation() ;
   }

   void sdbCatalogueCB::addContext( const UINT32 &handle, UINT32 tid,
                                    INT64 contextID )
   {
      _catMainCtrl.addContext( handle, tid, contextID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_INIT, "sdbCatalogueCB::init" )
   INT32 sdbCatalogueCB::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_INIT ) ;

      // 1. init param
      _routeID.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
      _strHostName = pmdGetKRCB()->getHostName() ;
      _strCatServiceName = pmdGetOptionCB()->catService() ;

      // register event handle
      pmdGetKRCB()->regEventHandler( this ) ;

      // 2. create objs
      _pNetWork = SDB_OSS_NEW _netRouteAgent( &_catMainCtrl ) ;
      if ( !_pNetWork )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for netRouteAgent" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // 3. member objs init
      rc = _catMainCtrl.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init main controller, rc: %d", rc ) ;

      rc = _catGTSMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cat GTS manager, rc: %d", rc ) ;

      rc = _catlogueMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init catlogue manager, rc: %d", rc ) ;

      rc = _catNodeMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cat node manager, rc: %d", rc ) ;

      rc = _catDCMgr.init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init cat dc manager, rc: %d", rc ) ;

      rc = _recycleBinMgr.init( _catDCMgr.getDCInfo()->getRecycleBinConf() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init recycle bin manager, rc: %d",
                   rc ) ;

      // 4. create listen
      if ( pmdGetOptionCB()->serviceMask() & PMD_SVC_MASK_CATALOG )
      {
         PD_LOG( PDEVENT, "Catalog listener is disabled" );
      }
      else
      {
         PD_TRACE1 ( SDB_CATALOGCB_INIT,
                     PD_PACK_ULONG ( _routeID.value ) ) ;

         _pNetWork->getFrame()->setBeatInfo(
               pmdGetOptionCB()->getOprTimeout() ) ;

         _pNetWork->setLocalID( _routeID );
         rc = _pNetWork->updateRoute( _routeID,
                                      _strHostName.c_str(),
                                      _strCatServiceName.c_str() );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to update route(routeID=%lld, host=%s, "
                     "service=%s, rc=%d)", _routeID.value, _strHostName.c_str(),
                     _strCatServiceName.c_str(), rc);
            goto error ;
         }
         rc = _pNetWork->listen( _routeID );
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to open listen-port(host=%s, service=%s, "
                     "rc=%d)", _strHostName.c_str(), _strCatServiceName.c_str(),
                     rc );
            goto error ;
         }

         PD_LOG ( PDEVENT, "Success to listen[host=%s, service=%s]"
                           " for catalog cb",
                  _strHostName.c_str(), _strCatServiceName.c_str() );
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGCB_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbCatalogueCB::active ()
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // 1. start catMgr edu
      _catMainCtrl.getAttachEvent()->reset() ;
      rc = pEDUMgr->startEDU ( EDU_TYPE_CATMGR,
                               (_pmdObjBase*)getMainController(),
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start cat main controller edu, "
                   "rc: %d", rc ) ;
      rc = _catMainCtrl.getAttachEvent()->wait( CAT_WAIT_EDU_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to wait cat manager edu "
                   "attach, rc: %d", rc ) ;

      // 2. start net edu
      rc = sdbGetPMDController()->registerNet( _pNetWork->getFrame(),
                                               MSG_ROUTE_SHARD_SERVCIE,
                                               FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register net monitor on "
                   "SHARD service, rc: %d", rc ) ;

      rc = pEDUMgr->startEDU ( EDU_TYPE_CATNETWORK,
                               (netRouteAgent*)netWork(),
                               &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Start CATNET failed, rc: %d", rc ) ;

      rc = pEDUMgr->waitUntil( eduID, PMD_EDU_RUNNING ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait CATNET active failed, rc: %d", rc ) ;

      rc = _recycleBinMgr.startBGJob() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start background job for "
                   "recycle bin manager, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbCatalogueCB::deactive ()
   {
      if ( NULL != _pNetWork )
      {
         // unregister monitor
         sdbGetPMDController()->unregNet( _pNetWork->getFrame() ) ;
         // stop listen
         _pNetWork->shutdownListen() ;
         // stop IO
         _pNetWork->stop() ;
      }

      return SDB_OK ;
   }

   INT32 sdbCatalogueCB::fini ()
   {
      // member objects fini
      _catDCMgr.fini() ;
      _catNodeMgr.fini() ;
      _catlogueMgr.fini() ;
      _catGTSMgr.fini() ;
      _catMainCtrl.fini() ;
      _recycleBinMgr.fini() ;

      // unregister event handle
      pmdGetKRCB()->unregEventHandler( this ) ;

      if ( _pNetWork != NULL )
      {
         SDB_OSS_DEL _pNetWork;
         _pNetWork = NULL;
      }

      SDB_ASSERT( _vecEventHandler.empty(),
                  "should not have event handlers" ) ;

      return SDB_OK ;
   }

   void sdbCatalogueCB::onConfigChange ()
   {
      if ( NULL != _pNetWork )
      {
         _pNetWork->getFrame()->setBeatInfo(
               pmdGetOptionCB()->getOprTimeout() ) ;
      }
   }

   void sdbCatalogueCB::onConfigSave()
   {
      _catDCMgr.updateGlobalAddr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_INSERTGROUPID, "sdbCatalogueCB::insertGroupID" )
   void sdbCatalogueCB::insertGroupID( UINT32 grpID, const string &name,
                                       BOOLEAN isActive )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_INSERTGROUPID ) ;
      PD_TRACE2 ( SDB_CATALOGCB_INSERTGROUPID,
                  PD_PACK_UINT ( grpID ),
                  PD_PACK_UINT ( isActive ) ) ;
      if ( grpID >= CAT_DATA_GROUP_ID_BEGIN )
      {
         if ( isActive )
         {
            _grpIdMap.insert( GRP_ID_MAP::value_type(grpID, name) );
         }
         else
         {
            _deactiveGrpIdMap.insert( GRP_ID_MAP::value_type(grpID, name) );
         }
         _iCurGrpId = _iCurGrpId > grpID ? _iCurGrpId : ++grpID ;
      }
      PD_TRACE_EXIT ( SDB_CATALOGCB_INSERTGROUPID ) ;
   }

   void sdbCatalogueCB::clearInfo()
   {
      _nodeIdMap.clear() ;
      _sysNodeIdMap.clear() ;
      _grpIdMap.clear() ;
      _deactiveGrpIdMap.clear() ;
      _locIdMap.clear() ;
      _idLocMap.clear() ;
   }

   sdbCatalogueCB::GRP_ID_MAP * sdbCatalogueCB::getGroupMap( BOOLEAN isActive )
   {
      if ( isActive )
      {
         return &_grpIdMap ;
      }
      else
      {
         return &_deactiveGrpIdMap ;
      }
   }

   const CHAR* sdbCatalogueCB::groupID2Name( UINT32 groupID )
   {
      if ( CATALOG_GROUPID == groupID )
      {
         return CATALOG_GROUPNAME ;
      }
      else if ( COORD_GROUPID == groupID )
      {
         return COORD_GROUPNAME ;
      }
      else if ( SDB_IS_DSID( groupID ) )
      {
         return CAT_DATASOURCE_GROUPNAME ;
      }

      GRP_ID_MAP::iterator it = _grpIdMap.find( groupID ) ;
      if ( it != _grpIdMap.end() )
      {
         return it->second.c_str() ;
      }
      it = _deactiveGrpIdMap.find( groupID ) ;
      if ( it != _deactiveGrpIdMap.end() )
      {
         return it->second.c_str() ;
      }
      return "" ;
   }

   UINT32 sdbCatalogueCB::groupName2ID( const string &groupName )
   {
      if ( 0 == ossStrcmp( groupName.c_str(), CATALOG_GROUPNAME ) )
      {
         return CATALOG_GROUPID ;
      }
      else if ( 0 == ossStrcmp( groupName.c_str(), COORD_GROUPNAME ) )
      {
         return COORD_GROUPID ;
      }

      GRP_ID_MAP::iterator it = _grpIdMap.begin() ;
      while ( it != _grpIdMap.end() )
      {
         if ( groupName == it->second )
         {
            return it->first ;
         }
         ++it ;
      }
      it = _deactiveGrpIdMap.begin() ;
      while ( it != _deactiveGrpIdMap.end() )
      {
         if ( groupName == it->second )
         {
            return it->first ;
         }
         ++it ;
      }
      return CAT_INVALID_GROUPID ;
   }

   INT32 sdbCatalogueCB::getGroupsName( vector< string > &vecNames )
   {
      vecNames.clear() ;
      GRP_ID_MAP::iterator it = _grpIdMap.begin() ;
      while ( it != _grpIdMap.end() )
      {
         vecNames.push_back( it->second ) ;
         ++it ;
      }
      it = _deactiveGrpIdMap.begin() ;
      while ( it != _deactiveGrpIdMap.end() )
      {
         vecNames.push_back( it->second ) ;
         ++it ;
      }
      return (INT32)vecNames.size() ;
   }

   INT32 sdbCatalogueCB::getGroupsID( vector< UINT32 > &vecIDs,
                                      BOOLEAN isActiveOnly )
   {
      GRP_ID_MAP::iterator it ;

      vecIDs.clear() ;

      it = _grpIdMap.begin() ;
      while ( it != _grpIdMap.end() )
      {
         vecIDs.push_back( it->first ) ;
         ++it ;
      }

      if ( !isActiveOnly )
      {
         it = _deactiveGrpIdMap.begin() ;
         while ( it != _deactiveGrpIdMap.end() )
         {
            vecIDs.push_back( it->first ) ;
            ++it ;
         }
      }
      return (INT32)vecIDs.size() ;
   }

   INT32 sdbCatalogueCB::getGroupNameMap ( map<std::string, UINT32> & nameMap,
                                           BOOLEAN isActiveOnly )
   {
      GRP_ID_MAP::iterator it ;

      nameMap.clear() ;

      it = _grpIdMap.begin() ;
      while ( it != _grpIdMap.end() )
      {
         nameMap[ it->second ] = it->first ;
         ++it ;
      }

      if ( !isActiveOnly )
      {
         it = _deactiveGrpIdMap.begin() ;
         while ( it != _deactiveGrpIdMap.end() )
         {
            nameMap[ it->second ] = it->first ;
            ++it ;
         }
      }

      return (INT32)nameMap.size() ;
   }

   INT32 sdbCatalogueCB::makeGroupsObj( BSONObjBuilder &builder,
                                        vector < UINT32 > &groups,
                                        BOOLEAN ignoreErr,
                                        BOOLEAN ignoreNonExist )
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder sub( builder.subarrayStart( CAT_GROUP_NAME ) ) ;
      for ( UINT32 i = 0 ; i < groups.size() ; ++i )
      {
         UINT32 groupID = groups[ i ] ;
         const CHAR *groupName = groupID2Name( groupID ) ;
         if ( NULL == groupName || 0 == groupName[ 0 ] )
         {
            if ( ignoreNonExist )
            {
               continue ;
            }
            else if ( !ignoreErr )
            {
               rc = SDB_CLS_GRP_NOT_EXIST ;
               goto error ;
            }
         }
         SDB_ASSERT( NULL != groupName && 0 != groupName[ 0 ],
                     "Group name can't be empty" ) ;
         sub.append( BSON( CAT_GROUPID_NAME << groups[ i ] <<
                           CAT_GROUPNAME_NAME << groupName ) ) ;
      }
      sub.done() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbCatalogueCB::makeGroupsObj( BSONObjBuilder &builder,
                                        const CAT_GROUP_SET &groups,
                                        BOOLEAN ignoreErr,
                                        BOOLEAN ignoreNonExist )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONArrayBuilder sub( builder.subarrayStart( CAT_GROUP_NAME ) ) ;
         for ( CAT_GROUP_SET_IT iter = groups.begin() ;
               iter != groups.end() ;
               ++ iter )
         {
            UINT32 groupID = *iter ;
            const CHAR *groupName = groupID2Name( groupID ) ;
            if ( NULL == groupName || 0 == groupName[ 0 ] )
            {
               if ( ignoreNonExist )
               {
                  continue ;
               }
               else if ( !ignoreErr )
               {
                  rc = SDB_CLS_GRP_NOT_EXIST ;
                  goto error ;
               }
            }
            SDB_ASSERT( NULL != groupName && 0 != groupName[ 0 ],
                        "Group name can't be empty" ) ;
            sub.append( BSON( CAT_GROUPID_NAME << groupID <<
                              CAT_GROUPNAME_NAME << groupName ) ) ;
         }
         sub.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to make group object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbCatalogueCB::makeGroupsObj( BSONObjBuilder &builder,
                                        vector < string > &groups,
                                        BOOLEAN ignoreErr,
                                        BOOLEAN ignoreNonExist )
   {
      INT32 rc = SDB_OK ;
      UINT32 groupID = 0 ;
      BSONArrayBuilder sub( builder.subarrayStart( CAT_GROUP_NAME ) ) ;
      for ( UINT32 i = 0 ; i < groups.size() ; ++i )
      {
         groupID = groupName2ID( groups[ i ] ) ;

         if ( CAT_INVALID_GROUPID == groupID )
         {
            if ( ignoreNonExist )
            {
               continue ;
            }
            else if ( !ignoreErr )
            {
               rc = SDB_CLS_GRP_NOT_EXIST ;
               goto error ;
            }
         }
         SDB_ASSERT( CAT_INVALID_GROUPID != groupID,
                     "Group ID can't be invalid" ) ;
         sub.append( BSON( CAT_GROUPID_NAME << groupID <<
                           CAT_GROUPNAME_NAME << groups[ i ] ) ) ;
      }
      sub.done() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 sdbCatalogueCB::makeFailedGroupsObj( BSONObjBuilder &builder,
                                              const CAT_GROUP_LIST &groups,
                                              BOOLEAN ignoreErr,
                                              BOOLEAN ignoreNonExist )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONArrayBuilder sub( builder.subarrayStart( CAT_FAILGROUP_NAME ) ) ;

         CAT_GROUP_LIST::const_iterator itr = groups.begin() ;
         while ( groups.end() != itr )
         {
            UINT32 groupID = *itr++ ;
            const CHAR *groupName = groupID2Name( groupID ) ;

            if ( NULL == groupName || 0 == groupName[ 0 ] )
            {
               if ( ignoreNonExist )
               {
                  continue ;
               }
               else if ( !ignoreErr )
               {
                  rc = SDB_CLS_GRP_NOT_EXIST ;
                  goto error ;
               }
            }
            SDB_ASSERT( NULL != groupName && 0 != groupName[ 0 ], "Group name can't be empty" ) ;
            sub.append( BSON( CAT_GROUPID_NAME << groupID << CAT_GROUPNAME_NAME << groupName ) ) ;
         }
         sub.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to make group object, occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_REMOVEGROUPID, "sdbCatalogueCB::removeGroupID" )
   void sdbCatalogueCB::removeGroupID( UINT32 grpID )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_REMOVEGROUPID ) ;
      PD_TRACE1 ( SDB_CATALOGCB_REMOVEGROUPID,
                  PD_PACK_UINT ( grpID ) ) ;
      if ( grpID >= CAT_DATA_GROUP_ID_BEGIN )
      {
         _grpIdMap.erase(grpID);
         _deactiveGrpIdMap.erase( grpID );
      }
      PD_TRACE_EXIT ( SDB_CATALOGCB_REMOVEGROUPID ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ACTIVEGROUP, "sdbCatalogueCB::activeGroup" )
   void sdbCatalogueCB::activeGroup( UINT32 groupID )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ACTIVEGROUP ) ;
      GRP_ID_MAP::iterator it = _deactiveGrpIdMap.find( groupID ) ;
      if ( it != _deactiveGrpIdMap.end() )
      {
         _grpIdMap.insert( GRP_ID_MAP::value_type( groupID, it->second ) ) ;
         _deactiveGrpIdMap.erase( it ) ;
      }
      PD_TRACE_EXIT ( SDB_CATALOGCB_ACTIVEGROUP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_DEACTIVEGROUP, "sdbCatalogueCB::deactiveGroup" )
   void sdbCatalogueCB::deactiveGroup( UINT32 groupID )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_DEACTIVEGROUP ) ;
      GRP_ID_MAP::iterator it = _grpIdMap.find( groupID ) ;
      if ( it != _grpIdMap.end() )
      {
         _deactiveGrpIdMap.insert( GRP_ID_MAP::value_type( groupID, it->second ) ) ;
         _grpIdMap.erase( it ) ;
      }
      PD_TRACE_EXIT ( SDB_CATALOGCB_DEACTIVEGROUP ) ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_INSERTNODEID, "sdbCatalogueCB::insertNodeID" )
   void sdbCatalogueCB::insertNodeID( UINT16 nodeID )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_INSERTNODEID ) ;
      PD_TRACE1 ( SDB_CATALOGCB_INSERTNODEID, PD_PACK_USHORT ( nodeID ) ) ;
      if ( nodeID >= CAT_DATA_NODE_ID_BEGIN )
      {
         _nodeIdMap.insert( NODE_ID_MAP::value_type(nodeID, nodeID) );
         _iCurNodeId = _iCurNodeId > nodeID ? _iCurNodeId : ++nodeID ;
      }
      else
      {
         _sysNodeIdMap.insert( NODE_ID_MAP::value_type(nodeID, nodeID) );
         _curSysNodeId = _curSysNodeId > nodeID ? _curSysNodeId : ++nodeID ;
      }
      PD_TRACE_EXIT ( SDB_CATALOGCB_INSERTNODEID ) ;
   }

   void sdbCatalogueCB::releaseNodeID( UINT16 nodeID )
   {
      if ( nodeID >= CAT_DATA_NODE_ID_BEGIN )
      {
         _nodeIdMap.erase( nodeID ) ;
      }
      else
      {
         _sysNodeIdMap.erase( nodeID ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_CHECKGROUPACTIVED, "sdbCatalogueCB::checkGroupActived" )
   BOOLEAN sdbCatalogueCB::checkGroupActived( const CHAR *gpName,
                                              BOOLEAN &gpExist )
   {
      BOOLEAN actived = FALSE ;
      gpExist = FALSE ;
      PD_TRACE_ENTRY( SDB_CATALOGCB_CHECKGROUPACTIVED ) ;
      GRP_ID_MAP::iterator it = _grpIdMap.begin() ;
      for ( ; _grpIdMap.end() != it ; ++it )
      {
         if ( 0 == ossStrcmp( gpName, it->second.c_str() ) )
         {
            actived = TRUE ;
            gpExist = TRUE ;
            goto done ;
         }
      }

      it = _deactiveGrpIdMap.begin() ;
      for ( ; _deactiveGrpIdMap.end() != it ; ++it )
      {
         if ( 0 == ossStrcmp( gpName, it->second.c_str() ) )
         {
            gpExist = TRUE ;
            break;
         }
      }

   done:
      PD_TRACE_EXIT( SDB_CATALOGCB_CHECKGROUPACTIVED ) ;
      return actived ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_CHECKGROUPACTIVED_ID, "sdbCatalogueCB::checkGroupActived" )
   BOOLEAN sdbCatalogueCB::checkGroupActived( UINT32 groupID,
                                              BOOLEAN &gpExist )
   {
      BOOLEAN actived = FALSE ;
      gpExist = FALSE ;

      PD_TRACE_ENTRY( SDB_CATALOGCB_CHECKGROUPACTIVED_ID ) ;

      GRP_ID_MAP::iterator it = _grpIdMap.find( groupID ) ;
      if ( it != _grpIdMap.end() )
      {
         actived = TRUE ;
         gpExist = TRUE ;
         goto done ;
      }

      it = _deactiveGrpIdMap.find( groupID ) ;
      if ( it != _deactiveGrpIdMap.end() )
      {
         gpExist = TRUE ;
         goto done ;
      }

   done:
      PD_TRACE_EXIT( SDB_CATALOGCB_CHECKGROUPACTIVED_ID ) ;

      return actived ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_GETAGROUPRAND, "sdbCatalogueCB::getAGroupRand" )
   INT32 sdbCatalogueCB::getAGroupRand( UINT32 &groupID )
   {
      INT32 rc = SDB_CAT_NO_NODEGROUP_INFO ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_GETAGROUPRAND ) ;
      UINT32 mapSize = _grpIdMap.size();
      PD_TRACE1 ( SDB_CATALOGCB_GETAGROUPRAND,
                  PD_PACK_UINT ( mapSize ) ) ;
      if ( mapSize > 0 )
      {
         UINT32 randNum = ossRand() % mapSize;
         UINT32 i = 0;
         GRP_ID_MAP::iterator iterMap = _grpIdMap.begin();
         for ( ; i < randNum && iterMap!=_grpIdMap.end(); i++ )
         {
            ++iterMap;
         }

         i = 0 ;
         while ( i++ < mapSize )
         {
            if ( iterMap == _grpIdMap.end() )
            {
               iterMap = _grpIdMap.begin() ;
            }

            if ( _catDCMgr.isImageEnabled() &&
                 !_catDCMgr.groupInImage( iterMap->second ) )
            {
               ++iterMap ;
               continue ;
            }
            else
            {
               groupID = iterMap->first ;
               rc = SDB_OK ;
               goto done ;
            }
         }
         rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGCB_GETAGROUPRAND, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ALLOCGROUPID, "sdbCatalogueCB::allocGroupID" )
   UINT32 sdbCatalogueCB::allocGroupID ()
   {
      INT32 i = 0;
      UINT32 id = 0 ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ALLOCGROUPID ) ;
      while ( i++ <= DATA_GROUP_ID_END - DATA_GROUP_ID_BEGIN )
      {
         if ( _iCurGrpId > DATA_GROUP_ID_END ||
              _iCurGrpId < CAT_DATA_GROUP_ID_BEGIN )
         {
            _iCurGrpId = CAT_DATA_GROUP_ID_BEGIN ;
         }
         GRP_ID_MAP::const_iterator it = _grpIdMap.find( _iCurGrpId );
         if ( it != _grpIdMap.end() )
         {
            _iCurGrpId++;
            continue;
         }
         it = _deactiveGrpIdMap.find( _iCurGrpId );
         if ( it != _deactiveGrpIdMap.end() )
         {
            _iCurGrpId++;
            continue;
         }
         id = _iCurGrpId ;
         goto done ;
      }
      id = CAT_INVALID_GROUPID ;
   done :
      PD_TRACE1 ( SDB_CATALOGCB_ALLOCGROUPID, PD_PACK_UINT ( id ) ) ;
      PD_TRACE_EXIT ( SDB_CATALOGCB_ALLOCGROUPID ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ALLOCCATANODEID, "sdbCatalogueCB::allocSystemNodeID" )
   UINT16 sdbCatalogueCB::allocSystemNodeID()
   {
      INT32 i = 0 ;
      UINT16 id = 0 ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ALLOCCATANODEID ) ;
      while ( i++ <= SYS_NODE_ID_END - SYS_NODE_ID_BEGIN )
      {
         if ( _curSysNodeId > SYS_NODE_ID_END )
         {
            _curSysNodeId = SYS_NODE_ID_BEGIN;
         }
         NODE_ID_MAP::const_iterator it = _sysNodeIdMap.find( _curSysNodeId );
         if ( _sysNodeIdMap.end() == it )
         {
            id = _curSysNodeId ;
            insertNodeID( _curSysNodeId ) ;
            goto done ;
         }
         _curSysNodeId++ ;
      }
      id = CAT_INVALID_NODEID ;
   done :
      PD_TRACE1 ( SDB_CATALOGCB_ALLOCCATANODEID, PD_PACK_USHORT ( id ) ) ;
      PD_TRACE_EXIT ( SDB_CATALOGCB_ALLOCCATANODEID ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ALLOCNODEID, "sdbCatalogueCB::allocNodeID" )
   UINT16 sdbCatalogueCB::allocNodeID()
   {
      INT32 i = 0;
      UINT16 id = 0 ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ALLOCNODEID ) ;
      while ( i++ <= DATA_NODE_ID_END - DATA_NODE_ID_BEGIN )
      {
         if ( _iCurNodeId > DATA_NODE_ID_END ||
              _iCurNodeId < CAT_DATA_NODE_ID_BEGIN )
         {
            _iCurNodeId = CAT_DATA_NODE_ID_BEGIN;
         }
         NODE_ID_MAP::const_iterator it = _nodeIdMap.find( _iCurNodeId );
         if ( _nodeIdMap.end() == it )
         {
            id = _iCurNodeId ;
            insertNodeID( _iCurNodeId ) ;
            goto done ;
         }
         _iCurNodeId++;
      }
      id = CAT_INVALID_NODEID ;
   done :
      PD_TRACE1 ( SDB_CATALOGCB_ALLOCNODEID, PD_PACK_USHORT ( id ) ) ;
      PD_TRACE_EXIT ( SDB_CATALOGCB_ALLOCNODEID ) ;
      return id;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_INSERTLOCID, "sdbCatalogueCB::insertLocID" )
   INT32 sdbCatalogueCB::insertLocID( const ossPoolString &locName,
                                      UINT32 locID,
                                      UINT16 refCount )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_INSERTLOCID ) ;
      PD_TRACE1 ( SDB_CATALOGCB_INSERTLOCID, PD_PACK_UINT ( locID ) ) ;

      CAT_LOC2ID_MAP::iterator nameItr = _locIdMap.find( locName ) ;
      CAT_ID2LOC_MAP::const_iterator idItr = _idLocMap.find( locID ) ;
      if ( nameItr != _locIdMap.end() )
      {
         // If the location is existed, increase the catLocationInfo count
         nameItr->second._count += refCount ;
         setNodeInfoChanged( TRUE ) ;
      }
      else
      {
         // Insert the new location and make sure _nextLocId is not allocated,
         // because allocLocID can also increase _nextLocId
         if ( locID >= CAT_NODE_LOCATION_ID_BEGIN &&
              locID <= CAT_NODE_LOCATION_ID_END &&
              idItr == _idLocMap.end() )
         {
            try
            {
               setNodeInfoChanged( TRUE ) ;
               _locIdMap.insert( CAT_LOC2ID_MAP::value_type( locName,
                                 catLocationInfo( locID, refCount ) ) ) ;
               _idLocMap.insert( CAT_ID2LOC_MAP::value_type( locID, locName ) ) ;
            }
            catch( exception &e )
            {
               // Rollback if occured exception
               _locIdMap.erase( locName ) ;
               _idLocMap.erase( locID ) ;
               setNodeInfoChanged( FALSE ) ;

               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Inserting locationID map occured exception: ",
                       "%s, rc: %d", e.what(), rc ) ;
               goto error ;
            }
            _nextLocId = _nextLocId > locID ? _nextLocId : locID + 1 ;
         }
         else
         {
            PD_LOG( PDWARNING, "Inserting an invalid locationID" ) ;
            rc = SDB_INVALIDARG ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB_CATALOGCB_INSERTLOCID ) ;
      return rc ;

   error:
      goto done ;
   }

   // Decrease the catLocationInfo count,
   // and check if the catLocationInfo need to be erased in _idLocMap and _locIdMap
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_RELEASELOCID, "sdbCatalogueCB::releaseLocID" )
   void sdbCatalogueCB::releaseLocID( const ossPoolString &locName )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_RELEASELOCID ) ;

      CAT_LOC2ID_MAP::iterator it = _locIdMap.find( locName ) ;
      if ( it != _locIdMap.end() )
      {
         // Decrease the catLocationInfo count
         --it->second._count ;

         // Check the catLocationInfo count
         if ( 0 == it->second._count )
         {
            _idLocMap.erase( it->second._locationID ) ;
            _locIdMap.erase( it ) ;
         }

         setNodeInfoChanged( TRUE ) ;
      }

      PD_TRACE_EXIT ( SDB_CATALOGCB_RELEASELOCID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ALLOCLOCID, "sdbCatalogueCB::allocLocID" )
   UINT32 sdbCatalogueCB::allocLocID( const ossPoolString &locName )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ALLOCLOCID ) ;

      UINT32 i = 0 ;
      UINT32 locID = MSG_INVALID_LOCATIONID ;

      CAT_LOC2ID_MAP::const_iterator itr = _locIdMap.find( locName ) ;
      if ( itr != _locIdMap.end() )
      {
         locID = itr->second._locationID ;
      }
      else
      {
         // If the location is not existed, alloc a new one
         while ( i++ <= CAT_NODE_LOCATION_ID_END - CAT_NODE_LOCATION_ID_BEGIN )
         {
            if ( _nextLocId > CAT_NODE_LOCATION_ID_END ||
                 _nextLocId < CAT_NODE_LOCATION_ID_BEGIN )
            {
               _nextLocId = CAT_NODE_LOCATION_ID_BEGIN ;
            }
            CAT_ID2LOC_MAP::const_iterator it = _idLocMap.find( _nextLocId ) ;
            if ( it == _idLocMap.end() )
            {
               locID = _nextLocId++ ;
               goto done ;
            }
            ++_nextLocId ;
         }
         PD_LOG( PDWARNING, "All the locationID had been allocated" ) ;
      }

   done:
      PD_TRACE_EXIT ( SDB_CATALOGCB_ALLOCLOCID ) ;
      return locID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_RELEASENODE,"sdbCatalogueCB::releaseNode" )
   void sdbCatalogueCB::releaseNode( UINT16 nodeID,
                                     const ossPoolString &locName )
   {
      PD_TRACE_ENTRY ( SDB_CATALOGCB_RELEASENODE ) ;

      releaseNodeID( nodeID ) ;
      if ( ! locName.empty() )
      {
         releaseLocID( locName ) ;
      }

      PD_TRACE_EXIT ( SDB_CATALOGCB_RELEASENODE ) ;
   }


   // The caller must make sure id has the correct serviceID
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ONREGISTERED,"sdbCatalogueCB::onRegistered" )
   void sdbCatalogueCB::onRegistered ( const MsgRouteID &nodeID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGCB_ONREGISTERED ) ;
      MsgRouteID id ;
      id.value = nodeID.value ;
      id.columns.serviceID = MSG_ROUTE_CAT_SERVICE ;
      PD_TRACE1 ( SDB_CATALOGCB_ONREGISTERED,
                  PD_PACK_ULONG ( id.value ) ) ;

      rc = _pNetWork->updateRoute( _routeID, id ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to update route(old=%d,new=%d host=%s, "
                  "service=%s, rc=%d)", _routeID.columns.nodeID,
                  id.columns.nodeID, _strHostName.c_str(),
                  _strCatServiceName.c_str(), rc );
      }
      _pNetWork->setLocalID( id ) ;
      _routeID.value = id.value ;
      PD_TRACE_EXIT ( SDB_CATALOGCB_ONREGISTERED ) ;
   }

   void sdbCatalogueCB::onPrimaryChange( BOOLEAN primary,
                                         SDB_EVENT_OCCUR_TYPE occurType )
   {
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      pmdEDUEventTypes eventType = primary ? PMD_EDU_EVENT_ACTIVE :
                                             PMD_EDU_EVENT_DEACTIVE ;

      if ( SDB_EVT_OCCUR_AFTER == occurType )
      {
         EDUID eduID = pEDUMgr->getSystemEDU( EDU_TYPE_CATMGR ) ;
         if ( PMD_INVALID_EDUID != eduID )
         {
            _catMainCtrl.getChangeEvent()->reset() ;
            if ( SDB_OK != pEDUMgr->postEDUPost( eduID, eventType ) )
            {
               _catMainCtrl.getChangeEvent()->signal() ;
            }
            _catMainCtrl.getChangeEvent()->wait( OSS_ONE_SEC * 120 ) ;
         }

         if ( primary )
         {
            _isActived = TRUE ;
            _catStartClearTaskJob( CLS_TASK_SEQUENCE ) ;
            _catStartCleanupExpiredTaskJob() ;
         }
         else
         {
            _isActived = FALSE ;
         }
      }
   }

   void sdbCatalogueCB::regEventHandler ( _catEventHandler *pHandler )
   {
      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must register in main thread" ) ;

      VEC_EVENT_HANDLER::iterator iter = find( _vecEventHandler.begin(),
                                               _vecEventHandler.end(),
                                               pHandler ) ;

      SDB_ASSERT( _vecEventHandler.end() == iter, "Handle can't be same" ) ;

      _vecEventHandler.push_back( pHandler ) ;

      PD_LOG( PDDEBUG, "Register cat event handler [%s]",
              pHandler->getHandlerName() ) ;
   }

   void sdbCatalogueCB::unregEventHandler ( _catEventHandler *pHandler )
   {
      SDB_ASSERT( pHandler, "Handle can't be NULL" ) ;
      SDB_ASSERT( pmdGetThreadEDUCB() &&
                  EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must unregister in main thread" ) ;

      VEC_EVENT_HANDLER::iterator iter = find( _vecEventHandler.begin(),
                                               _vecEventHandler.end(),
                                               pHandler ) ;
      if ( _vecEventHandler.end() != iter )
      {
         _vecEventHandler.erase( iter ) ;
      }

      PD_LOG( PDDEBUG, "Unregister cat event handler [%s]",
              pHandler->getHandlerName() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ONBEGINCMD, "sdbCatalogueCB::onBeginCommand" )
   INT32 sdbCatalogueCB::onBeginCommand ( MsgHeader *pReqMsg )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB_ONBEGINCMD ) ;

      for ( VEC_EVENT_HANDLER::iterator iter = _vecEventHandler.begin();
            iter != _vecEventHandler.end() ;
            ++iter )
      {
         _catEventHandler *pHandler = (*iter) ;
         rc = pHandler->onBeginCommand( pReqMsg ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING,
                    "Failed on begin command event [%d] with handler [%s]",
                    pReqMsg->opCode, pHandler->getHandlerName() ) ;
         }
      }

      PD_TRACE_EXIT( SDB_CATALOGCB_ONBEGINCMD ) ;

      // Ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ONENDCMD, "sdbCatalogueCB::onEndCommand" )
   INT32 sdbCatalogueCB::onEndCommand ( MsgHeader *pReqMsg, INT32 result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB_ONENDCMD ) ;

      // Over reverse order
      for ( VEC_EVENT_HANDLER::reverse_iterator iter = _vecEventHandler.rbegin();
            iter != _vecEventHandler.rend() ;
            ++iter )
      {
         _catEventHandler *pHandler = (*iter) ;
         rc = pHandler->onEndCommand( pReqMsg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING,
                    "Failed on end command event [%d] with handler [%s]",
                    pReqMsg->opCode, pHandler->getHandlerName() ) ;
         }
      }

      // check if we need force to secondary
      if ( _needForceSecondary )
      {
         sdbGetReplCB()->voteMachine()->force( CLS_ELECTION_STATUS_SEC ) ;
         _needForceSecondary = FALSE ;
      }

      PD_TRACE_EXIT( SDB_CATALOGCB_ONENDCMD ) ;

      // ignore errors
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_ONSENDREPLY, "sdbCatalogueCB::onSendReply" )
   INT32 sdbCatalogueCB::onSendReply ( MsgOpReply *pReply, INT32 result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB_ONSENDREPLY ) ;

      // Over reverse order
      for ( VEC_EVENT_HANDLER::reverse_iterator iter = _vecEventHandler.rbegin();
            iter != _vecEventHandler.rend() ;
            ++iter )
      {
         _catEventHandler *pHandler = (*iter) ;
         rc = pHandler->onSendReply( pReply, result ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed on send reply event [%d] with handler [%s]",
                      pReply->header.opCode, pHandler->getHandlerName() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATALOGCB_ONSENDREPLY, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_CHKUPGRADE, "sdbCatalogueCB::checkUpgrade" )
   INT32 sdbCatalogueCB::checkUpgrade()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB_CHKUPGRADE ) ;

      UINT32 currentVersion = CATALOG_VERSION_V0 ;

      rc = _catDCMgr.getCATVersion( currentVersion ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get CATALOG version, rc: %d", rc ) ;

      // check current version
      if ( CATALOG_VERSION_CUR == currentVersion )
      {
         // no need to upgrade
         goto done ;
      }
      else if ( isDCReadonly() )
      {
         // read only could not upgrade
         goto done ;
      }

      if ( currentVersion > CATALOG_VERSION_CUR )
      {
         rc = _onDowngrade( currentVersion ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to downgrade CATALOG version, "
                      "rc: %d", rc ) ;
      }
      else
      {
         rc = _onUpgrade( currentVersion ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to upgrade CATALOG version, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGCB_CHKUPGRADE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB__ONUPGRADE, "sdbCatalogueCB::_onUpgrade" )
   INT32 sdbCatalogueCB::_onUpgrade( UINT32 beginVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB__ONUPGRADE ) ;

      UINT32 endVersion = beginVersion ;

      // upgrade versions one by one to resolve dependency on versions
      for ( UINT32 version = beginVersion + 1 ;
            version <= CATALOG_VERSION_CUR ;
            ++ version )
      {
         for ( VEC_EVENT_HANDLER::iterator iter = _vecEventHandler.begin();
               iter != _vecEventHandler.end() ;
               ++ iter )
         {
            _catEventHandler *pHandler = (*iter) ;
            rc = pHandler->onUpgrade( version ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to upgrade CATALOG version to [%u] "
                       "on handler [%s], rc: %d", pHandler->getHandlerName(),
                       rc ) ;
               break ;
            }
         }
         if ( SDB_OK != rc )
         {
            break ;
         }
         PD_LOG( PDEVENT, "Done upgrade CATALOG events for version [%u]",
                 version ) ;
         endVersion = version ;
      }

      if ( endVersion > beginVersion )
      {
         INT32 tmpRC = _catDCMgr.setCATVersion( endVersion ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Failed to set CATALOG version to [%u] "
                    "in DC manager, rc: %d", endVersion, rc ) ;
            if ( SDB_OK == rc )
            {
               rc = tmpRC ;
            }
         }
         else
         {
            PD_LOG( PDEVENT, "Upgraded CATALOG version from [%u] to to [%u]",
                    beginVersion, endVersion ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_CATALOGCB__ONUPGRADE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB__ONDOWNGRADE, "sdbCatalogueCB::_onDowngrade" )
   INT32 sdbCatalogueCB::_onDowngrade( UINT32 beginVersion )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGCB__ONDOWNGRADE ) ;

      // no downgrade events, we can set the version to current version
      // so the next upgrade will notify something may be changed
      if ( beginVersion < CATALOG_VERSION_CUR )
      {
         INT32 tmpRC = _catDCMgr.setCATVersion( CATALOG_VERSION_CUR ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Failed to set CATALOG version to [%u] "
                    "in DC manager, rc: %d", CATALOG_VERSION_CUR, rc ) ;
            if ( SDB_OK == rc )
            {
               rc = tmpRC ;
            }
         }
         else
         {
            PD_LOG( PDEVENT, "Downgraded CATALOG version from [%u] to to [%u]",
                    beginVersion, CATALOG_VERSION_CUR ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_CATALOGCB__ONDOWNGRADE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGCB_SENDREPLY, "sdbCatalogueCB::sendReply" )
   INT32 sdbCatalogueCB::sendReply ( const NET_HANDLE &handle,
                                     MsgOpReply *pReply,
                                     INT32 result,
                                     void *pReplyData,
                                     UINT32 replyDataLen,
                                     BOOLEAN needSync )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply errReply ;
      PD_TRACE_ENTRY( SDB_CATALOGCB_SENDREPLY ) ;

      rc = onSendReply( pReply, result ) ;
      PD_RC_CHECK( rc, PDERROR, "On send reply failed, rc: %d", rc ) ;

      if ( needSync )
      {
         rc = _catMainCtrl.waitSync( handle, pReply, pReplyData, replyDataLen ) ;
         PD_RC_CHECK( rc, PDERROR, "Wait sync failed, rc: %d", rc ) ;
      }

   done :
      if ( !isDelayed() && pReply && _inPacketLevel == 0 )
      {
         BSONObj errInfo ;

         PD_LOG( PDDEBUG,
                 "Sending reply message [%d] with rc [%d]",
                 pReply->header.opCode, pReply->flags ) ;

         /// when error, but has no data, fill the error obj
         if ( pReply->flags &&
              pReply->header.messageLength == sizeof( MsgOpReply ) )
         {
            pmdEDUCB *cb = pmdGetThreadEDUCB() ;
            errInfo = utilGetErrorBson( pReply->flags,
                                        cb->getInfo( EDU_INFO_ERROR ) ) ;
            pReplyData = ( void* )errInfo.objdata() ;
            replyDataLen = (INT32)errInfo.objsize() ;
            pReply->numReturned = 1 ;
            pReply->header.messageLength += replyDataLen ;
         }

         if ( pReplyData )
         {
            rc = netWork()->syncSend( handle, &(pReply->header),
                                      pReplyData, replyDataLen ) ;
         }
         else
         {
            rc = netWork()->syncSend( handle, &(pReply->header) ) ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING,
                    "Failed to send reply message [%d], rc: %d",
                    pReply->header.opCode, rc ) ;
         }
      }
      PD_TRACE_EXITRC( SDB_CATALOGCB_SENDREPLY, rc ) ;
      return rc ;

   error :
      if ( pReply )
      {
         // Delete context if the reply message specifies contextID
         // since Coord will not send KillContext if Catalog reports error
         SINT64 contextID = pReply->contextID ;
         if ( -1 != contextID )
         {
            _catMainCtrl.delContextByID( contextID, TRUE ) ;
         }

         /// when org-operator is succeed
         if ( SDB_OK == result || SDB_DMS_EOC == result )
         {
            // Replace with error reply
            fillErrReply( pReply, &errReply, rc ) ;
            pReply = &errReply ;
            pReplyData = NULL ;
            replyDataLen = 0 ;
         }
      }
      goto done ;
   }

   void sdbCatalogueCB::fillErrReply ( const MsgOpReply *pReply,
                                       MsgOpReply *pErrReply, INT32 rc )
   {
      SDB_ASSERT( pReply, "pReply should not be NULL" ) ;
      SDB_ASSERT( pErrReply, "pErrReply should not be NULL" ) ;

      // Fill error reply message
      pErrReply->header.messageLength = sizeof( MsgOpReply ) ;
      pErrReply->header.flags = pReply->header.flags ;
      pErrReply->header.opCode = pReply->header.opCode ;
      pErrReply->header.requestID = pReply->header.requestID ;
      pErrReply->header.routeID.value = pReply->header.routeID.value ;
      pErrReply->header.TID = pReply->header.TID ;
      pErrReply->header.globalID = pReply->header.globalID ;

      pErrReply->flags = rc ;
      pErrReply->contextID = -1 ;
      pErrReply->numReturned = 0 ;
      pErrReply->startFrom = pReply->startFrom ;
      pErrReply->returnMask = pReply->returnMask ;
   }

   /*
      get global catalogue cb
   */
   sdbCatalogueCB* sdbGetCatalogueCB()
   {
      static sdbCatalogueCB s_catacb ;
      return &s_catacb ;
   }

}
