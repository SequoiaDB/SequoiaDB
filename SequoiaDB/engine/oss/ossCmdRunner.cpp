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

   Source File Name = ossCmdRunner.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ossCmdRunner.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"

#define SPT_CMD_RUNNER_MAX_READ_BUF    ( 4 * 1024 * 1024 )
#define SPT_CMD_MONITOR_ONCE_TIME      ( 10 )   // ms

namespace engine
{

   /*
      _ossCmdRunner implement
   */
   _ossCmdRunner::_ossCmdRunner()
   {
      _id = OSS_INVALID_PID ;
      _hasRead = FALSE ;
      _readResult = SDB_OK ;
      _timeout = -1 ;
      _stop = TRUE ;
      _pThread = NULL ;
   }

   _ossCmdRunner::~_ossCmdRunner()
   {
      done() ; 
   }

   void _ossCmdRunner::handleInOutPipe( OSSPID pid,
                                        OSSNPIPE * const npHandleStdin,
                                        OSSNPIPE * const npHandleStdout )
   {
      _stop = FALSE ;
      try
      {
         _event.reset() ;
         _pThread = new boost::thread( &_ossCmdRunner::asyncRead, this ) ;

         if ( NULL == _pThread )
         {
            PD_LOG ( PDSEVERE, "Failed to create new thread: %s",
                     "Allocate memory failed" ) ;
            _event.signal() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDSEVERE, "Failed to create new thread: %s",
                  e.what() ) ;
         _event.signal() ;
      }

      try
      {
         _monitorEvent.reset() ;
         boost::thread thrdMonitor( &_ossCmdRunner::monitor, this ) ;
         thrdMonitor.detach() ;
      }
      catch( std::exception &e )
      {
         PD_LOG ( PDSEVERE, "Failed to create new thread: %s",
                  e.what() ) ;
         _monitorEvent.signal() ;
         return ;
      }
   }

   void _ossCmdRunner::asyncRead()
   {
      _readResult = _readOut( _outStr, TRUE ) ;
      _hasRead = TRUE ;
      _event.signal() ;
   }

   void _ossCmdRunner::monitor()
   {
      INT32 rc = SDB_OK ;
      INT64 timeout = _timeout >= 0 ? _timeout : 0x7FFFFFFF ;
      INT64 onceTime = 0 ;

      while( TRUE )
      {
         onceTime = timeout > SPT_CMD_MONITOR_ONCE_TIME ?
                    SPT_CMD_MONITOR_ONCE_TIME : timeout ;
         rc = _event.wait( onceTime ) ;
         if ( SDB_TIMEOUT != rc )
         {
            break ;
         }
         timeout -= onceTime ;
#ifdef _WINDOWS
         // The asyncRead thread may cause the process hangs in Windows system,
         // we terminate the thread if the start thread is finished
         if ( _stop )
         {
            TerminateThread( _pThread->native_handle(), 0 ) ;
            _hasRead = TRUE ;
            _event.signal() ;
            continue ;
         }
#endif //_WINDOWS
         if ( timeout <= 0 )
         {
            PD_LOG( PDWARNING, "Monitor timeout" ) ;
            break ;
         }
      }

      if ( rc ) // timeout
      {
         UINT32 i = 0 ;
         ossTerminateProcess( _id, FALSE ) ;
         while ( ossIsProcessRunning( _id ) )
         {
            ossSleep( 100 ) ;
            i += 100 ;

            if ( i > 2 * OSS_ONE_SEC )
            {
               ossTerminateProcess( _id, TRUE ) ;
               break ;
            }
         }

         _stop = TRUE ;
         _event.wait() ;
      }
      _monitorEvent.signalAll( rc ) ;
   }

   INT32 _ossCmdRunner::exec( const CHAR *cmd, UINT32 &exit,
                              BOOLEAN isBackground,
                              INT64 timeout,
                              BOOLEAN needResize,
                              OSSHANDLE *pHandle,
                              BOOLEAN addShellPrefix,
                              BOOLEAN dupOut )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != cmd, "can not be null" ) ;

      std::list < const CHAR * > argv ;
      CHAR *arguments = NULL ;
      INT32 argLen = 0 ;
      ossResultCode res ;
      INT32 flags = OSS_EXEC_NODETACHED ;

#if defined( _LINUX )
      std::vector<std::string> vecArgs ;
#endif // _LINUX

      _timeout = -1 ;
      if ( !isBackground )
      {
         flags |= OSS_EXEC_SSAVE ;
         _timeout = timeout ;
      }

      if ( !needResize )
      {
         flags |= OSS_EXEC_NORESIZEARGV ;
      }

      res.exitcode = 0 ;
      res.termcode = 0 ;

#if defined( _LINUX )
      if ( addShellPrefix )
      {
         argv.push_back( "/bin/sh" ) ;
         argv.push_back( "-c" ) ;
         argv.push_back( cmd ) ;
      }
      else
      {
         vecArgs = boost::program_options::split_unix( cmd ) ;
         for ( UINT32 i = 0 ; i < vecArgs.size() ; ++i )
         {
            argv.push_back( vecArgs[ i ].c_str() ) ;
         }
      }
#else
      argv.push_back( cmd ) ;
#endif // _LINUX

      rc = ossBuildArguments( &arguments, argLen, argv ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build arguments, rc: %d", rc ) ;
         goto error ;
      }

      done() ;

      _event.signal() ;
      _monitorEvent.signal() ;
      _hasRead = FALSE ;
      _readResult = SDB_OK ;
      _outStr = "" ;

      if ( !dupOut )
      {
         _hasRead = TRUE ;
      }

      rc = ossExec( arguments, arguments, NULL, flags,
                    _id, res, NULL, dupOut ? &_out : NULL,
                    dupOut ? this : NULL,
                    pHandle ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec cmd:%s, rc:%d",
                 cmd, rc ) ;
         goto error ;
      }

      if ( !isBackground )
      {
         _monitorEvent.wait( -1, &rc ) ;
         if ( rc ) // run timeout
         {
            exit = (UINT32)rc ;
            rc = SDB_OK ;
            _outStr += "***Error: run it timeout" ;
            goto done ;
         }
      }
      else
      {
         // background mode, the process is not quit, so, the pipe will
         // all the way in use, so can't wait here
      }

      exit = res.exitcode ;
   done:
      if ( NULL != arguments )
      {
         SDB_OSS_FREE( arguments ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _ossCmdRunner::read( string &out, BOOLEAN readEOF )
   {
      INT32 rc = SDB_OK ;

      if ( !_hasRead )
      {
         done() ;
      }

      if ( _hasRead )
      {
         out = _outStr ;
         _outStr = "" ;
         _hasRead = FALSE ;
         rc = _readResult ;
      }
      else
      {
         rc = _readOut( out, readEOF ) ;
      }
      return rc ;
   }

   INT32 _ossCmdRunner::_readOut( string & out, BOOLEAN readEOF )
   {
      INT32 rc = SDB_OK ;
      INT64 readLen = 0 ;
      CHAR buff[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      INT64 totalSize = out.length() ;
      BOOLEAN addAndSo = FALSE ;

      while ( FALSE == _stop )
      {
         rc = ossReadNamedPipe( _out, buff, OSS_MAX_PATHSIZE, &readLen, 1 ) ;
         if ( SDB_OK != rc )
         {
            if ( _stop )
            {
               rc = SDB_OK ;
            }
            else if ( SDB_TIMEOUT == rc )
            {
               continue ;
            }
            else if ( SDB_EOF != rc )
            {
               PD_LOG( PDERROR, "failed to read data from pipe:%d", rc ) ;
               goto error ;
            }
            else if ( readEOF )
            {
               rc = SDB_OK ;
            }
            break ;
         }
         buff[ readLen ] = 0 ;

         if ( totalSize < SPT_CMD_RUNNER_MAX_READ_BUF )
         {
            out += buff ;
         }
         else if ( !addAndSo )
         {
            addAndSo = TRUE ;
            out += "......" ;
         }
         totalSize += readLen ;
         readLen = 0 ;
         buff[ 0 ] = 0 ;

         if ( !readEOF )
         {
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _ossCmdRunner::done()
   {
      _stop = TRUE ;
      if ( OSS_INVALID_PID != _id )
      {
         _monitorEvent.wait() ;
         ossCloseNamedPipe( _out ) ;
         _id = OSS_INVALID_PID ;
      }
      if ( _pThread )
      {
         delete _pThread ;
         _pThread = NULL ;
      }
      return SDB_OK ;
   }
}

