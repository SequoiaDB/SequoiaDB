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

   Source File Name = dpsDump.cpp

   Descriptive Name = Data Protection Service Log Formatter

   When/how to use: this program may be used on binary and text-formatted
   versions of data protection component. This file contains code to format log
   files.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/06/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sdbDpsDump.hpp"
#include "pmdOptionsMgr.hpp"
#include "dpsLogRecord.hpp"
#include "dpsLogFile.hpp"
#include "dpsDump.hpp"
#include "ossVer.hpp"
#include "utilStr.hpp"
#include "dpsLogRecordDef.hpp"
#include "ossPath.hpp"
#include "utilCommon.hpp"
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace fs = boost::filesystem ;
using namespace engine;

#ifndef R_OK
#define R_OK   4
#endif //R_OK

#define REPLOG_NAME_PREFIX "sequoiadbLog."
#define LOG_FILE_NAME "dpsDumpLog.log"
OSSFILE logFile ;
const static CHAR *logFMT = "Level: %s"OSS_NEWLINE"Function: %-32s"OSS_NEWLINE
                            "File: %s"OSS_NEWLINE"Line: %d"OSS_NEWLINE
                            "Message:"OSS_NEWLINE"%s"OSS_NEWLINE""OSS_NEWLINE;

void writeLog( BOOLEAN console, const CHAR *type, const CHAR *func,
               const CHAR *file, const int line, const CHAR *fmt, ... )
{

   INT32 rc = SDB_OK ;
   CHAR msg[4096] = { 0 } ;
   CHAR wContent[4096] = { 0 } ;
   va_list ap ;

   va_start( ap, fmt );
   vsnprintf( msg, 4096, fmt, ap ) ;
   va_end( ap ) ;

   ossSnprintf( wContent, 4096, logFMT, type, func, file, line, msg ) ;
   if ( console )
   {
      std::cout << type << ": " << msg << std::endl ;
   }

   {
      INT64 toWrite = ossStrlen( wContent ) ; ;
      INT64 writePos  = 0 ;
      INT64 writeSize = 0 ;
      while( toWrite > 0 )
      {
         rc = ossWrite( &logFile, wContent + writePos, toWrite, &writeSize ) ;
         if ( SDB_OK != rc && SDB_INTERRUPT != rc )
         {
            std::cout << "Failed to log to file" << std::endl ;
            goto done ;
         }

         rc = SDB_OK ;
         toWrite -= writeSize ;
         writePos += writeSize ;
      }
   }

done:
   return;
}

#define __LOG_WRAPPER( out, LEVEL, fmt, ... )                                  \
do                                                                             \
{                                                                              \
   writeLog( out, LEVEL, __FUNCTION__, __FILE__, __LINE__, fmt, __VA_ARGS__ ) ;\
} while ( FALSE )


#define LogError( fmt, ... )                    \
   __LOG_WRAPPER( TRUE, "Error", fmt, __VA_ARGS__ )

#define LogEvent( fmt, ... )                    \
   __LOG_WRAPPER( FALSE, "Event", fmt, __VA_ARGS__ )


/// filter factory implementation
_dpsFilterFactory::_dpsFilterFactory()
{
}

_dpsFilterFactory::~_dpsFilterFactory()
{
   std::list< dpsDumpFilter * >::iterator it = _filterList.begin() ;
   for ( ; it != _filterList.end() ; ++it )
   {
      release( *it ) ;
   }

   _filterList.clear() ;
}

_dpsFilterFactory* _dpsFilterFactory::getInstance()
{
   static _dpsFilterFactory factory ;
   return &factory ;
}

dpsDumpFilter* _dpsFilterFactory::createFilter( int type )
{
   dpsDumpFilter *filter = NULL ;

   switch( type )
   {
   case SDB_LOG_FILTER_TYPE :
      {
         filter = SDB_OSS_NEW dpsTypeFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_NAME :
      {
         filter = SDB_OSS_NEW dpsNameFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_LSN  :
      {
         filter = SDB_OSS_NEW dpsLsnFilter() ;
         break ;
      }
    case SDB_LOG_FILTER_META :
      {
          filter = SDB_OSS_NEW dpsMetaFilter() ;
          break ;
      }
   case SDB_LOG_FILTER_NONE :
      {
         filter = SDB_OSS_NEW dpsNoneFilter() ;
         break ;
      }
   case SDB_LOG_FILTER_LAST :
      {
         filter = SDB_OSS_NEW dpsLastFilter() ;
         break ;
      }

   default:
      LogError("unknown filter type: %d", type) ;
      goto error ;
   }

   if( NULL == filter )
   {
      LogError("Unable to allocate filter, type: %d", type) ;
      goto error ;
   }
   _filterList.push_back( filter ) ;

done:
   return filter ;
error:
   goto done ;
}

void _dpsFilterFactory::release( dpsDumpFilter *filter )
{
   if( NULL != filter )
   {
      SDB_OSS_DEL( filter ) ;
      filter = NULL ;
   }
}

/////////////////////////////////////////////////////////////////////
// this block is used to add filter's implementation of interface doFilter()
// and other interface was declared in macro DECLARE_FILTER(...)

////////////////////////////////////////////////////////////////////
/// for dpsTypeFilter
BOOLEAN _dpsTypeFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   BOOLEAN rc = FALSE ;
   dpsLogRecordHeader *pHeader =(dpsLogRecordHeader*)( pRecord ) ;
   if( pHeader->_type == dumper->opType )
   {
      rc = dpsDumpFilter::match( dumper, pRecord ) ;
      goto done ;
   }

done:
   return rc ;
}

INT32 _dpsTypeFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for _dpsNameFilter
BOOLEAN _dpsNameFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   BOOLEAN rc = FALSE ;
   dpsLogRecord record ;
   record.load( pRecord ) ;
   dpsLogRecord::iterator itr = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;

   if( 0 == ossStrncmp( dumper->name, "", sizeof( dumper->name ) ) )
   {
      rc = dpsDumpFilter::match( dumper, pRecord ) ;
      goto done ;
   }

   if( itr.valid() )
   {
      if( NULL != ossStrstr( itr.value(), dumper->name ) )
      {
         rc = dpsDumpFilter::match( dumper, pRecord ) ;
         goto done ;
      }
   }

done:
   return rc ;
}

INT32 _dpsNameFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", filename, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for _dpsMetaFilter
BOOLEAN _dpsMetaFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsMetaFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   return SDB_OK ;
}

/*
////////////////////////////////////////////////////////////////////
///< for _dpsMetaFilter
BOOLEAN _dpsMetaFilter::match( const dpsCmdData *data, CHAR *pRecord )
{
   return FALSE ;
}

INT32 _dpsMetaFilter::doFilte( const dpsCmdData *data, OSSFILE &out,
                              const CHAR *logFilePath )
{
   UINT64 len           = 0 ;
   INT32 rc             = SDB_OK ;
   UINT64 outBufferSize = BLOCK_SIZE ;
   CHAR *pOutBuffer     = NULL ;
   BOOLEAN start        = FALSE ;
   dpsMetaData metaData ;
   if( dpsLogFilter::isDir( data->srcPath ) )
   {
      INT32 const MAX_FILE_COUNT =
         _dpsLogFilter::getFileCount( data->srcPath ) ;
      if( 0 == MAX_FILE_COUNT )
      {
         printf( "Cannot find any Log files\nPlease check"
            " and input the correct log file path\n" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      printf("Analysis Log File Data begin...\n\n" ) ;
      start = TRUE ;
      for( INT32 idx = 0 ; idx < MAX_FILE_COUNT ; ++idx )
      {
         // src log file ;
         fs::path fileDir( data->srcPath ) ;
         const CHAR *filepath = fileDir.string().c_str() ;
         CHAR shortName[ 30 ] = { 0 } ;
         CHAR filename[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

         ossSnprintf( shortName, sizeof( shortName ) - 1, "sequoiadbLog.%d",
            idx ) ;
         utilBuildFullPath( filepath, shortName, OSS_MAX_PATHSIZE, filename ) ;

         if( !dpsLogFilter::isFileExisted( filename ) )
         {
            printf( "Warning: file:[%s] is missing\n", filename ) ;
            continue ;
         }

         rc = metaFilte( metaData, out, filename, idx ) ;
         if( rc )
         {
            printf( "!parse log file: [%s] error, rc = %d\n", filename, rc ) ;
            continue ;
         }
      }
      metaData.fileCount = metaData.metaList.size() ;

retry:
      pOutBuffer = ( CHAR * )SDB_OSS_REALLOC( pOutBuffer , outBufferSize + 1 ) ;
      if( NULL == pOutBuffer )
      {
         printf( "Failed to allocate %lld bytes, LINE:%d, FILE:%s\n",
            outBufferSize + 1, __LINE__, __FILE__ ) ;
         ossMemset( pOutBuffer, 0, outBufferSize + 1 ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      if( 0 < metaData.fileCount )
      {
         len = analysisMetaData( metaData, pOutBuffer, outBufferSize ) ;
         if( len >= outBufferSize )
         {
            outBufferSize += BLOCK_SIZE ;
            goto retry ;
         }

         if( data->output )
         {
            printf( "%s", pOutBuffer ) ;
         }
         else
         {
            rc = writeToFile( out, pOutBuffer ) ;
            if( rc )
            {
               goto error ;
            }
         }
      }
   }
   else
   {
      printf( "meta info need assigned a path of dir\n" ) ;
      rc = SDB_INVALIDPATH ;
      goto error ;
   }

done:
   if( start )
   {
      printf("\nAnalysis Log File Data end...\n" ) ;
   }
   return rc;
error:
   goto done;
}
*/

////////////////////////////////////////////////////////////////////
///< for _dpsLsnFilter
BOOLEAN _dpsLsnFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsLsnFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                              const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

////////////////////////////////////////////////////////////////////
///< for _dpsNoneFilter
BOOLEAN _dpsNoneFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsNoneFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      //PD_LOG( "!parse log file: [%s] error, rc = %d", logFilePath, rc ) ;
      goto error ;
   }

done:
   return rc;

error:
   goto done;
}

////////////////////////////////////////////////////////////////////
///< for lastFilter
BOOLEAN _dpsLastFilter::match( dpsDumper *dumper, CHAR *pRecord )
{
   return dpsDumpFilter::match( dumper, pRecord ) ;
}

INT32 _dpsLastFilter::doFilte( dpsDumper *dumper, OSSFILE &out,
                               const CHAR *logFilePath )
{
   INT32 rc = SDB_OK ;

   rc = dumper->filte( this, out, logFilePath ) ;
   if( rc )
   {
      goto error ;
   }

done:
   return rc ;

error:
   goto done ;
}



_dpsDumper::_dpsDumper()
: _metaContent( NULL ),
  _metaContentSize( 0 ),
  _filter( NULL ),
  _fileNum( 0 )
{
}

_dpsDumper::~_dpsDumper()
{
   if ( _metaContent )
   {
      SDB_OSS_FREE( _metaContent ) ;
      _metaContent = NULL ;
      _metaContentSize = 0 ;
   }
}

INT32 _dpsDumper::initialize( INT32 argc, CHAR** argv,
                              po::options_description &desc,
                              po::variables_map &vm )
{
   INT32 rc            = SDB_OK ;
   po::options_description all ( "Command options" ) ;

   DPS_FILTER_ADD_OPTIONS_BEGIN( desc )
      FILTER_OPTIONS
   DPS_FILTER_ADD_OPTIONS_END

   DPS_FILTER_ADD_OPTIONS_BEGIN ( all )
      FILTER_OPTIONS
      DPS_DUMP_HIDDEN_OPTIONS
   DPS_FILTER_ADD_OPTIONS_END

   rc = utilReadCommandLine( argc, argv, all, vm ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Failed to parse command line" << std::endl ;
      goto error ;
   }

   if( !_validCheck( vm ) )
   {
      std::cout << "Invalid arguments" << std::endl ;
      displayArgs( desc ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( vm.count( DPS_DUMP_HELP ) )
   {
      displayArgs( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( DPS_DUMP_HELPFULL ) )
   {
      displayArgs( all ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( DPS_DUMP_VER ) )
   {
      ossPrintVersion( "SequoiaDB version" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }

   /// alloc memory
   _metaContent = ( CHAR* )SDB_OSS_MALLOC( BLOCK_SIZE + 1 ) ;
   if ( !_metaContent )
   {
      std::cout << "Failed to alloc " << BLOCK_SIZE << " bytes memory"
                << std::endl ;
      rc = SDB_OOM ;
      goto error ;
   }
   _metaContentSize = BLOCK_SIZE ;
   ossMemset( _metaContent, 0 , BLOCK_SIZE ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::prepare( po::variables_map &vm )
{
   INT32 rc = SDB_OK ;
   rc = engine::pmdCfgRecord::init( NULL, &vm ) ;
   if( rc )
   {
      std::cout << "Invalid arguments" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::process( const po::options_description &desc,
                           const po::variables_map &vm )
{
   INT32 rc            = SDB_OK ;
   dpsDumpFilter *nextFilter = NULL ;
   string repairStr ;

   if( vm.count( DPS_DUMP_REPAIRE ) )
   {
      repairStr = vm[DPS_DUMP_REPAIRE].as<string>().c_str() ;
      rc = repaireHeader( repairStr ) ;
      if ( rc )
      {
         goto error ;
      }
      goto done ;
   }

   if( vm.count( DPS_DUMP_OUTPUT ) )
   {
      consolePrint = FALSE ;
   }
   else
   {
      consolePrint = TRUE ;
   }

   ///< we should deal with lsn filter first
   if( vm.count( DPS_DUMP_LSN ) && vm.count( DPS_DUMP_LAST ) )
   {
      std::cout << "--lsn cannot be used with --last!!" << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( vm.count( DPS_DUMP_LSN ) )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_LSN ) ;
      CHECK_FILTER( _filter ) ;
      const CHAR *pLsn = vm[ DPS_DUMP_LSN ].as<std::string>().c_str() ;
      try
      {
         lsn = boost::lexical_cast< UINT64 >( pLsn ) ;
      }
      catch( boost::bad_lexical_cast& e )
      {
         std::cout << "Unable to cast lsn to UINT64: " << e.what()
                   << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else if( vm.count( DPS_DUMP_LAST ) )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_LAST ) ;
      CHECK_FILTER( _filter ) ;
   }

   if( vm.count( DPS_DUMP_TYPE ) )
   {
      nextFilter = dpsFilterFactory::getInstance()
                   ->createFilter( SDB_LOG_FILTER_TYPE ) ;
      CHECK_FILTER( nextFilter ) ;
      ASSIGNED_FILTER( _filter, nextFilter ) ;
   }

   if( vm.count( DPS_DUMP_NAME ) )
   {
      nextFilter = dpsFilterFactory::getInstance()
                   ->createFilter( SDB_LOG_FILTER_NAME ) ;
      CHECK_FILTER( nextFilter ) ;
      ASSIGNED_FILTER( _filter, nextFilter ) ;
   }

   if( vm.count( DPS_DUMP_META ) )
   {
      if( NULL != _filter )
      {
         std::cout << "meta command must be used alone!" << std::endl ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_META ) ;
      CHECK_FILTER( _filter ) ;
   }

   if( NULL == _filter )
   {
      _filter = dpsFilterFactory::getInstance()
                ->createFilter( SDB_LOG_FILTER_NONE ) ;
      CHECK_FILTER( _filter ) ;
   }

   rc = dump();
   if ( SDB_OK != rc )
   {
      if ( DPS_LOG_REACH_HEAD == rc )
      {
         std::cout << "over the valid log lsn" << std::endl ;
         goto done ;
      }
      else
      {
         std::cout << "error occurs when processing, rc = " << rc << std::endl;
         goto error;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_changeFileName()
{
   static UINT32 fileId = 0;
   UINT32 len = 0;
   ossMemset( dstPath + dstPathLen, 0, OSS_MAX_PATHSIZE - dstPathLen + 1 ) ;
   len = ossSnprintf( dstPath + dstPathLen,
                      OSS_MAX_PATHSIZE - dstPathLen + 1,
                      ".%d", fileId++ ) ;
   if ( dstPathLen + len >= OSS_MAX_PATHSIZE)
   {
      LogError("outpath (%s) length is too long", dstPath);
      return SDB_INVALIDPATH;
   }
   return SDB_OK;
}

INT32 _dpsDumper::repaireHeader( const std::string &str )
{
   INT32  rc               = SDB_OK ;
   BOOLEAN _isDir          = FALSE ;
   dpsLogHeader *logHeader = NULL ;
   BOOLEAN opened          = FALSE ;
   UINT64 value            = 0 ;
   INT64 fileSize          = 0 ;
   INT64 len               = 0 ;
   INT64 offset            = 0 ;
   OSSFILE in ;
   vector< pmdAddrPair > items ;
   pmdOptionsCB opt ;
   CHAR pLogHead[ DPS_LOG_HEAD_LEN + 1 ] = { 0 } ;
   std::stringstream ss ;

   LogEvent( "Repair file:[ %s ] begin", srcPath ) ;

   const CHAR *pin = str.c_str() ;
   CHAR *pos = (CHAR*)ossStrchr( pin, ':' ) ;
   if ( NULL == pos )
   {
      ossPrintf( "repaire format must be: header:xx=y,dd=k"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
   }
   *pos = 0 ;
   if ( 0 != ossStrcasecmp( pin, "header" ) )
   {
      *pos = ':' ;
      ossPrintf( "repaire only support for type header"OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
   }
   *pos = ':' ;

   /// parse header member
   rc = opt.parseAddressLine( pos + 1, items, ",", "=", 0 ) ;
   if ( SDB_OK != rc )
   {
      ossPrintf( "Parse repaire value failed: %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   if ( 0 == items.size() )
   {
      goto done ;
   }

   rc = isDir( srcPath, _isDir ) ;
   if( SDB_FNE == rc || SDB_PERM == rc )
   {
      LogError( "Permission error or path not exist: %s", srcPath ) ;
      goto error ;
   }
   if( SDB_OK != rc )
   {
      LogError( "Failed to get check whether %s is dir, rc = %d",
                srcPath, rc ) ;
      goto error ;
   }

   if( _isDir )
   {
      rc = SDB_INVALIDARG ;
      LogError( "srcPath[%s] can't be directory", srcPath ) ;
      goto error ;
   }

   rc = ossOpen( srcPath, OSS_DEFAULT | OSS_READWRITE,
                 OSS_RU | OSS_RG, in ) ;
   if( rc )
   {
      LogError( "Unable to open file[%s], rc = %d", srcPath, rc ) ;
      goto error ;
   }
   opened = TRUE ;

   rc = _checkLogFile( in, fileSize, srcPath );
   if( rc )
   {
      goto error ;
   }
   SDB_ASSERT( fileSize > 0, "fileSize must be gt 0" ) ;

   rc = _readLogHead( in, offset, fileSize, NULL, 0, pLogHead, len ) ;
   if( rc && DPS_LOG_FILE_INVALID != rc )
   {
      LogEvent( "Read a invlid log file: %s", srcPath ) ;
      goto error;
   }

   logHeader = (dpsLogHeader*)pLogHead ;

   ss << "The header of file[" << srcPath << "] has been changed: " ;

   for ( UINT32 i = 0 ; i < items.size() ; ++i )
   {
      pmdAddrPair &aItem = items[ i ] ;

      /// must be nubmer
      if ( !ossIsInteger( aItem._service ) )
      {
         ossPrintf( "Field[%s]'s value is not number[%s]"OSS_NEWLINE,
                    aItem._host, aItem._service ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      value = ossAtoll( aItem._service ) ;

      ss << endl ;

      if ( 0 == ossStrcasecmp( aItem._host, "LogID" ) )
      {
         ss << "logID[" << logHeader->_logID << "]===>logID[" << value
            << "]" ;
         logHeader->_logID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "FirstLSNVer" ) )
      {
         ss << "FirstLSNVer[" << logHeader->_firstLSN.version
            << "]===>FirstLSNVer[" << value << "]" ;
         logHeader->_firstLSN.version = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "FirstLSNOffset" ) )
      {
         ss << "firstLSNOffset[" << logHeader->_firstLSN.offset
            << "]===>firstLSNOffset[" << value << "]" ;
         logHeader->_firstLSN.offset = value ;
      }
      else
      {
         ossPrintf( "Unknow header key: %s"OSS_NEWLINE, aItem._host ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   rc = ossSeek( &in, 0, OSS_SEEK_SET ) ;
   if ( rc )
   {
      LogError( "Seek to %llu failed, rc: %d", 0, rc ) ;
      goto error ;
   }

   rc = ossWriteN( &in, pLogHead, DPS_LOG_HEAD_LEN ) ;
   if (SDB_OK != rc)
   {
      LogError( "Failed to write header to file[%s]", srcPath ) ;
      goto error ;
   }

   LogEvent( "%s", ss.str().c_str() ) ;

done:
   if( opened )
      ossClose( in ) ;
   LogEvent( "Repair file:[ %s ] end", srcPath ) ;
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::dump()
{
   INT32   rc      = SDB_OK ;
   OSSFILE fileFrom, fileTo ;
   UINT32 id = 0;
   UINT32 i = 0;
   dpsCmdData data ;
   map<UINT32, string > mapFiles;
   BOOLEAN _isDir = FALSE ;

   rc = isDir( dstPath, _isDir ) ;
   // if SDB_FNE == rc, then treat dstPath as file-type, not dir-type
   if ( SDB_FNE == rc )
   {
      _isDir = FALSE ;
      rc = SDB_OK ;
   }
   else if( SDB_PERM == rc )
   {
      LogError( "Permission error: %s", dstPath ) ;
      goto error ;
   }
   else if( SDB_OK != rc )
   {
      LogError( "Failed to check whether %s is dir, rc = %d",
                dstPath, rc ) ;
      goto error ;
   }

   dstPathLen = ossStrlen(dstPath);
   if( _isDir )
   {
      if ( OSS_FILE_SEP_CHAR == dstPath[ dstPathLen - 1 ] )
      {
         (void)ossStrncpy( dstPath + dstPathLen, "tmpLog.log",
                        OSS_MAX_PATHSIZE + 1 - dstPathLen);
      }
      else
      {
         (void)ossStrncpy( dstPath + dstPathLen, OSS_FILE_SEP"tmpLog.log",
                        OSS_MAX_PATHSIZE + 1 - dstPathLen);
      }
      dstPathLen = ossStrlen(dstPath);
   }

   rc = _changeFileName();
   if (rc != SDB_OK)
   {
      goto error ;
   }

   if( !consolePrint )
   {
      rc = ossOpen( dstPath, OSS_REPLACE | OSS_READWRITE,
                    OSS_RU | OSS_WU | OSS_RG, fileTo ) ;
      if( rc )
      {
         LogError("Unable to open file: %s", dstPath ) ;
         goto error ;
      }
   }

   rc = _getSortedFileMap( srcPath, mapFiles ) ;
   if( SDB_OK != rc )
   {
      LogError( "Failed to get Files from path %s, please check, rc = %d",
                srcPath, rc ) ;
      goto error ;
   }

   // analysis meta info whatever "-m" is assigned
   rc = _analysisMeta(mapFiles);
   if ( SDB_OK != rc )
   {
      goto error;
   }

   rc = _writeTo( fileTo, _metaContent, consolePrint ) ;
   if( rc )
   {
      goto error ;
   }

   /// < meta need do specially
   if ( SDB_LOG_FILTER_META == _filter->getType() )
   {
      goto done;
   }

   if( 0 == _meta.metaList.size() )
   {
      LogError( "Cannot find any dpsLogFile in path: %s", srcPath ) ;
      rc = SDB_FNE ;
      goto error ;
   }

   id = _meta.fileBegin;
   for(i = 0 ; i < _meta.metaList.size(); ++i)
   {
      string &filename = mapFiles[_meta.metaList[id].index];
      (void) _filter->doFilte( this, fileTo, filename.c_str());
      ++id;
      id = id % _meta.metaList.size();
   }

done:
   ossClose( fileTo ) ;
   return rc ;

error:
   goto done ;
}

INT32 _dpsDumper::parseMeta( CHAR *buffer )
{
   return SDB_OK ;
}

INT32 _dpsDumper::doDataExchange( engine::pmdCfgExchange *pEx )
{
   resetResult() ;

   rdxPathRaw( pEx, DPS_DUMP_SOURCE, srcPath, OSS_MAX_PATHSIZE,
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "./" ) ;

   rdxPathRaw( pEx, DPS_DUMP_OUTPUT, dstPath, OSS_MAX_PATHSIZE,
               FALSE, PMD_CFG_CHANGE_FORBIDDEN, "./" ) ;

   rdxString( pEx, DPS_DUMP_NAME, name,
              OSS_MAX_PATHSIZE, FALSE, PMD_CFG_CHANGE_FORBIDDEN, "" ) ;

   rdxUShort( pEx, DPS_DUMP_TYPE, opType,
              FALSE, PMD_CFG_CHANGE_RUN, (UINT16)PDWARNING ) ;

   rdxInt( pEx, DPS_DUMP_LSN_AHEAD, lsnAhead, FALSE, PMD_CFG_CHANGE_RUN, 20 ) ;

   rdxInt( pEx, DPS_DUMP_LSN_BACK, lsnBack, FALSE, PMD_CFG_CHANGE_RUN, 20 ) ;

   rdxInt( pEx, DPS_DUMP_LAST, lastCount, FALSE, PMD_CFG_CHANGE_RUN, 0 ) ;

   return getResult() ;
}

INT32 _dpsDumper::postLoaded( engine::PMD_CFG_STEP step )
{
   return SDB_OK ;
}

INT32 _dpsDumper::preSaving()
{
   return SDB_OK ;
}

BOOLEAN _dpsDumper::_validCheck( const po::variables_map &vm )
{
   BOOLEAN valid = FALSE ;

   if(   vm.count( DPS_DUMP_HELP )
      || vm.count( DPS_DUMP_VER )
      || vm.count( DPS_DUMP_TYPE )
      || vm.count( DPS_DUMP_NAME )
      || vm.count( DPS_DUMP_META )
      || vm.count( DPS_DUMP_LSN )
      || vm.count( DPS_DUMP_SOURCE )
      || vm.count( DPS_DUMP_OUTPUT )
      || vm.count( DPS_DUMP_LAST )
      || vm.count( DPS_DUMP_REPAIRE )
      || vm.count( DPS_DUMP_HELPFULL ) )
   {
      valid = TRUE ;
   }

   return valid ;
}

INT32 _dpsDumper::_analysisMeta( map<UINT32, string > &mapFiles )
{
   INT32 rc = SDB_OK ;
   BOOLEAN _isDir  = FALSE;
   UINT32 i = 0;
   UINT32 id = 0 ;
   UINT32 beginLogID = DPS_INVALID_LOG_FILE_ID ;

   rc = isDir( srcPath, _isDir ) ;
   if( SDB_FNE == rc || SDB_PERM == rc )
   {
      LogError( "Permission error or path not exist: %s", srcPath ) ;
      goto error ;
   }
   if( SDB_OK != rc )
   {
      LogError( "Failed to get check whether %s is dir, rc = %d",
                srcPath, rc ) ;
      goto error ;
   }

   //get start file id to dump log record.
   for ( map<UINT32, string>::iterator it = mapFiles.begin() ;
         it != mapFiles.end() ;
         ++it )
   {
      dpsFileMeta meta;
      rc = _metaFilter( it->second.c_str(), it->first, meta ) ;
      if( rc && DPS_LOG_FILE_INVALID != rc )
      {
         LogError( "Failed to parse meta data of file:[%s], rc = %d",
                   it->second.c_str(), rc ) ;
         continue ;
      }
      rc = SDB_OK ;
      _meta.metaList.push_back( meta ) ;
   }

   _meta.fileCount = _meta.metaList.size() ;
   _meta.fileBegin = 0 ;

   // find begin file
   for( i = 0 ; i < _meta.fileCount ; ++i )
   {
      const dpsFileMeta &meta = _meta.metaList[i] ;
      if( DPS_INVALID_LOG_FILE_ID == meta.logID )
      {
         continue ;
      }

      if( DPS_INVALID_LOG_FILE_ID == beginLogID ||
          DPS_FILEID_COMPARE( meta.logID, beginLogID ) < 0 )
      {
         beginLogID = meta.logID ;
         _meta.fileBegin = i ;
      }
   }

   // find work file
   _meta.fileWork = _meta.fileBegin ;
   for( i = 1 ; i < _meta.fileCount ; ++i )
   {
      id = ( _meta.fileWork + 1 ) % _meta.fileCount ;
      if ( _meta.metaList[id].logID == DPS_INVALID_LOG_FILE_ID )
      {
         break ;
      }
      ++_meta.fileWork ;
      _meta.fileWork = _meta.fileWork % _meta.fileCount ;
   }

   if( 0 < _meta.fileCount )
   {
      /// Init Size
      UINT64 validLen = 0 ;

retry:
      if ( validLen > _metaContentSize )
      {
         SDB_OSS_FREE( _metaContent ) ;
         _metaContentSize = 0 ;

         _metaContent =(CHAR*)SDB_OSS_MALLOC( validLen ) ;
         if( !_metaContent )
         {
            LogError( "Failed to allocate memory for %lld bytes",
                      validLen ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _metaContentSize = validLen ;
         ossMemset( _metaContent, 0, _metaContentSize ) ;
      }

      validLen = _dumpMeta( _meta, _metaContent, _metaContentSize ) ;
      if( validLen >= _metaContentSize )
      {
         validLen += BLOCK_SIZE ;
         goto retry ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

static BOOLEAN isNumb( string &str )
{
   for(UINT32 i = 0 ; i < str.length() ; i++)
   {
      if(str[i] > '9' || str[i] < '0')
      {
         return FALSE;
      }
   }
   return TRUE;
}

void putMatchFileIntoMap( map<UINT32, string > &mapFiles,
                          const fs::path &filePath)
{
   const static UINT32 MATCH_SIZE = strlen(REPLOG_NAME_PREFIX);
   string fileName = filePath.filename().string();
   if ( 0 == ossStrncmp( fileName.c_str(), REPLOG_NAME_PREFIX, MATCH_SIZE))
   {
      string sequenceId = fileName.substr(MATCH_SIZE);
      if (sequenceId.length() <= 0 || !isNumb(sequenceId))
      {
         return;
      }
      mapFiles.insert( pair<UINT32, string>( ossAtoi(sequenceId.c_str()),
                                             filePath.string()) );
   }
}

INT32 _dpsDumper::_getSortedFileMap( const CHAR *dirPath,
                                     map<UINT32, string > &mapFiles)
{
   INT32 rc = SDB_OK ;
   try
   {
      fs::path dbDir ( dirPath ) ;
      fs::directory_iterator end_iter ;

      if ( !fs::exists ( dbDir ) )
      {
         rc = SDB_FE ;
         goto error ;
      }
      else if ( !fs::is_directory ( dbDir ) )
      {
         putMatchFileIntoMap( mapFiles, dbDir ) ;
         goto done ;
      }

      for ( fs::directory_iterator dir_iter ( dbDir ) ;
            dir_iter != end_iter ;
            ++dir_iter )
      {
         try
         {
            if ( fs::is_regular_file ( dir_iter->status() ) )
            {
               putMatchFileIntoMap( mapFiles, dir_iter->path() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "File or dir[%s] occur exception: %s",
                    dir_iter->path().string().c_str(),
                    e.what() ) ;
            /// skip the file or dir
         }
      }
   }
   catch ( fs::filesystem_error& e )
   {
      if ( e.code().value() == boost::system::errc::permission_denied ||
           e.code().value() == boost::system::errc::operation_not_permitted )
      {
         rc = SDB_PERM ;
      }
      else if( e.code().value() == boost::system::errc::too_many_files_open ||
               e.code().value() ==
               boost::system::errc::too_many_files_open_in_system )
      {
         rc = SDB_TOO_MANY_OPEN_FD ;
      }
      else
      {
         PD_LOG( PDERROR, "Enum directory[%s] failed: %s,%d",
                 dirPath, e.what(), e.code().value() ) ;
         rc = SDB_IO ;
      }
      goto error ;
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::isDir( const CHAR *path, BOOLEAN &dir )
{
   INT32 rc = SDB_OK ;
   SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;

   rc = ossAccess( path, R_OK ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

   rc = ossGetPathType( path, &fileType ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

   dir = FALSE ;
   if( SDB_OSS_DIR == fileType )
   {
      dir =  TRUE ;
   }
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN _dpsDumper::isFileExisted( const CHAR *path )
{
   INT32 rc = SDB_OK ;
   OSSFILE file ;
   rc = ossOpen( path, OSS_READONLY, OSS_RU, file ) ;
   if( rc )
   {
      if( SDB_PERM == rc )
      {
         LogError( "Permission error, file:[%s]", path ) ;
      }
      else
      {
         LogError( "File: [%s] is not existed", path ) ;
      }
      goto done ;
   }
   ossClose( file ) ;

done:
   return SDB_OK == rc ;
}

INT32 _dpsDumper::_metaFilter( const CHAR *filename, INT32 index,
                               dpsFileMeta &meta )
{
   SDB_ASSERT( filename, "filename cannot be NULL ") ;

   INT32 rc = SDB_OK  ;

   OSSFILE in ;
   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   CHAR pLogHead[ DPS_LOG_HEAD_LEN + 1 ] = { 0 } ;
   BOOLEAN opened             = FALSE ;
   INT64 fileSize             = 0 ;
   INT64 offset               = 0 ;
   dpsLogHeader *logHeader    = NULL ;
   dpsLogRecordHeader *header = NULL ;
   INT64 len                  = 0 ;
   INT64 totalRecordSize      = 0 ;
   UINT64 preLsn              = 0 ;

   LogEvent( "Parse file:[ %s ] begin", filename ) ;
   rc = ossOpen( filename, OSS_DEFAULT | OSS_READONLY,
                 OSS_RU | OSS_RG, in ) ;
   if( rc )
   {
      LogError( "Unable to open file[%s], rc = %d", filename, rc ) ;
      goto error ;
   }
   opened = TRUE ;

   rc = _checkLogFile( in, fileSize, filename );
   if( rc )
   {
      goto error ;
   }
   SDB_ASSERT( fileSize > 0, "fileSize must be gt 0" ) ;

   rc = _readLogHead( in, offset, fileSize, NULL, 0, pLogHead, len ) ;
   if( rc && DPS_LOG_FILE_INVALID != rc )
   {
      LogEvent("Read a invlid log file: %s", filename) ;
      goto error;
   }
   // start format log head
   totalRecordSize = fileSize - DPS_LOG_HEAD_LEN ;
   logHeader = (dpsLogHeader*)pLogHead ;

   // init meta data
   meta.index     = index ;
   meta.logID     = logHeader->_logID ;
   meta.firstLSN  = logHeader->_firstLSN.offset ;
   meta.lastLSN   = logHeader->_firstLSN.offset ;
   _fileNum       = logHeader->_fileNum;

   if( DPS_LOG_INVALID_LSN != logHeader->_firstLSN.offset )
   {
      while ( offset < fileSize )
      {
         rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
         if( rc && SDB_DPS_CORRUPTED_LOG != rc )
         {
            goto error ;
         }

         header = ( dpsLogRecordHeader * )pRecordHead ;
         if( SDB_DPS_CORRUPTED_LOG == rc )
         {
            meta.expectLSN = logHeader->_firstLSN.offset +
                             offset - DPS_LOG_HEAD_LEN ;
            meta.lastLSN = preLsn ;
            meta.validSize = offset - DPS_LOG_HEAD_LEN ;
            meta.restSize = totalRecordSize - meta.validSize ;

            LogEvent( "Record was corrupted with length: %d",
                      header->_length ) ;
            rc = SDB_OK ;
            break ;
         }
         if( LOG_TYPE_DUMMY == header->_type )
         {
            // reach end of file
            meta.expectLSN = header->_lsn + header->_length ;
            meta.lastLSN = header->_lsn ;
            meta.validSize = header->_lsn % totalRecordSize ;
            meta.restSize = totalRecordSize -
                            ( meta.validSize + header->_length ) ;
            break;
         }

         // hit invalid lsn
         if( preLsn > header->_lsn )
         {
            // current lsn will to be overwrite by next record
            meta.expectLSN = logHeader->_firstLSN.offset +
                             offset - DPS_LOG_HEAD_LEN ;
            meta.lastLSN = preLsn ;
            meta.validSize = offset - DPS_LOG_HEAD_LEN ;
            meta.restSize = totalRecordSize - meta.validSize ;
            break ;
         }

         preLsn = header->_lsn ;
         offset += header->_length ;
      }
   }
   else
   {
      meta.validSize = 0 ;
      meta.restSize = totalRecordSize ;
   }

done:
   if( opened )
      ossClose( in ) ;
   LogEvent("Parse file:[ %s ] end", filename ) ;

   return rc ;

error:
   goto done ;
}


INT64 _dpsDumper::_dumpMeta( const dpsMetaData& meta,
                             CHAR* pBuffer,
                             const UINT64 bufferSize )
{
   SDB_ASSERT( pBuffer, "pOutBuffer cannot be NULL " ) ;
   UINT64 len = 0 ;
   UINT32 fileIndex = 0;
   UINT32 lastIndex = 0;
   UINT32 lastFileId = 0;

   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "======================================="OSS_NEWLINE
                       ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    Log Files in total: %d"OSS_NEWLINE,
                       meta.fileCount ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    LogFile begin     : sequoiadbLog.%d"OSS_NEWLINE,
                       meta.metaList[_meta.fileBegin].index ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "    LogFile work      : sequoiadbLog.%d"OSS_NEWLINE,
                       meta.metaList[ meta.fileWork ].index ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        begin Lsn     : 0x%08lx"OSS_NEWLINE,
                       meta.metaList[ meta.fileBegin].firstLSN ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        current Lsn   : 0x%08lx"OSS_NEWLINE,
                       meta.metaList[ meta.fileWork ].lastLSN ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "        expect Lsn    : 0x%08lx"OSS_NEWLINE,
                       ( meta.metaList[ meta.fileWork ].expectLSN ) ) ;
   len += ossSnprintf( pBuffer + len, bufferSize - len,
                       "======================================="OSS_NEWLINE
                       ) ;

   // print lost file name
   fileIndex = _meta.fileBegin ;
   lastIndex = fileIndex ;
   lastFileId = meta.metaList[lastIndex].index;
   for (UINT32 i = 0; i < meta.metaList.size(); ++i)
   {
      const dpsFileMeta& fMeta = meta.metaList[ fileIndex ] ;

      if (lastFileId <= fMeta.index)
      {
         for( lastFileId++ ; lastFileId < fMeta.index ; lastFileId++ )
         {
            len += ossSnprintf( pBuffer + len, bufferSize - len,
                                OSS_NEWLINE"ERROR: Log File Name "
                                "(sequoiadbLog.%d) is Missing"OSS_NEWLINE,
                                lastFileId ) ;
         }
      }
      else
      {
         UINT32 lostFileNum = fMeta.index + _fileNum - lastFileId -1;

         for ( UINT32 j = 0; j < lostFileNum; j++ )
         {
            len += ossSnprintf( pBuffer + len, bufferSize - len,
                                OSS_NEWLINE"ERROR: Log File Name "
                                "(sequoiadbLog.%d) is Missing"OSS_NEWLINE,
                                lastFileId + 1 + j) ;
         }
      }

      // foreach file meta
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          OSS_NEWLINE"Log File Name: sequoiadbLog.%d"
                          OSS_NEWLINE,
                          fMeta.index) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Logic ID     : %d"OSS_NEWLINE, fMeta.logID ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "First LSN    : 0x%08lx"OSS_NEWLINE,
                          fMeta.firstLSN ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Last  LSN    : 0x%08lx"OSS_NEWLINE,
                          fMeta.lastLSN ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Valid Size   : %lld bytes"OSS_NEWLINE,
                          fMeta.validSize ) ;
      len += ossSnprintf( pBuffer + len, bufferSize - len,
                          "Rest Size    : %lld bytes"OSS_NEWLINE,
                          fMeta.restSize ) ;
      lastIndex = fileIndex;
      lastFileId = fMeta.index;
      ++fileIndex;
      fileIndex = fileIndex % meta.metaList.size();

   }

   return len ;
}

INT32 _dpsDumper::_checkLogFile( OSSFILE& file,
                                 INT64& size,
                                 const CHAR *filename )
{
   SDB_ASSERT( filename, "filename cannot null" ) ;

   INT32 rc = SDB_OK ;
   // calculate file size
   rc = ossGetFileSize( &file, &size ) ;
   if( rc )
   {
      LogError( "Failed to get file size: %s, rc = %d", filename, rc ) ;
      goto error ;
   }
   // make sure the size valid
   if( size < DPS_LOG_HEAD_LEN )
   {
      LogError( "Log file %s is %lld bytes, "
                "which is smaller than log file head",
                filename, size ) ;
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

   //log file must be multiple of page size
   if(( size - DPS_LOG_HEAD_LEN ) % DPS_DEFAULT_PAGE_SIZE != 0 )
   {
      LogError( "Log file %s is %lld bytes, "
                "which is not aligned with page size",
                filename, size ) ;
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

done:
   return rc;
error:
   goto done;
}

INT32 _dpsDumper::_readRecordHead( OSSFILE& in,
                                   const INT64 offset,
                                   const INT64 fileSize,
                                   CHAR *pRecordHeadBuffer )
{
   SDB_ASSERT( offset < fileSize, "offset out of range " ) ;
   SDB_ASSERT( pRecordHeadBuffer, "OutBuffer cannot be NULL " ) ;

   INT32 rc       = SDB_OK ;
   INT64 readPos  = 0 ;
   INT64 restLen  = sizeof( dpsLogRecordHeader ) ;
   INT64 fileRead = 0 ;
   dpsLogRecordHeader *header = NULL ;

   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, pRecordHeadBuffer + readPos,
                           restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes, "
                   "actual read %lld bytes, rc = %d",
                   restLen, fileRead, rc ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

   header = ( dpsLogRecordHeader * )pRecordHeadBuffer ;
   if ( header->_length < sizeof( dpsLogRecordHeader) ||
        header->_length > DPS_RECORD_MAX_LEN )
   {
      rc = SDB_DPS_CORRUPTED_LOG ;
      goto error ;
   }

   if ( LOG_TYPE_DUMMY == header->_type )
   {
      LogEvent( "Reach the Dummy record head, offset:%lld", offset ) ;
      goto done ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_readRecord( OSSFILE& in,
                               const INT64 offset,
                               const INT64 fileSize,
                               const INT64 readLen,
                               CHAR *pOutBuffer )
{
   INT32 rc = SDB_OK ;

   SDB_ASSERT( offset < fileSize, "offset out of range " ) ;
   SDB_ASSERT( readLen > 0, "readLen lt 0!!" ) ;
   INT64 restLen  = 0 ;
   INT64 readPos  = 0 ;
   INT64 fileRead = 0;

   restLen = readLen ;
   while( restLen > 0 )
   {
      rc = ossSeekAndRead( &in, offset, pOutBuffer + readPos,
                           restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes, "
                   "actual read %lld bytes, rc = %d",
                   restLen, fileRead, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_readLogHead( OSSFILE &in,
                                INT64& offset,
                                const INT64 fileSize,
                                CHAR *pOutBuffer,
                                const INT64 bufferSize,
                                CHAR *pOutHeader,
                                INT64& outLen )
{
   SDB_ASSERT( fileSize > DPS_LOG_HEAD_LEN,
               "fileSize must gt than DPS_LOG_HEAD_LEN" ) ;

   INT32 rc       = SDB_OK ;
   INT64 readPos  = 0 ;
   INT64 fileRead = 0 ;
   INT64 restLen  = DPS_LOG_HEAD_LEN ;
   CHAR  pBuffer[ DPS_LOG_HEAD_LEN + 1 ] = { 0 } ;
   dpsLogHeader *header = NULL ;
   INT64 len      = 0 ;

   while( restLen > 0 )
   {
      rc = ossRead( &in, pBuffer + readPos, restLen, &fileRead ) ;
      if( rc && SDB_INTERRUPT != rc)
      {
         LogError( "Failed to read from file, expect %lld bytes,  "
                   "actual read %lld bytes, rc = %d",
                   fileSize, fileRead, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= fileRead ;
      readPos += fileRead ;
   }

   header =( _dpsLogHeader* )pBuffer ;
   if( DPS_INVALID_LOG_FILE_ID != header->_logID )
   {
      UINT64 beginOffset = header->_firstLSN.offset ;
      beginOffset = beginOffset %( fileSize - DPS_LOG_HEAD_LEN ) ;
      offset += beginOffset ;
   }
   else
   {
      rc = DPS_LOG_FILE_INVALID ;
   }
   // modify offset
   offset += DPS_LOG_HEAD_LEN ;

   if( pOutBuffer )
   {
      len = engine::dpsDump::dumpLogFileHead( pBuffer, DPS_LOG_HEAD_LEN,
                                              pOutBuffer, bufferSize,
                                              DPS_DMP_OPT_HEX |
                                              DPS_DMP_OPT_HEX_WITH_ASCII |
                                              DPS_DMP_OPT_FORMATTED ) ;
   }

   if ( pOutHeader )
   {
      ossMemcpy( pOutHeader, pBuffer, DPS_LOG_HEAD_LEN );
   }

done:
   outLen = len ;
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_writeTo( OSSFILE &file,
                            const CHAR* pContent,
                            BOOLEAN console )
{
   const static UINT32 FILE_MAX_SIZE = 500 * 1024 * 1024;
   static UINT32 curFileSize = 0;

   INT32 rc        = SDB_OK ;

   ///< write buffer
   if ( console )
   {
      std::cout << pContent << endl;
      return rc;
   }

   UINT32 bufSize = ossStrlen( pContent ) ;
   if (bufSize + curFileSize > FILE_MAX_SIZE)
   {
      ossClose( file ) ;
      rc = _changeFileName();
      if ( rc != SDB_OK)
      {
         return rc;
      }

      rc = ossOpen ( dstPath, OSS_REPLACE | OSS_WRITEONLY,
                  OSS_RU|OSS_WU|OSS_RG, file ) ;
      if ( rc )
      {
         LogError ( "Error: Failed to open output file: %s, rc = %d"
                  OSS_NEWLINE, dstPath, rc ) ;
         return rc;
      }
      curFileSize = 0;
   }

   rc = ossWriteN( &file, pContent, bufSize) ;
   if (SDB_OK != rc)
   {
      LogError( "Failed to write data to file, data: %s", pContent ) ;
      goto error ;
   }
   curFileSize += bufSize;

   if (curFileSize < FILE_MAX_SIZE)
   {
      // write a enter into file
      rc = ossWriteN( &file, OSS_NEWLINE, OSS_NEWLINE_SIZE ) ;
      if ( SDB_OK != rc)
      {
         LogError( "Failed to write data to file, data: %s", pContent ) ;
         goto error ;
      }
      curFileSize += OSS_NEWLINE_SIZE ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::filte( dpsDumpFilter *filter,
                         OSSFILE &out,
                         const CHAR *filename )
{
   SDB_ASSERT( filter, "filter is NULL" ) ;
   SDB_ASSERT( filename, "filename cannot be NULL" ) ;

   INT32 rc = SDB_OK ;

   OSSFILE in ;
   CHAR pLogHead[ sizeof( dpsLogHeader ) + 1 ] = { 0 } ;
   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   CHAR *pRecordBuffer = NULL ;
   INT64 recordLength  = 0 ;
   INT32 ahead         = lsnAhead ;
   INT32 back          = lsnBack ;
   INT32 totalCount    = NONE_LSN_FILTER ; //ahead + back ;

   BOOLEAN opened      = FALSE ;
   INT64 fileSize      = 0 ;
   INT64 offset        = 0 ;
   CHAR *pOutBuffer    = NULL ; ///< buffer for formatted log
   INT64 outBufferSize = 0 ;
   INT64 len           = 0 ;
   BOOLEAN printLogHead= FALSE ;

   CHAR parseBegin[ BLOCK_SIZE ] = { 0 } ;
   len  = ossSnprintf( parseBegin, BLOCK_SIZE, OSS_NEWLINE""OSS_NEWLINE ) ;
   len += ossSnprintf( parseBegin + len, BLOCK_SIZE - len,
                       "parse file : [%s]"OSS_NEWLINE, filename ) ;

   rc = ossOpen( filename, OSS_DEFAULT | OSS_READONLY,
                 OSS_RU | OSS_WU | OSS_RG, in ) ;
   if( rc )
   {
      LogError( "Unable to open file: %s. rc = %d", filename, rc ) ;
      goto error ;
   }

   opened = TRUE ;

   rc = _checkLogFile( in, fileSize, filename ) ;
   if( rc )
   {
      goto error ;
   }
   SDB_ASSERT( fileSize > 0, "fileSize must be gt 0" ) ;
   len = DPS_LOG_HEAD_LEN * LOG_BUFFER_FORMAT_MULTIPLIER ;

retry_head:
   // format log head
   if( len > outBufferSize )
   {
      CHAR *pOrgBuff = pOutBuffer ;
      pOutBuffer =(CHAR*)SDB_OSS_REALLOC ( pOutBuffer, len + 1 ) ;
      if( !pOutBuffer )
      {
         LogError( "Failed to allocate memory for %lld bytes", len + 1 ) ;
         pOutBuffer = pOrgBuff ;
         rc = SDB_OOM ;
         goto error ;
      }
      outBufferSize = len ;
      ossMemset( pOutBuffer, 0, len + 1 ) ;
   }

   rc = _readLogHead( in, offset, fileSize,  pOutBuffer, outBufferSize,
                      pLogHead, len ) ;
   if( rc && DPS_LOG_FILE_INVALID != rc )
   {
      goto error ;
   }
   rc = SDB_OK ;
   if( len >= outBufferSize )
   {
      len += BLOCK_SIZE ;
      goto retry_head ;
   }

   // lsn must be done specially
   if( SDB_LOG_FILTER_LSN == filter->getType() )
   {
      dpsLogHeader *logHeader = ( dpsLogHeader * )pLogHead ;
      if(   ( logHeader->_firstLSN.offset > lsn )
         || (   lsn >= logHeader->_firstLSN.offset
              + fileSize - DPS_LOG_HEAD_LEN ) )
      {
         goto done ;
      }

      offset = DPS_LOG_HEAD_LEN + (lsn % ( fileSize - DPS_LOG_HEAD_LEN )) ;
      // seek to the log by lsn assigned
      {
         rc = _seekToLsnMatched( in, offset, fileSize, ahead ) ;

         if( rc && DPS_LOG_REACH_HEAD != rc )
         {
            LogError( "hit a invalid or corrupted log: %lld in file : [%s] "
                      "is invalid", lsn, filename ) ;
            goto error ;
         }
         totalCount = ahead + back + 1 ;
         if( DPS_LOG_REACH_HEAD == rc )
         {
            offset = DPS_LOG_HEAD_LEN ;
         }
      }
   }
   else if( SDB_LOG_FILTER_LAST == filter->getType() )
   {
      INT32 recordCount = lastCount - 1 ;
      offset = DPS_LOG_HEAD_LEN ;
      _seekToEnd( in, offset, fileSize ) ;
      rc = _seekToLsnMatched( in, offset, fileSize, recordCount ) ;
      if( rc && DPS_LOG_REACH_HEAD != rc )
      {
         //printf( "File was corrupted.\n" ) ;
         goto error ;
      }
      totalCount = recordCount + 1 ;
      if( DPS_LOG_REACH_HEAD == rc )
      {
         offset = DPS_LOG_HEAD_LEN ;
      }
   }

   // then dump each record
   while(   offset < fileSize &&
          ( NONE_LSN_FILTER  == totalCount || totalCount > 0 ) )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      if( rc && SDB_DPS_CORRUPTED_LOG != rc )
      {
         goto error ;
      }

      dpsLogRecordHeader *header = ( dpsLogRecordHeader *)pRecordHead ;
      if( SDB_DPS_CORRUPTED_LOG == rc && header->_length == 0 )
      {
         goto error ;
      }

      if( header->_length > recordLength )
      {
         CHAR *pNewBuf = ( CHAR * )SDB_OSS_REALLOC ( pRecordBuffer,
                                                     header->_length + 1 );
         if( !pNewBuf )
         {
            rc = SDB_OOM;
            LogError( "Failed to allocate %d bytes", header->_length + 1 ) ;
            goto error ;
         }
         pRecordBuffer = pNewBuf ;
         recordLength = header->_length ;
      }
      ossMemset( pRecordBuffer, 0, recordLength ) ;
      rc = _readRecord( in, offset, fileSize, header->_length, pRecordBuffer ) ;
      if( rc )
      {
         goto error;
      }

      // filte name
      if( !filter->match( this, pRecordBuffer ) )
      {
         offset += header->_length ;
         if( SDB_LOG_FILTER_LSN == filter->getType() )
         {
            --totalCount ;
         }
         continue ;
      }

      // find first record, then print log head
      if( !printLogHead )
      {
         printLogHead = TRUE ;
         rc = _writeTo( out, parseBegin, consolePrint ) ;
         if( rc )
         {
            goto error ;
         }
         // write to file
         rc = _writeTo( out, pOutBuffer, consolePrint ) ;
         if( rc )
         {
            goto error ;
         }
      }

      // dump log
      dpsLogRecord record ;
      record.load( pRecordBuffer ) ;
      len = recordLength * LOG_BUFFER_FORMAT_MULTIPLIER ;

retry_record:
      if( len > outBufferSize )
      {
         CHAR *pOrgBuff = pOutBuffer ;
         pOutBuffer =(CHAR*)SDB_OSS_REALLOC( pOutBuffer, len + 1 ) ;
         if( !pOutBuffer )
         {
            LogError( "Failed to allocate memory for %lld bytes", len + 1 ) ;
            pOutBuffer = pOrgBuff ;
            rc = SDB_OOM ;
            goto error ;
         }
         outBufferSize = len ;
         ossMemset( pOutBuffer, 0, len + 1 ) ;
      }

      len = record.dump( pOutBuffer, outBufferSize,
                         DPS_DMP_OPT_HEX | DPS_DMP_OPT_HEX_WITH_ASCII |
                         DPS_DMP_OPT_FORMATTED ) ;
      if( len >= outBufferSize )
      {
         len += BLOCK_SIZE ;
         goto retry_record ;
      }


      // write dump data
      rc = _writeTo( out, pOutBuffer, consolePrint ) ;
      if( rc )
      {
         goto error ;
      }

      offset += header->_length ;
      if(    ( SDB_LOG_FILTER_LSN  == filter->getType() )
          || ( SDB_LOG_FILTER_LAST == filter->getType() ) )
      {
         --totalCount ;
      }
   }

done:
   if( pRecordBuffer )
      SDB_OSS_FREE( pRecordBuffer ) ;
   if( pOutBuffer )
      SDB_OSS_FREE( pOutBuffer ) ;
   if( opened )
      ossClose( in ) ;
   return rc ;

error:
   goto done ;
}

INT32 _dpsDumper::_seekToLsnMatched( OSSFILE &in,
                                     INT64 &offset,
                                     const INT64 fileSize,
                                     INT32 &prevCount )
{
   SDB_ASSERT( offset >= DPS_LOG_HEAD_LEN, "offset lt DPS_LOG_HEAD_LEN" ) ;
   INT32 rc     = SDB_OK ;
   INT64 newOff = 0 ;
   INT32 count  = prevCount ;

   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   dpsLogRecordHeader *header = NULL ;

   if( offset > fileSize || offset < DPS_LOG_HEAD_LEN )
   {
      LogError( "wrong LSN position: %lld", offset ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if( count < 0 )
   {
      LogError( "pre-count must be gt 0, current is: %d", prevCount ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   prevCount = 0 ;
   while( offset < fileSize && prevCount < count )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      // ignore the return value
      header = ( dpsLogRecordHeader * )pRecordHead ;
      if ( header->_length < sizeof( dpsLogRecordHeader ) ||
           header->_length > DPS_RECORD_MAX_LEN )
      {
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( LOG_TYPE_DUMMY == header->_type )
      {
         LogError( "wrong lsn: %lld input", lsn ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      newOff = header->_lsn % ( fileSize - DPS_LOG_HEAD_LEN ) ;
      if( 0 == newOff || DPS_LOG_INVALID_LSN ==  header->_preLsn )
      {
         rc = DPS_LOG_REACH_HEAD;
         goto done ;
      }
      offset = DPS_LOG_HEAD_LEN +
               header->_preLsn % ( fileSize - DPS_LOG_HEAD_LEN ) ;
      ++prevCount ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _dpsDumper::_seekToEnd( OSSFILE &in,
                              INT64 &offset,
                              const INT64 fileSize )
{
   SDB_ASSERT( offset >= DPS_LOG_HEAD_LEN, "offset lt DPS_LOG_HEAD_LEN" ) ;
   INT32 rc    = SDB_OK ;
   INT64 prevOffset = offset ;

   CHAR pRecordHead[ sizeof( dpsLogRecordHeader ) + 1 ] = { 0 } ;
   dpsLogRecordHeader *header = NULL;

   if( offset > fileSize || offset < DPS_LOG_HEAD_LEN )
   {
      LogError( "Wrong LSN position: %lld", offset ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   while( offset < fileSize )
   {
      rc = _readRecordHead( in, offset, fileSize, pRecordHead ) ;
      // ignore the return value
      header = ( dpsLogRecordHeader * )pRecordHead ;
      if ( header->_length < sizeof( dpsLogRecordHeader ) ||
           header->_length > DPS_RECORD_MAX_LEN )
      {
         rc = SDB_DPS_CORRUPTED_LOG ;
         goto error ;
      }

      if ( LOG_TYPE_DUMMY == header->_type )
      {
         break ;
      }

      prevOffset = offset ;
      offset += header->_length ;
   }

done:
   offset = prevOffset ;
   return rc ;
error:
   goto done ;
}


//////////////////////////////////////////////////////////////////////////
// main entry
INT32 main( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   po::options_description desc( "Command options" ) ;
   po::variables_map vm ;
   dpsDumper dumper ;
   BOOLEAN logOpened = FALSE ;

   rc = ossOpen( LOG_FILE_NAME, OSS_REPLACE|OSS_READWRITE,
                 OSS_RU|OSS_WU|OSS_RG|OSS_RO, logFile ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "Failed to open the file "LOG_FILE_NAME ;
      if ( SDB_PERM == rc )
      {
         cout << " : permission error" ;
      }
      cout << endl ;
      goto error ;
   }
   logOpened = TRUE ;

   rc = dumper.initialize( argc, argv, desc, vm ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = dumper.prepare( vm );
   if ( SDB_OK != rc )
   {
      dumper.displayArgs( desc );
      goto error;
   }

   rc = dumper.process( desc, vm ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }

done:
   if ( logOpened )
   {
      ossClose( logFile ) ;
   }
   if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
   {
      rc = SDB_OK ;
   }
   return engine::utilRC2ShellRC( rc ) ;
error :
   goto done  ;
}
