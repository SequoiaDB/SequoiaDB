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

   Source File Name = sdbbp.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pd.hpp"
#include "ossUtil.h"
#include "ossNPipe.hpp"
#include "ossMem.h"
#include "ossMem.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossProc.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <boost/thread/thread.hpp>
#include <string>
#include "sptCommon.hpp"
#include "utilPipe.hpp"
#include "sptContainer.hpp"
#include "ossSignal.hpp"

using std::string ;
using namespace engine ;

#define SDB_BP_LOG_FILE "sdbbp.log"

#if !defined (SDB_SHELL)
#error "sdbbp should always have SDB_SHELL defined"
#endif

// PD_TRACE_DECLARE_FUNCTION ( SDB_READFROMPIPE, "readFromPipe" )
static INT32 readFromPipe ( OSSNPIPE & npipe , CHAR ** output )
{
   CHAR     c   = 0 ;
   INT32    rc  = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_READFROMPIPE );

   try
   {
      string   buf = "" ;
      while ( TRUE )
      {
         rc = ossReadNamedPipe ( npipe , &c , 1 , NULL ) ;
         if ( SDB_OK == rc )
         {
            buf += c ;
         }
         else if ( SDB_EOF == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            goto error ;
         }
      }

      if ( ! output )
         goto done ;

      boost::algorithm::trim ( buf ) ;
      *output = (CHAR *) SDB_OSS_MALLOC ( buf.size() + 1 ) ;
      if ( ! *output )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossStrncpy ( *output , buf.c_str() , buf.size() + 1 ) ;
   }
   catch ( std::bad_alloc & )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_READFROMPIPE, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MONITOR_THREAD, "monitor_thread" )
void monitor_thread ( const OSSPID  shpid ,
                      const CHAR * f2bName ,
                      const CHAR * b2fName )
{
   PD_TRACE_ENTRY ( SDB_MONITOR_THREAD );
   while ( ossIsProcessRunning ( shpid ) )
   {
      ossSleep( OSS_ONE_SEC ) ;
   }

   ossCleanNamedPipeByName ( f2bName ) ;
   ossCleanNamedPipeByName ( b2fName ) ;
   PD_TRACE_EXIT ( SDB_MONITOR_THREAD );
   exit (0) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_CREATESHMONTHREAD, "createShellMonitorThread" )
INT32 createShellMonitorThread ( const OSSPID & shpid ,
                                 const CHAR * f2bName ,
                                 const CHAR * b2fName )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_CREATESHMONTHREAD );

   try
   {
      boost::thread monitor ( monitor_thread , shpid , f2bName , b2fName ) ;
      monitor.detach() ;
   }
   catch ( boost::thread_resource_error & e )
   {
      rc = SDB_SYS ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_CREATESHMONTHREAD, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_ENTERDAEMONMODE, "enterDaemonMode" )
INT32 enterDaemonMode ( sptScope *scope ,
                        const OSSPID & shpid ,
                        const CHAR * waitName ,
                        const CHAR * f2bName ,
                        const CHAR * b2fName )
{
   INT32          rc          = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_ENTERDAEMONMODE );
   CHAR *         code        = NULL ;
   BOOLEAN        exit        = FALSE ;
   INT32          fd          = -1 ;
   INT32          hOutFd      = -1 ;

   OSSNPIPE f2bPipe ;
   OSSNPIPE b2fPipe ;
   OSSNPIPE waitPipe ;

   ossMemset ( &f2bPipe , 0 , sizeof ( f2bPipe ) ) ;
   ossMemset ( &b2fPipe , 0 , sizeof ( b2fPipe ) ) ;
   ossMemset ( &waitPipe , 0 , sizeof ( waitPipe ) ) ;

   SDB_ASSERT ( scope , "invalid argument" ) ;
   SDB_ASSERT ( waitName && waitName[0] != '\0' , "invalid argument" ) ;
   SDB_ASSERT ( f2bName && f2bName[0] != '\0' , "invalid argument" ) ;
   SDB_ASSERT ( b2fName && b2fName[0] != '\0' , "invalid argument" ) ;

   rc = ossCreateNamedPipe( f2bName , 0 , 0 , OSS_NPIPE_INBOUND,
                            1 , 0 , f2bPipe ) ;
   SH_VERIFY_RC

   rc = ossCreateNamedPipe( b2fName , 0 , 0 , OSS_NPIPE_OUTBOUND ,
                            1 , 0 , b2fPipe ) ;
   SH_VERIFY_RC

   rc = ossOpenNamedPipe ( waitName , OSS_NPIPE_OUTBOUND , 0 , waitPipe ) ;
   SH_VERIFY_RC

   rc = ossCloseNamedPipe ( waitPipe ) ;
   SH_VERIFY_RC

   rc = createShellMonitorThread ( shpid , f2bName , b2fName ) ;
   SH_VERIFY_RC

   while ( TRUE )
   {
      rc = ossConnectNamedPipe ( f2bPipe , OSS_NPIPE_INBOUND ) ;
      SH_VERIFY_RC

      rc = readFromPipe ( f2bPipe , &code )  ;
      SH_VERIFY_RC

      rc = ossDisconnectNamedPipe ( f2bPipe ) ;
      SH_VERIFY_RC

      rc = ossConnectNamedPipe ( b2fPipe , OSS_NPIPE_OUTBOUND ) ;
      SH_VERIFY_RC

      rc = ossNamedPipeToFd ( b2fPipe , &fd ) ;
      SH_VERIFY_RC

      hOutFd = ossDup( 1 ) ;
      rc = ossDup2( fd, 1 ) ;
      SH_VERIFY_RC

      if ( ossStrcmp ( CMD_QUIT , code ) == 0 )
         exit = TRUE ;

      if ( ! exit )
         scope->eval( code, ossStrlen( code ), "(sdbbp)", 1,
                      SPT_EVAL_FLAG_PRINT, NULL ) ;
      SAFE_OSS_FREE ( code ) ;
      ossPrintf ( " %d", sdbGetErrno() ) ;

      ossCloseFd( 1 ) ;
      ossDup2( hOutFd, 1 ) ;
      ossCloseFd( hOutFd ) ;

      rc = ossDisconnectNamedPipe ( b2fPipe ) ;
      SH_VERIFY_RC

      if ( exit )
      {
         break ;
      }
   }

done :
   SAFE_OSS_FREE ( code ) ;
   ossDeleteNamedPipe ( f2bPipe ) ;
   ossDeleteNamedPipe ( b2fPipe ) ;
   PD_TRACE_EXITRC ( SDB_ENTERDAEMONMODE, rc );
   return rc ;
error :
   ossCloseNamedPipe ( waitPipe ) ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDBBP_MAIN, "main" )
int main ( int argc , const char * argv[] )
{
   PD_TRACE_ENTRY ( SDB_SDBBP_MAIN );
   INT32             rc       = SDB_OK ;
   OSSPID            shpid    = OSS_INVALID_PID ;
   OSSPID            bppid    = OSS_INVALID_PID ;
   CHAR waitName[128]         = { 0 } ;
   CHAR f2bName[128]          = { 0 } ;
   CHAR b2fName[128]          = { 0 } ;
   engine::sptContainer container ;
   engine::sptScope *scope = NULL ;

   if ( ! freopen ( SDB_BP_LOG_FILE , "a" , stdout ) )
   {
      rc = ossResetTty() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      ossPrintf( "warning: failed to freopen stdout to log "
                 "file[%s]"OSS_NEWLINE, SDB_BP_LOG_FILE ) ;
   }

#if defined( _LINUX )
   signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

   ossMemset ( waitName , 0 , sizeof ( waitName ) ) ;
   ossMemset ( f2bName , 0 , sizeof ( f2bName ) ) ;
   ossMemset ( b2fName , 0 , sizeof ( b2fName ) ) ;

   SH_VERIFY_COND ( argc >= 2 , SDB_INVALIDARG )

   shpid = (OSSPID) ossAtoi ( argv[1] ) ;
   bppid = ossGetCurrentProcessID() ;

   rc = getWaitPipeName ( shpid , waitName , sizeof ( waitName ) ) ;
   SH_VERIFY_RC

   rc = getPipeNames2 ( shpid , bppid , f2bName , sizeof ( f2bName ) ,
                                        b2fName , sizeof ( b2fName ) ) ;
   SH_VERIFY_RC

   rc = container.init() ;
   SH_VERIFY_RC

   scope = container.newScope() ;
   SH_VERIFY_COND ( scope , SDB_SYS ) ;

   rc = enterDaemonMode ( scope , shpid , waitName , f2bName , b2fName ) ;
   SH_VERIFY_RC

done :
   if ( scope )
   {
      container.releaseScope( scope ) ;
   }
   container.fini() ;
   PD_TRACE_EXITRC ( SDB_SDBBP_MAIN, rc );
   return rc ;
error :
   goto done ;
}

