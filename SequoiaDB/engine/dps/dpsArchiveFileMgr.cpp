/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dpsArchiveFileMgr.cpp

   Descriptive Name = Data Protection Services Log Archive File Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/12/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsArchiveFileMgr.hpp"
#include "dpsLogFile.hpp"
#include "ossFile.hpp"
#include "utilStr.hpp"
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;

namespace engine
{
   #define DPS_ARCHIVE_TMP_FILE        ".archivelog.tmp"
   #define DPS_ARCHIVE_FILE_PARTIAL    ".p"
   #define DPS_ARCHIVE_FILE_MOVED      ".m"
   #define DPS_ARCHIVE_FILE_SEP        "."

   dpsArchiveFileMgr::dpsArchiveFileMgr()
   {
   }

   dpsArchiveFileMgr::~dpsArchiveFileMgr()
   {
   }

   void dpsArchiveFileMgr::setArchivePath( const std::string& archivePath )
   {
      _archivePath = archivePath ;
      if ( !utilStrEndsWith( _archivePath, OSS_FILE_SEP ) )
      {
         _archivePath += OSS_FILE_SEP ;
      }

      SDB_ASSERT( fs::is_directory( _archivePath ),
                  "archivePath is not directory" ) ;
   }

   const string& dpsArchiveFileMgr::getArchivePath() const
   {
      return _archivePath ;
   }

   string dpsArchiveFileMgr::getFullFilePath( UINT32 logicalFileId )
   {
      stringstream ss ;

      ss << _archivePath 
         << DPS_ARCHIVE_FILE_PREFIX
         << logicalFileId ;

      return ss.str() ;
   }

   string dpsArchiveFileMgr::getPartialFilePath( UINT32 logicalFileId )
   {
      stringstream ss ;

      ss << _archivePath
         << DPS_ARCHIVE_FILE_PREFIX
         << logicalFileId
         << DPS_ARCHIVE_FILE_PARTIAL ;

      return ss.str() ;
   }

   string dpsArchiveFileMgr::getMovedFilePath( UINT32 logicalFileId )
   {
      stringstream ss ;

      ss << _archivePath
         << DPS_ARCHIVE_FILE_PREFIX
         << logicalFileId
         << DPS_ARCHIVE_FILE_MOVED ;

      return ss.str() ;
   }

   string dpsArchiveFileMgr::getTmpFilePath()
   {
      stringstream ss ;

      ss << _archivePath
         << DPS_ARCHIVE_TMP_FILE ;

      return ss.str() ;
   }

   BOOLEAN dpsArchiveFileMgr::isArchiveFileName( const string& fileName )
   {
      if ( utilStrStartsWith( fileName, DPS_ARCHIVE_FILE_PREFIX ) )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN dpsArchiveFileMgr::isFullFileName( const string& fileName )
   {
      SDB_ASSERT( isArchiveFileName( fileName ), "not archive file" ) ;

      vector<string> substrs ;
      substrs = utilStrSplit( fileName, DPS_ARCHIVE_FILE_SEP ) ;
      if ( substrs.size() != 2 )
      {
         return FALSE ;
      }

      if ( !utilStrIsDigit( substrs.at( 1 ) ) )
      {
         return FALSE ;
      }

      return TRUE ;
   }

   BOOLEAN dpsArchiveFileMgr::isPartialFileName( const string& fileName )
   {
      SDB_ASSERT( isArchiveFileName( fileName ), "not archive file" ) ;

      if ( utilStrEndsWith( fileName, DPS_ARCHIVE_FILE_PARTIAL ) )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN dpsArchiveFileMgr::isMovedFileName( const string& fileName )
   {
      SDB_ASSERT( isArchiveFileName( fileName ), "not archive file" ) ;

      if ( utilStrEndsWith( fileName, DPS_ARCHIVE_FILE_MOVED ) )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN dpsArchiveFileMgr::isTmpFileName( const string& fileName )
   {
      if ( DPS_ARCHIVE_TMP_FILE == fileName )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 dpsArchiveFileMgr::getFileId( const string& fileName, UINT32& fileId )
   {
      INT32 rc = SDB_OK ;
      vector<string> substrs ;

      SDB_ASSERT( isArchiveFileName( fileName ), "not archive file name" ) ;

      try
      {
         substrs = utilStrSplit( fileName, DPS_ARCHIVE_FILE_SEP ) ;
         SDB_ASSERT( substrs.size() > 1, "invalid fileName" ) ;

         string fileIdStr = substrs.at( 1 ) ;

         if ( !utilStrIsDigit( fileIdStr ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "invalid fileName[%s]", fileName.c_str() ) ;
            goto error ;
         }

         fileId = ossAtoi( fileIdStr.c_str() ) ;
      }
      catch ( exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::fullFileExists( UINT32 logicalFileId,
                                            BOOLEAN& exist )
   {
      string path = getFullFilePath( logicalFileId ) ;
      return ossFile::exists( path, exist ) ;
   }

   INT32 dpsArchiveFileMgr::partialFileExists( UINT32 logicalFileId,
                                               BOOLEAN& exist )
   {
      string path = getPartialFilePath( logicalFileId ) ;
      return ossFile::exists( path, exist ) ;
   }

   INT32 dpsArchiveFileMgr::movedFileExists( UINT32 logicalFileId,
                                             BOOLEAN& exist )
   {
      string path = getMovedFilePath( logicalFileId ) ;
      return ossFile::exists( path, exist ) ;
   }

   INT32 dpsArchiveFileMgr::copyArchiveFile( const string& src,
                                             const string& dest,
                                             DPS_ARCHIVE_COPY_STATUS status,
                                             utilStreamInterrupt* si )
   {
      INT32 rc = SDB_OK ;
      ossFile srcFile ;
      ossFile destFile ;
      utilFileInStream fileIn ;
      utilFileOutStream fileOut ;
      utilZlibInStream zlibIn ;
      utilZlibOutStream zlibOut ;
      utilStream stream ;
      utilInStream* in = NULL ;
      utilOutStream* out = NULL ;
      INT64 streamSize = 0 ;

      rc = srcFile.open( src, OSS_READONLY, OSS_RU ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc=%d",
                 src.c_str(), rc ) ;
         goto error ;
      }

      rc = destFile.open( dest, OSS_CREATEONLY | OSS_READWRITE, OSS_DEFAULTFILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc=%d",
                 dest.c_str(), rc ) ;
         goto error ;
      }

      rc = fileIn.init( &srcFile, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init file instream, rc=%d", rc ) ;
         goto error ;
      }

      rc = fileOut.init( &destFile, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init file outstream, rc=%d", rc ) ;
         goto error ;
      }

      rc = stream.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init utilStream, rc=%d", rc ) ;
         goto error ;
      }

      in = &fileIn ;
      out = &fileOut ;

      rc = stream.copy( *in, *out, DPS_LOG_HEAD_LEN, &streamSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to copy archive header, rc=%d", rc ) ;
         goto error ;
      }
      if ( DPS_LOG_HEAD_LEN != streamSize )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid archive header size, " \
                 "expect=%lld, actual=%lld",
                 DPS_LOG_HEAD_LEN, streamSize ) ;
         goto error ;
      }

      if ( DPS_ARCHIVE_COPY_COMPRESS == status )
      {
         rc = zlibOut.init( fileOut ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init zlib outstream, rc=%d", rc ) ;
            goto error ;
         }

         in = &fileIn ;
         out = &zlibOut ;
      }
      else if ( DPS_ARCHIVE_COPY_UNCOMPRESS == status )
      {
         rc = zlibIn.init( fileIn ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to init zlib outstream, rc=%d", rc ) ;
            goto error ;
         }

         in = &zlibIn ;
         out = &fileOut ;
      }

      rc = stream.copy( *in, *out, NULL, si ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to copy archive body, rc=%d", rc ) ;
         goto error ;
      }

      out->close() ;
      in->close() ;
      destFile.close() ;
      srcFile.close() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::scanArchiveFiles( UINT32& minFileId,
                                              UINT32& maxFileId,
                                              BOOLEAN allowMoved )
   {
      INT32 rc = SDB_OK ;

      minFileId = DPS_INVALID_LOG_FILE_ID ;
      maxFileId = DPS_INVALID_LOG_FILE_ID ;

      try
      {
         fs::path dir ( _archivePath ) ;
         fs::directory_iterator endIter ;

         for ( fs::directory_iterator dirIter( dir ) ;
               dirIter != endIter ;
               ++dirIter )
         {
            const string fileName = dirIter->path().filename().string() ;
            if ( !isArchiveFileName( fileName ) )
            {
               continue ;
            }

            if ( isFullFileName( fileName ) ||
                 isPartialFileName( fileName ) ||
                 ( allowMoved && isMovedFileName( fileName ) ) )
            {
               UINT32 fileId = 0 ;
               rc = getFileId( fileName, fileId ) ;
               if ( SDB_OK != rc )
               {
                  SDB_ASSERT( FALSE, "invalid fileName" ) ;
                  PD_LOG( PDWARNING, "Failed to get file id of file[%s]",
                          fileName.c_str() ) ;
                  continue ; // ignore this file
               }

               if ( minFileId > fileId || DPS_INVALID_LOG_FILE_ID == minFileId )
               {
                  minFileId = fileId ;
               }

               if ( maxFileId < fileId || DPS_INVALID_LOG_FILE_ID == maxFileId )
               {
                  maxFileId = fileId ;
               }
            }
         }
      }
      catch( fs::filesystem_error& e )
      {
         if ( e.code() == boost::system::errc::permission_denied ||
              e.code() == boost::system::errc::operation_not_permitted )
         {
            rc = SDB_PERM ;
         }
         else
         {
            rc = SDB_IO ;
         }
         goto error ;
      }
      catch( std::exception& e )
      {
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::getTotalSize( INT64& totalSize )
   {
      INT32 rc = SDB_OK ;
      INT64 size = 0 ;

      try
      {
         fs::path dir ( _archivePath ) ;
         fs::directory_iterator endIter ;

         for ( fs::directory_iterator dirIter( dir ) ;
               dirIter != endIter ;
               ++dirIter )
         {
            INT64 fileSize = 0 ;
            const string filePath = dirIter->path().string() ;

            rc = ossFile::getFileSize( filePath, fileSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get file[%s] size, rc=%d",
                       filePath.c_str(), rc ) ;
               goto error ;
            }

            size += fileSize ;
         }

         totalSize = size ;
      }
      catch( fs::filesystem_error& e )
      {
         if ( e.code() == boost::system::errc::permission_denied ||
              e.code() == boost::system::errc::operation_not_permitted )
         {
            rc = SDB_PERM ;
         }
         else
         {
            rc = SDB_IO ;
         }
         goto error ;
      }
      catch( std::exception& e )
      {
         PD_LOG( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::moveArchiveFile( UINT32 fileId, BOOLEAN forward )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN existFull = FALSE ;
      BOOLEAN existPartial = FALSE ;
      BOOLEAN existMoved = FALSE ;
      string partialPath = getPartialFilePath( fileId ) ;
      string fullPath = getFullFilePath( fileId ) ;
      string movedPath = getMovedFilePath( fileId ) ;

      rc = ossFile::exists( fullPath, existFull ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                          fullPath.c_str() , rc ) ;
         goto error ;
      }

      rc = ossFile::exists( partialPath, existPartial ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                          partialPath.c_str() , rc ) ;
         goto error ;
      }

      rc = ossFile::exists( movedPath, existMoved ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                          movedPath.c_str() , rc ) ;
         goto error ;
      }

      if ( existMoved && (!forward || existFull || existPartial ) )
      {
         rc = ossFile::deleteFile( movedPath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s], rc=%d",
                    movedPath.c_str() , rc ) ;
            goto error ;
         }
      }

      if ( existFull )
      {
         rc = ossFile::rename( fullPath, movedPath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to rename file[%s] to [%s], rc=%d",
                    fullPath.c_str(), movedPath.c_str() , rc ) ;
            goto error ;
         }

         PD_LOG( PDEVENT, "Move archive file[%s]", fullPath.c_str() ) ;

         if ( existPartial )
         {
            rc = ossFile::deleteFile( partialPath ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to delete file[%s], rc=%d",
                       partialPath.c_str() , rc ) ;
               goto error ;
            }
         }
      }
      else if ( existPartial )
      {
         rc = ossFile::rename( partialPath, movedPath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to rename file[%s] to [%s], rc=%d",
                    partialPath.c_str(), movedPath.c_str() , rc ) ;
            goto error ;
         }

         PD_LOG( PDEVENT, "Move archive file[%s]", partialPath.c_str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::deleteFile( const string& filePath )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;

      rc = ossFile::exists( filePath, exist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                          filePath.c_str() , rc ) ;
         goto error ;
      }

      if ( exist )
      {
         rc = ossFile::deleteFile( filePath ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s], rc=%d",
                    filePath.c_str() , rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::deleteTmpFile()
   {
      SDB_ASSERT( !_archivePath.empty(), "archive path is empty" ) ;
      return deleteFile( getTmpFilePath() ) ;
   }

   INT32 dpsArchiveFileMgr::deleteArchiveFile( UINT32 fileId )
   {
      INT32 rc = SDB_OK ;

      string partialPath = getPartialFilePath( fileId ) ;
      string fullPath = getFullFilePath( fileId ) ;

      rc = deleteFile( partialPath ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = deleteFile( fullPath ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::deleteFilesByTime( UINT32 minFileId,
                                               UINT32 maxFileId,
                                               time_t time )
   {
      INT32 rc = SDB_OK ;

      for ( UINT32 fileId = minFileId ; fileId <= maxFileId ; fileId++ )
      {
         BOOLEAN continued = FALSE ;
         string movedPath = getMovedFilePath( fileId ) ;
         string partialPath = getPartialFilePath( fileId ) ;
         string fullPath = getFullFilePath( fileId ) ;

         rc = _deleteFileByTime( movedPath, time, continued ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    movedPath.c_str(), rc ) ;
            goto error ;
         }

         if ( !continued )
         {
            break ;
         }

         rc = _deleteFileByTime( partialPath, time, continued ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    partialPath.c_str(), rc ) ;
            goto error ;
         }

         if ( !continued )
         {
            break ;
         }

         rc = _deleteFileByTime( fullPath, time, continued ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    fullPath.c_str(), rc ) ;
            goto error ;
         }

         if ( !continued )
         {
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::_deleteFileByTime( const string& filePath,
                                               time_t time,
                                               BOOLEAN& continued )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      time_t lastTime = 0 ;

      continued = TRUE ;

      rc = ossFile::exists( filePath, exist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      if ( !exist )
      {
         goto done ;
      }

      rc = ossFile::getLastWriteTime( filePath, lastTime ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get last write time of file[%s], rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      if ( lastTime > time )
      {
         continued = FALSE ;
         goto done ;
      }

      rc = ossFile::deleteFile( filePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete expired archive file[%s], rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Delete expired archive file[%s]", filePath.c_str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::deleteFilesBySize( UINT32 minFileId,
                                               UINT32 maxFileId,
                                               INT64 deletedSize )
   {
      INT32 rc = SDB_OK ;

      for ( UINT32 fileId = minFileId ; fileId <= maxFileId ; fileId++ )
      {
         INT64 fileSize = 0 ;
         string movedPath = getMovedFilePath( fileId ) ;
         string partialPath = getPartialFilePath( fileId ) ;
         string fullPath = getFullFilePath( fileId ) ;

         rc = _deleteFileBySize( movedPath, fileSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    movedPath.c_str(), rc ) ;
            goto error ;
         }

         deletedSize -= fileSize ;

         rc = _deleteFileBySize( partialPath, fileSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    partialPath.c_str(), rc ) ;
            goto error ;
         }

         deletedSize -= fileSize ;

         rc = _deleteFileBySize( fullPath, fileSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to delete file[%s] if expired, rc=%d",
                    fullPath.c_str(), rc ) ;
            goto error ;
         }

         deletedSize -= fileSize ;

         if ( deletedSize <= 0 )
         {
            break ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dpsArchiveFileMgr::_deleteFileBySize( const string& filePath,
                                               INT64& deletedSize )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;
      INT64 fileSize = 0 ;

      rc = ossFile::exists( filePath, exist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to check if file[%s] exists, rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      if ( !exist )
      {
         goto done ;
      }

      rc = ossFile::getFileSize( filePath, fileSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get size of file[%s], rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      rc = ossFile::deleteFile( filePath ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete expired archive file[%s], rc=%d",
                 filePath.c_str(), rc ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Delete archive file[%s] for quota", filePath.c_str() ) ;

   done:
      deletedSize = fileSize ;
      return rc ;
   error:
      goto done ;
   }
}

