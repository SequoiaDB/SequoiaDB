/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = ossPath.cpp

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

#include "ossPath.hpp"
#include "ossErr.h"
#include "ossUtil.h"
#include "ossMem.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;

static INT32 _ossEnumFiles( const string &dirPath,
                            multimap<string, string> &mapFiles,
                            const CHAR *filter, UINT32 filterLen,
                            OSS_MATCH_TYPE type, UINT32 deep )
{
   INT32 rc = SDB_OK ;
   const CHAR *pFind = NULL ;

   try
   {
      fs::path dbDir ( dirPath ) ;
      fs::directory_iterator end_iter ;

      if ( 0 == deep )
      {
         goto done ;
      }

      if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
      {
         for ( fs::directory_iterator dir_iter ( dbDir );
               dir_iter != end_iter; ++dir_iter )
         {
            try
            {
               if ( fs::is_regular_file ( dir_iter->status() ) )
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
                     mapFiles.insert( pair<string, string>( fileName,
                                      dir_iter->path().string() ) );
                  }
               }
               else if ( fs::is_directory( dir_iter->path() ) && deep > 1 )
               {
                  _ossEnumFiles( dir_iter->path().string(), mapFiles,
                                 filter, filterLen, type, deep - 1 ) ;
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "File or dir[%s] occur exception: %s",
                       dir_iter->path().string().c_str(),
                       e.what() ) ;
            }
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   catch ( fs::filesystem_error& e )
   {
      if ( e.code() == boost::system::errc::permission_denied ||
           e.code() == boost::system::errc::operation_not_permitted )
      {
         rc = SDB_PERM ;
      }
      else if( e.code() == boost::system::errc::too_many_files_open ||
               e.code() == boost::system::errc::too_many_files_open_in_system )
      {
         rc = SDB_TOO_MANY_OPEN_FD ;
      }
      else
      {
         PD_LOG( PDERROR, "Enum directory[%s] failed, errno: %d",
                 dirPath.c_str(), e.code() ) ;
         rc = SDB_IO ;
      }
      goto error ;
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

INT32 ossEnumFiles( const string &dirPath,
                    multimap< string, string > &mapFiles,
                    const CHAR *filter,
                    UINT32 deep )
{
   string newFilter ;
   OSS_MATCH_TYPE type = OSS_MATCH_NULL ;

   if ( !filter || filter[0] == 0 || 0 == ossStrcmp( filter, "*" ) )
   {
      type = OSS_MATCH_NULL ;
   }
   else if ( filter[0] != '*' && filter[ ossStrlen( filter ) - 1 ] != '*' )
   {
      type = OSS_MATCH_ALL ;
      newFilter = filter ;
   }
   else if ( filter[0] == '*' && filter[ ossStrlen( filter ) - 1 ] == '*' )
   {
      type = OSS_MATCH_MID ;
      newFilter.assign( &filter[1], ossStrlen( filter ) - 2 ) ;
   }
   else if ( filter[0] == '*' )
   {
      type = OSS_MATCH_RIGHT ;
      newFilter.assign( &filter[1], ossStrlen( filter ) - 1 ) ;
   }
   else
   {
      type = OSS_MATCH_LEFT ;
      newFilter.assign( filter, ossStrlen( filter ) -1 ) ;
   }

   return _ossEnumFiles( dirPath, mapFiles, newFilter.c_str(),
                         newFilter.length(), type, deep ) ;
}

INT32 ossEnumFiles2( const string &dirPath,
                     multimap<string, string> &mapFiles,
                     const CHAR *filter,
                     OSS_MATCH_TYPE type,
                     UINT32 deep )
{
   return _ossEnumFiles( dirPath, mapFiles, filter,
                         filter ? ossStrlen( filter ) : 0,
                         type, deep ) ;
}

static INT32 _ossEnumSubDirs( const string &dirPath,
                              const string &parentSubDir,
                              vector< string > &subDirs,
                              UINT32 deep )
{
   INT32 rc = SDB_OK ;

   try
   {
      fs::path dbDir ( dirPath ) ;
      fs::directory_iterator end_iter ;

      string subDir ;

      if ( 0 == deep )
      {
         goto done ;
      }

      if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
      {
         for ( fs::directory_iterator dir_iter ( dbDir );
               dir_iter != end_iter; ++dir_iter )
         {
            try
            {
               if ( fs::is_directory( dir_iter->path() ) )
               {
                  if ( parentSubDir.empty() )
                  {
                     subDir = dir_iter->path().leaf().string() ;
                  }
                  else
                  {
                     string subDir = parentSubDir ;
                     subDir += OSS_FILE_SEP ;
                     subDir += dir_iter->path().leaf().string() ;
                  }
                  subDirs.push_back( subDir ) ;

                  if ( deep > 1 )
                  {
                     _ossEnumSubDirs( dir_iter->path().string(), subDir,
                                      subDirs,deep - 1 ) ;
                  }
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "File or dir[%s] occur exception: %s",
                       dir_iter->path().string().c_str(), e.what() ) ;
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

INT32 ossEnumSubDirs( const string &dirPath, vector < string > &subDirs,
                      UINT32 deep )
{
   return _ossEnumSubDirs( dirPath, "", subDirs, deep ) ;
}

