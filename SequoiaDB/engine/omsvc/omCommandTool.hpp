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

   Source File Name = omCommandTool.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/17/2017  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_COMMAND_TOOL_HPP__
#define OM_COMMAND_TOOL_HPP__

#include "authCB.hpp"
#include "rtnCB.hpp"
#include "pmd.hpp"
#include "omManager.hpp"
#include "restAdaptor.hpp"
#include "../bson/bson.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <map>
#include <string>

using namespace bson ;
using namespace std ;
using namespace boost::property_tree;

namespace engine
{
   class omXmlTool ;
   class omConfigTool ;
   class omDatabaseTool ;
   class omTaskTool ;
   class omErrorTool ;
   class omArgOptions ;

   struct simpleAddressInfo : public SDBObject
   {
      string hostName ;
      string port ;
   } ;

   INT32 getPacketFile( const string &businessType, string &filePath ) ;
   BOOLEAN pathCompare( const string &p1, const string &p2 ) ;
   INT32 getMaxTaskID( INT64 &taskID ) ;
   INT32 createTask( INT32 taskType, INT64 taskID,
                     const string &taskName, const string &agentHost,
                     const string &agentService, const BSONObj &taskInfo,
                     const BSONArray &resultInfo ) ;
   INT32 removeTask( INT64 taskID ) ;

   class omXmlTool : public SDBObject
   {
   public:
      INT32 readXml2Bson( const string &fileName, BSONObj &obj ) ;

   private:
      BOOLEAN _isStringValue( ptree &pt ) ;
      BOOLEAN _isArray( ptree &pt ) ;
      void _recurseParseObj( ptree &pt, BSONObj &out ) ;
      void _parseArray( ptree &pt, BSONArrayBuilder &arrayBuilder ) ;
      void _xml2Bson( ptree &pt, BSONObj &out ) ;
   } ;

   class omConfigTool : public omXmlTool
   {
   public:
      omConfigTool( string &rootPath, string &languageFileSep ) :
            _rootPath( rootPath ),
            _languageFileSep( languageFileSep )
      {
      }

      string getBuzDeployTemplatePath( const string &businessType,
                                       const string &operationType ) ;
      string getBuzConfigTemplatePath( const string &businessType,
                                       const string &deployMod,
                                       BOOLEAN isSeparateConfig ) ;
      string getBuzConfigTemplatePath( const string &businessType,
                                       const string &deployMod,
                                       const string &isSeparateConfig ) ;

      INT32 readBuzTypeList( list<BSONObj> &businessList ) ;
      INT32 readBuzDeployTemplate( const string &businessType,
                                   const string &operationType,
                                   list<BSONObj> &objList ) ;
      INT32 readBuzConfigTemplate( const string &businessType,
                                   const string &deployMod,
                                   BOOLEAN isSeparateConfig,
                                   BSONObj &obj ) ;
      INT32 readBuzConfigTemplate( const string &businessType,
                                   const string &deployMod,
                                   const string &isSeparateConfig,
                                   BSONObj &obj ) ;

   private:
      string _rootPath ;
      string _languageFileSep ;
   } ;

   class omDatabaseTool : public SDBObject
   {
   public:

      omDatabaseTool( pmdEDUCB* cb ) : _cb( cb )
      {
         _pKRCB  = pmdGetKRCB() ;
         _pRTNCB = _pKRCB->getRTNCB() ;
         _pDpsCB = _pKRCB->getDPSCB();
         _pDMSCB = _pKRCB->getDMSCB() ;
      }

   public:

      //task
      INT64 getTaskIdOfRunningBuz( const string &businessName ) ;
      INT64 getTaskIdOfRunningHost( const string &hostName ) ;
      BOOLEAN hasTaskRunning() ;
      INT32 getMaxTaskID( INT64 &taskID ) ;
      INT32 removeTask( INT64 taskID ) ;

      //business
      INT32 addBusinessInfo( const INT32 addType,
                             const string &clusterName,
                             const string &businessName,
                             const string &businessType,
                             const string &deployMod,
                             const BSONObj &businessInfo ) ;

      INT32 getOneBusinessInfo( const string &businessName,
                                BSONObj &businessInfo ) ;

      INT32 getBusinessInfoOfCluster( const string &clusterName,
                                      list<BSONObj> &buzInfoList ) ;

      BOOLEAN isBusinessExistOfCluster( const string &clusterName,
                                        const string &businessName ) ;

      BOOLEAN isBusinessExist( const string &businessName ) ;

      INT32 upsertBusinessInfo( const string &businessName,
                                const BSONObj &newBusinessInfo,
                                INT64 &updateNum ) ;
      INT32 removeBusiness( const string &businessName ) ;

      //cluster
      INT32 addCluster( const BSONObj &clusterInfo ) ;
      INT32 getClusterInfo( const string &clusterName,
                            BSONObj &clusterInfo ) ;

      INT32 updateClusterInfo( const string &clusterName,
                               const BSONObj &clusterInfo ) ;

      BOOLEAN isClusterExist( const string &clusterName ) ;

      INT32 updateClusterGrantConf( const string &clusterName,
                                    const string &grantName,
                                    const BOOLEAN privilege ) ;

      //configure
      INT32 getOneNodeConfig( const string &businessName,
                              const string &hostName,
                              const string &svcname,
                              BSONObj &config ) ;
      INT32 getConfigByBusiness( const string &businessName,
                                 list<BSONObj> &configList ) ;
      INT32 getConfigByHostName( const string &hostName,
                                 BSONObj &config ) ;
      INT32 getBusinessAddressWithConfig(
                                    const string &businessName,
                                    vector<simpleAddressInfo> &addressList ) ;
      INT32 getCatalogAddressWithConfig( const string &businessName,
                                    vector<simpleAddressInfo> &addressList ) ;
      INT32 getHostUsedPort( const string &hostName,
                             vector<string> &portList ) ;
      BOOLEAN isConfigExist( const string &businessName,
                             const string &hostName ) ;
      BOOLEAN isConfigExistOfCluster( const string &hostName,
                                      const string &clusterName ) ;
      INT32 insertConfigure( const string &businessName,
                             const string &hostName,
                             const string &businessType,
                             const string &clusterName,
                             const string &deployMode,
                             const BSONObj &oneNodeConfig ) ;
      INT32 upsertConfigure( const string &businessName,
                             const string &hostName,
                             const BSONObj &newConfig,
                             INT64 &updateNum ) ;
      INT32 appendConfigure( const string &businessName,
                             const string &hostName,
                             const BSONObj &oneNodeConfig ) ;
      INT32 removeConfigure( const string &businessName,
                             const string &hostName ) ;
      INT32 removeConfigure( const string &businessName ) ;

      //auth
      INT32 getAuth( const string &businessName,
                     string &authUser, string &authPasswd ) ;
      INT32 getAuth( const string &businessName, BSONObj &authInfo ) ;
      INT32 upsertAuth( const string &businessName, const string &authUser,
                        const string &authPasswd ) ;
      INT32 upsertAuth( const string &businessName, const string &authUser,
                        const string &authPasswd, BSONObj &options ) ;
      INT32 removeAuth( const string &businessName ) ;

      //host
      INT32 upsertPackage( const string &hostName,
                           const string &packageName,
                           const string &installPath,
                           const string &version ) ;
      INT32 getHostNameByAddress( const string &address,
                                  string &hostName ) ;
      INT32 getHostInfoByAddress( const string &address,
                                  BSONObj &hostInfo ) ;
      INT32 getHostInfoByCluster( const string &clusterName,
                                  list<BSONObj> &hostList ) ;
      BOOLEAN isHostExistOfClusterByAddr( const string &address,
                                          const string &clusterName ) ;
      BOOLEAN isHostExistOfCluster( const string &hostName,
                                    const string &clusterName ) ;
      BOOLEAN isHostExistOfClusterByIp( const string &IP,
                                        const string &clusterName ) ;
      BOOLEAN isHostHasPackage( const string &hostName,
                                const string &packageName ) ;
      BOOLEAN getHostPackagePath( const string &hostName,
                                  const string &packageName,
                                  string &installPath ) ;

      INT32 removeHost( const string &address,
                        const string &clusterName ) ;

      //relationship
      INT32 createRelationship( const string &name,
                                const string &fromBuzName,
                                const string &toBuzName,
                                const BSONObj &options ) ;
      BOOLEAN isRelationshipExist( const string &name ) ;
      BOOLEAN isRelationshipExistByBusiness( const string &businessName ) ;
      INT32 getRelationshipInfo( const string &name,
                                 string &fromBuzName, string &toBuzName ) ;
      INT32 getRelationshipOptions( const string &name, BSONObj &options ) ;
      INT32 getRelationshipList( list<BSONObj> &relationshipList ) ;
      INT32 removeRelationship( const string &name ) ;

      //plugin
      BOOLEAN isPluginExist( const string &name ) ;
      BOOLEAN isPluginBusinessTypeExist( const string &businessType ) ;
      INT32 getPluginInfoByBusinessType( const string &businessType,
                                         BSONObj &info ) ;
      INT32 getPluginList( list<BSONObj> &pluginList ) ;
      INT32 upsertPlugin( const string &name, const string &businessType,
                          const string &serviceName ) ;
      INT32 removePlugin( const string &name, const string &businessType ) ;

      //trans
      INT32 addPackageOfHosts( set<string> &hostList,
                               const string &packageName,
                               const string &installPath,
                               const string &version ) ;
      INT32 addNodeConfigOfBusiness( const string &clusterName,
                                     const string &businessName,
                                     const string &businessType,
                                     const BSONObj &newConfig ) ;
      INT32 updateNodeConfigOfBusiness( const string &businessName,
                                        const BSONObj &newConfig ) ;
      INT32 unbindBusiness( const string &businessName ) ;
      INT32 unbindHost( const string &clusterName,
                        list<string> &hostList ) ;

      //collection
      INT32 createCollection( const CHAR *pCollection ) ;
      INT32 createCollectionIndex( const CHAR *pCollection,
                                   const CHAR *pIndex ) ;
      INT32 removeCollectionIndex( const CHAR *pCollection,
                                   const CHAR *pIndex ) ;

   private:
      //task
      INT32 _getOneTasktInfo( const BSONObj &matcher, const BSONObj &selector,
                              BSONObj &taskInfo ) ;

      //host
      INT32 _getOneHostInfo( const BSONObj &matcher, const BSONObj &selector,
                             BSONObj &hostInfo ) ;

      //configure
      INT32 _getOneConfigure( const BSONObj &condition, const BSONObj &selector,
                              BSONObj &configure ) ;
      INT32 _removeConfigure( const BSONObj &condition ) ;

      //relationship
      INT32 _getOneRelationship( const BSONObj &condition,
                                 const BSONObj &selector,
                                 BSONObj &info ) ;

      //plugin
      INT32 _getOnePluginInfo( const BSONObj &condition,
                               const BSONObj &selector,
                               BSONObj &info ) ;

   private:
      pmdEDUCB    *_cb ;
      SDB_RTNCB   *_pRTNCB ;
      pmdKRCB     *_pKRCB ;
      SDB_DPSCB   *_pDpsCB ;
      SDB_DMSCB   *_pDMSCB ;
   } ;

   class omAuthTool : public SDBObject
   {
   public:
      omAuthTool( pmdEDUCB *cb, SDB_AUTHCB *pAuthCB ) : _cb( cb ),
                                                        _pAuthCB( pAuthCB )
      {
      }
   public:
      INT32 createOmsvcDefaultUsr( const CHAR *pPluginPasswd,
                                   INT32 pluginPasswdLen ) ;
      INT32 createAdminUsr() ;
      INT32 createPluginUsr( const CHAR *pPasswd, INT32 length ) ;

      INT32 getUsrInfo( const string &user, BSONObj &info ) ;
      void getPluginPasswd( string &passwd ) ;

      void generateRandomVisualString( CHAR* pPasswd, INT32 length ) ;

   private:
      pmdEDUCB    *_cb ;
      SDB_AUTHCB  *_pAuthCB ;
   } ;

   class omRestTool : public SDBObject
   {
   public:

      omRestTool( ossSocket *socket, restAdaptor *pRestAdaptor,
                  restResponse *response ) ;

      void sendRecord2Web( list<BSONObj> &records,
                           const BSONObj *pFilter = NULL,
                           BOOLEAN inFilter = TRUE ) ;

      void sendOkRespone() ;

      void sendResponse( INT32 rc, const string &detail ) ;
      void sendResponse( INT32 rc, const char *pDetail ) ;
      void sendResponse( const BSONObj &msg ) ;

      INT32 appendResponeContent( const BSONObj &content ) ;

      void appendResponeMsg( const BSONObj &msg ) ;

   private:
      ossSocket      *_socket ;
      restAdaptor    *_pRestAdaptor ;
      restResponse   *_response ;

      list<BSONObj> _msgList ;
   } ;

   class omTaskTool : public SDBObject
   {
   public:
      omTaskTool( pmdEDUCB *cb, string &localAgentHost,
                  string &localAgentService) :
            _cb( cb ),
            _localAgentHost( localAgentHost ),
            _localAgentService( localAgentService )
      {
      }
      INT32 createTask( INT32 taskType, INT64 taskID, const string &taskName,
                        const BSONObj &taskInfo, const BSONArray &resultInfo ) ;
      INT32 notifyAgentMsg( const CHAR *pCmd, const BSONObj &request,
                            string &errDetail, BSONObj &result ) ;
      INT32 notifyAgentTask( INT64 taskID, string &errDetail ) ;

   private:
      INT32 _sendMsgToLocalAgent( omManager *om,
                                  pmdRemoteSession *pRemoteSession,
                                  MsgHeader *pMsg ) ;
      INT32 _receiveFromAgent( pmdRemoteSession *pRemoteSession,
                               SINT32 &flag, BSONObj &result ) ;
      void _clearSession( omManager *om, pmdRemoteSession *pRemoteSession ) ;

   private:
      pmdEDUCB* _cb ;
      string _localAgentHost ;
      string _localAgentService ;
   } ;

   class omErrorTool : public SDBObject
   {
   public:
      omErrorTool() : _isSet( FALSE )
      {
      }
      void setError( BOOLEAN isCover, const CHAR *pFormat, ... ) ;
      const CHAR *getError() ;

   private:
      CHAR _errorDetail[ PD_LOG_STRINGMAX + 1 ] ;
      BOOLEAN _isSet ;
   } ;

   class omArgOptions : public SDBObject
   {
   public:
      omArgOptions( restRequest *pRequest ) ;

      INT32 parseRestArg( const CHAR *pFormat, ... ) ;

      const CHAR *getErrorMsg() ;

   private:
      INT32 _parserArg( const CHAR *pFormat, va_list &vaList ) ;

   private:
      restRequest      *_request ;
      omErrorTool       _errorMsg ;

   } ;
}

#endif /* OM_COMMAND_TOOL_HPP__ */
