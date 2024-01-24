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


/** \file cJSON.h
    \brief parse json.
*/
#ifndef CJSON2__H
#define CJSON2__H

#include "core.h"

typedef enum _cJsonValueType {
   CJSON_NONE = 0,
   CJSON_CUSTOM,
   CJSON_FALSE,
   CJSON_TRUE,
   CJSON_NULL,
   CJSON_NUMBER,
   CJSON_INT32,
   CJSON_INT64,
   CJSON_DOUBLE,
   CJSON_STRING,
   CJSON_ARRAY,
   CJSON_OBJECT,
   CJSON_TIMESTAMP,
   CJSON_DATE,
   CJSON_REGEX,
   CJSON_OPTIONS,
   CJSON_OID,
   CJSON_BINARY,
   CJSON_TYPE,
   CJSON_MINKEY,
   CJSON_MAXKEY,
   CJSON_UNDEFINED,
   CJSON_NUMBER_LONG,
   CJSON_DECIMAL,
   CJSON_PRECISION
} CJSON_VALUE_TYPE ;

/* CJSON struct */
typedef struct cJSON {
   CJSON_VALUE_TYPE keyType ;
   CJSON_VALUE_TYPE valType ;
   INT32  valInt ;
   INT32  length ;
   FLOAT64 valDouble ;
   INT64  valInt64 ;
   struct cJSON *pNext ;
   struct cJSON *pPrev ;
   struct cJSON *pParent ;
   struct cJSON *pChild ;
   CHAR  *pKey ;
   CHAR  *pValStr ;
} CJSON ;

/* CJSON_READ_INFO type  */
typedef enum _cJsonReadType {
   CJSON_READ_VALUE = 0,
   CJSON_READ_OBJECT,
   CJSON_READ_ARRAY
} CJSON_READ_TYPE ;

/* read func exec state */
typedef enum _cJsonReadExecState {
   CJSON_EXEC_SUCCESS = 0,
   CJSON_EXEC_ERROR,
   CJSON_EXEC_IGNORE
} CJSON_READ_EXEC_STATE ;

/* read value extern struct */
typedef struct _cJsonReadInfo {
   /* read info mode */
   CJSON_READ_TYPE readType ;
   /* exec state */
   CJSON_READ_EXEC_STATE execState ;
   /* object key type */
   CJSON_VALUE_TYPE keyType ;
   /* object key's value type */
   CJSON_VALUE_TYPE valType ;
   /* read function return item */
   CJSON *pItem ;
} CJSON_READ_INFO ;

/* cJSON memory block */
typedef struct _cJsonMemoryBlock {
   /* malloc times */
   INT32 mallocTimes ;
   /* buffer size */
   INT32 size ;
   /* buffer last size */
   INT32 lastSize ;
   /* memory buffer */
   CHAR *pBuffer ;
   /* previous block */
   struct _cJsonMemoryBlock *pPrev ;
   /* next block */
   struct _cJsonMemoryBlock *pNext ;
} CJSON_MEMORY_BLOCK ;

/* the state of the state machine */
typedef enum _cJsonState {
   STATE_READY = 0,
   STATE_OBJECT_START,
   STATE_OBJECT_KEY,
   STATE_OBJECT_COLON,
   STATE_OBJECT_VALUE,
   STATE_OBJECT_COMMA,
   STATE_OBJECT_END,
   STATE_ARRAY_START,
   STATE_ARRAY_VALUE,
   STATE_ARRAY_COMMA,
   STATE_ARRAY_END,
   STATE_ERROR
} CJSON_STATE ;

/* CJSON_READ_INFO type  */
typedef enum _cJsonParseMode {
   CJSON_LOOSE_PARSE = 0,
   CJSON_RIGOROUS_PARSE
} CJSON_PARSE_MODE ;

/* cJSON state machine */
typedef struct _cJsonMachine {
   /* parse state */
   CJSON_STATE state ;
   /* parse mode */
   CJSON_PARSE_MODE parseMode ;
   /* parse json level */
   UINT32 level ;
   /* whether check the end of json */
   BOOLEAN isCheckEnd ;
   /* struct CJSON root node */
   CJSON *pItem ;
   /* the current memory block */
   CJSON_MEMORY_BLOCK *pMemBlock ;
   /* the first memory block */
   CJSON_MEMORY_BLOCK *pFirstMemBlock ;
} CJSON_MACHINE ;

/* parse parameter struct */
typedef struct cValue {
   CJSON_VALUE_TYPE  valType ;
   INT32  valInt ;
   INT32  length ;
   FLOAT64 valDouble ;
   INT64  valInt64 ;
   CHAR  *pValStr ;
   CJSON *pChild ;
} CVALUE ;

#define CJSON_VALU_MATCH_MAX_SIZE 30

typedef enum _cJsonMatchType {
   CJSON_MATCH_VALUE = 0,
   CJSON_MATCH_FUNC
} CJSON_MATCH_TYPE ;

typedef const CHAR *(*INPUT_FUNC)( const CHAR *pStr,\
                                   const CJSON_MACHINE *pMachine,\
                                   CJSON_READ_INFO **ppReadInfo ) ;

/* CJSON value match */
typedef struct _cJsonValueMatch {
   /* match type */
   CJSON_MATCH_TYPE matchType ;
   /* the string length */
   UINT32 strLen ;
   /* parse function */
   INPUT_FUNC parseFun ;
   /* value string */
   CHAR string[ CJSON_VALU_MATCH_MAX_SIZE ] ;
} CJSON_VALUE_MATCH ;

SDB_EXTERN_C_START

SDB_EXPORT void cJsonSetPrintfLog( void (*pFun)( const CHAR *pFunc,
                                                 const CHAR *pFile,
                                                 UINT32 line,
                                                 const CHAR *pFmt,
                                                 ... ) ) ;
SDB_EXPORT CJSON_MACHINE* cJsonCreate() ;
SDB_EXPORT void cJsonInit( CJSON_MACHINE *pMachine,
                           CJSON_PARSE_MODE mode,
                           BOOLEAN isCheckEnd ) ;
SDB_EXPORT BOOLEAN cJsonParse( const CHAR *pStr, CJSON_MACHINE *pMachine ) ;
SDB_EXPORT void cJsonRelease( CJSON_MACHINE *pMachine ) ;

/* cJSON extend */
#define CJSON_PRINTF_LOG( fmt, ... )\
{\
   if( _pCJsonPrintfLogFun != NULL )\
   {\
      _pCJsonPrintfLogFun( __FUNC__, __FILE__, __LINE__, fmt, ##__VA_ARGS__ ) ;\
   }\
}

typedef void (*CJSON_PLOG_FUNC)( const CHAR *pFunc, \
                                 const CHAR *pFile, \
                                 UINT32 line, \
                                 const CHAR *pFmt, \
                                 ... ) ;
extern CJSON_PLOG_FUNC _pCJsonPrintfLogFun ;

SDB_EXPORT const CHAR* parseParameters( const CHAR *pStr,
                                        const CJSON_MACHINE *pMachine,
                                        const CHAR *pFormat,
                                        INT32 *pArgNum,
                                        ... ) ;
SDB_EXPORT BOOLEAN cJsonParseNumber( const CHAR *pStr,
                                     INT32 length,
                                     INT32 *pValInt,
                                     FLOAT64 *pValDouble,
                                     INT64 *pValLong,
                                     CJSON_VALUE_TYPE *pNumType ) ;
SDB_EXPORT void* cJsonMalloc( INT32 bytesNum,
                              const CJSON_MACHINE *pMachine ) ;
SDB_EXPORT void cJsonFree( void *pBuffer, const CJSON_MACHINE *pMachine ) ;
SDB_EXPORT BOOLEAN cJsonExtendAppend( CJSON_MATCH_TYPE matchType,
                                      INPUT_FUNC parseFun,
                                      UINT32 strLen,
                                      CHAR *pString ) ;

/* ReadInfo function */
SDB_EXPORT CJSON_READ_INFO*
   cJsonReadInfoCreate( const CJSON_MACHINE *pMachine ) ;
SDB_EXPORT void cJsonReadInfoRelease( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoAddItem( CJSON_READ_INFO *pReadInfo,
                                      CJSON *pItem ) ;

SDB_EXPORT void cJsonReadInfoSuccess( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoIGNORE( CJSON_READ_INFO *pReadInfo ) ;

SDB_EXPORT INT32 cJsonReadInfoExecState( CJSON_READ_INFO *pReadInfo ) ;

SDB_EXPORT void cJsonReadInfoTypeInt32( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeInt64( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeDouble( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeString( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeTrue( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeFalse( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeNull( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeObject( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeArray( CJSON_READ_INFO *pReadInfo ) ;
SDB_EXPORT void cJsonReadInfoTypeCustom( CJSON_READ_INFO *pReadInfo ) ;

/* Item function */
SDB_EXPORT CJSON* cJsonItemCreate( const CJSON_MACHINE *pMachine ) ;
SDB_EXPORT void cJsonItemRelease( const CJSON *pItem ) ;
SDB_EXPORT void cJsonItemKey        ( CJSON *pItem, CHAR *pKey ) ;
SDB_EXPORT void cJsonItemKeyType    ( CJSON *pItem,
                                      CJSON_VALUE_TYPE keyType ) ;
SDB_EXPORT void cJsonItemValueInt32 ( CJSON *pItem, INT32 val ) ;
SDB_EXPORT void cJsonItemValueInt64 ( CJSON *pItem, INT64 val ) ;
SDB_EXPORT void cJsonItemValueDouble( CJSON *pItem, FLOAT64 val ) ;
SDB_EXPORT void cJsonItemValueString( CJSON *pItem,
                                      CHAR *pValStr,
                                      INT32 length ) ;
SDB_EXPORT void cJsonItemValueTrue  ( CJSON *pItem ) ;
SDB_EXPORT void cJsonItemValueFalse ( CJSON *pItem ) ;
SDB_EXPORT void cJsonItemValueNull  ( CJSON *pItem ) ;
SDB_EXPORT void cJsonItemLinkChild  ( CJSON *pItem, CJSON *pChild ) ;
SDB_EXPORT void cJsonItemLinkNext   ( CJSON *pItem, CJSON *pNext ) ;

typedef struct _cJson_iterator {
   CJSON *pItem ;
   const CJSON_MACHINE *pMachine ;
} cJson_iterator ;

/* iterator */
SDB_EXPORT const cJson_iterator*
   cJsonIteratorInit( const CJSON_MACHINE *pMachine ) ;

SDB_EXPORT BOOLEAN cJsonIteratorMore( const cJson_iterator *pIter ) ;
SDB_EXPORT void cJsonIteratorNext( const cJson_iterator *pIter ) ;
SDB_EXPORT const BOOLEAN cJsonIteratorMoreSub( const cJson_iterator *pIter ) ;
SDB_EXPORT const cJson_iterator*
   cJsonIteratorSub( const cJson_iterator *pIter ) ;

SDB_EXPORT const CHAR* cJsonIteratorKey( const cJson_iterator *pIter ) ;

SDB_EXPORT CJSON_VALUE_TYPE cJsonIteratorType( const cJson_iterator *pIter ) ;

SDB_EXPORT INT32 cJsonIteratorSubNum( const cJson_iterator *pIter ) ;
SDB_EXPORT INT32 cJsonIteratorSubNum2( const CVALUE *pValue ) ;

SDB_EXPORT INT32 cJsonIteratorInt32( const cJson_iterator *pIter ) ;
SDB_EXPORT INT64 cJsonIteratorInt64( const cJson_iterator *pIter ) ;
SDB_EXPORT FLOAT64 cJsonIteratorDouble( const cJson_iterator *pIter ) ;
SDB_EXPORT const CHAR *cJsonIteratorString( const cJson_iterator *pIter ) ;
SDB_EXPORT BOOLEAN cJsonIteratorBoolean( const cJson_iterator *pIter ) ;

SDB_EXPORT void cJsonIteratorBinary( const cJson_iterator *pIter,
                                     CVALUE *pBinData,
                                     CVALUE *pBinType ) ;
SDB_EXPORT void cJsonIteratorRegex( const cJson_iterator *pIter,
                                    CVALUE *pRegex,
                                    CVALUE *pOptions ) ;
SDB_EXPORT void cJsonIteratorTimestamp( const cJson_iterator *pIter,
                                        CVALUE *pTimestamp ) ;
SDB_EXPORT void cJsonIteratorDate( const cJson_iterator *pIter,
                                   CVALUE *pDate ) ;
SDB_EXPORT void cJsonIteratorObjectId( const cJson_iterator *pIter,
                                       CVALUE *pOid ) ;
SDB_EXPORT void cJsonIteratorNumberLong( const cJson_iterator *pIter,
                                         CVALUE *pNumberLong ) ;
SDB_EXPORT void cJsonIteratorDecimal( const cJson_iterator *pIter,
                                      CVALUE *pDecimal,
                                      CVALUE *pPrecision ) ;
SDB_EXPORT void cJsonIteratorPrecision( const cJson_iterator *pIter,
                                        CVALUE *pPrecision,
                                        CVALUE *pScale ) ;

SDB_EXTERN_C_END

#endif // end CJSON__H