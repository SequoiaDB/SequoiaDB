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

   Source File Name = ossStackDump.cpp

   Descriptive Name = Operating System Services Stack Dump

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for dumping stack
   rewind.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossSignal.hpp"
#include "ossStackDump.hpp"
#include "ossUtil.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossVer.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"

// For power and arm systems, there are 31 general purpose registers. No macros
// defined in their headers. So do it here.
#if defined (_PPCLIN64) || defined (_ARMLIN64)
#define REG_R0   0
#define REG_R1   1
#define REG_R2   2
#define REG_R3   3
#define REG_R4   4
#define REG_R5   5
#define REG_R6   6
#define REG_R7   7
#define REG_R8   8
#define REG_R9   9
#define REG_R10  10
#define REG_R11  11
#define REG_R12  12
#define REG_R13  13
#define REG_R14  14
#define REG_R15  15
#define REG_R16  16
#define REG_R17  17
#define REG_R18  18
#define REG_R19  19
#define REG_R20  20
#define REG_R21  21
#define REG_R22  22
#define REG_R23  23
#define REG_R24  24
#define REG_R25  25
#define REG_R26  26
#define REG_R27  27
#define REG_R28  28
#define REG_R29  29
#define REG_R30  30
#endif /* (_PPCLIN64) || (_ARMLIN64) */

#if defined (_PPCLIN64)
#define REG_R31  31
#endif

#if defined (_LINUX)
#include <sys/prctl.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <stdint.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <boost/thread/thread.hpp>

// Restore default signal handler, and setup for generation of a core file.
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSRSTSYSSIG, "ossRestoreSystemSignal" )
void ossRestoreSystemSignal( const INT32 sigNum,
                             const BOOLEAN isCoreNeeded,
                             const CHAR *dumpDir )
{
   PD_TRACE_ENTRY ( SDB_OSSRSTSYSSIG );
   struct sigaction action ;
   struct rlimit rlim = { 0 } ;
   //rlim_t previous_rlim_max = 0 ;

   if ( ! isCoreNeeded )
   {
      rlim.rlim_max = 0 ;
      rlim.rlim_cur = 0 ;
      ( void )setrlimit( RLIMIT_CORE, &rlim ) ;
   }
   else if ( 0 == getrlimit( RLIMIT_CORE, &rlim ) )
   {
      //previous_rlim_max = rlim.rlim_max ;

      // full core dump
      rlim.rlim_max = RLIM_INFINITY ;
      rlim.rlim_cur = rlim.rlim_max ;
      setrlimit( RLIMIT_CORE, &rlim ) ;
   }

   // Set the core dump file here.
   prctl(PR_SET_DUMPABLE, 1, 0, 0, 0 ) ;

   // need to change to current working/dump directory
   if ( NULL != dumpDir )
   {
      chdir( dumpDir ) ;
   }
   action.sa_handler = SIG_DFL ;
   sigemptyset( &action.sa_mask ) ;

#if defined SA_RESTART
   action.sa_flags = 0 ;
   action.sa_flags |= SA_RESTART ;
#else
   action.sa_flags = 0 ;
#endif

#if defined SA_FULLDUMP
   if ( SIGABRT != sigNum )
   {
      action.sa_flags |= SA_FULLDUMP ;
   }
#endif

   sigaction(sigNum, &action, NULL) ;

   // SIGABRT will not be re-sent when return, so manually re-raise.
   if ( SIGABRT == sigNum )
   {
      raise( SIGABRT ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSRSTSYSSIG );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSIGHNDABT, "ossSignalHandlerAbort" )
void ossSignalHandlerAbort( OSS_HANDPARMS, const CHAR *dumpDir )
{
   // DO NOT GENERATE CORE FILE AND RESTORE SYSTEM SIGNAL IF IT'S STACK DUMP
   // SIGNAL
   PD_TRACE_ENTRY ( SDB_OSSSIGHNDABT );
   if ( signum != OSS_STACK_DUMP_SIGNAL &&
        signum != OSS_STACK_DUMP_SIGNAL_INTERNAL )
   {
      BOOLEAN genCoreFile = true ;

      ossRestoreSystemSignal( signum, genCoreFile, dumpDir ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSSIGHNDABT );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSFUNCADDR2NM, "ossFuncAddrToName" )
void ossFuncAddrToName( void * address,
                        ossPrimitiveFileOp * trapFile )
{
   int rc = 0 ;
   PD_TRACE_ENTRY ( SDB_OSSFUNCADDR2NM );
   Dl_info dlip ;
   uintptr_t instruction_offset ;
   const CHAR * symbol = NULL ;
   const CHAR * object = NULL ;
   const CHAR * emptyStr = "" ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      if ( NULL == address )
      {
         trapFile->Write( OSS_NEWLINE ) ;
      }
      else
      {
         rc = dladdr( address, &dlip ) ;
         if ( rc )
         {
            // dli_sname contains the name of nearest symbol
            // with address lower than the input address
            symbol = dlip.dli_sname ;
            if ( NULL == symbol )
            {
               symbol = emptyStr ;
            }

            // dli_fname is the pathname of shared object
            // that contains the input address
            object = dlip.dli_fname ;
            if ( NULL == object )
            {
               object = emptyStr ;
            }

            // dli_saddr is the exact address of symbol named in dli_sname
            if ( dlip.dli_saddr )
            {
               instruction_offset = (uintptr_t)address -
                                    (uintptr_t)dlip.dli_saddr ;
               trapFile->fWrite( "%s + 0x%04lx" OSS_NEWLINE "%s (%s)" OSS_NEWLINE,
                                 symbol,
                                 (UINT32_64)instruction_offset,
                                 "                  ",
                                 object ) ;
            }
            else
            {
               // dladdr() could not find the symbol address, so as the symbol
               // name. Print the address and the address( dli_fbase ) that
               // object was loaded. If the dli_fbase is not zero, we may
               // use ( address - dli_fbase ) along with nm command to figure
               // out the symbol and the line of code manually.
               trapFile->fWrite( "address: 0x" OSS_PRIXPTR
                                 " ; dli_fbase: 0x" OSS_PRIXPTR
                                 " ; offset: 0x" OSS_PRIXPTR
                                 " ; (%s)" OSS_NEWLINE,
                                 (UINT32_64)address,
                                 (UINT32_64)dlip.dli_fbase,//addr where obj is loaded
                                 (CHAR *)address - (CHAR *)dlip.dli_fbase,
                                 object ) ;
            }
         }
         else
         {
            // dladdr returns error
            trapFile->fWrite( "address: 0x" OSS_PRIXPTR OSS_NEWLINE, address ) ;
         }
      }
   }
   PD_TRACE_EXIT ( SDB_OSSFUNCADDR2NM );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPSYSTM, "ossDumpSystemTime" )
void ossDumpSystemTime( ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPSYSTM );
   struct tm otm ;
   struct timeval tv ;
   struct timezone tz ;
   time_t tt ;

   // Avoid catching signal to cause deadlock
   ossSignalShield sigShield ;
   sigShield.doNothing() ;

   gettimeofday ( &tv, &tz ) ;
   tt = tv.tv_sec ;
   localtime_r ( &tt, &otm ) ;
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
   PD_TRACE_EXIT ( SDB_OSSDUMPSYSTM );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPDBINFO, "ossDumpDatabaseInfo" )
void ossDumpDatabaseInfo ( ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPDBINFO );
   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      ossSprintVersion( "Version: ", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      trapFile->fWrite( "%s" OSS_NEWLINE, verText ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPDBINFO );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPSYSINFO, "ossDumpSystemInfo" )
void ossDumpSystemInfo( ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPSYSINFO );
   struct utsname name ;

   if ( -1 == uname( &name ) )
   {
      memset( &name, 0, sizeof( name ) ) ;
   }

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      trapFile->fWrite( "System:%s Release:%s Version:%s "
                        "Machine:%s Node:%s" OSS_NEWLINE,
                        name.sysname,
                        name.release,
                        name.version,
                        name.machine,
                        name.nodename ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPSYSINFO );
}

#define OSS_MCODE_LEN 16
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSMCHCODE, "ossMachineCode" )
CHAR *ossMachineCode( UINT32 instruction, CHAR * mCode )
{
   PD_TRACE_ENTRY ( SDB_OSSMCHCODE );
   char mcode[4] = { 0 } ;

   if ( NULL != mCode )
   {
      memcpy( mcode, &instruction, sizeof(mcode) ) ;
      ossSnprintf( mCode, OSS_MCODE_LEN, "%02X%02X%02X%02X",
         0xff & mcode[0], 0xff & mcode[1], 0xff & mcode[2], 0xff & mcode[3]) ;
      PD_TRACE1 ( SDB_OSSMCHCODE, PD_PACK_STRING(mCode) );
   }
   PD_TRACE_EXIT ( SDB_OSSMCHCODE );
   return mCode ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPSIGINFO, "ossDumpSigInfo" )
void ossDumpSigInfo ( oss_siginfo_t  pSigInfo,
                      ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPSIGINFO );
   CHAR buf[64] = { 0 } ;
   CHAR hexNum[10] = { 0 } ;
   CHAR *bytePtr, ch ;
   UINT32 i = 0 ;

   if (    ( NULL != pSigInfo )
        && ( NULL != trapFile )
        && ( trapFile->isValid() ) )
   {
      trapFile->Write( OSS_NEWLINE "Siginfo_t:" OSS_NEWLINE ) ;
      buf[0] = '\0' ;
      bytePtr = ( CHAR * ) pSigInfo ;
      while ( i < sizeof( siginfo_t ) )
      {
         ch = *bytePtr ;
         ossSnprintf( hexNum, sizeof( hexNum ), "%02X", (unsigned char)ch ) ;
         strcat( buf, hexNum ) ;
         bytePtr++ ;
         i++ ;

         if ( i % 16 == 0 )
         {
            trapFile->fWrite( "%s" OSS_NEWLINE, buf ) ;
            buf[0] = '\0' ;
         }
         else if ( i % 4 == 0 )
         {
            strcat( buf, " " ) ;
         }
      }
      if ( 0 != (i % 16) )
      {
         strcat( buf, OSS_NEWLINE ) ;
      }

      trapFile->Write( buf ) ;

      switch (pSigInfo->si_signo)
      {
         case SIGBUS:
         case SIGFPE:
         case SIGILL:
         case SIGSEGV:
            trapFile->fWrite ( "Signal #:%d, si_addr: 0x" OSS_PRIXPTR
                               ", si_code: 0x%08X" OSS_NEWLINE,
                               pSigInfo->si_signo,
                               pSigInfo->si_addr,
                               pSigInfo->si_code) ;
            break ;
         case SIGABRT:
         case SIGINT:
         case SIGPROF:
      #if defined SIGSYS
         case SIGSYS:
      #endif
         case SIGTRAP:
      #if defined SIGURG
         case SIGURG:
      #endif
      #if defined SIGPRE
         case SIGPRE:
      #endif
            trapFile->fWrite ( "Signal #: %d, si_pid: %d, si_uid: %d, "
                               "si_value: %08X" OSS_NEWLINE,
                               pSigInfo->si_signo,
                               pSigInfo->si_pid,
                               pSigInfo->si_uid,
                               pSigInfo->si_value.sival_int ) ;
             break ;
         case SIGCHLD:
            trapFile->fWrite ( "Signal #: %d(SIGCHLD), si_code: %d, "
                               "child_pid: %d, status: 0x%08X" OSS_NEWLINE,
                               pSigInfo->si_signo,
                               pSigInfo->si_code,
                               pSigInfo->si_pid,
                               pSigInfo->si_status ) ;
            break ;
         default :
            trapFile->fWrite ( "Signal #:%d, si_code: %d" OSS_NEWLINE,
                               pSigInfo->si_signo,
                               pSigInfo->si_code ) ;
            break ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPSIGINFO );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSWLKSTK, "ossWalkStack" )
void ossWalkStack ( UINT32 framesToSkip,
                    OSS_INSTRUCTION_PTR * ppInstruction,
                    UINT32 framesRequested )
{
   PD_TRACE_ENTRY ( SDB_OSSWLKSTK );
   void * syms[ OSS_MAX_BACKTRACE_FRAMES_SUPPORTED + 1 ] = { 0 } ;
   UINT32 numFrames, framesOnStk, i ;

   if ( NULL != ppInstruction )
   {
      numFrames = OSS_MIN( framesToSkip + framesRequested,
                           OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ) ;

      framesOnStk = (UINT32_64)backtrace( syms, numFrames ) ;

      for ( i = 0 ; i < framesRequested ; i++ )
      {
         ppInstruction[i] = NULL ;
      }
      for ( i = 0 ; i < ( framesOnStk - framesToSkip ) ; i++ )
      {
         ppInstruction[i] = ( OSS_INSTRUCTION_PTR )syms[ framesToSkip + i ] ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSWLKSTK );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETSYMBNFA, "ossGetSymbolNameFromAddress" )
void ossGetSymbolNameFromAddress( OSS_INSTRUCTION_PTR  pInstruction,
                                  CHAR *pName,
                                  size_t nameSize,
                                  UINT32_64 *pOffset )
{
   PD_TRACE_ENTRY ( SDB_OSSGETSYMBNFA );
   Dl_info dlip ;
   UINT32_64 offset = 0 ;
   BOOLEAN bErr = false ;
   INT32 rc = 0 ;

   if ( ( ! pInstruction ) || ( ! pName ) )
   {
      bErr = true ;
      goto exit ;
   }

   rc = dladdr( (void *)pInstruction, &dlip ) ;

   if ( rc && dlip.dli_sname )
   {
      ossStrncpy( pName, dlip.dli_sname, nameSize ) ;
      if ( dlip.dli_saddr )
      {
         offset = (UINT32_64)pInstruction - (UINT32_64)dlip.dli_saddr ;
      }
   }
   else if ( rc && dlip.dli_fname )
   {
      ossStrncpy( pName, dlip.dli_fname, nameSize ) ;
      offset = (UINT32_64)pInstruction - (UINT32_64)dlip.dli_fbase ;
   }
   else
   {
      bErr = true ;
   }
exit :
   if ( bErr )
   {
      if ( pInstruction )
      {
         ossSnprintf(pName, nameSize, "0x" OSS_PRIXPTR, (UINT32_64)pInstruction) ;
      }
      else
      {
         ossStrncpy (pName, OSS_UNKNOWN_STACKFRAME_NAME, nameSize);
      }
   }

   if ( pOffset )
   {
      *pOffset = offset ;
   }
   PD_TRACE_EXIT ( SDB_OSSGETSYMBNFA );
}
#if defined ( _ARMLIN64 )
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPREGSINFO5, "ossDumpRegistersInfo" )
void ossDumpRegistersInfo( ossSignalContext pContext,
                           ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPREGSINFO5 );
   UINT64 *r = NULL ;
   mcontext_t *mctx = &(pContext->uc_mcontext) ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      if ( NULL != pContext )
      {
         // Ther are 31 general purpose register in arm.
         // Register '31' is one of two registers depending on the instruction
         // context:
         // (1) For instructions dealing with the stack, it is the stack
         //     pointer, named rsp
         // (2) For all other instructions, it is a "zero" register, which
         //     returns 0 when read and discards data when written -- named rzr
         //     (xzr, wzr)
         r = mctx->regs ;
         trapFile->fWrite( "X0 0x%016lx   X1 0x%016lx  X2 0x%016lx  X3 0x%016lx" OSS_NEWLINE
                           "X4 0x%016lx   X5 0x%016lx  X6 0x%016lx  X7 0x%016lx" OSS_NEWLINE
                           "X8 0x%016lx   X9 0x%016lx  X10 0x%016lx X11 0x%016lx" OSS_NEWLINE
                           "X12 0x%016lx  X13 0x%016lx X14 0x%016lx X15 0x%016lx" OSS_NEWLINE
                           "X16 0x%016lx  X17 0x%016lx X18 0x%016lx X19 0x%016lx" OSS_NEWLINE
                           "X20 0x%016lx  X21 0x%016lx X22 0x%016lx X23 0x%016lx" OSS_NEWLINE
                           "X24 0x%016lx  X25 0x%016lx X26 0x%016lx X27 0x%016lx" OSS_NEWLINE
                           "X28 0x%016lx  X29 0x%016lx X30 0x%016lx " OSS_NEWLINE
                           "RSP/XZR 0x%016lx RIP 0x%016lx" OSS_NEWLINE,
                           r[REG_R0],  r[REG_R1],  r[REG_R2],  r[REG_R3],
                           r[REG_R4],  r[REG_R5],  r[REG_R6],  r[REG_R7],
                           r[REG_R8],  r[REG_R9],  r[REG_R10], r[REG_R11],
                           r[REG_R12], r[REG_R13], r[REG_R14], r[REG_R15],
                           r[REG_R16], r[REG_R17], r[REG_R18], r[REG_R19],
                           r[REG_R20], r[REG_R21], r[REG_R22], r[REG_R23],
                           r[REG_R24], r[REG_R25], r[REG_R26], r[REG_R27],
                           r[REG_R28], r[REG_R29], r[REG_R30],
                           mctx->sp, mctx->pc ) ;
      }
      else
      {
         trapFile->Write ("Unable to dump registers" OSS_NEWLINE) ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPREGSINFO5 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPST5, "ossDumpStackTrace" )
void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPST5 );
   void *syms[ OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ] ;
   CHAR  mCode[ OSS_MCODE_LEN ] ;
   mcontext_t *mctx = &( ( ( ossSignalContext)scp )->uc_mcontext ) ;
   UINT64 *rip = (UINT64 *)( mctx->pc ) ;
   UINT64 * const rsp = (UINT64 *)( mctx->sp ) ;
   Dl_info dlip ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      // Dump date
      ossDumpSystemTime ( trapFile ) ;
      // Dump database release
      ossDumpDatabaseInfo ( trapFile ) ;
      // Dump system info
      ossDumpSystemInfo( trapFile ) ;

      // Dump signal info
      ossDumpSigInfo( sigcode, trapFile  ) ;

      // Dump register info
      ossDumpRegistersInfo( ( ossSignalContext )scp, trapFile ) ;

      // Dump the instructions at the point of failure.
      trapFile->Write( OSS_NEWLINE "Point of failure:" OSS_NEWLINE ) ;
      if ( NULL == sigcode )
      {
         trapFile->Write( "Unable to provide disassembly information for "
                          "the point of faliure due to signal info pointer "
                          "is NULL" OSS_NEWLINE ) ;
      }
      else
      {
         if ( sigcode->si_addr != rip )
         {
            // point of failure disassembly info
            trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)rip ) ;
            ossFuncAddrToName( (void *)rip, trapFile ) ;
            trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                              rip,
                              ossMachineCode( *((UINT32*)rip), mCode ) );
            trapFile->fWrite( "%s" OSS_NEWLINE,
                              ossMachineCode( *((UINT32*)( rip+4 )),
                                              mCode ) ) ;

            // Dump stack frames from the point of failure to the bottom of
            // the stack ( actually OSS_MAX_BACKTRACE_FRAMES_SUPPORTED maximum )
            trapFile->Write(
               OSS_NEWLINE"StackTrace:" OSS_NEWLINE
               "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
            INT32 cnt = backtrace( syms, OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ) ;
            for ( INT32 i = 0 ; i < cnt ; i++ )
            {
               trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)syms[i] ) ;
               ossFuncAddrToName( syms[i], trapFile ) ;
            }
         }
         else
         {
            // the signal address ( where falut occurred ) is equal to
            // return address ( rip ) may imply the signal is caused
            // by a call through a bad function pointer, or corrupted
            // return address on the stack.

            // attempt to use rsp here.
            if ( dladdr( (void *)rsp, &dlip ) )
            {
               // point of failure disassembly info
               trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                                 rsp,
                                 ossMachineCode( *((UINT32*)rsp),
                                                 mCode ) ) ;
               trapFile->fWrite( "%s" OSS_NEWLINE,
                                 ossMachineCode( *((UINT32*)(rsp+4)),
                                                 mCode ) ) ;
               trapFile->Write(
                  OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                  "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
               trapFile->fWrite( "0x" OSS_PRIXPTR " [RSP]", rsp ) ;
               ossFuncAddrToName( (void *)rsp, trapFile ) ;
            }
            else
            {
               trapFile->Write( "Signal address is equal to "
                                "instruction pointer( rip )and the valid "
                                "return address could not be determined." OSS_NEWLINE ) ;
               trapFile->Write( OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                                "Unable to provide stack trace info due to "
                                "above reason" OSS_NEWLINE ) ;
            }
            // another thought could be dump raw stack info for
            // advanced users reference.
         }
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPST5 );
}

#elif defined ( _LIN64 )

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPREGSINFO, "ossDumpRegistersInfo" )
void ossDumpRegistersInfo( ossSignalContext pContext,
                           ossPrimitiveFileOp *trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPREGSINFO );
   greg_t * r ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      if ( NULL != pContext )
      {
         r = pContext->uc_mcontext.gregs ;
         trapFile->fWrite( "rax 0x%016lx  rbx 0x%016lx" OSS_NEWLINE
                           "rcx 0x%016lx  rdx 0x%016lx" OSS_NEWLINE
                           "rbp 0x%016lx  rsp 0x%016lx" OSS_NEWLINE
                           "rsi 0x%016lx  rdi 0x%016lx" OSS_NEWLINE
                           "efl 0x%016lx  rip 0x%016lx" OSS_NEWLINE
                           "r8  0x%016lx  r9  0x%016lx" OSS_NEWLINE
                           "r10 0x%016lx  r11 0x%016lx" OSS_NEWLINE
                           "r12 0x%016lx  r13 0x%016lx" OSS_NEWLINE
                           "r14 0x%016lx  r15 0x%016lx" OSS_NEWLINE,
                           r[REG_RAX], r[REG_RBX],
                           r[REG_RCX], r[REG_RDX],
                           r[REG_RBP], r[REG_RSP],
                           r[REG_RSI], r[REG_RDI],
                           r[REG_EFL], r[REG_RIP],
                           r[REG_R8],  r[REG_R9],
                           r[REG_R10], r[REG_R11],
                           r[REG_R12], r[REG_R13],
                           r[REG_R14], r[REG_R15] );
      }
      else
      {
         trapFile->Write ("Unable to dump registers" OSS_NEWLINE) ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPREGSINFO );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPST, "ossDumpStackTrace" )
void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPST );
   void *              syms[ OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ] ;
   CHAR                mCode[ OSS_MCODE_LEN ] ;
   greg_t *            r   = ( ( ossSignalContext)scp )->uc_mcontext.gregs ;
   UINT8 * const       rip = (UINT8 *)r[REG_RIP] ;
   UINT32_64  * const  rsp = (UINT32_64  *)r[REG_RSP] ;
   Dl_info             dlip ;
   INT32               cnt, i ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      // Dump date
      ossDumpSystemTime ( trapFile ) ;
      // Dump database release
      ossDumpDatabaseInfo ( trapFile ) ;
      // Dump system info
      ossDumpSystemInfo( trapFile ) ;

      // Dump signal info
      ossDumpSigInfo( sigcode, trapFile  ) ;

      // Dump register info
      ossDumpRegistersInfo( ( ossSignalContext )scp, trapFile ) ;

      // Dump the instructions at the point of failure.
      trapFile->Write( OSS_NEWLINE "Point of failure:" OSS_NEWLINE ) ;
      if ( NULL == sigcode )
      {
         trapFile->Write( "Unable to provide disassembly information for "
                          "the point of faliure due to signal info pointer "
                          "is NULL" OSS_NEWLINE ) ;
      }
      else
      {
         if ( sigcode->si_addr != rip )
         {
            // point of failure disassembly info
            trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)rip ) ;
            ossFuncAddrToName( (void *)rip, trapFile ) ;
            trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                              rip,
                              ossMachineCode( *((UINT32*)rip), mCode ) );
            trapFile->fWrite( "%s" OSS_NEWLINE,
                              ossMachineCode( *((UINT32*)( rip+4 )),
                                              mCode ) ) ;

            // Dump stack frames from the point of failure to the bottom of
            // the stack ( actually OSS_MAX_BACKTRACE_FRAMES_SUPPORTED maximum )
            trapFile->Write(
               OSS_NEWLINE "StackTrace:" OSS_NEWLINE
               "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
            cnt = backtrace( syms, OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ) ;
            for ( i = 0 ; i < cnt ; i++ )
            {
               trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)syms[i] ) ;
               ossFuncAddrToName( syms[i], trapFile ) ;
            }
         }
         else
         {
            // the signal address ( where falut occurred ) is equal to
            // return address ( rip ) may imply the signal is caused
            // by a call through a bad function pointer, or corrupted
            // return address on the stack.

            // attempt to use rsp here.
            if ( dladdr( (void *)rsp, &dlip ) )
            {
               // point of failure disassembly info
               trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                                 rsp,
                                 ossMachineCode( *((UINT32*)rsp),
                                                 mCode ) ) ;
               trapFile->fWrite( "%s" OSS_NEWLINE,
                                 ossMachineCode( *((UINT32*)(rsp+4)),
                                                 mCode ) ) ;
               trapFile->Write(
                  OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                  "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
               trapFile->fWrite( "0x" OSS_PRIXPTR " [RSP]", rsp ) ;
               ossFuncAddrToName( (void *)rsp, trapFile ) ;
            }
            else
            {
               trapFile->Write( "Signal address is equal to "
                                "instruction pointer( rip )and the valid "
                                "return address could not be determined." OSS_NEWLINE ) ;
               trapFile->Write( OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                                "Unable to provide stack trace info due to "
                                "above reason" OSS_NEWLINE ) ;
            }
            // another thought could be dump raw stack info for
            // advanced users reference.
         }
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPST );
}

#elif defined (_LIN32)

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPREGSINFO3, "ossDumpRegistersInfo" )
void ossDumpRegistersInfo( ossSignalContext pContext,
                           ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPREGSINFO3 );
   greg_t * r ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      if ( NULL != pContext )
      {
         r = pContext->uc_mcontext.gregs ;
         trapFile->fWrite( "Registers:" OSS_NEWLINE
                           "eax %08X  ebx %08X  ecx %08X  edx %08X" OSS_NEWLINE
                           "ebp %08X  esp %08X  edi %08X  esi %08X" OSS_NEWLINE
                           "efl %08X  eip %08X" OSS_NEWLINE
                           "cs %04hx  ss %04hx  ds %04hx  "
                           "ss %04hx  fs %04hx  ds %04hx" OSS_NEWLINE,
                           r[REG_EAX], r[REG_EBX], r[REG_ECX], r[REG_EDX],
                           r[REG_EBP], r[REG_ESP], r[REG_EDI], r[REG_ESI],
                           r[REG_EFL], r[REG_EIP],
                           r[REG_CS], r[REG_SS], r[REG_DS],
                           r[REG_ES], r[REG_FS], r[REG_GS] ) ;
      }
      else
      {
         trapFile->Write (OSS_NEWLINE"Unable to dump registers" OSS_NEWLINE) ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPREGSINFO3 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPST3, "ossDumpStackTrace" )
void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPST3 );
   void *          syms[OSS_MAX_BACKTRACE_FRAMES_SUPPORTED] ;
   CHAR            mCode[OSS_MCODE_LEN] ;
   greg_t *        r   = ((ossSignalContext)scp)->uc_mcontext.gregs ;
   UINT32 * eip = (UINT32 *)r[REG_EIP] ;
   INT32             cnt, i ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      // Dump system info
      ossDumpSystemInfo( trapFile ) ;

      // Dump signal info
      ossDumpSigInfo( sigcode, trapFile  ) ;

      // Dump register info
      ossDumpRegistersInfo( ( ossSignalContext )scp, trapFile ) ;

      // Dump the instructions where trap occurred
      trapFile->Write( OSS_NEWLINE"Point of failure:" OSS_NEWLINE ) ;
      if ( 0 == sigcode )
      {
         trapFile->Write( "Unable to provide disassembly information for "
                          "the point of faliure due to signal info pointer "
                          "is NULL" OSS_NEWLINE ) ;
      }
      else
      {
         if ( eip != sigcode->si_addr )
         {
            trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)eip ) ;
            ossFuncAddrToName(eip, trapFile ) ;
            trapFile->fWrite( "0x" OSS_PRIXPTR " : %s",
                              (UINT32_64)eip,
                              ossMachineCode( *eip, mCode ) ) ;
            trapFile->fWrite( "%s" OSS_NEWLINE, ossMachineCode( *(eip+1 ), mCode ) ) ;

            // Dump stack frames from the point of failure to the bottom of
            // the stack ( actually OSS_MAX_BACKTRACE_FRAMES_SUPPORTED maximum )
            trapFile->Write(
               OSS_NEWLINE"StackTrace:" OSS_NEWLINE
               "-----Address----- ----Function name + Offset---" OSS_NEWLINE) ;
            cnt = backtrace( syms, OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ) ;
            for ( i = 0 ; i < cnt ; i++ )
            {
               trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)syms[i] ) ;
               ossFuncAddrToName( syms[i], trapFile ) ;
            }
         }
         else
         {
            trapFile->Write( "Signal address equal to instruction pointer and "
                            "valid return address could not be determined." OSS_NEWLINE );
            trapFile->Write( OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                             "Unable to provide stack trace info due to "
                             "above reason" OSS_NEWLINE ) ;
         }
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPST3 );
}
#elif defined (_PPCLIN64)
typedef unsigned long greg_t ;
#define REG_NIP  32
#define REG_MSR  33
#define REG_ORIG_R3 34
#define REG_CTR  35
#define REG_LNK  36
#define REG_XER  37
#define REG_CCR  38
#define REG_SOFTE 39
#define REG_TRAP 40
#define REG_DAR  41
#define REG_DSISR 42
#define REG_RESULT 43
#define REG_REGS_COUNT 44
#define REG_FPR0 48      /* each FP reg occupies 2 slots in this space */
#define REG_FPSCR (PT_FPR0 + 32) /* each FP reg occupies 1 slot in 64-bit space */
#define REG_VR0 82       /* each Vector reg occupies 2 slots in 64-bit */
#define REG_VSCR (PT_VR0 + 32*2 + 1)
#define REG_VRSAVE (PT_VR0 + 33*2)


/*
 * Only store first 32 VSRs here. The second 32 VSRs in VR0-31
 */
#define PT_VSR0 150     /* each VSR reg occupies 2 slots in 64-bit */
#define PT_VSR31 (PT_VSR0 + 2*31)

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPREGSINFO4, "ossDumpRegistersInfo" )
void ossDumpRegistersInfo( ossSignalContext pContext,
                           ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPREGSINFO4 );
   greg_t * r ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      if ( NULL != pContext )
      {
         r = pContext->uc_mcontext.gp_regs ;
         trapFile->fWrite( "r0 0x%016lx   r1 0x%016lx  r2 0x%016lx  r3 0x%016lx" OSS_NEWLINE
                           "r4 0x%016lx   r5 0x%016lx  r6 0x%016lx  r7 0x%016lx" OSS_NEWLINE
                           "r8 0x%016lx   r9 0x%016lx  r10 0x%016lx r11 0x%016lx" OSS_NEWLINE
                           "r12 0x%016lx  r13 0x%016lx r14 0x%016lx r15 0x%016lx" OSS_NEWLINE
                           "r16 0x%016lx  r17 0x%016lx r18 0x%016lx r19 0x%016lx" OSS_NEWLINE
                           "r20 0x%016lx  r21 0x%016lx r22 0x%016lx r23 0x%016lx" OSS_NEWLINE
                           "r24 0x%016lx  r25 0x%016lx r26 0x%016lx r27 0x%016lx" OSS_NEWLINE
                           "r28 0x%016lx  r29 0x%016lx r30 0x%016lx r31 0x%016lx" OSS_NEWLINE
                           "f0 0x%016lx   f1 0x%016lx  f2 0x%016lx  f3 0x%016lx" OSS_NEWLINE
                           "f4 0x%016lx   f5 0x%016lx  f6 0x%016lx  f7 0x%016lx" OSS_NEWLINE
                           "f8 0x%016lx   f9 0x%016lx  f10 0x%016lx f11 0x%016lx" OSS_NEWLINE
                           "f12 0x%016lx  f13 0x%016lx f14 0x%016lx f15 0x%016lx" OSS_NEWLINE
                           "f16 0x%016lx  f17 0x%016lx f18 0x%016lx f19 0x%016lx" OSS_NEWLINE
                           "f20 0x%016lx  f21 0x%016lx f22 0x%016lx f23 0x%016lx" OSS_NEWLINE
                           "f24 0x%016lx  f25 0x%016lx f26 0x%016lx f27 0x%016lx" OSS_NEWLINE
                           "f28 0x%016lx  f29 0x%016lx f30 0x%016lx f31 0x%016lx" OSS_NEWLINE
                           "nip 0x%016lx  msr 0x%016lx origr3 0x%016lx ctr 0x%016lx" OSS_NEWLINE
                           "lnk 0x%016lx  xer 0x%016lx ccr 0x%016lx softe 0x%016lx" OSS_NEWLINE
                           "trap 0x%016lx dar 0x%016lx dsisr 0x%016lx result 0x%016lx" OSS_NEWLINE,
                           r[REG_R0],  r[REG_R1],  r[REG_R2],  r[REG_R3],
                           r[REG_R4],  r[REG_R5],  r[REG_R6],  r[REG_R7],
                           r[REG_R8],  r[REG_R9],  r[REG_R10], r[REG_R11],
                           r[REG_R12], r[REG_R13], r[REG_R14], r[REG_R15],
                           r[REG_R16], r[REG_R17], r[REG_R18], r[REG_R19],
                           r[REG_R20], r[REG_R21], r[REG_R22], r[REG_R23],
                           r[REG_R24], r[REG_R25], r[REG_R26], r[REG_R27],
                           r[REG_R28], r[REG_R29], r[REG_R30], r[REG_R31],
                           r[REG_FPR0 + 0],   r[REG_FPR0 + 1],  r[REG_FPR0 + 2],  r[REG_FPR0 + 3],
                           r[REG_FPR0 + 4],   r[REG_FPR0 + 5],  r[REG_FPR0 + 6],  r[REG_FPR0 + 7],
                           r[REG_FPR0 + 8],   r[REG_FPR0 + 9],  r[REG_FPR0 + 10], r[REG_FPR0 + 11],
                           r[REG_FPR0 + 12],  r[REG_FPR0 + 13], r[REG_FPR0 + 14], r[REG_FPR0 + 15],
                           r[REG_FPR0 + 16],  r[REG_FPR0 + 17], r[REG_FPR0 + 18], r[REG_FPR0 + 19],
                           r[REG_FPR0 + 20],  r[REG_FPR0 + 21], r[REG_FPR0 + 22], r[REG_FPR0 + 23],
                           r[REG_FPR0 + 24],  r[REG_FPR0 + 25], r[REG_FPR0 + 26], r[REG_FPR0 + 27],
                           r[REG_FPR0 + 28],  r[REG_FPR0 + 29], r[REG_FPR0 + 30], r[REG_FPR0 + 31],
                           r[REG_NIP], r[REG_MSR], r[REG_ORIG_R3], r[REG_CTR],
                           r[REG_LNK], r[REG_XER], r[REG_CCR], r[REG_SOFTE],
                           r[REG_TRAP], r[REG_DAR], r[REG_DSISR], r[REG_RESULT] ) ;
      }
      else
      {
         trapFile->Write ("Unable to dump registers" OSS_NEWLINE) ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPREGSINFO4 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPST4, "ossDumpStackTrace" )
void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile )
{
   PD_TRACE_ENTRY ( SDB_OSSDUMPST4 );
   void *              syms[ OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ] ;
   CHAR                mCode[ OSS_MCODE_LEN ] ;
   greg_t *            r   = ( ( ossSignalContext)scp )->uc_mcontext.gp_regs ;
   UINT8 * const       rip = (UINT8 *)r[REG_NIP] ;
   UINT32_64  * const  rsp = (UINT32_64  *)r[REG_R1] ;
   Dl_info             dlip ;
   INT32               cnt, i ;

   if ( ( NULL != trapFile ) && trapFile->isValid() )
   {
      // Dump date
      ossDumpSystemTime ( trapFile ) ;
      // Dump database release
      ossDumpDatabaseInfo ( trapFile ) ;
      // Dump system info
      ossDumpSystemInfo( trapFile ) ;

      // Dump signal info
      ossDumpSigInfo( sigcode, trapFile  ) ;

      // Dump register info
      ossDumpRegistersInfo( ( ossSignalContext )scp, trapFile ) ;

      // Dump the instructions at the point of failure.
      trapFile->Write( OSS_NEWLINE "Point of failure:" OSS_NEWLINE ) ;
      if ( NULL == sigcode )
      {
         trapFile->Write( "Unable to provide disassembly information for "
                          "the point of faliure due to signal info pointer "
                          "is NULL" OSS_NEWLINE ) ;
      }
      else
      {
         if ( sigcode->si_addr != rip )
         {
            // point of failure disassembly info
            trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)rip ) ;
            ossFuncAddrToName( (void *)rip, trapFile ) ;
            trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                              rip,
                              ossMachineCode( *((UINT32*)rip), mCode ) );
            trapFile->fWrite( "%s" OSS_NEWLINE,
                              ossMachineCode( *((UINT32*)( rip+4 )),
                                              mCode ) ) ;

            // Dump stack frames from the point of failure to the bottom of
            // the stack ( actually OSS_MAX_BACKTRACE_FRAMES_SUPPORTED maximum )
            trapFile->Write(
               OSS_NEWLINE"StackTrace:" OSS_NEWLINE
               "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
            cnt = backtrace( syms, OSS_MAX_BACKTRACE_FRAMES_SUPPORTED ) ;
            for ( i = 0 ; i < cnt ; i++ )
            {
               trapFile->fWrite( "0x" OSS_PRIXPTR " ", (UINT32_64)syms[i] ) ;
               ossFuncAddrToName( syms[i], trapFile ) ;
            }
         }
         else
         {
            // the signal address ( where falut occurred ) is equal to
            // return address ( rip ) may imply the signal is caused
            // by a call through a bad function pointer, or corrupted
            // return address on the stack.

            // attempt to use rsp here.
            if ( dladdr( (void *)rsp, &dlip ) )
            {
               // point of failure disassembly info
               trapFile->fWrite( OSS_NEWLINE"0x" OSS_PRIXPTR " : %s",
                                 rsp,
                                 ossMachineCode( *((UINT32*)rsp),
                                                 mCode ) ) ;
               trapFile->fWrite( "%s" OSS_NEWLINE,
                                 ossMachineCode( *((UINT32*)(rsp+4)),
                                                 mCode ) ) ;
               trapFile->Write(
                  OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                  "-----Address----- ----Function name + Offset---" OSS_NEWLINE);
               trapFile->fWrite( "0x" OSS_PRIXPTR " [RSP]", rsp ) ;
               ossFuncAddrToName( (void *)rsp, trapFile ) ;
            }
            else
            {
               trapFile->Write( "Signal address is equal to "
                                "instruction pointer( rip )and the valid "
                                "return address could not be determined." OSS_NEWLINE ) ;
               trapFile->Write( OSS_NEWLINE"StackTrace:" OSS_NEWLINE
                                "Unable to provide stack trace info due to "
                                "above reason" OSS_NEWLINE ) ;
            }
            // another thought could be dump raw stack info for
            // advanced users reference.
         }
      }
   }
   PD_TRACE_EXIT ( SDB_OSSDUMPST4 );
}
#elif defined ( _ALPHALIN64 )
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPREGSINFO6, "ossDumpRegistersInfo" )
void ossDumpRegistersInfo( ossSignalContext pContext,
                           ossPrimitiveFileOp * trapFile )
{
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDUMPST6, "ossDumpStackTrace" )
void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile )
{
}

#endif

#endif  // if defined ( _LINUX )

#if defined (_WINDOWS)
#include "ossLatch.hpp"
#include <windows.h>
#include <dbgHelp.h>

#if defined( _M_IA64 )
   #define OSS_THIS_IMAGE_MACHINE_TYPE IMAGE_FILE_MACHINE_IA64
#elif defined( _M_AMD64 )
   #define OSS_THIS_IMAGE_MACHINE_TYPE IMAGE_FILE_MACHINE_AMD64
#else
   #define OSS_THIS_IMAGE_MACHINE_TYPE IMAGE_FILE_MACHINE_I386
#endif

// DbgHelp functions, such as SymInitialize, are single threaded.
// Therefore, calls from more than one thread to this function will likely
// result in unexpected behavior or memory corruption. To avoid this,
// call SymInitialize only when your process starts and SymCleanup only
// when your process ends. It is not necessary for each thread in the process
// to call these functions.
enum SYS_MUTEX_TYPE
{
   _SymInitialize = 0,
   _SymFromAddr,
   _SymGetLineFromAddr64,
   _StackWalk64,
   _SymCleanup,
   _NumOfFunctions
} ;

HANDLE ossGetSysMutexHandle( SYS_MUTEX_TYPE type )
{
   static HANDLE s_sysMutexes[ _NumOfFunctions ] = {0} ;
   static BOOLEAN s_init = FALSE ;
   static ossSpinXLatch s_latch ;

   // init sys mutex
   if ( FALSE == s_init )
   {
      s_latch.get() ;
      if ( FALSE == s_init )
      {
         for ( int i = _SymInitialize; i < _NumOfFunctions ; i++ )
         {
             if ( ! s_sysMutexes[i] )
             {
                if ( i != _SymCleanup )
                {
                   s_sysMutexes[i] = CreateMutex( NULL,    // default security attr
                                                  false,   // initially not owned
                                                  NULL ) ; // unnamed mutex
                }
                else
                {
                   s_sysMutexes[i] = s_sysMutexes[ _SymInitialize ] ;
                }
             }
         }
         s_init = TRUE ;
      }
      s_latch.release() ;
   }
   // get handle
   if ( type >= _SymInitialize && type < _NumOfFunctions )
   {
      return s_sysMutexes[ (INT32)type ] ;
   }
   SDB_ASSERT( FALSE, "Invalid sys mutex type" ) ;
   return 0 ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSSYMINIT, "ossSymInitialize" )
UINT32 ossSymInitialize( HANDLE phProcess,
                         CHAR *  pUserSearchPath,
                         BOOLEAN bInvadeProcess )
{
   PD_TRACE_ENTRY ( SDB_OSSSYMINIT );
   static BOOLEAN  s_bSymInitialized = false ;
   UINT32 rc = ERROR_SUCCESS ;

   WaitForSingleObject( ossGetSysMutexHandle( _SymInitialize ), INFINITE ) ;
   if ( ! s_bSymInitialized )
   {
      if ( SymInitialize( phProcess, pUserSearchPath, bInvadeProcess ) )
      {
         s_bSymInitialized = TRUE ;
      }
      else
      {
         rc = GetLastError() ;
      }
   }
   ReleaseMutex ( ossGetSysMutexHandle( _SymInitialize ) ) ;

   if ( ERROR_SUCCESS != rc )
   {
      SetLastError( rc ) ;
   }
   PD_TRACE1 ( SDB_OSSSYMINIT, PD_PACK_UINT(rc) );
   PD_TRACE_EXIT ( SDB_OSSSYMINIT );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSWKSEX, "ossWalkStackEx" )
UINT32 ossWalkStackEx( LPEXCEPTION_POINTERS lpEP,
                       UINT32 framesToSkip,
                       UINT32 framesRequested ,
                       void ** ppInstruction )
{
   PD_TRACE_ENTRY ( SDB_OSSWKSEX );
   HANDLE       hProcess   = GetCurrentProcess() ;
   HANDLE       hThread    = GetCurrentThread() ;

   STACKFRAME64 stackFrame = { 0 } ;
   CONTEXT      cContext   = { 0 } ;
   PCONTEXT     pContext   = &cContext ;
   BOOLEAN      bSuccess   = true ;

   UINT32 numFrames, framesOnStk ;

   framesOnStk = 0 ;

   if ( NULL != ppInstruction )
   {
      numFrames = framesRequested ;

      if ( OSS_MAX_BACKTRACE_FRAMES_SUPPORTED <= ( framesToSkip + numFrames ) )
      {
         numFrames = OSS_MAX_BACKTRACE_FRAMES_SUPPORTED - framesToSkip -1 ;
      }

      if ( NULL == lpEP )
      {
         framesOnStk = CaptureStackBackTrace( framesToSkip,
                                              numFrames, ppInstruction, NULL ) ;
      }
      else
      {
         // At windows platform( 32bit ), exceptions are handled in a separate
         // context with a smaller stack. The real context and exception record
         // of the failure are passed to the exception handler as an input
         // parameter. Microsoft switched the context of the thread
         // and stored the exception context pointer inside the exception
         // pointer (the param that was passed to our exception handler ).
         // So, we can get the call chain for the exception by access
         // the exception context pointer and walk through the context record.
      #ifndef _WIN64
         stackFrame.AddrPC.Offset    = lpEP->ContextRecord->Eip ;
         stackFrame.AddrPC.Mode      = AddrModeFlat ;
         stackFrame.AddrStack.Offset = lpEP->ContextRecord->Esp ;
         stackFrame.AddrStack.Mode   = AddrModeFlat ;
         stackFrame.AddrFrame.Offset = lpEP->ContextRecord->Ebp ;
         stackFrame.AddrFrame.Mode   = AddrModeFlat ;
         ossMemcpy( pContext, lpEP->ContextRecord, sizeof( CONTEXT ) ) ;
      #else
         RtlCaptureContext( pContext ) ;
      #endif

         bSuccess = true ;
         WaitForSingleObject( ossGetSysMutexHandle( _StackWalk64 ), INFINITE ) ;
         while ( bSuccess && framesOnStk < numFrames )
         {
            bSuccess = StackWalk64( OSS_THIS_IMAGE_MACHINE_TYPE,
                                    hProcess,
                                    hThread,
                                    &stackFrame,
                                    pContext,
                                    NULL,
                                    SymFunctionTableAccess64,
                                    SymGetModuleBase64,
                                    NULL ) ;
            if ( bSuccess )
            {
               ppInstruction[ framesOnStk ] = (void*)stackFrame.AddrPC.Offset ;
            }
            else
            {
               ppInstruction[ framesOnStk ] = 0 ;
            }
            framesOnStk++ ;
         }
         ReleaseMutex ( ossGetSysMutexHandle( _StackWalk64 ) ) ;
      }
   }
   PD_TRACE_EXIT ( SDB_OSSWKSEX );
   return framesOnStk ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSWS, "ossWalkStack" )
UINT32 ossWalkStack( UINT32 framesToSkip,
                     UINT32 framesRequested,
                     void ** ppInstruction )
{
   PD_TRACE_ENTRY ( SDB_OSSWS );
   UINT32 numFrames, framesOnStk ;

   framesOnStk = 0 ;
   if ( NULL != ppInstruction )
   {
      numFrames = framesRequested ;

      if ( OSS_MAX_BACKTRACE_FRAMES_SUPPORTED <= ( framesToSkip + numFrames ) )
      {
         numFrames = OSS_MAX_BACKTRACE_FRAMES_SUPPORTED - framesToSkip -1 ;
      }
      framesOnStk = CaptureStackBackTrace( framesToSkip,
                                           numFrames, ppInstruction, NULL ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSWS );
   return framesOnStk ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSGETSYMBNFADDR, "ossGetSymbolNameFromAddress" )
void ossGetSymbolNameFromAddress( HANDLE hProcess,
                                  UINT64 pInstruction,
                                  SYMBOL_INFO * pSymbol,
                                  CHAR *pName,
                                  UINT32 nameSize )
{
   PD_TRACE_ENTRY ( SDB_OSSGETSYMBNFADDR );
   SYMBOL_INFO * symbol = NULL ;
   UINT64 displacement = 0 ;
   UINT32 dwDisplacement = 0 ;
   IMAGEHLP_LINE64 line = { 0 } ;
   UINT32 strLen = 0 ;

   if ( ( NULL != pInstruction ) & ( NULL != pSymbol ) )
   {
      line.SizeOfStruct = sizeof(IMAGEHLP_LINE64) ;
      WaitForSingleObject( ossGetSysMutexHandle( _SymFromAddr ), INFINITE ) ;
      if ( SymFromAddr( hProcess, pInstruction, &displacement, pSymbol ) )
      {
         strLen += ossSnprintf( pName, nameSize,
                                "0x%0llX %s", pInstruction, pSymbol->Name ) ;
         if ( SymGetLineFromAddr64( hProcess,
                                    pInstruction,
                                   (PDWORD)(&dwDisplacement), &line ) )
         {
            strLen += ossSnprintf( pName + strLen, nameSize - strLen,
                                   " %s [%d]", line.FileName, line.LineNumber );
         }
         else
         {
            strLen += ossSnprintf( pName + strLen, nameSize - strLen,
                                   OSS_UNKNOWN_STACKFRAME_NAME ) ;
         }
      }
      ReleaseMutex ( ossGetSysMutexHandle( _SymFromAddr ) ) ;
   }
   PD_TRACE_EXIT ( SDB_OSSGETSYMBNFADDR );
}

#endif // _WINDOWS

