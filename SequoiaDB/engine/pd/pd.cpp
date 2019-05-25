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
   ossPrimitiveFileOp _logFile ;
   UINT64             _fileSize ;
   ossSpinXLatch _mutex ;
} ;
typedef struct _pdLogFile pdLogFile ;

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

      if ( pDotStr && pDotStr > pSepStr )
      {
         shortName = pSepStr ? pSepStr + 1 : pdPathOrFile ;
         ossStrncpy( info._pdLogPath, pdPathOrFile, shortName - pdPathOrFile ) ;
      }
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
const static CHAR *PD_LOG_HEADER_FORMAT="%04d-%02d-%02d-%02d.%02d.%02d.%06d\
               \
Level:%s"OSS_NEWLINE"PID:%-37dTID:%d"OSS_NEWLINE"Function:%-32sLine:%d"\
OSS_NEWLINE"File:%s"OSS_NEWLINE"Message:"OSS_NEWLINE"%s"OSS_NEWLINE OSS_NEWLINE;
/* extern variables */

#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )

static void _pdRemoveOutOfDataFiles( pdCfgInfo &info )
{
   multimap< string, string >  mapFiles ;
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

   while ( mapFiles.size() > 0 &&
           mapFiles.size() >= (UINT32)info._pdFileMaxNum )
   {
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

   if ( SDB_OK == ossAccess( fileName ) )
   {
      rc = ossDelete( fileName ) ;
      if ( rc )
      {
         ossPrintf( "Delete file %s failed, rc: %d"OSS_NEWLINE,
                    fileName, rc ) ;
      }
   }
   rc = ossRenamePath( info._pdLogFile, fileName ) ;
   if ( rc )
   {
      ossPrintf( "Rename %s to %s failed, rc: %d"OSS_NEWLINE,
                 info._pdLogFile, fileName, rc ) ;
      ossDelete( info._pdLogFile ) ;
   }

   if ( info._pdFileMaxNum > 0 )
   {
      _pdRemoveOutOfDataFiles( info ) ;
   }

   return rc ;
}

#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

// PD_TRACE_DECLARE_FUNCTION ( SDB_PDLOGFILEWRITE, "pdLogFileWrite" )
static INT32 pdLogFileWrite ( _pdLogType type, const CHAR *pData )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_PDLOGFILEWRITE ) ;
   SDB_ASSERT ( type < PD_LOG_MAX, "type is out of range" ) ;
   SINT64 dataSize = ossStrlen ( pData ) ;
   pdLogFile &logFile = _pdLogFiles[type] ;
   pdCfgInfo &info = _getPDCfgInfo( type ) ;

   logFile._mutex.get() ;


#if defined ( SDB_ENGINE ) || defined ( SDB_FMP ) || defined ( SDB_TOOL )
open:
#endif //SDB_ENGINE || SDB_FMP || SDB_TOOL

   if ( !logFile._logFile.isExist() )
   {
      logFile._logFile.Close() ;
   }

   if ( !logFile._logFile.isValid() )
   {
      rc = logFile._logFile.Open ( info._pdLogFile ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to open log file, errno = %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      else
      {
         ossPrimitiveFileOp::offsetType fileSize ;
         rc = logFile._logFile.getSize( &fileSize ) ;
         if ( rc )
         {
            ossPrintf( "Failed to get log file size, rc: %d"OSS_NEWLINE, rc ) ;
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
      ossPrintf ( "Failed to write log file, rc: %d"OSS_NEWLINE,
                  rc ) ;
      goto error ;
   } // if ( rc )
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

   va_start(ap, format);
   vsnprintf(userInfo, PD_LOG_STRINGMAX, format, ap);
   va_end(ap);

   ossSnprintf(sysInfo, PD_LOG_STRINGMAX, PD_LOG_HEADER_FORMAT,
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

   pdLogRaw( level, sysInfo ) ;

   PD_TRACE_EXIT( SDB_PDLOG ) ;
   return ;
}

void pdLogRaw( PDLEVEL level, const CHAR *pData )
{
   INT32 rc = SDB_OK ;

   if ( getPDLevel() < level )
      return ;

   static OSS_THREAD_LOCAL BOOLEAN amIInPD = FALSE ;
   if ( amIInPD )
   {
      goto done ;
   }
   amIInPD = TRUE ;

#if defined (_DEBUG) && defined (SDB_ENGINE)
      ossPrintf ( "%s"OSS_NEWLINE, pData ) ;
#else
   /* We write into log file if the string is not empty */
   if ( _getPDCfgInfo( PD_DIAGLOG ).isEnabled() )
#endif
   {
      rc = pdLogFileWrite ( PD_DIAGLOG, pData ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to write into log file, rc: %d"OSS_NEWLINE, rc ) ;
         ossPrintf ( "%s"OSS_NEWLINE, pData ) ;
      }
   }

   amIInPD = FALSE ;

done:
   return ;
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
      "USER"
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

static INT32 _pdString2AuditMask( const CHAR *pStr, UINT32 &mask )
{
   if ( !pStr || !*pStr )
      return SDB_OK ;
   const CHAR *start = pStr ;
   while( *start && ' ' == *start )
   {
      ++start ;
   }
   if ( !*start )
      return SDB_OK ;
   const CHAR *end = start + ossStrlen( start ) - 1 ;
   while( end != start && ' ' == *end )
   {
      --end ;
   }
   UINT32 len = end - start + 1 ;
   if ( 0 == ossStrncasecmp( start, "ACCESS", len ) )
   {
      mask |= AUDIT_MASK_ACCESS ;
   }
   else if ( 0 == ossStrncasecmp( start, "CLUSTER", len ) )
   {
      mask |= AUDIT_MASK_CLUSTER ;
   }
   else if ( 0 == ossStrncasecmp( start, "SYSTEM", len ) )
   {
      mask |= AUDIT_MASK_SYSTEM ;
   }
   else if ( 0 == ossStrncasecmp( start, "DML", len ) )
   {
      mask |= AUDIT_MASK_DML ;
   }
   else if ( 0 == ossStrncasecmp( start, "DDL", len ) )
   {
      mask |= AUDIT_MASK_DDL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DCL", len ) )
   {
      mask |= AUDIT_MASK_DCL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DQL", len ) )
   {
      mask |= AUDIT_MASK_DQL ;
   }
   else if ( 0 == ossStrncasecmp( start, "DELETE", len ) )
   {
      mask |= AUDIT_MASK_DELETE ;
   }
   else if ( 0 == ossStrncasecmp( start, "UPDATE", len ) )
   {
      mask |= AUDIT_MASK_UPDATE ;
   }
   else if ( 0 == ossStrncasecmp( start, "INSERT", len ) )
   {
      mask |= AUDIT_MASK_INSERT ;
   }
   else if ( 0 == ossStrncasecmp( start, "OTHER", len ) )
   {
      mask |= AUDIT_MASK_OTHER ;
   }
   else if ( 0 == ossStrncasecmp( start, "NONE", len ) )
   {
      mask = 0 ;
   }
   else if ( 0 == ossStrncasecmp( start, "ALL", len ) )
   {
      mask = AUDIT_MASK_ALL ;
   }
   else
   {
      return SDB_INVALIDARG ;
   }
   return SDB_OK ;
}

INT32 pdString2AuditMask( const CHAR *pStr, UINT32 &mask )
{
   INT32 rc = SDB_OK ;
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
      rc = _pdString2AuditMask( p, mask ) ;
      if ( p1 )
      {
         *p1 = '|' ;
      }
      if ( rc )
      {
         break ;
      }
      p = p1 ? p1 + 1 : NULL ;

      if ( 0 == mask || AUDIT_MASK_ALL == mask )
      {
         break ;
      }
   }
   return rc ;
}

UINT32& getAuditMask()
{
   static UINT32 s_auditMask = AUDIT_MASK_DEFAULT ;
   return s_auditMask ;
}

UINT32 setAuditMask( UINT32 newMask )
{
   UINT32 oldMask = getAuditMask() ;
   getAuditMask() = newMask ;
   return oldMask ;
}

static OSS_THREAD_LOCAL UINT32 s_curAuditMask = 0 ;

UINT32& getCurAuditMask()
{
   return s_curAuditMask ;
}

UINT32 setCurAuditMask( UINT32 newMask )
{
   UINT32 oldMask = getCurAuditMask() ;
   getCurAuditMask() = newMask ;
   return oldMask ;
}

void initCurAuditMask( UINT32 newMask )
{
   s_curAuditMask = newMask ;
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

      if ( pDotStr && pDotStr > pSepStr )
      {
         shortName = pSepStr ? pSepStr + 1 : pdPathOrFile ;
         ossStrncpy( info._pdLogPath, pdPathOrFile, shortName - pdPathOrFile ) ;
      }
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

   if ( !( getCurAuditMask() & pdAuditType2Mask( type ) ) )
      return ;

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
         ossPrintf ( "Failed to write into audit log file, rc: %d"OSS_NEWLINE,
                     rc ) ;
         ossPrintf ( "%s"OSS_NEWLINE, pData ) ;
      }
   }

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
               Type:%s"OSS_NEWLINE
"PID:%-37dTID:%d"OSS_NEWLINE
"UserName:%-32sFrom:%s"OSS_NEWLINE
"Action:%-34sResult:%s(%d)"OSS_NEWLINE
"ObjectType:%-30sObjectName:%s"OSS_NEWLINE
#ifdef _DEBUG
"Function:%-32sLine:%d"OSS_NEWLINE"File:%s"OSS_NEWLINE
#endif //_DEBUG
"Message:"OSS_NEWLINE"%s"OSS_NEWLINE OSS_NEWLINE ;

// PD_TRACE_DECLARE_FUNCTION ( SDB_PDAUDIT, "pdAudit" )
void pdAudit( AUDIT_TYPE type, const CHAR *pUserName,
              const CHAR* ipAddr, UINT16 port,
              const CHAR *pAction, AUDIT_OBJ_TYPE objType,
              const CHAR *pObjName, INT32 result,
              const CHAR* func, const CHAR* file,
              UINT32 line, const CHAR* format, ... )
{
   if ( !( getCurAuditMask() & pdAuditType2Mask( type ) ) )
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

   va_start(ap, format);
   vsnprintf(userInfo, PD_LOG_STRINGMAX, format, ap);
   va_end(ap);

   if ( ipAddr && *ipAddr && port )
   {
      ossSnprintf( szFrom, sizeof(szFrom)-1, "%s:%u", ipAddr, port ) ;
   }

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

