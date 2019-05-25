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


/** \file cJSON_ext.h
    \brief parse json ext.
*/

#ifndef CJSON_EXT__H
#define CJSON_EXT__H

#include "cJSON.h"

SDB_EXTERN_C_START

SDB_EXPORT void cJsonExtAppendFunction() ;

SDB_EXTERN_C_END

#endif // end CJSON_EXT__H