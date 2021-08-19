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

   Source File Name = rawbson2csv.c

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/01/2014  ly  Initial Draft
          01/12/2016  hjw Add log and standard interface
   Last Changed =

*******************************************************************************/

#include "rawbson2csv.h"
#include "ossUtil.h"
#include "../client/bson/bson.h"
#include "common_decimal.h"
#include "../client/base64c.h"
#include "time.h"

#define TIME_FORMAT "%d-%d-%d-%d.%d.%d.%d"
#define DATE_FORMAT "%d-%d-%d"
#define INT32_LAST_YEAR 2038
#define RELATIVE_YEAR 1900
#define RELATIVE_MOD 12
#define RELATIVE_DAY 31
#define RELATIVE_HOUR 24
#define RELATIVE_MIN_SEC 60
#define CSV_STR_LEFTBRACKET    "("
#define CSV_STR_RIGHTBRACKET   ")"
#define CSV_STR_BACKSLASH      "/"

#define CSV_STR_NAN "NaN"
#define CSV_STR_NAN_LEN (sizeof(CSV_STR_NAN)-1)

#define CSV_STR_INFINITY "Infinity"
#define CSV_STR_INFINITY_LEN (sizeof(CSV_STR_INFINITY)-1)

#define CSV_STR_INFINITY2 "-"CSV_STR_INFINITY
#define CSV_STR_INFINITY2_LEN (sizeof(CSV_STR_INFINITY2)-1)

static CHAR _precision[20] = "%.16g" ;

static void local_time ( time_t *Time, struct tm *TM )
{
   if ( !Time || !TM )
      return ;
#if defined (__linux__ )
   localtime_r( Time, TM ) ;
#elif defined (_WIN32)
   // The Time represents the seconds elapsed since midnight (00:00:00),
   // January 1, 1970, UTC. This value is usually obtained from the time
   // function.
   localtime_s( TM, Time ) ;
#endif
}

typedef void (*UTIL_PLOG)( const CHAR *pFunc, \
                           const CHAR *pFile, \
                           UINT32 line, \
                           const CHAR *pFmt, \
                           ... ) ;

UTIL_PLOG _pPrintfLogFun = NULL ;

#define UTIL_RAW2BSON_PRINTF_LOG( fmt, ... )\
{\
   if( _pPrintfLogFun != NULL )\
   {\
      _pPrintfLogFun( __FUNC__, __FILE__, __LINE__, fmt, ##__VA_ARGS__ ) ;\
   }\
}

INT32 _appendString( const CHAR *delChar, INT32 delCharSize,
                     BOOLEAN isConvert, const CHAR *pBuffer, INT32 size,
                     CHAR **ppCSVBuf, INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;
   INT32 isDoubleChar = FALSE ;
   INT32 i = 0 ;
   INT32 movedSize = 1 ;

   for ( i = 0; i < size; )
   {
      movedSize = 1 ;
      if ( ppCSVBuf && (*pCSVSize) == 0 )
      {
         rc = SDB_SYS ;
         UTIL_RAW2BSON_PRINTF_LOG( "CSV Buffer is too small, rc=%d", rc ) ;
         goto error ;
      }
      if ( isDoubleChar )
      {
         if ( ppCSVBuf )
         {
            ossStrncpy( *ppCSVBuf, delChar, delCharSize ) ;
            movedSize = delCharSize ;
         }
         isDoubleChar = FALSE ;
      }
      else
      {
         if ( *(pBuffer + i) == delChar[0] &&
              0 == ossStrncmp( pBuffer + i, delChar, delCharSize ) )
         {
            if( isConvert == TRUE )
            {
               isDoubleChar = TRUE ;
               movedSize = delCharSize ;
            }
         }
         if ( ppCSVBuf )
         {
            ossStrncpy( *ppCSVBuf, pBuffer + i, movedSize ) ;
         }
         i += movedSize ;
      }
      if ( ppCSVBuf )
      {
         *ppCSVBuf += movedSize ;
         *pCSVSize -= movedSize ;
      }
      else
      {
         *pCSVSize += movedSize ;
      }
   }
   if( isDoubleChar )
   {
      if ( ppCSVBuf )
      {
         ossStrncpy( *ppCSVBuf, delChar, delCharSize ) ;
      }
      isDoubleChar = FALSE ;
      if ( ppCSVBuf )
      {
         *ppCSVBuf += delCharSize ;
         *pCSVSize -= delCharSize ;
      }
      else
      {
         *pCSVSize += delCharSize ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _appendObj( const CHAR *delChar, INT32 delCharSize, bson_iterator *pIt,
                  CHAR **ppCSVBuf, INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;
   INT32 size    = 0 ;
   INT32 objSize = 0 ;
   CHAR *pBuffer = NULL ;
   CHAR *pTempBuf = NULL ;

   size = bson_sprint_length_iterator ( pIt ) ;

   pBuffer = (CHAR *)SDB_OSS_MALLOC( size ) ;
   if ( !pBuffer )
   {
      rc = SDB_OOM ;
      UTIL_RAW2BSON_PRINTF_LOG( "Failed to malloc memory, size=%d, rc=%d",
                                size,
                                rc ) ;
      goto error ;
   }
   pTempBuf = pBuffer ;
   ossMemset( pTempBuf, 0, size ) ;

   objSize = size ;
   if( !bson_sprint_iterator ( &pTempBuf,
                               &size,
                               pIt, '"' ) )
   {
      rc = SDB_OOM ;
      UTIL_RAW2BSON_PRINTF_LOG( "Failed to call bson_sprint_iterator, rc=%d",
                                rc ) ;
      goto error ;
   }
   objSize -= size ;

   if ( ppCSVBuf && objSize > (*pCSVSize) )
   {
      rc = SDB_SYS ;
      UTIL_RAW2BSON_PRINTF_LOG( "Csv buffer is too small,\
 need %d, only %d, rc=%d.",
                                objSize,
                                (*pCSVSize),
                                rc ) ;
      goto error ;
   }

   rc = _appendString( delChar, delCharSize, TRUE, pBuffer, objSize,
                       ppCSVBuf, pCSVSize ) ;
   if ( rc )
   {
      UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d", rc ) ;
      goto error ;
   }
done:
   SAFE_OSS_FREE( pBuffer ) ;
   return rc ;
error:
   goto done ;
}

INT32 _appendNonString( const CHAR *delChar, bson_iterator *pIt,
                        CHAR **ppCSVBuf, INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;
   INT32 size = 0 ;

   size = bson_sprint_length_iterator( pIt ) ;

   if ( !ppCSVBuf )
   {
      (*pCSVSize) += ( size + 1 ) ;
      goto done ;
   }

   if ( size > (*pCSVSize) )
   {
      rc = SDB_SYS ;
      UTIL_RAW2BSON_PRINTF_LOG( "Csv buffer is too small, rc=%d", rc ) ;
      goto error ;
   }

   if ( !bson_sprint_iterator ( ppCSVBuf, pCSVSize,
                                pIt, '"' ) )
   {
      rc = SDB_OOM ;
      UTIL_RAW2BSON_PRINTF_LOG( "Failed to call bson_sprint_iterator, rc=%d",
                                rc ) ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _appendNonString2( const CHAR *pBuffer, INT32 size,
                         CHAR **ppCSVBuf, INT32 *pCSVSize )
{
   INT32 rc = SDB_OK ;

   if( !ppCSVBuf )
   {
      (*pCSVSize) += size ;
      goto done ;
   }

   if( size > (*pCSVSize) )
   {
      rc = SDB_SYS ;
      UTIL_RAW2BSON_PRINTF_LOG( "Csv buffer is too small, rc=%d", rc ) ;
      goto error ;
   }

   ossMemcpy( *ppCSVBuf, pBuffer, size ) ;
   (*pCSVSize) -= size ;
   *ppCSVBuf += size ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _appendValue( const CHAR *delChar, INT32 delCharSize, bson_iterator *pIt,
                    CHAR **ppBuffer, INT32 *pCSVSize,
                    BOOLEAN includeBinary,
                    BOOLEAN includeRegex,
                    BOOLEAN kickNull )
{
   INT32 rc = SDB_OK ;
   bson_type type    = bson_iterator_type( pIt ) ;
   INT32 tempSize    = 0 ;
   INT32 base64Size  = 0 ;
   INT32 binType     = 0 ;
   INT32 decimalSize = 0 ;
   FLOAT64 doubleNum = 0.0 ;
   const CHAR *pTemp = NULL ;
   CHAR *pBase64     = NULL ;
   CHAR *pDecimalStr = NULL ;
   CHAR temp[128]    = { 0 } ;
   bson_timestamp_t ts;
   time_t timer ;
   struct tm psr;
   bson_decimal decimal = SDB_DECIMAL_DEFAULT_VALUE ;

   if ( type == BSON_DOUBLE )
   {
      INT32 sign = 0 ;
      FLOAT64 tmpDouble = 0.0 ;
      CHAR doubleTmpBuf[512] = { 0 } ;

      doubleNum = bson_iterator_double( pIt ) ;
      if( bson_is_inf( doubleNum, &sign ) == FALSE )
      {
         tmpDouble = doubleNum;
         if ( doubleNum == tmpDouble )
         {
            tempSize = ossSnprintf ( doubleTmpBuf, 512, _precision, doubleNum ) ;
            rc = _appendString( delChar, delCharSize, TRUE, doubleTmpBuf, tempSize,
                                ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }

            if( ossStrchr( doubleTmpBuf, '.' ) == 0 &&
                ossStrchr( doubleTmpBuf, 'E' ) == 0 &&
                ossStrchr( doubleTmpBuf, 'e' ) == 0 &&
                ossStrchr( doubleTmpBuf, 'N' ) == 0 &&
                ossStrchr( doubleTmpBuf, 'n' ) == 0 )
            {
               rc = _appendString( delChar, delCharSize, TRUE, ".0", 2,
                                   ppBuffer, pCSVSize ) ;
               if ( rc )
               {
                  UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                            rc ) ;
                  goto error ;
               }
            }
         }
         else
         {
            rc = _appendString( delChar, delCharSize, TRUE, CSV_STR_NAN,
                                CSV_STR_NAN_LEN, ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         if( sign == 1 )
         {
            rc = _appendString( delChar, delCharSize, TRUE, CSV_STR_INFINITY,
                                CSV_STR_INFINITY_LEN, ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = _appendString( delChar, delCharSize, TRUE, CSV_STR_INFINITY2,
                                CSV_STR_INFINITY2_LEN, ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
      }
   }
   else if ( type == BSON_BOOL || type == BSON_NULL ||
             type == BSON_INT || type == BSON_LONG )
   {
      if( type != BSON_NULL || kickNull != TRUE )
      {
         rc = _appendNonString( delChar, pIt, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendNonString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
   }
   else if ( type == BSON_DECIMAL )
   {
      if( bson_iterator_decimal( pIt, &decimal ) == BSON_ERROR )
      {
         rc = SDB_SYS ;
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call bson_iterator_decimal" ) ;
         goto error ;
      }
      rc = sdb_decimal_to_str_get_len( &decimal, &decimalSize ) ;
      if ( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to get decimal size, rc=%d", rc ) ;
         goto error ;
      }
      pDecimalStr = (CHAR *)SDB_OSS_MALLOC( decimalSize ) ;
      if( pDecimalStr == NULL )
      {
         rc = SDB_OOM ;
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to malloc memory, size=%d, rc=%d",
                                   decimalSize,
                                   rc ) ;
         goto error ;
      }
      ossMemset( pDecimalStr, 0, decimalSize ) ;
      rc = sdb_decimal_to_str( &decimal, pDecimalStr, decimalSize ) ;
      if( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call sdb_decimal_to_str, rc=%d",
                                   rc ) ;
         goto error ;
      }
      rc = _appendNonString2( pDecimalStr,
                              ossStrlen( pDecimalStr ),
                              ppBuffer,
                              pCSVSize ) ;
      if ( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                   rc ) ;
         goto error ;
      }
   }
   else if( type == BSON_DATE )
   {
      timer = bson_iterator_date( pIt ) / 1000 ;
      local_time( &timer, &psr ) ;
      if( psr.tm_year + 1900 >= 0 &&
          psr.tm_year + 1900 <= 9999 )
      {
         rc = _appendString( delChar, delCharSize,
                             FALSE, delChar, delCharSize,
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d", rc ) ;
            goto error ;
         }
         tempSize = ossSnprintf ( temp, 64, "%04d-%02d-%02d",
                                  psr.tm_year + 1900,
                                  psr.tm_mon + 1,
                                  psr.tm_mday ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, temp, tempSize, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
         rc = _appendString( delChar, delCharSize,
                             FALSE, delChar, delCharSize,
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         CHAR dateNum[ 512 ] = {0} ;
         ossMemset( dateNum, 0, 512 ) ;
         tempSize = ossSnprintf( dateNum,
                                 512,
                                 "%lld",
                                 bson_iterator_date( pIt ) ) ;
         rc = _appendNonString2( dateNum,
                                 tempSize,
                                 ppBuffer,
                                 pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
   }
   else
   {
      rc = _appendString( delChar, delCharSize,
                          FALSE, delChar, delCharSize,
                          ppBuffer, pCSVSize ) ;
      if ( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d", rc ) ;
         goto error ;
      }
      if ( type == BSON_TIMESTAMP )
      {
         ts = bson_iterator_timestamp( pIt ) ;
         timer = (time_t)ts.t;
         local_time( &timer, &psr ) ;
         tempSize = ossSnprintf ( temp, 64,
                                  "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                                  psr.tm_year + 1900,
                                  psr.tm_mon + 1,
                                  psr.tm_mday,
                                  psr.tm_hour,
                                  psr.tm_min,
                                  psr.tm_sec,
                                  ts.i ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, temp, tempSize, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_UNDEFINED )
      {
         rc = _appendString( delChar, delCharSize,
                             TRUE, CSV_STR_UNDEFINED,
                             CSV_STR_UNDEFINED_SIZE,
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_MINKEY )
      {
         rc = _appendString( delChar, delCharSize,
                             TRUE, CSV_STR_MINKEY,
                             CSV_STR_MINKEY_SIZE, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_MAXKEY )
      {
         rc = _appendString( delChar, delCharSize,
                             TRUE, CSV_STR_MAXKEY,
                             CSV_STR_MAXKEY_SIZE, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_CODE )
      {
         pTemp = bson_iterator_code( pIt ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, pTemp, ossStrlen( pTemp ),
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_STRING || type == BSON_SYMBOL )
      {
         pTemp = bson_iterator_string( pIt ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, pTemp, ossStrlen( pTemp ),
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else if ( type == BSON_BINDATA )
      {
         if( TRUE == includeBinary )
         {
            rc = _appendString( delChar,
                                delCharSize,
                                TRUE,
                                CSV_STR_LEFTBRACKET,
                                1,
                                ppBuffer,
                                pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
            binType = (INT32)bson_iterator_bin_type( pIt ) ;
            tempSize = ossSnprintf ( temp, 64, "%d", (UINT8)binType ) ;
            rc = _appendString( delChar, delCharSize,
                                TRUE, temp, tempSize, ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
            rc = _appendString( delChar,
                                delCharSize,
                                TRUE,
                                CSV_STR_RIGHTBRACKET,
                                1,
                                ppBuffer,
                                pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
         pTemp = bson_iterator_bin_data( pIt ) ;
         tempSize = bson_iterator_bin_len ( pIt ) ;
         if( tempSize > 0 )
         {
            base64Size = getEnBase64Size ( tempSize ) ;
            //free before the function return.
            pBase64 = (CHAR *)SDB_OSS_MALLOC( base64Size ) ;
            if( NULL == pBase64 )
            {
               rc = SDB_OOM ;
               UTIL_RAW2BSON_PRINTF_LOG(
                        "Failed to malloc memory, size=%d, rc=%d",
                        base64Size,
                        rc ) ;
               goto error ;
            }
            ossMemset( pBase64, 0, base64Size ) ;
            if ( base64Encode( pTemp, tempSize, pBase64, base64Size ) < 0 )
            {
               rc = SDB_INVALIDARG ;
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call base64Encode, rc=%d",
                                         rc ) ;
               goto error ;
            }
            rc = _appendString( delChar, delCharSize,
                                TRUE, pBase64, base64Size - 1,
                                ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
      }
      else if ( type == BSON_REGEX )
      {
         if( TRUE == includeRegex )
         {
            rc = _appendString( delChar, delCharSize,
                                TRUE, CSV_STR_BACKSLASH, 1,
                                ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
         pTemp = bson_iterator_regex( pIt ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, pTemp, ossStrlen( pTemp ),
                             ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
         if( TRUE == includeRegex )
         {
            rc = _appendString( delChar, delCharSize,
                                TRUE, CSV_STR_BACKSLASH, 1,
                                ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
            pTemp = bson_iterator_regex_opts( pIt ) ;
            rc = _appendString( delChar, delCharSize,
                                TRUE, pTemp, ossStrlen( pTemp ),
                                ppBuffer, pCSVSize ) ;
            if ( rc )
            {
               UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                         rc ) ;
               goto error ;
            }
         }
      }
      else if ( type == BSON_OID )
      {
         bson_oid_to_string( bson_iterator_oid( pIt ), temp ) ;
         rc = _appendString( delChar, delCharSize,
                             TRUE, temp, 24, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = _appendObj( delChar, delCharSize, pIt, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendObj, rc=%d", rc ) ;
            goto error ;
         }
      }
      rc = _appendString( delChar, delCharSize,
                          FALSE, delChar, delCharSize,
                          ppBuffer, pCSVSize ) ;
      if ( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d", rc ) ;
         goto error ;
      }
   }
done:
   sdb_decimal_free( &decimal ) ;
   SAFE_OSS_FREE( pBase64 ) ;
   SAFE_OSS_FREE( pDecimalStr ) ;
   return rc ;
error:
   goto done ;
}

void setCsvPrecision( const CHAR *pFloatFmt )
{
   if( pFloatFmt != NULL )
   {
      INT32 length = ossStrlen( pFloatFmt ) ;
      length = length > 16 ? 16 : length ;
      ossStrncpy( _precision, pFloatFmt, length ) ;
      _precision[ length ] = 0 ;
   }
}

void setPrintfLog( void (*pFun)( const CHAR *pFunc,
                                 const CHAR *pFile,
                                 UINT32 line,
                                 const CHAR *pFmt,
                                 ... ) )
{
   _pPrintfLogFun = (UTIL_PLOG)pFun ;
}

INT32 getCSVSize ( const CHAR *delChar, const CHAR *delField, INT32 delFieldSize,
                   CHAR *pbson, INT32 *pCSVSize,
                   BOOLEAN includeBinary,
                   BOOLEAN includeRegex,
                   BOOLEAN kickNull )
{
   INT32 rc = SDB_OK ;

   rc = bson2csv( delChar, delField, delFieldSize,
                  pbson, NULL, pCSVSize,
                  includeBinary,
                  includeRegex,
                  kickNull ) ;
   if ( rc )
   {
      UTIL_RAW2BSON_PRINTF_LOG( "Failed to call bson2csv, rc=%d", rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 bson2csv( const CHAR *delChar, const CHAR *delField, INT32 delFieldSize,
                CHAR *pbson, CHAR **ppBuffer, INT32 *pCSVSize,
                BOOLEAN includeBinary,
                BOOLEAN includeRegex,
                BOOLEAN kickNull )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isFirst = TRUE ;
   bson_type fieldType ;
   bson_iterator it ;
   INT32 delCharSize = ossStrlen( delChar ) ;

   bson_iterator_from_buffer( &it, pbson ) ;


   while ( bson_iterator_next( &it ) )
   {
      fieldType = bson_iterator_type( &it ) ;
      //if BSON_EOO == fieldType ( which is 0 ),that means we hit end of object
      if ( BSON_EOO == fieldType )
      {
         break ;
      }
      // do NOT concat "," for first entrance
      if ( isFirst )
      {
         isFirst = FALSE ;
      }
      else
      {
         rc = _appendString( delChar, delCharSize, TRUE, delField,
                             delFieldSize, ppBuffer, pCSVSize ) ;
         if ( rc )
         {
            UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendString, rc=%d",
                                      rc ) ;
            goto error ;
         }
      }
      if ( BSON_UNDEFINED == fieldType )
      {
         continue ;
      }
      //then we check the data type
      rc = _appendValue( delChar, delCharSize, &it,
                         ppBuffer, pCSVSize,
                         includeBinary,
                         includeRegex,
                         kickNull ) ;
      if ( rc )
      {
         UTIL_RAW2BSON_PRINTF_LOG( "Failed to call appendValue, rc=%d", rc ) ;
         goto error ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}
