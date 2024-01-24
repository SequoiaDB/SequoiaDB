/*******************************************************************************
   Copyright (C) 2023-present SequoiaDB Ltd.

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


/** \file utilTypeCast.h
    \brief type cast.
*/
#ifndef UTIL_TYPE_CAST__H
#define UTIL_TYPE_CAST__H

#include "core.h"

typedef union
{
   INT32    intVal ;
   INT64    longVal ;
   FLOAT64  doubleVal ;
} utilNumberVal ;

SDB_EXTERN_C_START

/*
 * \brief Convert a string to a numeric value
 *        Note: 1. [+/-]inf, [+/-]Infinity and nan are not supported.
 *              2. If type is 1, it means decimal type, but it does not
 *                 support decimal type, it needs to be processed by itself.
 *
 * \param [in]  data          String pointer to be parsed
 * \param [in]  length        string length, if value is -1,
 *                            the string is decimal continues to parse
 * \param [out] type          Type of value:
 *                               0: INT32
 *                               1: INT64
 *                               2: FLOAT64
 *                               3: Decimal
 * \param [out] value         Numeric value
 * \param [out] valueLength   The length of the value
 * \retval SDB_OK Retrieval Success
 */
SDB_EXPORT INT32 utilStrToNumber( const CHAR* data, INT32 length,
                                  INT32 *type, utilNumberVal *value,
                                  INT32 *valueLength ) ;

SDB_EXTERN_C_END

#endif // end UTIL_TYPE_CAST__H