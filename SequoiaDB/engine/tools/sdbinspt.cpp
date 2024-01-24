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
#define OPTION_HELPFULL   "helpfull"


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
       ( COMMANDS_STRING(OPTION_SHOW_CONTENT, ",p"), boost::program_options::value<string>(), "display data/index content (true/false)" ) \
       ( OPTION_ONLY_META, boost::program_options::value<string>(), "inspect only meta(Header, SME, MME) (true/false)" ) \
       ( OPTION_JUDGE_BALANCE, boost::program_options::value<string>(), "Enable lob bucket balance analysis (true/false). The default is false. It's effective when the action parameter is inspect" ) \
       ( OPTION_FORCE, "force dump all invalid mb, delete list and index list and so on" )

//hidden options
#define HIDDEN_OPTIONS \
       ( OPTION_HELPFULL,"help all configs") \
       ( COMMANDS_STRING(OPTION_REPAIRE, ",r"), boost::program_options::value<string>(), OPTION_REPAIRE_DESP )

// bitwise operation
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

// max size of a output file
#define MAX_FILE_SIZE 500 * 1024 * 1024
// increase delta max 64MB
#define BUFFER_INC_SIZE 67108864
// buffer init 4MB
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
     // other value
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


    // since we are single-threaded program, we define a lots of global variables :)
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

    //to support lob
    OSSFILE gLobdFile;
    string  gLobdFileName;
    BOOLEAN gDumpLob = FALSE;
    UINT32  gLobdPageSize = 0;
    UINT32  gSequence = 0;
    BOOLEAN gExistLobs = FALSE;

    //to support bucket balance judgement.
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
    UINT64  gSegmentSize                                 = 0 ;
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
         ossPrintf ( "Error: Failed to open output file: %s, rc = %d"
                     OSS_NEWLINE, newFile, rc ) ;
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
      ossPrintf( "Parse repaire value failed: %d" OSS_NEWLINE, rc ) ;
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
   else if( vm.count( OPTION_HELPFULL) )
   {
      displayArg ( all ) ;
      rc = SDB_PMD_HELP_ONLY ;
      goto done ;
   }
   // for dbpath, copy to gDatabasePath
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
      // use current directory for default
      ossStrncpy ( gDatabasePath, ".", sizeof(gDatabasePath) ) ;
   }

   // for index path copy to gIndexPath
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

   // check --output, if not provide then use stdout
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

            ossSnprintf( outputFile, OSS_MAX_PATHSIZE, "%s" DMS_DUMPFILE ".%d",
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
         // if we can't open the file, let's output to screen
         ossMemset ( gOutputFile, 0, sizeof(gOutputFile) ) ;
      }
   }

   // whether do verbose or raw output
   if ( vm.count ( OPTION_VERBOSE ) )
   {
      ossStrToBoolean ( vm[OPTION_VERBOSE].as<string>().c_str(),
                        &gVerbose ) ;
      if ( !gVerbose )
         gDumpType = 0 ;
   }

   // whether dump user data
   if ( vm.count ( OPTION_DUMPDATA ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPDATA].as<string>().c_str(),
                        &gDumpData ) ;
   }

   // whether dump index
   if ( vm.count ( OPTION_DUMPINDEX ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPINDEX].as<string>().c_str(),
                        &gDumpIndex ) ;
   }

   // whether dump lob
   if ( vm.count ( OPTION_DUMPLOB ) )
   {
      ossStrToBoolean ( vm[OPTION_DUMPLOB].as<string>().c_str(),
                        &gDumpLob ) ;
   }

   // collection space name
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

   // collection name
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

   // by default we dump
   ossStrncpy ( actionString, ACTION_DUMP_STRING,
                sizeof(actionString) ) ;
   if ( vm.count ( OPTION_ACTION ) )
   {
      // inspect input
      const CHAR *action = vm[OPTION_ACTION].as<string>().c_str() ;
      if ( ossStrncasecmp ( action, ACTION_INSPECT_STRING,
           ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_INSPECT_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_INSPECT ;
      }
      // dump input
      else if ( ossStrncasecmp ( action, ACTION_DUMP_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_DUMP_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_DUMP ;
      }
      // stat input
      else if ( ossStrncasecmp( action, ACTION_STAT_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy( actionString, ACTION_STAT_STRING,
                     sizeof(actionString) ) ;
         gAction = ACTION_STAT ;
      }
      // all input
      else if ( ossStrncasecmp ( action, ACTION_ALL_STRING,
                ossStrlen(action) ) == 0 )
      {
         ossStrncpy ( actionString, ACTION_ALL_STRING,
                      sizeof(actionString) ) ;
         gAction = ACTION_INSPECT | ACTION_DUMP | ACTION_STAT ;
      }
      // if action options is not valid, let's display help
      else
      {
         dumpAndShowPrintf ( "Invalid Action Option: %s" OSS_NEWLINE,
                             action ) ;
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
   }
   else if ( !vm.count( OPTION_REPAIRE ) )
   {
      dumpAndShowPrintf ( "Action or repaire must be specified" OSS_NEWLINE ) ;
      // if no action specified, let's display help
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
                       ossStrlen(gOutputFile)?gOutputFile:"{stdout}" ) ;
   dumpAndShowPrintf ( "Verbose       : %s" OSS_NEWLINE,
                       gVerbose?"True":"False" ) ;
   dumpAndShowPrintf ( "CS Name       : %s" OSS_NEWLINE,
                       ossStrlen(gCSName)?gCSName:"{all}" ) ;
   dumpAndShowPrintf ( "CL Name       : %s" OSS_NEWLINE,
                       ossStrlen(gCLName)?gCLName:"{all}" ) ;
   dumpAndShowPrintf ( "Action        : %s" OSS_NEWLINE,
                       actionString ) ;
   dumpAndShowPrintf ( "Repaire       : %s" OSS_NEWLINE,
                       gRepairStr.c_str() ) ;
   dumpAndShowPrintf ( "Dump Options  :" OSS_NEWLINE ) ;
   dumpAndShowPrintf ( "   Dump Data  : %s" OSS_NEWLINE,
                       gDumpData?"True":"False" ) ;
   dumpAndShowPrintf ( "   Dump Index : %s" OSS_NEWLINE,
                       gDumpIndex?"True":"False" ) ;
   dumpAndShowPrintf ( "   Dump Lob   : %s" OSS_NEWLINE,
                       gDumpLob?"True":"False" ) ;
   dumpAndShowPrintf ( "   Start Page : %d" OSS_NEWLINE,
                       gStartingPage ) ;
   dumpAndShowPrintf ( "   Num Pages  : %d" OSS_NEWLINE,
                       gNumPages ) ;
   dumpAndShowPrintf ( "   Show record: %s" OSS_NEWLINE,
                       gShowRecordContent ? "True":"False") ;
   dumpAndShowPrintf ( "   Only Meta  : %s" OSS_NEWLINE,
                       gOnlyMeta ? "True":"False" ) ;
   dumpAndShowPrintf ( "   Balance    : %s" OSS_NEWLINE,
                       gBalance? "True":"False" ) ;
   dumpAndShowPrintf ( "   force      : %s" OSS_NEWLINE,
                       gForce ? "True":"False" ) ;
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
      ossPrintf ( "Error: Failed to write into file, rc = %d" OSS_NEWLINE,
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
         dumpPrintf ( "Error: Cannot allocate more than 2GB" OSS_NEWLINE ) ;
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
   // memory free by end of program
   gBuffer = (CHAR*)SDB_OSS_MALLOC ( gBufferSize ) ;
   if ( !gBuffer )
   {
      dumpPrintf ( "Error: Failed to allocate memory for %d bytes" OSS_NEWLINE,
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
void inspectHeader ( OSSFILE &file, UINT32 &pageSize, SINT32 &err )
{
   INT32 rc       = SDB_OK ;
   INT32 localErr = 0 ;
   UINT32 len     = 0 ;
   CHAR headerBuffer [ DMS_HEADER_SZ ] = {0};
   SINT64 lenRead = 0 ;
   UINT64 secretValue = 0 ;
   UINT32 segmentSize = 0 ;

   // seek to where header starts and read DMS_HEADER_SZ bytes
   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
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
      if ( reallocBuffer () )
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
      dumpPrintf ( "Error: Secret value[%llu] is not expected[%llu]" OSS_NEWLINE,
                    secretValue, gSecretValue ) ;
      ++err ;
   }
   if ( (UINT32)pageSize != gPageSize )
   {
      dumpPrintf ( "Error: Page size[%d] is not expected[%d]" OSS_NEWLINE,
                    pageSize, gPageSize ) ;
      ++err ;
   }
   if ( segmentSize != gSegmentSize )
   {
      dumpPrintf ( "Error: Segment size[%d] is not expected[%d]" OSS_NEWLINE,
                   segmentSize, gSegmentSize ) ;
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
   // seek to where header starts and read DMS_HEADER_SZ bytes
   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, headerBuffer,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
   // attempt to format, note if len is gBufferSize - 1, that means we write to
   // end of buffer, which represents the current buffer size is not sufficient,
   // then clearly we should attempt to realloc buffer and format again
retry :
   len = dmsDump::dumpHeader ( headerBuffer, DMS_HEADER_SZ,
                               gBuffer, gBufferSize, NULL,
                               DMS_SU_DMP_OPT_HEX |
                               DMS_SU_DMP_OPT_HEX_WITH_ASCII |
                               DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR |
                               gDumpType, pageSize, gPageNum ) ;
   if ( len >= gBufferSize - 1 )
   {
      // if len is same as buffer size, that means we run out of buffer memory
      if ( reallocBuffer () )
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

void inspectSME ( OSSFILE &file, const CHAR *pExpBuf, SINT32 &hwm, SINT32 &err )
{
   INT32 rc        = SDB_OK ;
   UINT32 len      = 0 ;
   CHAR *smeBuffer = NULL ;
   SINT64 lenRead  = 0 ;
   SINT32 localErr = 0 ;
   // free by end of function
   // since SME is too large to be held by stack, we do heap allocation
   smeBuffer = (CHAR*)SDB_OSS_MALLOC ( DMS_SME_SZ ) ;
   if ( !smeBuffer )
   {
      dumpPrintf ( "Error: Failed to allocate %d bytes for SME buffer"
                   OSS_NEWLINE, (INT32)DMS_SME_SZ ) ;

      goto error ;
   }
   // seek to SME
   rc = ossSeekAndRead ( &file, DMS_SME_OFFSET, smeBuffer,
                         DMS_SME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_SME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;

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
   // free by end of function
   // since SME is too large to be held by stack, we do heap allocation


   // seek to where SME starts
   rc = ossSeekAndRead ( &file, DMS_SME_OFFSET, pSmeBuffer,
                         DMS_SME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_SME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, rc = %d"
                   OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }
   // format it, if len == gBufferSize -1, that means buffer is not large enough
   // and we should attempt to realloc
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
   INT32 rc = SDB_OK ;
   SINT64 lenRead = 0 ;
   // calculate the starting offset of extent, and read extent head
   rc = ossSeekAndRead ( &file, gDataOffset + (SINT64)pageSize * extentID,
                         (CHAR*)&extentHead, DMS_EXTENT_METADATA_SZ,
                         &lenRead ) ;
   // make sure we successfully read something and it's expected size
   if ( rc || lenRead != DMS_EXTENT_METADATA_SZ )
   {
      dumpPrintf ( "Error: Failed to read extent head, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
      if ( !rc )
         rc = SDB_IO ;
   }
   return rc ;
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
   SINT64 lenRead ;
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
            dumpPrintf ( "Error: Failed to allocate extent buffer, rc = %d"
                         OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }
      buffer = gExtentBuffer ;
   }

   // seek to offset and read entire extent
   rc = ossSeekAndRead ( &file, gDataOffset + (SINT64)pageSize * extentID,
                         buffer, extentSize * pageSize,
                         &lenRead ) ;
   // output sanity check
   if ( rc || lenRead != extentSize * pageSize )
   {
      dumpPrintf ( "Error: Failed to read extent , read %lld bytes, "
                   "expect %d bytes, rc = %d" OSS_NEWLINE, lenRead,
                   extentSize * pageSize, rc ) ;
      // out of range, should jump out of loop to avoid printing valid log
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
         dumpPrintf ( "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                      extentHead._eyeCatcher[0],
                      extentHead._eyeCatcher[1] ) ;
         result = FALSE ;
      }
      // make sure the block size is greater than 0, and doesn't exceed max
      if ( extentHead._blockSize <= 0 ||
           extentHead._blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf ( "Error: Invalid block size: %d, pageSize: %d"
                      OSS_NEWLINE, extentHead._blockSize, pageSize ) ;
         result = FALSE ;
      }
      // make sure the collection id is what we want
      if ( extentHead._mbID != expID )
      {
         dumpPrintf ( "Error: Unexpected id: %d, expected %d" OSS_NEWLINE,
                      extentHead._mbID, expID ) ;
         result = FALSE ;
      }
      // make sure the version is not rediculous
      if ( extentHead._version > DMS_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "Error: Invalid version: %d, current %d" OSS_NEWLINE,
                      extentHead._version, DMS_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
      // first/last record offset must be sync
      if ( ( extentHead._firstRecordOffset != DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset == DMS_INVALID_OFFSET ) ||
           ( extentHead._firstRecordOffset == DMS_INVALID_OFFSET &&
             extentHead._lastRecordOffset != DMS_INVALID_OFFSET ) )
      {
         dumpPrintf ( "Error: Bad first/last offset: %d:%d" OSS_NEWLINE,
                      extentHead._firstRecordOffset,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      // first record doesn't go wild
      if ( extentHead._firstRecordOffset >=
            (UINT32)extentHead._blockSize * (UINT32)pageSize )
      {
         dumpPrintf ( "Error: Bad first record offset: %d" OSS_NEWLINE,
                      extentHead._firstRecordOffset ) ;
         result = FALSE ;
      }
      // last record doens't go wild
      if ( extentHead._lastRecordOffset >=
           (UINT32)extentHead._blockSize * (UINT32)pageSize )
      {
         dumpPrintf ( "Error: Bad last record offset: %d" OSS_NEWLINE,
                      extentHead._lastRecordOffset ) ;
         result = FALSE ;
      }
      // free space is making sense too
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
         dumpPrintf ( "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                      metaExt->_eyeCatcher[0],
                      metaExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      // make sure the block size is greater than 0, and doesn't exceed max
      if ( metaExt->_blockSize <= 0 ||
           metaExt->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf ( "Error: Invalid block size: %d, pageSize: %d"
                      OSS_NEWLINE, metaExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      // make sure the collection id is what we want
      if ( metaExt->_mbID != expID )
      {
         dumpPrintf ( "Error: Unexpected id: %d, expected %d" OSS_NEWLINE,
                      metaExt->_mbID, expID ) ;
         result = FALSE ;
      }
      // make sure the version is not rediculous
      if ( metaExt->_version > DMS_META_EXTENT_CURRENT_V )
      {
         dumpPrintf ( "Error: Invalid version: %d, current %d" OSS_NEWLINE,
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
         dumpPrintf( "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                     dictExt->_eyeCatcher[0],
                     dictExt->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( dictExt->_blockSize <= 0 ||
           dictExt->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf( "Error: Invalid block size: %d, pageSize: %d" OSS_NEWLINE,
                     dictExt->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( dictExt->_mbID != expID )
      {
         dumpPrintf( "Error: Unexpected id: %d, expected %d" OSS_NEWLINE,
                     dictExt->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( dictExt->_version > DMS_DICT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "Error: Invalid version: %d, current %d" OSS_NEWLINE,
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
         dumpPrintf( "Error: Invalid eye catcher: %c%c" OSS_NEWLINE,
                     extent->_eyeCatcher[0],
                     extent->_eyeCatcher[1] ) ;
         result = FALSE ;
      }
      if ( extent->_blockSize <= 0 ||
           extent->_blockSize * pageSize > gSegmentSize )
      {
         dumpPrintf( "Error: Invalid block size: %d, pageSize: %d" OSS_NEWLINE,
                     extent->_blockSize, pageSize ) ;
         result = FALSE ;
      }
      if ( extent->_mbID != expID )
      {
         dumpPrintf( "Error: Unexpected id: %d, expected %d" OSS_NEWLINE,
                     extent->_mbID, expID ) ;
         result = FALSE ;
      }
      if ( extent->_version > DMS_OPT_EXTENT_CURRENT_V )
      {
         dumpPrintf( "Error: Invalid version: %d, current %d" OSS_NEWLINE,
                     extent->_version, DMS_OPT_EXTENT_CURRENT_V ) ;
         result = FALSE ;
      }
   }
   else if ( INSPECT_EXTENT_TYPE_UNKNOWN == type )
   {
      if ( extentHead._eyeCatcher[0] == IXM_EXTENT_CB_EYECATCHER0 &&
                extentHead._eyeCatcher[1] == IXM_EXTENT_CB_EYECATCHER1 )
      {
         // this is index control block extent, let's go back to check again
         type = INSPECT_EXTENT_TYPE_INDEX_CB ;
         goto retry ;
      }
      else
      {
         // we don't know the type, let's get out of here
         dumpPrintf ( "Error: Unknown eye catcher: %c%c" OSS_NEWLINE,
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
      dumpPrintf ( "Error: Failed to get extent head, rc = %d" OSS_NEWLINE,
                   rc ) ;
      goto error ;
   }
   // sanity check
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

   // load full extent, block size is variable only for data extent
   // for Index and IndexCB extent, the size should always be 1
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

   // first let's see how many indexes we have
   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "Error: numIdx is out of range, max: %d" OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      ++err ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }
   // loop through all indexes
   for ( UINT16 i = 0 ; i < mb->_numIndexes ; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      // dump index cb extent
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
      // load index CB into memory
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID,
                        collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent, rc = %d" OSS_NEWLINE,
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
   dumpPrintf ( " Dump Index Def for Collection [%u]" OSS_NEWLINE,
                collectionID ) ;
   // first let's see how many indexes we have
   if ( mb->_numIndexes > DMS_COLLECTION_MAX_INDEX )
   {
      dumpPrintf ( "Error: numIdx is out of range, max: %d" OSS_NEWLINE,
                   DMS_COLLECTION_MAX_INDEX ) ;
      mb->_numIndexes = DMS_COLLECTION_MAX_INDEX ;
   }
   // loop through all indexes
   for ( UINT16 i = 0 ; i < mb->_numIndexes; ++i )
   {
      dmsExtentID indexCBExtentID = mb->_indexExtent[i] ;
      dmsExtentID indexRoot = DMS_INVALID_EXTENT ;
      // dump index cb extent
      dumpPrintf ( "    Index [ %u ] : 0x%08lx" OSS_NEWLINE,
                   i, indexCBExtentID ) ;
      // sanity check
      if ( indexCBExtentID == DMS_INVALID_EXTENT ||
           (UINT32)indexCBExtentID >= gPageNum )
      {
         dumpPrintf ( "Error: Index CB Extent ID is not valid: 0x%08lx (%d)"
                      OSS_NEWLINE, indexCBExtentID, indexCBExtentID ) ;
         continue ;
      }
      // load index CB into memory
      rc = loadExtent ( file, extentType, pageSize, indexCBExtentID,
                        collectionID ) ;
      if ( rc )
      {
         dumpPrintf ( "Error: Failed to load extent, rc = %d" OSS_NEWLINE,
                      rc ) ;
         continue ;
      }
retry :
      // dump index cb and get root, since index cb must be 1 page, so we put
      // pageSize as inSize
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
      // if the root doesn't exit, we should not do anything ( maybe it's a
      // newly created index without any data )
      indexRoots[i] = indexRoot ;
   }

done :
   return ;
error :
   goto done ;
}

void inspectCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id,
                            SINT32 hwm, CHAR *pExpBuffer, SINT32 &err,
                            UINT64 &ovfNum, UINT64 &compressedNum, UINT64 &deletingNum )
{
}

void inspectCollectionIndex( OSSFILE &file, UINT32 pageSize, UINT16 id,
                             SINT32 hwm, CHAR *pExpBuffer, SINT32 &err )
{
}

void inspectCollectionLob( OSSFILE &lobmFile, UINT32 pageSize,
                           UINT16 clId, SINT32 hwm,
                           CHAR *pExpBuffer, SINT32 &err,
                           UINT32 &pageCount, UINT32 &lobCount )
{
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
      UINT64 deletingNum = 0 ;
      inspectCollectionData( file, pageSize, id, hwm,
                             pExpBuffer, err, ovfNum,
                             compressedNum, deletingNum ) ;
      /// flush data info
      len = ossSnprintf( gBuffer, gBufferSize,
                         " ****The collection data info****" OSS_NEWLINE
                         "   Total Record           : %llu" OSS_NEWLINE
                         "   Total Data Pages       : %u" OSS_NEWLINE
                         "   Total Data Free Space  : %llu" OSS_NEWLINE
                         "   Total OVF Record       : %llu" OSS_NEWLINE
                         "   Total Compressed Record: %llu" OSS_NEWLINE
                         "   Total Deleting Record  : %llu" OSS_NEWLINE,
                         gMBStat._totalRecords.fetch(),
                         gMBStat._totalDataPages,
                         gMBStat._totalDataFreeSpace,
                         ovfNum,
                         compressedNum,
                         deletingNum ) ;
      len += ossSnprintf(gBuffer + len, gBufferSize - len, OSS_NEWLINE);
      flushOutput( gBuffer, len ) ;
   }
   else if ( SDB_INSPT_INDEX == gCurInsptType )
   {
      inspectCollectionIndex( file, pageSize, id, hwm, pExpBuffer, err ) ;
      /// flush index info
      len = ossSnprintf( gBuffer, gBufferSize,
                         " ****The collection index info****" OSS_NEWLINE
                         "   Total Index Pages      : %u" OSS_NEWLINE
                         "   Total Index Free Space : %llu" OSS_NEWLINE
                         "   Unique Index Number    : %u" OSS_NEWLINE
                         "   Global Index Number    : %u" OSS_NEWLINE,
                         gMBStat._totalIndexPages,
                         gMBStat._totalIndexFreeSpace,
                         gMBStat._uniqueIdxNum,
                         gMBStat._globIdxNum ) ;
      len += ossSnprintf(gBuffer + len, gBufferSize - len, OSS_NEWLINE);
      flushOutput( gBuffer, len ) ;
   }
   else if ( SDB_INSPT_LOB == gCurInsptType )
   {
      UINT32 lobPageCount = 0;
      UINT32 lobCount = 0;
      inspectCollectionLob( file, pageSize, id, hwm, pExpBuffer, err, lobPageCount, lobCount) ;
      /// flush index info

      len = ossSnprintf( gBuffer, gBufferSize,
                         "****The collection lob info****" OSS_NEWLINE
                         "   Collection ID            : %u" OSS_NEWLINE
                         "   Total Lob Pages          : %u" OSS_NEWLINE
                         "   Total Lobs               : %u" OSS_NEWLINE,
                         id,
                         lobPageCount,
                         lobCount);

      len += ossSnprintf(gBuffer + len, gBufferSize - len, OSS_NEWLINE);
      flushOutput( gBuffer, len ) ;
   }
}

void dumpCollectionData( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
}

void dumpCollectionIndex( OSSFILE &file, UINT32 pageSize, UINT16 id )
{
}

INT32 getBME(OSSFILE &lobmFile, CHAR *pBME)
{
    INT32 rc = SDB_OK ;
    SINT64 lenRead = 0 ;

    // calculate the starting offset of extent, and read extent head
    rc = ossSeekAndRead ( &lobmFile, DMS_BME_OFFSET,
                          pBME, DMS_BME_SZ, &lenRead ) ;

    if ( rc || lenRead != DMS_BME_SZ )
    {
       dumpPrintf ( "Error: Failed to read lobm buckets , read %lld bytes, "
                    "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
       if ( !rc )
          rc = SDB_IO ;
    }

    return rc ;
}

INT32 dumpCollectionLob( OSSFILE &lobmFile,  UINT32 pageSize, UINT16 id)
{
   return SDB_OK ;
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
      SINT64 lenRead = 0 ;
      rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                            DMS_MME_SZ, &lenRead ) ;
      if ( rc || lenRead != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                      "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
         ++err ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

retry :
   // attempt to inspect MME
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

    len += ossSnprintf(outBuf + len, outSize -len, "Inspect Lobd Balance:" OSS_NEWLINE);

    len += ossSnprintf(outBuf+ len, outSize -len,
                                 " Average Bucket Depth   :%.6f" OSS_NEWLINE,
                                 (float)sum/DMS_BUCKETS_NUM);

    len += ossSnprintf(outBuf+ len, outSize -len,
                                 " Max Bucket Depth       :%u" OSS_NEWLINE, maxDep);

    len += ossSnprintf(outBuf+ len, outSize -len,
                                 " Min Bucket Depth       :%u" OSS_NEWLINE,minDep);

    len += ossSnprintf(outBuf+ len, outSize -len,
                                 " Zero-Depth Number      :%u" OSS_NEWLINE, zeroDepCount);

    len += ossSnprintf(outBuf+ len, outSize -len,
                                 " Variance               :%.6f (%s)" OSS_NEWLINE,(float)variance, "Less is Better");

    len += ossSnprintf(outBuf+ len, outSize -len, OSS_NEWLINE);
    return len;
}


INT32 loadPages(OSSFILE &lobmFile, CHAR *pCache,
        UINT32 cacheSize, UINT32 pageSize, UINT32 &pageCount)
{
   INT32 rc = SDB_OK ;
   SINT64 lenRead = 0 ;

   // calculate the starting offset of extent, and read extent head
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
                 "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
      }
   }
   return rc ;
}

void inspectLobCollections ( OSSFILE &file, UINT32 pageSize, SINT32 hwm,
                             CHAR *pExpBuffer, SINT32 &err )
{
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
   if ( gRepaireMask & PMD_REPAIRE_MB_MASK_LOB_COMMITLSN )
   {
      dumpPrintf( "   LobCommitLSN[%llu] ==> [%llu]" OSS_NEWLINE,
                  pMB->_lobCommitLSN, gRepaireMB._lobCommitLSN ) ;
      pMB->_lobCommitLSN = gRepaireMB._lobCommitLSN ;
   }

   INT64 written = 0 ;
   INT32 rc = ossSeekAndWriteN( &file, DMS_MME_OFFSET + id * sizeof( dmsMB ),
                                (const CHAR*)pMB, sizeof( dmsMB ), written ) ;
   if ( rc )
   {
      dumpPrintf( " *****Save collection to file failed: %d" OSS_NEWLINE,
                  rc ) ;
   }
   else
   {
      dumpPrintf( " Save collection info to file succeed" OSS_NEWLINE ) ;
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
                      "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
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
         /// found
         goto done ;
      }
   }

   dumpPrintf( "Not found collection[%s] in space[%s]" OSS_NEWLINE,
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
                      "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
         goto error ;
      }
      gInitMME = TRUE ;
   }

retry :
   // attempt to dump to gBuffer, if gBuffer is not large enough, we have to
   // reallocBuffer and try again
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

   // only for data file
   if ( SDB_INSPT_DATA == gCurInsptType )
   {
      flushOutput ( gBuffer, len ) ;
   }
   else if (SDB_INSPT_LOB == gCurInsptType)
   {
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
   SDB_INSPT_ACTION_REPARE
} ;

void inspectLob(OSSFILE &file, const CHAR*pFileName)
{
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
      dumpPrintf ( "Error: %s page size is not valid: %d" OSS_NEWLINE,
                   pFileName, csPageSize ) ;
      return ;
   }
   if ( !DMS_IS_VALID_SEGMENT( gSegmentSize ) )
   {
      dumpPrintf( "Error: %s segment size is invalid: %d" OSS_NEWLINE,
                  pFileName, gSegmentSize ) ;
      return ;
   }
   inspectSME ( file, NULL, hwm, totalErr ) ;

   // allocate expected SME for global collectionspace inspect only
   if ( ossStrlen ( gCLName ) == 0 && ( FALSE == gOnlyMeta ||
        OSS_BIT_TEST( gAction, ACTION_STAT ) ) )
   {
      // allocate memory for expected SME
      // this buffer is used to store all allocated pages from collection
      // traversal, the result will be used to compare with real SME for
      // orphan/inconsistent pages
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
      dumpPrintf ( "Error: Failed to open %s, rc = %d" OSS_NEWLINE,
                   pFileName, rc ) ;
      goto error ;
   }

   isOpen = TRUE ;

   // first let's read 8 bytes in front of the file, and make sure it's our
   // storage unit file
   restLen = DMS_HEADER_EYECATCHER_LEN ;
   while ( restLen > 0 )
   {
      rc = ossRead ( &file, eyeCatcher + readPos, restLen, &readSize ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         dumpPrintf ( "Error: Failed to read %s, rc = %d" OSS_NEWLINE,
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

   // if it doens't match our eye catcher, we may or may not dump error
   if ( !csValid )
   {
      // if user specified the collection space, we should dump error if it's
      // not a valid SU file
      if ( specific )
      {
         dumpPrintf ( "Error: %s is not a valid storage unit" OSS_NEWLINE,
                      pFileName ) ;
      }
      goto done ;
   }

   // when we get here, that means it's our SU file and let's dump header, SME,
   // and all collections
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
      if( gCurInsptType != SDB_INSPT_LOB)
      {
         repaireCollections( file ) ;
      }
      break ;
   }
   case SDB_INSPT_ACTION_DUMP :
      if ( gStartingPage < 0 )
      {
         // entire collection space dump
         dumpPrintf ( " Dump collection space %s" OSS_NEWLINE, pFileName ) ;
         dumpHeader ( file, csPageSize ) ;
         // page size sanity check, should we need header sanity check function?
         // hmm.. maybe later :)
         if ( csPageSize != DMS_PAGE_SIZE4K &&
              csPageSize != DMS_PAGE_SIZE8K &&
              csPageSize != DMS_PAGE_SIZE16K &&
              csPageSize != DMS_PAGE_SIZE32K &&
              csPageSize != DMS_PAGE_SIZE64K &&
              gCurInsptType != SDB_INSPT_LOB)
        {
            dumpPrintf ( "Error: %s page size is not valid: %d" OSS_NEWLINE,
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
        dumpPrintf ( "Dump collection space is done" OSS_NEWLINE ) ;

        SAFE_OSS_FREE ( pSmeBuffer ) ;
      }
      else
      {
         dumpHeader ( file, csPageSize ) ;

         // specific pages dump
         for ( SINT32 i = 0; i < gNumPages && !gReachEnd; ++i )
         {
            dumpPrintf ( " Dump page %d" OSS_NEWLINE, gStartingPage + i ) ;
            dumpRawPage ( file, csPageSize, gStartingPage + i ) ;
         }

         if (gCurInsptType == SDB_INSPT_LOB)
         {
            dumpHeader ( gLobdFile, csPageSize ) ;
            // specific pages dump
            gDataOffset = DMS_SME_OFFSET;
            for ( SINT32 i = 0; i < gNumPages && !gReachEnd; ++i )
            {
               dumpPrintf ( " Dump page %d" OSS_NEWLINE, gStartingPage + i ) ;
               dumpRawPage ( gLobdFile, gLobdPageSize, gStartingPage + i ) ;
            }
         }

      }
      break ;
    case SDB_INSPT_ACTION_INSPECT :
    {
        dumpPrintf ( "Inspect collection space %s" OSS_NEWLINE, pFileName ) ;

        (gCurInsptType == SDB_INSPT_LOB)
                ? inspectLob(file, pFileName)
                : inspectData(file, pFileName);

        break;
    }
   default :
      dumpPrintf ( "Error: unexpected action" OSS_NEWLINE ) ;
      goto error ;
   }
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   // close input file
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
      dumpPrintf ( "Error: Failed to open %s, rc = %d" OSS_NEWLINE,
                   csFullName.c_str(), rc ) ;
      goto error ;
   }

   isOpen = TRUE ;

   // read header and check file type
   rc = ossSeekAndRead ( &file, DMS_HEADER_OFFSET, (CHAR *)&dataHeader,
                         DMS_HEADER_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_HEADER_SZ )
   {
      dumpPrintf ( "Error: Failed to read header, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
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
   rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                         DMS_MME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_MME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                   "rc = %d" OSS_NEWLINE, lenRead, rc ) ;
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

   // clear global info
   gPageSize      = 0 ;
   gSegmentSize   = 0 ;
   gSecretValue   = 0 ;
   gInitMME       = FALSE ;
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   gCl2PageMap.clear();

   // prepare
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
        ///TODO: multi sequence need use loop instead.
        INT32 rc = SDB_OK;
        if(!gOnlyMeta)
        {
           csFileName =  rtnMakeSUFileName( csName, sequence, DMS_LOB_DATA_SU_EXT_NAME ) ;
           gLobdFileName = rtnFullPathName( gLobPath, csFileName ) ;

           UINT32 iMode = ( SDB_INSPT_ACTION_REPARE == action ) ? OSS_READWRITE : OSS_READONLY;

           rc = ossOpen (gLobdFileName.c_str(), iMode, OSS_RU | OSS_WU | OSS_RG,   gLobdFile);
           if ( rc != SDB_OK)
           {
              dumpPrintf ( "Error: Failed to open %s, rc = %d" OSS_NEWLINE, gLobdFileName.c_str(), rc ) ;
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
              dumpPrintf ( "Error: Failed to close %s, rc = %d" OSS_NEWLINE, gLobdFileName.c_str(), rc ) ;
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
      // if we can't find the path, let's show error
      dumpPrintf ( "Error: dump path %s is not a valid directory" OSS_NEWLINE,
                   gDatabasePath ) ;
   }

done :
   return ;
error :
   goto done ;
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
      // if we can't find the path, let's show error
      dumpPrintf ( "Error: inspect path %s is not a valid directory" OSS_NEWLINE,
                   gDatabasePath ) ;
   }
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
         dumpPrintf ( "Error: Invalid arguments" OSS_NEWLINE ) ;
         displayArg ( desc ) ;
      }
      goto done ;
   }
   if (( OSS_BIT_TEST ( gAction, ACTION_DUMP ) ||
         OSS_BIT_TEST ( gAction, ACTION_INSPECT))
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

   // allocate mme buffer
   gMMEBuff = (CHAR*)SDB_OSS_MALLOC( DMS_MME_SZ ) ;
   if ( !gMMEBuff )
   {
      dumpPrintf ( "Error: Failed to allocate mme buffer, exit" OSS_NEWLINE ) ;
      goto done ;
   }
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   gDictBuffer = (CHAR*)SDB_OSS_MALLOC( UTIL_MAX_DICT_TOTAL_SIZE ) ;
   if ( !gDictBuffer )
   {
      dumpPrintf( "Error: Failed to allocate dictionary buffer, "
                  "exit" OSS_NEWLINE ) ;
      goto done ;
   }
   ossMemset( gDictBuffer, 0, UTIL_MAX_DICT_TOTAL_SIZE ) ;

   // allocate some buffer initially
   rc = reallocBuffer () ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to realloc buffer, exit" OSS_NEWLINE ) ;
      goto done ;
   }
   // allocate a fake EDUCB
   cb = SDB_OSS_NEW pmdEDUCB ( NULL, EDU_TYPE_AGENT ) ;
   if ( !cb )
   {
      dumpPrintf ( "Failed to allocate memory for educb" OSS_NEWLINE ) ;
      goto done ;
   }
   // are we doing database inspection?
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

   // are we doing database dump?
   if ( OSS_BIT_TEST ( gAction, ACTION_DUMP ) )
   {
      // dump specific pages?
      if ( gStartingPage >= 0 )
      {
         dumpPages () ;
      }
      else
      {
         // if we don't specify pages to dump, let's dump entire database
         inspectDB( SDB_INSPT_ACTION_DUMP ) ;
      }
   }

   if ( 0 != ossStrlen( gCSName ) && !gHitCS )
   {
      dumpPrintf( "Warning: Cannot find any collection space "
                  "named %s" OSS_NEWLINE, gCSName ) ;
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
   // close output file
   if ( ossStrlen ( gOutputFile ) != 0 )
   {
      ossClose ( gFile ) ;
   }
   // free cb
   if ( cb )
   {
      SDB_OSS_DEL ( cb ) ;
   }
   return rc ;
}
