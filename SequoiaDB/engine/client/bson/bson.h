/**
 * @file bson.h
 * @brief BSON Declarations
 */
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
/*    Copyright 2009-2012 10gen Inc.
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

#ifndef BSON_H_
#define BSON_H_
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "common_decimal.h"

#if defined(__GNUC__) || defined(__xlC__)
    #define SDB_EXPORT
#else
    #ifdef SDB_STATIC_BUILD
        #define SDB_EXPORT
    #elif defined(SDB_DLL_BUILD)
        #define SDB_EXPORT __declspec(dllexport)
    #else
        #define SDB_EXPORT __declspec(dllimport)
    #endif
#endif

#ifdef __cplusplus
#define SDB_EXTERN_C_START extern "C" {
#define SDB_EXTERN_C_END }
#else
#define SDB_EXTERN_C_START
#define SDB_EXTERN_C_END
#endif

#include <stdint.h>

#define bson_little_endian64(out, in) ( memcpy(out, in, 8) )
#define bson_little_endian32(out, in) ( memcpy(out, in, 4) )
#define bson_little_endian16(out, in) ( memcpy(out, in, 2) )
#define bson_big_endian64(out, in) ( bson_swap_endian64(out, in) )
#define bson_big_endian32(out, in) ( bson_swap_endian32(out, in) )
#define bson_big_endian16(out, in) ( bson_swap_endian16(out, in) )

SDB_EXTERN_C_START

#define BSON_OK 0
#define BSON_ERROR -1

enum bson_error_t {
    BSON_SIZE_OVERFLOW = 1 /**< Trying to create a BSON object larger than INT_MAX. */
};


enum bson_validity_t {
    BSON_VALID = 0,                 /**< BSON is valid and UTF-8 compliant. */
    BSON_NOT_UTF8 = ( 1<<1 ),       /**< A key or a string is not valid UTF-8. */
    BSON_FIELD_HAS_DOT = ( 1<<2 ),  /**< Warning: key contains '.' character. */
    BSON_FIELD_INIT_DOLLAR = ( 1<<3 ), /**< Warning: key starts with '$' character. */
    BSON_ALREADY_FINISHED = ( 1<<4 )  /**< Trying to modify a finished BSON object. */
};

enum bson_binary_subtype_t {
    BSON_BIN_BINARY = 0, /**< Binary / Generic . */
    BSON_BIN_FUNC = 1, /**< Function . */
    BSON_BIN_BINARY_OLD = 2, /**< Binary(Old) . */
    BSON_BIN_UUID = 3, /**< UUID (Old). */
    BSON_BIN_MD5 = 5, /**< MD5 . */
    BSON_BIN_USER = 128 /**< User defined. */
};

typedef enum {
    BSON_MINKEY = -1, /**< Min key. */
    BSON_EOO = 0, /**< End of object. */
    BSON_DOUBLE = 1, /**< Floatint point. */
    BSON_STRING = 2, /**< UTF-8 string. */
    BSON_OBJECT = 3, /**< Embedded document. */
    BSON_ARRAY = 4, /**< Array. */
    BSON_BINDATA = 5, /**< Binary data. */
    BSON_UNDEFINED = 6, /**< Undefined-Deprecated. */
    BSON_OID = 7, /**< Objectid. */
    BSON_BOOL = 8, /**< BOOlean. */
    BSON_DATE = 9, /**< UTC datetime. */
    BSON_NULL = 10, /**< Null value. */
    BSON_REGEX = 11, /**< Regular expression. */
    BSON_DBREF = 12, /**< DBPointer-Deprecated. */  
    BSON_CODE = 13, /**< JavaScript code. */
    BSON_SYMBOL = 14, /**< Symbol-Deprecated. */
    BSON_CODEWSCOPE = 15, /**< JavaScript code w/scope. */
    BSON_INT = 16, /**< 32-bit integer. */
    BSON_TIMESTAMP = 17, /**< Timestamp. */
    BSON_LONG = 18, /**< 64-bit integer. */

    BSON_DECIMAL = 100, /** decimal type */
    BSON_MAXKEY = 127 /**< Max key. */
} bson_type;

typedef int bson_bool_t;

typedef struct {
    const char *cur;
    bson_bool_t first;
} bson_iterator;

#define BSON_MAX_STACK_SIZE 32
typedef struct {
    char *data;    /**< Pointer to a block of data in this BSON object. */
    char *cur;     /**< Pointer to the current position. */
    int dataSize;  /**< The number of bytes allocated to char *data. */
    bson_bool_t finished; /**< When finished, the BSON object can no longer be modified. */
    int stack[BSON_MAX_STACK_SIZE];        /**< A stack used to keep track of nested BSON elements. */
    char stackType[BSON_MAX_STACK_SIZE];        /**< A stack used to keep track of nested BSON types. */
    int stackPos;         /**< Index of current stack position. */
    int err; /**< Bitfield representing errors or warnings on this buffer */
    char *errstr; /**< A string representation of the most recent error or warning. */
    int ownmem; /**< Own mem represent whether the data is pointing to a our own memory */
} bson;

#pragma pack(1)
typedef union {
    char bytes[12];
    int ints[3];
} bson_oid_t;
#pragma pack()

typedef int64_t bson_date_t; /* milliseconds since epoch UTC */

typedef struct {
    int i; /* increment */
    int t; /* time in seconds */
} bson_timestamp_t;

/* ----------------------------
   READING
   ------------------------------ */

/**
 * Create a BSON object and in initilize it.
 *
 * @return the newly created bson object.
 */
SDB_EXPORT bson* bson_create( void );

/**
 * Free a BSON object.
 *
 * @param b the BSON object.
 */
SDB_EXPORT void  bson_dispose(bson* b);

/**
 * Size of a BSON object.
 *
 * @param b the BSON object.
 *
 * @return the size.
 */
SDB_EXPORT int bson_size( const bson *b );

/**
 * Minimun finished size of an unfinished BSON object.
 *
 * @param b the BSON object.
 *
 * @return the BSON object's minimun finished size.
 */
SDB_EXPORT int bson_buffer_size( const bson *b );

/**
 * Print a string representation of a BSON object.
 *
 * @param b the BSON object to print.
 */
SDB_EXPORT void bson_print( const bson *b );

/**
 * Print a string representation of BSON Iterator to buffer ( without key ).
 *
 * @param i the BSON Iterator to print.
 * @param delChar the string delimiter.
 */
SDB_EXPORT int bson_sprint_iterator ( char **pbuf, int *left, bson_iterator *i,
                                      char delChar ) ;

/**
 * Print a string representation of a BSON object to buffer.
 *
 * @param b the BSON object to print.
 */
SDB_EXPORT int bson_sprint( char *buffer, int bufsize, const bson *b );

/**
 * Estimate the length of a bson iterator
 *
 * @param b the BSON Iterator, note this doesn't include key
 * @return estimated length of the string representation if succeed, 0 if failed
 */
SDB_EXPORT int bson_sprint_length_iterator ( bson_iterator *i ) ;

/**
 * Estimate the length of the string representation of the specified BSON object.
 * The actual space needed will be less than or equal to the estimated value.
 *
 * @param b the BSON object to print.
 * @return  estimated length of the string representation (> 0) if succeed,
 *          0 if failed
 */
SDB_EXPORT int bson_sprint_length( const bson *b );

/**
 * Return a pointer to the raw buffer stored by this bson object.
 *
 * @param b a BSON object
 */
SDB_EXPORT const char *bson_data( const bson *b );

/**
 * Print a string representation of a BSON object.
 *
 * @param bson the raw data to print.
 * @param depth the depth to recurse the object.x
 */

/**
 * Print a string representation of a BSON object to buffer.
 *
 * @param bson the raw data to print.
 * @param depth the depth to recurse the object.x
 * @return 0 for failure
 * @return 1 for success
 */
SDB_EXPORT int bson_sprint_raw ( char **pbuf, int *left, const char *data, int isobj );

/**
 * Estimate the length of the string representation of the specified BSON object.
 * The actual space needed will be less than or equal to the estimated value.
 *
 * @param data the raw data to print.
 * @param isobj 1 if the raw data is a object, 0 if array
 * @return  estimated length of the string representation (> 0) if succeed,
 *          0 if failed
 */
SDB_EXPORT int bson_sprint_length_raw( const char *data, int isobj );

/**
 * Advance a bson_iterator to the named field.
 *
 * @param it the bson_iterator to use.
 * @param obj the BSON object to use.
 * @param name the name of the field to find.
 *
 * @return the type of the found object or BSON_EOO if it is not found.
 */
SDB_EXPORT bson_type bson_find( bson_iterator *it, const bson *obj, const char *name );

/**
 * Create a bson_iterator on the heap.
 *
 * @return the newly created iterator.
 */
SDB_EXPORT bson_iterator* bson_iterator_create( void );

/**
 * Free a bson_iterator which build on the heap.
 */
SDB_EXPORT void bson_iterator_dispose(bson_iterator*);
/**
 * Initialize a bson_iterator.
 *
 * @param i the bson_iterator to initialize.
 * @param bson the BSON object to associate with the iterator.
 */
SDB_EXPORT void bson_iterator_init( bson_iterator *i , const bson *b );

/**
 * Initialize a bson iterator from a const char* buffer. Note
 * that this is mostly used internally.
 *
 * @param i the bson_iterator to initialize.
 * @param buffer the buffer to point to.
 */
SDB_EXPORT void bson_iterator_from_buffer( bson_iterator *i, const char *buffer );

/* more returns true for eoo. best to loop with bson_iterator_next(&it) */
/**
 * Check to see if the bson_iterator has more data.
 *
 * @param i the iterator.
 *
 * @return  returns true if there is more data.
 */
SDB_EXPORT bson_bool_t bson_iterator_more( const bson_iterator *i );

/**
 * Point the iterator at the next BSON object.
 *
 * @param i the bson_iterator.
 *
 * @return the type of the next BSON object.
 */
SDB_EXPORT bson_type bson_iterator_next( bson_iterator *i );

/**
 * Get the type of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the type of the current BSON object.
 */
SDB_EXPORT bson_type bson_iterator_type( const bson_iterator *i );

/**
 * Get the key of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the key of the current BSON object.
 */
SDB_EXPORT const char *bson_iterator_key( const bson_iterator *i );

/**
 * Get the value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
SDB_EXPORT const char *bson_iterator_value( const bson_iterator *i );

/* these convert to the right type (return 0 if non-numeric) */
/**
 * Get the double value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
SDB_EXPORT double bson_iterator_double( const bson_iterator *i );

/**
 * Get the int value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
SDB_EXPORT int bson_iterator_int( const bson_iterator *i );

/**
 * Get the long value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
SDB_EXPORT int64_t bson_iterator_long( const bson_iterator *i );

/**
 * Get the decimal's sign and scale of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_iterator_decimal_scale( const bson_iterator *i, 
                                            int *sign, int *scale ) ;

/**
 * Get the decimal's typemod of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_iterator_decimal_typemod( const bson_iterator *i, 
                                              int *typemod ) ;

/**
 * Get the decimal's weight of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_iterator_decimal_weight( const bson_iterator *i, 
                                             int *weight ) ;

/**
 * Get the decimal's size of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_iterator_decimal_size( const bson_iterator *i, 
                                           int *size ) ;

/**
 * Get the decimal value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_iterator_decimal( const bson_iterator *i, 
                                      bson_decimal *decimal ) ;


/* return the bson timestamp as a whole or in parts */
/**
 * Get the timestamp value of the BSON object currently pointed to by
 * the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
SDB_EXPORT bson_timestamp_t bson_iterator_timestamp( const bson_iterator *i );
SDB_EXPORT int bson_iterator_timestamp_time( const bson_iterator *i );
SDB_EXPORT int bson_iterator_timestamp_increment( const bson_iterator *i );

/**
 * Get the boolean value of the BSON object currently pointed to by
 * the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
/* false: boolean false, 0 in any type, or null */
/* true: anything else (even empty strings and objects) */
SDB_EXPORT bson_bool_t bson_iterator_bool( const bson_iterator *i );

/**
 * Get the double value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
/* these assume you are using the right type */
double bson_iterator_double_raw( const bson_iterator *i );

/**
 * Get the int value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
int bson_iterator_int_raw( const bson_iterator *i );

/**
 * Get the long value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
int64_t bson_iterator_long_raw( const bson_iterator *i );

/**
 * Get the bson_bool_t value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
bson_bool_t bson_iterator_bool_raw( const bson_iterator *i );

/**
 * Get the bson_oid_t value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
SDB_EXPORT bson_oid_t *bson_iterator_oid( const bson_iterator *i );

/**
 * Get the string value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
/* these can also be used with bson_code and bson_symbol*/
SDB_EXPORT const char *bson_iterator_string( const bson_iterator *i );

/**
 * Get the string length of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the length of the current BSON object.
 */
int bson_iterator_string_len( const bson_iterator *i );

/**
 * Get the code value of the BSON object currently pointed to by the
 * iterator. Works with bson_code, bson_codewscope, and BSON_STRING
 * returns NULL for everything else.
 *
 * @param i the bson_iterator
 *
 * @return the code value of the current BSON object.
 */
/* works with bson_code, bson_codewscope, and BSON_STRING */
/* returns NULL for everything else */
SDB_EXPORT const char *bson_iterator_code( const bson_iterator *i );

/**
 * Calls bson_empty on scope if not a bson_codewscope
 *
 * @param i the bson_iterator.
 * @param scope the bson scope.
 */
/* calls bson_empty on scope if not a bson_codewscope */
SDB_EXPORT void bson_iterator_code_scope( const bson_iterator *i, bson *scope );

/**
 * Get the date value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the date value of the current BSON object.
 */
/* both of these only work with bson_date */
SDB_EXPORT bson_date_t bson_iterator_date( const bson_iterator *i );

/**
 * Get the time value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the time value of the current BSON object.
 */
SDB_EXPORT time_t bson_iterator_time_t( const bson_iterator *i );

/**
 * Get the length of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the length of the current BSON binary object.
 */
SDB_EXPORT int bson_iterator_bin_len( const bson_iterator *i );

/**
 * Get the type of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the type of the current BSON binary object.
 */
SDB_EXPORT char bson_iterator_bin_type( const bson_iterator *i );

/**
 * Get the value of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON binary object.
 */
SDB_EXPORT const char *bson_iterator_bin_data( const bson_iterator *i );

/**
 * Get the value of the BSON regex object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON regex object.
 */
SDB_EXPORT const char *bson_iterator_regex( const bson_iterator *i );

/**
 * Get the options of the BSON regex object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 *
 * @return the options of the current BSON regex object.
 */
SDB_EXPORT const char *bson_iterator_regex_opts( const bson_iterator *i );

/**
 * Get the DB name of the BSON DBRef object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 *
 * @return the DB name of the current BSON DBRef object.
 */
SDB_EXPORT const char *bson_iterator_dbref( const bson_iterator *i );

/**
 * Get the DB OID of the BSON DBRef object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 *
 * @return the DB OID of the current BSON DBRef object.
 */
SDB_EXPORT bson_oid_t *bson_iterator_dbref_oid( const bson_iterator *i );

/* these work with BSON_OBJECT and BSON_ARRAY */
/**
 * Get the BSON subobject currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 * @param sub the BSON subobject destination.
 */
SDB_EXPORT void bson_iterator_subobject( const bson_iterator *i, bson *sub );

/**
 * Get a bson_iterator that on the BSON subobject.
 *
 * @param i the bson_iterator.
 * @param sub the iterator to point at the BSON subobject.
 */
SDB_EXPORT void bson_iterator_subiterator( const bson_iterator *i, bson_iterator *sub );

/* str must be at least 24 hex chars + null byte */
/**
 * Create a bson_oid_t from a string.
 *
 * @param oid the bson_oid_t destination.
 * @param str a null terminated string comprised of at least 24 hex chars.
 */
SDB_EXPORT void bson_oid_from_string( bson_oid_t *oid, const char *str );

/**
 * Create a string representation of the bson_oid_t.
 *
 * @param oid the bson_oid_t source.
 * @param str the string representation destination.
 */
SDB_EXPORT void bson_oid_to_string( const bson_oid_t *oid, char *str );

/**
 * Create a bson_oid object.
 *
 * @param oid the destination for the newly created bson_oid_t.
 */
SDB_EXPORT void bson_oid_gen( bson_oid_t *oid );

/**
 * Set a function to be used to generate the second four bytes
 * of an object id.
 *
 * @param func a pointer to a function that returns an int.
 */
SDB_EXPORT void bson_set_oid_fuzz( int ( *func )( void ) );

/**
 * Set a function to be used to generate the incrementing part
 * of an object id (last four bytes). If you need thread-safety
 * in generating object ids, you should set this function.
 *
 * @param func a pointer to a function that returns an int.
 */
SDB_EXPORT void bson_set_oid_inc( int ( *func )( void ) );

/**
 * Get the time a bson_oid_t was created.
 *
 * @param oid the bson_oid_t.
 */
SDB_EXPORT time_t bson_oid_generated_time( bson_oid_t *oid ); /* Gives the time the OID was created */

/* ----------------------------
   BUILDING
   ------------------------------ */

/**
 *  Initialize a new bson object. If not created
 *  with bson_new, you must initialize each new bson
 *  object using this function.
 *
 *  @note When finished, you must pass the bson object to
 *      bson_destroy( ).
 */
SDB_EXPORT void bson_init( bson *b );

/**
 * Initialize a BSON object, and point its data
 * pointer to the provided char*.
 *
 * @param b the BSON object to initialize.
 * @param data the raw BSON data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_init_data( bson *b , const char *data );

/**
 * Initialize a BSON object, and point its data
 * pointer to the provided char*, then finish
 * building this BSON object.
 *
 * @param b the BSON object to initialize.
 * @param data the raw BSON data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_init_finished_data( bson *b, const char *data ) ;

/**
 * Initialize a BSON object, and set its
 * buffer to the given size.
 *
 * @param b the BSON object to initialize.
 * @param size the initial size of the buffer.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT void bson_init_size( bson *b, int size );

/**
 * Grow a bson object.
 *
 * @param b the bson to grow.
 * @param bytesNeeded the additional number of bytes needed.
 *
 * @return BSON_OK or BSON_ERROR with the bson error object set.
 *   Exits if allocation fails.
 */
int bson_ensure_space( bson *b, const int bytesNeeded );

/**
 * Finalize a bson object.
 *
 * @param b the bson object to finalize.
 *
 * @return the standard error code. To deallocate memory,
 *   call bson_destroy on the bson object.
 */
SDB_EXPORT int bson_finish( bson *b );

/**
 * Destroy a bson object.
 *
 * @param b the bson object to destroy.
 *
 */
SDB_EXPORT void bson_destroy( bson *b );

/**
 * Returns a pointer to a static empty BSON object.
 *
 * @param obj the BSON object to initialize.
 *
 * @return the empty initialized BSON object.
 */
/* returns pointer to static empty bson object */
SDB_EXPORT bson *bson_empty( bson *obj );

/**
 * Check BSON object is empty or not.
 *
 * @param obj the BSON object.
 *
 * @return true or false.
 */
SDB_EXPORT bson_bool_t bson_is_empty( bson *obj );

/**
 * Make a complete copy of the a BSON object.
 * The source bson object must be in a finished
 * state; otherwise, the copy will fail.
 *
 * @param out the copy destination BSON object.
 * @param in the copy source BSON object.
 */
SDB_EXPORT int bson_copy( bson *out, const bson *in ); /* puts data in new buffer. NOOP if out==NULL */

/**
 * Append a previously created bson_oid_t to a bson object.
 *
 * @param b the bson to append to.
 * @param name the key for the bson_oid_t.
 * @param oid the bson_oid_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_oid( bson *b, const char *name, const bson_oid_t *oid );

/**
 * Append a bson_oid_t to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the bson_oid_t.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_new_oid( bson *b, const char *name );

/**
 * Append an int to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the int.
 * @param i the int to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_int( bson *b, const char *name, const int i );

/**
 * Append an long to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the long.
 * @param i the long to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_long( bson *b, const char *name, const int64_t i );

/**
 * Append an decimal to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the decimal.
 * @param decimal the decimal to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_decimal( bson *b, const char *name, 
                                    const bson_decimal *decimal ) ;

/**
 * Append an decimal to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the decimal.
 * @param value the string format of the decimal to append.
 * @param precision, the precision of decimal
 * @param scale, the scale of decimal
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_decimal2( bson *b, const char *name, 
                                     const char *value, int precision, 
                                     int scale ) ;
/**
 * Append an decimal to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the decimal.
 * @param value the string format of the decimal to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_decimal3( bson *b, const char *name, 
                                     const char *value ) ;

/**
 * Append an double to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the double.
 * @param d the double to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_double( bson *b, const char *name, const double d );

/**
 * Append a string to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the string.
 * @param str the string to append.
 *
 * @return BSON_OK or BSON_ERROR.
*/
SDB_EXPORT int bson_append_string( bson *b, const char *name, const char *str );

/**
 * Append len bytes of a string to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the string.
 * @param str the string to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_string_n( bson *b, const char *name, const char *str, int len );

/**
 * Append a symbol to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the symbol.
 * @param str the symbol to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_symbol( bson *b, const char *name, const char *str );

/**
 * Append len bytes of a symbol to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the symbol.
 * @param str the symbol to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_symbol_n( bson *b, const char *name, const char *str, int len );

/**
 * Append code to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the code to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_code( bson *b, const char *name, const char *str );

/**
 * Append len bytes of code to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the code to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_code_n( bson *b, const char *name, const char *str, int len );

/**
 * Append code to a bson with scope.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the string to append.
 * @param scope a BSON object containing the scope.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_code_w_scope( bson *b, const char *name, const char *code, const bson *scope );

/**
 * Append len bytes of code to a bson with scope.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the string to append.
 * @param len the number of bytes from str to append.
 * @param scope a BSON object containing the scope.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_code_w_scope_n( bson *b, const char *name, const char *code, int size, const bson *scope );

/**
 * Append binary data to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the data.
 * @param type the binary data type.
 * @param str the binary data.
 * @param len the length of the data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_binary( bson *b, const char *name, char type, const char *str, int len );

/**
 * Append a bson_bool_t to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the boolean value.
 * @param v the bson_bool_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_bool( bson *b, const char *name, const bson_bool_t v );

/**
 * Append a null value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the null value.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_null( bson *b, const char *name );

/**
 * Append an undefined value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the undefined value.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_undefined( bson *b, const char *name );

/**
 * Append a regex value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the regex value.
 * @param pattern the regex pattern to append.
 * @param the regex options.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_regex( bson *b, const char *name, const char *pattern, const char *opts );

/**
 * Append a minkey to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the minkey.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_minkey( bson *b, const char *name ) ;

/**
 * Append a maxkey to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the maxkey.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_maxkey( bson *b, const char *name ) ;

/**
 * Append bson data to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the bson data.
 * @param bson the bson object to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_bson( bson *b, const char *name, const bson *bson );

/**
 * Append array data to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the bson data.
 * @param bson the bson array to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_array( bson *b, const char *name, const bson *bson );

/**
 * Append a BSON element to a bson from the current point of an iterator.
 *
 * @param b the bson to append to.
 * @param name_or_null the key for the BSON element, or NULL.
 * @param elem the bson_iterator.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_element( bson *b, const char *name_or_null, const bson_iterator *elem );

/**
 * Append all the elements of a bson to another bson.
 *
 * @param dst the destination bson.
 * @param src the source bson.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_elements( bson *dst, const bson *src );

/**
 * Append a bson_timestamp_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the timestampe value.
 * @param ts the bson_timestamp_t value to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_timestamp( bson *b, const char *name, bson_timestamp_t *ts );
SDB_EXPORT int bson_append_timestamp2( bson *b, const char *name, int time, int increment );

/* these both append a bson_date */
/**
 * Append a bson_date_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the date value.
 * @param millis the bson_date_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_date( bson *b, const char *name, bson_date_t millis );

/**
 * Append a time_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the date value.
 * @param secs the time_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_time_t( bson *b, const char *name, time_t secs );

/**
 * Start appending a new object to a bson.
 *
 * @param b the bson to append to.
 * @param name the name of the new object.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_start_object( bson *b, const char *name );

/**
 * Start appending a new array to a bson.
 *
 * @param b the bson to append to.
 * @param name the name of the new array.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_start_array( bson *b, const char *name );

/**
 * Finish appending a new object or array to a bson.
 *
 * @param b the bson to append to.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_finish_object( bson *b );

/**
 * Finish appending a new object or array to a bson. This
 * is simply an alias for bson_append_finish_object.
 *
 * @param b the bson to append to.
 *
 * @return BSON_OK or BSON_ERROR.
 */
SDB_EXPORT int bson_append_finish_array( bson *b );

void bson_numstr( char *str, int i );

void bson_incnumstr( char *str );

/* Error handling and standard library function over-riding. */
/* -------------------------------------------------------- */

/* bson_err_handlers shouldn't return!!! */
typedef void( *bson_err_handler )( const char *errmsg );

typedef int (*bson_printf_func)( const char *, ... );
typedef int (*bson_fprintf_func)( FILE *, const char *, ... );
typedef int (*bson_sprintf_func)( char *, const char *, ... );

extern void *( *bson_malloc_func )( size_t );
extern void *( *bson_realloc_func )( void *, size_t );
extern void ( *bson_free_func )( void * );

extern bson_printf_func bson_printf;
extern bson_fprintf_func bson_fprintf;
extern bson_sprintf_func bson_sprintf;
extern bson_printf_func bson_errprintf;

SDB_EXPORT void bson_free( void *ptr );

/**
 * Allocates memory and checks return value, exiting fatally if malloc() fails.
 *
 * @param size bytes to allocate.
 *
 * @return a pointer to the allocated memory.
 *
 * @sa malloc(3)
 */
SDB_EXPORT void *bson_malloc( int size );

/**
 * Changes the size of allocated memory and checks return value,
 * exiting fatally if realloc() fails.
 *
 * @param ptr pointer to the space to reallocate.
 * @param size bytes to allocate.
 *
 * @return a pointer to the allocated memory.
 *
 * @sa realloc()
 */
void *bson_realloc( void *ptr, int size );

/**
 * Set a function for error handling.
 *
 * @param func a bson_err_handler function.
 *
 * @return the old error handling function, or NULL.
 */
SDB_EXPORT bson_err_handler set_bson_err_handler( bson_err_handler func );

/* does nothing if ok != 0 */
/**
 * Exit fatally.
 *
 * @param ok exits if ok is equal to 0.
 */
void bson_fatal( int ok );

/**
 * Exit fatally with an error message.
  *
 * @param ok exits if ok is equal to 0.
 * @param msg prints to stderr before exiting.
 */
void bson_fatal_msg( int ok, const char *msg );

/**
 * Invoke the error handler, but do not exit.
 *
 * @param b the buffer object.
 */
void bson_builder_error( bson *b );

/**
 * Cast an int64_t to double. This is necessary for embedding in
 * certain environments.
 *
 */
SDB_EXPORT double bson_int64_to_double( int64_t i64 );

SDB_EXPORT void bson_swap_endian32( void *outp, const void *inp );
SDB_EXPORT void bson_swap_endian64( void *outp, const void *inp );

SDB_EXPORT bson_bool_t bson_is_inf( double d, int *pSign ) ;

/**
 * when this value is not zero, the bson_print
 * method will
 * show the string which is the same with that 
 * shows in sdb shell.
 */
SDB_EXPORT void bson_set_js_compatibility(int compatible);

/**
 * get whether bson_print method will show the string
 * which is the same with that shows in sdb shell or not
 */
SDB_EXPORT int bson_get_js_compatibility();

SDB_EXTERN_C_END
#endif