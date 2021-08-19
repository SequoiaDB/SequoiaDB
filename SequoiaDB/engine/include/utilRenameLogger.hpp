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

   Source File Name = utilRenameLogger.hpp

   Descriptive Name = util rename cs cl

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          12/17/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_RENAMELOGGER_HPP_
#define UTIL_RENAMELOGGER_HPP_

#include <vector>
#include "ossFile.hpp"
#include "ossUtil.hpp"
#include "dms.hpp"

namespace engine
{
   #define UTIL_RENAME_LOG_FILENAME     ".SEQUOIADB_RENAME_INFO"
   #define UTIL_RENAME_LOG_FILESIZE_MAX 512
   #define UTIL_RENAME_LOG_SEP          '='
   #define UTIL_RENAME_LOG_OLDNAME      "oldname"
   #define UTIL_RENAME_LOG_NEWNAME      "newname"
   #define UTIL_RENAME_LOG_FIELD_NUM    2

   struct _utilRenameLog
   {
      CHAR oldName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      CHAR newName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;

      _utilRenameLog()
      {
         ossMemset( oldName, 0, sizeof( oldName ) ) ;
         ossMemset( newName, 0, sizeof( newName ) ) ;
      }

      _utilRenameLog( const CHAR* oldname, const CHAR* newname )
      {
         ossMemset( oldName, 0, sizeof( oldName ) ) ;
         ossStrncpy( oldName, oldname, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         ossMemset( newName, 0, sizeof( newName ) ) ;
         ossStrncpy( newName, newname, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      }

      void toString( CHAR* strLog, UINT32 size ) const
      {
         ossSnprintf( strLog, size - 1,
                      "%s%c%s"OSS_NEWLINE
                      "%s%c%s"OSS_NEWLINE,
                      UTIL_RENAME_LOG_OLDNAME, UTIL_RENAME_LOG_SEP, oldName,
                      UTIL_RENAME_LOG_NEWNAME, UTIL_RENAME_LOG_SEP, newName ) ;
      }
   } ;
   typedef _utilRenameLog utilRenameLog ;

   BOOLEAN utilStr2RenameLog( const string& str, utilRenameLog& log ) ;

   enum UTIL_RENAME_LOGGER_MODE
   {
      UTIL_RENAME_LOGGER_WRITE = 0,
      UTIL_RENAME_LOGGER_READ
   } ;

   class _utilRenameLogger : public SDBObject
   {
      public:
         _utilRenameLogger () ;
         ~_utilRenameLogger () ;

         INT32 init( UTIL_RENAME_LOGGER_MODE mode = UTIL_RENAME_LOGGER_WRITE ) ;
         INT32 log( const utilRenameLog& log ) ;
         INT32 load( utilRenameLog& log );
         INT32 clear() ;
         const CHAR* fileName() ;

      private:
         BOOLEAN _isOpened ;
         OSSFILE _file ;
         CHAR _fileName[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN _fileExist ;
   } ;
   typedef _utilRenameLogger utilRenameLogger ;

}

#endif //UTIL_RENAMELOGGER_HPP_

