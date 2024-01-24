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

   Source File Name = omManager.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "omManager.hpp"
#include "../bson/lib/md5.hpp"
#include "authCB.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "../bson/bsonobj.h"
#include "../util/fromjson.hpp"
#include "ossProc.hpp"
#include "rtn.hpp"
#include "rtnBackgroundJob.hpp"
#include "pmdController.hpp"
#include "omManagerJob.hpp"
#include "omCommand.hpp"
#include "omCommandTool.hpp"
#include "ossVer.h"
#include "omStrategyMgr.hpp"
#include "omStrategyObserverJob.hpp"
#include "rtnContextDump.hpp"

using namespace bson ;

namespace engine
{

   #define OM_UPDATE_PLUGIN_PASSWD_TIMEOUT         (86400)
   #define OM_WAIT_CB_ATTACH_TIMEOUT               ( 300 * OSS_ONE_SEC )

   const utilCSUniqueID OM_DEPLOY_CSUID = UTIL_CSUNIQUEID_SYS_MIN + 2 ;
   const utilCLUniqueID OM_DEPLOY_CLUSTER_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 1 ) ;
   const utilCLUniqueID OM_DEPLOY_HOST_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 2 ) ;
   const utilCLUniqueID OM_DEPLOY_BUSINESS_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 3 ) ;
   const utilCLUniqueID OM_DEPLOY_CONF_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 4 ) ;
   const utilCLUniqueID OM_DEPLOY_TASKINFO_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 5 ) ;
   const utilCLUniqueID OM_DEPLOY_BUSINESS_AUTH_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 6 ) ;
   const utilCLUniqueID OM_DEPLOY_RELATIONSHIP_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 7 ) ;
   const utilCLUniqueID OM_DEPLOY_PLUGINS_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 8 ) ;
   const utilCLUniqueID OM_DEPLOY_SETTINGS_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 9 ) ;
   const utilCLUniqueID OM_DEPLOY_HISTORY_CLUID =
               utilBuildCLUniqueID( OM_DEPLOY_CSUID, UTIL_CSUNIQUEID_SYS_MIN + 10 ) ;

   /*
      Message Map
   */
   BEGIN_OBJ_MSG_MAP( _omManager, _pmdObjBase )
      ON_MSG( MSG_OM_UPDATE_TASK_REQ, _onAgentUpdateTaskReq )
   END_OBJ_MSG_MAP()

   /*
      implement om manager
   */
   _omManager::_omManager()
   :_rsManager(),
    _msgHandler( &_rsManager ),
    _netAgent( &_msgHandler )
   {
      _hwRouteID.value             = MSG_INVALID_ROUTEID ;
      _hwRouteID.columns.groupID   = OMAGENT_GROUPID ;
      _hwRouteID.columns.nodeID    = 0 ;
      _hwRouteID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

      _isInitTable         = FALSE ;

      _pDmsCB              = NULL ;
      _pRtnCB              = NULL ;
      _pEDUCB              = NULL ;
      _hostVersion         = SDB_OSS_NEW omHostVersion() ;
      _taskManager         = SDB_OSS_NEW omTaskManager() ;
      _updatePluinUsrTimer = NET_INVALID_TIMER_ID ;
      _updateTimestamp     = 0 ;

      _myNodeID.value      = MSG_INVALID_ROUTEID ;
      _needReply           = TRUE ;
      _inPacketLevel       = 0 ;
      _pendingContextID    = -1 ;
      ossMemset( (void*)&_replyHeader, 0, sizeof(_replyHeader) ) ;
   }

   _omManager::~_omManager()
   {
      if ( NULL != _hostVersion )
      {
         SDB_OSS_DEL _hostVersion ;
         _hostVersion = NULL ;
      }

      if ( NULL != _taskManager )
      {
         SDB_OSS_DEL _taskManager ;
         _taskManager = NULL ;
      }
   }

   INT32 _omManager::init ()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB* pKrcb = NULL ;

      //set bson to string for js format
      BSONObj::setJSCompatibility( TRUE ) ;

      // create collection space and collection
      pKrcb   = pmdGetKRCB() ;
      _pDmsCB = pKrcb->getDMSCB() ;
      _pRtnCB = pKrcb->getRTNCB() ;

      pKrcb->regEventHandler( this ) ;

      _pmdOptionsMgr *pOptMgr = pKrcb->getOptionCB() ;

      // get options
      _wwwRootPath = pmdGetOptionCB()->getWWWPath() ;

      // set remote session manager to pmdController
      sdbGetPMDController()->setRSManager( &_rsManager ) ;

      rc = _rsManager.init( getRouteAgent() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init remote session manager, rc: %d",
                   rc ) ;

      _readAgentPort() ;
      _createVersionFile() ;

      _myNodeID.value             = MSG_INVALID_ROUTEID ;
      _myNodeID.columns.groupID   = OM_GROUPID ;
      _myNodeID.columns.nodeID    = OM_NODE_ID_BEGIN ;
      pmdSetNodeID( _myNodeID ) ;

      _myNodeID.columns.serviceID = MSG_ROUTE_OM_SERVICE ;
      _netAgent.updateRoute( _myNodeID, pKrcb->getHostName(),
                             pOptMgr->getOMService() ) ;
      rc = _netAgent.listen( _myNodeID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Create listen failed:host=%s,port=%s",
                  pKrcb->getHostName(), pOptMgr->getOMService() ) ;
         goto error ;
      }
      _netAgent.setLocalID( _myNodeID ) ;

      PD_LOG ( PDEVENT, "Create listen success:host=%s,port=%s",
               pKrcb->getHostName(), pOptMgr->getOMService() ) ;

      pKrcb->setBusinessOK( TRUE ) ;

   done:
      return rc;
   error:
      goto done;

   }

   INT32 _omManager::_createJobs()
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN returnResult    = FALSE ;
      _rtnBaseJob *pJob       = NULL ;
      EDUID jobID             = PMD_INVALID_EDUID ;
      pJob = SDB_OSS_NEW omHostNotifierJob( this, _hostVersion ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "failed to create omHostNotifierJob:rc=%d", rc ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, &jobID,
                                     returnResult ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "create omHostNotifierJob failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::refreshVersions()
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObjBuilder builder ;
      SINT64 contextID = -1 ;

      BSONObjBuilder resultBuilder ;
      BSONObj result ;
      pmdKRCB *pKrcb     = pmdGetKRCB() ;
      _SDB_DMSCB *pDMSCB = pKrcb->getDMSCB() ;
      _SDB_RTNCB *pRTNCB = pKrcb->getRTNCB() ;
      _pmdEDUCB *pEDUCB  = pmdGetThreadEDUCB() ;

      selector = BSON( OM_CLUSTER_FIELD_NAME << "" <<
                       OM_CLUSTER_FIELD_GRANTCONF << "" ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order, hint, 0,
                     pEDUCB, 0, -1, pDMSCB, pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "fail to query table:%s,rc=%d",
                 OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, pEDUCB, pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG( PDERROR, "failed to get record from table:%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         {
            BOOLEAN privilege = TRUE ;
            BSONObj record( buffObj.data() ) ;
            BSONObj grantConf = record.getObjectField(
                                                OM_CLUSTER_FIELD_GRANTCONF ) ;
            string clusterName = record.getStringField(
                                                OM_CLUSTER_FIELD_NAME ) ;
            BSONObjIterator iter( grantConf ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               BSONObj grantInfo = ele.embeddedObject() ;
               string tmpName = grantInfo.getStringField(
                                                OM_CLUSTER_FIELD_GRANTNAME ) ;

               if ( OM_CLUSTER_FIELD_HOSTFILE == tmpName )
               {
                  privilege = grantInfo.getBoolField(
                                                OM_CLUSTER_FIELD_PRIVILEGE ) ;
               }
            }

            _hostVersion->incVersion( clusterName ) ;
            _hostVersion->setPrivilege( clusterName, privilege ) ;
         }
      }
   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pRTNCB->contextDelete( contextID, pEDUCB ) ;
      }
      goto done ;
   }

   void _omManager::updateClusterVersion( string cluster )
   {
      _hostVersion->incVersion( cluster ) ;
   }

   void _omManager::removeClusterVersion( string cluster )
   {
      _hostVersion->removeVersion( cluster ) ;
   }

   void _omManager::updateClusterHostFilePrivilege( string clusterName,
                                                    BOOLEAN privilege )
   {
      _hostVersion->setPrivilege( clusterName, privilege ) ;
      _hostVersion->incVersion( clusterName ) ;
   }

   omTaskManager *_omManager::getTaskManager()
   {
      return _taskManager ;
   }

   void _omManager::onRegistered( const MsgRouteID &nodeID )
   {
      //do nothing here
   }

   void _omManager::onPrimaryChange( BOOLEAN primary,
                                     SDB_EVENT_OCCUR_TYPE occurType )
   {
      INT32 rc = SDB_OK ;

      PD_LOG( PDDEBUG, "#########onPrimaryChange, primary=%d,occurType=%d",
              primary, occurType ) ;
      if ( !_isInitTable )
      {
         if ( SDB_EVT_OCCUR_AFTER == occurType )
         {
            PD_LOG( PDDEBUG, "#########onPrimaryChange---occurType=%d",
                    occurType ) ;
            rc = _initOmTables();
            PD_RC_CHECK ( rc, PDERROR, "Failed to initial the om tables rc = %d",
                          rc ) ;

            rc = _updateTable() ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to update om tables rc = %d",
                          rc ) ;

            rc = omGetStrategyMgr()->init( pmdGetThreadEDUCB() ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to init strategy manager, rc:%d",
                          rc) ;

            rc = _createJobs() ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to create jobs:rc=%d",
                          rc ) ;

            rc = refreshVersions() ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to update cluster version:rc=%d",
                          rc ) ;

            _isInitTable = TRUE ;
         }
      }

   done:
      return ;
   error:
      PMD_RESTART_DB( rc ) ;
      goto done ;
   }

   #define OM_DEFAULT_PLUGIN_PASSWD_SIZE 17
   #define OM_DEFAULT_PLUGIN_PASSWD_LEN (OM_DEFAULT_PLUGIN_PASSWD_SIZE-1)
   INT32 _omManager::_initOmTables()
   {
      INT32 rc = SDB_OK ;
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      omDatabaseTool dbTool( cb ) ;
      omAuthTool authTool( cb, pAuthCB ) ;

      // SYSDEPLOY.SYSCLUSTER
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_CLUSTER, OM_DEPLOY_CLUSTER_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_CLUSTER,
                                         OM_CS_DEPLOY_CL_CLUSTERIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSHOST
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_HOST, OM_DEPLOY_HOST_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_HOST,
                                         OM_CS_DEPLOY_CL_HOSTIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_HOST,
                                         OM_CS_DEPLOY_CL_HOSTIDX2 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSBUSINESS
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_BUSINESS, OM_DEPLOY_BUSINESS_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_BUSINESS,
                                         OM_CS_DEPLOY_CL_BUSINESSIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSCONFIGURE
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_CONFIGURE, OM_DEPLOY_CONF_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSTASKINFO
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_TASKINFO, OM_DEPLOY_TASKINFO_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_TASKINFO,
                                         OM_CS_DEPLOY_CL_TASKINFOIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSBUSINESSAUTH
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_BUSINESS_AUTH, OM_DEPLOY_BUSINESS_AUTH_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_BUSINESS_AUTH,
                                         OM_CS_DEPLOY_CL_BUSINESSAUTHIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSRELATIONSHIP
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_RELATIONSHIP, OM_DEPLOY_RELATIONSHIP_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_RELATIONSHIP,
                                         OM_CS_DEPLOY_CL_RELATIONSHIPIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSPLUGINS
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_PLUGINS, OM_DEPLOY_PLUGINS_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_PLUGINS,
                                         OM_CS_DEPLOY_CL_PLUGINSIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

      {
         CHAR passwd[OM_DEFAULT_PLUGIN_PASSWD_SIZE] ;

         ossMemset( passwd, 0, OM_DEFAULT_PLUGIN_PASSWD_SIZE ) ;
         authTool.generateRandomVisualString( passwd,
                                              OM_DEFAULT_PLUGIN_PASSWD_LEN ) ;

         rc = authTool.createOmsvcDefaultUsr( passwd,
                                              OM_DEFAULT_PLUGIN_PASSWD_LEN ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to create om user: rc=%d", rc ) ;
            goto error ;
         }

         _usrPluginPasswd = passwd ;
      }

      // SYSDEPLOY.SYSSETTINGS
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_SETTINGS, OM_DEPLOY_SETTINGS_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_SETTINGS,
                                         OM_CS_DEPLOY_CL_SETTINGSIDX ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.setSetting( OM_SETTINGS_CONFIG_MAXEXECTIME, REST_TIMEOUT ) ;
      if ( rc )
      {
         goto error ;
      }

      // SYSDEPLOY.SYSHISTORY
      rc = dbTool.createCollection( OM_CS_DEPLOY_CL_HISTORY, OM_DEPLOY_HISTORY_CLUID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_HISTORY,
                                         OM_CS_DEPLOY_CL_HISTORYIDX1 ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_getBussinessInfo( const string &businessName,
                                        string &businessType,
                                        string &clusterName,
                                        string &deployMode )
   {
      INT32 rc = SDB_OK ;
      BSONObj empty ;
      BSONObj matcher ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID   = -1 ;

      matcher = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, empty, matcher, empty,
                     empty, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "query table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "get record failed:table=%s,rc=%d",
                      OM_CS_DEPLOY_CL_BUSINESS, rc ) ;

         BSONObj result( buffObj.data() ) ;
         businessType = result.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         clusterName  = result.getStringField( OM_BUSINESS_FIELD_CLUSTERNAME ) ;
         deployMode   = result.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;
      }

      if ( "" == businessType || "" == clusterName || "" == deployMode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "business info is invalid:name=%s,type=%s,cluster=%s,"
                 "deployMode=%s",
                 businessName.c_str(), businessType.c_str(),
                 clusterName.c_str(), deployMode.c_str() ) ;
         goto error ;
      }
   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::getBizHostInfo( const string &businessName,
                                     list <string> &hostsList )
   {
      INT32 rc = SDB_OK ;
      BSONObj empty ;
      BSONObj matcher ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID   = -1 ;

      matcher = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, empty, matcher, empty,
                     empty, 0, cb, 0, -1, pdmsCB, pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "query table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
      while( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }

         PD_RC_CHECK( rc, PDERROR, "get record failed:table=%s,rc=%d",
                      OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;

         BSONObj result( buffObj.data() ) ;
         hostsList.push_back( result.getStringField(
                                             OM_CONFIGURE_FIELD_HOSTNAME ) ) ;
      }

      if ( hostsList.size() == 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "business have not host info:business=%s",
                 businessName.c_str() ) ;
         goto error ;
      }
   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_appendBusinessInfo( const string &businessName,
                                          const string &businessType,
                                          const string &clusterName,
                                          const string &deployMode )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      INT32 rc     = SDB_OK ;
      BSONObj matcher ;
      BSONObj tmp ;
      BSONObj updator ;
      BSONObj hint ;
      matcher = BSON( OM_CONFIGURE_FIELD_BUSINESSNAME << businessName ) ;
      tmp     = BSON( OM_CONFIGURE_FIELD_BUSINESSTYPE << businessType
                      << OM_CONFIGURE_FIELD_CLUSTERNAME << clusterName
                      << OM_CONFIGURE_FIELD_DEPLOYMODE << deployMode ) ;
      updator = BSON( "$set" << tmp ) ;
      rc = rtnUpdate( OM_CS_DEPLOY_CL_CONFIGURE, matcher, updator, hint,
                      0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "update table failed:table=%s,business=%s,"
                   "rc=%d", OM_CS_DEPLOY_CL_CONFIGURE, businessName.c_str(),
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::appendBizHostInfo( const string &businessName,
                                        list <string> &hostsList )
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      INT32 rc     = SDB_OK ;
      BSONObj matcher ;
      BSONObj tmp ;
      BSONObj updator ;
      BSONObj hint ;
      BSONArrayBuilder arrayBuilder ;
      list <string>::iterator iter ;
      matcher = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;

      iter = hostsList.begin() ;
      while ( iter != hostsList.end() )
      {
         BSONObj oneHost = BSON( OM_CONFIGURE_FIELD_HOSTNAME << *iter ) ;
         arrayBuilder.append( oneHost ) ;
         iter++ ;
      }

      tmp     = BSON( OM_BUSINESS_FIELD_LOCATION << arrayBuilder.arr() ) ;
      updator = BSON( "$set" << tmp ) ;
      rc = rtnUpdate( OM_CS_DEPLOY_CL_BUSINESS, matcher, updator, hint,
                      0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "update table failed:table=%s,business=%s,"
                   "updator=%s,rc=%d", OM_CS_DEPLOY_CL_BUSINESS,
                   businessName.c_str(), updator.toString().c_str(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_updateConfTable()
   {
      INT32 rc = SDB_OK ;
      BSONObj empty ;
      BSONObj isNull ;
      BSONObj matcher ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID   = -1 ;

      isNull  = BSON( "$isnull" << 1 ) ;
      matcher = BSON( OM_CONFIGURE_FIELD_BUSINESSTYPE << isNull ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, empty, matcher, empty,
                     empty, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "query table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
      while ( TRUE )
      {
         string businessName ;
         string businessType ;
         string clusterName ;
         string deployMode ;
         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_RC_CHECK( rc, PDERROR, "get record failed:table=%s,rc=%d",
                         OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         }

         BSONObj result( buffObj.data() ) ;
         businessName = result.getStringField(OM_CONFIGURE_FIELD_BUSINESSNAME) ;
         rc = _getBussinessInfo( businessName, businessType, clusterName, deployMode ) ;
         PD_RC_CHECK( rc, PDERROR, "get business info failed:business=%s,rc=%d",
                      businessName.c_str(), rc ) ;
         rc = _appendBusinessInfo( businessName, businessType, clusterName, deployMode ) ;
         PD_RC_CHECK( rc, PDERROR, "append business info failed:business=%s,"
                      "rc=%d", businessName.c_str(), rc ) ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_updateBusinessTable()
   {
      INT32 rc = SDB_OK ;
      BSONArrayBuilder arrayBuilder ;
      BSONObj empty ;
      BSONObj isNull ;
      BSONObj discover ;
      BSONObj matcher ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      SINT64 contextID   = -1 ;
      list <string> hostsList ;

      isNull   = BSON( "$isnull" << 1 ) ;
      discover = BSON( OM_BUSINESS_FIELD_ADDTYPE
                       << OM_BUSINESS_ADDTYPE_DISCOVERY ) ;
      arrayBuilder.append( discover ) ;
      matcher  = BSON( OM_BUSINESS_FIELD_LOCATION << isNull
                       << "$not" << arrayBuilder.arr() ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, empty, matcher, empty,
                     empty, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "query table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
      while ( TRUE )
      {
         string businessName ;
         string businessType ;
         string clusterName ;
         rtnContextBuf buffObj ;
         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_RC_CHECK( rc, PDERROR, "get record failed:table=%s,rc=%d",
                         OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         }

         BSONObj result( buffObj.data() ) ;
         businessName = result.getStringField( OM_BUSINESS_FIELD_NAME ) ;
         rc = getBizHostInfo( businessName, hostsList ) ;
         PD_RC_CHECK( rc, PDERROR, "get business host info failed:business=%s,"
                      "rc=%d", businessName.c_str(), rc ) ;
         rc = appendBizHostInfo( businessName, hostsList ) ;
         PD_RC_CHECK( rc, PDERROR, "append business host info failed:"
                      "business=%s,rc=%d", businessName.c_str(), rc ) ;
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_appendClusterGrant( const string& clusertName,
                                          const string& grantName,
                                          BOOLEAN privilege )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID   = -1 ;
      BOOLEAN isFind     = FALSE ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      BSONObj sort ;
      BSONObj hint ;

      {
         BSONObj selector ;
         BSONObj matcher = BSON( OM_CLUSTER_FIELD_NAME << clusertName <<
               OM_CLUSTER_FIELD_GRANTCONF "." OM_CLUSTER_FIELD_GRANTNAME <<
               grantName ) ;

         rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, sort,
                        hint, 0, cb, 0, 1, pdmsCB, pRtnCB, contextID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         while ( TRUE )
         {
            rtnContextBuf buffObj ;

            rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }

               PD_LOG( PDERROR, "get record failed:table=%s,rc=%d",
                       OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
               goto error ;
            }
            isFind = TRUE ;
         }
      }

      if ( FALSE == isFind )
      {
         BSONObj updator ;
         BSONObj matcher = BSON( OM_CLUSTER_FIELD_NAME << clusertName ) ;
         BSONObjBuilder grantInfoBuilder ;

         grantInfoBuilder.append( OM_CLUSTER_FIELD_GRANTNAME, grantName ) ;
         grantInfoBuilder.appendBool( OM_CLUSTER_FIELD_PRIVILEGE, privilege ) ;

         updator = BSON( "$push" <<
              BSON( OM_CLUSTER_FIELD_GRANTCONF << grantInfoBuilder.obj() ) ) ;

         rc = rtnUpdate( OM_CS_DEPLOY_CL_CLUSTER, matcher, updator, hint,
                      0, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "update table failed:table=%s,updator=%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, updator.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;

   }

   INT32 _omManager::_updateClusterTable()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID   = -1 ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      BSONObj sort ;
      BSONObj hint ;
      BSONObj selector ;
      BSONObj matcher ;
      set<string> clusterList ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, sort,
                     hint, 0, cb, 0, -1, pdmsCB, pRtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "get record failed:table=%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         {
            BSONObj result( buffObj.data() ) ;
            string clusterName ;

            clusterName = result.getStringField( OM_CLUSTER_FIELD_NAME ) ;
            clusterList.insert( clusterName ) ;
         }
      }

      for ( set<string>::iterator iter = clusterList.begin();
               iter != clusterList.end(); ++iter )
      {
         string clusterName = *iter ;

         //HostFile
         rc = _appendClusterGrant( clusterName,
                                   OM_CLUSTER_FIELD_HOSTFILE, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to add cluster grant config %s, rc=%d",
                    OM_CLUSTER_FIELD_HOSTFILE, rc ) ;
            goto error ;
         }

         //RootUser
         rc = _appendClusterGrant( clusterName,
                                   OM_CLUSTER_FIELD_ROOTUSER, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to add cluster grant config %s, rc=%d",
                    OM_CLUSTER_FIELD_ROOTUSER, rc ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_appendHostPackage( const string &hostName,
                                         const BSONObj &packageInfo )
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      BSONObj matcher ;
      BSONObj updator ;
      BSONObj hint ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName ) ;

      updator = BSON( "$push" <<
                           BSON( OM_HOST_FIELD_PACKAGES << packageInfo ) ) ;

      rc = rtnUpdate( OM_CS_DEPLOY_CL_HOST, matcher, updator, hint,
                      0, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table failed:table=%s,updator=%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, updator.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omManager::_getOMVersion( string &version )
   {
      INT32 major        = 0 ;
      INT32 minor        = 0 ;

      stringstream stream ;

      ossGetVersion ( &major, &minor, NULL, NULL, NULL, NULL ) ;
      stream << major << "." << minor ;
      version = stream.str() ;
   }

   INT32 _omManager::_updateHostTable()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID   = -1 ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pdmsCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRtnCB = pKRCB->getRTNCB() ;
      BSONObj sort ;
      BSONObj hint ;
      BSONObj selector ;
      BSONObj matcher ;
      string omVersion ;

      _getOMVersion( omVersion ) ;

      matcher = BSON( OM_HOST_FIELD_PACKAGES << BSON( "$exists" << 0 ) ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, sort,
                     hint, 0, cb, 0, -1, pdmsCB, pRtnCB, contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, pRtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "get record failed:table=%s,rc=%d",
                    OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         {
            BSONObj package ;
            BSONObj hostInfo( buffObj.data() ) ;
            BSONObj oma = hostInfo.getObjectField( OM_HOST_FIELD_OMA ) ;
            string version = oma.getStringField( OM_HOST_FIELD_OM_VERSION ) ;
            string hostName = hostInfo.getStringField( OM_HOST_FIELD_NAME ) ;
            string installPath = hostInfo.getStringField(
                                                   OM_HOST_FIELD_INSTALLPATH ) ;

            if ( version.empty() )
            {
               version = omVersion ;
            }

            package = BSON(
                           OM_HOST_FIELD_PACKAGENAME << OM_BUSINESS_SEQUOIADB <<
                           OM_HOST_FIELD_INSTALLPATH << installPath <<
                           OM_HOST_FIELD_VERSION << version ) ;

            rc = _appendHostPackage( hostName, package ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to append host applications: rc=%d",
                       rc ) ;
               goto error ;
            }
         }
      }

   done:
      if ( -1 != contextID )
      {
         pRtnCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_updateAuthTable()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID   = -1 ;
      pmdEDUCB *cb       = pmdGetThreadEDUCB() ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      _SDB_DMSCB *pDMSCB = pKRCB->getDMSCB() ;
      _SDB_RTNCB *pRTNCB = pKRCB->getRTNCB() ;
      BSONObj selector ;
      BSONObj condition ;
      BSONObj order ;
      BSONObj hint ;
      omDatabaseTool dbTool( cb ) ;

      try
      {
         condition = BSON( OM_AUTH_FIELD_ENCRYPTION <<
                                 BSON( "$exists" << 0 ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS_AUTH, selector, condition, order,
                     hint, 0, cb, 0, -1, pDMSCB, pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "query table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "get record failed:table=%s,rc=%d",
                    OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
            goto error ;
         }

         try
         {
            BSONObj authInfo( buffObj.data() ) ;
            string businessName = authInfo.getStringField(
                                          OM_AUTH_FIELD_BUSINESS_NAME ) ;
            string user = authInfo.getStringField( OM_AUTH_FIELD_USER ) ;
            string passwd = authInfo.getStringField( OM_AUTH_FIELD_PASSWD ) ;

            rc = dbTool.upsertAuth( businessName, user, passwd ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to update table:table=%s,rc=%d",
                       OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         pRTNCB->contextDelete ( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_updatePluginIndex()
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      omDatabaseTool dbTool( cb ) ;

      dbTool.removePlugin( "SequoiaSQL-PostgreSQL",
                           OM_BUSINESS_SEQUOIASQL_POSTGRESQL ) ;

      rc = dbTool.createCollectionIndex( OM_CS_DEPLOY_CL_PLUGINS,
                                         OM_CS_DEPLOY_CL_PLUGINSIDX1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to create index: name=%s, rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
         goto error ;
      }

      rc = dbTool.removeCollectionIndex( OM_CS_DEPLOY_CL_PLUGINS,
                                         OM_CS_DEPLOY_CL_PLUGINSIDX2 ) ;
      if ( SDB_IXM_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "failed to drop index: name=%s, rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_updateTable()
   {
      INT32 rc = SDB_OK ;
      //OM_CS_DEPLOY_CL_CONFIGURE
      rc = _updateConfTable() ;
      PD_RC_CHECK( rc, PDERROR, "update table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;

      rc = _updateBusinessTable() ;
      PD_RC_CHECK( rc, PDERROR, "update table failed:table=%s,rc=%d",
                   OM_CS_DEPLOY_CL_BUSINESS, rc ) ;

      rc = _updateClusterTable() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      rc = _updateHostTable() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      rc = _updatePluginIndex() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table index failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_PLUGINS, rc ) ;
         goto error ;
      }

      rc = _updateAuthTable() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "update table failed:table=%s,rc=%d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::active ()
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = PMD_INVALID_EDUID ;

      // start om manager edu
      rc = pEDUMgr->startEDU( EDU_TYPE_OMMGR, (_pmdObjBase*)this, &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start OM Manager edu, rc: %d", rc ) ;
      // wait attach
      rc = _attachEvent.wait( OM_WAIT_CB_ATTACH_TIMEOUT ) ;
      PD_RC_CHECK( rc, PDERROR, "Wait OM Manager edu attach failed, rc: %d",
                   rc ) ;

      // start om net
      rc = pEDUMgr->startEDU( EDU_TYPE_OMNET, (netRouteAgent*)&_netAgent,
                              &eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start om net, rc: %d", rc ) ;

      /// start om strategy observer job
      rc = omStartStrategyObserverJob() ;
      if ( rc )
      {
         goto error ;
      }

      // start update plugin user timer (24 hour)
      _updateTimestamp = (INT64)time( NULL ) ;
      _updatePluinUsrTimer = setTimer(
                              OM_UPDATE_PLUGIN_PASSWD_TIMEOUT * OSS_ONE_SEC ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::deactive ()
   {
      _netAgent.shutdownListen() ;
      // stop io
      _netAgent.stop() ;

      return SDB_OK ;
   }

   INT32 _omManager::fini ()
   {
      pmdGetKRCB()->unregEventHandler( this ) ;
      _rsManager.fini() ;
      omGetStrategyMgr()->fini() ;

      _mapID2Host.clear() ;
      _mapHost2ID.clear() ;

      return SDB_OK ;
   }

   void _omManager::attachCB( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
      _rsManager.registerEDU( cb ) ;
      _msgHandler.attach( cb ) ;
      _timerHandler.attach( cb ) ;
      _attachEvent.signalAll() ;
   }

   void _omManager::detachCB( _pmdEDUCB *cb )
   {
      _msgHandler.detach() ;
      _timerHandler.detach() ;
      _rsManager.unregEUD( cb ) ;
      _pEDUCB = NULL ;
   }

   UINT32 _omManager::setTimer( UINT32 milliSec )
   {
      UINT32 timeID = NET_INVALID_TIMER_ID ;
      _netAgent.addTimer( milliSec, &_timerHandler, timeID ) ;
      return timeID ;
   }

   void _omManager::killTimer( UINT32 timerID )
   {
      _netAgent.removeTimer( timerID ) ;
   }

   void _omManager::onTimer( UINT64 timerID, UINT32 interval )
   {
      if( _updatePluinUsrTimer == timerID )
      {
         _updatePluginPasswd() ;
      }
   }

   /** \fn INT32 _omManager::md5Authenticate( const BSONObj &obj, pmdEDUCB *cb )
       \brief Authentication using MD5.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _omManager::md5Authenticate( const BSONObj &obj, pmdEDUCB *cb,
                                      BSONObj &outUserObj )
   {
      INT32 rc = SDB_OK ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      if ( NULL == pAuthCB )
      {
         SDB_ASSERT( FALSE, "Auth cb can not be null" ) ;
         goto done ;
      }

      rc = pAuthCB->md5Authenticate( obj, cb, &outUserObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /** \fn INT32 _omManager::SCRAMSHAAuthenticate( const BSONObj &obj,
                                                   pmdEDUCB *cb,
                                                   BSONObj *pOutUserObj )
       \brief Authentication using SCRAM-SHA256.
       \param [in] obj Bson data in the message sent by the client.
       \param [in] cb  Pmd EDU cb.
       \param [out] pOutUserObj Bson data in the message we need to send to
                    the client.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _omManager::SCRAMSHAAuthenticate( const BSONObj &obj,
                                           _pmdEDUCB *cb,
                                           BSONObj &outUserObj )
   {
      INT32 rc = SDB_OK ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      if ( NULL == pAuthCB )
      {
         SDB_ASSERT( FALSE, "Auth cb can not be null" ) ;
         goto done ;
      }

      rc = pAuthCB->SCRAMSHAAuthenticate( obj, cb, &outUserObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::authUpdatePasswd( string user, string oldPasswd,
                                       string newPasswd, pmdEDUCB *cb )
   {
      INT32 rc            = SDB_OK ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      if ( NULL == pAuthCB )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pAuthCB->updatePasswd( user, oldPasswd, newPasswd, cb ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   netRouteAgent* _omManager::getRouteAgent()
   {
      return &_netAgent ;
   }

   MsgRouteID _omManager::_incNodeID()
   {
      ++_hwRouteID.columns.nodeID ;
      if ( 0 == _hwRouteID.columns.nodeID )
      {
         _hwRouteID.columns.nodeID = 1 ;
         ++_hwRouteID.columns.groupID ;
      }
      return _hwRouteID ;
   }

   MsgRouteID _omManager::updateAgentInfo( const string &host,
                                           const string &service )
   {
      MsgRouteID nodeID ;
      ossScopedLock lock( &_omLatch, EXCLUSIVE ) ;
      MAP_HOST2ID_IT it = _mapHost2ID.find( host ) ;
      if ( it != _mapHost2ID.end() )
      {
         omAgentInfo &info = it->second ;
         nodeID.value = info._id ;
         _netAgent.updateRoute( nodeID, host.c_str(), service.c_str() ) ;
         info._host = host ;
         info._service = service ;
      }
      else
      {
         nodeID = _incNodeID() ;
         omAgentInfo &info = _mapHost2ID[ host ] ;
         info._id = nodeID.value ;
         info._host = host ;
         info._service = service ;
         _mapID2Host[ info._id ] = &info ;
         _netAgent.updateRoute( nodeID, host.c_str(), service.c_str() ) ;
      }

      return nodeID ;
   }

   MsgRouteID _omManager::getAgentIDByHost( const string &host )
   {
      MsgRouteID nodeID ;
      nodeID.value = MSG_INVALID_ROUTEID ;
      ossScopedLock lock( &_omLatch, SHARED ) ;
      MAP_HOST2ID_IT it = _mapHost2ID.find( host ) ;
      if ( it != _mapHost2ID.end() )
      {
         nodeID.value = it->second._id ;
      }
      return nodeID ;
   }

   INT32 _omManager::getHostInfoByID( MsgRouteID routeID, string &host,
                                      string &service )
   {
      INT32 rc = SDB_OK ;
      omAgentInfo *pInfo = NULL ;

      ossScopedLock lock( &_omLatch, SHARED ) ;
      MAP_ID2HOSTPTR_IT it = _mapID2Host.find( routeID.value ) ;
      if ( it != _mapID2Host.end() )
      {
         pInfo = it->second ;
         host = pInfo->_host ;
         service = pInfo->_service ;
      }
      else
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
      }

      return rc ;
   }

   void _omManager::delAgent( const string &host )
   {
      ossScopedLock lock( &_omLatch, EXCLUSIVE ) ;
      MAP_HOST2ID_IT it = _mapHost2ID.find( host ) ;
      if ( it != _mapHost2ID.end() )
      {
         MsgRouteID nodeID ;
         nodeID.value = it->second._id ;
         _mapID2Host.erase( it->second._id ) ;
         _netAgent.delRoute( nodeID ) ;
         _mapHost2ID.erase( it ) ;
      }
   }

   void _omManager::delAgent( MsgRouteID routeID )
   {
      ossScopedLock lock( &_omLatch, EXCLUSIVE ) ;
      MAP_ID2HOSTPTR_IT it = _mapID2Host.find( routeID.value ) ;
      if ( it != _mapID2Host.end() )
      {
         MsgRouteID nodeID ;
         nodeID.value = it->first ;
         string host = it->second->_host ;
         _netAgent.delRoute( nodeID ) ;
         _mapID2Host.erase( it ) ;
         _mapHost2ID.erase( host ) ;
      }
   }

   INT32 _omManager::_updatePluginPasswd()
   {
      INT32 rc = SDB_OK ;
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      SDB_AUTHCB *pAuthCB = pmdGetKRCB()->getAuthCB() ;
      omAuthTool authTool( cb, pAuthCB ) ;
      CHAR passwd[OM_DEFAULT_PLUGIN_PASSWD_SIZE] ;

      ossMemset( passwd, 0, OM_DEFAULT_PLUGIN_PASSWD_SIZE ) ;
      authTool.generateRandomVisualString( passwd,
                                           OM_DEFAULT_PLUGIN_PASSWD_LEN ) ;

      rc = authTool.createPluginUsr( passwd, OM_DEFAULT_PLUGIN_PASSWD_LEN ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "failed to update plugin passwd: rc=%d", rc ) ;
         goto error ;
      }

      _usrPluginPasswd = passwd ;

      _updateTimestamp = (INT64)time( NULL ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _omManager::getPluginPasswd( string &passwd )
   {
      passwd = _usrPluginPasswd ;
   }

   void _omManager::getUpdatePluginPasswdTimeDiffer( INT64 &differ )
   {
      differ = (INT64)time( NULL ) - _updateTimestamp ;
      differ = OM_UPDATE_PLUGIN_PASSWD_TIMEOUT - differ ;

      if ( 0 >= differ )
      {
         differ = OM_UPDATE_PLUGIN_PASSWD_TIMEOUT ;
      }
   }

   void _omManager::_checkTaskTimeout( const BSONObj &task )
   {
      BSONElement element ;
      time_t now ;
      unsigned long long nowMills ;
      unsigned long long lastMills ;

      element   = task.getField( OM_TASKINFO_FIELD_END_TIME ) ;
      lastMills = element.timestampTime() ;

      now = time( NULL ) ;
      nowMills = now * 1000 ;

      if ( ( nowMills - lastMills ) > 10 * 60 * OSS_ONE_SEC )
      {
         INT32 rc = SDB_OK ;
         BSONElement taskIDEle ;
         INT64 taskID ;
         const CHAR* hostName = pmdGetKRCB()->getHostName();
         string localAgentHost = hostName ;
         string localAgentPort = this->getLocalAgentPort() ;

         omInterruptTaskCommand interruptTask( NULL, NULL, NULL, NULL,
                                               localAgentHost, localAgentPort,
                                               "" ) ;

         interruptTask.init( pmdGetThreadEDUCB() ) ;
         taskIDEle = task.getField( OM_TASKINFO_FIELD_TASKID ) ;
         taskID    = taskIDEle.Long() ;
         rc = interruptTask.notifyAgentInteruptTask( taskID ) ;
         if ( SDB_OK != rc && SDB_OM_TASK_NOT_EXIST != rc )
         {
            PD_LOG( PDERROR, "notify agent interrupt task failed:taskID="
                    OSS_LL_PRINT_FORMAT",rc=%d", taskID, rc ) ;
            goto done ;
         }

         interruptTask.updateTaskStatus( taskID, OM_TASK_STATUS_CANCEL,
                                         SDB_TIMEOUT ) ;
      }

   done:
      pmdGetThreadEDUCB()->resetInfo( EDU_INFO_ERROR ) ;
      return ;
   }

   string _omManager::getLocalAgentPort()
   {
      return _localAgentPort ;
   }

   void _omManager::_readAgentPort()
   {
      INT32 rc = SDB_OK ;
      CHAR conf[OSS_MAX_PATHSIZE + 1] = { 0 } ;
      po::options_description desc ( "Config options" ) ;
      po::variables_map vm ;
      CHAR hostport[OSS_MAX_HOSTNAME + 6] = { 0 } ;
      _localAgentPort = boost::lexical_cast<string>( SDBCM_DFT_PORT ) ;
      rc = ossGetHostName( hostport, OSS_MAX_HOSTNAME ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "get host name failed:rc=%d", rc ) ;
         goto error ;
      }

      ossStrncat ( hostport, SDBCM_CONF_PORT, ossStrlen(SDBCM_CONF_PORT) ) ;

      desc.add_options()
         (SDBCM_CONF_DFTPORT, po::value<string>(), "sdbcm default "
         "listening port")
         (hostport, po::value<string>(), "sdbcm specified listening port")
      ;

      rc = ossGetEWD ( conf, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get excutable file's working "
                  "directory" ) ;
         goto error ;
      }

      if ( ( ossStrlen ( conf ) + ossStrlen ( SDBCM_CONF_PATH_FILE ) + 2 ) >
           OSS_MAX_PATHSIZE )
      {
         PD_LOG ( PDERROR, "Working directory too long" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossStrncat( conf, OSS_FILE_SEP, 1 );
      ossStrncat( conf, SDBCM_CONF_PATH_FILE,
                  ossStrlen( SDBCM_CONF_PATH_FILE ) );
      rc = utilReadConfigureFile ( conf, desc, vm ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to read configure file, rc = %d", rc ) ;
         goto error ;
      }
      else if ( vm.count( hostport ) )
      {
         _localAgentPort = vm[hostport].as<string>() ;
      }
      else if ( vm.count( SDBCM_CONF_DFTPORT ) )
      {
         _localAgentPort = vm[SDBCM_CONF_DFTPORT].as<string>() ;
      }
      else
      {
         _localAgentPort = boost::lexical_cast<string>( SDBCM_DFT_PORT ) ;
      }

   done:
      return ;
   error:
      goto done ;
   }

   void _omManager::_createVersionFile()
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string versionInfo ;
      OSSFILE pFile ;
      CHAR versionFile[ OSS_MAX_PATHSIZE + 1 ] ;
      string wwwPath = pmdGetOptionCB()->getWWWPath() ;
      rc = ossGetEWD( versionFile, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "get current path failed:rc=%d", rc ) ;
         goto error ;
      }

      ossSnprintf( versionFile, OSS_MAX_PATHSIZE, wwwPath.c_str() ) ;
      utilCatPath( versionFile, OSS_MAX_PATHSIZE, OM_PATH_CONFIG ) ;
      utilCatPath( versionFile, OSS_MAX_PATHSIZE, OM_PATH_VERSION ) ;

      rc = ossOpen( versionFile, OSS_REPLACE|OSS_READWRITE,
                    OSS_RU|OSS_WU|OSS_RG|OSS_RO, pFile ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "open file failed:file=%s,rc=%d", versionFile,
                 rc ) ;
         goto error ;
      }

      {
         INT32 ver = 0 ;
         INT32 subVer = 0 ;
         INT32 fix = 0 ;
         INT32 release = 0 ;
         const CHAR *pBuild = NULL ;
         const CHAR *pGitVer = NULL ;
         ossGetVersion( &ver, &subVer, &fix, &release, &pBuild, &pGitVer ) ;
         ss << "{\n  version:\"" << ver << "." << subVer ;
         if ( fix > 0 )
         {
            ss << "." << fix ;
         }
         ss << "\",\n  buildTime:\"" << pBuild << "\",\n  release:\""
            << release << "\"" ;
         if ( pGitVer )
         {
            ss << ",\n  git version:\"" << pGitVer << "\"" ;
         }
         ss << "\n}";
      }

      versionInfo = ss.str() ;
      rc = ossWriteN( &pFile, versionInfo.c_str(), versionInfo.length() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "write file failed:file=%s,content=%s,rc=%d",
                 versionFile, versionInfo.c_str(), rc ) ;
         goto error ;
      }

      ossClose( pFile ) ;

   done:
      return ;
   error:
      goto done ;
   }

   BOOLEAN _omManager::_isCommand( const CHAR *pCheckName )
   {
      if ( pCheckName && '$' == pCheckName[0] )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   void _omManager::_sendResVector2Agent( NET_HANDLE handle,
                                          MsgHeader *pSrcMsg,
                                          INT32 flag,
                                          vector < BSONObj > &objs,
                                          INT64 contextID )
   {
      INT32 rc          = SDB_OK ;
      CHAR *pbuffer     = NULL ;
      INT32 bufferSize  = 0 ;
      MsgOpReply *reply = NULL ;

      rc = msgBuildReplyMsg( &pbuffer, &bufferSize,
                             pSrcMsg->opCode, flag, contextID,
                             0, objs.size(), pSrcMsg->requestID,
                             &objs ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "build reply msg failed:rc=%d", rc ) ;
         goto error ;
      }

      reply = ( MsgOpReply *) pbuffer ;
      reply->header.TID = pSrcMsg->TID ;
      rc = _netAgent.syncSend( handle, (MsgHeader *)pbuffer ) ;
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "send response to agent failed:rc=%d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != pbuffer )
      {
         SDB_OSS_FREE( pbuffer ) ;
      }
      return ;
   error:
      goto done ;
   }

   void _omManager::_sendRes2Agent( NET_HANDLE handle,
                                    MsgHeader *pSrcMsg,
                                    INT32 flag,
                                    const rtnContextBuf &buffObj,
                                    INT64 contextID )
   {
      MsgOpReply reply ;
      INT32 rc                   = SDB_OK ;
      const CHAR *pBody          = buffObj.data() ;
      INT32 bodyLen              = buffObj.size() ;
      reply.header.messageLength = sizeof( MsgOpReply ) + bodyLen ;
      reply.header.opCode        = MAKE_REPLY_TYPE( pSrcMsg->opCode ) ;
      reply.header.TID           = pSrcMsg->TID ;
      reply.header.routeID.value = 0 ;
      reply.header.requestID     = pSrcMsg->requestID ;
      reply.header.globalID      = pSrcMsg->globalID ;
      reply.contextID            = contextID ;
      reply.flags                = flag ;
      reply.startFrom            = 0 ;
      reply.numReturned          = buffObj.recordNum() ;

      if ( bodyLen > 0 )
      {
         rc = _netAgent.syncSend ( handle, (MsgHeader *)( &reply ),
                                   (void*)pBody, bodyLen ) ;
      }
      else
      {
         rc = _netAgent.syncSend ( handle, (MsgHeader *)( &reply ) ) ;
      }

      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "send response to agent failed:rc=%d", rc ) ;
      }
   }

   void _omManager::_sendRes2Agent( NET_HANDLE handle,
                                    MsgHeader *pSrcMsg,
                                    INT32 flag,
                                    const BSONObj &obj,
                                    INT64 contextID )
   {
      MsgOpReply reply ;
      INT32 rc                   = SDB_OK ;
      reply.header.opCode        = MAKE_REPLY_TYPE( pSrcMsg->opCode ) ;
      reply.header.TID           = pSrcMsg->TID ;
      reply.header.routeID.value = 0 ;
      reply.header.requestID     = pSrcMsg->requestID ;
      reply.contextID            = contextID ;
      reply.flags                = flag ;
      reply.startFrom            = 0 ;
      reply.header.globalID      = pSrcMsg->globalID ;

      if ( !obj.isEmpty() )
      {
         const CHAR *pBody          = obj.objdata() ;
         INT32 bodyLen              = obj.objsize() ;
         reply.header.messageLength = sizeof( MsgOpReply ) + bodyLen ;
         reply.numReturned          = 1 ;
         rc = _netAgent.syncSend ( handle, (MsgHeader *)( &reply ),
                                   (void*)pBody, bodyLen ) ;
      }
      else
      {
         reply.header.messageLength = sizeof( MsgOpReply ) ;
         reply.numReturned          = 0 ;
         rc = _netAgent.syncSend ( handle, (MsgHeader *)( &reply ) ) ;
      }

      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "send response to agent failed:rc=%d", rc ) ;
      }
   }

   INT32 _omManager::_onAgentUpdateTaskReq( NET_HANDLE handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      INT32 flags                = 0 ;
      const CHAR *pCollectionName   = NULL ;
      const CHAR *pQuery         = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderByBuffer = NULL ;
      const CHAR *pHintBuffer    = NULL ;
      SINT64 numToSkip           = -1 ;
      SINT64 numToReturn         = -1 ;
      BSONObj response ;

      rc = msgExtractQuery ( (const CHAR *)pMsg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQuery,
                             &pFieldSelector, &pOrderByBuffer, &pHintBuffer ) ;
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "extract omAgent's command msg failed:rc=%d",
                     rc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      PD_LOG( PDEVENT, "receive agent's command:%s", pCollectionName ) ;
      if ( _isCommand( pCollectionName ) )
      {
         if ( ossStrcasecmp( OM_AGENT_UPDATE_TASK,
                             ( pCollectionName + 1 ) ) == 0 )
         {
            BSONObj updateReq( pQuery ) ;
            BSONObj taskUpdateInfo ;
            INT64 taskID ;

            BSONElement ele = updateReq.getField( OM_TASKINFO_FIELD_TASKID ) ;
            taskID = ele.numberLong() ;

            BSONObj filter = BSON( OM_TASKINFO_FIELD_TASKID << 1 ) ;
            taskUpdateInfo = updateReq.filterFieldsUndotted( filter, false ) ;
            rc = _taskManager->updateTask( taskID, taskUpdateInfo) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "update task failed:rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "unreconigzed agent request:command=%s",
                        pCollectionName ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "unreconigzed agent request:command=%s",
                     pCollectionName ) ;
         goto error ;
      }

   done:
      if ( SDB_OK == rc )
      {
         _sendRes2Agent( handle, pMsg, rc, response ) ;
      }
      else
      {
         string errorInfo = _pEDUCB->getInfo( EDU_INFO_ERROR ) ;
         response = BSON( OP_ERR_DETAIL << errorInfo ) ;
         _sendRes2Agent( handle, pMsg, rc, response ) ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_processQueryMsg( MsgHeader *pMsg,
                                       rtnContextBuf &buf,
                                       INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      INT32 flags                   = 0 ;
      const CHAR *pCollectionName   = NULL ;
      const CHAR *pQuery            = NULL ;
      const CHAR *pFieldSelector    = NULL ;
      const CHAR *pOrderByBuffer    = NULL ;
      const CHAR *pHintBuffer       = NULL ;
      SINT64 numToSkip              = -1 ;
      SINT64 numToReturn            = -1 ;

      // extract command
      rc = msgExtractQuery ( (const CHAR *)pMsg, &flags, &pCollectionName,
                             &numToSkip, &numToReturn, &pQuery,
                             &pFieldSelector, &pOrderByBuffer,
                             &pHintBuffer ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse the request(rc=%d)!",
                   rc ) ;

      if ( _isCommand( pCollectionName ) )
      {
         rc = SDB_RTN_CMD_NO_NODE_AUTH ;
         goto error ;
      }
      else
      {
         try
         {
            BSONObj matcher ( pQuery ) ;
            BSONObj selector ( pFieldSelector ) ;
            BSONObj orderBy ( pOrderByBuffer ) ;
            BSONObj hint ( pHintBuffer ) ;

            rc = rtnQuery( pCollectionName, selector, matcher, orderBy,
                           hint, flags, _pEDUCB, numToSkip,
                           numToReturn, _pDmsCB, _pRtnCB, contextID,
                           NULL, TRUE ) ;
            if ( rc )
            {
               if ( rc != SDB_DMS_EOC )
               {
                  PD_LOG ( PDERROR, "Failed to query on collection[%s], "
                           "rc: %d", pCollectionName, rc ) ;
               }
               goto error ;
            }
            else if ( flags & FLG_QUERY_WITH_RETURNDATA )
            {
               rtnContextDump contextDump( 0, _pEDUCB->getID() ) ;
               rc = contextDump.open( BSONObj(), BSONObj(), -1, 0 ) ;
               PD_RC_CHECK( rc, PDERROR, "Open dump context failed, rc: %d",
                            rc ) ;

               while ( TRUE )
               {
                  rc = rtnGetMore( contextID, -1, buf, _pEDUCB, _pRtnCB ) ;
                  if ( rc )
                  {
                     contextID = -1 ;
                     if ( SDB_DMS_EOC != rc )
                     {
                        PD_LOG( PDERROR, "Get more failed, rc: %d", rc ) ;
                        goto error ;
                     }
                     rc = SDB_OK ;
                     break ;
                  }
                  // add to dump context
                  rc = contextDump.appendObjs( buf.data(), buf.size(),
                                               buf.recordNum(), TRUE ) ;
                  PD_RC_CHECK( rc, PDERROR, "Append objs to dump context "
                               "failed, rc: %d", rc ) ;
               }

               rc = contextDump.getMore( -1, buf, _pEDUCB ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               PD_RC_CHECK( rc, PDERROR, "Get more from dump context failed, "
                            "rc: %d", rc ) ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Occur exception: %s", e.what () ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_processGetMoreMsg( MsgHeader *pMsg,
                                         rtnContextBuf &buf,
                                         INT64 &contextID )
   {
      INT32 rc         = SDB_OK ;
      INT32 numToRead  = 0 ;
      BOOLEAN rtnDel   = TRUE ;

      rc = msgExtractGetMore ( (CHAR*)pMsg, &numToRead, &contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract get more msg failed(rc=%d)!", rc ) ;

      rc = rtnGetMore( contextID, numToRead, buf, _pEDUCB, _pRtnCB ) ;
      if ( rc )
      {
         rtnDel = FALSE ;
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG ( PDERROR, "Failed to get more, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      return rc ;
   error:
      _delContextByID( contextID, rtnDel ) ;
      contextID = -1 ;
      goto done ;
   }

   INT32 _omManager::_processAdvanceMsg( MsgHeader *pMsg,
                                         rtnContextBuf &buf,
                                         INT64 &contextID )
   {
      INT32 rc         = SDB_OK ;
      INT64 tmpContextID = -1 ;
      const CHAR *pOption = NULL ;
      const CHAR *pBackData = NULL ;
      INT32 backDataSize = 0 ;

      rc = msgExtractAdvanceMsg( (const CHAR*)pMsg, &tmpContextID, &pOption,
                                 &pBackData, &backDataSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract advance msg failed(rc=%d)!", rc ) ;

      try
      {
         BSONObj option( pOption ) ;
         rc = rtnAdvance( tmpContextID, option, pBackData, backDataSize,
                          _pEDUCB, _pRtnCB ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_processKillContext( MsgHeader * pMsg )
   {
      INT32 rc = SDB_OK ;
      INT32 contextNum = 0 ;
      const INT64 *pContextIDs = NULL ;

      rc = msgExtractKillContexts( (const CHAR *)pMsg, &contextNum,
                                   &pContextIDs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse killcontexts "
                   "request(rc=%d)", rc ) ;

      for( INT32 i = 0 ; i < contextNum ; i++ )
      {
         _delContextByID( pContextIDs[i], TRUE ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omManager::_processSessionInit( MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      MsgComSessionInitReq *pMsgReq = (MsgComSessionInitReq*)pMsg ;

      /// check wether the route id is right
      MsgRouteID localRouteID = _netAgent.localID() ;
      if ( pMsgReq->dstRouteID.value != localRouteID.value )
      {
         rc = SDB_INVALID_ROUTEID ;
         PD_LOG ( PDERROR, "Session init failed: route id does not match."
                  "Message info: [%s], Local route id: %s",
                  msg2String( pMsg ).c_str(),
                  routeID2String( localRouteID ).c_str() ) ;
      }

      return rc ;
   }

   INT32 _omManager::_processDisconnectMsg( NET_HANDLE handle,
                                            MsgHeader * pMsg )
   {
      PD_LOG( PDEVENT, "Recieve disconnect msg[handle: %u, tid: %u]",
              handle, pMsg->TID ) ;
      // release the ' handle + tid ' all context
      _delContext( handle, pMsg->TID ) ;
      // not reply
      return SDB_OK ;
   }

   INT32 _omManager::_processInterruptMsg( NET_HANDLE handle,
                                           MsgHeader *pMsg )
   {
      PD_LOG( PDEVENT, "Recieve interrupt msg[handle: %u, tid: %u]",
              handle, pMsg->TID ) ;
      // release the ' handle + tid ' all context
      _delContext( handle, pMsg->TID ) ;
      // not reply
      return SDB_OK ;
   }

   INT32 _omManager::_processRemoteDisc( NET_HANDLE handle,
                                         MsgHeader *pMsg )
   {
      _delContextByHandle( handle ) ;
      return SDB_OK ;
   }

   INT32 _omManager::_processPacketMsg( const NET_HANDLE &handle,
                                        MsgHeader *header,
                                        INT64 &contextID,
                                        rtnContextBuf &buf )
   {
      INT32 rc = SDB_OK ;
      INT32 pos = 0 ;
      MsgHeader *pTmpMsg = NULL ;

      ++_inPacketLevel ;

      pos += sizeof( MsgHeader ) ;
      while( pos < header->messageLength )
      {
         pTmpMsg = ( MsgHeader* )( ( CHAR*)header + pos ) ;

         rc = _processMsg( handle, pTmpMsg ) ;
         if ( rc )
         {
            goto error ;
         }
         pos += pTmpMsg->messageLength ;
      }

   done:
      --_inPacketLevel ;
      if ( 0 == _inPacketLevel )
      {
         contextID = _pendingContextID ;
         _pendingContextID = -1 ;
         buf = _pendingBuff ;
         _pendingBuff = rtnContextBuf() ;
      }

      return rc ;
   error:
      goto done ;
   }

   void _omManager::_addContext( const UINT32 &handle,
                                 UINT32 tid,
                                 INT64 contextID )
   {
      if ( -1 != contextID )
      {
         PD_LOG( PDDEBUG, "add context( handle=%u, contextID=%lld )",
                 handle, contextID );
         ossScopedLock lock( &_contextLatch ) ;
         _contextLst[ contextID ] = ossPack32To64( handle, tid ) ;
      }
   }

   void _omManager::_delContextByHandle( const UINT32 &handle )
   {
      PD_LOG ( PDDEBUG, "delete context( handle=%u )", handle ) ;
      UINT32 saveTid = 0 ;
      UINT32 saveHandle = 0 ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.begin() ;
      while ( iterMap != _contextLst.end() )
      {
         ossUnpack32From64( iterMap->second, saveHandle, saveTid ) ;
         if ( handle != saveHandle )
         {
            ++iterMap ;
            continue ;
         }
         // rtn delete
         _pRtnCB->contextDelete( iterMap->first, _pEDUCB ) ;
         _contextLst.erase( iterMap++ ) ;
      }
   }

   void _omManager::_delContext( const UINT32 &handle,
                                 UINT32 tid )
   {
      PD_LOG ( PDDEBUG, "delete context( handle=%u, tid=%u )",
               handle, tid ) ;
      UINT32 saveTid = 0 ;
      UINT32 saveHandle = 0 ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.begin() ;
      while ( iterMap != _contextLst.end() )
      {
         ossUnpack32From64( iterMap->second, saveHandle, saveTid ) ;
         if ( handle != saveHandle || tid != saveTid )
         {
            ++iterMap ;
            continue ;
         }
         // rtn delete
         _pRtnCB->contextDelete( iterMap->first, _pEDUCB ) ;
         _contextLst.erase( iterMap++ ) ;
      }
   }

   void _omManager::_delContextByID( INT64 contextID, BOOLEAN rtnDel )
   {
      PD_LOG ( PDDEBUG, "delete context( contextID=%lld )", contextID ) ;

      ossScopedLock lock( &_contextLatch ) ;
      CONTEXT_LIST::iterator iterMap = _contextLst.find( contextID ) ;
      if ( iterMap != _contextLst.end() )
      {
         if ( rtnDel )
         {
            _pRtnCB->contextDelete( iterMap->first, _pEDUCB ) ;
         }
         _contextLst.erase( iterMap ) ;
      }
   }

   INT32 _omManager::_defaultMsgFunc( NET_HANDLE handle, MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;

      rc = _processMsg( handle, pMsg ) ;
      return rc ;
   }

   void _omManager::_onMsgBegin( MsgHeader *pMsg )
   {
      // set reply header ( except flags, length )
      _replyHeader.numReturned          = 0 ;
      _replyHeader.startFrom            = 0 ;
      _replyHeader.header.opCode        = MAKE_REPLY_TYPE( pMsg->opCode ) ;
      _replyHeader.header.requestID     = pMsg->requestID ;
      _replyHeader.header.TID           = pMsg->TID ;
      _replyHeader.header.routeID.value = 0 ;

      if ( MSG_BS_INTERRUPTE      == pMsg->opCode ||
           MSG_BS_INTERRUPTE_SELF == pMsg->opCode ||
           MSG_BS_DISCONNECT      == pMsg->opCode ||
           MSG_COM_REMOTE_DISC    == pMsg->opCode )
      {
         _needReply = FALSE ;
      }
      else
      {
         _needReply = TRUE ;
      }

      MON_START_OP( _pEDUCB->getMonAppCB() ) ;
      _pEDUCB->getMonAppCB()->setLastOpType( pMsg->opCode ) ;
   }

   void _omManager::_onMsgEnd( )
   {
      MON_END_OP( _pEDUCB->getMonAppCB() ) ;
   }

   INT32 _omManager::_processMsg( const NET_HANDLE &handle,
                                  MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;
      INT64 contextID = -1 ;

      if ( MSG_PACKET == pMsg->opCode )
      {
         rc = _processPacketMsg( handle, pMsg, contextID, buffObj ) ;
      }
      else
      {
         _onMsgBegin( pMsg ) ;
         switch ( pMsg->opCode )
         {
            case MSG_BS_QUERY_REQ:
               rc = _processQueryMsg( pMsg, buffObj, contextID );
               break;
            case MSG_BS_GETMORE_REQ :
               rc = _processGetMoreMsg( pMsg, buffObj, contextID ) ;
               break ;
            case MSG_BS_ADVANCE_REQ :
               rc = _processAdvanceMsg( pMsg, buffObj, contextID ) ;
               break ;
            case MSG_BS_KILL_CONTEXT_REQ:
               rc = _processKillContext( pMsg ) ;
               break;
            case MSG_COM_SESSION_INIT_REQ :
               rc = _processSessionInit( pMsg ) ;
               break;
            case MSG_BS_INTERRUPTE :
               rc = _processInterruptMsg( handle, pMsg ) ;
               break ;
            case MSG_BS_DISCONNECT :
               rc = _processDisconnectMsg( handle, pMsg ) ;
               break ;
            case MSG_COM_REMOTE_DISC :
               rc = _processRemoteDisc( handle, pMsg ) ;
               break ;
            case MSG_BS_INTERRUPTE_SELF :
               rc = SDB_OK ;
               break ;
            default :
               rc = SDB_CLS_UNKNOW_MSG ;
               break ;
         }
      }

      if ( rc && SDB_DMS_EOC != rc )
      {
         PD_LOG( PDERROR, "prcess msg[opCode:(%d)%d, len: %d, "
                 "tid: %d, reqID: %lld, nodeID: %u.%u.%u] failed, rc: %d",
                 IS_REPLY_TYPE(pMsg->opCode), GET_REQUEST_TYPE(pMsg->opCode),
                 pMsg->messageLength, pMsg->TID, pMsg->requestID,
                 pMsg->routeID.columns.groupID, pMsg->routeID.columns.nodeID,
                 pMsg->routeID.columns.serviceID, rc ) ;
      }

      /// add context
      if(  MSG_BS_QUERY_REQ == pMsg->opCode && contextID != -1 )
      {
         _addContext( handle, pMsg->TID, contextID );
      }

      if ( _needReply )
      {
         if ( rc && 0 == buffObj.size() )
         {
            _errorInfo = utilGetErrorBson( rc, _pEDUCB->getInfo(
                                           EDU_INFO_ERROR ) ) ;
            buffObj = rtnContextBuf( _errorInfo ) ;
         }

         if ( _inPacketLevel > 0 )
         {
            _pendingContextID = contextID ;
            _pendingBuff = buffObj ;
         }
         else
         {
            /// send reply
            _replyHeader.header.messageLength = sizeof( MsgOpReply ) +
                                                buffObj.size();
            _replyHeader.flags                = rc ;
            _replyHeader.contextID            = contextID ;
            _replyHeader.startFrom            = (INT32)buffObj.getStartFrom() ;
            _replyHeader.numReturned          = buffObj.recordNum() ;

            rc = _reply( handle, &_replyHeader, buffObj.data(),
                         buffObj.size() ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "failed to send reply, rc: %d", rc ) ;
            }
         }
      }

      _onMsgEnd() ;

      return rc ;
   }

   INT32 _omManager::_reply( const NET_HANDLE &handle,
                             MsgOpReply *pReply,
                             const CHAR *pReplyData,
                             UINT32 replyDataLen )
   {
      INT32 rc = SDB_OK ;

      // check message length
      if ( (UINT32)( pReply->header.messageLength ) !=
           sizeof( MsgOpReply ) + replyDataLen )
      {
         PD_LOG ( PDERROR, "Reply message length error[%u != %u]",
                  pReply->header.messageLength,
                  sizeof( MsgOpReply ) + replyDataLen ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // Send message
      if ( replyDataLen > 0 )
      {
         rc = _netAgent.syncSend ( handle, (MsgHeader *)pReply,
                                   (void*)pReplyData, replyDataLen ) ;
      }
      else
      {
         rc = _netAgent.syncSend ( handle, (MsgHeader *)pReply ) ;
      }

      PD_RC_CHECK ( rc, PDDEBUG, "Fail to send reply message[opCode:(%d)%d, "
              "tid: %d, reqID: %lld, nodeID: %u.%u.%u], rc: %d",
              IS_REPLY_TYPE( pReply->header.opCode ),
              GET_REQUEST_TYPE( pReply->header.opCode ),
              pReply->header.TID, pReply->header.requestID,
              pReply->header.routeID.columns.groupID,
              pReply->header.routeID.columns.nodeID,
              pReply->header.routeID.columns.serviceID, rc ) ;
      PD_LOG( PDDEBUG, "Succeed in sending reply message[opCode:(%d)%d, "
              "tid: %d, reqID: %lld, nodeID: %u.%u.%u]",
              IS_REPLY_TYPE( pReply->header.opCode ),
              GET_REQUEST_TYPE( pReply->header.opCode ),
              pReply->header.TID, pReply->header.requestID,
              pReply->header.routeID.columns.groupID,
              pReply->header.routeID.columns.nodeID,
              pReply->header.routeID.columns.serviceID ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   /*
      get the global om manager object point
   */
   omManager* sdbGetOMManager()
   {
      static omManager s_omManager ;
      return &s_omManager ;
   }

}


