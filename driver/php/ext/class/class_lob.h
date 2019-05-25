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

#ifndef CLASS_LOB_H__
#define CLASS_LOB_H__

#include "php_driver.h"

PHP_METHOD( SequoiaLob, __construct ) ;

PHP_METHOD( SequoiaLob, close ) ;
PHP_METHOD( SequoiaLob, getSize ) ;
PHP_METHOD( SequoiaLob, getCreateTime ) ;
PHP_METHOD( SequoiaLob, getModificationTime ) ;
PHP_METHOD( SequoiaLob, write ) ;
PHP_METHOD( SequoiaLob, read ) ;
PHP_METHOD( SequoiaLob, seek ) ;
PHP_METHOD( SequoiaLob, lock ) ;
PHP_METHOD( SequoiaLob, lockAndSeek ) ;

#endif