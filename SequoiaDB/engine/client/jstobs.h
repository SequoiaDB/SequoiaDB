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

/** \file Jstobs.h
    \brief Json and Bson convert to each other.
*/
#ifndef JSTOBS__H
#define JSTOBS__H

#include "cJSON.h"
#include "core.h"
#include "bson/bson.h"

#define JSON_DECIMAL_NO_CONVERT  0
#define JSON_DECIMAL_TO_DOUBLE   1
#define JSON_DECIMAL_NO_STRING   2

SDB_EXTERN_C_START

/** \fn
*/

/** \fn BOOLEAN JsonSetPrintfLog()
    \brief json convert bson print log
    \param [in]  pFun print log function
*/
SDB_EXPORT void JsonSetPrintfLog( void (*pFun)( const CHAR *pFunc,
                                                const CHAR *pFile,
                                                UINT32 line,
                                                const CHAR *pFmt,
                                                ... ) ) ;

//Compatible with the old version, the new code is not recommended use.
SDB_EXPORT BOOLEAN jsonToBson ( bson *bs, const CHAR *json_str ) ;

//Compatible with the old version, the new code is not recommended use.
SDB_EXPORT BOOLEAN jsonToBson2 ( bson *bs,
                                 const CHAR *json_str,
                                 BOOLEAN isMongo,
                                 BOOLEAN isBatch ) ;

/** \fn BOOLEAN json2bson2( const CHAR *pJson, bson *pBson )
    \brief Json converts to bson.
    \param [in]  pJson The json string to convert
    \param [out] pBson The return bson object
    \retval TRUE Operation Success
    \retval FALSE Operation Fail
*/
SDB_EXPORT BOOLEAN json2bson2( const CHAR *pJson, bson *pBson ) ;

/** \fn BOOLEAN json2bson( const CHAR *pJson,
                           CJSON_MACHINE *pMachine,
                           INT32 parseMode,
                           BOOLEAN isCheckEnd,
                           BOOLEAN isUnicode,
                           bson *pBson )
    \brief Json converts to bson.
    \param [in]  pJson The json string to convert
    \param [in]  pMachine The json parse state machine
    \param [in]  parseMode The json parse mode, 0:loose mode; 1:rigorous mode;
    \param [in]  isCheckEnd whether to check the end of json
    \param [in]  isUnicode whether to escape Unicode encoding
    \param [in]  decimalto decimal type cast
                  JSON_DECIMAL_NO_CONVERT: no convert
                  JSON_DECIMAL_TO_DOUBLE:  convert to double
                  JSON_DECIMAL_NO_STRING:  convert to string
    \param [out] pBson The return bson object
    \retval TRUE Operation Success
    \retval FALSE Operation Fail
*/
SDB_EXPORT BOOLEAN json2bson( const CHAR *pJson,
                              CJSON_MACHINE *pMachine,
                              INT32 parseMode,
                              BOOLEAN isCheckEnd,
                              BOOLEAN isUnicode,
                              INT32 decimalto,
                              bson *pBson ) ;

SDB_EXPORT void setJsonPrecision( const CHAR *pFloatFmt ) ;

/** \fn BOOLEAN bsonToJson ( CHAR *buffer, INT32 bufsize, const bson *b,
                             BOOLEAN toCSV, BOOLEAN skipUndefined)
    \brief Bson converts to json.
    \param [in] buffer the buffer to convert
    \param [in] bufsize the buffer's size
    \param [in] b The bson object to convert
    \param [in] toCSV bson to csv or not
    \param [in] skipUndefined to skip undefined filed or not
    \param [out] buffer The return json string
    \retval TRUE Operation Success
    \retval FALSE Operation Fail
    \note Before calling this funtion,need to build up
             a buffer for the convertion result.
*/
SDB_EXPORT BOOLEAN bsonToJson ( CHAR *buffer, INT32 bufsize, const bson *b,
                                BOOLEAN toCSV, BOOLEAN skipUndefined ) ;

/** \fn BOOLEAN bsonToJson2 ( CHAR *buffer, INT32 bufsize, const bson *b,
                              BOOLEAN isStrict )
    \brief Bson converts to json.
    \param [in] buffer the buffer to convert
    \param [in] bufsize the buffer's size
    \param [in] b The bson object to convert
    \param [in] isStrict Strict export of data types
    \param [out] buffer The return json string
    \retval TRUE Operation Success
    \retval FALSE Operation Fail
    \note Before calling this funtion,need to build up
             a buffer for the convertion result.
*/

SDB_EXPORT BOOLEAN bsonToJson2 ( CHAR *buffer, INT32 bufsize, const bson *b,
                                 BOOLEAN isStrict ) ;

SDB_EXTERN_C_END

#endif // end JSTOBS__H