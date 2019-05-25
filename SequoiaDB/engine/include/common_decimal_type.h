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

#ifndef BSON_COMMON_DECIMAL_TYPE_H_
#define BSON_COMMON_DECIMAL_TYPE_H_

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

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

#if defined (__linux__) || defined (_AIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined (_WIN32)
#include <Windows.h>
#include <WinBase.h>
#endif

#include <stdint.h>

#ifdef __cplusplus
#define SDB_EXTERN_C_START extern "C" {
#define SDB_EXTERN_C_END }
#else
#define SDB_EXTERN_C_START
#define SDB_EXTERN_C_END
#endif

SDB_EXTERN_C_START

#define DECIMAL_SIGN_MASK           0xC000
#define SDB_DECIMAL_POS             0x0000
#define SDB_DECIMAL_NEG             0x4000
#define SDB_DECIMAL_SPECIAL_SIGN    0xC000

#define SDB_DECIMAL_SPECIAL_NAN     0x0000
#define SDB_DECIMAL_SPECIAL_MIN     0x0001
#define SDB_DECIMAL_SPECIAL_MAX     0x0002

#define DECIMAL_DSCALE_MASK         0x3FFF


#define SDB_DECIMAL_DBL_DIG        ( DBL_DIG )



/*
 * Hardcoded precision limit - arbitrary, but must be small enough that
 * dscale values will fit in 14 bits.
 */
#define DECIMAL_MAX_PRECISION       1000
#define DECIMAL_MAX_DISPLAY_SCALE   DECIMAL_MAX_PRECISION
#define DECIMAL_MIN_DISPLAY_SCALE   0
#define DECIMAL_MIN_SIG_DIGITS      16

#define DECIMAL_NBASE               10000
#define DECIMAL_HALF_NBASE          5000
#define DECIMAL_DEC_DIGITS          4     /* decimal digits per NBASE digit */
#define DECIMAL_MUL_GUARD_DIGITS    2     /* these are measured in NBASE digits */
#define DECIMAL_DIV_GUARD_DIGITS    4

typedef struct {
   int typemod;    /* precision & scale define:  
                         precision = (typmod >> 16) & 0xffff
                         scale     = typmod & 0xffff */
   int ndigits;    /* length of digits */
   int sign;       /* the decimal's sign */
   int dscale;     /* display scale */
   int weight;     /* weight of first digit */
   int isOwn;      /* is digits allocated self */
   short *buff ;   /* start of palloc'd space for digits[] */
   short *digits;  /* real decimal data */
} bson_decimal ;

#define DECIMAL_DEFAULT_VALUE \
   { \
      -1,               /* typemode */ \
      0,                /* ndigits */ \
      SDB_DECIMAL_POS,  /* sign */ \
      0,                /* dscale */ \
      0,                /* weight */ \
      0,                /* isOwn */ \
      NULL,             /* buff */ \
      NULL              /* digits */\
   }

#define DECIMAL_HEADER_SIZE  12  /*size + typemod + dscale + weight*/

#pragma pack(1)
typedef struct 
{
  int    size;    //total size of this value

  int    typemod; //precision + scale

  short  dscale;  //sign + dscale

  short  weight;  //weight of this decimal (NBASE=10000)

} __decimal ;

#pragma pack()

SDB_EXTERN_C_END


#endif



