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

   Source File Name = utilRenameLogger.cpp

   Descriptive Name = util rename logger

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== ======== ==============================================
          12/17/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilRenameLogger.hpp"
#include "utilTrace.hpp"
#include "utilStr.hpp"
#include "ossPath.hpp"
#include "pmd.hpp"

using namespace std ;

namespace engine
{
   BOOLEAN utilStr2RenameLog( const string& str, utilRenameLog& log )
   {
      BOOLEAN isOk = TRUE ;
      INT32 fieldSize = 0 ;
      const CHAR* pLine = NULL ;
      const CHAR *pSep = NULL ;

      vector<string> lines = utilStrSplit( str, OSS_NEWLINE ) ;
      if( UTIL_RENAME_LOG_FIELD_NUM != lines.size() )
      {
         goto error ;
      }

      // first line
      pLine = lines.at( 0 ).c_str() ;
      pSep = ossStrchr( pLine, UTIL_RENAME_LOG_SEP ) ;
      if ( NULL == pSep )
      {
         goto error ;
      }
      fieldSize = pSep - pLine ;
      if (    ( sizeof( UTIL_RENAME_LOG_OLDNAME ) - 1 ) == fieldSize
           && 0 == ossStrncmp( pLine, UTIL_RENAME_LOG_OLDNAME, fieldSize ) )
      {
         // it is ok
      }
      else
      {
         goto error ;
      }
      ossStrncpy( log.oldName, pSep + 1, DMS_COLLECTION_SPACE_NAME_SZ ) ;

      // second line
      pLine = lines.at( 1 ).c_str() ;
      pSep = ossStrchr( pLine, UTIL_RENAME_LOG_SEP ) ;
      if ( NULL == pSep )
      {
         goto error ;
      }
      fieldSize = pSep - pLine ;
      if (    ( sizeof( UTIL_RENAME_LOG_NEWNAME ) - 1 ) == fieldSize
           && 0 == ossStrncmp( pLine, UTIL_RENAME_LOG_NEWNAME, fieldSize ) )
      {
         // it is ok
      }
      else
      {
         goto error ;
      }
      ossStrncpy( log.newName, pSep + 1, DMS_COLLECTION_SPACE_NAME_SZ ) ;

      isOk = TRUE ;

   done :
      return isOk ;
   error :
      isOk = FALSE ;
      goto done ;
   }

   _utilRenameLogger::_utilRenameLogger()
   : _isOpened( FALSE ), _fileExist( FALSE )
   {
      ossMemset( _fileName, 0, OSS_MAX_PATHSIZE + 1 ) ;
   }

   _utilRenameLogger::~_utilRenameLogger()
   {
      if ( _isOpened )
      {
         ossClose( _file ) ;
         _isOpened = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGGER_INIT, "_utilRenameLogger::init" )
   INT32 _utilRenameLogger::init( UTIL_RENAME_LOGGER_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGGER_INIT ) ;

      CHAR fileName[ 64 ] = { 0 } ;

      static ossAtomic64 s_suffixID( 0 ) ;

      ossSnprintf( fileName, sizeof( fileName ) - 1, "%s_%llu",
                   UTIL_RENAME_LOG_FILENAME, s_suffixID.inc() ) ;

      rc = init( fileName, mode ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize rename logger [%s], "
                   "rc: %d", fileName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGGER_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGGER_INIT_NAME, "_utilRenameLogger::init" )
   INT32 _utilRenameLogger::init( const CHAR *fileName,
                                  UTIL_RENAME_LOGGER_MODE mode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_UTILRENAMELOGGER_INIT_NAME ) ;

      INT32 fileMode = OSS_READWRITE ;
      INT32 permission = OSS_RU | OSS_WU | OSS_RG | OSS_RO ;

      rc = utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                              fileName,
                              OSS_MAX_PATHSIZE,
                              _fileName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build full file path , rc: %d", rc  ) ;

      rc = ossFile::exists( _fileName, _fileExist ) ;
      PD_RC_CHECK( rc, PDERROR,
                    "Failed to check existence of file[%s], rc: %d",
                    _fileName, rc ) ;

      switch ( mode )
      {
         case UTIL_RENAME_LOGGER_WRITE :
         {
            // delete file if exists
            if ( _fileExist )
            {
               PD_LOG( PDWARNING, "File[%s] already exists!", _fileName ) ;
               rc = ossDelete( _fileName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to delete file[%s], rc: %d",
                            _fileName, rc ) ;
               _fileExist = FALSE ;
            }
            // create
            fileMode = OSS_READWRITE | OSS_CREATE ;
            rc = ossOpen( _fileName, fileMode, permission, _file ) ;
            PD_RC_CHECK ( rc, PDERROR,
                          "Failed to open file[%s], rc: %d", _fileName, rc ) ;
            _fileExist = TRUE ;
            _isOpened = TRUE ;
            break ;
         }

         case UTIL_RENAME_LOGGER_READ :
         {
            if ( _fileExist )
            {
               fileMode = OSS_READWRITE ;
               rc = ossOpen( _fileName, fileMode, permission, _file ) ;
               PD_RC_CHECK ( rc, PDERROR, "Failed to open file[%s], rc: %d",
                             _fileName, rc ) ;
               _isOpened = TRUE ;
            }
            else
            {
               rc = SDB_FNE ;
            }
            break ;
         }

         default :
               rc = SDB_INVALIDARG ;
               SDB_ASSERT( FALSE, "Invalid rename logger mode!" ) ;
               break ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGGER_INIT_NAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGGER_LOG, "_utilRenameLogger::log" )
   INT32 _utilRenameLogger::log( const utilRenameLog& log )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_UTILRENAMELOGGER_LOG ) ;

      SINT64 written = 0 ;
      CHAR strLog[ UTIL_RENAME_LOG_FILESIZE_MAX ] = { 0 } ;

      SDB_ASSERT( _fileExist, "File doesn't exist" ) ;
      PD_CHECK( _fileExist, SDB_FNE, error, PDERROR,
                "File[%s] doesn't exist, rc: %d", _fileName, rc ) ;

      log.toString( strLog, UTIL_RENAME_LOG_FILESIZE_MAX ) ;
      rc = ossSeekAndWriteN( &_file, 0, strLog, ossStrlen( strLog ),
                             written ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to write start info into file[%s], rc: %d",
                    _fileName, rc ) ;

      ossFsync( &_file ) ;

   done :
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGGER_LOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGGER_LOAD, "_utilRenameLogger::load" )
   INT32 _utilRenameLogger::load( utilRenameLog& log )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_UTILRENAMELOGGER_LOAD ) ;

      INT64 fileSize = 0 ;
      SINT64 readSize = 0 ;
      CHAR* readBuf = NULL ;

      SDB_ASSERT( _fileExist, "File doesn't exist" ) ;
      PD_CHECK ( _fileExist, SDB_FNE, error, PDERROR,
                 "File[%s] doesn't exist, rc: %d", _fileName, rc ) ;

      // if the file too large, it is abnormal
      rc = ossGetFileSize( &_file, &fileSize ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to get size of file[%s], rc: %d", _fileName, rc ) ;

      if ( fileSize > UTIL_RENAME_LOG_FILESIZE_MAX )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK ( rc, PDERROR,
                       "File size is too large[%s], rc: %d", _fileName, rc ) ;
         goto error ;
      }

      // read all string
      readBuf = ( CHAR* )SDB_OSS_MALLOC( fileSize + 1 ) ;
      if ( !readBuf )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset( readBuf, 0, fileSize + 1 ) ;

      rc = ossSeekAndReadN( &_file, 0, fileSize, readBuf, readSize ) ;
      PD_RC_CHECK ( rc, PDERROR,
                    "Failed to read file[%s], rc: %d", _fileName, rc ) ;

      // convert
      if ( !utilStr2RenameLog( readBuf, log ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDWARNING,
                 "Failed to convert string[%s] to rename info, rc: %d",
                 readBuf, rc ) ;
         goto error ;
      }

   done :
      if ( readBuf )
      {
         SDB_OSS_FREE( readBuf ) ;
         readBuf = NULL ;
      }
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGGER_LOAD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _utilRenameLogger::clear()
   {
      INT32 rc = SDB_OK ;

      if ( _isOpened )
      {
         ossClose( _file ) ;
         _isOpened = FALSE ;
      }
      rc = ossDelete( _fileName ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to delete file[%s], rc: %d", _fileName, rc ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   const CHAR* _utilRenameLogger::fileName()
   {
      return _fileName ;
   }

   BOOLEAN _utilRenameLogger::isOpened() const
   {
      return _isOpened ;
   }

   /*
      _utilRenameLogManager implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGMGR_LOAD, "_utilRenameLogManager::load" )
   INT32 _utilRenameLogManager::load()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGMGR_LOAD ) ;

      try
      {
         multimap< string, string > mapFiles ;

         const CHAR *dbPath = pmdGetOptionCB()->getDbPath() ;

         // use wildcard to match all rename log files including old version
         // file
         rc = ossEnumFiles( dbPath, mapFiles, UTIL_RENAME_LOG_FILENAME "*" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to enum rename log files in "
                      "dbpath [%s], rc: %d", dbPath, rc ) ;

         for ( multimap< string, string >::iterator iter = mapFiles.begin() ;
               iter != mapFiles.end() ;
               ++ iter )
         {
            BOOLEAN canIgnore = FALSE ;

            utilRenameLogger logger ;
            utilRenameLog log ;

            _UTIL_STRING_MAP_IT oldToNewIter, newToOldIter ;

            // NOTE: first is file name, second is full name
            const CHAR *fileName = iter->first.c_str() ;

            rc = logger.init( fileName, UTIL_RENAME_LOGGER_READ ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize rename log file "
                         "[%s], rc: %d", fileName, rc ) ;

            rc = logger.load( log ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to load rename log from file "
                         "[%s], rc: %d", fileName, rc ) ;

            // check if already have conflict or duplicated logs
            oldToNewIter = _find( log.oldName, _oldToNewMap ) ;
            newToOldIter = _find( log.newName, _newToOldMap ) ;

            if ( ( oldToNewIter != _oldToNewMap.end() ) ||
                 ( newToOldIter != _newToOldMap.end() ) )
            {
               if ( oldToNewIter != _oldToNewMap.end() )
               {
                  PD_CHECK( 0 == ossStrcmp( oldToNewIter->second.c_str(),
                                            log.newName ),
                            SDB_SYS, error, PDERROR,
                            "Failed to load rename log file [%s], "
                            "[%s] -> [%s] is conflict to [%s] -> [%s]",
                            fileName, log.oldName, log.newName,
                            oldToNewIter->first.c_str(),
                            oldToNewIter->second.c_str() ) ;
               }
               if ( newToOldIter != _newToOldMap.end() )
               {
                  PD_CHECK( 0 == ossStrcmp( newToOldIter->second.c_str(),
                                            log.oldName ),
                            SDB_SYS, error, PDERROR,
                            "Failed to load rename log file [%s], "
                            "[%s] -> [%s] is conflict to [%s] -> [%s]",
                            fileName, log.oldName, log.newName,
                            newToOldIter->second.c_str(),
                            newToOldIter->second.c_str() ) ;
               }
               PD_LOG( PDDEBUG, "Got duplicated rename log file [%s]: "
                       "[%s] -> [%s]", fileName, log.oldName, log.newName ) ;
               canIgnore = TRUE ;
            }

            if ( canIgnore )
            {
               // can be ignored, clear file
               logger.clear() ;
            }
            else
            {
               // save to mapping
               _oldToFileMap.insert( make_pair( log.oldName, fileName ) ) ;
               _oldToNewMap.insert( make_pair( log.oldName, log.newName ) ) ;
               _newToOldMap.insert( make_pair( log.newName, log.oldName ) ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check rename collection spaces, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGMGR_LOAD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGMGR__CLEAR, "_utilRenameLogManager::_clear" )
   INT32 _utilRenameLogManager::_clear( const CHAR *fileName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGMGR__CLEAR ) ;

      SDB_ASSERT( NULL != fileName, "file name is invalid" ) ;

      CHAR filePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      rc = utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                              fileName,
                              OSS_MAX_PATHSIZE,
                              filePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build path for file [%s], rc: %d",
                   fileName, rc ) ;

      rc = ossDelete( filePath ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to delete file [%s], rc: %d",
                   fileName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGMGR__CLEAR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   _utilRenameLogManager::_UTIL_STRING_MAP_IT
   _utilRenameLogManager::_find( const CHAR *name,
                                 _UTIL_STRING_MAP &targetMap )
   {
      SDB_ASSERT( NULL != name, "name is invalid" ) ;

      _UTIL_STRING_MAP_IT iter ;

      try
      {
         iter = targetMap.find( name ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to get iterators, occur exception %s",
                 e.what() ) ;

         for ( _UTIL_STRING_MAP_IT tempIter = targetMap.begin() ;
               tempIter != targetMap.end() ;
               ++ tempIter )
         {
            if ( 0 == ossStrncmp( name,
                                  tempIter->first.c_str(),
                                  DMS_COLLECTION_SPACE_NAME_SZ ) )
            {
               iter = tempIter ;
               break ;
            }
         }
      }

      return iter ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGMGR_CLEARALL, "_utilRenameLogManager::clearAll" )
   INT32 _utilRenameLogManager::clearAll()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGMGR_CLEARALL ) ;

      for ( _UTIL_STRING_MAP::iterator iter = _oldToFileMap.begin() ;
            iter != _oldToFileMap.end() ;
            ++ iter )
      {
         const CHAR *fileName = iter->second.c_str() ;

         // remove file
         rc = _clear( fileName ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to clear rename log file [%s], "
                      "rc: %d", fileName, rc ) ;
      }

      // clear mapping
      _oldToFileMap.clear() ;
      _oldToNewMap.clear() ;
      _newToOldMap.clear() ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGMGR_CLEARALL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGMGR_CLEAR, "_utilRenameLogManager::clear" )
   INT32 _utilRenameLogManager::clear( const utilRenameLog &log )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGMGR_CLEAR ) ;

      _UTIL_STRING_MAP_IT oldToFileIter = _find( log.oldName, _oldToFileMap ) ;
      _UTIL_STRING_MAP_IT oldToNewIter = _find( log.oldName, _oldToNewMap ) ;
      _UTIL_STRING_MAP_IT newToOldIter = _find( log.newName, _newToOldMap ) ;

      if ( oldToFileIter != _oldToFileMap.end() )
      {
         const CHAR *fileName = oldToFileIter->second.c_str() ;

         // remove file
         rc = _clear( fileName ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to clear rename log file [%s], "
                      "rc: %d", fileName, rc ) ;

         _oldToFileMap.erase( oldToFileIter ) ;
      }

      if ( oldToNewIter != _oldToNewMap.end() )
      {
         _oldToNewMap.erase( oldToNewIter ) ;
      }

      if ( newToOldIter != _newToOldMap.end() )
      {
         _newToOldMap.erase( newToOldIter ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGMGR_CLEAR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   BOOLEAN _utilRenameLogManager::hasRenamed()
   {
      return _oldToFileMap.size() > 0 ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRENAMELOGMGR_GETRENAMELOG, "_utilRenameLogManager::getRenameLog" )
   INT32 _utilRenameLogManager::getRenameLog( const CHAR *csName,
                                              utilRenameLog &log )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRENAMELOGMGR_GETRENAMELOG ) ;

      _UTIL_STRING_MAP_IT oldToNewIter = _find( csName, _oldToNewMap ) ;
      _UTIL_STRING_MAP_IT newToOldIter = _find( csName, _newToOldMap ) ;

      if ( ( oldToNewIter != _oldToNewMap.end() ) &&
           ( newToOldIter != _newToOldMap.end() ) )
      {
         // name in both mappings, should be conflict
         PD_LOG( PDERROR, "Failed to get rename log for [%s], has conflicts: "
                 "[%s] -> [%s] and [%s] -> [%s]",
                 oldToNewIter->first.c_str(), oldToNewIter->second.c_str(),
                 newToOldIter->second.c_str(), newToOldIter->first.c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( oldToNewIter != _oldToNewMap.end() )
      {
         // found in old -> new mapping, the given name is old name
         ossStrncpy( log.oldName,
                     oldToNewIter->first.c_str(),
                     DMS_COLLECTION_SPACE_NAME_SZ ) ;
         log.oldName[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
         ossStrncpy( log.newName,
                     oldToNewIter->second.c_str(),
                     DMS_COLLECTION_SPACE_NAME_SZ ) ;
         log.newName[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
      }
      else if ( newToOldIter != _newToOldMap.end() )
      {
         // found in new -> old mapping, the given name is new name
         ossStrncpy( log.oldName,
                     newToOldIter->second.c_str(),
                     DMS_COLLECTION_SPACE_NAME_SZ ) ;
         log.oldName[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
         ossStrncpy( log.newName,
                     newToOldIter->first.c_str(),
                     DMS_COLLECTION_SPACE_NAME_SZ ) ;
         log.newName[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0 ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGMGR_GETRENAMELOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
