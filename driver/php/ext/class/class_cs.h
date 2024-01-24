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

#ifndef CLASS_CS_H__
#define CLASS_CS_H__

#include "php_driver.h"

PHP_METHOD( SequoiaCS, __construct ) ;

//cs
PHP_METHOD( SequoiaCS, drop ) ;
PHP_METHOD( SequoiaCS, getName ) ;

//cl
//e.g. Rename selectCollection
PHP_METHOD( SequoiaCS, selectCL ) ;
PHP_METHOD( SequoiaCS, createCL ) ;
PHP_METHOD( SequoiaCS, getCL ) ;
PHP_METHOD( SequoiaCS, listCL ) ;
//e.g. Rename dropCollection
PHP_METHOD( SequoiaCS, dropCL ) ;
PHP_METHOD( SequoiaCS, renameCL ) ;
PHP_METHOD( SequoiaCS, alter ) ;
PHP_METHOD( SequoiaCS, setDomain ) ;
PHP_METHOD( SequoiaCS, getDomainName ) ;
PHP_METHOD( SequoiaCS, removeDomain ) ;
PHP_METHOD( SequoiaCS, enableCapped ) ;
PHP_METHOD( SequoiaCS, disableCapped ) ;
PHP_METHOD( SequoiaCS, setAttributes ) ;
PHP_METHOD( SequoiaCS, setAttributes ) ;

#endif
