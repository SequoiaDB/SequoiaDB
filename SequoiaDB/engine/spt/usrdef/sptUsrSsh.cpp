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

   Source File Name = sptUsrSsh.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrSsh.hpp"
#include "sptLibssh2Session.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, close )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, exec )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, copy2Remote )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, copyFromRemote )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, toString )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, getLastRet )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, getLastOutStr )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, getLocalIP )
JS_MEMBER_FUNC_DEFINE( _sptUsrSsh, getPeerIP )
JS_CONSTRUCT_FUNC_DEFINE( _sptUsrSsh, construct )
JS_DESTRUCT_FUNC_DEFINE( _sptUsrSsh, destruct )

JS_BEGIN_MAPPING( _sptUsrSsh, "Ssh" )
   JS_ADD_MEMBER_FUNC( "close", close )
   JS_ADD_MEMBER_FUNC( "exec", exec )
   JS_ADD_MEMBER_FUNC( "push", copy2Remote )
   JS_ADD_MEMBER_FUNC( "pull", copyFromRemote )
   JS_ADD_MEMBER_FUNC( "getLocalIP", getLocalIP )
   JS_ADD_MEMBER_FUNC( "getPeerIP", getPeerIP )
   JS_ADD_MEMBER_FUNC( "toString", toString )
   JS_ADD_MEMBER_FUNC( "getLastRet", getLastRet )
   JS_ADD_MEMBER_FUNC( "getLastOut", getLastOutStr )
   JS_ADD_CONSTRUCT_FUNC( construct )
   JS_ADD_DESTRUCT_FUNC( destruct )
JS_MAPPING_END()

   _sptUsrSsh::_sptUsrSsh()
   :_session( NULL )
   {
      _lastRet = 0 ;
   }

   _sptUsrSsh::~_sptUsrSsh()
   {
      SAFE_OSS_DELETE( _session ) ;
   }

   INT32 _sptUsrSsh::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail)
   {
      INT32 rc = SDB_OK ;
      string passwd ;
      string errmsg ;
      INT32 port = SPT_SSH_PORT ;

      rc = arg.getString( 0, _host ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "hostname must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "hostname must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get hostname, rc: %d", rc ) ;

      rc = arg.getString( 1, _user ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "user must be string" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get user, rc: %d", rc ) ;
      }

      rc = arg.getString( 2, passwd ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "password must be string" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get password, rc: %d", rc ) ;
      }

      rc = arg.getNative( 3, (void*)&port, SPT_NATIVE_INT32 ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "port must be uint or int" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get port, rc: %d", rc ) ;
      }

      _session = SDB_OSS_NEW _sptLibssh2Session( _host.c_str(),
                                                 _user.c_str(),
                                                 passwd.c_str(),
                                                 &port ) ;
      if ( NULL == _session )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _session->open() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open ssh session:%d", rc ) ;
         _session->getLastError( errmsg ) ;
         goto error ;
      }
      _localIP = _session->getLocalIPAddr() ;
      _peerIP = _session->getPeerIPAddr() ;

      rval.addSelfProperty("_host")->setValue( _host ) ;
      rval.addSelfProperty("_port")->setValue( port ) ;
      rval.addSelfProperty("_usrname")->setValue( _user ) ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _session ) ;
      if ( detail.isEmpty() )
      {
         if ( !errmsg.empty() )
         {
            detail = BSON( SPT_ERR << errmsg ) ;
         }
         else
         {
            detail = BSON( SPT_ERR << "failed to ssh to specified host" ) ;
         }
      }
      goto done ;
   }

   INT32 _sptUsrSsh::destruct()
   {
      SAFE_OSS_DELETE( _session ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSsh::toString( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               bson::BSONObj & detail )
   {
      string str = _user ;
      str += "@" ;
      str += _host ;
      rval.getReturnVal().setValue( str ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSsh::copy2Remote( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string local ;
      string dst ;
      INT32 mode = 0755 ;

      string errMsg ;

      rc = arg.getString( 0, local ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "local_file must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "local_file must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get local_file, rc: %d", rc ) ;

      rc = arg.getString( 1, dst ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "dst_file must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "dst_file must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get dst_file, rc: %d", rc ) ;

      rc = arg.getNative( 2, (void*)&mode, SPT_NATIVE_INT32 ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "mode must be native type" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get mode, rc: %d", rc ) ;
      }

      if ( !_session->isOpened() )
      {
         detail = BSON( SPT_ERR << "connection is shutdown" ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }

      rc = _session->copy2Remote( SPT_CP_PROTOCOL_SCP,
                                  local.c_str(),
                                  dst.c_str(),
                                  mode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to copy file:%d", rc ) ;
         _session->getLastError( errMsg ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( !errMsg.empty() )
      {
         detail = BSON( SPT_ERR << errMsg ) ;
      }
      goto done ;
   }

   INT32 _sptUsrSsh::copyFromRemote( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string remote ;
      string local ;
      INT32 mode = 0 ;
      string errMsg ;

      rc = arg.getString( 0, remote ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "remote_file must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "remote_file must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get remote_file, rc: %d", rc ) ;

      rc = arg.getString( 1, local ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "local_file must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "local_file must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get local_file, rc: %d", rc ) ;

      rc = arg.getNative( 2, &mode, SPT_NATIVE_INT32 ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "mode must be native type" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get mode, rc: %d", rc ) ;
      }

      if ( !_session->isOpened() )
      {
         detail = BSON( SPT_ERR << "connection is shutdown" ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }

      rc = _session->copyFromRemote( SPT_CP_PROTOCOL_SCP,
                                     remote.c_str(),
                                     local.c_str(),
                                     mode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to copy file:%d", rc ) ;
         _session->getLastError( errMsg ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      if ( !errMsg.empty() )
      {
         detail = BSON( SPT_ERR << errMsg ) ;
      }
      goto done ;
   }

   INT32 _sptUsrSsh::close( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      if ( _session )
      {
         _session->close() ;
      }
      return SDB_OK ;
   }

   INT32 _sptUsrSsh::exec( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != _session, "can not be null" ) ;
      string cmd ;
      string errMsg ;
      string sig ;
      _lastRet = 0 ;
      _lastOutStr = "" ;

      rc = arg.getString( 0, cmd ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get command, rc: %d", rc ) ;

      if ( !_session->isOpened() )
      {
         detail = BSON( SPT_ERR << "connection is shutdown" ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }

      rc = _session->exec( cmd.c_str(), _lastRet, _lastOutStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd[%s], rc: %d", cmd.c_str(), rc ) ;
         if ( _lastOutStr.empty() )
         {
            _session->getLastError( errMsg ) ;
         }
         else
         {
            errMsg = _lastOutStr ;
         }
         goto error ;
      }
      else if ( SDB_OK != _lastRet )
      {
         errMsg = _lastOutStr ;
         rc = _lastRet ;
         goto error ;
      }

      rval.getReturnVal().setValue( _lastOutStr ) ;

   done:
      return rc ;
   error:
      if ( !errMsg.empty() )
      {
         detail = BSON( SPT_ERR << errMsg ) ;
      }
      goto done ;
   }

   INT32 _sptUsrSsh::getLastRet( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      rval.getReturnVal().setValue( _lastRet ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSsh::getLastOutStr( const _sptArguments & arg,
                                    _sptReturnVal & rval,
                                    BSONObj & detail )
   {
      rval.getReturnVal().setValue( _lastOutStr ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrSsh::getLocalIP( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      if ( _localIP.empty() )
      {
         detail = BSON( SPT_ERR << "not connect" ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }
      rval.getReturnVal().setValue( _localIP ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrSsh::getPeerIP( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      if ( _peerIP.empty() )
      {
         detail = BSON( SPT_ERR << "not connect" ) ;
         rc = SDB_NETWORK ;
         goto error ;
      }
      rval.getReturnVal().setValue( _peerIP ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

