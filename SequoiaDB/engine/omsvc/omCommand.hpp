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

   #define REST_CONSTRUCTOR_PARA_INHERIT( classA, classB ) \
      classA( OMREST_CLASS_PARAMETER ): classB( OMREST_CLASS_INPUT_PARAMETER )

   class omAuthCommand : public omRestCommandBase
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omAuthCommand, omRestCommandBase )
         {
         }

         ~omAuthCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      public:
         virtual INT32   doCommand() ;

         virtual const CHAR* name() { return "" ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omExtendBusinessCommand, omAuthCommand )
      {
         _force = FALSE ;
      }

      ~omExtendBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_EXTEND_BUSINESS_REQ ; }

      virtual INT32 doCommand() ;

   private:
      INT32 _check( BSONObj &extendConfig ) ;
      INT32 _checkRestInfo( BSONObj &extendConfig ) ;
      INT32 _readConfigProperties( BSONObj& buzDetail ) ;
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
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _extendMod ;
      string _deployMod ;
      BOOLEAN _force ;
   } ;

   class omShrinkBusinessCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omShrinkBusinessCommand, omAuthCommand )
      {
      }

      ~omShrinkBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_SHRINK_BUSINESS_REQ ; }

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
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
   } ;

   class omLogoutCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omLogoutCommand, omAuthCommand )
         {
         }

         ~omLogoutCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LOGOUT_REQ ; }

         virtual INT32   doCommand() ;
   };

   class omChangePasswdCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omChangePasswdCommand, omAuthCommand )
         {
         }

         ~omChangePasswdCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_CHANGE_PASSWD_REQ ; }

         virtual INT32   doCommand() ;
   };

   class omCheckSessionCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omCheckSessionCommand, omAuthCommand )
         {
         }

         ~omCheckSessionCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_CHECK_SESSION_REQ ; }

         virtual INT32   doCommand() ;

      protected:
   };

   class omCreateClusterCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omCreateClusterCommand,
                                     omAuthCommand )
      {
      }

      ~omCreateClusterCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_CREATE_CLUSTER_REQ ; }

      virtual INT32 doCommand() ;

   private:

      INT32 _getRestInfo( string &clusterName, BSONObj &clusterInfo ) ;

   } ;

   class omQueryClusterCommand : public omCheckSessionCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryClusterCommand,
                                        omCheckSessionCommand )
         {
         }

         ~omQueryClusterCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_QUERY_CLUSTER_REQ ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omUpdateHostInfoCommand,
                                        omCheckSessionCommand )
         {
         }

         ~omUpdateHostInfoCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_UPDATE_HOST_INFO_REQ ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omScanHostCommand,
                                        omCheckSessionCommand )
         {
         }

         ~omScanHostCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_SCAN_HOST_REQ ; }

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
   };

   class omCheckHostCommand : public omScanHostCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omCheckHostCommand,
                                        omScanHostCommand )
         {
         }

         ~omCheckHostCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_CHECK_HOST_REQ ; }

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
         INT32           _getTmpAgentPort( pmdSubSession *subSession,
                                           string &tmpAgentPort ) ;

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
         REST_CONSTRUCTOR_PARA_INHERIT( omAddHostCommand, omScanHostCommand )
         {
         }

         ~omAddHostCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_ADD_HOST_REQ ; }

         virtual INT32   doCommand() ;

      protected:
                         // overwrite
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
         REST_CONSTRUCTOR_PARA_INHERIT( omListHostCommand,
                                        omCheckSessionCommand )
         {
         }

         ~omListHostCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_HOST_REQ ; }

         virtual INT32   doCommand() ;

      protected:
         void            _sendHostInfo2Web( list<BSONObj> &hosts ) ;

      private:
   } ;

   class omQueryHostCommand : public omListHostCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryHostCommand, omListHostCommand )
         {
         }

         ~omQueryHostCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_QUERY_HOST_REQ ; }

         virtual INT32   doCommand() ;

      private:
   } ;

   class omListBusinessTypeCommand : public omCheckSessionCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omListBusinessTypeCommand,
                                        omCheckSessionCommand )
         {
         }

         ~omListBusinessTypeCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_BUSINESS_TYPE_REQ ; }

         virtual INT32  doCommand() ;

      protected:
         INT32          _readConfigFile( const string &file, BSONObj &obj ) ;
         void           _recurseParseObj( ptree &pt, BSONObj &out ) ;
         void           _parseArray( ptree &pt,
                                     BSONArrayBuilder &arrayBuilder ) ;
         BOOLEAN        _isStringValue( ptree &pt ) ;
         BOOLEAN        _isArray( ptree &pt ) ;

         INT32          _getBusinessList( list<BSONObj> &businessList ) ;
   } ;

   class omGetBusinessTemplateCommand : public omListBusinessTypeCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omGetBusinessTemplateCommand,
                                        omListBusinessTypeCommand )
         {
         }

         ~omGetBusinessTemplateCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_GET_BUSINESS_TEMPLATE_REQ ; }

         virtual INT32  doCommand() ;

      protected:
         INT32          _readConfTemplate( const string &businessType,
                                           const string &file,
                                           list<BSONObj> &clusterTypeList ) ;
         INT32          _readConfDetail( const string &file,
                                         BSONObj &bsonConfDetail ) ;

   } ;

   class omGetConfigTemplateCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omGetConfigTemplateCommand,
                                     omAuthCommand )
      {
      }

      ~omGetConfigTemplateCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_GET_CONFIG_TEMPLATE_REQ ; }

      virtual INT32 doCommand() ;

   private:
      INT32 _getTemplate( BSONObj &configTemplate ) ;

   private:
      string _businessType ;
   } ;

   class omGetBusinessConfigCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omGetBusinessConfigCommand,
                                     omAuthCommand )
      {
      }

      ~omGetBusinessConfigCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_CONFIG_BUSINESS_REQ ; }

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
      string _clusterName ;
      string _deployMod ;
      string _businessType ;
      string _businessName ;
      string _operationType ;
   } ;

   class omAddBusinessCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omAddBusinessCommand, omAuthCommand )
      {
         _force = FALSE ;
      }

      ~omAddBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_INSTALL_BUSINESS_REQ ; }

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
      string _clusterName ;
      string _businessName ;
      string _businessType ;
      string _deployMod ;
      BOOLEAN _force ;
   } ;

   class omListTaskCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omListTaskCommand, omAuthCommand )
         {
         }

         ~omListTaskCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_TASK_REQ ; }

         virtual INT32  doCommand() ;

      private:
         INT32          _getTaskList( list<BSONObj> &taskList ) ;
         void           _sendTaskList2Web( list<BSONObj> &taskList ) ;
   } ;

   class omQueryTaskCommand : public omScanHostCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryTaskCommand, omScanHostCommand )
         {
         }

         ~omQueryTaskCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_QUERY_TASK_REQ ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omListNodeCommand, omAuthCommand )
         {
         }

         ~omListNodeCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_NODE_REQ ; }

         virtual INT32  doCommand() ;

      private:
         INT32          _getNodeList( string businessName,
                                      list<simpleNodeInfo> &nodeList ) ;
         void           _sendNodeList2Web( list<simpleNodeInfo> &nodeList ) ;
   } ;

   class omGetNodeConfCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omGetNodeConfCommand, omAuthCommand )
         {
         }

         ~omGetNodeConfCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_GET_NODE_CONF_REQ ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omQueryNodeConfCommand, omAuthCommand )
      {
      }

      ~omQueryNodeConfCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_QUERY_NODE_CONF_REQ ; }

      virtual INT32 doCommand() ;
   } ;

   class omQueryBusinessCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryBusinessCommand, omAuthCommand )
         {
         }

         ~omQueryBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_QUERY_BUSINESS_REQ ; }

         virtual INT32  doCommand() ;

      private:
         void           _sendBusinessInfo2Web( list<BSONObj> &businessInfo ) ;
   } ;

   class omListBusinessCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omListBusinessCommand, omAuthCommand )
         {
         }

         ~omListBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_BUSINESS_REQ ; }

         virtual INT32  doCommand() ;

      private:
         void           _sendBusinessList2Web( list<BSONObj> &businessList ) ;
   };

   class omListHostBusinessCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omListHostBusinessCommand,
                                        omAuthCommand )
         {
         }

         ~omListHostBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_LIST_HOST_BUSINESS_REQ ; }

         virtual INT32  doCommand() ;

      private:
         void           _sendHostBusiness2Web( list<BSONObj> &businessList ) ;
   };

   class omStartBusinessCommand : public omScanHostCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omStartBusinessCommand,
                                        omScanHostCommand )
         {
         }

         ~omStartBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return "" ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omStopBusinessCommand,
                                        omScanHostCommand )
         {
         }

         ~omStopBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return "" ; }

         virtual INT32  doCommand() ;
   } ;

   class omRemoveClusterCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omRemoveClusterCommand, omAuthCommand )
         {
         }

         ~omRemoveClusterCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_REMOVE_CLUSTER_REQ ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omRemoveHostCommand, omAuthCommand )
      {
      }

      ~omRemoveHostCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_REMOVE_HOST_REQ ; }

      virtual INT32  doCommand() ;

   private:
      INT32 _check( const BSONObj &hostList ) ;

      INT32 _generateRequest( const BSONObj &hostList,
                              BSONObj &taskConfig, BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig, const BSONArray &resultInfo,
                         INT64 &taskID ) ;

   private:
      string _clusterName ;
   } ;

   class omRemoveBusinessCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omRemoveBusinessCommand, omAuthCommand )
      {
         _force = FALSE ;
      }

      ~omRemoveBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_REMOVE_BUSINESS_REQ ; }

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
      BOOLEAN  _force ;
      string   _clusterName ;
      string   _businessName ;
      string   _businessType ;
      string   _deployMod ;
   } ;

   class omQueryHostStatusCommand : public omStartBusinessCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryHostStatusCommand,
                                        omStartBusinessCommand )
         {
         }

         ~omQueryHostStatusCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_QUERY_HOST_STATUS_REQ ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omPredictCapacity, omAuthCommand )
         {
         }

         ~omPredictCapacity()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_PREDICT_CAPACITY_REQ ; }

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
         REST_CONSTRUCTOR_PARA_INHERIT( omGetLogCommand, omAuthCommand )
         {
         }

         ~omGetLogCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_GET_LOG_REQ ; }

         virtual INT32   doCommand() ;

      protected:
         INT32           _getFileContent( string filePath, CHAR **pFileContent,
                                          INT32 &fileContentLen ) ;
   } ;

   class omSetBusinessAuthCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omSetBusinessAuthCommand, omAuthCommand )
      {
      }

      ~omSetBusinessAuthCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_SET_BUSINESS_AUTH_REQ ; }

      virtual INT32 doCommand() ;
   } ;

   class omRemoveBusinessAuthCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omRemoveBusinessAuthCommand,
                                        omAuthCommand )
         {
         }

         ~omRemoveBusinessAuthCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_REMOVE_BUSINESS_AUTH_REQ ; }

         virtual INT32   doCommand() ;
   } ;

   class omQueryBusinessAuthCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omQueryBusinessAuthCommand,
                                        omAuthCommand )
         {
         }

         ~omQueryBusinessAuthCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_QUERY_BUSINESS_AUTH_REQ ; }

         virtual INT32   doCommand() ;

      protected:
         void            _sendAuthInfo2Web( list<BSONObj> &authInfo ) ;
   } ;

   class omDiscoverBusinessCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omDiscoverBusinessCommand, omAuthCommand )
      {
      }

      ~omDiscoverBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_DISCOVER_BUSINESS_REQ ; }

      virtual INT32 doCommand() ;

   private:

      INT32 _checkHostPort( const string &hostName, const string &port ) ;
      INT32 _checkBusinssCFG( BSONObj &configInfo ) ;
      INT32 _checkWebLinkCFG( BSONObj &buzInfo ) ;
      INT32 _checkSequoiaDBCFG( BSONObj &buzInfo ) ;
      INT32 _checkMySQLCFG( BSONObj &buzInfo ) ;
      INT32 _checkPostgreSQLCFG( BSONObj &buzInfo ) ;

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
      INT32 _syncSQL( omRestTool &restTool, const BSONObj &buzInfo ) ;
      INT32 _storeBusinessInfo( const INT32 addType,
                                const string &deployMod,
                                const BSONObj &buzInfo ) ;
      INT32 _syncBusiness( omRestTool &restTool, BSONObj &configInfo ) ;

   private:

      string _clusterName ;
      string _businessName ;
      string _businessType ;

   } ;

   class omUnDiscoverBusinessCommand : public omAuthCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omUnDiscoverBusinessCommand,
                                        omAuthCommand )
         {
         }

         ~omUnDiscoverBusinessCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_UNDISCOVER_BUSINESS_REQ ; }

         virtual INT32 doCommand() ;

      protected:
         INT32 _UnDiscoverBusiness( const string &clusterName,
                                    const string &businessName ) ;
   } ;

   class omSsqlExecCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omSsqlExecCommand, omAuthCommand )
      {
      }

      ~omSsqlExecCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_SSQL_EXEC_REQ ; }

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
   } ;

   class omInterruptTaskCommand : public omScanHostCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omInterruptTaskCommand,
                                        omScanHostCommand )
         {
         }

         ~omInterruptTaskCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return OM_INTERRUPT_TASK_REQ ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omForwardPluginCommand, omAuthCommand )
      {
      }

      ~omForwardPluginCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return "" ; }

      virtual INT32 doCommand() ;
   } ;

   class omGetFileCommand : public omGetLogCommand
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omGetFileCommand, omGetLogCommand )
         {
         }

         ~omGetFileCommand()
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return "" ; }

         virtual INT32   doCommand() ;

         virtual INT32   undoCommand() ;
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

   class omStrategyCmdBase : public omRestCommandBase
   {
      public:
         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyCmdBase, omRestCommandBase )
         {
         }

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

         virtual const CHAR* name() { return "" ; }

         virtual ~omStrategyCmdBase() ;

      protected:
         INT32    _getAndCheckBusiness( string &clsName,
                                        string &bizName ) ;
   } ;

   class omStrategyTaskInsert : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_ADD_REQ ; }

         virtual INT32 doCommand() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyTaskInsert,
                                        omStrategyCmdBase )
         {
         }

         ~omStrategyTaskInsert() ;
   } ;

   class omStrategyTaskList : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_LIST_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyTaskList, omStrategyCmdBase )
         {
         }

         ~omStrategyTaskList() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyUpdateTaskStatus : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_UPDATE_STATUS_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyUpdateTaskStatus,
                                        omStrategyCmdBase )
         {
         }

         ~omStrategyUpdateTaskStatus() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyTaskDel : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_DEL_REQ ; }

         virtual INT32 doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyTaskDel, omStrategyCmdBase )
         {
         }

         ~omStrategyTaskDel() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyInsert : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_ADD_REQ ; }

         virtual INT32 doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyInsert, omStrategyCmdBase )
         {
         }

         ~omStrategyInsert() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   } ;

   class omStrategyList : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_LIST_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyList, omStrategyCmdBase )
         {
         }

         ~omStrategyList() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   } ;

   class omStrategyUpdateNice : public omStrategyCmdBase
   {
   public:

      virtual const CHAR* name() { return OM_TASK_STRATEGY_UPDATE_NICE_REQ ; }

      virtual INT32   doCommand() ;

   public:

      REST_CONSTRUCTOR_PARA_INHERIT( omStrategyUpdateNice, omStrategyCmdBase )
      {
      }

      ~omStrategyUpdateNice() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyAddIps : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_ADD_IPS_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyAddIps, omStrategyCmdBase )
         {
         }

         ~omStrategyAddIps() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyDelIps : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_DEL_IPS_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyDelIps, omStrategyCmdBase )
         {
         }

         ~omStrategyDelIps() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyDel : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_DEL_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyDel, omStrategyCmdBase )
         {
         }

         ~omStrategyDel() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyUpdateStatus : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_UPDATE_STAT_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyUpdateStatus,
                                        omStrategyCmdBase )
         {
         }

         ~omStrategyUpdateStatus() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omStrategyUpdateSortID : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_UPDATE_SORT_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyUpdateSortID,
                                        omStrategyCmdBase )
         {
         }

         ~omStrategyUpdateSortID() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   } ;

   class omStrategyUpdateUser : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_UPDATE_USER_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyUpdateUser,
                                        omStrategyCmdBase )
         {
         }

         ~omStrategyUpdateUser() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   } ;

   class omStrategyFlush : public omStrategyCmdBase
   {
      public:

         virtual const CHAR* name() { return OM_TASK_STRATEGY_FLUSH ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omStrategyFlush, omStrategyCmdBase )
         {
         }

         ~omStrategyFlush() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   } ;

   class omGetSystemInfoCommand : public omAuthCommand
   {
      public:

         virtual const CHAR* name() { return OM_GET_SYSTEM_INFO_REQ ; }

         virtual INT32   doCommand() ;

      public:

         REST_CONSTRUCTOR_PARA_INHERIT( omGetSystemInfoCommand, omAuthCommand )
         {
         }

         ~omGetSystemInfoCommand() ;

         DECLARE_OMREST_CMD_AUTO_REGISTER() ;

   };

   class omSyncBusinessConfigureCommand : public omAuthCommand
   {
   public:

      REST_CONSTRUCTOR_PARA_INHERIT( omSyncBusinessConfigureCommand,
                                     omAuthCommand )
      {
      }

      ~omSyncBusinessConfigureCommand();

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;


      virtual const CHAR* name() { return OM_SYNC_BUSINESS_CONF_REQ ; }

      virtual INT32 doCommand() ;

   private:

      void _parseHostMap( const BSONObj &hosts, map<string, string> &hostMap ) ;

      void _hostName2Address( map<string, string> &hostMap,
                              string &hostName, string &address ) ;

      INT32 _checkExecResult( omRestTool &restTool, const BSONObj &result,
                              map<string, string> &hostMap ) ;

      INT32 _syncBusinessConfig( omRestTool &restTool,
                                 vector<simpleAddressInfo> &addressList ) ;

      INT32 _generateRequest( vector<simpleAddressInfo> &addressList,
                              BSONObj &request ) ;

   private:
      string _clusterName ;
      string _businessName ;
      string _businessType ;
   } ;

   class omGrantSysConfigureCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omGrantSysConfigureCommand,
                                     omAuthCommand )
      {
      }

      ~omGrantSysConfigureCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_GRANT_SYSCONF_REQ ; }

      virtual INT32 doCommand() ;

   private:

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
      REST_CONSTRUCTOR_PARA_INHERIT( omUnbindBusinessCommand, omAuthCommand )
      {
      }

      ~omUnbindBusinessCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_UNBIND_BUSINESS_REQ ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omUnbindHostCommand, omAuthCommand )
      {
      }

      ~omUnbindHostCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_UNBIND_HOST_REQ ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omDeployPackageCommand, omAuthCommand )
      {
         _enforced = FALSE ;
      }

      ~omDeployPackageCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_DEPLOY_PACKAGE_REQ ; }

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
      string   _clusterName ;
      string   _packageName ;
      string   _installPath ;
      string   _user ;
      string   _passwd ;

   } ;

   class omCreateRelationshipCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omCreateRelationshipCommand,
                                     omAuthCommand )
      {
      }

      ~omCreateRelationshipCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_CREATE_RELATIONSHIP_REQ ; }

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
      string _name ;
      string _fromBuzName ;
      string _toBuzName ;
   } ;

   class omRemoveRelationshipCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omRemoveRelationshipCommand,
                                     omAuthCommand )
      {
      }

      ~omRemoveRelationshipCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_REMOVE_RELATIONSHIP_REQ ; }

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
      string _name ;
      string _fromBuzName ;
      string _toBuzName ;
   } ;

   class omListRelationshipCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omListRelationshipCommand, omAuthCommand )
      {
      }

      ~omListRelationshipCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_LIST_RELATIONSHIP_REQ ; }

      virtual INT32 doCommand() ;
   } ;

   class omRegisterPluginsCommand : public omRestCommandBase
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omRegisterPluginsCommand,
                                     omRestCommandBase )
      {
      }

      ~omRegisterPluginsCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return "" ; }

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
      REST_CONSTRUCTOR_PARA_INHERIT( omListPluginsCommand, omAuthCommand )
      {
      }

      ~omListPluginsCommand() ;

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_LIST_PLUGIN_REQ ; }

      virtual INT32 doCommand() ;
   } ;

   class omRestartBusinessCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omRestartBusinessCommand, omAuthCommand )
      {
      }

      ~omRestartBusinessCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      virtual const CHAR* name() { return OM_RESTART_BUSINESS_REQ ; }

      virtual INT32 doCommand() ;

   private:
      INT32 _check() ;

      INT32 _generateRequest( const BSONObj &options,
                              BSONObj &taskConfig,
                              BSONArray &resultInfo ) ;

      void _generateTaskConfig( list<BSONObj> &configList,
                                BSONObj &taskConfig ) ;

      void _generateResultInfo( list<BSONObj> &configList,
                                BSONArray &resultInfo ) ;

      INT32 _createTask( const BSONObj &taskConfig,
                         const BSONArray &resultInfo,
                         INT64 &taskID ) ;

      BOOLEAN _isRequestNode( list<simpleAddressInfo> &addrList,
                              const string &hostName, const string &port ) ;

      INT32 _getBusinessConfig( const BSONObj &options,
                                list<BSONObj> &configList ) ;

   private:
      string _clusterName ;
      string _businessName ;
      string _businessType ;
   } ;

   class omModifyBusinessConfigCommand : public omAuthCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omModifyBusinessConfigCommand,
                                     omAuthCommand )
      {
      }

      ~omModifyBusinessConfigCommand()
      {
      }

      virtual const CHAR* name() { return OM_MODIFY_BUSINESS_CONFIG_REQ ; }

      virtual INT32 doCommand() ;

   private:
      INT32 _check( BSONObj &configInfo ) ;

      INT32 _generateRequest( BSONObj &configInfo, BSONObj &request ) ;

      INT32 _modifyConfig( BSONObj &configInfo, BSONObj &result ) ;

   private:
      string _clusterName ;
      string _businessName ;
      string _businessType ;
   } ;

   class omUpdateBusinessConfigCommand : public omModifyBusinessConfigCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omUpdateBusinessConfigCommand,
                                     omModifyBusinessConfigCommand )
      {
      }

      ~omUpdateBusinessConfigCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      const CHAR* name() { return OM_UPDATE_BUSINESS_CONFIG_REQ ; }
   } ;

   class omDeleteBusinessConfigCommand : public omModifyBusinessConfigCommand
   {
   public:
      REST_CONSTRUCTOR_PARA_INHERIT( omDeleteBusinessConfigCommand,
                                     omModifyBusinessConfigCommand )
      {
      }

      ~omDeleteBusinessConfigCommand()
      {
      }

      DECLARE_OMREST_CMD_AUTO_REGISTER() ;

      const CHAR* name() { return OM_DELETE_BUSINESS_CONFIG_REQ ; }
   } ;
}

#endif /* OM_GETFILECOMMAND_HPP__ */

