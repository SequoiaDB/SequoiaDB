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

   Source File Name = omToolCmdDir.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omToolCmdDir.hpp"
#include "utilStr.hpp"
#include "ossIO.hpp"
#include "ossPath.hpp"
#include <vector>

namespace omTool
{
   static const CHAR *_systemDirList[] = {
      "/bin",
      "/boot",
      "/dev",
      "/etc",
      "/lib",
      "/lib64",
      "/lost+found",
      "/media",
      "/proc",
      "/root",
      "/run",
      "/sbin",
      "/snap",
      "/srv",
      "/sys",
      "/tmp",
      "/usr",
      "/var",
   } ;

   #define OMTOOL_SYSTEM_DIR_LIST_NUM (sizeof(_systemDirList)/sizeof(_systemDirList[0]))

   void _splitPath( const CHAR *path, vector<string> &list ) ;

   IMPLEMENT_OMTOOL_CMD_AUTO_REGISTER( omToolCmdDir ) ;

   omToolCmdDir::omToolCmdDir() : _uid( -1 ),
                                  _gid( -1 )
   {
   }

   omToolCmdDir::~omToolCmdDir()
   {
   }

   INT32 omToolCmdDir::doCommand()
   {
      INT32 rc = SDB_OK ;
      string path = _options->path() ;
      string user = _options->user() ;

      rc = _check( path, user ) ;
      if ( rc )
      {
         goto error ;
      }

      {
         CHAR tmpPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         vector<string> list ;
         vector<string>::iterator it ;

         _splitPath( path.c_str(), list ) ;

         for( it = list.begin(); it != list.end(); ++it )
         {
            UINT32 iPermission = 0 ;
            BOOLEAN isExist = TRUE ;

            rc = engine::utilCatPath( tmpPath, OSS_MAX_PATHSIZE, it->c_str() ) ;
            if( rc )
            {
               goto error ;
            }

            rc = _exist( tmpPath, isExist ) ;
            if ( rc )
            {
               goto error ;
            }

            if ( !isExist )
            {
               rc = _mkdir( tmpPath ) ;
               if ( rc )
               {
                  goto error ;
               }

               rc = _setDirOwnership( tmpPath ) ;
               if ( rc )
               {
                  goto error ;
               }
            }

            rc = ossPermissions( tmpPath, iPermission ) ;
            if( rc )
            {
               cout << "Get dir permissions failed, errno: " << rc
                    << ", path: " << tmpPath << endl ;
               goto error ;
            }

            iPermission |= OSS_RO ;
            iPermission |= OSS_XO ;

            rc = _chmod( tmpPath, iPermission ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         rc = _setDirOwnership( tmpPath ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolCmdDir::_check( const string &path, const string &user )
   {
      INT32 rc = SDB_OK ;
      UINT32 i = 0 ;
      BOOLEAN isExist = TRUE ;

      if ( path.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "Invalid path" << endl ;
         goto error ;
      }

      if ( user.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "Invalid user name" << endl ;
         goto error ;
      }

      for( i = 0; i < OMTOOL_SYSTEM_DIR_LIST_NUM; ++i )
      {
         if( path.find( _systemDirList[i] ) == 0 )
         {
            rc = SDB_INVALIDARG ;
            cout << "Invalid path, can't create directories in the system path,"
                    " path: " << path << endl ;
            goto error ;
         }
      }

      rc = _exist( path.c_str(), isExist ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( isExist )
      {
         rc = _isEmpty( path.c_str() ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      rc = ossGetUserInfo( user.c_str(), _uid, _gid ) ;
      if ( rc )
      {
         cout << "Get user id failed, errno: " << rc
              << ", user: " << user << endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolCmdDir::_setDirOwnership( const CHAR *path )
   {
      INT32 rc = SDB_OK ;

      rc = ossChown( path, _uid, _gid ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolCmdDir::_setParentDirPermissions( const CHAR *path )
   {
      INT32 rc = SDB_OK ;
      CHAR tmpPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      vector<string> list ;
      vector<string>::iterator it ;

      _splitPath( path, list ) ;

      for( it = list.begin(); it != list.end(); ++it )
      {
         UINT32 iPermission = 0 ;

         rc = engine::utilCatPath( tmpPath, OSS_MAX_PATHSIZE, it->c_str() ) ;
         if( rc )
         {
            goto error ;
         }

         rc = ossPermissions( tmpPath, iPermission ) ;
         if( rc )
         {
            goto error ;
         }

         iPermission |= OSS_RO ;
         iPermission |= OSS_XO ;

         rc = _chmod( tmpPath, iPermission ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolCmdDir::_mkdir( const CHAR *path )
   {
      INT32 rc = ossMkdir( path, OSS_DEFAULTDIR ) ;
      if ( rc )
      {
         if( SDB_PERM == rc )
         {
            cout << "Failed to change permissions, Permission denied, path: "
                 << path << endl ;
         }
         else if( SDB_FE == rc )
         {
            cout << "Failed to change permissions, dir already exist, path: "
                 << path << endl ;
         }
         else
         {
            cout << "Failed to change permissions, errno: " << rc
                 << ", path: " << path << endl ;
         }
      }
      return rc ;
   }

   INT32 omToolCmdDir::_chmod( const CHAR *path, UINT32 iPermission )
   {
      INT32 rc = ossChmod( path, iPermission ) ;
      if ( rc )
      {
         if( SDB_PERM == rc )
         {
            cout << "Failed to change permissions, Permission denied, path: "
                 << path << endl ;
         }
         else if( SDB_FNE == rc )
         {
            cout << "Failed to change permissions, path not exist, path: "
                 << path << endl ;
         }
         else
         {
            cout << "Failed to change permissions, errno: " << rc
                 << ", path: " << path << endl ;
         }
      }
      return rc ;
   }

   INT32 omToolCmdDir::_exist( const CHAR *path, BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      isExist = FALSE ;

      // try to access file
      rc = ossAccess( path ) ;
      if ( rc && SDB_FNE != rc )
      {
         cout << "Access file failed, path: " << path
              << ", errno: " << rc << endl ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }

      rc = SDB_OK ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolCmdDir::_isEmpty( const CHAR *path )
   {
      INT32 rc = SDB_OK ;
      SDB_OSS_FILETYPE type ;
      multimap< string, string > mapFiles ;
      vector< string > dirs ;

      rc = ossAccess( path ) ;
      if ( rc )
      {
         cout << "Path not exist, path: " << path << endl ;
         goto error ;
      }

      // check path type
      rc = ossGetPathType( path, &type ) ;
      if ( rc )
      {
         cout << "Failed to get path type, path: " << path << endl ;
         goto error ;
      }

      if ( SDB_OSS_DIR != type )
      {
         rc = SDB_INVALIDARG ;
         cout << "Path must be dir, path: " << path << endl ;
         goto error ;
      }

      rc = ossEnumFiles( path, mapFiles, NULL, 1 ) ;
      if( rc )
      {
         cout << "Failed to enum files, path: " << path << endl ;
         goto error ;
      }

      if ( !mapFiles.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "Path is not empty, path: " << path << endl ;
         goto error ;
      }

      rc = ossEnumSubDirs( path, dirs, 1 ) ;
      if( rc )
      {
         cout << "Failed to enum dirs, path: " << path << endl ;
         goto error ;
      }

      if ( !dirs.empty() )
      {
         rc = SDB_INVALIDARG ;
         cout << "Path is not empty, path: " << path << endl ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _splitPath( const CHAR *path, vector<string> &list )
   {
      INT32 i = 0 ;
      INT32 start = 0 ;
      INT32 len = ossStrlen( path ) ;
      const CHAR *p = path ;

      for( ; i < len; ++i )
      {
         if( *p == OSS_FILE_SEP_CHAR && i - start > 0 )
         {
            string tmp( path, start, i - start ) ;

            list.push_back( tmp ) ;
            start = i + 1 ;
         }
         ++p ;
      }

      if( i - start > 0 )
      {
         string tmp( path, start, i - start ) ;

         list.push_back( tmp ) ;
      }
   }
}