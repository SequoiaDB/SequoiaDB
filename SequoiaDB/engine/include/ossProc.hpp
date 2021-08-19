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

   Source File Name = ossProc.hpp

   Descriptive Name = Operating System Services Process Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for process op.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/24/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSPROC_HPP__
#define OSSPROC_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossNPipe.hpp"
#include <sys/types.h>
#include <list>
#include <vector>
#include <string>

#if defined (_LINUX)
#define PROC_SELF_EXE                  "/proc/self/exe"
#define PROC_STATUS_ZOMBIE             'Z'
#endif
#define OSS_EXECV_CAST                 char*const*
// define ossExec execute flags
#define OSS_EXEC_INHERIT_HANDLES       1 // inherit fd/handles in new process
#define OSS_EXEC_SSAVE                 2 // sync process, return result
#define OSS_EXEC_NORESIZEARGV          4 // not resize buffer for argv for rename
#define OSS_EXEC_NODETACHED            8 // not detached

// define term code
#define OSS_EXIT_NORMAL 0
#define OSS_EXIT_ERROR  1
#define OSS_EXIT_TRAP   2
#define OSS_EXIT_KILL   3

/*
   _ossResultCode define
*/
class _ossResultCode : public SDBObject
{
public :
   UINT32 termcode ;
   UINT32 exitcode ;
} ;
typedef class _ossResultCode ossResultCode ;

class _ossIExecHandle
{
public:
   virtual ~_ossIExecHandle() {}
public:
   virtual void  handleInOutPipe( OSSPID pid,
                                  OSSNPIPE * const npHandleStdin,
                                  OSSNPIPE * const npHandleStdout ) = 0 ;
} ;
typedef _ossIExecHandle ossIExecHandle ;

/**
   \brief Exec a new program.

   This call allows a process to request start up as a child process.
   The program can run as a normal process or as a daemon

   \param program [in]
      Null terminated name of the file that contains
      the program to be executed.

   \param arguments [in]
      A set of argument strings passed to the new process.
      These strings represent "command parameters" for the program.
      \li Strings are delimited by null characters.
      \li Two adjacent null characters mark the end of the set.
      \li By convention, the first string is the name of the program.
      \li If arguments are NULL, the program is called with no parameters.

   \param environment [in]
      A set of environment strings passed to the new process.
      These strings convey configuration parameters and represent the
      combination of the current value of all "set symbols" or "environment
      variables" for the current program.
      \li Strings are delimited by null characters.
      \li Two adjacent null characters mark the end of the set.
      \li If enviornment is NULL, the child process is given the environment
          of the current process.

   \param flag [in]
      An indicator that shows how the requested program is to execute:
      \li OSS_EXEC_INHERIT_HANDLES - child process inherit parent's handles
          [UNIX/NT]
      \li OSS_EXEC_SSAVE - synchronously as a normal child process [UNIX/NT]

   \param pid [out]
      A reference to DWORD[NT] or pid_t[UNIX] of the newly created process

   \param result [out]
      The exit status of a synchronously executed child process (only
      applicable with OSS_EXEC_SSAVE flag option).

*/
INT32 ossExec ( const CHAR * program,
                const CHAR * arguments,
                const CHAR * environment,
                INT32 flag,
                OSSPID &pid,
                ossResultCode &result,
                OSSNPIPE * const npHandleStdin,
                OSSNPIPE * const npHandleStdout,
                ossIExecHandle *pHandle = NULL,
                OSSHANDLE *pProcessHandle = NULL ) ;

BOOLEAN  ossIsProcessRunning ( OSSPID pid ) ;

void     ossCloseProcessHandle( OSSHANDLE &handle ) ;

INT32    ossGetExitCodeProcess( OSSHANDLE handle, UINT32 &exitCode ) ;

/*
   get excutable file's working directory
*/
INT32    ossGetEWD ( CHAR *pBuffer, INT32 maxlen ) ;

INT32    ossTerminateProcess( const OSSPID &pid, BOOLEAN force = FALSE ) ;

INT32    ossBuildArguments ( CHAR **pArgumentBuffer,
                             INT32 &buffSize,
                             std::list<const CHAR*> &argv ) ;

INT32    ossStartProcess( std::list<const CHAR*> &argv,
                          OSSPID &pid, INT32 flag = 0,
                          ossResultCode *pRetCode = NULL,
                          OSSHANDLE *pProcessHandle = NULL ) ;

#if defined (_WINDOWS)

INT32 ossWaitInterrupt ( HANDLE handle, DWORD timeout ) ;

INT32 ossStartService ( const CHAR *serviceName ) ;
INT32 ossStopService( const CHAR *serviceName,
                      DWORD dwMilliseconds ) ;

#elif defined (_LINUX)

/*
   It calls waitpid until the given pid stop
   When the pid is still running, will return SDB_TIMEOUT
*/
INT32 ossWaitChild ( OSSPID pid, ossResultCode &result,
                     BOOLEAN block = TRUE ) ;
void  ossEnableNameChanges ( const INT32 argc, CHAR **pArgv0 ) ;
void  ossRenameProcess (  const CHAR *pNewName ) ;
INT32 ossVerifyPID ( OSSPID inputpid, const CHAR *processName,
                     const CHAR *promptName = NULL ) ;

#endif // _WINDOWS

struct _ossProcInfo
{
   std::string       _procName ;
   OSSPID            _pid ;
} ;
typedef _ossProcInfo ossProcInfo ;

INT32 ossEnumProcesses( std::vector< ossProcInfo > &procs,
                        const CHAR *pNameFilter,
                        BOOLEAN matchWhole = TRUE,
                        BOOLEAN findOne = FALSE ) ;

OSSUID ossGetCurrentProcessUID() ;
OSSGID ossGetCurrentProcessGID() ;
INT32 ossSetCurrentProcessUID( OSSUID uid ) ;
INT32 ossSetCurrentProcessGID( OSSGID gid ) ;

#endif // OSSPROC_HPP__

