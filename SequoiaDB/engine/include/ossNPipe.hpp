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

   Source File Name = ossNPipe.hpp

   Descriptive Name = Operating System Services Named Pipe Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for NPIPE operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/25/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSSNPIPE_HPP__
#define OSSNPIPE_HPP__
#include "core.hpp"
#include "oss.hpp"
#include <vector>
#include <string>

#if defined (_WINDOWS)
   #define  ossPopen(cmd,mode)         _popen(cmd,mode)
   #define  ossPclose(file)            _pclose(file)
#else
   #define  ossPopen(cmd,mode)         popen(cmd,mode)
   #define  ossPclose(file)            pclose(file)
#endif // _WINDOWS

/*
 * Steps to create OSS named pipe
 * Server
 * 1) ossCreateNamedPipe                  -- return immediate
 * 2) ossConnectNamedPipe                 -- linux return immediate, windows
 *                                           will wait until client open pipe
 * 3) ossReadNamedPipe / ossWriteNamedPipe
 * 4) ossDisconnectNamedPipe
 * 6) ossDeleteNamedPipe
 * Client
 * 1) ossOpenNamedPipe
 * 2) ossWriteNamedPipe / ossReadNamedPipe
 * 3) ossCloseNamedPipe
 *
 * Create     = ossCreateNamedPipe     -- server
 * Connect    = ossConnectNamedPipe    -- server
 * Open       = ossOpenNamedPipe       -- client
 * Read       = ossReadNamedPipe       -- both
 * Write      = ossWriteNamedPipe      -- both
 * Disconnect = ossDisconnectNamedPipe -- server
 * Close      = ossCloseNamedPipe      -- client
 * Delete     = ossDeleteNamedPipe     -- server
 *
 */

#define OSS_NPIPE_MAX_NAME_LEN            255
class _OSSNPIPE : public SDBObject
{
public :
#if defined (_WINDOWS)
   HANDLE _handle ;
   INT32 _overlappedFlag ;
   OVERLAPPED _overlapped ;
#elif defined (_LINUX)
   INT32 _handle ;
   INT32 _bufSize ;
#endif
   UINT32 _state ;
   CHAR   _name [ OSS_NPIPE_MAX_NAME_LEN + 1 ] ;

   _OSSNPIPE()
   {
#if defined (_WINDOWS)
      _handle = INVALID_HANDLE_VALUE ;
      _overlapped.hEvent = NULL ;
      _overlappedFlag = 0 ;
#elif defined (_LINUX)
      _handle = -1 ;
#endif
   }
} ;
typedef class _OSSNPIPE OSSNPIPE ;

#if defined (_WINDOWS)
#define OSS_NPIPE_PREFIX              "\\\\"
#define OSS_NPIPE_LOCAL_PREFIX        OSS_NPIPE_PREFIX".\\pipe\\"
#else
#define OSS_NPIPE_LOCAL_PREFIX        "/var/sequoiadb"
#endif

#define OSS_NPIPE_INBOUND            0x00000001
#define OSS_NPIPE_OUTBOUND           0x00000002
#define OSS_NPIPE_DUPLEX             (OSS_NPIPE_INBOUND|OSS_NPIPE_OUTBOUND)

#define OSS_NPIPE_NONBLOCK           0x00000010
#define OSS_NPIPE_BLOCK              0x00000000 // by default it's blocking pipe
#define OSS_NPIPE_BLOCK_WITH_TIMEOUT 0x00000020

#define OSS_NPIPE_INFINITE_TIMEOUT   -1

#if defined (_WINDOWS)
#define OSS_NPIPE_UNLIMITED_INSTANCES      PIPE_UNLIMITED_INSTANCES
#define OSS_NPIPE_FIRST_PIPE_INSTANCE      FILE_FLAG_FIRST_PIPE_INSTANCE
#define OSS_NPIPE_NOWAIT                   PIPE_NOWAIT
#define OSS_NPIPE_OVERLAP_ENABLED          0x00000001
// this bit is set by internal, indicating the pipe is waiting for IO
#define OSS_NPIPE_OVERLAP_IOPENDING        0x00000002
#elif defined (_LINUX)
#define OSS_NPIPE_UNLIMITED_INSTANCES      0
#endif

INT32 ossCreateNamedPipe ( const CHAR *name,
                           UINT32 inboundBufferSize,  // not valid on linux
                           UINT32 outboundBufferSize, // not valid on linux
                           // bitwise OR of
                           // OSS_NPIPE_INBOUND
                           // OSS_NPIPE_OUTBOUND
                           // OSS_NPIPE_DUPLEX
                           // OSS_NPIPE_NONBLOCK
                           // OSS_NPIPE_BLOCK_WITH_TIMEOUT
                           // OSS_NPIPE_BLOCK
                           UINT32 action,
                           UINT32 numInstances,       // ignore on linux
                           // unit: second
                           // can be OSS_NPIPE_INFINITE_TIMEOUT
                           SINT32 defaultTimeout,     // ms
                           OSSNPIPE &handle,
                           const CHAR *pRootPath = OSS_NPIPE_LOCAL_PREFIX ) ;

INT32 ossOpenNamedPipe ( const CHAR *name,
                         UINT32 action,
                         SINT32 openTimeout, // ignore on linux, ms
                         OSSNPIPE &handle,
                         const CHAR *pRootPath = OSS_NPIPE_LOCAL_PREFIX ) ;

INT32 ossConnectNamedPipe ( OSSNPIPE &handle,
                            UINT32 action,
                            // timeout is not used in linux
                            SINT32 timeout = OSS_NPIPE_INFINITE_TIMEOUT ) ;

INT32 ossReadNamedPipe ( OSSNPIPE &handle,
                         CHAR *pBuffer,
                         INT64 bufSize,
                         INT64 *bufRead,
                         SINT32 timeout = OSS_NPIPE_INFINITE_TIMEOUT ) ; // ms

INT32 ossWriteNamedPipe ( OSSNPIPE &handle,
                          const CHAR *pBuffer,
                          INT64 bufSize,
                          INT64 *bufWrite ) ;

INT32 ossDisconnectNamedPipe ( OSSNPIPE &handle ) ;

INT32 ossCloseNamedPipe ( OSSNPIPE &handle ) ;

INT32 ossDeleteNamedPipe ( OSSNPIPE &handle ) ;

// intends to be used when one thread want to kill the whole program
// and clean up.
INT32 ossCleanNamedPipeByName ( const CHAR * pipeName,
                                const CHAR *pRootPath = OSS_NPIPE_LOCAL_PREFIX ) ;

// convert named pipe to C file descriptor
INT32 ossNamedPipeToFd ( OSSNPIPE &handle , INT32 * fd ) ;

// enumate all named pipes that EXACT matches pattern
// if pattern is NULL, the call will enumerate all pipes in the system
// For example if we are looking for name "sequoiadb_engine_50000",
// then pattern will be "sequoiadb_engine_50000", any other pipe name
// will not match it.
// Users can check the size of "names" to verify whether the given pipe
// name exists in the system
INT32 ossEnumNamedPipes ( std::vector<std::string> &names,
                          const CHAR *pattern = NULL,
                          OSS_MATCH_TYPE type = OSS_MATCH_ALL,
                          const CHAR *rootPath = OSS_NPIPE_LOCAL_PREFIX ) ;

#endif

