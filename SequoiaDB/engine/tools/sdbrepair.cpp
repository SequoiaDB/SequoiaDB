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

#include "dmsStorageData.hpp"
#include "ossUtil.hpp"
#include "ossIO.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdEDU.hpp"
#include "ixmExtent.hpp"

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace std ;
using namespace engine ;
namespace po = boost::program_options;
namespace fs = boost::filesystem ;

#define BUFFERSIZE          256
#define OPTION_HELP         "help"
#define OPTION_DBPATH       "dbpath"
#define OPTION_CSNAME       "csname"
#define OPTION_CLNAME       "clname"
#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()

#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (string(a) +string( b)).c_str()
#define COMMANDS_OPTIONS \
       ( COMMANDS_STRING(OPTION_HELP, ",h"), "help" )\
       ( COMMANDS_STRING(OPTION_DBPATH, ",d"), boost::program_options::value<string>(), "database path" ) \
       ( COMMANDS_STRING(OPTION_CSNAME, ",c"), boost::program_options::value<string>(), "collection space name" ) \
       ( COMMANDS_STRING(OPTION_CLNAME, ",l"), boost::program_options::value<string>(), "collection name" ) \

CHAR    gDatabasePath [ OSS_MAX_PATHSIZE + 1 ]       = {0} ;
CHAR    gCSName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
CHAR    gCLName [ DMS_COLLECTION_NAME_SZ + 1 ]       = {0} ;
CHAR   *gMMEBuff                                     = NULL ;

#define dumpPrintf(x,...)                                               \
   do {                                                                        \
      ossPrintf ( x, ##__VA_ARGS__ ) ;                                         \
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

string sdbMakeSUFileName( const string &csName, UINT32 sequence,
                          const string &extName )
{
   CHAR tmp[10] = {0} ;

   ossSnprintf( tmp, 9, "%d", sequence ) ;
   string suFile = csName ;
   suFile += "." ;
   suFile += tmp ;
   suFile += "." ;
   suFile += extName ;

   return suFile ;
}

string sdbFullPathName( const string &path, const string &name )
{
   string fullPathName = path ;
   if ( path.length() > 0 &&
        0 != ossStrncmp(&(path.c_str())[path.length()-1], OSS_FILE_SEP, 1 ) )
   {
      fullPathName += OSS_FILE_SEP ;
   }
   fullPathName += name ;

   return fullPathName ;
}

BOOLEAN sdbVerifyCollectionSpaceFileName ( const CHAR *pFileName,
                                           CHAR *pSUName,
                                           UINT32 bufferSize,
                                           UINT32 &sequence,
                                           const CHAR *extFilter )
{
   BOOLEAN ret = FALSE ;
   UINT32 size = 0 ;
   const CHAR *pDot = ossStrchr ( pFileName, '.' ) ;
   const CHAR *pDotr = ossStrrchr ( pFileName, '.' ) ;

   if ( !pDot || !pDotr || pDotr - pDot <= 1 || pDot == pFileName ||
        *(pDotr + 1) == 0 || ossStrchr( pDot + 1, '.' ) != pDotr )
   {
      goto done ;
   }

   if ( extFilter && 0 != ossStrcmp( pDotr + 1, extFilter ) )
   {
      goto done ;
   }

   {
      sequence = 0 ;
      const CHAR *pSeqPos = pDot + 1 ;
      while ( pSeqPos < pDotr )
      {
         if ( *pSeqPos >= '0' && *pSeqPos <= '9' )
         {
            sequence = 10 * sequence + ( *pSeqPos - '0' ) ;
         }
         else
         {
            goto done ;
         }
         ++pSeqPos ;
      }
   }

   size = pDot - pFileName ;
   if ( size > bufferSize )
   {
      goto done ;
   }

   if ( pSUName )
   {
      ossStrncpy ( pSUName, pFileName, size ) ;
      pSUName[size] = 0 ;
   }

   ret = TRUE ;

done :
   return ret ;
}

INT32 resolveArgument ( po::options_description &desc, INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
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

   dumpPrintf( "Run Options   :"OSS_NEWLINE ) ;
   dumpPrintf ( "Database Path : %s"OSS_NEWLINE,
                gDatabasePath ) ;
   dumpPrintf ( "CS Name       : %s"OSS_NEWLINE,
                ossStrlen(gCSName)?gCSName:"{all}" ) ;
   dumpPrintf ( "CL Name       : %s"OSS_NEWLINE,
                ossStrlen(gCLName)?gCLName:"{all}" ) ;
   dumpPrintf ( OSS_NEWLINE ) ;

done :
   return rc ;
error :
   goto done ;
}

void inspectCollections ( OSSFILE &file )
{
   INT32 rc = SDB_OK ;
   dmsMB *mb = NULL ;
   BOOLEAN hasModify = FALSE ;

   SINT64 lenRead = 0 ;
   rc = ossSeekAndRead ( &file, DMS_MME_OFFSET, gMMEBuff,
                         DMS_MME_SZ, &lenRead ) ;
   if ( rc || lenRead != DMS_MME_SZ )
   {
      dumpPrintf ( "Error: Failed to read sme, read %lld bytes, "
                   "rc = %d"OSS_NEWLINE, lenRead, rc ) ;
      goto error ;
   }

   for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
   {
      mb = (dmsMB*)( (CHAR*)gMMEBuff + (i*DMS_MB_SIZE) ) ;

      if ( DMS_MB_FLAG_FREE != mb->_flag &&
           !OSS_BIT_TEST ( mb->_flag, DMS_MB_FLAG_DROPED ) )
      {
         if ( ossStrlen( gCLName ) != 0 &&
              ossStrcmp( mb->_collectionName, gCLName ) != 0 )
         {
            continue ;
         }

         UINT16 baseFlag = DMS_MB_BASE_FLAG(mb->_flag) ;
         UINT16 oprFlag  = DMS_MB_OPR_FLAG(mb->_flag) ;
         UINT16 phaseFlag = DMS_MB_PHASE_FLAG(mb->_flag) ;

         if ( DMS_MB_FLAG_USED != baseFlag || 0 != oprFlag || 0 != phaseFlag )
         {
            dumpPrintf( "Collection %s mb flag is %d(0x%x), set to %d(0x%x)",
                        mb->_collectionName, mb->_flag, mb->_flag,
                        DMS_MB_FLAG_USED, DMS_MB_FLAG_USED ) ;
            hasModify = TRUE ;
            mb->_flag = DMS_MB_FLAG_USED ;
         }

         if ( ossStrlen( gCLName ) != 0 )
         {
            break ;
         }
      }
   }

   if ( hasModify )
   {
      SINT64 lenWrite = 0 ;
      rc = ossSeekAndWrite( &file, DMS_MME_OFFSET, gMMEBuff, DMS_MME_SZ,
                            &lenWrite ) ;
      if ( rc || lenWrite != DMS_MME_SZ )
      {
         dumpPrintf ( "Error: Failed to write sme, write %lld bytes, "
                      "rc = %d"OSS_NEWLINE, lenWrite, rc ) ;
         goto error ;
      }
   }

done :
   return ;
error :
   goto done ;
}

void actionCSAttempt ( const CHAR *pFile, const CHAR *expectEye,
                       BOOLEAN specific )
{
   INT32    rc = SDB_OK ;
   OSSFILE  file ;
   BOOLEAN  isOpen = FALSE ;
   CHAR     eyeCatcher[DMS_HEADER_EYECATCHER_LEN+1] = {0} ;
   SINT64   readSize = 0 ;

   SINT64   restLen = 0 ;
   SINT64   readPos = 0 ;

   rc = ossOpen ( pFile, OSS_READWRITE , OSS_RU | OSS_WU | OSS_RG, file ) ;
   if ( rc )
   {
      dumpPrintf ( "Error: Failed to open %s, rc = %d"OSS_NEWLINE,
                   pFile, rc ) ;
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
                      pFile, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;
      restLen -= readSize ;
      readPos += readSize ;
   }

   if ( ossStrncmp ( eyeCatcher, expectEye, DMS_HEADER_EYECATCHER_LEN ) )
   {
      if ( specific )
      {
         dumpPrintf ( "Error: %s is not a valid storage unit"OSS_NEWLINE,
                      pFile ) ;
      }
      goto done ;
   }

   rc = ossSeek ( &file, 0, OSS_SEEK_SET ) ;
   if ( rc )
   {
      dumpPrintf ( "Error, Failed to seek to beginning of the file %s"
                   OSS_NEWLINE, pFile ) ;
      goto error ;
   }

   inspectCollections ( file ) ;
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

void actionCSAttemptEntry( const CHAR *csName, UINT32 sequence,
                           BOOLEAN specific )
{
   string csFileName ;
   string csFullName ;

   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   csFileName = sdbMakeSUFileName( csName, sequence,
                                   DMS_DATA_SU_EXT_NAME ) ;
   csFullName = sdbFullPathName( gDatabasePath, csFileName ) ;

   actionCSAttempt( csFullName.c_str(), DMS_DATASU_EYECATCHER,
                    specific ) ;

}

void repairDB ()
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
            if ( sdbVerifyCollectionSpaceFileName ( pFileName, csName,
                                DMS_COLLECTION_SPACE_NAME_SZ, sequence,
                                DMS_DATA_SU_EXT_NAME ) )
            {
               if ( ossStrlen ( gCSName ) == 0 ||
                    ossStrncmp ( gCSName, csName,
                                 DMS_COLLECTION_SPACE_NAME_SZ ) == 0 )
               {
                  actionCSAttemptEntry ( csName, sequence,
                                         ossStrlen ( gCSName ) != 0 ) ;
               }
            }
         }
      }
   }
   else
   {
      dumpPrintf ( "Error: db path %s is not a valid directory"OSS_NEWLINE,
                   gDatabasePath ) ;
   }
}

INT32 main ( INT32 argc, CHAR **argv )
{
   INT32 rc = SDB_OK ;
   po::options_description desc ( "Command options" ) ;
   init ( desc ) ;
   rc = resolveArgument ( desc, argc, argv ) ;
   if ( rc )
   {
      if ( SDB_PMD_HELP_ONLY != rc )
      {
         dumpPrintf ( "Error: Invalid arguments"OSS_NEWLINE ) ;
         displayArg ( desc ) ;
      }
      goto done ;
   }

   gMMEBuff = (CHAR*)SDB_OSS_MALLOC( DMS_MME_SZ ) ;
   if ( !gMMEBuff )
   {
      dumpPrintf ( "Error: Failed to allocate mme buffer, exit"OSS_NEWLINE ) ;
      goto done ;
   }
   ossMemset( gMMEBuff, 0, DMS_MME_SZ ) ;

   repairDB() ;

done :
   if ( gMMEBuff )
   {
      SDB_OSS_FREE ( gMMEBuff ) ;
      gMMEBuff = NULL ;
   }
   return rc ;
}

