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

   Source File Name = clsLocalValidation.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsLocalValidation.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"

#define CLS_DISK_DETECT_FILE_NAME     ".SEQUOIADB_DISK_DETECT_FILE"
#define CLS_DISK_DETECT_FILE_CONTENT  "N"
#define CLS_DISK_DETECT_INTERVAL_TIME ( OSS_ONE_SEC * 60 )

using namespace bson ;

namespace engine
{
   _clsDiskDetector::_clsDiskDetector()
   {
      _isMonitoredRole = TRUE ;
      _lastTick = 0 ;
      _hasInit = FALSE ;
   }

   _clsDiskDetector::~_clsDiskDetector()
   {
   }

   INT32 _clsDiskDetector::detect()
   {
      INT32  rc = SDB_OK ;

      if ( !_hasInit )
      {
         rc = init() ;
         if ( rc )
         {
            goto error ;
         }
         _hasInit = TRUE ;
      }

      if ( !_isNeedToDetect() )
      {
         goto done ;
      }

      if ( _it != _filePathsSet.end() )
      {
         // The file in the _filePathsSet are written in turns,
         // and only one file is written at a time
         rc = _tryToWriteFile( _it->c_str() ) ;
         if ( rc )
         {
            goto error ;
         }
         ++_it ;

         if ( _it == _filePathsSet.end() )
         {
            _it = _filePathsSet.begin() ;
         }
      }

      _lastTick = pmdGetDBTick() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsDiskDetector::_isNeedToDetect()
   {
      if ( _isMonitoredRole &&
           pmdGetTickSpanTime( _lastTick ) >= CLS_DISK_DETECT_INTERVAL_TIME )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _clsDiskDetector::init()
   {
      INT32 rc = SDB_OK ;
      SDB_ROLE role = utilGetRoleEnum( pmdGetOptionCB()->dbroleStr() ) ;

      if ( SDB_ROLE_COORD == role || SDB_ROLE_MAX == role )
      {
         _isMonitoredRole = FALSE ;
         _hasInit = TRUE ;
         goto done ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getDbPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init db path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getIndexPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init index path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getLobPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init lob path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getLobMetaPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init lobm path, rc: %d", rc ) ;
         goto error ;
      }

      rc = _addFilePath( pmdGetOptionCB()->getReplLogPath() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init replicalog path, rc: %d", rc ) ;
         goto error ;
      }

      _it = _filePathsSet.begin() ;
      _hasInit = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDiskDetector::_addFilePath( const CHAR* pFilePath )
   {
      SDB_ASSERT( pFilePath, "pFilePath can't be null" ) ;

      INT32 rc = SDB_OK ;
      CHAR tmpFilePath[ OSS_MAX_PATHSIZE +  1 ] = { 0 } ;

      // eg: /opt/sequoiadb/databases/20000/ --->
      //     /opt/sequoiadb/databases/20000/.SEQUOIADB_DISK_DETECT_FILE
      rc = utilBuildFullPath( pFilePath, CLS_DISK_DETECT_FILE_NAME,
                              OSS_MAX_PATHSIZE, tmpFilePath ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build disk detect file path, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         _filePathsSet.insert( tmpFilePath ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when inserting file path:"
                 " %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsDiskDetector::_tryToWriteFile( const CHAR* pFilePath )
   {
      SDB_ASSERT( pFilePath, "pFilePath can't be null" ) ;

      UINT32 rc = SDB_OK ;
      OSSFILE file ;

      rc = ossOpen( pFilePath, OSS_REPLACE | OSS_WRITEONLY, OSS_WU | OSS_RU,
                    file ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to open file[%s], rc: %d", pFilePath, rc ) ;
         goto error ;
      }

      rc = ossWriteN( &file, CLS_DISK_DETECT_FILE_CONTENT,
                      ossStrlen( CLS_DISK_DETECT_FILE_CONTENT ) ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write file[%s], rc: %d", pFilePath, rc ) ;
         goto error ;
      }

   done:
      if ( file.isOpened() )
      {
         ossClose( file ) ;
         ossDelete( pFilePath ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsLocalValidation::run()
   {
      INT32 rc = SDB_OK ;

      if ( pmdGetOptionCB()->detectDisk() )
      {
         rc = _diskDetector.detect() ;
         if ( rc == SDB_IO )
         {
            PD_LOG( PDERROR, "Unexpected error" ) ;
            goto error ;
         }
      }

      /// 4. update validation tick
      pmdUpdateValidationTick() ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

