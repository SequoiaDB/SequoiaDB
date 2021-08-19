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

   Source File Name = ossFile.hpp

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
#ifndef OSS_FILE_HPP_
#define OSS_FILE_HPP_

#include "oss.hpp"
#include "ossIO.hpp"
#include <string>
#include <ctime>

using namespace std ;

namespace engine
{
   class ossFile: public SDBObject
   {
   private:
      // disallow copy and assign
      ossFile( const ossFile& ) ;
      void operator=( const ossFile& ) ;

   public:
      ossFile() ;
      ~ossFile() ;

   public:
      static INT32 exists( const string& filePath, BOOLEAN& exist ) ;
      static INT32 deleteFile( const string& filePath ) ;
      static INT32 deleteFileIfExists( const string& filePath ) ;
      static INT32 getFileSize( const string& filePath, INT64& fileSize ) ;
      static INT32 getLastWriteTime( const string& filePath, time_t& time ) ;
      static INT32 rename( const string& oldFilePath, const string& newFilePath ) ;
      static INT32 extend( const string& filePath, INT64 increment ) ;
      static string getFileName( const string& filePath ) ; 

   public:
      INT32    open( const string& filePath, UINT32 mode, UINT32 permission ) ;
      INT32    close() ;
      BOOLEAN  isOpened() const ;
      INT32    seek( INT64 offset, OSS_SEEK whence = OSS_SEEK_SET,
                     INT64* position = NULL ) ;
      INT32    sync() ;
      INT32    extend( const INT64 incrementSize ) ;
      INT32    truncate( const INT64 fileLen ) ;
      INT32    getFileSize( INT64& size ) ;
      const string&  getPath() const ;
      
      INT32    read( CHAR* buffer, INT64 bufferLen, INT64& readSize ) ;
      INT32    readN( CHAR* buffer, INT64 bufferLen, INT64& readSize ) ;
      INT32    seekAndRead( INT64 offset, CHAR* buffer,
                            INT64 bufferLen, INT64& readSize ) ;
      INT32    seekAndReadN( INT64 offset, CHAR* buffer,
                             INT64 bufferLen, INT64& readSize ) ;
      
      INT32    write( const CHAR* buffer, INT64 bufferLen, INT64& writeSize ) ;
      INT32    writeN( const CHAR* buffer, INT64 bufferLen ) ;
      INT32    seekAndWrite( INT64 offset, const CHAR* buffer,
                             INT64 bufferLen, INT64& writeSize ) ;
      INT32    seekAndWriteN( INT64 offset, const CHAR* buffer,
                              INT64 bufferLen ) ;

      OSS_INLINE INT32 position( INT64& pos )
      {
         return seek( 0, OSS_SEEK_CUR, &pos ) ;
      }

   private:
      _OSS_FILE  _file ;
      string     _path ;
   } ;
}

#endif /* OSS_FILE_HPP_ */

