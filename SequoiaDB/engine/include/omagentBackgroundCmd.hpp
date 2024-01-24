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

   Source File Name = omagentBackgroundCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_BACKGROUD_CMD_HPP_
#define OMAGENT_BACKGROUD_CMD_HPP_

#include "omagentCmdBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _omaAsyncCommand
   */
   class _omaAsyncCommand : public _omaCommand
   {
   public:
      _omaAsyncCommand() ;
      virtual ~_omaAsyncCommand() ;

   protected:
      virtual void _aggrFlowArray( const BSONObj& array1,
                                   const BSONObj& array2,
                                   BSONArray& out ) ;
   } ;

   /*
      _omaAddHost
   */
   class _omaAddHost : public _omaCommand
   {
      public:
         _omaAddHost ( AddHostInfo &info ) ;
         ~_omaAddHost () ;
         virtual const CHAR * name () { return OMA_CMD_ADD_HOST ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT32 _getAddHostInfo( BSONObj &retObj1, BSONObj &retObj2 ) ;
         
      private:
         AddHostInfo   _addHostInfo ;
   } ;

   /*
      _omaCheckAddHostInfo
   */
   class _omaCheckAddHostInfo : public _omaCommand
   {
      public:
         _omaCheckAddHostInfo() ;
         ~_omaCheckAddHostInfo () ;
         
      public:
         virtual const CHAR* name () { return OMA_CMD_CHECK_ADD_HOST_INFO ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /*
      _omaRemoveHost
   */
   class _omaRemoveHost : public _omaCommand
   {
      public:
         _omaRemoveHost ( RemoveHostInfo &info ) ;
         ~_omaRemoveHost () ;
         virtual const CHAR * name () { return OMA_CMD_REMOVE_HOST ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT32 _getRemoveHostInfo( BSONObj &retObj1, BSONObj &retObj2 ) ;
         
      private:
         RemoveHostInfo   _removeHostInfo ;
   } ;

   /*
      _omaCreateTmpCoord
   */
   class _omaCreateTmpCoord : public _omaCommand
   {
      public:
         _omaCreateTmpCoord( INT64 taskID ) ;
         ~_omaCreateTmpCoord() ;
         virtual const CHAR* name() { return OMA_CMD_INSTALL_TMP_COORD ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      public:
         INT32 createTmpCoord( BSONObj &cfgObj, BSONObj &retObj ) ;

      private:
         INT64   _taskID ;
   } ;

   /*
      _omaRemoveTmpCoord
   */
   class _omaRemoveTmpCoord : public _omaCommand
   {
      public:
         _omaRemoveTmpCoord( INT64 taskID, string &tmpCoordSvcName ) ;
         ~_omaRemoveTmpCoord() ;
         virtual const CHAR* name() { return OMA_CMD_REMOVE_TMP_COORD ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;
         
      public:
         INT32 removeTmpCoord( BSONObj &retObj ) ;

      private:
         INT64  _taskID ;
         string _tmpCoordSvcName ;
   } ;

   /*
      install standalone
   */
   class _omaInstallStandalone : public _omaCommand
   {
      public:
         _omaInstallStandalone ( INT64 taskID, InstDBInfo &info ) ;
         virtual ~_omaInstallStandalone () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_INSTALL_STANDALONE ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64           _taskID ;
         InstDBInfo      _info ;
   } ;

   /*
      install catalog
   */
   class _omaInstallCatalog : public _omaCommand
   {
      public:
         _omaInstallCatalog ( INT64 taskID, string &tmpCoordSvcName,
                              InstDBInfo &info ) ;
         virtual ~_omaInstallCatalog () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_INSTALL_CATALOG ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64         _taskID ;
         string        _tmpCoordSvcName ;
         InstDBInfo    _info ;
   } ;

   /*
      install coord
   */
   class _omaInstallCoord : public _omaCommand
   {
      public:
         _omaInstallCoord ( INT64 taskID, string &tmpCoordSvcName,
                            InstDBInfo &info ) ;
         virtual ~_omaInstallCoord () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_INSTALL_COORD ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64         _taskID ;
         string        _tmpCoordSvcName ;
         InstDBInfo    _info ;

   } ;

   /*
      install data node
   */
   class _omaInstallDataNode : public _omaCommand
   {
      public:
         _omaInstallDataNode ( INT64 taskID, string tmpCoordSvcName,
                               InstDBInfo &info ) ;
         virtual ~_omaInstallDataNode () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_INSTALL_DATA_NODE ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64         _taskID ;
         string        _tmpCoordSvcName ;
         InstDBInfo    _info ;
   } ;

   /*
      rollback standalone
   */
   class _omaRollbackStandalone : public _omaCommand
   {
      public:
         _omaRollbackStandalone( BSONObj &bus, BSONObj &sys ) ;
         ~_omaRollbackStandalone() ;

      public:
         virtual const CHAR* name() { return OMA_ROLLBACK_STANDALONE ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj   _bus ;
         BSONObj   _sys ;
   } ;

   /*
      rollback catalog
   */
   class _omaRollbackCatalog : public _omaCommand
   {
      public:
         _omaRollbackCatalog ( INT64 taskID,
                               string &tmpCoordSvcName ) ;
         ~_omaRollbackCatalog () ;

      public:
         virtual const CHAR* name () { return OMA_ROLLBACK_CATALOG ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64    _taskID ;
         string   _tmpCoordSvcName ;
   } ;

   /*
      rollback coord
   */
   class _omaRollbackCoord : public _omaCommand
   {
      public:
         _omaRollbackCoord ( INT64 taskID,
                             string &tmpCoordSvcName ) ;
         ~_omaRollbackCoord () ;

      public:
         virtual const CHAR* name () { return OMA_ROLLBACK_COORD ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT64    _taskID ;
         string   _tmpCoordSvcName ;
   } ;

   /*
      rollback data groups
   */
   class _omaRollbackDataRG : public _omaCommand
   {
      public:
         _omaRollbackDataRG (  INT64 taskID,
                               string &tmpCoordSvcNamem,
                               set<string> &info ) ;
         ~_omaRollbackDataRG () ;

      public:
         virtual const CHAR* name () { return OMA_ROLLBACK_DATA_RG ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         void _getInstalledDataGroupInfo( BSONObj &obj ) ;         

      private:
         INT64         _taskID ;
         string        _tmpCoordSvcName ;
         set<string>   &_info ;
   } ;

   /*
      remove standalone 
   */
   class _omaRmStandalone : public _omaCommand
   {
      public:
         _omaRmStandalone ( BSONObj &bus, BSONObj &sys ) ;
         virtual ~_omaRmStandalone () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_RM_STANDALONE ; }
         virtual INT32 init ( const CHAR *pUninstallInfo ) ;

      private:
         BSONObj   _bus ;
         BSONObj   _sys ;
   } ;

   /*
      remove catalog group 
   */
   class _omaRmCataRG : public _omaCommand
   {
      public:
         _omaRmCataRG ( INT64 taskID, string &tmpCoordSvcName, BSONObj &info ) ;
         virtual ~_omaRmCataRG () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_RM_CATA_RG ; }
         virtual INT32 init ( const CHAR *pInfo ) ;

      private:
         INT64     _taskID ;
         string    _tmpCoordSvcName ;
         BSONObj   _info ;
   } ;

   /*
      remove coord group 
   */
   class _omaRmCoordRG : public _omaCommand
   {
      public:
         _omaRmCoordRG ( INT64 taskID, string &tmpCoordSvcName, BSONObj &info ) ;
         virtual ~_omaRmCoordRG () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_RM_COORD_RG ; }
         virtual INT32 init ( const CHAR *pInfo ) ;

      private:
         INT64     _taskID ;
         string    _tmpCoordSvcName ;
         BSONObj   _info ;
   } ;

   /*
      remove data group 
   */
   class _omaRmDataRG : public _omaCommand
   {
      public:
         _omaRmDataRG ( INT64 taskID,
                        string &tmpCoordSvcName,
                        BSONObj &info ) ;
         virtual ~_omaRmDataRG () ;

      public:
         virtual const CHAR* name () { return OMA_CMD_RM_DATA_RG ; }
         virtual INT32 init ( const CHAR *pInfo ) ;

      private:
         INT64     _taskID ;
         string    _tmpCoordSvcName ;
         BSONObj   _info ;
   } ;

   /*
      init for execute js script
   */
   class _omaInitEnv : public _omaCommand
   {
      public:
         _omaInitEnv ( INT64 taskID, BSONObj &info ) ;
         virtual ~_omaInitEnv () ;

      public:
         virtual const CHAR* name () { return OMA_INIT_ENV ; }
         virtual INT32 init ( const CHAR *pInfo ) ;

      private:
         INT64     _taskID ;
         BSONObj   _info ;
   } ;


   /*
      _omaAddZNode
   */
   class _omaAddZNode : public _omaCommand
   {
      public:
         _omaAddZNode ( AddZNInfo &info ) ;
         ~_omaAddZNode () ;
         virtual const CHAR * name () { return OMA_CMD_INSTALL_ZOOKEEPER ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT32 _getAddZNInfo( BSONObj &retObj1, BSONObj &retObj2 ) ;
         
      private:
         AddZNInfo   _addZNInfo ;
   } ;

   /*
      _omaRemoveZNode
   */
   class _omaRemoveZNode : public _omaCommand
   {
      public:
         _omaRemoveZNode ( RemoveZNInfo &info ) ;
         ~_omaRemoveZNode () ;
         virtual const CHAR * name () { return OMA_CMD_REMOVE_ZOOKEEPER ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      private:
         INT32 _getRemoveZNInfo( BSONObj &retObj1, BSONObj &retObj2 ) ;
         
      private:
         RemoveZNInfo   _removeZNInfo ;
   } ;

   /*
      _omaCheckZNode
   */
   class _omaCheckZNodes : public _omaCommand
   {
      public:
         _omaCheckZNodes ( vector<CheckZNInfo> &info ) ;
         ~_omaCheckZNodes () ;
         virtual const CHAR * name () { return OMA_CMD_CHECK_ZOOKEEPER ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;

      protected:
         INT32 _getCheckZNInfos( BSONObj &retObj1, BSONObj &retObj2 ) ;
         
      protected:
         vector<CheckZNInfo>   _checkZNInfos ;
   } ;

   /*
      _omaCheckZNode
   */
   class _omaCheckZNEnv : public _omaCheckZNodes
   {
      public:
         _omaCheckZNEnv ( vector<CheckZNInfo> &info ) ;
         ~_omaCheckZNEnv () ;
         virtual const CHAR * name () { return OMA_CMD_CHECK_ZOOKEEPER_ENV ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   class _omaCheckSsqlOlap : public _omaCommand
   {
      public:
         _omaCheckSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaCheckSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_CHECK_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   class _omaInstallSsqlOlap : public _omaCommand
   {
      public:
         _omaInstallSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaInstallSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_INSTALL_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   class _omaTrustSsqlOlap : public _omaCommand
   {
      public:
         _omaTrustSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaTrustSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_TRUST_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   class _omaCheckHdfsSsqlOlap : public _omaCommand
   {
      public:
         _omaCheckHdfsSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaCheckHdfsSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_CHECK_HDFS_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   class _omaInitClusterSsqlOlap : public _omaCommand
   {
      public:
         _omaInitClusterSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaInitClusterSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_INIT_CLUSTER_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   class _omaRemoveSsqlOlap : public _omaCommand
   {
      public:
         _omaRemoveSsqlOlap( const BSONObj& config, const BSONObj& sysInfo ) ;
         ~_omaRemoveSsqlOlap() ;
         virtual const CHAR * name() { return OMA_CMD_REMOVE_SEQUOIASQL_OLAP ; }
         virtual INT32 init( const CHAR *pInstallInfo ) ;

      private:
         BSONObj _config ;
         BSONObj _sysInfo ;
   } ;

   /*
      _omaRunPsqlCmd
   */
   class _omaRunPsqlCmd : public _omaCommand
   {
      public:
         _omaRunPsqlCmd( SsqlExecInfo &ssqlInfo ) ;
         ~_omaRunPsqlCmd() ;
         virtual const CHAR * name() { return OMA_CMD_SSQL_EXEC ; }
         virtual INT32 init( const CHAR *nullInfo ) ;

      private:
         SsqlExecInfo      _ssqlInfo ;
   } ;

   /*
      _omaCleanSsqlExecCmd
   */
   class _omaCleanSsqlExecCmd : public _omaCommand
   {
      public:
         _omaCleanSsqlExecCmd( SsqlExecInfo &ssqlInfo ) ;
         ~_omaCleanSsqlExecCmd() ;
         virtual const CHAR * name() { return OMA_CMD_CLEAN_SSQL_EXEC ; }
         virtual INT32 init( const CHAR *nullInfo ) ;

      private:
         SsqlExecInfo      _ssqlInfo ;
   } ;

   /*
      _omaGetPsqlCmd
   */
   class _omaGetPsqlCmd : public _omaCommand
   {
      public:
         _omaGetPsqlCmd( SsqlExecInfo &ssqlInfo ) ;
         ~_omaGetPsqlCmd() ;
         virtual const CHAR * name() { return OMA_CMD_GET_PSQL ; }
         virtual INT32 init( const CHAR *nullInfo ) ;

      private:
         SsqlExecInfo      _ssqlInfo ;
   } ;

   /*
      add business
   */
   class _omaAddBusiness : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaAddBusiness() ;
      virtual ~_omaAddBusiness() ;

   public:
      virtual const CHAR* name() { return OMA_CMD_ADD_BUSINESS ; }
      virtual INT32 init( const CHAR *pInstallInfo ) ;
      virtual INT32 convertResult( const BSONObj& itemInfo,
                                   BSONObj& taskInfo ) ;

   private:
      void _aggrFlowArray( const BSONObj& array1, const BSONObj& array2,
                           BSONArray& out ) ;

   private:
      INT64 _taskID ;
   } ;

   /*
      remove business
   */
   class _omaRemoveBusiness : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaRemoveBusiness() ;
      virtual ~_omaRemoveBusiness() ;

   public:
      virtual const CHAR* name() { return OMA_CMD_REMOVE_BUSINESS ; }
      virtual INT32 init( const CHAR *pInstallInfo ) ;
      virtual INT32 convertResult( const BSONObj& itemInfo,
                                   BSONObj& taskInfo ) ;

   private:
      void _aggrFlowArray( const BSONObj& array1, const BSONObj& array2,
                           BSONArray& out ) ;

   private:
      INT64 _taskID ;
   } ;

   /*
      extend sequoiadb
   */
   class _omaExtendDB : public _omaCommand
   {

   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaExtendDB() ;
      virtual ~_omaExtendDB() ;

   public:
      virtual const CHAR* name() { return OMA_CMD_EXTEND_SEQUOIADB ; }
      virtual INT32 init( const CHAR *pInstallInfo ) ;
      virtual INT32 convertResult( const BSONObj& itemInfo,
                                   BSONObj& taskInfo ) ;

   private:
      void _aggrFlowArray( const BSONObj& array1, const BSONObj& array2,
                           BSONArray& out ) ;

   private:
      INT64 _taskID ;
   } ;

   /*
      shrink business
   */
   class _omaShrinkBusiness : public _omaCommand
   {

   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaShrinkBusiness() ;
      virtual ~_omaShrinkBusiness() ;

   public:
      virtual const CHAR* name() { return OMA_CMD_SHRINK_BUSINESS ; }
      virtual INT32 init( const CHAR *pInstallInfo ) ;
      virtual INT32 convertResult( const BSONObj& itemInfo,
                                   BSONObj& taskInfo ) ;

   private:
      void _aggrFlowArray( const BSONObj& array1, const BSONObj& array2,
                           BSONArray& out ) ;

   private:
      INT64 _taskID ;
   } ;

   /*
      deploy package
   */
   class _omaDeployPackage : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaDeployPackage() ;
      virtual ~_omaDeployPackage() ;

      virtual const CHAR* name() { return OMA_CMD_DEOLOY_PACKAGE ; }

      virtual INT32 init( const CHAR *pInstallInfo ) ;

      virtual INT32 convertResult( const BSONObj& itemInfo,
                                   BSONObj& taskInfo ) ;

   private:
      void _aggrFlowArray( const BSONObj& array1, const BSONObj& array2,
                           BSONArray& out ) ;

   private:
      INT64 _taskID ;
   } ;

   /*
      restart Business
   */
   class _omaRestartBusiness : public _omaAsyncCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaRestartBusiness() ;
      ~_omaRestartBusiness() ;

      const CHAR* name () { return OMA_CMD_RESTART_BUSINESS ; }

      INT32 init ( const CHAR *pInterruptInfo ) ;

      INT32 setRuningStatus( const BSONObj& itemInfo,
                             BSONObj& taskInfo ) ;

      INT32 convertResult( const BSONObj& itemInfo,
                           BSONObj& taskInfo ) ;

   private:
      void _matchNode( const string &businessType,
                       const BSONObj &itemInfo,
                       const BSONObj &taskResultInfo,
                       BOOLEAN &isNode,
                       INT32 &updateProgress,
                       BSONObj &updateFlow,
                       BSONObj &updateInfo ) ;
   } ;

   /*
      _omaStartPlugins
   */
   class _omaStartPlugins : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaStartPlugins() ;
      ~_omaStartPlugins() ;
      virtual const CHAR* name () { return OMA_CMD_START_PLUGIN ; }
      virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /*
      _omaStopPlugins
   */
   class _omaStopPlugins : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER() ;

   public:
      _omaStopPlugins() ;
      ~_omaStopPlugins() ;
      virtual const CHAR* name () { return OMA_CMD_STOP_PLUGIN ; }
      virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

} // namespace engine


#endif // OMAGENT_BACKGROUD_CMD_HPP_
