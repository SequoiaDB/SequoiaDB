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

#ifndef CLASS_SEQUOIADB_H__
#define CLASS_SEQUOIADB_H__

#include "php_driver.h"

PHP_METHOD( SequoiaDB, __construct ) ;
PHP_METHOD( SequoiaDB, install ) ;
PHP_METHOD( SequoiaDB, getError ) ;

PHP_METHOD( SequoiaDB, connect ) ;
PHP_METHOD( SequoiaDB, close ) ;
PHP_METHOD( SequoiaDB, isValid ) ;
PHP_METHOD( SequoiaDB, syncDB ) ;

PHP_METHOD( SequoiaDB, snapshot ) ;
PHP_METHOD( SequoiaDB, resetSnapshot ) ;
PHP_METHOD( SequoiaDB, list ) ;

PHP_METHOD( SequoiaDB, listCS ) ;
PHP_METHOD( SequoiaDB, selectCS ) ;
PHP_METHOD( SequoiaDB, createCS ) ;
PHP_METHOD( SequoiaDB, getCS ) ;
PHP_METHOD( SequoiaDB, dropCS ) ;

PHP_METHOD( SequoiaDB, listCL ) ;
PHP_METHOD( SequoiaDB, getCL ) ;
PHP_METHOD( SequoiaDB, truncate ) ;

PHP_METHOD( SequoiaDB, listDomain ) ;
PHP_METHOD( SequoiaDB, createDomain ) ;
PHP_METHOD( SequoiaDB, getDomain ) ;
PHP_METHOD( SequoiaDB, dropDomain ) ;

PHP_METHOD( SequoiaDB, listGroup ) ;
PHP_METHOD( SequoiaDB, getGroup ) ;
PHP_METHOD( SequoiaDB, createGroup ) ;
PHP_METHOD( SequoiaDB, getGroup ) ;
PHP_METHOD( SequoiaDB, removeGroup ) ;
PHP_METHOD( SequoiaDB, createCataGroup ) ;

PHP_METHOD( SequoiaDB, execSQL ) ;
PHP_METHOD( SequoiaDB, execUpdateSQL ) ;

PHP_METHOD( SequoiaDB, createUser ) ;
PHP_METHOD( SequoiaDB, removeUser ) ;

PHP_METHOD( SequoiaDB, flushConfigure ) ;

PHP_METHOD( SequoiaDB, listProcedure ) ;
PHP_METHOD( SequoiaDB, createJsProcedure ) ;
PHP_METHOD( SequoiaDB, removeProcedure ) ;
PHP_METHOD( SequoiaDB, evalJs ) ;

PHP_METHOD( SequoiaDB, transactionBegin ) ;
PHP_METHOD( SequoiaDB, transactionCommit ) ;
PHP_METHOD( SequoiaDB, transactionRollback ) ;

PHP_METHOD( SequoiaDB, backupOffline ) ;
PHP_METHOD( SequoiaDB, listBackup ) ;
PHP_METHOD( SequoiaDB, removeBackup ) ;

PHP_METHOD( SequoiaDB, listTask ) ;
PHP_METHOD( SequoiaDB, waitTask ) ;
PHP_METHOD( SequoiaDB, cancelTask ) ;

PHP_METHOD( SequoiaDB, setSessionAttr ) ;
PHP_METHOD( SequoiaDB, getSessionAttr ) ;
PHP_METHOD( SequoiaDB, forceSession ) ;

PHP_METHOD( SequoiaDB, analyze ) ;

#endif
