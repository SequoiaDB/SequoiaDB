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

   Source File Name = sptUsrSystem.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrSystem.hpp"
#include "ossSocket.hpp"
#include "sptUsrSystemCommon.hpp"
using namespace bson ;
using std::pair ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrSystem, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrSystem, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrSystem, getInfo )
   JS_MEMBER_FUNC_DEFINE( _sptUsrSystem, memberHelp )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getObj )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, ping )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, type )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getReleaseInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHostsMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delAHostMap )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getCpuInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotCpuInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getMemInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotMemInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getDiskInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotDiskInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getNetcardInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, snapshotNetcardInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getIpTablesInfo )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHostName )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, sniffPort )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getPID )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getTID )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getEWD )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listProcess )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, killProcess )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, addGroup )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, setUserConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, delGroup )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listLoginUsers )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listAllUsers )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, listGroups )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getCurrentUser )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getSystemConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getProcUlimitConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, setProcUlimitConfigs )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, runService )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, createSshKey )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getHomePath )
   JS_STATIC_FUNC_DEFINE( _sptUsrSystem, getUserEnv )

   JS_BEGIN_MAPPING( _sptUsrSystem, "System" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getInfo", getInfo, 0 )
      JS_ADD_MEMBER_FUNC( "help", memberHelp )
      JS_ADD_STATIC_FUNC( "getObj", getObj )
      JS_ADD_STATIC_FUNC( "ping", ping )
      JS_ADD_STATIC_FUNC( "type", type )
      JS_ADD_STATIC_FUNC( "getReleaseInfo", getReleaseInfo )
      JS_ADD_STATIC_FUNC( "getHostsMap", getHostsMap )
      JS_ADD_STATIC_FUNC( "getAHostMap", getAHostMap )
      JS_ADD_STATIC_FUNC( "addAHostMap", addAHostMap )
      JS_ADD_STATIC_FUNC( "delAHostMap", delAHostMap )
      JS_ADD_STATIC_FUNC( "getCpuInfo", getCpuInfo )
      JS_ADD_STATIC_FUNC( "snapshotCpuInfo", snapshotCpuInfo )
      JS_ADD_STATIC_FUNC( "getMemInfo", getMemInfo )
      JS_ADD_STATIC_FUNC( "snapshotMemInfo", snapshotMemInfo )
      JS_ADD_STATIC_FUNC( "getDiskInfo", getDiskInfo )
      JS_ADD_STATIC_FUNC( "snapshotDiskInfo", snapshotDiskInfo )
      JS_ADD_STATIC_FUNC( "getNetcardInfo", getNetcardInfo )
      JS_ADD_STATIC_FUNC( "snapshotNetcardInfo", snapshotNetcardInfo )
      JS_ADD_STATIC_FUNC( "getIpTablesInfo", getIpTablesInfo )
      JS_ADD_STATIC_FUNC( "getHostName", getHostName )
      JS_ADD_STATIC_FUNC( "sniffPort", sniffPort )
      JS_ADD_STATIC_FUNC( "getPID", getPID )
      JS_ADD_STATIC_FUNC( "getTID", getTID )
      JS_ADD_STATIC_FUNC( "getEWD", getEWD )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listProcess", listProcess, 0 )
      JS_ADD_STATIC_FUNC( "killProcess", killProcess )
      JS_ADD_STATIC_FUNC( "addUser", addUser )
      JS_ADD_STATIC_FUNC( "addGroup", addGroup )
      JS_ADD_STATIC_FUNC( "setUserConfigs", setUserConfigs )
      JS_ADD_STATIC_FUNC( "delUser", delUser )
      JS_ADD_STATIC_FUNC( "delGroup", delGroup )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listLoginUsers", listLoginUsers, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listAllUsers", listAllUsers, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_listGroups", listGroups, 0 )
      JS_ADD_STATIC_FUNC( "getCurrentUser", getCurrentUser )
      JS_ADD_STATIC_FUNC( "getSystemConfigs", getSystemConfigs )
      JS_ADD_STATIC_FUNC( "getProcUlimitConfigs", getProcUlimitConfigs )
      JS_ADD_STATIC_FUNC( "setProcUlimitConfigs", setProcUlimitConfigs )
      JS_ADD_STATIC_FUNC( "runService", runService )
      JS_ADD_STATIC_FUNC( "getUserEnv", getUserEnv )
      JS_ADD_STATIC_FUNC_WITHATTR( "_createSshKey", createSshKey, 0 )
      JS_ADD_STATIC_FUNC_WITHATTR( "_getHomePath", getHomePath, 0 )
   JS_MAPPING_END()

   _sptUsrSystem::_sptUsrSystem()
   {
   }

   _sptUsrSystem::~_sptUsrSystem()
   {
   }

   INT32 _sptUsrSystem::getObj( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sptUsrSystem * systemObj = _sptUsrSystem::crtInstance() ;
      if ( !systemObj )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Create object failed" ) ;
      }
      else
      {
         rval.setUsrObjectVal<_sptUsrSystem>( systemObj ) ;
      }
      return rc ;
   }

   INT32 _sptUsrSystem::construct( const _sptArguments & arg,
                                   _sptReturnVal & rval,
                                   BSONObj & detail )
   {
      detail = BSON( SPT_ERR << "Please get System Obj by calling Remote "
                                "member function: getSystem()" ) ;
      return SDB_SYS ;
   }

   INT32 _sptUsrSystem::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptUsrSystem::getInfo( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj remoteInfo ;
      BSONObjBuilder builder ;

      if ( 0 < arg.argc() )
      {
         rc = arg.getBsonobj( 0, remoteInfo ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "remoteInfo must be obj" ) ;
            goto error ;
         }
      }

      builder.append( "type", "System" ) ;
      builder.appendElements( remoteInfo ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::ping( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string host ;
      string err ;
      rc = arg.getString( 0, host ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get hostname, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::ping( host, err, retObj ) ;
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

   INT32 _sptUsrSystem::type( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string type ;
      string err ;
      if ( 0 < arg.argc() )
      {
         PD_LOG( PDERROR, "type() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _sptUsrSystemCommon::type( err , type ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( type ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getReleaseInfo( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;
      if ( 0 < arg.argc() )
      {
         PD_LOG( PDERROR, "getReleaseInfo() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _sptUsrSystemCommon::getReleaseInfo( err, retObj ) ;
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

   INT32 _sptUsrSystem::getHostsMap( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;
      if ( 0 < arg.argc() )
      {
         detail = BSON( SPT_ERR << "getHostsMap() should have non arguments" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getHostsMap( err, retObj ) ;
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

   INT32 _sptUsrSystem::getAHostMap( const _sptArguments & arg,
                                     _sptReturnVal &rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string err ;
      string ip ;

      rc = arg.getString( 0, hostname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "hostname must config" ) ;
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

      rc = _sptUsrSystemCommon::getAHostMap( hostname, err, ip ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( ip ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::addAHostMap( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string ip ;
      INT32  isReplace = 1 ;
      string err ;
      VEC_HOST_ITEM vecItems ;

      // hostname
      rc = arg.getString( 0, hostname ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         err = "hostname must be config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "hostname must be string" ;
         goto error ;
      }
      else if ( hostname.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "hostname can't be empty" ;
         goto error ;
      }

      // ip
      rc = arg.getString( 1, ip ) ;
      if ( rc == SDB_OUT_OF_BOUND )
      {
         err = "ip must be config" ;
         goto error ;
      }
      else if ( rc )
      {
         err = "ip must be string" ;
         goto error ;
      }
      else if ( ip.empty() )
      {
         rc = SDB_INVALIDARG ;
         err = "ip can't be empty" ;
         goto error ;
      }

      // isReplace
      if ( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, (void*)&isReplace, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            err = "isReplace must be BOOLEAN" ;
            goto error ;
         }
      }

      rc = _sptUsrSystemCommon::addAHostMap( hostname, ip, isReplace, err ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::delAHostMap( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string err ;
      VEC_HOST_ITEM vecItems ;

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

      rc = _sptUsrSystemCommon::delAHostMap( hostname, err ) ;
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

   INT32 _sptUsrSystem::getCpuInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      rc = _sptUsrSystemCommon::getCpuInfo( err, retObj ) ;
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

   INT32 _sptUsrSystem::snapshotCpuInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      rc = _sptUsrSystemCommon::snapshotCpuInfo( err, retObj ) ;
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

   INT32 _sptUsrSystem::getMemInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      rc = _sptUsrSystemCommon::getMemInfo( err, retObj ) ;
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

   INT32 _sptUsrSystem::snapshotMemInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      return getMemInfo( arg, rval, detail ) ;
   }

   INT32 _sptUsrSystem::getDiskInfo( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      rc = _sptUsrSystemCommon::getDiskInfo( err, retObj ) ;
      if ( SDB_OK != rc )
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

   INT32 _sptUsrSystem::snapshotDiskInfo( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          bson::BSONObj &detail )
   {
      return getDiskInfo( arg, rval, detail ) ;
   }

   INT32 _sptUsrSystem::getNetcardInfo( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getNetcardInfo( err, retObj ) ;
      if ( SDB_OK != rc )
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

   INT32 _sptUsrSystem::getIpTablesInfo( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _sptUsrSystemCommon::getIpTablesInfo( err, retObj ) ;
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

   INT32 _sptUsrSystem::snapshotNetcardInfo( const _sptArguments &arg,
                                             _sptReturnVal &rval,
                                             bson::BSONObj &detail )
   {
      BSONObj retObj ;
      INT32 rc = SDB_OK ;
      stringstream ss ;
      string err ;

      if ( 0 < arg.argc() )
      {
         PD_LOG ( PDERROR, "paramenter can't be greater then 0" ) ;
         rc = SDB_INVALIDARG ;
         ss << "paramenter can't be greater then 0" ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::snapshotNetcardInfo( err, retObj ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         PD_LOG( PDERROR, "snapshotNetcardInfo failed:rc=%d", rc ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getHostName( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string hostname ;
      string err ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getHostName( err, hostname ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( hostname ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::sniffPort ( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 port = 0 ;
      BSONObj retObj ;
      string err ;
      if ( 0 == arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         err = "not specified the port to sniff" ;
         goto error ;
      }
      rc = arg.getNative( 0, &port, SPT_NATIVE_INT32 ) ;
      if ( rc )
      {
         string svcname ;
         UINT16 tempPort ;
         rc = arg.getString( 0, svcname ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "failed to get port argument: %d", rc ) ;
            err = "port must be number or string" ;
            goto error ;
         }

         rc = ossGetPort( svcname.c_str(), tempPort ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "failed to get port by serviceName: %d", rc ) ;
            err = "Invalid svcname: " + svcname ;
            goto error ;
         }
         port = tempPort ;
      }

      rc = _sptUsrSystemCommon::sniffPort( port, err, retObj ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getPID ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      string err ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         err = "No need arguments" ;
         goto error ;
      }
      rc = _sptUsrSystemCommon::getPID( id, err ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      rval.getReturnVal().setValue( id ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err.c_str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getTID ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 id = 0 ;
      string err ;

      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         err = "No need arguments" ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getTID( id, err ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      rval.getReturnVal().setValue( id ) ;

   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err.c_str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::getEWD ( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string ewd ;
      if ( 0 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         err = "No need arguments" ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getEWD( ewd, err ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      rval.getReturnVal().setValue( ewd ) ;
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err.c_str() ) ;
      goto done ;
   }

   INT32 _sptUsrSystem::listProcess( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc         = SDB_OK ;
      BSONObj          optionObj ;
      BSONObj          retObj ;
      BOOLEAN          showDetail = FALSE ;
      string           err ;
      // get optionObj
      if( arg.argc() > 0 )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be object" ) ;
            PD_LOG( PDERROR, "optionObj must be object, rc: %d", rc ) ;
            goto error ;
         }
         showDetail = optionObj.getBoolField( "detail" ) ;
      }

      rc = _sptUsrSystemCommon::listProcess( showDetail, err, retObj ) ;
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

   INT32 _sptUsrSystem::killProcess( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            optionObj ;
      string             err ;
      // check argument
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "killProcess() only have an argument" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BsonObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::killProcess( optionObj, err ) ;
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

   INT32 _sptUsrSystem::addUser( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 rc          = SDB_OK ;
      BSONObj           userObj ;
      string            err ;
      // check argument and build cmd
      rc = arg.getBsonobj( 0, userObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "userObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "userObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get userObj, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::addUser( userObj, err ) ;
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

   INT32 _sptUsrSystem::addGroup( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32 rc        = SDB_OK ;
      BSONObj         groupObj ;
      string          err ;
      // check argument and build cmd
      rc = arg.getBsonobj( 0, groupObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "groupObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "groupObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get groupObj, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::addGroup( groupObj, err ) ;
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

   INT32 _sptUsrSystem::setUserConfigs( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        BSONObj &detail )
   {
      INT32 rc          = SDB_OK ;
      BSONObj           optionObj ;
      string            err ;
      // check argument and build cmd
      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::setUserConfigs( optionObj, err ) ;
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

   INT32 _sptUsrSystem::delUser( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      INT32 rc          = SDB_OK ;
      BSONObj           optionObj ;
      string            err ;
      // check argument and build cmd
      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be config" ) ;
      }
      else if ( SDB_INVALIDARG == rc )
      {
         detail = BSON( SPT_ERR << "optionObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get optionObj, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::delUser( optionObj, err ) ;
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

   INT32 _sptUsrSystem::delGroup( const _sptArguments & arg,
                                  _sptReturnVal & rval,
                                  BSONObj & detail )
   {
      INT32 rc          = SDB_OK ;
      string            name ;
      string            err ;
      // check argument
      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "name must be config" ) ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "name must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get name, rc: %d", rc ) ;

      rc = _sptUsrSystemCommon::delGroup( name, err ) ;
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

   INT32 _sptUsrSystem::listLoginUsers( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            optionObj ;
      BSONObj            retObj ;
      string             err ;
      // check argument
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
      }
      rc = _sptUsrSystemCommon::listLoginUsers( optionObj, err, retObj ) ;
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

   INT32 _sptUsrSystem::listAllUsers( const _sptArguments & arg,
                                      _sptReturnVal & rval,
                                      BSONObj & detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            retObj ;
      BSONObj            optionObj ;
      string             err ;
      // check argument
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
      }

      rc = _sptUsrSystemCommon::listAllUsers( optionObj, err, retObj ) ;
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

   INT32 _sptUsrSystem::listGroups( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            retObj ;
      BSONObj            optionObj ;
      string             err ;
      // check argument
      if ( 1 < arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "too much arguments" ) ;
         goto error ;
      }
      if ( 1 == arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "optionObj must be config" ) ;
            goto error ;
         }
      }

      rc = _sptUsrSystemCommon::listGroups( optionObj, err, retObj ) ;
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

   INT32 _sptUsrSystem::getCurrentUser( const _sptArguments & arg,
                                        _sptReturnVal & rval,
                                        BSONObj & detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            retObj ;
      string             err ;
      rc = _sptUsrSystemCommon::getCurrentUser( err, retObj ) ;
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

   INT32 _sptUsrSystem::getSystemConfigs( const _sptArguments &arg,
                                          _sptReturnVal &rval,
                                          BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            retObj ;
      string             type ;
      string             err ;
      if ( 0 < arg.argc() )
      {
         rc = arg.getString( 0, type) ;
         if ( SDB_OUT_OF_BOUND == rc )
         {
            detail = BSON( SPT_ERR << "type must be config" ) ;
         }
         else if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "type must be string" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get type, rc: %d", rc ) ;
      }

      rc = _sptUsrSystemCommon::getSystemConfigs( type, err, retObj ) ;
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

   INT32 _sptUsrSystem::getProcUlimitConfigs( const _sptArguments &arg,
                                              _sptReturnVal &rval,
                                              BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;
      // check argument
      if ( 1 <= arg.argc() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "getUlimitConfigs() should have non arguments" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::getProcUlimitConfigs( err, retObj ) ;
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

   INT32 _sptUsrSystem::setProcUlimitConfigs( const _sptArguments &arg,
                                              _sptReturnVal &rval,
                                              BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      BSONObj            configsObj ;
      string             err ;
      // get argument
      rc = arg.getBsonobj( 0, configsObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "configsObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "configsObj must be obj" ) ;
         goto error ;
      }

      rc = _sptUsrSystemCommon::setProcUlimitConfigs( configsObj, err ) ;
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

   INT32 _sptUsrSystem::runService( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    BSONObj &detail )
   {
      INT32 rc           = SDB_OK ;
      string             serviceName ;
      string             command ;
      string             options ;
      string             retStr ;
      string             err ;
      // check argument
      rc = arg.getString( 0, serviceName ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "serviceName must be config" ) ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "serviceName must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get serviceName, rc: %d", rc ) ;

      rc = arg.getString( 1, command ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get command, rc: %d", rc ) ;


      if ( 2 < arg.argc() )
      {
         rc = arg.getString( 2, options ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "options must be string" ) ;
            goto error ;
         }
      }

      rc = _sptUsrSystemCommon::runService( serviceName, command, options,
                                           err, retStr ) ;
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

   INT32 _sptUsrSystem::createSshKey( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string err ;
      rc = _sptUsrSystemCommon::createSshKey( err ) ;
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

   INT32 _sptUsrSystem::getHomePath( const _sptArguments & arg,
                                     _sptReturnVal & rval,
                                     BSONObj & detail )
   {
      INT32              rc = SDB_OK ;
      string             homeDir ;
      string             err ;
      rc = _sptUsrSystemCommon::getHomePath( homeDir, err ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( homeDir ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSystem::getUserEnv( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      string err ;
      rc = _sptUsrSystemCommon::getUserEnv( err, retObj ) ;
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

   INT32 _sptUsrSystem::staticHelp( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      stringstream ss ;
      ss << "Local static functions:" << endl ;
      ss << "var system = remoteObj.getSystem()" << endl ;
      ss << " System.ping( hostname )" << endl ;
      ss << " System.type()" << endl ;
      ss << " System.getReleaseInfo()" << endl ;
      ss << " System.getHostsMap()" << endl ;
      ss << " System.getAHostMap( hostname )" << endl ;
      ss << " System.addAHostMap( hostname, ip, [isReplace] )" << endl ;
      ss << " System.delAHostMap( hostname )" << endl ;
      ss << " System.getCpuInfo()" << endl ;
      ss << " System.snapshotCpuInfo()" << endl ;
      ss << " System.getMemInfo()" << endl ;
      ss << " System.snapshotMemInfo()" << endl ;
      ss << " System.getDiskInfo()" << endl ;
      ss << " System.snapshotDiskInfo()" << endl ;
      ss << " System.getNetcardInfo()" << endl ;
      ss << " System.snapshotNetcardInfo()" << endl ;
      ss << " System.getIpTablesInfo()" << endl ;
      ss << " System.getHostName()" << endl ;
      ss << " System.sniffPort( port )" << endl ;
      ss << " System.getPID()" << endl ;
      ss << " System.getTID()" << endl ;
      ss << " System.getEWD()" << endl ;
      ss << " System.listProcess( [optionObj], [filterObj] )" << endl ;
      ss << " System.isProcExist( optionObj )" << endl ;
      ss << " System.killProcess( optionObj )" << endl ;
      ss << " System.getUserEnv()" << endl ;
#if defined (_LINUX)
      ss << " System.addUser( userObj )" << endl ;
      ss << " System.addGroup( groupObj )" << endl ;
      ss << " System.setUserConfigs( optionObj )" << endl ;
      ss << " System.delUser( optionObj )" << endl ;
      ss << " System.delGroup( name )" << endl ;
      ss << " System.listLoginUsers( [optionObj], [filterObj] )" << endl ;
      ss << " System.listAllUsers( [optionObj], [filterObj] )" << endl ;
      ss << " System.listGroups( [optionObj], [filterObj] )" << endl ;
      ss << " System.getCurrentUser()" << endl ;
      ss << " System.isUserExist( username )" << endl ;
      ss << " System.isGroupExist( groupname )" << endl ;
      ss << " System.getProcUlimitConfigs()" << endl ;
      ss << " System.setProcUlimitConfigs( configsObj )" << endl ;
      ss << " System.getSystemConfigs( [type] )" << endl ;
#endif
      ss << " System.runService( servicename, command, [option] )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSystem::memberHelp( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      stringstream ss ;
      ss << "Remote System member functions:" << endl ;
      ss << "   ping( hostname )" << endl ;
      ss << "   type()" << endl ;
      ss << "   getReleaseInfo()" << endl ;
      ss << "   getHostsMap()" << endl ;
      ss << "   getAHostMap( hostname )" << endl ;
      ss << "   addAHostMap( hostname, ip, [isReplace] )" << endl ;
      ss << "   delAHostMap( hostname )" << endl ;
      ss << "   getCpuInfo()" << endl ;
      ss << "   snapshotCpuInfo()" << endl ;
      ss << "   getMemInfo()" << endl ;
      ss << "   snapshotMemInfo()" << endl ;
      ss << "   getDiskInfo()" << endl ;
      ss << "   snapshotDiskInfo()" << endl ;
      ss << "   getNetcardInfo()" << endl ;
      ss << "   snapshotNetcardInfo()" << endl ;
      ss << "   getIpTablesInfo()" << endl ;
      ss << "   getHostName()" << endl ;
      ss << "   sniffPort( port )" << endl ;
      ss << "   getPID()" << endl ;
      ss << "   getTID()" << endl ;
      ss << "   getEWD()" << endl ;
      ss << "   listProcess( [optionObj], [filterObj] )" << endl ;
      ss << "   isProcExist( optionObj )" << endl ;
      ss << "   killProcess( optionObj )" << endl ;
      ss << "   getUserEnv()" << endl ;
      ss << "   addUser( userObj )" << endl ;
      ss << "   addGroup( groupObj )" << endl ;
      ss << "   setUserConfigs( optionObj )" << endl ;
      ss << "   delUser( optionObj )" << endl ;
      ss << "   delGroup( name )" << endl ;
      ss << "   listLoginUsers( [optionObj], [filterObj] )" << endl ;
      ss << "   listAllUsers( [optionObj], [filterObj] )" << endl ;
      ss << "   listGroups( [optionObj], [filterObj] )" << endl ;
      ss << "   getCurrentUser()" << endl ;
      ss << "   isUserExist( username )" << endl ;
      ss << "   isGroupExist( groupname )" << endl ;
      ss << "   getProcUlimitConfigs()" << endl ;
      ss << "   setProcUlimitConfigs( configsObj )" << endl ;
      ss << "   getSystemConfigs( [type] )" << endl ;
      ss << "   buildTrusty()" << endl ;
      ss << "   removeTrusty()" << endl ;
      ss << "   runService( servicename, command, [option] )" << endl ;
      ss << "   getInfo()" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}

