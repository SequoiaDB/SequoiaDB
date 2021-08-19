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

   Source File Name = ossStackDump.hpp

   Descriptive Name = Operating System Services Stack Dump Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions to rewind windows
   and linux stacks.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_STACK_DUMP_HPP
#define OSS_STACK_DUMP_HPP

#include "ossFeat.hpp"
#include "ossTypes.hpp"

#define OSS_UNKNOWN_STACKFRAME_NAME "?unknown"
#define OSS_FUNC_NAME_LEN_MAX 1024

typedef char OSS_INSTRUCTION ;
typedef const OSS_INSTRUCTION * OSS_INSTRUCTION_PTR ;

class ossPrimitiveFileOp ;

#if defined (_LINUX)
   #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 128
#elif defined (_WINDOWS)
   #if defined (_WINDOWS32)
       #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 62
   #elif defined(_WINDOWS64)
       #define OSS_MAX_BACKTRACE_FRAMES_SUPPORTED 128
   #endif
#endif

#if defined (_LINUX)

   #include "ossSignal.hpp"

   void ossDumpStackTrace( OSS_HANDPARMS, ossPrimitiveFileOp * trapFile ) ;

   void ossWalkStack ( UINT32 framesToSkip,
                       OSS_INSTRUCTION_PTR * ppInstruction,
                       UINT32 framesRequested ) ;

   void ossGetSymbolNameFromAddress ( OSS_INSTRUCTION_PTR pInstruction,
                                      CHAR * pName,
                                      size_t nameSize,
                                      UINT32_64 *pOffset ) ;

   void ossRestoreSystemSignal( const INT32 sigNum,
                                const BOOLEAN isCoreNeeded,
                                const CHAR *dumpDir ) ;

   void ossSignalHandlerAbort( OSS_HANDPARMS, const CHAR *dumpDir ) ;
#elif defined (_WINDOWS)
   #include <windows.h>
   #include <dbgHelp.h>

   UINT32 ossWalkStack ( UINT32 framesToSkip,
                         UINT32 framesRequested,
                         void ** ppInstruction ) ;

   UINT32 ossWalkStackEx( LPEXCEPTION_POINTERS lpEP,
                          UINT32 framesToSkip,
                          UINT32 framesRequested ,
                          void ** ppInstruction ) ;

   void ossGetSymbolNameFromAddress( HANDLE hProcess,
                                     UINT64 pInstruction,
                                     SYMBOL_INFO * pSymbol,
                                     CHAR * pName,
                                     UINT32 nameSize ) ;
   UINT32 ossSymInitialize( HANDLE  phProcess,
                            PSTR    pUserSearchPath,
                            BOOLEAN bInvadeProcess ) ;
#endif  // _LINUX

#endif  // OSS_STACK_DUMP_HPP

