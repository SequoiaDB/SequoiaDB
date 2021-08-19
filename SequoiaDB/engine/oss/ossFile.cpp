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

   Source File Name = ossFile.cpp

   Descriptive Name = File operation methods

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
#include "ossFile.hpp"
#include "pd.hpp"
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;

namespace engine
{

   ossFile::ossFile()
   {
   }

   ossFile::~ossFile()
   {
      if ( isOpened() )
      {
         close() ;
      }
   }

   INT32 ossFile::open( const string& filePath, UINT32 mode, UINT32 permission )
   {
      INT32 rc = SDB_OK ;

      rc = ossOpen( filePath.c_str(), mode, permission, _file ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _path = filePath ;

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 ossFile::close()
   {
      if ( _file.isOpened() )
      {
         _path = "" ;
         return ossClose( _file ) ;
      }
      else
      {
         return SDB_OK ;
      }
   }

   BOOLEAN ossFile::isOpened() const
   {
      return _file.isOpened() ;
   }
   
   INT32 ossFile::seek( INT64 offset, OSS_SEEK whence, INT64* position )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeek( &_file, offset, whence, position ) ;
   }
   
   INT32 ossFile::sync()
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossFsync( &_file ) ;
   }

   INT32 ossFile::extend( const INT64 incrementSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossExtendFile( &_file, incrementSize ) ;
   }

   INT32 ossFile::truncate( const INT64 fileLen )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossTruncateFile( &_file, fileLen ) ;
   }
   
   INT32 ossFile::getFileSize( INT64& size )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossGetFileSize( &_file, &size ) ;
   }
   
   const string& ossFile::getPath() const
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return _path ;
   }
      
   INT32 ossFile::read( CHAR* buffer, INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossRead( &_file, buffer, bufferLen, &readSize ) ;
   }
   
   INT32 ossFile::readN( CHAR* buffer, INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossReadN( &_file, bufferLen, buffer, readSize ) ;
   }
   
   INT32 ossFile::seekAndRead( INT64 offset, CHAR* buffer,
                               INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndRead( &_file, offset, buffer, bufferLen, &readSize ) ;
   }
   
   INT32 ossFile::seekAndReadN( INT64 offset, CHAR* buffer,
                                INT64 bufferLen, INT64& readSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndReadN( &_file, offset, bufferLen, buffer, readSize ) ;
   }
      
   INT32 ossFile::write( const CHAR* buffer, INT64 bufferLen, INT64& writeSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossWrite( &_file, buffer, bufferLen, &writeSize ) ;
   }
   
   INT32 ossFile::writeN( const CHAR* buffer, INT64 bufferLen )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossWriteN( &_file, buffer, bufferLen ) ;
   }
   
   INT32 ossFile::seekAndWrite( INT64 offset, const CHAR* buffer,
                                INT64 bufferLen, INT64& writeSize )
   {
      SDB_ASSERT( isOpened(), "file should be opened" ) ;
      return ossSeekAndWrite( &_file, offset, buffer, bufferLen, &writeSize ) ;
   }
   
   INT32 ossFile::seekAndWriteN( INT64 offset, const CHAR* buffer, 
                                 INT64 bufferLen )
   {
      INT64 writeSize = 0 ;
      INT32 rc = SDB_OK ;
      
      SDB_ASSERT( isOpened(), "file should be opened" ) ;

      rc = ossSeekAndWriteN( &_file, offset, buffer, bufferLen, writeSize ) ;
      if ( SDB_OK == rc )
      {
         SDB_ASSERT( writeSize == bufferLen, "writeSize != bufferLen" ) ;
      }

      return rc ;
   }

   INT32 ossFile::exists( const string& filePath, BOOLEAN& exist )
   {
      INT32 rc = SDB_OK ;

      rc = ossAccess( filePath.c_str() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            exist = FALSE ;
            rc = SDB_OK ;
            goto done ;
         }

         goto error ;
      }

      exist = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 ossFile::deleteFile( const string& filePath )
   {
      return ossDelete( filePath.c_str() ) ;
   }

   INT32 ossFile::deleteFileIfExists( const string& filePath )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN exist = FALSE ;

      rc = ossFile::exists( filePath, exist ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( !exist )
      {
         goto done;
      }

      rc = ossFile::deleteFile( filePath ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 ossFile::getFileSize( const string& filePath, INT64& fileSize )
   {
      return ossGetFileSizeByName( filePath.c_str(), &fileSize ) ;
   }

   INT32 ossFile::getLastWriteTime( const string& filePath, time_t& time )
   {
      INT32 rc = SDB_OK ;

      try
      {
         fs::path file ( filePath ) ;
         time = fs::last_write_time( file ) ;
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

   INT32 ossFile::rename( const string& oldFilePath, const string& newFilePath )
   {
      return ossRenamePath( oldFilePath.c_str(), newFilePath.c_str() ) ;
   }

   INT32 ossFile::extend( const string& filePath, INT64 increment )
   {
      INT32 rc = SDB_OK ;
      ossFile file ;

      rc = file.open( filePath, OSS_READWRITE, OSS_DEFAULTFILE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc=%d",
                 filePath.c_str(), rc ) ;
         goto error;
      }

      rc = file.extend( increment ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extend file[%s] with %lld size, rc=%d",
                 filePath.c_str(), increment, rc ) ;
         goto error ;
      }

   done:
      file.close() ;
      return rc ;
   error:
      goto done ;
   }

   string ossFile::getFileName( const string& filePath )
   {
      fs::path file ( filePath ) ;
      return file.filename().string() ;
   }
}

