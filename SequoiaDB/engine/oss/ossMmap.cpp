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

   Source File Name = ossMmap.cpp

   Descriptive Name = Operating System Services Memory Map

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for Memory Mapping
   Files.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossMmap.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
#if defined (_LINUX)
#include <sys/mman.h>
#elif defined (_WINDOWS)
// this defines DMS page size, need to verify with Windows page granularity
#include "dms.hpp"
#endif

#define OSS_MMAP_INIT_CAPACITY         ( 128 )

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_OPEN, "_ossMmapFile::open" )
INT32 _ossMmapFile::open ( const CHAR *pFilename,
                           UINT32 iMode,
                           UINT32 iPermission )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSMMF_OPEN );
   rc = ossOpen ( pFilename, iMode, iPermission, _file ) ;
   if ( SDB_OK == rc )
   {
      _opened = TRUE ;
   }
   else
   {
      PD_LOG ( PDERROR, "Failed to open file, rc: %d", rc ) ;
      goto error ;
   }
   ossStrncpy ( _fileName, pFilename, OSS_MAX_PATHSIZE ) ;

done :
   PD_TRACE_EXITRC ( SDB__OSSMMF_OPEN, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_CLOSE, "_ossMmapFile::close" )
void _ossMmapFile::close ()
{
   PD_TRACE_ENTRY ( SDB__OSSMMF_CLOSE ) ;

   engine::ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;

   // clear all maped regions
   for ( UINT32 i = 0 ; i < _size ; ++i )
   {
#if defined (_LINUX)
      munmap((void*)(_pSegArray[i]._ptr), _pSegArray[i]._length) ;
#elif defined (_WINDOWS)
      if ( _pSegArray[i]._maphandle )
      {
         CloseHandle ( _pSegArray[i]._maphandle ) ;
      }
      UnmapViewOfFile((LPCVOID)(_pSegArray[i]._ptr)) ;
#endif
   }
   _clearSeg() ;
   // close opened file
   if ( _opened )
   {
      ossClose ( _file ) ;
      _opened = FALSE ;
   }
   PD_TRACE_EXIT ( SDB__OSSMMF_CLOSE );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_SIZE, "_ossMmapFile::size" )
INT32 _ossMmapFile::size ( UINT64 &fileSize )
{
   PD_TRACE_ENTRY ( SDB__OSSMMF_SIZE ) ;
   SDB_ASSERT ( _opened, "file is not opened" ) ;
   INT32 rc = SDB_OK ;
   rc = ossGetFileSize ( &_file, (INT64*)&fileSize ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get file size, rc: %d", rc ) ;
      goto error ;
   }
   PD_TRACE1 ( SDB__OSSMMF_SIZE, PD_PACK_ULONG(fileSize) ) ;

done :
   PD_TRACE_EXITRC ( SDB__OSSMMF_SIZE, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_MAP, "_ossMmapFile::map" )
INT32 _ossMmapFile::map ( UINT64 offset, UINT32 length, void **pAddress )
{
   PD_TRACE_ENTRY ( SDB__OSSMMF_MAP );
   SDB_ASSERT ( _opened, "file is not opened" ) ;
   INT32 rc = SDB_OK ;
   INT32 err = 0 ;
   ossMmapSegment seg ( 0,0,0 ) ;
   UINT64 fileSize = 0 ;
   void *segment = NULL ;
#if defined (_WINDOWS)
   SYSTEM_INFO si;
#endif
   // if we don't want to map anything, just return success
   if ( 0 == length )
   {
      goto done ;
   }
   // then let's get file size to make sure we are mapping right range
   rc = ossGetFileSize ( &_file, (INT64*)&fileSize ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to get file size, rc: %d", rc ) ;
      goto error ;
   }

   if ( offset >= fileSize )
   {
      PD_LOG ( PDERROR, "Offset is greater than file size" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( offset + length > fileSize )
   {
      PD_LOG ( PDWARNING, "offset+length is greater than file size" ) ;
      length-=fileSize - offset - 1 ;
   }

   SDB_ASSERT ( length!=0, "invalid length to map" ) ;

   rc = _ensureSpace( _size + 1 ) ;
   if ( rc )
   {
      goto error ;
   }

   // map region into memory
#if defined (_LINUX)
   segment = mmap( NULL, length, PROT_READ|PROT_WRITE, MAP_SHARED,
                   _file.fd, offset ) ;
   if ( MAP_FAILED == segment )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to map offset %ld length %d, errno=%d",
               offset, length, err ) ;
      if ( ENOMEM == err )
      {
         rc = SDB_OOM ;
      }
      else if ( EACCES == err )
      {
         rc = SDB_PERM ;
      }
      else
      {
         rc = SDB_SYS ;
      }
      goto error ;
   }
   // advise kernel to not copy the memory during fork
   // we don't care the return value anyway
   madvise ( segment, length, MADV_DONTFORK|MADV_SEQUENTIAL ) ;
#elif defined (_WINDOWS)
   // make sure the requested offset is aligned with OS memory allocation
   // granularity. Otherwise MapViewOfFile will fail
   GetSystemInfo(&si);
   if ( offset % si.dwAllocationGranularity != 0 )
   {
      PD_LOG ( PDERROR, "Page size is smaller than mem granularity" ) ;
      PD_LOG ( PDERROR, "Page size: %d; Granularity %d", DMS_PAGE_SIZE64K,
               si.dwAllocationGranularity ) ;
      rc = SDB_SYS ;
      goto error ;
   }
   seg._maphandle = CreateFileMapping ( _file.hFile, NULL, PAGE_READWRITE,
                                        0 /* high 32 bit, 0 in our case */,
                                        0 /* low 32 bit*/,
                                        NULL ) ;
   if ( NULL == seg._maphandle )
   {
      DWORD err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to create file mapping, err: %d", err );
      close () ;
      goto error ;
   }

   segment = MapViewOfFile ( seg._maphandle, FILE_MAP_ALL_ACCESS,
                             (DWORD)(offset>>32) /* high 32 bits */,
                             (DWORD)offset /* low 32 bits */,
                             length ) ;
   if ( NULL == segment )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to map view of file, offset %lld length %d, "
               "err=%d", offset, length, err ) ;
      rc = SDB_SYS ;
      goto error ;
   }
#endif
   _totalLength += length ;
   seg._ptr = (ossValuePtr)segment;
   seg._length = length ;
   seg._offset = offset ;

   _pSegArray[ _size ] = seg ;
   ++_size ;

   if ( pAddress )
   {
      *pAddress = segment ;
   }

done :
   PD_TRACE_EXITRC ( SDB__OSSMMF_MAP, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_FLHALL, "_ossMmapFile::flushAll" )
INT32 _ossMmapFile::flushAll ( BOOLEAN sync )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSMMF_FLHALL ) ;

   for ( UINT32 i = 0; i < _size ; i++ )
   {
      rc = flush ( i, sync ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

done:
   PD_TRACE_EXITRC ( SDB__OSSMMF_FLHALL, rc );
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_FLUSH, "_ossMmapFile::flush" )
INT32 _ossMmapFile::flush ( UINT32 segmentID, BOOLEAN sync )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSMMF_FLUSH );
   INT32 err = 0 ;

   engine::ossScopedRWLock lock( &_rwMutex, SHARED ) ;

   if  ( segmentID >= _size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
#if defined (_LINUX)
   if ( msync((void*)_pSegArray[segmentID]._ptr, _pSegArray[segmentID]._length,
              sync ? MS_SYNC:MS_ASYNC) )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to msync, err=%d", err ) ;
      goto error ;
   }
#elif defined (_WINDOWS)
   if ( !FlushViewOfFile((LPCVOID)_pSegArray[segmentID]._ptr,
                        _pSegArray[segmentID]._length ) )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to FlushViewOfFile, err=%d", err );
      goto error ;
   }
   if ( !FlushFileBuffers(_file.hFile) )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to FlushFileBuffers, err=%d", err );
      goto error ;
   }
#endif

done :
   PD_TRACE_EXITRC ( SDB__OSSMMF_FLUSH, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_FLUSHBLOCK, "_ossMmapFile::flushBlock" )
INT32 _ossMmapFile::flushBlock( UINT32 segmentID, UINT32 offset,
                                INT32 length, BOOLEAN sync )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSMMF_FLUSHBLOCK );
   INT32 err = 0 ;
   ossMmapSegment *pSegment = NULL ;
   ossValuePtr ptr = 0 ;

   engine::ossScopedRWLock lock( &_rwMutex, SHARED ) ;

   if( segmentID >= _size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pSegment = &_pSegArray[segmentID] ;
   if ( offset > pSegment->_length )
   {
      /// offset more than segment size
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( length == 0 || offset == pSegment->_length )
   {
      goto done ;
   }
   else if ( length < 0 ||
             length + offset > pSegment->_length )
   {
      length = pSegment->_length - offset ;
   }

   ptr = pSegment->_ptr + offset ;

#if defined (_LINUX)
   if ( msync((void*)ptr, length, sync ? MS_SYNC : MS_ASYNC) )
   {
      err = ossGetLastError () ;
      PD_LOG ( PDERROR, "Failed to msync, err=%d", err ) ;
      goto error ;
   }
#elif defined (_WINDOWS)
   if ( !FlushViewOfFile( (LPCVOID)ptr, length ) )
   {
      err = ossGetLastError() ;
      PD_LOG ( PDERROR, "Failed to FlushViewOfFile, err=%d", err );
      goto error ;
   }
   if ( !FlushFileBuffers( _file.hFile ) )
   {
      err = ossGetLastError() ;
      PD_LOG ( PDERROR, "Failed to FlushFileBuffers, err=%d", err );
      goto error ;
   }
#endif

done :
   PD_TRACE_EXITRC ( SDB__OSSMMF_FLUSHBLOCK, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB__OSSMMF_UNLINK, "_ossMmapFile::unlink" )
INT32 _ossMmapFile::unlink ()
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSMMF_UNLINK );
   close() ;
   rc = ossDelete ( _fileName ) ;
   PD_TRACE_EXITRC ( SDB__OSSMMF_UNLINK, rc );
   return rc ;
}

void _ossMmapFile::_clearSeg()
{
   _capacity = 0 ;
   _size = 0 ;
   if ( _pSegArray )
   {
      SDB_OSS_DEL [] _pSegArray ;
      _pSegArray = NULL ;
   }
   if ( _pTmpArray )
   {
      SDB_OSS_DEL [] _pTmpArray ;
      _pTmpArray = NULL ;
   }
}

INT32 _ossMmapFile::_ensureSpace( UINT32 size )
{
   INT32 rc = SDB_OK ;
   ossMmapSegment* pTmp = NULL ;
   UINT32 newSize = 0 ;

   /// first check
   if ( size <= _capacity )
   {
      return rc ;
   }

   engine::ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;

   /// double check
   if ( size <= _capacity )
   {
      goto done ;
   }

   newSize = _capacity << 1 ;
   if ( 0 == newSize )
   {
      newSize = OSS_MMAP_INIT_CAPACITY ;
   }
   if ( newSize < size )
   {
      newSize = size ;
   }

   pTmp = SDB_OSS_NEW ossMmapSegment[ newSize ] ;
   if ( !pTmp )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   /// copy data
   for ( UINT32 i = 0 ; i < _size ; ++i )
   {
      pTmp[ i ] = _pSegArray[ i ] ;
   }

   /// if tmp is not null, need to free first
   if ( _pTmpArray )
   {
      SDB_OSS_DEL [] _pTmpArray ;
      _pTmpArray = NULL ;
   }
   _pTmpArray = _pSegArray ;
   _pSegArray = pTmp ;
   _capacity = newSize ;

done:
   return rc ;
error:
   goto done ;
}

