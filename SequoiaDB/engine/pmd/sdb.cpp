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
#include "ossProc.hpp"
#include "utilLinenoiseWrapper.hpp"
#include "pd.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossVer.hpp"
#include "ossTimeZone.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "../spt/sptHelp.hpp"
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
#include <boost/thread/thread.hpp>
#if defined (_WINDOWS)
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

using namespace bson ;

using std::ostream ;
using std::vector ;
using std::string ;
using std::bad_alloc ;
using namespace engine ;

#define SDB_OPTION_HELP         "help"
#define SDB_OPTION_VERSION      "version"
#define SDB_OPTION_LANGUAGE     "language"
#define SDB_OPTION_FILE         "file"
#define SDB_OPTION_EVAL         "eval"
#define SDB_OPTION_SHELL        "shell"
#define ELSE_STATEMENT          "else{}"

#if defined (_WINDOWS)
   #define SDB_PB_PROGRAM_NAME  "sdbbp.exe"
#else
   #define SDB_PB_PROGRAM_NAME  "sdbbp"
#endif // _WINDOWS

#define SPT_LANG_EN             "en"
#define SPT_LANG_CN             "cn"

namespace po = boost::program_options ;
po::options_description display ( "Command options" ) ;
po::positional_options_description destd ;
po::variables_map vm ;

#if !defined (SDB_SHELL)
#error "sdbbp should always have SDB_SHELL defined"
#endif

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

void printUsage()
{
   ossPrintf ( "Usage:" OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb (Interactive mode)" OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb -f <FILE> (Batch mode), eg: ./sdb -e \"var v = \'123\'\" -f example.js" OSS_NEWLINE ) ;
   ossPrintf ( "   ./sdb -s <CMD> (Front end mode), eg: ./sdb -s \"var db = new Sdb(\'localhost\', 11810)\"" OSS_NEWLINE ) ;
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

   if ( vm.count( SDB_OPTION_HELP ) )
   {
      std::cout << display << std::endl ;
      rc = SDB_SDB_HELP_ONLY ;
      goto done ;
   }
   if ( vm.count( SDB_OPTION_VERSION ) )
   {
      ossPrintVersion( "SequoiaDB shell version" ) ;
      rc = SDB_SDB_VERSION_ONLY ;
      goto done ;
   }
   if ( vm.count( SDB_OPTION_EVAL ) )
   {
      argInfo.variable = vm[SDB_OPTION_EVAL].as<string>() ;
   }

   SDB_ASSERT ( argv , "Invalid argument" ) ;
   SDB_ASSERT ( argc >= 1 , "Argc must be >= 1" ) ;

   argInfo.program = (string)( argv[0] ) ;
   argInfo.language = SPT_LANG_EN ;
   if ( vm.count( SDB_OPTION_LANGUAGE ) )
   {
      string l = vm[SDB_OPTION_LANGUAGE].as<string>() ;
      argInfo.language = (l == SPT_LANG_EN || l == SPT_LANG_CN) ? l : SPT_LANG_EN ;
      argc -= 2 ;
   }
   if ( 1 == argc )
   {
      // Empty. Normal interactive mode
   }
   else if ( vm.count( SDB_OPTION_SHELL ) )
   {
      // Front-end mode
      argInfo.mode = FRONTEND_MODE ;
      argInfo.cmd = vm[SDB_OPTION_SHELL].as<string>() ;
   }
   else if ( vm.count( SDB_OPTION_FILE ) )
   {
      // Batch mode
      argInfo.mode = BATCH_MODE ;
      argInfo.filename = vm[SDB_OPTION_FILE].as<string>() ;
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
      // If input buffer is not a compiable unit,we need to get next line.
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
         // Save old exception state
         JSExceptionState *exnState = JS_SaveExceptionState( context ) ;
         // Set reporter is null to avold print error msg
         JSErrorReporter older = JS_SetErrorReporter( context, NULL ) ;
         // Concatenate the else statement at the end of the str
         content = str + content ;
         // Try to compile script
         if( JS_CompileScript( context, global, content.c_str(), content.size(),
                               NULL, 0 ) )
         {
            ret = TRUE ;
         }
         else
         {
            ret = FALSE ;
         }
         // Restore error reporter and exception state
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

   // read var
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
      // trim begin space
      while ( ' ' == *toker )
      {
         ++toker ;
      }
      // trim end space
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
      ossPrintf( "File %s is empty." OSS_NEWLINE , filename ) ;
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

   // set language for dispaly help info
   sptHelp::setLanguage( lang ) ;
   // set sdb defined can continue get next line function
   setCanContinueNextLineCallback( boost::bind( sdbdefCanContinueGetNextLine,
                                                scope, _1 ) ) ;

   // initialize and load the history
   historyInit () ;
   linenoiseHistoryLoad( historyFile.c_str() ) ;
   g_lnBuilder.loadCmd( historyFile.c_str() ) ;

   ossPrintf ( "Welcome to SequoiaDB shell!" OSS_NEWLINE ) ;
   ossPrintf ( "help() for help, Ctrl+c or quit to exit" OSS_NEWLINE ) ;

   while ( TRUE )
   {
      // code is freed in loop_next: or at the end of this function
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
      sdbSetIsNeedSaveHistory( TRUE ) ;
      // result is freed in loop_next:
      rc = scope->eval ( code , history.size(),
                         "(shell)" , 1, SPT_EVAL_FLAG_PRINT,
                         NULL ) ;

      if ( sdbIsNeedSaveHistory() )
      {
         linenoiseHistoryAdd ( code ) ;
         g_lnBuilder.addCmd( code ) ;
      }

      ossGetCurrentTime ( tmEnd ) ;
      // takes time
      tkTime = ( tmEnd.time * 1000000 + tmEnd.microtm ) -
               ( tmBegin.time * 1000000 + tmBegin.microtm ) ;
      sec = tkTime/1000000 ;
      microSec = tkTime%1000000 ;
      ossPrintf ( "Takes %lld.%06llds." OSS_NEWLINE , sec, microSec ) ;

   loop_next :
         SAFE_OSS_FREE ( code ) ;
   }

   SAFE_OSS_FREE ( code ) ;
   PD_TRACE_EXITRC ( SDB_ENTERINTATVMODE, rc );
   return rc ;
}

// Concatenate into a string delimited by \0 and ended with \0\0
// caller should free *args in the case of success
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

   // caller is responsible for freeing *args
   *args = (CHAR*) SDB_OSS_MALLOC ( argSize ) ;
   if ( NULL == *args )
   {
      rc = SDB_OOM ;
      ossPrintf( "Alloc memory failed" OSS_NEWLINE ) ;
      goto error ;
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
INT32 createDaemonProcess ( const CHAR *program, const OSSPID &ppid,
                            CHAR *f2dbuf, CHAR *d2fbuf,
                            CHAR *f2dCtlbuf, CHAR *d2fCtlbuf )
{
   CHAR *         args     = NULL ;
   INT32          rc       = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_CREATEDAEMONPROC );
   OSSPID         pid ;
   ossResultCode  result ;
   OSSNPIPE       waitPipe ;
   CHAR           waitName[128]   = { 0 } ;
   CHAR           f2dName[128]    = { 0 } ;
   CHAR           d2fName[128]    = { 0 } ;
   CHAR           f2dCtlName[128] = { 0 } ;
   CHAR           d2fCtlName[128] = { 0 } ;

   ossMemset ( &waitPipe, 0, sizeof( waitPipe ) ) ;
   ossMemset ( waitName,  0, sizeof( waitName ) ) ;
   ossMemset ( f2dName,   0, sizeof( f2dName ) ) ;
   ossMemset ( d2fName,   0, sizeof( d2fName ) ) ;

   SDB_ASSERT ( program && program[0] != '\0' , "Invalid argument" ) ;

   /// make sure the prgram exist
   rc = ossAccess( program ) ;
   if ( rc )
   {
      ossPrintf( "The program[%s] is not exist, rc: %d" OSS_NEWLINE,
                  program, rc ) ;
      goto error ;
   }

   rc = getWaitPipeName ( ppid ,  waitName , sizeof ( waitName ) ) ;
   if ( rc )
   {
      ossPrintf( "Get wait pipe name failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // clear the name pipe fd in windows
   clearDirtyShellPipe( SDB_SHELL_WAIT_PIPE_PREFIX ) ;

   // waitPipe is deleted in done:
   rc = ossCreateNamedPipe ( waitName , 0 , 0 , OSS_NPIPE_INBOUND ,
                             1 , 0 , waitPipe ) ;
   if ( rc )
   {
      ossPrintf( "Create named pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // args is freed in done ;
   rc = formatArgs ( program , ppid , &args ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = ossExec ( program , args , NULL , OSS_EXEC_NODETACHED , pid ,
                  result , NULL , NULL ) ;
   if ( rc )
   {
      ossPrintf( "Run program[%s] failed, rc: %d" OSS_NEWLINE, program, rc ) ;
      goto error ;
   }

   rc = getPipeNames2 ( ppid, pid,
                        f2dName, sizeof( f2dName ),
                        d2fName, sizeof( d2fName ),
                        f2dCtlName, sizeof( f2dCtlName ),
                        d2fCtlName, sizeof( d2fCtlName ) ) ;
   if ( rc )
   {
      ossPrintf( "Get pipe name failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   ossStrcpy ( f2dbuf, f2dName ) ;
   ossStrcpy ( d2fbuf, d2fName ) ;
   ossStrcpy ( f2dCtlbuf, f2dCtlName ) ;
   ossStrcpy ( d2fCtlbuf, d2fCtlName ) ;

   rc = ossConnectNamedPipe ( waitPipe , OSS_NPIPE_INBOUND ) ;
   if ( rc )
   {
      ossPrintf( "Connect to pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   rc = ossDisconnectNamedPipe ( waitPipe ) ;
   if ( rc )
   {
      ossPrintf( "Disconnect pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

done :
   if ( SDB_FE != rc )
   {
      ossDeleteNamedPipe ( waitPipe ) ;
   }
   SAFE_OSS_FREE ( args ) ;
   PD_TRACE_EXITRC ( SDB_CREATEDAEMONPROC, rc );
   return rc ;
error :
   goto done ;
}

static BOOLEAN isExit = FALSE ;

#if defined (_WINDOWS)

static void* readThread( const CHAR* bpf2dCtlName, const CHAR* bpd2fCtlName )
{
   INT32 rc = SDB_OK ;
   OSSNPIPE f2dCtlPipe ;
   OSSNPIPE d2fCtlPipe ;
   HANDLE handleStdin = NULL ;
   DWORD  consolemode = 0 ;

   handleStdin = GetStdHandle( STD_INPUT_HANDLE ) ;
   if ( handleStdin == INVALID_HANDLE_VALUE )
   {
      goto error ;
   }
   if ( !GetConsoleMode( handleStdin, &consolemode ) )
   {
      goto error ;
   }
   SetConsoleMode( handleStdin, consolemode & ~ENABLE_ECHO_INPUT ) ;

   while( !isExit )
   {
      CHAR line[256] = { 0 } ;
      DWORD len = 0 ;
      if( GetNumberOfConsoleInputEvents( handleStdin, &len ) )
      {
         if( len <= 0 )
         {
            ossSleep( 100 ) ;
            continue ;
         }
      }

      if ( fgets( line, 255, stdin ) != NULL )
      {
         rc = ossOpenNamedPipe( bpf2dCtlName, OSS_NPIPE_OUTBOUND, 0,
                                f2dCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dCtlName, rc ) ;
            goto error ;
         }

         rc = ossWriteNamedPipe( f2dCtlPipe, line, ossStrlen( line ), NULL ) ;
         if ( rc )
         {
            ossPrintf( "Write to pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = ossCloseNamedPipe ( f2dCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Close pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = ossOpenNamedPipe( bpd2fCtlName, OSS_NPIPE_INBOUND, 0,
                                d2fCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpd2fCtlName, rc ) ;
            goto error ;
         }

         rc = ossCloseNamedPipe ( d2fCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Close pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }
   }

done:
   SetConsoleMode( handleStdin, consolemode ) ;
   CloseHandle( handleStdin ) ;
   return NULL ;
error:
   goto done ;
}

#else

static void* readThread( const CHAR* bpf2dCtlName, const CHAR* bpd2fCtlName )
{
   INT32 rc = SDB_OK ;
   OSSNPIPE f2dCtlPipe ;
   OSSNPIPE d2fCtlPipe ;

   // trun off echo
   struct termios off_termios ;
   struct termios orig_termios ;

   if ( tcgetattr( STDIN_FILENO, &orig_termios ) == -1 ) goto error ;

   off_termios = orig_termios ;
   off_termios.c_lflag &= ~(ECHO) ;

   if ( tcsetattr( STDIN_FILENO, TCSADRAIN, &off_termios ) < 0 ) goto error ;

   while( !isExit )
   {
      CHAR buf[256] = {0} ;
      INT32 len = 0 ;
      if( !ioctl( STDIN_FILENO, FIONREAD, &len ) )
      {
         if( len <= 0 )
         {
            ossSleep( 100 ) ;
            continue ;
         }
      }

      if ( fgets( buf, 255, stdin ) != NULL )
      {
         rc = ossOpenNamedPipe( bpf2dCtlName, OSS_NPIPE_OUTBOUND, 0,
                                f2dCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dCtlName, rc ) ;
            goto error ;
         }

         rc = ossWriteNamedPipe( f2dCtlPipe, buf, ossStrlen( buf ), NULL ) ;
         if ( rc )
         {
            ossPrintf( "Write to pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = ossCloseNamedPipe ( f2dCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Close pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = ossOpenNamedPipe( bpd2fCtlName, OSS_NPIPE_INBOUND, 0,
                                d2fCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpd2fCtlName, rc ) ;
            goto error ;
         }

         rc = ossCloseNamedPipe ( d2fCtlPipe ) ;
         if ( rc )
         {
            ossPrintf( "Close pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }

   }

   tcsetattr( STDIN_FILENO, TCSADRAIN, &orig_termios ) ;

done:
   return NULL ;
error:
   goto done ;
}

#endif

#define SDB_FRONTEND_RECEIVEBUFFERSIZE 128
// PD_TRACE_DECLARE_FUNCTION ( SDB_ENTERFRONTENDMODE, "enterFrontEndMode" )
INT32 enterFrontEndMode ( const CHAR *program, const CHAR *cmd )
{
   CHAR     c           = '\0' ;
   INT32    rc          = SDB_OK ;
   OSSPID   ppid        = OSS_INVALID_PID ;
   CHAR *   p           = NULL ;
   INT32    id          = 0 ;
   INT32    retCode     = SDB_OK ;
   OSSNPIPE f2dPipe ;
   OSSNPIPE d2fPipe ;
   OSSNPIPE f2dCtlPipe ;
   OSSNPIPE d2fCtlPipe ;
   CHAR     f2dName[128]      = {0} ;
   CHAR     d2fName[128]      = {0} ;
   CHAR     f2dCtlName[128]   = {0} ;
   CHAR     d2fCtlName[128]   = {0} ;
   CHAR     bpf2dName[128]    = {0} ;
   CHAR     bpd2fName[128]    = {0} ;
   CHAR     bpf2dCtlName[128] = {0} ;
   CHAR     bpd2fCtlName[128] = {0} ;
   CHAR     pbFullPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   CHAR     receiveBuffer[SDB_FRONTEND_RECEIVEBUFFERSIZE] = {0} ;
   CHAR    *pCurrentReceivePtr = receiveBuffer ;
   boost::thread readTh ;

   PD_TRACE_ENTRY ( SDB_ENTERFRONTENDMODE ) ;

   ossMemset ( &f2dPipe,    0, sizeof( f2dPipe ) ) ;
   ossMemset ( &d2fPipe,    0, sizeof( d2fPipe ) ) ;
   ossMemset ( &f2dCtlPipe, 0, sizeof( f2dCtlPipe ) ) ;
   ossMemset ( &d2fCtlPipe, 0, sizeof( d2fCtlPipe ) ) ;

   SDB_ASSERT ( program && program[0] != '\0' , "invalid argument" ) ;
   if ( !cmd || cmd[0] == '\0' )
   {
      goto done ;
   }

   /// prepare the fullpath
   rc = ossGetEWD( pbFullPath, OSS_MAX_PATHSIZE ) ;
   if ( rc )
   {
      ossPrintf( "Get current path failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   else
   {
      UINT32 strLen = ossStrlen( pbFullPath ) ;
      if ( strLen + ossStrlen( SDB_PB_PROGRAM_NAME ) + 2 > OSS_MAX_PATHSIZE )
      {
         rc = SDB_INVALIDARG ;
         ossPrintf( "Path[%s] is to long" OSS_NEWLINE, pbFullPath ) ;
         goto error ;
      }
      else if ( strLen > 0 && pbFullPath[strLen-1] != OSS_FILE_SEP_CHAR )
      {
         ossStrncat( pbFullPath, OSS_FILE_SEP, 1 ) ;
      }
      ossStrncat( pbFullPath, SDB_PB_PROGRAM_NAME,
                  ossStrlen( SDB_PB_PROGRAM_NAME ) ) ;
   }

   // clear the name pipe fd in windows
   clearDirtyShellPipe( SDB_SHELL_F2B_PIPE_PREFIX ) ;
   clearDirtyShellPipe( SDB_SHELL_B2F_PIPE_PREFIX ) ;
   clearDirtyShellPipe( SDB_SHELL_CTL_F2B_PIPE_PREFIX ) ;
   clearDirtyShellPipe( SDB_SHELL_CTL_B2F_PIPE_PREFIX ) ;

   // get current process id
   ppid = ossGetParentProcessID () ;
   SH_VERIFY_COND ( ppid != OSS_INVALID_PID , SDB_SYS ) ;

   // set f2dName to be "sdb-shell-b2f-xxx(ppid)"
   // set d2fName to be "sdb-shell-f2b-xxx(ppid)"
   // set f2dCtlName to be "sdb-shell-ctl-b2f-xxx(ppid)"
   // set d2fCtlName to be "sdb-shell-ctl-f2b-xxx(ppid)"

   // f2dName and d2fName are used as matching condition to get
   // the bpf2d and bpd2f name pipe if the two name pipe existed
   // in /var/sequoiadb

   // f2dCtlName and d2fCtlName are used as matching condition to get
   // the bpf2dCtl and bpd2fCtl name pipe if the two name pipe existed
   // in /var/sequoiadb
   rc = getPipeNames ( ppid,
                       f2dName, sizeof( f2dName ),
                       d2fName, sizeof( d2fName ),
                       f2dCtlName, sizeof( f2dCtlName ),
                       d2fCtlName, sizeof( d2fCtlName ) ) ;
   if ( rc )
   {
      ossPrintf( "Build pipe names failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // there may be four name pipe in /var/sequoiadb,
   // get the their names if they existed
   rc = getPipeNames1 ( bpf2dName, sizeof( bpf2dName ),
                        bpd2fName, sizeof( bpd2fName ),
                        bpf2dCtlName, sizeof( bpf2dCtlName ),
                        bpd2fCtlName, sizeof( bpd2fCtlName ),
                        f2dName, d2fName,
                        f2dCtlName, d2fCtlName ) ;

   if ( rc == SDB_OK )
   {
      // the second parameter 45 is ascii value of '-'
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
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dName, rc ) ;
            goto error ;
         }
      }
      else
      {
         // first we should delete the old pipes
         rc = ossCleanNamedPipeByName ( bpf2dName ) ;
         if ( rc )
         {
            ossPrintf( "Clean pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dName, rc ) ;
            goto error ;
         }
         rc = ossCleanNamedPipeByName ( bpd2fName ) ;
         if ( rc )
         {
            ossPrintf( "Clean pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpd2fName, rc ) ;
            goto error ;
         }
         rc = ossCleanNamedPipeByName ( bpf2dCtlName ) ;
         if ( rc )
         {
            ossPrintf( "Clean pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dCtlName, rc ) ;
            goto error ;
         }
         rc = ossCleanNamedPipeByName ( bpd2fCtlName ) ;
         if ( rc )
         {
            ossPrintf( "Clean pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpd2fCtlName, rc ) ;
            goto error ;
         }

         rc = createDaemonProcess ( pbFullPath, ppid, bpf2dName, bpd2fName,
                                    bpf2dCtlName, bpd2fCtlName ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = ossOpenNamedPipe ( bpf2dName , OSS_NPIPE_OUTBOUND , 0 , f2dPipe ) ;
         if ( rc )
         {
            ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                       bpf2dName, rc ) ;
            goto error ;
         }
      }
   }
   else if ( rc == SDB_FNE )
   {
      // named pipe does not exist, so we need to create the daemon process
      // which will create those named pipes

      // create a process which will create two name pipe
      rc = createDaemonProcess ( pbFullPath, ppid, bpf2dName, bpd2fName,
                                 bpf2dCtlName, bpd2fCtlName ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = ossOpenNamedPipe ( bpf2dName , OSS_NPIPE_OUTBOUND , 0 , f2dPipe ) ;
      if ( rc )
      {
         ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                    bpf2dName, rc ) ;
      }
   }
   else
   {
      ossPrintf( "Get pipe names failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // also write the trailing \0 to mark end of write
   rc = ossWriteNamedPipe ( f2dPipe , cmd , ossStrlen ( cmd ) , NULL ) ;
   if ( rc )
   {
      ossPrintf( "Write to pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   rc = ossCloseNamedPipe ( f2dPipe ) ;
   if ( rc )
   {
      ossPrintf( "Close pipe failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   rc = ossOpenNamedPipe ( bpd2fName , OSS_NPIPE_INBOUND ,
                           OSS_NPIPE_INFINITE_TIMEOUT , d2fPipe ) ;
   if ( rc )
   {
      ossPrintf( "Open pipe[%s] failed, rc: %d" OSS_NEWLINE,
                 bpd2fName, rc ) ;
      goto error ;
   }

   try
   {
      readTh = boost::thread( readThread, bpf2dCtlName, bpd2fCtlName ) ;
   }
   catch ( boost::thread_resource_error )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   // rest are the actual message
   // if we failed at first loop, we'll never enter here since rc != SDB_OK
   while ( TRUE )
   {
      rc = ossReadNamedPipe ( d2fPipe , &c , 1 , NULL ) ;
      // loop until reading something
      if ( rc )
         break ;
      if ( '\0' == c )
      {
         ossPrintf ( "%s", receiveBuffer ) ;
         ossMemset ( receiveBuffer, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
         pCurrentReceivePtr = &receiveBuffer[0] ;
      }
      else if ( pCurrentReceivePtr - &receiveBuffer[0] ==
                SDB_FRONTEND_RECEIVEBUFFERSIZE - 1 )
      {
         receiveBuffer[SDB_FRONTEND_RECEIVEBUFFERSIZE-1] = 0 ;
         ossPrintf ( "%s", receiveBuffer ) ;
         ossMemset ( receiveBuffer, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
         pCurrentReceivePtr = &receiveBuffer[0] ;
         *pCurrentReceivePtr = c ;
         ++pCurrentReceivePtr ;
      }
      else if ( pCurrentReceivePtr - &receiveBuffer[0] <
                SDB_FRONTEND_RECEIVEBUFFERSIZE - 1 )
      {
         if ( '\n' == c )
         {
            *pCurrentReceivePtr = c ;
            ossPrintf ( "%s", receiveBuffer ) ;
            ossMemset ( receiveBuffer, 0, SDB_FRONTEND_RECEIVEBUFFERSIZE ) ;
            pCurrentReceivePtr = &receiveBuffer[0] ;
         }
         else
         {
            *pCurrentReceivePtr = c ;
            pCurrentReceivePtr++ ;
         }
      }
      else
      {
         // something wrong, we should never hit here
         ossPrintf ( "SEVERE Error, we should never hit here" OSS_NEWLINE ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }
   pCurrentReceivePtr = &receiveBuffer[ossStrlen(receiveBuffer)] ;
   while ( pCurrentReceivePtr != &receiveBuffer[0] &&
           *pCurrentReceivePtr != ' ' )
   {
      --pCurrentReceivePtr ;
   }
   if ( *pCurrentReceivePtr == ' ' )
   {
      retCode = ossAtoi ( pCurrentReceivePtr+1 ) ;
   }
   *pCurrentReceivePtr = '\0' ;
   ossPrintf ( "%s" , receiveBuffer ) ;
   SH_VERIFY_COND ( SDB_OK == rc || SDB_EOF == rc , rc )

   rc = ossCloseNamedPipe( d2fPipe ) ;
   if ( rc )
   {
      goto error ;
   }

   isExit = TRUE ;
   readTh.join() ;

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
   // The rule is
   // SDB_SYS : SDB_RETURNCODE_SYSTEM
   // SDB_OK  : SDB_RETURNCODE_OK
   // SDB_DMS_EOC && !sdbHasReadData() : SDB_RETURNCODE_EMPTY
   // SDB_DMS_EOC && sdbHasReadData : SDB_OK
   // Others  : SDB_RETURNCODE_ERROR
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

   setlocale(LC_CTYPE, "zh_CN.UTF-8");
#if defined( _LINUX )
   signal( SIGCHLD, SIG_IGN ) ;
#endif // _LINUX

   // Initialize TZ, ignore error
   rc = ossInitTZEnv() ;
   if ( rc )
   {
      ossPrintf( "Failed to init the TZ environment variable, rc: %d" OSS_NEWLINE, rc ) ;
      rc = SDB_OK ;
   }

   linenoiseSetCompletionCallback( (linenoiseCompletionCallback*)lineComplete ) ;

   rc = container.init() ;
   if ( rc )
   {
      ossPrintf( "Init container failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   scope = container.newScope() ;
   SH_VERIFY_COND ( scope , SDB_SYS ) ;

   // parse Argument into argInfo
   rc = parseArguments ( argc , argv , argInfo ) ;
   if( SDB_SDB_HELP_ONLY == rc || SDB_SDB_VERSION_ONLY == rc )
   {
      rc = SDB_OK ;
      goto done ;
   }
   else if ( rc )
   {
      ossPrintf( "Parse args failed, rc: %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

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

