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

   Source File Name = utilJsonFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilJsonFile.hpp"
#include "ossMem.h"
#include "pd.hpp"
#include "../util/fromjson.hpp"

namespace engine
{
   INT32 utilJsonFile::read( ossFile& file, bson::BSONObj& data )
   {
      INT64 fileSize = 0 ;
      INT64 readSize = 0 ;
      CHAR* buf = NULL ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( file.isOpened(), "file should be opened" ) ;

      rc = file.getFileSize( fileSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get file size, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( fileSize >= 0, "file size should be >= 0" ) ;

      if ( 0 == fileSize )
      {
         // no data in file
         goto done ;
      }

      buf = (CHAR*)SDB_OSS_MALLOC( fileSize + 1 ) ; // one more byte for safe
      if ( NULL == buf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc, mem size = %lld, rc=%d",
                 fileSize + 1, rc ) ;
         goto error ;
      }

      rc = file.seekAndReadN( 0, buf, fileSize, readSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read from file, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( readSize == fileSize, "readSize != fileSize" ) ;
      buf[ readSize ] = '\0' ; // safe guard

      rc = bson::fromjson( buf, data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to convert json[%s] to bson, rc=%d",
                 buf, rc ) ;
         goto error ;
      }

   done:
      SAFE_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 utilJsonFile::write( ossFile& file, bson::BSONObj& data )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( file.isOpened(), "file must be opened" ) ;

      string json = data.toString() ;

      rc = file.truncate( 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to truncate json file, rc=%d", rc );
         goto error ;
      }

      rc = file.seekAndWriteN( 0, json.c_str(), json.size() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write json to file, " \
                 "json size=%d, rc=%d",
                 json.size(), rc );
         goto error ;
      }

      rc = file.sync() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to sync json file, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

