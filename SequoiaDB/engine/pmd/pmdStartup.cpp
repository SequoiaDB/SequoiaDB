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

   Source File Name = pmdStartup.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdStartup.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   #define PMD_STARTUP_START_CHAR         "STARTUP"
   #define PMD_STARTUP_STOP_CHAR          "STOPOFF"
   #define PMD_STARTUP_RESTART_CHAR       "RESTART"
   #define PMD_STARTUP_START_CHAR_LEN     ossStrlen(PMD_STARTUP_START_CHAR)
   #define PMD_STARTUP_STOP_CHAR_LEN      ossStrlen(PMD_STARTUP_STOP_CHAR)
   #define PMD_STARTUP_RESTART_CHAR_LEN   ossStrlen(PMD_STARTUP_RESTART_CHAR)

   #define PMD_STARTUP_NORMAL_STR       "normal"
   #define PMD_STARTUP_CRASH_STR        "crash"
   #define PMD_STARTUP_FAULT_STR        "fault"

   #define PMD_STARTUP_STR_LEN            ( 32 )

   const CHAR *g_startupChars[] = {
   /*SDB_START_NORMAL*/  "STOPOFF:DO",  "STOPOFF:OK",
   /*SDB_START_CRASH*/   "STARTUP:DO",  "STARTUP:OK",
   /*SDB_START_ERROR*/   "RESTART:DO",  "RESTART:OK"
   } ;

   static const CHAR* _getStartupStr( SDB_START_TYPE type, BOOLEAN ok )
   {
      UINT32 i = 0 ;
      UINT32 j = ok ? 1 : 0 ;
      if ( SDB_START_CRASH == type )
      {
         i = 1 ;
      }
      else if ( SDB_START_ERROR == type )
      {
         i = 2 ;
      }
      return g_startupChars[ i * 2 + j ] ;
   }

   static void _parseStartupStr( const CHAR *str, SDB_START_TYPE &type,
                                 BOOLEAN &ok )
   {
      BOOLEAN find = FALSE ;
      UINT32 pos = 0 ;
      for ( ; pos < sizeof( g_startupChars ) / sizeof( CHAR* ) ; ++pos )
      {
         if ( 0 == ossStrncmp( g_startupChars[ pos ], str,
                               ossStrlen( g_startupChars[ pos ] ) ) )
         {
            find = TRUE ;
            break ;
         }
      }

      if ( !find )
      {
         type = SDB_START_CRASH ;
         ok   = FALSE ;
      }
      else
      {
         UINT32 i = 0 , j = 0 ;
         i = pos / 2 ;
         j = pos % 2 ;
         ok = j > 0 ? TRUE : FALSE ;
         type = SDB_START_CRASH ;
         if ( 0 == i )
         {
            type = SDB_START_NORMAL ;
         }
         else if ( 2 == i )
         {
            type = SDB_START_ERROR ;
         }
      }
   }

   const CHAR* pmdGetStartTypeStr( SDB_START_TYPE type )
   {
      switch( type )
      {
         case SDB_START_NORMAL :
            return PMD_STARTUP_NORMAL_STR ;
         case SDB_START_ERROR :
            return PMD_STARTUP_FAULT_STR ;
         default :
            break ;
      }
      return PMD_STARTUP_CRASH_STR ;
   }

   SDB_START_TYPE pmdStr2StartType( const CHAR* str )
   {
      if ( 0 == ossStrcmp( str, PMD_STARTUP_NORMAL_STR ) )
      {
         return SDB_START_NORMAL ;
      }
      else if ( 0 == ossStrcmp( str, PMD_STARTUP_FAULT_STR ) )
      {
         return SDB_START_ERROR ;
      }
      else
      {
         return SDB_START_CRASH ;
      }
   }

   _pmdStartup::_pmdStartup () :
   _ok(FALSE),
   _startType(SDB_START_NORMAL),
   _fileOpened ( FALSE ),
   _fileLocked ( FALSE ),
   _restart( FALSE )
   {
   }

   _pmdStartup::~_pmdStartup ()
   {
   }

   void _pmdStartup::ok ( BOOLEAN bOK )
   {
      _ok = bOK ;
   }

   BOOLEAN _pmdStartup::isOK () const
   {
      return _ok ;
   }

   BOOLEAN _pmdStartup::needRestart() const
   {
      return SDB_START_NORMAL != _startType ? TRUE : FALSE ;
   }

   void _pmdStartup::restart( BOOLEAN bRestart, INT32 rc )
   {
      _restart = bRestart ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTUP_INIT, "_pmdStartup::init" )
   INT32 _pmdStartup::init ( const CHAR *pPath, BOOLEAN onlyCheck )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDSTARTUP_INIT );
      _fileOpened = FALSE ;
      SINT64 readden = 0 ;
      UINT32 mode = OSS_READWRITE ;
      const UINT32 maxTryLockTime = 2 ;
      UINT32 lockTime = 0 ;

      _fileName = pPath ;
      if ( !_fileName.empty() &&
           OSS_FILE_SEP_CHAR != _fileName.at( _fileName.length() - 1 ) )
      {
         _fileName += OSS_FILE_SEP ;
      }
      _fileName += PMD_STARTUP_FILE_NAME ;

      PD_TRACE1 ( SDB__PMDSTARTUP_INIT, PD_PACK_STRING ( _fileName.c_str() ) ) ;
      rc = ossAccess ( _fileName.c_str() ) ;
      if ( SDB_FNE == rc )
      {
         _startType = SDB_START_NORMAL ;
         _ok = TRUE ;
         mode |= OSS_REPLACE ;
         rc = SDB_OK ;

         if ( onlyCheck )
         {
            goto done ;
         }
      }
      else if ( SDB_PERM == rc )
      {
         PD_LOG ( PDSEVERE, "Permission denied when creating startup file" ) ;
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG ( PDSEVERE, "Failed to access startup file, rc = %d", rc ) ;
         goto error ;
      }
      else
      {
         _ok = FALSE ;
      }

      rc = ossOpen ( _fileName.c_str(), mode, OSS_RU|OSS_WU|OSS_RG, _file ) ;
      if ( SDB_OK != rc )
      {
#if defined (_WINDOWS)
         if ( SDB_PERM == rc )
         {
            PD_LOG ( PDERROR, "Failed to open startup file due to perm "
                     "error, please check if the directory is granted "
                     "with the right permission, or if there is another "
                     "instance is running with the directory" ) ;
         }
         else
#endif
         {
            PD_LOG ( PDERROR, "Failed to create startup file, rc = %d", rc ) ;
         }
         goto error ;
      }
      _fileOpened = TRUE ;

   retry:
      rc = ossLockFile ( &_file, OSS_LOCK_EX ) ;
      if ( SDB_PERM == rc )
      {
         if ( onlyCheck )
         {
            rc = SDB_OK ;
            _startType = SDB_START_NORMAL ;
            goto done ;
         }

         if ( lockTime++ < maxTryLockTime )
         {
            goto retry ;
         }
         PD_LOG ( PDERROR, "The startup file is already locked, most likely "
                  "there is another instance running in the directory" ) ;
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to lock startup file, rc = %d", rc ) ;
         goto error ;
      }
      _fileLocked = TRUE ;

      if ( !_ok )
      {
         CHAR text[ PMD_STARTUP_STR_LEN + 1 ] = { 0 } ;
         rc = ossSeekAndReadN( &_file, 0, PMD_STARTUP_STR_LEN,
                               text, readden ) ;
         _parseStartupStr( text, _startType, _ok ) ;

         PD_TRACE2 ( SDB__PMDSTARTUP_INIT, PD_PACK_INT ( _startType ),
                     PD_PACK_UINT( _ok ) ) ;
         rc = SDB_OK ;

         if ( onlyCheck )
         {
            goto done ;
         }
      }

      rc = _writeStartStr( SDB_START_CRASH, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      if ( onlyCheck && _fileOpened )
      {
         if ( _fileLocked )
         {
            ossLockFile ( &_file, OSS_LOCK_UN ) ;
            _fileLocked = FALSE ;
         }
         ossClose( _file ) ;
         _fileOpened = FALSE ;
      }
      PD_TRACE_EXITRC ( SDB__PMDSTARTUP_INIT, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdStartup::_writeStartStr( SDB_START_TYPE startType, BOOLEAN ok )
   {
      INT32 rc = SDB_OK ;
      CHAR szText [ PMD_STARTUP_STR_LEN + 1 ] = { 0 } ;
      INT64 written = 0 ;
      ossStrncpy( szText, _getStartupStr( startType, ok ),
                  PMD_STARTUP_STR_LEN ) ;
      if ( !_fileOpened )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeekAndWriteN( &_file, 0, szText, PMD_STARTUP_STR_LEN,
                             written ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Write %s to startup file failed, rc: %d",
                 szText, rc ) ;
         goto error ;
      }
      ossFsync( &_file ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSTARTUP_FINAL, "_pmdStartup::final" )
   INT32 _pmdStartup::final ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSTARTUP_FINAL ) ;
      if ( _fileOpened && _fileLocked )
      {
         SDB_START_TYPE tmpType = SDB_START_NORMAL ;
         if ( _restart )
         {
            tmpType = SDB_START_ERROR ;
         }
         _writeStartStr( tmpType, _ok ) ;
      }

      if ( _fileLocked )
      {
         ossLockFile ( &_file, OSS_LOCK_UN ) ;
      }
      if ( _fileOpened )
      {
         ossClose( _file ) ;
         _fileOpened = FALSE ;
      }
      if ( _ok && !_restart )
      {
         ossDelete ( _fileName.c_str() ) ;
      }
      PD_TRACE_EXIT ( SDB__PMDSTARTUP_FINAL ) ;
      return SDB_OK ;
   }

   pmdStartup & pmdGetStartup ()
   {
      static pmdStartup _startUp ;
      return _startUp ;
   }

}


