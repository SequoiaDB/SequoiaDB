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

#ifndef CLASS_GROUP_H__
#define CLASS_GROUP_H__

#include "php_driver.h"

PHP_METHOD( SequoiaGroup, __construct ) ;

PHP_METHOD( SequoiaGroup, isCatalog ) ;
PHP_METHOD( SequoiaGroup, getName ) ;
PHP_METHOD( SequoiaGroup, start ) ;
PHP_METHOD( SequoiaGroup, stop ) ;
PHP_METHOD( SequoiaGroup, reelect ) ;

PHP_METHOD( SequoiaGroup, getNodeNum ) ;
PHP_METHOD( SequoiaGroup, getDetail ) ;
PHP_METHOD( SequoiaGroup, getMaster ) ;
PHP_METHOD( SequoiaGroup, getSlave ) ;
PHP_METHOD( SequoiaGroup, getNode ) ;
PHP_METHOD( SequoiaGroup, createNode ) ;
PHP_METHOD( SequoiaGroup, removeNode ) ;
PHP_METHOD( SequoiaGroup, attachNode ) ;
PHP_METHOD( SequoiaGroup, detachNode ) ;


#endif