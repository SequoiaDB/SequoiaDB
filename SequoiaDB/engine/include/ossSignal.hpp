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

   Source File Name = ossSignal.hpp

   Descriptive Name = Operating System Services Signal Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains structure and functions for
   signal processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SIGNAL_HPP
#define OSS_SIGNAL_HPP

#include <setjmp.h>

#if defined (_LINUX)
   #include <signal.h>
   #include <errno.h>
   #include <sys/ucontext.h>
   #include <sys/resource.h>
   #include <pthread.h>
   // stack dump signal for linux: 23
   #define OSS_STACK_DUMP_SIGNAL SIGURG
   #define OSS_STACK_DUMP_SIGNAL_INTERNAL SIGUSR1

   #define OSS_TEST_SIGNAL                35       /// SIGRTMIN+1
   #define OSS_INTERNAL_TEST_SIGNAL       36       /// SIGRTMIN+2

   #define OSS_FREEZE_SIGNAL              37       /// SIGRTMIN+3
   #define OSS_FREEZE_SIGNAL_INTERNAL     38       /// SIGRTMIN+4

   #define OSS_MEM_DUMP_SIGNAL            39       /// SIGTTMIN+5
   #define OSS_MEM_DUMP_SIGNAL_INTERNAL   40       /// SIGTTMIN+6

   #define OSS_MEM_TRIM_SIGNAL            41       /// SIGTTMIN+7
   #define OSS_MEM_TRIM_SIGNAL_INTERNAL   42       /// SIGTTMIN+8

   typedef ucontext_t * ossSignalContext ;

   /* Set Handler for Signal */
   typedef siginfo_t *           oss_siginfo_t ;

   #define OSS_HANDPARMS         int signum, oss_siginfo_t sigcode, void * scp
   #define OSS_HANDARGS_DUMMY    0, 0, 0
   #define OSS_HANDARGS          signum, sigcode, scp

#elif defined (_WINDOWS)
   #include <windows.h>

   #define OSS_HANDPARMS         unsigned long signum, \
                                 PEXCEPTION_RECORD pExceptionRecord, \
                                 PCONTEXT          pContextRecord
   #define OSS_HANDARGS_DUMMY    0, 0, 0
   #define OSS_HANDARGS          signum, pExceptionRecord, pContextRecord
#endif  // ifdef _LINUX



/*
 * Wrapper macros for sigsetjmp and siglongjmp
 *     ossSetJump
 *     ossLongJump
 */
#if defined (_LINUX)
   #define ossLongJump(jmpBuf, arg) siglongjmp( jmpBuf, arg )
   #define ossSetJump( jb, arg) sigsetjmp ( jb, arg )
#elif defined (_WINDOWS)
   #define ossLongJump(jmpBuf, arg) longjmp( jmpNuf, arg )
   #define sqloSetJump( jb, arg) setjmp ( jb )
#else
   #error Undefined operating system
#endif

typedef void (* OSS_SIGFUNCPTR)(OSS_HANDPARMS) ;

#endif // OSS_SIGNAL_HPP
