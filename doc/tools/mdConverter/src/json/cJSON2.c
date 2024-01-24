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

#include <math.h>
#include "ossMem.h"
#include "ossUtil.h"
#include "cJSON2.h"

typedef enum _stringType {
   TYPE_STRING_NONE = 0,
   TYPE_STRING_QUOTE,
   TYPE_STRING_DOUBLE_QUOTES
} STRING_TYPE ;

typedef enum _dollarSymbolType {
   SYMBOL_NONE = 0,
   SYMBOL_LOGICAL,            //logical
   SYMBOL_MATCHER,            //match
   SYMBOL_SELECTOR,           //select
   SYMBOL_UPDATE,             //update
   SYMBOL_AGGREGATION,        //aggregation
   SYMBOL_INTERNAL,           //internal
   SYMBOL_DATATYPE,           //data type
   SYMBOL_UNCERTAIN           //data type or other( uncertain )
} DOLLAR_SYMBOL_TYPE ;

/* CJSON match */
typedef struct _cJsonMatch {
   /* dollar symbol type */
   DOLLAR_SYMBOL_TYPE type ;
   /* data type */
   CJSON_VALUE_TYPE dataType ;
   /* the same characters of number on a previous symbol */
   UINT32 sameCharNum ;
   /* symbol length */
   UINT32 strLen ;
   /* dollar symbol */
   CHAR symbol[30] ;
} CJSON_MATCH ;

#define CHAR_LEFT_ROUND_BRACKET     '('
#define CHAR_RIGHT_ROUND_BRACKET    ')'
#define CHAR_LEFT_CURLY_BRACE       '{'
#define CHAR_RIGHT_CURLY_BRACE      '}'
#define CHAR_LEFT_SQUARE_BRACKET    '['
#define CHAR_RIGHT_SQUARE_BRACKET   ']'
#define CHAR_DOUBLE_QUOTES '"'
#define CHAR_SLASH  '\\'
#define CHAR_COMMA  ','
#define CHAR_COLON  ':'
#define CHAR_DOLLAR '$'
#define CHAR_QUOTE  '\''
#define CHAR_SPACE  32
#define CHAR_HT     9

CJSON_PLOG_FUNC _pCJsonPrintfLogFun = NULL ;

static const CHAR* skip( const CHAR *pStr ) ;
static const CHAR* skip2JsonStart( const CHAR *pStr ) ;
static BOOLEAN checkCustomType( CJSON *pItem,
                                CJSON_MACHINE *pMachine,
                                CJSON_VALUE_TYPE *pKeyType,
                                CJSON_VALUE_TYPE *pValType ) ;

static const CHAR* readToken( CJSON *pItem,
                              const CHAR *pStr,
                              CJSON_MACHINE *pMachine ) ;
static const CHAR* readKey( CJSON *pItem,
                            const CHAR *pStr,
                            CJSON_MACHINE *pMachine ) ;
static const CHAR* readValue( CJSON *pItem,
                              const CHAR *pStr,
                              CJSON_MACHINE *pMachine ) ;

static const CHAR* tokenReady( CJSON *pItem,
                               const CHAR *pStr,
                               CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjStart( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjKey( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjColon( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjValue( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjComma( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenObjEnd( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenArrStart( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenArrValue( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenArrComma( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine ) ;
static const CHAR* tokenArrEnd( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine ) ;

static const CHAR* readObject( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readArray( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readString( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readNumber( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readPINF( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readNINF( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readTrue( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readFalse( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo ) ;
static const CHAR* readNull( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo ) ;

static const CHAR* parseValue( const CHAR *pStr,
                               INPUT_FUNC *pParseFun,
                               INT32 *pListIndex ) ;
static CHAR* parseString( const CHAR *pStr,
                          INT32 length,
                          const CJSON_MACHINE *pMachine ) ;
static const CHAR* parseNumber( const CHAR *pStr,
                                INT32 *pValInt,
                                FLOAT64 *pValDouble,
                                INT64 *pValLong,
                                CJSON_VALUE_TYPE *pNumType ) ;
static const CHAR* parseCommand( const CHAR *pStr,
                                 STRING_TYPE type,
                                 DOLLAR_SYMBOL_TYPE *pKeyAttr,
                                 CJSON_VALUE_TYPE *pKeyType ) ;
static const CHAR* parseArgImpl( const CHAR *pStr,
                                 CHAR character,
                                 const CJSON_MACHINE *pMachine,
                                 va_list *pVaList ) ;
static const CHAR* parseArgs( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              const CHAR *pFormat,
                              INT32 *pArgNum,
                              va_list *pVaList ) ;

#define INPUT_LEN_STR( str ) sizeof(str)-1,str
static const CJSON_MATCH _command[] = {
   /* dollar type      data type    sameCharNum    symbol length   symbol  */
   { SYMBOL_SELECTOR,    0,                 0, INPUT_LEN_STR( "$abs" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$add" ) },
   { SYMBOL_UPDATE,      0,                 4, INPUT_LEN_STR( "$addtoset" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$all" ) },
   { SYMBOL_LOGICAL,     0,                 2, INPUT_LEN_STR( "$and" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$avg" ) },
   { SYMBOL_DATATYPE,    CJSON_BINARY,      1, INPUT_LEN_STR( "$binary" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$bit" ) },
   { SYMBOL_UPDATE,      0,                 4, INPUT_LEN_STR( "$bitand" ) },
   { SYMBOL_UPDATE,      0,                 4, INPUT_LEN_STR( "$bitnot" ) },
   { SYMBOL_UPDATE,      0,                 4, INPUT_LEN_STR( "$bitor" ) },
   { SYMBOL_UPDATE,      0,                 4, INPUT_LEN_STR( "$bitxor" ) },
   { SYMBOL_SELECTOR,    0,                 1, INPUT_LEN_STR( "$cast" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$ceiling" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$count" ) },
   { SYMBOL_DATATYPE,    CJSON_DATE,        1, INPUT_LEN_STR( "$date" ) },
   { SYMBOL_DATATYPE,    CJSON_DECIMAL,     2, INPUT_LEN_STR( "$decimal" ) },
   { SYMBOL_SELECTOR,    0,                 3, INPUT_LEN_STR( "$default" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$divide" ) },
   { SYMBOL_MATCHER,     0,                 1, INPUT_LEN_STR( "$elemMatch" ) },
   { SYMBOL_SELECTOR,    0,                 10,INPUT_LEN_STR( "$elemMatchOne" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$et" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$exists" ) },
   { SYMBOL_MATCHER,     0,                 1, INPUT_LEN_STR( "$field" ) },
   { SYMBOL_AGGREGATION, 0,                 3, INPUT_LEN_STR( "$first" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$floor" ) },
   { SYMBOL_AGGREGATION, 0,                 1, INPUT_LEN_STR( "$group" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$gt" ) },
   { SYMBOL_MATCHER,     0,                 3, INPUT_LEN_STR( "$gte" ) },
   { SYMBOL_MATCHER,     0,                 1, INPUT_LEN_STR( "$in" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$inc" ) },
   { SYMBOL_SELECTOR,    0,                 4, INPUT_LEN_STR( "$include" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$isnull" ) },
   { SYMBOL_AGGREGATION, 0,                 1, INPUT_LEN_STR( "$last" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$limit" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$lower" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$lt" ) },
   { SYMBOL_MATCHER,     0,                 3, INPUT_LEN_STR( "$lte" ) },
   { SYMBOL_SELECTOR,    0,                 3, INPUT_LEN_STR( "$ltrim" ) },
   { SYMBOL_AGGREGATION, 0,                 1, INPUT_LEN_STR( "$match" ) },
   { SYMBOL_AGGREGATION, 0,                 3, INPUT_LEN_STR( "$max" ) },
   { SYMBOL_MATCHER,     0,                 4, INPUT_LEN_STR( "$maxDistance" ) },
   { SYMBOL_DATATYPE,    CJSON_MAXKEY,      4, INPUT_LEN_STR( "$maxKey" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$mergearrayset" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$min" ) },
   { SYMBOL_DATATYPE,    CJSON_MINKEY,      4, INPUT_LEN_STR( "$minKey" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$mod" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$multiply" ) },
   { SYMBOL_MATCHER,     0,                 1, INPUT_LEN_STR( "$ne" ) },
   { SYMBOL_MATCHER,     0,                 3, INPUT_LEN_STR( "$near" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$nin" ) },
   { SYMBOL_LOGICAL,     0,                 2, INPUT_LEN_STR( "$not" ) },
   { SYMBOL_DATATYPE,    CJSON_NUMBER_LONG, 2, INPUT_LEN_STR( "$numberLong" ) },
   { SYMBOL_DATATYPE,    CJSON_OID,         1, INPUT_LEN_STR( "$oid" ) },
   { SYMBOL_DATATYPE,    CJSON_OPTIONS,     2, INPUT_LEN_STR( "$options" ) },
   { SYMBOL_LOGICAL,     0,                 2, INPUT_LEN_STR( "$or" ) },
   { SYMBOL_UPDATE,      0,                 1, INPUT_LEN_STR( "$pop" ) },
   { SYMBOL_DATATYPE,    CJSON_PRECISION,   2, INPUT_LEN_STR( "$precision" ) },
   { SYMBOL_AGGREGATION, 0,                 3, INPUT_LEN_STR( "$project" ) },
   { SYMBOL_UPDATE,      0,                 2, INPUT_LEN_STR( "$pull" ) },
   { SYMBOL_UPDATE,      0,                 5, INPUT_LEN_STR( "$pull_all" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$push" ) },
   { SYMBOL_UPDATE,      0,                 5, INPUT_LEN_STR( "$push_all" ) },
   { SYMBOL_DATATYPE,    CJSON_REGEX,       1, INPUT_LEN_STR( "$regex" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$rename" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$replace" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$rtrim" ) },
   { SYMBOL_UPDATE,      0,                 1, INPUT_LEN_STR( "$set" ) },
   { SYMBOL_MATCHER,     0,                 2, INPUT_LEN_STR( "$size" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$skip" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$slice" ) },
   { SYMBOL_AGGREGATION, 0,                 2, INPUT_LEN_STR( "$sort" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$strlen" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$substr" ) },
   { SYMBOL_SELECTOR,    0,                 4, INPUT_LEN_STR( "$subtract" ) },
   { SYMBOL_SELECTOR,    0,                 3, INPUT_LEN_STR( "$sum" ) },
   { SYMBOL_DATATYPE,    CJSON_TIMESTAMP,   1, INPUT_LEN_STR( "$timestamp" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$trim" ) },
   { SYMBOL_UNCERTAIN,   CJSON_TYPE,        2, INPUT_LEN_STR( "$type" ) },
   { SYMBOL_DATATYPE,    CJSON_UNDEFINED,   1, INPUT_LEN_STR( "$undefined" ) },
   { SYMBOL_UPDATE,      0,                 3, INPUT_LEN_STR( "$unset" ) },
   { SYMBOL_SELECTOR,    0,                 2, INPUT_LEN_STR( "$upper" ) },
   { SYMBOL_MATCHER,     0,                 1, INPUT_LEN_STR( "$within" ) },
   { SYMBOL_INTERNAL,    0,                 1, INPUT_LEN_STR( "$Aggr" ) },
   { SYMBOL_INTERNAL,    0,                 1, INPUT_LEN_STR( "$Meta" ) },
   { SYMBOL_INTERNAL,    0,                 2, INPUT_LEN_STR( "$Modify" ) },
   { SYMBOL_INTERNAL,    0,                 1, INPUT_LEN_STR( "$SetOnInsert" ) },
} ;

static const INT32 _commandSize = sizeof( _command ) / sizeof( CJSON_MATCH ) ;

#define CJSON_MATCH_INPUT( func, str ) sizeof(str)-1,func,str
static CJSON_VALUE_MATCH _valueList[50] = {
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readString, "\"" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readString, "'" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "+" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "-" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "." ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "0" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "1" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "2" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "3" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "4" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "5" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "6" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "7" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "8" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNumber, "9" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readFalse,  "false" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNull,   "null" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readTrue,   "true" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readArray,  "[" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readObject, "{" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readPINF,   "inf" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readPINF,   "+inf" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readPINF,   "1.#INF" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readPINF,   "Infinity" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readPINF,   "+Infinity" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNINF,   "-inf" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNINF,   "-1.#INF" ) },
   { CJSON_MATCH_VALUE, CJSON_MATCH_INPUT( readNINF,   "-Infinity" ) },
} ;

static INT32 _valueListSize = 28 ; // _valueList current size

static const INT32 _valueListMaxSize = \
      sizeof( _valueList ) / sizeof( CJSON_VALUE_MATCH ) ;

SDB_EXPORT BOOLEAN cJsonParse( const CHAR *pStr, CJSON_MACHINE *pMachine )
{
   BOOLEAN rc = TRUE ;
   if( pMachine == NULL )
   {
      CJSON_PRINTF_LOG( "State machine is NULL" ) ;
      goto error ;
   }
   pMachine->pItem = cJsonItemCreate( pMachine ) ;
   if( pMachine->pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pStr = readToken( pMachine->pItem, pStr, pMachine ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse JSON" ) ;
      goto error ;
   }
   if( pMachine->isCheckEnd && *pStr )
   {
      CJSON_PRINTF_LOG( "Exist redundant after JSON" ) ;
      goto error ;
   }
done:
   return rc ;
error:
   rc = FALSE ;
   goto done ;
}

static const CHAR* readToken( CJSON *pItem,
                              const CHAR *pStr,
                              CJSON_MACHINE *pMachine )
{
   while( TRUE )
   {
      switch( pMachine->state )
      {
      case STATE_READY:
         /* pStr: xxxxx */
         pStr = tokenReady( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_OBJECT_START:
         /* pStr: {xxxxxx */
         pStr = tokenObjStart( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_OBJECT_KEY:
         /* pStr: { xxxxx */
         pStr = tokenObjKey( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_OBJECT_COLON:
         /* pStr: { <key>xxxxxx */
         pStr = tokenObjColon( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_OBJECT_VALUE:
         /* pStr: { <key>: xxxxxx */
         pStr = tokenObjValue( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_OBJECT_COMMA:
         /* pStr: { <key>: <value>xxxxx */
         pStr = tokenObjComma( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         if( pMachine->state == STATE_OBJECT_KEY )
         {
            pItem = pItem->pNext ;
         }
         break ;
      case STATE_OBJECT_END:
         /* pStr: { <key>: <value> } */
         pStr = tokenObjEnd( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         goto done ;
         break ;
      case STATE_ARRAY_START:
         /* pStr: [xxxxxx */
         pStr = tokenArrStart( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_ARRAY_VALUE:
         /* pStr: [ xxxxxx */
         pStr = tokenArrValue( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         break ;
      case STATE_ARRAY_COMMA:
         /* pStr: [ <value>,xxxxx */
         pStr = tokenArrComma( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         if( pMachine->state == STATE_ARRAY_VALUE )
         {
            pItem = pItem->pNext ;
         }
         break ;
      case STATE_ARRAY_END:
         /* pStr: [ <value> ] */
         pStr = tokenArrEnd( pItem, pStr, pMachine ) ;
         if( pStr == NULL )
         {
            goto error ;
         }
         goto done ;
         break ;
      case STATE_ERROR:
         pStr = NULL ;
         break ;
      default:
         CJSON_PRINTF_LOG( "Unknow state: ", pMachine->state ) ;
         goto error ;
      }
   }
done:
   return pStr ;
error:
   pMachine->state = STATE_ERROR ;
   pStr = NULL ;
   goto done ;
}

static const CHAR* tokenReady( CJSON *pItem,
                               const CHAR *pStr,
                               CJSON_MACHINE *pMachine )
{
   const CHAR *pTmp = NULL ;
   pStr = skip( pStr ) ;
   if( *pStr == 0 )
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON is missing '{'" ) ;
      goto error ;
   }
   pTmp = skip2JsonStart( pStr ) ;
   if( *pTmp == 0 )
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON is missing '{'" ) ;
      goto error ;
   }
   pMachine->state = STATE_OBJECT_START ;
   pMachine->level = 0 ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjStart( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   pStr = skip( pStr + 1 ) ;
   if( *pStr == 0 )
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON key is empty" ) ;
      goto error ;
   }
   if( *pStr == CHAR_RIGHT_CURLY_BRACE )
   {
      pMachine->state = STATE_OBJECT_END ;
   }
   else
   {
      pMachine->state = STATE_OBJECT_KEY ;
   }
   ++pMachine->level ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjKey( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine )
{
   if( *pStr == CHAR_COMMA )
   {
      // { xx : xx, , <--- is error
      CJSON_PRINTF_LOG( "Syntax Error: extra ','" ) ;
      goto error ;
   }
   pStr = readKey( pItem, pStr, pMachine ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse JSON key" ) ;
      goto error ;
   }
   pMachine->state = STATE_OBJECT_COLON ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjColon( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   if( *pStr != CHAR_COLON )
   {
      pStr = skip( pStr + 1 ) ;
   }
   if( *pStr != CHAR_COLON )
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON '%s' missing ':'", pItem->pKey ) ;
      goto error ;
   }
   pStr = skip( pStr + 1 ) ;
   pMachine->state = STATE_OBJECT_VALUE ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjValue( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   pStr = readValue( pItem, pStr, pMachine ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse object '%s' value", pItem->pKey ) ;
      goto error ;
   }
   pMachine->state = STATE_OBJECT_COMMA ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjComma( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   CJSON *pNewItem = NULL ;
   if( *pStr == CHAR_COMMA )
   {
      pStr = skip( pStr + 1 ) ;
      if( *pStr == CHAR_RIGHT_CURLY_BRACE )
      {
         pMachine->state = STATE_OBJECT_END ;
      }
      else
      {
         pNewItem = cJsonItemCreate( pMachine ) ;
         if( pNewItem == NULL )
         {
            CJSON_PRINTF_LOG( "Failed to create new item" ) ;
            goto error ;
         }
         pItem->pNext = pNewItem ;
         pNewItem->pPrev   = pItem ;
         pNewItem->pParent = pItem->pParent ;
         pMachine->state   = STATE_OBJECT_KEY ;
      }
   }
   else if( *pStr == CHAR_RIGHT_CURLY_BRACE )
   {
      pMachine->state = STATE_OBJECT_END ;
   }
   else
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON key '%s' missing ',' or '}'",
                        pItem->pKey ) ;
      goto error ;
   }
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenObjEnd( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine )
{
   --pMachine->level ;
   ++pStr ;
   return pStr ;
}

static const CHAR* tokenArrStart( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   pStr = skip( pStr + 1 ) ;
   if( *pStr == 0 )
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON array is missing ']'" ) ;
      goto error ;
   }
   if( *pStr == CHAR_RIGHT_SQUARE_BRACKET )
   {
      pMachine->state = STATE_ARRAY_END ;
   }
   else
   {
      pMachine->state = STATE_ARRAY_VALUE ;
   }
   ++pMachine->level ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenArrValue( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   if( *pStr == CHAR_COMMA )
   {
      // [ xx , , <--- is error
      CJSON_PRINTF_LOG( "Syntax Error: extra ','" ) ;
      goto error ;
   }
   pStr = readValue( pItem, pStr, pMachine ) ;
   if( pStr == NULL )
   {
      INT32 num = 0 ;
      CJSON *pTmp = pItem ;
      while( TRUE )
      {
         pTmp = pTmp->pPrev ;
         if( pTmp == NULL )
         {
            break ;
         }
         ++num ;
      }
      CJSON_PRINTF_LOG( "Failed to parse array '%d' value", num ) ;
      goto error ;
   }
   pMachine->state = STATE_ARRAY_COMMA ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenArrComma( CJSON *pItem,
                                  const CHAR *pStr,
                                  CJSON_MACHINE *pMachine )
{
   CJSON *pNewItem = NULL ;
   if( *pStr == CHAR_COMMA )
   {
      pStr = skip( pStr + 1 ) ;
      if( *pStr == CHAR_RIGHT_SQUARE_BRACKET )
      {
         pMachine->state = STATE_ARRAY_END ;
      }
      else
      {
         pNewItem = cJsonItemCreate( pMachine ) ;
         if( pNewItem == NULL )
         {
            CJSON_PRINTF_LOG( "Failed to create new item" ) ;
            goto error ;
         }
         pItem->pNext = pNewItem ;
         pNewItem->pPrev = pItem ;
         pNewItem->pParent = pItem->pParent ;
         pMachine->state = STATE_ARRAY_VALUE ;
      }
   }
   else if( *pStr == CHAR_RIGHT_SQUARE_BRACKET )
   {
      pMachine->state = STATE_ARRAY_END ;
   }
   else
   {
      CJSON_PRINTF_LOG( "Syntax Error: JSON array missing ',' or ']'" ) ;
      goto error ;
   }
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* tokenArrEnd( CJSON *pItem,
                                const CHAR *pStr,
                                CJSON_MACHINE *pMachine )
{
   --pMachine->level ;
   ++pStr ;
   return pStr ;
}

static const CHAR* readKey( CJSON *pItem,
                            const CHAR *pStr,
                            CJSON_MACHINE *pMachine )
{
   STRING_TYPE type = TYPE_STRING_NONE ;
   DOLLAR_SYMBOL_TYPE keyAttr = SYMBOL_NONE ;
   CJSON_VALUE_TYPE keyType = CJSON_NONE ;
   INT32 keyLen  = 0 ;
   const CHAR *pStrStart = NULL ;

   // step: 1
   if( *pStr == CHAR_QUOTE )
   {
      // 'xxxx
      type = TYPE_STRING_QUOTE ;
      ++pStr ;
   }
   else if( *pStr == CHAR_DOUBLE_QUOTES )
   {
      // "xxxx
      type = TYPE_STRING_DOUBLE_QUOTES ;
      ++pStr ;
   }
   pStrStart = pStr ;

   // step: 2
   pStr = parseCommand( pStr, type, &keyAttr, &keyType ) ;
   if( pStr == NULL )
   {
      goto error ;
   }
   keyLen = pStr - pStrStart ;
   
   if( ( type == TYPE_STRING_QUOTE && *pStr == CHAR_QUOTE ) ||
       ( type == TYPE_STRING_DOUBLE_QUOTES && *pStr == CHAR_DOUBLE_QUOTES ) )
   {
      ++pStr ;
   }

   // step: 3
   if( keyAttr == SYMBOL_NONE )
   {
      // xxx
      if( *pStrStart == CHAR_DOLLAR &&
          pMachine->parseMode == CJSON_RIGOROUS_PARSE )
      {
         // $???
         CJSON_PRINTF_LOG( "Rigorous mode, can's use\
 an undefined command: %.*s", keyLen, pStrStart ) ;
         goto error ;
      }
      pItem->pKey = parseString( pStrStart, keyLen, pMachine ) ;
      if( pItem->pKey == NULL )
      {
         CJSON_PRINTF_LOG( "Failed to parse JSON key" ) ;
         goto error ;
      }
   }
   else
   {
      // $xxx
      if( keyAttr == SYMBOL_DATATYPE ||
          keyAttr == SYMBOL_UNCERTAIN )
      {
         // $<type>
         pItem->keyType = keyType ;
      }
      pItem->pKey = parseString( pStrStart, keyLen, pMachine ) ;
      if( pItem->pKey == NULL )
      {
         CJSON_PRINTF_LOG( "Failed to parse dollar command key" ) ;
         goto error ;
      }
   }
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* readValue( CJSON *pItem,
                              const CHAR *pStr,
                              CJSON_MACHINE *pMachine )
{
   INT32 listIndex = _valueListSize - 1 ;
   INPUT_FUNC parseFun = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   const CHAR *pTmpStr = NULL ;

   while( TRUE )
   {
      pTmpStr = parseValue( pStr, &parseFun, &listIndex ) ;
      if( pTmpStr == NULL )
      {
         CJSON_PRINTF_LOG( "Failed to parse JSON value" ) ;
         goto error ;
      }
      if( parseFun == NULL )
      {
         CJSON_PRINTF_LOG( "The parse function is null" ) ;
         goto error ;
      }
      pTmpStr = parseFun( pTmpStr, pMachine, &pReadInfo ) ;
      if( pTmpStr == NULL )
      {
         //CJSON_PRINTF_LOG( "Failed to call parse function" ) ;
         goto error ;
      }
      if( cJsonReadInfoExecState( pReadInfo ) == CJSON_EXEC_IGNORE )
      {
         --listIndex ;
         continue ;
      }
      if( pReadInfo )
      {
         if( pItem->keyType == CJSON_NONE &&
             pReadInfo->keyType != CJSON_NONE )
         {
            pItem->keyType = pReadInfo->keyType ;
         }
         if( pReadInfo->valType != CJSON_NONE )
         {
            pItem->valType = pReadInfo->valType ;
         }
         if( pReadInfo->readType == CJSON_READ_VALUE )
         {
            if( pReadInfo->pItem )
            {
               pItem->valInt    = pReadInfo->pItem->valInt ;
               pItem->length    = pReadInfo->pItem->length ;
               pItem->valDouble = pReadInfo->pItem->valDouble ;
               pItem->valInt64  = pReadInfo->pItem->valInt64 ;
               pItem->pValStr   = pReadInfo->pItem->pValStr ;
            }
         }
         else if( pReadInfo->readType == CJSON_READ_OBJECT ||
                  pReadInfo->readType == CJSON_READ_ARRAY )
         {
            pItem->pChild = pReadInfo->pItem ;
            pReadInfo->pItem->pParent = pItem ;
         }
      }
      pStr = pTmpStr ;
      break ;
   }
   pStr = skip( pStr ) ;
done:
   return pStr ;
error:
   pStr = NULL ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* readObject( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   BOOLEAN check = TRUE ;
   CJSON_VALUE_TYPE keyType= CJSON_NONE ;
   CJSON_VALUE_TYPE valType = CJSON_NONE ;
   CJSON *pItem = NULL ;
   CJSON_MACHINE *pStateMachine = NULL ;
   CJSON_READ_INFO *pReadInfo   = NULL ;

   /*
      Under normal circumstances prohibit the state machine information,
      only object and array can be modified, because object and array need
      to modify the state to enter the token function
   */
   pStateMachine = (CJSON_MACHINE *)pMachine ;
   pReadInfo = cJsonReadInfoCreate( pStateMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pStateMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pStateMachine->state = STATE_OBJECT_START ;
   pStr = readToken( pItem, pStr, pStateMachine ) ;
   if( pStr == NULL )
   {
      goto error ;
   }
   check = checkCustomType( pItem, pStateMachine, &keyType, &valType ) ;
   if( check == FALSE )
   {
      goto error ;
   }
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   if( keyType == CJSON_CUSTOM )
   {
      cJsonReadInfoTypeCustom( pReadInfo ) ;
   }
   else
   {
      cJsonReadInfoTypeObject( pReadInfo ) ;
   }
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readArray( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo )
{
   CJSON *pItem  = NULL ;
   CJSON_MACHINE *pStateMachine = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   /*
      Under normal circumstances prohibit the state machine information,
      only object and array can be modified, because object and array need
      to modify the state to enter the token function
   */
   pStateMachine = (CJSON_MACHINE *)pMachine ;
   pReadInfo = cJsonReadInfoCreate( pStateMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pStateMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pStateMachine->state = STATE_ARRAY_START ;
   pStr = readToken( pItem, pStr, pStateMachine ) ;
   if( pStr == NULL )
   {
      goto error ;
   }
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   cJsonReadInfoTypeArray( pReadInfo ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readString( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   INT32 length = 0 ;
   BOOLEAN isSlash = FALSE ;
   STRING_TYPE stringType = TYPE_STRING_NONE ;
   CHAR *pValString = NULL ;
   CJSON *pItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;
   const CHAR *pStrStart = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   if( *pStr == CHAR_QUOTE )
   {
      stringType = TYPE_STRING_QUOTE ;
   }
   else if( *pStr == CHAR_DOUBLE_QUOTES )
   {
      stringType = TYPE_STRING_DOUBLE_QUOTES ;
   }
   ++pStr ;
   pStrStart = pStr ;
   while( TRUE )
   {
      if( *pStr == 0 )
      {
         if( stringType == TYPE_STRING_DOUBLE_QUOTES )
         {
            CJSON_PRINTF_LOG( "Syntax Error: JSON string is missing \"" ) ;
         }
         else if( stringType == TYPE_STRING_QUOTE )
         {
            CJSON_PRINTF_LOG( "Syntax Error: JSON string is missing '" ) ;
         }
         else
         {
            length = pStr - pStrStart ;
            CJSON_PRINTF_LOG( "Syntax Error: JSON string '%s' is not over yet",
                              length,
                              pStrStart ) ;
         }
         goto error ;
      }
      else if( *pStr == CHAR_SLASH && isSlash == FALSE )
      {
         isSlash = TRUE ;
         ++pStr ;
         continue ;
      }
      if( ( stringType == TYPE_STRING_DOUBLE_QUOTES &&
            *pStr == CHAR_DOUBLE_QUOTES &&
            isSlash == FALSE ) ||
          ( stringType == TYPE_STRING_QUOTE &&
            *pStr == CHAR_QUOTE &&
            isSlash == FALSE ) )
      {
         break ;
      }
      if( isSlash == TRUE )
      {
         isSlash = FALSE ;
      }
      ++pStr ;
   }
   length = pStr - pStrStart ;
   pValString = parseString( pStrStart, length, pMachine ) ;
   if( pValString == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse string value" ) ;
      goto error ;
   }
   cJsonItemValueString( pItem, pValString, length ) ;
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   cJsonReadInfoTypeString( pReadInfo ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readNumber( const CHAR *pStr,
                               const CJSON_MACHINE *pMachine,
                               CJSON_READ_INFO **ppReadInfo )
{
   CJSON_VALUE_TYPE numType = 0 ;
   INT32 valInt      = 0 ;
   FLOAT64 valDouble = 0 ;
   INT64 valInt64    = 0 ;
   CJSON *pItem      = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   pStr = parseNumber( pStr, &valInt, &valDouble, &valInt64, &numType ) ;
   if( pStr == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to parse number value" ) ;
      goto error ;
   }
   pStr = skip( pStr ) ;
   if( *pStr != CHAR_COMMA &&
       *pStr != CHAR_RIGHT_CURLY_BRACE &&
       *pStr != CHAR_RIGHT_SQUARE_BRACKET )
   {
      CJSON_PRINTF_LOG( "Failed to parse an invalid number" ) ;
      goto error ;
   }
   if( numType == CJSON_INT32 )
   {
      cJsonItemValueInt32( pItem, valInt ) ;
      cJsonReadInfoTypeInt32( pReadInfo ) ;
   }
   else if( numType == CJSON_INT64 )
   {
      cJsonItemValueInt64( pItem, valInt64 ) ;
      cJsonReadInfoTypeInt64( pReadInfo ) ;
   }
   else if( numType == CJSON_DOUBLE )
   {
      cJsonItemValueDouble( pItem, valDouble ) ;
      cJsonReadInfoTypeDouble( pReadInfo ) ;
   }
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonItemRelease( pItem ) ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

#define CJSON_INFINTY ((1.79769e+308)*2)
static const CHAR* readPINF( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo )
{
   CJSON *pItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   cJsonItemValueDouble( pItem, CJSON_INFINTY ) ;
   cJsonReadInfoTypeDouble( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readNINF( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo )
{
   CJSON *pItem = NULL ;
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   pItem = cJsonItemCreate( pMachine ) ;
   if( pItem == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new item" ) ;
      goto error ;
   }
   cJsonItemValueDouble( pItem, -CJSON_INFINTY ) ;
   cJsonReadInfoTypeDouble( pReadInfo ) ;
   cJsonReadInfoAddItem( pReadInfo, pItem ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readTrue( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo )
{
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   cJsonReadInfoTypeTrue( pReadInfo ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readFalse( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              CJSON_READ_INFO **ppReadInfo )
{
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   cJsonReadInfoTypeFalse( pReadInfo ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* readNull( const CHAR *pStr,
                             const CJSON_MACHINE *pMachine,
                             CJSON_READ_INFO **ppReadInfo )
{
   CJSON_READ_INFO *pReadInfo = NULL ;

   pReadInfo = cJsonReadInfoCreate( pMachine ) ;
   if( pReadInfo == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new ReadInfo" ) ;
      goto error ;
   }
   cJsonReadInfoTypeNull( pReadInfo ) ;
   ++pStr ;
   *ppReadInfo = pReadInfo ;
done:
   return pStr ;
error:
   pStr = NULL ;
   cJsonReadInfoRelease( pReadInfo ) ;
   goto done ;
}

static const CHAR* parseValue( const CHAR *pStr,
                               INPUT_FUNC *pParseFun,
                               INT32 *pListIndex )
{
   UINT32 x = 0 ;
   INT32 y = *pListIndex ;
   INT32 length = 0 ;
   const CHAR *pStrStart = pStr ;
   const CHAR *pEndOfVal = NULL ;
   while( *pStr && y >= 0 )
   {
      if( _valueList[y].strLen == 0 )
      {
         break ;
      }
      if( x < _valueList[y].strLen && _valueList[y].string[x] == *pStr )
      {
         ++x ;
         if( x == _valueList[y].strLen )
         {
            if( _valueList[y].matchType == CJSON_MATCH_FUNC )
            {
               pEndOfVal = skip( pStr + 1 ) ;
               if( *pEndOfVal == CHAR_LEFT_ROUND_BRACKET  )
               {
                  break ;
               }
            }
            else
            {
               break ;
            }
         }
         ++pStr ;
      }
      else
      {
         pStr = pStrStart ;
         x = 0 ;
         --y ;
      }
   }
   if( *pStr == 0 )
   {
      length = pStr - pStrStart ;
      if( length == 0 )
      {
         CJSON_PRINTF_LOG( "Syntax Error: JSON value is empty" ) ;
      }
      else
      {
         CJSON_PRINTF_LOG( "Syntax Error: '%.*s' is not over yet",
                           length,
                           pStrStart ) ;
      }
      goto error ;
   }
   else if( y < 0 )
   {
      while( TRUE )
      {
         if( *pStr == 0 )
         {
            length = pStr - pStrStart ;
            CJSON_PRINTF_LOG( "Syntax Error: '%.*s' is not over yet",
                              length,
                              pStrStart ) ;
            goto error ;
         }
         if( *pStr == CHAR_COMMA ||
             *pStr == CHAR_RIGHT_CURLY_BRACE ||
             *pStr == CHAR_RIGHT_SQUARE_BRACKET ||
             *pStr == CHAR_SPACE ||
             *pStr == CHAR_HT )
         {
            break ;
         }
         ++pStr ; 
      } 
      length = pStr - pStrStart ;
      CJSON_PRINTF_LOG( "ReferenceError: '%.*s' is not defined",
                        length,
                        pStrStart ) ;
      goto error ;
   }
   *pParseFun = _valueList[y].parseFun ;
   *pListIndex = y ;
done:
   return pStr ;
error:
   pStr = NULL ;
   goto done ;
}

static const UINT8 _firstByteMark[7] = {
   0x00,
   0x00,
   0xC0,
   0xE0,
   0xF0,
   0xF8,
   0xFC
} ;
static CHAR* parseString( const CHAR *pStr,
                          INT32 length,
                          const CJSON_MACHINE *pMachine )
{
   UINT8 uc  = 0 ;
   UINT8 uc2 = 0 ;
   INT32 len = 0 ;
   UINT32 ucTmp = 0 ;
   CHAR *pOut = NULL ;
   CHAR *pNewStr = NULL ;

   pOut = (CHAR *)cJsonMalloc( length + 1, pMachine ) ;
   if( pOut == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to malloc string memory" ) ;
      goto error ;
   }
   ossMemset( pOut, 0, length + 1 ) ;
   
   pNewStr = pOut ;

   while( length > 0 )
   {
      if( *pStr != '\\' )
      {
         *pOut = *pStr ;
         ++pOut ;
         ++pStr ;
         --length ;
      }
      else
      {
         ++pStr ;
         if( length > 0 )
         {
            --length ;
         }
         switch( *pStr )
         {
         case 'b':
         {
            *pOut = '\b' ;
            ++pOut ;
            break ;
         }
         case 'f':
         {
            *pOut = '\f' ;
            ++pOut ;
            break ;
         }
         case 'n':
         {
            *pOut = '\n' ;
            ++pOut ;
            break ;
         }
         case 'r':
         {
            *pOut = '\r' ;
            ++pOut ;
            break ;
         }
         case 't':
         {
            *pOut = '\t';
            ++pOut ;
            break ;
         }
         case 'u':    /* transcode utf16 to utf8. */
         {
            /* get the unicode char. */
            sscanf( pStr + 1, "%4x", &ucTmp ) ;
            uc = (UINT8)ucTmp ;
            pStr += 4 ;
            length -= 4 ;

            if( ( uc >= 0xDC00 && uc <= 0xDFFF ) || uc == 0 )
            {
               // check for invalid.
               break ;
            }

            if( uc >= 0xD800 && uc <= 0xDBFF )
            {
               // UTF16 surrogate pairs.
               if( pStr[1] != '\\' || pStr[2] != 'u' )
               {
                  // missing second-half of surrogate.
                  break ;
               }
               sscanf( pStr + 3, "%4x", &ucTmp ) ;
               uc2 = (UINT8)ucTmp ;
               pStr += 6 ;
               length -= 6 ;
               if( uc2 < 0xDC00 || uc2 > 0xDFFF )
               {
                  // invalid second-half of surrogate.
                  break ;
               }
               uc = 0x10000 | ( ( uc & 0x3FF ) << 10 ) | ( uc2 & 0x3FF ) ;
            }

            len = 4 ;
            if( uc < 0x80 )
            {
               len = 1 ;
            }
            else if( uc < 0x800 )
            {
               len = 2 ;
            }
            else if( uc < 0x10000 )
            {
               len = 3 ;
            }
            pOut += len ;
            switch( len )
            {
            case 4:
            {
               *--pOut = ( ( uc | 0x80 ) & 0xBF ) ;
               uc >>= 6 ;
            }
            case 3:
            {
               *--pOut = ( ( uc | 0x80 ) & 0xBF ) ;
               uc >>= 6 ;
            }
            case 2:
            {
               *--pOut = ( ( uc | 0x80 ) & 0xBF ) ;
               uc >>= 6 ;
            }
            case 1:
            {
               *--pOut = ( uc | _firstByteMark[len] ) ;
            }
            }
            pOut += len ;
            break ;
         }
         default:
         {
            *pOut = *pStr ;
            ++pOut ;
            break ;
         }
         }
         ++pStr ;
         --length ;
      }
   }
   *pOut = 0 ;
done:
   return pNewStr ;
error:
   pNewStr = NULL ;
   goto done ;
}

#define CJSON_INT32_MIN (-2147483647-1)
static const CHAR* parseNumber( const CHAR *pStr,
                                INT32 *pValInt,
                                FLOAT64 *pValDouble,
                                INT64 *pValLong,
                                CJSON_VALUE_TYPE *pNumType )
{
   INT32 subscale = 0 ;
   INT32 signsubscale = 1 ;
   FLOAT64 decimal = 0 ;
   FLOAT64 n = 0 ;
   FLOAT64 sign = 1 ;
   FLOAT64 scale = 0 ;
   INT32 n1 = 0 ;
   INT64 n2 = 0 ;
   volatile INT64 n3 = 0 ;
   CJSON_VALUE_TYPE numType = CJSON_INT32 ;

   //step 1
   if( *pStr == '-' )
   {
      //+xxx
      sign = -1 ;
      ++pStr ;
   }
   else if( *pStr == '+' )
   {
      //-xxx
      sign = 1 ;
      ++pStr ;
   }

   //step 2
   while( *pStr == '0' )
   {
      //0xxxxx
      ++pStr ;
   }

   //step 3
   while( *pStr >='0' && *pStr <= '9' )
   {
      //<number>xxxx
      INT32 num = *pStr - '0' ;
      n3 = ( n2 *10 ) + num ;
      if( ( n3 - num ) / 10 != n2 )
      {
         numType = CJSON_DOUBLE ;
      }
      n  = ( n * 10.0 ) + num ;   
      n1 = ( n1 * 10  ) + num ;
      n2 = n3 ;
      ++pStr ;
      if( numType == CJSON_INT32 && (INT64)n1 != n2 )
      {
         numType = CJSON_INT64 ;
      }
   }

   //step 4
   if( *pStr == '.' && pStr[1] >= '0' && pStr[1] <= '9' ) 
   {
      //<number>.xxx
      numType = CJSON_DOUBLE ;
      ++pStr ;
      do
      {
         decimal = decimal * 10 + ( *pStr - '0' ) ;
         ++pStr ;
         ++scale ;
      }
      while( *pStr >= '0' && *pStr <= '9' ) ;
      n = n + decimal / pow( 10.0, scale ) ;
   }

   //step 5
   if( *pStr == 'e' || *pStr == 'E' )
   {
      numType = CJSON_DOUBLE ;
      //<number>[e/E]xxx
      ++pStr ;
      if( *pStr == '+' )
      {
         ++pStr ;
      }
      else if( *pStr == '-' )
      {
         signsubscale = -1 ;
         ++pStr ;
      }
      while( *pStr >= '0' && *pStr <= '9' )
      {
         subscale = ( subscale * 10 ) + ( *pStr - '0' ) ;
         ++pStr ;
      }
   }

   //step 6
   if ( numType == CJSON_DOUBLE )
   {
      // number = +/- number.fraction * 10^+/- exponent
      n = sign * n * pow( 10.0, ( subscale * signsubscale * 1.0 ) ) ;
   }
   else if ( numType == CJSON_INT64 )
   {
      if( subscale != 0 )
      {
         n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
      }
      else
      {
         /*
            if subscale = 0, that means we don't need to translate into FLOAT64,
            which may cause loose precision
         */
         n2 = ( ( (INT64)sign ) * n2 ) ;
      }
      if( n2 == CJSON_INT32_MIN )
      {
         n1 = CJSON_INT32_MIN ;
         numType = CJSON_INT32 ;
      }
   }
   else if ( numType == CJSON_INT32 )
   {
      n1 = (INT32)( sign * n1 * pow( 10.0, subscale * 1.00 ) ) ;
      n2 = (INT64)( sign * n2 * pow( 10.0, subscale * 1.00 ) ) ;
      if( (INT64)n1 != n2 )
      {
         numType = CJSON_INT64 ;
      }
   }
   *pValDouble = n ;
   *pValInt = n1 ;
   *pValLong = n2 ;
   *pNumType = numType ;
   return pStr ;
}

/* parse dollar command */
static const CHAR* parseCommand( const CHAR *pStr,
                                 STRING_TYPE type,
                                 DOLLAR_SYMBOL_TYPE *pKeyAttr,
                                 CJSON_VALUE_TYPE *pKeyType )
{
   UINT32 x = 0 ;
   INT32 y = 0 ;
   INT32 length = 0 ;
   BOOLEAN isCHeck = TRUE ;
   BOOLEAN isSlash = FALSE ;
   const CHAR *pStrStart = pStr ;
   const CHAR *pColon    = NULL ;

   *pKeyType = SYMBOL_NONE ;
   while( TRUE )
   {
      if( *pStr == 0 ||
          ( type == TYPE_STRING_NONE  && ( *pStr == CHAR_COLON ||
                                           *pStr == CHAR_SPACE ||
                                           *pStr == CHAR_HT ) )||
          ( type == TYPE_STRING_QUOTE &&
            *pStr == CHAR_QUOTE &&
            isSlash == FALSE ) ||
          ( type == TYPE_STRING_DOUBLE_QUOTES &&
            *pStr == CHAR_DOUBLE_QUOTES &&
            isSlash == FALSE ) )
      {
         break ;
      }
      if( *pStr == CHAR_SLASH && isSlash == FALSE && type != TYPE_STRING_NONE )
      {
         isSlash = TRUE ;
         ++length ;
         ++pStr ;
         continue ;
      }
      else if( isSlash == TRUE )
      {
         isSlash = FALSE ;
      }
      if( type != TYPE_STRING_NONE && *pStr == CHAR_COLON )
      {
         pColon = pStr ;
      }
      if( y < _commandSize )
      {
         if( !isCHeck && _command[y].sameCharNum == x )
         {
            isCHeck = TRUE ;
         }
         if( isCHeck )
         {
            if( _command[y].symbol[x] == *pStr )
            {
               ++x ;
               ++length ;
               ++pStr ;
            }
            else
            {
               isCHeck = FALSE ;
               ++y ;
            }
         }
         else
         {
            ++y ;
         }
      }
      else
      {
         ++length ;
         ++pStr ;
      }
   }
   if( *pStr == 0 )
   {
      if( pColon == NULL )
      {
         CJSON_PRINTF_LOG( "Syntax Error: '%.*s' missing ':'",
                            length,
                            pStrStart ) ;
      }
      else
      {
         if( type == TYPE_STRING_QUOTE )
         {
            CJSON_PRINTF_LOG( "Syntax Error: '%.*s' missing '",
                              pColon - pStrStart,
                              pStrStart ) ;
         }
         else
         {
            CJSON_PRINTF_LOG( "Syntax Error: '%.*s' missing \"",
                              pColon - pStrStart,
                              pStrStart ) ;
         }
      }
      goto error ;
   }
   if( type == TYPE_STRING_NONE && length == 0 )
   {
      CJSON_PRINTF_LOG( "Syntax Error: invalid key" ) ;
      goto error ;
   }
   if( y < _commandSize && x == _command[y].strLen )
   {
      *pKeyAttr = _command[y].type ;
      *pKeyType = _command[y].dataType ;
   }
done:
   return pStr ;
error:
   pStr = NULL ;
   goto done ;
}

#define CJSON_MAX_INT   (2147483647)
#define CJSON_MIN_INT   (-2147483647-1)
#define CJSON_MAX_INT64 (9223372036854775807LL)
#define CJSON_MIN_INT64 (-9223372036854775807LL-1)
static const CHAR *_cJsonDataTypeString[] = {
   "unknow",
   "custom",
   "boolean",
   "boolean",
   "null",
   "number",
   "int",
   "long",
   "double",
   "string",
   "array",
   "object",
   "timestamp",
   "date",
   "regex",
   "options",
   "oid",
   "binary",
   "type",
   "minKey",
   "maxKey",
   "undefined",
   "numberLong",
   "decimal",
   "precision",
} ;
/* parse argument implement */
static const CHAR* parseArgImpl( const CHAR *pStr,
                                 CHAR character,
                                 const CJSON_MACHINE *pMachine,
                                 va_list *pVaList )
{
   CJSON_VALUE_TYPE valType = CJSON_NONE ;
   INT32 valInt      = 0 ;
   INT32 length      = 0 ;
   FLOAT64 valDouble = 0 ;
   INT64 valInt64    = 0 ;
   CHAR *pValString  = NULL ;

   if( *pStr == CHAR_DOUBLE_QUOTES || *pStr == CHAR_QUOTE )
   {
      // "xxxx  or  'xxxx
      BOOLEAN isSlash = FALSE ;
      STRING_TYPE stringType = TYPE_STRING_NONE ;
      const CHAR *pStrStart = NULL ;
      valType = CJSON_STRING ;
      if( *pStr == CHAR_DOUBLE_QUOTES )
      {
         stringType = TYPE_STRING_DOUBLE_QUOTES ;
      }
      else
      {
         stringType = TYPE_STRING_QUOTE ;
      }
      ++pStr ;
      pStrStart = pStr ;
      while( TRUE )
      {
         if( *pStr == 0 )
         {
            if( stringType == TYPE_STRING_DOUBLE_QUOTES )
            {
               CJSON_PRINTF_LOG( "Syntax Error: argument missing \"" ) ;
            }
            else
            {
               CJSON_PRINTF_LOG( "Syntax Error: argument missing '" ) ;
            }
            goto error ;
         }
         if( ( stringType == TYPE_STRING_DOUBLE_QUOTES &&
               *pStr == CHAR_DOUBLE_QUOTES &&
               isSlash == FALSE ) ||
             ( stringType == TYPE_STRING_QUOTE &&
               *pStr == CHAR_QUOTE &&
               isSlash == FALSE ) )
         {
            break ;
         }
         if( *pStr == CHAR_SLASH && isSlash == FALSE )
         {
            isSlash = TRUE ;
         }
         else if( isSlash == TRUE )
         {
            isSlash = FALSE ;
         }
         ++pStr ;
      }
      length = pStr - pStrStart ;
      pValString = parseString( pStrStart, length, pMachine ) ;
      pStr = skip( pStr + 1 ) ;
   }
   else if( ( *pStr >= '0' && *pStr <= '9' ) || *pStr == '+' || *pStr == '-' )
   {
      // <number>
      valType = CJSON_NUMBER ;
      pStr = parseNumber( pStr, &valInt, &valDouble, &valInt64, &valType ) ;
      pStr = skip( pStr ) ;
      if( *pStr != CHAR_COMMA && *pStr != CHAR_RIGHT_ROUND_BRACKET )
      {
         CJSON_PRINTF_LOG( "The argument is an invalid number" ) ;
         goto error ;
      }
   }
   else
   {
      const CHAR *pStrStart = pStr ;
      //true false null
      if( pStrStart[0] == 't' &&
          pStrStart[1] == 'r' &&
          pStrStart[2] == 'u' &&
          pStrStart[3] == 'e' )
      {
         pStrStart = skip( pStrStart + 4 ) ;
         if( *pStrStart == CHAR_COMMA ||
             *pStrStart == CHAR_RIGHT_ROUND_BRACKET )
         {
            valType = CJSON_TRUE ;
            pStr = pStrStart ;
         }
      }
      else if( pStrStart[0] == 'f' &&
               pStrStart[1] == 'a' &&
               pStrStart[2] == 'l' &&
               pStrStart[3] == 's' &&
               pStrStart[4] == 'e' )
      {
         pStrStart = skip( pStrStart + 5 ) ;
         if( *pStrStart == CHAR_COMMA ||
             *pStrStart == CHAR_RIGHT_ROUND_BRACKET )
         {
            valType = CJSON_FALSE ;
            pStr = pStrStart ;
         }
      }
      else if( pStrStart[0] == 'n' &&
               pStrStart[1] == 'u' &&
               pStrStart[2] == 'l' &&
               pStrStart[3] == 'l' )
      {
         pStrStart = skip( pStrStart + 4 ) ;
         if( *pStrStart == CHAR_COMMA ||
             *pStrStart == CHAR_RIGHT_ROUND_BRACKET )
         {
            valType = CJSON_NULL ;
            pStr = pStrStart ;
         }
      }
      if( valType == CJSON_NONE )
      {
         const CHAR *pStrStart = pStr ;
         while( TRUE )
         {
            if( *pStr == CHAR_COMMA || *pStr == CHAR_RIGHT_ROUND_BRACKET )
            {
               break ;
            }
            ++pStr ; 
         } 
         length = pStr - pStrStart ;
         CJSON_PRINTF_LOG( "ReferenceError: '%.*s' is not defined",
                           length,
                           pStrStart ) ;
         goto error ;
      }
   }
   switch( character )
   {
   case 'l':   //int
   {
      INT32 *pArg = va_arg( *pVaList, INT32* ) ;
      if( pArg == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      if( valType == CJSON_INT64 )
      {
         if( valInt64 > CJSON_MAX_INT )
         {
            *pArg = CJSON_MAX_INT ;
         }
         else if( valInt64 < CJSON_MIN_INT )
         {
            *pArg = CJSON_MIN_INT ;
         }
         else
         {
            *pArg = (INT32)valInt64 ;
         }
      }
      else if( valType == CJSON_INT32 )
      {
         *pArg = valInt ;
      }
      else if( valType == CJSON_DOUBLE )
      {
         if( valDouble > CJSON_MAX_INT )
         {
            *pArg = CJSON_MAX_INT ;
         }
         else if( valDouble < CJSON_MIN_INT )
         {
            *pArg = CJSON_MIN_INT ;
         }
         else
         {
            *pArg = (INT32)valDouble ;
         }
      }
      else
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type int in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 'L':   //int64
   {
      INT64 *pArg = va_arg( *pVaList, INT64* ) ;
      if( pArg == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      if( valType == CJSON_INT64 )
      {
         *pArg = valInt64 ;
      }
      else if( valType == CJSON_INT32 )
      {
         *pArg = (INT64)valInt ;
      }
      else if( valType == CJSON_DOUBLE )
      {
         if( valDouble > CJSON_MAX_INT64 )
         {
            *pArg = CJSON_MAX_INT64 ;
         }
         else if( valDouble < CJSON_MIN_INT64 )
         {
            *pArg = CJSON_MIN_INT64 ;
         }
         else
         {
            *pArg = (INT64)valDouble ;
         }
      }
      else
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type int64 in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 'd':   //double
   {
      FLOAT64 *pArg = va_arg( *pVaList, FLOAT64* ) ;
      if( pArg == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      if( valType == CJSON_INT64 )
      {
         *pArg = (FLOAT64)valInt64 ;
      }
      else if( valType == CJSON_INT32 )
      {
         *pArg = (FLOAT64)valInt ;
      }
      else if( valType == CJSON_DOUBLE )
      {
         *pArg = valDouble ;
      }
      else
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type double in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 's':   //string
   {
      INT32 *pArg1  = va_arg( *pVaList, INT32* ) ;
      CHAR **ppArg2 = va_arg( *pVaList, CHAR** ) ;
      if( pArg1 == NULL || ppArg2 == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      if( valType == CJSON_STRING )
      {
         *pArg1 = length ;
         *ppArg2 = pValString ;
      }
      else
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type string in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 'b':   //boolean
   {
      INT32 *pArg1  = va_arg( *pVaList, INT32* ) ;
      CHAR **ppArg2 = va_arg( *pVaList, CHAR** ) ;
      if( pArg1 == NULL || ppArg2 == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      if( valType != CJSON_TRUE && valType != CJSON_FALSE )
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type boolean in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 'z':   //any type
   {
      CVALUE *pArg = va_arg( *pVaList, CVALUE* ) ;
      if( pArg == NULL )
      {
         CJSON_PRINTF_LOG( "The parameter is null, system error" ) ;
         goto error ;
      }
      pArg->valType = valType ;
      if( valType == CJSON_INT64 )
      {
         pArg->valInt64 = valInt64 ;
      }
      else if( valType == CJSON_INT32 )
      {
         pArg->valInt = valInt ;
      }
      else if( valType == CJSON_DOUBLE )
      {
         pArg->valDouble = valDouble ;
      }
      else if( valType == CJSON_STRING )
      {
         pArg->length  = length ;
         pArg->pValStr = pValString ;
      }
      else if( valType == CJSON_TRUE || valType == CJSON_FALSE )
      {
      }
      else
      {
         CJSON_PRINTF_LOG( "Cannot use type %s as type int in argument",
                           _cJsonDataTypeString[valType] ) ;
         goto error ;
      }
      break ;
   }
   case 0:
   {
      break ;
   }
   default:
      CJSON_PRINTF_LOG( "Unknow type: %c", character ) ;
      goto error ;
      break ;
   }
done:
   return pStr ;
error:
   pStr = NULL ;
   goto done ;
}

static const CHAR* parseArgs( const CHAR *pStr,
                              const CJSON_MACHINE *pMachine,
                              const CHAR *pFormat,
                              INT32 *pArgNum,
                              va_list *pVaList )
{
   CHAR character = 0 ;
   BOOLEAN isOptional   = FALSE ;
   INT32 argNum         = 0 ;
   const CHAR *specWalk = pFormat ;
   while( TRUE )
   {
      character = *specWalk ;
      if( character == '|' )
      {
         isOptional = TRUE ;
      }
      else
      {
         if( *pStr == CHAR_RIGHT_ROUND_BRACKET )
         {
            if( isOptional == TRUE || character == 0 )
            {
               break ;
            }
            else
            {
               CJSON_PRINTF_LOG( "Not enough arguments in call to function" ) ;
               goto error ;
            }
         }
         ++argNum ;
         pStr = parseArgImpl( pStr, character, pMachine, pVaList ) ;
         if( pStr == NULL )
         {
            CJSON_PRINTF_LOG( "Failed to parse the No. %d argument",
                              argNum ) ;
            goto error ;
         }
         if( *pStr == CHAR_COMMA )
         {
            pStr = skip( pStr + 1 ) ;
         }
         else if( *pStr == CHAR_RIGHT_ROUND_BRACKET )
         {
            // do nothing
         }
         else
         {
            CJSON_PRINTF_LOG( "Syntax error: the No. %d argument "
                              "missing ',' or ')'",
                              argNum ) ;
            goto error ;
         }
      }
      if( *specWalk )
      {
         ++specWalk ;
      }
   }
   *pArgNum = argNum ;
done:
   return pStr ;
error:
   pStr = NULL ;
   goto done ;
}

SDB_EXPORT const CHAR* parseParameters( const CHAR *pStr,
                                        const CJSON_MACHINE *pMachine,
                                        const CHAR *pFormat,
                                        INT32 *pArgNum,
                                        ... )
{
   va_list vaList ;
   pStr = skip( pStr + 1 ) ;
   if( *pStr != CHAR_LEFT_ROUND_BRACKET )
   {
      CJSON_PRINTF_LOG( "The function expression '('" ) ;
      goto error ;
   }
   pStr = skip( pStr + 1 ) ;
   va_start( vaList, pArgNum ) ;
   pStr = parseArgs( pStr, pMachine, pFormat, pArgNum, &vaList ) ;
   va_end( vaList ) ;
   if( pStr == NULL )
   {
      goto error ;
   }
   ++pStr ;
done:
   return pStr ;
error:
   pStr = NULL ;
   goto done ;
}

SDB_EXPORT BOOLEAN cJsonParseNumber( const CHAR *pStr,
                                     INT32 length,
                                     INT32 *pValInt,
                                     FLOAT64 *pValDouble,
                                     INT64 *pValLong,
                                     CJSON_VALUE_TYPE *pNumType )
{
   BOOLEAN flag = TRUE ;
   const CHAR *pTmp = NULL ;

   if( pStr == NULL )
   {
      goto error ;
   }
   pTmp = parseNumber( pStr,
                       pValInt,
                       pValDouble,
                       pValLong,
                       pNumType ) ;
   if( pTmp - pStr != length )
   {
      goto error ;
   }
done:
   return flag ;
error:
   flag = FALSE ;
   goto done ;
}

static BOOLEAN checkCustomType( CJSON *pItem,
                                CJSON_MACHINE *pMachine,
                                CJSON_VALUE_TYPE *pKeyType,
                                CJSON_VALUE_TYPE *pValType )
{
   INT32 customType   = CJSON_NONE ;
   INT32 assistType   = CJSON_NONE ;
   INT32 normalKeyNum = 0 ;
   BOOLEAN rv         = TRUE ;
   CHAR *pKey         = NULL ;
   while( pItem )
   {
      switch( pItem->keyType )
      {
      case CJSON_TIMESTAMP:
      case CJSON_DATE:
      case CJSON_REGEX:
      case CJSON_OID:
      case CJSON_BINARY:
      case CJSON_MINKEY:
      case CJSON_MAXKEY:
      case CJSON_UNDEFINED:
      case CJSON_NUMBER_LONG:
      case CJSON_DECIMAL:
      {
         if( normalKeyNum > 0 )
         {
            if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
            {
               //$命令同级下存在其他普通字段
               CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                 pKey,
                                 pItem->pKey ) ;
               goto error ;
            }
            goto done ;
         }
         if( customType == CJSON_NONE )
         {
            customType = pItem->keyType ;
            if( customType == CJSON_REGEX &&
                assistType != CJSON_NONE &&
                assistType != CJSON_OPTIONS )
            {
               //$regex和非$options的辅助字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            else if( customType == CJSON_BINARY &&
                     assistType != CJSON_NONE &&
                     assistType != CJSON_TYPE )
            {
               //$binary和非$type的辅助字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            else if( customType == CJSON_DECIMAL &&
                     assistType != CJSON_NONE &&
                     assistType != CJSON_PRECISION )
            {
               //$decimal和非$precision的辅助字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            pKey = pItem->pKey ;
         }
         else
         {
            //同一级下,有多个类型字段
            if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
            {
               CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                 pKey,
                                 pItem->pKey ) ;
               goto error ;
            }
            goto done ;
         }
         break ;
      }
      case CJSON_OPTIONS:
      case CJSON_TYPE:
      case CJSON_PRECISION:
      {
         if( assistType == CJSON_NONE )
         {
            assistType = pItem->keyType ;
            if( assistType == CJSON_OPTIONS &&
                customType != CJSON_NONE &&
                customType != CJSON_REGEX )
            {
               //$options和非$regex的类型字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            else if( assistType == CJSON_TYPE &&
                     customType != CJSON_NONE &&
                     customType != CJSON_BINARY )
            {
               //$type和非$binary的类型字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            else if( assistType == CJSON_PRECISION &&
                     customType != CJSON_NONE &&
                     customType != CJSON_DECIMAL )
            {
               //$type和非$binary的类型字段
               if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
               {
                  CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                    pKey,
                                    pItem->pKey ) ;
                  goto error ;
               }
               goto done ;
            }
            pKey = pItem->pKey ;
         }
         else
         {
            //同一级下,有多个辅助字段
            if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
            {
               CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                 pKey,
                                 pItem->pKey ) ;
               goto error ;
            }
            goto done ;
         }
         break ;
      }
      default:
         ++normalKeyNum ;
         if( customType != CJSON_NONE )
         {
            if( pMachine->parseMode == CJSON_RIGOROUS_PARSE )
            {
               //$命令同级下存在其他普通字段
               CJSON_PRINTF_LOG( "The %s and the %s can not coexist",
                                 pKey,
                                 pItem->pKey ) ;
               goto error ;
            }
            goto done ;
         }
         break ;
      }
      pItem = pItem->pNext ;
   }
   if( customType != CJSON_NONE )
   {
      *pKeyType = CJSON_CUSTOM ;
   }
done:
   return rv ;
error:
   rv = FALSE ;
   pMachine->state = STATE_ERROR ;
   goto done ;
}

static const CHAR* skip( const CHAR *pStr )
{
   while( pStr && *pStr && ((UINT8)*pStr) <= 32 )
   {
      ++pStr ;
   }
   return pStr ;
}

static const CHAR* skip2JsonStart( const CHAR *pStr )
{
   while( pStr && *pStr && *pStr != CHAR_LEFT_CURLY_BRACE )
   {
      ++pStr ;
   }
   return pStr ;
}

#define CJSON_MALLOC_DEFAULT_SIZE 4096
static CJSON_MEMORY_BLOCK* cJsonCreateBlock( INT32 size )
{
   INT32 bufferSize = CJSON_MALLOC_DEFAULT_SIZE ;
   CJSON_MEMORY_BLOCK *pBlock = NULL ;

   pBlock = (CJSON_MEMORY_BLOCK*)\
            SDB_OSS_MALLOC( sizeof( CJSON_MEMORY_BLOCK ) ) ;
   if( pBlock == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to malloc memory, size: %d",
                        sizeof( CJSON_MEMORY_BLOCK ) ) ;
      goto error ;
   }
   ossMemset( pBlock, 0, sizeof( CJSON_MEMORY_BLOCK ) ) ;
   if( size > bufferSize )
   {
      INT32 quoiten = size / bufferSize ;
      INT32 remainder = size % bufferSize ;
      bufferSize = quoiten * CJSON_MALLOC_DEFAULT_SIZE ;
      if( remainder > 0 )
      {
         bufferSize += CJSON_MALLOC_DEFAULT_SIZE ;
      }
   }
   pBlock->pBuffer = (CHAR*)SDB_OSS_MALLOC( bufferSize ) ;
   if( pBlock->pBuffer == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to malloc memory, size: %d",
                        bufferSize ) ;
      goto error ;
   }
   pBlock->size = bufferSize ;
   pBlock->lastSize = bufferSize ;
   pBlock->pNext = NULL ;
   pBlock->pPrev = NULL ;
done:
   return pBlock ;
error:
   SDB_OSS_FREE( pBlock->pBuffer ) ;
   SDB_OSS_FREE( pBlock ) ;
   pBlock = NULL ;
   goto done ;
}

SDB_EXPORT void* cJsonMalloc( INT32 bytesNum, const CJSON_MACHINE *pMachine )
{
   INT32 useSize = 0 ;
   void *pBuffer = NULL ;
   CJSON_MEMORY_BLOCK *pBlock = NULL ;
   /*
      In the parse function, it is forbidden to modify the state machine
      information, but in the allocation of memory, the state machine save
      the memory list of information.
   */
   CJSON_MACHINE *pStateMachine = (CJSON_MACHINE *)pMachine ;
   if( pStateMachine->pFirstMemBlock == NULL )
   {
      pBlock = cJsonCreateBlock( bytesNum ) ;
      if( pBlock == NULL )
      {
         CJSON_PRINTF_LOG( "Failed to create new memory block" ) ;
         goto error ;
      }
      pStateMachine->pFirstMemBlock = pBlock ;
      pStateMachine->pMemBlock = pBlock ;
   }
   else
   {
      pBlock = pStateMachine->pMemBlock ;
      while( pBlock )
      {
         if( pBlock->lastSize < bytesNum )
         {
            if( pBlock->pNext )
            {
               pBlock = pBlock->pNext ;
            }
            else
            {
               pBlock = cJsonCreateBlock( bytesNum ) ;
               if( pBlock == NULL )
               {
                  CJSON_PRINTF_LOG( "Failed to create new memory block" ) ;
                  goto error ;
               }
               pBlock->pPrev = pStateMachine->pMemBlock ;
               pStateMachine->pMemBlock->pNext = pBlock ;
               pStateMachine->pMemBlock = pBlock ;
               break ;
            }
         }
         else
         {
            break ;
         }
      }
   }
   ++pBlock->mallocTimes ;
   useSize = pBlock->size - pBlock->lastSize ;
   pBuffer = (void*)( pBlock->pBuffer + useSize ) ;
   pBlock->lastSize -= bytesNum ;
done:
   return pBuffer ;
error:
   goto done ;
}

SDB_EXPORT void cJsonFree( void *pBuffer, const CJSON_MACHINE *pMachine )
{
}

SDB_EXPORT CJSON* cJsonItemCreate( const CJSON_MACHINE *pMachine )
{
   CJSON *pNode = (CJSON*)cJsonMalloc( sizeof( CJSON ), pMachine ) ;
   if( pNode )
   {
      ossMemset( pNode, 0, sizeof( CJSON ) ) ;
   }
   return pNode ;
}

SDB_EXPORT void cJsonItemRelease( const CJSON *pItem )
{
}

SDB_EXPORT void cJsonItemKey( CJSON *pItem, CHAR *pKey )
{
   pItem->pKey = pKey ;
}

SDB_EXPORT void cJsonItemKeyType( CJSON *pItem, CJSON_VALUE_TYPE keyType )
{
   pItem->keyType = keyType ;
}

SDB_EXPORT void cJsonItemValueInt32 ( CJSON *pItem, INT32 val )
{
   pItem->valType = CJSON_INT32 ;
   pItem->valInt = val ;
}

SDB_EXPORT void cJsonItemValueInt64 ( CJSON *pItem, INT64 val )
{
   pItem->valType = CJSON_INT64 ;
   pItem->valInt64 = val ;
}

SDB_EXPORT void cJsonItemValueDouble( CJSON *pItem, FLOAT64 val )
{
   pItem->valType = CJSON_DOUBLE ;
   pItem->valDouble = val ;
}

SDB_EXPORT void cJsonItemValueString( CJSON *pItem, CHAR *pValStr, INT32 length )
{
   pItem->valType = CJSON_STRING ;
   pItem->pValStr = pValStr ;
   pItem->length  = length ;
}

SDB_EXPORT void cJsonItemValueTrue( CJSON *pItem )
{
   pItem->valType = CJSON_TRUE ;
}

SDB_EXPORT void cJsonItemValueFalse( CJSON *pItem )
{
   pItem->valType = CJSON_FALSE ;
}

SDB_EXPORT void cJsonItemValueNull( CJSON *pItem )
{
   pItem->valType = CJSON_NULL ;
}

SDB_EXPORT void cJsonItemLinkChild ( CJSON *pItem, CJSON *pChild )
{
   pItem->pChild = pChild ;
   pChild->pParent = pItem ;
}

SDB_EXPORT void cJsonItemLinkNext  ( CJSON *pItem, CJSON *pNext )
{
   pItem->pNext = pNext ;
   pItem->pParent = pNext->pParent ;
   pNext->pPrev = pItem ;
}

SDB_EXPORT CJSON_READ_INFO* cJsonReadInfoCreate( const CJSON_MACHINE *pMachine )
{
   CJSON_READ_INFO *pInfo = (CJSON_READ_INFO*)\
         cJsonMalloc( sizeof( CJSON_READ_INFO ), pMachine ) ;
   if( pInfo )
   {
      ossMemset( pInfo, 0, sizeof( CJSON_READ_INFO ) ) ;
      cJsonReadInfoSuccess( pInfo ) ;
   }
   return pInfo ;
}

SDB_EXPORT void cJsonReadInfoRelease( CJSON_READ_INFO *pReadInfo )
{
}

SDB_EXPORT void cJsonReadInfoAddItem( CJSON_READ_INFO *pReadInfo, CJSON *pItem )
{
   pReadInfo->pItem = pItem ;
}

SDB_EXPORT void cJsonReadInfoSuccess( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->execState = CJSON_EXEC_SUCCESS ;
}

SDB_EXPORT void cJsonReadInfoIGNORE( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->execState = CJSON_EXEC_IGNORE ;
}

SDB_EXPORT INT32 cJsonReadInfoExecState( CJSON_READ_INFO *pReadInfo )
{
   return pReadInfo->execState ;
}

SDB_EXPORT void cJsonReadInfoSet( CJSON_READ_INFO *pReadInfo, INT32 readType )
{
   pReadInfo->readType = readType ;
}

SDB_EXPORT void cJsonReadInfoTypeInt32( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_INT32 ;
}

SDB_EXPORT void cJsonReadInfoTypeInt64( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_INT64 ;
}

SDB_EXPORT void cJsonReadInfoTypeDouble( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_DOUBLE ;
}

SDB_EXPORT void cJsonReadInfoTypeString( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_STRING ;
}

SDB_EXPORT void cJsonReadInfoTypeTrue( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_TRUE ;
}

SDB_EXPORT void cJsonReadInfoTypeFalse( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_FALSE ;
}

SDB_EXPORT void cJsonReadInfoTypeNull( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_VALUE ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_NULL ;
}

SDB_EXPORT void cJsonReadInfoTypeObject( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_OBJECT ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_OBJECT ;
}

SDB_EXPORT void cJsonReadInfoTypeArray( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_ARRAY ;
   pReadInfo->keyType  = CJSON_NONE ;
   pReadInfo->valType  = CJSON_ARRAY ;
}

SDB_EXPORT void cJsonReadInfoTypeCustom( CJSON_READ_INFO *pReadInfo )
{
   pReadInfo->readType = CJSON_READ_OBJECT ;
   pReadInfo->keyType  = CJSON_CUSTOM ;
   pReadInfo->valType  = CJSON_OBJECT ;
}

SDB_EXPORT CJSON_MACHINE* cJsonCreate()
{
   CJSON_MACHINE *pMachine = NULL ;
   pMachine = (CJSON_MACHINE*)SDB_OSS_MALLOC( sizeof( CJSON_MACHINE ) ) ;
   if( pMachine == NULL )
   {
      CJSON_PRINTF_LOG( "Failed to create new state machine" ) ;
   }
   else
   {
      ossMemset( pMachine, 0, sizeof( CJSON_MACHINE ) ) ;
      pMachine->state = STATE_READY ;
      pMachine->parseMode = CJSON_LOOSE_PARSE ;
      pMachine->level = 0 ;
      pMachine->isCheckEnd = FALSE ;
   }
   return pMachine ;
}

SDB_EXPORT void cJsonInit( CJSON_MACHINE *pMachine,
                           CJSON_PARSE_MODE mode,
                           BOOLEAN isCheckEnd )
{
   CJSON_MEMORY_BLOCK *pBlock = NULL ;
   pMachine->state = STATE_READY ;
   pMachine->parseMode = mode ;
   pMachine->level = 0 ;
   pMachine->isCheckEnd = isCheckEnd ;
   pMachine->pItem = NULL ;
   pMachine->pMemBlock = pMachine->pFirstMemBlock ;
   pBlock = pMachine->pMemBlock ;
   while( pBlock )
   {
      pBlock->lastSize = pBlock->size ;
      pBlock = pBlock->pNext ;
   }
}

SDB_EXPORT void cJsonRelease( CJSON_MACHINE *pMachine )
{
   CJSON_MEMORY_BLOCK *pBlock = NULL ;
   CJSON_MEMORY_BLOCK *pNext  = NULL ;
   if( pMachine )
   {
      pBlock = pMachine->pFirstMemBlock ;
      while( pBlock )
      {
         pNext = pBlock->pNext ;
         SAFE_OSS_FREE( pBlock->pBuffer ) ;
         SAFE_OSS_FREE( pBlock ) ;
         pBlock = pNext ;
      }
      SAFE_OSS_FREE( pMachine ) ;
   }
}

SDB_EXPORT void cJsonSetPrintfLog( void (*pFun)( const CHAR *pFunc,
                                                 const CHAR *pFile,
                                                 UINT32 line,
                                                 const CHAR *pFmt,
                                                 ... ) )
{
   _pCJsonPrintfLogFun = (CJSON_PLOG_FUNC)pFun ;
}

SDB_EXPORT BOOLEAN cJsonExtendAppend( CJSON_MATCH_TYPE matchType,
                                      INPUT_FUNC parseFun,
                                      UINT32 strLen,
                                      CHAR *pString )
{
   if( _valueListMaxSize > _valueListSize )
   {
      _valueList[ _valueListSize ].matchType = matchType ;
      _valueList[ _valueListSize ].strLen = strLen ;
      _valueList[ _valueListSize ].parseFun = parseFun ;
      ossStrncpy( _valueList[ _valueListSize ].string,
                  pString,
                  CJSON_VALU_MATCH_MAX_SIZE ) ;
      ++_valueListSize ;
      return TRUE ;
   }
   else
   {
      CJSON_PRINTF_LOG( "Max type: %d, current type: %d",
                        _valueListMaxSize,
                        _valueListSize ) ;
      return FALSE ;
   }
}
