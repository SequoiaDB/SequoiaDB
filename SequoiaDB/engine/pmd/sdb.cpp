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

   Source File Name = sdb.cpp

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

#include "sdbOptionMgr.hpp"
#include "ossUtil.h"
#include "ossProc.hpp"
#include "ossNPipe.hpp"
#include "ossMem.h"
#include "ossMem.hpp"
#include "utilLinenoiseWrapper.hpp"
#include "oss.h"
#include "pd.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossTypes.h"
#include "ossVer.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "../spt/sptHelp.hpp"
#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/bind.hpp>
#include "sptCommon.hpp"
#include "sptSPScope.hpp"
#include "utilPath.hpp"
#include "utilPipe.hpp"
#include "sptContainer.hpp"
#include "ossSignal.hpp"
#include "ossIO.hpp"
#include <locale.h>
using namespace bson ;

using std::ostream ;
using std::vector ;
using std::string ;
using std::bad_alloc ;
using namespace engine ;

#define ELSE_STATEMENT     "else{}"

namespace po = boost::program_options ;
po::options_description display ( "Command options" ) ;
po::positional_options_description destd ;
po::variables_map vm ;

#if !defined (SDB_SHELL)
#error "sdbbp should always have SDB_SHELL defined"
#endif

#if defined (_WINDOWS)
   #define SDB_PB_PROGRAM_NAME         "sdbbp.exe"
#else
   #define SDB_PB_PROGRAM_NAME         "sdbbp"
#endif // _WINDOWS

#define SPT_LANG_EN                    "en"
#define SPT_LANG_CN                    "cn"

enum RunMode
{
   INTERACTIVE_MODE,
   BATCH_MODE,
   FRONTEND_MODE,
} ;

struct ArgInfo
{
   RunMode mode ;
   string       program ; // the program name
   string       filename ; // available in batch mode
   string       cmd ; // available in front-end mode
   string       variable ; // variable
   string       language ; // language, can be "en" or "cn"
} ;

// PD_TRACE_DECLARE_FUNCTION ( SDB_READFILE, "readFile" )
INT32 readFile ( const CHAR * name , CHAR ** buf , UINT32 * bufSize,
                 UINT32 * readSize )
{
   PD_TRACE_ENTRY ( SDB_READFILE );
   ossPrimitiveFileOp op ;
   ossPrimitiveFileOp::offsetType offset ;
   INT32 rc = SDB_OK ;

   SDB_ASSERT ( name && buf && bufSize, "Invalid arguments" ) ;

   rc = op.Open ( name , OSS_PRIMITIVE_FILE_OP_READ_WRITE ) ;
   if ( rc != SDB_OK )
   {
      ossPrintf ( "Can't open file: %s"OSS_NEWLINE, name ) ;
      goto error ;
   }

   rc = op.getSize ( &offset ) ;
   if ( rc != SDB_OK )
   {
      goto error ;
   }

   if ( *bufSize < offset.offset + 1 )
   {
      if ( *buf )
      {
         SDB_OSS_FREE( *buf ) ;
         *buf = NULL ;
         *bufSize = 0 ;
      }
      *buf = (CHAR *) SDB_OSS_MALLOC ( offset.offset + 1 ) ;
      if ( ! *buf )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR , "fail to alloc memory" ) ;
         goto error ;
      }
      *bufSize = offset.offset + 1 ;
   }

   rc = op.Read ( offset.offset , *buf , NULL ) ;
   if ( rc != SDB_OK )
   {
      goto error ;
   }
   (*buf)[ offset.offset ] = 0 ;
   if ( readSize ) *readSize = offset.offset ;

done :
   op.Close() ;
   PD_TRACE_EXITRC ( SDB_READFILE, rc );
   return rc ;
error :
   goto done ;
}

void printUsage()
{
   ossPrintf ( "Usage:"OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb (Interactive mode)"OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb -f <FILE> (Batch mode), eg: ./sdb -e \"var v = \'123\'\" -f example.js"OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb -s <CMD> (Front end mode), eg: ./sdb -s \"var db = new Sdb(\'localhost\', 11810)\""OSS_NEWLINE ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_PARSEARGUMENTS, "parseArguments" )
INT32 parseArguments ( int argc , CHAR ** argv , ArgInfo & argInfo )
{
   INT32 rc          = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_PARSEARGUMENTS );
   argInfo.mode      = INTERACTIVE_MODE ;
   argInfo.filename  = "" ;
   argInfo.variable  = "" ;
   argInfo.cmd       = "" ;
   argInfo.program   = "" ;

   SDB_POSITIONAL_OPTIONS_DESCRIPTION

   SDB_ADD_PARAM_OPTIONS_BEGIN ( display )
      SDB_COMMANDS_OPTIONS
   SDB_ADD_PARAM_OPTIONS_END

   try
   {
      po::store ( po::command_line_parser ( argc , argv ).options(
                  display ).positional ( destd ).run ()  , vm ) ;
      po::notify ( vm );
   }
   catch ( po::unknown_option &e )
   {
      std::cerr << "Unknown argument: "
                << e.get_option_name() << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      std::cerr << "Invalid argument: "
                << e.get_option_name() << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::error &e )
   {
      std::cerr << e.what() << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( vm.count( "help" ) )
   {
      std::cout << display << std::endl ;
      rc = SDB_SDB_HELP_ONLY ;
      goto done ;
   }
   if ( vm.count( "version" ) )
   {
      ossPrintVersion( "SequoiaDB shell version" ) ;
      rc = SDB_SDB_VERSION_ONLY ;
      goto done ;
   }
   if ( vm.count( "eval" ) )
   {
      argInfo.variable = vm["eval"].as<string>() ;
   }

   SDB_ASSERT ( argv , "invalid argument" ) ;
   SDB_ASSERT ( argc >= 1 , "argc must be >= 1" ) ;

   argInfo.program = (string)( argv[0] ) ;
   argInfo.language = SPT_LANG_EN ;
   if ( vm.count( "language" ) )
   {
      string l = vm["language"].as<string>() ;
      argInfo.language = (l == SPT_LANG_EN || l == SPT_LANG_CN) ? l : SPT_LANG_EN ;
      argc -= 2 ;
   }
   if ( 1 == argc )
   {
   }
   else if ( vm.count( "shell" ) )
   {
      argInfo.mode = FRONTEND_MODE ;
      argInfo.cmd = vm["shell"].as<string>() ;
   }
   else if ( vm.count( "file" ) )
   {
      argInfo.mode = BATCH_MODE ;
      argInfo.filename = vm["file"].as<string>() ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      printUsage() ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_PARSEARGUMENTS, rc );
   return rc ;
error :
   goto done ;
}

BOOLEAN sdbdefCanContinueGetNextLine( sptScope *scope, const CHAR *str )
{
   BOOLEAN ret = FALSE ;
   if( SPT_SCOPE_TYPE_SP == scope->getType() )
   {
      sptSPScope* spScope = dynamic_cast< sptSPScope* >( scope ) ;
      JSContext *context = spScope->getContext() ;
      JSObject *global = spScope->getGlobalObj() ;
      if( NULL == spScope )
      {
         goto error ;
      }
      if( !JS_BufferIsCompilableUnit( context, global, str, ossStrlen( str ) ) )
      {
         ret = TRUE ;
      }
      /* If stdin buffer is not emptry, it means
       *    that user uses the copy function.
       */
      else if( !isStdinEmpty() )
      {
         string content = ELSE_STATEMENT ;
         JSExceptionState *exnState = JS_SaveExceptionState( context ) ;
         JSErrorReporter older = JS_SetErrorReporter( context, NULL ) ;
         content = str + content ;
         if( JS_CompileScript( context, global, content.c_str(), content.size(),
                               NULL, 0 ) )
         {
            ret = TRUE ;
         }
         else
         {
            ret = FALSE ;
         }
         JS_SetErrorReporter( context, older ) ;
         JS_RestoreExceptionState( context, exnState ) ;
      }
   }
done:
   return ret ;
error:
   ret = FALSE ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_ENTERBATCHMODE, "enterBatchMode" )
INT32 enterBatchMode( sptScope * scope , const CHAR *filename,
                      const CHAR *variable )
{
   INT32    rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_ENTERBATCHMODE );
   UINT32   varLen   = 0 ;
   CHAR *   toker    = NULL ;
   CHAR *   last     = NULL ;
   string   content ;

   SDB_ASSERT ( scope , "invalid argument" ) ;
   SDB_ASSERT ( filename && filename[0] != '\0' , "invalid arguement" ) ;

   if ( variable && variable[0] != '\0' )
   {
      varLen = ossStrlen ( variable ) ;
      content = variable ;
   }

   CHAR *fileNameTmp = ossStrdup( filename ) ;
   if ( !fileNameTmp )
   {
      rc = SDB_OOM ;
      PD_LOG( PDERROR, "Failed to dup string: %s", filename ) ;
      goto error ;
   }

   toker = ossStrtok( fileNameTmp, ",;", &last ) ;
   while( toker )
   {
      while ( ' ' == *toker )
      {
         ++toker ;
      }
      CHAR *lastPos = *toker ? toker + ossStrlen( toker ) : NULL ;
      while ( lastPos && ' ' == *( lastPos - 1 ) )
      {
         --lastPos ;
      }
      if ( lastPos && ' ' == *lastPos )
      {
         *lastPos = 0 ;
      }
      if( !content.empty() )
      {
         content += OSS_NEWLINE ;
      }
      content += "import( '" + string( toker ) + "' ) ;" ;
      toker = ossStrtok( last, ",;", &last ) ;
   }
   if ( content.length() > varLen )
   {
      rc = scope->eval ( content.c_str() , (UINT32)content.length() ,
                         "shell" , 1, SPT_EVAL_FLAG_PRINT, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else
   {
      ossPrintf( "File %s is empty."OSS_NEWLINE , filename ) ;
   }

done :
   SAFE_OSS_FREE ( fileNameTmp ) ;
   PD_TRACE_EXITRC ( SDB_ENTERBATCHMODE, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_ENTERINTATVMODE, "enterInteractiveMode" )
INT32 enterInteractiveMode ( sptScope *scope, const CHAR *lang )
{
   INT32    rc          = SDB_OK ;
   CHAR *   code        = NULL ;
   INT64 sec            = 0 ;
   INT64 microSec       = 0 ;
   INT64 tkTime         = 0 ;
   ossTimestamp tmBegin ;
   ossTimestamp tmEnd ;
   string history ;

   SDB_ASSERT ( scope , "invalid argument" ) ;
   PD_TRACE_ENTRY ( SDB_ENTERINTATVMODE );

   sptHelp::setLanguage( lang ) ;

   setCanContinueNextLineCallback( boost::bind( sdbdefCanContinueGetNextLine,
                                                scope, _1 ) ) ;

   historyInit () ;
   linenoiseHistoryLoad( historyFile.c_str() ) ;
   g_lnBuilder.loadCmd( historyFile.c_str() ) ;

   ossPrintf ( "Welcome to SequoiaDB shell!"OSS_NEWLINE ) ;
   ossPrintf ( "help() for help, Ctrl+c or quit to exit"OSS_NEWLINE ) ;

   while ( TRUE )
   {
      if ( ! getNextCommand ( "> ", &code ) )
         break ;

      if ( !code || '\0' == code[0] )
         goto loop_next ;

      history = string( code ) ;
      if ( ossStrcmp ( CMD_QUIT, code ) == 0 ||
                ossStrcmp ( CMD_QUIT1,
                boost::algorithm::erase_all_copy( string(code), " " ).c_str() ) == 0 )
      {
         linenoiseHistorySave( historyFile.c_str() ) ;
         break ;
      }
      else if ( ossStrcmp( CMD_CLEAR, code ) == 0 ||
                ossStrcmp( CMD_CLEAR1,
                boost::algorithm::erase_all_copy( string(code), " " ).c_str() ) == 0 )
      {
         linenoiseClearScreen() ;
         goto loop_next ;
      }
      else if ( ossStrcmp ( CMD_CLEARHISTORY,
           boost::algorithm::erase_all_copy( history, " " ).c_str() ) == 0 ||
           ossStrcmp ( CMD_CLEARHISTORY1,
           boost::algorithm::erase_all_copy( history, " " ).c_str() ) == 0 )
      {
         historyClear() ;
         goto loop_next ;
      }

      rc = SDB_OK ;
      ossGetCurrentTime ( tmBegin ) ;
      rc = scope->eval ( code , history.size(),
                         "(shell)" , 1, SPT_EVAL_FLAG_PRINT,
                         NULL ) ;
      ossGetCurrentTime ( tmEnd ) ;

      tkTime = ( tmEnd.time * 1000000 + tmEnd.microtm ) -
               ( tmBegin.time * 1000000 + tmBegin.microtm ) ;
      sec = tkTime/1000000 ;
      microSec = tkTime%1000000 ;
      ossPrintf ( "Takes %lld.%06llds."OSS_NEWLINE , sec, microSec ) ;

   loop_next :
         SAFE_OSS_FREE ( code ) ;
   }

   SAFE_OSS_FREE ( code ) ;
   PD_TRACE_EXITRC ( SDB_ENTERINTATVMODE, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_FORMATARGS, "formatArgs" )
INT32 formatArgs ( const CHAR * program ,
                   const OSSPID & ppid ,
                   CHAR ** args )
{
   SDB_ASSERT ( program && program[0] != '\0' , "invalid argument" ) ;
   SDB_ASSERT ( args , "invalid argument" ) ;
   PD_TRACE_ENTRY ( SDB_FORMATARGS );

   INT32 rc          = SDB_OK ;
   INT32 progLen     = ossStrlen ( program ) ;
   INT32 ppidLen     = 0 ;
   INT32 argSize     = 0 ;
   CHAR *p           = NULL ;
   CHAR buf[128] ;
   INT32 bufSize     = sizeof ( buf ) / sizeof ( CHAR ) ;

   ossSnprintf ( buf , bufSize , "%u" , ppid ) ;

   ppidLen = ossStrlen ( buf ) ;
   argSize = progLen + 1 + ppidLen + 2 ;

   *args = (CHAR*) SDB_OSS_MALLOC ( argSize ) ;
   if ( NULL == *args )
   {
      rc = SDB_OOM ;
      SH_VERIFY_RC
   }

   p = *args ;
   ossStrcpy ( p , program ) ;
   p += progLen + 1 ;

   ossStrcpy( p , buf ) ;
   p += ppidLen + 1 ;
   *p = '\0' ;

done :
   PD_TRACE_EXITRC ( SDB_FORMATARGS, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_CREATEDAEMONPROC, "createDaemonProcess" )
INT32 createDaemonProcess ( const CHAR * program , const OSSPID & ppid ,
                            CHAR * f2dbuf , CHAR * d2fbuf )
{
   CHAR *         args     = NULL ;
   INT32          rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_CREATEDAEMONPROC );
   OSSPID         pid ;
   ossResultCode  result ;
   OSSNPIPE       waitPipe ;
   CHAR           waitName[128] ;
   CHAR           f2dName[128] ;
   CHAR           d2fName[128] ;

   ossMemset ( &waitPipe , 0 , sizeof ( waitPipe ) ) ;
   ossMemset ( waitName , 0 , sizeof ( waitName ) ) ;
   ossMemset ( f2dName , 0 , sizeof ( f2dName ) ) ;
   ossMemset ( d2fName , 0 , sizeof ( d2fName ) ) ;

   SDB_ASSERT ( program && program[0] != '\0' , "Invalid argument" ) ;

   rc = ossAccess( program ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "The program[%s] is not exist, rc: %d",
              program, rc ) ;
      goto error ;
   }

   rc = getWaitPipeName ( ppid ,  waitName , sizeof ( waitName ) ) ;
   SH_VERIFY_RC

   clearDirtyShellPipe( SDB_SHELL_WAIT_PIPE_PREFIX ) ;

   rc = ossCreateNamedPipe ( waitName , 0 , 0 , OSS_NPIPE_INBOUND ,
                             1 , 0 , waitPipe ) ;
   SH_VERIFY_RC

   rc = formatArgs ( program , ppid , &args ) ;
   SH_VERIFY_RC

   rc = ossExec ( program , args , NULL , OSS_EXEC_NODETACHED , pid ,
                  result , NULL , NULL ) ;
   SH_VERIFY_RC

   rc = getPipeNames2 ( ppid , pid , f2dName , sizeof ( f2dName ) ,
                                     d2fName , sizeof ( d2fName ) ) ;
   SH_VERIFY_RC

   ossStrcpy ( f2dbuf , f2dName ) ;
   ossStrcpy ( d2fbuf , d2fName ) ;

   rc = ossConnectNamedPipe ( waitPipe , OSS_NPIPE_INBOUND ) ;
   SH_VERIFY_RC

   rc = ossDisconnectNamedPipe ( waitPipe ) ;
   SH_VERIFY_RC

done :
   ossDeleteNamedPipe ( waitPipe ) ;
   SAFE_OSS_FREE ( args ) ;
   PD_TRACE_EXITRC ( SDB_CREATEDAEMONPROC, rc );
   return rc ;
error :
   goto done ;
}

#define SDB_FRONTEND_RECEIVEBUFFERSIZE 128
// PD_TRACE_DECLARE_FUNCTION ( SDB_ENTERFRONTENDMODE, "enterFrontEndMode" )
INT32 enterFrontEndMode ( const CHAR * program , const CHAR * cmd )
{
   CHAR     c           = '\0' ;
   INT32    rc          = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_ENTERFRONTENDMODE );
   OSSPID   ppid        = OSS_INVALID_PID ;
   OSSNPIPE f2dPipe ;
   OSSNPIPE d2fPipe ;
   CHAR     f2dName[128]          = {0} ;
   CHAR     d2fName[128]          = {0} ;
   CHAR     bpf2dName[128]        = {0} ;
   CHAR     bpd2fName[128]        = {0} ;
   CHAR     receiveBuffer1[SDB_FRONTEND_RECEIVEBUFFERSIZE]   = {0} ;
   CHAR     receiveBuffer2[SDB_FRONTEND_RECEIVEBUFFERSIZE]   = {0} ;
   CHAR     receiveBufferFinal[2*SDB_FRONTEND_RECEIVEBUFFERSIZE] = {0} ;
   CHAR    *pCurrentReceivePtr    = receiveBuffer1 ;
   CHAR *   p           = NULL ;
   INT32    id          = 0 ;
   INT32    retCode     = SDB_OK ;
   CHAR     pbFullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   ossMemset ( &f2dPipe , 0 , sizeof ( f2dPipe ) ) ;
   ossMemset ( &d2fPipe , 0 , sizeof ( d2fPipe ) ) ;

   SDB_ASSERT ( program && program[0] != '\0' , "invalid argument" ) ;
   if ( !cmd || cmd[0] == '\0' )
   {
      goto done ;
   }

   rc = ossGetEWD( pbFullPath, OSS_MAX_PATHSIZE ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Get current path failed, rc: %d", rc ) ;
      goto error ;
   }
   else
   {
      UINT32 strLen = ossStrlen( pbFullPath ) ;
      if ( strLen + ossStrlen( SDB_PB_PROGRAM_NAME ) + 2 > OSS_MAX_PATHSIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Path[%s] is to long", pbFullPath ) ;
         goto error ;
      }
      else if ( strLen > 0 && pbFullPath[strLen-1] != OSS_FILE_SEP_CHAR )
      {
         ossStrncat( pbFullPath, OSS_FILE_SEP, 1 ) ;
      }
      ossStrncat( pbFullPath, SDB_PB_PROGRAM_NAME,
                  ossStrlen( SDB_PB_PROGRAM_NAME ) ) ;
   }

   clearDirtyShellPipe( SDB_SHELL_F2B_PIPE_PREFIX ) ;
   clearDirtyShellPipe( SDB_SHELL_B2F_PIPE_PREFIX ) ;

   ppid = ossGetParentProcessID () ;
   SH_VERIFY_COND ( ppid != OSS_INVALID_PID , SDB_SYS ) ;

   rc = getPipeNames ( ppid , f2dName , sizeof ( f2dName ) ,
                       d2fName , sizeof ( d2fName ) ) ;
   SH_VERIFY_RC

   rc = getPipeNames1 ( bpf2dName , sizeof ( bpf2dName ) ,
                        bpd2fName , sizeof ( bpd2fName ) ,
                        f2dName , d2fName ) ;

   if ( rc == SDB_OK )
   {
      p = ossStrrchr ( bpf2dName , 45 ) ;
      p = p + 1 ;
      while ( *p != '\0' )
      {
         id = id * 10 ;
         id += ( INT32 )( *p - '0' ) ;
         p++ ;
      }
      if ( ossIsProcessRunning  ( (OSSPID)id ) )
      {
         rc = ossOpenNamedPipe ( bpf2dName , OSS_NPIPE_OUTBOUND , 0 , f2dPipe ) ;
         SH_VERIFY_RC
      }
      else
      {
         rc = ossCleanNamedPipeByName ( bpf2dName ) ;
         SH_VERIFY_RC
         rc = ossCleanNamedPipeByName ( bpd2fName ) ;
         SH_VERIFY_RC

         rc = createDaemonProcess ( pbFullPath, ppid, bpf2dName, bpd2fName ) ;
         SH_VERIFY_RC

         rc = ossOpenNamedPipe ( bpf2dName , OSS_NPIPE_OUTBOUND , 0 , f2dPipe ) ;
         SH_VERIFY_RC
      }
   }
   else if ( rc == SDB_FNE )
   {

      rc = createDaemonProcess ( pbFullPath, ppid, bpf2dName, bpd2fName ) ;
      SH_VERIFY_RC

      rc = ossOpenNamedPipe ( bpf2dName , OSS_NPIPE_OUTBOUND , 0 , f2dPipe ) ;
      SH_VERIFY_RC
   }
   else
   {
      SH_VERIFY_RC
   }

   rc = ossWriteNamedPipe ( f2dPipe , cmd , ossStrlen ( cmd ) , NULL ) ;
   SH_VERIFY_RC

   rc = ossCloseNamedPipe ( f2dPipe ) ;
   SH_VERIFY_RC

   rc = ossOpenNamedPipe ( bpd2fName , OSS_NPIPE_INBOUND ,
                              OSS_NPIPE_INFINITE_TIMEOUT , d2fPipe ) ;
   SH_VERIFY_RC

   ossMemset ( receiveBuffer1, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
   ossMemset ( receiveBuffer2, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
   while ( TRUE )
   {
      rc = ossReadNamedPipe ( d2fPipe , &c , 1 , NULL ) ;
      if ( rc )
         break ;
      if ( ( pCurrentReceivePtr - &receiveBuffer1[0] <
             SDB_FRONTEND_RECEIVEBUFFERSIZE-1 &&
             pCurrentReceivePtr >= &receiveBuffer1[0] ) ||
           ( pCurrentReceivePtr - &receiveBuffer2[0] <
             SDB_FRONTEND_RECEIVEBUFFERSIZE-1 &&
             pCurrentReceivePtr >= &receiveBuffer2[0] ) )
      {
         *pCurrentReceivePtr = c ;
         ++pCurrentReceivePtr ;
      }
      else if ( pCurrentReceivePtr - &receiveBuffer1[0] ==
                SDB_FRONTEND_RECEIVEBUFFERSIZE-1 )
      {
         receiveBuffer2[SDB_FRONTEND_RECEIVEBUFFERSIZE-1] = '\0' ;
         ossPrintf ( "%s", receiveBuffer2 ) ;
         ossMemset ( receiveBuffer2, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
         pCurrentReceivePtr = &receiveBuffer2[0] ;
         *pCurrentReceivePtr = c ;
         ++pCurrentReceivePtr ;
      }
      else if ( pCurrentReceivePtr - &receiveBuffer2[0] ==
                SDB_FRONTEND_RECEIVEBUFFERSIZE-1 )
      {
         receiveBuffer1[SDB_FRONTEND_RECEIVEBUFFERSIZE-1] = '\0' ;
         ossPrintf ( "%s", receiveBuffer1 ) ;
         ossMemset ( receiveBuffer1, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
         pCurrentReceivePtr = &receiveBuffer1[0] ;
         *pCurrentReceivePtr = c ;
         ++pCurrentReceivePtr ;
      }
      else
      {
         ossPrintf ( "SEVERE Error, we should never hit here"OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }
   if ( pCurrentReceivePtr - &receiveBuffer1[0] <=
        SDB_FRONTEND_RECEIVEBUFFERSIZE-1 &&
        pCurrentReceivePtr >= &receiveBuffer1[0] )
   {
      ossStrncpy ( receiveBufferFinal, receiveBuffer2,
                   SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
      ossStrncat ( receiveBufferFinal, receiveBuffer1,
                   SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
   }
   else if ( pCurrentReceivePtr - &receiveBuffer2[0] <=
             SDB_FRONTEND_RECEIVEBUFFERSIZE-1 &&
             pCurrentReceivePtr >= &receiveBuffer2[0] )
   {
      ossStrncpy ( receiveBufferFinal, receiveBuffer1,
                   SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
      ossStrncat ( receiveBufferFinal, receiveBuffer2,
                   SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
   }
   pCurrentReceivePtr = &receiveBufferFinal[ossStrlen(receiveBufferFinal)] ;
   while ( pCurrentReceivePtr != &receiveBufferFinal[0] &&
           *pCurrentReceivePtr != ' ' )
   {
      --pCurrentReceivePtr ;
   }
   if ( *pCurrentReceivePtr == ' ' )
   {
      retCode = ossAtoi ( pCurrentReceivePtr+1 ) ;
   }
   *pCurrentReceivePtr = '\0' ;
   ossPrintf ( "%s" , receiveBufferFinal ) ;
   SH_VERIFY_COND ( SDB_OK == rc || SDB_EOF == rc , rc )

   rc = ossCloseNamedPipe( d2fPipe ) ;
   SH_VERIFY_RC

done :
   if ( SDB_OK != retCode )
   {
      rc = retCode ;
   }
   PD_TRACE_EXITRC ( SDB_ENTERFRONTENDMODE, rc );
   return rc ;
error :
   ossCloseNamedPipe ( f2dPipe ) ;
   ossCloseNamedPipe ( d2fPipe ) ;
   goto done ;
}

INT32 rc2ReturnCode( INT32 rc )
{
   INT32 retCode = SDB_RETURNCODE_OK ;
   switch ( rc )
   {
   case SDB_SYS :
      retCode = SDB_RETURNCODE_SYSTEM ;
      break ;
   case SDB_OK :
      retCode = SDB_RETURNCODE_OK ;
      break ;
   case SDB_DMS_EOC :
      retCode = sdbHasReadData() ? SDB_RETURNCODE_OK : SDB_RETURNCODE_EMPTY ;
      sdbSetReadData( FALSE ) ;
      break ;
   default :
      retCode = SDB_RETURNCODE_ERROR ;
   }

   return retCode ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_SDB_MAIN, "main" )
int main ( int argc , CHAR **argv )
{
   engine::sptContainer container ;
   engine::sptScope *scope = NULL ;
   INT32             rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_SDB_MAIN );
   ArgInfo           argInfo ;
   CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   setlocale(LC_CTYPE, "zh_CN.UTF-8");
#if defined( _LINUX )
   signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX
   linenoiseSetCompletionCallback( (linenoiseCompletionCallback*)lineComplete ) ;

   rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
   if ( rc )
   {
      std::cout << "Get current path failed: " << rc << std::endl ;
      goto error ;
   }

   rc = container.init() ;
   SH_VERIFY_RC

   scope = container.newScope() ;
   SH_VERIFY_COND ( scope , SDB_SYS ) ;

   rc = parseArguments ( argc , argv , argInfo ) ;
   if( SDB_SDB_HELP_ONLY == rc || SDB_SDB_VERSION_ONLY == rc )
   {
      rc = SDB_OK ;
      goto done ;
   }
   SH_VERIFY_RC

   switch ( argInfo.mode )
   {
   case INTERACTIVE_MODE :
      rc = enterInteractiveMode( scope, argInfo.language.c_str() ) ;
      break ;

   case BATCH_MODE :
      rc = enterBatchMode( scope , argInfo.filename.c_str() ,
                           argInfo.variable.c_str() ) ;
      break ;

   case FRONTEND_MODE :
      rc = enterFrontEndMode ( argInfo.program.c_str(),
                               argInfo.cmd.c_str() ) ;
      break ;

   default :
      rc = SDB_INVALIDARG ;
   }

done :
   if ( scope )
   {
      container.releaseScope( scope ) ;
   }
   container.fini() ;
   PD_TRACE_EXITRC ( SDB_SDB_MAIN, rc );

   return rc2ReturnCode( rc ) ;
error :
   goto done ;
}

