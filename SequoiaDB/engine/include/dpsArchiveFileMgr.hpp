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

   Source File Name = dpsArchiveFileMgr.hpp

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
#ifndef DPS_ARCHIVE_FILE_MGR_HPP_
#define DPS_ARCHIVE_FILE_MGR_HPP_

#include "dpsArchiveFile.hpp"
#include "oss.hpp"
#include <string>
#include <ctime>

using namespace std ;

namespace engine
{
   enum DPS_ARCHIVE_COPY_STATUS
   {
      DPS_ARCHIVE_COPY_PLAIN = 0,
      DPS_ARCHIVE_COPY_COMPRESS = 1,
      DPS_ARCHIVE_COPY_UNCOMPRESS = 2,
   } ;

   class dpsArchiveFileMgr: public SDBObject
   {
   public:
      dpsArchiveFileMgr() ;
      ~dpsArchiveFileMgr() ;

   public:
      void setArchivePath( const string& archivePath ) ;
      const string& getArchivePath() const ;

      string getFullFilePath( UINT32 logicalFileId ) ;
      string getPartialFilePath( UINT32 logicalFileId ) ;
      string getMovedFilePath( UINT32 logicalFileId ) ;
      string getTmpFilePath() ;

      BOOLEAN isArchiveFileName( const string& fileName ) ;
      BOOLEAN isFullFileName( const string& fileName ) ;
      BOOLEAN isPartialFileName( const string& fileName ) ;
      BOOLEAN isMovedFileName( const string& fileName ) ;
      BOOLEAN isTmpFileName( const string& fileName ) ;
      INT32   getFileId( const string& fileName, UINT32& fileId ) ;

      INT32 fullFileExists( UINT32 logicalFileId, BOOLEAN& exist ) ;
      INT32 partialFileExists( UINT32 logicalFileId, BOOLEAN& exist ) ;
      INT32 movedFileExists( UINT32 logicalFileId, BOOLEAN& exist ) ;

      INT32 copyArchiveFile( const string& src, const string& dest,
               DPS_ARCHIVE_COPY_STATUS status = DPS_ARCHIVE_COPY_PLAIN,
               utilStreamInterrupt* si = NULL ) ;

      INT32 scanArchiveFiles( UINT32& minFileId,
                                  UINT32& maxFileId,
                                  BOOLEAN allowMoved = FALSE ) ;
      INT32 getTotalSize( INT64& totalSize ) ;

      INT32 moveArchiveFile( UINT32 fileId, BOOLEAN forward = FALSE ) ;

      INT32 deleteFile( const string& filePath ) ;
      INT32 deleteTmpFile() ;
      INT32 deleteArchiveFile( UINT32 fileId ) ;
      INT32 deleteFilesByTime( UINT32 minFileId,
                                   UINT32 maxFileId,
                                   time_t time ) ;
      INT32 deleteFilesBySize( UINT32 minFileId,
                               UINT32 maxFileId,
                               INT64 deletedSize ) ;

   private:
      INT32 _deleteFileByTime( const string& filePath,
                               time_t time,
                               BOOLEAN& continued ) ;
      INT32 _deleteFileBySize( const string& filePath,
                               INT64& deletedSize ) ;

   private:
      string   _archivePath ;
   } ;
}

#endif /* DPS_ARCHIVE_FILE_MGR_HPP_ */
