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

   Source File Name = omagentBackgroundCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentUtil.hpp"
#include "omagentBackgroundCmd.hpp"
#include "utilStr.hpp"
#include "omagentMgr.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omaAddHost
   */
   _omaAddHost::_omaAddHost ( AddHostInfo &info )
   {
      _addHostInfo = info ;
   }

   _omaAddHost::~_omaAddHost ()
   {
   }

   INT32 _omaAddHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getAddHostInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get add host info for js file, "
                     "rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Add host passes argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ADD_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_ADD_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaAddHost::_getAddHostInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObjBuilder bob ;
      BSONObj subObj ;


      try
      {
         bob.append( OMA_FIELD_IP, _addHostInfo._item._ip.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _addHostInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _addHostInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _addHostInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _addHostInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_AGENTSERVICE, _addHostInfo._item._agentService.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH, _addHostInfo._item._installPath.c_str() ) ;
         bob.append( OMA_FIELD_VERSION, _addHostInfo._item._version.c_str() ) ;
         subObj = bob.obj() ;

         builder.append( OMA_FIELD_SDBUSER,
                         _addHostInfo._common._sdbUser.c_str() ) ;
         builder.append( OMA_FIELD_SDBPASSWD,
                         _addHostInfo._common._sdbPasswd.c_str() ) ;
         builder.append( OMA_FIELD_SDBUSERGROUP,
                         _addHostInfo._common._userGroup.c_str() ) ;
         builder.append( OMA_FIELD_INSTALLPACKET,
                         _addHostInfo._common._installPacket.c_str() ) ;
         builder.append( OMA_FIELD_HOSTINFO, subObj ) ;
         retObj1 = builder.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _addHostInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckAddHostInfo
   */
   _omaCheckAddHostInfo::_omaCheckAddHostInfo()
   {
   }

   _omaCheckAddHostInfo::~_omaCheckAddHostInfo()
   {
   }

   INT32 _omaCheckAddHostInfo::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus( pInstallInfo ) ;
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Check add host information passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FIEL_ADD_HOST_CHECK_INFO, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FIEL_ADD_HOST_CHECK_INFO, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaRemoveHost
   */
   _omaRemoveHost::_omaRemoveHost ( RemoveHostInfo &info )
   {
      _removeHostInfo = info ;
   }

   _omaRemoveHost::~_omaRemoveHost ()
   {
   }

   INT32 _omaRemoveHost::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getRemoveHostInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get remove host info for js file, "
                     "rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Remove host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_REMOVE_HOST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_REMOVE_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaRemoveHost::_getRemoveHostInfo( BSONObj &retObj1,
                                             BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;

      try
      {
         bob.append( OMA_FIELD_IP, _removeHostInfo._item._ip.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _removeHostInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _removeHostInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _removeHostInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _removeHostInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_CLUSTERNAME, _removeHostInfo._item._clusterName.c_str() ) ;
         bob.appendArray( OMA_FIELD_PACKAGES, _removeHostInfo._item._packages ) ;

         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _removeHostInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCreateTmpCoord
   */
   _omaCreateTmpCoord::_omaCreateTmpCoord( INT64 taskID )
   {
      _taskID = taskID ;
   }

   _omaCreateTmpCoord::~_omaCreateTmpCoord()
   {
   }

   INT32 _omaCreateTmpCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSONObj(pInstallInfo).copy() ;
         BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install temporary coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_TMP_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_TMP_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   INT32 _omaCreateTmpCoord::createTmpCoord( BSONObj &cfgObj, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      rc = init( cfgObj.objdata() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init to create "
                  "temporary coord, rc = %d", rc ) ;
         goto error ;
      }
      rc = doit( retObj ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create temporary coord, rc = %d", rc ) ;
         goto error ;
      }     
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaRemoveTmpCoord
   */
   _omaRemoveTmpCoord::_omaRemoveTmpCoord( INT64 taskID,
                                           string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRemoveTmpCoord::~_omaRemoveTmpCoord ()
   {
   }

   INT32 _omaRemoveTmpCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON( OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName ) ;
         BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Remove temporary coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_REMOVE_TMP_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_REMOVE_TMP_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   INT32 _omaRemoveTmpCoord::removeTmpCoord( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      rc = init( NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to init to remove temporary coord, "
                  "rc = %d", rc ) ;
         goto error ;
      }
      rc = doit( retObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to remove temporary coord, rc = %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaInstallStandalone
   */
   _omaInstallStandalone::_omaInstallStandalone( INT64 taskID,
                                                 InstDBInfo &info )
   {
      _taskID              = taskID ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallStandalone::~_omaInstallStandalone()
   {
   }

   INT32 _omaInstallStandalone::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON ( OMA_FIELD_TASKID << _taskID ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install standalone passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_STANDALONE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_INSTALL_STANDALONE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallCatalog
   */
   _omaInstallCatalog::_omaInstallCatalog( INT64 taskID,
                                           string &tmpCoordSvcName,
                                           InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallCatalog::~_omaInstallCatalog()
   {
   }

   INT32 _omaInstallCatalog::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install catalog passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_CATALOG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_CATALOG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallCoord
   */
   _omaInstallCoord::_omaInstallCoord( INT64 taskID,
                                       string &tmpCoordSvcName,
                                       InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallCoord::~_omaInstallCoord()
   {
   }

   INT32 _omaInstallCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER         << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD       << _info._sdbPasswd.c_str() <<
                 OMA_FIELD_SDBUSERGROUP    << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER            << _info._user.c_str() <<
                 OMA_FIELD_PASSWD          << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT         << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME  << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2    << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG   << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install coord passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaInstallDataNode
   */
   _omaInstallDataNode::_omaInstallDataNode( INT64 taskID,
                                             string tmpCoordSvcName,
                                             InstDBInfo &info )
   {
      _taskID              = taskID ;
      _tmpCoordSvcName     = tmpCoordSvcName ;
      _info._hostName      = info._hostName;
      _info._svcName       = info._svcName ;
      _info._dbPath        = info._dbPath ;
      _info._confPath      = info._confPath ;
      _info._dataGroupName = info._dataGroupName ;
      _info._sdbUser       = info._sdbUser ;
      _info._sdbPasswd     = info._sdbPasswd ;
      _info._sdbUserGroup  = info._sdbUserGroup ;
      _info._user          = info._user ;
      _info._passwd        = info._passwd ;
      _info._sshPort       = info._sshPort ;
      _info._conf          = info._conf.copy() ;
   }

   _omaInstallDataNode::~_omaInstallDataNode()
   {
   }

   INT32 _omaInstallDataNode::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus = BSON (
                 OMA_FIELD_SDBUSER          << _info._sdbUser.c_str() <<
                 OMA_FIELD_SDBPASSWD        << _info._sdbPasswd.c_str() << 
                 OMA_FIELD_SDBUSERGROUP     << _info._sdbUserGroup.c_str() <<
                 OMA_FIELD_USER             << _info._user.c_str() <<
                 OMA_FIELD_PASSWD           << _info._passwd.c_str() <<
                 OMA_FIELD_SSHPORT          << _info._sshPort.c_str() <<
                 OMA_FIELD_INSTALLGROUPNAME << _info._dataGroupName.c_str() <<
                 OMA_FIELD_INSTALLHOSTNAME  << _info._hostName.c_str() <<
                 OMA_FIELD_INSTALLSVCNAME   << _info._svcName.c_str() <<
                 OMA_FIELD_INSTALLPATH2     << _info._dbPath.c_str() <<
                 OMA_FIELD_INSTALLCONFIG    << _info._conf ) ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Install data node passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_DATANODE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_INSTALL_DATANODE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      rollback standalone
   */
   _omaRollbackStandalone::_omaRollbackStandalone ( BSONObj &bus, BSONObj &sys )
   {
      _bus    = bus.copy() ;
      _sys    = sys.copy() ;
   }

   _omaRollbackStandalone::~_omaRollbackStandalone ()
   {
   }
   
   INT32 _omaRollbackStandalone::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      
      ss << "var " << JS_ARG_BUS << " = " 
         << _bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << _sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Rollback standalone passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_ROLLBACK_STANDALONE, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                  FILE_ROLLBACK_STANDALONE, rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      rollback catalog
   */
   _omaRollbackCatalog::_omaRollbackCatalog (
                                   INT64 taskID,
                                   string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackCatalog::~_omaRollbackCatalog ()
   {
   }
   
   INT32 _omaRollbackCatalog::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;

         ss << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback catalog passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ROLLBACK_CATALOG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_CATALOG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }
   
   /*
      rollback coord
   */

   _omaRollbackCoord::_omaRollbackCoord ( INT64 taskID,
                                          string &tmpCoordSvcName )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackCoord::~_omaRollbackCoord ()
   {
   }
   
   INT32 _omaRollbackCoord::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj sys = BSON (
                 OMA_FIELD_TASKID << _taskID <<
                 OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         ss << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback coord passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ROLLBACK_COORD, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_COORD, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }


   /*
      rollback data groups
   */

   _omaRollbackDataRG::_omaRollbackDataRG ( INT64 taskID,
                                            string &tmpCoordSvcName,
                                            set<string> &info )
   : _info( info )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
   }

   _omaRollbackDataRG::~_omaRollbackDataRG ()
   {
   }
   
   INT32 _omaRollbackDataRG::init ( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus ;
         BSONObj sys ;
         _getInstalledDataGroupInfo( bus ) ;
         sys = BSON( OMA_FIELD_TASKID << _taskID <<
                     OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Rollback data groups passes "
                  "argument: %s", _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_ROLLBACK_DATA_RG, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                         FILE_ROLLBACK_DATA_RG, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson, exception is: %s",
                      e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   void _omaRollbackDataRG::_getInstalledDataGroupInfo( BSONObj &obj )
   {
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      set<string>::iterator it = _info.begin() ;

      for( ; it != _info.end(); it++ )
      {
         string groupname = *it ;
         bab.append( groupname.c_str() ) ;
      }
      bob.appendArray( OMA_FIELD_UNINSTALLGROUPNAMES, bab.arr() ) ;
      obj = bob.obj() ;
   }

   /*
      remove standalone
   */
   _omaRmStandalone::_omaRmStandalone( BSONObj &bus, BSONObj &sys )
   {
      _bus = bus.copy() ;
      _sys = sys.copy() ;
   }

   _omaRmStandalone::~_omaRmStandalone()
   {
   }

   INT32 _omaRmStandalone::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      
         ss << "var " << JS_ARG_BUS << " = " 
            << _bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove standalone passes argument: %s",
               _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_REMOVE_STANDALONE, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_STANDALONE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove catalog group
   */
   _omaRmCataRG::_omaRmCataRG ( INT64 taskID, string &tmpCoordSvcName,
                                BSONObj &info )
   {
      _taskID = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info = info.copy() ;
   }

   _omaRmCataRG::~_omaRmCataRG ()
   {
   }
   
   INT32 _omaRmCataRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;

      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove catalog group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_REMOVE_CATALOG_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                  FILE_REMOVE_CATALOG_RG, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove coord group
   */

   _omaRmCoordRG::_omaRmCoordRG ( INT64 taskID, string &tmpCoordSvcName,
                                  BSONObj &info )
   {
      _taskID          = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info            = info.copy() ;
   }

   _omaRmCoordRG::~_omaRmCoordRG ()
   {
   }
   
   INT32 _omaRmCoordRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;

      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove coord group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_REMOVE_COORD_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_COORD_RG, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      remove data rg
   */
   _omaRmDataRG::_omaRmDataRG ( INT64 taskID, string &tmpCoordSvcName,
                                BSONObj &info )
   {
      _taskID = taskID ;
      _tmpCoordSvcName = tmpCoordSvcName ;
      _info = info.copy() ;
   }

   _omaRmDataRG::~_omaRmDataRG ()
   {
   }
   
   INT32 _omaRmDataRG::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID <<
                          OMA_FIELD_TMPCOORDSVCNAME << _tmpCoordSvcName.c_str() ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Remove data group passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_REMOVE_DATA_RG, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_REMOVE_DATA_RG, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      init for executing js
   */
   _omaInitEnv::_omaInitEnv ( INT64 taskID, BSONObj &info )
   {
      _taskID = taskID ;
      _info = info.copy() ;
   }

   _omaInitEnv::~_omaInitEnv ()
   {
   }
   
   INT32 _omaInitEnv::init ( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus = _info.copy() ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _taskID ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;
      PD_LOG ( PDDEBUG, "Init for executing js passes "
               "argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_INIT_ENV, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_INIT_ENV, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaAddZNode
   */
   _omaAddZNode::_omaAddZNode ( AddZNInfo &info )
   {
      _addZNInfo = info ;
   }

   _omaAddZNode::~_omaAddZNode ()
   {
   }

   INT32 _omaAddZNode::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getAddZNInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get installing znode's info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Installing znode passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_INSTALL_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_INSTALL_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaAddZNode::_getAddZNInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      vector<string>::iterator it = _addZNInfo._common._serverInfo.begin() ;




      try
      {
         bob.append( OMA_FIELD_DEPLOYMOD, _addZNInfo._common._deployMod.c_str() ) ;
         bob.append( OMA_FIELD_PACKET_PATH, _addZNInfo._common._installPacket.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _addZNInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _addZNInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _addZNInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSER, _addZNInfo._common._sdbUser.c_str() ) ;
         bob.append( OMA_FIELD_SDBPASSWD, _addZNInfo._common._sdbPasswd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSERGROUP, _addZNInfo._common._userGroup.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _addZNInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_ZOOID3, _addZNInfo._item._zooid.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH3, _addZNInfo._item._installPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPATH3, _addZNInfo._item._dataPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPORT3, _addZNInfo._item._dataPort.c_str() ) ;
         bob.append( OMA_FIELD_ELECTPORT3, _addZNInfo._item._electPort.c_str() ) ;
         bob.append( OMA_FIELD_CLIENTPORT3, _addZNInfo._item._clientPort.c_str() ) ;
         bob.append( OMA_FIELD_SYNCLIMIT3, _addZNInfo._item._syncLimit.c_str() ) ;
         bob.append( OMA_FIELD_INITLIMIT3, _addZNInfo._item._initLimit.c_str() ) ;
         bob.append( OMA_FIELD_TICKTIME3, _addZNInfo._item._tickTime.c_str() ) ;
         bob.append( OMA_FIELD_CLUSTERNAME3, _addZNInfo._common._clusterName.c_str() ) ;
         bob.append( OMA_FIELD_BUSINESSNAME3, _addZNInfo._common._businessName.c_str() ) ;
         for ( ; it != _addZNInfo._common._serverInfo.end(); it++ )
         {
            bab.append( *it ) ;
         }
         bob.appendArray( OMA_FIELD_SERVERINFO, bab.arr() ) ;

         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _addZNInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaRemoveZNode
   */
   _omaRemoveZNode::_omaRemoveZNode ( RemoveZNInfo &info )
   {
      _removeZNInfo = info ;
   }

   _omaRemoveZNode::~_omaRemoveZNode ()
   {
   }

   INT32 _omaRemoveZNode::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getRemoveZNInfo( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get removing znode's info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Removing znode passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_REMOVE_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_REMOVE_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaRemoveZNode::_getRemoveZNInfo( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      vector<string>::iterator it ;



      try
      {
         bob.append( OMA_FIELD_DEPLOYMOD, _removeZNInfo._common._deployMod.c_str() ) ;
         bob.append( OMA_FIELD_HOSTNAME, _removeZNInfo._item._hostName.c_str() ) ;
         bob.append( OMA_FIELD_USER, _removeZNInfo._item._user.c_str() ) ;
         bob.append( OMA_FIELD_PASSWD, _removeZNInfo._item._passwd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSER, _removeZNInfo._common._sdbUser.c_str() ) ;
         bob.append( OMA_FIELD_SDBPASSWD, _removeZNInfo._common._sdbPasswd.c_str() ) ;
         bob.append( OMA_FIELD_SDBUSERGROUP, _removeZNInfo._common._userGroup.c_str() ) ;
         bob.append( OMA_FIELD_SSHPORT, _removeZNInfo._item._sshPort.c_str() ) ;
         bob.append( OMA_FIELD_ZOOID3, _removeZNInfo._item._zooid.c_str() ) ;
         bob.append( OMA_FIELD_INSTALLPATH3, _removeZNInfo._item._installPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPATH3, _removeZNInfo._item._dataPath.c_str() ) ;
         bob.append( OMA_FIELD_DATAPORT3, _removeZNInfo._item._dataPort.c_str() ) ;
         bob.append( OMA_FIELD_ELECTPORT3, _removeZNInfo._item._electPort.c_str() ) ;
         bob.append( OMA_FIELD_CLIENTPORT3, _removeZNInfo._item._clientPort.c_str() ) ;
         bob.append( OMA_FIELD_SYNCLIMIT3, _removeZNInfo._item._syncLimit.c_str() ) ;
         bob.append( OMA_FIELD_INITLIMIT3, _removeZNInfo._item._initLimit.c_str() ) ;
         bob.append( OMA_FIELD_TICKTIME3, _removeZNInfo._item._tickTime.c_str() ) ;

         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << _removeZNInfo._taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckZNodes
   */
   _omaCheckZNodes::_omaCheckZNodes ( vector<CheckZNInfo> &info )
   {
      _checkZNInfos = info ;
   }

   _omaCheckZNodes::~_omaCheckZNodes ()
   {
   }

   INT32 _omaCheckZNodes::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getCheckZNInfos( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get checking znodes' info "
                     "for js file, rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking znodes pass argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_CHECK_ZOOKEEPER, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_ZOOKEEPER, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _omaCheckZNodes::_getCheckZNInfos( BSONObj &retObj1, BSONObj &retObj2 )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONArrayBuilder bab ;
      const CHAR *pStr = NULL ;
      INT64 taskID     = 0 ;
      vector<CheckZNInfo>::iterator it = _checkZNInfos.begin() ;


      if ( it == _checkZNInfos.end() )
      {
         rc = SDB_SYS;
         PD_LOG_MSG ( PDERROR, "No znodes' info to check" ) ;
         goto error ;
      }

      pStr = it->_common._deployMod.c_str() ;
      taskID = it->_taskID ;

      try
      {
         for( ; it != _checkZNInfos.end(); it++ )
         {
            BSONObjBuilder builder ;
            BSONObj obj ;
            
            builder.append( OMA_FIELD_HOSTNAME, it->_item._hostName.c_str() ) ;
            builder.append( OMA_FIELD_USER, it->_item._user.c_str() ) ;
            builder.append( OMA_FIELD_PASSWD, it->_item._passwd.c_str() ) ;
            builder.append( OMA_FIELD_SDBUSER, it->_common._sdbUser.c_str() ) ;
            builder.append( OMA_FIELD_SDBPASSWD, it->_common._sdbPasswd.c_str() ) ;
            builder.append( OMA_FIELD_SDBUSERGROUP, it->_common._userGroup.c_str() ) ;
            builder.append( OMA_FIELD_SSHPORT, it->_item._sshPort.c_str() ) ;
            builder.append( OMA_FIELD_ZOOID3, it->_item._zooid.c_str() ) ;
            builder.append( OMA_FIELD_INSTALLPATH3, it->_item._installPath.c_str() ) ;
            builder.append( OMA_FIELD_DATAPATH3, it->_item._dataPath.c_str() ) ;
            builder.append( OMA_FIELD_DATAPORT3, it->_item._dataPort.c_str() ) ;
            builder.append( OMA_FIELD_ELECTPORT3, it->_item._electPort.c_str() ) ;
            builder.append( OMA_FIELD_CLIENTPORT3, it->_item._clientPort.c_str() ) ;
            builder.append( OMA_FIELD_SYNCLIMIT3, it->_item._syncLimit.c_str() ) ;
            builder.append( OMA_FIELD_INITLIMIT3, it->_item._initLimit.c_str() ) ;
            builder.append( OMA_FIELD_TICKTIME3, it->_item._tickTime.c_str() ) ;
            builder.append( OMA_FIELD_CLUSTERNAME3, it->_common._clusterName.c_str() ) ;
            builder.append( OMA_FIELD_BUSINESSNAME3, it->_common._businessName.c_str() ) ;

            obj = builder.obj() ;
            bab.append( obj ) ;
         }

         bob.append( OMA_FIELD_DEPLOYMOD, pStr ) ;
         bob.append( OMA_FIELD_SERVERINFO, bab.arr() ) ;

         retObj1 = bob.obj() ;
         retObj2 = BSON( OMA_FIELD_TASKID << taskID ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR, "Failed to build bson for add host, "
                      "exception is: %s", e.what() ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omaCheckZNEnv
   */
   _omaCheckZNEnv::_omaCheckZNEnv ( vector<CheckZNInfo> &info )
   :_omaCheckZNodes( info )
   {
   }

   _omaCheckZNEnv::~_omaCheckZNEnv ()
   {
   }

   INT32 _omaCheckZNEnv::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObj bus ;
         BSONObj sys ;
         stringstream ss ;
         rc = _getCheckZNInfos( bus, sys ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get info for"
                     "js file, rc = %d", rc ) ;
            goto error ;
         }

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << sys.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking znodes' environment pass argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_CHECK_ZOOKEEPER_ENV, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_CHECK_ZOOKEEPER_ENV, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaCheckSsqlOlap::_omaCheckSsqlOlap( const BSONObj& config, const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaCheckSsqlOlap::~_omaCheckSsqlOlap()
   {
   }

   INT32 _omaCheckSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CHECK, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CHECK, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaInstallSsqlOlap::_omaInstallSsqlOlap( const BSONObj& config,
                                             const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaInstallSsqlOlap::~_omaInstallSsqlOlap()
   {
   }

   INT32 _omaInstallSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Installing sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CONFIG ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CONFIG, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_INSTALL, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_INSTALL, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaTrustSsqlOlap::_omaTrustSsqlOlap( const BSONObj& config,
                                       const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaTrustSsqlOlap::~_omaTrustSsqlOlap()
   {
   }

   INT32 _omaTrustSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Trusting sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_TRUST, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_TRUST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaCheckHdfsSsqlOlap::_omaCheckHdfsSsqlOlap( const BSONObj& config,
                                                 const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaCheckHdfsSsqlOlap::~_omaCheckHdfsSsqlOlap()
   {
   }

   INT32 _omaCheckHdfsSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Checking HDFS for sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_CHECK_HDFS, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_CHECK_HDFS, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaInitClusterSsqlOlap::_omaInitClusterSsqlOlap( const BSONObj& config,
                                                     const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaInitClusterSsqlOlap::~_omaInitClusterSsqlOlap()
   {
   }

   INT32 _omaInitClusterSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Init cluster for sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_INIT, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_INIT, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   _omaRemoveSsqlOlap::_omaRemoveSsqlOlap( const BSONObj& config, 
                                           const BSONObj& sysInfo )
   {
      _config = config ;
      _sysInfo = sysInfo ;
   }

   _omaRemoveSsqlOlap::~_omaRemoveSsqlOlap()
   {
   }

   INT32 _omaRemoveSsqlOlap::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;

         ss << "var " << JS_ARG_BUS << " = " 
            << _config.toString(FALSE, TRUE).c_str() << " ; "
            << "var " << JS_ARG_SYS << " = "
            << _sysInfo.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Removing sequoiasql olap passes argument: %s",
                  _jsFileArgs.c_str() ) ;

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_COMMON ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_COMMON, rc ) ;
            goto error ;
         }

         rc = addJsFile( FILE_SEQUOIASQL_OLAP_REMOVE, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SEQUOIASQL_OLAP_REMOVE, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error :
      goto done ;
   }

   /*
      _omaRunPsqlCmd
   */
   _omaRunPsqlCmd::_omaRunPsqlCmd( SsqlExecInfo &ssqlInfo )
                  :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaRunPsqlCmd::~_omaRunPsqlCmd()
   {
   }
   
   INT32 _omaRunPsqlCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_RUN_PSQL, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_RUN_PSQL, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaCleanSsqlExecCmd
   */
   _omaCleanSsqlExecCmd::_omaCleanSsqlExecCmd( SsqlExecInfo &ssqlInfo )
                        :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaCleanSsqlExecCmd::~_omaCleanSsqlExecCmd()
   {
   }
   
   INT32 _omaCleanSsqlExecCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_CLEAN_SSQL_EXEC, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_CLEAN_SSQL_EXEC, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      _omaGetPsqlCmd
   */
   _omaGetPsqlCmd::_omaGetPsqlCmd( SsqlExecInfo &ssqlInfo )
                  :_ssqlInfo( ssqlInfo )
   {
   }
   
   _omaGetPsqlCmd::~_omaGetPsqlCmd()
   {
   }
   
   INT32 _omaGetPsqlCmd::init( const CHAR *nullInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus ;
      BSONObj sys = BSON( OMA_FIELD_TASKID << _ssqlInfo._taskID ) ;

      bus = BSON( OMA_FIELD_HOSTNAME << _ssqlInfo._hostName << 
                  FIELD_NAME_SERVICE_NAME << _ssqlInfo._serviceName <<
                  OMA_FIELD_USER << _ssqlInfo._sshUser <<
                  OMA_FIELD_PASSWD << _ssqlInfo._sshPasswd <<
                  OMA_FIELD_INSTALLPATH << _ssqlInfo._installPath <<
                  OMA_FIELD_DBNAME << _ssqlInfo._dbName <<
                  OMA_FIELD_DBUSER << _ssqlInfo._dbUser <<
                  OMA_FIELD_DBPASSWD << _ssqlInfo._dbPasswd <<
                  OMA_FIELD_SQL << _ssqlInfo._sql << 
                  OMA_FIELD_RESULTFORMAT << _ssqlInfo._resultFormat ) ;
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString(FALSE, TRUE).c_str() << " ; "
         << "var " << JS_ARG_SYS << " = "
         << sys.toString(FALSE, TRUE).c_str() << " ; " ;
      _jsFileArgs = ss.str() ;

      PD_LOG ( PDDEBUG, "ssql execute argument: %s", _jsFileArgs.c_str() ) ;
      rc = addJsFile( FILE_GET_PSQL, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                      FILE_GET_PSQL, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   /*
      add business
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaAddBusiness )

   _omaAddBusiness::_omaAddBusiness()
   {
   }

   _omaAddBusiness::~_omaAddBusiness()
   {
   }

   INT32 _omaAddBusiness::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;
   
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString( FALSE, TRUE ).c_str() << " ; " ;
   
      _jsFileArgs = ss.str() ;
      PD_LOG( PDDEBUG, "Add Business argument: %s",
              _jsFileArgs.c_str() ) ;
   
      rc = addJsFile( FILE_ADD_BUSINESS, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to add js file[%s], rc = %d ",
                 FILE_ADD_BUSINESS, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;

   }

   INT32 _omaAddBusiness::convertResult( const BSONObj& itemInfo,
                                         BSONObj& taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 updateErrno = SDB_OK ;
      INT32 updateProgress = 0 ;
      INT32 errnoNum = taskInfo.getIntField( OMA_FIELD_ERRNO ) ;
      INT32 progress = taskInfo.getIntField( OMA_FIELD_PROGRESS ) ;
      string detail  = taskInfo.getStringField( OMA_FIELD_DETAIL ) ;
      string updateDetail ;
      string updateHostName ;
      BSONObj resultInfo = taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      BSONObj condition  = BSON( OMA_FIELD_ERRNO      << "" <<
                                 OMA_FIELD_DETAIL     << "" <<
                                 OMA_FIELD_PROGRESS   << "" <<
                                 OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj oneResultCondition = BSON( OMA_FIELD_HOSTNAME    << "" <<
                                         OMA_FIELD_PORT2       << "" <<
                                         OMA_FIELD_STATUS      << 0 <<
                                         OMA_FIELD_STATUSDESC  << "" <<
                                         OMA_FIELD_ERRNO       << 0 <<
                                         OMA_FIELD_DETAIL      << ""  ) ;
      BSONObj updateFlow = itemInfo.getObjectField( OMA_FIELD_FLOW ) ;
      BSONObj nodeResult = itemInfo.filterFieldsUndotted(
                                                    oneResultCondition, TRUE ) ;
      BSONObj taskInfo2 = taskInfo.filterFieldsUndotted( condition, FALSE ) ;
      BSONObjBuilder newTaskInfo ;
      BSONArrayBuilder newResultInfo ;

      rc = omaGetStringElement( itemInfo, OMA_FIELD_HOSTNAME, updateHostName ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_ERRNO, updateErrno ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateErrno = SDB_OK ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_PROGRESS, updateProgress ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateProgress = -1 ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_DETAIL, updateDetail ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateDetail = "" ;
      }

      {
         BSONObjIterator resultIter( resultInfo ) ;

         while( resultIter.more() )
         {
            BSONElement resultEle = resultIter.next() ;
            BSONObj oneResult = resultEle.embeddedObject() ;
            string hostName = oneResult.getStringField( OMA_FIELD_HOSTNAME ) ;

            if( updateHostName == hostName )
            {
               BSONObjBuilder newOneResultInfoBuilder ;
               BSONArray newFlowArray ;
               BSONObj flow = oneResult.getObjectField( OMA_FIELD_FLOW ) ;
               if( errnoNum == SDB_OK && updateErrno )
               {
                  errnoNum  = updateErrno ;
                  detail = updateDetail ;
               }
               if( updateProgress > 0 )
               {
                  progress += updateProgress ;
               }
               if( progress > 100 )
               {
                  progress = 100 ;
               }
               else if( progress < 0 )
               {
                  progress = 0 ;
               }
               _aggrFlowArray( flow, updateFlow, newFlowArray ) ;
               newOneResultInfoBuilder.appendElements( nodeResult ) ;
               newOneResultInfoBuilder.append( OMA_FIELD_FLOW, newFlowArray ) ;
               newResultInfo.append( newOneResultInfoBuilder.obj() ) ;
            }
            else
            {
               newResultInfo.append( oneResult ) ;
            }
         }
      }

      newTaskInfo.append( OMA_FIELD_ERRNO, errnoNum ) ;
      newTaskInfo.append( OMA_FIELD_DETAIL, detail ) ;
      newTaskInfo.append( OMA_FIELD_PROGRESS, progress ) ;
      newTaskInfo.append( OMA_FIELD_RESULTINFO, newResultInfo.arr() ) ;
      newTaskInfo.appendElements( taskInfo2 ) ;

      taskInfo = newTaskInfo.obj() ;

   done:
      return rc ;
   }

   void _omaAddBusiness::_aggrFlowArray( const BSONObj& array1,
                                         const BSONObj& array2,
                                         BSONArray& out )
   {
      BSONArrayBuilder arrayBuilder ;

      {
         BSONObjIterator iter( array1 ) ;

         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            arrayBuilder.append( ele.String() ) ;
         }
      }

      {
         BSONObjIterator iter( array2 ) ;

         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            arrayBuilder.append( ele.String() ) ;
         }
      }

      out = arrayBuilder.arr() ;
   }

   /*
      add business
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaRemoveBusiness )

   _omaRemoveBusiness::_omaRemoveBusiness()
   {
   }

   _omaRemoveBusiness::~_omaRemoveBusiness()
   {
   }

   INT32 _omaRemoveBusiness::init( const CHAR *pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;
   
      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString( FALSE, TRUE ).c_str() << " ; " ;
   
      _jsFileArgs = ss.str() ;
      PD_LOG( PDDEBUG, "Remove Business argument: %s",
              _jsFileArgs.c_str() ) ;
   
      rc = addJsFile( FILE_REMOVE_BUSINESS, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to add js file[%s], rc = %d ",
                 FILE_REMOVE_BUSINESS, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;

   }

   INT32 _omaRemoveBusiness::convertResult( const BSONObj& itemInfo,
                                            BSONObj& taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 updateErrno = SDB_OK ;
      INT32 updateProgress = 0 ;
      INT32 errnoNum = taskInfo.getIntField( OMA_FIELD_ERRNO ) ;
      INT32 progress = taskInfo.getIntField( OMA_FIELD_PROGRESS ) ;
      string detail  = taskInfo.getStringField( OMA_FIELD_DETAIL ) ;
      string updateDetail ;
      string updateHostName ;
      BSONObj resultInfo = taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      BSONObj condition  = BSON( OMA_FIELD_ERRNO      << "" <<
                                 OMA_FIELD_DETAIL     << "" <<
                                 OMA_FIELD_PROGRESS   << "" <<
                                 OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj oneResultCondition = BSON( OMA_FIELD_HOSTNAME    << "" <<
                                         OMA_FIELD_PORT2       << "" <<
                                         OMA_FIELD_STATUS      << 0 <<
                                         OMA_FIELD_STATUSDESC  << "" <<
                                         OMA_FIELD_ERRNO       << 0 <<
                                         OMA_FIELD_DETAIL      << ""  ) ;
      BSONObj updateFlow = itemInfo.getObjectField( OMA_FIELD_FLOW ) ;
      BSONObj nodeResult = itemInfo.filterFieldsUndotted(
                                                    oneResultCondition, TRUE ) ;
      BSONObj taskInfo2 = taskInfo.filterFieldsUndotted( condition, FALSE ) ;
      BSONObjBuilder newTaskInfo ;
      BSONArrayBuilder newResultInfo ;

      rc = omaGetStringElement( itemInfo, OMA_FIELD_HOSTNAME, updateHostName ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_ERRNO, updateErrno ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateErrno = SDB_OK ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_PROGRESS, updateProgress ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateProgress = -1 ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_DETAIL, updateDetail ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateDetail = "" ;
      }

      {
         BSONObjIterator resultIter( resultInfo ) ;

         while( resultIter.more() )
         {
            BSONElement resultEle = resultIter.next() ;
            BSONObj oneResult = resultEle.embeddedObject() ;
            string hostName = oneResult.getStringField( OMA_FIELD_HOSTNAME ) ;

            if( updateHostName == hostName )
            {
               BSONObjBuilder newOneResultInfoBuilder ;
               BSONArray newFlowArray ;
               BSONObj flow = oneResult.getObjectField( OMA_FIELD_FLOW ) ;
               if( errnoNum == SDB_OK && updateErrno )
               {
                  errnoNum  = updateErrno ;
                  detail = updateDetail ;
               }
               if( updateProgress > 0 )
               {
                  progress += updateProgress ;
               }
               if( progress > 100 )
               {
                  progress = 100 ;
               }
               else if( progress < 0 )
               {
                  progress = 0 ;
               }
               _aggrFlowArray( flow, updateFlow, newFlowArray ) ;
               newOneResultInfoBuilder.appendElements( nodeResult ) ;
               newOneResultInfoBuilder.append( OMA_FIELD_FLOW, newFlowArray ) ;
               newResultInfo.append( newOneResultInfoBuilder.obj() ) ;
            }
            else
            {
               newResultInfo.append( oneResult ) ;
            }
         }
      }

      newTaskInfo.append( OMA_FIELD_ERRNO, errnoNum ) ;
      newTaskInfo.append( OMA_FIELD_DETAIL, detail ) ;
      newTaskInfo.append( OMA_FIELD_PROGRESS, progress ) ;
      newTaskInfo.append( OMA_FIELD_RESULTINFO, newResultInfo.arr() ) ;
      newTaskInfo.appendElements( taskInfo2 ) ;

      taskInfo = newTaskInfo.obj() ;

   done:
      return rc ;
   }

   void _omaRemoveBusiness::_aggrFlowArray( const BSONObj& array1,
                                            const BSONObj& array2,
                                            BSONArray& out )
   {
      BSONArrayBuilder arrayBuilder ;

      {
         BSONObjIterator iter( array1 ) ;

         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            arrayBuilder.append( ele.String() ) ;
         }
      }

      {
         BSONObjIterator iter( array2 ) ;

         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            arrayBuilder.append( ele.String() ) ;
         }
      }

      out = arrayBuilder.arr() ;
   }

   /*
     _omaExtendDB implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaExtendDB )

   _omaExtendDB::_omaExtendDB()
   {
   }

   _omaExtendDB::~_omaExtendDB()
   {
   }

   INT32 _omaExtendDB::init( const CHAR* pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;

      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString( FALSE, TRUE ).c_str() << " ; " ;

      _jsFileArgs = ss.str() ;
      PD_LOG( PDDEBUG, "Extend SequoiaDB argument: %s",
              _jsFileArgs.c_str() ) ;

      rc = addJsFile( FILE_EXTEND_SEQUOIADB, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to add js file[%s], rc = %d ",
                 FILE_EXTEND_SEQUOIADB, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   void _omaExtendDB::_aggrFlowArray( const BSONObj& array1,
                                      const BSONObj& array2,
                                      BSONArray& out )
   {
      BSONArrayBuilder arrayBuilder ;

      {
         BSONObjIterator iter( array1 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }

      {
         BSONObjIterator iter( array2 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }
      out = arrayBuilder.arr() ;
   }

   INT32 _omaExtendDB::convertResult( const BSONObj& itemInfo,
                                      BSONObj& taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 updateErrno = SDB_OK ;
      INT32 updateProgress = 0 ;
      INT32 errnoNum = taskInfo.getIntField( OMA_FIELD_ERRNO ) ;
      INT32 progress = taskInfo.getIntField( OMA_FIELD_PROGRESS ) ;
      string detail  = taskInfo.getStringField( OMA_FIELD_DETAIL ) ;
      string updateDetail ;
      string updateHostName ;
      string updateSvcname ;
      BSONObj resultInfo = taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      BSONObj condition  = BSON( OMA_FIELD_ERRNO << "" <<
                                 OMA_FIELD_DETAIL << "" <<
                                 OMA_FIELD_PROGRESS << "" <<
                                 OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj oneResultCondition = BSON( OMA_FIELD_HOSTNAME << "" <<
                                         OMA_FIELD_DATAGROUPNAME << "" <<
                                         OMA_FIELD_SVCNAME << "" <<
                                         OMA_FIELD_ROLE << "" <<
                                         OMA_FIELD_STATUS << 0 <<
                                         OMA_FIELD_STATUSDESC << "" <<
                                         OMA_FIELD_ERRNO << 0 <<
                                         OMA_FIELD_DETAIL << ""  ) ;
      BSONObj updateFlow = itemInfo.getObjectField( OMA_FIELD_FLOW ) ;
      BSONObj nodeResult = itemInfo.filterFieldsUndotted(
                                                    oneResultCondition, TRUE ) ;
      BSONObj taskInfo2 = taskInfo.filterFieldsUndotted( condition, FALSE ) ;
      BSONObjBuilder newTaskInfo ;
      BSONArrayBuilder newResultInfo ;

      rc = omaGetStringElement( itemInfo, OMA_FIELD_HOSTNAME, updateHostName ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_SVCNAME, updateSvcname ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_ERRNO, updateErrno ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateErrno = SDB_OK ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_PROGRESS, updateProgress ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateProgress = -1 ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_DETAIL, updateDetail ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateDetail = "" ;
      }

      {
         BSONObjIterator resultIter( resultInfo ) ;

         while( resultIter.more() )
         {
            BSONElement resultEle = resultIter.next() ;
            BSONObj oneResult = resultEle.embeddedObject() ;
            string hostName = oneResult.getStringField( OMA_FIELD_HOSTNAME ) ;
            string svcname = oneResult.getStringField( OMA_FIELD_SVCNAME ) ;

            if( updateHostName == hostName && updateSvcname == svcname )
            {
               BSONObjBuilder newOneResultInfoBuilder ;
               BSONArray newFlowArray ;
               BSONObj flow = oneResult.getObjectField( OMA_FIELD_FLOW ) ;
               if( errnoNum == SDB_OK && updateErrno )
               {
                  errnoNum  = updateErrno ;
                  detail = updateDetail ;
               }
               if( updateProgress > 0 )
               {
                  progress += updateProgress ;
               }
               if( progress > 100 )
               {
                  progress = 100 ;
               }
               else if( progress < 0 )
               {
                  progress = 0 ;
               }
               _aggrFlowArray( flow, updateFlow, newFlowArray ) ;
               newOneResultInfoBuilder.appendElements( nodeResult ) ;
               newOneResultInfoBuilder.append( OMA_FIELD_FLOW, newFlowArray ) ;
               newResultInfo.append( newOneResultInfoBuilder.obj() ) ;
            }
            else
            {
               newResultInfo.append( oneResult ) ;
            }
         }
      }

      newTaskInfo.append( OMA_FIELD_ERRNO, errnoNum ) ;
      newTaskInfo.append( OMA_FIELD_DETAIL, detail ) ;
      newTaskInfo.append( OMA_FIELD_PROGRESS, progress ) ;
      newTaskInfo.append( OMA_FIELD_RESULTINFO, newResultInfo.arr() ) ;
      newTaskInfo.appendElements( taskInfo2 ) ;

      taskInfo = newTaskInfo.obj() ;

   done:
      return rc ;
   }

   /*
     _omaShrinkBusiness implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaShrinkBusiness )

   _omaShrinkBusiness::_omaShrinkBusiness()
   {
   }

   _omaShrinkBusiness::~_omaShrinkBusiness()
   {
   }

   INT32 _omaShrinkBusiness::init( const CHAR* pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;

      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString( FALSE, TRUE ).c_str() << " ; " ;

      _jsFileArgs = ss.str() ;
      PD_LOG( PDDEBUG, "Extend SequoiaDB argument: %s",
              _jsFileArgs.c_str() ) ;

      rc = addJsFile( FILE_SHRINK_BUSINESS, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to add js file[%s], rc = %d ",
                 FILE_SHRINK_BUSINESS, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   void _omaShrinkBusiness::_aggrFlowArray( const BSONObj& array1,
                                            const BSONObj& array2,
                                            BSONArray& out )
   {
      BSONArrayBuilder arrayBuilder ;

      {
         BSONObjIterator iter( array1 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }

      {
         BSONObjIterator iter( array2 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }
      out = arrayBuilder.arr() ;
   }

   INT32 _omaShrinkBusiness::convertResult( const BSONObj& itemInfo,
                                            BSONObj& taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 updateErrno = SDB_OK ;
      INT32 updateProgress = 0 ;
      INT32 progress = taskInfo.getIntField( OMA_FIELD_PROGRESS ) ;
      string updateDetail ;
      string updateHostName ;
      string updateSvcname ;
      BSONObj resultInfo = taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      BSONObj condition  = BSON( OMA_FIELD_ERRNO << "" <<
                                 OMA_FIELD_DETAIL << "" <<
                                 OMA_FIELD_PROGRESS << "" <<
                                 OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj oneResultCondition = BSON( OMA_FIELD_HOSTNAME << "" <<
                                         OMA_FIELD_DATAGROUPNAME << "" <<
                                         OMA_FIELD_SVCNAME << "" <<
                                         OMA_FIELD_ROLE << "" <<
                                         OMA_FIELD_STATUS << 0 <<
                                         OMA_FIELD_STATUSDESC << "" <<
                                         OMA_FIELD_ERRNO << 0 <<
                                         OMA_FIELD_DETAIL << ""  ) ;
      BSONObj updateFlow = itemInfo.getObjectField( OMA_FIELD_FLOW ) ;
      BSONObj nodeResult = itemInfo.filterFieldsUndotted(
                                                    oneResultCondition, TRUE ) ;
      BSONObj taskInfo2 = taskInfo.filterFieldsUndotted( condition, FALSE ) ;
      BSONObjBuilder newTaskInfo ;
      BSONArrayBuilder newResultInfo ;

      rc = omaGetStringElement( itemInfo, OMA_FIELD_HOSTNAME, updateHostName ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_SVCNAME, updateSvcname ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_ERRNO, updateErrno ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateErrno = SDB_OK ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_PROGRESS, updateProgress ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateProgress = -1 ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_DETAIL, updateDetail ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateDetail = "" ;
      }

      {
         BSONObjIterator resultIter( resultInfo ) ;

         while( resultIter.more() )
         {
            BSONElement resultEle = resultIter.next() ;
            BSONObj oneResult = resultEle.embeddedObject() ;
            string hostName = oneResult.getStringField( OMA_FIELD_HOSTNAME ) ;
            string svcname = oneResult.getStringField( OMA_FIELD_SVCNAME ) ;

            if( updateHostName == hostName && updateSvcname == svcname )
            {
               BSONObjBuilder newOneResultInfoBuilder ;
               BSONArray newFlowArray ;
               BSONObj flow = oneResult.getObjectField( OMA_FIELD_FLOW ) ;
               if( updateProgress > 0 )
               {
                  progress += updateProgress ;
               }
               if( progress > 100 )
               {
                  progress = 100 ;
               }
               else if( progress < 0 )
               {
                  progress = 0 ;
               }
               _aggrFlowArray( flow, updateFlow, newFlowArray ) ;
               newOneResultInfoBuilder.appendElements( nodeResult ) ;
               newOneResultInfoBuilder.append( OMA_FIELD_FLOW, newFlowArray ) ;
               newResultInfo.append( newOneResultInfoBuilder.obj() ) ;
            }
            else
            {
               newResultInfo.append( oneResult ) ;
            }
         }
      }

      newTaskInfo.append( OMA_FIELD_ERRNO, SDB_OK ) ;
      newTaskInfo.append( OMA_FIELD_DETAIL, "" ) ;
      newTaskInfo.append( OMA_FIELD_PROGRESS, progress ) ;
      newTaskInfo.append( OMA_FIELD_RESULTINFO, newResultInfo.arr() ) ;
      newTaskInfo.appendElements( taskInfo2 ) ;

      taskInfo = newTaskInfo.obj() ;

   done:
      return rc ;
   }

   /*
     _omaDeoloyPackage implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _omaDeployPackage )

   _omaDeployPackage::_omaDeployPackage()
   {
   }

   _omaDeployPackage::~_omaDeployPackage()
   {
   }

   INT32 _omaDeployPackage::init( const CHAR* pInstallInfo )
   {
      INT32 rc = SDB_OK ;
      stringstream ss ;
      BSONObj bus( pInstallInfo ) ;

      ss << "var " << JS_ARG_BUS << " = " 
         << bus.toString( FALSE, TRUE ).c_str() << " ; " ;

      _jsFileArgs = ss.str() ;
      PD_LOG( PDDEBUG, "Deploy package argument: %s",
              _jsFileArgs.c_str() ) ;

      rc = addJsFile( FILE_DEPLOY_PACKAGE, _jsFileArgs.c_str() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to add js file[%s], rc = %d ",
                 FILE_DEPLOY_PACKAGE, rc ) ;
         goto error ;
      }
         
   done:
      return rc ;
   error:
     goto done ;
   }

   void _omaDeployPackage::_aggrFlowArray( const BSONObj& array1,
                                           const BSONObj& array2,
                                           BSONArray& out )
   {
      BSONArrayBuilder arrayBuilder ;

      {
         BSONObjIterator iter( array1 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }

      {
         BSONObjIterator iter( array2 ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            arrayBuilder.append( ele.String() ) ;
         }
      }
      out = arrayBuilder.arr() ;
   }

   INT32 _omaDeployPackage::convertResult( const BSONObj& itemInfo,
                                           BSONObj& taskInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 updateErrno = SDB_OK ;
      INT32 updateProgress = 0 ;
      INT32 progress = taskInfo.getIntField( OMA_FIELD_PROGRESS ) ;
      string updateDetail ;
      string updateHostName ;
      BSONObj resultInfo = taskInfo.getObjectField( OMA_FIELD_RESULTINFO ) ;
      BSONObj condition  = BSON( OMA_FIELD_ERRNO      << "" <<
                                 OMA_FIELD_DETAIL     << "" <<
                                 OMA_FIELD_PROGRESS   << "" <<
                                 OMA_FIELD_RESULTINFO << "" ) ;
      BSONObj oneResultCondition = BSON( OMA_FIELD_HOSTNAME   << "" <<
                                         OMA_FIELD_IP         << "" <<
                                         OMA_FIELD_VERSION    << "" <<
                                         OMA_FIELD_STATUS     << 0  <<
                                         OMA_FIELD_STATUSDESC << "" <<
                                         OMA_FIELD_ERRNO      << 0  <<
                                         OMA_FIELD_DETAIL     << ""  ) ;
      BSONObj updateFlow = itemInfo.getObjectField( OMA_FIELD_FLOW ) ;
      BSONObj nodeResult = itemInfo.filterFieldsUndotted(
                                                    oneResultCondition, TRUE ) ;
      BSONObj taskInfo2 = taskInfo.filterFieldsUndotted( condition, FALSE ) ;
      BSONObjBuilder newTaskInfo ;
      BSONArrayBuilder newResultInfo ;

      rc = omaGetStringElement( itemInfo, OMA_FIELD_HOSTNAME, updateHostName ) ;
      if( rc )
      {
         rc = SDB_OK ;
         goto done ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_ERRNO, updateErrno ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateErrno = SDB_OK ;
      }

      rc = omaGetIntElement( itemInfo, OMA_FIELD_PROGRESS, updateProgress ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateProgress = -1 ;
      }

      rc = omaGetStringElement( itemInfo, OMA_FIELD_DETAIL, updateDetail ) ;
      if( rc )
      {
         rc = SDB_OK ;
         updateDetail = "" ;
      }

      {
         BSONObjIterator resultIter( resultInfo ) ;

         while( resultIter.more() )
         {
            BSONElement resultEle = resultIter.next() ;
            BSONObj oneResult = resultEle.embeddedObject() ;
            string hostName = oneResult.getStringField( OMA_FIELD_HOSTNAME ) ;

            if( updateHostName == hostName )
            {
               BSONObjBuilder newOneResultInfoBuilder ;
               BSONArray newFlowArray ;
               BSONObj flow = oneResult.getObjectField( OMA_FIELD_FLOW ) ;
               if( updateProgress > 0 )
               {
                  progress += updateProgress ;
               }
               if( progress > 100 )
               {
                  progress = 100 ;
               }
               else if( progress < 0 )
               {
                  progress = 0 ;
               }
               _aggrFlowArray( flow, updateFlow, newFlowArray ) ;
               newOneResultInfoBuilder.appendElements( nodeResult ) ;
               newOneResultInfoBuilder.append( OMA_FIELD_FLOW, newFlowArray ) ;
               newResultInfo.append( newOneResultInfoBuilder.obj() ) ;
            }
            else
            {
               newResultInfo.append( oneResult ) ;
            }
         }
      }

      newTaskInfo.append( OMA_FIELD_ERRNO, SDB_OK ) ;
      newTaskInfo.append( OMA_FIELD_DETAIL, "" ) ;
      newTaskInfo.append( OMA_FIELD_PROGRESS, progress ) ;
      newTaskInfo.append( OMA_FIELD_RESULTINFO, newResultInfo.arr() ) ;
      newTaskInfo.appendElements( taskInfo2 ) ;

      taskInfo = newTaskInfo.obj() ;

   done:
      return rc ;
   }

   
   /************************** start plugins ************************/
   /*
      _omaStartPlugins
   */

   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStartPlugins )

   _omaStartPlugins::_omaStartPlugins()
   {
   }

   _omaStartPlugins::~_omaStartPlugins()
   {
   }

   INT32 _omaStartPlugins::init( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_START_PLUGINS, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

   /************************** stop plugins ************************/
   /*
      _omaStopPlugins
   */

   IMPLEMENT_OACMD_AUTO_REGISTER( _omaStopPlugins )

   _omaStopPlugins::_omaStopPlugins()
   {
   }

   _omaStopPlugins::~_omaStopPlugins()
   {
   }

   INT32 _omaStopPlugins::init( const CHAR *pInfo )
   {
      INT32 rc = SDB_OK ;
      try
      {
         stringstream ss ;
         BSONObj bus( pInfo ) ;

         ss << "var " << JS_ARG_BUS << " = " 
            << bus.toString(FALSE, TRUE).c_str() << " ; " ;
         _jsFileArgs = ss.str() ;
         PD_LOG ( PDDEBUG, "Scan host passes argument: %s",
                  _jsFileArgs.c_str() ) ;
         rc = addJsFile( FILE_STOP_PLUGINS, _jsFileArgs.c_str() ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add js file[%s], rc = %d ",
                     FILE_SCAN_HOST, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to build bson, exception is: %s",
                  e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
     goto done ;
   }

}

