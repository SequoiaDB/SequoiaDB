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

   Source File Name = omagentRemoteUsrCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentRemoteUsrCmd.hpp"
#include "omagentMgr.hpp"
#include "omagentDef.hpp"
#include "sptUsrCmdCommon.hpp"
#include <boost/algorithm/string.hpp>

#define SPT_USER_CMD_ONCE_SLEEP_TIME            ( 2 )
using namespace bson ;

namespace engine
{
   /*
      _remoteCmdRun implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdRun )

    _remoteCmdRun::_remoteCmdRun()
   {
   }

   _remoteCmdRun::~_remoteCmdRun()
   {
   }

   const CHAR* _remoteCmdRun::name()
   {
      return OMA_REMOTE_CMD_RUN ;
   }

   INT32 _remoteCmdRun::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string command ;
      string ev ;
      UINT32 timeout = 0 ;
      UINT32 useShell = 1 ;
      string err ;
      string strOut ;
      BSONObjBuilder builder ;
      _sptUsrCmdCommon _cmdCommon ;

      // get argument
      if ( FALSE == _valueObj.hasField( "command" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be string" ) ;
         goto error ;
      }
      command = _valueObj.getStringField( "command" ) ;

      if ( TRUE == _valueObj.hasField( "args" ) )
      {
         if( String != _valueObj.getField( "args" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "args must be string" ) ;
            goto error ;
         }
         else
         {
            ev = _valueObj.getStringField( "args" ) ;
         }
      }

      if ( TRUE == _optionObj.hasField( "timeout" ) )
      {
         if( NumberInt != _optionObj.getField( "timeout" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "timeout must be int" ) ;
            goto error ;
         }
         else
         {
            timeout = _optionObj.getIntField( "timeout" ) ;
         }
      }

      if ( TRUE == _optionObj.hasField( "useShell" ) )
      {
         if( NumberInt != _optionObj.getField( "useShell" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "useShell must be int" ) ;
            goto error ;
         }
         else
         {
            useShell = _optionObj.getIntField( "useShell" ) ;
         }
      }

      rc = _cmdCommon.exec( command, ev, timeout, useShell, err, strOut ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         builder.append( OP_ERR_DETAIL, err ) ;
         retObj = builder.obj() ;
         goto error ;
      }

      builder.append( "strOut", strOut ) ;
      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteCmdStart implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdStart )

    _remoteCmdStart::_remoteCmdStart()
   {
   }

   _remoteCmdStart::~_remoteCmdStart()
   {
   }

   const CHAR* _remoteCmdStart::name()
   {
      return OMA_REMOTE_CMD_START ;
   }

   INT32 _remoteCmdStart::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      string command ;
      string ev ;
      UINT32 useShell = 1 ;
      UINT32 timeout  = 100 ;
      string strOut ;
      string err ;
      INT32 pid ;
      _sptUsrCmdCommon _cmdCommon ;

      if ( FALSE == _valueObj.hasField( "command" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be string" ) ;
         goto error ;
      }
      command = _valueObj.getStringField( "command" ) ;

      if ( TRUE == _valueObj.hasField( "args" ) )
      {
         if ( String != _valueObj.getField( "args" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "environment should be a string" ) ;
            goto error ;
         }
         ev = _valueObj.getStringField( "args" ) ;
      }

      if ( TRUE == _optionObj.hasField( "timeout" ) )
      {
         if ( NumberInt != _optionObj.getField( "timeout" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "timeout should be a number" ) ;
            goto error ;
         }
         timeout = _optionObj.getIntField( "timeout" ) ;
      }

      // useShell, default : 1
      if ( TRUE == _optionObj.hasField( "useShell" ) )
      {
         if ( NumberInt != _optionObj.getField( "useShell" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "useShell should be a number" ) ;
            goto error ;
         }
         useShell = _optionObj.getIntField( "useShell" ) ;
      }

      rc = _cmdCommon.start( command, ev, useShell, timeout, err, pid, strOut ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, err.c_str() ) ;
         goto error ;
      }
      builder.append( "pid", pid ) ;
      builder.append( "strOut", strOut ) ;

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteCmdRunJS implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdRunJS )

    _remoteCmdRunJS::_remoteCmdRunJS()
   {
   }

   _remoteCmdRunJS::~_remoteCmdRunJS()
   {
   }

   const CHAR* _remoteCmdRunJS::name()
   {
      return OMA_REMOTE_CMD_RUN_JS ;
   }

   INT32 _remoteCmdRunJS::init ( const CHAR *pInfomation )
   {
      INT32 rc = SDB_OK ;
      string errmsg ;
      BSONObjBuilder bob ;
      BSONObj rval ;
      BSONObj detail ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // get js code
      if ( FALSE == _valueObj.hasField( "code" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "code must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "code" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "code must be string" ) ;
         goto error ;
      }
      _code = _valueObj.getStringField( "code" ) ;

      // get scope
      _jsScope = sdbGetOMAgentMgr()->getScopeBySession() ;
      if ( !_jsScope )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG ( PDERROR, "Failed to get scope, rc = %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteCmdRunJS::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string errmsg ;
      const sptResultVal *pRval = NULL ;
      BSONObjBuilder builder ;
      BSONObj rval ;

      // run js code
      rc = _jsScope->eval( _code.c_str(), _code.size(),
                           "", 1, SPT_EVAL_FLAG_NONE, &pRval ) ;
      if ( rc )
      {
         errmsg = _jsScope->getLastErrMsg() ;
         rc = _jsScope->getLastError() ;
         PD_LOG_MSG ( PDERROR, "%s", errmsg.c_str() ) ;
         goto error ;
      }

      // set result
      rval = pRval->toBSON() ;
      rc = final ( rval, retObj ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to extract result for command[%s], "
                      "rc = %d", name(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteCmdRunJS::final( BSONObj &rval, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      string strOut ;

      PD_LOG ( PDDEBUG, "Js return raw result for command[%s]: %s",
               name(), rval.toString(FALSE, TRUE).c_str() ) ;

      if ( FALSE == rval.hasField( "" ) )
      {
         strOut = "" ;
      }
      else
      {
         BSONElement ele = rval.getField( "" ) ;

         if ( String == ele.type() )
         {
            strOut = rval.getStringField( "" );
         }
         else
         {
            strOut = ele.toString( FALSE, TRUE ) ;
         }
      }
      bob.append( "strOut", strOut ) ;
      retObj = bob.obj() ;

      return rc ;
   }

}
