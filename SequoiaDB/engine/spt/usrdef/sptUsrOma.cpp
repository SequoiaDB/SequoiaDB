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

   Source File Name = sptUsrOma.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrOma.hpp"
#include "cmdUsrOmaUtil.hpp"
#include "omagentDef.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "ossProc.hpp"
#include "ossIO.hpp"
#include "msgDef.h"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "../bson/bsonobj.h"
#include "pmdDaemon.hpp"
#include "pmdDef.hpp"
#include "utilNodeOpr.hpp"
#include "sptUsrOmaCommon.hpp"

using namespace bson ;

namespace engine
{
   #define SDB_SDBCM_WAIT_TIMEOUT      ( 15 )   /// seconds

   /*
      Function Define
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrOma, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrOma, destruct )
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, toString)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, createCoord)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, removeCoord)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, createData)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, removeData)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, createOM)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, removeOM)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, startNode)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, stopNode)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, runCommand)
   JS_MEMBER_FUNC_DEFINE(_sptUsrOma, close)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getOmaInstallInfo)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getOmaInstallFile)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getOmaConfigFile)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getOmaConfigs)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getIniConfigs)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, setOmaConfigs)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, setIniConfigs)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, getAOmaSvcName)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, addAOmaSvcName)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, delAOmaSvcName)
   JS_STATIC_FUNC_DEFINE(_sptUsrOma, start )

   /*
      Function Map
   */
   JS_BEGIN_MAPPING( _sptUsrOma, "Oma" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC(destruct)
      JS_ADD_MEMBER_FUNC("toString", toString)
      JS_ADD_MEMBER_FUNC("createCoord", createCoord)
      JS_ADD_MEMBER_FUNC("removeCoord", removeCoord)
      JS_ADD_MEMBER_FUNC("createData", createData)
      JS_ADD_MEMBER_FUNC("removeData", removeData)
      JS_ADD_MEMBER_FUNC("createOM", createOM)
      JS_ADD_MEMBER_FUNC("removeOM", removeOM)
      JS_ADD_MEMBER_FUNC("startNode", startNode)
      JS_ADD_MEMBER_FUNC("stopNode", stopNode)
      JS_ADD_MEMBER_FUNC_WITHATTR("_runCommand", runCommand, 0)
      JS_ADD_MEMBER_FUNC("close", close)
      JS_ADD_STATIC_FUNC("getOmaInstallInfo", getOmaInstallInfo)
      JS_ADD_STATIC_FUNC("getOmaInstallFile", getOmaInstallFile)
      JS_ADD_STATIC_FUNC("getOmaConfigFile", getOmaConfigFile)
      JS_ADD_STATIC_FUNC("getOmaConfigs", getOmaConfigs)
      JS_ADD_STATIC_FUNC("getIniConfigs", getIniConfigs)
      JS_ADD_STATIC_FUNC("setOmaConfigs", setOmaConfigs)
      JS_ADD_STATIC_FUNC("setIniConfigs", setIniConfigs)
      JS_ADD_STATIC_FUNC("getAOmaSvcName", getAOmaSvcName)
      JS_ADD_STATIC_FUNC("addAOmaSvcName", addAOmaSvcName)
      JS_ADD_STATIC_FUNC("delAOmaSvcName", delAOmaSvcName)
      JS_ADD_STATIC_FUNC("start", start )
   JS_MAPPING_END()

   /*
      _sptUsrOma Implement
   */
   _sptUsrOma::_sptUsrOma()
   {
      CHAR tmpName[ 10 ] = { 0 } ;
      ossSnprintf( tmpName, sizeof( tmpName ) - 1, "%u", SDBCM_DFT_PORT ) ;
      _hostname = "localhost" ;
      _svcname = tmpName ;
   }

   _sptUsrOma::~_sptUsrOma()
   {
   }

   INT32 _sptUsrOma::construct( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      INT32 retCode = SDB_OK ;
      CHAR* retBuf = NULL ;

      if ( arg.argc() >= 1 )
      {
         rc = arg.getString( 0, _hostname ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "hostname must be string" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get hostname, rc: %d", rc ) ;
      }
      if ( arg.argc() >= 2 )
      {
         rc = arg.getString( 1, _svcname ) ;
         if ( rc )
         {
            UINT16 port = 0 ;
            rc = arg.getNative( 1, (void*)&port, SPT_NATIVE_INT16 ) ;
            if ( rc )
            {
               detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
            }
            else if ( port <= 0 || port >= 65535 )
            {
               detail = BSON( SPT_ERR << "svcname must in range ( 0, 65535 )" ) ;
               rc = SDB_INVALIDARG ;
            }
            else
            {
               _svcname = boost::lexical_cast< string >( port ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;
      }

      rc = _assit.connect( _hostname.c_str(), _svcname.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to connect %s:%s, rc: %d",
                   _hostname.c_str(), _svcname.c_str(), rc ) ;

      rval.addSelfProperty( "_host" )->setValue( _hostname ) ;
      rval.addSelfProperty( "_svcname" )->setValue( _svcname ) ;

      // check remote info
      rc = _assit.runCommand( "oma test", NULL, &retBuf, retCode ) ;
      sdbClearErrorInfo() ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to check target server info" ) ;
         goto error ;
      }

      if( SDB_OK != retCode )
      {
         rc = retCode ;
         detail = BSON( SPT_ERR << "Target server must be sdbcm" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _assit.disconnect() ;
      goto done ;
   }

   INT32 _sptUsrOma::toString( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               BSONObj & detail )
   {
      string name = _hostname ;
      name += ":" ;
      name += _svcname ;
      rval.getReturnVal().setValue( name ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrOma::destruct()
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrOma::createCoord( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
      return _createNode( arg, rval, detail, SDB_ROLE_COORD_STR ) ;
   }

   INT32 _sptUsrOma::removeCoord( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
      return _removeNode( arg, rval, detail, SDB_ROLE_COORD_STR ) ;
   }

   INT32 _sptUsrOma::_createNode( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail,
                                  const CHAR *pNodeStr )
   {
      INT32 rc = SDB_OK ;
      string svcname ;
      string dbpath ;
      BSONObj config ;

      rc = arg.getString( 0, svcname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "svcname must be config" ) ;
      }
      else if ( rc )
      {
         INT32 port = 0 ;
         rc = arg.getNative( 0, (void*)&port, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
         }
         else if ( port <= 0 || port > 65535 )
         {
            detail = BSON( SPT_ERR << "svcname must in range ( 0, 65536 )" ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            svcname = boost::lexical_cast< string >( port ) ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;

      rc = arg.getString( 1, dbpath ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "dbpath must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "dbpath must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get dbpath, rc: %d", rc ) ;

      if ( arg.argc() >= 3 )
      {
         rc = arg.getBsonobj( 2, config ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "config must be object" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get config, rc: %d", rc ) ;
      }

      // add role
      {
         BSONObjBuilder builder ;
         BSONObjIterator it( config ) ;
         while ( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 == ossStrcmp( PMD_OPTION_ROLE, e.fieldName() ) )
            {
               continue ;
            }
            builder.append( e ) ;
         }
         builder.append( PMD_OPTION_ROLE, pNodeStr ) ;
         config = builder.obj() ;
      }

      rc = _assit.createNode( svcname.c_str(), dbpath.c_str(),
                              config.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create %s[%s], rc: %d",
                   pNodeStr, svcname.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::createData( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      return _createNode( arg, rval, detail, SDB_ROLE_STANDALONE_STR ) ;
   }

   INT32 _sptUsrOma::_removeNode( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail,
                                  const CHAR *pNodeStr )
   {
      INT32 rc = SDB_OK ;
      string svcname ;
      BSONObj config ;

      rc = arg.getString( 0, svcname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "svcname must be config" ) ;
      }
      else if ( rc )
      {
         INT32 port = 0 ;
         rc = arg.getNative( 0, (void*)&port, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
         }
         else if ( port <= 0 || port > 65535 )
         {
            detail = BSON( SPT_ERR << "svcname must in range ( 0, 65536 )" ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            svcname = boost::lexical_cast< string >( port ) ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;

      if ( arg.argc() >= 2 )
      {
         rc = arg.getBsonobj( 1, config ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "config must be object" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get config, rc: %d", rc ) ;
      }

      // add role
      {
         BSONObjBuilder builder ;
         BSONObjIterator it( config ) ;
         while ( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 == ossStrcmp( PMD_OPTION_ROLE, e.fieldName() ) )
            {
               continue ;
            }
            builder.append( e ) ;
         }
         builder.append( PMD_OPTION_ROLE, pNodeStr ) ;
         config = builder.obj() ;
      }

      rc = _assit.removeNode( svcname.c_str(), config.objdata() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove %s[%s], rc: %d",
                   pNodeStr, svcname.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::removeData( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      return _removeNode( arg, rval, detail, SDB_ROLE_STANDALONE_STR ) ;
   }

   INT32 _sptUsrOma::createOM( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               BSONObj & detail )
   {
      return _createNode( arg, rval, detail, SDB_ROLE_OM_STR ) ;
   }

   INT32 _sptUsrOma::removeOM( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               BSONObj & detail )
   {
      return _removeNode( arg, rval, detail, SDB_ROLE_OM_STR ) ;
   }

   INT32 _sptUsrOma::startNode( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string svcname ;

      rc = arg.getString( 0, svcname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "svcname must be config" ) ;
      }
      else if ( rc )
      {
         INT32 port = 0 ;
         rc = arg.getNative( 0, (void*)&port, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
         }
         else if ( port <= 0 || port > 65535 )
         {
            detail = BSON( SPT_ERR << "svcname must in range ( 0, 65536 )" ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            svcname = boost::lexical_cast< string >( port ) ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;

      rc = _assit.startNode( svcname.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Start node[%s] failed, rc: %d",
                   svcname.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::stopNode( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string svcname ;

      rc = arg.getString( 0, svcname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "svcname must be config" ) ;
      }
      else if ( rc )
      {
         INT32 port = 0 ;
         rc = arg.getNative( 0, (void*)&port, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "svcname must be string or int" ) ;
         }
         else if ( port <= 0 || port > 65535 )
         {
            detail = BSON( SPT_ERR << "svcname must in range ( 0, 65536 )" ) ;
            rc = SDB_INVALIDARG ;
         }
         else
         {
            svcname = boost::lexical_cast< string >( port ) ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get svcname, rc: %d", rc ) ;

      rc = _assit.stopNode( svcname.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Stop node[%s] failed, rc: %d",
                   svcname.c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::runCommand( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      INT32 retCode = SDB_OK ;
      CHAR *retBuffer = NULL ;
      INT32 needRecv = TRUE ;
      BSONObj mergeObj ;
      BSONObj recvObj ;
      string command ;

      // merge arg
      rc = _mergeArg( arg, detail, command, &mergeObj ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to mergeArg" ) ;
         PD_LOG( PDERROR, "Failed to merge arg, rc: %d", rc ) ;
         goto error ;
      }

      // get argument needRecv
      if ( arg.argc() >= 5 )
      {
         rc = arg.getNative( 4, &needRecv, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "needRecv must be bool" ) ;
            goto error ;
         }
      }

      // run command and get retrun BSONObj
      rc = _assit.runCommand( command, mergeObj.objdata(),
                              &retBuffer, retCode, needRecv ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << getErrDesp( rc ) ) ;
         PD_LOG( PDERROR, "Failed to run command in client, rc: %d", rc ) ;
         goto error ;
      }

      // if need recv, need to build recvObj ;
      if ( needRecv )
      {
         // build recvObj
         SDB_ASSERT( retBuffer, "retBuffer can't be null" ) ;
         try
         {
            recvObj.init( retBuffer ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to build recvObj" ) ;
            PD_LOG( PDERROR, "Failed to build recvObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }

         // if remote cm failed to exec command, retObj contain error detail
         if ( SDB_OK != retCode )
         {
            rc = retCode ;
            detail = BSON( SPT_ERR << recvObj.getStringField( OP_ERR_DETAIL ) ) ;
            PD_LOG( PDERROR, "Failed to run command in remote sdbcm, "
                             "rc: %d", rc ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( recvObj ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::close( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrOma::start( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      CHAR progName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      BOOLEAN asProc = FALSE ;
      BOOLEAN asStandalone = FALSE ;
      string procShortName = PMDDMN_EXE_NAME ;
      OSSPID pid = OSS_INVALID_PID ;
      vector < ossProcInfo > procs ;
      list<const CHAR*> argvs ;
      BSONObj optionObj ;
      utilNodeInfo cmInfo ;
      stringstream portArg ;
      string portStr ;
      stringstream aliveTimeArg ;
      string alivetimeStr ;
      stringstream outStr ;

      rc = ossGetEWD ( progName, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to get excutable file's working "
                        "directory" ) ;
         goto error ;
      }

      if ( 0 < arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
            goto error ;
         }

#if defined (_WINDOWS)
         asProc = optionObj.getBoolField( "asproc" ) ;
         if ( asProc )
         {
            argvs.push_back( "--asproc" ) ;
         }
#endif
         if ( optionObj.hasField( "alivetime" ) )
         {
            BSONType alivetimeType = optionObj.getField( "alivetime" ).type() ;

            if ( NumberInt == alivetimeType )
            {
               aliveTimeArg << optionObj.getIntField( "alivetime" ) ;
               alivetimeStr = aliveTimeArg.str() ;
            }
            else if ( String == alivetimeType )
            {
               alivetimeStr = optionObj.getStringField( "alivetime" ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR << "alivetime must be int" ) ;
               goto error ;
            }
            argvs.push_back( "--alivetime" ) ;
            argvs.push_back( alivetimeStr.c_str() ) ;
         }

         if ( optionObj.hasField( "port" ) )
         {
            BSONType portType = optionObj.getField( "port" ).type() ;
            if ( NumberInt == portType )
            {
               portArg << optionObj.getIntField( "port" ) ;
               portStr = portArg.str() ;
            }
            else if ( String == portType )
            {
               portStr = optionObj.getStringField( "port" ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               detail = BSON( SPT_ERR << "port must be int or string" ) ;
               goto error ;
            }
            argvs.push_back( "--port" ) ;
            argvs.push_back( portStr.c_str() ) ;
         }

         asStandalone = optionObj.getBoolField( "standalone" ) ;
         if ( asStandalone )
         {
            asProc = TRUE ;
            procShortName = SDBSDBCMPROG ;
            argvs.push_back( "--standalone" ) ;
         }
      }
      utilCatPath( progName, OSS_MAX_PATHSIZE, procShortName.c_str() ) ;
      argvs.push_front( progName ) ;

      if ( !asStandalone )
      {
         // first to check whether the process exist
         ossEnumProcesses( procs, procShortName.c_str(), TRUE, TRUE ) ;
         if ( procs.size() > 0 )
         {
            // find it
            outStr << "Success: sdbcmd is already started ("
                   << (*procs.begin())._pid << ")" << endl ;
            goto done ;
         }
      }

      // start progress
      rc = _startSdbcm ( argvs, pid, asProc ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Failed to start sdbcm" ) ;
         goto error ;
      }

      if ( !asStandalone )
      {
         while ( ossIsProcessRunning( pid ) )
         {
            procs.clear() ;
            ossEnumProcesses( procs, procShortName.c_str(), TRUE, TRUE ) ;
            if ( procs.size() > 0 )
            {
               outStr << "Success: sdbcmd is successfully started "
                      << "(" << (*procs.begin())._pid << ")" << endl ;
               break ;
            }
            ossSleep( 200 ) ;
         }

         if ( procs.size() == 0 )
         {
            rc = SDB_SYS ;
            detail = BSON( SPT_ERR << "Failed to start sdbcm" ) ;
            goto error ;
         }
      }

      // wait bussiness ok
      rc = utilWaitNodeOK( cmInfo, NULL,
                           asStandalone ? pid : OSS_INVALID_PID,
                           SDB_TYPE_OMA, SDB_SDBCM_WAIT_TIMEOUT,
                           asStandalone ? TRUE : FALSE ) ;
      if ( SDB_OK == rc )
      {
         outStr << "Success: " << SDB_TYPE_OMA_STR
                << "(" << cmInfo._svcname.c_str() << ")"
                << " is successfully start (" << cmInfo._pid << ")" << endl ;
      }
      else
      {
         detail = BSON( SPT_ERR << "Failed to wait sdbcm ok" ) ;
         goto error ;
      }

   done:
      rval.getReturnVal().setValue( outStr.str() ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getOmaInstallInfo( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      rc = _sptUsrOmaCommon::getOmaInstallInfo( retObj, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getOmaInstallFile( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string retStr ;
      rc = _sptUsrOmaCommon::getOmaInstallFile( retStr, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getOmaConfigFile( const _sptArguments & arg,
                                       _sptReturnVal & rval,
                                       BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      string err ;

      rc = _sptUsrOmaCommon::getOmaConfigFile( confFile, err ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( confFile ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getOmaConfigs( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      BSONObjBuilder argBuilder ;
      string err ;

      if ( arg.argc() > 0 )
      {
         rc = arg.getString( 0, confFile ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "confFile must be string" ) ;
            goto error ;
         }
         argBuilder.append( "confFile", confFile ) ;
      }

      rc = _sptUsrOmaCommon::getOmaConfigs( argBuilder.obj(), conf, err ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( conf ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getIniConfigs( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      BSONObjBuilder argBuilder ;
      string err ;

      rc = arg.getString( 0, confFile ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "confFile must be string" ) ;
         goto error ;
      }

      argBuilder.append( "confFile", confFile ) ;

      if ( arg.argc() > 1 )
      {
         BSONObj options ;

         rc = arg.getBsonobj( 1, options ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "options must be object" ) ;
            goto error ;
         }

         argBuilder.appendElements( options ) ;
      }

      rc = _sptUsrOmaCommon::getIniConfigs( argBuilder.obj(), conf, err ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( conf ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::setOmaConfigs( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      BSONObjBuilder argBuilder ;
      string err ;

      rc = arg.getBsonobj( 0, conf ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "obj must be config" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "obj must be object" ) ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         rc = arg.getString( 1, confFile ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "confFile must be string" ) ;
            goto error ;
         }
         argBuilder.append( "confFile", confFile ) ;
      }

      rc = _sptUsrOmaCommon::setOmaConfigs( argBuilder.obj(), conf, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::setIniConfigs( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string confFile ;
      BSONObj conf ;
      BSONObjBuilder argBuilder ;
      string err ;

      rc = arg.getBsonobj( 0, conf ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "obj must be config" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "obj must be object" ) ;
         goto error ;
      }

      rc = arg.getString( 1, confFile ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "confFile must be string" ) ;
         goto error ;
      }

      argBuilder.append( "confFile", confFile ) ;

      if ( arg.argc() > 2 )
      {
         BSONObj options ;

         rc = arg.getBsonobj( 2, options ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "options must be object" ) ;
            goto error ;
         }

         argBuilder.appendElements( options ) ;
      }

      rc = _sptUsrOmaCommon::setIniConfigs( argBuilder.obj(), conf, err ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::getAOmaSvcName( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObjBuilder argBuilder ;
      string err ;
      string svcName ;

      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
         goto error ;
      }
      argBuilder.append( "hostname", hostname ) ;

      if ( arg.argc() > 1 )
      {
         rc = arg.getString( 1, confFile ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "confFile must be string" ) ;
            goto error ;
         }

         argBuilder.append( "confFile", confFile ) ;
      }

      rc = _sptUsrOmaCommon::getAOmaSvcName( argBuilder.obj(), svcName, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( svcName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::addAOmaSvcName( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string svcname ;
      INT32 isReplace = TRUE ;
      string confFile ;
      BSONObjBuilder valueObjBuilder ;
      BSONObjBuilder optionObjBuilder ;
      BSONObjBuilder matchObjBuilder ;
      string err ;

      // hostname
      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
         goto error ;
      }
      else if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "hostname can't be empty" ) ;
         goto error ;
      }
      valueObjBuilder.append( "hostname", hostname ) ;

      // svcname
      rc = arg.getString( 1, svcname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "svcname must be config" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "svcname must be string" ) ;
         goto error ;
      }
      else if ( svcname.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "svcname can't be empty" ) ;
         goto error ;
      }
      valueObjBuilder.append( "svcname", svcname ) ;

      // get isReplace
      if ( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, (void*)&isReplace, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "isReplace must be BOOLEAN" ) ;
            goto error ;
         }
         optionObjBuilder.appendBool( "isReplace", isReplace ) ;
      }

      // get confFile
      if ( arg.argc() > 3 )
      {
         rc = arg.getString( 3, confFile ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "confFile must be string" ) ;
            goto error ;
         }
         matchObjBuilder.append( "confFile", confFile ) ;
      }

      rc = _sptUsrOmaCommon::addAOmaSvcName( valueObjBuilder.obj(),
                                            optionObjBuilder.obj(),
                                            matchObjBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrOma::delAOmaSvcName( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string confFile ;
      BSONObjBuilder argBuilder ;
      string err ;

      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
         goto error ;
      }
      argBuilder.append( "hostname", hostname ) ;

      if ( arg.argc() > 1 )
      {
         rc = arg.getString( 1, confFile ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "confFile must be string" ) ;
            goto error ;
         }
         argBuilder.append( "confFile", confFile ) ;
      }

      rc = _sptUsrOmaCommon::delAOmaSvcName( argBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_WINDOWS)
   INT32  _sptUsrOma::_startSdbcm ( list<const CHAR*> &argv, OSSPID &pid,
                                    BOOLEAN asProc )
   {
      if ( asProc )
      {
         return ossStartProcess( argv, pid ) ;
      }
      else
      {
         return ossStartService( PMDDMN_SVCNAME_DEFAULT ) ;
      }
   }

#elif defined (_LINUX)
   INT32  _sptUsrOma::_startSdbcm ( list<const CHAR*> &argv, OSSPID &pid,
                                    BOOLEAN asProc )
   {
      return ossStartProcess( argv, pid ) ;
   }
#endif

   INT32 _sptUsrOma::_mergeArg( const _sptArguments &arg,
                                bson::BSONObj &detail,
                                string &command,
                                BSONObj *mergeObj )
   {
      INT32 rc = SDB_OK ;
      BSONObj optionObj ;
      BSONObj matchObj ;
      BSONObj valueObj ;

      // get command
      rc = arg.getString( 0, command ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
         PD_LOG( PDERROR, "Command must be config, rc: %d", rc ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
         PD_LOG( PDERROR, "Command must be string, rc: %d", rc ) ;
         goto error ;
      }
      else if ( command.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "command can't be empty" ) ;
         PD_LOG( PDERROR, "Command can't be empty, rc: %d", rc ) ;
         goto error ;
      }

      // get optionObj
      if ( arg.argc() >= 2 )
      {
         rc = arg.getBsonobj( 1, optionObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get BSONObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get matchObj
      if ( arg.argc() >= 3 )
      {
         rc = arg.getBsonobj( 2, matchObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "matchObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get mathcObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get valueObj
      if ( arg.argc() >= 4 )
      {
         rc = arg.getBsonobj( 3, valueObj ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "valueObj must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get valueObj , rc: %d", rc ) ;
            goto error ;
         }
      }

      // merge argument
      {
         BSONObjBuilder builder ;
         builder.append( "$optionObj", optionObj ) ;
         builder.append( "$matchObj", matchObj ) ;
         builder.append( "$valueObj", valueObj ) ;
         *mergeObj = builder.obj() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

}