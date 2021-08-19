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

   Source File Name = rplStatus.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplMonitorStore.hpp"
#include "utilJsonFile.hpp"
#include "../bson/bson.hpp"
#include <string>

using namespace std ;
using namespace bson ;
using namespace engine ;

namespace replay
{
   #define RPL_STATUS_NEXT_LSN               "nextLSN"
   #define RPL_STATUS_NEXT_FILEID            "nextFileId"
   #define RPL_STATUS_LAST_LSN               "lastLSN"
   #define RPL_STATUS_LAST_FILEID            "lastFileId"
   #define RPL_STATUS_LAST_MOVED_FILETIME    "lastMovedFileTime"
   #define RPL_STATUS_OUTPUT_TYPE            "outputType"
   #define RPL_STATUS_SERIAL                 "serial"
   #define RPL_STATUS_SUMMIT_TIME            "summitTime"
   #define RPL_STATUS_EXTRA                  "extra"

   rplMonitorStore::rplMonitorStore( Monitor *monitor )
   {
      _outputter = NULL ;
      _monitor = monitor ;
   }

   rplMonitorStore::~rplMonitorStore()
   {
      _outputter = NULL ;
      _monitor = NULL ;
   }

   void rplMonitorStore::captureOutputter( rplOutputter *outputter )
   {
      _outputter = outputter ;
   }

   INT32 rplMonitorStore::_initMonitor( ossFile& statusFile )
   {
      INT32 rc = SDB_OK ;
      BSONObj data ;
      BSONElement ele ;

      rc = utilJsonFile::read( statusFile, data ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read json from status file[%s], "
                   "rc=%d", statusFile.getPath().c_str(), rc) ;

      if ( data.isEmpty() )
      {
         _monitor->setIsLoadFromFile( FALSE ) ;
      }
      else
      {
         _monitor->setIsLoadFromFile( TRUE ) ;
      }

      ele = data.getField( RPL_STATUS_NEXT_LSN ) ;
      if ( EOO != ele.type() )
      {
         DPS_LSN_OFFSET nextLSN ;
         if ( NumberLong != ele.type() && NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_NEXT_LSN ) ;
         }

         nextLSN = ( DPS_LSN_OFFSET )ele.numberLong() ;
         if ( nextLSN < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid value of field[%s]: %d",
                         RPL_STATUS_NEXT_LSN, nextLSN ) ;
         }

         _monitor->setNextLSN(nextLSN) ;
      }

      ele = data.getField( RPL_STATUS_LAST_LSN ) ;
      if ( EOO != ele.type() )
      {
         DPS_LSN_OFFSET lastLSN ;
         if ( NumberLong != ele.type() && NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_LAST_LSN ) ;
         }

         lastLSN = ( DPS_LSN_OFFSET )ele.numberLong() ;
         if ( lastLSN < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid value of field[%s]: %d",
                         RPL_STATUS_LAST_LSN, lastLSN ) ;
         }

         _monitor->setLastLSN(lastLSN);
      }

      ele = data.getField( RPL_STATUS_NEXT_FILEID ) ;
      if ( EOO != ele.type() )
      {
         UINT32 nextFileId ;
         if ( NumberLong != ele.type() && NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_NEXT_FILEID ) ;
         }

         nextFileId = ( UINT32 )ele.numberLong() ;
         _monitor->setNextFileId( nextFileId ) ;
      }

      ele = data.getField( RPL_STATUS_LAST_FILEID ) ;
      if ( EOO != ele.type() )
      {
         UINT32 lastFileId ;
         if ( NumberLong != ele.type() && NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_LAST_FILEID ) ;
         }

         lastFileId = ( UINT32 )ele.numberLong() ;
         _monitor->setLastFileId( lastFileId ) ;
      }

      ele = data.getField( RPL_STATUS_LAST_MOVED_FILETIME ) ;
      if ( EOO != ele.type() )
      {
         time_t lastMovedFileTime ;
         if ( NumberLong != ele.type() && NumberInt != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_LAST_MOVED_FILETIME ) ;
         }

         lastMovedFileTime = ( time_t )ele.numberLong() ;
         _monitor->setLastMovedFileTime( lastMovedFileTime ) ;
      }

      ele = data.getField( RPL_STATUS_OUTPUT_TYPE ) ;
      if ( EOO != ele.type() )
      {
         _monitor->setOutputType( ele.str() ) ;
      }

      ele = data.getField( RPL_STATUS_SERIAL ) ;
      if ( EOO != ele.type() )
      {
         _monitor->setSerial( ele.numberLong() ) ;
      }

      ele = data.getField( RPL_STATUS_EXTRA ) ;
      if ( EOO != ele.type() )
      {
         if ( Object == ele.type() )
         {
            _monitor->setExtraInfo( ele.Obj() ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_RC_CHECK( rc, PDERROR, "Invalid json field[%s]",
                         RPL_STATUS_EXTRA ) ;
         }
      }

      ele = data.getField( RPL_STATUS_SUMMIT_TIME ) ;
      if ( EOO != ele.type() )
      {
         _monitor->setSubmitTime( ele.numberLong() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // do not save monitor
   INT32 rplMonitorStore::init()
   {
      _isNeedWriteStatusFile = FALSE ;
      return SDB_OK ;
   }

   // monitor stores in file
   INT32 rplMonitorStore::init( const CHAR *storeFile )
   {
      INT32 rc = SDB_OK ;
      UINT32 mode;
      BOOLEAN exist = FALSE;
      ossFile statusFile ;

      _isNeedWriteStatusFile = TRUE ;

      if ( NULL == storeFile )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "File name is NULL, rc=%d", rc ) ;
      }

      rc = ossFile::exists( storeFile, exist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check if file[%s] exists, rc=%d",
                   storeFile, rc ) ;

      if ( exist )
      {
         mode = OSS_READWRITE ;
      }
      else
      {
         mode = OSS_CREATEONLY | OSS_READWRITE ;
      }

      rc = statusFile.open( storeFile, mode, OSS_DEFAULTFILE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open status file[%s], rc=%d",
                   storeFile, rc ) ;

      rc = _initMonitor( statusFile ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to read status file[%s], rc=%d",
                   storeFile, rc ) ;

      _statusFileName = storeFile ;
      _tmpStatusFileName = _statusFileName + ".tmp" ;

   done:
      if ( statusFile.isOpened() )
      {
         statusFile.close() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 rplMonitorStore::_saveMonitor()
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj data ;
      ossFile tmpStatusFile ;
      UINT32 mode = OSS_CREATE | OSS_READWRITE ;

      rc = tmpStatusFile.open( _tmpStatusFileName, mode, OSS_DEFAULTFILE) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file(%s), rc = %d",
                   _tmpStatusFileName.c_str(), rc ) ;
      try
      {
         builder.append( RPL_STATUS_OUTPUT_TYPE, _monitor->getOutputType() ) ;
         builder.append( RPL_STATUS_SERIAL, (INT64)_monitor->getSerial() ) ;
         builder.append( RPL_STATUS_NEXT_LSN, (INT64)_monitor->getNextLSN() ) ;
         builder.append( RPL_STATUS_NEXT_FILEID, _monitor->getNextFileId() ) ;
         builder.append( RPL_STATUS_LAST_LSN, (INT64)_monitor->getLastLSN() ) ;
         builder.append( RPL_STATUS_LAST_FILEID, _monitor->getLastFileId() ) ;
         builder.append( RPL_STATUS_LAST_MOVED_FILETIME,
                         (INT64)_monitor->getLastMovedFileTime() ) ;
         builder.append( RPL_STATUS_SUMMIT_TIME,
                         (INT64)_monitor->getSubmitTime() ) ;

         if ( NULL != _outputter )
         {
            BSONObj extra ;
            rc = _outputter->getExtraStatus( extra ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get extra status, rc = %d",
                         rc ) ;
            if ( !extra.isEmpty() )
            {
               builder.append( RPL_STATUS_EXTRA, extra ) ;
            }

            _monitor->setExtraInfo( extra ) ;
         }

         data = builder.obj() ;
      }
      catch ( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                      e.what() ) ;
      }

      rc = engine::utilJsonFile::write( tmpStatusFile, data ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write json to status file[%s], "
                   "rc=%d", _tmpStatusFileName.c_str(), rc ) ;

      rc = tmpStatusFile.close() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to close status file[%s], "
                   "rc=%d", _tmpStatusFileName.c_str(), rc ) ;

      rc = ossRenamePath( _tmpStatusFileName.c_str(),
                          _statusFileName.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rename file, from %s to %s, "
                   "rc=%d", _tmpStatusFileName.c_str(),
                   _statusFileName.c_str(), rc ) ;

   done:
      if ( tmpStatusFile.isOpened() )
      {
         tmpStatusFile.close() ;
      }

      return rc ;
   error:
      goto done ;
   }

   INT32 rplMonitorStore::submitAndSave()
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _outputter )
      {
         rc = _outputter->submit() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to summit outputter, rc=%d", rc ) ;
      }

      rc = save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save status file, rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplMonitorStore::save()
   {
      INT32 rc = SDB_OK ;

      if ( _isNeedWriteStatusFile )
      {
         rc = _saveMonitor() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save status file[%s], rc=%d",
                      _statusFileName.c_str(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // before save monitor, we must make sure outputter is synchronized saved.
   INT32 rplMonitorStore::flushAndSave()
   {
      INT32 rc = SDB_OK ;
      if ( NULL != _outputter )
      {
         rc = _outputter->flush() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sync outputter, rc=%d", rc ) ;
      }

      rc = save() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save status file, rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}


