/*    Copyright 2012 SequoiaDB Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
 
/*
 * Copyright 2009-2012 10gen, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BSON_COMMON_DECIMAL_FUN_H_
#define BSON_COMMON_DECIMAL_FUN_H_

#include "common_decimal_type.h"

SDB_EXTERN_C_START

SDB_EXPORT void decimal_init( bson_decimal *decimal );
SDB_EXPORT int decimal_init1( bson_decimal *decimal, int precision, 
                              int scale ) ;

SDB_EXPORT void decimal_free( bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_zero( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_zero( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_is_special( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_nan( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_nan( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_min( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_min( const bson_decimal *decimal ) ;

SDB_EXPORT void decimal_set_max( bson_decimal *decimal ) ;
SDB_EXPORT int decimal_is_max( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_round( bson_decimal *decimal, int rscale ) ;

SDB_EXPORT int decimal_to_int( const bson_decimal *decimal ) ;
SDB_EXPORT double decimal_to_double( const bson_decimal *decimal ) ;
SDB_EXPORT int64_t decimal_to_long( const bson_decimal *decimal ) ;

SDB_EXPORT int decimal_to_str_get_len( const bson_decimal *decimal, 
                                       int *size ) ;
SDB_EXPORT int decimal_to_str( const bson_decimal *decimal, char *value, 
                               int value_size ) ;

SDB_EXPORT int decimal_from_int( int value, bson_decimal *decimal ) ;

SDB_EXPORT int decimal_from_long( int64_t value, bson_decimal *decimal) ;

SDB_EXPORT int decimal_from_double( double value, bson_decimal *decimal ) ;

SDB_EXPORT int decimal_from_str( const char *value, bson_decimal *decimal ) ;

SDB_EXPORT int decimal_get_typemod( const bson_decimal *decimal, int *precision, 
                                    int *scale ) ;
SDB_EXPORT int decimal_get_typemod2( const bson_decimal *decimal ) ;
SDB_EXPORT int decimal_copy( const bson_decimal *source, 
                             bson_decimal *target ) ;

int decimal_from_bsonvalue( const char *value, bson_decimal *decimal ) ;

int decimal_to_jsonstr( const bson_decimal *decimal, char *value, 
                        int value_size ) ;

int decimal_to_jsonstr_len( int sign, int weight, int dscale, 
                            int typemod, int *size ) ;

SDB_EXPORT int decimal_cmp( const bson_decimal *left, 
                            const bson_decimal *right ) ;

SDB_EXPORT int decimal_add( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_sub( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_mul( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_div( const bson_decimal *left, 
                            const bson_decimal *right, bson_decimal *result ) ;

SDB_EXPORT int decimal_abs( bson_decimal *decimal ) ;

SDB_EXPORT int decimal_ceil( const bson_decimal *decimal, 
                             bson_decimal *result ) ;

SDB_EXPORT int decimal_floor( const bson_decimal *decimal, 
                              bson_decimal *result ) ;

SDB_EXPORT int decimal_mod( const bson_decimal *left, const bson_decimal *right, 
                            bson_decimal *result ) ;

int decimal_update_typemod( bson_decimal *decimal, int typemod ) ;

int decimal_is_out_of_precision( bson_decimal *decimal, int typemod ) ;


SDB_EXTERN_C_END

#endif