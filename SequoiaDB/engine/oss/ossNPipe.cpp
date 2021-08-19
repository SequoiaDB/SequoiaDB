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

   Source File Name = ossNPipe.cpp

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

#include "core.hpp"
#include "ossNPipe.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "ossProc.hpp"
#include "ossIO.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined (_LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#else
#include <io.h>
#include <fcntl.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;

using namespace std ;

#if defined (_WINDOWS)
#define FileDirectoryInformation 1
#define STATUS_NO_MORE_FILES 0x80000006L
#define STATUS_NO_SUCH_FILE  0xC000000FL
typedef struct
{
   USHORT Length ;
   USHORT MaximumLength ;
   PWSTR Buffer ;
} UNICODE_STRING, *PUNICODE_STRING ;

typedef struct
{
   union
   {
      NTSTATUS Status ;
      PVOID Pointer ;
   } ;
   ULONG_PTR Information ;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK ;

typedef struct
{
   ULONG NextEntryOffset ;
   ULONG FileIndex ;
   LARGE_INTEGER CreationTime ;
   LARGE_INTEGER LastAccessTime ;
   LARGE_INTEGER LastWriteTime ;
   LARGE_INTEGER ChangeTime ;
   LARGE_INTEGER EndOfFile ;
   LARGE_INTEGER AllocationSize ;
   ULONG FileAttributes ;
   ULONG FileNameLength ;
   union
   {
      struct
      {
         WCHAR FileName[1] ;
      } FileDirectoryInformationClass ;
      struct
      {
         DWORD dwUknown1 ;
         WCHAR FileName[1] ;
      } FileFullDirectoryInformationClass ;
      struct
      {
         DWORD dwUknown2 ;
         USHORT AltFileNameLen ;
         WCHAR AltFileName[12] ;
         WCHAR FileName[1] ;
      } FileBothDirectoryInformationClass ;
   } ;
} FILE_QUERY_DIRECTORY, *PFILE_QUERY_DIRECTORY ;

// ntdll!NtQueryDirectoryFile ( NT Specific )
//
// The function searches a directory for a file whose name and
// attributes match those specified in the function call.
//
// NTSYSAPI
// NTSTATUS
// NTAPI
// NtQueryDirectoryFile (
//    IN HANDLE FileHandle,
//    IN HANDLE EventHandle OPTIONAL,
//    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
//    IN PVOID ApcContext OPTIONAL,
//    OUT PIO_STATUS_BLOCK IoStatusBlock,
//    OUT PVOID Buffer,
//    IN ULONG BufferLength,
//    IN FILE_INFORMATION_CLASS InformationClass,
//    IN BOOLEAN ReturnByOne,
//    IN PUNICODE_STRING FileTemplate OPTIONAL,
//    IN BOOLEAN Reset
// ) ;
typedef LONG ( WINAPI *PROCNTQDF ) ( HANDLE, HANDLE, PVOID, PVOID,
                                     PIO_STATUS_BLOCK, PVOID, ULONG,
                                     UINT, BOOL, PUNICODE_STRING, BOOL ) ;
#define OSS_NPIPE_NTQUERYDIRECTORYFILE "NtQueryDirectoryFile"
// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSENUMNMPS, "_ossEnumNamedPipes" )
static INT32 _ossEnumNamedPipes ( vector<string> &names,
                                  const CHAR *pRootPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSENUMNMPS );
   LONG ntStatus ;
   IO_STATUS_BLOCK IoStatus ;
   HANDLE hPipe = INVALID_HANDLE_VALUE ;
   BOOL bReset = TRUE ;
   PFILE_QUERY_DIRECTORY DirInfo, TmpInfo ;
   PROCNTQDF NtQueryDirectoryFile ;
   LPSTR pszString = NULL ;
   LPWSTR pszWString = NULL ;
   DWORD dwString ;
   CHAR DirInfoBuffer [1024] ;
   // load NtQueryDirectoryFile function pointer from ntdll
   NtQueryDirectoryFile = (PROCNTQDF)GetProcAddress(
                                     GetModuleHandle(L"ntdll"),
                                     OSS_NPIPE_NTQUERYDIRECTORYFILE
                                     ) ;
   // make sure the function pointer is valid
   if ( !NtQueryDirectoryFile )
   {
      PD_LOG ( PDERROR, "Failed to load "OSS_NPIPE_NTQUERYDIRECTORYFILE ) ;
      rc = SDB_SYS ;
      goto done ;
   }

   // prepare open pipe string
   rc = ossANSI2WC ( pRootPath, &pszWString, &dwString ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to convert ansi to wc, rc = %d",
               rc ) ;
      goto done ;
   }
   // open pipe file
   hPipe = CreateFile ( pszWString, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, 0, NULL ) ;
   SDB_OSS_FREE ( pszWString ) ;
   pszWString = NULL ;
   if ( INVALID_HANDLE_VALUE == hPipe )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to open %s, error = %d", pRootPath,
               rc ) ;
      goto error ;
   }

   DirInfo = (PFILE_QUERY_DIRECTORY) DirInfoBuffer ;
   if ( !DirInfo )
   {
      PD_LOG ( PDERROR, "Failed to allocate memory for DirInfo" ) ;
      rc = SDB_OOM ;
      goto done ;
   }

   while ( TRUE )
   {
      // iterate pipes
      ntStatus = NtQueryDirectoryFile ( hPipe, NULL, NULL, NULL, &IoStatus,
                                        DirInfo, 1024, FileDirectoryInformation,
                                        FALSE, NULL,
                                        bReset ) ;
      if ( ntStatus != NO_ERROR )
      {
         if ( ntStatus == STATUS_NO_MORE_FILES )
            break ;
         PD_LOG ( PDERROR, "Failed to call NtQueryDirectoryFile, status = %d",
                  ntStatus ) ;
         rc = SDB_SYS ;
         goto done ;
      }
      TmpInfo = DirInfo ;
      while ( TRUE )
      {
         // store old values before we mangle the buffer
         const INT32 endStringAt = TmpInfo->FileNameLength/sizeof(WCHAR) ;
         const WCHAR oldValue = TmpInfo->FileDirectoryInformationClass.FileName[
                                      endStringAt] ;
         // place a null char at the end of string so we can convert to string
         TmpInfo->FileDirectoryInformationClass.FileName[endStringAt] = NULL ;
         rc = ossWC2ANSI ( TmpInfo->FileDirectoryInformationClass.FileName,
                           &pszString, &dwString ) ;
         if ( rc )
         {
            TmpInfo->FileDirectoryInformationClass.FileName[endStringAt] =
                  oldValue ;
            PD_LOG ( PDERROR, "Failed to convert wc to ansi, rc = %d",
                     rc ) ;
            goto done ;
         }
         // add pipe name to output
         names.push_back ( string ( pszString ) ) ;
         SDB_OSS_FREE ( pszString ) ;
         pszString = NULL ;
         TmpInfo->FileDirectoryInformationClass.FileName[endStringAt] =
               oldValue ;
         if ( TmpInfo->NextEntryOffset == 0 )
            break ;
         TmpInfo = (PFILE_QUERY_DIRECTORY) ((DWORD)TmpInfo +
                                            TmpInfo->NextEntryOffset ) ;
      } // while ( TRUE )
      bReset = FALSE ;
   } // while ( TRUE )
done :

   if ( pszString )
   {
      SDB_OSS_FREE ( pszString ) ;
      pszString = NULL ;
   }
   if ( pszWString )
   {
      SDB_OSS_FREE ( pszWString ) ;
      pszWString = NULL ;
   }
   if ( INVALID_HANDLE_VALUE != hPipe )
      CloseHandle ( hPipe ) ;
   PD_TRACE_EXITRC ( SDB__OSSENUMNMPS, rc );
   return rc ;
error :
   rc = SDB_SYS ;
   goto done ;
}

// enumate all named pipes that EXACT matches pattern
// if pattern is NULL, the call will enumerate all pipes in the system
// For example if we are looking for name "sequoiadb_engine_50000",
// then pattern will be "sequoiadb_engine_50000", any other pipe name
// will not match it.
// Users can check the size of "names" to verify whether the given pipe
// name exists in the system
// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSENUMNMPS2, "ossEnumNamedPipes" )
INT32 ossEnumNamedPipes ( std::vector<std::string> &names,
                          const CHAR *pattern,
                          OSS_MATCH_TYPE type,
                          const CHAR *rootPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSENUMNMPS2 );
   const CHAR *pFind = NULL ;
   std::string path ;
   std::vector<std::string> tempNames ;

   SDB_ASSERT( rootPath && 0 != *rootPath, "rootPath can't be empty" ) ;

   if ( rootPath && 0 != *rootPath )
   {
      path = rootPath ;
      if ( rootPath[ ossStrlen( rootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         path += OSS_FILE_SEP_CHAR ;
      }
   }

   if ( !pattern )
   {
      return _ossEnumNamedPipes ( names, path.c_str() ) ;
   }
   rc = _ossEnumNamedPipes ( tempNames, path.c_str() ) ;
   if ( rc )
   {
      goto error ;
   }

   for ( INT32 i = 0; i < tempNames.size(); ++i )
   {
      const CHAR *pName = tempNames[i].c_str() ;
      if ( OSS_MATCH_NULL == type ||
           ( OSS_MATCH_LEFT == type &&
             0 == ossStrncmp( pName, pattern, ossStrlen( pattern ) ) ) ||
           ( OSS_MATCH_MID == type && ossStrstr( pName, pattern ) ) ||
           ( OSS_MATCH_RIGHT == type &&
             ( pFind = ossStrstr( pName, pattern ) ) &&
             pFind[ossStrlen(pattern)] == 0 ) ||
           ( OSS_MATCH_ALL == type &&
             0 == ossStrcmp( pName, pattern ) )
          )
      {
         names.push_back ( tempNames[i] ) ;
      }
   }
done :
   PD_TRACE_EXITRC ( SDB__OSSENUMNMPS2, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCRTNMP, "ossCreateNamedPipe" )
INT32 ossCreateNamedPipe ( const CHAR *name,
                           UINT32 inboundBufferSize,
                           UINT32 outboundBufferSize,
                           UINT32 action,
                           UINT32 numInstances,
                           SINT32 defaultTimeout,
                           OSSNPIPE &handle,
                           const CHAR *pRootPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSCRTNMP );
   UINT32 openMode = 0 ;
   UINT32 pipeMode = 0 ;
   LPWSTR lpwstrName = NULL ;
   std::string fullName ;

   SDB_ASSERT ( name && name[0] != '\0',
                "name can't be empty or null" ) ;

   if ( pRootPath && 0 != *pRootPath )
   {
      fullName = pRootPath ;
      if ( pRootPath[ ossStrlen( pRootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         fullName += OSS_FILE_SEP_CHAR ;
      }
   }
   fullName += name ;

   if ( fullName.length() >= OSS_NPIPE_MAX_NAME_LEN )
   {
      PD_LOG ( PDERROR, "Named pipe is too long: %s", fullName.c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   handle._state = action ;
   // read or write or read/write
   switch ( action & OSS_NPIPE_DUPLEX )
   {
   case OSS_NPIPE_INBOUND :
      openMode = PIPE_ACCESS_INBOUND ;
      break ;
   case OSS_NPIPE_OUTBOUND :
      openMode = PIPE_ACCESS_OUTBOUND ;
      break ;
   default :
      openMode = PIPE_ACCESS_DUPLEX ;
      break ;
   }

   if ( numInstances > PIPE_UNLIMITED_INSTANCES )
   {
      numInstances = PIPE_UNLIMITED_INSTANCES ;
   }

   openMode |= OSS_NPIPE_FIRST_PIPE_INSTANCE ;
   if ( action & OSS_NPIPE_BLOCK_WITH_TIMEOUT )
   {
      handle._overlappedFlag = OSS_NPIPE_OVERLAP_ENABLED ;
      openMode |= FILE_FLAG_OVERLAPPED ;
   }
   else
   {
      handle._overlappedFlag = 0 ;
   }
   rc = ossANSI2WC ( fullName.c_str(), &lpwstrName, NULL ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to convert name %s to double bytes",
               fullName.c_str() ) ;
      goto error ;
   }

   ossMemset ( handle._name, 0, sizeof(handle._name) ) ;
   ossStrncpy ( handle._name, fullName.c_str(), OSS_NPIPE_MAX_NAME_LEN + 1 ) ;
   handle._handle = CreateNamedPipe ( lpwstrName, openMode, pipeMode,
                                      numInstances, outboundBufferSize,
                                      inboundBufferSize, defaultTimeout,
                                      NULL ) ;
   if ( INVALID_HANDLE_VALUE == handle._handle )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to create named pipe, error = %d", rc ) ;
      goto error ;
   }

   if ( handle._overlappedFlag )
   {
      handle._overlapped.Offset = 0 ;
      handle._overlapped.OffsetHigh = 0 ;
      handle._overlapped.hEvent = CreateEvent ( NULL,    // security
                                                FALSE,   // auto reset
                                                FALSE,   // init state not set
                                                NULL ) ; // unnamed
      if ( NULL == handle._overlapped.hEvent )
      {
         PD_LOG( PDERROR, "Create event failed, errno = %d",
                 ossGetLastError() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }

done :
   if ( lpwstrName )
      SDB_OSS_FREE ( lpwstrName ) ;
   PD_TRACE_EXITRC ( SDB_OSSCRTNMP, rc );
   return rc ;
error :
   ossDeleteNamedPipe( handle ) ;
   switch ( rc )
   {
   case ERROR_OUT_OF_STRUCTURES :
   case ERROR_NOT_ENOUGH_MEMORY :
      rc = SDB_OSS_NORES ;
      break ;
   case ERROR_PATH_NOT_FOUND :
   case ERROR_INVALID_PARAMETER :
   case ERROR_INVALID_NAME :
      rc = SDB_INVALIDARG ;
      break ;
   default :
      if ( rc >= 0 )
      {
         rc = SDB_SYS ;
      }
      break ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSOPENNMP, "ossOpenNamedPipe" )
INT32 ossOpenNamedPipe ( const CHAR *name,
                         UINT32 action,
                         SINT32 openTimeout,
                         OSSNPIPE &handle,
                         const CHAR *pRootPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSOPENNMP );
   SINT32 openMode   = 0 ;
   DWORD flagAttr    = FILE_ATTRIBUTE_NORMAL ;
   DWORD waitTimeout = 0 ;
   BOOLEAN doWait    = TRUE ;
   LPWSTR lpwstrName = NULL ;
   std::string fullName ;

   SDB_ASSERT ( name && name[0] != '\0',
                "name can't be empty or null" ) ;

   if ( pRootPath && 0 != *pRootPath )
   {
      fullName = pRootPath ;
      if ( pRootPath[ ossStrlen( pRootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         fullName += OSS_FILE_SEP_CHAR ;
      }
   }
   fullName += name ;

   if ( fullName.length() >= OSS_NPIPE_MAX_NAME_LEN )
   {
      PD_LOG ( PDERROR, "Named pipe is too long: %s", fullName.c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   switch ( action & OSS_NPIPE_DUPLEX )
   {
   case OSS_NPIPE_INBOUND :
      openMode = GENERIC_READ ;
      break ;
   case OSS_NPIPE_OUTBOUND :
      openMode = GENERIC_WRITE ;
      break ;
   default :
      openMode = GENERIC_READ | GENERIC_WRITE ;
      break ;
   }
   if ( action & OSS_NPIPE_NONBLOCK )
   {
      waitTimeout = 0 ;
      doWait = FALSE ;
   }
   else if ( openTimeout == OSS_NPIPE_INFINITE_TIMEOUT )
   {
      waitTimeout = NMPWAIT_WAIT_FOREVER ;
   }
   else if ( openTimeout <= 0 )
   {
      waitTimeout = NMPWAIT_USE_DEFAULT_WAIT ;
   }
   else
   {
      waitTimeout = openTimeout * 1000 ;
   }

   rc = ossANSI2WC ( fullName.c_str(), &lpwstrName, NULL ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to convert name %s to double bytes",
               name ) ;
      goto error ;
   }
   // wait until the pipe is availiable
   if ( doWait && !WaitNamedPipe ( lpwstrName, waitTimeout ) )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to wait named process in %d millisec, err=%d",
               waitTimeout, rc ) ;
      goto error ;
   }

   // check for overlap flag
   if ( action & OSS_NPIPE_BLOCK_WITH_TIMEOUT )
   {
      handle._overlappedFlag = OSS_NPIPE_OVERLAP_ENABLED ;
      flagAttr |= FILE_FLAG_OVERLAPPED ;
   }
   else
   {
      handle._overlappedFlag = 0 ;
   }

   handle._handle = CreateFile ( lpwstrName, openMode,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, flagAttr,
                                 NULL ) ;
   if ( INVALID_HANDLE_VALUE == handle._handle )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to open pipe, error = %d", rc ) ;
      goto error ;
   }
   // everything must be fine here, otherwise we should jump to error
   if ( handle._overlappedFlag )
   {
      handle._overlapped.Offset = 0 ;
      handle._overlapped.OffsetHigh = 0 ;
      handle._overlapped.hEvent = CreateEvent ( NULL,    // security
                                                FALSE,   // auto reset
                                                FALSE,   // init state not set
                                                NULL ) ; // unnamed
      if ( NULL == handle._overlapped.hEvent )
      {
         rc = ossGetLastError () ;
         PD_LOG( PDERROR, "Failed to create event, error = %d", rc ) ;
         goto error ;
      }
   }
   handle._state = action ;
   ossMemset ( handle._name, 0, sizeof(handle._name) ) ;
   ossStrncpy ( handle._name, fullName.c_str(), OSS_NPIPE_MAX_NAME_LEN + 1 ) ;
done :
   if ( lpwstrName )
      SDB_OSS_FREE ( lpwstrName ) ;
   PD_TRACE_EXITRC ( SDB_OSSOPENNMP, rc );
   return rc ;
error :
   ossCloseNamedPipe( handle ) ;
   switch ( rc )
   {
   case ERROR_TOO_MANY_OPEN_FILES :
      rc = SDB_OSS_NORES ;
      break ;
   case ERROR_PATH_NOT_FOUND :
   case ERROR_FILE_NOT_FOUND :
   case ERROR_BAD_NETPATH :
      rc = SDB_FNE ;
      break ;
   case ERROR_INVALID_PARAMETER :
      rc = SDB_INVALIDARG ;
      break ;
   default :
      if ( rc >= 0 )
      {
         rc = SDB_SYS ;
      }
      break ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCONNNMP, "ossConnectNamedPipe" )
INT32 ossConnectNamedPipe ( OSSNPIPE &handle,
                            UINT32 action,
                            SINT32 timeout )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSCONNNMP );
   DWORD connectTimeout = 0 ;
   DWORD recv ;
   if ( !ConnectNamedPipe ( handle._handle,
                            handle._overlappedFlag ? &(handle._overlapped) :
                                                     NULL ) )
   {
      rc = ossGetLastError () ;
      if ( handle._overlappedFlag &&
           ( ERROR_PIPE_LISTENING == rc || ERROR_IO_PENDING == rc ) )
      {
         OSS_BIT_SET ( handle._overlappedFlag, OSS_NPIPE_OVERLAP_IOPENDING ) ;
         rc = SDB_OK ;
      }
      else if ( ERROR_PIPE_CONNECTED )
      {
         rc = SDB_OK ;
      }
      else if ( ERROR_NO_DATA == rc )
      {
         rc = SDB_EOF ;
      }
      else
      {
         PD_LOG ( PDERROR, "Failed to open pipe %s, error = %d",
                  handle._name, rc ) ;
         rc = SDB_SYS ;
      }
   }
   if ( OSS_BIT_TEST(handle._overlappedFlag, OSS_NPIPE_OVERLAP_IOPENDING) )
   {
      OSS_BIT_CLEAR ( handle._overlappedFlag,
                      OSS_NPIPE_OVERLAP_IOPENDING ) ;

      // if we are waiting for IO, let's wait for event
      if ( OSS_NPIPE_INFINITE_TIMEOUT == timeout )
      {
         connectTimeout = -1 ;
      }
      else
      {
         connectTimeout = timeout ;
      }
      // wait until something arrive
      rc = ossWaitInterrupt ( handle._overlapped.hEvent,
                              connectTimeout ) ;
      if ( rc )
      {
         goto error ;
      }
      else
      {
         // check how many bytes we received
         if ( !GetOverlappedResult ( handle._handle, &handle._overlapped,
                                     &recv, FALSE ) )
         {
            rc = ossGetLastError () ;
            PD_LOG ( PDERROR, "Failed to get overlapped result, rc = %d",
                     rc ) ;
            rc = SDB_SYS ;
         }
      }
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSCONNNMP, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSRENMP, "ossReadNamedPipe" )
INT32 ossReadNamedPipe ( OSSNPIPE &handle,
                         CHAR *pBuffer,
                         INT64 bufSize,
                         INT64 *bufRead,
                         SINT32 timeout )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSRENMP );
   DWORD tempRead = 0 ;
   DWORD timeWait ;
   if ( !ReadFile ( handle._handle, (void*)(pBuffer),
                    bufSize,
                    &tempRead,
                    handle._overlappedFlag?&handle._overlapped:NULL ) )
   {
      rc = ossGetLastError () ;
      // if we get io pending, let's try to wait and read it
      if ( handle._overlappedFlag && ( ERROR_IO_PENDING == rc ) )
      {
         rc = SDB_OK ;
         if ( timeout == OSS_NPIPE_INFINITE_TIMEOUT )
         {
            timeWait = -1 ;
         }
         else
         {
            timeWait = timeout ;
         }
         // wait for interrupt
         rc = ossWaitInterrupt ( handle._overlapped.hEvent, timeWait ) ;
         if ( SDB_OK == rc )
         {
            if ( !GetOverlappedResult ( handle._handle, &handle._overlapped,
                                        &tempRead, FALSE ) )
            {
               rc = ossGetLastError () ;
               PD_LOG ( PDERROR, "Failed to get overlapped result, rc = %d",
                        rc ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
         else
         {
            // if something wrong with overlapped IO, we need to cancel the
            // read request
            if ( !CancelIo ( handle._handle ) )
            {
               rc = ossGetLastError () ;
               PD_LOG ( PDERROR, "Failed to cancel overlapped io, rc = %d",
                        rc ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            // if something sent anything before canceling io, let's receive
            if ( !GetOverlappedResult ( handle._handle, &handle._overlapped,
                                        &tempRead, FALSE ) )
            {
               rc = ossGetLastError () ;
               if ( ERROR_OPERATION_ABORTED == rc )
               {
                  // this is expected, so we get timeout here
                  rc = SDB_TIMEOUT ;
                  goto error ;
               }
               PD_LOG ( PDERROR, "Failed to get overlapped result, rc = %d",
                        rc ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
      }
      else if ( ERROR_NO_DATA == rc )
      {
         if ( tempRead == 0 && OSS_BIT_TEST ( handle._state,
                                              OSS_NPIPE_NONBLOCK ) )
         {
            rc = SDB_OK ;
         }
         else
         {
            rc = SDB_EOF ;
         }
      }
      else if ( ERROR_PIPE_NOT_CONNECTED == rc || ERROR_BROKEN_PIPE == rc )
      {
         rc = SDB_EOF ;
      }
      else
      {
         rc = SDB_SYS ;
      }
   }
   if ( bufRead )
      *bufRead = tempRead ;
done :
   PD_TRACE_EXITRC ( SDB_OSSRENMP, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSWTNMP, "ossWriteNamedPipe" )
INT32 ossWriteNamedPipe ( OSSNPIPE &handle,
                          const CHAR *pBuffer,
                          INT64 bufSize,
                          INT64 *bufWrite )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSWTNMP );
   DWORD tempWrite = 0 ;
   if ( !WriteFile ( handle._handle, (void*)(pBuffer),
                     bufSize,
                     &tempWrite,
                     handle._overlappedFlag ? &handle._overlapped : NULL ) )
   {
      rc = ossGetLastError () ;
      if ( handle._overlappedFlag && ( ERROR_IO_PENDING == rc ) )
      {
         // wait for interrupt forever
         rc = ossWaitInterrupt ( handle._overlapped.hEvent,
                                 OSS_NPIPE_INFINITE_TIMEOUT ) ;
         if ( SDB_OK == rc )
         {
            if ( !GetOverlappedResult ( handle._handle, &handle._overlapped,
                                        &tempWrite, FALSE ) )
            {
               rc = ossGetLastError () ;
               PD_LOG ( PDERROR, "Failed to get overlapped result, rc = %d",
                        rc ) ;
            }
         }
      }
   }
   if ( 0 != rc )
   {
      switch ( rc )
      {
      case ERROR_NO_DATA :
         if ( handle._overlappedFlag && handle._overlapped.hEvent )
         {
            SetEvent ( handle._overlapped.hEvent ) ;
         }
         break ;
      }
      rc = SDB_SYS ;
   }
   if ( bufWrite )
      *bufWrite = tempWrite ;
   PD_TRACE_EXITRC ( SDB_OSSWTNMP, rc );
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDISCONNNMP, "ossDisconnectNamedPipe" )
INT32 ossDisconnectNamedPipe ( OSSNPIPE &handle )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSDISCONNNMP );
   FlushFileBuffers ( handle._handle ) ;
   if ( !DisconnectNamedPipe ( handle._handle ) )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to disconnect named pipe: %s, error = %d",
               handle._name, rc ) ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSDISCONNNMP, rc );
   return rc ;
error :
   switch ( rc )
   {
   case ERROR_PIPE_NOT_CONNECTED :
      rc = SDB_OK ;
      break ;
   default :
      rc = SDB_SYS ;
      break ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCLSNMP, "ossCloseNamedPipe" )
INT32 ossCloseNamedPipe ( OSSNPIPE &handle )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSCLSNMP );

   if ( INVALID_HANDLE_VALUE == handle._handle )
   {
      goto done ;
   }

   if ( !CloseHandle ( handle._handle ) )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to close named pipe: %s, error = %d",
                handle._name, rc ) ;
      goto error ;
   }

done :
   handle._handle = INVALID_HANDLE_VALUE ;
   if ( handle._overlappedFlag &&
        NULL != handle._overlapped.hEvent )
   {
      CloseHandle ( handle._overlapped.hEvent ) ;
      handle._overlapped.hEvent = NULL ;
   }
   PD_TRACE_EXITRC ( SDB_OSSCLSNMP, rc );
   return rc ;
error :
   switch ( rc )
   {
   case ERROR_PIPE_NOT_CONNECTED :
      rc = SDB_OK ;
      break ;
   default :
      rc = SDB_SYS ;
      break ;
   }
   goto done ;
}

INT32 ossDeleteNamedPipe ( OSSNPIPE &handle )
{
   return ossCloseNamedPipe ( handle ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSNMP2FD, "ossNamedPipeToFd" )
INT32 ossNamedPipeToFd ( OSSNPIPE & handle , INT32 * output )
{
   INT32 rc    = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSNMP2FD );
   int   fd    = -1 ;
   int   flag  = 0 ;

   if ( ! output )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( handle._state & OSS_NPIPE_DUPLEX )
   {
      flag |= _O_RDWR ;
   }
   else if ( handle._state & OSS_NPIPE_INBOUND )
   {
      flag |= _O_RDONLY ;
   }
   else if ( handle._state & OSS_NPIPE_OUTBOUND )
   {
      flag |= _O_WRONLY ;
   }

   fd = _open_osfhandle ( (intptr_t) handle._handle , flag ) ;
   if ( -1 == fd )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   *output = fd ;

done :
   PD_TRACE_EXITRC ( SDB_OSSNMP2FD, rc );
   return rc ;
error :
   goto done ;
}

INT32 ossCleanNamedPipeByName ( const CHAR * pipeName,
                                const CHAR *pRootPath )
{
   (void *) pipeName ; // avoid compiler warning

   // Windows will automatically clean up any open pipe when process exits,
   // So there is no need to do anything here
   return SDB_OK ;
}

#elif defined (_LINUX)
// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCRTNP, "ossCreateNamedPipe" )
INT32 ossCreateNamedPipe ( const CHAR *name,
                           UINT32 inboundBufferSize,
                           UINT32 outboundBufferSize,
                           UINT32 action,
                           UINT32 numInstances,
                           SINT32 defaultTimeout,
                           OSSNPIPE &handle,
                           const CHAR *pRootPath )
{
   INT32 rc = SDB_OK ;
   std::string pathName ;
   PD_TRACE_ENTRY ( SDB_OSSCRTNP ) ;
   SDB_ASSERT ( name && name[0] != '\0',
                "name can't be empty or null" ) ;

   if ( pRootPath && 0 != *pRootPath )
   {
      pathName = pRootPath ;
      if ( pRootPath[ ossStrlen( pRootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         pathName += OSS_FILE_SEP_CHAR ;
      }

      /// make sure exist
      rc = ossMkdir( pRootPath, OSS_PERMALL ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Create pipe dir[%s] failed, rc: %d",
                 pRootPath, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
   }
   pathName += name ;

   if ( pathName.length() >= OSS_NPIPE_MAX_NAME_LEN )
   {
      PD_LOG ( PDERROR, "Named pipe is too long: %s", pathName.c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   handle._state = action ;
   ossMemset ( handle._name, 0, sizeof(handle._name) ) ;
   ossStrncpy ( handle._name, pathName.c_str(), OSS_NPIPE_MAX_NAME_LEN + 1 ) ;
   handle._handle = mkfifo ( pathName.c_str(), OSS_PERMALL ) ;
   if ( -1 == handle._handle )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to create named pipe, errno = %d", rc ) ;
      switch ( rc )
      {
      case EEXIST :
         rc = SDB_FE ;
         break ;
      case EACCES :
      case ENOENT :
      case ENOTDIR :
      case ENAMETOOLONG :
         rc = SDB_INVALIDARG ;
         break ;
      case EROFS :
      case ENOSPC :
         rc = SDB_OSS_NORES ;
         break ;
      default :
         rc = SDB_SYS ;
         break ;
      }
      goto error ;
   }
   /// set the permission, because mkfifo can't make the group_write and
   /// other write permission
   try
   {
      fs::permissions( fs::path( pathName ),
                       fs::owner_all | fs::group_all | fs::others_all ) ;
   }
   catch( std::exception &e )
   {
      /// ignored the error
   }

done :
   PD_TRACE_EXITRC ( SDB_OSSCRTNP, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSOPENNP, "ossOpenNamedPipe" )
INT32 ossOpenNamedPipe ( const CHAR *name,
                         UINT32 action,
                         SINT32 openTimeout,
                         OSSNPIPE &handle,
                         const CHAR *pRootPath )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSOPENNP );
   INT32 openMode = 0 ;
   std::string pathName ;
   struct stat64 st ;
   SDB_ASSERT ( name && name[0] != '\0',
                "name can't be empty or null" ) ;

   if ( pRootPath && 0 != *pRootPath )
   {
      pathName = pRootPath ;
      if ( pRootPath[ ossStrlen( pRootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         pathName += OSS_FILE_SEP_CHAR ;
      }

      /// make sure exist
      rc = ossMkdir( pRootPath, OSS_PERMALL ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Create pipe dir[%s] failed, rc: %d",
                 pRootPath, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
   }
   pathName += name ;

   if ( pathName.length() >= OSS_NPIPE_MAX_NAME_LEN )
   {
      PD_LOG ( PDERROR, "Named pipe is too long: %s", pathName.c_str() ) ;
      rc = SDB_INVALIDARG ;
      // we don't need to check rc in error here
      goto done ;
   }

   switch ( action & OSS_NPIPE_DUPLEX )
   {
   case OSS_NPIPE_INBOUND :
      openMode = O_RDONLY ;
      break ;
   case OSS_NPIPE_OUTBOUND :
      openMode = O_WRONLY ;
      break ;
   default :
      openMode = O_RDWR ;
      break ;
   }

   if ( action & OSS_NPIPE_NONBLOCK  )
   {
      openMode |= O_NONBLOCK ;
   }
   do
   {
      handle._handle = open ( pathName.c_str(), openMode ) ;
   }
   while ( ( -1 == handle._handle ) && ( (rc = ossGetLastError()) == EINTR ) ) ;
   if ( -1 == handle._handle )
   {
// only display the error in release mode because debug mode may display
// messages on screen and we don't want this error ruin the sdb output
#if defined ( _DEBUG )
#else
      PD_LOG ( PDERROR, "Failed to open named pipe: %s, errno = %d",
               pathName.c_str(), rc ) ;
#endif
      goto error ;
   }
   // when fd is opened, we have to check the fd is FIFO
   if ( -1 == oss_fstat ( handle._handle, &st ) )
   {
      rc = ossGetLastError () ;
      close ( handle._handle ) ;
      PD_LOG ( PDERROR, "Failed to fstat named pipe: %s, errno = %d",
               pathName.c_str(), rc ) ;
      goto error ;
   }
   if ( !S_ISFIFO ( st.st_mode ) )
   {
      close ( handle._handle ) ;
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "name %s is not pipe", handle._name ) ;
      goto done ;
   }
   // check number of bytes can be written to pipe
   if ( -1 == ( handle._bufSize = fpathconf ( handle._handle, _PC_PIPE_BUF )))
   {
      rc = ossGetLastError () ;
      close ( handle._handle ) ;
      PD_LOG ( PDERROR, "Failed to get pipe buf size for %s, errno = %d",
               pathName.c_str(), rc ) ;
      goto error ;
   }
   handle._state = action ;
   ossStrncpy ( handle._name, pathName.c_str(), OSS_NPIPE_MAX_NAME_LEN + 1 ) ;
done :
   PD_TRACE_EXITRC ( SDB_OSSOPENNP, rc );
   return rc ;
error :
   if ( rc >= 0 )
   {
      switch ( rc )
      {
      case ENOENT :
         rc = SDB_FNE ;
         break ;
      case EACCES :
      case EROFS :
         rc = SDB_PERM ;
         break ;
      case EMFILE :
         rc = SDB_TOO_MANY_OPEN_FD ;
         break ;
      default :
         rc = SDB_SYS ;
         break ;
      }
   }
   goto done ;
}

INT32 ossConnectNamedPipe ( OSSNPIPE &handle,
                            UINT32 action,
                            SINT32 timeout )
{
   return ossOpenNamedPipe ( handle._name, handle._state, timeout, handle,
                             NULL ) ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSRDNP, "ossReadNamedPipe" )
INT32 ossReadNamedPipe ( OSSNPIPE &handle,
                         CHAR *pBuffer,
                         INT64 bufSize,
                         INT64 *bufRead,
                         SINT32 timeout )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSRDNP );
   fd_set fds ;
   ssize_t readSize ;
   struct timeval selectTimeout ;
   if ( OSS_BIT_TEST ( handle._state, OSS_NPIPE_BLOCK_WITH_TIMEOUT ) &&
        timeout != OSS_NPIPE_INFINITE_TIMEOUT )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( handle._handle, &fds ) ;
      selectTimeout.tv_sec = timeout / 1000 ;
      selectTimeout.tv_usec = ( timeout % 1000 ) * 1000 ;
      rc = select ( handle._handle+1, &fds, NULL, NULL, &selectTimeout ) ;
      if ( 0 == rc )
      {
         rc = SDB_TIMEOUT ;
         goto done ;
      }
      else if ( rc > 0 && ( FD_ISSET ( handle._handle, &fds ) ) )
      {
         // data arrive
      }
      else
      {
         rc = ossGetLastError () ;
         goto error ;
      }
   }
   do
   {
      readSize = read ( handle._handle, pBuffer, bufSize ) ;
   } while ( -1 == readSize && ( rc = ossGetLastError() ) == EINTR ) ;

   if ( 0 == readSize )
   {
      rc = SDB_EOF ;
      goto done ;
   }
   if ( -1 == readSize )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to read from pipe %s, errno = %d",
               handle._name, rc ) ;
      goto error ;
   }

   if ( bufRead )
      *bufRead = readSize ;
   rc = SDB_OK ;
done :
   PD_TRACE_EXITRC ( SDB_OSSRDNP, rc );
   return rc ;
error :
   switch ( rc )
   {
   case EAGAIN :
      rc = SDB_EOF ;
      break ;
   case EINTR :
      rc = SDB_INTERRUPT ;
      break ;
   default :
      rc = SDB_SYS ;
      break ;
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSWTNP, "ossWriteNamedPipe" )
INT32 ossWriteNamedPipe ( OSSNPIPE &handle,
                          const CHAR *pBuffer,
                          INT64 bufSize,
                          INT64 *bufWrite )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSWTNP );
   INT64 len = bufSize ;
   INT64 hasWrite = 0 ;
   ssize_t writeSize = 0 ;

   if ( bufSize <= 0 )
   {
      goto done ;
   }
   do
   {
      len = bufSize > handle._bufSize ? handle._bufSize : bufSize ;
      writeSize = write ( handle._handle, &pBuffer[hasWrite], len ) ;
      if ( writeSize > 0 )
      {
         bufSize -= writeSize ;
         hasWrite += writeSize ;
      }
      if ( bufSize <= 0 )
      {
         break ;
      }
   } while ( -1 == writeSize && ( rc = ossGetLastError()) == EINTR ) ;
   if ( -1 == writeSize )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to write to pipe %s, errno = %d",
               handle._name, rc ) ;
      goto error ;
   }
   if ( bufWrite )
      *bufWrite = hasWrite ;
   rc = SDB_OK ;
done :
   PD_TRACE_EXITRC ( SDB__OSSWTNP, rc );
   return rc ;
error :
   switch ( rc )
   {
   case EINTR :
      rc = SDB_INTERRUPT ;
      break ;
   default :
      rc = SDB_SYS ;
      break ;
   }
   goto done ;
}

INT32 ossDisconnectNamedPipe ( OSSNPIPE &handle )
{
   return ossCloseNamedPipe ( handle ) ;
}

INT32 ossCloseNamedPipe ( OSSNPIPE &handle )
{
   INT32 rc = SDB_OK ;
   if ( -1 != handle._handle )
   {
      close ( handle._handle ) ;
      handle._handle = -1 ;
   }
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSDELNP, "ossDeleteNamedPipe" )
INT32 ossDeleteNamedPipe ( OSSNPIPE &handle )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSDELNP );
   rc = unlink ( handle._name ) ;
   if ( -1 == rc )
   {
      rc = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to delete pipe: %s, errno = %d",
               handle._name, rc ) ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_OSSDELNP, rc );
   return rc ;
error :
   if ( rc >= 0 )
   {
      switch ( rc )
      {
      case EACCES :
      case EPERM :
      case EROFS :
         rc = SDB_PERM ;
         break ;
      case ENOENT :
         rc = SDB_FNE ;
         break ;
      case ENOMEM :
         rc = SDB_OOM ;
         break ;
      default :
         rc = SDB_SYS ;
         break ;
      }
   }
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSNP2FD, "ossNamedPipeToFd" )
INT32 ossNamedPipeToFd ( OSSNPIPE & handle , INT32 * output )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_OSSNP2FD );

   if ( ! output )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *output = handle._handle ;

done :
   PD_TRACE_EXITRC ( SDB_OSSNP2FD, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_OSSCLNPBYNM, "ossCleanNamedPipeByName" )
INT32 ossCleanNamedPipeByName ( const CHAR * pipeName,
                                const CHAR *pRootPath )
{
   PD_TRACE_ENTRY ( SDB_OSSCLNPBYNM );
   std::string pathName ;
   SDB_ASSERT ( pipeName && pipeName[0] != '\0',
                "name can't be empty or null" ) ;

   if ( pRootPath && 0 != *pRootPath )
   {
      pathName = pRootPath ;
      if ( pRootPath[ ossStrlen( pRootPath ) - 1 ] != OSS_FILE_SEP_CHAR )
      {
         pathName += OSS_FILE_SEP_CHAR ;
      }
   }
   pathName += pipeName ;
   INT32 rc = unlink ( pathName.c_str() ) ;
   if ( -1 == rc )
      rc = SDB_SYS ;
   PD_TRACE_EXITRC ( SDB_OSSCLNPBYNM, rc );
   return rc ;
}

static INT32 _ossEnumNamedPipes( const string &dirPath,
                                 vector<string > &names,
                                 const CHAR *filter, UINT32 filterLen,
                                 OSS_MATCH_TYPE type )
{
   INT32 rc = SDB_OK ;
   const CHAR *pFind = NULL ;

   try
   {
      fs::path dbDir ( dirPath ) ;
      fs::directory_iterator end_iter ;

      if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
      {
         for ( fs::directory_iterator dir_iter ( dbDir );
               dir_iter != end_iter; ++dir_iter )
         {
            try
            {
               if ( fs::is_regular_file ( dir_iter->status() ) ||
                    fs::is_directory( dir_iter->path() ) )
               {
                  continue ;
               }
               else
               {
                  const std::string fileName =
                     dir_iter->path().filename().string() ;

                  if ( ( OSS_MATCH_NULL == type ) ||
                       ( OSS_MATCH_LEFT == type &&
                         0 == ossStrncmp( fileName.c_str(), filter,
                                          filterLen ) ) ||
                       ( OSS_MATCH_MID == type &&
                         ossStrstr( fileName.c_str(), filter ) ) ||
                       ( OSS_MATCH_RIGHT == type &&
                         ( pFind = ossStrstr( fileName.c_str(), filter ) ) &&
                         pFind[filterLen] == 0 ) ||
                       ( OSS_MATCH_ALL == type &&
                         0 == ossStrcmp( fileName.c_str(), filter ) )
                     )
                  {
                     names.push_back( fileName ) ;
                  }
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "File or dir[%s] occur exception: %s",
                       dir_iter->path().string().c_str(),
                       e.what() ) ;
               /// skip the file or dir
            }
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 ossEnumNamedPipes( vector<string > &names,
                         const CHAR *pattern,
                         OSS_MATCH_TYPE type,
                         const CHAR * rootPath )
{
   INT32 rc = SDB_OK ;
   /// make sure exist
   if ( rootPath )
   {
      rc = ossMkdir( rootPath, OSS_PERMALL ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Create pipe dir[%s] failed, rc: %d",
                 rootPath, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
   }
   rc = _ossEnumNamedPipes( rootPath, names, pattern,
                            ossStrlen( pattern ), type ) ;

done:
   return rc ;
error:
   goto done ;
}

#endif // _LINUX

