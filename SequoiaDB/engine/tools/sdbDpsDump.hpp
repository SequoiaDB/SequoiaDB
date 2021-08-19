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
#ifndef _SDB_TOOL_DPS_DUMP_HPP_
#define _SDB_TOOL_DPS_DUMP_HPP_

#include "oss.hpp"
#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <vector>
#include "pmdOptionsMgr.hpp"
#include "dpsLogFile.hpp"

#define DPS_DUMP_HELP      "help"
#define DPS_DUMP_VER       "version"
#define DPS_DUMP_TYPE      "type"
#define DPS_DUMP_NAME      "name"
#define DPS_DUMP_META      "meta"
#define DPS_DUMP_LSN       "lsn"
#define DPS_DUMP_SOURCE    "source"
#define DPS_DUMP_OUTPUT    "output"
#define DPS_DUMP_LSN_AHEAD "ahead"
#define DPS_DUMP_LSN_BACK  "back"
#define DPS_DUMP_LAST      "last"
#define DPS_DUMP_REPAIRE   "repaire"
#define DPS_DUMP_HELPFULL  "helpfull"

#define DPS_FILTER_ADD_OPTIONS_BEGIN( desc ) desc.add_options()
#define DPS_FILTER_ADD_OPTIONS_END ;
#define DPS_FILTER_COMMANDS_STRING( a, b ) \
   ( std::string( a ) + std::string( b ) ).c_str()

#define FILTER_OPTIONS \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_HELP,      ",h" ), "help" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_VER,       ",v" ), "show version" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_META,      ",m" ), "show meta info of logs" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_TYPE,      ",t" ), boost::program_options::value< INT32 >(), "specify the record type" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_NAME,      ",n" ), boost::program_options::value< std::string >(), "specify the name of collectionspace/collections" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_LSN,       ",l" ), boost::program_options::value< std::string >(), "specify the lsn, -a/-b may help" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_LAST,      ",e" ), boost::program_options::value< INT32 >(), "specify the number of last records of file to display ")\
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_SOURCE,    ",s" ), boost::program_options::value<std::string>(), "specify source log file or path, default: current path" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_OUTPUT,    ",o" ), boost::program_options::value< std::string >(), "specify output file or path, default: screen output" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_LSN_AHEAD, ",a" ), boost::program_options::value< INT32 >(), "specify the number of records to display before the lsn specified by -l/--lsn" ) \
   ( DPS_FILTER_COMMANDS_STRING( DPS_DUMP_LSN_BACK,  ",b" ), boost::program_options::value< INT32 >(), "specify the number of records to display after the lsn specified by -l/--lsn" )

#define DPS_DUMP_REPAIRE_DESP \
   "repaire the dpsLogFile header info, like \n--repaire header:LogID=1,FirstLSNVer=1200\n"\
   "-header support key:\n"\
   "  LogID(u64)   FirstLSNVer(u32)   FirstLSNOffset(u64)\n"\

//hidden options
#define DPS_DUMP_HIDDEN_OPTIONS \
   ( DPS_DUMP_HELPFULL, "help all configs" ) \
   ( DPS_FILTER_COMMANDS_STRING(DPS_DUMP_REPAIRE, ",r"), boost::program_options::value<string>(), DPS_DUMP_REPAIRE_DESP )

struct _dpsFileMeta : public SDBObject
{
   UINT32 index ;
   UINT32 logID ;
   INT64  validSize ;
   INT64  restSize ;
   UINT64 firstLSN ;
   UINT64 lastLSN ;
   UINT64 expectLSN ;

   _dpsFileMeta() : index(0), logID(DPS_INVALID_LOG_FILE_ID), validSize(0),
      restSize(0), firstLSN(0), lastLSN(0), expectLSN(0) {}
   ~_dpsFileMeta() {}
};
typedef _dpsFileMeta dpsFileMeta ;

struct _dpsCmdData : public SDBObject
{
   UINT16  type ;
   INT32   lsnAhead ;
   INT32   lsnBack ;
   INT32   lastCount ;
   BOOLEAN consolePrint ;
   UINT64  lsn ;
   CHAR   *inputName ;
   CHAR   *srcPath ;
   CHAR   *dstPath ;
} ;
typedef _dpsCmdData dpsCmdData ;

#define MAKE_CMD_DATA( DATA, TYPE, AHEAD, BACK, LAST,       \
                       CONSOLE, LSN, INPUTNAME, SRC, DST)   \
{                                                           \
   DATA.type = TYPE ;                                       \
   DATA.lsnAhead = AHEAD ;                                  \
   DATA.lsnBack  = BACK ;                                   \
   DATA.lastCount = LAST ;                                  \
   DATA.consolePrint = CONSOLE ;                            \
   DATA.lsn = LSN ;                                         \
   DATA.inputName = INPUTNAME ;                             \
   DATA.srcPath = SRC ;                                     \
   DATA.dstPath = DST ;                                     \
}


#define DPS_LOG_REACH_HEAD 1
#define DPS_LOG_FILE_INVALID 2
#define DPS_LOG_INVALID_LSN 0xffffffffffffffff
#define LOG_BUFFER_FORMAT_MULTIPLIER 10
#define NONE_LSN_FILTER -1
#define BLOCK_SIZE 64 * 1024

struct _dpsMetaData : SDBObject
{
   UINT32 fileBegin ;
   UINT32 fileWork ;
   UINT32 fileCount ;
   std::vector<dpsFileMeta> metaList ;
   _dpsMetaData() : fileBegin( 0 ), fileWork( 0 ), fileCount( 0 )
   {}
   ~_dpsMetaData() {}
} ;
typedef _dpsMetaData dpsMetaData ;

enum _dumpType
{
   SDB_DUMP_INVALID = 0,
   SDB_DUMP_NONE,            // dump all
   SDB_DUMP_TYPE,            // dump logs of specified type
   SDB_DUMP_NAME,            // dump logs belongs to specified name
   SDB_DUMP_LSN,             // dump logs around specified lsn
   SDB_DUMP_META,            // dump meta data of log files
   SDB_DUMP_LAST,            // dump last logs of specified file
} ;
typedef _dumpType dumpType ;

#define ASSIGNED_FILTER( filter, nextFilter )   \
do                                              \
{                                               \
   if( NULL == filter )                         \
      filter = nextFilter ;                     \
   else                                         \
      filter->append( nextFilter ) ;            \
} while ( FALSE )

#define CHECK_FILTER( filter )                                 \
do                                                             \
{                                                              \
   if ( NULL == filter )                                       \
   {                                                           \
      std::cout << "Failed to allocate filter" << std::endl ;  \
      goto error;                                              \
   }                                                           \
} while ( FALSE )

enum SDB_DPS_LOG_FILTER_TYPE
{
   SDB_LOG_FILTER_INVALID = 0,
   SDB_LOG_FILTER_NONE,
   SDB_LOG_FILTER_TYPE,
   SDB_LOG_FILTER_NAME,
   SDB_LOG_FILTER_LSN,
   SDB_LOG_FILTER_META,
   SDB_LOG_FILTER_LAST,
} ;

class _dpsDumper;
class _dpsDumpFilter : public SDBObject
{
public:
   _dpsDumpFilter( SDB_DPS_LOG_FILTER_TYPE type ) : _nextFilter( NULL ),
      _type( type )
   {}

   virtual ~_dpsDumpFilter()
   {
      _nextFilter = NULL ;
   }

   virtual INT32 doFilte( _dpsDumper *dumper, OSSFILE& out,
                          const CHAR* logFilePath ) = 0 ;

   virtual BOOLEAN match( _dpsDumper *dumper, CHAR *pRecord )
   {
      BOOLEAN rc = TRUE ;
      if( NULL != _nextFilter )
      {
         rc = _nextFilter->match( dumper, pRecord ) ;
         goto done;
      }
   done:
      return rc ;
   }

   void append( _dpsDumpFilter *filter )
   {
      SDB_ASSERT( NULL == _nextFilter, "_nextFilter must be NULL" ) ;
      if( _nextFilter )
      {
         filter->append( _nextFilter ) ;
      }
      _nextFilter = filter ;
   }

   SDB_DPS_LOG_FILTER_TYPE getType() const
   {
      return _type ;
   }

protected:
   _dpsDumpFilter *_nextFilter ;
   SDB_DPS_LOG_FILTER_TYPE _type ;

} ;
typedef _dpsDumpFilter dpsDumpFilter ;


// declare filter by DECLARE_FILTER
#define DECLARE_FILTER( filterName, alias, typeIndex )                  \
class filterName : public dpsDumpFilter                                 \
{                                                                       \
public:                                                                 \
   filterName() : dpsDumpFilter( typeIndex ) {}                         \
   virtual  ~filterName() {}                                            \
                                                                        \
   virtual BOOLEAN match( _dpsDumper *dumper, CHAR *pRecord ) ;         \
   virtual INT32 doFilte( _dpsDumper *dumper, OSSFILE &out,             \
                          const CHAR *logFilePath ) ;                   \
                                                                        \
} ;                                                                     \
typedef filterName alias ;

#define FILTER_DEFINITION( TYPE, TYPE_INDEX ) \
   DECLARE_FILTER( _dps##TYPE##Filter, dps##TYPE##Filter, TYPE_INDEX )

FILTER_DEFINITION( Type, SDB_LOG_FILTER_TYPE )
FILTER_DEFINITION( Name, SDB_LOG_FILTER_NAME )
FILTER_DEFINITION( Lsn,  SDB_LOG_FILTER_LSN  )
FILTER_DEFINITION( None, SDB_LOG_FILTER_NONE )
FILTER_DEFINITION( Last, SDB_LOG_FILTER_LAST )
FILTER_DEFINITION( Meta, SDB_LOG_FILTER_META )

class _dpsFilterFactory
{
public:
   static _dpsFilterFactory* getInstance() ;

   dpsDumpFilter* createFilter( int type ) ;

   void release( dpsDumpFilter *filter ) ;

private:
   _dpsFilterFactory()  ;
   ~_dpsFilterFactory() ;

   std::list< dpsDumpFilter *> _filterList ;
} ;
typedef _dpsFilterFactory dpsFilterFactory;

class _dpsDumper : public engine::_pmdCfgRecord
{
public:
   _dpsDumper();
   ~_dpsDumper();

   void displayArgs( const po::options_description &desc )
   {
      std::cout << desc << std::endl ;
   }

   INT32 initialize( INT32 argc,
                     char** agrv,
                     po::options_description &desc,
                     po::variables_map &vm ) ;

   INT32 prepare( po::variables_map &vm );

   INT32 process( const po::options_description &desc,
                  const po::variables_map &vm ) ;

   INT32 dump();

   INT32 parseMeta( CHAR *buffer );

   INT32 filte( dpsDumpFilter *filter, OSSFILE& out, const CHAR *filename ) ;

   INT32 repaireHeader( const std::string &str ) ;

public:
   virtual INT32 doDataExchange( engine::pmdCfgExchange *pEx ) ;
   virtual INT32 postLoaded( engine::PMD_CFG_STEP step ) ;
   virtual INT32 preSaving() ;

public:
   static INT32   getFileCount( const CHAR *path, INT32 &fileCount );
   static INT32   isDir( const CHAR *path, BOOLEAN &dir ) ;
   static BOOLEAN isFileExisted( const CHAR *path );
   static INT32   sortFiles(dpsMetaData& meta);
   static INT32   toFile( OSSFILE& out, const CHAR *buffer);

private:

   BOOLEAN     _validCheck( const po::variables_map &vm ) ;

   INT32       _analysisMeta(map<UINT32, string > &mapFiles) ;

   INT32       _metaFilter( const CHAR *filename, INT32 index,
                            dpsFileMeta& meta ) ;
   INT64       _dumpMeta( const dpsMetaData& meta,
                          CHAR* pBuffer, const UINT64 bufferSize ) ;
   INT32       _changeFileName();
   INT32       _getSortedFileMap( const CHAR *dirPath, map<UINT32, string> &mapFiles);

private:
   INT32   _checkLogFile( OSSFILE &file, INT64 &size, const CHAR *filename );
   INT32   _readLogHead( OSSFILE &in, INT64 &offset, const INT64 fileSize,
                         CHAR *pOutBuffer, const INT64 bufferSize,
                         CHAR *pOutHeader, INT64 &outLen ) ;
   INT32   _readRecordHead( OSSFILE &in, const INT64 offset,
                            const INT64 fileSize, CHAR *pRecordHeadBuffer ) ;
   INT32   _readRecord( OSSFILE &in, const INT64 offset,
                        const INT64 fileSize, const INT64 readLen,
                        CHAR *pOutBuffer ) ;

   INT32 _writeTo( OSSFILE &fd, const CHAR* pContent, BOOLEAN console = FALSE) ;

   INT32 _seekToEnd( OSSFILE &in, INT64 &offset, const INT64 fileSize ) ;
   INT32 _seekToLsnMatched( OSSFILE &in, INT64 &offset,
                            const INT64 fileSize, INT32 &prevCount ) ;
public:
   UINT16   opType ;
   INT32    lsnAhead ;
   INT32    lsnBack ;
   INT32    lastCount ;
   BOOLEAN  consolePrint ;
   UINT64   lsn ;
   CHAR     name[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR     srcPath[ OSS_MAX_PATHSIZE + 1 ] ;
   CHAR     dstPath[ OSS_MAX_PATHSIZE + 1 ] ;
   UINT32   dstPathLen;
private:
   CHAR    *_metaContent;
   UINT64   _metaContentSize ;
   dpsDumpFilter *_filter ;
   dpsMetaData _meta ;
   UINT32 _fileNum;
};
typedef _dpsDumper dpsDumper ;

#endif
