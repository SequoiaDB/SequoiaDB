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

      INT32 fileMode = OSS_READWRITE ;
      INT32 permission = OSS_RU | OSS_WU | OSS_RG | OSS_RO ;

      rc = utilBuildFullPath( pmdGetOptionCB()->getDbPath(),
                              UTIL_RENAME_LOG_FILENAME,
                              OSS_MAX_PATHSIZE,
                              _fileName ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to build full file path , rc: %d", rc  );

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
      PD_TRACE_EXITRC( SDB_UTILRENAMELOGGER_INIT, rc ) ;
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
}


