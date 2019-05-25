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

   Source File Name = omCommand.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_GETFILECOMMAND_HPP__
#define OM_GETFILECOMMAND_HPP__

#include "omCommandInterface.hpp"
#include "restAdaptor.hpp"
#include "pmdRestSession.hpp"
#include "pmdRemoteSession.hpp"
#include "rtnCB.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "omManager.hpp"
#include "omTaskManager.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/all.hpp>
#include <map>
#include <string>

using namespace bson;
using namespace boost::property_tree;

namespace engine
{
   struct simpleHostInfo : public SDBObject
   {
      string hostName ;
      string clusterName ;
      string ip ;
      string user ;
      string passwd ;
      string installPath ;
      string agentPort ;
      string sshPort ;
   } ;

   struct simpleNodeInfo : public SDBObject
   {
      string hostName ;
      string svcName ;
      string role ;
   } ;

   class omContextAssist
   {
      public:
         omContextAssist( SINT64 contextID, pmdEDUCB *cb, SDB_RTNCB *rtncb) ;
         ~omContextAssist() ;

      public:
         INT32 init( const BSONObj &selector, const BSONObj &matcher,
                     INT64 numToSkip, INT64 numToReturn ) ;
         INT32 getNext( BSONObj &data ) ;

      private:
         pmdEDUCB    *_cb ;
         SDB_RTNCB   *_rtncb ;
         SINT64      _orgContextID ;
         SINT64      _contextID ;
   } ;

   class omAuthCommand : public omRestCommandBase
   {
      public:
         omAuthCommand( restAdaptor *pRestAdaptor,
                        pmdRestSession *pRestSession ) ;

         ~omAuthCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         void            _decryptPasswd( const string &encryptPasswd,
                                         const string &time,
                                         string &decryptPasswd) ;
         INT32           _getSdbUsrInfo( const string &clusterName,
                                         string &sdbUser,
                                         string &sdbPasswd,
                                         string &sdbUserGroup ) ;

         INT32           _getQueryPara( BSONObj &selector, BSONObj &matcher,
                                        BSONObj &order, BSONObj &hint,
                                        SINT64 &numSkip, SINT64 &numReturn ) ;
         string          _getLanguage() ;
         void            _setFileLanguageSep() ;

      protected:
         string          _languageFileSep ;
   };

   class omExtendBusinessCommand : public omAuthCommand
   {
   public:
      omExtendBusinessCommand( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession,
                               const CHAR *pRootPath,
                               string &localAgentHost,
                               string &localAgentService ) ;
      ~omExtendBusinessCommand() ;

   public:
      virtual INT32 doCommand() ;

   private:
      INT32 _getRestInfo( BSONObj &extendConfig, string& extendConfigMod ) ;
      INT32 _checkBusiness() ;
      INT32 _readConfigProperties( const string& extendConfigMod,
                                   BSONObj& buzDetail ) ;
      INT32 _getClusterInfo( BSONObj& hostsInfoForCluster,
                             BSONObj& buzInfoForCluster ) ;
      INT32 _checkExtendConfig( const BSONObj& confProperties,
                                const BSONObj& hostsDetail,
                                const BSONObj& buzInfoForCluster,
                                BSONObj& extendConfig ) ;
      INT32 _createExtendTask( const BSONObj& extendConfig,
                               const BSONObj& hostsInfoForCluster,
                               INT64& taskID ) ;
      INT32 _generateTaskInfo( const BSONObj& hostsInfoForCluster,
                               const BSONObj& extendConfig,
                               BSONObj& taskConfig ) ;
      INT32 _generateTaskResultInfo( const BSONObj &taskConfig,
                                     BSONArray &resultInfo ) ;

   private:
      string _rootPath ;
      string _localAgentHost ;
      string _localAgentService ;
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
   } ;

   class omShrinkBusinessCommand : public omAuthCommand
   {
   public:
      omShrinkBusinessCommand( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession,
                               const CHAR *pRootPath,
                               string &localAgentHost,
                               string &localAgentService ) ;
      ~omShrinkBusinessCommand() ;

   public:
      virtual INT32 doCommand() ;

   private:
      INT32 _getRestInfo( BSONObj &shrinkConfig ) ;

      INT32 _checkBusiness() ;

      INT32 _checkSdbConfig( const BSONObj &shrinkConfig ) ;

      INT32 _checkConfig( const BSONObj &shrinkConfig ) ;

      INT32 _createTask( vector<simpleAddressInfo> &addressList,
                         const BSONObj &shrinkConfig,
                         INT64 &taskID ) ;

      INT32 _generateTaskInfo( vector<simpleAddressInfo> &addressList,
                               const BSONObj &shrinkConfig,
                               BSONObj &taskConfig ) ;

      INT32 _generateTaskResultInfo( const BSONObj &shrinkConfig,
                                     BSONArray &resultInfo ) ;

   private:
      string _rootPath ;
      string _localAgentHost ;
      string _localAgentService ;
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
   } ;

   class omLogoutCommand : public omAuthCommand
   {
      public:
         omLogoutCommand( restAdaptor *pRestAdaptor,
                          pmdRestSession *pRestSession ) ;
         ~omLogoutCommand() ;

      public:
         virtual INT32   doCommand() ;
   };

   class omChangePasswdCommand : public omAuthCommand
   {
      public:
         omChangePasswdCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;
         ~omChangePasswdCommand() ;

      public:
         virtual INT32   doCommand() ;

      private:
         INT32           _getRestDetail( string &user, string &oldPasswd,
                                         string &newPasswd, string &time ) ;
   };

   class omCheckSessionCommand : public omAuthCommand
   {
      public:
         omCheckSessionCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;

         ~omCheckSessionCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
   };

   class omCreateClusterCommand : public omAuthCommand
   {
   public:

      omCreateClusterCommand( restAdaptor *pRestAdaptor,
                              pmdRestSession *pRestSession ) ;

      virtual ~omCreateClusterCommand() ;

      virtual INT32 doCommand() ;

   private:

      INT32 _getRestInfo( string &clusterName, BSONObj &clusterInfo ) ;

   } ;

   class omQueryClusterCommand : public omCheckSessionCommand
   {
      public:
         omQueryClusterCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;

         ~omQueryClusterCommand() ;

      public:
         virtual INT32   doCommand() ;
   };

   struct omScanHostInfo
   {
      string hostName ;
      string ip ;
      string user ;
      string passwd ;
      string sshPort ;
      string agentPort ;
      bool isNeedUninstall ;

      omScanHostInfo()
      {
         hostName        = "" ;
         ip              = "" ;
         user            = "" ;
         passwd          = "" ;
         sshPort         = "" ;
         agentPort       = "" ;
         isNeedUninstall = false ;
      }

      omScanHostInfo( const omScanHostInfo &right )
      {
         hostName        = right.hostName ;
         ip              = right.ip ;
         user            = right.user ;
         passwd          = right.passwd ;
         sshPort         = right.sshPort ;
         agentPort       = right.agentPort ;
         isNeedUninstall = right.isNeedUninstall ;
      }
   } ;

   class omUpdateHostInfoCommand : public omCheckSessionCommand
   {
      public:
         omUpdateHostInfoCommand( restAdaptor *pRestAdaptor,
                                  pmdRestSession *pRestSession ) ;

         ~omUpdateHostInfoCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         INT32           _getUpdateInfo( map< string, string > &mapHost ) ;
         INT32           _updateHostIP( map< string, string > &mapHost ) ;
         INT32           _getClusterName( const string &hostName,
                                          string &clusterName ) ;
   } ;

   class omScanHostCommand : public omCheckSessionCommand
   {
      public:
         omScanHostCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession,
                            const string &localAgentHost,
                            const string &localAgentService ) ;

         ~omScanHostCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         bool            _isHostNameExist( const string &hostName ) ;
         bool            _isHostIPExist( const string &hostName ) ;
         bool            _isHostExist( const omScanHostInfo &host ) ;
         void            _filterExistHost( list<omScanHostInfo> &hostInfoList,
                                           list<BSONObj> &hostResult ) ;
         void            _generateArray( list<BSONObj> &hostInfoList,
                                         const string &arrayKeyName,
                                         BSONObj &result ) ;
         void            _sendResult2Web( list<BSONObj> &hostResult ) ;
         INT32           _notifyAgentTask( INT64 taskID ) ;
         INT32           _sendMsgToLocalAgent( omManager *om,
                                               pmdRemoteSession *remoteSession,
                                               MsgHeader *pMsg,
                                               BOOLEAN isUseLocalHost = FALSE ) ;
         INT32           _getScanHostList( string &clusterName,
                                           list<omScanHostInfo> &hostInfo ) ;
         void            _clearSession( omManager *om,
                                        pmdRemoteSession *remoteSession) ;
         void            _generateHostList( list<omScanHostInfo> &hostInfoList,
                                            BSONObj &bsonRequest ) ;

         void            _markHostExistence( BSONObj &oneHost ) ;

      private:
         INT32           _parseResonpse( VEC_SUB_SESSIONPTR &subSessionVec,
                                         BSONObj &response,
                                         list<BSONObj> &bsonResult ) ;
         INT32           _checkRestHostInfo( BSONObj &hostInfo ) ;

      protected:
         string          _localAgentHost ;
         string          _localAgentService ;

      private:


   };

   class omCheckHostCommand : public omScanHostCommand
   {
      public:
         omCheckHostCommand( restAdaptor *pRestAdaptor,
                             pmdRestSession *pRestSession,
                             const string &localAgentHost,
                             const string &localAgentService ) ;

         ~omCheckHostCommand() ;

      public:
         virtual INT32   doCommand() ;

      private:
         INT32           _getCheckHostList( string &clusterName,
                                          list<omScanHostInfo> &hostInfoList ) ;
         INT32           _doCheck( list<omScanHostInfo> &hostInfoList,
                                        list<BSONObj> &hostResult ) ;

         void            _updateUninstallFlag(
                                            list<omScanHostInfo> &hostInfoList,
                                            const string &ip,
                                            const string &agentPort,
                                            bool isNeedUninstall ) ;
         INT32           _installAgent( list<omScanHostInfo> &hostInfoList,
                                        list<BSONObj> &hostResult ) ;
         INT32           _addCheckHostReq( omManager *om,
                                          pmdRemoteSession *remoteSession,
                                          list<omScanHostInfo> &hostInfoList ) ;
         void            _updateDiskInfo( BSONObj &onehost ) ;

         INT32           _checkResFormat( BSONObj &result ) ;
         void            _errorCheckHostEnv( list<omScanHostInfo> &hostInfoList,
                                             list<BSONObj> &hostResult,
                                             MsgRouteID id, int flag,
                                             const string &error ) ;
         INT32           _checkHostEnv( list<omScanHostInfo> &hostInfoList,
                                        list<BSONObj> &hostResult ) ;

         bool            _isNeedUnistall( list<omScanHostInfo> &hostInfoList ) ;
         void            _generateUninstallReq(
                                             list<omScanHostInfo> &hostInfoList,
                                             BSONObj &bsonRequest ) ;
         INT32           _uninstallAgent( list<omScanHostInfo> &hostInfoList ) ;

         void            _updateAgentService(
                                            list<omScanHostInfo> &hostInfoList,
                                            const string &ip,
                                            const string &port ) ;

         void            _eraseFromList( list<omScanHostInfo> &hostInfoList,
                                         BSONObj &oneHost ) ;
         void            _eraseFromListByIP( list<omScanHostInfo> &hostInfoList,
                                             const string &ip ) ;
         void            _eraseFromListByHost(
                                             list<omScanHostInfo> &hostInfoList,
                                             const string &hostName ) ;

         INT32           _notifyAgentExit(
                                          list<omScanHostInfo> &hostInfoList ) ;
         INT32           _addAgentExitReq( omManager *om,
                                          pmdRemoteSession *remoteSession,
                                          list<omScanHostInfo> &hostInfoList ) ;

      private:
         map<UINT64, omScanHostInfo> _id2Host ;
   };

   class omAddHostCommand : public omScanHostCommand
   {
      public:
         omAddHostCommand( restAdaptor *pRestAdaptor,
                           pmdRestSession *pRestSession,
                           const string &localAgentHost,
                           const string &localAgentService ) ;

         ~omAddHostCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         INT32           _getRestHostList( string &clusterName,
                                           list<BSONObj> &hostInfo ) ;

      private:
         void            _generateTableField( BSONObjBuilder &builder,
                                              const string &newFieldName,
                                              BSONObj &bsonOld,
                                              const string &oldFiledName ) ;
         INT32           _getClusterInstallPath( const string &clusterName,
                                                 string &installPath ) ;
         INT32           _checkHostExistence( list<BSONObj> &hostInfoList ) ;

         INT32           _checkHostDisk( list<BSONObj> &hostInfoList ) ;

         INT32           _generateTaskInfo( const string &clusterName,
                                            list<BSONObj> &hostInfoList,
                                            BSONObj &taskInfo,
                                            BSONArray &resultInfo ) ;
         INT64           _generateTaskID() ;

         INT32           _saveTask( INT64 taskID, const BSONObj &taskInfo,
                                    const BSONArray &resultInfo ) ;

         INT32           _removeTask( INT64 taskID ) ;

         INT32           _checkTaskExistence( list<BSONObj> &hostInfoList ) ;
   };

   class omListHostCommand : public omCheckSessionCommand
   {
      public:
         omListHostCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;
         ~omListHostCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         void            _sendHostInfo2Web( list<BSONObj> &hosts ) ;

      private:
   } ;

   class omQueryHostCommand : public omListHostCommand
   {
      public:
         omQueryHostCommand( restAdaptor *pRestAdaptor,
                             pmdRestSession *pRestSession ) ;

         ~omQueryHostCommand() ;

      public:
         virtual INT32   doCommand() ;

      private:
   } ;

   class omListBusinessTypeCommand : public omCheckSessionCommand
   {
      public:
         omListBusinessTypeCommand( restAdaptor *pRestAdaptor,
                                    pmdRestSession *pRestSession,
                                    const CHAR *pRootPath,
                                    const CHAR *pSubPath ) ;
         virtual ~omListBusinessTypeCommand() ;

      public:
         virtual INT32  doCommand() ;

      protected:
         INT32          _readConfigFile( const string &file, BSONObj &obj ) ;
         void           _recurseParseObj( ptree &pt, BSONObj &out ) ;
         void           _parseArray( ptree &pt,
                                     BSONArrayBuilder &arrayBuilder ) ;
         BOOLEAN        _isStringValue( ptree &pt ) ;
         BOOLEAN        _isArray( ptree &pt ) ;

         INT32          _getBusinessList( list<BSONObj> &businessList ) ;

      protected:
         string          _rootPath ;
         string          _subPath ;

   } ;

   class omGetBusinessTemplateCommand : public omListBusinessTypeCommand
   {
      public:
         omGetBusinessTemplateCommand( restAdaptor *pRestAdaptor,
                                         pmdRestSession *pRestSession,
                                         const CHAR *pRootPath,
                                         const CHAR *pSubPath ) ;
         virtual ~omGetBusinessTemplateCommand() ;

      public:
         virtual INT32  doCommand() ;

      protected:
         INT32          _readConfTemplate( const string &businessType,
                                           const string &file,
                                           list<BSONObj> &clusterTypeList ) ;
         INT32          _readConfDetail( const string &file,
                                         BSONObj &bsonConfDetail ) ;

   } ;

   class omGetBusinessConfigCommand : public omAuthCommand
   {
   public:
      omGetBusinessConfigCommand( restAdaptor *pRestAdaptor,
                                  pmdRestSession *pRestSession,
                                  string &rootPath ) ;

      ~omGetBusinessConfigCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _checkRestInfo( const BSONObj &templateInfo ) ;

      INT32 _checkTemplate( BSONObj &deployModInfo ) ;

      INT32 _check( const BSONObj &templateInfo, BSONObj &deployModInfo ) ;

      INT32 _getPropertyValue( const BSONObj &property, const string &name,
                               string &value ) ;

      INT32 _getDeployProperty( const BSONObj &templateInfo,
                                const BSONObj &deployModInfo,
                                BSONObj &deployProperty ) ;

      INT32 _getBuzInfoOfCluster( BSONObj &buzInfoOfCluster ) ;

      INT32 _getHostInfoOfCluster( BSONObj &hostsInfoOfCluster ) ;

      INT32 _generateRequest( omRestTool &restTool,
                              const BSONObj &templateInfo,
                              const BSONObj &deployModInfo ) ;

   private:
      string _rootPath ;
      string _clusterName ;
      string _deployMod ;
      string _businessType ;
      string _businessName ;
      string _operationType ;
   } ;

   class omAddBusinessCommand : public omAuthCommand
   {
   public:
      omAddBusinessCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession,
                            string &rootPath,
                            string &path,
                            string &localAgentHost,
                            string &localAgentService ) ;

      ~omAddBusinessCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _checkRestInfo( const BSONObj &configInfo ) ;

      INT32 _checkTemplate() ;

      INT32 _getBuzInfoOfCluster( BSONObj &buzInfoOfCluster ) ;

      INT32 _getHostInfoOfCluster( BSONObj &hostsInfoOfCluster ) ;

      INT32 _checkConfig( BSONObj &deployConfig ) ;

      INT32 _check( BSONObj &configInfo ) ;

      INT32 _generateRequest( const BSONObj &configInfo,
                              BSONObj &taskConfig,
                              BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig, const BSONArray &resultInfo,
                         INT64 &taskID ) ;

   private:
      string _rootPath ;
      string _localAgentHost ;
      string _localAgentService ;
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
      BOOLEAN _force ;
   } ;

   class omListTaskCommand : public omAuthCommand
   {
      public:
         omListTaskCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;
         virtual ~omListTaskCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         INT32          _getTaskList( list<BSONObj> &taskList ) ;
         void           _sendTaskList2Web( list<BSONObj> &taskList ) ;
   } ;

   class omQueryTaskCommand : public omScanHostCommand
   {
      public:
         omQueryTaskCommand( restAdaptor *pRestAdaptor,
                             pmdRestSession *pRestSession,
                             const string &localAgentHost,
                             const string &localAgentService ) ;
         virtual ~omQueryTaskCommand() ;

      public:
         virtual INT32  doCommand() ;

      protected:
         INT32          _getSsqlResult( BSONObj &oneTask ) ;
         INT32          _ssqlGetMore( INT64 taskID, INT32 &flag,
                                      BSONObj &result ) ;
         INT32          _updateSsqlTask( INT64 taskID,
                                         const BSONObj &taskInfo ) ;

      private:
         void           _sendTaskInfo2Web( list<BSONObj> &tasks ) ;
         void           _sendOneTaskInfo2Web( BSONObj &oneTask ) ;
         void           _modifyTaskInfo( BSONObj &task ) ;
   } ;

   class omListNodeCommand : public omAuthCommand
   {
      public:
         omListNodeCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;
         virtual ~omListNodeCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         INT32          _getNodeList( string businessName,
                                      list<simpleNodeInfo> &nodeList ) ;
         void           _sendNodeList2Web( list<simpleNodeInfo> &nodeList ) ;
   } ;

   class omGetNodeConfCommand : public omAuthCommand
   {
      public:
         omGetNodeConfCommand( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession ) ;
         virtual ~omGetNodeConfCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         INT32          _getNodeInfo( const string &hostName,
                                      const string &svcName,
                                      BSONObj &nodeinfo ) ;
         void           _expandNodeInfo( BSONObj &oneConfig,
                                         const string &svcName,
                                         BSONObj &nodeinfo ) ;
         void           _sendNodeInfo2Web( BSONObj &nodeList ) ;
   } ;

   class omQueryNodeConfCommand : public omAuthCommand
   {
   public:
      omQueryNodeConfCommand( restAdaptor *pRestAdaptor,
                              pmdRestSession *pRestSession ) ;
      virtual ~omQueryNodeConfCommand() ;

   public:
      virtual INT32 doCommand() ;
   } ;

   class omQueryBusinessCommand : public omAuthCommand
   {
      public:
         omQueryBusinessCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession ) ;
         virtual ~omQueryBusinessCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         void           _sendBusinessInfo2Web( list<BSONObj> &businessInfo ) ;
   } ;

   class omListBusinessCommand : public omAuthCommand
   {
      public:
         omListBusinessCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;
         virtual ~omListBusinessCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         void           _sendBusinessList2Web( list<BSONObj> &businessList ) ;
   };

   class omListHostBusinessCommand : public omAuthCommand
   {
      public:
         omListHostBusinessCommand( restAdaptor *pRestAdaptor,
                                    pmdRestSession *pRestSession ) ;
         virtual ~omListHostBusinessCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         void           _sendHostBusiness2Web( list<BSONObj> &businessList ) ;
   };

   class omStartBusinessCommand : public omScanHostCommand
   {
      public:
         omStartBusinessCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession,
                                 string localAgentHost,
                                 string localAgentService ) ;
         virtual ~omStartBusinessCommand() ;

      public:
         virtual INT32  doCommand() ;

      protected:
         INT32          _getNodeInfo( const string &businessName,
                                      BSONObj &nodeInfos,
                                      BOOLEAN &isExistFlag ) ;
         INT32          _getHostInfo( const string &hostName,
                                      simpleHostInfo &hostInfo,
                                      BOOLEAN &isExistFlag ) ;

      private:
         INT32          _expandNodeInfoToBuilder( const BSONObj &record,
                                             BSONArrayBuilder &arrayBuilder ) ;
   } ;

   class omStopBusinessCommand : public omScanHostCommand
   {
      public:
         omStopBusinessCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession,
                                string localAgentHost,
                                string localAgentService ) ;
         virtual ~omStopBusinessCommand() ;

      public:
         virtual INT32  doCommand() ;
   } ;

   class omRemoveClusterCommand : public omAuthCommand
   {
      public:
         omRemoveClusterCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;
         virtual ~omRemoveClusterCommand() ;

      public:
         virtual INT32  doCommand() ;

      private:
         INT32          _getClusterExistHostFlag( const string &clusterName,
                                                  BOOLEAN &flag ) ;
         INT32          _getClusterExistFlag( const string &clusterName,
                                              BOOLEAN &flag ) ;
         INT32          _removeCluster( const string &clusterName ) ;
   } ;

   class omRemoveHostCommand : public omAuthCommand
   {
   public:
      omRemoveHostCommand( restAdaptor *pRestAdaptor,
                           pmdRestSession *pRestSession,
                           string &localAgentHost,
                           string &localAgentService ) ;

      virtual ~omRemoveHostCommand() ;

      virtual INT32  doCommand() ;

   private:
      INT32 _check( const BSONObj &hostList ) ;

      INT32 _generateRequest( const BSONObj &hostList,
                              BSONObj &taskConfig, BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig, const BSONArray &resultInfo,
                         INT64 &taskID ) ;

   private:
      string _clusterName ;
      string _localAgentHost ;
      string _localAgentService ;
   } ;

   class omRemoveBusinessCommand : public omAuthCommand
   {
   public:
      omRemoveBusinessCommand( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession,
                               string localAgentHost,
                               string localAgentService ) ;

      virtual ~omRemoveBusinessCommand() ;

      virtual INT32 doCommand() ;

   private:
      BOOLEAN _isDiscoveredBusiness( BSONObj &buzInfo ) ;

      INT32 _check( BSONObj &buzInfo ) ;

      INT32 _generateTaskConfig( list<BSONObj> &configList,
                                 BSONObj &taskConfig ) ;

      void _generateResultInfo( list<BSONObj> &configList,
                                BSONArray &resultInfo ) ;

      INT32 _generateRequest( const BSONObj &buzInfo,
                              BSONObj &taskConfig, BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig,
                         const BSONArray &resultInfo,
                         INT64 &taskID ) ;
      
   private:
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
      string _localAgentHost ;
      string _localAgentService ;
   } ;

   class omQueryHostStatusCommand : public omStartBusinessCommand
   {
      public:
         omQueryHostStatusCommand( restAdaptor *pRestAdaptor,
                                   pmdRestSession *pRestSession,
                                   string localAgentHost,
                                   string localAgentService ) ;

         ~omQueryHostStatusCommand() ;

      public:
         virtual INT32   doCommand() ;

      private:
         INT32           _getRestHostList( list<string> &hostNameList ) ;
         INT32           _verifyHostInfo( list<string> &hostNameList,
                                          list<fullHostInfo> &hostInfoList ) ;
         INT32           _addQueryHostStatusReq( omManager *om,
                                          pmdRemoteSession *remoteSession,
                                          list<fullHostInfo> &hostInfoList ) ;
         INT32           _getHostStatus( list<fullHostInfo> &hostInfoList,
                                         BSONObj &bsonStatus ) ;
         void            _appendErrorResult( BSONArrayBuilder &arrayBuilder,
                                             const string &host, INT32 err,
                                             const string &detail ) ;
         void            _formatHostStatusOneNet( BSONObj &oneNet ) ;
         void            _formatHostStatusNet( BSONObj &net ) ;
         void            _formatHostStatusCPU( BSONObj &cpu ) ;
         void            _formatHostStatus( BSONObj &status ) ;

         void            _seperateMegaBitValue( BSONObj &obj, long value ) ;
   } ;

   class omPredictCapacity : public omAuthCommand
   {
      public:
         omPredictCapacity( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;

         ~omPredictCapacity() ;

      public:
         virtual INT32   doCommand() ;

      private:
         INT32           _getHostList( BSONObj &hostInfos,
                                       list<string> &hostNameList ) ;

         INT32           _getTemplateValue( BSONObj &properties,
                                            INT32 &replicaNum,
                                            INT32 &groupNum ) ;

         INT32           _getRestInfo( list<string> &hostNameList,
                                       string &clusterName,
                                       INT32 &replicaNum, INT32 &groupNum ) ;

         INT32           _predictCapacity( list<simpleHostDisk> &hostInfoList,
                                           INT32 replicaNum, INT32 groupNum,
                                           UINT64 &totalSize, UINT64 &validSize,
                                           UINT32 &redundancyRate ) ;
   } ;

   class omGetLogCommand : public omAuthCommand
   {
      public:
         omGetLogCommand( restAdaptor *pRestAdaptor,
                          pmdRestSession *pRestSession ) ;
         virtual ~omGetLogCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         INT32           _getFileContent( string filePath, CHAR **pFileContent,
                                          INT32 &fileContentLen ) ;

      protected:
   } ;

   class omSetBusinessAuthCommand : public omAuthCommand
   {
   public:
      omSetBusinessAuthCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;

      virtual ~omSetBusinessAuthCommand() ;

   public:
      virtual INT32 doCommand() ;
   } ;

   class omRemoveBusinessAuthCommand : public omAuthCommand
   {
      public:
         omRemoveBusinessAuthCommand( restAdaptor *pRestAdaptor,
                                      pmdRestSession *pRestSession ) ;
         virtual ~omRemoveBusinessAuthCommand() ;

      public:
         virtual INT32   doCommand() ;
   } ;

   class omQueryBusinessAuthCommand : public omAuthCommand
   {
      public:
         omQueryBusinessAuthCommand( restAdaptor *pRestAdaptor,
                                     pmdRestSession *pRestSession ) ;
         virtual ~omQueryBusinessAuthCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         void            _sendAuthInfo2Web( list<BSONObj> &authInfo ) ;
   } ;

   class omDiscoverBusinessCommand : public omAuthCommand
   {
   public:

      omDiscoverBusinessCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession,
                                 string &localAgentHost,
                                 string &localAgentService ) ;

      virtual ~omDiscoverBusinessCommand() ;

      virtual INT32 doCommand() ;

   private:

      INT32 _getRestInfo( BSONObj &configInfo ) ;

      INT32 _checkHostPort( const string &hostName, const string &port ) ;
      INT32 _checkBusinssCFG( BSONObj &configInfo ) ;
      INT32 _checkWebLinkCFG( BSONObj &buzInfo ) ;
      INT32 _checkSequoiasqlCFG( BSONObj &buzInfo ) ;
      INT32 _checkSequoiaDBCFG( BSONObj &buzInfo ) ;

      void _generateRequest( const string &hostName,
                             const string &svcname,
                             const string &authUser,
                             const string &authPwd,
                             const string &agentService,
                             BSONObj &request ) ;
      void _parseHostMap( const BSONObj &hosts, map<string, string> &hostMap ) ;
      void _hostName2Address( map<string, string> &hostMap,
                              string &hostName, string &address ) ;
      INT32 _checkSyncSdbResult( omRestTool &restTool,
                                 const BSONObj &hostInfo,
                                 map<string, string> &hostMap ) ;
      INT32 _syncSequoiaDB( omRestTool &restTool, const BSONObj &buzInfo ) ;
      INT32 _storeBusinessInfo( const INT32 addType,
                                const string &deployMod,
                                const BSONObj &buzInfo ) ;
      INT32 _syncBusiness( omRestTool &restTool, BSONObj &configInfo ) ;

   private:

      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _localAgentHost ;
      string _localAgentService ;

   } ;

   class omUnDiscoverBusinessCommand : public omAuthCommand
   {
      public:
         omUnDiscoverBusinessCommand( restAdaptor *pRestAdaptor,
                                      pmdRestSession *pRestSession ) ;

         virtual ~omUnDiscoverBusinessCommand() ;

      public:
         virtual INT32   doCommand() ;

      protected:
         INT32           _getRestBusinessInfo( string &clusterName,
                                               string &businessName ) ;

         INT32           _UnDiscoverBusiness( const string &clusterName,
                                              const string &businessName ) ;
   } ;

   class omSsqlExecCommand : public omAuthCommand
   {
   public:
      omSsqlExecCommand( restAdaptor *pRestAdaptor,
                         pmdRestSession *pRestSession,
                         const string &localAgentHost,
                         const string &localAgentPort ) ;
      virtual ~omSsqlExecCommand() ;
      virtual INT32 doCommand() ;

   private:
      INT32 _check() ;
      INT32 _execSsql( const string &sql, const string &dbName ) ;
      INT32 _generateRequest( const string &sql, const string &dbName,
                              BSONObj &request ) ;

   private:
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _localAgentHost ;
      string _localAgentService ;
   } ;

   class omInterruptTaskCommand : public omScanHostCommand
   {
      public:
         omInterruptTaskCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession,
                                 const string &localAgentHost,
                                 const string &localAgentPort ) ;

         virtual ~omInterruptTaskCommand() ;

      public:
         virtual INT32   doCommand() ;

      public:
         INT32           updateTaskStatus( INT64 taskID, INT32 status,
                                           INT32 errNo ) ;
         INT32           notifyAgentInteruptTask( INT64 taskID ) ;

      protected:
         INT32           _parseInterruptTaskInfo() ;

      protected:
         BOOLEAN         _isFinished ;
         INT64           _taskID ;
   } ;

   class omForwardPluginCommand : public omAuthCommand
   {
   public:
      omForwardPluginCommand( restAdaptor *pRestAdaptor,
                              pmdRestSession *pRestSession,
                              const string &businessType ) ;

      ~omForwardPluginCommand() ;

      virtual INT32 doCommand() ;

   private:
      string _businessType ;
   } ;

   class omGetFileCommand : public omGetLogCommand
   {
      public:
         omGetFileCommand( restAdaptor *pRestAdaptor,
                           pmdRestSession *pRestSession,
                           const CHAR *pRootPath, const CHAR *pSubPath ) ;
         virtual ~omGetFileCommand() ;

      public:
         virtual INT32   doCommand() ;
         virtual INT32   undoCommand() ;

      protected:
         string          _rootPath ;
         string          _subPath ;
   };

   class restFileController : public SDBObject
   {
      public:
         static restFileController* getTransferInstance() ;

         INT32 getTransferedPath( const char *src_file, string &transfered ) ;

         bool isFileAuthorPublic( const char *file ) ;

      private:
         restFileController() ;
         restFileController(const restFileController &) ;
         restFileController& operator = ( const restFileController & ) ;

      private:
         typedef map < string, string >::iterator mapIteratorType ;
         typedef map < string, string >::value_type mapValueType ;
         map < string, string > _transfer ;

         map < string, string > _publicAccessFiles ;
   };

   class omTaskStrategyInsert : public omRestCommandBase
   {
      public:

         virtual INT32 doCommand() ;

      public:

         omTaskStrategyInsert( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession ) ;

         ~omTaskStrategyInsert() ;
   };

   class omTaskStrategyList : public omRestCommandBase
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omTaskStrategyList( restAdaptor *pRestAdaptor,
                             pmdRestSession *pRestSession ) ;

         ~omTaskStrategyList() ;
   };

   class omTaskStrategyUpdateNice : public omRestCommandBase
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omTaskStrategyUpdateNice( restAdaptor *pRestAdaptor,
                                   pmdRestSession *pRestSession ) ;

         ~omTaskStrategyUpdateNice() ;
   };

   class omTaskStrategyAddIps : public omRestCommandBase
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omTaskStrategyAddIps( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession ) ;

         ~omTaskStrategyAddIps() ;
   };

   class omTaskStrategyDelIps : public omRestCommandBase
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omTaskStrategyDelIps( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession ) ;

         ~omTaskStrategyDelIps() ;
   };

   class omTaskStrategyDel : public omRestCommandBase
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omTaskStrategyDel( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;

         ~omTaskStrategyDel() ;
   };

   class omGetSystemInfoCommand : public omAuthCommand
   {
      public:

         virtual INT32   doCommand() ;

      public:

         omGetSystemInfoCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession ) ;

         ~omGetSystemInfoCommand() ;
   };

   class omSyncBusinessConfigureCommand : public omAuthCommand
   {
   public:

      omSyncBusinessConfigureCommand( restAdaptor *pRestAdaptor,
                                      pmdRestSession *pRestSession,
                                      string &localAgentHost,
                                      string &localAgentService ) ;

      ~omSyncBusinessConfigureCommand();

      virtual INT32 doCommand() ;

   private:

      INT32 _getRestInfo() ;

      void _parseHostMap( const BSONObj &hosts, map<string, string> &hostMap ) ;

      void _hostName2Address( map<string, string> &hostMap,
                              string &hostName, string &address ) ;

      INT32 _checkExecResult( omRestTool &restTool, const BSONObj &result,
                              map<string, string> &hostMap ) ;

      INT32 _syncBusinessConfig( omRestTool &restTool,
                                 vector<simpleAddressInfo> &addressList ) ;

      void _generateRequest( vector<simpleAddressInfo> &addressList,
                             BSONObj &request ) ;

   private:

      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _localAgentHost ;
      string _localAgentService ;
   } ;

   class omGrantSysConfigureCommand : public omAuthCommand
   {
   public:

      omGrantSysConfigureCommand( restAdaptor *pRestAdaptor,
                                  pmdRestSession *pRestSession ) ;

      ~omGrantSysConfigureCommand() ;

      virtual INT32 doCommand() ;

   private:

      INT32 _getRestInfo() ;

      INT32 _checkCluster() ;

      INT32 _grantSysConf() ;

   private:

      string  _clusterName ;
      string  _grantName ;
      BOOLEAN _privilege ;

   } ;

   class omUnbindBusinessCommand : public omAuthCommand
   {
   public:
      omUnbindBusinessCommand( restAdaptor *pRestAdaptor,
                               pmdRestSession *pRestSession ) ;

      ~omUnbindBusinessCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _check() ;

   private:
      string _clusterName ;
      string _businessName ;

   } ;

   class omUnbindHostCommand : public omAuthCommand
   {
   public:
      omUnbindHostCommand( restAdaptor *pRestAdaptor,
                           pmdRestSession *pRestSession ) ;

      ~omUnbindHostCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _getRestInfo( list<string> &hostList ) ;
      INT32 _checkHost( list<string> &hostList ) ;

   private:
      string _clusterName ;

   } ;

   class omDeployPackageCommand : public omAuthCommand
   {
   public:
      omDeployPackageCommand( restAdaptor *pRestAdaptor,
                              pmdRestSession *pRestSession,
                              string &localAgentHost,
                              string &localAgentService ) ;

      ~omDeployPackageCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _check( const BSONObj &restHostInfo, BSONObj &clusterInfo,
                    BSONObj &hostsInfo, string &packetPath ) ;

      void _generateRequest( const BSONObj &clusterInfo,
                             const BSONObj &hostInfo,
                             const string &packetPath,
                             BSONObj &taskConfig,
                             BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig, const BSONArray &resultInfo,
                         INT64 &taskID ) ;

   private:
      BOOLEAN  _enforced ;
      string   _localAgentHost ;
      string   _localAgentService ;
      string   _clusterName ;
      string   _packageName ;
      string   _installPath ;
      string   _user ;
      string   _passwd ;

   } ;

   class omCreateRelationshipCommand : public omAuthCommand
   {
   public:
      omCreateRelationshipCommand( restAdaptor *pRestAdaptor,
                                   pmdRestSession *pRestSession,
                                   string &localAgentHost,
                                   string &localAgentService ) ;

      ~omCreateRelationshipCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _check( BSONObj &fromBuzInfo, BSONObj &toBuzInfo ) ;

      INT32 _createRelationship( const BSONObj &options,
                                 const BSONObj &fromBuzInfo,
                                 const BSONObj &toBuzInfo ) ;

      INT32 _generateRequest( const BSONObj &options,
                              const BSONObj &fromBuzInfo,
                              const BSONObj &toBuzInfo,
                              BSONObj &request ) ;

   private:
      string _localAgentHost ;
      string _localAgentService ;
      string _name ;
      string _fromBuzName ;
      string _toBuzName ;
   } ;

   class omRemoveRelationshipCommand : public omAuthCommand
   {
   public:
      omRemoveRelationshipCommand( restAdaptor *pRestAdaptor,
                                   pmdRestSession *pRestSession,
                                   string &localAgentHost,
                                   string &localAgentService ) ;

      ~omRemoveRelationshipCommand() ;

      virtual INT32 doCommand() ;

   private:
      INT32 _check( BSONObj &options, BSONObj &fromBuzInfo,
                    BSONObj &toBuzInfo ) ;

      INT32 _removeRelationship( const BSONObj &options,
                                 const BSONObj &fromBuzInfo,
                                 const BSONObj &toBuzInfo ) ;

      INT32 _generateRequest( const BSONObj &options,
                              const BSONObj &fromBuzInfo,
                              const BSONObj &toBuzInfo,
                              BSONObj &request ) ;

   private:
      string _localAgentHost ;
      string _localAgentService ;
      string _name ;
      string _fromBuzName ;
      string _toBuzName ;
   } ;

   class omListRelationshipCommand : public omAuthCommand
   {
   public:
      omListRelationshipCommand( restAdaptor *pRestAdaptor,
                                 pmdRestSession *pRestSession ) ;

      ~omListRelationshipCommand() ;

      virtual INT32 doCommand() ;
   } ;

   class omRegisterPluginsCommand : public omRestCommandBase
   {
   public:
      omRegisterPluginsCommand( restAdaptor *pRestAdaptor,
                                pmdRestSession *pRestSession ) ;

      ~omRegisterPluginsCommand() ;

      virtual INT32 doCommand() ;
   private:
      INT32 _check( const string &role, const string &publicKey ) ;

      INT32 _updatePlugin( omRestTool &restTool,
                           const string &pluginName,
                           const string &businessType,
                           const string &publicKey,
                           const string &serviceName ) ;

      INT32 _encrypt( const string &publicKey, const string &src,
                      string &dest ) ;
   } ;

   class omListPluginsCommand : public omAuthCommand
   {
   public:
      omListPluginsCommand( restAdaptor *pRestAdaptor,
                            pmdRestSession *pRestSession ) ;

      ~omListPluginsCommand() ;

      virtual INT32 doCommand() ;
   } ;
}

#endif /* OM_GETFILECOMMAND_HPP__ */

