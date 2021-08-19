/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

#include <time.h>
#include "ossUtil.h"
#include "cJSON_ext.h"
#include "bson/bson.h"
#include "oss.h"

static const CHAR* readBinary( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readRegex( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readTimestamp( const CHAR *pStr,
                                  const CJSON_MACHINE *pMachine,
                                  CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readDate( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readObjectId( const CHAR *pStr,
                                 const CJSON_MACHINE *pMachine,
                                 CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readMaxKey( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readMinKey( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readNumberLong( const CHAR *pStr,
                                   const CJSON_MACHINE *pMachine,
                                   CJSON_READ_INFO **ppReadInfo ) ;
static void _appendFunction( void ) ;

CHAR *_pEmptyString = "" ;

CHAR *_cJsonDollarCmdKey[] = {
   "$oid",
   "$timestamp",
   "$date",
   "$regex", "$options",
   "$binary", "$type",
   "$minKey",
   "$maxKey"
} ;

#define CJSON_STR_OID       _cJsonDollarCmdKey[0]
#define CJSON_STR_TIMESTAMP _cJsonDollarCmdKey[1]
#define CJSON_STR_DATE      _cJsonDollarCmdKey[2]
#define CJSON_STR_REGEX     _cJsonDollarCmdKey[3]
#define CJSON_STR_OPTIONS   _cJsonDollarCmdKey[4]
#define CJSON_STR_BINARY    _cJsonDollarCmdKey[5]
#define CJSON_STR_TYPE      _cJsonDollarCmdKey[6]
#define CJSON_STR_MINKEY    _cJsonDollarCmdKey[7]
#define CJSON_STR_MAXKEY    _cJsonDollarCmdKey[8]

#define CJSON_LEN_STR( str ) sizeof(str)-1,str
SDB_EXPORT void cJsonExtAppendFunction()
{
   static ossOnce initOnce = OSS_ONCE_INIT;

   ossOnceRun( &initOnce, _appendFunction ) ;
}

static void _appendFunction( void )
{
   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readBinary,
                      CJSON_LEN_STR( "BinData" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readMaxKey,
                      CJSON_LEN_STR( "MaxKey" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readMinKey,
                      CJSON_LEN_STR( "MinKey" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readNumberLong,
                      CJSON_LEN_STR( "NumberLong" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readObjectId,
                      CJSON_LEN_STR( "ObjectId" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readRegex,
                      CJSON_LEN_STR( "Regex" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readDate,
                      CJSON_LEN_STR( "SdbDate" ) ) ;

   cJsonExtendAppend( CJSON_MATCH_FUNC,
                      readTimestamp,
                      CJSON_LEN_STR( "Timestamp" ) ) ;

}

static const CHAR* readBinary( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum     = 0 ;
   INT32 binDataLen = 0 ;
   INT32 binType    = 0 ;
   CHAR *pBinData   = NULL ;
   CJSON *pDataItem = NULL ;
   CJSON *pTypeItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   CVALUE typeStruct ;

   pStr = parseParameters( pStr,
                           pMachine,
                           "sz",
                           &argNum,
                           &binDataLen,
                           &pBinData,
                           &typeStruct ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse Bindata argument" ) ;
      goto error ;
   }
   if( argNum > 2 )
   {
      CJSON_PRINTF_LOG( "Function Bindata no more than 2 argument" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pDataItem = cJsonItemCreate( pMachine ) ;
   if( pDataItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pTypeItem = cJsonItemCreate( pMachine ) ;
   if( pTypeItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   if( typeStruct.valType == CJSON_STRING )
   {
      binType = ossAtoi( typeStruct.pValStr ) ;
   }
   else if( typeStruct.valType == CJSON_INT32 )
   {
      binType = typeStruct.valInt ;
   }
   else
   {
      CJSON_PRINTF_LOG( "Failed to read BinData, the No.2 argument "
                        "must be string type or integer type" ) ;
      goto error ;
   }

   cJsonItemKey( pDataItem, CJSON_STR_BINARY ) ;
   cJsonItemKeyType( pDataItem, CJSON_BINARY ) ;
   cJsonItemValueString( pDataItem, pBinData, binDataLen ) ;

   cJsonItemKey( pTypeItem, CJSON_STR_TYPE ) ;
   cJsonItemKeyType( pTypeItem, CJSON_TYPE ) ;
   cJsonItemValueInt32( pTypeItem, binType ) ;

   cJsonItemLinkNext( pDataItem, pTypeItem ) ;

   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pDataItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pDataItem ) ;
   cJsonItemRelease( pTypeItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readRegex( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum     = 0 ;
   INT32 regexLen   = 0 ;
   INT32 optionsLen = 0 ;
   CHAR *pRegex     = NULL ;
   CHAR *pOptions   = NULL ;
   CJSON *pRegexItem   = NULL ;
   CJSON *pOptionsItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pStr = parseParameters( pStr,
                           pMachine,
                           "s|s",
                           &argNum,
                           &regexLen,
                           &pRegex,
                           &optionsLen,
                           &pOptions ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse Regex argument" ) ;
      goto error ;
   }
   if( argNum > 2 )
   {
      CJSON_PRINTF_LOG( "Function Regex no more than 2 argument" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pRegexItem = cJsonItemCreate( pMachine ) ;
   if( pRegexItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pOptionsItem = cJsonItemCreate( pMachine ) ;
   if( pOptionsItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }

   cJsonItemKey( pRegexItem, CJSON_STR_REGEX ) ;
   cJsonItemKeyType( pRegexItem, CJSON_REGEX ) ;
   cJsonItemValueString( pRegexItem, pRegex, regexLen ) ;

   cJsonItemKey( pOptionsItem, CJSON_STR_OPTIONS ) ;
   cJsonItemKeyType( pOptionsItem, CJSON_OPTIONS ) ;
   if( pOptions == NULL )
   {
      cJsonItemValueString( pOptionsItem, _pEmptyString, 0 ) ;
   }
   else
   {
      cJsonItemValueString( pOptionsItem, pOptions, optionsLen ) ;
   }

   cJsonItemLinkNext( pRegexItem, pOptionsItem ) ;

   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pRegexItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pRegexItem ) ;
   cJsonItemRelease( pOptionsItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static void getCurrentTime( time_t *pTimestamp, INT32 *pMicrotm )
{
#if defined (_LINUX) || defined (_AIX)
   struct timeval tv ;

   // obtain the current time, expressed as seconds and microseconds since
   // the Epoch
   if ( -1 == gettimeofday( &tv, NULL ) )
   {
       *pTimestamp = 0 ;
       *pMicrotm   = 0 ;
   }
   else
   {
       *pTimestamp = tv.tv_sec ;
       *pMicrotm   = tv.tv_usec ;
   }
#elif defined (_WINDOWS)
   FILETIME       fileTime ;
   ULARGE_INTEGER uLargeIntegerTime ;

   GetSystemTimeAsFileTime( &fileTime ) ;

   //convert FILETIME into ULARGER_INTEGER
   uLargeIntegerTime.LowPart  = fileTime.dwLowDateTime ;
   uLargeIntegerTime.HighPart = fileTime.dwHighDateTime ;

   // FILETIME contains a 64-bit value representing the number of
   // 100-nanosecond intervals since January 1, 1601 (UTC).
   uLargeIntegerTime.QuadPart -= ( DELTA_EPOCH_IN_MICROSECS * 10 ) ;

   // 1 FILETIME = 100 ns
   *pTimestamp = ( uLargeIntegerTime.QuadPart / OSS_TEN_MILLION ) ;
   *pMicrotm   = ( uLargeIntegerTime.QuadPart % OSS_TEN_MILLION ) / 10 ;
#endif
}
static void local_time( time_t *pTime, struct tm *pTm )
{
#if defined (_LINUX ) || defined (_AIX)
   localtime_r( pTime, pTm ) ;
#elif defined (_WINDOWS)
   // The Time represents the seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC. This value is usually obtained from the time
   // function.
   localtime_s( pTm, pTime ) ;
#endif
}
#define CJSON_TIMESTAMP_FORMAT "%04d-%02d-%02d-%02d.%02d.%02d.%06d"
#define CJSON_TIME_STRING_SIZE 64
#define CJSON_TIME_RELATIVE_YEAR 1900

static const CHAR* readTimestamp( const CHAR *pStr,
                                  const CJSON_MACHINE *pMachine,
                                  CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum      = 0 ;
   INT32 length      = 0 ;
   INT32 microtm     = 0 ;
   time_t timestamp  = 0 ;
   CHAR *pTimestampString     = NULL ;
   CJSON *pTimestampItem      = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   CVALUE arg1 ;
   CVALUE arg2 ;

   pStr = parseParameters( pStr, pMachine, "|zz", &argNum, &arg1, &arg2 ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse Timestamp argument" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pTimestampItem = cJsonItemCreate( pMachine ) ;
   if( pTimestampItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }

   cJsonItemKeyType( pTimestampItem, CJSON_TIMESTAMP ) ;
   cJsonItemKey( pTimestampItem, CJSON_STR_TIMESTAMP ) ;
   if( argNum == 0 )
   {
      struct tm psr ;
      pTimestampString = cJsonMalloc( CJSON_TIME_STRING_SIZE, pMachine ) ;
      if( pTimestampString == NULL )
      {
         goto error ;
      }
      ossMemset( pTimestampString, 0, CJSON_TIME_STRING_SIZE ) ;
      getCurrentTime( &timestamp, &microtm ) ;
      local_time( &timestamp, &psr ) ;
      length = ossSnprintf( pTimestampString,
                            CJSON_TIME_STRING_SIZE,
                            CJSON_TIMESTAMP_FORMAT,
                            psr.tm_year + CJSON_TIME_RELATIVE_YEAR,
                            psr.tm_mon + 1,
                            psr.tm_mday,
                            psr.tm_hour,
                            psr.tm_min,
                            psr.tm_sec,
                            microtm ) ;
      if( length < 0 )
      {
         CJSON_PRINTF_LOG( "Failed to formatting timestamp" ) ;
         goto error ;
      }
      cJsonItemValueString( pTimestampItem, pTimestampString, length ) ;
   }
   else if( argNum == 1 )
   {
      if( arg1.valType == CJSON_STRING )
      {
         cJsonItemValueString( pTimestampItem, arg1.pValStr, arg1.length ) ;
      }
      else
      {
         CJSON_PRINTF_LOG( "Failed to read Timestamp, the No.1 argument\
 must be string type" ) ;
         goto error ;
      }
   }
   else if( argNum == 2 )
   {
      if( arg1.valType == CJSON_INT32 && arg2.valType == CJSON_INT32 )
      {
         struct tm psr ;
         pTimestampString = cJsonMalloc( CJSON_TIME_STRING_SIZE, pMachine ) ;
         if( pTimestampString == NULL )
         {
            goto error ;
         }
         ossMemset( pTimestampString, 0, CJSON_TIME_STRING_SIZE ) ;
         timestamp = (time_t)arg1.valInt ;
         microtm   = arg2.valInt ;
         local_time( (time_t*)&timestamp, &psr ) ;
         length = ossSnprintf( pTimestampString,
                               CJSON_TIME_STRING_SIZE,
                               CJSON_TIMESTAMP_FORMAT,
                               psr.tm_year + CJSON_TIME_RELATIVE_YEAR,
                               psr.tm_mon + 1,
                               psr.tm_mday,
                               psr.tm_hour,
                               psr.tm_min,
                               psr.tm_sec,
                               microtm ) ;
         if( length < 0 )
         {
            CJSON_PRINTF_LOG( "Failed to formatting timestamp" ) ;
            goto error ;
         }
         cJsonItemValueString( pTimestampItem, pTimestampString, length ) ;
      }
      else
      {
         if( arg1.valType != CJSON_INT32 )
         {
            CJSON_PRINTF_LOG( "Failed to read Timestamp, the No.1\
 argument must be integer type" ) ;
            goto error ;
         }
         else if( arg2.valType != CJSON_INT32 )
         {
            CJSON_PRINTF_LOG( "Failed to read Timestamp, the No.2\
 argument must be integer type" ) ;
            goto error ;
         }
      }
   }
   else
   {
      CJSON_PRINTF_LOG( "Function Timestamp no more than 2 argument" ) ;
      goto error ;
   }
   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pTimestampItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonFree( pTimestampString, pMachine ) ;
   cJsonItemRelease( pTimestampItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

#define CJSON_DATE_FORMAT "%d-%d-%d"

static const CHAR* readDate( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum      = 0 ;
   INT32 microtm     = 0 ;
   time_t timestamp  = 0 ;
   CHAR *pDateString = NULL ;
   CJSON *pDateItem  = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   CVALUE arg ;

   pStr = parseParameters( pStr, pMachine, "|z", &argNum, &arg ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse SdbDate argument" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pDateItem = cJsonItemCreate( pMachine ) ;
   if( pDateItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }

   cJsonItemKeyType( pDateItem, CJSON_DATE ) ;
   cJsonItemKey( pDateItem, CJSON_STR_DATE ) ;
   if( argNum == 0 )
   {
      getCurrentTime( &timestamp, &microtm ) ;
      timestamp = timestamp * 1000 ;
      cJsonItemValueInt64( pDateItem, (INT64)timestamp ) ;
   }
   else if( argNum == 1 )
   {
      // try to convert the string to a number
      if( arg.valType == CJSON_STRING )
      {
         INT32 valInt      = 0 ;
         FLOAT64 valDouble = 0 ;
         INT64 valInt64    = 0 ;
         CJSON_VALUE_TYPE type = CJSON_NONE ;

         if( cJsonParseNumber( arg.pValStr,
                               arg.length,
                               &valInt,
                               &valDouble,
                               &valInt64,
                               &type ) == TRUE )
         {
            // SdbDate( "123456" )
            if( type == CJSON_INT32 )
            {
               cJsonItemValueInt32( pDateItem, valInt ) ;
            }
            else if( type == CJSON_INT64 )
            {
               cJsonItemValueInt64( pDateItem, valInt64 ) ;
            }
            else
            {
               CJSON_PRINTF_LOG( "Failed to read date, the '%.*s' "
                                 "is out of the range of date time",
                                 arg.length,
                                 arg.pValStr ) ;
               goto error ;

            }
         }
         else
         {
            // SdbDate( "YYYY-MM-DD" )
            cJsonItemValueString( pDateItem, arg.pValStr, arg.length ) ;
         }
      }
      else if( arg.valType == CJSON_INT32 || arg.valType == CJSON_INT64 )
      {
         if( arg.valType == CJSON_INT32 )
         {
            cJsonItemValueInt32( pDateItem, arg.valInt ) ;
         }
         else if( arg.valType == CJSON_INT64 )
         {
            cJsonItemValueInt64( pDateItem, arg.valInt64 ) ;
         }
      }
      else
      {
         CJSON_PRINTF_LOG( "Failed to read SdbDate, the No.1 argument\
 must be string type or integer type" ) ;
         goto error ;
      }
   }
   else
   {
      CJSON_PRINTF_LOG( "Function SdbDate no more than 1 argument" ) ;
      goto error ;
   }
   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pDateItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonFree( pDateString, pMachine ) ;
   cJsonItemRelease( pDateItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

#define CJSON_OID_STR_SIZE 25

static const CHAR* readObjectId( const CHAR *pStr,
                                 const CJSON_MACHINE *pMachine,
                                 CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum    = 0 ;
   INT32 oidLen    = 0 ;
   CHAR *pOid      = NULL ;
   CJSON *pOidItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pStr = parseParameters( pStr, pMachine, "|s", &argNum, &oidLen, &pOid ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse ObjectId argument" ) ;
      goto error ;
   }
   if( argNum == 1 )
   {
      if( oidLen <= 24 )
      {
         INT32 i = 0 ;
         CHAR c = 0 ;
         for( i = 0; i < oidLen; ++i )
         {
            c = *( pOid + i ) ;
            if( ( c < '0' || c > '9' ) &&
                ( c < 'a' || c > 'f' ) &&
                ( c < 'A' || c > 'F' ) )
            {
               CJSON_PRINTF_LOG( "Function ObjectId argument\
 must be a hex string" ) ;
               goto error ;
            }
         }
      }
      else
      {
         CJSON_PRINTF_LOG( "argument ObjectId must be a string\
 of length 24" ) ;
         goto error ;
      }
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pOidItem = cJsonItemCreate( pMachine ) ;
   if( pOidItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }

   cJsonItemKeyType( pOidItem, CJSON_OID ) ;
   cJsonItemKey( pOidItem, CJSON_STR_OID ) ;
   if( argNum == 0 )
   {
      CHAR *pOidString = NULL ;
      bson_oid_t oid ;
      pOidString = cJsonMalloc( CJSON_OID_STR_SIZE, pMachine ) ;
      if( pOidString == NULL )
      {
         goto error ;
      }
      ossMemset( pOidString, 0, CJSON_OID_STR_SIZE ) ;
      bson_oid_gen( &oid ) ;
      bson_oid_to_string( &oid, pOidString ) ;
      cJsonItemValueString( pOidItem, pOidString, CJSON_OID_STR_SIZE - 1 ) ;
   }
   else if( argNum == 1 )
   {
      cJsonItemValueString( pOidItem, pOid, oidLen ) ;
   }
   else
   {
      CJSON_PRINTF_LOG( "Function ObjectId no more than 1 argument" ) ;
      goto error ;
   }
   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pOidItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pOidItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readMaxKey( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum = 0 ;
   CJSON *pMaxKeyItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pStr = parseParameters( pStr, pMachine, "", &argNum ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse MaxKey argument" ) ;
      goto error ;
   }
   if( argNum > 0 )
   {
      CJSON_PRINTF_LOG( "Function MaxKey no arguments" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pMaxKeyItem = cJsonItemCreate( pMachine ) ;
   if( pMaxKeyItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   cJsonItemKeyType( pMaxKeyItem, CJSON_MAXKEY ) ;
   cJsonItemKey( pMaxKeyItem, CJSON_STR_MAXKEY ) ;
   cJsonItemValueInt32( pMaxKeyItem, 1 ) ;

   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pMaxKeyItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pMaxKeyItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readMinKey( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum = 0 ;
   CJSON *pMinKeyItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pStr = parseParameters( pStr, pMachine, "", &argNum ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse MinKey argument" ) ;
      goto error ;
   }
   if( argNum > 0 )
   {
      CJSON_PRINTF_LOG( "Function MinKey no arguments" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pMinKeyItem = cJsonItemCreate( pMachine ) ;
   if( pMinKeyItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   cJsonItemKeyType( pMinKeyItem, CJSON_MINKEY ) ;
   cJsonItemKey( pMinKeyItem, CJSON_STR_MINKEY ) ;
   cJsonItemValueInt32( pMinKeyItem, 1 ) ;

   cJsonReadInfoTypeCustom( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pMinKeyItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pMinKeyItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readNumberLong( const CHAR *pStr,
                                   const CJSON_MACHINE *pMachine,
                                   CJSON_READ_INFO **ppReadInfo )
{
   INT32 argNum = 0 ;
   INT64 numberLong = 0 ;
   CJSON *pLongItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   CVALUE arg ;

   pStr = parseParameters( pStr, pMachine, "z", &argNum, &arg ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse NumberLong argument" ) ;
      goto error ;
   }
   if( argNum > 1 )
   {
      CJSON_PRINTF_LOG( "Function NumberLong no more than 1 argument" ) ;
      goto error ;
   }
   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pLongItem = cJsonItemCreate( pMachine ) ;
   if( pLongItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   if( arg.valType == CJSON_INT32 )
   {
      numberLong = arg.valInt ;
   }
   else if( arg.valType == CJSON_INT64 )
   {
      numberLong = arg.valInt64 ;
   }
   else if( arg.valType == CJSON_STRING )
   {
      INT32 valInt = 0 ;
      FLOAT64 valDouble = 0 ;
      INT64 valInt64 = 0 ;
      CJSON_VALUE_TYPE type = CJSON_NONE ;

      if( cJsonParseNumber( arg.pValStr,
                            arg.length,
                            &valInt,
                            &valDouble,
                            &valInt64,
                            &type ) == FALSE )
      {
         CJSON_PRINTF_LOG( "Failed to read NumberLong, "
                           "the No.1 argument is an invalid number" ) ;
         goto error ;
      }
      if( type == CJSON_INT32 )
      {
         numberLong = valInt ;
      }
      else if( type == CJSON_INT64 )
      {
         numberLong = valInt64 ;
      }
      else
      {
         CJSON_PRINTF_LOG( "Failed to read NumberLong, the No.1 argument\
must be integer type or string type" ) ;
         goto error ;
      }
   }
   else
   {
      CJSON_PRINTF_LOG( "Failed to read NumberLong, the No.1 argument\
must be integer type or string type" ) ;
      goto error ;
   }
   cJsonItemValueInt64( pLongItem, numberLong ) ;

   cJsonReadInfoTypeInt64( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pLongItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pLongItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}
