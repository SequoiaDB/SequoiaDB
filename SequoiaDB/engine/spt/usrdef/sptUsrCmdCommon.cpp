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

   Source File Name = sptUsrCmdCommon.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/24/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossCmdRunner.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "pd.hpp"
#include "sptUsrCmdCommon.hpp"
#include <string>

namespace engine
{
   #define SPT_USER_CMD_ONCE_SLEEP_TIME            ( 2 )

   _sptUsrCmdCommon::_sptUsrCmdCommon()
   {
      _retCode    = 0 ;
   }

   _sptUsrCmdCommon::~_sptUsrCmdCommon()
   {
   }

   INT32 _sptUsrCmdCommon::exec( const string& command, const string& env,
                                const INT32& timeout, const INT32& useShell,
                                std::string &err, std::string &retStr )
   {
      INT32 rc = SDB_OK ;
      ossCmdRunner runner ;

      _command.clear() ;

      _command = command ;
      utilStrTrim( _command ) ;

      if( !env.empty() )
      {
         _command += " " ;
         _command += env ;
      }

      _strOut = "" ;
      _retCode = 0 ;
      rc = runner.exec( _command.c_str(), _retCode, FALSE,
                        0 == timeout ? -1 : (INT64)timeout,
                        FALSE, NULL, useShell ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << _command << "] failed" ;
         err = ss.str() ;
         goto error ;
      }
      else
      {
         rc = runner.read( _strOut ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "read run command[" << _command << "] result failed" ;
            err = ss.str() ;
            goto error ;
         }
         else if ( SDB_OK != _retCode )
         {
            err = _strOut ;
            rc = _retCode ;
            goto error ;
         }
         retStr = _strOut ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrCmdCommon::start( const string& command, const string& env,
                                 const INT32& useShell, const INT32& timeout,
                                 string &err, INT32 &pid, string &retStr )
   {
      INT32 rc = SDB_OK ;
      ossCmdRunner runner ;
      OSSHANDLE processHandle = (OSSHANDLE)0 ;

#if defined( _LINUX )
      struct sigaction    ignore ;
      struct sigaction    savechild ;
      sigset_t            childmask ;
      sigset_t            savemask ;
      BOOLEAN             restoreSigMask         = FALSE ;
      BOOLEAN             restoreSIGCHLDHandling = FALSE ;
#endif // _LINUX

      _command.clear() ;
      _command = command ;
      utilStrTrim( _command ) ;

      if ( !env.empty() )
      {
         _command += " " ;
         _command += env ;
         utilStrTrim( _command ) ;
      }

#if defined( _LINUX )
      sigemptyset ( &childmask ) ;
      sigaddset ( &childmask, SIGCHLD ) ;
      rc = pthread_sigmask( SIG_BLOCK, &childmask, &savemask ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to block sigchld, err=%d", rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      restoreSigMask = TRUE ;

      ignore.sa_handler = SIG_DFL ;
      sigemptyset ( &ignore.sa_mask ) ;
      ignore.sa_flags = 0 ;
      rc = sigaction ( SIGCHLD, &ignore, &savechild ) ;
      if ( rc < 0 )
      {
         PD_LOG ( PDERROR, "Failed to run sigaction, err = %d", rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      restoreSIGCHLDHandling = TRUE ;
#endif //_LINUX

      _strOut = "" ;
      _retCode = 0 ;
      rc = runner.exec( _command.c_str(), _retCode, TRUE, -1, FALSE,
                        timeout > 0 ? &processHandle : NULL,
                        useShell ? TRUE : FALSE,
                        timeout > 0 ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << _command << "] failed" ;
         err = ss.str() ;
         goto error ;
      }
      else
      {
         OSSPID childPid = runner.getPID() ;
         if ( 0 != processHandle )
         {
            INT64 times = 0 ;
            while ( times < timeout )
            {
               if ( ossIsProcessRunning( childPid ) )
               {
                  ossSleep( SPT_USER_CMD_ONCE_SLEEP_TIME ) ;
                  times += SPT_USER_CMD_ONCE_SLEEP_TIME ;
               }
               else
               {
                  rc = ossGetExitCodeProcess( processHandle, _retCode ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "get exit code from process[ " << childPid
                        << " ] failed" ;
                     err = ss.str() ;
                     goto error ;
                  }
                  rc = runner.read( _strOut ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "read run command[" << _command
                        << "] result failed" ;
                     err = ss.str() ;
                     goto error ;
                  }

                  if ( SDB_OK != _retCode )
                  {
                     err = _strOut ;
                     rc = _retCode ;
                     goto error ;
                  }
                  break ;
               }
            }
         }
         retStr = _strOut ;
         pid = childPid ;
      }

   done:
      if ( 0 != processHandle )
      {
         ossCloseProcessHandle( processHandle ) ;
      }
#if defined( _LINUX )
      if ( restoreSIGCHLDHandling )
      {
         sigaction ( SIGCHLD, &savechild, NULL ) ;
      }
      if ( restoreSigMask )
      {
         pthread_sigmask ( SIG_SETMASK, &savemask, NULL ) ;
      }
#endif // _LINUX
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrCmdCommon::getLastRet( string &err, UINT32 &lastRet )
   {
      lastRet = _retCode ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmdCommon::getLastOut( string &err, string &lastOut )
   {
      lastOut = _strOut ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmdCommon::getCommand( string &err, string &command )
   {
      command = _command ;
      return SDB_OK ;
   }
}
