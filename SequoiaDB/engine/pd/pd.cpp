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

   Source File Name = pd.cpp

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
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#if defined (_LINUX) || defined (_AIX)
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "pd.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "utilStr.hpp"
#include "ossIO.hpp"

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
#include "ossPath.hpp"
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

#include "pdTrace.hpp"

PDLEVEL& getPDLevel()
{
   static PDLEVEL s_pdLevel = PDWARNING ;
   return s_pdLevel ;
}

PDLEVEL setPDLevel( PDLEVEL newLevel )
{
   PDLEVEL oldLevel = getPDLevel() ;
   getPDLevel() = newLevel ;
   return oldLevel ;
}

const CHAR* getPDLevelDesp ( PDLEVEL level )
{
   const static CHAR *s_PDLEVELSTRING[] =
   {
      "SEVERE",
      "ERROR",
      "EVENT",
      "WARNING",
      "INFO",
      "DEBUG"
   } ;
   if ( level >= 0 && level < (INT32)(sizeof(s_PDLEVELSTRING)/sizeof(CHAR*)) )
   {
      return s_PDLEVELSTRING[(UINT32)level] ;
   }
   return "UNKNOW" ;
}

/* private variables */
struct _pdLogFile : public SDBObject
{
   // ossPrimitiveFileOp is native file interface, which returns errno or
   // GetLastError, instead of database error code
   ossPrimitiveFileOp _logFile ;
   UINT64             _fileSize ;
   ossSpinXLatch _mutex ;
} ;
typedef struct _pdLogFile pdLogFile ;

// currently we only have diag log, we may have different types of logs later
enum _pdLogType
{
   PD_DIAGLOG  = 0,
   PD_AUDIT    = 1,
   PD_LOG_MAX
} ;
pdLogFile _pdLogFiles [ PD_LOG_MAX ] ;

/*
   _pdCfgInfo define
*/
typedef struct _pdCfgInfo
{
   CHAR        _pdLogFile[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR        _pdLogPath[ OSS_MAX_PATHSIZE + 1 ] ;
   INT32       _pdFileMaxNum ;
   UINT64      _pdFileMaxSize ;

   _pdCfgInfo()
   {
      _pdLogFile[0]  = 0 ;
      _pdLogPath[0]  = 0 ;
      _pdFileMaxNum  = 0 ;
      _pdFileMaxSize = 0 ;
   }

   BOOLEAN isEnabled() const
   {
      if ( _pdLogFile[0] != 0 && ( _pdFileMaxNum > 0 || _pdFileMaxNum == -1 ) &&
           _pdFileMaxSize > 0 )
      {
         return TRUE ;
      }
      return FALSE ;
   }
} pdCfgInfo ;

/// define the pd config
pdCfgInfo g_pdCfg[ PD_LOG_MAX ] ;

static pdCfgInfo& _getPDCfgInfo( _pdLogType type )
{
   if ( type >= PD_DIAGLOG && type < PD_LOG_MAX )
      return g_pdCfg[ type ] ;
   return g_pdCfg[ PD_DIAGLOG ] ;
}

const CHAR* getDialogName ()
{
   return _getPDCfgInfo( PD_DIAGLOG )._pdLogFile ;
}

const CHAR* getDialogPath ()
{
   return _getPDCfgInfo( PD_DIAGLOG )._pdLogPath ;
}

void setDiagFileNum( INT32 fileMaxNum )
{
   pdCfgInfo &info = _getPDCfgInfo( PD_DIAGLOG ) ;
   info._pdFileMaxNum = fileMaxNum ;
}

void setAuditFileNum( INT32 fileMaxNum )
{
   pdCfgInfo &info = _getPDCfgInfo( PD_AUDIT ) ;
   info._pdFileMaxNum = fileMaxNum ;
}

void sdbEnablePD( const CHAR *pdPathOrFile, INT32 fileMaxNum,
                  UINT32 fileMaxSize )
{
   sdbDisablePD() ;

   pdCfgInfo &info = _getPDCfgInfo( PD_DIAGLOG ) ;
   const CHAR *shortName = PD_DFT_DIAGLOG ;

   if ( pdPathOrFile && 0 != pdPathOrFile[0] )
   {
      const CHAR *pDotStr = ossStrrchr( pdPathOrFile, '.' ) ;
      const CHAR *pSepStr1 = ossStrrchr( pdPathOrFile, '/' ) ;
      const CHAR *pSepStr2 = ossStrrchr( pdPathOrFile, '\\' ) ;
      const CHAR *pSepStr = pSepStr1 >= pSepStr2 ? pSepStr1 : pSepStr2 ;

      // is file
      if ( pDotStr && pDotStr > pSepStr )
      {
         shortName = pSepStr ? pSepStr + 1 : pdPathOrFile ;
         ossStrncpy( info._pdLogPath, pdPathOrFile, shortName - pdPathOrFile ) ;
      }
      // is path
      else
      {
         ossStrncpy( info._pdLogPath, pdPathOrFile, OSS_MAX_PATHSIZE ) ;
      }
   }

   if ( 0 == info._pdLogPath[0] )
   {
      ossStrcpy( info._pdLogPath, "." ) ;
   }

   if ( info._pdLogPath[0] != 0 )
   {
      engine::utilBuildFullPath( info._pdLogPath, shortName,
                                 OSS_MAX_PATHSIZE, info._pdLogFile ) ;
      info._pdLogFile[ OSS_MAX_PATHSIZE ] = 0 ;
   }
   info._pdFileMaxNum = fileMaxNum ;
   info._pdFileMaxSize = (UINT64)fileMaxSize * 1024 * 1024 ;
}

void sdbDisablePD()
{
   pdLogFile &logFile = _pdLogFiles[ PD_DIAGLOG ] ;
   if ( logFile._logFile.isValid() )
   {
      logFile._logFile.Close() ;
   }
   _getPDCfgInfo( PD_DIAGLOG )._pdLogFile[0] = 0 ;
}

BOOLEAN sdbIsPDEnabled ()
{
   return _getPDCfgInfo( PD_DIAGLOG ).isEnabled() ;
}

/*
 * Log Header format
 * Arguments:
 * 1) Year (UINT32)
 * 2) Month (UINT32)
 * 3) Day (UINT32)
 * 4) Hour (UINT32)
 * 5) Minute (UINT32)
 * 6) Second (UINT32)
 * 7) Microsecond (UINT32)
 * 8) Level (string)
 * 9) Process ID (UINT64)
 * 10) Thread ID (UINT64)
 * 11) File Name (string)
 * 12) Function Name (string)
 * 13) Line number (UINT32)
 * 14) Message
 */
const static CHAR *PD_LOG_HEADER_FORMAT = "%04d-%02d-%02d-%02d.%02d.%02d.%06d\
               \
Level:%s" OSS_NEWLINE "PID:%-37dTID:%d" OSS_NEWLINE "Function:%-32sLine:%d" \
OSS_NEWLINE "File:%s" OSS_NEWLINE "Message:" OSS_NEWLINE "%s" OSS_NEWLINE OSS_NEWLINE;
/* extern variables */

// driver don't use the code
#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )

static void _pdRemoveOutOfDataFiles( pdCfgInfo &info )
{
   multimap< string, string >  mapFiles ;
   multimap< string, string >::iterator iter ;
   const CHAR *p = ossStrrchr( info._pdLogFile, OSS_FILE_SEP_CHAR ) ;
   if ( p )
   {
      p = p + 1 ;
   }
   else
   {
      p = info._pdLogFile ;
   }
   CHAR filter[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
   ossSnprintf( filter, OSS_MAX_PATHSIZE, "%s.*", p ) ;
   ossEnumFiles( info._pdLogPath, mapFiles, filter, 1 ) ;

   iter = mapFiles.begin() ;
   while ( mapFiles.end() != iter )
   {
      if ( ossStrstr( iter->first.c_str(), PD_DFT_LOG_DECRYPT_SUFFIX ) )
      {
         mapFiles.erase( iter++ ) ;
      }
      else
      {
         ++iter ;
      }
   }

   while ( mapFiles.size() > 0 &&
           mapFiles.size() >= (UINT32)info._pdFileMaxNum )
   {
      // remove the oldest file
      ossDelete( mapFiles.begin()->second.c_str() ) ;
      mapFiles.erase( mapFiles.begin() ) ;
   }
}

static INT32 _pdLogArchive( pdCfgInfo &info )
{
   INT32 rc = SDB_OK ;
   CHAR strTime[ 50 ] = {0} ;
   CHAR fileName[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
   time_t tTime = time( NULL ) ;

   ossSnprintf( fileName, OSS_MAX_PATHSIZE, "%s.%s", info._pdLogFile,
                engine::utilAscTime( tTime, strTime, sizeof( strTime ) ) ) ;

   // if exist, remove
   if ( SDB_OK == ossAccess( fileName ) )
   {
      rc = ossDelete( fileName ) ;
      if ( rc )
      {
         ossPrintf( "Delete file %s failed, rc: %d" OSS_NEWLINE,
                    fileName, rc ) ;
      }
   }
   // rename
   rc = ossRenamePath( info._pdLogFile, fileName ) ;
   if ( rc )
   {
      ossPrintf( "Rename %s to %s failed, rc: %d" OSS_NEWLINE,
                 info._pdLogFile, fileName, rc ) ;
      ossDelete( info._pdLogFile ) ;
   }

   // remove out of data files
   if ( info._pdFileMaxNum > 0 )
   {
      _pdRemoveOutOfDataFiles( info ) ;
   }

   return rc ;
}

#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

// PD_TRACE_DECLARE_FUNCTION ( SDB_PDLOGFILEWRITE, "pdLogFileWrite" )
// return code is errno
static INT32 pdLogFileWrite ( _pdLogType type, const CHAR *pData )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_PDLOGFILEWRITE ) ;
   SDB_ASSERT ( type < PD_LOG_MAX, "type is out of range" ) ;
   SINT64 dataSize = ossStrlen ( pData ) ;
   pdLogFile &logFile = _pdLogFiles[type] ;
   pdCfgInfo &info = _getPDCfgInfo( type ) ;

   // lock file first
   logFile._mutex.get() ;

   // if file not exist, need open
   //if ( SDB_OK != ossAccess( info._pdLogFile ) )
   //{
   //   logFile._logFile.Close() ;
   //}

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
open:
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

   /// check file whether exist
   if ( !logFile._logFile.isExist() )
   {
      logFile._logFile.Close() ;
   }

   // attempt to open the file
   if ( !logFile._logFile.isValid() )
   {
      rc = logFile._logFile.Open ( info._pdLogFile ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to open log file, errno = %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }
      else
      {
         ossPrimitiveFileOp::offsetType fileSize ;
         rc = logFile._logFile.getSize( &fileSize ) ;
         if ( rc )
         {
            ossPrintf( "Failed to get log file size, rc: %d" OSS_NEWLINE, rc ) ;
            logFile._fileSize = 0 ;
         }
         else
         {
            logFile._fileSize = (UINT64)fileSize.offset ;
         }
      }
      logFile._logFile.seekToEnd () ;
   }

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
   // if file size up the limit
   if ( logFile._fileSize + dataSize > info._pdFileMaxSize )
   {
      logFile._logFile.Close() ;
      _pdLogArchive( info ) ;
      goto open ;
   }
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

   PD_TRACE1 ( SDB_PDLOGFILEWRITE, PD_PACK_RAW ( pData, dataSize ) ) ;
   rc = logFile._logFile.Write ( pData, dataSize ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to write log file, rc: %d" OSS_NEWLINE,
                  rc ) ;
      goto error ;
   } // if ( rc )
   // update file size
   logFile._fileSize += dataSize ;

done :
   logFile._mutex.release() ;
   PD_TRACE_EXITRC ( SDB_PDLOGFILEWRITE, rc ) ;
   return rc ;
error :
   goto done ;
}

void pdLog( PDLEVEL level, const CHAR* func, const CHAR* file,
            UINT32 line, const std::string &message )
{
   pdLog(level, func, file, line, message.c_str() );
}

/*
 * Problem Detemination Log
 * Input:
 * Log level (PDSEVERE/PDERROR/PDWARNING/PDINFO/PDEVENT/PDDEBUG)
 * function name (char*)
 * file name (char*)
 * line number (integer)
 * output string (char*)
 * <followed by arguments>
 * Output:
 *    N/A
 */
// PD_TRACE_DECLARE_FUNCTION ( SDB_PDLOG, "pdLog" )
void pdLog( PDLEVEL level, const CHAR* func, const CHAR* file,
            UINT32 line, const CHAR* format, ...)
{
   if ( getPDLevel() < level )
      return ;
   va_list ap;
   PD_TRACE_ENTRY ( SDB_PDLOG ) ;
   struct tm otm ;
   struct timeval tv;
   struct timezone tz;
   time_t tt ;
   CHAR userInfo[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;
   CHAR sysInfo[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;  // for log header

   ossSignalShield sigShield ;
   sigShield.doNothing() ;

   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;

#if defined (_WINDOWS)
   localtime_s( &otm, &tt ) ;
#else
   localtime_r( &tt, &otm ) ;
#endif

   // create the user information
   va_start(ap, format);
   vsnprintf(userInfo, PD_LOG_STRINGMAX, format, ap);
   va_end(ap);

   // create log header
   ossSnprintf(sysInfo, PD_LOG_STRINGMAX + 1, PD_LOG_HEADER_FORMAT,
               otm.tm_year+1900,            // 1) Year (UINT32)
               otm.tm_mon+1,                // 2) Month (UINT32)
               otm.tm_mday,                 // 3) Day (UINT32)
               otm.tm_hour,                 // 4) Hour (UINT32)
               otm.tm_min,                  // 5) Minute (UINT32)
               otm.tm_sec,                  // 6) Second (UINT32)
               tv.tv_usec,                  // 7) Microsecond (UINT32)
               getPDLevelDesp(level),       // 8) Level (string)
               ossGetCurrentProcessID(),    // 9) Process ID (UINT64)
               ossGetCurrentThreadID(),     // 10) Thread ID (UINT64)
               func,                        // 11) Function Name (string)
               line,                        // 12) Line number (UINT32)
               file,                        // 13) File Name (string)
               userInfo                     // 14) Message
   ) ;

   // If the userInfo is too long, the expected end of line(2 of them) are
   // truncated in the sysInfo above. Add them if they are not there.
   // If the end of the log( before the terminate character) is not '\0',
   // nor NEWLINE NEWLINE, it should be adjusted.
   if ( sysInfo[ PD_LOG_STRINGMAX - 1 ] != '\0' )
   {
      INT32 position = PD_LOG_STRINGMAX - ossStrlen( OSS_NEWLINE ) * 2 ;
      if ( 0 != ossStrcmp( sysInfo + position, OSS_NEWLINE OSS_NEWLINE ) )
      {
         // The plus 1 is for the terminate character.
         ossSnprintf( sysInfo + position, ossStrlen( OSS_NEWLINE ) * 2 + 1,
                      OSS_NEWLINE OSS_NEWLINE ) ;
      }
   }

   pdLogRaw( level, sysInfo ) ;

   PD_TRACE_EXIT( SDB_PDLOG ) ;
   return ;
}

void pdLogRaw( PDLEVEL level, const CHAR *pData )
{
   INT32 rc = SDB_OK ;

   if ( getPDLevel() < level )
      return ;

   // use thread specific pointer to make sure there's no nested pdLog (i.e.
   // calling pdLog in signal handler when the thread is already in pdLog
   // function will not proceed)
   static OSS_THREAD_LOCAL BOOLEAN amIInPD = FALSE ;
   if ( amIInPD )
   {
      goto done ;
   }
   amIInPD = TRUE ;

   /* We write into log file if the string is not empty */
   if ( _getPDCfgInfo( PD_DIAGLOG ).isEnabled() )
   {
      rc = pdLogFileWrite ( PD_DIAGLOG, pData ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to write into log file, rc: %d" OSS_NEWLINE, rc ) ;
         ossPrintf ( "%s" OSS_NEWLINE, pData ) ;
      }
   }

   // make sure to reset this before leaving
   amIInPD = FALSE ;

done:
   return ;
}

#define PD_MAX_SHIELD_RC_COUNT 64

static volatile BOOLEAN s_diagSecureOn = TRUE ;
static OSS_THREAD_LOCAL BOOLEAN s_localDiagSecureOn = TRUE ;
static OSS_THREAD_LOCAL UINT64 s_shieldLogMask = 0 ;
static OSS_THREAD_LOCAL UINT64 s_hasIncCntMask = 0 ;
static UINT32 s_shieldLogCnt[PD_MAX_SHIELD_RC_COUNT] = { 0 } ;

struct _pdRCMaskItem
{
   INT32    _rc ;
   BOOLEAN  _needStats ;
   UINT64   _mask ;
} ;

static _pdRCMaskItem s_rcMaskMap[] =
{
   { SDB_IXM_DUP_KEY, TRUE, LOG_MASK_IXM_DUP_KEY },
   { SDB_IXM_ADVANCE_EOC, FALSE, LOG_MASK_IXM_ADVANCE_EOC },
   { SDB_RTN_INVALID_HINT, FALSE, LOG_MASK_RTN_INVALID_HINT }
} ;

static UINT64 _pdRC2Mask( INT32 rc )
{
   INT32 mSize = sizeof( s_rcMaskMap ) ;
   for ( INT32 i = 0 ; i < mSize ; i++ )
   {
      _pdRCMaskItem& item = s_rcMaskMap[i] ;
      if ( item._rc == rc )
      {
         return item._mask ;
      }
   }
   return 0 ;
}

static void _pdIncShieldLogCntByRC( INT32 rc )
{
   INT32 mSize = sizeof( s_rcMaskMap ) ;
   for ( INT32 i = 0 ; i < mSize ; i++ )
   {
      _pdRCMaskItem& item = s_rcMaskMap[i] ;
      if ( item._rc == rc )
      {
         if ( ( item._needStats ) &&
              ( !OSS_BIT_TEST( s_hasIncCntMask, item._mask ) ) )
         {
            if ( i < PD_MAX_SHIELD_RC_COUNT )
            {
               s_shieldLogCnt[i]++ ;
            }
            OSS_BIT_SET( s_hasIncCntMask, item._mask ) ;
         }
         break ;
      }
   }
}

void pdEnableDiaglogSecure()
{
   s_diagSecureOn = TRUE ;
}

void pdDisableDiaglogSecure()
{
   s_diagSecureOn = FALSE ;
}

BOOLEAN pdIsDiaglogSecureEnabled()
{
   return s_diagSecureOn ;
}

void pdLocalEnableDiaglogSecure()
{
   s_localDiagSecureOn = TRUE ;
}

void pdLocalDisableDiaglogSecure()
{
   s_localDiagSecureOn = FALSE ;
}

BOOLEAN pdLocalIsDiaglogSecureEnabled()
{
   return s_localDiagSecureOn ;
}

void pdEnableShieldLogMask( UINT64 mask )
{
   OSS_BIT_SET( s_shieldLogMask, mask ) ;
}

void pdDisableShieldLogMask( UINT64 mask )
{
   OSS_BIT_CLEAR( s_shieldLogMask, mask ) ;
   OSS_BIT_CLEAR( s_hasIncCntMask, mask ) ;
}

BOOLEAN pdTestShieldLogMask( UINT64 mask )
{
   if ( OSS_BIT_TEST( s_shieldLogMask, mask ) )
   {
      return TRUE ;
   }
   return FALSE ;
}

BOOLEAN pdHasShieldLogMask()
{
   if ( s_shieldLogMask != 0 )
   {
      return TRUE ;
   }
   return FALSE ;
}

BOOLEAN pdIsShieldLog()
{
   if ( pdHasShieldLogMask() )
   {
      INT32 rc = pdGetLastError() ;
      if ( rc != SDB_OK )
      {
         UINT64 rcMask = _pdRC2Mask( rc ) ;
         if ( pdTestShieldLogMask( rcMask ) )
         {
            _pdIncShieldLogCntByRC( rc ) ;
            return TRUE ;
         }
      }
   }
   return FALSE ;
}

void pdPrintShieldInfo()
{
   UINT32 i = 0 ;
   while ( i < PD_MAX_SHIELD_RC_COUNT && i < sizeof( s_rcMaskMap ) )
   {
      if ( s_shieldLogCnt[i] > 0 )
      {
         PD_LOG( PDWARNING, "Shield log[rc: %d] printing %u times",
                 s_rcMaskMap[i]._rc , s_shieldLogCnt[i] ) ;
         s_shieldLogCnt[i] = 0 ;
      }
      i++ ;
   }
}

INT32 pdError( INT32 rc )
{
   pdSetLastError( rc ) ;
   return rc ;
}

pdLogShield::pdLogShield() : _addRCMask( 0 )
{
}

pdLogShield::~pdLogShield()
{
   clearRC() ;
}

void pdLogShield::clearRC()
{
   pdDisableShieldLogMask( _addRCMask ) ;
}

void pdLogShield::addRC( INT32 rc )
{
   UINT64 mask = _pdRC2Mask( rc ) ;
   if ( mask != 0 && !pdTestShieldLogMask( mask ) )
   {
      pdEnableShieldLogMask( mask ) ;
      OSS_BIT_SET( _addRCMask, mask ) ;
   }
}

#ifdef _DEBUG

void pdassert(const CHAR* string, const CHAR* func, const CHAR* file, UINT32 line)
{
   pdLog(PDSEVERE, func, file, line, string);
   ossPanic() ;
}

void pdcheck(const CHAR* string, const CHAR* func, const CHAR* file, UINT32 line)
{
   pdLog(PDSEVERE, func, file, line, string);
}

#endif // _DEBUG

/*
   PD Audit Implement
*/

struct _pdAuditConfig
{
   UINT32            _userMask ;
   UINT32            _userConfigMask ;
   UINT32            _csMask ;
   UINT32            _csConfigMask ;
   UINT32            _clMask ;
   UINT32            _clConfigMask ;

   UINT32            _version ;

   BOOLEAN isInConfig( AUDIT_TYPE auditType ) const
   {
      UINT32 oprMask = pdAuditType2Mask( auditType ) ;
      INT32 level = pdGetAuditTypeMinLevel( auditType ) ;
      for ( ; level >= AUDIT_LEVEL_USER ; --level )
      {
         switch ( level )
         {
            case AUDIT_LEVEL_CL :
               if ( _clConfigMask & oprMask )
               {
                  return ( oprMask & _clMask ) ? TRUE : FALSE ;
               }
               break ;
            case AUDIT_LEVEL_CS :
               if ( _csConfigMask & oprMask )
               {
                  return ( oprMask & _csMask ) ? TRUE : FALSE ;
               }
               break ;
            case AUDIT_LEVEL_USER :
               if ( _userConfigMask & oprMask )
               {
                  return ( oprMask & _userMask ) ? TRUE : FALSE ;
               }
               break ;
            default :
               break ;
         }
      }
      return ( oprMask & pdGetAuditMask() ) ? TRUE : FALSE ;
   }

   void  updateConfig( AUDIT_LEVEL level, UINT32 mask, UINT32 configMask )
   {
      switch ( level )
      {
         case AUDIT_LEVEL_CL :
            _clConfigMask = configMask ;
            _clMask = mask ;
            break ;
         case AUDIT_LEVEL_CS :
            _csConfigMask = configMask ;
            _csMask = mask ;
            break ;
         case AUDIT_LEVEL_USER :
            if ( configMask != _userConfigMask ||
                 mask != _userMask )
            {
               _userConfigMask = configMask ;
               _userMask = mask ;
               ++_version ;
            }
            break ;
         default :
            break ;
      }
   }

   void  getConfig( AUDIT_LEVEL level, UINT32 &mask, UINT32 &configMask )
   {
      switch ( level )
      {
         case AUDIT_LEVEL_CL :
            configMask = _clConfigMask  ;
            mask = _clMask ;
            break ;
         case AUDIT_LEVEL_CS :
            configMask = _csConfigMask ;
            mask = _csMask ;
            break ;
         case AUDIT_LEVEL_USER :
            configMask = _userConfigMask ;
            mask = _userMask ;
            break ;
         default :
            break ;
      }
   }

   UINT32 getVersion() const
   {
      return _version ;
   }

   void  clearConfig( AUDIT_LEVEL level )
   {
      switch ( level )
      {
         case AUDIT_LEVEL_CL :
            _clConfigMask = 0 ;
            _clMask = 0 ;
            break ;
         case AUDIT_LEVEL_CS :
            _csConfigMask = 0 ;
            _csMask = 0 ;
            break ;
         case AUDIT_LEVEL_USER :
            if ( 0 != _userConfigMask ||
                 0 != _userMask )
            {
               _userConfigMask = 0 ;
               _userMask = 0 ;
               _version = 0 ;
            }
            break ;
         default :
            break ;
      }
   }

   void  clearUpBoundConfig( AUDIT_LEVEL level )
   {
      INT32 tmpLevel = AUDIT_LEVEL_CL ;
      for ( ; tmpLevel >= (INT32)level ; --tmpLevel )
      {
         clearConfig( (AUDIT_LEVEL)tmpLevel ) ;
      }
   }

   void  clearAllConfig()
   {
      _userMask         = 0 ;
      _userConfigMask   = 0 ;
      _csMask           = 0 ;
      _csConfigMask     = 0 ;
      _clMask           = 0 ;
      _clConfigMask     = 0 ;
      _version          = 0 ;
   }
} ;
typedef _pdAuditConfig pdAuditConfig ;

const CHAR* pdAuditObjType2String( AUDIT_OBJ_TYPE objtype )
{
   const static CHAR* s_objtypeString[] = {
      "SYSTEM",
      "COLLECTIONSPACE",
      "COLLECTION",
      "GROUP",
      "NODE",
      "DOMAIN",
      "PROCEDURE",
      "FILE",
      "SESSION",
      "USER",
      "SEQUENCE",
      "RECYCLEBIN",
      "ROLE"
      } ;
   if ( (UINT32)objtype >= 0 &&
        (UINT32)objtype < sizeof(s_objtypeString) / sizeof(const CHAR*) )
   {
      return s_objtypeString[ (UINT32)objtype ] ;
   }
   return "" ;
}

UINT32 pdAuditType2Mask( AUDIT_TYPE auditType )
{
   switch( auditType )
   {
      case AUDIT_ACCESS :
         return AUDIT_MASK_ACCESS ;
      case AUDIT_CLUSTER :
         return AUDIT_MASK_CLUSTER ;
      case AUDIT_SYSTEM :
         return AUDIT_MASK_SYSTEM ;
      case AUDIT_DML :
         return AUDIT_MASK_DML ;
      case AUDIT_DDL :
         return AUDIT_MASK_DDL ;
      case AUDIT_DCL :
         return AUDIT_MASK_DCL ;
      case AUDIT_DQL :
         return AUDIT_MASK_DQL ;
      case AUDIT_DELETE :
         return AUDIT_MASK_DELETE ;
      case AUDIT_UPDATE :
         return AUDIT_MASK_UPDATE ;
      case AUDIT_INSERT :
         return AUDIT_MASK_INSERT ;
      case AUDIT_OTHER :
         return AUDIT_MASK_OTHER ;
      default :
         break ;
   }
   return 0 ;
}

AUDIT_LEVEL pdGetAuditTypeMinLevel( AUDIT_TYPE auditType )
{
   switch( auditType )
   {
      case AUDIT_DML :
      case AUDIT_DQL :
      case AUDIT_DELETE :
      case AUDIT_UPDATE :
      case AUDIT_INSERT :
         return AUDIT_LEVEL_CL ;
      default :
         break ;
   }
   return AUDIT_LEVEL_USER ;
}

const CHAR* pdGetAuditTypeDesp( AUDIT_TYPE auditType )
{
   switch( auditType )
   {
      case AUDIT_ACCESS :
         return "ACCESS" ;
      case AUDIT_CLUSTER :
         return "CLUSTER" ;
      case AUDIT_SYSTEM :
         return "SYSTEM" ;
      case AUDIT_DML :
         return "DML" ;
      case AUDIT_DDL :
         return "DDL" ;
      case AUDIT_DCL :
         return "DCL" ;
      case AUDIT_DQL :
         return "DQL" ;
      case AUDIT_DELETE :
         return "DELETE" ;
      case AUDIT_UPDATE :
         return "UPDATE" ;
      case AUDIT_INSERT :
         return "INSERT" ;
      case AUDIT_OTHER :
         return "OTHER" ;
      default :
         break ;
   }
   return "UNKNOW" ;
}

static INT32 _pdString2AuditMask( const CHAR *pStr,
                                  UINT32 &mask,
                                  BOOLEAN allowNot,
                                  UINT32 *pConfigMask )
{
   if ( !pStr || !*pStr )
   {
      return SDB_OK ;
   }

   const CHAR *start = pStr ;
   BOOLEAN isNot = FALSE ;
   UINT32 theMask = 0 ;

   while( ' ' == *start )
   {
      ++start ;
   }

   if ( !*start )
   {
      return SDB_OK ;
   }

   if ( allowNot && '!' == *start && !isNot )
   {
      isNot = TRUE ;
      ++start ;
      while( ' ' == *start )
      {
         ++start ;
      }
      if ( !*start )
      {
         return SDB_INVALIDARG ;
      }
   }

   const CHAR *end = start + ossStrlen( start ) - 1 ;
   while( end != start && ' ' == *end )
   {
      --end ;
   }
   UINT32 len = end - start + 1 ;
   /// compare
   if ( 0 == ossStrncasecmp( start, "ACCESS", len ) )
   {
      theMask = AUDIT_MASK_ACCESS ;
   }
   else if ( 0 == ossStrncasecmp( start, "CLUSTER", len ) )
   {
      theMask = AUDIT_MASK_CLUSTER ;
   }
   else if ( 0 == ossStrncasecmp( start, "SYSTEM", len ) )
   {
      theMask = AUDIT_MASK_SYSTEM ;
   }
   else if ( 0 == ossStrncasecmp( start, "DML", len ) )
   {
      theMask = AUDIT_MASK_DML ;
   }
   else if ( 0 == ossStrncasecmp( start, "DDL", len ) )
   {
      theMask = AUDIT_MASK_DDL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DCL", len ) )
   {
      theMask = AUDIT_MASK_DCL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DQL", len ) )
   {
      theMask = AUDIT_MASK_DQL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DELETE", len ) )
   {
      theMask = AUDIT_MASK_DELETE ;
   }
   else if ( 0 == ossStrncasecmp( start, "UPDATE", len ) )
   {
      theMask = AUDIT_MASK_UPDATE ;
   }
   else if ( 0 == ossStrncasecmp( start, "INSERT", len ) )
   {
      theMask = AUDIT_MASK_INSERT ;
   }
   else if ( 0 == ossStrncasecmp( start, "OTHER", len ) )
   {
      theMask = AUDIT_MASK_OTHER ;
   }
   else if ( 0 == ossStrncasecmp( start, "NONE", len ) )
   {
      /// do nothing
   }
   else if ( 0 == ossStrncasecmp( start, "ALL", len ) )
   {
      theMask = AUDIT_MASK_ALL ;
   }
   else
   {
      return SDB_INVALIDARG ;
   }

   if ( pConfigMask )
   {
      *pConfigMask |= theMask ;
   }
   if ( !isNot )
   {
      mask |= theMask ;
   }

   return SDB_OK ;
}

INT32 pdString2AuditMask( const CHAR *pStr,
                          UINT32 &mask,
                          BOOLEAN allowNot,
                          UINT32 *pConfigMask )
{
   INT32 rc = SDB_OK ;
   mask = 0 ;
   if ( pConfigMask )
   {
      *pConfigMask = 0 ;
   }

   if ( !pStr || !*pStr )
   {
      return SDB_OK ;
   }

   const CHAR *p = pStr ;
   CHAR *p1 = NULL ;
   while( p && *p )
   {
      p1 = (CHAR*)ossStrchr( p, '|' ) ;
      if ( p1 )
      {
         *p1 = 0 ;
      }
      rc = _pdString2AuditMask( p, mask, allowNot, pConfigMask ) ;
      if ( p1 )
      {
         *p1 = '|' ;
      }
      if ( rc )
      {
         break ;
      }
      p = p1 ? p1 + 1 : NULL ;
   }
   return rc ;
}

static OSS_THREAD_LOCAL _pdAuditConfig s_curAuditConfig ;

void pdInitCurAuditMask( UINT32 mask )
{
   s_curAuditConfig.clearAllConfig() ;
}

void pdUpdateCurAuditMask( AUDIT_LEVEL level, UINT32 mask, UINT32 configMask )
{
   s_curAuditConfig.updateConfig( level, mask, configMask ) ;
}

void pdClearCurAuditMask( AUDIT_LEVEL level )
{
   s_curAuditConfig.clearConfig( level ) ;
}

void pdClearCurUpBoundAuditMask( AUDIT_LEVEL level )
{
   s_curAuditConfig.clearUpBoundConfig( level ) ;
}

void pdClearCurAllAuditMask()
{
   s_curAuditConfig.clearAllConfig() ;
}

void pdGetCurAuditMask( AUDIT_LEVEL level, UINT32 &mask,UINT32 &configMask )
{
   s_curAuditConfig.getConfig( level, mask, configMask ) ;
}

BOOLEAN pdIsAuditTypeEnabled( AUDIT_TYPE auditType )
{
   return s_curAuditConfig.isInConfig( auditType ) ;
}

UINT32 pdGetCurAuditVersion()
{
   return s_curAuditConfig.getVersion() ;
}

UINT32& pdGetAuditMask()
{
   static UINT32 s_auditMask = AUDIT_MASK_DEFAULT ;
   return s_auditMask ;
}

UINT32 pdSetAuditMask( UINT32 newMask )
{
   UINT32 oldMask = pdGetAuditMask() ;
   pdGetAuditMask() = newMask ;
   return oldMask ;
}

const CHAR* getAuditName ()
{
   return _getPDCfgInfo( PD_AUDIT )._pdLogFile ;
}

const CHAR* getAuditPath ()
{
   return _getPDCfgInfo( PD_AUDIT )._pdLogPath ;
}

void sdbEnableAudit( const CHAR *pdPathOrFile, INT32 fileMaxNum,
                     UINT32 fileMaxSize )
{
   sdbDisableAudit() ;

   pdCfgInfo &info = _getPDCfgInfo( PD_AUDIT ) ;
   const CHAR *shortName = PD_DFT_AUDIT ;

   if ( pdPathOrFile && 0 != pdPathOrFile[0] )
   {
      const CHAR *pDotStr = ossStrrchr( pdPathOrFile, '.' ) ;
      const CHAR *pSepStr1 = ossStrrchr( pdPathOrFile, '/' ) ;
      const CHAR *pSepStr2 = ossStrrchr( pdPathOrFile, '\\' ) ;
      const CHAR *pSepStr = pSepStr1 >= pSepStr2 ? pSepStr1 : pSepStr2 ;

      // is file
      if ( pDotStr && pDotStr > pSepStr )
      {
         shortName = pSepStr ? pSepStr + 1 : pdPathOrFile ;
         ossStrncpy( info._pdLogPath, pdPathOrFile, shortName - pdPathOrFile ) ;
      }
      // is path
      else
      {
         ossStrncpy( info._pdLogPath, pdPathOrFile, OSS_MAX_PATHSIZE ) ;
      }
   }

   if ( 0 == info._pdLogPath[0] )
   {
      ossStrcpy( info._pdLogPath, "." ) ;
   }

   if ( info._pdLogPath[0] != 0 )
   {
      engine::utilBuildFullPath( info._pdLogPath, shortName,
                                 OSS_MAX_PATHSIZE, info._pdLogFile ) ;
      info._pdLogFile[ OSS_MAX_PATHSIZE ] = 0 ;
   }
   info._pdFileMaxNum = fileMaxNum ;
   info._pdFileMaxSize = (UINT64)fileMaxSize * 1024 * 1024 ;
}

void sdbDisableAudit()
{
   pdLogFile &logFile = _pdLogFiles[ PD_AUDIT ] ;
   if ( logFile._logFile.isValid() )
   {
      logFile._logFile.Close() ;
   }
   _getPDCfgInfo( PD_AUDIT )._pdLogFile[0] = 0 ;
}

BOOLEAN sdbIsAuditEnabled ()
{
   return _getPDCfgInfo( PD_AUDIT ).isEnabled() ;
}

void pdAuditRaw( AUDIT_TYPE type, const CHAR *pData )
{
   INT32 rc = SDB_OK ;

   if ( !pdIsAuditTypeEnabled( type ) )
      return ;

   // use thread specific pointer to make sure there's no nested pdLog (i.e.
   // calling pdLog in signal handler when the thread is already in pdLog
   // function will not proceed)
   static OSS_THREAD_LOCAL BOOLEAN amIInPD = FALSE ;
   if ( amIInPD )
   {
      goto done ;
   }
   amIInPD = TRUE ;

   /* We write into log file if the string is not empty */
   if ( _getPDCfgInfo( PD_AUDIT ).isEnabled() )
   {
      rc = pdLogFileWrite ( PD_AUDIT, pData ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to write into audit log file, rc: %d" OSS_NEWLINE,
                     rc ) ;
         ossPrintf ( "%s" OSS_NEWLINE, pData ) ;
      }
   }

   // make sure to reset this before leaving
   amIInPD = FALSE ;

done:
   return ;
}

void pdAudit( AUDIT_TYPE type, const CHAR *pUserName,
              const CHAR* ipAddr, UINT16 port,
              const CHAR *pAction, AUDIT_OBJ_TYPE objType,
              const CHAR *pObjName, INT32 result,
              const CHAR* func, const CHAR* file,
              UINT32 line, const std::string &message )
{
   pdAudit( type, pUserName, ipAddr, port, pAction, objType,
            pObjName, result, func, file, line, message.c_str() ) ;
}

/*
 * Audit Log Header format
 * Arguments:
 * 1) Year (UINT32)
 * 2) Month (UINT32)
 * 3) Day (UINT32)
 * 4) Hour (UINT32)
 * 5) Minute (UINT32)
 * 6) Second (UINT32)
 * 7) Microsecond (UINT32)
 * 8) Type (string)
 * 9) Process ID (UINT64)
 * 10) Thread ID (UINT64)
 * 11) UserName (string)
 * 12) From (string)
 * 13) Action (string)
 * 14) Result (string) SUCCEED/FAILED
 * 15) ResultCode (INT32)
 * 16) ObjectType (string)
 * 17) ObjectName (string)
 *************ONLY IN DEBUG*************
 * 18) File Name (string)
 * 19) Function Name (string)
 * 20) Line number (UINT32)
 *************ONLY IN DEBUG*************
 * 21) Message
 */
const static CHAR *PD_AUDIT_LOG_HEADER_FORMAT="%04d-%02d-%02d-%02d.%02d.%02d.%06d\
               Type:%s" OSS_NEWLINE
"PID:%-37dTID:%d" OSS_NEWLINE
"UserName:%-32sFrom:%s" OSS_NEWLINE
"Action:%-34sResult:%s(%d)" OSS_NEWLINE
"ObjectType:%-30sObjectName:%s" OSS_NEWLINE
#ifdef _DEBUG
"Function:%-32sLine:%d" OSS_NEWLINE "File:%s" OSS_NEWLINE
#endif //_DEBUG
"Message:" OSS_NEWLINE "%s" OSS_NEWLINE OSS_NEWLINE ;

// PD_TRACE_DECLARE_FUNCTION ( SDB_PDAUDIT, "pdAudit" )
void pdAudit( AUDIT_TYPE type, const CHAR *pUserName,
              const CHAR* ipAddr, UINT16 port,
              const CHAR *pAction, AUDIT_OBJ_TYPE objType,
              const CHAR *pObjName, INT32 result,
              const CHAR* func, const CHAR* file,
              UINT32 line, const CHAR* format, ... )
{
   if ( !pdIsAuditTypeEnabled( type ) )
      return ;
   va_list ap;
   PD_TRACE_ENTRY ( SDB_PDAUDIT ) ;
   struct tm otm ;
   struct timeval tv;
   struct timezone tz;
   time_t tt ;
   CHAR userInfo[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;
   CHAR sysInfo[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;  // for log header
   CHAR szFrom[ 30 ] = { 0 } ;   // ip + port

   ossSignalShield sigShield ;
   sigShield.doNothing() ;

   gettimeofday(&tv, &tz);
   tt = tv.tv_sec ;

#if defined (_WINDOWS)
   localtime_s( &otm, &tt ) ;
#else
   localtime_r( &tt, &otm ) ;
#endif

   // create the user information
   va_start(ap, format);
   vsnprintf(userInfo, PD_LOG_STRINGMAX, format, ap);
   va_end(ap);

   if ( ipAddr && *ipAddr && port )
   {
      ossSnprintf( szFrom, sizeof(szFrom)-1, "%s:%u", ipAddr, port ) ;
   }

   // create log header
   ossSnprintf(sysInfo, PD_LOG_STRINGMAX, PD_AUDIT_LOG_HEADER_FORMAT,
               otm.tm_year+1900,            // 1) Year (UINT32)
               otm.tm_mon+1,                // 2) Month (UINT32)
               otm.tm_mday,                 // 3) Day (UINT32)
               otm.tm_hour,                 // 4) Hour (UINT32)
               otm.tm_min,                  // 5) Minute (UINT32)
               otm.tm_sec,                  // 6) Second (UINT32)
               tv.tv_usec,                  // 7) Microsecond (UINT32)
               pdGetAuditTypeDesp(type),    // 8) Type (string)
               ossGetCurrentProcessID(),    // 9) Process ID (UINT64)
               ossGetCurrentThreadID(),     // 10) Thread ID (UINT64)
               pUserName ? pUserName : "",  // 11) UserName (string)
               szFrom,                      // 12) From (string)
               pAction ? pAction : "",      // 13) Action (string)
               result ? "FAILED" : "SUCCEED",//14) Result (string)
               result,                      // 15) ResultCode (INT32)
               pdAuditObjType2String(objType),// 16) ObjectType (string)
               pObjName ? pObjName : "",    // 17) ObjectName (string)
#ifdef _DEBUG
               func,                        // 18) Function Name (string)
               line,                        // 19) Line number (UINT32)
               file,                        // 20) File Name (string)
#endif //_DEBUG
               userInfo                     // 21) Message
   ) ;

   pdAuditRaw( type, sysInfo ) ;

   PD_TRACE_EXIT( SDB_PDAUDIT ) ;
   return ;
}

