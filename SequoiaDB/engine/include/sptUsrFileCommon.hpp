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

   Source File Name = sptUsrFileCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/26/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRFILE_COMMON_HPP_
#define SPT_USRFILE_COMMON_HPP_

#include "../bson/bson.hpp"
#include <string>

namespace engine
{
   #define SPT_FILE_COMMON_FIELD_PERMISSION     "permission"
   #define SPT_FILE_COMMON_FIELD_MODE           "mode"
   #define SPT_FILE_COMMON_FIELD_SIZE           "size"
   #define SPT_FILE_COMMON_FIELD_WHERE          "where"
   #define SPT_FILE_COMMON_FIELD_IS_REPLACE     "isReplace"
   #define SPT_FILE_COMMON_FIELD_VALUE          "value"
   #define SPT_FILE_COMMON_FIELD_PATHNAME       "pathname"
   #define SPT_FILE_COMMON_FIELD_DETAIL         "detail"
   #define SPT_FILE_COMMON_FIELD_IS_RECURSIVE   "isRecursive"
   #define SPT_FILE_COMMON_FIELD_USERNAME       "username"
   #define SPT_FILE_COMMON_FIELD_GROUPNAME      "groupname"

   class _sptUsrFileCommon: public SDBObject
   {
   public:
      _sptUsrFileCommon() ;

      ~_sptUsrFileCommon() ;

      inline string getFileName()
      {
         return _filename ;
      }

      INT32 open( const string &filename, bson::BSONObj optionObj,
                  std::string &err ) ;

      INT32 read( const bson::BSONObj &optionObj, std::string &err,
                  CHAR** buf, SINT64 &readLen ) ;

      INT32 readLine( std::string &err, CHAR** buf, SINT64 &readLen ) ;

      INT32 write( const CHAR* buf, SINT64 size, std::string &err ) ;

      INT32 truncate( INT64 size, string &err ) ;

      INT32 seek( INT64 seekSize, const bson::BSONObj &optionObj,
                  std::string &err ) ;

      INT32 close( std::string &err ) ;

      static INT32 remove( const std::string &path, std::string &err ) ;

      static INT32 exist( const std::string &path, std::string &err,
                          BOOLEAN &isExist ) ;

      static INT32 copy( const std::string &src, const std::string &dst,
                         const bson::BSONObj &optionObj, std::string &err ) ;

      static INT32 mkdir( const std::string &name,
                          const bson::BSONObj &optionObj, std::string &err ) ;

      static INT32 move( const std::string &src, const std::string &dst,
                         std::string &err ) ;

      static INT32 find( const bson::BSONObj &optionObj, std::string &err,
                         bson::BSONObj &retObj ) ;

      static INT32 list( const bson::BSONObj &optionObj, std::string &err,
                         bson::BSONObj &retObj ) ;

      static INT32 chmod( std::string &pathname, INT32 mode,
                          bson::BSONObj optionObj, std::string &err ) ;

      static INT32 chown( std::string &pathname, bson::BSONObj ownerObj,
                          bson::BSONObj optionObj, std::string &err ) ;

      static INT32 chgrp( std::string &pathname, std::string groupname,
                          bson::BSONObj optionObj,std::string &err ) ;

      static INT32 getUmask( std::string &err, std::string &retStr ) ;

      static INT32 setUmask( INT32 mask, std::string &err ) ;

      static INT32 getPathType( const std::string &pathname,
                                std::string &err,
                                std::string &pathType ) ;

      static INT32 getPermission( const std::string &pathname,
                                  std::string &err,
                                  INT32 &permission ) ;

      static INT32 isEmptyDir( const std::string &pathname,
                               std::string &err,
                               BOOLEAN &isEmpty ) ;

      static INT32 getStat( const std::string &pathname,
                            std::string &err,
                            bson::BSONObj &retObj ) ;

      static INT32 md5( const std::string &filename,
                        std::string &err,
                        std::string &code ) ;

      static INT32 readFile( const std::string &filename, std::string &err,
                             CHAR **pBuf, INT64 &readSize ) ;

      static INT32 getFileSize( const std::string &filename,
                                std::string &err, INT64 &size ) ;
   private:
      static INT32 _extractFindInfo( const CHAR* buf,
                                     bson::BSONObjBuilder &builder ) ;

      static INT32 _extractListInfo( const CHAR* buf,
                                     bson::BSONObjBuilder &builder,
                                     BOOLEAN showDetail ) ;

   private:
      OSSFILE _file ;
      string  _filename ;
   } ;

   typedef _sptUsrFileCommon sptUsrFileCommon ;
}
#endif
