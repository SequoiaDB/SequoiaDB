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

/** \file Jstobs.h
    \brief Json and Bson convert to each other.
*/
#ifndef JSTOBS2__H
#define JSTOBS2__H

#include "cJSON2.h"
#include "core.h"
#include "bson/bson.h"

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
                           bson *pBson )
    \brief Json converts to bson.
    \param [in]  pJson The json string to convert
    \param [in]  pMachine The json parse state machine
    \param [in]  parseMode The json parse mode, 0:loose mode; 1:rigorous mode;
    \param [out] pBson The return bson object 
    \retval TRUE Operation Success
    \retval FALSE Operation Fail
*/
SDB_EXPORT BOOLEAN json2bson( const CHAR *pJson,
                              CJSON_MACHINE *pMachine,
                              INT32 parseMode,
                              BOOLEAN isCheckEnd,
                              bson *pBson ) ;

SDB_EXTERN_C_END

#endif // end JSTOBS__H