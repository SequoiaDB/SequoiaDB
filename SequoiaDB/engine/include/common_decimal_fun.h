/*******************************************************************************
   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#ifndef BSON_COMMON_DECIMAL_FUN_H_
#define BSON_COMMON_DECIMAL_FUN_H_

#include "common_decimal_type.h"

SDB_EXTERN_C_START

SDB_EXPORT void sdb_decimal_init( bson_decimal *decimal );
SDB_EXPORT int  sdb_decimal_init1( bson_decimal *decimal,
                                   int precision, 
                                   int scale ) ;

SDB_EXPORT void sdb_decimal_free( bson_decimal *decimal ) ;

SDB_EXPORT void sdb_decimal_set_zero( bson_decimal *decimal ) ;
SDB_EXPORT int  sdb_decimal_is_zero( const bson_decimal *decimal ) ;

SDB_EXPORT int  sdb_decimal_is_special( const bson_decimal *decimal ) ;

SDB_EXPORT void sdb_decimal_set_nan( bson_decimal *decimal ) ;
SDB_EXPORT int  sdb_decimal_is_nan( const bson_decimal *decimal ) ;

SDB_EXPORT void sdb_decimal_set_min( bson_decimal *decimal ) ;
SDB_EXPORT int  sdb_decimal_is_min( const bson_decimal *decimal ) ;

SDB_EXPORT void sdb_decimal_set_max( bson_decimal *decimal ) ;
SDB_EXPORT int  sdb_decimal_is_max( const bson_decimal *decimal ) ;

SDB_EXPORT int  sdb_decimal_round( bson_decimal *decimal, int rscale ) ;

SDB_EXPORT int     sdb_decimal_to_int( const bson_decimal *decimal ) ;
SDB_EXPORT double  sdb_decimal_to_double( const bson_decimal *decimal ) ;
SDB_EXPORT int64_t sdb_decimal_to_long( const bson_decimal *decimal ) ;

SDB_EXPORT int     sdb_decimal_to_str_get_len( const bson_decimal *decimal, 
                                               int *size ) ;
SDB_EXPORT int     sdb_decimal_to_str( const bson_decimal *decimal,
                                       char *value, 
                                       int value_size ) ;

// the caller is responsible for freeing this decimal( sdb_decimal_free )
SDB_EXPORT int  sdb_decimal_from_int( int value, bson_decimal *decimal ) ;

// the caller is responsible for freeing this decimal( sdb_decimal_free )
SDB_EXPORT int  sdb_decimal_from_long( int64_t value, bson_decimal *decimal) ;

// the caller is responsible for freeing this decimal( sdb_decimal_free )
SDB_EXPORT int  sdb_decimal_from_double( double value, bson_decimal *decimal ) ;

// the caller is responsible for freeing this decimal( sdb_decimal_free )
SDB_EXPORT int  sdb_decimal_from_str( const char *value, bson_decimal *decimal ) ;

SDB_EXPORT int  sdb_decimal_get_typemod( const bson_decimal *decimal,
                                         int *precision, 
                                         int *scale ) ;
SDB_EXPORT int  sdb_decimal_get_typemod2( const bson_decimal *decimal ) ;
SDB_EXPORT int  sdb_decimal_copy( const bson_decimal *source, 
                                  bson_decimal *target ) ;

int sdb_decimal_from_bsonvalue( const char *value, bson_decimal *decimal ) ;

int sdb_decimal_to_jsonstr( const bson_decimal *decimal,
                            char *value, 
                            int value_size ) ;

int sdb_decimal_to_jsonstr_len( int sign, int weight, int dscale, 
                                int typemod, int *size ) ;

SDB_EXPORT int sdb_decimal_cmp( const bson_decimal *left, 
                                const bson_decimal *right ) ;

SDB_EXPORT int sdb_decimal_add( const bson_decimal *left, 
                                const bson_decimal *right,
                                bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_sub( const bson_decimal *left, 
                                const bson_decimal *right,
                                bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_mul( const bson_decimal *left, 
                                const bson_decimal *right,
                                bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_div( const bson_decimal *left, 
                                const bson_decimal *right,
                                bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_abs( bson_decimal *decimal ) ;

SDB_EXPORT int sdb_decimal_ceil( const bson_decimal *decimal, 
                                 bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_floor( const bson_decimal *decimal, 
                                  bson_decimal *result ) ;

SDB_EXPORT int sdb_decimal_mod( const bson_decimal *left,
                                const bson_decimal *right, 
                                bson_decimal *result ) ;

int sdb_decimal_update_typemod( bson_decimal *decimal, int typemod ) ;

int sdb_decimal_is_out_of_precision( bson_decimal *decimal, int typemod ) ;

int sdb_decimal_view_from_bsonvalue( const char *value,
                                     bson_decimal *decimal ) ;

SDB_EXTERN_C_END

/*
   For compatialbe with old version
*/

#define decimal_init             sdb_decimal_init
#define decimal_init1            sdb_decimal_init1
#define decimal_free             sdb_decimal_free
#define decimal_set_zero         sdb_decimal_set_zero
#define decimal_is_zero          sdb_decimal_is_zero
#define decimal_is_special       sdb_decimal_is_special
#define decimal_set_nan          sdb_decimal_set_nan
#define decimal_is_nan           sdb_decimal_is_nan
#define decimal_set_min          sdb_decimal_set_min
#define decimal_is_min           sdb_decimal_is_min
#define decimal_set_max          sdb_decimal_set_max
#define decimal_is_max           sdb_decimal_is_max
#define decimal_round            sdb_decimal_round
#define decimal_to_int           sdb_decimal_to_int
#define decimal_to_double        sdb_decimal_to_double
#define decimal_to_long          sdb_decimal_to_long
#define decimal_to_str_get_len   sdb_decimal_to_str_get_len
#define decimal_to_str           sdb_decimal_to_str

#define decimal_from_int         sdb_decimal_from_int
#define decimal_from_long        sdb_decimal_from_long
#define decimal_from_double      sdb_decimal_from_double
#define decimal_from_str         sdb_decimal_from_str

#define decimal_get_typemod      sdb_decimal_get_typemod
#define decimal_get_typemod2     sdb_decimal_get_typemod2
#define decimal_copy             sdb_decimal_copy
#define decimal_from_bsonvalue   sdb_decimal_from_bsonvalue
#define decimal_to_jsonstr       sdb_decimal_to_jsonstr
#define decimal_to_jsonstr_len   sdb_decimal_to_jsonstr_len

#define decimal_cmp              sdb_decimal_cmp
#define decimal_add              sdb_decimal_add
#define decimal_sub              sdb_decimal_sub
#define decimal_mul              sdb_decimal_mul
#define decimal_div              sdb_decimal_div
#define decimal_abs              sdb_decimal_abs
#define decimal_ceil             sdb_decimal_ceil
#define decimal_floor            sdb_decimal_floor
#define decimal_mod              sdb_decimal_mod
#define decimal_update_typemod   sdb_decimal_update_typemod
#define decimal_is_out_of_precision sdb_decimal_is_out_of_precision

#endif // BSON_COMMON_DECIMAL_FUN_H_
