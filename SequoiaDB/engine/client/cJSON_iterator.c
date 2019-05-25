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

#include <time.h>
#include "ossUtil.h"
#include "cJSON.h"

static CJSON_VALUE_TYPE getCustomType( const CJSON *pItem ) ;
static CJSON* getCustomVal( const CJSON *pItem,
                            CJSON_VALUE_TYPE type ) ;
static INT32 getIteratorSubNum( const CJSON *pItem ) ;
static void setValue( CJSON *pItem, CVALUE *pValue ) ;

SDB_EXPORT const cJson_iterator*
      cJsonIteratorInit( const CJSON_MACHINE *pMachine )
{
   cJson_iterator *pIter = NULL ;
   if( pMachine == NULL )
   {
      goto error ;
   }
   pIter = (cJson_iterator*)cJsonMalloc( sizeof( cJson_iterator ), pMachine ) ;
   if( pIter == NULL )
   {
      goto error ;
   }
   pIter->pItem = pMachine->pItem ;
   pIter->pMachine = pMachine ;
done:
   return pIter ;
error:
   goto done ;
}

SDB_EXPORT BOOLEAN cJsonIteratorMore( const cJson_iterator *pIter )
{
   if( pIter == NULL || pIter->pItem == NULL )
   {
      return FALSE ;
   }
   return TRUE ;
}

SDB_EXPORT void cJsonIteratorNext( const cJson_iterator *pIter )
{
   cJson_iterator *pIter2 = NULL ;
   if( pIter && pIter->pItem )
   {
      pIter2 = (cJson_iterator*)pIter ;
      pIter2->pItem = pIter2->pItem->pNext ;
   }
}

SDB_EXPORT const BOOLEAN cJsonIteratorMoreSub( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->pChild ? TRUE : FALSE ;
   }
   return FALSE ;
}

SDB_EXPORT const cJson_iterator* cJsonIteratorSub( const cJson_iterator *pIter )
{
   cJson_iterator *pIterSub = NULL ;
   if( pIter && pIter->pItem )
   {
      pIterSub = (cJson_iterator*)cJsonMalloc( sizeof( cJson_iterator ),
                                               pIter->pMachine ) ;
      if( pIterSub == NULL )
      {
         goto error ;
      }
      pIterSub->pMachine = pIter->pMachine ;
      pIterSub->pItem = pIter->pItem->pChild ;
   }
done:
   return pIterSub ;
error:
   goto done ;
}

SDB_EXPORT const CHAR* cJsonIteratorKey( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->pKey ;
   }
   return NULL ;
}

SDB_EXPORT CJSON_VALUE_TYPE cJsonIteratorType( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      if( pIter->pItem->keyType == CJSON_CUSTOM &&
          pIter->pItem->valType == CJSON_OBJECT )
      {
         return getCustomType( pIter->pItem->pChild ) ;
      }
      else
      {
         return pIter->pItem->valType ;
      }
   }
   return CJSON_NONE ;
}

SDB_EXPORT INT32 cJsonIteratorInt32( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->valInt ;
   }
   return 0 ;
}

SDB_EXPORT INT64 cJsonIteratorInt64( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->valInt64 ;
   }
   return 0 ;
}

SDB_EXPORT FLOAT64 cJsonIteratorDouble( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->valDouble ;
   }
   return 0 ;
}

SDB_EXPORT const CHAR *cJsonIteratorString( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return pIter->pItem->pValStr ;
   }
   return NULL ;
}

SDB_EXPORT BOOLEAN cJsonIteratorBoolean( const cJson_iterator *pIter )
{
   if( pIter && pIter->pItem )
   {
      return ( pIter->pItem->valType == CJSON_TRUE ) ;
   }
   return FALSE ;
}

SDB_EXPORT INT32 cJsonIteratorSubNum( const cJson_iterator *pIter )
{
   INT32 num = 0 ;
   if( pIter && pIter->pItem )
   {
      num = getIteratorSubNum( pIter->pItem->pChild ) ;
   }
   return num ;
}

SDB_EXPORT INT32 cJsonIteratorSubNum2( const CVALUE *pValue )
{
   INT32 num = 0 ;
   if( pValue && pValue->pChild )
   {
      num = getIteratorSubNum( pValue->pChild ) ;
   }
   return num ;
}

SDB_EXPORT void cJsonIteratorBinary( const cJson_iterator *pIter,
                                     CVALUE *pBinData,
                                     CVALUE *pBinType )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pBinItem  = getCustomVal( pIter->pItem->pChild, CJSON_BINARY ) ;
      CJSON *pTypeItem = getCustomVal( pIter->pItem->pChild, CJSON_TYPE ) ;
      setValue( pBinItem, pBinData ) ;
      setValue( pTypeItem, pBinType ) ;
   }
}

SDB_EXPORT void cJsonIteratorRegex( const cJson_iterator *pIter,
                                    CVALUE *pRegex,
                                    CVALUE *pOptions )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pRegexItem   = getCustomVal( pIter->pItem->pChild, CJSON_REGEX ) ;
      CJSON *pOptionsItem = getCustomVal( pIter->pItem->pChild,
                                          CJSON_OPTIONS ) ;
      setValue( pRegexItem, pRegex ) ;
      setValue( pOptionsItem, pOptions ) ;
   }
}

SDB_EXPORT void cJsonIteratorTimestamp( const cJson_iterator *pIter,
                                        CVALUE *pTimestamp )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pTimeItem = getCustomVal( pIter->pItem->pChild,
                                       CJSON_TIMESTAMP ) ;
      setValue( pTimeItem, pTimestamp ) ;
   }
}

SDB_EXPORT void cJsonIteratorDate( const cJson_iterator *pIter,
                                   CVALUE *pDate )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pTimeItem = getCustomVal( pIter->pItem->pChild, CJSON_DATE ) ;
      if( pTimeItem->valType == CJSON_OBJECT )
      {
         CJSON *pNumberLong = getCustomVal( pTimeItem->pChild,
                                            CJSON_NUMBER_LONG ) ;
         if( pNumberLong && pNumberLong->valType == CJSON_STRING )
         {
            pDate->valInt64 = ossAtoll( pNumberLong->pValStr ) ;
            pDate->valType  = CJSON_INT64 ;
         }
         else
         {
            setValue( pTimeItem, pDate ) ;
         }
      }
      else
      {
         setValue( pTimeItem, pDate ) ;
      }
   }
}

SDB_EXPORT void cJsonIteratorObjectId( const cJson_iterator *pIter,
                                       CVALUE *pOid )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pOidItem = getCustomVal( pIter->pItem->pChild, CJSON_OID ) ;
      setValue( pOidItem, pOid ) ;
   }
}

SDB_EXPORT void cJsonIteratorNumberLong( const cJson_iterator *pIter,
                                         CVALUE *pNumberLong )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pNumberLongItem = getCustomVal( pIter->pItem->pChild,
                                             CJSON_NUMBER_LONG ) ;
      setValue( pNumberLongItem, pNumberLong ) ;
   }
}

SDB_EXPORT void cJsonIteratorDecimal( const cJson_iterator *pIter,
                                      CVALUE *pDecimal,
                                      CVALUE *pPrecision )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pDecimalItem   = getCustomVal( pIter->pItem->pChild,
                                            CJSON_DECIMAL ) ;
      CJSON *pPrecisionItem = getCustomVal( pIter->pItem->pChild,
                                            CJSON_PRECISION ) ;
      setValue( pDecimalItem, pDecimal ) ;
      setValue( pPrecisionItem, pPrecision ) ;
   }
}

SDB_EXPORT void cJsonIteratorPrecision( const cJson_iterator *pIter,
                                        CVALUE *pPrecision,
                                        CVALUE *pScale )
{
   if( pIter && pIter->pItem )
   {
      CJSON *pPrecisionItem = getCustomVal( pIter->pItem->pChild,
                                            CJSON_PRECISION ) ;
      if( pPrecisionItem && pPrecisionItem->valType == CJSON_ARRAY )
      {
         if( getIteratorSubNum( pPrecisionItem->pChild ) == 2 )
         {
            setValue( pPrecisionItem->pChild, pPrecision ) ;
            setValue( pPrecisionItem->pChild->pNext, pScale ) ;
         }
      }
   }
}


static CJSON_VALUE_TYPE getCustomType( const CJSON *pItem )
{
   CJSON_VALUE_TYPE type = CJSON_NONE ;
   while( pItem )
   {
      if( pItem->keyType == CJSON_TIMESTAMP ||
          pItem->keyType == CJSON_DATE ||
          pItem->keyType == CJSON_REGEX ||
          pItem->keyType == CJSON_OID ||
          pItem->keyType == CJSON_BINARY ||
          pItem->keyType == CJSON_MINKEY ||
          pItem->keyType == CJSON_MAXKEY ||
          pItem->keyType == CJSON_UNDEFINED ||
          pItem->keyType == CJSON_NUMBER_LONG ||
          pItem->keyType == CJSON_DECIMAL )
      {
         type = pItem->keyType ;
         break ;
      }
      pItem = pItem->pNext ;
   }
   return type ;
}

static CJSON* getCustomVal( const CJSON *pItem,
                            CJSON_VALUE_TYPE type )
{
   CJSON *pValItem = NULL ;
   while( pItem )
   {
      if( pItem->keyType == type )
      {
         pValItem = (CJSON *)pItem ;
         break ;
      }
      pItem = pItem->pNext ;
   }
   return pValItem ;
}

static INT32 getIteratorSubNum( const CJSON *pItem )
{
   INT32 i = 0 ;
   while( pItem )
   {
      ++i ;
      pItem = pItem->pNext ;
   }
   return i ;
}

static void setValue( CJSON *pItem, CVALUE *pValue )
{
   if( pValue )
   {
      ossMemset( pValue, 0, sizeof( CVALUE ) ) ;
      pValue->valType = CJSON_NONE ;
   }
   if( pItem && pValue )
   {
      pValue->valType   = pItem->valType ;
      pValue->valInt    = pItem->valInt ;
      pValue->length    = pItem->length ;
      pValue->valDouble = pItem->valDouble ;
      pValue->valInt64  = pItem->valInt64 ;
      pValue->pValStr   = pItem->pValStr ;
      pValue->pChild    = pItem->pChild ;
   }
}