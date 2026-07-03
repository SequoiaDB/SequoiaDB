/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sdbinspt.cpp

   Descriptive Name = SequoiaDB Inspect

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data dump and integrity check.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsInspect.hpp"
#include "dmsDump.hpp"
#include "ossUtil.hpp"
#include "ossIO.hpp"
#include "rtn.hpp"
#include "pmdEDU.hpp"
#include "ixmExtent.hpp"
#include "ossVer.h"
#include "pmdOptionsMgr.hpp"
#include "utilLZWDictionary.hpp"
#include "ossMmap.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <stdarg.h>

using namespace std ;
using namespace engine ;
using namespace bson ;
namespace po = boost::program_options;
namespace fs = boost::filesystem ;

#define BUFFERSIZE          256
#define OPTION_HELP         "help"
#define OPTION_VERSION      "version"
#define OPTION_DBPATH       "dbpath"
#define OPTION_INDEXPATH    "indexpath"
#define OPTION_LOBMPATH     "lobmpath"
#define OPTION_LOBPATH      "lobpath"
#define OPTION_OUTPUT       "output"
#define OPTION_MAX_FILESZ   "maxfilesize"
#define OPTION_VERBOSE      "verbose"
#define OPTION_HEX          "hex"
#define OPTION_CSNAME       "csname"
#define OPTION_CLNAME       "clname"
#define OPTION_ACTION       "action"
#define OPTION_DUMPDATA     "dumpdata"
#define OPTION_DUMPINDEX    "dumpindex"
#define OPTION_DUMPLOB      "dumplob"
#define OPTION_PAGESTART    "pagestart"
#define OPTION_NUMPAGE      "numpage"
#define OPTION_SHOW_CONTENT "record"
#define OPTION_ONLY_META    "meta"
#define OPTION_CRCCHECK     "crccheck"
#define OPTION_JUDGE_BALANCE "balance"
#define OPTION_REPAIRE      "repaire"
#define OPTION_RECOVER      "recover"
#define OPTION_RECOVER_MBID "recover-with-mbid"
#define OPTION_FORCE        "force"
#define OPTION_HELPFULL     "helpfull"


#define OPTION_REPAIRE_DESP \
   "repaire the db info, like --repaire mb:Flag=0,Attr=1\n"\
   "-mb support key:\n"\
   "  IndexPages(u)      LID(u)            Attr(u)\n"\
   "  IndexFreeSpace(u)  DataPages(u)      Flag(u)\n"\
   "  DataFreeSpace(u)   LobPages(u)       Records(u)\n"\
   "  IndexNum(u)        CompressType(u)   Lobs(u)\n"\
   "  CommitFlag(u)      CommitLSN(u64)\n"\
   "  IdxCommitFlag(u)   IdxCommitLSN(u64)\n"\
   "  LobCommitFlag(u)   LobCommitLSN(u64)\n"\
   "  FirstExtentID(u)   LastExtentID(u)\n"\
   "  MBExtExtentID(u)   DictExtentID(u)   OptExtentID(u)"

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()

#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()
#define COMMANDS_OPTIONS \
       ( COMMANDS_STRING(OPTION_HELP, ",h"), "help" )\
       ( OPTION_VERSION, "version" )\
       ( COMMANDS_STRING(OPTION_DBPATH, ",d"), boost::program_options::value<string>(), "database path, default:." ) \
       ( COMMANDS_STRING(OPTION_INDEXPATH, ",x"), boost::program_options::value<string>(), "index path, default with <dbpath>" ) \
       ( COMMANDS_STRING(OPTION_LOBPATH, ",g"), boost::program_options::value<string>(), "lob path, default with <dbpath>" ) \
       ( COMMANDS_STRING(OPTION_LOBMPATH, ",m"), boost::program_options::value<string>(), "lob meta path, default with <dbpath>" ) \
       ( COMMANDS_STRING(OPTION_OUTPUT, ",o"), boost::program_options::value<string>(), "output file, empty for <stdout>" ) \
       ( COMMANDS_STRING(OPTION_VERBOSE, ",v"), boost::program_options::value<string>(), "dump formated information (ture/false), default:true" ) \
       ( COMMANDS_STRING(OPTION_HEX, ",e"), boost::program_options::value<string>(), "dump hex data (ture/false), default:true" ) \
       ( COMMANDS_STRING(OPTION_CSNAME, ",c"), boost::program_options::value<string>(), "collection space name" ) \
       ( COMMANDS_STRING(OPTION_CLNAME, ",l"), boost::program_options::value<string>(), "collection name" ) \
       ( COMMANDS_STRING(OPTION_ACTION, ",a"), boost::program_options::value<string>(), "action (inspect/dump/stat/all)" ) \
       ( COMMANDS_STRING(OPTION_DUMPDATA, ",t"), boost::program_options::value<string>(), "dump/inspect data (true/false), default: false" ) \
       ( COMMANDS_STRING(OPTION_DUMPINDEX, ",i"), boost::program_options::value<string>(), "dump/inspect index (true/false), default: false" ) \
       ( COMMANDS_STRING(OPTION_DUMPLOB, ",b"), boost::program_options::value<string>(), "dump/inspect lob (true/false), default: false" ) \
       ( COMMANDS_STRING(OPTION_PAGESTART, ",s"), boost::program_options::value<SINT32>(), "starting page number, -1 for invalid, default: -1" ) \
       ( COMMANDS_STRING(OPTION_NUMPAGE, ",n"), boost::program_options::value<SINT32>(), "number of pages, take effect when valid <pagestart>" ) \
       ( COMMANDS_STRING(OPTION_SHOW_CONTENT, ",p"), boost::program_options::value<string>(), "display data/index content (true/false), default: false" ) \
       ( OPTION_ONLY_META, boost::program_options::value<string>(), "inspect only meta(Header, SME, MME) (true/false), default:false" ) \
       ( COMMANDS_STRING(OPTION_CRCCHECK, ",C"), boost::program_options::value<string>(), "verify lob data page crc when inspect lob, slower, default: false" ) \
       ( OPTION_MAX_FILESZ, boost::program_options::value<SINT32>(), "the max size (MB) for a output file, range:[10,524288], default: 500" ) \
       ( OPTION_FORCE, "force dump all invalid mb, delete list and index list and so on, default:false" )

//hidden options
#define HIDDEN_OPTIONS \
       ( OPTION_HELPFULL,"help all configs") \
       ( OPTION_JUDGE_BALANCE, boost::program_options::value<string>(), "deprecated" ) \
       ( COMMANDS_STRING(OPTION_REPAIRE, ",r"), boost::program_options::value<string>(), OPTION_REPAIRE_DESP ) \
       ( OPTION_RECOVER, "recover the dropped collection" ) \
       ( OPTION_RECOVER_MBID, boost::program_options::value<SINT32>(), "recover the dropped collection with specified MBID" )
       

#define HELP_EXAMPLE_INSPECT \
   "    bin/sdbdmsdump -d <dbpath> -o <output_file> -a inspect -t true -i true -b true"

#define HELP_EXAMPLE_DUMP \
   "    bin/sdbdmsdump -d <dbpath> -o <output_file> -a dump -t true -i true -b true -p true"

// bitwise operation
#define ACTION_INSPECT           0x01
#define ACTION_DUMP              0x02
#define ACTION_STAT              0x04
#define ACTION_REPAIRE           0x08
#define ACTION_RECOVER           0x10

#define ACTION_INSPECT_STRING    "inspect"
#define ACTION_STAT_STRING       "stat"
#define ACTION_DUMP_STRING       "dump"
#define ACTION_ALL_STRING        "all"

#define DMS_DUMPFILE "dmsdump"

#define W_OK 2

// max size of a output file
#define MAX_FILE_SIZE 500 * 1024 * 1024
// increase delta max 64MB
#define BUFFER_INC_SIZE 67108864
// buffer init 4MB
#define BUFFER_INIT_SIZE 4194304

#define MB_SIZE          ( 1024 * 1024 )
#define GB_SIZE          ( MB_SIZE * 1024 )

#define PMD_REPAIRE_MB_MASK_FLAG             0x00000001
#define PMD_REPAIRE_MB_MASK_LID              0x00000002
#define PMD_REPAIRE_MB_MASK_ATTR             0x00000004
#define PMD_REPAIRE_MB_MASK_RECORD           0x00000008
#define PMD_REPAIRE_MB_MASK_DATAPAGE         0x00000010
#define PMD_REPAIRE_MB_MASK_IDXPAGE          0x00000020
#define PMD_REPAIRE_MB_MASK_LOBPAGE          0x00000040
#define PMD_REPAIRE_MB_MASK_DATAFREE         0x00000080
#define PMD_REPAIRE_MB_MASK_IDXFREE          0x00000100
#define PMD_REPAIRE_MB_MASK_IDXNUM           0x00000200
#define PMD_REPAIRE_MB_MASK_COMPRESSTYPE     0x00000400
#define PMD_REPAIRE_MB_MASK_LOBS             0x00000800
#define PMD_REPAIRE_MB_MASK_COMMITFLAG       0x00001000
#define PMD_REPAIRE_MB_MASK_COMMITLSN        0x00002000
#define PMD_REPAIRE_MB_MASK_IDX_COMMITFLAG   0x00004000
#define PMD_REPAIRE_MB_MASK_IDX_COMMITLSN    0x00008000
#define PMD_REPAIRE_MB_MASK_LOB_COMMITFLAG   0x00010000
#define PMD_REPAIRE_MB_MASK_LOB_COMMITLSN    0x00020000
#define PMD_REPAIRE_MB_MASK_LOBPAGES         0x00040000
#define PMD_REPAIRE_MB_MASK_FIRSTEXTENTID    0x00080000
#define PMD_REPAIRE_MB_MASK_LASTEXTENTID     0x00100000
#define PMD_REPAIRE_MB_MASK_MBEXTEXTENTID    0x00200000
#define PMD_REPAIRE_MB_MASK_DICTEXTENTID     0x00400000
#define PMD_REPAIRE_MB_MASK_OPTEXTENTID      0x00800000


#define RETRY_COUNT 2

namespace
{
     // other value
    enum SDB_INSPT_TYPE
    {
       SDB_INSPT_DATA,
       SDB_INSPT_INDEX,
       SDB_INSPT_LOB
    };

    typedef std::map< INT32, INT32 >                     MAP_PAGES ;
    typedef std::map< UINT16, MAP_PAGES >                MAP_CL_PAGES ;

    /// global: collection pages in lob
    MAP_CL_PAGES  gMapLobCLPages ;

    enum SDB_PAGE_LIST_TYPE
    {
        SDB_PAGE_PREV   = 0,
        SDB_PAGE_NEXT
    } ;

    #define MAKE_PAGE_REL_KEY(pageID,listType)     (((pageID)<<1)|(listType))
    #define GET_PAGEID_FROM_KEY(key)               ((key)>>1)
    #define GET_LISTTYPE_FROM_KEY(key)             ((key)&0x00000001)

    typedef std::map<UINT32, INT32 >               MAP_PAGE_RELATION ;

    /*
       _lobBucketInfo define
    */
    struct _lobBucketInfo
    {
       MAP_PAGE_RELATION   _mapPages ;
       UINT32              _dep ;

       _lobBucketInfo()
       {
          _dep = 0 ;
       }
    } ;
    typedef _lobBucketInfo lobBucketInfo ;

    typedef std::map<UINT32, lobBucketInfo>              MAP_BUCKET_PAGE_INFO ;

    #define LOB_BLK_DEP_SZ             ( 11 )
    /*
       _lobBMEStat define
    */
    struct _lobBMEStat
    {
       UINT64   _sum ;
       UINT64   _value ;   /// dep1^2 + dep2^2 + ...
       UINT32   _minDep ;
       UINT32   _maxDep ;
       UINT32   _depNum[ LOB_BLK_DEP_SZ ] ;

       UINT32   _validBlkCount ;
       UINT32   _validPageCount ;
       UINT32   _invalidPageCount ;

       UINT32   _totalLobCount ;

       _lobBMEStat()
       {
         reset() ;
       }

       void reset()
       {
          _sum = 0 ;
          _value = 0 ;
          _minDep = OSS_UINT32_MAX ;
          _maxDep = 0 ;
          ossMemset( _depNum, 0, sizeof( _depNum ) ) ;

          _validBlkCount = 0 ;
          _validPageCount = 0 ;
          _invalidPageCount = 0 ;

          _totalLobCount = 0 ;
       }
    } ;
    typedef _lobBMEStat lobBMEStat ;

    /// global for lob
    lobBMEStat gLobBMEStat ;

    /*
       _inspectFileStat define
    */
    struct _inspectFileStat
    {
       UINT64              _fileSize ;
       UINT32              _pageNum ;
       UINT32              _usedPages ;
       INT32               _hwmPageID ;
       UINT64              _freePageSize ;
       UINT64              _freeTailSize ;
       UINT64              _totalFreeSize ;

       _inspectFileStat()
       {
          reset() ;
       }

       void reset()
       {
          _fileSize = 0 ;
          _pageNum = 0 ;
          _usedPages = 0 ;
          _hwmPageID = -1 ;
          _freePageSize = 0 ;
          _freeTailSize = 0 ;
          _totalFreeSize = 0 ;
       }
    } ;
    typedef _inspectFileStat inspectFileStat ;

    inspectFileStat gInspectFileStat ;

    /*
       _inspectStat define
    */
    struct _inspectStat
    {
       UINT64              _readBytes ;
       UINT64              _readTimes ;
       UINT64              _ioReadBytes ;
       UINT64              _ioReadTimes ;
       INT64               _minReadLen ;
       INT64               _maxReadLen ;

       UINT64              _writeBytes ;
       UINT64              _writeTimes ;

       UINT32              _totalErr ;

       UINT32              _totalLobFileNum ;
       UINT32              _totalDataFileNum ;
       UINT32              _totalIndexFileNum ;
       UINT64              _totalLobFileSize ;
       UINT64              _totalDataFileSize ;
       UINT64              _totalIndexFileSize ;

       UINT64              _totalFreePageSize ;
       UINT64              _totalFreeTailSize ;
       UINT64              _totalFreeSize ;

       /// time
       UINT64              _startTime ;
       UINT64              _endTime ;

       _inspectStat()
       {
          reset() ;
       }

       void reset()
       {
          _readBytes = 0 ;
          _readTimes = 0 ;
          _ioReadBytes = 0 ;
          _ioReadTimes = 0 ;
          _minReadLen = -1 ;
          _maxReadLen = -1 ;

          _writeBytes = 0 ;
          _writeTimes = 0 ;

          _totalErr = 0 ;

          _totalLobFileNum = 0 ;
          _totalDataFileNum = 0 ;
          _totalIndexFileNum = 0 ;
          _totalLobFileSize = 0 ;
          _totalDataFileSize = 0 ;
          _totalIndexFileSize = 0 ;

          _totalFreePageSize = 0 ;
          _totalFreeTailSize = 0 ;
          _totalFreeSize = 0 ;

          _startTime = 0 ;
          _endTime = 0 ;
       }
    } ;
    typedef _inspectStat inspectStat ;

    inspectStat gStat ;

    /*
       _inspectSMEInfo define
    */
    struct _inspectSMEInfo
    {
       dmsSpaceManagementExtent     *_pSME ;
       INT32                        _hwm ;

       _inspectSMEInfo()
       {
          _pSME = NULL ;
          _hwm  = -1 ;
       }

       INT32 init()
       {
          if ( !_pSME )
          {
             _pSME = SDB_OSS_NEW dmsSpaceManagementExtent() ;
             if ( !_pSME )
             {
                return SDB_OOM ;
             }
          }

          /// reset
          ossMemset( _pSME->_smeMask, 0, sizeof( _pSME->_smeMask ) ) ;
          _hwm = -1 ;

          return SDB_OK ;
       }

       void fini()
       {
          if ( _pSME )
          {
             SDB_OSS_DEL _pSME ;
             _pSME = NULL ;
          }
          _hwm = -1 ;
       }

       BOOLEAN isInit() const { return _pSME ? TRUE : FALSE ; }

       CHAR getBitMask( UINT32 bitNum ) const
       {
          SDB_ASSERT( _pSME , "Invalid _pSME" ) ;
          return _pSME->getBitMask( bitNum ) ;
       }
       void freeBitMask( UINT32 bitNum )
       {
          SDB_ASSERT( _pSME , "Invalid _pSME" ) ;
          _pSME->freeBitMask( bitNum ) ;
       }
       void setBitMask( UINT32 bitNum )
       {
          SDB_ASSERT( _pSME , "Invalid _pSME" ) ;
          _pSME->setBitMask( bitNum ) ;

          if ( -1 == _hwm || _hwm < (INT32)bitNum )
          {
             _hwm = bitNum ;
          }
       }
    } ;
    typedef _inspectSMEInfo inspectSMEInfo ;

    // since we are single-threaded program, we define a lots of global variables :)
    CHAR    gDatabasePath [ OSS_MAX_PATHSIZE + 1 ]       = {0} ;
    CHAR    gIndexPath[ OSS_MAX_PATHSIZE + 1 ]           = {0} ;
    CHAR    gLobPath[ OSS_MAX_PATHSIZE + 1 ]             = {0} ;
    CHAR    gLobmPath[ OSS_MAX_PATHSIZE + 1 ]            = {0} ;
    CHAR    gOutputFileName [ OSS_MAX_PATHSIZE + 1 ]     = {0} ;
    BOOLEAN gOutputIsDir                                 = FALSE ;
    BOOLEAN gVerbose                                     = TRUE ;
    BOOLEAN gHex                                         = TRUE ;
    UINT32  gDumpType                                    = DMS_SU_DMP_OPT_FORMATTED;
    CHAR    gCSName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
    CHAR    gCLName [ DMS_COLLECTION_NAME_SZ + 1 ]       = {0} ;
    CHAR    gAction                                      = 0 ;
    BOOLEAN gDumpData                                    = FALSE ;
    BOOLEAN gDumpIndex                                   = FALSE ;
    SINT32  gStartingPage                                = -1 ;
    SINT32  gNumPages                                    = 1 ;
    SINT64  gMaxFileSize                                 = MAX_FILE_SIZE ;
    SINT32  gOutputFileIndex                             = -1 ;
    SINT64  gOutputFileCurSize                           = 0 ;
    std::string gRepairStr ;
    OSSFILE gOutputFile ; /// output file

    OSSFILE gFile ;  /// data/index/lobm file
    CHAR   *gFileCacheBuff                               = NULL ;
    INT64   gFileCacheOffset                             = -1 ;
    INT64   gFileCacheLen                                = 0 ;

    //to support lob
    OSSFILE gLobdFile;
    CHAR   *gLobdCacheBuff                               = NULL ;
    INT64   gLobdCacheOffset                             = -1 ;
    INT64   gLobdCacheLen                                = 0 ;

    string  gLobdFileName;
    BOOLEAN gDumpLob                                     = FALSE;
    UINT32  gLobdPageSize                                = 0;
    UINT32  gLobmPageSize                                = 0;
    /// lobm header version, decides the lob bucket hash algorithm
    UINT32  gLobmVersion                                 = DMS_LOB_CUR_VERSION;
    UINT32  gSequence                                    = 0;
    BOOLEAN gExistLobs                                   = FALSE;

    //to support bucket balance judgement.
    CHAR   *gBMEBuff                                     = NULL ;

    CHAR *  gBuffer                                      = NULL ;
    UINT32  gBufferSize                                  = 0 ;
    CHAR *  gExtentBuffer                                = NULL ;
    UINT32  gExtentBufferSize                            = 0 ;
    CHAR *  gDictBuffer                                  = NULL ;

    pmdEDUCB *cb                                         = NULL ;

    UINT64  gSecretValue                                 = 0 ;
    UINT32  gPageSize                                    = 0 ;
    UINT64  gSegmentSize                                 = 0 ;
    UINT32  gDataOffset                                  = 0 ;
    UINT32  gPageNum                                     = 0 ;
    UINT32  gPageUsedNum                                 = 0 ;
    CHAR   *gMMEBuff                                     = NULL ;
    CHAR   *gSMEBuff                                     = NULL ;
    BOOLEAN gInitMME                                     = FALSE ;
    BOOLEAN gShowRecordContent                           = FALSE ;
    BOOLEAN gOnlyMeta                                    = FALSE ;
    BOOLEAN gCrcCheck                                    = FALSE ;
    BOOLEAN gForce                                       = FALSE ;
    BOOLEAN gReachEnd                                    = FALSE ;
    BOOLEAN gHitError                                    = FALSE ;
    BOOLEAN gHitCS                                       = FALSE ;
    SDB_INSPT_TYPE gCurInsptType                         = SDB_INSPT_DATA ;
    dmsMBStatInfo gMBStat ;
    dmsMB         gRepaireMB ;
    UINT32        gRepaireMask                           = 0 ;

    dmsStorageUnitHeader gHeader ;

    BOOLEAN gRecover                                     = FALSE ;
    INT32   gRecoverMBID                                 = DMS_INVALID_MBID ;
    INT32   gDoRecoverMBID                               = DMS_INVALID_MBID ;

    inspectSMEInfo gInspectSMEInfo ;

}

/*
    sdbIndexStat define
*/
struct _sdbIndexStat
{
   UINT32   _indexPages ;
   UINT32   _level ;
   UINT64   _freeSpaces ;
   UINT64   _keyNodes ;
   UINT64   _delKeyNodes ;

   _sdbIndexStat()
   {
      _indexPages = 0 ;
      _level = 0 ;
      _freeSpaces = 0 ;
      _keyNodes = 0 ;
      _delKeyNodes = 0 ;
   }
} ;
typedef _sdbIndexStat sdbIndexStat ;

/*
   _sdbIndexDef define
*/
struct _sdbIndexDef
{
   dmsExtentID    _root ;
   string         _name ;

   _sdbIndexDef( dmsExtentID root = DMS_INVALID_EXTENT )
   {
      _root = root ;
   }

   _sdbIndexDef( dmsExtentID root, const string &name )
   {
      _root = root ;
      _name = name ;
   }
} ;
typedef _sdbIndexDef sdbIndexDef ;

INT32 prepareCompressor( OSSFILE &file, UINT32 pageSize, dmsMB *mb, UINT16 id,
                         CHAR *pExpBuffer, dmsCompressorEntry &compressorEntry,
                         SINT32 &err ) ;

void dumpPrintf ( const CHAR *format, ... ) ;

void  clearFileCache( OSSFILE *pFile )
{
   if ( NULL == pFile || &gFile == pFile )
   {
      gFileCacheOffset = -1 ;
      gFileCacheLen = 0 ;
   }

   if ( NULL == pFile || &gLobdFile == pFile )
   {
      gLobdCacheOffset = -1 ;
      gLobdCacheLen = 0 ;
   }
}

void  releaseFileCache( OSSFILE *pFile )
{
   if ( NULL == pFile || &gFile == pFile )
   {
      if ( gFileCacheBuff )
      {
         SDB_OSS_FREE( gFileCacheBuff ) ;
         gFileCacheBuff = NULL ;
      }
      gFileCacheOffset = -1 ;
      gFileCacheLen = 0 ;
   }

   if ( NULL == pFile || &gLobdFile == pFile )
   {
      if ( gLobdCacheBuff )
      {
         SDB_OSS_FREE( gLobdCacheBuff ) ;
         gLobdCacheBuff = NULL ;
      }
      gLobdCacheOffset = -1 ;
      gLobdCacheLen = 0 ;
   }
}

#define FILE_CACHE_SIZE          ( 1 * MB_SIZE )

void  getFileCacheInfo( OSSFILE *pFile, CHAR *&pCacheBuff, INT64 &cacheOffset,
                        INT64 &cacheLen, BOOLEAN canAlloc )
{
   if ( &gFile == pFile )
   {
      if ( !gFileCacheBuff && canAlloc )
      {
         gFileCacheBuff = (CHAR*)SDB_OSS_MALLOC( FILE_CACHE_SIZE ) ;
         gFileCacheOffset = -1 ;
         gFileCacheLen = 0 ;
      }
      pCacheBuff = gFileCacheBuff ;
      cacheOffset = gFileCacheOffset ;
      cacheLen = gFileCacheLen ;
   }
   else if ( &gLobdFile == pFile )
   {
      if ( !gLobdCacheBuff && canAlloc )
      {
         gLobdCacheBuff = (CHAR*)SDB_OSS_MALLOC( FILE_CACHE_SIZE ) ;
         gLobdCacheOffset = -1 ;
         gLobdCacheLen = 0 ;
      }
      pCacheBuff = gLobdCacheBuff ;
      cacheOffset = gLobdCacheOffset ;
      cacheLen = gLobdCacheLen ;
   }
   else
   {
      pCacheBuff = NULL ;
      cacheOffset = -1 ;
      cacheLen = 0 ;
   }
}

void  setFileCacheInfo( const CHAR *pCacheBuff, INT64 cacheOffset, INT64 cacheLen )
{
   if ( gFileCacheBuff == pCacheBuff )
   {
      gFileCacheOffset = cacheOffset ;
      gFileCacheLen = cacheLen ;
   }
   else if ( gLobdCacheBuff == pCacheBuff )
   {
      gLobdCacheOffset = cacheOffset ;
      gLobdCacheLen = cacheLen ;
   }
}

INT32 readData( OSSFILE *pFile, INT64 offset, INT64 len, CHAR *pBuff,
                BOOLEAN useCache = TRUE,
                const CHAR *pReadDesp = NULL )
{
   INT32 rc = SDB_OK ;

   CHAR *pCacheBuff = NULL ;
   INT64 cacheOffset = -1 ;
   INT64 cacheLen = 0 ;

   CHAR *pReadBuff = NULL ;
   INT64 readOffset = 0 ;
   INT64 readLen = 0 ;
   INT64 hasReadLen = 0 ;

   INT64 fileSize = -1 ;

   gStat._readTimes++ ;
   gStat._readBytes += len ;

   if ( gStat._minReadLen < 0 || len < gStat._minReadLen )
   {
      gStat._minReadLen = len ;
   }
   if ( gStat._maxReadLen < 0 || len > gStat._maxReadLen )
   {
      gStat._maxReadLen = len ;
   }

   getFileCacheInfo( pFile, pCacheBuff, cacheOffset, cacheLen, useCache ) ;

   /// check whether in cache
   if ( pCacheBuff && cacheOffset >= 0 && cacheLen > 0 &&
        offset >= cacheOffset && offset < cacheOffset + cacheLen )
   {
      /// copy all from cache
      if ( offset + len <= cacheOffset + cacheLen )
      {
         ossMemcpy( pBuff, pCacheBuff + ( offset - cacheOffset ), len ) ;
         goto done ;
      }
      /// copy some from cache
      else
      {
         INT64 copyLen = cacheOffset + cacheLen - offset ;
         ossMemcpy( pBuff, pCacheBuff + ( offset - cacheOffset ), copyLen ) ;
         pBuff += copyLen ;
         offset += copyLen ;
         len -= copyLen ;
      }
   }

   /// read from file
   if ( useCache && pCacheBuff && len <= FILE_CACHE_SIZE &&
        SDB_OK == ossGetFileSize( pFile, &fileSize ) &&
        offset + len < fileSize )
   {
      /// clear cache
      clearFileCache( pFile ) ;

      pReadBuff = pCacheBuff ;
      readOffset = offset ;
      readLen = FILE_CACHE_SIZE ;

      if ( readOffset + readLen > fileSize )
      {
         readLen = fileSize - readOffset ;
      }
   }
   else
   {
      pReadBuff = pBuff ;
      readOffset = offset ;
      readLen = len ;
   }

   rc = ossSeekAndRead( pFile, readOffset, pReadBuff, readLen, &hasReadLen ) ;
   if ( rc || hasReadLen != readLen )
   {
      dumpPrintf( "*** Error: Failed to read %s, read %lld bytes, "
                  "expect %lld bytes, offset: %lld, rc: %d" OSS_NEWLINE,
                  ( pReadDesp && *pReadDesp ) ? pReadDesp : "data",
                  hasReadLen, readLen, readOffset, rc ) ;
      if ( SDB_OK == rc )
      {
         rc = SDB_IO ;
      }
      goto error ;
   }

   gStat._ioReadBytes += readLen ;
   gStat._ioReadTimes++ ;

   /// copy from cache
   if ( pReadBuff == pCacheBuff )
   {
      /// update cache info
      setFileCacheInfo( pReadBuff, readOffset, readLen ) ;

      ossMemcpy( pBuff, pCacheBuff + ( offset - readOffset ), len ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 switchFile( OSSFILE& file, const INT32 size )
{
   INT32 rc = SDB_OK ;
   CHAR newFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   INT32 retryCount = 0;

   if( gMaxFileSize <= gOutputFileCurSize + size )
   {
      if ( file.isOpened() )
      {
         ossClose( file ) ;
      }

   retry:
      ++gOutputFileIndex ;

      if ( gOutputIsDir )
      {
         ossSnprintf( newFile, OSS_MAX_PATHSIZE, "%s" DMS_DUMPFILE ".%d",
                      gOutputFileName, gOutputFileIndex ) ;
      }
      else
      {
         ossSnprintf( newFile, OSS_MAX_PATHSIZE, "%s.%d",
                      gOutputFileName, gOutputFileIndex ) ;
      }

      rc = ossOpen ( newFile, OSS_REPLACE | OSS_WRITEONLY,
                     OSS_RU|OSS_WU|OSS_RG, file ) ;
      if ( rc )
      {
         ossPrintf ( "*** Error: Failed to open output file: %s, rc: %d" OSS_NEWLINE,
                     newFile, rc ) ;
         ++retryCount ;
         if( RETRY_COUNT < retryCount )
         {
            ossPrintf( "retry times more than %d, return" OSS_NEWLINE,
                       RETRY_COUNT ) ;
            goto error ;
         }
         ossPrintf( "retry again. times : %d" OSS_NEWLINE, retryCount ) ;
         ossMemset ( newFile, 0, OSS_MAX_PATHSIZE + 1 ) ;
         goto retry;
      }

      gOutputFileCurSize = 0 ;
   }

done:
   return rc ;
error:
   goto done ;
}

#define dumpAndShowPrintf(x,...)                                               \
   do {                                                                        \
      ossPrintf ( x, ##__VA_ARGS__ ) ;                                         \
      if ( ossStrlen ( gOutputFileName ) )                                         \
         dumpPrintf ( x, ##__VA_ARGS__ ) ;                                     \
   } while (0)

void init ( po::options_description &desc )
{
   ADD_PARAM_OPTIONS_BEGIN ( desc )
      COMMANDS_OPTIONS
   ADD_PARAM_OPTIONS_END
}

void displayArg ( po::options_description &desc )
{
   std::cout << desc << std::endl ;
   std::cout << "  examples:" << std::endl ;
   std::cout << HELP_EXAMPLE_INSPECT << std::endl ;
   std::cout << HELP_EXAMPLE_DUMP << std::endl << std::endl ;
}

INT32 parseRepaireString( const std::string &str )
{
   const CHAR *pin = str.c_str() ;
   CHAR *pos = (CHAR*)ossStrchr( pin, ':' ) ;
   if ( NULL == pos )
   {
      ossPrintf( "repaire format must be: mb:xx=y,dd=k" OSS_NEWLINE ) ;
      return SDB_INVALIDARG ;
   }
   *pos = 0 ;
   if ( 0 != ossStrcasecmp( pin, "mb" ) )
   {
      *pos = ':' ;
      ossPrintf( "repaire only support for type mb" OSS_NEWLINE ) ;
      return SDB_INVALIDARG ;
   }
   *pos = ':' ;

   /// parse mb member
   vector< pmdAddrPair > items ;
   pmdOptionsCB opt ;
   INT32 rc = opt.parseAddressLine( pos + 1, items, ",", "=", 0 ) ;
   if ( SDB_OK != rc )
   {
      ossPrintf( "Parse repaire value failed: %d"OSS_NEWLINE, rc ) ;
      return rc ;
   }
   UINT64 value = 0 ;
   for ( UINT32 i = 0 ; i < items.size() ; ++i )
   {
      pmdAddrPair &aItem = items[ i ] ;

      /// must be nubmer
      if ( !ossIsInteger( aItem._service ) )
      {
         ossPrintf( "Field[%s]'s value is not number[%s]" OSS_NEWLINE,
                    aItem._host, aItem._service ) ;
         return SDB_INVALIDARG ;
      }
      value = ossAtoll( aItem._service ) ;

      if ( 0 == ossStrcasecmp( aItem._host, "Flag" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_FLAG ;
         gRepaireMB._flag = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "LID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LID ;
         gRepaireMB._logicalID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "Attr" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_ATTR ;
         gRepaireMB._attributes = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "Records" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_RECORD ;
         gRepaireMB._totalRecords = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "DataPages" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_DATAPAGE ;
         gRepaireMB._totalDataPages = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "IndexPages" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_IDXPAGE ;
         gRepaireMB._totalIndexPages = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "LobPages" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LOBPAGE ;
         gRepaireMB._totalLobPages = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "DataFreeSpace" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_DATAFREE ;
         gRepaireMB._totalDataFreeSpace = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "IndexFreeSpace" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_IDXFREE ;
         gRepaireMB._totalIndexFreeSpace = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "IndexNum" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_IDXNUM ;
         gRepaireMB._numIndexes = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "CompressType" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_COMPRESSTYPE ;
         gRepaireMB._compressorType = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "Lobs" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LOBS ;
         gRepaireMB._totalLobs = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "CommitFlag" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_COMMITFLAG ;
         gRepaireMB._commitFlag = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "CommitLSN" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_COMMITLSN ;
         gRepaireMB._commitLSN = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "IdxCommitFlag" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_IDX_COMMITFLAG ;
         gRepaireMB._idxCommitFlag = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "IdxCommitLSN" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_IDX_COMMITLSN ;
         gRepaireMB._idxCommitLSN = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "LobCommitFlag" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LOB_COMMITFLAG ;
         gRepaireMB._lobCommitFlag = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "LobCommitLSN" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LOB_COMMITLSN ;
         gRepaireMB._lobCommitLSN = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "FirstExtentID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_FIRSTEXTENTID ;
         gRepaireMB._firstExtentID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "LastExtentID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_LASTEXTENTID ;
         gRepaireMB._lastExtentID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "MBExtExtentID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_MBEXTEXTENTID ;
         gRepaireMB._mbExExtentID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "DictExtentID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_DICTEXTENTID ;
         gRepaireMB._dictExtentID = value ;
      }
      else if ( 0 == ossStrcasecmp( aItem._host, "OptExtentID" ) )
      {
         gRepaireMask |= PMD_REPAIRE_MB_MASK_OPTEXTENTID ;
         gRepaireMB._mbOptExtentID = value ;
      }
      else
      {
         ossPrintf( "Unknow mb key: %s" OSS_NEWLINE, aItem._host ) ;
         return SDB_INVALIDARG ;
      }
   }

   return SDB_OK ;
}

// resolve input argument
INT32 resolveArgument ( po::options_description &desc, INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   CHAR actionString[BUFFERSIZE] = {0} ;
   CHAR outputFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   po::variables_map vm ;
   po::options_description all ( "Command options" ) ;
   ADD_PARAM_OPTIONS_BEGIN ( all )
      COMMANDS_OPTIONS
      HIDDEN_OPTIONS
   ADD_PARAM_OPTIONS_END

   try
   {
      po::store ( po::parse_command_line ( argc, argv, all ), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      std::cerr <<  "Unknown argument: " << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      std::cerr <<  "Invalid argument: "
                << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch( po::error &e )
   {
      std::cerr << e.what () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( vm.count ( OPTION_HELP ) )
   {
      displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   else if ( vm.count( OPTION_VERSION ) )
   {
      ossPrintVersion( "SDB DmsDump" ) ;
      rc = SDB_PMD_VERSION_ONLY ;
      goto done ;
   }
   else if( vm.count( OPTION_HELPFULL) )
   {
      displayArg ( all ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   // for dbpath, copy to gDatabasePath
   if ( vm.count ( OPTION_DBPATH ) )
   {
      string strDBPath = vm[OPTION_DBPATH].as<string>() ;
      const CHAR *dbpath = strDBPath.c_str() ;
      if ( ossStrlen ( dbpath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "*** Error: db path is too long: %s" OSS_NEWLINE, dbpath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gDatabasePath, dbpath, sizeof(gDatabasePath) ) ;
   }
   else
   {
      // use current directory for default
      ossStrncpy ( gDatabasePath, ".", sizeof(gDatabasePath) ) ;
   }

   // for index path copy to gIndexPath
   if ( vm.count( OPTION_INDEXPATH ) )
   {
      string strIndexPath = vm[OPTION_INDEXPATH].as<string>() ;
      const CHAR *indexPath = strIndexPath.c_str() ;
      if ( ossStrlen ( indexPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "*** Error: index path is too long: %s" OSS_NEWLINE, indexPath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gIndexPath, indexPath, OSS_MAX_PATHSIZE ) ;
   }
   else
   {
      ossStrcpy( gIndexPath, gDatabasePath ) ;
   }

   if ( vm.count( OPTION_LOBPATH ) )
   {
      string strLobPath = vm[OPTION_LOBPATH].as<string>() ;
      const CHAR *lobPath = strLobPath.c_str() ;
      if ( ossStrlen ( lobPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "*** Error: lob path is too long: %s" OSS_NEWLINE, lobPath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gLobPath, lobPath, OSS_MAX_PATHSIZE ) ;
   }
   else
   {
      ossStrcpy( gLobPath, gDatabasePath ) ;
   }

   if ( vm.count( OPTION_LOBMPATH ) )
   {
      string strLobmPath = vm[OPTION_LOBMPATH].as<string>() ;
      const CHAR *lobmPath = strLobmPath.c_str() ;
      if ( ossStrlen ( lobmPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "*** Error: lob meta path is too long: %s" OSS_NEWLINE, lobmPath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gLobmPath, lobmPath, OSS_MAX_PATHSIZE ) ;
   }
   else
   {
      ossStrcpy( gLobmPath, gDatabasePath ) ;
   }

   // check --output, if not provide then use stdout
   if ( vm.count ( OPTION_OUTPUT ) )
   {
      string strOutput = vm[OPTION_OUTPUT].as<string>() ;
      const CHAR *output = strOutput.c_str() ;
      INT32 rc = SDB_OK ;
      SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;

      if ( ossStrlen ( output ) + ossStrlen( DMS_DUMPFILE ) + 2 > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "*** Error: output is too long: %s" OSS_NEWLINE, output ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gOutputFileName, output, sizeof(gOutputFileName) ) ;
      ++gOutputFileIndex ;

      /// check file exist and type
      rc = ossAccess( output, W_OK ) ;
      if ( SDB_OK == rc )
      {
         INT32 retValue = ossGetPathType( gOutputFileName, &fileType ) ;
         if( SDB_OSS_DIR == fileType && SDB_OK == retValue )
         {
            INT32 len = ossStrlen( gOutputFileName ) ;
            if ( OSS_FILE_SEP_CHAR != gOutputFileName[ len - 1 ] )
            {
               gOutputFileName[ len ] = OSS_FILE_SEP_CHAR ;
               gOutputFileName[ len + 1 ] = '\0' ;
            }
            gOutputIsDir = TRUE ;

            ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s" DMS_DUMPFILE ".%d",
                         gOutputFileName, gOutputFileIndex ) ;
         }
         else
         {
            ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s.%d",
                         gOutputFileName, gOutputFileIndex ) ;
         }
      }
      else if ( SDB_FNE == rc )
      {
         ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s.%d",
                      gOutputFileName, gOutputFileIndex ) ;
      }
      else
      {
         goto error ;
      }

      rc = ossOpen ( outputFile, OSS_REPLACE | OSS_WRITEONLY,
                     OSS_RU|OSS_WU|OSS_RG, gOutputFile ) ;
      if ( rc )
      {
         ossPrintf ( "*** Error: Failed to open output file: %s, rc: %d" OSS_NEWLINE,
                     outputFile, rc ) ;
         // if we can't open the file, let's output to screen
         ossMemset ( gOutputFileName, 0, sizeof(gOutputFileName) ) ;
      }
   }

   // whether do dump formated info
   if ( vm.count ( OPTION_VERBOSE ) )
   {
      ossStrToBoolean ( vm[OPTION_VERBOSE].as<string>().c_str(), &gVerbose ) ;
      if ( !gVerbose )
      {
         gDumpType = 0 ;
      }
   }

   /// wheth do dump hex data
   if ( vm.count ( OPTION_HEX ) )
   {
      ossStrToBoolean ( vm[OPTION_HEX].as<string>().c_str(), &gHex ) ;
   }

   if ( gHex )
   {
      gDumpType |= ( DMS_SU_DMP_OPT_HEX |
                     DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                     DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR ) ;
   }

   // whether dump user data
   if ( vm.count ( OPTION_DUMPDATA ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPDATA].as<string>().c_str(), &gDumpData ) ;
   }

   // whether dump index
   if ( vm.count ( OPTION_DUMPINDEX ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPINDEX].as<string>().c_str(), &gDumpIndex ) ;
   }

   // whether dump lob
   if ( vm.count ( OPTION_DUMPLOB ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPLOB].as<string>().c_str(), &gDumpLob ) ;
   }

   // collection space name
   if ( vm.count ( OPTION_CSNAME ) )
   {
      string strCSName = vm[OPTION_CSNAME].as<string>() ;
      const CHAR *csname = strCSName.c_str() ;
      if ( ossStrlen ( csname ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         ossPrintf ( "*** Error: collection space name is too long: %s" OSS_NEWLINE, csname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gCSName, csname, sizeof(gCSName) ) ;
   }

   // collection name
   if ( vm.count ( OPTION_CLNAME ) )
   {
      string strCLName = vm[OPTION_CLNAME].as<string>() ;
      const CHAR *clname = strCLName.c_str() ;
      if ( ossStrlen ( clname ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         ossPrintf ( "*** Error: collection name is too long: %s" OSS_NEWLINE, clname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gCLName, clname, sizeof(gCLName) ) ;
   }

   // starting page
   if ( vm.count ( OPTION_PAGESTART ) )
   {
      gStartingPage = vm[OPTION_PAGESTART].as<SINT32>() ;
   }

   // num pages
   if ( vm.count ( OPTION_NUMPAGE ) )
   {
      gNumPages = vm[OPTION_NUMPAGE].as<SINT32>() ;
   }

   // filesize
   if ( vm.count ( OPTION_MAX_FILESZ ) )
   {
      SINT64 fileSz = vm[OPTION_MAX_FILESZ].as<SINT32>() ;
      if ( fileSz < 10 || fileSz > 512 * 1024 )
      {
         gMaxFileSize = MAX_FILE_SIZE ;
      }
      else
      {
         gMaxFileSize = fileSz * MB_SIZE ;
      }
   }

   if ( vm.count ( OPTION_ACTION ) )
   {
      // inspect input
      string strAction = vm[OPTION_ACTION].as<string>() ;
      const CHAR *action = strAction.c_str() ;

      if ( ossStrncasecmp ( action, ACTION_INSPECT_STRING, ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_INSPECT_STRING, sizeof(actionString) ) ;
         gAction = ACTION_INSPECT ;
      }
      // dump input
      else if ( ossStrncasecmp ( action, ACTION_DUMP_STRING, ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_DUMP_STRING, sizeof(actionString) ) ;
         gAction = ACTION_DUMP ;
      }
      // stat input
      else if ( ossStrncasecmp( action, ACTION_STAT_STRING, ossStrlen(action) ) == 0 )
      {
         ossStrncpy( actionString, ACTION_STAT_STRING, sizeof(actionString) ) ;
         gAction = ACTION_STAT ;
      }
      // all input
      else if ( ossStrncasecmp ( action, ACTION_ALL_STRING, ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_ALL_STRING, sizeof(actionString) ) ;
         gAction = ACTION_INSPECT | ACTION_DUMP | ACTION_STAT ;
      }
      // if action options is not valid, let's display help
      else
      {
         ossPrintf ( "Invalid Action Option: %s" OSS_NEWLINE, action ) ;
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
   }
   else if ( !vm.count( OPTION_REPAIRE ) && !vm.count( OPTION_RECOVER ) &&
             !vm.count( OPTION_RECOVER_MBID ) )
   {
      ossPrintf ( "Action or repaire or recover must be specified" OSS_NEWLINE ) ;
      // if no action specified, let's display help
      displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( OPTION_SHOW_CONTENT ) )
   {
      ossStrToBoolean( vm[OPTION_SHOW_CONTENT].as<string>().c_str(), &gShowRecordContent ) ;
   }
   if ( vm.count( OPTION_ONLY_META ) )
   {
      ossStrToBoolean( vm[OPTION_ONLY_META].as<string>().c_str(), &gOnlyMeta ) ;
   }
   if ( vm.count( OPTION_CRCCHECK ) )
   {
      ossStrToBoolean( vm[OPTION_CRCCHECK].as<string>().c_str(), &gCrcCheck ) ;
   }

   /* deprecated
   if ( vm.count( OPTION_JUDGE_BALANCE) )
   {
      /// ossStrToBoolean( vm[OPTION_JUDGE_BALANCE].as<string>().c_str(), &gBalance) ;
   }
   */

   if ( vm.count( OPTION_FORCE ) )
   {
      gForce = TRUE ;
   }

   if ( vm.count( OPTION_REPAIRE ) )
   {
      gRepairStr = vm[OPTION_REPAIRE].as<string>().c_str() ;
      rc = parseRepaireString( gRepairStr ) ;
      if ( rc )
      {
         goto done ;
      }
      if ( gAction != 0 )
      {
         ossPrintf( "Repaire can't use with other action" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( 0 == ossStrlen( gCLName ) || 0 == ossStrlen( gCSName ) )
      {
         ossPrintf( "Repaire must specify the collection space and "
                    "collection" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      gAction = ACTION_REPAIRE ;
   }

   /// recover
   if ( vm.count( OPTION_RECOVER ) )
   {
      gRecover = TRUE ;
   }

   if ( vm.count( OPTION_RECOVER_MBID ) )
   {
      gRecover = TRUE ;
      gRecoverMBID = vm[OPTION_RECOVER_MBID].as<SINT32>() ;

      if ( gRecoverMBID < 0 && gRecoverMBID >= DMS_MME_SLOTS )
      {
         ossPrintf( "Invalid param(%s)" OSS_NEWLINE, OPTION_RECOVER_MBID ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
   }

   if ( gRecover )
   {
      if ( gAction != 0 )
      {
         ossPrintf( "Recover can't use with other action" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( 0 == ossStrlen( gCLName ) || 0 == ossStrlen( gCSName ) )
      {
         ossPrintf( "Recover must specify the collection space and "
                    "collection" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      gAction = ACTION_RECOVER ;
      /// auto set
      gDumpData = TRUE ;
   }

   if ( !gDumpData && !gDumpIndex && !gDumpLob )
   {
      ossPrintf( "Must specific data, index or lob" OSS_NEWLINE ) ;
      rc = SDB_INVALIDARG ;
      goto done ;
   }

   if ( gStartingPage >= 0 )
   {
      if ( (UINT32)( gDumpData + gDumpIndex + gDumpLob) > 1 )
      {
         ossPrintf( "Dump from starting page only for the one of data, index or lob"
                    OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      if ( gNumPages <= 0 )
      {
         ossPrintf( "Invalid param '%s' value: %d, must grater than zero" OSS_NEWLINE,
                    OPTION_NUMPAGE, gNumPages ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      if ( ossStrlen ( gCSName ) == 0 )
      {
         ossPrintf ( "Colletion space name must be specified for page dump" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   // show input parameters on screen so people can see it
   // save them into output as well
   dumpAndShowPrintf ( "Run Options   :" OSS_NEWLINE ) ;
   dumpAndShowPrintf ( "Database Path : %s" OSS_NEWLINE,
                       gDatabasePath ) ;
   dumpAndShowPrintf ( "Index path    : %s" OSS_NEWLINE,
                       gIndexPath ) ;
   dumpAndShowPrintf ( "Lobm path     : %s" OSS_NEWLINE,
                       gLobmPath) ;
   dumpAndShowPrintf ( "Lob path      : %s" OSS_NEWLINE,
                       gLobPath ) ;
   dumpAndShowPrintf ( "Output File   : %s" OSS_NEWLINE,
                       ossStrlen(gOutputFileName) ? gOutputFileName : "{stdout}" ) ;
   dumpAndShowPrintf ( "Max file size : %d" OSS_NEWLINE,
                       (INT32)( gMaxFileSize / MB_SIZE ) ) ;
   dumpAndShowPrintf ( "Verbose       : %s" OSS_NEWLINE,
                       gVerbose ? "True" : "False" ) ;
   dumpAndShowPrintf ( "Hex           : %s" OSS_NEWLINE,
                       gHex ? "True" : "False" ) ;
   dumpAndShowPrintf ( "CS Name       : %s" OSS_NEWLINE,
                       ossStrlen(gCSName) ? gCSName : "{all}" ) ;
   dumpAndShowPrintf ( "CL Name       : %s" OSS_NEWLINE,
                       ossStrlen(gCLName) ? gCLName : "{all}" ) ;
   dumpAndShowPrintf ( "Action        : %s" OSS_NEWLINE,
                       actionString ) ;
   dumpAndShowPrintf ( "Repaire       : %s" OSS_NEWLINE,
                       gRepairStr.c_str() ) ;
   if ( gRecover )
   {
      dumpAndShowPrintf ( "Recover       : %s" OSS_NEWLINE,
                          gRecover ? "True" : "False" ) ;
      dumpAndShowPrintf ( "Recover MBID  : %d" OSS_NEWLINE,
                          gRecoverMBID ) ;
   }
   dumpAndShowPrintf ( "Dump/Inspect Options  :" OSS_NEWLINE ) ;
   dumpAndShowPrintf ( "   Dump Data  : %s" OSS_NEWLINE,
                       gDumpData ? "True" : "False" ) ;
   dumpAndShowPrintf ( "   Dump Index : %s" OSS_NEWLINE,
                       gDumpIndex ? "True" : "False" ) ;
   dumpAndShowPrintf ( "   Dump Lob   : %s" OSS_NEWLINE,
                       gDumpLob ? "True" : "False" ) ;
   dumpAndShowPrintf ( "   Start Page : %d" OSS_NEWLINE,
                       gStartingPage ) ;
   dumpAndShowPrintf ( "   Num Pages  : %d" OSS_NEWLINE,
                       gNumPages ) ;
   dumpAndShowPrintf ( "   Show record: %s" OSS_NEWLINE,
                       gShowRecordContent ? "True" : "False") ;
   dumpAndShowPrintf ( "   Only Meta  : %s" OSS_NEWLINE,
                       gOnlyMeta ? "True" : "False" ) ;
   dumpAndShowPrintf ( "   force      : %s" OSS_NEWLINE,
                       gForce ? "True" : "False" ) ;
   dumpAndShowPrintf ( OSS_NEWLINE ) ;

done :
   return rc ;
error :
   goto done ;
}

// write output from pBuffer for size bytes, to output file
// if output file is not defined, or failed writing to file, we write to screen
// stdout
void flushOutput ( const CHAR *pBuffer, INT32 size )
{
   INT32 rc = SDB_OK ;
   SINT64 writeSize ;
   SINT64 writtenSize = 0 ;

   if ( 0 == size )
   {
      goto done ;
   }
   else if ( ossStrlen ( gOutputFileName ) == 0 )
   {
      goto error ;
   }

   rc = switchFile( gOutputFile, size ) ;
   if( rc )
   {
      goto error ;
   }

   do
   {
      rc = ossWrite ( &gOutputFile, &pBuffer[writtenSize], size-writtenSize,
                      &writeSize ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         break ;
      }
      rc = SDB_OK ;
      writtenSize += writeSize ;
   } while ( writtenSize < size ) ;

   gOutputFileCurSize += writtenSize ;
   gStat._writeTimes++ ;
   gStat._writeBytes += writtenSize ;

   if ( rc )
   {
      ossPrintf ( "*** Error: Failed to write into file, rc: %d" OSS_NEWLINE,
                  rc ) ;
      goto error ;
   }

done :
   return ;
error :
   ossPrintf ( "%s", pBuffer ) ;
   goto done ;
}

// dump some text into output
#define DUMP_PRINTF_BUFFER_SZ 4095
void dumpPrintf ( const CHAR *format, ... )
{
   INT32 len = 0 ;
   CHAR tempBuffer [ DUMP_PRINTF_BUFFER_SZ + 1 ] = {0} ;
   va_list ap ;
   va_start ( ap, format ) ;
   len = vsnprintf ( tempBuffer, DUMP_PRINTF_BUFFER_SZ, format, ap );
   va_end ( ap ) ;
   flushOutput ( tempBuffer, len ) ;
}

// reallocate global output buffer.
// This buffer is used for dumping format output. By default it's starting from
// BUFFER_INIT_SIZE, and once the caller found current gBufferSize is smaller
// than required, they'll call this function again to double the memory. The
// incremental upper limit is BUFFER_INC_SIZE, and the total amount of buffer
// cannot exceed 2GB ( for protection only )
INT32 reallocBuffer ()
{
   INT32 rc = SDB_OK ;
   if ( gBufferSize == 0 )
   {
      gBufferSize = BUFFER_INIT_SIZE ;
   }
   else
   {
      if ( gBufferSize > 0x7FFFFFFF )
      {
         dumpPrintf ( "*** Error: Cannot allocate more than 2GB" OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      gBufferSize += ( gBufferSize > BUFFER_INC_SIZE ? BUFFER_INC_SIZE : gBufferSize ) ;
   }

   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
   // memory free by end of program
   gBuffer = (CHAR*)SDB_OSS_MALLOC ( gBufferSize ) ;
   if ( !gBuffer )
   {
      dumpPrintf ( "*** Error: Failed to allocate memory for %d bytes" OSS_NEWLINE,
                   gBufferSize ) ;
      rc = SDB_OOM ;
      gBufferSize = 0 ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

// This function allocate extent buffer, which is used to hold a single extent
// read from file. The argument size represents the number of bytes required by
// the extent. If the required size is greater than the current size, we'll
// reallocate required buffer size
INT32 getExtentBuffer ( INT32 size )
{
   INT32 rc = SDB_OK ;

   if ( (UINT32)size > gExtentBufferSize )
   {
      if ( gExtentBuffer )
      {
         SDB_OSS_FREE ( gExtentBuffer ) ;
         gExtentBuffer = NULL ;
         gExtentBufferSize = 0 ;
      }
      gExtentBuffer = (CHAR*)SDB_OSS_MALLOC ( size ) ;
      if ( !gExtentBuffer )
      {
         dumpPrintf ( "*** Error: Failed to allocate extent buffer for %d bytes" OSS_NEWLINE,
                      size ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      gExtentBufferSize = size ;
   }

done :
   return rc ;
error :
   goto done ;
}

// clear global output dump buffer
void clearBuffer ()
{
   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
   gBufferSize = 0 ;
}

// inspect SU's header
/// map the lobm header version to the lob bucket hash algorithm
static UTIL_LOB_HASH_TYPE gLobHashType ()
{
   return ( gLobmVersion >= DMS_LOB_VERSION_3 ) ?
          UTIL_LOB_HASH_MD5 : UTIL_LOB_HASH_DJB2 ;
}

INT32 inspectLobmHeader ( OSSFILE &file, INT64 fileSize, SINT32 &err )
{
   INT32 rc       = SDB_OK ;
   UINT32 len     = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT32 localErr = 0;

   rc = readData( &file, DMS_HEADER_OFFSET, DMS_HEADER_SZ, headerBuffer,
                  FALSE, "lobm header" ) ;
   if ( rc )
   {
      ++err ;
      goto error ;
   }

   /// remember the version to choose the bucket hash algorithm later
   gLobmVersion = ( ( const dmsStorageUnitHeader * )headerBuffer )->_version ;
   // attempt to format, note if len is gBufferSize - 1, that means we write to
   // end of buffer, which represents the current buffer size is not sufficient,
   // then clearly we should attempt to realloc buffer and format again
retry :
   localErr = 0;
   len = dmsInspect::inspectLobmHeader( headerBuffer, DMS_HEADER_SZ,
                                        gBuffer, gBufferSize, gSequence,
                                        gPageNum, gLobmPageSize, gSecretValue,
                                        fileSize, localErr ) ;
   if ( len >= gBufferSize - 1 )
   {
      // if len is same as buffer size, that means we run out of buffer memory
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         // if we failed to realloc more memory
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }

   flushOutput ( gBuffer, len ) ;
   err += localErr ;

done :
   return rc;
error :
   goto done ;
}


// inspect SU's header
INT32 inspectLobdHeader ( UINT64 fileSize, SINT32 &totalErr )
{
   INT32 rc       = SDB_OK ;
   UINT32 len     = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT32 localErr;

   rc = readData( &gLobdFile, DMS_HEADER_OFFSET, DMS_HEADER_SZ, headerBuffer,
                  FALSE, "lobd header" ) ;
   if ( rc )
   {
      ++totalErr ;
      goto error ;
   }
   // attempt to format, note if len is gBufferSize - 1, that means we write to
   // end of buffer, which represents the current buffer size is not sufficient,
   // then clearly we should attempt to realloc buffer and format again
retry :
    localErr = 0;
    len = dmsInspect::inspectLobdHeader ( headerBuffer, DMS_HEADER_SZ,
                                          gBuffer, gBufferSize,
                                          gSequence, gSecretValue,
                                          fileSize, localErr ) ;
    if ( len >= gBufferSize - 1 )
    {
      // if len is same as buffer size, that means we run out of buffer memory
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         // if we failed to realloc more memory
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
    }
    flushOutput ( gBuffer, len ) ;
    totalErr += localErr;

done :
   return rc;
error :
   goto done ;
}


// inspect SU's header
void inspectHeader ( OSSFILE &file, UINT32 &pageSize, SINT32 &err )
{
   INT32 rc       = SDB_OK ;
   INT32 localErr = 0 ;
   UINT32 len     = 0 ;
   CHAR *headerBuffer = (CHAR*)&gHeader ;
   UINT64 secretValue = 0 ;
   UINT32 segmentSize = 0 ;

   rc = readData( &file, DMS_HEADER_OFFSET, DMS_HEADER_SZ, headerBuffer,
                  FALSE, "header" ) ;
   if ( rc )
   {
      ++err ;
      goto error ;
   }
   // attempt to format, note if len is gBufferSize - 1, that means we write to
   // end of buffer, which represents the current buffer size is not sufficient,
   // then clearly we should attempt to realloc buffer and format again
retry :
   localErr = 0 ;
   len = dmsInspect::inspectHeader ( headerBuffer, DMS_HEADER_SZ,
                                     gBuffer, gBufferSize,
                                     pageSize, gPageNum,
                                     segmentSize,
                                     secretValue, localErr ) ;
   if ( len >= gBufferSize - 1 )
   {
      // if len is same as buffer size, that means we run out of buffer memory
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         // if we failed to realloc more memory
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   err += localErr ;
   flushOutput ( gBuffer, len ) ;

   // check
   if ( secretValue != gSecretValue )
   {
      dumpPrintf ( "*** Error: Secret value[%llu] is not expected[%llu]" OSS_NEWLINE,
                    secretValue, gSecretValue ) ;
      ++err ;
   }
   if ( (UINT32)pageSize != gPageSize )
   {
      dumpPrintf ( "*** Error: Page size[%d] is not expected[%d]" OSS_NEWLINE,
                    pageSize, gPageSize ) ;
      ++err ;
   }
   if ( segmentSize != gSegmentSize )
   {
      dumpPrintf ( "*** Error: Segment size[%d] is not expected[%d]" OSS_NEWLINE,
                   segmentSize, gSegmentSize ) ;
      ++err ;
   }

done :
   return ;
error :
   goto done ;
}

void dumpHeader ( OSSFILE &file, UINT32 &pageSize, UINT32 &pageNum )
{
   INT32 rc                            = SDB_OK ;
   UINT32 len                          = 0 ;
   CHAR *headerBuffer = ( CHAR* )&gHeader ;

   rc = readData( &file, DMS_HEADER_OFFSET, DMS_HEADER_SZ, headerBuffer,
                  FALSE, "header" ) ;
   if ( rc )
   {
      goto error ;
   }
   // attempt to format, note if len is gBufferSize - 1, that means we write to
   // end of buffer, which represents the current buffer size is not sufficient,
   // then clearly we should attempt to realloc buffer and format again
retry :
   len = dmsDump::dumpHeader ( headerBuffer, DMS_HEADER_SZ,
                               gBuffer, gBufferSize, NULL,
                               gDumpType, pageSize, pageNum ) ;
   if ( len >= gBufferSize - 1 )
   {
      // if len is same as buffer size, that means we run out of buffer memory
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         // if we failed to realloc more memory
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   // flush result to output ( if we can't allocate more memory, we shouldn't
   // hit here anyway )
   flushOutput ( gBuffer, len ) ;

done :
   return ;
error :
   goto done ;
}

void inspectSME ( OSSFILE &file, const CHAR *pExpBuf, SINT32 &hwm, SINT32 &err,
                  BOOLEAN withRead = TRUE )
{
   INT32 rc        = SDB_OK ;
   UINT32 len      = 0 ;
   CHAR *smeBuffer = gSMEBuff ;
   SINT32 localErr = 0 ;
   SINT32 expHWM   = -1 ;
   CHAR *pInspectSMEBuff = NULL ;
   inspectSMEInfo *pInspectSMEInfo = (inspectSMEInfo*)pExpBuf ;

   if ( pInspectSMEInfo )
   {
      pInspectSMEBuff = (CHAR*)( pInspectSMEInfo->_pSME ) ;
      expHWM = pInspectSMEInfo->_hwm ;
   }

   if ( withRead )
   {
      rc = readData( &file, DMS_SME_OFFSET, DMS_SME_SZ, smeBuffer, FALSE, "sme" ) ;
      if ( rc )
      {
         ++err ;
         goto error ;
      }
   }

retry :
   localErr = 0 ;
   len = dmsInspect::inspectSME ( smeBuffer, DMS_SME_SZ,
                                  gBuffer, gBufferSize,
                                  pInspectSMEBuff, gPageNum,
                                  gPageUsedNum, hwm, localErr, expHWM ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;
   err += localErr ;

done :
   return ;
error :
   goto done ;
}

void dumpSME ( OSSFILE &file, CHAR *pSmeBuffer, SINT32 &hwmPages )
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   // free by end of function
   // since SME is too large to be held by stack, we do heap allocation

   rc = readData( &file, DMS_SME_OFFSET, DMS_SME_SZ, pSmeBuffer, FALSE, "sme" ) ;
   if ( rc )
   {
      goto error ;
   }

   // format it, if len == gBufferSize -1, that means buffer is not large enough
   // and we should attempt to realloc
retry :
   len = dmsDump::dumpSME ( pSmeBuffer, DMS_SME_SZ,
                            gBuffer, gBufferSize, gPageNum, gPageUsedNum, hwmPages ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   // write output
   flushOutput ( gBuffer, len ) ;

done :
   return ;
error :
   goto done ;
}

// extract extent header by
// 1) input file
// 2) extent id
// 3) SU page size
// and output to extentHead structure
INT32 getExtentHead ( OSSFILE &file, dmsExtentID extentID, UINT32 pageSize,
                      dmsExtent &extentHead )
{
   return readData( &file, gDataOffset + (SINT64)pageSize * extentID,
                    DMS_EXTENT_METADATA_SZ, (CHAR*)&extentHead, TRUE,
                    "extent header" ) ;
}

// extract full extent by
// 1) input file
// 2) extent id
// 3) page size
// 4) extent size
// This function store output to global gExtentBuffer
INT32 getExtent ( OSSFILE &file, dmsExtentID extentID, UINT32 pageSize,
                  SINT32 extentSize, BOOLEAN dictExtent = FALSE )
{
   INT32 rc = SDB_OK ;
   CHAR *buffer = NULL ;

   if ( dictExtent )
   {
      buffer = gDictBuffer ;
   }
   else
   {
      // only realloc extent memory when it's not large enough
      if ( gExtentBufferSize < (UINT32)(extentSize * pageSize ) )
      {
         rc = getExtentBuffer ( extentSize*pageSize ) ;
         if ( rc )
         {
            dumpPrintf ( "*** Error: Failed to allocate extent buffer, rc: %d" OSS_NEWLINE,
                         rc ) ;
            goto error ;
         }
      }
      buffer = gExtentBuffer ;
   }

   rc = readData( &file, gDataOffset + (SINT64)pageSize * extentID,
                  extentSize * pageSize, buffer, TRUE, "extent" ) ;
   // output sanity check
   if ( rc )
   {
      // out of range, should jump out of loop to avoid printing valid log
      gReachEnd = TRUE ;
   }

done :
   return rc ;
error :
   goto done ;
}

enum INSPECT_EXTENT_TYPE
{
   INSPECT_EXTENT_TYPE_DATA = 0,
   INSPECT_EXTENT_TYPE_INDEX,
   INSPECT_EXTENT_TYPE_INDEX_CB,
   INSPECT_EXTENT_TYPE_MBEX,
   INSPECT_EXTENT_TYPE_DICT,
   INSPECT_EXTENT_TYPE_EXTOPT,
   // for unknown type, that means we do not know which type of extent it is.
   // For example if we are provided by a single extent id without any other
   // information, in this case our extract function should first read extent
   // header and determine the type of extent
   INSPECT_EXTENT_TYPE_UNKNOWN
} ;

// check if an extent is valid, TRUE means valid, FALSE means invalid
// If type = INSPECT_EXTENT_TYPE_UNKNOWN, we'll first detect the extent type,
// and then assign type to the correct value, then do validation
BOOLEAN extentSanityCheck ( dmsExtent &extentHead,
                            INSPECT_EXTENT_TYPE &type, // in-out
                            UINT32 pageSize,
                            UINT16 expID )
{
   BOOLEAN result = TRUE ;

retry :
   if ( INSPECT_EXTENT_TYPE_DATA == type )
   {
      // make sure eye catcher is "DE" ( Data Extent )
      if ( extentHead._eyeCatcher[0] != DMS_EXTENT_EYECATCHER0 ||
           extentHead._eyeCatcher[1] != DMS_EXTENT_EYECATCHER1 )
      {
         dumpPrintf ( "*** Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                      extentHead._eyeCatcher[0],
                      extentHead._eyeCatcher[1] ) ;
         result = FALSE ;
      }
      // make sure the block size is greater than 0, and doesn't exceed max
      if ( extentHead._blockSize <= 0 ||
           (UINT64)extentHead._blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf ( "*** Error: Invalid block size: %u, pageSize: %d" OSS_NEWLINE,
                      extentHead._blockSize, pageSize ) ;
         result = FALSE ;
      }
      // make sure the collection id is what we want
      if ( extentHead._mbID != expID )
      {
         dumpPrintf ( "*** Error: Unexpected id: %u, expected %u" OSS_NEWLINE,
                      extentHead._mbID, expID ) ;
         result = FALSE ;
      }
      // make sure the version is not rediculous
      if ( extentHead._version > DMS_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "*** Error: Invalid version: %d, current %d" OSS_NEWLINE,
                      extentHead._version, DMS_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
      // first/last record offset must be sync
      if ( ( extentHead._firstRecordOffset != DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset == DMS_INVALID_OFFSET ) ||
           ( extentHead._firstRecordOffset == DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset != DMS_INVALID_OFFSET ) )
      {
         dumpPrintf ( "*** Error: Bad first/last offset: %d:%d" OSS_NEWLINE,
                      extentHead._firstRecordOffset,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      // first record doesn't go wild
      if ( extentHead._firstRecordOffset >= extentHead._blockSize * (SINT32)pageSize )
      {
         dumpPrintf ( "*** Error: Bad first record offset: %d" OSS_NEWLINE,
                      extentHead._firstRecordOffset ) ;
         result = FALSE ;
      }
      // last record doens't go wild
      if ( extentHead._lastRecordOffset >= extentHead._blockSize * (SINT32)pageSize )
      {
         dumpPrintf ( "*** Error: Bad last record offset: %d" OSS_NEWLINE,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      // free space is making sense too
      if ( (UINT32)extentHead._freeSpace >
           extentHead._blockSize * (SINT32)pageSize - DMS_EXTENT_METADATA_SZ )
      {
         dumpPrintf ( "*** Error: Invalid free space: %d, extentSize: %u" OSS_NEWLINE,
                      extentHead._freeSpace,
                      extentHead._blockSize * pageSize ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_INDEX == type )
   {
      // index extent sanity check
   }
   else if ( INSPECT_EXTENT_TYPE_INDEX_CB == type )
   {
      // index control block sanity check
   }
   else if ( INSPECT_EXTENT_TYPE_MBEX == type )
   {
      dmsMetaExtent *metaExt = ( dmsMetaExtent* )&extentHead ;
      // make sure eye catcher is "ME" ( Meta Extent )
      if ( metaExt->_eyeCatcher[0] != DMS_META_EXTENT_EYECATCHER0 ||
           metaExt->_eyeCatcher[1] != DMS_META_EXTENT_EYECATCHER1 )
      {
         dumpPrintf ( "*** Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                      metaExt->_eyeCatcher[0],
                      metaExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      // make sure the block size is greater than 0, and doesn't exceed max
      if ( metaExt->_blockSize <= 0 ||
           (UINT64)metaExt->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf ( "*** Error: Invalid block size: %d, pageSize: %d" OSS_NEWLINE,
                      metaExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      // make sure the collection id is what we want
      if ( metaExt->_mbID != expID )
      {
         dumpPrintf ( "*** Error: Unexpected id: %u, expected %u" OSS_NEWLINE,
                      metaExt->_mbID, expID ) ;
         result = FALSE ;
      }
      // make sure the version is not rediculous
      if ( metaExt->_version > DMS_META_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "*** Error: Invalid version: %d, current %d" OSS_NEWLINE,
                      metaExt->_version, DMS_META_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_DICT == type )
   {
      dmsDictExtent *dictExt = ( dmsDictExtent* )&extentHead ;
      if ( dictExt->_eyeCatcher[0] != DMS_DICT_EXTENT_EYECATCHER0 ||
           dictExt->_eyeCatcher[1] != DMS_DICT_EXTENT_EYECATCHER1 )
      {
         dumpPrintf( "*** Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                     dictExt->_eyeCatcher[0],
                     dictExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( dictExt->_blockSize <= 0 ||
           (UINT64)dictExt->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf( "*** Error: Invalid block size: %d, pageSize: %d" OSS_NEWLINE,
                     dictExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( dictExt->_mbID != expID )
      {
         dumpPrintf( "*** Error: Unexpected id: %u, expected %u" OSS_NEWLINE,
                     dictExt->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( dictExt->_version > DMS_DICT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "*** Error: Invalid version: %d, current %d" OSS_NEWLINE,
                     dictExt->_version, DMS_DICT_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_EXTOPT == type )
   {
      dmsOptExtent *extent = (dmsOptExtent *)&extentHead ;
      if ( extent->_eyeCatcher[0] != DMS_OPT_EXTENT_EYECATCHER0 ||
           extent->_eyeCatcher[1] != DMS_OPT_EXTENT_EYECATCHER1 )
      {
         dumpPrintf( "*** Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                     extent->_eyeCatcher[0],
                     extent->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( extent->_blockSize <= 0 ||
           (UINT64)extent->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf( "*** Error: Invalid block size: %d, pageSize: %d" OSS_NEWLINE,
                     extent->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( extent->_mbID != expID )
      {
         dumpPrintf( "*** Error: Unexpected id: %u, expected %u" OSS_NEWLINE,
                     extent->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( extent->_version > DMS_OPT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "*** Error: Invalid version: %d, current %d" OSS_NEWLINE,
                     extent->_version, DMS_OPT_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_UNKNOWN == type )
   {
      // if we do not know which type to read, let's first check eye catcher
      if ( extentHead._eyeCatcher[0] == DMS_EXTENT_EYECATCHER0 &&
           extentHead._eyeCatcher[1] == DMS_EXTENT_EYECATCHER1 )
      {
         // so this is data extent, let's go back to check again
         type = INSPECT_EXTENT_TYPE_DATA ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == IXM_EXTENT_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == IXM_EXTENT_EYECATCHER1 )
      {
         // this is index extent, let's go back to check again
         type = INSPECT_EXTENT_TYPE_INDEX ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == IXM_EXTENT_CB_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == IXM_EXTENT_CB_EYECATCHER1 )
      {
         // this is index control block extent, let's go back to check again
         type = INSPECT_EXTENT_TYPE_INDEX_CB ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == DMS_META_EXTENT_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == DMS_META_EXTENT_EYECATCHER1 )
      {
         type = INSPECT_EXTENT_TYPE_MBEX ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == DMS_DICT_EXTENT_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == DMS_DICT_EXTENT_EYECATCHER1 )
      {
         type = INSPECT_EXTENT_TYPE_DICT ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == DMS_OPT_EXTENT_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == DMS_OPT_EXTENT_EYECATCHER1 )
      {
         type = INSPECT_EXTENT_TYPE_EXTOPT ;
         goto retry ;
      }
      else
      {
         // we don't know the type, let's get out of here
         dumpPrintf ( "*** Error: Unknown eye catcher: %c%c" OSS_NEWLINE,
                      extentHead._eyeCatcher[0],
                      extentHead._eyeCatcher[1] ) ;
         result = FALSE ;
      }
   }
   else
   {
      SDB_ASSERT ( FALSE, "should never hit here" ) ;
   }
   return result ;
}

INT32 loadMB ( UINT16 collectionID, dmsMB *&mb )
{
   INT32 rc = SDB_SYS ;
   mb = NULL ;

   if ( gInitMME && collectionID < DMS_MME_SLOTS )
   {
      dmsMetadataManagementExtent *pMME = (dmsMetadataManagementExtent*)gMMEBuff ;
      mb = &(pMME->_mbList[collectionID]) ;
      rc = SDB_OK ;
   }

   return rc ;
}

// load a given extent into memory
// first we need to load extent header and do sanity check. If we found
// something strange, we don't want to load garbage.
// Then let's load the full extent and return
INT32 loadExtent ( OSSFILE &file, INSPECT_EXTENT_TYPE &type,
                   UINT32 pageSize, dmsExtentID extentID,
                   UINT16 collectionID )
{
   INT32 rc = SDB_OK ;
   dmsExtent extentHead ;

   // load header first
   rc = getExtentHead ( file, extentID, pageSize, extentHead ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to get extent head, rc: %d" OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   // sanity check
   if ( !extentSanityCheck ( extentHead, type, pageSize, collectionID ) )
   {
      dumpPrintf ( "*** Error: Failed head sanity check, dump head:" OSS_NEWLINE ) ;
      INT32 len = ossHexDumpBuffer ( (CHAR*)&extentHead,
                                     DMS_EXTENT_METADATA_SZ,
                                     gBuffer, gBufferSize, NULL,
                                     OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
      flushOutput ( gBuffer, len ) ;
      rc = SDB_DMS_CORRUPTED_EXTENT ;
      goto error ;
   }

   // load full extent, block size is variable only for data extent
   // for Index and IndexCB extent, the size should always be 1
   rc = getExtent ( file, extentID, pageSize,
                    ( INSPECT_EXTENT_TYPE_DATA == type ||
                      INSPECT_EXTENT_TYPE_MBEX == type ||
                      INSPECT_EXTENT_TYPE_DICT == type ) ?
                      extentHead._blockSize : 1,
                      INSPECT_EXTENT_TYPE_DICT == type ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to get extent %d, rc: %d" OSS_NEWLINE,
                   extentID, rc ) ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

void inspectOverflowedRecords ( OSSFILE &file, UINT32 pageSize,
                                UINT16 collectionID, dmsExtentID ovfFromExtent,
                                std::set<dmsRecordID> &overRIDList,
                                SINT32 &err,
                                dmsCompressorEntry *compressorEntry,
                                UINT64 &compressedNum )
{
   INT32 rc        = SDB_OK ;
   SINT32 oldErr   = err ;
   SINT32 localErr = 0 ;
   UINT32 len      = 0 ;
   INT32 count     = 0 ;
   std::set<dmsRecordID>::iterator it ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DATA ;
   dmsExtentID currentExtentID = DMS_INVALID_EXTENT ;
   BOOLEAN isCompressed = FALSE ;
   dmsExtent *pExtent = NULL ;

   /*dumpPrintf ( "Inspect Overflow-Records for Collection [%d]'s extent [%d]"
                OSS_NEWLINE, collectionID, ovfFromExtent ) ; */

   for ( it = overRIDList.begin() ; it != overRIDList.end() ; ++it )
   {
      dmsRecordID rid = *it ;
      dmsOffset offset = 0 ;
      if ( rid._extent > (SINT32)gPageNum )
      {
         dumpPrintf ( "*** Error: overflowed rid extent is out of range: "
                      "0x08x (%d) 0x08x (%d)" OSS_NEWLINE,
                      rid._extent, rid._extent,
                      rid._offset, rid._offset ) ;
         ++err ;
         continue ;
      }

      if ( currentExtentID != rid._extent )
      {
         // only load extent when it's not what we are opening now
         rc = loadExtent ( file, extentType, pageSize, rid._extent, collectionID ) ;
         if ( rc )
         {
            dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                         rid._extent, rc ) ;
            ++err ;
            goto error ;
         }
         currentExtentID = rid._extent ;
      }

   retry :
      // inspect record
      offset = rid._offset ;
      localErr = 0 ;
      isCompressed = FALSE ;
      pExtent = (dmsExtent*)gExtentBuffer ;
      // we are not getting 'isDeleteing' value from current method,
      // for all the overflow records we check here are not in 'Deleting' stat
      len = dmsInspect::inspectDataRecord ( cb, gExtentBuffer + offset,
                                            pExtent->_blockSize * pageSize - offset,
                                            gBuffer, gBufferSize,
                                            count, rid._extent, offset, NULL, localErr,
                                            compressorEntry, isCompressed ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      err += localErr ;
      ++count ;

      if ( isCompressed )
      {
         ++compressedNum ;
      }
   } // end for

done :
   if ( oldErr == err )
   {
      /*dumpPrintf ( " Inspect Overflow-Records for Collection [%d]' extent [%d] "
                   "Done Succeed" OSS_NEWLINE, collectionID,
                   ovfFromExtent ) ;*/
   }
   else
   {
      dumpPrintf ( " Inspect Overflow-Records for Collection [%d]' extent [%d] "
                   "done with errors: %d" OSS_NEWLINE, collectionID,
                   ovfFromExtent, err-oldErr ) ;
   }
   return ;
error :
   goto done ;
}

void dumpOverflowedRecords ( OSSFILE &file, UINT32 pageSize,
                             UINT16 collectionID, dmsExtentID ovfFromExtID,
                             std::set<dmsRecordID> &overRIDList,
                             dmsCompressorEntry *compressorEntry )
{
   INT32 rc = SDB_OK ;
   UINT32 len = 0 ;
   std::set<dmsRecordID>::iterator it ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DATA ;
   dmsExtentID currentExtentID = DMS_INVALID_EXTENT ;
   dmsExtent *pExtent = NULL ;

   dumpPrintf ( " Dump Overflow-Records for Collection [%d]'s extent [%d]" OSS_NEWLINE,
                collectionID, ovfFromExtID ) ;

   for ( it = overRIDList.begin() ; it != overRIDList.end() ; ++it )
   {
      dmsRecordID rid = *it ;
      dmsOffset offset = 0 ;
      if ( currentExtentID != rid._extent )
      {
         // only load extent when it's not what we are opening now
         rc = loadExtent ( file, extentType, pageSize, rid._extent, collectionID ) ;
         if ( rc )
         {
            dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                         rid._extent, rc ) ;
            goto error ;
         }
         currentExtentID = rid._extent ;
      }

   retry :
      // attempt to format the record
      offset = rid._offset ;
      pExtent = (dmsExtent*)gExtentBuffer ;
      dumpPrintf ( "    OvfRecord 0x%08x : 0x%08x:" OSS_NEWLINE,
                   rid._extent, rid._offset ) ;
      len = dmsDump::dumpDataRecord ( cb, gExtentBuffer + offset,
                                      pExtent->_blockSize * pageSize - offset,
                                      gBuffer, gBufferSize, offset,
                                      compressorEntry, NULL ) ;

      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      dumpPrintf ( OSS_NEWLINE ) ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectIndexDef ( OSSFILE &file, UINT32 pageSize, UINT16 collectionID,
                       dmsMB *mb, CHAR *pExpBuffer,
                       std::map<UINT16, sdbIndexDef> &indexRoots,
                       SINT32 &err )
{
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   INT32 localErr = 0 ;
   INT32 oldErr   = err ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX_CB ;
   string idxName ;

   // first let's see how many indexes we have
   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "*** Error: numIdx is out of range, max: %d" OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      ++err ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }

   // loop through all indexes
   for ( UINT16 i = 0 ; i < mb->_numIndexes ; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      idxName.clear() ;

      // dump index cb extent
      if ( indexCBExtentID == DMS_INVALID_EXTENT ||
           (UINT32)indexCBExtentID >= gPageNum )
      {
         dumpPrintf ( "*** Error: Index CB Extent ID is not valid: 0x%08x (%d)" OSS_NEWLINE,
                      indexCBExtentID, indexCBExtentID ) ;
         ++err ;
         continue ;
      }

      if ( pExpBuffer )
      {
         inspectSMEInfo *pSME=( inspectSMEInfo* )pExpBuffer ;
         if ( pSME->getBitMask( indexCBExtentID ) != DMS_SME_FREE )
         {
            dumpPrintf ( "*** Error: SME extent 0x%08x (%d) is not free" OSS_NEWLINE,
                         indexCBExtentID, indexCBExtentID ) ;
            ++err ;
         }
         pSME->setBitMask( indexCBExtentID ) ;
      }

      // load index CB into memory
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent, rc: %d" OSS_NEWLINE,
                      rc ) ;
         ++err ;
         continue ;
      }

      try
      {
         BSONObj indexDef( gExtentBuffer+sizeof(ixmIndexCBExtent) ) ;
         if ( indexDef.hasField ( IXM_UNIQUE_FIELD ) )
         {
            BSONElement e = indexDef.getField( IXM_UNIQUE_FIELD ) ;
            if ( e.booleanSafe() )
            {
               gMBStat._uniqueIdxNum += 1 ;
            }
         }
         if ( indexDef.hasField ( IXM_GLOBAL_FIELD ) )
         {
            BSONElement e = indexDef.getField( IXM_GLOBAL_FIELD ) ;
            if ( e.booleanSafe() )
            {
               gMBStat._globIdxNum += 1 ;
            }
         }
         if ( indexDef.hasField( IXM_NAME_FIELD ) )
         {
            idxName = indexDef.getStringField( IXM_NAME_FIELD ) ;
         }
      }
      catch( std::exception & )
      {
         /// donothing
      }
      gMBStat._totalIndexPages += 1 ;

   retry :
      // dump index cb and get root, since index cb must be 1 page, so we put
      // pageSize as inSize
      localErr = 0 ;
      len = dmsInspect::inspectIndexCBExtent ( gExtentBuffer, pageSize,
                                               gBuffer, gBufferSize,
                                               i, indexCBExtentID,
                                               collectionID,
                                               indexRoot,
                                               localErr ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      err += localErr ;
      indexRoots[i] = sdbIndexDef( indexRoot, idxName ) ;
   }

done :
   if ( oldErr != err )
   {
      dumpPrintf ( " Inspect Index Def for Collection [%u] Done with Error: %d" OSS_NEWLINE,
                   collectionID, err-oldErr ) ;
   }
   return ;
error :
   goto done ;
}

void dumpIndexDef ( OSSFILE &file, UINT32 pageSize, UINT16 collectionID,
                    dmsMB *mb, std::map<UINT16, dmsExtentID> &indexRoots )
{
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX_CB ;
   dumpPrintf ( " Dump Index Def for Collection [%u]" OSS_NEWLINE,
                collectionID ) ;

   // first let's see how many indexes we have
   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "*** Error: numIdx is out of range, max: %d" OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }

   // loop through all indexes
   for ( UINT16 i = 0 ; i < mb->_numIndexes; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      // dump index cb extent
      dumpPrintf ( "    Index [ %u ] : 0x%08x" OSS_NEWLINE,
                   i, indexCBExtentID ) ;
      // sanity check
      if ( indexCBExtentID == DMS_INVALID_EXTENT ||
           (UINT32)indexCBExtentID >= gPageNum )
      {
         dumpPrintf ( "*** Error: Index CB Extent ID is not valid: 0x%08x (%d)" OSS_NEWLINE,
                      indexCBExtentID, indexCBExtentID ) ;
         continue ;
      }
      // load index CB into memory
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent, rc: %d" OSS_NEWLINE,
                      rc ) ;
         continue ;
      }

   retry :
      // dump index cb and get root, since index cb must be 1 page, so we put
      // pageSize as inSize
      len = dmsDump::dumpIndexCBExtent ( gExtentBuffer, pageSize,
                                         gBuffer, gBufferSize,
                                         NULL,
                                         gDumpType,
                                         indexRoot ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      dumpPrintf ( OSS_NEWLINE ) ;

      // if the root doesn't exit, we should not do anything ( maybe it's a
      // newly created index without any data )
      indexRoots[i] = indexRoot ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectIndexExtents ( OSSFILE &file, UINT32 pageSize,
                           dmsExtentID rootID, UINT16 collectionID,
                           CHAR *pExpBuffer, sdbIndexStat &indexStat,
                           SINT32 &err )
{
   UINT32 len      = 0 ;
   INT32 rc        = SDB_OK ;
   SINT32 localErr = 0 ;
   UINT32 keyNodes = 0 ;
   UINT32 delKeyNodes = 0 ;
   BufBuilder builder( 128 ) ;
   /// UINT64( extent, level )
   std::set<UINT64> childExtents ;
   // set to index type
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX ;
   ixmExtentHead *pExtentHead = NULL ;
   childExtents.insert( ossPack32To64( rootID, 1 ) ) ;

   while ( !childExtents.empty() )
   {
      dmsExtentID childID = DMS_INVALID_EXTENT ;
      UINT32 level = 0 ;
      UINT32 hi = 0 ;

      ossUnpack32From64( *(childExtents.begin()), hi, level ) ;
      childExtents.erase( childExtents.begin() ) ;
      childID = (dmsExtentID)hi ;

      if ( childID == DMS_INVALID_EXTENT || childID >= (SINT32)gPageNum )
      {
         dumpPrintf ( "*** Error: index extent ID is not valid: 0x%08x (%d)" OSS_NEWLINE,
                      childID, childID ) ;
         ++err ;
         continue ;
      }

      if ( pExpBuffer )
      {
         inspectSMEInfo *pSME=(inspectSMEInfo*)pExpBuffer ;
         if ( pSME->getBitMask( childID ) != DMS_SME_FREE )
         {
            dumpPrintf ( "*** Error: SME extent 0x%08x (%d) is not free" OSS_NEWLINE,
                         childID, childID ) ;
            ++err ;
         }
         pSME->setBitMask( childID ) ;
      }

      rc = loadExtent ( file, extentType, pageSize, childID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                      childID, rc ) ;
         ++err ;
         continue ;
      }

      pExtentHead = (ixmExtentHead*)gExtentBuffer ;
      gMBStat._totalIndexPages += 1 ;
      gMBStat._totalIndexFreeSpace += pExtentHead->_totalFreeSize ;

      indexStat._indexPages += 1 ;
      indexStat._freeSpaces += pExtentHead->_totalFreeSize ;

      if ( level > indexStat._level )
      {
         indexStat._level = level ;
      }

   retry :
      localErr = 0 ;
      keyNodes = 0 ;
      delKeyNodes = 0 ;
      builder.reset() ;
      len = dmsInspect::inspectIndexExtent ( cb, gExtentBuffer, pageSize,
                                             gBuffer, gBufferSize,
                                             collectionID, childID,
                                             level, childExtents,
                                             keyNodes, delKeyNodes,
                                             localErr,
                                             &builder ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }

      flushOutput ( gBuffer, len ) ;
      err += localErr ;
      indexStat._keyNodes += keyNodes ;
      indexStat._delKeyNodes += delKeyNodes ;
   }

done :
   return ;
error :
   goto done ;
}

// dump all extents for a given index
void dumpIndexExtents ( OSSFILE &file, UINT32 pageSize, dmsExtentID rootID,
                        UINT16 collectionID, UINT32 &totalPages )
{
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   std::set<dmsExtentID> childExtents ;
   // set to index type
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX ;
   ixmExtentHead *pExtentHead = NULL ;
   childExtents.insert( rootID ) ;

   totalPages = 0 ;

   while ( !childExtents.empty() )
   {
      dmsExtentID childID = *( childExtents.begin() ) ;
      childExtents.erase( childExtents.begin() ) ;
      ++totalPages ;

      dumpPrintf ( "    Dump Index Page %d:" OSS_NEWLINE, childID ) ;
      if ( childID == DMS_INVALID_EXTENT || (UINT32)childID >= gPageNum )
      {
         dumpPrintf ( "*** Error: index extent ID is not valid: 0x%08x (%d)" OSS_NEWLINE,
                      childID, childID ) ;
         continue ;
      }
      rc = loadExtent ( file, extentType, pageSize, childID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                      childID, rc ) ;
         continue ;
      }

      pExtentHead = (ixmExtentHead*)gExtentBuffer ;
      gMBStat._totalIndexPages += 1 ;
      gMBStat._totalIndexFreeSpace += pExtentHead->_totalFreeSize ;

   retry :
      len = dmsDump::dumpIndexExtent( gExtentBuffer, pageSize,
                                      gBuffer, gBufferSize, NULL,
                                      gDumpType,
                                      childExtents,
                                      gShowRecordContent ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      dumpPrintf ( OSS_NEWLINE ) ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectDictPageState( CHAR *pExpBuffer, dmsExtentID extentID, SINT32 &err )
{
   dmsDictExtent *dictExtent = (dmsDictExtent*)gDictBuffer ;
   inspectSMEInfo *sme = (inspectSMEInfo*)pExpBuffer ;

   for ( INT32 i = 0; i < dictExtent->_blockSize; ++i )
   {
      if ( sme->getBitMask( extentID + i ) != DMS_SME_FREE )
      {
         dumpPrintf( "*** Error: Compression Dictionary extent 0x%08x (%d) "
                     "is not free" OSS_NEWLINE,
                     extentID + i, extentID + i ) ;
         ++err ;
      }
      else
      {
         sme->setBitMask( extentID + i ) ;
      }
   }
}

INT32 inspectLobdCollection( OSSFILE &file, UINT32 pageId,
                             UINT32 sequence, SINT32 &err )
{
   INT32 rc = SDB_OK;
   SINT64 len = 0;
   SINT32 localErr = 0;
   CHAR lobMeta[ sizeof(dmsLobMeta) ] = { 0 } ;

   rc = readData( &file, DMS_HEADER_SZ + (INT64)gLobdPageSize * pageId,
                  (SINT64)sizeof(dmsLobMeta), (CHAR*)lobMeta,
                  FALSE, "lobd" ) ;
   if ( rc)
   {
      ++err ;
      goto error;
   }

retry_dmsLobMeta:
   localErr = 0;
   len = dmsInspect::inspectDmsLobMeta( (dmsLobMeta*)lobMeta, gBuffer,
                                        gBufferSize, localErr ) ;
   if (len >= gBufferSize -1 )
   {
      // if our buffer is not large enough, let's allocate more memory and
      // try again
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer();
         goto error;
      }
      goto retry_dmsLobMeta;
   }
   flushOutput( gBuffer, (UINT32)len ) ;
   err += localErr ;

done:
    return rc;
error:
    goto done;
}

INT32 inspectLobmMeta( OSSFILE &file, CHAR *pExpBuffer, UINT32 pageId,
                       CHAR *pageBuf, UINT32 pageSize, UINT16 clID,
                       UINT32 &sequence, SINT32 &err)
{
   INT32 rc = SDB_OK;
   SINT64 len = 0;
   SINT32 localErr = 0;
   dmsLobDataMapBlk *blk = (dmsLobDataMapBlk*)pageBuf;

   rc = readData( &file, DMS_BME_OFFSET + DMS_BME_SZ + (INT64)pageSize * pageId,
                  pageSize, pageBuf, TRUE, "lobm page" ) ;
   if ( rc )
   {
      ++err ;
      goto error;
   }

   sequence = blk->_sequence ;

retry_dmsLobDataMapBlk:
   localErr = 0 ;
   len = dmsInspect::inspectDmsLobDataMapBlk( blk, gBuffer, gBufferSize,
                                              pageId, pageSize, gLobdPageSize,
                                              clID, localErr );
   if ( (UINT32)len >= gBufferSize -1 )
   {
      // if our buffer is not large enough, let's allocate more memory and
      // try again
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry_dmsLobDataMapBlk;
   }
   flushOutput( gBuffer, len ) ;
   err += localErr;

   if ( 0 == localErr )
   {
      gMBStat._totalLobSize += blk->_dataLen ;
   }

   if ( pExpBuffer && DMS_LOB_PAGE_NORMAL == blk->_status )
   {
      inspectSMEInfo *pSME = (inspectSMEInfo*)pExpBuffer ;
      if ( pSME->getBitMask( pageId ) != DMS_SME_FREE )
      {
         dumpPrintf ( "*** Error: dmsLobDataMapBlk 0x%08x (%d) is refered by two. "
                      "this maybe caused a loop (prev page id: %d, "
                      "next page id: %d)" OSS_NEWLINE,
                      pageId , pageId, blk->_prevPageInBucket,
                      blk->_nextPageInBucket ) ;
         ++err ;
      }
      pSME->setBitMask( pageId ) ;
   }

done:
   return rc;
error:
   goto done;
}

void inspectCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id,
                            SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   INT32 len       = 0 ;
   INT32 tmpErr    = err ;
   SINT32 localErr = 0 ;
   std::set<dmsRecordID> extentRIDList ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DATA ;
   dmsMB *mb = NULL ;
   dmsExtentID tempExtent = DMS_INVALID_EXTENT ;
   dmsExtentID firstExtent = DMS_INVALID_EXTENT ;
   dmsExtent *pExtent = NULL ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;
   dmsCompressorEntry compressorEntry ;
   UINT64 totalRecord = 0 ;
   BOOLEAN extScan = FALSE ;
   BOOLEAN capped = FALSE ;

   UINT64 ovfNum = 0 ;
   UINT64 compressedNum = 0 ;
   UINT64 deletingNum = 0 ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      ++err ;
      goto error ;
   }

   capped = OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_CAPPED ) ;
   firstExtent = mb->_firstExtentID ;
   ossStrncpy( collectionName, mb->_collectionName, DMS_COLLECTION_NAME_SZ ) ;

   dumpPrintf ( "  Inspect Data for collection [%d : %s]" OSS_NEWLINE,
                id, collectionName ) ;

   if ( !OSS_BIT_TEST( gAction, ACTION_STAT ) && gOnlyMeta )
   {
      goto done ;
   }

   /// when the meta expand extent is valid, need to set this page to occupied
   if ( pExpBuffer &&
        DMS_INVALID_EXTENT != mb->_mbExExtentID )
   {
      dmsMetaExtent *pMetaEx = NULL ;
      extentType = INSPECT_EXTENT_TYPE_MBEX ;
      rc = loadExtent( file, extentType, pageSize, mb->_mbExExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Failed to load mb expand extent %d, rc: %d" OSS_NEWLINE,
                     mb->_mbExExtentID, rc ) ;
         goto error ;
      }
      pMetaEx = ((dmsMetaExtent*)gExtentBuffer)  ;

      inspectSMEInfo *pSME=(inspectSMEInfo*)pExpBuffer ;
      for ( INT32 i = 0 ; i < pMetaEx->_blockSize ; ++i )
      {
         if ( pSME->getBitMask( mb->_mbExExtentID + i ) != DMS_SME_FREE )
         {
            dumpPrintf ( "*** Error: Meta Expand extent 0x%08x (%d) is not free" OSS_NEWLINE,
                         mb->_mbExExtentID + i , mb->_mbExExtentID + i ) ;
            ++err ;
         }
         pSME->setBitMask( mb->_mbExExtentID + i ) ;
      }
   }

   rc = prepareCompressor( file, pageSize, mb, id, pExpBuffer, compressorEntry, err ) ;
   if ( rc )
   {
      goto error ;
   }

   extentType = INSPECT_EXTENT_TYPE_DATA ;
   // loop through all extents
   while ( DMS_INVALID_EXTENT != firstExtent )
   {
      // if one of the extent is not valid, we have to break the loop since the
      // chain is breaking
      if ( (UINT32)firstExtent >= gPageNum )
      {
         dumpPrintf ( "*** Error: data extent 0x%08x (%d) is out of range" OSS_NEWLINE,
                      firstExtent, firstExtent ) ;
         ++err ;
         break ;
      }
      rc = loadExtent ( file, extentType, pageSize, firstExtent, id ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                      firstExtent, rc ) ;
         ++err ;
         goto error ;
      }

      pExtent = (dmsExtent*)gExtentBuffer ;
      gMBStat._totalDataPages += pExtent->_blockSize ;
      gMBStat._totalDataFreeSpace += pExtent->_freeSpace ;
      gMBStat._totalRecords += pExtent->_recCount ;
      gMBStat._rcTotalRecords.add( pExtent->_recCount ) ;

      if ( pExpBuffer )
      {
         inspectSMEInfo *pSME=(inspectSMEInfo*)pExpBuffer ;

         for ( INT32 i = 0 ; i < pExtent->_blockSize ; ++i )
         {
            if ( pSME->getBitMask( firstExtent + i ) != DMS_SME_FREE )
            {
               dumpPrintf ( "*** Error: SME extent 0x%08x (%d) is not free" OSS_NEWLINE,
                            firstExtent + i , firstExtent + i ) ;
               ++err ;
            }
            pSME->setBitMask( firstExtent + i ) ;
         }
      }

      /// don't inspect data extent
      if ( !OSS_BIT_TEST ( gAction, ACTION_INSPECT ) )
      {
         firstExtent = pExtent->_nextExtent ;
         continue ;
      }
      extScan = TRUE ;

   retry_data :
      UINT64 extTotalRecord = 0 ;
      UINT64 extCompressedNum = 0 ;
      UINT64 extDeletingNum = 0 ;

      extentRIDList.clear() ;
      tempExtent = firstExtent ;
      localErr = 0 ;
      // attempt to inspect extent text, note firstExtent will be assigned to
      // next extent id, until hitting DMS_INVALID_EXTENT as end of collection
      len = dmsInspect::inspectDataExtent ( cb, gExtentBuffer,
                                            pExtent->_blockSize * pageSize,
                                            gBuffer, gBufferSize, hwm, id, tempExtent,
                                            &extentRIDList, localErr,
                                            &compressorEntry,
                                            extTotalRecord,
                                            extCompressedNum,
                                            extDeletingNum,
                                            capped ) ;
      if ( (UINT32)len >= gBufferSize-1 )
      {
         // if our buffer is not large enough, let's allocate more memory and
         // try again
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_data ;
      }

      totalRecord += extTotalRecord ;
      // compressedNum is not include the compressed records which have overflow,
      // they will be included later
      compressedNum += extCompressedNum ;
      deletingNum += extDeletingNum ;
      ovfNum += extentRIDList.size() ;

      flushOutput ( gBuffer, len ) ;

      // inspect the extent's ovf recrods
      if ( extentRIDList.size() != 0 )
      {
         extCompressedNum = 0 ;
         inspectOverflowedRecords( file, pageSize, id, firstExtent,
                                   extentRIDList, err, &compressorEntry,
                                   extCompressedNum ) ;
         // add the number of overflow compressed records
         compressedNum += extCompressedNum ;
      }

      firstExtent = tempExtent ;
      err += localErr ;
   } //end while

   /// inspect the stat info
   if ( !OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
        0 != compressedNum )
   {
      dumpPrintf ( "*** Error: Collection is not compressed, but has %llu "
                   "compressed records" OSS_NEWLINE, compressedNum ) ;
      ++err ;
   }

   if ( gMBStat._totalRecords != mb->_totalRecords )
   {
      dumpPrintf ( "*** Error: Collection records is not the same[ "
                   "mb->_totalRecords: %llu, ext total records: %llu]" OSS_NEWLINE,
                   mb->_totalRecords,
                   gMBStat._totalRecords ) ;
      ++err ;
   }

   if ( extScan && totalRecord != mb->_totalRecords )
   {
      dumpPrintf ( "*** Error: Collection records is not the same[ "
                   "mb->_totalRecords: %llu, scan total records: %llu]" OSS_NEWLINE,
                   mb->_totalRecords, totalRecord ) ;
      ++err ;
   }

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( tmpErr == err )
   {
      dumpPrintf ( "  Inspect Data for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "  Inspect Data for collection Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                   err - tmpErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      FLOAT64 freeSpaceRatio = 0.0 ;
      FLOAT64 ovfRatio = 0.0 ;
      FLOAT64 compressRatio = 0.0 ;

      if ( gMBStat._totalDataPages > 0 )
      {
         freeSpaceRatio = gMBStat._totalDataFreeSpace /
                          ( (FLOAT64)( gMBStat._totalDataPages ) * pageSize ) ;
      }

      if ( gMBStat._totalRecords > 0 )
      {
         ovfRatio = ovfNum / (FLOAT64)gMBStat._totalRecords ;
         compressRatio = compressedNum / (FLOAT64)gMBStat._totalRecords ;
      }

      dumpPrintf( "  ++++ The collection data info ++++" OSS_NEWLINE
                  "    Total Record              : %llu" OSS_NEWLINE
                  "    Total Data Pages          : %u" OSS_NEWLINE
                  "    Total Data Free Space     : %llu (%.2f%%)" OSS_NEWLINE
                  "    Total OVF Record          : %llu (%.2f%%)" OSS_NEWLINE
                  "    Total Compressed Record   : %llu (%.2f%%)" OSS_NEWLINE
                  "    Total Deleting Record     : %llu" OSS_NEWLINE OSS_NEWLINE,
                  gMBStat._totalRecords,
                  gMBStat._totalDataPages,
                  gMBStat._totalDataFreeSpace, freeSpaceRatio * 100,
                  ovfNum, ovfRatio * 100,
                  compressedNum, compressRatio * 100,
                  deletingNum ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }

   return ;
error :
   goto done ;
}

void inspectCollectionIndex( OSSFILE &file, UINT32 pageSize, UINT16 id,
                             SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   dmsMB *mb       = NULL ;
   INT32 tmpErr    = err ;
   std::map<UINT16, sdbIndexDef> indexRoots ;
   std::map<UINT16, sdbIndexDef>::iterator it ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      ++err ;
      goto error ;
   }
   ossStrncpy( collectionName, mb->_collectionName,
               DMS_COLLECTION_NAME_SZ ) ;

   dumpPrintf ( "  Inspect Index for collection [%d : %s]" OSS_NEWLINE,
                id, collectionName ) ;

   inspectIndexDef ( file, pageSize, id, mb, pExpBuffer, indexRoots, err ) ;

   if ( !OSS_BIT_TEST( gAction, ACTION_STAT ) && gOnlyMeta )
   {
      goto done ;
   }

   // after inspect index def, we should iterate all indexes
   for ( it = indexRoots.begin() ; it != indexRoots.end() ; ++it )
   {
      sdbIndexStat indexStat ;
      INT32 localErr = 0 ;
      UINT16 indexID = it->first ;
      const sdbIndexDef &idxItem = it->second ;
      FLOAT64 delKeyRatio = 0.0 ;
      FLOAT64 freeSpaceRatio = 0.0 ;

      dumpPrintf ( "    Index Inspection for Collection [%04u], Index [%02u] (%s)" OSS_NEWLINE,
                   id, indexID, idxItem._name.c_str() ) ;

      if ( DMS_INVALID_EXTENT != idxItem._root )
      {
         /// add indexCB page
         ++indexStat._indexPages ;

         inspectIndexExtents ( file, pageSize, idxItem._root, id,
                               pExpBuffer, indexStat, localErr ) ;
         err += localErr ;

         if ( indexStat._delKeyNodes > 0 )
         {
            delKeyRatio = indexStat._delKeyNodes /
                          (FLOAT64)( indexStat._delKeyNodes + indexStat._keyNodes ) ;
         }
         if ( indexStat._indexPages > 1 )
         {
            freeSpaceRatio = indexStat._freeSpaces /
                             ( (FLOAT64)( indexStat._indexPages - 1 ) * pageSize ) ;
         }
      }

      if ( 0 == localErr )
      {
         dumpPrintf ( "    Index Inspection Done Succeed" OSS_NEWLINE
                      "      IndexPage        : %u" OSS_NEWLINE
                      "      TotalFreeSpace   : %llu (%.2f%%)" OSS_NEWLINE
                      "      MaxLevel         : %u" OSS_NEWLINE
                      "      KeyNodeNum       : %llu" OSS_NEWLINE
                      "      DelKeyNodeNum    : %llu (%.2f%%)" OSS_NEWLINE OSS_NEWLINE,
                      indexStat._indexPages,
                      indexStat._freeSpaces, freeSpaceRatio * 100,
                      indexStat._level,
                      indexStat._keyNodes,
                      indexStat._delKeyNodes, delKeyRatio * 100 ) ;
      }
      else
      {
         dumpPrintf ( "    Index Inspection Done with Errors: %d" OSS_NEWLINE
                      "      IndexPage        : %u" OSS_NEWLINE
                      "      TotalFreeSpace   : %llu (%.2f%%)" OSS_NEWLINE
                      "      MaxLevel         : %u" OSS_NEWLINE
                      "      KeyNodeNum       : %llu" OSS_NEWLINE
                      "      DelKeyNodeNum    : %llu (%.2f%%)" OSS_NEWLINE OSS_NEWLINE,
                      localErr,
                      indexStat._indexPages,
                      indexStat._freeSpaces, freeSpaceRatio * 100,
                      indexStat._level,
                      indexStat._keyNodes,
                      indexStat._delKeyNodes, delKeyRatio * 100 ) ;
      }
   }

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( tmpErr == err )
   {
      dumpPrintf ( "  Inspect Index for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "  Inspect Index for collection Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                   err - tmpErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      FLOAT64 freeSpaceRatio = 0.0 ;
      if ( gMBStat._totalIndexPages > indexRoots.size() )
      {
         freeSpaceRatio = gMBStat._totalIndexFreeSpace /
                          ( (FLOAT64)( gMBStat._totalIndexPages - indexRoots.size() ) * pageSize ) ;
      }

      dumpPrintf( "  ++++ The collection index info ++++" OSS_NEWLINE
                  "    Total Index Pages        : %u" OSS_NEWLINE
                  "    Total Index Free Space   : %llu (%.2f%%)" OSS_NEWLINE
                  "    Unique Index Number      : %u" OSS_NEWLINE
                  "    Global Index Number      : %u" OSS_NEWLINE OSS_NEWLINE,
                  gMBStat._totalIndexPages,
                  gMBStat._totalIndexFreeSpace, freeSpaceRatio * 100,
                  gMBStat._uniqueIdxNum,
                  gMBStat._globIdxNum ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }

   return ;
error :
   goto done ;
}

void inspectCollectionLob( OSSFILE &lobmFile, UINT32 pageSize,
                           UINT16 clId, SINT32 hwm,
                           CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc             = SDB_OK ;
   INT32 beginPageID    = DMS_LOB_INVALID_PAGEID ;
   INT32 endPageID      = DMS_LOB_INVALID_PAGEID ;
   UINT32 sequence      = 0 ;
   INT32 tmpErr         = err ;
   MAP_PAGES *pMapPages = NULL ;
   dmsMB *mb = NULL ;
   dmsLobDataMapBlk blk ;
   MAP_CL_PAGES::iterator it ;
   MAP_PAGES::iterator itMap ;
   /// page crc check counters
   UINT64 crcPass       = 0 ;
   UINT64 crcFail       = 0 ;
   UINT64 crcSkip       = 0 ;
   CHAR  *lobdCrcBuf    = NULL ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ; /// sec

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( clId, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      ++err ;
      goto error ;
   }

   dumpPrintf ( "  Inspect Lob for collection [%d : %s]" OSS_NEWLINE,
                clId, mb->_collectionName ) ;

   it = gMapLobCLPages.find( clId ) ;
   if ( it == gMapLobCLPages.end() )
   {
      /// not found the collection's lob pages
      goto done ;
   }

   pMapPages = &( it->second ) ;

   itMap = pMapPages->begin() ;
   while( itMap != pMapPages->end() )
   {
      beginPageID = itMap->first ;
      endPageID = itMap->second ;
      ++itMap ;

      while( beginPageID <= endPageID )
      {
         rc = inspectLobmMeta( lobmFile, pExpBuffer, beginPageID, (CHAR*)&blk,
                               pageSize, clId, sequence, err ) ;
         if ( rc )
         {
            goto error ;
         }

         ++gMBStat._totalLobPages ;
         if ( DMS_LOB_META_SEQUENCE == sequence )
         {
            ++gMBStat._totalLobs ;
         }

         /// verify the data page crc when the blk carries a full crc.
         /// crc verification is opt-in ( --crccheck true ): it reads back
         /// every data page and recomputes crc, which is much slower than a
         /// plain inspect, so keep it off by default.
         /// guard _dataLen against the page size: inspect may run on a
         /// corrupt file, a bad _dataLen must not overflow the read buffer
         /// ( such a blk is already flagged by inspectDmsLobDataMapBlk ).
         if ( gCrcCheck && !gOnlyMeta && blk.isCrcValid() &&
              blk._dataLen > 0 && blk._dataLen <= gLobdPageSize )
         {
            if ( NULL == lobdCrcBuf )
            {
               lobdCrcBuf = ( CHAR * )SDB_OSS_MALLOC( gLobdPageSize ) ;
               if ( NULL == lobdCrcBuf )
               {
                  rc = SDB_OOM ;
                  dumpPrintf( "*** Error: Failed to alloc lobd crc buffer" OSS_NEWLINE ) ;
                  ++err ;
                  goto error ;
               }
            }
            rc = readData( &gLobdFile,
                           DMS_HEADER_SZ + (INT64)gLobdPageSize * beginPageID,
                           blk._dataLen, lobdCrcBuf, FALSE, "lobd crc" ) ;
            if ( rc )
            {
               ++err ;
               goto error ;
            }
            UINT32 actualCrc = utilCrc32c( lobdCrcBuf, blk._dataLen ) ;
            if ( actualCrc == blk._crc )
            {
               ++crcPass ;
            }
            else
            {
               ++crcFail ;
               ++err ;
               dumpPrintf( "*** Error: Lob data page(%d) crc mismatch, "
                           "expect[0x%08x] actual[0x%08x] dataLen[%u]" OSS_NEWLINE,
                           beginPageID, blk._crc, actualCrc, blk._dataLen ) ;
            }
         }
         else if ( gCrcCheck && !gOnlyMeta )
         {
            ++crcSkip ;
         }

         if ( gOnlyMeta )
         {
            ++beginPageID ;
            continue ;
         }

         // just inspect lobmeta in page 0
         if ( DMS_LOB_META_SEQUENCE == sequence )
         {
            rc = inspectLobdCollection( gLobdFile, beginPageID, sequence, err ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         ++beginPageID ;
      }
   }

   gMapLobCLPages.erase( it ) ;

   /// check lob info with mb
   if ( gMBStat._totalLobPages != mb->_totalLobPages )
   {
      dumpPrintf( "*** Error: Total lob pages is not the same, "
                  "MB->_totalLobPages: %u, MBStat->_totalLobPages: %u" OSS_NEWLINE,
                  mb->_totalLobPages, gMBStat._totalLobPages ) ;
      ++err ;
   }

   if ( gMBStat._totalLobs != mb->_totalLobs )
   {
      dumpPrintf( "*** Error: Total lobs is not the same, "
                  "MB->_totalLobs: %llu, MBStat->_totalLobPages: %llu" OSS_NEWLINE,
                  mb->_totalLobs, gMBStat._totalLobs ) ;
      ++err ;
   }

done:
   if ( NULL != lobdCrcBuf )
   {
      SDB_OSS_FREE( lobdCrcBuf ) ;
      lobdCrcBuf = NULL ;
   }
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( tmpErr == err )
   {
      dumpPrintf( "  Inspect Lob for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                  timeSpan ) ;
   }
   else
   {
      dumpPrintf( "  Inspect Lob for collection Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                  err - tmpErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      FLOAT64 lobUsageRate = 0.0 ;

      if ( gMBStat._totalLobPages > 0 && gLobdPageSize > 0 )
      {
         lobUsageRate = gMBStat._totalLobSize / ( (FLOAT64)gMBStat._totalLobPages * gLobdPageSize ) ;
      }

      dumpPrintf( "  ++++ The collection lob info ++++" OSS_NEWLINE
                  "    Collection ID     : %u" OSS_NEWLINE
                  "    Total Lob Pages   : %u" OSS_NEWLINE
                  "    Total Lob Size    : %llu" OSS_NEWLINE
                  "    Lob Usage Rate    : %.2f%%" OSS_NEWLINE
                  "    Total Lobs        : %llu" OSS_NEWLINE,
                  clId,
                  gMBStat._totalLobPages,
                  gMBStat._totalLobSize,
                  lobUsageRate * 100,
                  gMBStat._totalLobs ) ;

      if ( !gOnlyMeta )
      {
         if ( gCrcCheck )
         {
            dumpPrintf( "    Page CRC Check    : Pass %llu, Fail %llu, "
                        "NoCRC %llu" OSS_NEWLINE,
                        crcPass, crcFail, crcSkip ) ;
         }
         else
         {
            dumpPrintf( "    Page CRC Check    : skipped "
                        "(use --crccheck true)" OSS_NEWLINE ) ;
         }
      }
      dumpPrintf( OSS_NEWLINE ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }
   return ;
error:
   goto done ;
}

void inspectCollection ( OSSFILE &file, UINT32 pageSize, UINT16 id,
                         SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
   gMBStat.reset() ;

   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      inspectCollectionData( file, pageSize, id, hwm, pExpBuffer, err ) ;
      gInspectFileStat._totalFreeSize += gMBStat._totalDataFreeSpace ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      inspectCollectionIndex( file, pageSize, id, hwm, pExpBuffer, err ) ;
      gInspectFileStat._totalFreeSize += gMBStat._totalIndexFreeSpace ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      inspectCollectionLob( file, pageSize, id, hwm, pExpBuffer, err ) ;
   }
}

static void _checkCommonExtent( UINT32 pageSize, const dmsMB *pMB,
                                const dmsExtent *pExtent,
                                const dmsSpaceManagementExtent *pSMEBitmap,
                                dmsExtentID &extID,
                                MAP_PAGES &mapPages )
{
   if ( pExtent->_blockSize <= 0 ||
        (UINT64)pExtent->_blockSize * pageSize > gSegmentSize ||
        DMS_EXTENT_FLAG_FREED == pExtent->_flag )
   {
      extID += 1 ;
   }
   else if ( pExtent->_mbID != pMB->_blockID )
   {
      /// when the page is valid
      if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( extID ) )
      {
         extID += pExtent->_blockSize ;
      }
      else
      {
         extID += 1 ;
      }
   }
   else
   {
      /// extent is in used
      if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( extID ) )
      {
         mapPages[ extID ] = extID + pExtent->_blockSize - 1 ;
         extID += pExtent->_blockSize ;
      }
      else
      {
         INT32 idx = 1 ;
         while( idx < pExtent->_blockSize )
         {
            /// some page has be allocated
            if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( extID + idx ) )
            {
               break ;
            }
            ++idx ;
         }
   
         /// extent invalid
         if ( idx < pExtent->_blockSize )
         {
            extID += 1 ;
         }
         else
         {
            mapPages[ extID ] = extID + pExtent->_blockSize - 1 ;
            extID += pExtent->_blockSize ;
         }
      }
   }
}

static std::string _formatPages( MAP_PAGES &mapPages )
{
   std::stringstream ss ;

   if ( mapPages.empty() )
   {
      ss << "-" ;
   }
   else if ( 1 == mapPages.size() )
   {
      ss << mapPages.begin()->first ;
   }
   else
   {
      MAP_PAGES::iterator it = mapPages.begin() ;
      UINT32 idx = 0 ;
      ss << "[" ;
      while( it != mapPages.end() )
      {
         if ( idx > 0 )
         {
            ss << ", " ;
         }
         ss << it->first ;

         ++it ;
         ++idx ;
      }
      ss << "] *** Warning" ;
   }

   return ss.str() ;
}

static UINT32 _setBitmapByPages( dmsSpaceManagementExtent *pSMEBitmap, MAP_PAGES &mapPages  )
{
   UINT32 hasSetPages = 0 ;
   MAP_PAGES::iterator it = mapPages.begin() ;
   while( it != mapPages.end() )
   {
      for ( INT32 startPages = it->first ; startPages <= it->second ; ++startPages )
      {
         if ( DMS_SME_FREE == pSMEBitmap->getBitMask( startPages ) )
         {
            pSMEBitmap->setBitMask( startPages ) ;
            ++hasSetPages ;
         }
      }
      ++it ;
   }
   return hasSetPages ;
}

INT32 getBME( OSSFILE &lobmFile, CHAR *pBME )
{
   return readData( &lobmFile, DMS_BME_OFFSET, DMS_BME_SZ, pBME, FALSE, "bme" ) ;
}

void recoverCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id,
                            dmsMB *pMB, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   INT32 tmpErr    = err ;
   dmsExtent *pExtent = NULL ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   dmsSpaceManagementExtent *pSMEBitmap = ( dmsSpaceManagementExtent* )gSMEBuff ;
   dmsExtentID startExtent = 0 ;
   dmsExtentID firstExtent = DMS_INVALID_EXTENT ;
   dmsExtentID lastExtent  = DMS_INVALID_EXTENT ;
   UINT16 tmpFlag = 0 ;

   MAP_PAGE_RELATION pageRele ;
   MAP_PAGES         mapFreePages ;
   MAP_PAGES         mapUsedPages ;
   MAP_PAGES         mapMetaPages ;
   MAP_PAGES         mapDictPages ;
   MAP_PAGES         mapOptPages ;

   std::set<dmsExtentID>   exceptionPages ;

   UINT32            freePagesNum = 0 ;
   UINT32            usedPagesNum = 0 ;
   UINT32            freeExtentNum= 0 ;
   UINT32            usedExtentNum= 0 ;
   UINT32            hasSetPage   = 0 ;

   BOOLEAN           needForce = FALSE ;
   BOOLEAN           doRecover = FALSE ;
   SINT64            hasWritten = 0 ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   beginTime = ossGetCurrentMilliseconds() ;
   ossStrncpy( collectionName, pMB->_collectionName, DMS_COLLECTION_NAME_SZ ) ;

   dumpPrintf ( "  Recover Data for collection [%d : %s]" OSS_NEWLINE,
                id, collectionName ) ;

   /// scan the extent
   while( startExtent < (INT32)gPageNum )
   {
      rc = getExtent ( file, startExtent, pageSize, 1, FALSE ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to get extent %d, rc: %d" OSS_NEWLINE,
                      startExtent, rc ) ;
         ++err ;
         goto error ;
      }

      pExtent = (dmsExtent*)gExtentBuffer ;

      /// check the extent
      if ( pExtent->_eyeCatcher[0] == DMS_EXTENT_EYECATCHER0 &&
           pExtent->_eyeCatcher[1] == DMS_EXTENT_EYECATCHER1 )
      {
         if ( pExtent->_blockSize <= 0 ||
              (UINT64)pExtent->_blockSize * pageSize > gSegmentSize ||
              DMS_EXTENT_FLAG_FREED == pExtent->_flag )
         {
            /// invalid page
            startExtent += 1 ;
         }
         else if ( pExtent->_mbID != pMB->_blockID )
         {
            /// when the page is valid
            if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( startExtent ) )
            {
               startExtent += pExtent->_blockSize ;
            }
            else
            {
               startExtent += 1 ;
            }
         }
         else
         {
            dmsExtentID prevExtent = DMS_INVALID_EXTENT ;
            dmsExtentID nextExtent = DMS_INVALID_EXTENT ;

            /// extent is in used
            if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( startExtent ) )
            {
               if ( gForce || mapUsedPages.empty() )
               {
                  mapUsedPages[ startExtent ] = startExtent + pExtent->_blockSize - 1 ;
               }
               else
               {
                  MAP_PAGES::reverse_iterator rit = mapUsedPages.rbegin() ;
                  if ( rit != mapUsedPages.rend() && rit->second + 1 == startExtent )
                  {
                     /// update
                     rit->second += pExtent->_blockSize ;
                  }
                  else
                  {
                     mapUsedPages[ startExtent ] = startExtent + pExtent->_blockSize - 1 ;
                  }
               }
               usedPagesNum += pExtent->_blockSize ;
               ++usedExtentNum ;
            }
            else
            {
               INT32 idx = 1 ;
               while( idx < pExtent->_blockSize )
               {
                  /// some page has be allocated
                  if ( DMS_SME_ALLOCATED == pSMEBitmap->getBitMask( startExtent + idx ) )
                  {
                     break ;
                  }
                  ++idx ;
               }

               /// extent invalid
               if ( idx < pExtent->_blockSize )
               {
                  startExtent += 1 ;
                  continue ;
               }

               if ( gForce || mapFreePages.empty() )
               {
                  mapFreePages[ startExtent ] = startExtent + pExtent->_blockSize - 1 ;
               }
               else
               {
                  MAP_PAGES::reverse_iterator rit = mapFreePages.rbegin() ;
                  if ( rit != mapFreePages.rend() && rit->second + 1 == startExtent )
                  {
                     /// update
                     rit->second += pExtent->_blockSize ;
                  }
                  else
                  {
                     mapFreePages[ startExtent ] = startExtent + pExtent->_blockSize - 1 ;
                  }
               }

               freePagesNum += pExtent->_blockSize ;
               ++freeExtentNum ;
            }

            /// calc stat info
            gMBStat._totalDataPages += pExtent->_blockSize ;
            gMBStat._totalRecords += pExtent->_recCount ;

            /// process the relation
            prevExtent = pExtent->_prevExtent ;
            nextExtent = pExtent->_nextExtent ;

            if ( DMS_INVALID_EXTENT == prevExtent && DMS_INVALID_EXTENT == nextExtent )
            {
               if ( DMS_INVALID_EXTENT == firstExtent )
               {
                  firstExtent = startExtent ;
               }
               else
               {
                  dumpPrintf ( "*** Error: Conflict first extent(%d) with %d" OSS_NEWLINE,
                               firstExtent, startExtent ) ;
                  ++err ;
                  exceptionPages.insert( startExtent ) ;
               }

               if ( DMS_INVALID_EXTENT == lastExtent )
               {
                  lastExtent = startExtent ;
               }
               else
               {
                  dumpPrintf ( "*** Error: Conflict last extent(%d) with %d" OSS_NEWLINE,
                               lastExtent, startExtent ) ;
                  ++err ;
                  exceptionPages.insert( startExtent ) ;
               }
            }
            else
            {
               if ( DMS_INVALID_EXTENT != prevExtent )
               {
                  /// push to relation
                  if ( prevExtent > startExtent )
                  {
                     pageRele[ MAKE_PAGE_REL_KEY( startExtent, SDB_PAGE_PREV ) ] = prevExtent ;
                  }
                  /// release from relation
                  else
                  {
                     MAP_PAGE_RELATION::iterator it ;
                     it = pageRele.find( MAKE_PAGE_REL_KEY( prevExtent, SDB_PAGE_NEXT ) ) ;
                     if ( it == pageRele.end() )
                     {
                        dumpPrintf( "*** Error: Extent(%d)'s prev(%d) occur incorrect" OSS_NEWLINE,
                                    startExtent, prevExtent ) ;
                        ++err ;
                        exceptionPages.insert( startExtent ) ;
                     }
                     else if ( it->second != startExtent )
                     {
                        dumpPrintf( "*** Error: Extent(%d)'s prev(%d) is not its next(%d)" OSS_NEWLINE,
                                    startExtent, prevExtent, it->second ) ;
                        ++err ;
                        exceptionPages.insert( startExtent ) ;
                     }
                     else
                     {
                        /// correct, remove
                        pageRele.erase( it ) ;
                     }
                  }
               }
               else
               {
                  if ( DMS_INVALID_EXTENT == firstExtent )
                  {
                     firstExtent = startExtent ;
                  }
                  else
                  {
                     dumpPrintf ( "*** Error: Conflict first extent(%d) with %d" OSS_NEWLINE,
                                  firstExtent, startExtent ) ;
                     ++err ;
                     exceptionPages.insert( startExtent ) ;
                  }
               }

               if ( DMS_INVALID_EXTENT != nextExtent )
               {
                  /// push to relation
                  if ( nextExtent > startExtent )
                  {
                     pageRele[ MAKE_PAGE_REL_KEY( startExtent, SDB_PAGE_NEXT ) ] = nextExtent ;
                  }
                  /// release from relation
                  else
                  {
                     MAP_PAGE_RELATION::iterator it ;
                     it = pageRele.find( MAKE_PAGE_REL_KEY( nextExtent, SDB_PAGE_PREV ) ) ;
                     if ( it == pageRele.end() )
                     {
                        dumpPrintf( "*** Error: Extent(%d)'s next(%d) occur incorrect" OSS_NEWLINE,
                                    startExtent, nextExtent ) ;
                        ++err ;
                        exceptionPages.insert( startExtent ) ;
                     }
                     else if ( it->second != startExtent )
                     {
                        dumpPrintf( "*** Error: Extent(%d)'s next(%d) is not its prev(%d)" OSS_NEWLINE,
                                    startExtent, nextExtent, it->second ) ;
                        ++err ;
                        exceptionPages.insert( startExtent ) ;
                     }
                     else
                     {
                        /// correct, remove
                        pageRele.erase( it ) ;
                     }
                  }
               }
               else
               {
                  if ( DMS_INVALID_EXTENT == lastExtent )
                  {
                     lastExtent = startExtent ;
                  }
                  else
                  {
                     dumpPrintf ( "*** Error: Conflict last extent(%d) with %d" OSS_NEWLINE,
                                  lastExtent, startExtent ) ;
                     ++err ;
                     exceptionPages.insert( startExtent ) ;
                  }
               }
            }
         }

         startExtent += pExtent->_blockSize ;
      }
      else if ( pExtent->_eyeCatcher[0] == DMS_META_EXTENT_EYECATCHER0 &&
                pExtent->_eyeCatcher[1] == DMS_META_EXTENT_EYECATCHER1 )
      {
         _checkCommonExtent( pageSize, pMB, pExtent, pSMEBitmap, startExtent, mapMetaPages ) ;
      }
      else if ( pExtent->_eyeCatcher[0] == DMS_DICT_EXTENT_EYECATCHER0 &&
                pExtent->_eyeCatcher[1] == DMS_DICT_EXTENT_EYECATCHER1 )
      {
         _checkCommonExtent( pageSize, pMB, pExtent, pSMEBitmap, startExtent, mapDictPages ) ;
      }
      else if ( pExtent->_eyeCatcher[0] == DMS_OPT_EXTENT_EYECATCHER0 &&
                pExtent->_eyeCatcher[1] == DMS_OPT_EXTENT_EYECATCHER1 )
      {
         _checkCommonExtent( pageSize, pMB, pExtent, pSMEBitmap, startExtent, mapOptPages ) ;
      }
      else
      {
         startExtent += 1 ;
      }
   }

   /// check page rele
   {
      MAP_PAGE_RELATION::iterator itRele = pageRele.begin() ;
      while( itRele != pageRele.end() )
      {
         dumpPrintf( "*** Error: Extent(%d)'s %s(%d) occur incorrect link" OSS_NEWLINE,
                     GET_PAGEID_FROM_KEY( itRele->first ),
                     SDB_PAGE_PREV == GET_LISTTYPE_FROM_KEY( itRele->first ) ? "prev" : "next",
                     itRele->second ) ;
         ++err ;
         exceptionPages.insert( itRele->first ) ;
         ++itRele ;
      }
      pageRele.clear() ;
   }

   /// dump recover analysis info
   dumpPrintf( "  ++++ The collection data recovery analysis info ++++" OSS_NEWLINE
               "    Found Allocated Pages     : %u" OSS_NEWLINE
               "    Found Allocated Extents   : %u" OSS_NEWLINE
               "    Found Freed Pages         : %u" OSS_NEWLINE
               "    Found Freed Extents       : %u" OSS_NEWLINE
               "    Found Dict ExtentID       : %s" OSS_NEWLINE
               "    Found Meta ExtentID       : %s" OSS_NEWLINE
               "    Found Option ExtentID     : %s" OSS_NEWLINE
               "    Found First ExtentID      : %d" OSS_NEWLINE
               "    Found Last ExtentID       : %d" OSS_NEWLINE
               "    Found Total Records       : %llu (%ssame with %llu) " OSS_NEWLINE
               "    Found Total Pages         : %u (%ssame with %u)" OSS_NEWLINE
               "    Conflict Extent Count     : %u" OSS_NEWLINE OSS_NEWLINE,
               usedPagesNum, usedExtentNum,
               freePagesNum, freeExtentNum,
               _formatPages( mapDictPages ).c_str(),
               _formatPages( mapMetaPages ).c_str(),
               _formatPages( mapOptPages ).c_str(),
               firstExtent, lastExtent,
               gMBStat._totalRecords,
               gMBStat._totalRecords == pMB->_totalRecords ? "" : "*** not the ",
               pMB->_totalRecords,
               gMBStat._totalDataPages,
               gMBStat._totalDataPages == pMB->_totalDataPages ? "" : "*** not the ",
               pMB->_totalDataPages,
               exceptionPages.size() ) ;

   if ( !exceptionPages.empty() || mapDictPages.size() > 1 )
   {
      needForce = TRUE ;
   }
   else if ( mapMetaPages.size() > 1 || mapOptPages.size() > 1 )
   {
      needForce = TRUE ;
   }
   else if ( gMBStat._totalDataPages > 0 && ( DMS_INVALID_EXTENT == firstExtent ||
                                              DMS_INVALID_EXTENT == lastExtent ) )
   {
      needForce = TRUE ;
   }

   if ( needForce )
   {
      if ( !gForce )
      {
         dumpPrintf ( "*** Waring: Unexpected items exist. You need use '--%s' option to force "
                      "recovery. After recovery, you can use '--%s' to fix "
                      "non-compliant items" OSS_NEWLINE,
                      OPTION_FORCE, OPTION_REPAIRE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      dumpPrintf ( "### Note: Unexpected items exist. After recovery, you can use '--%s' to fix "
                   "non-compliant items" OSS_NEWLINE,
                   OPTION_REPAIRE ) ;
   }

   /// begin to recover
   /// 1. set bitmap
   hasSetPage += _setBitmapByPages( pSMEBitmap, mapDictPages ) ;
   hasSetPage += _setBitmapByPages( pSMEBitmap, mapMetaPages ) ;
   hasSetPage += _setBitmapByPages( pSMEBitmap, mapOptPages ) ;
   hasSetPage += _setBitmapByPages( pSMEBitmap, mapFreePages ) ;

   dumpPrintf ( "   Set %u pages SME Bitmap from [%d] => [%d]" OSS_NEWLINE,
                hasSetPage, DMS_SME_FREE, DMS_SME_ALLOCATED ) ;

   /// 2. set meta pages
   if ( DMS_INVALID_EXTENT == pMB->_dictExtentID && !mapDictPages.empty() )
   {
      dumpPrintf ( "   Set Dict ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_dictExtentID, mapDictPages.begin()->first ) ;
      pMB->_dictExtentID = mapDictPages.begin()->first ;
   }

   if ( DMS_INVALID_EXTENT == pMB->_mbExExtentID && !mapMetaPages.empty() )
   {
      dumpPrintf ( "   Set MetaEx ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_mbExExtentID, mapMetaPages.begin()->first ) ;
      pMB->_mbExExtentID = mapMetaPages.begin()->first ;
   }

   if ( DMS_INVALID_EXTENT == pMB->_mbOptExtentID && !mapOptPages.empty() )
   {
      dumpPrintf ( "   Set Opt ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_mbOptExtentID, mapOptPages.begin()->first ) ;
      pMB->_mbOptExtentID = mapOptPages.begin()->first ;
   }

   /// 3. set data extent link
   if ( 0 == gMBStat._totalDataPages ||
        ( exceptionPages.empty() &&
          DMS_INVALID_EXTENT != firstExtent &&
          DMS_INVALID_EXTENT != lastExtent ) )
   {
      dumpPrintf ( "   Set First ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_firstExtentID, firstExtent ) ;
      pMB->_firstExtentID = firstExtent ;

      dumpPrintf ( "   Set Last ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_lastExtentID, lastExtent ) ;
      pMB->_lastExtentID = lastExtent ;
   }
   else
   {
      dmsExtentID prevExtent = DMS_INVALID_EXTENT ;
      dmsExtentID nextExtent = DMS_INVALID_EXTENT ;
      startExtent = DMS_INVALID_EXTENT ;

      /// rebuild extent link
      MAP_PAGES::iterator it = mapUsedPages.begin() ;
      while( it != mapUsedPages.begin() )
      {
         mapFreePages[ it->first ] = it->second ;
         ++it ;
      }

      it = mapFreePages.begin() ;
      while( it != mapFreePages.end() )
      {
         prevExtent = startExtent ;
         startExtent = it->first ;
         ++it ;

         if ( it != mapFreePages.end() )
         {
            nextExtent = it->first ;
         }
         else
         {
            nextExtent = DMS_INVALID_EXTENT ;
         }

         rc = getExtent ( file, startExtent, pageSize, 1, FALSE ) ;
         if ( rc )
         {
            dumpPrintf ( "*** Error: Failed to get extent %d, rc: %d" OSS_NEWLINE,
                         startExtent, rc ) ;
            ++err ;
            goto error ;
         }

         pExtent = (dmsExtent*)gExtentBuffer ;

         if ( pExtent->_prevExtent != prevExtent )
         {
            dumpPrintf ( "   Set Extent(%d)'s PrevExtent from [%d] => [%d]" OSS_NEWLINE,
                         startExtent, pExtent->_prevExtent, prevExtent ) ;
            pExtent->_prevExtent = prevExtent ;
         }

         if ( pExtent->_nextExtent != nextExtent )
         {
            dumpPrintf ( "   Set Extent(%d)'s NextExtent from [%d] => [%d]" OSS_NEWLINE,
                         startExtent, pExtent->_nextExtent, nextExtent ) ;
            pExtent->_nextExtent = nextExtent ;
         }

         if ( DMS_INVALID_EXTENT == prevExtent )
         {
            firstExtent = startExtent ;
         }
         if ( DMS_INVALID_EXTENT == nextExtent )
         {
            lastExtent = startExtent ;
         }

         /// write data
         rc = ossSeekAndWriteN( &file, gDataOffset + pageSize * startExtent,
                                gExtentBuffer, DMS_EXTENT_METADATA_SZ, hasWritten ) ;
         if ( rc )
         {
            dumpPrintf ( "*** Error: Failed to write extent %d, rc: %d" OSS_NEWLINE,
                         startExtent, rc ) ;
            ++err ;
            goto error ;
         }
      }

      dumpPrintf ( "   Rebuild Extent Link Complete, FirstExtent: %d, LastExtent: %d" OSS_NEWLINE,
                   firstExtent, lastExtent ) ;

      dumpPrintf ( "   Set First ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_firstExtentID, firstExtent ) ;
      pMB->_firstExtentID = firstExtent ;

      dumpPrintf ( "   Set Last ExtentID from [%d] => [%d]" OSS_NEWLINE,
                   pMB->_lastExtentID, lastExtent ) ;
      pMB->_lastExtentID = lastExtent ;
   }

   /// 4. set flag, and inc _logicalID
   tmpFlag = pMB->_flag ;
   DMS_SET_MB_INUSE( tmpFlag ) ;
   dumpPrintf ( "   Set Flag from [%d] => [%d]" OSS_NEWLINE,
                pMB->_flag, tmpFlag ) ;
   pMB->_flag = tmpFlag ;

   dumpPrintf ( "   Set LogicalID from [%d] => [%d]" OSS_NEWLINE,
                pMB->_logicalID, pMB->_logicalID + 1 ) ;
   ++( pMB->_logicalID ) ;

   /// 5. save mb
   rc = ossSeekAndWriteN( &file, DMS_MME_OFFSET + id * sizeof( dmsMB ),
                          (const CHAR*)pMB, sizeof( dmsMB ), hasWritten ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to write metablock %u, rc: %d" OSS_NEWLINE,
                   id, rc ) ;
      ++err ;
      goto error ;
   }

   /// 6. save sme
   if ( hasSetPage > 0 )
   {
      rc = ossSeekAndWriteN( &file, DMS_SME_OFFSET,
                             (const CHAR*)gSMEBuff, DMS_SME_SZ, hasWritten ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to write SME, rc: %d" OSS_NEWLINE, rc ) ;
         ++err ;
         goto error ;
      }
   }

   /// 7. update header
   ++gHeader._numMB ;
   rc = ossSeekAndWriteN( &file, DMS_HEADER_OFFSET,
                          (const CHAR*)&gHeader, DMS_HEADER_SZ, hasWritten ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to write Header, rc: %d" OSS_NEWLINE, rc ) ;
      ++err ;
      goto error ;
   }

   // 8. remove the meta file
   {
      string csFileName = rtnMakeSUFileName( gCSName, 1, DMS_METAFILE_NAME_POSIX ) ;
      string csFullName = rtnFullPathName( gDatabasePath, csFileName ) ;
      if ( SDB_OK == ossAccess( csFullName.c_str(), F_OK ) )
      {
         dumpPrintf ( "   Delete meta file[%s]" OSS_NEWLINE, csFullName.c_str() ) ;
         ossDelete( csFullName.c_str() ) ;
      }
   }

   doRecover = TRUE ;
   gDoRecoverMBID = id ;

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( doRecover )
   {
      if ( tmpErr == err )
      {
         dumpPrintf ( "  Recover Data for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                      timeSpan ) ;
      }
      else
      {
         dumpPrintf ( "  Recover Data for collection Done Succeed with Warings: %d   [%.2f s]" OSS_NEWLINE,
                      err - tmpErr, timeSpan ) ;
      }
   }
   else
   {
      if ( tmpErr == err )
      {
         dumpPrintf ( "  Recover Data for collection Not Complete   [%.2f s]" OSS_NEWLINE,
                      timeSpan ) ;
      }
      else
      {
         dumpPrintf ( "  Recover Data for collection Not Complete with Errors: %d   [%.2f s]" OSS_NEWLINE,
                      err - tmpErr, timeSpan ) ;
      }
   }

   dumpPrintf( OSS_NEWLINE ) ;
   return ;
error :
   goto done ;
}

#define LOB_SEGMENT_SIZE         ( 64 * 1024 * 1024 )

#define DMS_LOB_GET_HASH_FROM_BLK( blk, hash )\
        do\
        {\
           (hash) = utilLobHash( (blk)->_oid, (blk)->_sequence,\
                                 gLobHashType() ) ;\
        } while( FALSE )

static dmsLobDataMapBlk* _getLobBlk( ossMmapFile &lobmMapFile, UINT32 pageID )
{
   UINT32 segID = pageID / LOB_SEGMENT_SIZE ;
   ossValuePtr ptr = lobmMapFile.getSegmentInfo( segID ) ;
   if ( 0 == ptr )
   {
      return NULL ;
   }
   return (dmsLobDataMapBlk*)ptr + ( pageID % LOB_SEGMENT_SIZE ) ;
}

static UINT32 _getLobBucketByHash( UINT32 hash )
{
   return hash & DMS_BUCKETS_MODULO ;
}

static INT32 _push2Bucket( UINT32 bucket,
                           DMS_LOB_PAGEID pageId,
                           _dmsLobDataMapBlk &blk,
                           const dmsLobRecord *pRecord,
                           dmsBucketsManagementExtent *pBMEMgr,
                           ossMmapFile &lobmMapFile,
                           BOOLEAN &hasUpdateBME )
{
   INT32 rc = SDB_OK ;

   DMS_LOB_PAGEID &pageInBucket = pBMEMgr->_buckets[bucket] ;
   /// empty bucket
   if ( DMS_LOB_INVALID_PAGEID == pageInBucket )
   {
      pageInBucket = pageId ;
      blk._prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
      blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
      hasUpdateBME = TRUE ;
   }
   /// neet to find the last one
   else
   {
      DMS_LOB_PAGEID tmpPage = pageInBucket ;
      _dmsLobDataMapBlk *lastBlk = NULL ;
      do
      {
         lastBlk = _getLobBlk( lobmMapFile, tmpPage ) ;
         if ( !lastBlk )
         {
            dumpPrintf ( "*** Error: Read page(%d) failed" OSS_NEWLINE, tmpPage ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         /// check exist
         if ( pRecord &&
              blk._clLogicalID == lastBlk->_clLogicalID &&
              lastBlk->equals( pRecord->_oid->getData(),
                               pRecord->_sequence ) )
         {
            rc = SDB_LOB_SEQUENCE_EXISTS ;
            goto error ;
         }

         if ( DMS_LOB_INVALID_PAGEID == lastBlk->_nextPageInBucket )
         {
            lastBlk->_nextPageInBucket = pageId ;
            blk._prevPageInBucket = tmpPage ;
            blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
            break ;
         }
         else
         {
            tmpPage = lastBlk->_nextPageInBucket ;
            continue ;
         }
      } while ( TRUE ) ;
   }
   /// set page to normal
   blk.setNormal() ;

done:
   return rc ;
error:
   goto done ;
}

void recoverCollectionLob( OSSFILE &lobmFile, UINT32 pageSize,
                           UINT16 clId, dmsMB *pMB, SINT32 &err )
{
   INT32 rc             = SDB_OK ;
   INT32 tmpErr         = err ;

   dmsLobDataMapBlk blk ;
   ossMmapFile lobmMapFile ;

   CHAR *pBME = NULL ;
   dmsBucketsManagementExtent *pBMEMgr = NULL ;
   dmsSpaceManagementExtent *pSMEBitmap = ( dmsSpaceManagementExtent* )gSMEBuff ;

   INT32  pageID = 0 ;
   UINT64 offset = 0 ;
   UINT32 length = 0 ;

   MAP_PAGES         mapFreePages ;
   MAP_PAGES         mapUsedPages ;
   UINT32            freePagesNum = 0 ;
   UINT32            usedPagesNum = 0 ;
   UINT32            hasSetPage = 0 ;
   UINT32            totalPushed = 0 ;

   BOOLEAN           needForce = FALSE ;
   BOOLEAN           hasUpdateBME = FALSE ;
   BOOLEAN           doRecover = FALSE ;
   SINT64            hasWritten = 0 ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ; /// sec

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "  Recover Lob for collection [%d : %s]" OSS_NEWLINE,
                clId, pMB->_collectionName ) ;

   if ( !gBMEBuff )
   {
      gBMEBuff = (CHAR *)SDB_OSS_MALLOC( DMS_BME_SZ ) ;
      if ( !gBMEBuff )
      {
         dumpPrintf( "*** Error: Allocate bucket memory(%d) failed" OSS_NEWLINE,
                     DMS_BME_SZ ) ;
         ++err ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   //load bme
   pBME = gBMEBuff ;

   rc = getBME( lobmFile, pBME ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Read BME failed, rc: %d" OSS_NEWLINE, rc ) ;
      ++err ;
      goto error ;
   }
   pBMEMgr = ( dmsBucketsManagementExtent* )pBME ;

   /// analyze pages
   pageID = 0 ;
   while ( pageID <= (INT32)gPageNum )
   {
      offset = gDataOffset + pageID * sizeof(dmsLobDataMapBlk) ;
      rc = readData( &lobmFile, offset, sizeof(dmsLobDataMapBlk), (CHAR *)&blk,
                     TRUE, "lobm page" ) ;
      if ( rc )
      {
         ++err ;
         goto error ;
      }

      /// invalid page
      if ( blk._mbID != clId || blk._clLogicalID != pMB->_logicalID || blk.isUndefined() )
      {
         ++pageID ;
         continue ;
      }

      if ( DMS_SME_ALLOCATED != pSMEBitmap->getBitMask( pageID ) )
      {
         if ( mapFreePages.empty() )
         {
            mapFreePages[ pageID ] = pageID ;
         }
         else
         {
            MAP_PAGES::reverse_iterator rit = mapFreePages.rbegin() ;
            if ( rit != mapFreePages.rend() && rit->second + 1 == pageID )
            {
               /// update
               rit->second = pageID ;
            }
            else
            {
               mapFreePages[ pageID ] = pageID ;
            }
         }
         ++freePagesNum ;
      }
      else
      {
         if ( mapUsedPages.empty() )
         {
            mapUsedPages[ pageID ] = pageID ;
         }
         else
         {
            MAP_PAGES::reverse_iterator rit = mapUsedPages.rbegin() ;
            if ( rit != mapUsedPages.rend() && rit->second + 1 == pageID )
            {
               /// update
               rit->second = pageID ;
            }
            else
            {
               mapUsedPages[ pageID ] = pageID ;
            }
         }
         ++usedPagesNum ;
      }

      if ( DMS_LOB_META_SEQUENCE == blk._sequence )
      {
         ++gMBStat._totalLobs ;
      }
      ++gMBStat._totalLobPages ;

      ++pageID ;
   }

   /// dump recover analysis info
   dumpPrintf( "  ++++ The collection lob recovery analysis info ++++" OSS_NEWLINE
               "    Found Allocated Pages     : %u" OSS_NEWLINE
               "    Found Freed Pages         : %u" OSS_NEWLINE
               "    Found Total Lobs          : %llu (%ssame with %llu) " OSS_NEWLINE
               "    Found Total Pages         : %u (%ssame with %u)" OSS_NEWLINE OSS_NEWLINE,
               usedPagesNum,
               freePagesNum,
               gMBStat._totalLobs,
               gMBStat._totalLobs == pMB->_totalLobs ? "" : "*** not the ",
               pMB->_totalLobs,
               gMBStat._totalLobPages,
               gMBStat._totalLobPages == pMB->_totalLobPages ? "" : "*** not the ",
               pMB->_totalLobPages ) ;

   if ( gMBStat._totalLobs != pMB->_totalLobs ||
        gMBStat._totalLobPages != pMB->_totalLobPages )
   {
      needForce = TRUE ;
   }

   if ( needForce )
   {
      if ( !gForce )
      {
         dumpPrintf ( "*** Waring: Unexpected items exist. You need use '--%s' option to force "
                      "recovery. After recovery, you can use '--%s' to fix "
                      "non-compliant items" OSS_NEWLINE,
                      OPTION_FORCE, OPTION_REPAIRE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      dumpPrintf ( "### Note: Unexpected items exist. After recovery, you can use '--%s' to fix "
                   "non-compliant items" OSS_NEWLINE,
                   OPTION_REPAIRE ) ;
   }

   // 2. open mmap file
   {
      string csFileName = rtnMakeSUFileName( gCSName, 1, DMS_LOB_META_SU_EXT_NAME ) ;
      string csFullName = rtnFullPathName( gLobmPath, csFileName ) ;
      rc = lobmMapFile.open( csFullName.c_str(), OSS_READWRITE|OSS_EXCLUSIVE,
                             OSS_RU|OSS_WU|OSS_RG ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Open lob meta file failed, rc: %d" OSS_NEWLINE, rc ) ;
         ++err ;
         goto error ;
      }
   }

   // 3. mmap data
   pageID = 0 ;
   offset = gDataOffset ;
   while( pageID < (INT32)gPageNum )
   {
      length = ( gPageNum - pageID ) * sizeof( dmsLobDataMapBlk ) ;
      if ( length > (INT64)LOB_SEGMENT_SIZE )
      {
         length = LOB_SEGMENT_SIZE ;
      }
      else
      {
         length = ossRoundUpToMultipleX( length, 64 * 1024 ) ;
         if ( offset + length > gInspectFileStat._fileSize )
         {
            length = gInspectFileStat._fileSize - offset ;
         }
      }

      rc = lobmMapFile.map( offset, length, NULL ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Map lob meta file(Offste:%llu, Length:%u) failed, rc: %d"
                     OSS_NEWLINE, offset, length, rc ) ;
         ++err ;
         goto error ;
      }

      offset += length ;
      pageID += ( length / sizeof( dmsLobDataMapBlk ) ) ;
   }

   /// 4. set bitmap
   hasSetPage += _setBitmapByPages( pSMEBitmap, mapFreePages ) ;
   dumpPrintf ( "   Set %u pages SME Bitmap from [%d] => [%d]" OSS_NEWLINE,
                freePagesNum, DMS_SME_FREE, DMS_SME_ALLOCATED ) ;

   /// 5. save sme
   if ( hasSetPage > 0 )
   {
      rc = ossSeekAndWriteN( &lobmFile, DMS_SME_OFFSET,
                             (const CHAR*)gSMEBuff, DMS_SME_SZ, hasWritten ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to write SME, rc: %d" OSS_NEWLINE, rc ) ;
         ++err ;
         goto error ;
      }
   }

   /// 6. push page to bucket
   {
      INT32 beginPageID = DMS_LOB_INVALID_PAGEID ;
      INT32 endPageID = DMS_LOB_INVALID_PAGEID ;
      dmsLobDataMapBlk *pBlk = NULL ;
      UINT32 __hash = 0 ;
      UINT32 testBucketNo = 0 ;

      MAP_PAGES::iterator it = mapUsedPages.begin() ;
      while( it != mapUsedPages.begin() )
      {
         mapFreePages[ it->first ] = it->second ;
         ++it ;
      }

      it = mapFreePages.begin() ;
      while( it != mapFreePages.end() )
      {
         beginPageID = it->first ;
         endPageID = it->second ;

         while( beginPageID <= endPageID )
         {
            pBlk = _getLobBlk( lobmMapFile, beginPageID ) ;
            if ( NULL == pBlk )
            {
               dumpPrintf ( "*** Error: Read page(%d) failed" OSS_NEWLINE, beginPageID ) ;
               ++err ;
               goto error ;
            }
            else
            {
               dmsLobRecord record ;
               record.set( ( const bson::OID* )pBlk->_oid, pBlk->_sequence, 0,
                           pBlk->_dataLen, NULL, gLobHashType() ) ;
               /// add page to bucket
               DMS_LOB_GET_HASH_FROM_BLK( pBlk, __hash ) ;
               testBucketNo = _getLobBucketByHash( __hash ) ;
               rc = _push2Bucket( testBucketNo, beginPageID, *pBlk, &record, pBMEMgr,
                                  lobmMapFile, hasUpdateBME ) ;
               /// ignore page exist
               if ( rc && SDB_LOB_SEQUENCE_EXISTS != rc )
               {
                  dumpPrintf ( "*** Error: Push page(%d) to bucket failed, rc: %d" OSS_NEWLINE,
                               beginPageID, rc ) ;
                  ++err ;
                  goto error ;
               }
               else if ( SDB_OK == rc )
               {
                  ++totalPushed ;
               }
            }

            ++beginPageID ;
         }

         ++it ;
      }
   }

   /// 7. save BME
   if ( hasUpdateBME )
   {
      rc = ossSeekAndWriteN( &lobmFile, DMS_BME_OFFSET,
                             (const CHAR*)gBMEBuff, DMS_BME_SZ, hasWritten ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to write BME, rc: %d" OSS_NEWLINE, rc ) ;
         ++err ;
         goto error ;
      }
   }

   /// 8. save pages
   lobmMapFile.flushAll() ;

   doRecover = TRUE ;

done:
   if ( totalPushed > 0 )
   {
      dumpPrintf ( "   Pushed %u pages to lob bucket" OSS_NEWLINE, totalPushed ) ;
   }

   lobmMapFile.close() ;

   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( doRecover )
   {
      if ( tmpErr == err )
      {
         dumpPrintf ( "  Recover lob for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                      timeSpan ) ;
      }
      else
      {
         dumpPrintf ( "  Recover lob for collection Done Succeed with Warings: %d   [%.2f s]" OSS_NEWLINE,
                      err - tmpErr, timeSpan ) ;
      }
   }
   else
   {
      if ( tmpErr == err )
      {
         dumpPrintf ( "  Recover lob for collection Not Complete   [%.2f s]" OSS_NEWLINE,
                      timeSpan ) ;
      }
      else
      {
         dumpPrintf ( "  Recover lob for collection Not Complete with Errors: %d   [%.2f s]" OSS_NEWLINE,
                      err - tmpErr, timeSpan ) ;
      }
   }

   dumpPrintf( OSS_NEWLINE ) ;
   return ;
error:
   goto done ;
}

void recoverCollection ( OSSFILE &file, UINT32 pageSize, UINT16 id,
                         dmsMB *pMB, SINT32 &err )
{
   gMBStat.reset() ;

   // check sequoiadb is not running
   INT32 rc = pmdGetStartup().init( gDatabasePath ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: The data node is running, can't do recover" OSS_NEWLINE ) ;
      gHitError = TRUE ;
      ++err ;
      goto done ;
   }

   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      recoverCollectionData( file, pageSize, id, pMB, err ) ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      /// can't recover index file
      dumpPrintf( "*** Warning: Index does not support recovery" OSS_NEWLINE ) ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      /// can't recover lob file, because the lob page will be reset
      dumpPrintf( "*** Warning: Lob does not support recovery" OSS_NEWLINE ) ;
      // recoverCollectionLob( file, pageSize, id, pMB, err ) ;
   }

done:
   pmdGetStartup().final() ;
   return ;
}

void dumpCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
   INT32 rc = SDB_OK ;
   INT32 len = 0 ;
   std::set<dmsRecordID> extentRIDList ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DATA ;
   dmsMB *mb = NULL ;
   dmsExtentID tempExtent = DMS_INVALID_EXTENT ;
   dmsExtentID firstExtent = DMS_INVALID_EXTENT ;
   dmsCompressorEntry compressorEntry ;
   BOOLEAN capped = FALSE ;
   dmsExtent *pExtent = NULL ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ; /// sec

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   capped = OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_CAPPED ) ;

   // dump mb expand extent
   if ( DMS_INVALID_EXTENT != mb->_mbExExtentID )
   {
      UINT32 size = 0 ;
      extentType = INSPECT_EXTENT_TYPE_MBEX ;

      dumpPrintf ( " Dump Meta Extent for collection [%d, %s]" OSS_NEWLINE,
                   id, mb->_collectionName ) ;

      rc = loadExtent( file, extentType, pageSize, mb->_mbExExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Failed to load mb expand extent %d, rc: %d"
                     OSS_NEWLINE, mb->_mbExExtentID, rc ) ;
         goto error ;
      }
      size = ((dmsMetaExtent*)gExtentBuffer)->_blockSize*pageSize ;

   retry_mbEx:
      len = dmsDump::dumpMBEx( gExtentBuffer, size,
                               gBuffer, gBufferSize, NULL,
                               gDumpType, mb->_mbExExtentID ) ;
      if ( (UINT32)len >= gBufferSize -1 )
      {
         // if our buffer is not large enough, let's allocate more memory and
         // try again
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_mbEx ;
      }
      flushOutput ( gBuffer, len ) ;
   }

   if ( gOnlyMeta )
   {
      goto done ;
   }

   {
      SINT32 err = 0 ;
      rc = prepareCompressor(file, pageSize, mb, id, NULL, compressorEntry, err  ) ;
      if ( rc )
      {
         goto error ;
      }
   }

   if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
   {
      UINT32 size = 0 ;
      dumpPrintf ( " Dump Compression Dictionary Extent for collection [%d, %s]"
                   OSS_NEWLINE, id, mb->_collectionName ) ;

      size = ((dmsDictExtent*)gDictBuffer)->_blockSize * pageSize ;

   retry_dictExt:
      len = dmsDump::dumpDictExtent( gDictBuffer, size,
                                     gBuffer, gBufferSize, NULL,
                                     gDumpType, mb->_dictExtentID ) ;
      if ( (UINT32)len >= gBufferSize - 1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer() ;
            goto error ;
         }
         goto retry_dictExt ;
      }
      flushOutput( gBuffer, len ) ;
   }

   if ( DMS_INVALID_EXTENT != mb->_mbOptExtentID )
   {
      UINT32 size = 0 ;
      extentType = INSPECT_EXTENT_TYPE_EXTOPT ;
      dumpPrintf( " Dump extend option extent for collection [%d, %s]" OSS_NEWLINE,
                  id, mb->_collectionName ) ;
      rc = loadExtent( file, extentType, pageSize, mb->_mbOptExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Failed to load mb extend option extent %d, rc: %d"
                     OSS_NEWLINE, mb->_mbOptExtentID, rc ) ;
         goto error ;
      }
      size = ((dmsOptExtent*)gExtentBuffer)->_blockSize * pageSize ;

   retry_extOptExt:
      len = dmsDump::dumpExtOptExtent( gExtentBuffer, size,
                                       gBuffer, gBufferSize, NULL,
                                       gDumpType, mb->_mbExExtentID,
                                       capped ? DMS_STORAGE_CAPPED : DMS_STORAGE_NORMAL ) ;
      if ( (UINT32)len >= gBufferSize - 1 )
      {
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer() ;
            goto error ;
         }
         goto retry_extOptExt ;
      }
      flushOutput( gBuffer, len ) ;
   }

   extentType = INSPECT_EXTENT_TYPE_DATA ;
   firstExtent = mb->_firstExtentID ;
   dumpPrintf ( " Dump Data for collection [%d, %s]" OSS_NEWLINE,
                id, mb->_collectionName ) ;
   // loop through all extents
   while ( DMS_INVALID_EXTENT != firstExtent )
   {
      // load the given extent
      rc = loadExtent ( file, extentType, pageSize, firstExtent, id ) ;
      if ( rc )
      {
         dumpPrintf ( "*** Error: Failed to load extent %d, rc: %d" OSS_NEWLINE,
                      firstExtent, rc ) ;
         goto error ;
      }

      pExtent = (dmsExtent*)gExtentBuffer ;
      gMBStat._totalDataPages += pExtent->_blockSize ;
      gMBStat._totalDataFreeSpace += pExtent->_freeSpace ;
      gMBStat._totalRecords += pExtent->_recCount ;
      gMBStat._rcTotalRecords.add( pExtent->_recCount ) ;

   retry_data :
      extentRIDList.clear() ;
      tempExtent = firstExtent ;
      // attempt to dump extent text, note firstExtent will be assigned to next
      // extent id, until hitting DMS_INVALID_EXTENT as end of collection
      len = dmsDump::dumpDataExtent ( cb, gExtentBuffer,
                                      pExtent->_blockSize*pageSize,
                                      gBuffer, gBufferSize, NULL,
                                      gDumpType, tempExtent, &compressorEntry,
                                      &extentRIDList,  gShowRecordContent, capped ) ;

      if ( (UINT32)len >= gBufferSize-1 )
      {
         // if our buffer is not large enough, let's allocate more memory and
         // try again
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_data ;
      }
      flushOutput ( gBuffer, len ) ;

      if ( extentRIDList.size() != 0 && gShowRecordContent )
      {
         dumpOverflowedRecords ( file, pageSize, id, firstExtent,
                                 extentRIDList, &compressorEntry ) ;
      }

      firstExtent = tempExtent ;
   }

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " Dump Data for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                  timeSpan ) ;
   }
   else
   {
      dumpPrintf( " Dump Data for collection Done Failed: %d   [%.2f s]" OSS_NEWLINE,
                  rc, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " ++++ The collection data info ++++" OSS_NEWLINE
                  "   Collection ID     : %u" OSS_NEWLINE
                  "   Total Data Pages  : %u" OSS_NEWLINE OSS_NEWLINE,
                  id,
                  gMBStat._totalDataPages ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }

   return ;
error :
   goto done ;
}

void dumpCollectionIndex( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
   INT32 rc = SDB_OK ;
   dmsMB *mb = NULL ;
   std::map<UINT16, dmsExtentID> indexRoots ;
   std::map<UINT16, dmsExtentID>::iterator it ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   dumpPrintf ( " Dump Index for collection [%d, %s]" OSS_NEWLINE,
                id, mb->_collectionName ) ;

   dumpIndexDef ( file, pageSize, id, mb, indexRoots ) ;

   if ( gOnlyMeta )
   {
      goto done ;
   }

   // after dump index def, we should iterate all indexes and dump their
   // extents
   for ( it = indexRoots.begin() ; it != indexRoots.end() ; ++it )
   {
      UINT32 indexPages = 0 ;
      UINT16 indexID = it->first ;
      dmsExtentID rootID = it->second ;
      dumpPrintf ( "    Index Dump for collection [%04u], Index [%02u]" OSS_NEWLINE,
                   id, indexID ) ;
      if ( DMS_INVALID_EXTENT != rootID )
      {
         dumpIndexExtents ( file, pageSize, rootID, id, indexPages ) ;
      }
      else
      {
         dumpPrintf ( "      Root extent does not exist" OSS_NEWLINE ) ;
      }

      dumpPrintf ( "    Index Dump for collection Done, Index Pages: %u" OSS_NEWLINE OSS_NEWLINE,
                   indexPages ) ;

      gMBStat._totalIndexPages += indexPages ;
   }

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " Dump Index for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                  timeSpan ) ;
   }
   else
   {
      dumpPrintf( " Dump Index for collection Done Failed: %d   [%.2f s]" OSS_NEWLINE,
                  rc, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " ++++ The collection index info ++++" OSS_NEWLINE
                  "   Collection ID     : %u" OSS_NEWLINE
                  "   Total Index Pages : %u" OSS_NEWLINE OSS_NEWLINE,
                  id,
                  gMBStat._totalIndexPages ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }

   return ;
error :
   goto done ;
}

INT32 dumpLobmMeta( OSSFILE &file, UINT32 pageId, CHAR *pageBuf, UINT32 pageSize, UINT32 &sequence )
{
   INT32 rc = SDB_OK ;
   SINT64 len = 0 ;
   dmsLobDataMapBlk *blk = NULL ;

   rc = readData( &file, DMS_BME_OFFSET + DMS_BME_SZ + (INT64)pageSize * pageId,
                  pageSize, pageBuf, TRUE, "lobm page" ) ;
   if ( rc )
   {
      goto error ;
   }

   blk = (dmsLobDataMapBlk*)pageBuf ;
   sequence = blk->_sequence ;

retry_dmsLobDataMapBlk:

   len = dmsDump::dumpDmsLobDataMapBlk( pageId, blk, gBuffer, gBufferSize, NULL,
                                        gDumpType, pageSize, gLobHashType() ) ;
   if ( (UINT32)len >= gBufferSize -1 )
   {
      // if our buffer is not large enough, let's allocate more memory and
      // try again
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry_dmsLobDataMapBlk ;
   }

done:
   flushOutput( gBuffer, len ) ;
   return rc;
error:
   goto done;
}

INT32 dumpLobdCollection( OSSFILE &file, UINT32 pageId, CHAR* lobdPageBuf, UINT32 sequence)
{
   INT32 rc = SDB_OK ;
   SINT64 len = 0 ;
   UINT32 metaSize = 0 ;

   rc = readData( &file, DMS_HEADER_SZ + (INT64)gLobdPageSize * pageId,
                  gLobdPageSize, lobdPageBuf,
                  gShowRecordContent ? TRUE : FALSE, "lobd data" ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( DMS_LOB_META_SEQUENCE == sequence )
   {
      if( gLobdPageSize < DMS_LOB_META_LENGTH )
      {
         dumpPrintf ( "*** Error: LobMeta size (%d) in lobd file is too small "
                      "expected size (%d)" OSS_NEWLINE,
                      gLobdPageSize, DMS_LOB_META_LENGTH ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      dmsLobMeta *lobMeta = (dmsLobMeta *)lobdPageBuf ;
      metaSize = DMS_LOB_META_LENGTH ;

   retry_dmsLobMeta:
      len = dmsDump::dumpDmsLobMeta( lobdPageBuf, metaSize,
                                     gBuffer, gBufferSize, NULL,
                                     gDumpType ) ;
      if ( (UINT32)len >= gBufferSize -1 )
      {
         // if our buffer is not large enough, let's allocate more memory and
         // try again
         if ( SDB_OK != ( rc = reallocBuffer() ) )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_dmsLobMeta;
      }
      flushOutput( gBuffer, (UINT32)len ) ;

      if ( lobMeta->_version < DMS_LOB_META_MERGE_DATA_VERSION )
      {
         /// old version, the whole page is meta page
         goto done ;
      }
   }

   if( !gShowRecordContent )
   {
      goto done;
   }

retry_dmsLobData:
   len = dmsDump::dumpDmsLobData( lobdPageBuf + metaSize,
                                  gLobdPageSize - metaSize,
                                  gBuffer, gBufferSize , NULL,
                                  gDumpType ) ;
   if ( (UINT32)len >= gBufferSize -1 )
   {
      // if our buffer is not large enough, let's allocate more memory and
      // try again
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry_dmsLobData ;
   }
   flushOutput( gBuffer, (UINT32)len) ;

done:
    return rc;
error:
    goto done;
}

INT32 dumpCollectionLob( OSSFILE &lobmFile, UINT32 pageSize, UINT16 id )
{
   INT32 rc = SDB_OK ;
   CHAR *pPageBuf = NULL ;
   CHAR *pLobdPageBuf = NULL ;
   dmsMB *mb = NULL ;
   MAP_PAGES *pMapPages = NULL ;
   MAP_CL_PAGES::iterator it ;
   MAP_PAGES::iterator itMap ;
   INT32 beginPageID = DMS_LOB_INVALID_PAGEID ;
   INT32 endPageID = DMS_LOB_INVALID_PAGEID ;
   UINT32 sequence = 0 ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ; /// sec

   beginTime = ossGetCurrentMilliseconds() ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to load metadata block, rc: %d" OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   dumpPrintf ( " Dump Lob for collection [%d : %s]" OSS_NEWLINE,
                id, mb->_collectionName ) ;

   it = gMapLobCLPages.find( id ) ;
   if ( it == gMapLobCLPages.end() )
   {
      /// not found the collection's lob pages
      goto done ;
   }
   pMapPages = &(it->second) ;

   if ( !gOnlyMeta )
   {
      pLobdPageBuf = (CHAR*)SDB_OSS_MALLOC( gLobdPageSize ) ;
      if ( NULL == pLobdPageBuf)
      {
         dumpPrintf ( "*** Error: Failed to alloc buffer with size %u, rc: %d"
                      OSS_NEWLINE, gLobdPageSize, SDB_OOM ) ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   pPageBuf = (CHAR*)SDB_OSS_MALLOC( pageSize ) ;
   if ( NULL == pPageBuf)
   {
      dumpPrintf ( "*** Error: Failed to alloc buffer with size %u, rc: %d"
                   OSS_NEWLINE, pageSize, SDB_OOM ) ;
      rc = SDB_OOM ;
      goto error;
   }

   /// dump collection objects
   itMap = pMapPages->begin() ;
   while( itMap != pMapPages->end() )
   {
      beginPageID = itMap->first ;
      endPageID = itMap->second ;
      ++itMap ;

      while( beginPageID <= endPageID )
      {
         dumpPrintf ( " Dump Lob Page %d:" OSS_NEWLINE, beginPageID ) ;
         rc = dumpLobmMeta( lobmFile, beginPageID, pPageBuf, pageSize, sequence ) ;
         if ( rc )
         {
            goto error ;
         }

         ++gMBStat._totalLobPages ;
         if ( DMS_LOB_META_SEQUENCE == sequence )
         {
            ++gMBStat._totalLobs ;
         }

         if ( gOnlyMeta )
         {
            ++beginPageID ;
            continue ;
         }

         // just dump lobmeta in page 0.
         if ( ( !gShowRecordContent || !gHex ) && DMS_LOB_META_SEQUENCE != sequence )
         {
            ++beginPageID ;
            continue ;
         }

         rc = dumpLobdCollection( gLobdFile, beginPageID, pLobdPageBuf, sequence ) ;
         if ( rc )
         {
            goto error ;
         }

         ++beginPageID ;
      }
   }

   gMapLobCLPages.erase( it ) ;

done :
   SAFE_OSS_FREE( pPageBuf ) ;
   SAFE_OSS_FREE( pLobdPageBuf ) ;

   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " Dump Lob for collection Done Succeed   [%.2f s]" OSS_NEWLINE,
                  timeSpan ) ;
   }
   else
   {
      dumpPrintf( " Dump Lob for collection Done Failed: %d   [%.2f s]" OSS_NEWLINE,
                  rc, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( " ++++ The collection lob info ++++" OSS_NEWLINE
                  "   Collection ID     : %u" OSS_NEWLINE
                  "   Total Lob Pages   : %u" OSS_NEWLINE
                  "   Total Lobs        : %llu" OSS_NEWLINE OSS_NEWLINE,
                  id,
                  gMBStat._totalLobPages,
                  gMBStat._totalLobs ) ;
   }
   else
   {
      dumpPrintf( OSS_NEWLINE ) ;
   }

   return rc ;
error :
   goto done ;
}

void dumpCollection ( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
   gMBStat.reset() ;

   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      dumpCollectionData( file, pageSize, id ) ;
      gInspectFileStat._totalFreeSize += gMBStat._totalDataFreeSpace ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      dumpCollectionIndex( file, pageSize, id ) ;
      gInspectFileStat._totalFreeSize += gMBStat._totalIndexFreeSpace ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      dumpCollectionLob( file, pageSize, id ) ;
   }
}

void inspectCollections ( OSSFILE &file, UINT32 pageSize,
                          vector<UINT16> &collections,
                          SINT32 hwm, CHAR *pExpBuffer,
                          SINT32 &err )
{
   vector<UINT16>::iterator it ;
   for ( it = collections.begin() ; it != collections.end() ; ++it )
   {
      inspectCollection ( file, pageSize, *it, hwm, pExpBuffer, err ) ;
   }
}

void dumpCollections ( OSSFILE &file, UINT32 pageSize,
                       vector<UINT16> &collections )
{
   vector<UINT16>::iterator it ;
   for ( it = collections.begin() ; it != collections.end() ; ++it )
   {
      dumpCollection( file, pageSize, *it ) ;
   }
}

// inspect collections, this function will first inspect MME, and then based on
// the input CLName inspectMME may choose to inspect zero or more collections.
// Note pExpBuffer is not NULL only in full collectionspace inspection
void inspectCollections ( OSSFILE &file, UINT32 pageSize, SINT32 hwm,
                          CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   SINT32 localErr = 0 ;
   vector<UINT16> collections ;

   if ( FALSE == gInitMME )
   {
      SDB_ASSERT( FALSE, "MME should be init before" ) ;
      ++err ;
      rc = SDB_SYS ;

      dumpPrintf( "*** Error: MME is not init" OSS_NEWLINE ) ;
      goto error ;
   }

retry :
   // attempt to inspect MME
   collections.clear() ;
   localErr = 0 ;
   len = dmsInspect::inspectMME ( gMMEBuff, DMS_MME_SZ,
                                  gBuffer, gBufferSize,
                                  ossStrlen ( gCLName ) ? gCLName : NULL,
                                  ( SDB_INSPT_DATA == gCurInsptType ) ? hwm : -1,
                                  collections, localErr ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;
   err += localErr ;

   if ( collections.size() != 0 )
   {
      inspectCollections ( file, pageSize, collections, hwm, pExpBuffer, err ) ;
   }
   if ( pExpBuffer )
   {
      inspectSME ( file, pExpBuffer, hwm, err, FALSE ) ;
   }

done :
   return ;
error :
   goto done ;
}

void recoverCollections ( OSSFILE &file, UINT32 pageSize, SINT32 &err )
{
   dmsMB *pMB = NULL ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   if ( FALSE == gInitMME )
   {
      SDB_ASSERT( FALSE, "MME should be init before" ) ;
      ++err ;

      dumpPrintf( "*** Error: MME is not init" OSS_NEWLINE ) ;
      goto error ;
   }

   if ( DMS_INVALID_MBID != gRecoverMBID )
   {
      pMB = ( dmsMB* )( gMMEBuff + gRecoverMBID * sizeof( dmsMB ) ) ;
      ossStrncpy( collectionName, pMB->_collectionName, DMS_COLLECTION_NAME_SZ ) ;

      /// check the name
       if ( 0 != ossStrcmp( gCLName, collectionName ) )
      {
         dumpPrintf( "*** Error: The %s 's name(%s) is not the same with %s" OSS_NEWLINE,
                     OPTION_RECOVER_MBID, collectionName, gCLName ) ;
         ++err ;
         goto error ;
      }
      /// check flag
      else if ( DMS_MB_FLAG_FREE != pMB->_flag &&
                !OSS_BIT_TEST ( pMB->_flag, DMS_MB_FLAG_DROPED ) &&
                DMS_INVALID_MBID == gDoRecoverMBID )
      {
         dumpPrintf( "*** Warning: The collection is not dropped, no need to recover" OSS_NEWLINE ) ;
         goto done ;
      }
   }
   else
   {
      vector<UINT16> collections ;

      /// find the recover-mbid by slot
      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         pMB = ( dmsMB* )( gMMEBuff + i * sizeof( dmsMB ) ) ;
         ossStrncpy( collectionName, pMB->_collectionName, DMS_COLLECTION_NAME_SZ ) ;

         if ( 0 == ossStrcmp( gCLName, collectionName ) )
         {
            if ( SDB_INSPT_DATA == gCurInsptType &&
                 DMS_MB_FLAG_FREE != pMB->_flag &&
                 !OSS_BIT_TEST ( pMB->_flag, DMS_MB_FLAG_DROPED ) )
            {
               dumpPrintf( "*** Warning: The collection is not dropped, no need to recover" OSS_NEWLINE ) ;
               goto done ;
            }
            collections.push_back( i ) ;
         }
      }

      /// not found collection
      if ( collections.empty() )
      {
         dumpPrintf( "*** Warning: The collection is not found" OSS_NEWLINE ) ;
         goto done ;
      }
      /// more than one collection with the same name
      else if ( collections.size() > 1 )
      {
         dumpPrintf( "*** Warning: Found %u collections[", collections.size() ) ;
         for ( UINT32 i = 0 ; i < collections.size() ; ++i )
         {
            if ( 0 ==i )
            {
               dumpPrintf( "%u", collections[i] ) ;
            }
            else
            {
               dumpPrintf( ", %u", collections[i] ) ;
            }
         }
         dumpPrintf( "] with the same name, you should use param '--%s' to specify "
                     "the MBID" OSS_NEWLINE, OPTION_RECOVER_MBID ) ;
         goto error ;
      }

      gRecoverMBID = collections[ 0 ] ;
      pMB = ( dmsMB* )( gMMEBuff + gRecoverMBID * sizeof( dmsMB ) ) ;
   }

   /// recover the collection
   recoverCollection( file, pageSize, gRecoverMBID, pMB, err ) ;

done :
   return ;
error :
   goto done ;
}

INT32 parseLobmBMEAndPages( OSSFILE &lobmFile, UINT32 pageSize, UINT32 maxPages,
                            CHAR *pSME, CHAR *pBME, MAP_CL_PAGES &mapCLPages,
                            lobBMEStat &bmeStat, SINT32 *pErr )
{
   INT32 rc = SDB_OK ;

   dmsLobDataMapBlk blk ;
   MAP_BUCKET_PAGE_INFO mapBucketInfo ;
   MAP_BUCKET_PAGE_INFO::iterator itBlk ;

   dmsSpaceManagementExtent *pSMEBitmap = NULL ;
   dmsBucketsManagementExtent *pBMEMgr = NULL ;
   INT32  pageID = 0 ;
   INT64  offset = 0 ;

   UINT32 dep = 0 ;
   INT32  prevPageID = DMS_LOB_INVALID_PAGEID ;
   INT32  nextPageID = DMS_LOB_INVALID_PAGEID ;
   UINT32 bucketID = 0 ;

   SDB_ASSERT( pageSize == sizeof(dmsLobDataMapBlk), "Invalid page size" ) ;

   pSMEBitmap = (dmsSpaceManagementExtent*)pSME ;
   pBMEMgr = (dmsBucketsManagementExtent*)pBME ;

   bmeStat.reset() ;

   while ( pageID <= (INT32)maxPages )
   {
      dep = 0 ;

      /// free page
      if ( DMS_SME_ALLOCATED != pSMEBitmap->getBitMask( pageID ) )
      {
         ++pageID ;
         continue ;
      }

      offset = gDataOffset + pageID * sizeof(dmsLobDataMapBlk) ;
      rc = readData( &lobmFile, offset, sizeof(dmsLobDataMapBlk), (CHAR *)&blk,
                     TRUE, "lobm page" ) ;
      if ( rc )
      {
         if ( pErr )
         {
            ++(*pErr) ;
         }
         goto error ;
      }

      /// invalid page
      if ( blk._mbID >= DMS_MME_SLOTS || blk._mbID < 0 || blk.isUndefined() )
      {
         ++bmeStat._invalidPageCount ;
         ++pageID ;
         continue ;
      }
      else
      {
         MAP_PAGES &mapPages = mapCLPages[ blk._mbID ] ;
         if ( mapPages.empty() )
         {
            mapPages[ pageID ] = pageID ;
         }
         else
         {
            MAP_PAGES::reverse_iterator rit = mapPages.rbegin() ;
            if ( rit != mapPages.rend() && rit->second + 1 == pageID )
            {
               /// update
               rit->second = pageID ;
            }
            else
            {
               mapPages[ pageID ] = pageID ;
            }
         }
      }

      ++bmeStat._validPageCount ;

      if ( DMS_LOB_META_SEQUENCE == blk._sequence )
      {
         ++bmeStat._totalLobCount ;
      }

      /// First bucket page
      prevPageID = blk._prevPageInBucket ;
      nextPageID = blk._nextPageInBucket ;

      /// calc hash code and bucket id
      bucketID = dmsStorageLob::getBucketID(  blk, gLobHashType() ) ;

      if ( DMS_LOB_INVALID_PAGEID == prevPageID )
      {
         INT32 bucketPageID = pBMEMgr->_buckets[bucketID] ;
         if ( bucketPageID != pageID )
         {
            dumpPrintf( "*** Error: Bucket(%u)'s pageID(%d) is not the same "
                        "with page(%d)" OSS_NEWLINE,
                        bucketID, bucketPageID, pageID ) ;
            if ( pErr )
            {
               ++( *pErr ) ;
            }
         }
         else
         {
            ++bmeStat._validBlkCount ;
         }
      }

      if ( DMS_LOB_INVALID_PAGEID == prevPageID && DMS_LOB_INVALID_PAGEID == nextPageID )
      {
         dep = 1 ;
      }
      else
      {
         lobBucketInfo &blkInfo = mapBucketInfo[ bucketID ] ;
         ++blkInfo._dep ;

         if ( DMS_LOB_INVALID_PAGEID != prevPageID )
         {
            /// push to relation
            if ( prevPageID > pageID )
            {
               blkInfo._mapPages[ MAKE_PAGE_REL_KEY( pageID, SDB_PAGE_PREV ) ] = prevPageID ;
            }
            /// release from relation
            else
            {
               MAP_PAGE_RELATION::iterator it ;
               it = blkInfo._mapPages.find( MAKE_PAGE_REL_KEY( prevPageID, SDB_PAGE_NEXT ) ) ;
               if ( it == blkInfo._mapPages.end() )
               {
                  dumpPrintf( "*** Error: PageID(%d)'s prev(%d) occur incorrect" OSS_NEWLINE,
                              pageID, prevPageID ) ;
                  if ( pErr )
                  {
                     ++( *pErr ) ;
                  }
               }
               else if ( it->second != pageID )
               {
                  dumpPrintf( "*** Error: PageID(%d)'s prev(%d) is not its next(%d)" OSS_NEWLINE,
                              pageID, prevPageID, it->second ) ;
                  if ( pErr )
                  {
                     ++( *pErr ) ;
                  }
               }
               else
               {
                  /// correct, remove
                  blkInfo._mapPages.erase( it ) ;
               }
            }
         }

         if ( DMS_LOB_INVALID_PAGEID != nextPageID )
         {
            /// push to relation
            if ( nextPageID > pageID )
            {
               blkInfo._mapPages[ MAKE_PAGE_REL_KEY( pageID, SDB_PAGE_NEXT ) ] = nextPageID ;
            }
            /// release from relation
            else
            {
               MAP_PAGE_RELATION::iterator it ;
               it = blkInfo._mapPages.find( MAKE_PAGE_REL_KEY( nextPageID, SDB_PAGE_PREV ) ) ;
               if ( it == blkInfo._mapPages.end() )
               {
                  dumpPrintf( "*** Error: PageID(%d)'s next(%d) occur incorrect" OSS_NEWLINE,
                              pageID, nextPageID ) ;
                  if ( pErr )
                  {
                     ++( *pErr ) ;
                  }
               }
               else if ( it->second != pageID )
               {
                  dumpPrintf( "*** Error: PageID(%d)'s next(%d) is not its prev(%d)" OSS_NEWLINE,
                              pageID, nextPageID, it->second ) ;
                  if ( pErr )
                  {
                     ++( *pErr ) ;
                  }
               }
               else
               {
                  /// correct, remove
                  blkInfo._mapPages.erase( it ) ;
               }
            }
         }

         if ( blkInfo._mapPages.empty() )
         {
            dep = blkInfo._dep ;
            /// release the bucket info
            mapBucketInfo.erase( bucketID ) ;
         }
      }

      if ( dep != 0 )
      {
         if ( bmeStat._maxDep < dep )
         {
            bmeStat._maxDep = dep ;
         }
         if ( dep < bmeStat._minDep )
         {
            bmeStat._minDep = dep ;
         }
         if ( dep < LOB_BLK_DEP_SZ )
         {
            ++( bmeStat._depNum[ dep ] ) ;
         }
         else
         {
            ++( bmeStat._depNum[ LOB_BLK_DEP_SZ - 1 ] ) ;
         }

         bmeStat._sum += dep ;
         bmeStat._value += ( dep * dep ) ;
      }

      ++pageID ;
   }

   /// check mapBucketInfo
   itBlk = mapBucketInfo.begin() ;
   while ( itBlk != mapBucketInfo.end() )
   {
      bucketID = itBlk->first ;
      lobBucketInfo &blkInfo = itBlk->second ;
      MAP_PAGE_RELATION::iterator it ;

      it = blkInfo._mapPages.begin() ;
      while( it != blkInfo._mapPages.end() )
      {
         dumpPrintf( "*** Error: PageID(%d)'s %s(%d) occur incorrect link, bucket: %u" OSS_NEWLINE,
                     GET_PAGEID_FROM_KEY( it->first ),
                     SDB_PAGE_PREV == GET_LISTTYPE_FROM_KEY( it->first ) ? "prev" : "next",
                     it->second, bucketID ) ;
         if ( pErr )
         {
            ++( *pErr ) ;
         }
         ++it ;
      }

      dep = blkInfo._dep ;
      if ( bmeStat._maxDep < dep )
      {
         bmeStat._maxDep = dep ;
      }
      if ( dep < bmeStat._minDep )
      {
         bmeStat._minDep = dep ;
      }

      if ( dep < LOB_BLK_DEP_SZ )
      {
         ++( bmeStat._depNum[ dep ] ) ;
      }
      else
      {
         ++( bmeStat._depNum[ LOB_BLK_DEP_SZ - 1 ] ) ;
      }

      bmeStat._sum += dep ;
      bmeStat._value += ( dep * dep ) ;

      ++itBlk ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 inspectBME( OSSFILE &file, UINT32 pageSize, UINT32 maxPages, CHAR *pSME,
                  MAP_CL_PAGES &mapCLPages, lobBMEStat &bmeStat, SINT32 &err )
{
   INT32 rc = SDB_OK ;
   CHAR *pBME = NULL ;
   INT32 localErr = err ;
   dmsBucketsManagementExtent *pBMEMgr = NULL ;
   INT32 pageID = DMS_LOB_INVALID_PAGEID ;
   UINT32 validBlkCount = 0 ;
   INT32  minValidBlkIndex = -1 ;
   INT32  maxValidBlkIndex = -1 ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   SDB_ASSERT( file.isOpened(), "file is not open" ) ;

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf( "  Inspect Bucket Management Extent:" OSS_NEWLINE ) ;

   if ( !gBMEBuff )
   {
      gBMEBuff = (CHAR *)SDB_OSS_MALLOC( DMS_BME_SZ ) ;
      if ( !gBMEBuff )
      {
         dumpPrintf( "*** Error: Allocate bucket memory(%d) failed" OSS_NEWLINE,
                     DMS_BME_SZ ) ;
         ++err ;
         rc = SDB_OOM ;
         goto error ;
      }
   }

   //load bme
   pBME = gBMEBuff ;

   rc = getBME( file, pBME ) ;
   if ( rc )
   {
      ++err ;
      goto error ;
   }

   pBMEMgr = ( dmsBucketsManagementExtent* )pBME ;

   // parse BME & traverse buckets & make up a map<collectionId, map<page,page>>
   rc = parseLobmBMEAndPages( file, pageSize, maxPages, pSME, pBME,
                              mapCLPages, bmeStat, &err ) ;
   if ( rc )
   {
      goto error ;
   }

   /// check bme
   for( UINT32 i = 0 ; i < DMS_BUCKETS_NUM ; i++ )
   {
      pageID = pBMEMgr->_buckets[ i ] ;

      if ( DMS_LOB_INVALID_PAGEID == pageID )
      {
         ++( bmeStat._depNum[0] ) ;
      }
      else
      {
         ++validBlkCount ;

         if ( -1 == minValidBlkIndex )
         {
            minValidBlkIndex = i ;
         }
         maxValidBlkIndex = i ;
      }
   }

   if ( validBlkCount != bmeStat._validBlkCount )
   {
      dumpPrintf( "*** Error: Bucket valid count(%u) is not the same with "
                  "the count(%u) parsed by pages" OSS_NEWLINE,
                  validBlkCount, bmeStat._validBlkCount ) ;
      ++err ;
   }

done :
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }
   if ( localErr == err )
   {
      dumpPrintf( "  Inspect Bucket Management Extent Done Succeed   [%.2f s]" OSS_NEWLINE,
                  timeSpan ) ;
   }
   else
   {
      dumpPrintf( "  Inspect Bucket Management Extent Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                  err - localErr, timeSpan ) ;
   }

   if ( bmeStat._validBlkCount + bmeStat._invalidPageCount != 0 )
   {
      FLOAT64 avg = 0.0 ;
      FLOAT64 variance = 0.0 ;
      FLOAT64 ratio = 0.0 ;

      if ( bmeStat._validBlkCount > 0 )
      {
         avg = (FLOAT64)bmeStat._sum / bmeStat._validBlkCount ;
         FLOAT64 tmpV = ( (FLOAT64)bmeStat._sum * bmeStat._sum ) / bmeStat._validBlkCount ;
         variance = ( bmeStat._value - tmpV ) / bmeStat._validBlkCount ;
      }

      dumpPrintf ( "  Bucket Management Extent Balance:" OSS_NEWLINE
                   "    Valid Bucket Count    : %u" OSS_NEWLINE
                   "    Min Valid Bucket Pos  : %d" OSS_NEWLINE
                   "    Max Valid Bucket Pos  : %d" OSS_NEWLINE
                   "    Valid Page Count      : %u" OSS_NEWLINE
                   "    Invalid Page Count    : %u" OSS_NEWLINE
                   "    Total Lob Count       : %u" OSS_NEWLINE
                   "    Max Bucket Depth      : %u" OSS_NEWLINE
                   "    Min Bucket Depth      : %u" OSS_NEWLINE
                   "    Average Bucket Depth  : %.6f" OSS_NEWLINE
                   "    Variance              : %.6f (Less is Better)" OSS_NEWLINE,
                   bmeStat._validBlkCount,
                   minValidBlkIndex,
                   maxValidBlkIndex,
                   bmeStat._validPageCount,
                   bmeStat._invalidPageCount,
                   bmeStat._totalLobCount,
                   bmeStat._maxDep,
                   bmeStat._minDep,
                   avg,
                   variance ) ;

      for ( UINT32 i = 0 ; i < LOB_BLK_DEP_SZ ; ++i )
      {
         if ( bmeStat._depNum[i] > 0 )
         {
            if ( 0 ==i )
            {
               ratio = ( FLOAT64 ) bmeStat._depNum[i] / DMS_BUCKETS_NUM ;
            }
            else
            {
               ratio = ( FLOAT64 ) bmeStat._depNum[i] / bmeStat._validBlkCount ;
            }

            if ( i + 1 == LOB_BLK_DEP_SZ )
            {
               dumpPrintf ( "   >= %02d-Depth Number     : %u (%.2f%%)" OSS_NEWLINE,
                            i, bmeStat._depNum[i], ratio * 100 ) ;
            }
            else
            {
               dumpPrintf ( "      %02d-Depth Number     : %u (%.2f%%)" OSS_NEWLINE,
                            i, bmeStat._depNum[i], ratio * 100 ) ;
            }
         }
      }
   }

   dumpPrintf( OSS_NEWLINE ) ;

   return rc;
error :
   goto done ;
}

void inspectLobCollections ( OSSFILE &file, UINT32 pageSize, SINT32 hwm,
                             CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc = SDB_OK ;
   UINT32 len = 0 ;
   vector<UINT16> collections ;
   CHAR *pSME = gSMEBuff ;
   MAP_CL_PAGES::iterator it ;

   //load bme
   rc = inspectBME( file, pageSize, hwm, pSME, gMapLobCLPages, gLobBMEStat, err ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( FALSE == gInitMME )
   {
      SDB_ASSERT( FALSE, "MME should be init before" ) ;
      ++err ;
      rc = SDB_SYS ;

      dumpPrintf( "*** Error: MME is not init" OSS_NEWLINE ) ;
      goto error ;
   }

retry :
   // attempt to inspect MME
   collections.clear() ;
   len = dmsInspect::inspectMME ( gMMEBuff, DMS_MME_SZ,
                                  gBuffer, gBufferSize,
                                  ossStrlen ( gCLName ) ? gCLName : NULL,
                                  ( SDB_INSPT_DATA == gCurInsptType ) ? hwm : -1,
                                  collections, err ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }

   if ( collections.size() != 0 )
   {
      inspectCollections ( file, pageSize, collections, hwm, pExpBuffer, err ) ;
   }

   /// check remaining pages
   if ( 0 == ossStrlen( gCLName ) && !gMapLobCLPages.empty() )
   {
      it = gMapLobCLPages.begin() ;
      while( it != gMapLobCLPages.end() )
      {
         MAP_PAGES &mapPages = it->second ;
         MAP_PAGES::iterator itMap = mapPages.begin() ;
         while ( itMap != mapPages.end() )
         {
            ++err ;
            dumpPrintf( "*** Error: Lob page (%d, %d) out of MME, MBID: %u" OSS_NEWLINE,
                        itMap->first, itMap->second, it->first ) ;
            ++itMap ;
         }
         ++it ;
      }
   }

   /// re-check SME with pages
   if ( pExpBuffer )
   {
      inspectSME ( file, pExpBuffer, hwm, err, FALSE ) ;
   }

done :
   return ;
error :
   goto done ;
}

void dumpRawPage ( OSSFILE &file, UINT32 pageSize, SINT32 pageID )
{
   INT32 rc = SDB_OK ;
   UINT32 len = 0 ;

   rc = getExtent ( file, pageID, pageSize, 1 ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to get extent %d, rc: %d"
                   OSS_NEWLINE, pageID, rc ) ;
      goto error ;
   }

retry :
   len = dmsDump::dumpRawPage ( gExtentBuffer, gExtentBufferSize,
                                gBuffer, gBufferSize ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   return ;
error :
   goto done ;
}

void repaireCollection( OSSFILE &file, dmsMB *pMB, UINT32 id )
{
   dumpPrintf( "Begin to repaire collection: %s.%s, ID: %u" OSS_NEWLINE,
               gCSName, gCLName, id ) ;
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_FLAG )
   {
      dumpPrintf( "   Flag[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_flag, gRepaireMB._flag ) ;
      pMB->_flag = gRepaireMB._flag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_ATTR )
   {
      dumpPrintf( "   Attr[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_attributes, gRepaireMB._attributes ) ;
      pMB->_attributes = gRepaireMB._attributes ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LID )
   {
      dumpPrintf( "   LID[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_logicalID, gRepaireMB._logicalID ) ;
      pMB->_logicalID = gRepaireMB._logicalID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_RECORD )
   {
      dumpPrintf( "   Record[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_totalRecords, gRepaireMB._totalRecords ) ;
      pMB->_totalRecords = gRepaireMB._totalRecords ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_DATAPAGE )
   {
      dumpPrintf( "   DataPages[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_totalDataPages, gRepaireMB._totalDataPages ) ;
      pMB->_totalDataPages = gRepaireMB._totalDataPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXPAGE )
   {
      dumpPrintf( "   IndexPages[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_totalIndexPages, gRepaireMB._totalIndexPages ) ;
      pMB->_totalIndexPages = gRepaireMB._totalIndexPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOBPAGE )
   {
      dumpPrintf( "   LobPages[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_totalLobPages, gRepaireMB._totalLobPages ) ;
      pMB->_totalLobPages = gRepaireMB._totalLobPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_DATAFREE )
   {
      dumpPrintf( "   DataFreeSpace[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_totalDataFreeSpace, gRepaireMB._totalDataFreeSpace ) ;
      pMB->_totalDataFreeSpace = gRepaireMB._totalDataFreeSpace ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXFREE )
   {
      dumpPrintf( "   IndexFreeSpace[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_totalIndexFreeSpace, gRepaireMB._totalIndexFreeSpace ) ;
      pMB->_totalIndexFreeSpace = gRepaireMB._totalIndexFreeSpace ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXNUM )
   {
      dumpPrintf( "   IndexNum[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_numIndexes, gRepaireMB._numIndexes ) ;
      pMB->_numIndexes = gRepaireMB._numIndexes ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMPRESSTYPE )
   {
      dumpPrintf( "   CompressType[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_compressorType, gRepaireMB._compressorType ) ;
      pMB->_compressorType = gRepaireMB._compressorType ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOBS )
   {
      dumpPrintf( "   Lobs[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_totalLobs, gRepaireMB._totalLobs ) ;
      pMB->_totalLobs = gRepaireMB._totalLobs ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMMITFLAG )
   {
      dumpPrintf( "   CommitFlag[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_commitFlag, gRepaireMB._commitFlag ) ;
      pMB->_commitFlag = gRepaireMB._commitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMMITLSN )
   {
      dumpPrintf( "   CommitLSN[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_commitLSN, gRepaireMB._commitLSN ) ;
      pMB->_commitLSN = gRepaireMB._commitLSN ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDX_COMMITFLAG )
   {
      dumpPrintf( "   IdxCommitFlag[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_idxCommitFlag, gRepaireMB._idxCommitFlag ) ;
      pMB->_idxCommitFlag = gRepaireMB._idxCommitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDX_COMMITLSN )
   {
      dumpPrintf( "   IdxCommitLSN[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_idxCommitLSN, gRepaireMB._idxCommitLSN ) ;
      pMB->_idxCommitLSN = gRepaireMB._idxCommitLSN ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOB_COMMITFLAG )
   {
      dumpPrintf( "   LobCommitFlag[%u] ==> [%u]" OSS_NEWLINE,
                  pMB->_lobCommitFlag, gRepaireMB._lobCommitFlag ) ;
      pMB->_lobCommitFlag = gRepaireMB._lobCommitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_FIRSTEXTENTID )
   {
      dumpPrintf( "   FirstExtentID[%d] ==> [%d]" OSS_NEWLINE,
                  pMB->_firstExtentID, gRepaireMB._firstExtentID ) ;
      pMB->_firstExtentID = gRepaireMB._firstExtentID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LASTEXTENTID )
   {
      dumpPrintf( "   LastExtentID[%d] ==> [%d]" OSS_NEWLINE,
                  pMB->_lastExtentID, gRepaireMB._lastExtentID ) ;
      pMB->_lastExtentID = gRepaireMB._lastExtentID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_MBEXTEXTENTID )
   {
      dumpPrintf( "   MBExtExtentID[%d] ==> [%d]" OSS_NEWLINE,
                  pMB->_mbExExtentID, gRepaireMB._mbExExtentID ) ;
      pMB->_mbExExtentID = gRepaireMB._mbExExtentID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_DICTEXTENTID )
   {
      dumpPrintf( "   DictExtentID[%d] ==> [%d]" OSS_NEWLINE,
                  pMB->_dictExtentID, gRepaireMB._dictExtentID ) ;
      pMB->_dictExtentID = gRepaireMB._dictExtentID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_OPTEXTENTID )
   {
      dumpPrintf( "   OptExtentID[%d] ==> [%d]" OSS_NEWLINE,
                  pMB->_mbOptExtentID, gRepaireMB._mbOptExtentID ) ;
      pMB->_mbOptExtentID = gRepaireMB._mbOptExtentID ;
   }

   INT64 written = 0 ;
   INT32 rc = ossSeekAndWriteN( &file, DMS_MME_OFFSET + id * sizeof( dmsMB ),
                                (const CHAR*)pMB, sizeof( dmsMB ), written ) ;
   if ( rc )
   {
      dumpPrintf( "*** Save collection to file failed: %d" OSS_NEWLINE,
                  rc ) ;
   }
   else
   {
      dumpPrintf( "Save collection info to file succeed" OSS_NEWLINE ) ;
   }
}

void repaireCollections( OSSFILE &file )
{
   dmsMB *pMB = NULL ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   if ( FALSE == gInitMME )
   {
      SDB_ASSERT( FALSE, "MME should be init before" ) ;

      dumpPrintf( "*** Error: MME is not init" OSS_NEWLINE ) ;
      goto done ;
   }

   for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
   {
      pMB = ( dmsMB* )( gMMEBuff + i * sizeof( dmsMB ) ) ;
      ossStrncpy( collectionName, pMB->_collectionName,
                  DMS_COLLECTION_NAME_SZ ) ;
      if ( 0 == ossStrcmp( gCLName, collectionName ) )
      {
         repaireCollection( file, pMB, i ) ;
         /// found
         goto done ;
      }
   }

   dumpPrintf( "Not found collection[%s] in space[%s]" OSS_NEWLINE,
               gCLName, gCSName ) ;

done:
   return ;
}

void dumpCollections ( OSSFILE &file, UINT32 pageSize, SINT32 maxPages, CHAR *pSmeBuffer )
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   vector<UINT16> collections ;

   if ( FALSE == gInitMME )
   {
      SDB_ASSERT( FALSE, "MME should be init before" ) ;
      rc = SDB_SYS ;

      dumpPrintf( "*** Error: MME is not init" OSS_NEWLINE ) ;
      goto error ;
   }

retry :
   // attempt to dump to gBuffer, if gBuffer is not large enough, we have to
   // reallocBuffer and try again
   collections.clear() ;
   len = dmsDump::dumpMME ( gMMEBuff, DMS_MME_SZ,
                            gBuffer, gBufferSize, NULL,
                            gDumpType,
                            ossStrlen ( gCLName ) ? gCLName : NULL,
                            collections,
                            gForce ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( SDB_OK != ( rc = reallocBuffer() ) )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }

   // only for data file
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      flushOutput ( gBuffer, len ) ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      CHAR *pBME = NULL ;
      SINT32 err = 0 ;

      if ( !gBMEBuff )
      {
         gBMEBuff = (CHAR *)SDB_OSS_MALLOC( DMS_BME_SZ ) ;
         if ( !gBMEBuff )
         {
            dumpPrintf( "*** Error: Allocate bucket memory(%d) failed" OSS_NEWLINE,
                        DMS_BME_SZ ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }

      //load bme
      pBME = gBMEBuff ;

      rc = getBME( file, pBME ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = parseLobmBMEAndPages( file, pageSize, maxPages, pSmeBuffer, pBME,
                                 gMapLobCLPages, gLobBMEStat, &err ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( !gOnlyMeta )
      {
         UINT32 unusedPageSize = 0 ;
         UINT32 unusedPageNum = 0 ;
         dumpHeader( gLobdFile, unusedPageSize, unusedPageNum ) ;
      }
   }

   // let's dump individual collections in collection list
   if ( collections.size() != 0 )
   {
      dumpCollections ( file, pageSize, collections ) ;
   }

done :
   return ;
error :
   goto done ;
}

enum SDB_INSPT_ACTION
{
   SDB_INSPT_ACTION_DUMP = 0,
   SDB_INSPT_ACTION_INSPECT,
   SDB_INSPT_ACTION_REPARE,
   SDB_INSPT_ACTION_RECOVER
} ;

void inspectLob( OSSFILE &file, const CHAR *pFileName )
{
   INT32 totalErr                   = 0 ;
   SINT32 hwm                       = -1 ;
   CHAR *pExpSMEInfo                = NULL ;
   INT64 fileSize                   = 0 ;
   INT64 lobdFileSize               = 0 ;
   UINT32 rc                        = SDB_OK ;

   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;

   gInspectFileStat.reset() ;

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Inspect collection space %s" OSS_NEWLINE, pFileName ) ;

   /// 1. get file size
   rc = ossGetFileSizeByName( pFileName, &fileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  pFileName, rc ) ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._fileSize = fileSize ;

   /// 2. inspect lobm header, re-init gPageNum, gLobmPageSize
   rc = inspectLobmHeader ( file, fileSize, totalErr ) ;
   if ( rc )
   {
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._pageNum = gPageNum ;

   /// check page size
   if ( gLobmPageSize != DMS_PAGE_SIZE64B &&
        gLobmPageSize != DMS_PAGE_SIZE256B ) /// old version lob
   {
      dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                    pFileName, gLobmPageSize ) ;
      ++totalErr ;
      rc = SDB_DMS_INVALID_PAGESIZE ;
      goto error ;
   }

   if ( gLobdPageSize != DMS_PAGE_SIZE4K &&
        gLobdPageSize != DMS_PAGE_SIZE8K &&
        gLobdPageSize != DMS_PAGE_SIZE16K &&
        gLobdPageSize != DMS_PAGE_SIZE32K &&
        gLobdPageSize != DMS_PAGE_SIZE64K &&
        gLobdPageSize != DMS_PAGE_SIZE128K &&
        gLobdPageSize != DMS_PAGE_SIZE256K &&
        gLobdPageSize != DMS_PAGE_SIZE512K )
   {
      dumpPrintf ( "*** Error: %s lob data page size is not valid: %d" OSS_NEWLINE,
                   pFileName, gLobdPageSize ) ;
      rc = SDB_DMS_INVALID_PAGESIZE ;
      ++totalErr ;
      goto error ;
   }

   /// 3. inspect sme
   inspectSME( file, NULL, hwm, totalErr ) ;
   gInspectFileStat._hwmPageID = hwm ;
   gInspectFileStat._usedPages = gPageUsedNum ;

   // allocate expected SME for global collectionspace inspect only
   if ( ossStrlen( gCLName ) == 0 &&
        ( FALSE == gOnlyMeta || OSS_BIT_TEST( gAction, ACTION_STAT ) ) )
   {
      // allocate memory for expected SME
      // this buffer is used to store all allocated pages from collection
      // traversal, the result will be used to compare with real SME for
      // orphan/inconsistent pages
      rc = gInspectSMEInfo.init() ;
      if ( rc )
      {
         dumpPrintf ( "*** Waring: Failed to allocate %u bytes for inspect "
                      "SME buffer" OSS_NEWLINE, DMS_SME_SZ ) ;
         /// ignore error
      }
      else
      {
         pExpSMEInfo = (CHAR*)&gInspectSMEInfo ;
      }
   }

   /// 4. inspect lobd header
   rc = ossGetFileSizeByName( gLobdFileName.c_str(), &lobdFileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  gLobdFileName.c_str(), rc ) ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._fileSize += lobdFileSize ;

   rc = inspectLobdHeader( lobdFileSize, totalErr ) ;
   if ( rc )
   {
      goto error ;
   }

   /// 5. inspect collections
   inspectLobCollections ( file, gLobmPageSize, hwm, pExpSMEInfo, totalErr ) ;

   /// 6. calc stat
   if ( gInspectFileStat._pageNum > gInspectFileStat._usedPages )
   {
      gInspectFileStat._freePageSize = ( gInspectFileStat._pageNum - gInspectFileStat._usedPages ) *
                                       (UINT64)gLobdPageSize ;
   }

   if ( (INT32)gInspectFileStat._pageNum > gInspectFileStat._hwmPageID + 1 )
   {
      /// lobm
      /*
      gInspectFileStat._freeTailSize = fileSize - gDataOffset -
                                       ( gInspectFileStat._hwmPageID + 1 ) * (UINT64)gLobmPageSize ;
      */
      /// lobd
      gInspectFileStat._freeTailSize += ( lobdFileSize - DMS_HEADER_SZ -
                                          ( gInspectFileStat._hwmPageID + 1 ) * (UINT64)gLobdPageSize ) ;
   }
   gInspectFileStat._totalFreeSize += gInspectFileStat._freePageSize ;

done:
   gStat._totalErr += totalErr ;
   gStat._totalLobFileNum += 1 ;
   gStat._totalLobFileSize += gInspectFileStat._fileSize ;
   gStat._totalFreePageSize += gInspectFileStat._freePageSize ;
   gStat._totalFreeTailSize += gInspectFileStat._freeTailSize ;
   gStat._totalFreeSize += gInspectFileStat._totalFreeSize ;

   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( 0 == totalErr )
   {
      dumpPrintf ( "Inspect collection space is Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Inspect collection space is Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                    totalErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The collection space info ++++" OSS_NEWLINE
                  "  File Size       : %.2f (MB) (lobm and lobd)" OSS_NEWLINE
                  "  Pages Num       : %u" OSS_NEWLINE
                  "  Used Pages      : %u" OSS_NEWLINE
                  "  Max Used PageID : %d" OSS_NEWLINE
                  "  Free Page Size  : %.2f (MB)" OSS_NEWLINE
                  "  Free Tail Size  : %.2f (MB)" OSS_NEWLINE
                  "  Total Free Size : %.2f (MB)" OSS_NEWLINE,
                  (FLOAT64)gInspectFileStat._fileSize / MB_SIZE,
                  gInspectFileStat._pageNum,
                  gInspectFileStat._usedPages,
                  gInspectFileStat._hwmPageID,
                  (FLOAT64)gInspectFileStat._freePageSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._freeTailSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._totalFreeSize / MB_SIZE ) ;
   }

   return ;
error :
   goto done;
}

void inspectData( OSSFILE &file, const CHAR *pFileName )
{
   INT32 totalErr          = 0 ;
   SINT32 hwm              = -1 ;
   UINT32 csPageSize       = 0 ;
   CHAR *pExpSMEInfo       = NULL ;
   INT64 fileSize          = 0 ;
   INT32 rc                = SDB_OK ;

   UINT64 beginTime        = 0 ;
   UINT64 endTime          = 0 ;
   FLOAT64 timeSpan        = 0.0 ;

   gInspectFileStat.reset() ;

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Inspect collection space %s" OSS_NEWLINE, pFileName ) ;

   /// 1. get file size
   rc = ossGetFileSizeByName( pFileName, &fileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  pFileName, rc ) ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._fileSize = fileSize ;

   /// 2. inspect header
   inspectHeader ( file, csPageSize, totalErr ) ;
   if ( csPageSize != DMS_PAGE_SIZE4K &&
        csPageSize != DMS_PAGE_SIZE8K &&
        csPageSize != DMS_PAGE_SIZE16K &&
        csPageSize != DMS_PAGE_SIZE32K &&
        csPageSize != DMS_PAGE_SIZE64K )
   {
      dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                   pFileName, csPageSize ) ;
      rc = SDB_DMS_INVALID_PAGESIZE ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._pageNum = gPageNum ;

   if ( !DMS_IS_VALID_SEGMENT( gSegmentSize ) )
   {
      dumpPrintf( "*** Error: %s segment size is invalid: %d" OSS_NEWLINE,
                  pFileName, gSegmentSize ) ;
      rc = SDB_SYS ;
      ++totalErr ;
      goto error ;
   }

   /// 3. inspect sme
   inspectSME ( file, NULL, hwm, totalErr ) ;
   gInspectFileStat._hwmPageID = hwm ;
   gInspectFileStat._usedPages = gPageUsedNum ;

   // allocate expected SME for global collectionspace inspect only
   if ( ossStrlen( gCLName ) == 0 &&
        ( FALSE == gOnlyMeta || OSS_BIT_TEST( gAction, ACTION_STAT ) ) )
   {
      // allocate memory for expected SME
      // this buffer is used to store all allocated pages from collection
      // traversal, the result will be used to compare with real SME for
      // orphan/inconsistent pages
      rc = gInspectSMEInfo.init() ;
      if ( rc )
      {
         dumpPrintf ( "*** Waring: Failed to allocate %u bytes for inspect "
                      "SME buffer" OSS_NEWLINE, DMS_SME_SZ ) ;
         /// ignore error
      }
      else
      {
         pExpSMEInfo = (CHAR*)&gInspectSMEInfo ;
      }
   }

   inspectCollections ( file, csPageSize, hwm, pExpSMEInfo, totalErr ) ;

   /// calc stat
   if ( gInspectFileStat._pageNum > gInspectFileStat._usedPages )
   {
      gInspectFileStat._freePageSize = ( gInspectFileStat._pageNum - gInspectFileStat._usedPages ) *
                                       (UINT64)gPageSize ;
   }

   if ( (INT32)gInspectFileStat._pageNum > gInspectFileStat._hwmPageID + 1 )
   {
      gInspectFileStat._freeTailSize += ( fileSize - gDataOffset -
                                          ( gInspectFileStat._hwmPageID + 1 ) * (UINT64)gPageSize ) ;
   }
   gInspectFileStat._totalFreeSize += gInspectFileStat._freePageSize ;

done:
   gStat._totalErr += totalErr ;
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      gStat._totalDataFileNum += 1 ;
      gStat._totalDataFileSize += gInspectFileStat._fileSize ;
   }
   else
   {
      gStat._totalIndexFileNum += 1 ;
      gStat._totalIndexFileSize += gInspectFileStat._fileSize ;
   }
   gStat._totalFreePageSize += gInspectFileStat._freePageSize ;
   gStat._totalFreeTailSize += gInspectFileStat._freeTailSize ;
   gStat._totalFreeSize += gInspectFileStat._totalFreeSize ;

   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( 0 == totalErr )
   {
      dumpPrintf ( "Inspect collection space is Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Inspect collection space is Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                    totalErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The collection space info ++++" OSS_NEWLINE
                  "  File Size       : %.2f (MB)" OSS_NEWLINE
                  "  Pages Num       : %u" OSS_NEWLINE
                  "  Used Pages      : %u" OSS_NEWLINE
                  "  Max Used PageID : %d" OSS_NEWLINE
                  "  Free Page Size  : %.2f (MB)" OSS_NEWLINE
                  "  Free Tail Size  : %.2f (MB)" OSS_NEWLINE
                  "  Total Free Size : %.2f (MB)" OSS_NEWLINE,
                  (FLOAT64)gInspectFileStat._fileSize / MB_SIZE,
                  gInspectFileStat._pageNum,
                  gInspectFileStat._usedPages,
                  gInspectFileStat._hwmPageID,
                  (FLOAT64)gInspectFileStat._freePageSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._freeTailSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._totalFreeSize / MB_SIZE ) ;
   }

   return ;
error:
   goto done ;
}

void dumpData( OSSFILE &file, const CHAR *pFileName )
{
   SINT32 hwm              = -1 ;
   UINT32 csPageSize       = 0 ;
   INT64 fileSize          = 0 ;
   INT64 lobdFileSize      = 0 ;
   INT32 rc                = SDB_OK ;

   UINT64 beginTime        = 0 ;
   UINT64 endTime          = 0 ;
   FLOAT64 timeSpan        = 0.0 ;

   gInspectFileStat.reset() ;

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Dump collection space %s" OSS_NEWLINE, pFileName ) ;

   /// 1. get file size
   rc = ossGetFileSizeByName( pFileName, &fileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  pFileName, rc ) ;
      goto error ;
   }
   gInspectFileStat._fileSize = fileSize ;

   if ( SDB_INSPT_LOB == gCurInsptType )
   {
      /// get lobd file size
      rc = ossGetFileSizeByName( gLobdFileName.c_str(), &lobdFileSize ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                     gLobdFileName.c_str(), rc ) ;
         goto error ;
      }
      gInspectFileStat._fileSize += lobdFileSize ;
   }

   dumpHeader( file, csPageSize, gPageNum ) ;

   // page size sanity check, should we need header sanity check function?
   // hmm.. maybe later :)

   if ( SDB_INSPT_LOB == gCurInsptType )
   {
      UINT32 tmpPageSize = 0 ;
      UINT32 tmpPageNum = 0 ;

      if ( csPageSize != DMS_PAGE_SIZE64B &&
           csPageSize != DMS_PAGE_SIZE256B ) /// old version lob
      {
         dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                       pFileName, csPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }

      if ( gLobdPageSize != DMS_PAGE_SIZE4K &&
           gLobdPageSize != DMS_PAGE_SIZE8K &&
           gLobdPageSize != DMS_PAGE_SIZE16K &&
           gLobdPageSize != DMS_PAGE_SIZE32K &&
           gLobdPageSize != DMS_PAGE_SIZE64K &&
           gLobdPageSize != DMS_PAGE_SIZE128K &&
           gLobdPageSize != DMS_PAGE_SIZE256K &&
           gLobdPageSize != DMS_PAGE_SIZE512K )
      {
         dumpPrintf ( "*** Error: %s lob data page size is not valid: %d" OSS_NEWLINE,
                      pFileName, gLobdPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }

      dumpHeader( gLobdFile, tmpPageSize, tmpPageNum ) ;
   }
   else
   {
      if ( csPageSize != DMS_PAGE_SIZE4K &&
           csPageSize != DMS_PAGE_SIZE8K &&
           csPageSize != DMS_PAGE_SIZE16K &&
           csPageSize != DMS_PAGE_SIZE32K &&
           csPageSize != DMS_PAGE_SIZE64K )
      {
         dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                       pFileName, csPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }
   }

   gInspectFileStat._pageNum = gPageNum ;

   dumpSME( file, gSMEBuff, hwm ) ;
   gInspectFileStat._hwmPageID = hwm ;
   gInspectFileStat._usedPages = gPageUsedNum ;

   dumpCollections( file, csPageSize, hwm, gSMEBuff ) ;

   /// calc stat
   if ( gInspectFileStat._pageNum > gInspectFileStat._usedPages )
   {
      if ( SDB_INSPT_LOB == gCurInsptType )
      {
         gInspectFileStat._freePageSize = ( gInspectFileStat._pageNum - gInspectFileStat._usedPages ) *
                                          (UINT64)gLobdPageSize ;

      }
      else
      {
         gInspectFileStat._freePageSize = ( gInspectFileStat._pageNum - gInspectFileStat._usedPages ) *
                                          (UINT64)csPageSize ;
      }
   }

   if ( (INT32)gInspectFileStat._pageNum > gInspectFileStat._hwmPageID + 1 )
   {
      if ( SDB_INSPT_LOB == gCurInsptType )
      {
         /// lobd
         gInspectFileStat._freeTailSize += ( lobdFileSize - DMS_HEADER_SZ -
                                             ( gInspectFileStat._hwmPageID + 1 ) * (UINT64)gLobdPageSize ) ;
      }
      else
      {
         gInspectFileStat._freeTailSize += ( fileSize - gDataOffset -
                                             ( gInspectFileStat._hwmPageID + 1 ) * (UINT64)csPageSize ) ;
      }
   }
   gInspectFileStat._totalFreeSize += gInspectFileStat._freePageSize ;

done:
   if ( SDB_INSPT_LOB == gCurInsptType )
   {
      gStat._totalLobFileNum += 1 ;
      gStat._totalLobFileSize += gInspectFileStat._fileSize ;
   }
   else if ( SDB_INSPT_DATA == gCurInsptType )
   {
      gStat._totalDataFileNum += 1 ;
      gStat._totalDataFileSize += gInspectFileStat._fileSize ;
   }
   else
   {
      gStat._totalIndexFileNum += 1 ;
      gStat._totalIndexFileSize += gInspectFileStat._fileSize ;
   }
   gStat._totalFreePageSize += gInspectFileStat._freePageSize ;
   gStat._totalFreeTailSize += gInspectFileStat._freeTailSize ;
   gStat._totalFreeSize += gInspectFileStat._totalFreeSize ;

   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf ( "Dump collection space Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Dump collection space Done Failed: %d   [%.2f s]" OSS_NEWLINE,
                    rc, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The collection space info ++++" OSS_NEWLINE
                  "  File Size       : %.2f (MB)" OSS_NEWLINE
                  "  Pages Num       : %u" OSS_NEWLINE
                  "  Used Pages      : %u" OSS_NEWLINE
                  "  Max Used PageID : %d" OSS_NEWLINE
                  "  Free Page Size  : %.2f (MB)" OSS_NEWLINE
                  "  Free Tail Size  : %.2f (MB)" OSS_NEWLINE
                  "  Total Free Size : %.2f (MB)" OSS_NEWLINE,
                  (FLOAT64)gInspectFileStat._fileSize / MB_SIZE,
                  gInspectFileStat._pageNum,
                  gInspectFileStat._usedPages,
                  gInspectFileStat._hwmPageID,
                  (FLOAT64)gInspectFileStat._freePageSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._freeTailSize / MB_SIZE,
                  (FLOAT64)gInspectFileStat._totalFreeSize / MB_SIZE ) ;
   }

   return ;
error:
   goto done ;
}

void recoverLob( OSSFILE &file, const CHAR *pFileName )
{
   INT32 totalErr                   = 0 ;
   SINT32 hwm                       = -1 ;
   INT64 fileSize                   = 0 ;
   UINT32 rc                        = SDB_OK ;

   /*
   UINT64 beginTime = 0 ;
   UINT64 endTime = 0 ;
   FLOAT64 timeSpan = 0.0 ;
   */

   gInspectFileStat.reset() ;

   // beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Recover collection space %s" OSS_NEWLINE, pFileName ) ;

   /// 1. get file size
   rc = ossGetFileSizeByName( pFileName, &fileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  pFileName, rc ) ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._fileSize = fileSize ;

   /// 2. inspect lobm header, re-init gPageNum, gLobmPageSize
   rc = inspectLobmHeader ( file, fileSize, totalErr ) ;
   if ( rc )
   {
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._pageNum = gPageNum ;

   /// check page size
   if ( gLobmPageSize != DMS_PAGE_SIZE64B &&
        gLobmPageSize != DMS_PAGE_SIZE256B ) /// old version lob
   {
      dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                    pFileName, gLobmPageSize ) ;
      ++totalErr ;
      rc = SDB_DMS_INVALID_PAGESIZE ;
      goto error ;
   }

   /// 3. inspect sme
   inspectSME( file, NULL, hwm, totalErr ) ;
   gInspectFileStat._hwmPageID = hwm ;
   gInspectFileStat._usedPages = gPageUsedNum ;

   /// 5. inspect collections
   recoverCollections( file, gLobmPageSize, totalErr ) ;

done:
   gStat._totalErr += totalErr ;
   gStat._totalLobFileNum += 1 ;
   gStat._totalLobFileSize += gInspectFileStat._fileSize ;
   gStat._totalFreePageSize += gInspectFileStat._freePageSize ;
   gStat._totalFreeTailSize += gInspectFileStat._freeTailSize ;
   gStat._totalFreeSize += gInspectFileStat._totalFreeSize ;

   /*
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( 0 == totalErr )
   {
      dumpPrintf ( "Inspect collection space is Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Inspect collection space is Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                    totalErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The collection space info ++++" OSS_NEWLINE
                  "  File Size       : %.2f (MB) (lobm and lobd)" OSS_NEWLINE
                  "  Pages Num       : %u" OSS_NEWLINE,
                  (FLOAT64)gInspectFileStat._fileSize / MB_SIZE,
                  gInspectFileStat._pageNum ) ;
   }
   */

   return ;
error :
   goto done;
}

void recoverData( OSSFILE &file, const CHAR *pFileName )
{
   INT32 totalErr          = 0 ;
   SINT32 hwm              = -1 ;
   UINT32 csPageSize       = 0 ;
   INT64 fileSize          = 0 ;
   INT32 rc                = SDB_OK ;

   /*
   UINT64 beginTime        = 0 ;
   UINT64 endTime          = 0 ;
   FLOAT64 timeSpan        = 0.0 ;
   */

   gInspectFileStat.reset() ;

   // beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Recover collection space %s" OSS_NEWLINE, pFileName ) ;

   /// 1. get file size
   rc = ossGetFileSizeByName( pFileName, &fileSize ) ;
   if ( rc )
   {
      dumpPrintf( "*** Error: Get file size failed, file: %s, rc: %d" OSS_NEWLINE,
                  pFileName, rc ) ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._fileSize = fileSize ;

   /// 2. inspect header
   inspectHeader ( file, csPageSize, totalErr ) ;
   if ( csPageSize != DMS_PAGE_SIZE4K &&
        csPageSize != DMS_PAGE_SIZE8K &&
        csPageSize != DMS_PAGE_SIZE16K &&
        csPageSize != DMS_PAGE_SIZE32K &&
        csPageSize != DMS_PAGE_SIZE64K )
   {
      dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                   pFileName, csPageSize ) ;
      rc = SDB_DMS_INVALID_PAGESIZE ;
      ++totalErr ;
      goto error ;
   }
   gInspectFileStat._pageNum = gPageNum ;

   /// 3. inspect sme
   inspectSME ( file, NULL, hwm, totalErr ) ;
   gInspectFileStat._hwmPageID = hwm ;
   gInspectFileStat._usedPages = gPageUsedNum ;

   recoverCollections( file, csPageSize, totalErr ) ;

done:
   gStat._totalErr += totalErr ;
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      gStat._totalDataFileNum += 1 ;
      gStat._totalDataFileSize += gInspectFileStat._fileSize ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      gStat._totalIndexFileNum += 1 ;
      gStat._totalIndexFileSize += gInspectFileStat._fileSize ;
   }
   else
   {
      gStat._totalLobFileNum += 1 ;
      gStat._totalLobFileSize += gInspectFileStat._fileSize ;
   }

   gStat._totalFreePageSize += gInspectFileStat._freePageSize ;
   gStat._totalFreeTailSize += gInspectFileStat._freeTailSize ;
   gStat._totalFreeSize += gInspectFileStat._totalFreeSize ;

   /*
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( 0 == totalErr )
   {
      dumpPrintf ( "Recover collection space is Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Recover collection space is Done with Errors: %d   [%.2f s]" OSS_NEWLINE,
                    totalErr, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The collection space info ++++" OSS_NEWLINE
                  "  File Size       : %.2f (MB)" OSS_NEWLINE
                  "  Pages Num       : %u" OSS_NEWLINE,
                  (FLOAT64)gInspectFileStat._fileSize / MB_SIZE,
                  gInspectFileStat._pageNum ) ;
   }
   */

   return ;
error:
   goto done ;
}

void dumpPage( OSSFILE &file, const CHAR *pFileName, SINT32 beginPageID, SINT32 pageNum )
{
   UINT32 csPageSize       = 0 ;
   INT32 rc                = SDB_OK ;
   UINT32 totalPages       = 0 ;

   UINT64 beginTime        = 0 ;
   UINT64 endTime          = 0 ;
   FLOAT64 timeSpan        = 0.0 ;

   gInspectFileStat.reset() ;

   beginTime = ossGetCurrentMilliseconds() ;

   dumpPrintf ( "Dump pages[%d, %d] for collection space %s" OSS_NEWLINE,
                beginPageID, beginPageID + pageNum - 1, pFileName ) ;

   dumpHeader( file, csPageSize, gPageNum ) ;

   // page size sanity check, should we need header sanity check function?
   // hmm.. maybe later :)
   if ( SDB_INSPT_LOB == gCurInsptType )
   {
      UINT32 tmpPageSize = 0 ;
      UINT32 tmpPageNum = 0 ;

      if ( csPageSize != DMS_PAGE_SIZE64B &&
           csPageSize != DMS_PAGE_SIZE256B ) /// old version lob
      {
         dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                       pFileName, csPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }

      if ( gLobdPageSize != DMS_PAGE_SIZE4K &&
           gLobdPageSize != DMS_PAGE_SIZE8K &&
           gLobdPageSize != DMS_PAGE_SIZE16K &&
           gLobdPageSize != DMS_PAGE_SIZE32K &&
           gLobdPageSize != DMS_PAGE_SIZE64K &&
           gLobdPageSize != DMS_PAGE_SIZE128K &&
           gLobdPageSize != DMS_PAGE_SIZE256K &&
           gLobdPageSize != DMS_PAGE_SIZE512K )
      {
         dumpPrintf ( "*** Error: %s lob data page size is not valid: %d" OSS_NEWLINE,
                      pFileName, gLobdPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }

      dumpHeader( gLobdFile, tmpPageSize, tmpPageNum ) ;
   }
   else
   {
      if ( csPageSize != DMS_PAGE_SIZE4K &&
           csPageSize != DMS_PAGE_SIZE8K &&
           csPageSize != DMS_PAGE_SIZE16K &&
           csPageSize != DMS_PAGE_SIZE32K &&
           csPageSize != DMS_PAGE_SIZE64K )
      {
         dumpPrintf ( "*** Error: %s page size is not valid: %d" OSS_NEWLINE,
                       pFileName, csPageSize ) ;
         rc = SDB_DMS_INVALID_PAGESIZE ;
         goto error ;
      }
   }

   // specific pages dump
   for ( SINT32 i = 0 ; i < pageNum && !gReachEnd ; ++i )
   {
      ++totalPages ;
      dumpPrintf( " Dump page %d" OSS_NEWLINE, beginPageID + i ) ;
      dumpRawPage( file, csPageSize, beginPageID + i ) ;
   }

   if ( SDB_INSPT_LOB == gCurInsptType )
   {
      gDataOffset = DMS_SME_OFFSET ;
      for ( SINT32 i = 0 ; i < pageNum && !gReachEnd ; ++i )
      {
         dumpPrintf ( " Dump lob data page %d" OSS_NEWLINE, beginPageID + i ) ;
         dumpRawPage ( gLobdFile, gLobdPageSize, beginPageID + i ) ;
      }
   }

done:
   endTime = ossGetCurrentMilliseconds() ;
   if ( endTime > beginTime )
   {
      timeSpan = ( (FLOAT64)endTime - beginTime ) / OSS_ONE_SEC ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf ( "Dump pages for collection space Done Succeed   [%.2f s]" OSS_NEWLINE,
                   timeSpan ) ;
   }
   else
   {
      dumpPrintf ( "Dump pages for collection space Done Failed: %d   [%.2f s]" OSS_NEWLINE,
                    rc, timeSpan ) ;
   }

   if ( SDB_OK == rc )
   {
      dumpPrintf( "++++ The dump pages info ++++" OSS_NEWLINE
                  "  Total Pages     : %u" OSS_NEWLINE,
                  totalPages ) ;
   }

   return ;
error:
   goto done ;
}

void actionCSAttempt ( const CHAR *pFileName, vector<const CHAR *> &expectEyeVec,
                       BOOLEAN specific, SDB_INSPT_ACTION action )
{
   INT32    rc = SDB_OK ;
   CHAR     eyeCatcher[DMS_HEADER_EYECATCHER_LEN+1] = {0} ;
   SINT64   readSize = 0 ;

   SINT64   restLen = 0 ;
   SINT64   readPos = 0 ;
   UINT32   iMode = OSS_DEFAULT | OSS_EXCLUSIVE ;
   BOOLEAN  csValid = FALSE ;

   if ( SDB_INSPT_ACTION_REPARE == action || SDB_INSPT_ACTION_RECOVER == action )
   {
      iMode |= OSS_READWRITE ;
   }
   else
   {
      iMode |= OSS_READONLY ;
   }

   rc = ossOpen ( pFileName, iMode, OSS_RU | OSS_WU | OSS_RG, gFile ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to open %s, rc: %d" OSS_NEWLINE,
                   pFileName, rc ) ;
      goto error ;
   }

   // first let's read 8 bytes in front of the file, and make sure it's our
   // storage unit file
   restLen = DMS_HEADER_EYECATCHER_LEN ;
   while ( restLen > 0 )
   {
      rc = ossRead ( &gFile, eyeCatcher + readPos, restLen, &readSize ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         dumpPrintf ( "*** Error: Failed to read %s, rc: %d" OSS_NEWLINE,
                      pFileName, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= readSize ;
      readPos += readSize ;
   }

   for ( vector<const CHAR *>::iterator itr = expectEyeVec.begin() ;
         itr != expectEyeVec.end() ;
         ++itr )
   {
      if ( 0 == ossStrncmp( eyeCatcher, *itr, DMS_HEADER_EYECATCHER_LEN ) )
      {
         csValid = TRUE ;
         break ;
      }
   }

   // if it doens't match our eye catcher, we may or may not dump error
   if ( !csValid )
   {
      // if user specified the collection space, we should dump error if it's
      // not a valid SU file
      if ( specific )
      {
         dumpPrintf ( "*** Error: %s is not a valid storage unit" OSS_NEWLINE,
                      pFileName ) ;
      }
      goto done ;
   }

   // when we get here, that means it's our SU file and let's dump header, SME,
   // and all collections
   rc = ossSeek ( &gFile, 0, OSS_SEEK_SET ) ;
   if ( rc )
   {
      dumpPrintf ( "*** Error: Failed to seek to beginning of the file %s, rc: %d"
                   OSS_NEWLINE, pFileName, rc ) ;
      goto error ;
   }

   switch ( action )
   {
      case SDB_INSPT_ACTION_REPARE :
         if( SDB_INSPT_DATA == gCurInsptType )
         {
            repaireCollections( gFile ) ;
         }
         break ;
      case SDB_INSPT_ACTION_DUMP :
         if ( gStartingPage < 0 )
         {
            dumpData( gFile, pFileName ) ;
         }
         else
         {
            dumpPage( gFile, pFileName, gStartingPage, gNumPages ) ;
         }
         break ;
      case SDB_INSPT_ACTION_INSPECT :
         if ( SDB_INSPT_LOB == gCurInsptType )
         {
            inspectLob( gFile, pFileName ) ;
         }
         else
         {
            inspectData( gFile, pFileName ) ;
         }
         break ;
      case SDB_INSPT_ACTION_RECOVER :
         if ( SDB_INSPT_LOB == gCurInsptType )
         {
            recoverLob( gFile, pFileName ) ;
         }
         else
         {
            recoverData( gFile, pFileName ) ;
         }
         break ;
      default :
         dumpPrintf ( "*** Error: unexpected action" OSS_NEWLINE ) ;
         goto error ;
   }
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   // close input file
   if ( gFile.isOpened() )
   {
      clearFileCache( &gFile ) ;
      ossClose ( gFile ) ;
   }
   return ;
error :
   goto done ;
}

INT32 prepareForDump( const CHAR *csName, UINT32 sequence )
{
   string csFileName = rtnMakeSUFileName( csName, sequence,
                                          DMS_DATA_SU_EXT_NAME ) ;
   string csFullName = rtnFullPathName( gDatabasePath, csFileName ) ;

   OSSFILE file ;
   INT32 rc = SDB_OK ;
   dmsStorageUnitHeader dataHeader ;

   rc = ossOpen ( csFullName.c_str(), OSS_DEFAULT|OSS_READONLY|OSS_EXCLUSIVE,
                  OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( rc )
   {
      dumpAndShowPrintf( "*** Error: Failed to open %s, rc: %d" OSS_NEWLINE,
                         csFullName.c_str(), rc ) ;
      goto error ;
   }

   // read header and check file type
   rc = readData( &file, DMS_HEADER_OFFSET, DMS_HEADER_SZ, (CHAR *)&dataHeader, FALSE, "header" ) ;
   if ( rc )
   {
      dumpAndShowPrintf( "*** Error: Read header failed, file: %s, rc: %d" OSS_NEWLINE,
                         csFullName.c_str(), rc ) ;
      goto error ;
   }

   if ( 0 != ossStrncmp( dataHeader._eyeCatcher, DMS_DATASU_EYECATCHER,
                         DMS_HEADER_EYECATCHER_LEN ) &&
        0 != ossStrncmp( dataHeader._eyeCatcher, DMS_DATACAPSU_EYECATCHER,
                         DMS_HEADER_EYECATCHER_LEN ) )
   {
      dumpAndShowPrintf ( "*** Error: File[%s] is not dms storage unit data file" OSS_NEWLINE,
                          csFullName.c_str() ) ;
      goto error ;
   }

   // set info
   gPageSize = dataHeader._pageSize ;
   gSegmentSize = dataHeader._segmentSize ;
   gSecretValue = dataHeader._secretValue ;
   gLobdPageSize = dataHeader._lobdPageSize;
   gSequence = dataHeader._sequence;
   gExistLobs = dataHeader._createLobs;

   if ( 0 == gSegmentSize )
   {
      gSegmentSize = DMS_SEGMENT_SZ ;
   }

   // read mme and check
   rc = readData( &file, DMS_MME_OFFSET, DMS_MME_SZ, gMMEBuff, FALSE, "MME" ) ;
   if ( rc )
   {
      dumpAndShowPrintf( "*** Error: Read MME failed, file: %s, rc: %d" OSS_NEWLINE,
                         csFullName.c_str(), rc ) ;
      goto error ;
   }
   gInitMME = TRUE ;

done:
   if ( file.isOpened() )
   {
      ossClose ( file ) ;
   }
   return rc ;
error:
   if ( SDB_OK == rc )
   {
      rc = SDB_SYS ;
   }
   goto done ;
}

void actionCSAttemptEntry( const CHAR *csName, UINT32 sequence,
                           BOOLEAN specific, SDB_INSPT_ACTION action )
{
   if ( !gHitCS )
   {
      gHitCS = TRUE ;
   }

   gReachEnd = FALSE ;
   gHitError = FALSE ;

   string csFileName ;
   string csFullName ;

   // clear global info
   gPageSize      = 0 ;
   gLobdPageSize  = 0 ;
   gSegmentSize   = 0 ;
   gSecretValue   = 0 ;
   gInitMME       = FALSE ;
   gDoRecoverMBID = DMS_INVALID_MBID ;
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   gMapLobCLPages.clear() ;
   gLobBMEStat.reset() ;

   // prepare
   if ( SDB_OK != prepareForDump( csName, sequence ) )
   {
      return ;
   }

   if ( gDumpData ||
        SDB_INSPT_ACTION_REPARE == action ||
        SDB_INSPT_ACTION_RECOVER == action )
   {
      csFileName = rtnMakeSUFileName( csName, sequence,
                                      DMS_DATA_SU_EXT_NAME ) ;
      csFullName = rtnFullPathName( gDatabasePath, csFileName ) ;
      gDataOffset = DMS_MME_OFFSET + DMS_MME_SZ ;
      gPageNum    = 0 ;
      gPageUsedNum = 0 ;
      gCurInsptType = SDB_INSPT_DATA ;
      vector<const CHAR *> eyeCatcherVec ;
      eyeCatcherVec.push_back( DMS_DATASU_EYECATCHER ) ;
      eyeCatcherVec.push_back( DMS_DATACAPSU_EYECATCHER ) ;

      actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;
   }

   if ( gDumpIndex && !gHitError )
   {
      csFileName = rtnMakeSUFileName( csName, sequence,
                                      DMS_INDEX_SU_EXT_NAME ) ;
      csFullName = rtnFullPathName( gIndexPath, csFileName ) ;
      gDataOffset = DMS_SME_OFFSET + DMS_SME_SZ ;
      gPageNum    = 0 ;
      gPageUsedNum = 0 ;
      gCurInsptType = SDB_INSPT_INDEX ;
      vector<const CHAR *> eyeCatcherVec ;
      eyeCatcherVec.push_back( DMS_INDEXSU_EYECATCHER ) ;

      actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;
   }

   if ( gDumpLob && gExistLobs && !gHitError )
   {
      INT32 rc = SDB_OK ;
      csFileName =  rtnMakeSUFileName( csName, sequence, DMS_LOB_DATA_SU_EXT_NAME ) ;
      gLobdFileName = rtnFullPathName( gLobPath, csFileName ) ;

      UINT32 iMode = OSS_DEFAULT | OSS_EXCLUSIVE ;
      if ( SDB_INSPT_ACTION_REPARE == action )
      {
         iMode |= OSS_READWRITE ;
      }
      else
      {
         iMode |= OSS_READONLY ;
      }

      rc = ossOpen ( gLobdFileName.c_str(), iMode, OSS_RU | OSS_WU | OSS_RG, gLobdFile ) ;
      if ( rc != SDB_OK )
      {
         dumpPrintf ( "*** Error: Failed to open file %s, rc: %d" OSS_NEWLINE,
                      gLobdFileName.c_str(), rc ) ;
         return ;
      }

      csFileName = rtnMakeSUFileName( csName, sequence, DMS_LOB_META_SU_EXT_NAME ) ;
      csFullName = rtnFullPathName( gLobmPath, csFileName ) ;

      gDataOffset = DMS_BME_OFFSET + DMS_BME_SZ;
      gPageNum    = 0 ;
      gPageUsedNum = 0 ;
      gCurInsptType = SDB_INSPT_LOB ;
      vector<const CHAR *> eyeCatcherVec ;
      eyeCatcherVec.push_back( DMS_LOBM_EYECATCHER ) ;

      actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;

      clearFileCache( &gLobdFile ) ;
      ossClose ( gLobdFile ) ;
   }
}

void dumpPages ()
{
   CHAR csName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
   UINT32 sequence = 0 ;
   fs::path dbDir ( gDatabasePath ) ;
   fs::directory_iterator end_iter ;

   if ( ossStrlen ( gCSName ) == 0 )
   {
      dumpAndShowPrintf ( "Colletion Space Name must be specified for page dump"
                          OSS_NEWLINE ) ;
      goto done ;
   }

   if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
   {
      for ( fs::directory_iterator dir_iter ( dbDir );
            dir_iter != end_iter; ++dir_iter )
      {
         if ( fs::is_regular_file ( dir_iter->status() ) )
         {
            const std::string fileName = dir_iter->path().filename().string() ;
            const CHAR *pFileName = fileName.c_str() ;
            if ( rtnVerifyCollectionSpaceFileName ( pFileName, csName,
                                                    DMS_COLLECTION_SPACE_NAME_SZ,
                                                    sequence ) )
            {
               if ( 0 == ossStrncmp ( gCSName, csName, DMS_COLLECTION_SPACE_NAME_SZ ) )
               {
                  actionCSAttemptEntry( csName, sequence, TRUE, SDB_INSPT_ACTION_DUMP ) ;
               }
            }
         }
      }
   }
   else
   {
      // if we can't find the path, let's show error
      dumpAndShowPrintf ( "*** Error: dump path %s is not a valid directory" OSS_NEWLINE,
                          gDatabasePath ) ;
   }

done :
   return ;
}

// database inspection may entry code
void inspectDB( SDB_INSPT_ACTION action )
{
   CHAR csName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
   UINT32 sequence = 0 ;

   fs::path dbDir ( gDatabasePath ) ;
   fs::directory_iterator end_iter ;
   if ( fs::exists ( dbDir ) && fs::is_directory ( dbDir ) )
   {
      for ( fs::directory_iterator dir_iter ( dbDir );
            dir_iter != end_iter; ++dir_iter )
      {
         if ( fs::is_regular_file ( dir_iter->status() ) )
         {
            const std::string fileName = dir_iter->path().filename().string() ;
            const CHAR *pFileName = fileName.c_str() ;
            if ( rtnVerifyCollectionSpaceFileName ( pFileName, csName,
                                                    DMS_COLLECTION_SPACE_NAME_SZ,
                                                    sequence ) )
            {
               if ( ossStrlen ( gCSName ) == 0 ||
                    0 == ossStrncmp ( gCSName, csName, DMS_COLLECTION_SPACE_NAME_SZ ) )
               {
                  actionCSAttemptEntry ( csName, sequence,
                                         ossStrlen ( gCSName ) != 0 ? TRUE : FALSE,
                                         action ) ;
               }
            }
         }
      }
   }
   else
   {
      // if we can't find the path, let's show error
      dumpAndShowPrintf ( "*** Error: inspect path %s is not a valid directory" OSS_NEWLINE,
                          gDatabasePath ) ;
   }
}

INT32 prepareCompressor( OSSFILE &file, UINT32 pageSize, dmsMB *mb, UINT16 id,
                         CHAR *pExpBuffer, dmsCompressorEntry &compressorEntry,
                         SINT32 &err )
{
   INT32 rc = SDB_OK ;
   UTIL_COMPRESSOR_TYPE type = (UTIL_COMPRESSOR_TYPE)( mb->_compressorType ) ;

   dmsCompressorGuard gard( &compressorEntry, EXCLUSIVE ) ;

   compressorEntry.setCompressor( getCompressorByType( type ) ) ;
   compressorEntry.setFlags( mb->_compressFlags ) ;

   if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
   {
      /* LZW compression, need to load the dictionary. */
      INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DICT ;
      rc = loadExtent( file, extentType, pageSize, mb->_dictExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "*** Error: Failed to load dictionary extent %d, rc: %d" OSS_NEWLINE,
                     mb->_dictExtentID, rc ) ;
         goto error ;
      }

      if ( pExpBuffer )
      {
         inspectDictPageState( pExpBuffer, mb->_dictExtentID, err ) ;
      }

      compressorEntry.setDictionary( gDictBuffer + DMS_DICTEXTENT_HEADER_SZ ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

// main function
INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   po::options_description desc ( "Command options" ) ;
   init ( desc ) ;
   rc = resolveArgument ( desc, argc, argv ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
      {
         dumpAndShowPrintf ( "*** Error: Invalid arguments" OSS_NEWLINE ) ;
         displayArg ( desc ) ;
         goto error ;
      }
      else
      {
         rc = SDB_OK ;
         goto done ;
      }
   }

   // allocate mme buffer
   gMMEBuff = (CHAR*)SDB_OSS_MALLOC( DMS_MME_SZ ) ;
   if ( !gMMEBuff )
   {
      dumpAndShowPrintf ( "*** Error: Failed to allocate mme buffer, exit" OSS_NEWLINE ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   // allocate sme buffer
   gSMEBuff = (CHAR*)SDB_OSS_MALLOC( DMS_SME_SZ ) ;
   if ( !gSMEBuff )
   {
      dumpAndShowPrintf ( "*** Error: Failed to allocate sme buffer, exit" OSS_NEWLINE ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( gSMEBuff, 0, DMS_SME_SZ ) ;

   /// allocate dict buff
   gDictBuffer = (CHAR*)SDB_OSS_MALLOC( UTIL_MAX_DICT_TOTAL_SIZE ) ;
   if ( !gDictBuffer )
   {
      dumpAndShowPrintf( "*** Error: Failed to allocate dictionary buffer, exit" OSS_NEWLINE ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset( gDictBuffer, 0, UTIL_MAX_DICT_TOTAL_SIZE ) ;

   // allocate some buffer initially
   rc = reallocBuffer () ;
   if ( rc )
   {
      dumpAndShowPrintf ( "*** Error: Failed to realloc buffer, exit" OSS_NEWLINE ) ;
      goto error ;
   }
   // allocate a fake EDUCB
   cb = SDB_OSS_NEW pmdEDUCB ( NULL, EDU_TYPE_AGENT ) ;
   if ( !cb )
   {
      dumpAndShowPrintf ( "*** Error: Failed to allocate memory for educb, exit" OSS_NEWLINE ) ;
      rc = SDB_OOM ;
      goto error ;
   }

   // init time
   gStat._startTime = ossGetCurrentMilliseconds() ;

   // we doing database inspection
   if ( OSS_BIT_TEST ( gAction, ACTION_INSPECT ) ||
        OSS_BIT_TEST ( gAction, ACTION_STAT ) )
   {
      inspectDB( SDB_INSPT_ACTION_INSPECT ) ;
   }

   if ( OSS_BIT_TEST ( gAction, ACTION_REPAIRE ) )
   {
      /// repaire db
      inspectDB( SDB_INSPT_ACTION_REPARE ) ;
   }

   if ( OSS_BIT_TEST( gAction, ACTION_RECOVER ) )
   {
      /// recover db
      inspectDB( SDB_INSPT_ACTION_RECOVER ) ;
   }

   // we doing database dump
   if ( OSS_BIT_TEST ( gAction, ACTION_DUMP ) )
   {
      // dump specific pages
      if ( gStartingPage >= 0 )
      {
         dumpPages() ;
      }
      else
      {
         // if we don't specify pages to dump, let's dump entire database
         inspectDB( SDB_INSPT_ACTION_DUMP ) ;
      }
   }

   if ( 0 != ossStrlen( gCSName ) && !gHitCS )
   {
      dumpAndShowPrintf( "Warning: Cannot find any collection space named %s" OSS_NEWLINE,
                         gCSName ) ;
   }

   /// end time
   gStat._endTime = ossGetCurrentMilliseconds() ;
   if ( gStat._endTime < gStat._startTime )
   {
      gStat._endTime = gStat._startTime ;
   }

   /// print result
   {
      CHAR szStartTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      CHAR szEndTime[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      FLOAT64 timesRatio = 0.0 ;
      FLOAT64 readSpeed = 0.0 ;
      FLOAT64 writeSpeed = 0.0 ;
      ossTimestamp tmpTime ;
      FLOAT64 cost = gStat._endTime - gStat._startTime ;
      FLOAT64 totalSize = 0 ;

      totalSize = gStat._totalDataFileSize + gStat._totalIndexFileSize + gStat._totalLobFileSize ;

      tmpTime.time = gStat._startTime / OSS_ONE_SEC ;
      tmpTime.microtm = ( gStat._startTime % OSS_ONE_SEC ) * 1000 ;
      ossTimestampToString( tmpTime, szStartTime ) ;

      tmpTime.time = gStat._endTime / OSS_ONE_SEC ;
      tmpTime.microtm = ( gStat._endTime % OSS_ONE_SEC ) * 1000 ;
      ossTimestampToString( tmpTime, szEndTime ) ;

      /// calc ratio
      if ( gStat._readTimes > gStat._ioReadTimes && gStat._readTimes > 0 )
      {
         timesRatio = (FLOAT64)( gStat._readTimes - gStat._ioReadTimes ) / gStat._readTimes ;
      }

      if ( cost >= OSS_ONE_SEC )
      {
         readSpeed = (FLOAT64)gStat._readBytes / ( cost / OSS_ONE_SEC ) / MB_SIZE ;
         writeSpeed = (FLOAT64)gStat._writeBytes / ( cost / OSS_ONE_SEC ) / MB_SIZE ;
      }

      dumpAndShowPrintf( OSS_NEWLINE ) ;
      dumpAndShowPrintf( "++++++++  Result report  ++++++++" OSS_NEWLINE ) ;
      dumpAndShowPrintf( "  Start Time             : %s" OSS_NEWLINE, szStartTime ) ;
      dumpAndShowPrintf( "  End Time               : %s" OSS_NEWLINE, szEndTime ) ;

      if ( cost >= 300 * OSS_ONE_SEC )
      {
         dumpAndShowPrintf( "  Cost                   : %.2f (min)" OSS_NEWLINE,
                            cost / ( 60 * OSS_ONE_SEC ) ) ;
      }
      else
      {
         dumpAndShowPrintf( "  Cost                   : %.2f (sec)" OSS_NEWLINE,
                            cost / OSS_ONE_SEC ) ;
      }

      if ( ACTION_DUMP != gAction )
      {
         dumpAndShowPrintf( "  Total Errors           : %u" OSS_NEWLINE, gStat._totalErr ) ;
      }
      else
      {
         dumpAndShowPrintf( "  Total Errors           : -" OSS_NEWLINE ) ;
      }
      dumpAndShowPrintf( "  Output File Num        : %u" OSS_NEWLINE, gOutputFileIndex + 1 ) ;
      dumpAndShowPrintf( "  Total Data File Num    : %u" OSS_NEWLINE, gStat._totalDataFileNum ) ;
      dumpAndShowPrintf( "  Total Index File Num   : %u" OSS_NEWLINE, gStat._totalIndexFileNum ) ;
      dumpAndShowPrintf( "  Total Lob File Num     : %u" OSS_NEWLINE, gStat._totalLobFileNum ) ;
      dumpAndShowPrintf( "  Total Data File Size   : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalDataFileSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total Index File Size  : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalIndexFileSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total Lob File Size    : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalLobFileSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total File Size        : %.3f (GB)" OSS_NEWLINE,
                         totalSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total Free Page Size   : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalFreePageSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total Free Tail Size   : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalFreeTailSize / GB_SIZE ) ;
      dumpAndShowPrintf( "  Total Free Size        : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._totalFreeSize / GB_SIZE ) ;

      dumpAndShowPrintf( "  Data Read IO Detail Info :" OSS_NEWLINE ) ;
      dumpAndShowPrintf( "    Read Bytes           : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._readBytes / GB_SIZE ) ;
      dumpAndShowPrintf( "    IO Read Bytes        : %.3f (GB)" OSS_NEWLINE,
                         (FLOAT64)gStat._ioReadBytes / GB_SIZE ) ;
      dumpAndShowPrintf( "    Read Times           : %llu" OSS_NEWLINE, gStat._readTimes ) ;
      dumpAndShowPrintf( "    IO Read Times        : %llu" OSS_NEWLINE, gStat._ioReadTimes ) ;
      dumpAndShowPrintf( "    Min Read Len         : %lld" OSS_NEWLINE, gStat._minReadLen ) ;
      dumpAndShowPrintf( "    Max Read Len         : %lld" OSS_NEWLINE, gStat._maxReadLen ) ;
      dumpAndShowPrintf( "    Read Speed           : %.2f (MB/S)" OSS_NEWLINE, readSpeed ) ;
      dumpAndShowPrintf( "    Read Cache Ratio     : %.2f%%" OSS_NEWLINE, timesRatio * 100 ) ;

      dumpAndShowPrintf( "  Output Write IO Detail Info :" OSS_NEWLINE ) ;
      dumpAndShowPrintf( "    Write Times          : %llu" OSS_NEWLINE, gStat._writeTimes ) ;
      if ( gStat._writeBytes > GB_SIZE )
      {
         dumpAndShowPrintf( "    Write Bytes          : %.3f (GB)" OSS_NEWLINE,
                            (FLOAT64)gStat._writeBytes / GB_SIZE ) ;
         dumpAndShowPrintf( "    Write Speed          : %.2f (MB/S)" OSS_NEWLINE, writeSpeed ) ;
      }
      else if ( gStat._writeBytes > MB_SIZE )
      {
         dumpAndShowPrintf( "    Write Bytes          : %.3f (MB)" OSS_NEWLINE,
                            (FLOAT64)gStat._writeBytes / MB_SIZE ) ;
         dumpAndShowPrintf( "    Write Speed          : %.2f (MB/S)" OSS_NEWLINE, writeSpeed ) ;
      }
      else
      {
         dumpAndShowPrintf( "    Write Bytes          : %llu" OSS_NEWLINE, gStat._writeBytes ) ;
         dumpAndShowPrintf( "    Write Speed          : %.2f (Byte/S)" OSS_NEWLINE,
                            writeSpeed * MB_SIZE ) ;
      }

      dumpAndShowPrintf( OSS_NEWLINE ) ;
   }

done :
   // free format output buffer
   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
   // free extent holding buffer
   if ( gExtentBuffer )
   {
      SDB_OSS_FREE ( gExtentBuffer ) ;
      gExtentBuffer = NULL ;
   }
   if ( gMMEBuff )
   {
      SDB_OSS_FREE ( gMMEBuff ) ;
      gMMEBuff = NULL ;
   }
   if ( gDictBuffer )
   {
      SDB_OSS_FREE( gDictBuffer ) ;
      gDictBuffer = NULL ;
   }
   if ( gBMEBuff )
   {
      SDB_OSS_FREE( gBMEBuff ) ;
      gBMEBuff = NULL ;
   }
   if ( gSMEBuff )
   {
      SDB_OSS_FREE( gSMEBuff ) ;
      gSMEBuff = NULL ;
   }
   // close output file
   if ( gOutputFile.isOpened() )
   {
      ossClose ( gOutputFile ) ;
   }
   /// release cache
   releaseFileCache( NULL ) ;
   gInspectSMEInfo.fini() ;

   // free cb
   if ( cb )
   {
      SDB_OSS_DEL ( cb ) ;
   }
   return utilRC2ShellRC( rc ) ;
error:
   goto done ;
}

