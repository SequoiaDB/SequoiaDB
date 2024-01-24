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

#ifndef CLASS_CL_H__
#define CLASS_CL_H__

#include "php_driver.h"

PHP_METHOD( SequoiaCL, __construct ) ;

//cl
PHP_METHOD( SequoiaCL, drop ) ;
PHP_METHOD( SequoiaCL, alter ) ;
PHP_METHOD( SequoiaCL, enableSharding ) ;
PHP_METHOD( SequoiaCL, disableSharding ) ;
PHP_METHOD( SequoiaCL, enableCompression ) ;
PHP_METHOD( SequoiaCL, disableCompression ) ;
PHP_METHOD( SequoiaCL, setAttributes ) ;
PHP_METHOD( SequoiaCL, split ) ;
PHP_METHOD( SequoiaCL, splitAsync ) ;
PHP_METHOD( SequoiaCL, getFullName ) ;
PHP_METHOD( SequoiaCL, getCSName ) ;
//e.g. Rename getCollectionName
PHP_METHOD( SequoiaCL, getName ) ;
PHP_METHOD( SequoiaCL, attachCL ) ;
PHP_METHOD( SequoiaCL, detachCL ) ;
PHP_METHOD( SequoiaCL, createAutoIncrement ) ;
PHP_METHOD( SequoiaCL, dropAutoIncrement ) ;

//record
PHP_METHOD( SequoiaCL, insert ) ;
PHP_METHOD( SequoiaCL, bulkInsert ) ;
PHP_METHOD( SequoiaCL, remove ) ;
PHP_METHOD( SequoiaCL, update ) ;
PHP_METHOD( SequoiaCL, upsert ) ;
PHP_METHOD( SequoiaCL, find ) ;
PHP_METHOD( SequoiaCL, findAndUpdate ) ;
PHP_METHOD( SequoiaCL, findAndRemove ) ;
PHP_METHOD( SequoiaCL, explain ) ;
PHP_METHOD( SequoiaCL, count ) ;
PHP_METHOD( SequoiaCL, aggregate ) ;

//不需要实现
//PHP_METHOD( SequoiaCL, getDataBlocks ) ;
//PHP_METHOD( SequoiaCL, getQueryMeta ) ;

//index
PHP_METHOD( SequoiaCL, createIndex ) ;
PHP_METHOD( SequoiaCL, dropIndex ) ;
PHP_METHOD( SequoiaCL, getIndex ) ;
PHP_METHOD( SequoiaCL, getIndexes ) ;
PHP_METHOD( SequoiaCL, getIndexInfo ) ;
PHP_METHOD( SequoiaCL, getIndexStat ) ;
PHP_METHOD( SequoiaCL, createIdIndex ) ;
PHP_METHOD( SequoiaCL, dropIdIndex ) ;


//lob
PHP_METHOD( SequoiaCL, openLob ) ;
PHP_METHOD( SequoiaCL, removeLob ) ;
PHP_METHOD( SequoiaCL, truncateLob ) ;
PHP_METHOD( SequoiaCL, listLob ) ;
PHP_METHOD( SequoiaCL, listLobPieces ) ;
PHP_METHOD( SequoiaCL, createLobID ) ;

#endif
