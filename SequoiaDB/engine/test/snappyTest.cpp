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

   Source File Name = snappyTest.cpp

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

#include "core.hpp"
#include "../bson/bson.h"
#include "../snappy/snappy.h"
#include "ossUtil.hpp"
#include "ossIO.hpp"
#include "ossMmap.hpp"
#include <vector>
#include <string>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

using namespace bson ;
using namespace snappy ;
using namespace std ;
namespace po = boost::program_options ;

#define STR_LEN_PER_LEVEL 100
#define STR_MAX_LEN       1024*1024
#define FIELD_NAME_LEN    128
#define FIELD_NAME_PREFIX "Field"
#define FIELD_NAME_PREFIX_LEN 5
#define TEMPFILE            "snappyTestFile"

#define OPTION_HELP         "help"
#define OPTION_LEVELS       "level"
#define OPTION_NUMDOUBLE    "numDouble"
#define OPTION_NUMSTRING    "numString"
#define OPTION_NUMOBJECT    "numObject"
#define OPTION_NUMARRAY     "numArray"
#define OPTION_NUMBINDATA   "numBinData"
#define OPTION_NUMOID       "numOID"
#define OPTION_NUMBOOL      "numBool"
#define OPTION_NUMDATE      "numDate"
#define OPTION_NUMINT       "numInt"
#define OPTION_NUMTIMESTAMP "numTimestamp"
#define OPTION_NUMLONG      "numLong"
#define OPTION_NUMRECORDS   "numRecords"
#define OPTION_COMPRESS     "compress"

#define ADD_PARAM_OPTIONS_BEGIN( desc )\
        desc.add_options()

#define ADD_PARAM_OPTIONS_END ;

#define COMMANDS_STRING( a, b ) (std::string(a) + std::string( b)).c_str()
#define COMMANDS_OPTIONS \
       ( COMMANDS_STRING(OPTION_HELP, ",h"),                          "help" )\
       ( COMMANDS_STRING(OPTION_LEVELS, ",l"), boost::program_options::value<int>(), "level of bson(default 1)") \
       ( COMMANDS_STRING(OPTION_NUMDOUBLE, ",d"), boost::program_options::value<int>(), "number of double elements") \
       ( COMMANDS_STRING(OPTION_NUMSTRING, ",s"), boost::program_options::value<int>(), "number of string elements" ) \
       ( COMMANDS_STRING(OPTION_NUMOBJECT, ",o"), boost::program_options::value<int>(), "number of object elements" ) \
       ( COMMANDS_STRING(OPTION_NUMARRAY, ",a"), boost::program_options::value<int>(), "number of array elements" ) \
       ( COMMANDS_STRING(OPTION_NUMBINDATA, ",b"), boost::program_options::value<int>(), "number of bindata elements" ) \
       ( COMMANDS_STRING(OPTION_NUMOID, ",c"), boost::program_options::value<int>(), "number of object id elements" ) \
       ( COMMANDS_STRING(OPTION_NUMBOOL, ",f"), boost::program_options::value<int>(), "number of boolean elements" ) \
       ( COMMANDS_STRING(OPTION_NUMDATE, ",e"), boost::program_options::value<int>(), "number of date elements" ) \
       ( COMMANDS_STRING(OPTION_NUMINT, ",i"), boost::program_options::value<int>(), "number of int elements" ) \
       ( COMMANDS_STRING(OPTION_NUMTIMESTAMP, ",t"), boost::program_options::value<int>(), "number of timestamp elements" ) \
       ( COMMANDS_STRING(OPTION_NUMLONG, ",n"),  boost::program_options::value<int>(), "number of long elements" ) \
       ( COMMANDS_STRING(OPTION_NUMRECORDS, ",r"), boost::program_options::value<int>(), "number of records" ) \
       ( COMMANDS_STRING(OPTION_COMPRESS, ",p"), boost::program_options::value<std::string>(), "compress the record or not(default no)" )

INT32 gNumDouble    = 1 ;
INT32 gNumString    = 1 ;
INT32 gNumObject    = 1 ;
INT32 gNumArray     = 1 ;
INT32 gNumBinData   = 1 ;
INT32 gNumOID       = 1 ;
INT32 gNumBool      = 1 ;
INT32 gNumDate      = 1 ;
INT32 gNumInt       = 1 ;
INT32 gNumTimestamp = 1 ;
INT32 gNumLong      = 1 ;
INT32 gLevels       = 1 ;
INT32 gNumRecords   = 10000 ;
BOOLEAN gCompress   = FALSE ;

BSONObj bsonGen ( INT32 level, INT32 &fieldID, BOOLEAN isArray ) ;
void strGen ( CHAR *pBuffer, INT32 strSize )
{
   INT32 i = 0 ;
   for ( i = 0; i < strSize; ++i )
   {
      pBuffer[i] = ossRand() % 26 + 'a' ;
   }
   pBuffer[i] = 0 ;
}

INT32 bsonAddField ( INT32 level, INT32 &fieldID, BSONType type,
                     BSONObjBuilder &ob, BOOLEAN isArray )
{
   INT32 rc = SDB_OK ;
   CHAR fieldBuffer[FIELD_NAME_LEN] = {0} ;
   if ( 0 == level )
      goto done ;
   if ( !isArray )
   {
      ossStrcpy ( fieldBuffer, FIELD_NAME_PREFIX ) ;
   }
   ossSnprintf ( &fieldBuffer[isArray?0:FIELD_NAME_PREFIX_LEN],
                 isArray?FIELD_NAME_LEN:(FIELD_NAME_LEN-FIELD_NAME_PREFIX_LEN),
                 "%010d",
                 fieldID ) ;
   try
   {
      switch ( type )
      {
      case NumberDouble :
      {
         FLOAT64 val = (FLOAT64)ossRand () ;
         ob.appendNumber ( fieldBuffer, (double)val ) ;
         break ;
      }
      case String :
      {
         INT32 strLen = STR_LEN_PER_LEVEL * level ;
         CHAR *pStrBuffer = (CHAR*)SDB_OSS_MALLOC ( STR_MAX_LEN ) ;
         if ( !pStrBuffer )
         {
            ossPrintf ( "Failed to allocate memory for %d bytes"OSS_NEWLINE,
                        STR_MAX_LEN ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         strLen = OSS_MIN ( strLen, STR_MAX_LEN-1 ) ;
         strGen ( pStrBuffer, strLen ) ;
         ob.append ( fieldBuffer, pStrBuffer ) ;
         SDB_OSS_FREE ( pStrBuffer ) ;
         break ;
      }
      case Object :
      {
         BSONObj obj ;
         if ( 1 >= level )
            goto done ;
         obj = bsonGen ( level - 1, fieldID, FALSE ) ;
         ob.append ( fieldBuffer, obj ) ;
         break ;
      }
      case Array :
      {
         BSONObj obj ;
         INT32 tempID = 0 ; // all array start from 0
         if ( 1 >= level )
            goto done ;
         obj = bsonGen ( level - 1, tempID, TRUE ) ;
         fieldID += tempID ;
         ob.appendArray ( fieldBuffer, obj ) ;
         break ;
      }
      case BinData :
      {
         INT32 strLen = STR_LEN_PER_LEVEL * level ;
         CHAR *pStrBuffer = (CHAR*)SDB_OSS_MALLOC ( STR_MAX_LEN ) ;
         if ( !pStrBuffer )
         {
            ossPrintf ( "Failed to allocate memory for %d bytes"OSS_NEWLINE,
                        STR_MAX_LEN ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         strLen = OSS_MIN ( strLen, STR_MAX_LEN-1 ) ;
         strGen ( pStrBuffer, strLen ) ;
         ob.appendBinData ( fieldBuffer, strLen, BinDataGeneral, pStrBuffer ) ;
         SDB_OSS_FREE ( pStrBuffer ) ;
         break ;
      }
      case jstOID :
      {
         ob.appendOID ( fieldBuffer, NULL, TRUE ) ;
         break ;
      }
      case Bool :
      {
         BOOLEAN b = ossRand() % 2 ;
         ob.appendBool ( fieldBuffer, b ) ;
         break ;
      }
      case Date :
      {
         Date_t date (time(NULL)*1000) ;
         ob.appendDate ( fieldBuffer, date ) ;
         break ;
      }
      case jstNULL :
      {
         ob.appendNull ( fieldBuffer ) ;
         break ;
      }
      case NumberInt :
      {
         INT32 val = (INT32)ossRand () ;
         ob.appendNumber ( fieldBuffer, (int)val ) ;
         break ;
      }
      case Timestamp :
      {
         ob.appendTimestamp ( fieldBuffer, time(NULL)*1000, 0 ) ;
         break ;
      }
      case NumberLong :
      {
         INT64 val = (INT64)ossRand () ;
         val |= 0x8000000000000000LL ;
         ob.appendNumber ( fieldBuffer, (long long)val ) ;
         break ;
      }
      default :
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ++fieldID ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

BSONObj bsonGen ( INT32 level, INT32 &fieldID, BOOLEAN isArray )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder ob ;
   BSONObj emptyObj ;
   vector<BSONType> fieldList ;
   for ( INT32 i = 0; i < gNumDouble; ++i )
      fieldList.push_back(NumberDouble) ;
   for ( INT32 i = 0; i < gNumString; ++i )
      fieldList.push_back(String) ;
   for ( INT32 i = 0; i < gNumObject; ++i )
      fieldList.push_back(Object) ;
   for ( INT32 i = 0; i < gNumArray; ++i )
      fieldList.push_back(Array) ;
   for ( INT32 i = 0; i < gNumBinData; ++i )
      fieldList.push_back(BinData) ;
   for ( INT32 i = 0; i < gNumOID; ++i )
      fieldList.push_back(jstOID) ;
   for ( INT32 i = 0; i < gNumBool; ++i )
      fieldList.push_back(Bool) ;
   for ( INT32 i = 0; i < gNumDate; ++i )
      fieldList.push_back(Date) ;
   for ( INT32 i = 0; i < gNumInt; ++i )
      fieldList.push_back(NumberInt) ;
   for ( INT32 i = 0; i < gNumTimestamp; ++i )
      fieldList.push_back(Timestamp) ;
   for ( INT32 i = 0; i < gNumLong; ++i )
      fieldList.push_back(NumberLong) ;
   while ( !fieldList.empty() )
   {
      INT32 pos = ossRand() % fieldList.size() ;
      vector<BSONType>::iterator it = fieldList.begin() ;
      BSONType type = fieldList[pos] ;
      rc = bsonAddField ( level, fieldID, type, ob, isArray ) ;
      if ( rc )
      {
         ossPrintf ( "Failed to add field for type %d"OSS_NEWLINE,
                     fieldList[pos] ) ;
         goto error ;
      }
      for ( INT32 i = 0; i < pos; ++i, ++it ) ;
      fieldList.erase(it) ;
   }
   return ob.obj() ;
error :
   return emptyObj ;
}

BSONObj bsonGen ( INT32 level, BOOLEAN isArray )
{
   INT32 fieldID = 0 ;
   return bsonGen ( level, fieldID, isArray ) ;
}

INT32 testUncompress ( BOOLEAN compress )
{
   INT32 rc = SDB_OK ;
   ossTick tk1, tk2 ;
   ossTickDelta diff ;
   UINT32 sec, microsec ;
   ossTickConversionFactor factor ;
   BSONObj obj ;
   INT32 loopRound = 0 ;
   CHAR *pUncompressedBuffer = NULL ;
   size_t uncompressedLength = 0 ;
   UINT64 fileSize = 0 ;
   ossMmapFile file ;
   CHAR *pAddress = NULL ;
   CHAR *pCurrent = NULL ;
   rc = file.open ( TEMPFILE, OSS_CREATE|OSS_READWRITE|OSS_EXCLUSIVE,
                    OSS_RU|OSS_WU ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to open file %s, rc = %d"OSS_NEWLINE, TEMPFILE, rc ) ;
      goto error ;
   }
   rc = file.size ( fileSize ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to get file size, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }
   SDB_ASSERT ( fileSize < 0xFFFFFFFF,
                "mmap doesn't support single segment greater than 4GB" ) ;
   ossPrintf ( "Uncompress from file ( size = %llu )"OSS_NEWLINE,
               fileSize ) ;
   rc = file.map ( 0, (UINT32)fileSize, (void**)&pAddress ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to map, rc = %d", rc ) ;
      goto error ;
   }
   pCurrent = pAddress ;
   if ( compress )
   {
      rc = GetUncompressedLength ( pCurrent+4, 5,
                                   &uncompressedLength ) ;
      if ( FALSE == rc )
      {
         ossPrintf ( "Failed to parse compressed buffer"OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      pUncompressedBuffer = (CHAR*)SDB_OSS_MALLOC(uncompressedLength*2 ) ;
      if ( !pUncompressedBuffer )
      {
         ossPrintf ( "Failed to allocate memory for %d bytes"OSS_NEWLINE,
                     (INT32)uncompressedLength * 2 ) ;
         rc = SDB_OOM ;
         goto error ;
      }
   }
   tk1.sample () ;
   while ( (UINT64)pCurrent - (UINT64)pAddress < fileSize )
   {
      if ( !compress )
      {
         BSONObj obj ( pCurrent ) ;
         obj.toString () ;
         pCurrent += obj.objsize() ;
      }
      else
      {
         size_t finalLength = 0 ;
         rc = GetUncompressedLength ( pCurrent+4, 5, &finalLength ) ;
         if ( FALSE == rc )
         {
            ossPrintf ( "Failed to parse compressed buffer"OSS_NEWLINE );
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         SDB_ASSERT ( finalLength <= uncompressedLength*2,
                      "buffer is not large enough" ) ;
         rc = RawUncompress ( pCurrent+4, *(INT32*)pCurrent,
                              pUncompressedBuffer ) ;
         if ( FALSE == rc )
         {
            ossPrintf ( "Failed to uncompress buffer at round %d"OSS_NEWLINE,
                        loopRound );
            rc = SDB_SYS ;
            goto error ;
         }
         {
            BSONObj obj ( pUncompressedBuffer ) ;
            obj.toString() ;
         }
         pCurrent += *(INT32*)pCurrent + 4 ;
      }
      ++loopRound ;
   }
   rc = SDB_OK ;
   tk2.sample () ;
   diff = tk2 - tk1 ;
   diff.convertToTime ( factor, sec, microsec ) ;
   ossPrintf ( "Read %d %s records, elapsed time : %d seconds %d microsec"
               OSS_NEWLINE, loopRound, compress?"compressed":"uncompressed",
               sec, microsec ) ;
done :
   file.close () ;
   return rc ;
error :
   goto done ;
}

INT32 testCompress ( INT32 bsonLevel, INT32 loopRound, BOOLEAN compress )
{
   INT32 rc = SDB_OK ;
   OSSFILE file ;
   INT64 fileWritten = 0 ;
   ossTick tk1, tk2 ;
   ossTickDelta diff ;
   UINT32 sec, microsec ;
   ossTickConversionFactor factor ;
   BSONObj obj ;
   CHAR *pCompressedBuffer = NULL ;
   size_t compressedLength = 0 ;
   INT64 fileSize = 0 ;
   INT32 recordSize = 0 ;
   ossDelete ( TEMPFILE ) ;
   rc = ossOpen ( TEMPFILE, OSS_CREATE|OSS_READWRITE|OSS_EXCLUSIVE,
                  OSS_RU|OSS_WU, file ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to open file %s, rc = %d"OSS_NEWLINE, TEMPFILE, rc ) ;
      goto error ;
   }

   tk1.sample () ;
   for ( INT32 i = 0; i < loopRound; ++i )
   {
      obj = bsonGen ( bsonLevel, FALSE ) ;
      if ( 0 == i )
      {
         ossPrintf ( "level: %d"OSS_NEWLINE"avgSize: %d"OSS_NEWLINE,
                     bsonLevel, obj.objsize() ) ;
         recordSize = obj.objsize() ;
      }
      if ( !compress )
      {
         rc = ossWrite ( &file, obj.objdata(), obj.objsize(), &fileWritten ) ;
         if ( rc )
         {
            ossPrintf ( "Failed to write into file %s, rc = %d"OSS_NEWLINE,
                        TEMPFILE, rc ) ;
            goto error ;
         }
         if ( obj.objsize() != fileWritten )
         {
            ossPrintf ( "Length written doesn't match obj size. "
                        "Expected %d, actually %lld",
                        obj.objsize(), fileWritten ) ;
            rc = SDB_IO ;
            goto error ;
         }
      }
      else
      {
         if ( 0 == i )
         {
            size_t maxSize = MaxCompressedLength ( obj.objsize() ) ;
            pCompressedBuffer = (CHAR*)SDB_OSS_MALLOC ( maxSize+4 ) ;
            if ( !pCompressedBuffer )
            {
               ossPrintf ( "Failed to allocate memory for %d bytes"
                           OSS_NEWLINE, (INT32)maxSize+4 ) ;
               rc = SDB_OOM ;
               goto error ;
            }
         }
         RawCompress ( obj.objdata(), obj.objsize(),
                       &pCompressedBuffer[4], &compressedLength ) ;
         *(INT32*)pCompressedBuffer = (INT32)compressedLength ;
         rc = ossWrite ( &file, pCompressedBuffer, compressedLength+4,
                         &fileWritten ) ;
         if ( rc )
         {
            ossPrintf ( "Failed to write into file %s, rc = %d"OSS_NEWLINE,
                        TEMPFILE, rc ) ;
            goto error ;
         }
         if ( compressedLength+4 != (size_t)fileWritten )
         {
            ossPrintf ( "Length written doesn't match obj size. "
                        "Expected %d, actually %lld"OSS_NEWLINE,
                        (INT32)compressedLength+4, fileWritten ) ;
            rc = SDB_IO ;
            goto error ;
         }
      }
   }
   tk2.sample () ;
   diff = tk2 - tk1 ;
   diff.convertToTime ( factor, sec, microsec ) ;
   ossPrintf ( "Write %d %s records, elapsed time : %d seconds %d microsec"
               OSS_NEWLINE, loopRound, compress?"compressed":"uncompressed",
               sec, microsec ) ;
   rc = ossGetFileSize ( &file, &fileSize ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to get file size, rc = %lld"OSS_NEWLINE,
                  fileSize ) ;
      goto error ;
   }
   ossPrintf ( "File size for %d records is %lld ( saved %%%lld )"OSS_NEWLINE,
               loopRound, fileSize,
               100-100*fileSize/(recordSize * loopRound ) ) ;
done :
   ossClose ( file ) ;
   if ( pCompressedBuffer )
   {
      SDB_OSS_FREE ( pCompressedBuffer ) ;
   }
   return rc ;
error :
   goto done ;
}

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
      ossPrintf ( "Unknown argument: %s", e.get_option_name ().c_str () ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   catch ( po::invalid_option_value &e )
   {
      ossPrintf ( "Invalid argument: %s", e.get_option_name ().c_str () ) ;
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

   if ( vm.count ( OPTION_LEVELS ) )
      gLevels = vm[OPTION_LEVELS].as<int>() ;
   if ( vm.count ( OPTION_NUMDOUBLE ) )
      gNumDouble = vm[OPTION_NUMDOUBLE].as<int>() ;
   if ( vm.count ( OPTION_NUMSTRING ) )
      gNumString = vm[OPTION_NUMSTRING].as<int>() ;
   if ( vm.count ( OPTION_NUMOBJECT ) )
      gNumObject = vm[OPTION_NUMOBJECT].as<int>() ;
   if ( vm.count ( OPTION_NUMARRAY ) )
      gNumArray = vm[OPTION_NUMARRAY].as<int>() ;
   if ( vm.count ( OPTION_NUMBINDATA ) )
      gNumBinData = vm[OPTION_NUMBINDATA].as<int>() ;
   if ( vm.count ( OPTION_NUMOID ) )
      gNumOID = vm[OPTION_NUMOID].as<int>() ;
   if ( vm.count ( OPTION_NUMBOOL ) )
      gNumBool = vm[OPTION_NUMBOOL].as<int>() ;
   if ( vm.count ( OPTION_NUMDATE ) )
      gNumDate = vm[OPTION_NUMDATE].as<int>() ;
   if ( vm.count ( OPTION_NUMINT ) )
      gNumInt = vm[OPTION_NUMINT].as<int>() ;
   if ( vm.count ( OPTION_NUMTIMESTAMP ) )
      gNumTimestamp = vm[OPTION_NUMTIMESTAMP].as<int>() ;
   if ( vm.count ( OPTION_NUMLONG ) )
      gNumLong = vm[OPTION_NUMLONG].as<int>() ;
   if ( vm.count ( OPTION_NUMRECORDS ) )
      gNumRecords = vm[OPTION_NUMRECORDS].as<int>() ;
   if ( vm.count ( OPTION_COMPRESS ) )
   {
      const CHAR *p = vm [ OPTION_COMPRESS ].as<std::string>().c_str() ;
      ossStrToBoolean ( p, &gCompress ) ;
   }
done :
   return rc ;
error :
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
      if ( SDB_PMD_HELP_ONLY != rc )
      {
         ossPrintf ( "Invalid argument"OSS_NEWLINE ) ;
         displayArg ( desc ) ;
      }
      goto done ;
   }
   rc = testCompress ( gLevels, gNumRecords, gCompress ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to test func %s compress, rc = %d"OSS_NEWLINE,
                  gCompress?"with":"without", rc ) ;
      goto error ;
   }
   rc = testUncompress ( gCompress ) ;
   if ( rc )
   {
      ossPrintf ( "Failed to test func %s compress, rc = %d"OSS_NEWLINE,
                  gCompress?"with":"without", rc ) ;
      goto error ;
   }
done :
   return rc==SDB_OK?0:1 ;
error :
   ossPrintf ( "error: rc = %d"OSS_NEWLINE,
               rc ) ;
   goto done ;
}
