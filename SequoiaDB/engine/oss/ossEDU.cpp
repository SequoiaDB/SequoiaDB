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

   Source File Name = ossEDU.cpp

   Descriptive Name = Operating System Services Engine Dispatchable Unit

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for creating EDU
   stack.

   Dependencies: ossStackDump.cpp

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossFeat.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossMem.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "ossVer.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined(_LINUX)
#include <dlfcn.h>
#include <execinfo.h>
#endif

#include "ossStackDump.hpp"
#include "ossEDU.hpp"
#include <time.h>

#include "pmdEnv.hpp"

namespace  engine
{
   // For whenever trying to use thread-based long jump pointer, here are
   // steps
   // 1) determine if the per-thread variable is initialized
   // 2) if not, run reset(new ossValuePtr(<init value>));
   // 3) set the *_longJmpPtr to whenever you like
   // 4) same method to use it
   // ex:
   // if(_longJmpPtr.get() == 0 )
   //    _longJmpPtr.reset(new ossValuePtr(0));
   // (*_longJmpPtr)=(ossValuePtr)0xabcdabcd;
   //
   // everytime you use *_longJmpPtr, it's thread specific value

   oss_edu_data* ossGetThreadEDUData()
   {
      static OSS_THREAD_LOCAL BOOLEAN s_init = FALSE ;
      static OSS_THREAD_LOCAL oss_edu_data s_eduData ;
      if ( !s_init )
      {
         s_eduData.init() ;
         s_init = TRUE ;
      }
      return &s_eduData ;
   }

#if defined(_LINUX)

   void ossOneTimeOnly()
   {
      void *pSyms[1] ;
      backtrace( pSyms, 1 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSNTHND, "ossNestedTrapHandler" )
   static void ossNestedTrapHandler( OSS_HANDPARMS )
   {
      PD_TRACE_ENTRY ( SDB_OSSNTHND );
      oss_edu_data * pEduData = ossGetThreadEDUData() ;

      if (    ( NULL != pEduData )
           && ( OSS_EDU_DATA_EYE_CATCHER == pEduData->ossEDUEyeCatcher1 )
           && ( OSS_EDU_DATA_EYE_CATCHER == pEduData->ossEDUEyeCatcher2 ) )
      {
         ossLongJump( pEduData->ossNestedSignalHanderJmpBuf, 1 ) ;
      }
      PD_TRACE_EXIT ( SDB_OSSNTHND );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSST, "ossStackTrace" )
   void ossStackTrace( OSS_HANDPARMS, const CHAR *dumpDir )
   {
      PD_TRACE_ENTRY ( SDB_OSSST );
      ossPrimitiveFileOp trapFile ;
      CHAR fileName [ OSS_MAX_PATHSIZE ] = {0} ;
      UINT32 strLen = 0 ;

      oss_edu_data * pEduData = ossGetThreadEDUData() ;

      sigset_t savemask, tmpmask ;
      SINT32 prevNestedHandlerLevel = 0 ;
      OSS_SIGFUNCPTR  pPrevNestedHandler = NULL ;

      // save current mask
      pthread_sigmask( SIG_SETMASK, NULL, &savemask ) ;

      sigemptyset( &tmpmask ) ;
      sigaddset( &tmpmask, SIGSEGV ) ;
      sigaddset( &tmpmask, SIGILL  ) ;
      sigaddset( &tmpmask, SIGTRAP ) ;
      sigaddset( &tmpmask, SIGBUS  ) ;

      if ( NULL != pEduData )
      {
         OSS_GET_NESTED_HANDLER_LEVEL( pEduData, prevNestedHandlerLevel ) ;

         pPrevNestedHandler = pEduData->ossEDUNestedSignalHandler ;
         pEduData->ossEDUNestedSignalHandler =
            (OSS_SIGFUNCPTR)ossNestedTrapHandler ;

         OSS_CLEAR_NESTED_HANDLER_LEVEL( pEduData ) ;

         if ( 0 == ossSetJump( pEduData->ossNestedSignalHanderJmpBuf, 1 ) )
         {
            // unblock it for this thread, so we can use nested trap handler
            // to longjmp() back.
            pthread_sigmask( SIG_UNBLOCK, &tmpmask, NULL ) ;
         }
         else
         {
            // jumped back
            //
            // trapFile.Write("long jump back\n" ) ;
            trapFile.Close() ;
            goto error ;
         }

         ossSnprintf ( fileName, OSS_MAX_PATHSIZE, "%d.%d.trap",
                       ossGetCurrentProcessID(),
                       ossGetCurrentThreadID() ) ;
         if ( ossStrlen ( dumpDir ) + ossStrlen ( OSS_PRIMITIVE_FILE_SEP ) +
              ossStrlen ( fileName ) > OSS_MAX_PATHSIZE )
         {
            PD_LOG ( PDERROR, "path + file name is too long" ) ;
            goto error ;
         }

         ossMemset( fileName, 0, sizeof( fileName ) ) ;
         strLen += ossSnprintf( fileName, sizeof( fileName ), "%s%s",
                                dumpDir, OSS_PRIMITIVE_FILE_SEP ) ;
         ossSnprintf( fileName + strLen, sizeof(fileName) - strLen,
                      "%d.%d.trap",
                      ossGetCurrentProcessID(),
                      ossGetCurrentThreadID() ) ;

         ossOneTimeOnly() ;

         trapFile.Open( fileName ) ;

         if ( trapFile.isValid() )
         {
            trapFile.seekToEnd () ;
            ossDumpStackTrace( OSS_HANDARGS, &trapFile ) ;
         }

         // testing
         // trapFile.Write("testing trap while dumping stack trace\n" ) ;
         // memcpy( NULL, "CRASH", 5 ) ;

         trapFile.Close() ;
      }

   done :
      // restore original signal handler ;
      if ( NULL != pEduData )
      {
         pEduData->ossEDUNestedSignalHandler = pPrevNestedHandler ;
         OSS_SET_NESTED_HANDLER_LEVEL( pEduData, prevNestedHandlerLevel ) ;
      }

      // restore original signal mask ;
      pthread_sigmask( SIG_SETMASK, &savemask, NULL ) ;

      PD_TRACE_EXIT ( SDB_OSSST );
      return ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSEDUCTHND, "ossEDUCodeTrapHandler" )
   void ossEDUCodeTrapHandler( OSS_HANDPARMS )
   {
      PD_TRACE_ENTRY ( SDB_OSSEDUCTHND );
      oss_edu_data * pEduData = NULL ;
      const CHAR *dumpPath = ossGetTrapExceptionPath () ;
      if ( !dumpPath )
      {
         goto done ;
      }

      pEduData = ossGetThreadEDUData() ;

      if ( NULL == pEduData )
      {
         ossSignalHandlerAbort( OSS_HANDARGS, dumpPath ) ;
         goto done ;
      }

      if ( OSS_AM_I_INSIDE_SIGNAL_HANDLER( pEduData ) )
      {
         if ( NULL != pEduData->ossEDUNestedSignalHandler )
         {
            if ( OSS_AM_I_HANDLING_NESTED_SIGNAL( pEduData ) )
            {
               // in case it traps while dealing with a nested signal( e.g,
               // there is another trap or receives another signal, say SIGBUS,
               // while ossEDUNestedSignalHandler is in progress ),
               // we need an emergency exit, acutally nothing more we can do,
               // simply reset signal handler to system default one, SIG_DEL,
               // to avoid getting stuck in infinite recursion.
               ossSignalHandlerAbort( OSS_HANDARGS, dumpPath ) ;
            }
            OSS_INVOKE_NESTED_SIGNAL_HANDLER( pEduData ) ;
            pEduData->ossEDUNestedSignalHandler( OSS_HANDARGS ) ;
            // in most cases, we should not get here, since the nested signal
            // handler will long jump to other places.
            OSS_LEAVE_NESTED_SIGNAL_HANDLER( pEduData ) ;
         }
         else
         {
            // when a nested trap detected and there is no associated
            // signal handler, try to dump the stack trace then bring engine
            // down.
            ossSignalHandlerAbort( OSS_HANDARGS, dumpPath ) ;
            ossStackTrace( OSS_HANDARGS, dumpPath ) ;
            goto done ;
         }
      }
      OSS_ENTER_SIGNAL_HANDLER( pEduData ) ;

      // dump stack trace
      ossStackTrace( OSS_HANDARGS, dumpPath ) ;
#if defined ( SDB_ENGINE )
      // freeze engine here
      pmdSleepInstance() ;
#endif
      // restore system default signal handler
      ossSignalHandlerAbort( OSS_HANDARGS, dumpPath ) ;

      OSS_LEAVE_SIGNAL_HANDLER( pEduData ) ;
   done :
      PD_TRACE_EXIT ( SDB_OSSEDUCTHND );
      return ;
   }

#elif defined(_WINDOWS)

#ifndef _WIN64
      #define SEGREG "Gs : %08lX Fs : %08lX\n"   \
                     "ES : %08lX Ds : %08lX\n"
      #define INTREG "Edi: %p Esi: %p\n"         \
                     "Eax: %p Ebx: %p\n"         \
                     "Ecx: %p Edx: %p\n   "
      #define CTXREG "Ebp: %p Eip: %p\n"         \
                     "Esp: %p EFlags: %p\n"      \
                     "Cs : %08lX Ss : %08lX\n"
#elif defined(_M_AMD64)
      #define SEGREG "Gs : %016X Fs : %016X\n"   \
                     "Es : %016X Ds : %016X\n"
      #define INTREG "Rdi: %p Rsi: %p\n"         \
                     "Rax: %p Rbx: %p\n"         \
                     "Rcx: %p Rdx: %p\n"         \
                     "R8 : %p R9 : %p\n"         \
                     "R10: %p R11: %p\n"         \
                     "R12: %p R13: %p\n"         \
                     "R14: %p R15: %p\n"
      #define CTXREG "Rbp: %p Rip: %p\n"         \
                     "Rsp: %p EFlags: %p\n"      \
                     "Cs : %016X Ss : %016X\n"
#endif

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSEDUEXCFLT, "ossEDUExceptionFilter" )
   SINT32 ossEDUExceptionFilter( LPEXCEPTION_POINTERS lpEP )
   {
      SINT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_OSSEDUEXCFLT );
      const CHAR *dumpDir = ossGetTrapExceptionPath () ;
      if ( !dumpDir )
      {
         goto done ;
      }
      else
      {
         oss_edu_data * pEduData = ossGetThreadEDUData() ;

         OSS_ENTER_SIGNAL_HANDLER( pEduData ) ;

         ossStackTrace( lpEP, dumpDir ) ;

         OSS_LEAVE_SIGNAL_HANDLER( pEduData ) ;

         // The following function will disable the pop-up window that appears
         // when the crash occurs.
         SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX ) ;

         // Terminate the process
         TerminateProcess( GetCurrentProcess(), (UINT)-1 ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_OSSEDUEXCFLT, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDMPSYSTM, "ossDumpSystemTime" )
   void ossDumpSystemTime( ossPrimitiveFileOp * trapFile )
   {
      PD_TRACE_ENTRY ( SDB_OSSDMPSYSTM );
      struct tm otm ;
      struct timeval tv ;
      struct timezone tz ;
      time_t tt ;
      gettimeofday ( &tv, &tz ) ;
      tt = tv.tv_sec ;
      localtime_s ( &otm, &tt ) ;
      if ( ( NULL != trapFile ) && trapFile->isValid() )
      {
         trapFile->fWrite ( "Timestamp: %04d-%02d-%02d-%02d.%02d.%02d.%06d"
                            OSS_NEWLINE,
                            otm.tm_year+1900,            // 1) Year (UINT32)
                            otm.tm_mon+1,                // 2) Month (UINT32)
                            otm.tm_mday,                 // 3) Day (UINT32)
                            otm.tm_hour,                 // 4) Hour (UINT32)
                            otm.tm_min,                  // 5) Minute (UINT32)
                            otm.tm_sec,                  // 6) Second (UINT32)
                            tv.tv_usec                   // 7) Microsecond (UINT32)
                          );
      }
      PD_TRACE_EXIT ( SDB_OSSDMPSYSTM );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDMPDBINFO, "ossDumpDatabaseInfo" )
   void ossDumpDatabaseInfo ( ossPrimitiveFileOp * trapFile )
   {
      PD_TRACE_ENTRY ( SDB_OSSDMPDBINFO );
      if ( ( NULL != trapFile ) && trapFile->isValid() )
      {
         CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossSprintVersion( "Version: ", verText, OSS_MAX_PATHSIZE, FALSE ) ;
         trapFile->fWrite( "%s"OSS_NEWLINE, verText ) ;
      }
      PD_TRACE_EXIT ( SDB_OSSDMPDBINFO );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSTKTRA, "ossStackTrace" )
   void ossStackTrace( LPEXCEPTION_POINTERS lpEP, const CHAR * dumpDir )
   {
      PD_TRACE_ENTRY ( SDB_OSSSTKTRA );
      SYMBOL_INFO  * pSymbol = NULL ;
      HANDLE  hProcess ;
      void  * stack[ OSS_MAX_BACKTRACE_FRAMES_SUPPORTED + 1 ] = { 0 } ;
      CHAR pName[ OSS_FUNC_NAME_LEN_MAX + 1 ]  ;
      UINT32 frames = 0 ;
      SYSTEM_INFO sysInfo = { 0 } ;
      OSVERSIONINFOEX OSVerInfo={ 0 } ;
      ossPrimitiveFileOp trapFile ;
      CHAR fileName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      UINT32 StrLen = 0 ;

      ossSnprintf ( fileName, OSS_MAX_PATHSIZE, "%d.%d.trap",
                    ossGetCurrentProcessID(),
                    ossGetCurrentThreadID() ) ;
      if ( OSS_MAX_PATHSIZE <
              ossStrlen ( dumpDir ) + ossStrlen ( OSS_PRIMITIVE_FILE_SEP ) +
              ossStrlen ( fileName ) )
      {
          pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                  "path + file name is too long" ) ;
          goto error ;
      }

      ossMemset( fileName, 0, sizeof( fileName ) ) ;
      StrLen += ossSnprintf( fileName, sizeof( fileName ), "%s%s",
                             dumpDir, OSS_PRIMITIVE_FILE_SEP ) ;
      ossSnprintf( fileName + StrLen, sizeof(fileName) - StrLen,
                   "%d.%d.trap",
                   ossGetCurrentProcessID(),
                   ossGetCurrentThreadID() ) ;

      trapFile.Open( fileName ) ;

      if ( trapFile.isValid() )
      {
         trapFile.seekToEnd () ;
         // System info
         trapFile.Write(" -------- System Information --------\n" ) ;
         ossDumpSystemTime ( &trapFile ) ;
         ossDumpDatabaseInfo ( &trapFile ) ;
         GetSystemInfo( &sysInfo ) ;
         switch( sysInfo.wProcessorArchitecture )
         {
            case PROCESSOR_ARCHITECTURE_INTEL:
                 trapFile.Write( "Processor : Intel x86\n" ) ;
                 break ;
            case PROCESSOR_ARCHITECTURE_IA64:
                 trapFile.Write( "Processor : Intel IA64\n" ) ;
                 break ;
            case PROCESSOR_ARCHITECTURE_AMD64:
                 trapFile.Write( "Processor : AMD 64\n") ;
                 break ;
            default:
                 trapFile.Write( "Unknown processor architecture ") ;
         }
         trapFile.fWrite( " Number of processors: %u Page size: %u\n"
                          " Min application address: %lx"
                          " Max application address: %lx\n"
                          "  Active processor mask: %u\n",
                         sysInfo.dwNumberOfProcessors,
                         sysInfo.dwPageSize,
                         sysInfo.lpMinimumApplicationAddress,
                         sysInfo.lpMaximumApplicationAddress,
                         sysInfo.dwActiveProcessorMask ) ;

         // OS info
         OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
         GetVersionEx ( (OSVERSIONINFO*) &OSVerInfo ) ;
         if ( OSVerInfo.dwMajorVersion == 6 )
         {
            if ( OSVerInfo.dwMinorVersion == 0 )
            {
               if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
               {
                  trapFile.Write( "Windows Vista " ) ;
               }
               else
               {
                  trapFile.Write( "Windows Server 2008 " ) ;
               }
            }
            if ( OSVerInfo.dwMinorVersion == 1 )
            {
               if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
               {
                  trapFile.Write( "Windows 7 " ) ;
               }
               else
               {
                  trapFile.Write( "Windows Server 2008 " ) ;
               }
            }
         }
         if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 2 )
         {
            if ( OSVerInfo.wProductType == VER_NT_WORKSTATION )
            {
               trapFile.Write( "Windows XP " ) ;
            }
            else
            {
               trapFile.Write( "Windows Server 2003 " ) ;
            }
         }
         if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 1 )
         {
            trapFile.Write("Windows XP ") ;
         }
         if ( OSVerInfo.dwMajorVersion == 5 && OSVerInfo.dwMinorVersion == 0 )
         {
            trapFile.Write("Windows 2000 ") ;
         }
         trapFile.fWrite( "%s ( %d.%d) Build:%d\n",
                          OSVerInfo.szCSDVersion,
                          OSVerInfo.dwMajorVersion,
                          OSVerInfo.dwMinorVersion,
                          OSVerInfo.dwBuildNumber ) ;

         hProcess = GetCurrentProcess() ;

         pSymbol = ( SYMBOL_INFO * )SDB_OSS_MALLOC(
                       sizeof( SYMBOL_INFO ) +
                       OSS_FUNC_NAME_LEN_MAX * sizeof( char ) ) ;
         if ( NULL != pSymbol )
         {
            ossMemset( pSymbol, 0,
                       sizeof( SYMBOL_INFO ) +
                       OSS_FUNC_NAME_LEN_MAX * sizeof(char) ) ;
            pSymbol->MaxNameLen = OSS_FUNC_NAME_LEN_MAX ;
            pSymbol->SizeOfStruct = sizeof( SYMBOL_INFO ) ;
            ossSymInitialize( hProcess, NULL, TRUE ) ;

            if ( NULL != lpEP )
            {
               //  dump registers
               trapFile.Write( "-------- Registers --------\n" ) ;
            #ifndef _WIN64
               trapFile.fWrite( SEGREG,
                                lpEP->ContextRecord->SegGs,
                                lpEP->ContextRecord->SegFs,
                                lpEP->ContextRecord->SegEs,
                                lpEP->ContextRecord->SegDs ) ;
               trapFile.fWrite( INTREG,
                                (void*)lpEP->ContextRecord->Edi,
                                (void*)lpEP->ContextRecord->Esi,
                                (void*)lpEP->ContextRecord->Eax,
                                (void*)lpEP->ContextRecord->Ebx,
                                (void*)lpEP->ContextRecord->Ecx,
                                (void*)lpEP->ContextRecord->Edx ) ;
               trapFile.fWrite( CTXREG,
                                (void*)lpEP->ContextRecord->Ebp,
                                (void*)lpEP->ContextRecord->Eip,
                                (void*)lpEP->ContextRecord->Esp,
                                (void*)lpEP->ContextRecord->EFlags,
                                lpEP->ContextRecord->SegCs,
                                lpEP->ContextRecord->SegSs ) ;
            #elif defined(_M_AMD64)
               trapFile.fWrite( SEGREG,
                                lpEP->ContextRecord->SegGs,
                                lpEP->ContextRecord->SegFs,
                                lpEP->ContextRecord->SegEs,
                                lpEP->ContextRecord->SegDs ) ;
               trapFile.fWrite( INTREG,
                                (void*)lpEP->ContextRecord->Rdi,
                                (void*)lpEP->ContextRecord->Rsi,
                                (void*)lpEP->ContextRecord->Rax,
                                (void*)lpEP->ContextRecord->Rbx,
                                (void*)lpEP->ContextRecord->Rcx,
                                (void*)lpEP->ContextRecord->Rdx,
                                (void*)lpEP->ContextRecord->R8,
                                (void*)lpEP->ContextRecord->R9,
                                (void*)lpEP->ContextRecord->R10,
                                (void*)lpEP->ContextRecord->R11,
                                (void*)lpEP->ContextRecord->R12,
                                (void*)lpEP->ContextRecord->R13,
                                (void*)lpEP->ContextRecord->R14,
                                (void*)lpEP->ContextRecord->R15 ) ;
               trapFile.fWrite( CTXREG,
                                (void*)lpEP->ContextRecord->Rbp,
                                (void*)lpEP->ContextRecord->Rip,
                                (void*)lpEP->ContextRecord->Rsp,
                                (void*)lpEP->ContextRecord->EFlags,
                                lpEP->ContextRecord->SegCs,
                                lpEP->ContextRecord->SegSs ) ;
            #endif

               // point of failure
               trapFile.fWrite( "-------- Point of failure --------\n" ) ;
               ossMemset( pName, 0, sizeof( pName ) ) ;
               ossGetSymbolNameFromAddress( hProcess,
                  (UINT64)lpEP->ExceptionRecord->ExceptionAddress,
                  pSymbol, pName, sizeof( pName ) ) ;
               trapFile.fWrite( "%s\n", pName ) ;
            }  // if NULL != lpEP

            // dump stack frames
            trapFile.fWrite( "\n-------- Stack frames --------\n" ) ;
         #ifndef _WIN64
            if ( NULL != lpEP )
            {
               frames = ossWalkStackEx( lpEP,
                                        0,
                                        OSS_MAX_BACKTRACE_FRAMES_SUPPORTED,
                                        stack ) ;
            }
            else
            {
         #endif
               frames = ossWalkStack( 0,
                                      OSS_MAX_BACKTRACE_FRAMES_SUPPORTED,
                                      stack ) ;
         #ifndef _WIN64
            }
         #endif

            for ( UINT32 i = 0; i < frames; i++ )
            {
                ossMemset( pName, 0, sizeof( pName ) ) ;
                ossGetSymbolNameFromAddress(
                   hProcess, (UINT64)stack[i], pSymbol, pName, sizeof( pName ) ) ;
                trapFile.fWrite(  "%3i: %s\n", i, pName ) ;
            }
            SDB_OSS_FREE( pSymbol ) ;
         }  // if NULL != pSymbol
      }
   error :
      trapFile.Close() ;
      PD_TRACE_EXIT ( SDB_OSSSTKTRA );
      return ;
   }

#endif

   /*
      Static var define
   */
   static CHAR ossTrapExceptionPath [ OSS_MAX_PATHSIZE+1 ] = {0} ;

   void ossSetTrapExceptionPath ( const CHAR *path )
   {
      ossMemset ( ossTrapExceptionPath, 0, sizeof(ossTrapExceptionPath ) ) ;
      ossStrncpy ( ossTrapExceptionPath, path, OSS_MAX_PATHSIZE ) ;
   }

   const CHAR* ossGetTrapExceptionPath ()
   {
      if ( ossTrapExceptionPath[0] == 0 )
      {
         return NULL ;
      }
      return ossTrapExceptionPath ;
   }

}  // namespace engine

#ifdef _LINUX

   _ossSigSet::_ossSigSet ()
   {
      emptySet () ;
   }

   _ossSigSet::~_ossSigSet ()
   {
   }

   void _ossSigSet::emptySet ()
   {
      memset ( _sigArray, 0, sizeof (_sigArray) ) ;
   }

   void _ossSigSet::fillSet ()
   {
      memset ( &_sigArray[1], 1, sizeof (_sigArray) ) ;
      _sigArray[32] = 0 ;
      _sigArray[33] = 0 ;
   }

   void _ossSigSet::sigAdd ( INT32 sigNum )
   {
      if ( sigNum > 0 && sigNum <= OSS_MAX_SIGAL
         && 32 != sigNum && 33 != sigNum )
      {
         _sigArray[sigNum] = 1 ;
      }
   }

   void _ossSigSet::sigDel ( INT32 sigNum )
   {
      if ( sigNum > 0 && sigNum <= OSS_MAX_SIGAL )
      {
         _sigArray[sigNum] = 0 ;
      }
   }

   BOOLEAN _ossSigSet::isMember ( INT32 sigNum )
   {
      if ( sigNum > 0 && sigNum <= OSS_MAX_SIGAL )
      {
         return _sigArray[sigNum] ? TRUE : FALSE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OSSREGSIGHND, "ossRegisterSignalHandle" )
   INT32 ossRegisterSignalHandle ( ossSigSet & sigSet, SIG_HANDLE handle )
   {
      // handle=NULL means reset signal handler to default ( SIG_DFL )
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_OSSREGSIGHND );
      struct sigaction newact ;

      sigemptyset ( &newact.sa_mask ) ;
      newact.sa_flags = 0 ;
      newact.sa_handler = ( __sighandler_t )handle ;
      // SIGKILL and SIGSTOP can't be overwritten
      sigSet.sigDel ( SIGKILL ) ;
      sigSet.sigDel ( SIGSTOP ) ;
      INT32 sig = 1 ;
      while ( sig <= OSS_MAX_SIGAL )
      {
         if ( sigSet.isMember( sig ) && sigaction ( sig, &newact, NULL ) )
         {
            PD_LOG ( PDWARNING, "Failed to register signal handler for %d, "
                     "errno = %d", sig, ossGetLastError() ) ;
         }
         ++sig ;
      }
      //PD_LOG ( PDDEBUG, "Register signal handler succeed" ) ;

      PD_TRACE_EXITRC ( SDB_OSSREGSIGHND, rc );
      return rc ;
   }

#endif //_LINUX

