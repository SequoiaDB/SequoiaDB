/*******************************************************************************
   Copyright (C) 2012-2018 SequoiaDB Ltd.

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

/** \file php_sequoiadb.h
    \brief XXX
 */
#ifndef PHP_SEQUOIADB_H
#define PHP_SEQUOIADB_H

#include "php_driver.h"

#define PHP_SEQUOIADB_VERSION "1.0.0"
extern zend_module_entry sequoiadb_module_entry;
#define phpext_sequoiadb_ptr &sequoiadb_module_entry

#ifdef PHP_WIN32
#define PHP_SEQUOIADB_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define PHP_SEQUOIADB_API __attribute__ ((visibility("default")))
#else
#define PHP_SEQUOIADB_API PHPAPI
#endif

#define PHP_JSON_OUTPUT_ARRAY	 0
#define PHP_JSON_OUTPUT_OBJECT 1

#define PHP_GET_VALUE_ERROR -2
#define PHP_GET_VALUE_NOTFIND -1

#define RETURN_ARRAY_TYPE TRUE
#define RETURN_STRING_TYPE FALSE

PHP_MINIT_FUNCTION(sequoiadb);
PHP_MSHUTDOWN_FUNCTION(sequoiadb);
PHP_RINIT_FUNCTION(sequoiadb);
PHP_RSHUTDOWN_FUNCTION(sequoiadb);
PHP_MINFO_FUNCTION(sequoiadb);

#ifdef ZTS
#define SEQUOIADB_G(v) TSRMG(sequoiadb_globals_id, zend_sequoiadb_globals *, v)
#else
#define SEQUOIADB_G(v) (sequoiadb_globals.v)
#endif

#endif