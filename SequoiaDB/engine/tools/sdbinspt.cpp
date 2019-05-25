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

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <stdarg.h>

using namespace std ;
using namespace engine ;
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
#define OPTION_VERBOSE      "verbose"
#define OPTION_CSNAME       "csname"
#define OPTION_CLNAME       "clname"
#define OPTION_ACTION       "action"
#define OPTION_DUMPDATA     "dumpdata"
#define OPTION_DUMPINDEX    "dumpindex"
#define OPTION_DUMPLOB        "dumplob"
#define OPTION_PAGESTART    "pagestart"
#define OPTION_NUMPAGE      "numpage"
#define OPTION_SHOW_CONTENT "record"
#define OPTION_ONLY_META    "meta"
#define OPTION_JUDGE_BALANCE "balance"
#define OPTION_REPAIRE      "repaire"
#define OPTION_FORCE        "force"

#define OPTION_REPAIRE_DESP \
   "repaire the db info, like --repaire mb:Flag=0,Attr=1\n"\
   "-mb support key:\n"\
   "  IndexPages(u)      LID(u)            Attr(u)\n"\
   "  IndexFreeSpace(u)  DataPages(u)      Flag(u)\n"\
   "  DataFreeSpace(u)   LobPages(u)       Records(u)\n"\
   "  IndexNum(u)        CompressType(u)   Lobs(u)\n"\
   "  CommitFlag(u)      CommitLSN(u64)\n"\
   "  IdxCommitFlag(u)   IdxCommitLSN(u64)\n"\
   "  LobCommitFlag(u)   LobCommitLSN(u64)"

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()

#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()
#define COMMANDS_OPTIONS \
       ( COMMANDS_STRING(OPTION_HELP, ",h"), "help" )\
       ( OPTION_VERSION, "version" )\
       ( COMMANDS_STRING(OPTION_DBPATH, ",d"), boost::program_options::value<string>(), "database path" ) \
       ( COMMANDS_STRING(OPTION_INDEXPATH, ",x"), boost::program_options::value<string>(), "index path" ) \
       ( COMMANDS_STRING(OPTION_LOBPATH, ",g"), boost::program_options::value<string>(), "lob path" ) \
       ( COMMANDS_STRING(OPTION_LOBMPATH, ",m"), boost::program_options::value<string>(), "lob meta path" ) \
       ( COMMANDS_STRING(OPTION_OUTPUT, ",o"), boost::program_options::value<string>(), "output file" ) \
       ( COMMANDS_STRING(OPTION_VERBOSE, ",v"), boost::program_options::value<string>(), "verbose (ture/false)" ) \
       ( COMMANDS_STRING(OPTION_CSNAME, ",c"), boost::program_options::value<string>(), "collection space name" ) \
       ( COMMANDS_STRING(OPTION_CLNAME, ",l"), boost::program_options::value<string>(), "collection name" ) \
       ( COMMANDS_STRING(OPTION_ACTION, ",a"), boost::program_options::value<string>(), "action (inspect/dump/stat/all)" ) \
       ( COMMANDS_STRING(OPTION_DUMPDATA, ",t"), boost::program_options::value<string>(), "dump data (true/false)" ) \
       ( COMMANDS_STRING(OPTION_DUMPINDEX, ",i"), boost::program_options::value<string>(), "dump index (true/false)" ) \
       ( COMMANDS_STRING(OPTION_DUMPLOB, ",b"), boost::program_options::value<string>(), "dump lob (true/false)" ) \
       ( COMMANDS_STRING(OPTION_PAGESTART, ",s"), boost::program_options::value<SINT32>(), "starting page number" ) \
       ( COMMANDS_STRING(OPTION_NUMPAGE, ",n"), boost::program_options::value<SINT32>(), "number of pages" ) \
       ( COMMANDS_STRING(OPTION_SHOW_CONTENT, ",p"), boost::program_options::value<string>(), "display data/index content(true/false)" ) \
       ( OPTION_ONLY_META, boost::program_options::value<string>(), "inspect only meta(Header, SME, MME), true/false" ) \
       ( OPTION_JUDGE_BALANCE, boost::program_options::value<string>(), "open lob buckets balance judgement" ) \
       ( OPTION_FORCE, "force dump all invalid mb, delete list and index list and so on" ) \
       ( COMMANDS_STRING(OPTION_REPAIRE, ",r"), boost::program_options::value<string>(), OPTION_REPAIRE_DESP )

#define ACTION_INSPECT           0x01
#define ACTION_DUMP              0x02
#define ACTION_STAT              0x04
#define ACTION_REPAIRE           0x08

#define ACTION_INSPECT_STRING    "inspect"
#define ACTION_STAT_STRING       "stat"
#define ACTION_DUMP_STRING       "dump"
#define ACTION_ALL_STRING        "all"

#define DMS_DUMPFILE "dmsdump"

#define W_OK 2

#define MAX_FILE_SIZE 500 * 1024 * 1024
#define BUFFER_INC_SIZE 67108864
#define BUFFER_INIT_SIZE 4194304

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


#define RETRY_COUNT 5

namespace 
{
    enum SDB_INSPT_TYPE
    {
       SDB_INSPT_DATA,
       SDB_INSPT_INDEX,
       SDB_INSPT_LOB
    };
    
    struct LobFragments
    {
        BYTE oid[DMS_LOB_OID_LEN];
        map<UINT32,UINT32> sequences;  //pair<sequenceId, PageId>
    };

    typedef std::map<UINT32, std::vector<LobFragments> > ClMap;
    ClMap gCl2PageMap;


    CHAR    gDatabasePath [ OSS_MAX_PATHSIZE + 1 ]       = {0} ;
    CHAR    gIndexPath[ OSS_MAX_PATHSIZE + 1 ]           = {0} ;
    CHAR    gLobPath[ OSS_MAX_PATHSIZE + 1 ]           = {0} ;
    CHAR    gLobmPath[ OSS_MAX_PATHSIZE + 1 ]           = {0} ;
    CHAR    gOutputFile [ OSS_MAX_PATHSIZE + 1 ]         = {0} ;
    BOOLEAN gVerbose                                     = TRUE ;
    UINT32  gDumpType                                    = DMS_SU_DMP_OPT_FORMATTED;
    CHAR    gCSName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
    CHAR    gCLName [ DMS_COLLECTION_NAME_SZ + 1 ]       = {0} ;
    CHAR    gAction                                      = 0 ;
    BOOLEAN gDumpData                                    = FALSE ;
    BOOLEAN gDumpIndex                                   = FALSE ;
    SINT32  gStartingPage                                = -1 ;
    SINT32  gNumPages                                    = 1 ;
    SINT32  gCurFileIndex                                = 0 ;
    std::string gRepairStr ;
    OSSFILE gFile ;

    OSSFILE gLobdFile;
    string gLobdFileName;
    BOOLEAN gDumpLob = FALSE;
    UINT32 gLobdPageSize = 0;
    UINT32 gLobmPageSize = 0;
    UINT32 gSequence = 0;
    BOOLEAN gExistLobs = FALSE;

    BOOLEAN gBalance = FALSE;
    UINT32 *gBucketDep = NULL;


    CHAR *  gBuffer                                      = NULL ;
    UINT32  gBufferSize                                  = 0 ;
    CHAR *  gExtentBuffer = NULL ;
    UINT32  gExtentBufferSize = 0 ;
    CHAR *  gDictBuffer = NULL ;

    pmdEDUCB *cb             = NULL ;

    UINT64  gSecretValue                                 = 0 ;
    UINT32  gPageSize                                    = 0 ;
    UINT32  gDataOffset                                  = 0 ;
    UINT32  gPageNum                                     = 0 ;
    CHAR   *gMMEBuff                                     = NULL ;
    BOOLEAN gInitMME                                     = FALSE ;
    BOOLEAN gShowRecordContent                           = FALSE ;
    BOOLEAN gOnlyMeta                                    = FALSE ;
    BOOLEAN gForce                                       = FALSE ;
    BOOLEAN gReachEnd                                    = FALSE ;
    BOOLEAN gHitCS                                       = FALSE ;
    SDB_INSPT_TYPE gCurInsptType                         = SDB_INSPT_DATA ;
    dmsMBStatInfo gMBStat ;
    dmsMB         gRepaireMB ;
    UINT32        gRepaireMask                           = 0 ;
}


INT32 prepareCompressor( OSSFILE &file, UINT32 pageSize, dmsMB *mb, UINT16 id,
                         CHAR *pExpBuffer, dmsCompressorEntry &compressorEntry,
                         SINT32 &err ) ;

INT32 switchFile( OSSFILE& file, const INT32 size )
{
   INT32 rc = SDB_OK ;
   CHAR newFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   INT64 fileSize = 0 ;
   INT32 retryCount = 0;

   rc = ossGetFileSize( &file, &fileSize ) ;
   if( rc )
   {
      printf( "Error: can not get fileSize. rc: %d\n", rc ) ;
      goto error ;
   }

   if( MAX_FILE_SIZE <= fileSize + size )
   {
      ossClose( file ) ;
   retry:

      ++gCurFileIndex ;
      ossSnprintf( newFile, OSS_MAX_PATHSIZE, "%s.%d",
                   gOutputFile, gCurFileIndex ) ;
      rc = ossOpen ( newFile, OSS_REPLACE | OSS_WRITEONLY,
                     OSS_RU|OSS_WU|OSS_RG, file ) ;
      if ( rc )
      {
         printf ( "Error: Failed to open output file: %s, rc = %d"
                  OSS_NEWLINE, newFile, rc ) ;
         ++retryCount ;
         if( RETRY_COUNT < retryCount )
         {
            printf( "retry times more than %d, return\n", RETRY_COUNT ) ;
            goto error ;
         }
         printf( "retry again. times : %d\n", retryCount ) ;
         ossMemset ( newFile, 0, OSS_MAX_PATHSIZE + 1 ) ;
         goto retry;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

void dumpPrintf ( const CHAR *format, ... ) ;
#define dumpAndShowPrintf(x,...)                                               \
   do {                                                                        \
      ossPrintf ( x, ##__VA_ARGS__ ) ;                                         \
      if ( ossStrlen ( gOutputFile ) )                                         \
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
}

INT32 parseRepaireString( const std::string &str )
{
   const CHAR *pin = str.c_str() ;
   CHAR *pos = (CHAR*)ossStrchr( pin, ':' ) ;
   if ( NULL == pos )
   {
      ossPrintf( "repaire format must be: mb:xx=y,dd=k"OSS_NEWLINE ) ;
      return SDB_INVALIDARG ;
   }
   *pos = 0 ;
   if ( 0 != ossStrcasecmp( pin, "mb" ) )
   {
      *pos = ':' ;
      ossPrintf( "repaire only support for type mb"OSS_NEWLINE ) ;
      return SDB_INVALIDARG ;
   }
   *pos = ':' ;

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

      if ( !ossIsInteger( aItem._service ) )
      {
         ossPrintf( "Field[%s]'s value is not number[%s]"OSS_NEWLINE,
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
      else
      {
         ossPrintf( "Unknow mb key: %s"OSS_NEWLINE, aItem._host ) ;
         return SDB_INVALIDARG ;
      }
   }

   return SDB_OK ;
}

INT32 resolveArgument ( po::options_description &desc, INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   CHAR actionString[BUFFERSIZE] = {0} ;
   CHAR outputFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   po::variables_map vm ;
   try
   {
      po::store ( po::parse_command_line ( argc, argv, desc ), vm ) ;
      po::notify ( vm ) ;
   }
   catch ( po::unknown_option &e )
   {
      PD_LOG ( PDWARNING, ( ( std::string ) "Unknown argument: " +
                e.get_option_name ()).c_str () ) ;
      std::cerr <<  "Unknown argument: " << e.get_option_name () << std::endl ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      PD_LOG ( PDWARNING, ( ( std::string ) "Invalid argument: " +
               e.get_option_name () ).c_str () ) ;
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
   if ( vm.count ( OPTION_DBPATH ) )
   {
      const CHAR *dbpath = vm[OPTION_DBPATH].as<string>().c_str() ;
      if ( ossStrlen ( dbpath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "Error: db path is too long: %s", dbpath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gDatabasePath, dbpath, sizeof(gDatabasePath) ) ;
   }
   else
   {
      ossStrncpy ( gDatabasePath, ".", sizeof(gDatabasePath) ) ;
   }

   if ( vm.count( OPTION_INDEXPATH ) )
   {
      const CHAR *indexPath = vm[OPTION_INDEXPATH].as<string>().c_str() ;
      if ( ossStrlen ( indexPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "Error: index path is too long: %s", indexPath ) ;
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
      const CHAR *lobPath = vm[OPTION_LOBPATH].as<string>().c_str() ;
      if ( ossStrlen ( lobPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "Error: lob path is too long: %s", lobPath ) ;
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
      const CHAR *lobmPath = vm[OPTION_LOBMPATH].as<string>().c_str() ;
      if ( ossStrlen ( lobmPath ) > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "Error: lob meta path is too long: %s", lobmPath ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gLobmPath, lobmPath, OSS_MAX_PATHSIZE ) ;
   }
   else
   {
      ossStrcpy( gLobmPath, gDatabasePath ) ;
   }

   if ( vm.count ( OPTION_OUTPUT ) )
   {
      const CHAR *output = vm[OPTION_OUTPUT].as<string>().c_str() ;
      INT32 rc = SDB_OK ;
      if ( ossStrlen ( output ) + 5 > OSS_MAX_PATHSIZE )
      {
         ossPrintf ( "Error: output is too long: %s", output ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gOutputFile, output, sizeof(gOutputFile) ) ;
      SDB_OSS_FILETYPE fileType = SDB_OSS_UNK ;
      rc = ossAccess( output, W_OK ) ;
      if ( SDB_OK == rc )
      {
         INT32 retValue = ossGetPathType( gOutputFile, &fileType ) ;
         if( SDB_OSS_DIR == fileType && !retValue )
         {
            INT32 len = ossStrlen( gOutputFile ) ;
            if ( OSS_FILE_SEP_CHAR != gOutputFile[ len - 1 ] )
            {
               gOutputFile[ len - 1 ] = OSS_FILE_SEP_CHAR ;
               gOutputFile[ len ] = '\0' ;
            }

            ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s"DMS_DUMPFILE".%d",
                         gOutputFile, gCurFileIndex ) ;
         }
         else
         {
            ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s.%d",
                         gOutputFile, gCurFileIndex ) ;
         }
      }
      else if ( SDB_FNE == rc )
      {
         ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s.%d",
                      gOutputFile, gCurFileIndex ) ;
      }
      else
      {
         goto error ;
      }

      rc = ossOpen ( outputFile, OSS_REPLACE | OSS_WRITEONLY,
                     OSS_RU|OSS_WU|OSS_RG, gFile ) ;
      if ( rc )
      {
         ossPrintf ( "Error: Failed to open output file: %s, rc = %d"
                     OSS_NEWLINE,
                     outputFile, rc ) ;
         ossMemset ( gOutputFile, 0, sizeof(gOutputFile) ) ;
      }
   }

   if ( vm.count ( OPTION_VERBOSE ) )
   {
      ossStrToBoolean ( vm[OPTION_VERBOSE].as<string>().c_str(),
                        &gVerbose ) ;
      if ( !gVerbose )
         gDumpType = 0 ;
   }

   if ( vm.count ( OPTION_DUMPDATA ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPDATA].as<string>().c_str(),
                        &gDumpData ) ;
   }

   if ( vm.count ( OPTION_DUMPINDEX ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPINDEX].as<string>().c_str(),
                        &gDumpIndex ) ;
   }

   if ( vm.count ( OPTION_DUMPLOB ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPLOB].as<string>().c_str(),
                        &gDumpLob ) ;
   }

   if ( vm.count ( OPTION_CSNAME ) )
   {
      const CHAR *csname = vm[OPTION_CSNAME].as<string>().c_str() ;
      if ( ossStrlen ( csname ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         ossPrintf ( "Error: collection space name is too long: %s", csname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gCSName, csname, sizeof(gCSName) ) ;
   }

   if ( vm.count ( OPTION_CLNAME ) )
   {
      const CHAR *clname = vm[OPTION_CLNAME].as<string>().c_str() ;
      if ( ossStrlen ( clname ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         ossPrintf ( "Error: collection name is too long: %s", clname ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy ( gCLName, clname, sizeof(gCLName) ) ;
   }

   if ( vm.count ( OPTION_PAGESTART ) )
   {
      gStartingPage = vm[OPTION_PAGESTART].as<SINT32>() ;
   }

   if ( vm.count ( OPTION_NUMPAGE ) )
   {
      gNumPages = vm[OPTION_NUMPAGE].as<SINT32>() ;
   }

   ossStrncpy ( actionString, ACTION_DUMP_STRING,
                sizeof(actionString) ) ;
   if ( vm.count ( OPTION_ACTION ) )
   {
      const CHAR *action = vm[OPTION_ACTION].as<string>().c_str() ;
      if ( ossStrncasecmp ( action, ACTION_INSPECT_STRING,
           ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_INSPECT_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_INSPECT ;
      }
      else if ( ossStrncasecmp ( action, ACTION_DUMP_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_DUMP_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_DUMP ;
      }
      else if ( ossStrncasecmp( action, ACTION_STAT_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy( actionString, ACTION_STAT_STRING,
                     sizeof(actionString) ) ;
         gAction = ACTION_STAT ;
      }
      else if ( ossStrncasecmp ( action, ACTION_ALL_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_ALL_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_INSPECT | ACTION_DUMP | ACTION_STAT ;
      }
      else
      {
         dumpAndShowPrintf ( "Invalid Action Option: %s"OSS_NEWLINE,
                             action ) ;
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
   }
   else if ( !vm.count( OPTION_REPAIRE ) )
   {
      dumpAndShowPrintf ( "Action or repaire must be specified"OSS_NEWLINE ) ;
      displayArg ( desc ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }

   if( vm.count( OPTION_SHOW_CONTENT ) )
   {
      ossStrToBoolean( vm[OPTION_SHOW_CONTENT].as<string>().c_str(),
                       &gShowRecordContent ) ;
   }
   if ( vm.count( OPTION_ONLY_META ) )
   {
      ossStrToBoolean( vm[OPTION_ONLY_META].as<string>().c_str(),
                       &gOnlyMeta ) ;
   }

   if ( vm.count( OPTION_JUDGE_BALANCE) )
   {
      ossStrToBoolean( vm[OPTION_JUDGE_BALANCE].as<string>().c_str(),
                       &gBalance) ;
   }
   
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
         ossPrintf( "Repaire can't use with other action"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( 0 == ossStrlen( gCLName ) || 0 == ossStrlen( gCSName ) )
      {
         ossPrintf( "Repaire must specify the collection space and "
                    "collection"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      gAction = ACTION_REPAIRE ;
   }

   dumpAndShowPrintf ( "Run Options   :"OSS_NEWLINE ) ;
   dumpAndShowPrintf ( "Database Path : %s"OSS_NEWLINE,
                       gDatabasePath ) ;
   dumpAndShowPrintf ( "Index path    : %s"OSS_NEWLINE,
                       gIndexPath ) ;
   dumpAndShowPrintf ( "Lobm path     : %s"OSS_NEWLINE,
                       gLobmPath) ;
   dumpAndShowPrintf ( "Lob path      : %s"OSS_NEWLINE,
                       gLobPath ) ;
   dumpAndShowPrintf ( "Output File   : %s"OSS_NEWLINE,
                       ossStrlen(gOutputFile)?gOutputFile:"{stdout}" ) ;
   dumpAndShowPrintf ( "Verbose       : %s"OSS_NEWLINE,
                       gVerbose?"True":"False" ) ;
   dumpAndShowPrintf ( "CS Name       : %s"OSS_NEWLINE,
                       ossStrlen(gCSName)?gCSName:"{all}" ) ;
   dumpAndShowPrintf ( "CL Name       : %s"OSS_NEWLINE,
                       ossStrlen(gCLName)?gCLName:"{all}" ) ;
   dumpAndShowPrintf ( "Action        : %s"OSS_NEWLINE,
                       actionString ) ;
   dumpAndShowPrintf ( "Repaire       : %s"OSS_NEWLINE,
                       gRepairStr.c_str() ) ;
   dumpAndShowPrintf ( "Dump Options  :"OSS_NEWLINE ) ;
   dumpAndShowPrintf ( "   Dump Data  : %s"OSS_NEWLINE,
                       gDumpData?"True":"False" ) ;
   dumpAndShowPrintf ( "   Dump Index : %s"OSS_NEWLINE,
                       gDumpIndex?"True":"False" ) ;
   dumpAndShowPrintf ( "   Dump Lob   : %s"OSS_NEWLINE,
                       gDumpLob?"True":"False" ) ;
   dumpAndShowPrintf ( "   Start Page : %d"OSS_NEWLINE,
                       gStartingPage ) ;
   dumpAndShowPrintf ( "   Num Pages  : %d"OSS_NEWLINE,
                       gNumPages ) ;
   dumpAndShowPrintf ( "   Show record: %s"OSS_NEWLINE,
                       gShowRecordContent ? "True":"False") ;
   dumpAndShowPrintf ( "   Only Meta  : %s"OSS_NEWLINE,
                       gOnlyMeta ? "True":"False" ) ;
   dumpAndShowPrintf ( "   Balance    : %s"OSS_NEWLINE,
                       gBalance? "True":"False" ) ;
   dumpAndShowPrintf ( "   force      : %s"OSS_NEWLINE,
                       gForce ? "True":"False" ) ;
   dumpAndShowPrintf ( OSS_NEWLINE ) ;
done :
   return rc ;
error :
   goto done ;
}

void flushOutput ( const CHAR *pBuffer, INT32 size )
{
   INT32 rc = SDB_OK ;
   SINT64 writeSize ;
   SINT64 writtenSize = 0 ;

   if ( 0 == size )
   {
      goto done ;
   }
   else if ( ossStrlen ( gOutputFile ) == 0 )
   {
      goto error ;
   }

   rc = switchFile( gFile, size ) ;
   if( rc )
   {
      goto error ;
   }

   do
   {
      rc = ossWrite ( &gFile, &pBuffer[writtenSize], size-writtenSize,
                      &writeSize ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         break ;
      }
      rc = SDB_OK ;
      writtenSize += writeSize ;
   } while ( writtenSize < size ) ;
   if ( rc )
   {
      ossPrintf ( "Error: Failed to write into file, rc = %d"OSS_NEWLINE,
                  rc ) ;
      goto error ;
   }

done :
   return ;
error :
   ossPrintf ( "%s", pBuffer ) ;
   goto done ;
}

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
         dumpPrintf ( "Error: Cannot allocate more than 2GB"OSS_NEWLINE ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      gBufferSize += gBufferSize > BUFFER_INC_SIZE?
                     BUFFER_INC_SIZE : gBufferSize ;
   }

   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
   gBuffer = (CHAR*)SDB_OSS_MALLOC ( gBufferSize ) ;
   if ( !gBuffer )
   {
      dumpPrintf ( "Error: Failed to allocate memory for %d bytes"OSS_NEWLINE,
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
         dumpPrintf ( "Error: Failed to allocate extent buffer for %d bytes"
                      OSS_NEWLINE, size ) ;
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

void clearBuffer ()
{
   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
   gBufferSize = 0 ;
}

INT32 inspectLobmHeader ( OSSFILE &file, INT64 &fileSize, SINT32 &err )
{
    INT32 rc       = SDB_OK ;
    UINT32 len     = 0 ;
    CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
    SINT64 lenRead = 0 ;
    SINT32 localErr = 0;

    rc = ossSeekAndRead(&file, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
    if ( rc || lenRead != DMS_HEADER_SZ )
    {
      dumpPrintf("  Error: Failed to read header, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      
      goto error ;
    }
retry :
    localErr = 0;
    len = dmsInspect::inspectLobmHeader(headerBuffer, DMS_HEADER_SZ,
                                     gBuffer, gBufferSize, gSequence,
                                     gPageNum, gLobmPageSize, gSecretValue,
                                     fileSize, localErr) ;
    if ( len >= gBufferSize - 1 )
    {
        if ( reallocBuffer() )
        {
            clearBuffer () ;
            goto error ;
        }
        goto retry ;
    }
    flushOutput ( gBuffer, len ) ;
    err += localErr;
   
done :
    return rc;
error :
    goto done ;
}


INT32 inspectLobdHeader (UINT64 fileSize, SINT32 &totalErr)
{
   INT32 rc       = SDB_OK ;
   UINT32 len     = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT64 lenRead = 0 ;
   SINT32 localErr;

   rc = ossSeekAndRead ( &gLobdFile, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
retry :
    localErr = 0;
    len = dmsInspect::inspectLobdHeader ( headerBuffer, DMS_HEADER_SZ,
                                     gBuffer, gBufferSize,
                                     gSequence, gSecretValue,
                                     fileSize, localErr) ;
    if ( len >= gBufferSize - 1 )
    {
      if ( reallocBuffer () )
      {
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


void inspectHeader ( OSSFILE &file, UINT32 &pageSize, SINT32 &err )
{
   INT32 rc       = SDB_OK ;
   INT32 localErr = 0 ;
   UINT32 len     = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT64 lenRead = 0 ;
   UINT64 secretValue = 0 ;

   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      ++err ;
      goto error ;
   }
retry :
   localErr = 0 ;
   len = dmsInspect::inspectHeader ( headerBuffer, DMS_HEADER_SZ,
                                     gBuffer, gBufferSize,
                                     pageSize, gPageNum,
                                     secretValue, localErr ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( reallocBuffer () )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   err += localErr ;
   flushOutput ( gBuffer, len ) ;

   if ( secretValue != gSecretValue )
   {
      dumpPrintf ( "Error: Secret value[%llu] is not expected[%llu]"OSS_NEWLINE,
                    secretValue, gSecretValue ) ;
      ++err ;
   }
   if ( (UINT32)pageSize != gPageSize )
   {
      dumpPrintf ( "Error: Page size[%d] is not expected[%d]"OSS_NEWLINE,
                    pageSize, gPageSize ) ;
      ++err ;
   }

done :
   return ;
error :
   goto done ;
}

void dumpHeader ( OSSFILE &file, UINT32 &pageSize )
{
   INT32 rc                            = SDB_OK ;
   UINT32 len                          = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT64 lenRead                      = 0 ;
   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
retry :
   len = dmsDump::dumpHeader ( headerBuffer, DMS_HEADER_SZ,
                               gBuffer, gBufferSize, NULL,
                               DMS_SU_DMP_OPT_HEX |
                               DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                               DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                               gDumpType, pageSize, gPageNum ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( reallocBuffer () )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;

done :
   return ;
error :
   goto done ;
}

void inspectSME ( OSSFILE &file, const CHAR *pExpBuf, SINT32 &hwm, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   UINT32 len      = 0 ;
   CHAR *smeBuffer = NULL ;
   SINT64 lenRead  = 0 ;
   SINT32 localErr = 0 ;
   smeBuffer = (CHAR*)SDB_OSS_MALLOC ( DMS_SME_SZ ) ;
   if ( !smeBuffer )
   {
      dumpPrintf ( "Error: Failed to allocate %d bytes for SME buffer"
                   OSS_NEWLINE, (INT32)DMS_SME_SZ ) ;

      goto error ;
   }
   rc = ossSeekAndRead ( &file, DMS_SME_OFFSET, smeBuffer,
                         DMS_SME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_SME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      
      goto error ;
   }
retry :
   localErr = 0 ;
   len = dmsInspect::inspectSME ( smeBuffer, DMS_SME_SZ,
                                  gBuffer, gBufferSize,
                                  pExpBuf, gPageNum,
                                  hwm, localErr ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( reallocBuffer () )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;
   err += localErr ;
done :
   if ( smeBuffer )
   {
      SDB_OSS_FREE ( smeBuffer ) ;
   }
   return ;
error :
   goto done ;
}

void dumpSME ( OSSFILE &file, CHAR *pSmeBuffer)
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   SINT64 lenRead = 0 ;
   

   rc = ossSeekAndRead ( &file, DMS_SME_OFFSET, pSmeBuffer,
                         DMS_SME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_SME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, rc = %d"
                   OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
retry :
   len = dmsDump::dumpSME ( pSmeBuffer, DMS_SME_SZ,
                            gBuffer, gBufferSize, gPageNum ) ;
   if ( len >= gBufferSize - 1 )
   {
      if ( reallocBuffer () )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }
   flushOutput ( gBuffer, len ) ;

done :

   return ;
error :
   goto done ;
}

INT32 getExtentHead ( OSSFILE &file, dmsExtentID extentID, UINT32 pageSize,
                      dmsExtent &extentHead )
{
   INT32 rc = SDB_OK ;
   SINT64 lenRead = 0 ;
   rc = ossSeekAndRead ( &file, gDataOffset + (SINT64)pageSize * extentID,
                         (CHAR*)&extentHead, DMS_EXTENT_METADATA_SZ,
                         &lenRead ) ;
   if ( rc || lenRead != DMS_EXTENT_METADATA_SZ )
   {
      dumpPrintf ( "Error: Failed to read extent head, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      if ( !rc )
         rc = SDB_IO ;
   }
   return rc ;
}

INT32 getExtent ( OSSFILE &file, dmsExtentID extentID, UINT32 pageSize,
                  SINT32 extentSize, BOOLEAN dictExtent = FALSE )
{
   INT32 rc = SDB_OK ;
   SINT64 lenRead ;
   CHAR *buffer = NULL ;

   if ( dictExtent )
   {
      buffer = gDictBuffer ;
   }
   else
   {
      if ( gExtentBufferSize < (UINT32)(extentSize * pageSize ) )
      {
         rc = getExtentBuffer ( extentSize*pageSize ) ;
         if ( rc )
         {
            dumpPrintf ( "Error: Failed to allocate extent buffer, rc = %d"
                         OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }
      buffer = gExtentBuffer ;
   }

   rc = ossSeekAndRead ( &file, gDataOffset + (SINT64)pageSize * extentID,
                         buffer, extentSize * pageSize,
                         &lenRead ) ;
   if ( rc || lenRead != extentSize * pageSize )
   {
      dumpPrintf ( "Error: Failed to read extent , read %lld bytes, "
                   "expect %d bytes, rc = %d"OSS_NEWLINE, lenRead,
                   extentSize * pageSize, rc ) ;
      gReachEnd = TRUE ;

      if ( !rc )
         rc = SDB_IO ;
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
   INSPECT_EXTENT_TYPE_UNKNOWN
} ;
BOOLEAN extentSanityCheck ( dmsExtent &extentHead,
                            INSPECT_EXTENT_TYPE &type, // in-out
                            UINT32 pageSize,
                            UINT16 expID )
{
   BOOLEAN result = TRUE ;

retry :
   if ( INSPECT_EXTENT_TYPE_DATA == type )
   {
      if ( extentHead._eyeCatcher[0] != DMS_EXTENT_EYECATCHER0 ||
           extentHead._eyeCatcher[1] != DMS_EXTENT_EYECATCHER1 )
      {
         dumpPrintf ( "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                      extentHead._eyeCatcher[0],
                      extentHead._eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( extentHead._blockSize <= 0 ||
           extentHead._blockSize * pageSize > DMS_SEGMENT_SZ )
      {
         dumpPrintf ( "Error: Invalid block size: %d, pageSize: %d"
                      OSS_NEWLINE, extentHead._blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( extentHead._mbID != expID )
      {
         dumpPrintf ( "Error: Unexpected id: %d, expected %d"OSS_NEWLINE,
                      extentHead._mbID, expID ) ;
         result = FALSE ;
      }
      if ( extentHead._version > DMS_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "Error: Invalid version: %d, current %d"OSS_NEWLINE,
                      extentHead._version, DMS_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
      if ( ( extentHead._firstRecordOffset != DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset == DMS_INVALID_OFFSET ) ||
           ( extentHead._firstRecordOffset == DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset != DMS_INVALID_OFFSET ) )
      {
         dumpPrintf ( "Error: Bad first/last offset: %d:%d"OSS_NEWLINE,
                      extentHead._firstRecordOffset,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      if ( extentHead._firstRecordOffset >=
            extentHead._blockSize * (SINT32)pageSize )
      {
         dumpPrintf ( "Error: Bad first record offset: %d"OSS_NEWLINE,
                      extentHead._firstRecordOffset ) ;
         result = FALSE ;
      }
      if ( extentHead._lastRecordOffset >=
           extentHead._blockSize * (SINT32)pageSize )
      {
         dumpPrintf ( "Error: Bad last record offset: %d"OSS_NEWLINE,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      if ( (UINT32)extentHead._freeSpace >
           extentHead._blockSize * (SINT32)pageSize - DMS_EXTENT_METADATA_SZ )
      {
         dumpPrintf ( "Error: Invalid free space: %d, extentSize: %d"
                      OSS_NEWLINE, extentHead._freeSpace,
                      extentHead._blockSize * pageSize ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_INDEX == type )
   {
   }
   else if ( INSPECT_EXTENT_TYPE_INDEX_CB == type )
   {
   }
   else if ( INSPECT_EXTENT_TYPE_MBEX == type )
   {
      dmsMetaExtent *metaExt = ( dmsMetaExtent* )&extentHead ;
      if ( metaExt->_eyeCatcher[0] != DMS_META_EXTENT_EYECATCHER0 ||
           metaExt->_eyeCatcher[1] != DMS_META_EXTENT_EYECATCHER1 )
      {
         dumpPrintf ( "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                      metaExt->_eyeCatcher[0],
                      metaExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( metaExt->_blockSize <= 0 ||
           metaExt->_blockSize * pageSize > DMS_SEGMENT_SZ )
      {
         dumpPrintf ( "Error: Invalid block size: %d, pageSize: %d"
                      OSS_NEWLINE, metaExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( metaExt->_mbID != expID )
      {
         dumpPrintf ( "Error: Unexpected id: %d, expected %d"OSS_NEWLINE,
                      metaExt->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( metaExt->_version > DMS_META_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "Error: Invalid version: %d, current %d"OSS_NEWLINE,
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
         dumpPrintf( "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                     dictExt->_eyeCatcher[0],
                     dictExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( dictExt->_blockSize <= 0 ||
           dictExt->_blockSize * pageSize > DMS_SEGMENT_SZ )
      {
         dumpPrintf( "Error: Invalid block size: %d, pageSize: %d"OSS_NEWLINE,
                     dictExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( dictExt->_mbID != expID )
      {
         dumpPrintf( "Error: Unexpected id: %d, expected %d"OSS_NEWLINE,
                     dictExt->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( dictExt->_version > DMS_DICT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "Error: Invalid version: %d, current %d"OSS_NEWLINE,
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
         dumpPrintf( "Error: Invalid eye catcher: %c%c"OSS_NEWLINE,
                     extent->_eyeCatcher[0],
                     extent->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( extent->_blockSize <= 0 ||
           extent->_blockSize * pageSize > DMS_SEGMENT_SZ )
      {
         dumpPrintf( "Error: Invalid block size: %d, pageSize: %d"OSS_NEWLINE,
                     extent->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( extent->_mbID != expID )
      {
         dumpPrintf( "Error: Unexpected id: %d, expected %d"OSS_NEWLINE,
                     extent->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( extent->_version > DMS_OPT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "Error: Invalid version: %d, current %d"OSS_NEWLINE,
                     extent->_version, DMS_OPT_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_UNKNOWN == type )
   {
      if ( extentHead._eyeCatcher[0] == DMS_EXTENT_EYECATCHER0 &&
           extentHead._eyeCatcher[1] == DMS_EXTENT_EYECATCHER1 )
      {
         type = INSPECT_EXTENT_TYPE_DATA ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == IXM_EXTENT_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == IXM_EXTENT_EYECATCHER1 )
      {
         type = INSPECT_EXTENT_TYPE_INDEX ;
         goto retry ;
      }
      else if ( extentHead._eyeCatcher[0] == IXM_EXTENT_CB_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == IXM_EXTENT_CB_EYECATCHER1 )
      {
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
         dumpPrintf ( "Error: Unknown eye catcher: %c%c"OSS_NEWLINE,
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
      dmsMetadataManagementExtent *pMME =
         (dmsMetadataManagementExtent*)gMMEBuff ;
      mb = &(pMME->_mbList[collectionID]) ;
      rc = SDB_OK ;
   }

   return rc ;
}

INT32 loadExtent ( OSSFILE &file, INSPECT_EXTENT_TYPE &type,
                   UINT32 pageSize, dmsExtentID extentID,
                   UINT16 collectionID )
{
   INT32 rc = SDB_OK ;
   dmsExtent extentHead ;
   rc = getExtentHead ( file, extentID, pageSize, extentHead ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to get extent head, rc = %d"OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }
   if ( !extentSanityCheck ( extentHead, type, pageSize,
                             collectionID ) )
   {
      dumpPrintf ( "Error: Failed head sanity check, dump head:"
                   OSS_NEWLINE ) ;
      INT32 len = ossHexDumpBuffer ( (CHAR*)&extentHead,
                                     DMS_EXTENT_METADATA_SZ,
                                     gBuffer, gBufferSize, NULL,
                                     OSS_HEXDUMP_PREFIX_AS_ADDR ) ;
      flushOutput ( gBuffer, len ) ;
      rc = SDB_DMS_CORRUPTED_EXTENT ;
      goto error ;
   }

   rc = getExtent ( file, extentID, pageSize,
                    ( INSPECT_EXTENT_TYPE_DATA == type ||
                      INSPECT_EXTENT_TYPE_MBEX == type ||
                      INSPECT_EXTENT_TYPE_DICT == type ) ?
                      extentHead._blockSize : 1,
                      INSPECT_EXTENT_TYPE_DICT == type) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to get extent %d, rc = %d"
                   OSS_NEWLINE, extentID, rc ) ;
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

   dumpPrintf ( "Inspect Overflow-Records for Collection [%d]'s extent [%d]"
                OSS_NEWLINE, collectionID, ovfFromExtent ) ;

   for ( it = overRIDList.begin() ; it != overRIDList.end() ; ++it )
   {
      dmsRecordID rid = *it ;
      dmsOffset offset = 0 ;
      if ( rid._extent > (SINT32)gPageNum )
      {
         dumpPrintf ( "Error: overflowed rid extent is out of range: "
                      "0x08lx (%d) 0x08lx (%d)"OSS_NEWLINE,
                      rid._extent, rid._extent,
                      rid._offset, rid._offset ) ;
         ++err ;
         continue ;
      }

      if ( currentExtentID != rid._extent )
      {
         rc = loadExtent ( file, extentType, pageSize, rid._extent,
                           collectionID ) ;
         if ( rc )
         {
            dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                         rid._extent, rc ) ;
            ++err ;
            goto error ;
         }
         currentExtentID = rid._extent ;
      }

retry :
      offset = rid._offset ;
      localErr = 0 ;
      len = dmsInspect::inspectDataRecord ( cb, gExtentBuffer + offset,
              ((dmsExtent*)gExtentBuffer)->_blockSize * pageSize - offset,
              gBuffer, gBufferSize, count, offset, NULL, localErr,
              compressorEntry, isCompressed ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
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
      dumpPrintf ( " Inspect Overflow-Records for Collection [%d]' extent [%d] "
                   "done without Error"OSS_NEWLINE, collectionID,
                   ovfFromExtent ) ;
   }
   else
   {
      dumpPrintf ( " Inspect Overflow-Records for Collection [%d]' extent [%d] "
                   "done with error: %d"OSS_NEWLINE, collectionID,
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
   dumpPrintf ( " Dump Overflow-Records for Collection [%d]'s extent [%d]"
                OSS_NEWLINE, collectionID, ovfFromExtID ) ;

   for ( it = overRIDList.begin() ; it != overRIDList.end() ; ++it )
   {
      dmsRecordID rid = *it ;
      dmsOffset offset = 0 ;
      if ( currentExtentID != rid._extent )
      {
         rc = loadExtent ( file, extentType, pageSize, rid._extent,
                           collectionID ) ;
         if ( rc )
         {
            dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                         rid._extent, rc ) ;
            goto error ;
         }
         currentExtentID = rid._extent ;
      }

retry :
      offset = rid._offset ;
      dumpPrintf ( "    OvfRecord 0x%08x : 0x%08x:"OSS_NEWLINE,
                   rid._extent, rid._offset ) ;
      len = dmsDump::dumpDataRecord ( cb, gExtentBuffer + offset,
                 ((dmsExtent*)gExtentBuffer)->_blockSize * pageSize - offset,
                 gBuffer, gBufferSize, offset, compressorEntry, NULL ) ;

      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
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
                       std::map<UINT16, dmsExtentID> &indexRoots,
                       SINT32 &err )
{
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   INT32 localErr = 0 ;
   INT32 oldErr   = err ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX_CB ;

   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "Error: numIdx is out of range, max: %d"OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      ++err ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }
   for ( UINT16 i = 0 ; i < mb->_numIndexes ; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      if ( indexCBExtentID == DMS_INVALID_EXTENT ||
           (UINT32)indexCBExtentID >= gPageNum )
      {
         dumpPrintf ( "Error: Index CB Extent ID is not valid: 0x%08lx (%d)"
                      OSS_NEWLINE, indexCBExtentID, indexCBExtentID ) ;
         ++err ;
         continue ;
      }
      if ( pExpBuffer )
      {
         dmsSpaceManagementExtent *pSME=( dmsSpaceManagementExtent*)pExpBuffer;
         if ( pSME->getBitMask( indexCBExtentID ) != DMS_SME_FREE )
         {
            dumpPrintf ( "Error: SME extent 0x%08lx (%d) is not free"
                         OSS_NEWLINE, indexCBExtentID, indexCBExtentID ) ;
            ++err ;
         }
         pSME->setBitMask( indexCBExtentID ) ;
      }
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID,
                        collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent, rc = %d"OSS_NEWLINE,
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
      }
      catch( std::exception &e )
      {
      }
      gMBStat._totalIndexPages += 1 ;

retry :
      localErr = 0 ;
      len = dmsInspect::inspectIndexCBExtent ( gExtentBuffer, pageSize,
                                               gBuffer, gBufferSize,
                                               collectionID,
                                               indexRoot,
                                               localErr ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      err += localErr ;
      dumpPrintf ( OSS_NEWLINE ) ;
      indexRoots[i] = indexRoot ;
   }

done :
   if ( oldErr != err )
   {
      dumpPrintf ( " Inspect Index Def for Collection [%u] Done with (%d) Error"
                   OSS_NEWLINE, collectionID, err-oldErr ) ;
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
   dumpPrintf ( " Dump Index Def for Collection [%u]"OSS_NEWLINE,
                collectionID ) ;
   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "Error: numIdx is out of range, max: %d"OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }
   for ( UINT16 i = 0 ; i < mb->_numIndexes; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      dumpPrintf ( "    Index [ %u ] : 0x%08lx"OSS_NEWLINE,
                   i, indexCBExtentID ) ;
      if ( indexCBExtentID == DMS_INVALID_EXTENT ||
           (UINT32)indexCBExtentID >= gPageNum )
      {
         dumpPrintf ( "Error: Index CB Extent ID is not valid: 0x%08lx (%d)"
                      OSS_NEWLINE, indexCBExtentID, indexCBExtentID ) ;
         continue ;
      }
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID,
                        collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent, rc = %d"OSS_NEWLINE,
                      rc ) ;
         continue ;
      }
retry :
      len = dmsDump::dumpIndexCBExtent ( gExtentBuffer, pageSize,
                                         gBuffer, gBufferSize,
                                         NULL,
                                         DMS_SU_DMP_OPT_HEX |
                                         DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                         DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                         gDumpType,
                                         indexRoot ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      dumpPrintf ( OSS_NEWLINE ) ;
      indexRoots[i] = indexRoot ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectIndexExtents ( OSSFILE &file, UINT32 pageSize,
                           dmsExtentID rootID, UINT16 collectionID,
                           CHAR *pExpBuffer, SINT32 &err )
{
   UINT32 len      = 0 ;
   INT32 rc        = SDB_OK ;
   SINT32 localErr = 0 ;
   std::deque<dmsExtentID> childExtents ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX ;
   ixmExtentHead *pExtentHead = NULL ;
   childExtents.push_back ( rootID ) ;

   while ( !childExtents.empty() )
   {
      dmsExtentID childID = childExtents.front() ;
      childExtents.pop_front() ;

      if ( childID == DMS_INVALID_EXTENT || childID >= (SINT32)gPageNum )
      {
         dumpPrintf ( "Error: index extent ID is not valid: 0x%08lx (%d)"
                      OSS_NEWLINE, childID, childID ) ;
         ++err ;
         continue ;
      }
      if ( pExpBuffer )
      {
         dmsSpaceManagementExtent *pSME=(dmsSpaceManagementExtent*)pExpBuffer ;
         if ( pSME->getBitMask( childID ) != DMS_SME_FREE )
         {
            dumpPrintf ( "Error: SME extent 0x%08lx (%d) is not free"
                         OSS_NEWLINE, childID, childID ) ;
            ++err ;
         }
         pSME->setBitMask( childID ) ;
      }

      rc = loadExtent ( file, extentType, pageSize, childID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                      childID, rc ) ;
         ++err ;
         continue ;
      }

      pExtentHead = (ixmExtentHead*)gExtentBuffer ;
      gMBStat._totalIndexPages += 1 ;
      gMBStat._totalIndexFreeSpace += pExtentHead->_totalFreeSize ;

retry :
      localErr = 0 ;
      len = dmsInspect::inspectIndexExtent ( cb, gExtentBuffer, pageSize,
                                             gBuffer, gBufferSize,
                                             collectionID,
                                             childID, childExtents,
                                             localErr ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      err += localErr ;
   }

done :
   return ;
error :
   goto done ;
}

void dumpIndexExtents ( OSSFILE &file, UINT32 pageSize,
                        dmsExtentID rootID, UINT16 collectionID )
{
   UINT32 len = 0 ;
   INT32 rc = SDB_OK ;
   std::deque<dmsExtentID> childExtents ;
   INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_INDEX ;
   childExtents.push_back ( rootID ) ;
   UINT32 count = 0 ;

   while ( !childExtents.empty() )
   {
      dmsExtentID childID = childExtents.front() ;
      childExtents.pop_front() ;
      ++count ;

      dumpPrintf ( "    Dump Index Page %d:"OSS_NEWLINE, childID ) ;
      if ( childID == DMS_INVALID_EXTENT || (UINT32)childID >= gPageNum )
      {
         dumpPrintf ( "Error: index extent ID is not valid: 0x%08lx (%d)"
                      OSS_NEWLINE, childID, childID ) ;
         continue ;
      }
      rc = loadExtent ( file, extentType,
                        pageSize, childID, collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                      childID, rc ) ;
         continue ;
      }

retry :
      len = dmsDump::dumpIndexExtent( gExtentBuffer, pageSize,
                                      gBuffer, gBufferSize, NULL,
                                      DMS_SU_DMP_OPT_HEX |
                                      DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                      DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                      gDumpType,
                                      childExtents,
                                      gShowRecordContent ) ;
      if ( len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry ;
      }
      flushOutput ( gBuffer, len ) ;
      dumpPrintf ( OSS_NEWLINE ) ;
   }
   dumpPrintf ( "    Total: %u pages"OSS_NEWLINE, count ) ;
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   return ;
error :
   goto done ;
}

void inspectDictPageState( CHAR *pExpBuffer, dmsExtentID extentID,
                           SINT32 &err )
{
   dmsDictExtent *dictExtent = (dmsDictExtent*)gDictBuffer ;
   dmsSpaceManagementExtent *sme = (dmsSpaceManagementExtent*)pExpBuffer ;
   for ( INT32 i = 0; i < dictExtent->_blockSize; ++i )
   {
      if ( sme->getBitMask( extentID + i ) != DMS_SME_FREE )
      {
         dumpPrintf( "Error: Compression Dictionary extent 0x%08lx (%d) "
                     "is not free"OSS_NEWLINE, extentID + i, extentID + i ) ;
         ++err ;
      }
      else
      {
         sme->setBitMask( extentID + i ) ;
      }
   }
}


INT32 inspectLobdCollection(OSSFILE &file, UINT32 pageId, 
                            UINT32 sequence, SINT32 &err)
{
    INT32 rc = SDB_OK;
    SINT64 len = 0;
    SINT32 localErr = 0;
    CHAR *lobMeta = (CHAR*) SDB_OSS_MALLOC(sizeof(dmsLobMeta));
    PD_CHECK( lobMeta != NULL, SDB_OOM, error, PDERROR, "malloc failed" );

    rc = ossSeekAndRead ( &file, DMS_HEADER_SZ + gLobdPageSize* pageId,
                          (CHAR*)lobMeta, (SINT64) sizeof(dmsLobMeta), &len ) ;
    
    if ( SDB_OK != rc)
    {
       dumpPrintf ( "Error: Failed to read lobd dmsLobMeta , read %lld bytes, "
                    "rc = %d"OSS_NEWLINE, len, rc ) ;
       if ( !rc )rc = SDB_IO ;
       goto error;
    }

    if(len <(SINT64) sizeof(dmsLobMeta) )
    {
         ossSnprintf ( gBuffer, gBufferSize,
                              "Error: LobMeta size (%d) in lobd file is too small "
                              "expected over size (%d)"OSS_NEWLINE,
                              len, sizeof(dmsLobMeta)) ;
         goto error;
    }
                
retry_dmsLobMeta:
    localErr = 0;
    len = dmsInspect::inspectDmsLobMeta((dmsLobMeta*)lobMeta, gBuffer, gBufferSize,localErr);
    if (len >= gBufferSize -1 )
    {
       if ( reallocBuffer () )
       {
          clearBuffer();
          goto error;
       }
       goto  retry_dmsLobMeta;
    }
    flushOutput(gBuffer, (UINT32)len);
    err += localErr;

done:
    SAFE_OSS_FREE(lobMeta);
    return rc;
error:
    goto done;

}

INT32 inspectLobmMeta(OSSFILE &file, 
        CHAR *pExpBuffer, UINT32 pageId, 
        CHAR *pageBuf, UINT32 pageSize, 
        UINT16 clID, SINT32 &err)
{
    INT32 rc = SDB_OK;
    SINT64 len = 0;
    SINT32 localErr = 0;
    dmsLobDataMapBlk *blk = (dmsLobDataMapBlk*)pageBuf;
    
    rc = ossSeekAndRead ( &file, DMS_BME_OFFSET+ DMS_BME_SZ + 
                   pageSize * pageId, pageBuf, pageSize, &len ) ;
    
    if ( SDB_OK != rc || len < pageSize)
    {
       dumpPrintf ( "Error: Failed to read lobm dmsLobDataMapBlk , read %u bytes, "
                    "rc = %d"OSS_NEWLINE, len, rc ) ;
       if ( !rc )rc = SDB_IO ;
       goto error;
    }

retry_dmsLobDataMapBlk:
    localErr = 0;
    len = dmsInspect::inspectDmsLobDataMapBlk(blk, gBuffer, gBufferSize, clID, localErr);
    if ( (UINT32)len >= gBufferSize -1 )
    {
       if ( reallocBuffer () )
       {
          clearBuffer () ;
          goto error ;
       }
       goto  retry_dmsLobDataMapBlk;
    }
    err += localErr;

    if ( pExpBuffer && DMS_LOB_PAGE_NORMAL == blk->_status )
    {
       dmsSpaceManagementExtent *pSME = ( dmsSpaceManagementExtent*)pExpBuffer ;
       if ( pSME->getBitMask( pageId ) != DMS_SME_FREE )
       {
          dumpPrintf ( "Error: dmsLobDataMapBlk 0x%08lx (%d) is refered by two. this maybe caused a loop "
                     "(prev page id :%d)"OSS_NEWLINE, pageId , pageId, blk->_prevPageInBucket) ;
          ++err ;
       }
       pSME->setBitMask( pageId ) ;
    }
    
done:
    flushOutput( gBuffer, len) ;
    return rc;
error:
    goto done;

}

void inspectCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id,
                            SINT32 hwm, CHAR *pExpBuffer, SINT32 &err,
                            UINT64 &ovfNum, UINT64 &compressedNum )
{
   INT32 rc        = SDB_OK ;
   INT32 len       = 0 ;
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

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to load metadata block, rc = %d"OSS_NEWLINE,
                   rc ) ;
      ++err ;
      goto error ;
   }

   capped = OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_CAPPED ) ;
   firstExtent = mb->_firstExtentID ;
   ossStrncpy( collectionName, mb->_collectionName, DMS_COLLECTION_NAME_SZ ) ;
   dumpPrintf ( "Inspect Data for collection [%d : %s]"OSS_NEWLINE,
                id, collectionName ) ;

   if ( !OSS_BIT_TEST( gAction, ACTION_STAT ) &&
        gOnlyMeta )
   {
      goto done ;
   }

   if ( pExpBuffer &&
        DMS_INVALID_EXTENT != mb->_mbExExtentID )
   {
      dmsMetaExtent *pMetaEx = NULL ;
      extentType = INSPECT_EXTENT_TYPE_MBEX ;
      rc = loadExtent( file, extentType, pageSize, mb->_mbExExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "Error: Failed to load mb expand extent %d, rc = %d"
                     OSS_NEWLINE, mb->_mbExExtentID, rc ) ;
         goto error ;
      }
      pMetaEx = ((dmsMetaExtent*)gExtentBuffer)  ;

      dmsSpaceManagementExtent *pSME=(dmsSpaceManagementExtent*)pExpBuffer ;
      for ( INT32 i = 0 ; i < pMetaEx->_blockSize ; ++i )
      {
         if ( pSME->getBitMask( mb->_mbExExtentID + i ) != DMS_SME_FREE )
         {
            dumpPrintf ( "Error: Meta Expand extent 0x%08lx (%d) is not free"
                         OSS_NEWLINE, mb->_mbExExtentID + i ,
                         mb->_mbExExtentID + i ) ;
            ++err ;
         }
         pSME->setBitMask( mb->_mbExExtentID + i ) ;
      }
   }

   prepareCompressor( file, pageSize, mb, id,
                      pExpBuffer, compressorEntry, err ) ;

   extentType = INSPECT_EXTENT_TYPE_DATA ;
   while ( DMS_INVALID_EXTENT != firstExtent )
   {
      if ( (UINT32)firstExtent >= gPageNum )
      {
         dumpPrintf ( "Error: data extent 0x%08lx (%d) is out of range"
                      OSS_NEWLINE, firstExtent, firstExtent ) ;
         ++err ;
         break ;
      }
      rc = loadExtent ( file, extentType, pageSize, firstExtent, id ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                      firstExtent, rc ) ;
         ++err ;
         goto error ;
      }

      pExtent = (dmsExtent*)gExtentBuffer ;
      gMBStat._totalDataPages += pExtent->_blockSize ;
      gMBStat._totalDataFreeSpace += pExtent->_freeSpace ;
      gMBStat._totalRecords += pExtent->_recCount ;

      if ( pExpBuffer )
      {
         dmsSpaceManagementExtent *pSME=(dmsSpaceManagementExtent*)pExpBuffer ;

         for ( INT32 i = 0 ; i < pExtent->_blockSize ; ++i )
         {
            if ( pSME->getBitMask( firstExtent + i ) != DMS_SME_FREE )
            {
               dumpPrintf ( "Error: SME extent 0x%08lx (%d) is not free"
                            OSS_NEWLINE, firstExtent + i , firstExtent + i ) ;
               ++err ;
            }
            pSME->setBitMask( firstExtent + i ) ;
         }
      }

      if ( !OSS_BIT_TEST ( gAction, ACTION_INSPECT ) )
      {
         firstExtent = pExtent->_nextExtent ;
         continue ;
      }
      extScan = TRUE ;

retry_data :
      UINT64 extTotalRecord = 0 ;
      UINT64 extCompressedNum = 0 ;

      extentRIDList.clear() ;
      tempExtent = firstExtent ;
      localErr = 0 ;
      len = dmsInspect::inspectDataExtent ( cb, gExtentBuffer,
                               ((dmsExtent*)gExtentBuffer)->_blockSize*pageSize,
                               gBuffer, gBufferSize, hwm, id, tempExtent,
                               &extentRIDList, localErr,
                               &compressorEntry,
                               extTotalRecord,
                               extCompressedNum,
                               capped ) ;
      if ( (UINT32)len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_data ;
      }

      totalRecord += extTotalRecord ;
      compressedNum += extCompressedNum ;
      ovfNum += extentRIDList.size() ;

      flushOutput ( gBuffer, len ) ;

      if ( extentRIDList.size() != 0 )
      {
         extCompressedNum = 0 ;
         inspectOverflowedRecords( file, pageSize, id, firstExtent,
                                   extentRIDList, err, &compressorEntry,
                                   extCompressedNum ) ;
         compressedNum += extCompressedNum ;
      }

      firstExtent = tempExtent ;
      err += localErr ;
   } //end while

   if ( !OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
        0 != compressedNum )
   {
      dumpPrintf ( "Error: Collection is not compressed, but has %llu "
                   "compressed records"OSS_NEWLINE, compressedNum ) ;
   }
   else if ( gMBStat._totalRecords != mb->_totalRecords )
   {
      dumpPrintf ( "Error: Collection records is not the same[ "
                   "mb->_totalRecords: %llu, ext total records: %llu"
                   OSS_NEWLINE, mb->_totalRecords,
                   gMBStat._totalRecords ) ;
   }
   else if ( extScan && totalRecord != mb->_totalRecords )
   {
      dumpPrintf ( "Error: Collection records is not the same[ "
                   "mb->_totalRecords: %llu, scan total records: %llu"
                   OSS_NEWLINE, mb->_totalRecords, totalRecord ) ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectCollectionIndex( OSSFILE &file, UINT32 pageSize, UINT16 id,
                             SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   dmsMB *mb       = NULL ;
   std::map<UINT16, dmsExtentID> indexRoots ;
   std::map<UINT16, dmsExtentID>::iterator it ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to load metadata block, rc = %d"OSS_NEWLINE,
                   rc ) ;
      ++err ;
      goto error ;
   }
   ossStrncpy( collectionName, mb->_collectionName,
               DMS_COLLECTION_NAME_SZ ) ;

   dumpPrintf ( "Inspect Index for collection [%d : %s]"OSS_NEWLINE,
                id, collectionName ) ;

   inspectIndexDef ( file, pageSize, id, mb, pExpBuffer, indexRoots, err ) ;

   if ( !OSS_BIT_TEST( gAction, ACTION_STAT ) &&
        gOnlyMeta )
   {
      goto done ;
   }

   for ( it = indexRoots.begin() ; it != indexRoots.end() ; ++it )
   {
      UINT16 indexID = it->first ;
      dumpPrintf ( "    Index Inspection for Collection [%02u], Index [%02u]"
                   OSS_NEWLINE, id, indexID ) ;
      dmsExtentID rootID = it->second ;
      if ( DMS_INVALID_EXTENT != rootID )
      {
         inspectIndexExtents ( file, pageSize, rootID, id,
                               pExpBuffer, err ) ;
      }
   }

done :
   return ;
error :
   goto done ;
}

void inspectCollectionLob( OSSFILE &lobmFile, UINT32 pageSize, 
                              UINT16 clId, SINT32 hwm, 
                              CHAR *pExpBuffer, SINT32 &err, 
                              UINT32 &pageCount, UINT32 &lobCount)
{
   pageCount = 0;
   lobCount = 0;

   INT32 rc        = SDB_OK ;
   INT32 pageId = DMS_LOB_INVALID_PAGEID;
   vector<LobFragments> &vctFrgmts = gCl2PageMap[clId];

   CHAR *blkBuf = (CHAR*) SDB_OSS_MALLOC(pageSize);
   PD_CHECK( blkBuf != NULL, SDB_OOM, error, PDERROR, "malloc failed" );
    
   for(UINT32 i = 0; i < vctFrgmts.size(); i ++, lobCount++)
   {
      LobFragments &fragmt = vctFrgmts[i];
      map<UINT32, UINT32> &pages = fragmt.sequences;
      map<UINT32, UINT32>::iterator it = pages.begin();
      for(; it != pages.end(); it++, pageCount++)
      {
         pageId = it->second;
         rc = inspectLobmMeta(lobmFile, pExpBuffer, pageId, blkBuf, pageSize, clId, err);
         if ( rc ) goto error;

         if(gOnlyMeta)
         {
            continue;
         }
         
         if (DMS_LOB_META_SEQUENCE == it->first && gShowRecordContent) 
         {
            rc = inspectLobdCollection(gLobdFile, pageId, it->first, err);
            if ( rc ) goto error;
         }
      }
   }

done:
    SAFE_OSS_FREE(blkBuf);
    return;
error:
    goto done ;
}

void inspectCollection ( OSSFILE &file, UINT32 pageSize, UINT16 id,
                         SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
   UINT32 len = 0 ;
   gMBStat.reset() ;
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      UINT64 ovfNum = 0 ;
      UINT64 compressedNum = 0 ;
      inspectCollectionData( file, pageSize, id, hwm,
                             pExpBuffer, err, ovfNum,
                             compressedNum ) ;
      len = ossSnprintf( gBuffer, gBufferSize,
                         " ****The collection data info****"OSS_NEWLINE
                         "   Total Record           : %llu"OSS_NEWLINE
                         "   Total Data Pages       : %u"OSS_NEWLINE
                         "   Total Data Free Space  : %llu"OSS_NEWLINE
                         "   Total OVF Record       : %llu"OSS_NEWLINE
                         "   Total Compressed Record: %llu"OSS_NEWLINE,
                         gMBStat._totalRecords,
                         gMBStat._totalDataPages,
                         gMBStat._totalDataFreeSpace,
                         ovfNum,
                         compressedNum ) ;
      flushOutput( gBuffer, len ) ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      inspectCollectionIndex( file, pageSize, id, hwm, pExpBuffer, err ) ;
      len = ossSnprintf( gBuffer, gBufferSize,
                         " ****The collection index info****"OSS_NEWLINE
                         "   Total Index Pages      : %u"OSS_NEWLINE
                         "   Total Index Free Space : %llu"OSS_NEWLINE
                         "   Unique Index Number    : %u"OSS_NEWLINE,
                         gMBStat._totalIndexPages,
                         gMBStat._totalIndexFreeSpace,
                         gMBStat._uniqueIdxNum ) ;
      flushOutput( gBuffer, len ) ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      UINT32 lobPageCount = 0;
      UINT32 lobCount = 0;
      inspectCollectionLob( file, pageSize, id, hwm, pExpBuffer, err, lobPageCount, lobCount) ;

      len = ossSnprintf( gBuffer, gBufferSize,
                         "****The collection lob info****"OSS_NEWLINE
                         "   Collection ID            : %u"OSS_NEWLINE
                         "   Total Lob Pages          : %u"OSS_NEWLINE
                         "   Total Lobs               : %u"OSS_NEWLINE,
                         id,
                         lobPageCount,
                         lobCount);
                         
      len += ossSnprintf(gBuffer + len, gBufferSize - len, OSS_NEWLINE);
      flushOutput( gBuffer, len ) ;
   }
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

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to load metadata block, rc = %d"OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   capped = OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_CAPPED ) ;

   if ( DMS_INVALID_EXTENT != mb->_mbExExtentID )
   {
      UINT32 size = 0 ;
      extentType = INSPECT_EXTENT_TYPE_MBEX ;

      dumpPrintf ( " Dump Meta Extent for Collection [%d]"OSS_NEWLINE, id ) ;

      rc = loadExtent( file, extentType, pageSize, mb->_mbExExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "Error: Failed to load mb expand extent %d, rc = %d"
                     OSS_NEWLINE, mb->_mbExExtentID, rc ) ;
         goto error ;
      }
      size = ((dmsMetaExtent*)gExtentBuffer)->_blockSize*pageSize ;

   retry_mbEx:
      len = dmsDump::dumpMBEx( gExtentBuffer, size,
                               gBuffer, gBufferSize, NULL,
                               DMS_SU_DMP_OPT_HEX |
                               DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                               DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                               gDumpType, mb->_mbExExtentID ) ;
      if ( (UINT32)len >= gBufferSize -1 )
      {
         if ( reallocBuffer () )
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
      prepareCompressor(file, pageSize, mb, id, NULL, compressorEntry, err  ) ;
   }

   if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
   {
      UINT32 size = 0 ;
      dumpPrintf ( "Dump Compression Dictionary Extent for Collection [%d]"
                   OSS_NEWLINE, id ) ;

      size = ((dmsDictExtent*)gDictBuffer)->_blockSize * pageSize ;

   retry_dictExt:
      len = dmsDump::dumpDictExtent( gDictBuffer, size,
                                     gBuffer, gBufferSize, NULL,
                                     DMS_SU_DMP_OPT_HEX |
                                     DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                     DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                     gDumpType, mb->_dictExtentID ) ;
      if ( (UINT32)len >= gBufferSize - 1 )
      {
         if ( reallocBuffer() )
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
      dumpPrintf( "Dump extend option extent for collection [%d]"OSS_NEWLINE,
                  id ) ;
      rc = loadExtent( file, extentType, pageSize, mb->_mbOptExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "Error: Failed to load mb extend option extent %d, rc = %d"
                     OSS_NEWLINE, mb->_mbOptExtentID, rc ) ;
         goto error ;
      }
      size = ((dmsOptExtent*)gExtentBuffer)->_blockSize * pageSize ;

   retry_extOptExt:
      len = dmsDump::dumpExtOptExtent( gExtentBuffer, size,
                                       gBuffer, gBufferSize, NULL,
                                       DMS_SU_DMP_OPT_HEX |
                                       DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                       DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                       gDumpType, mb->_mbExExtentID,
                                       capped ? DMS_STORAGE_CAPPED : DMS_STORAGE_NORMAL ) ;
      if ( (UINT32)len >= gBufferSize - 1 )
      {
         if ( reallocBuffer() )
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
   dumpPrintf ( " Dump Data for Collection [%d]"OSS_NEWLINE, id ) ;
   while ( DMS_INVALID_EXTENT != firstExtent )
   {
      rc = loadExtent ( file, extentType, pageSize, firstExtent, id ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent %d, rc = %d"OSS_NEWLINE,
                      firstExtent, rc ) ;
         goto error ;
      }

retry_data :
      extentRIDList.clear() ;
      tempExtent = firstExtent ;
      len = dmsDump::dumpDataExtent ( cb, gExtentBuffer,
                               ((dmsExtent*)gExtentBuffer)->_blockSize*pageSize,
                               gBuffer, gBufferSize, NULL,
                               DMS_SU_DMP_OPT_HEX |
                               DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                               DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                               gDumpType, tempExtent, &compressorEntry,
                               &extentRIDList,  gShowRecordContent, capped ) ;

      if ( (UINT32)len >= gBufferSize-1 )
      {
         if ( reallocBuffer () )
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

   rc = loadMB ( id, mb ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to load metadata block, rc = %d"OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }

   dumpIndexDef ( file, pageSize, id, mb, indexRoots ) ;

   if ( gOnlyMeta )
   {
      goto done ;
   }

   for ( it = indexRoots.begin() ; it != indexRoots.end() ; ++it )
   {
      UINT16 indexID = it->first ;
      dmsExtentID rootID = it->second ;
      dumpPrintf ( "    Index Dump for Collection [%02u], Index [%02u]"
                   OSS_NEWLINE, id, indexID ) ;
      if ( DMS_INVALID_EXTENT != rootID )
      {
         dumpIndexExtents ( file, pageSize, rootID, id ) ;
      }
      else
      {
         dumpPrintf ( "       Root extent does not exist"OSS_NEWLINE ) ;
      }
   }

done :
   return ;
error :
   goto done ;
}

INT32 getBME(OSSFILE &lobmFile, CHAR *pBME)
{
    INT32 rc = SDB_OK ;
    SINT64 lenRead = 0 ;
    
    rc = ossSeekAndRead ( &lobmFile, DMS_BME_OFFSET,
                          pBME, DMS_BME_SZ, &lenRead ) ;
    
    if ( rc || lenRead != DMS_BME_SZ )
    {
       dumpPrintf ( "Error: Failed to read lobm buckets , read %lld bytes, "
                    "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
       if ( !rc )
          rc = SDB_IO ;
    }

    return rc ;
}

void cacheLobDataMapBlk(dmsLobDataMapBlk *blk, UINT32 pageId)
{
   UINT32 i = 0;
   vector<LobFragments> &pages = gCl2PageMap[blk->_mbID];
   for (i = 0; i < pages.size(); i ++ )
   {
       LobFragments &frag = pages[i];
       if (0 == ossMemcmp(frag.oid, blk->_oid, DMS_LOB_OID_LEN))
       {
           frag.sequences[blk->_sequence] = pageId;
           break;
       }
   }
   
   if (i == pages.size())
   {
       LobFragments frag;
       ossMemcpy(frag.oid, blk->_oid, DMS_LOB_OID_LEN);
       frag.sequences[blk->_sequence] = pageId;
       pages.push_back(frag);
   }
}


INT32 loadLobDataMapBlk(OSSFILE &lobmFile, 
        CHAR *outBuf, UINT32 outSize,
        INT32 pageId, dmsLobDataMapBlk *blk, 
        UINT32 pageSize, UINT32 &dep,
        UINT32 &msgLen)
{
    INT32 rc = SDB_OK ;
    SINT64 lenRead = 0 ;
    INT32 nextPageId = pageId;
    pair<set<UINT32>::iterator, bool> ret;
    set<UINT32> pages;
    
    dep = 0;
    static const UINT32 FIRSTPAGEOFFSET = DMS_BME_OFFSET + DMS_BME_SZ;
    do
    {
        ret = pages.insert(nextPageId);
        if(!ret.second)
        {
            msgLen += ossSnprintf(outBuf + msgLen, outSize - msgLen, 
                    "Error: Failed to load lobm buckets , because there was a loop in bucket"
                    "about page(%u)"OSS_NEWLINE, nextPageId);
           return SDB_DMS_RECORD_INVALID;
        }

        rc = ossSeekAndRead ( &lobmFile, FIRSTPAGEOFFSET + pageSize * nextPageId,
                              (CHAR*)blk, pageSize, &lenRead ) ;
        
        if ( SDB_OK != rc || lenRead != pageSize )
        {
           dumpPrintf ( "Error: Failed to read lobm buckets , read %lld bytes, "
                        "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
           return rc;
        }
        
        dep++;
        cacheLobDataMapBlk(blk, nextPageId);

        nextPageId = blk->_nextPageInBucket;
    } while(nextPageId != DMS_LOB_INVALID_PAGEID);

    return rc;
}


INT32 parseBME(OSSFILE &lobmFile, UINT32 pageSize, CHAR *pBME)
{
   INT32 rc = SDB_OK;
   INT32 pageId = DMS_LOB_INVALID_PAGEID;
   UINT32 dep = 0;
   UINT32 len = 0;
   UINT32 msgLen = 0;
   UINT32 localErr = 0;
   dmsLobDataMapBlk *blkBuf = (dmsLobDataMapBlk*)SDB_OSS_MALLOC(pageSize);
   PD_CHECK( blkBuf != NULL, SDB_OOM, error, PDERROR, "malloc failed" );

   for(UINT32 i = 0; i < DMS_BUCKETS_NUM; i++)
   {
      pageId = ((dmsBucketsManagementExtent*)pBME)->_buckets[i];
      if (pageId == DMS_LOB_INVALID_PAGEID) 
      { 
          continue;
      }
      
   retry_inspectBucketLoop:
      rc = loadLobDataMapBlk(lobmFile, gBuffer, gBufferSize, pageId, blkBuf, pageSize, dep, msgLen);
      if ( msgLen >= gBufferSize - 1 )
      {
         flushOutput(gBuffer,len);
         len = msgLen = 0;
         goto retry_inspectBucketLoop ;
      }
      len = msgLen;

      if (rc == SDB_DMS_RECORD_INVALID)
      {
         localErr++;
      }
      else if ( rc != SDB_OK) 
      {
         break;
      }

      if(gBucketDep) 
      {
         gBucketDep[i] = dep;
      }
   }

done:

retry_summary:
   if ( 0 == localErr )
   {
      msgLen += ossSnprintf ( gBuffer + len, gBufferSize - len,
                        " Inspect Bucket Management Exent Done "
                        "without Error"OSS_NEWLINE ) ;
   }
   else
   {
      msgLen += ossSnprintf ( gBuffer + len, gBufferSize - len,
                        " Inspect Bucket Management Exent Done "
                        "with Error: %d"OSS_NEWLINE, localErr ) ;
   }

   if ( msgLen >= gBufferSize - 1 )
   {
      flushOutput(gBuffer,len);
      len = msgLen = 0;
      goto retry_summary ;
   }
   
   flushOutput ( gBuffer, len ) ;
   SAFE_OSS_FREE(blkBuf);
   return rc;
error:
   goto done;
}

INT32 dumpLobmMeta(OSSFILE &file, UINT32 pageId, CHAR *pageBuf, UINT32 pageSize)
{
    INT32 rc = SDB_OK;
    SINT64 len = 0;
    
    rc = ossSeekAndRead ( &file, DMS_BME_OFFSET+ DMS_BME_SZ + 
                   pageSize* pageId, (CHAR*)pageBuf, pageSize, &len ) ;
    
    if ( SDB_OK != rc || len < pageSize)
    {
       dumpPrintf ( "Error: Failed to read lobm dmsLobDataMapBlk , read %u bytes, "
                    "rc = %d"OSS_NEWLINE, len, rc ) ;
       goto error;
    }

retry_dmsLobDataMapBlk:

    len = dmsDump::dumpDmsLobDataMapBlk((dmsLobDataMapBlk*)pageBuf, gBuffer, gBufferSize, NULL,
                             DMS_SU_DMP_OPT_HEX |
                             DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                             DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                             gDumpType, pageSize);
    if ( (UINT32)len >= gBufferSize -1 )
    {
       if ( reallocBuffer () )
       {
          clearBuffer () ;
          goto error ;
       }
       goto  retry_dmsLobDataMapBlk;
    }
    
done:
    flushOutput( gBuffer, len) ;
    return rc;
error:
    goto done;

}

INT32 dumpLobdCollection(OSSFILE &file, UINT32 pageId,
        CHAR* lobdPageBuf,UINT32 sequence)
{
    INT32 rc = SDB_OK;
    SINT64 len = 0;
    UINT32 metaSize = 0;
     
    rc = ossSeekAndRead ( &file, DMS_HEADER_SZ + gLobdPageSize* pageId,
                          (CHAR*)lobdPageBuf, gLobdPageSize, &len ) ;
    if ( SDB_OK != rc)
    {
       dumpPrintf ( "Error: Failed to read lobd dmsLobMeta , read %lld bytes, "
                    "rc = %d"OSS_NEWLINE, len, rc ) ;
       if ( !rc )rc = SDB_IO ;
       return rc;
    }

    if ( DMS_LOB_META_SEQUENCE == sequence)
    {
        if((UINT32)len < sizeof(dmsLobMeta) )
        {
             ossSnprintf ( gBuffer, gBufferSize,
                                  "Error: LobMeta size (%d) in lobd file is too small "
                                  "expected size (%d)"OSS_NEWLINE,
                                  len, sizeof(dmsLobMeta)) ;
             goto error;
        }
                
        dmsLobMeta *lobMeta = (dmsLobMeta *)lobdPageBuf;
        metaSize = (lobMeta->hasPiecesInfo()) 
                                           ? DMS_LOB_META_LENGTH 
                                           : sizeof(dmsLobMeta);
        
        if(len < metaSize) 
        {
             ossSnprintf ( gBuffer, gBufferSize,
                                  "Error: LobMeta size (%d) in lobd file is too small "
                                  "expected size (%d)"OSS_NEWLINE,
                                  len, metaSize ) ;
             goto error;
        }

    retry_dmsLobMeta:

        len = dmsDump::dumpDmsLobMeta(lobdPageBuf, metaSize,
                                 gBuffer, gBufferSize, NULL,
                                 DMS_SU_DMP_OPT_HEX |
                                 DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                 DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                 gDumpType);
        if ((UINT32)len >= gBufferSize -1 )
        {
           if ( reallocBuffer () )
           {
              clearBuffer () ;
              goto error ;
           }
           goto  retry_dmsLobMeta;
        }
        flushOutput( gBuffer, (UINT32)len) ;
    }

    if(!gShowRecordContent) goto done;
    
retry_dmsLobData:
    len = dmsDump::dumpDmsLobData(lobdPageBuf + metaSize, gLobdPageSize - metaSize,
                                             gBuffer , gBufferSize , NULL,
                                             DMS_SU_DMP_OPT_HEX |
                                             DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                                             DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                                             gDumpType);
    if ((UINT32)len >= gBufferSize -1 )
    {
       if ( reallocBuffer () )
       {
          clearBuffer () ;
          goto error ;
       }
       goto  retry_dmsLobData;
    }
    flushOutput( gBuffer, (UINT32)len) ;
    
done:
    return rc;
error:
    goto done;

}

INT32 dumpCollectionLob( OSSFILE &lobmFile,  UINT32 pageSize, UINT16 id)
{

    INT32 rc = SDB_OK ;
    CHAR *pLobdPageBuf = NULL;
    INT32 pageId = DMS_LOB_INVALID_PAGEID;
    vector<LobFragments> &vctFrgmts = gCl2PageMap[id];

    if (!gOnlyMeta)
    {
        pLobdPageBuf = (CHAR*)SDB_OSS_MALLOC(gLobdPageSize);
         if ( NULL == pLobdPageBuf)
        {
            dumpPrintf ( "Error: Failed to alloc buffer with size %u,rc = %d"
                    OSS_NEWLINE, gLobdPageSize, SDB_OOM ) ;
            return SDB_OOM ;
        }
    }

    CHAR *pPageBuf = (CHAR*)SDB_OSS_MALLOC(pageSize);
     if ( NULL == pPageBuf)
    {
        dumpPrintf ( "Error: Failed to alloc buffer with size %u,rc = %d"
                OSS_NEWLINE, pageSize, SDB_OOM ) ;
        rc = SDB_OOM ;
        goto error;
    }

    for(UINT32 i = 0; i < vctFrgmts.size(); i ++)
    {

        LobFragments &fragmt = vctFrgmts[i];
        std::map<UINT32, UINT32> &pages = fragmt.sequences;
        std::map<UINT32, UINT32>::iterator it = pages.begin();

        for(;it != pages.end(); ++it)
        {
            pageId = it->second;
            rc = dumpLobmMeta(lobmFile, pageId, pPageBuf, pageSize);
            if ( rc ) goto error ;

            if (gOnlyMeta)
            {
               continue;  
            }
            
            if (!gShowRecordContent && DMS_LOB_META_SEQUENCE != it->first) 
            {
                continue;
            }

            rc = dumpLobdCollection(gLobdFile, pageId, pLobdPageBuf, it->first);
            if ( rc ) goto error ;
        }
    }

done :
    SAFE_OSS_FREE(pPageBuf);
    SAFE_OSS_FREE(pLobdPageBuf);
    return rc;
error :
   goto done ;
 
}

void dumpCollection ( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      dumpCollectionData( file, pageSize, id ) ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      dumpCollectionIndex( file, pageSize, id ) ;
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
   for ( it = collections.begin(); it != collections.end(); ++it )
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
      dumpCollection ( file, pageSize, *it ) ;
   }
}

void inspectCollections ( OSSFILE &file, UINT32 pageSize, SINT32 hwm,
                          CHAR *pExpBuffer, SINT32 &err )
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   SINT32 localErr = 0 ;
   vector<UINT16> collections ;

   if ( FALSE == gInitMME )
   {
      SINT64 lenRead = 0 ;
      rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                            DMS_MME_SZ, &lenRead ) ;
      if ( rc || lenRead != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                      "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
         ++err ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

retry :
   collections.clear() ;
   localErr = 0 ;
   len = dmsInspect::inspectMME ( gMMEBuff, DMS_MME_SZ,
                                  gBuffer, gBufferSize,
                                  ossStrlen ( gCLName ) ? gCLName : NULL,
                                  ( SDB_INSPT_DATA==gCurInsptType ) ? hwm : -1,
                                  collections, localErr ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( reallocBuffer () )
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
      inspectCollections ( file, pageSize, collections, hwm,
                           pExpBuffer, err ) ;
   }
   if ( pExpBuffer )
   {
      inspectSME ( file, pExpBuffer, hwm, err ) ;
   }

done :
   return ;
error :
   goto done ;
}

UINT32 inspectBalance(CHAR *outBuf, INT64 outSize)
{
    SDB_ASSERT(gBalance,"impossible");
    SDB_ASSERT(gBucketDep,"forbidden");

    UINT32 len = 0;
    UINT64 sum = 0;
    double value = 0.0;
    double variance = 0.0;
    UINT32 maxDep = 0;
    UINT32 minDep = (UINT32)-1;
    UINT32 zeroDepCount =0;

    for(UINT32 i = 0; i < DMS_BUCKETS_NUM; i++)
    {
        if (minDep > gBucketDep[i])
        {
            minDep = gBucketDep[i];
        }
            
        if (gBucketDep[i] == 0) 
        {
            zeroDepCount++;
            continue;
        }
            
        if (maxDep < gBucketDep[i]) 
        {
            maxDep = gBucketDep[i];
        }

        sum += gBucketDep[i];
        value += gBucketDep[i] *gBucketDep[i] ;
    }

    variance = (value - (double)(sum *sum /DMS_BUCKETS_NUM)) / DMS_BUCKETS_NUM;
    
    len += ossSnprintf(outBuf + len, outSize -len, "Inspect Lobd Balance:"OSS_NEWLINE);
    
    len += ossSnprintf(outBuf+ len, outSize -len, 
                                 " Average Bucket Depth   :%.6f"OSS_NEWLINE,
                                 (float)sum/DMS_BUCKETS_NUM);
                                 
    len += ossSnprintf(outBuf+ len, outSize -len, 
                                 " Max Bucket Depth       :%u"OSS_NEWLINE, maxDep);
                                 
    len += ossSnprintf(outBuf+ len, outSize -len, 
                                 " Min Bucket Depth       :%u"OSS_NEWLINE,minDep);
                                 
    len += ossSnprintf(outBuf+ len, outSize -len, 
                                 " Zero-Depth Number      :%u"OSS_NEWLINE, zeroDepCount);
                                 
    len += ossSnprintf(outBuf+ len, outSize -len, 
                                 " Variance               :%.6f (%s)"OSS_NEWLINE,(float)variance, "Less is Better");
                                 
    len += ossSnprintf(outBuf+ len, outSize -len, OSS_NEWLINE);
    return len;
}


INT32 loadPages(OSSFILE &lobmFile, CHAR *pCache, 
        UINT32 cacheSize, UINT32 pageSize, UINT32 &pageCount)
{
   INT32 rc = SDB_OK ;
   SINT64 lenRead = 0 ;

   rc = ossRead(&lobmFile, pCache, (SINT64)cacheSize, &lenRead);
   switch (rc)
   {
      case SDB_EOF:
      case SDB_OK:
      {
         pageCount = (UINT32)( lenRead /pageSize );
         return SDB_OK;
      }
      default:
      {
         dumpPrintf ( "Error: Failed to read lobm pages , read %lld bytes, "
                 "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      }
   }
   return rc ;
}

void parsePages(OSSFILE &lobmFile, CHAR *pCache, 
        UINT32 startPage, UINT32 endPage,
        UINT32 pageSize, CHAR *pSME)
{
   dmsLobDataMapBlk *lobDataMapBlk = NULL;
   dmsSpaceManagementExtent *bitmap = (dmsSpaceManagementExtent *)pSME;
   while(startPage < endPage)
   {
      lobDataMapBlk = (dmsLobDataMapBlk*)pCache;
      if ( (bitmap->getBitMask(startPage) == DMS_SME_ALLOCATED)
              && lobDataMapBlk->isNormal() )
      {
         cacheLobDataMapBlk(lobDataMapBlk, startPage);
      }  
      pCache += pageSize;
      startPage += 1;
   }
}


INT32 loadLobMetaData(OSSFILE &lobmFile, UINT32 pageSize, CHAR *pSME)
{
   INT32 rc = SDB_OK ;
   UINT32 pageCount = 0;
   UINT32 startPage = 0;
   SDB_ASSERT(lobmFile.isOpened(),"forbidden");

   const static UINT32 MAX_CACHE_PAGE_SZ = DMS_PAGE_SIZE256K /pageSize;

   UINT32 cacheSize = ( ( gPageNum > MAX_CACHE_PAGE_SZ ) 
                                ? MAX_CACHE_PAGE_SZ  : gPageNum ) * pageSize;

   CHAR *pCache = (CHAR*)SDB_OSS_MALLOC(cacheSize);
   PD_CHECK( pCache != NULL, SDB_OOM, error, PDERROR, "malloc failed" );

   rc = ossSeek(&lobmFile, DMS_BME_OFFSET + DMS_BME_SZ, OSS_SEEK_SET);
   if ( rc != SDB_OK )
   {
      dumpPrintf ( "Error: Failed to seek lobm pages "OSS_NEWLINE);
      goto error;
   }

   do
   {
      rc = loadPages(lobmFile, pCache, cacheSize, pageSize, pageCount);
      if ( rc != SDB_OK )
      {
         dumpPrintf ( "Error: Failed to load lobm buckets, rc = %d"OSS_NEWLINE,  rc ) ;
         goto error ;
      }

      parsePages(lobmFile, pCache, startPage, startPage + pageCount, pageSize, pSME);
      startPage += pageCount;
       
   }while(startPage < gPageNum);

done :
    SAFE_OSS_FREE(pCache);
    return rc;
error :
   goto done ;
}

INT32 InspectBME(OSSFILE &file, UINT32 pageSize)
{
   INT32 rc = SDB_OK ;
   SDB_ASSERT(file.isOpened(),"forbidden");

   CHAR *pBME= (CHAR*)SDB_OSS_MALLOC(DMS_BME_SZ);
   PD_CHECK( pBME != NULL, SDB_OOM, error, PDERROR, "malloc failed" );

   rc = getBME(file,pBME);
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to load lobm buckets, rc = %d"OSS_NEWLINE,  rc ) ;
      goto error ;
   }

   rc = parseBME(file, pageSize,  pBME);
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to parse lobm buckets, rc = %d"OSS_NEWLINE,  rc ) ;
      goto error ;
   }

done :
    SAFE_OSS_FREE(pBME);
    return rc;
error :
   goto done ;
}

void inspectLobCollections ( OSSFILE &file, UINT32 pageSize, SINT32 hwm,
                          CHAR *pExpBuffer, SINT32 &err )
{

   INT32 rc = SDB_OK;
   UINT32 len = 0;
   vector<UINT16> collections ;

   if (gBalance)
   {
      gBucketDep = (UINT32 *) SDB_OSS_MALLOC(sizeof(UINT32) *DMS_BUCKETS_NUM);
      PD_CHECK( gBucketDep != NULL, SDB_OOM, error, PDERROR, "malloc failed" );
      ossMemset(gBucketDep, 0, sizeof(UINT32) *DMS_BUCKETS_NUM);
   }

   rc = InspectBME(file, pageSize);
   if ( rc ) goto error ;

   if (gBalance) 
   {
   retry_inspectBalance:
      len = inspectBalance(gBuffer, gBufferSize);
      if ( len >= gBufferSize - 1 )
      {
         if ( reallocBuffer () )
         {
            clearBuffer () ;
            goto error ;
         }
         goto retry_inspectBalance ;
      }
      flushOutput ( gBuffer, len ) ;
   }

   if ( FALSE == gInitMME )
   {
      SINT64 lenRead = 0 ;
      rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
            DMS_MME_SZ, &lenRead ) ;
      if ( rc || lenRead != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
         "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

   retry :
   collections.clear() ;
   len = dmsInspect::inspectMME ( gMMEBuff, DMS_MME_SZ,
                                 gBuffer, gBufferSize,
                                 ossStrlen ( gCLName ) ? gCLName : NULL,
                                 ( SDB_INSPT_DATA==gCurInsptType ) ? hwm : -1,
                                 collections, err ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( reallocBuffer () )
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
   if ( pExpBuffer )
   {
      inspectSME ( file, pExpBuffer, hwm, err ) ;
   }

done :
   SAFE_OSS_FREE(gBucketDep);
   return ;
error :
   goto done ;
}

void dumpRawPage ( OSSFILE &file, UINT32 pageSize, SINT32 pageID )
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   rc = getExtent ( file, pageID, pageSize, 1 ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to get extent %d, rc = %d"
                   OSS_NEWLINE, pageID, rc ) ;
      goto error ;
   }
retry :
   len = dmsDump::dumpRawPage ( gExtentBuffer, gExtentBufferSize,
                                gBuffer, gBufferSize ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( reallocBuffer () )
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

void repaireCollection( OSSFILE &file, dmsMB *pMB,
                        UINT32 id )
{
   dumpPrintf( "Begin to repaire collection: %s.%s, ID: %u"OSS_NEWLINE,
               gCSName, gCLName, id ) ;
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_FLAG )
   {
      dumpPrintf( "   Flag[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_flag, gRepaireMB._flag ) ;
      pMB->_flag = gRepaireMB._flag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_ATTR )
   {
      dumpPrintf( "   Attr[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_attributes, gRepaireMB._attributes ) ;
      pMB->_attributes = gRepaireMB._attributes ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LID )
   {
      dumpPrintf( "   LID[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_logicalID, gRepaireMB._logicalID ) ;
      pMB->_logicalID = gRepaireMB._logicalID ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_RECORD )
   {
      dumpPrintf( "   Record[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_totalRecords, gRepaireMB._totalRecords ) ;
      pMB->_totalRecords = gRepaireMB._totalRecords ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_DATAPAGE )
   {
      dumpPrintf( "   DataPages[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_totalDataPages, gRepaireMB._totalDataPages ) ;
      pMB->_totalDataPages = gRepaireMB._totalDataPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXPAGE )
   {
      dumpPrintf( "   IndexPages[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_totalIndexPages, gRepaireMB._totalIndexPages ) ;
      pMB->_totalIndexPages = gRepaireMB._totalIndexPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOBPAGE )
   {
      dumpPrintf( "   LobPages[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_totalLobPages, gRepaireMB._totalLobPages ) ;
      pMB->_totalLobPages = gRepaireMB._totalLobPages ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_DATAFREE )
   {
      dumpPrintf( "   DataFreeSpace[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_totalDataFreeSpace, gRepaireMB._totalDataFreeSpace ) ;
      pMB->_totalDataFreeSpace = gRepaireMB._totalDataFreeSpace ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXFREE )
   {
      dumpPrintf( "   IndexFreeSpace[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_totalIndexFreeSpace, gRepaireMB._totalIndexFreeSpace ) ;
      pMB->_totalIndexFreeSpace = gRepaireMB._totalIndexFreeSpace ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDXNUM )
   {
      dumpPrintf( "   IndexNum[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_numIndexes, gRepaireMB._numIndexes ) ;
      pMB->_numIndexes = gRepaireMB._numIndexes ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMPRESSTYPE )
   {
      dumpPrintf( "   CompressType[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_compressorType, gRepaireMB._compressorType ) ;
      pMB->_compressorType = gRepaireMB._compressorType ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOBS )
   {
      dumpPrintf( "   Lobs[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_totalLobs, gRepaireMB._totalLobs ) ;
      pMB->_totalLobs = gRepaireMB._totalLobs ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMMITFLAG )
   {
      dumpPrintf( "   CommitFlag[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_commitFlag, gRepaireMB._commitFlag ) ;
      pMB->_commitFlag = gRepaireMB._commitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_COMMITLSN )
   {
      dumpPrintf( "   CommitLSN[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_commitLSN, gRepaireMB._commitLSN ) ;
      pMB->_commitLSN = gRepaireMB._commitLSN ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDX_COMMITFLAG )
   {
      dumpPrintf( "   IdxCommitFlag[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_idxCommitFlag, gRepaireMB._idxCommitFlag ) ;
      pMB->_idxCommitFlag = gRepaireMB._idxCommitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_IDX_COMMITLSN )
   {
      dumpPrintf( "   IdxCommitLSN[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_idxCommitLSN, gRepaireMB._idxCommitLSN ) ;
      pMB->_idxCommitLSN = gRepaireMB._idxCommitLSN ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOB_COMMITFLAG )
   {
      dumpPrintf( "   LobCommitFlag[%u] ==> [%u]"OSS_NEWLINE,
                  pMB->_lobCommitFlag, gRepaireMB._lobCommitFlag ) ;
      pMB->_lobCommitFlag = gRepaireMB._lobCommitFlag ;
   }
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOB_COMMITLSN )
   {
      dumpPrintf( "   LobCommitLSN[%llu] ==> [%llu]"OSS_NEWLINE,
                  pMB->_lobCommitLSN, gRepaireMB._lobCommitLSN ) ;
      pMB->_lobCommitLSN = gRepaireMB._lobCommitLSN ;
   }

   INT64 written = 0 ;
   INT32 rc = ossSeekAndWriteN( &file, DMS_MME_OFFSET + id * sizeof( dmsMB ),
                                (const CHAR*)pMB, sizeof( dmsMB ), written ) ;
   if ( rc )
   {
      dumpPrintf( " *****Save collection to file failed: %d"OSS_NEWLINE,
                  rc ) ;
   }
   else
   {
      dumpPrintf( " Save collection info to file succeed"OSS_NEWLINE ) ;
   }
}

void repaireCollections( OSSFILE &file )
{
   INT32 rc = SDB_OK ;
   dmsMB *pMB = NULL ;
   CHAR collectionName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

   if ( FALSE == gInitMME )
   {
      SINT64 lenRead = 0 ;
      rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                            DMS_MME_SZ, &lenRead ) ;
      if ( rc || lenRead != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                      "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

   for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
   {
      pMB = ( dmsMB* )( gMMEBuff + i * sizeof( dmsMB ) ) ;
      ossStrncpy( collectionName, pMB->_collectionName,
                  DMS_COLLECTION_NAME_SZ ) ;
      if ( 0 == ossStrcmp( gCLName, collectionName ) )
      {
         repaireCollection( file, pMB, i ) ;
         goto done ;
      }
   }

   dumpPrintf( "Not found collection[%s] in space[%s]"OSS_NEWLINE,
               gCLName, gCSName ) ;

done:
   return ;
error:
   goto done ;
}

void dumpCollections ( OSSFILE &file, UINT32 pageSize, CHAR *pSmeBuffer)
{
   INT32 rc = SDB_OK ;
   UINT32 len ;
   vector<UINT16> collections ;

   if ( FALSE == gInitMME )
   {
      SINT64 lenRead = 0 ;
      rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                            DMS_MME_SZ, &lenRead ) ;
      if ( rc || lenRead != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                      "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

retry :
   collections.clear() ;
   len = dmsDump::dumpMME ( gMMEBuff, DMS_MME_SZ,
                            gBuffer, gBufferSize, NULL,
                            DMS_SU_DMP_OPT_HEX |
                            DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                            DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                            gDumpType,
                            ossStrlen ( gCLName ) ? gCLName : NULL,
                            collections,
                            gForce ) ;
   if ( len >= gBufferSize-1 )
   {
      if ( reallocBuffer () )
      {
         clearBuffer () ;
         goto error ;
      }
      goto retry ;
   }

   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      flushOutput ( gBuffer, len ) ;
   }
   else if (SDB_INSPT_LOB == gCurInsptType)
   {    
        rc = loadLobMetaData(file, pageSize, pSmeBuffer);
        if ( rc ) 
        {
            goto error ;
        }

        /*if(gBalance) 
        {
            len = inspectBalance(gBuffer, gBufferSize);
            flushOutput(gBuffer,len);
        }*/         
        
        if (!gOnlyMeta)
        {
            UINT32 unused = 0;
            dumpHeader(gLobdFile, unused);
        }
    }

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
   SDB_INSPT_ACTION_REPARE
} ;

void inspectLob(OSSFILE &file, const CHAR*pFileName)
{
   INT32 totalErr         = 0 ;
   SINT32 hwm             = 0 ;
   CHAR     *inspectSMEBuffer = NULL;
   INT64 fileSize = 0;
   UINT32 rc = SDB_OK;

   rc = ossGetFileSizeByName(pFileName, &fileSize);
   if ( rc  != SDB_OK )
   {
      goto error ;
   }

   rc = inspectLobmHeader ( file, fileSize, totalErr ) ;
   if ( rc  != SDB_OK )
   {
      goto error ;
   }

   (void) inspectSME ( file, NULL, hwm, totalErr ) ;

   if ( ossStrlen ( gCLName ) == 0 && ( FALSE == gOnlyMeta ||
         OSS_BIT_TEST( gAction, ACTION_STAT ) ) )
   {
      inspectSMEBuffer = (CHAR*)SDB_OSS_MALLOC ( DMS_SME_SZ ) ;
      dmsSpaceManagementExtent *pSME = ( dmsSpaceManagementExtent*)inspectSMEBuffer ;
      if ( !inspectSMEBuffer )
      {
         dumpPrintf ( "Error: Failed to allocate %d bytes for Exp SME buffer"
         OSS_NEWLINE, (INT32)DMS_SME_SZ ) ;
         return;
      }
      for ( UINT32 i = 0; i < DMS_SME_SZ ; ++i )
      {
         pSME->freeBitMask( i ) ;
      }
   }

   if(!gOnlyMeta) 
   {
      (void)ossGetFileSizeByName(gLobdFileName.c_str(), &fileSize);
      rc = inspectLobdHeader(fileSize, totalErr);
      if ( rc  != SDB_OK )
      {
         goto error ;
      }
   }
   
   (void) inspectLobCollections ( file, gLobmPageSize, hwm, inspectSMEBuffer, totalErr ) ;

done:
   if ( 0 == totalErr )
   {
      dumpPrintf ( "Inspection collection space is Done without Error"
      OSS_NEWLINE ) ;
   }
   else
   {
      dumpPrintf ( "Inspection collection space is Done with %d Error(s)"
      OSS_NEWLINE, totalErr ) ;
   }
   SAFE_OSS_FREE ( inspectSMEBuffer ) ;
   return ;
error :
   goto done;
}

void inspectData(OSSFILE &file, const CHAR*pFileName)
{
    INT32 totalErr         = 0 ;
    SINT32 hwm             = 0 ;
    UINT32 csPageSize = 0;
    CHAR *inspectSMEBuffer = NULL;

    inspectHeader ( file, csPageSize, totalErr ) ;
    if ( csPageSize != DMS_PAGE_SIZE4K &&
       csPageSize != DMS_PAGE_SIZE8K &&
       csPageSize != DMS_PAGE_SIZE16K &&
       csPageSize != DMS_PAGE_SIZE32K &&
       csPageSize != DMS_PAGE_SIZE64K )
    {
     dumpPrintf ( "Error: %s page size is not valid: %d"OSS_NEWLINE,
                  pFileName, csPageSize ) ;
     return;
    }
    inspectSME ( file, NULL, hwm, totalErr ) ;

    if ( ossStrlen ( gCLName ) == 0 && ( FALSE == gOnlyMeta ||
       OSS_BIT_TEST( gAction, ACTION_STAT ) ) )
    {
     inspectSMEBuffer = (CHAR*)SDB_OSS_MALLOC ( DMS_SME_SZ ) ;
     dmsSpaceManagementExtent *pSME = ( dmsSpaceManagementExtent*)inspectSMEBuffer ;
     if ( !inspectSMEBuffer )
     {
        dumpPrintf ( "Error: Failed to allocate %d bytes for Exp SME buffer"
                     OSS_NEWLINE, (INT32)DMS_SME_SZ ) ;
        return;
     }
     for ( UINT32 i = 0; i < DMS_SME_SZ ; ++i )
     {
        pSME->freeBitMask( i ) ;
     }
    }

    inspectCollections ( file, csPageSize, hwm, inspectSMEBuffer,
                       totalErr ) ;
    if ( 0 == totalErr )
    {
     dumpPrintf ( "Inspection collection space is Done without Error"
                  OSS_NEWLINE ) ;
    }
    else
    {
     dumpPrintf ( "Inspection collection space is Done with %d Error(s)"
                  OSS_NEWLINE, totalErr ) ;
    }

    SDB_OSS_FREE ( inspectSMEBuffer ) ;

   return ;
}

void actionCSAttempt ( const CHAR *pFileName, vector<const CHAR *> &expectEyeVec,
                       BOOLEAN specific, SDB_INSPT_ACTION action )
{
   INT32    rc = SDB_OK ;
   OSSFILE  file ;
   BOOLEAN  isOpen = FALSE ;
   UINT32   csPageSize = 0 ;
   CHAR     eyeCatcher[DMS_HEADER_EYECATCHER_LEN+1] = {0} ;
   SINT64   readSize = 0 ;

   SINT64   restLen = 0 ;
   SINT64   readPos = 0 ;
   UINT32   iMode = OSS_DEFAULT | OSS_EXCLUSIVE ;
   BOOLEAN  csValid = FALSE ;

   if ( SDB_INSPT_ACTION_REPARE == action )
   {
      iMode |= OSS_READWRITE ;
   }
   else
   {
      iMode |= OSS_READONLY ;
   }

   rc = ossOpen ( pFileName, iMode, OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to open %s, rc = %d"OSS_NEWLINE,
                   pFileName, rc ) ;
      goto error ;
   }

   isOpen = TRUE ;

   restLen = DMS_HEADER_EYECATCHER_LEN ;
   while ( restLen > 0 )
   {
      rc = ossRead ( &file, eyeCatcher + readPos, restLen, &readSize ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         dumpPrintf ( "Error: Failed to read %s, rc = %d"OSS_NEWLINE,
                      pFileName, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= readSize ;
      readPos += readSize ;
   }

   for ( vector<const CHAR *>::iterator itr = expectEyeVec.begin();
         itr != expectEyeVec.end(); ++itr )
   {
      if ( 0 == ossStrncmp( eyeCatcher, *itr, DMS_HEADER_EYECATCHER_LEN ) )
      {
         csValid = TRUE ;
         break ;
      }
   }

   if ( !csValid )
   {
      if ( specific )
      {
         dumpPrintf ( "Error: %s is not a valid storage unit"OSS_NEWLINE,
                      pFileName ) ;
      }
      goto done ;
   }

   rc = ossSeek ( &file, 0, OSS_SEEK_SET ) ;
   if ( rc )
   {
      dumpPrintf ( "Error, Failed to seek to beginning of the file %s"
                   OSS_NEWLINE, pFileName ) ;
      goto error ;
   }

   switch ( action )
   {
   case SDB_INSPT_ACTION_REPARE :
   {
      if(gCurInsptType != SDB_INSPT_LOB) repaireCollections( file ) ;
      break ;
   }
   case SDB_INSPT_ACTION_DUMP :
      if ( gStartingPage < 0 )
      {
         dumpPrintf ( " Dump collection space %s"OSS_NEWLINE, pFileName ) ;
         dumpHeader ( file, csPageSize ) ;
         if ( csPageSize != DMS_PAGE_SIZE4K &&
              csPageSize != DMS_PAGE_SIZE8K &&
              csPageSize != DMS_PAGE_SIZE16K &&
              csPageSize != DMS_PAGE_SIZE32K &&
              csPageSize != DMS_PAGE_SIZE64K &&
              gCurInsptType != SDB_INSPT_LOB)
        {
            dumpPrintf ( "Error: %s page size is not valid: %d"OSS_NEWLINE,
                         pFileName, csPageSize ) ;
            goto error ;
        }

        CHAR *pSmeBuffer = (CHAR*)SDB_OSS_MALLOC ( DMS_SME_SZ ) ;
        if ( !pSmeBuffer )
         {
            dumpPrintf ( "Error: Failed to allocate %d bytes for SME buffer"
                         OSS_NEWLINE, (INT32)DMS_SME_SZ ) ;
            goto error ;
        }

        dumpSME ( file, pSmeBuffer ) ;
        
        dumpCollections ( file, csPageSize, pSmeBuffer ) ;
        dumpPrintf ( "Dump collection space is done"OSS_NEWLINE ) ;
        
        SAFE_OSS_FREE ( pSmeBuffer ) ;
      }
      else
      {
         dumpHeader ( file, csPageSize ) ;

         for ( SINT32 i = 0; i < gNumPages && !gReachEnd; ++i )
         {
            dumpPrintf ( " Dump page %d"OSS_NEWLINE, gStartingPage + i ) ;
            dumpRawPage ( file, csPageSize, gStartingPage + i ) ;
         }
         
         if (gCurInsptType == SDB_INSPT_LOB)
         {
            dumpHeader ( gLobdFile, csPageSize ) ;
            gDataOffset = DMS_SME_OFFSET;
            for ( SINT32 i = 0; i < gNumPages && !gReachEnd; ++i )
            {
               dumpPrintf ( " Dump page %d"OSS_NEWLINE, gStartingPage + i ) ;
               dumpRawPage ( gLobdFile, gLobdPageSize, gStartingPage + i ) ;
            }
         }

      }
      break ;
    case SDB_INSPT_ACTION_INSPECT :
    {
        dumpPrintf ( "Inspect collection space %s"OSS_NEWLINE, pFileName ) ;
        
        (gCurInsptType == SDB_INSPT_LOB) 
                ? inspectLob(file, pFileName) 
                : inspectData(file, pFileName);
        
        break;
    }
   default :
      dumpPrintf ( "Error: unexpected action"OSS_NEWLINE ) ;
      goto error ;
   }
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   if ( isOpen )
   {
      ossClose ( file ) ;
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
   BOOLEAN isOpen = FALSE ;
   INT32 rc = SDB_OK ;
   dmsStorageUnitHeader dataHeader ;
   SINT64 lenRead                      = 0 ;

   rc = ossOpen ( csFullName.c_str(), OSS_DEFAULT|OSS_READONLY|OSS_EXCLUSIVE,
                  OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to open %s, rc = %d"OSS_NEWLINE,
                   csFullName.c_str(), rc ) ;
      goto error ;
   }

   isOpen = TRUE ;

   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, (CHAR *)&dataHeader,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
   if ( 0 != ossStrncmp( dataHeader._eyeCatcher, DMS_DATASU_EYECATCHER,
                         DMS_HEADER_EYECATCHER_LEN )
        &&
        0 != ossStrncmp( dataHeader._eyeCatcher, DMS_DATACAPSU_EYECATCHER,
                         DMS_HEADER_EYECATCHER_LEN ) )
   {
      dumpPrintf ( "Error: File[%s] is not dms storage unit data file"
                   OSS_NEWLINE, csFullName.c_str() ) ;
      goto error ;
   }

   gPageSize = dataHeader._pageSize ;
   gSecretValue = dataHeader._secretValue ;
   gLobdPageSize = dataHeader._lobdPageSize;
   gSequence = dataHeader._sequence;
   gExistLobs = dataHeader._createLobs;
   
   rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                         DMS_MME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_MME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
   gInitMME = TRUE ;

done:
   if ( isOpen )
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

   string csFileName ;
   string csFullName ;

   gPageSize      = 0 ;
   gSecretValue   = 0 ;
   gInitMME       = FALSE ;
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   gCl2PageMap.clear();

   if ( SDB_OK != prepareForDump( csName, sequence ) )
   {
      return ;
   }

   if ( gDumpData ||
        SDB_INSPT_ACTION_REPARE == action )
   {
      csFileName = rtnMakeSUFileName( csName, sequence,
                                      DMS_DATA_SU_EXT_NAME ) ;
      csFullName = rtnFullPathName( gDatabasePath, csFileName ) ;
      gDataOffset = DMS_MME_OFFSET + DMS_MME_SZ ;
      gPageNum    = 0 ;
      gCurInsptType = SDB_INSPT_DATA ;
      vector<const CHAR *> eyeCatcherVec ;
      eyeCatcherVec.push_back( DMS_DATASU_EYECATCHER ) ;
      eyeCatcherVec.push_back( DMS_DATACAPSU_EYECATCHER ) ;
      actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;
   }
   if ( gDumpIndex )
   {
      csFileName = rtnMakeSUFileName( csName, sequence,
                                      DMS_INDEX_SU_EXT_NAME ) ;
      csFullName = rtnFullPathName( gDatabasePath, csFileName ) ;
      gDataOffset = DMS_SME_OFFSET + DMS_SME_SZ ;
      gPageNum    = 0 ;
      gCurInsptType = SDB_INSPT_INDEX ;
      vector<const CHAR *> eyeCatcherVec ;
      eyeCatcherVec.push_back( DMS_INDEXSU_EYECATCHER ) ;
      actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;
   }
   if ( gDumpLob && gExistLobs )
   {
        INT32 rc = SDB_OK;
        if(!gOnlyMeta)
        {
           csFileName =  rtnMakeSUFileName( csName, sequence, DMS_LOB_DATA_SU_EXT_NAME ) ;
           gLobdFileName = rtnFullPathName( gLobPath, csFileName ) ;

           UINT32 iMode = ( SDB_INSPT_ACTION_REPARE == action ) ? OSS_READWRITE : OSS_READONLY;
           
           rc = ossOpen (gLobdFileName.c_str(), iMode, OSS_RU | OSS_WU | OSS_RG,   gLobdFile);
           if ( rc != SDB_OK)
           {
              dumpPrintf ( "Error: Failed to open %s, rc = %d"OSS_NEWLINE, gLobdFileName.c_str(), rc ) ;
              return;
           }
        }
                
        csFileName = rtnMakeSUFileName( csName, sequence, DMS_LOB_META_SU_EXT_NAME ) ;
        csFullName = rtnFullPathName( gLobmPath, csFileName ) ;
        gDataOffset = DMS_BME_OFFSET + DMS_BME_SZ;
        gPageNum    = 0 ;
        gCurInsptType = SDB_INSPT_LOB ;
        vector<const CHAR *> eyeCatcherVec ;
        eyeCatcherVec.push_back( DMS_LOBM_EYECATCHER) ;
        actionCSAttempt( csFullName.c_str(), eyeCatcherVec, specific, action ) ;

        if(!gOnlyMeta)
        {
            rc = ossClose (gLobdFile);
            if ( rc != SDB_OK)
            {
              dumpPrintf ( "Error: Failed to close %s, rc = %d"OSS_NEWLINE, gLobdFileName.c_str(), rc ) ;
            }
        }
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
      goto error ;
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
                             DMS_COLLECTION_SPACE_NAME_SZ, sequence ) )
            {
               if ( ossStrncmp ( gCSName, csName,
                                 DMS_COLLECTION_SPACE_NAME_SZ ) == 0 )
               {
                  actionCSAttemptEntry ( csName, sequence, TRUE,
                                         SDB_INSPT_ACTION_DUMP ) ;
               }
            }
         }
      }
   }
   else
   {
      dumpPrintf ( "Error: dump path %s is not a valid directory"OSS_NEWLINE,
                   gDatabasePath ) ;
   }

done :
   return ;
error :
   goto done ;
}

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
                                DMS_COLLECTION_SPACE_NAME_SZ, sequence ) )
            {
               if ( ossStrlen ( gCSName ) == 0 ||
                    ossStrncmp ( gCSName, csName,
                                 DMS_COLLECTION_SPACE_NAME_SZ ) == 0 )
               {
                  actionCSAttemptEntry ( csName, sequence,
                                         ossStrlen ( gCSName ) != 0,
                                         action ) ;
               }
            }
         }
      }
   }
   else
   {
      dumpPrintf ( "Error: inspect path %s is not a valid directory"OSS_NEWLINE,
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

   if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
   {
      /* LZW compression, need to load the dictionary. */
      INSPECT_EXTENT_TYPE extentType = INSPECT_EXTENT_TYPE_DICT ;
      rc = loadExtent( file, extentType, pageSize, mb->_dictExtentID, id ) ;
      if ( rc )
      {
         dumpPrintf( "Error: Failed to load dictionary extent %d, rc = %d"
                     OSS_NEWLINE, mb->_dictExtentID, rc ) ;
         goto error ;
      }

      if ( pExpBuffer )
      {
         inspectDictPageState( pExpBuffer, mb->_dictExtentID, err ) ;
      }

      if ( compressorEntry.getCompressor() &&
           UTIL_COMPRESSOR_LZW == type )
      {
         compressorEntry.setDictionary( gDictBuffer + DMS_DICTEXTENT_HEADER_SZ ) ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

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
         dumpPrintf ( "Error: Invalid arguments"OSS_NEWLINE ) ;
         displayArg ( desc ) ;
      }
      goto done ;
   }
   if (( OSS_BIT_TEST ( gAction, ACTION_DUMP ) 
            || OSS_BIT_TEST ( gAction, ACTION_INSPECT))
        && !gDumpData && !gDumpIndex && !gDumpLob )
   {
      dumpPrintf( "Error: should specific dump data, index or lob" ) ;
      goto done ;
   }
   if ( gStartingPage >= 0 && ((UINT32)( gDumpData + gDumpIndex + gDumpLob)  > 1 ) )  
   {
      dumpPrintf( "Error: dump from starting page only for the one of data, index or lob" ) ;
      goto done ;
   }

   gMMEBuff = (CHAR*)SDB_OSS_MALLOC( DMS_MME_SZ ) ;
   if ( !gMMEBuff )
   {
      dumpPrintf ( "Error: Failed to allocate mme buffer, exit"OSS_NEWLINE ) ;
      goto done ;
   }
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   gDictBuffer = (CHAR*)SDB_OSS_MALLOC( UTIL_MAX_DICT_TOTAL_SIZE ) ;
   if ( !gDictBuffer )
   {
      dumpPrintf( "Error: Failed to allocate dictionary buffer, "
                  "exit"OSS_NEWLINE ) ;
      goto done ;
   }
   ossMemset( gDictBuffer, 0, UTIL_MAX_DICT_TOTAL_SIZE ) ;

   rc = reallocBuffer () ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to realloc buffer, exit"OSS_NEWLINE ) ;
      goto done ;
   }
   cb = SDB_OSS_NEW pmdEDUCB ( NULL, EDU_TYPE_AGENT ) ;
   if ( !cb )
   {
      dumpPrintf ( "Failed to allocate memory for educb"OSS_NEWLINE ) ;
      goto done ;
   }
   if ( OSS_BIT_TEST ( gAction, ACTION_INSPECT ) ||
        OSS_BIT_TEST ( gAction, ACTION_STAT ) )
   {
      inspectDB( SDB_INSPT_ACTION_INSPECT ) ;
   }

   if ( OSS_BIT_TEST ( gAction, ACTION_REPAIRE ) )
   {
      inspectDB( SDB_INSPT_ACTION_REPARE ) ;
   }

   if ( OSS_BIT_TEST ( gAction, ACTION_DUMP ) )
   {
      if ( gStartingPage >= 0 )
      {
         dumpPages () ;
      }
      else
      {
         inspectDB( SDB_INSPT_ACTION_DUMP ) ;
      }
   }

   if ( 0 != ossStrlen( gCSName ) && !gHitCS )
   {
      dumpPrintf( "Warning: Cannot find any collection space "
                  "named %s"OSS_NEWLINE, gCSName ) ;
   }

done :
   if ( gBuffer )
   {
      SDB_OSS_FREE ( gBuffer ) ;
      gBuffer = NULL ;
   }
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
   if ( ossStrlen ( gOutputFile ) != 0 )
   {
      ossClose ( gFile ) ;
   }
   if ( cb )
   {
      SDB_OSS_DEL ( cb ) ;
   }
   return rc ;
}
