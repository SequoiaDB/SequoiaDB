/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilCipherFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilCipherFile.hpp"
#include "ossUtil.hpp"

#ifdef _LINUX
#include <pwd.h>
#define UTIL_CIPHER_FILE_SUFFIX_PATH   "/sequoiadb/passwd"
#else
#define UTIL_USER_DIRECTORY            "USERPROFILE"
#define UTIL_CIPHER_FILE_SUFFIX_PATH   "\\sequoiadb\\passwd"
#endif

_utilCipherFile::~_utilCipherFile()
{
   if ( _isOpen )
   {
      _file.close() ;
      _isOpen = FALSE ;
   }
}

INT32 _utilCipherFile::init( string &filePath, UINT32 role )
{
   INT32  rc    = SDB_OK ;
   CHAR   *ptr  = NULL ;
   UINT32 iMode = ( ( R_ROLE == role ) ?
                    OSS_READONLY : ( OSS_CREATE | OSS_READWRITE ) ) ;
   CHAR   path[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

   if ( filePath.empty() )
   {
      rc = utilBuildDefaultCipherFilePath( _filePath ) ;
      if ( rc )
      {
         goto error ;
      }
      filePath = _filePath ;
   }

   //create dir
   if ( filePath.empty() )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "File path can't be empty, rc: %d", rc ) ;
      goto error ;
   }
   if ( filePath.length() > OSS_MAX_PATHSIZE )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG ( PDERROR, "File path[%s] is too long, rc: %d",
               filePath.c_str(), rc ) ;
      goto error ;
   }

   ossStrncpy( path, filePath.c_str(), OSS_MAX_PATHSIZE ) ;
   ptr = ossStrrchr( path, OSS_FILE_SEP_CHAR ) ;
   if ( ptr != NULL && ptr != &path[0] )
   {
      *ptr = 0 ;
      if ( ossAccess( path ) )
      {
         ossMkdir( path ) ;
      }
   }

   rc = _file.open( filePath, iMode, OSS_RU | OSS_WU ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Can't open cipher file[%s], rc: %d",
               filePath.c_str(), rc ) ;
      goto error;
   }
   _filePath = filePath ;
   _isOpen = TRUE ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _utilCipherFile::read( CHAR **fileContent, INT64 &contentLen )
{
   INT32 rc = SDB_OK ;
   INT64 fileSize = 0 ;

   if ( !_isOpen )
   {
      rc = SDB_SYS ;
      PD_LOG ( PDERROR, "Failed to read cipher file[%s]. The cipher file"
               " is not opened now, rc: %d", _filePath.c_str(), rc ) ;
      goto error;
   }

   rc = _file.getFileSize( fileSize ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to get size of cipher file[%s], rc: %d",
               _filePath.c_str(), rc ) ;
      goto error;
   }

   if ( 0 == fileSize )
   {
      PD_LOG ( PDDEBUG, "Cipher file[%s] is empty", _filePath.c_str() ) ;
      goto done;
   }

   *fileContent = ( CHAR* )SDB_OSS_MALLOC( fileSize ) ;
   if ( NULL == *fileContent )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to malloc for file content, rc: %d", rc ) ;
      goto error ;
   }

   rc = _file.read( *fileContent, fileSize, contentLen ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to read cipher file[%s], rc: %d",
               _filePath.c_str(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   if( *fileContent )
   {
      SDB_OSS_FREE( *fileContent ) ;
      *fileContent = NULL ;
   }
   goto done ;
}

INT32 _utilCipherFile::write( const string &fileContent )
{
   INT32  rc   = SDB_OK ;
   INT64  wCnt = 0 ;
   UINT64 len  = fileContent.length() ;

   if ( !_isOpen )
   {
      rc = SDB_SYS ;
      PD_LOG ( PDERROR, "Failed to write cipher file[%s]. The cipher file"
               " is not opened now, rc: %d", _filePath.c_str(), rc ) ;
      goto error;
   }

   rc = _file.seek( 0 ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to seek the cipher file[%s], rc: %d",
               getFilePath(), rc ) ;
      goto error ;
   }

   rc = _file.truncate( 0 ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to truncate the cipher file[%s], rc: ",
               getFilePath(), rc ) ;
      goto error ;
   }

   rc = _file.write( fileContent.c_str(), len, wCnt ) ;
   if( SDB_OK != rc )
   {
      PD_LOG ( PDERROR, "Failed to write cipher text to the cipher file[%s],"
               " rc: ", getFilePath(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilBuildDefaultCipherFilePath( string &cipherFilePath )
{
   INT32 rc = SDB_OK ;
   string cipherFilePathTmp ;

   try
   {
#ifdef _LINUX
      struct passwd *pwd = getpwuid( getuid() ) ;
      cipherFilePathTmp = pwd->pw_dir ;
#else
      cipherFilePathTmp = getenv( UTIL_USER_DIRECTORY ) ;
#endif
      if ( cipherFilePathTmp.empty() )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Failed to get user directory, rc: %d", rc ) ;
         goto error ;
      }
      cipherFilePathTmp += UTIL_CIPHER_FILE_SUFFIX_PATH ;
      cipherFilePath = cipherFilePathTmp ;
   }
   catch( std::exception )
   {
      rc = SDB_OOM ;
      PD_LOG ( PDERROR, "Failed to build cipher file path, rc: %d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

string utilGetUserShortNameFromUserFullName( const string &userFullName,
                                             string *clusterName )
{
   // The format of userFullName and userShortName are as follows:
   // userFullName  = "admin@cluster"
   // userFullName  = "admin"
   // userShortName = "admin"
   string userShortName ;
   string::size_type atPos = userFullName.find_last_of( "@" ) ;

   if ( string::npos != atPos )
   {
      userShortName = userFullName.substr( 0, atPos ) ;

      if ( clusterName )
      {
         *clusterName = userFullName.substr( atPos + 1 ) ;
      }
   }
   else
   {
      userShortName = userFullName ;
   }

   return userShortName ;
}

