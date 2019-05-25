/*******************************************************************************
   Copyright (C) 2012-2014 SequoiaDB Ltd.

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

#include "class_timestamp.h"

#define TIMESTAMP_FORMAT "%d-%d-%d-%d.%d.%d.%d"
#define TIMESTAMP_FORMAT2 "%d-%d-%d-%d:%d:%d.%d"

extern INT32 timestampDesc ;

static void local_time ( time_t *Time, struct tm *TM )
{
   if ( !Time || !TM )
      return ;
#if defined (__linux__ ) || defined (_AIX)
   localtime_r( Time, TM ) ;
#elif defined (_WIN32)
   localtime_s( TM, Time ) ;
#else
#error "unimplemented local_time()"
#endif
}

PHP_METHOD( SequoiaTimestamp, __construct )
{
   INT32 rc = SDB_OK ;
   zval *pTimestamp  = NULL ;
   zval *pThisObj    = getThis() ;
   struct phpTimestamp *pDriverTimestamp = (struct phpTimestamp *)\
            emalloc( sizeof( struct phpTimestamp ) ) ;

   if( !pDriverTimestamp )
   {
      goto error ;
   }

   pDriverTimestamp->second = 0 ;
   pDriverTimestamp->micros = 0 ;

   if( PHP_GET_PARAMETERS( "|z", &pTimestamp ) == FAILURE )
   {
      goto error ;
   }

   if( pTimestamp )
   {
      if( Z_TYPE_P( pTimestamp ) == IS_LONG )
      {
         pDriverTimestamp->second = Z_LVAL_P( pTimestamp ) ;
         pDriverTimestamp->micros = 0 ;
      }
      else if( Z_TYPE_P( pTimestamp ) == IS_STRING )
      {
         INT32 numType = PHP_NOT_NUMBER ;
         INT32 numInt  = 0 ;
         INT64 numLong = 0 ;
         double numDouble = 0 ;

         php_parseNumber( Z_STRVAL_P( pTimestamp ),
                          Z_STRLEN_P( pTimestamp ),
                          &numType,
                          &numInt,
                          &numLong,
                          &numDouble TSRMLS_CC ) ;
         if( numType == PHP_NOT_NUMBER )
         {
            INT32 micros   = 0 ;
            time_t seconds = 0 ;

            if( php_date2Time( Z_STRVAL_P( pTimestamp ), 1,
                               &seconds, &micros ) == FALSE )
            {
               goto error ;
            }

            pDriverTimestamp->second = (INT32)seconds ;
            pDriverTimestamp->micros = micros ;
         }
         else
         {
            if( numType == PHP_IS_INT32 )
            {
               pDriverTimestamp->second = numInt ;
            }
            else if( numType == PHP_IS_INT64 )
            {
               pDriverTimestamp->second = (INT32)numLong ;
            }
            pDriverTimestamp->micros = 0 ;
         }
      }
   }
   else
   {
      pDriverTimestamp->second = (INT32)time( NULL ) ;
      pDriverTimestamp->micros = 0 ;
   }

done:
   PHP_SAVE_RESOURCE( pThisObj,
                      "$timestamp",
                      pDriverTimestamp,
                      timestampDesc ) ;
   return ;
error:
   pDriverTimestamp = NULL ;
   goto done ;
}

PHP_METHOD( SequoiaTimestamp, __toString )
{
   time_t second = 0 ;
   INT32 micros = 0 ;
   struct phpTimestamp *pDriverTimestamp = NULL ;
   zval *pThisObj = getThis() ;
   CHAR pDateStr[64] ;
   struct tm tmTime ;
   PHP_READ_RESOURCE( pThisObj,
                      "$timestamp",
                      pDriverTimestamp,
                      struct phpTimestamp*,
                      SDB_TIMESTAMP_HANDLE_NAME,
                      timestampDesc ) ;
   if( !pDriverTimestamp )
   {
      goto error ;
   }
   second = (time_t)pDriverTimestamp->second ;
   micros = pDriverTimestamp->micros ;
   ossMemset( pDateStr, 0, 64 ) ;
   local_time( &second, &tmTime ) ;
   ossSnprintf( pDateStr,
                64,
                "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                tmTime.tm_year + 1900,
                tmTime.tm_mon + 1,
                tmTime.tm_mday,
                tmTime.tm_hour,
                tmTime.tm_min,
                tmTime.tm_sec,
                micros ) ;
   PHP_RETVAL_STRING( pDateStr, 1 ) ;
done:
   return ;
error:
   RETVAL_EMPTY_STRING() ;
   goto done ;
}