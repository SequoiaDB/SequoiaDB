/* common_decimal.c */
/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include <float.h>
#include "common_decimal.h"
#include "bson.h"

#if defined (__linux__) || defined (_AIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined (_WIN32)
#include <Windows.h>
#include <WinBase.h>
#endif


static const int round_powers[4] = { 0, 1000, 100, 10 } ;

static short const_one_data[1] = { 1 } ;
static bson_decimal const_one =
                       { -1, 1, SDB_DECIMAL_POS, 0, 0, 0, NULL, const_one_data } ;

static const char decimal_str_start[]     = "{ \"$decimal\": \"" ;
static const char decimal_str_end[]       = "\"" ;
static const char precision_str_start[]   = ", \"$precision\": [" ;
static const char precision_str_end[]     = "]" ;
static const char json_str_end[]          = " }" ;

#define decimal_free_safe(digits) \
   do { \
      if ( ( digits ) != NULL ) \
      {\
         bson_free( digits ) ; \
         ( digits ) = NULL ; \
      }\
   } while ( 0 )

#define Max(x, y)          ((x) > (y) ? (x) : (y))
#define Min(x, y)          ((x) < (y) ? (x) : (y))

#define SDB_DECIMAL_MAX_DWEIGHT     131072
#define SDB_DECIMAL_MAX_DSCALE      16383

/// SDB_DECIMAL_MAX_DWEIGHT/SDB_DECIMAL_DEC_DIGITS
#define SDB_DECIMAL_MAX_WEIGHT      32768

static void _decimal_free_buff( bson_decimal *decimal ) ;
static void _decimal_strip( bson_decimal *decimal ) ;
static int _decimal_apply_typmod( bson_decimal *decimal, int typmod ) ;
static void _decimal_set_nan( bson_decimal *decimal ) ;
static int _decimal_is_digit( const char value ) ;
static void _decimal_trunc( bson_decimal *decimal, int rscale ) ;
static int _decimal_sub_abs( const bson_decimal *left,
                             const bson_decimal *right, bson_decimal *result ) ;
static int _decimal_add_abs( const bson_decimal *left,
                             const bson_decimal *right, bson_decimal *result ) ;
static int _decimal_cmp_abs( const bson_decimal *left,
                             const bson_decimal *right ) ;

static int _decimal_sub( const bson_decimal *left, const bson_decimal *right,
                         bson_decimal *result ) ;
static int _decimal_add( const bson_decimal *left, const bson_decimal *right,
                         bson_decimal *result ) ;
static int _decimal_mul( const bson_decimal *left, const bson_decimal *right,
                         bson_decimal *result, int dscale ) ;
static int _decimal_get_div_scale( const bson_decimal *left,
                                   const bson_decimal *right ) ;

static int _decimal_div( const bson_decimal *left, const bson_decimal *right,
                         bson_decimal *result, int rscale, int isRound ) ;

static int _decimal_sprint_len( int sign, int weight, int scale ) ;

static int _decimal_alloc( bson_decimal *decimal, int ndigits ) ;

static int _decimal_is_out_of_bound( bson_decimal *decimal ) ;

void _decimal_free_buff( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   if ( decimal->isOwn )
   {
      decimal_free_safe( decimal->buff ) ;
      decimal->isOwn = 0 ;
   }

   decimal->buff   = NULL ;
   decimal->digits = NULL ;
}

/*
 * _decimal_strip
 *
 * Strip any leading and trailing zeroes from a numeric variable
 */
void _decimal_strip( bson_decimal *decimal )
{
   short *digits = NULL ;
   int ndigits = 0 ;

   if ( NULL == decimal || !decimal->isOwn )
   {
      return ;
   }

   digits = decimal->digits ;
   ndigits = decimal->ndigits ;

   /* Strip leading zeroes */
   while ( ndigits > 0 && *digits == 0 )
   {
      digits++ ;
      decimal->weight-- ;
      ndigits-- ;
   }

   /* Strip trailing zeroes */
   while ( ndigits > 0 && digits[ ndigits - 1 ] == 0 )
   {
      ndigits-- ;
   }

   /* If it's zero, normalize the sign and weight */
   if ( ndigits == 0 )
   {
      decimal->sign   = SDB_DECIMAL_POS ;
      decimal->weight = 0 ;
   }

   decimal->digits  = digits ;
   decimal->ndigits = ndigits ;
}

int _decimal_apply_typmod( bson_decimal *decimal, int typmod )
{
   int precision = 0 ;
   int scale     = 0 ;
   int maxdigits = 0 ;
   int ddigits   = 0 ;
   int i         = 0 ;
   int rc        = 0 ;

   /* Do nothing if we have a default typmod (-1) */
   if ( typmod == -1 )
   {
      goto done ;
   }

   precision = ( typmod >> 16 ) & 0xffff ;
   scale     = typmod & 0xffff ;
   maxdigits = precision - scale ;

   /* Round to target scale (and set var->dscale) */
   rc = sdb_decimal_round( decimal, scale ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   /*
   * Check for overflow - note we can't do this before rounding, because
   * rounding could raise the weight.  Also note that the var's weight could
   * be inflated by leading zeroes, which will be stripped before storage
   * but perhaps might not have been yet. In any case, we must recognize a
   * true zero, whose weight doesn't mean anything.
   */
   ddigits = ( decimal->weight + 1) * SDB_DECIMAL_DEC_DIGITS ;
   if ( ddigits > maxdigits )
   {
      /* Determine true weight; and check for all-zero result */
      for ( i = 0 ; i < decimal->ndigits ; i++ )
      {
         short dig = decimal->digits[i] ;
         if ( dig )
         {
            /* Adjust for any high-order decimal zero digits */
            if ( dig < 10 )
            {
               ddigits -= 3 ;
            }
            else if ( dig < 100 )
            {
               ddigits -= 2 ;
            }
            else if (dig < 1000)
            {
               ddigits -= 1 ;
            }

            if ( ddigits > maxdigits )
            {
               rc = -10 ;
               goto error ;
            }
            break ;
         }

         ddigits -= SDB_DECIMAL_DEC_DIGITS ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

void _decimal_set_nan( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   sdb_decimal_free( decimal ) ;

   decimal->ndigits = 0 ;
   decimal->weight  = 0 ;
   decimal->sign    = SDB_DECIMAL_SPECIAL_SIGN ;
   decimal->dscale  = SDB_DECIMAL_SPECIAL_NAN ;
}

int _decimal_is_digit( const char value )
{
   if ( value >= '0' && value <= '9' )
   {
      return 1 ;
   }

   return 0 ;
}

/*
 * _decimal_trunc
 *
 * Truncate (towards zero) the value of a variable at rscale decimal digits
 * after the decimal point.  NOTE: we allow rscale < 0 here, implying
 * truncation before the decimal point.
 */
void _decimal_trunc( bson_decimal *decimal, int rscale )
{
   int di ;
   int ndigits ;
   if ( NULL == decimal || decimal->isOwn != 1 )
   {
      return ;
   }

   decimal->dscale = rscale ;

   /* decimal digits wanted */
   di = ( decimal->weight + 1 ) * SDB_DECIMAL_DEC_DIGITS + rscale ;

   /*
    * If di <= 0, the value loses all digits.
    */
   if ( di <= 0 )
   {
      decimal->ndigits = 0 ;
      decimal->weight  = 0 ;
      decimal->sign    = SDB_DECIMAL_POS ;
   }
   else
   {
      /* NBASE digits wanted */
      ndigits = (di + SDB_DECIMAL_DEC_DIGITS - 1) / SDB_DECIMAL_DEC_DIGITS ;

      if ( ndigits <= decimal->ndigits )
      {
         decimal->ndigits = ndigits;

         /* 0, or number of decimal digits to keep in last NBASE digit */
         di %= SDB_DECIMAL_DEC_DIGITS ;
         if (di > 0)
         {
            /* Must truncate within last NBASE digit */
            short *digits = decimal->digits ;
            int extra     = 0 ;
            int pow10     = 0 ;

            pow10 = round_powers[ di ] ;
            extra = digits[--ndigits] % pow10 ;
            digits[ndigits] -= extra ;
         }
      }
   }
}

/*
 * _decimal_sub_abs()
 *
 * Subtract the absolute value of right from the absolute value of left
 * and store in result. result might point to one of the operands
 * without danger.
 *
 * ABS(left) MUST BE GREATER OR EQUAL ABS(right) !!!
 */
int _decimal_sub_abs( const bson_decimal *left, const bson_decimal *right,
                      bson_decimal *result )
{
   short *res_buf    = NULL ;
   short *res_digits = NULL ;
   int res_ndigits   = 0 ;
   int res_weight    = 0 ;
   int res_rscale    = 0 ;
   int rscale1       = 0 ;
   int rscale2       = 0 ;
   int res_dscale    = 0 ;
   int i      = 0 ;
   int i1     = 0 ;
   int i2     = 0 ;
   int borrow = 0 ;
   int rc     = 0 ;

   /* copy these values into local vars for speed in inner loop */
   int leftNdigits    = left->ndigits ;
   short *leftDigits  = left->digits ;

   int rightNdigits   = right->ndigits ;
   short *rightDigits = right->digits ;

   res_weight = left->weight ;
   res_dscale = Max( left->dscale, right->dscale ) ;

   /* Note: here we are figuring rscale in base-NBASE digits */
   rscale1     = left->ndigits - left->weight - 1 ;
   rscale2     = right->ndigits - right->weight - 1 ;
   res_rscale  = Max( rscale1, rscale2 ) ;

   res_ndigits = res_rscale + res_weight + 1 ;
   if ( res_ndigits <= 0 )
   {
      res_ndigits = 1 ;
   }

   res_buf = bson_malloc( ( res_ndigits + 1 ) * sizeof( short ) ) ;
   if ( NULL == res_buf )
   {
      rc = -2 ;
      goto error ;
   }

   res_buf[0] = 0 ;          /* spare digit for later rounding */
   res_digits = res_buf + 1 ;

   // align the decimal point. ex. left=1.2  right=341.2134
   //                              ==>  1.2000  ==>341.2134
   i1 = res_rscale + left->weight + 1 ;
   i2 = res_rscale + right->weight + 1 ;
   for ( i = res_ndigits - 1 ; i >= 0 ; i-- )
   {
      i1-- ;
      i2-- ;
      if ( i1 >= 0 && i1 < leftNdigits )
      {
         borrow += leftDigits[i1] ;
      }

      if (i2 >= 0 && i2 < rightNdigits)
      {
         borrow -= rightDigits[i2] ;
      }

      if ( borrow < 0 )
      {
         res_digits[i] = borrow + SDB_DECIMAL_NBASE ;
         borrow = -1 ;
      }
      else
      {
         res_digits[i] = borrow ;
         borrow = 0 ;
      }
   }

   if ( borrow != 0 )
   {
      //right is greater than left
      rc = -6 ;
      goto error ;
   }

   sdb_decimal_free( result ) ;
   result->ndigits = res_ndigits ;
   result->buff    = res_buf ;
   result->digits  = res_digits ;
   result->weight  = res_weight ;
   result->dscale  = res_dscale ;
   result->isOwn   = 1 ;

   /* Remove leading/trailing zeroes */
   _decimal_strip( result ) ;

done:
   return rc ;
error:
   if ( NULL != res_buf )
   {
      bson_free( res_buf ) ;
   }
   goto done ;
}

/*
 * _decimal_add_abs() -
 *
 * Add the absolute values of two variables into result.
 * result might point to one of the operands without danger.
 */
int _decimal_add_abs( const bson_decimal *left, const bson_decimal *right,
                     bson_decimal *result )
{
   short *res_buf    = NULL ;
   short *res_digits = NULL ;
   int res_ndigits   = 0 ;
   int res_weight    = 0 ;
   int res_rscale    = 0 ;
   int rscale1       = 0 ;
   int rscale2       = 0 ;
   int res_dscale    = 0 ;
   int i       = 0 ;
   int i1      = 0 ;
   int i2      = 0 ;
   int carry   = 0 ;
   int rc      = 0 ;

   /* copy these values into local vars for speed in inner loop */
   int leftNdigits    = left->ndigits ;
   short *leftDigits  = left->digits ;

   int rightNdigits   = right->ndigits ;
   short *rightDigits = right->digits ;

   res_weight = Max( left->weight, right->weight ) + 1 ;
   res_dscale = Max( left->dscale, right->dscale ) ;

   /* Note: here we are figuring rscale in base-NBASE digits */
   rscale1    = left->ndigits - left->weight - 1 ;
   rscale2    = right->ndigits - right->weight - 1 ;
   res_rscale = Max( rscale1, rscale2 ) ;

   res_ndigits = res_rscale + res_weight + 1 ;
   if ( res_ndigits <= 0 )
   {
      res_ndigits = 1 ;
   }

   res_buf = bson_malloc( ( res_ndigits + 1 ) * sizeof( short ) ) ;
   if ( NULL == res_buf )
   {
      rc = -2 ;
      goto error ;
   }
   res_buf[0] = 0 ;       /* spare digit for later rounding */
   res_digits = res_buf + 1 ;

   i1 = res_rscale + left->weight + 1 ;
   i2 = res_rscale + right->weight + 1 ;
   for ( i = res_ndigits - 1 ; i >= 0 ; i-- )
   {
      i1-- ;
      i2-- ;
      if ( i1 >= 0 && i1 < leftNdigits )
      {
         carry += leftDigits[i1] ;
      }

      if ( i2 >= 0 && i2 < rightNdigits )
      {
         carry += rightDigits[i2] ;
      }

      if ( carry >= SDB_DECIMAL_NBASE )
      {
         res_digits[i] = carry - SDB_DECIMAL_NBASE ;
         carry = 1 ;
      }
      else
      {
         res_digits[i] = carry ;
         carry = 0;
      }
   }

   if ( carry != 0 )
   {
      /* else we failed to allow for carry out */
      rc = -6 ;
      goto error ;
   }

   sdb_decimal_free( result ) ;
   result->ndigits = res_ndigits ;
   result->buff    = res_buf ;
   result->digits  = res_digits ;
   result->weight  = res_weight ;
   result->dscale  = res_dscale ;
   result->isOwn   = 1 ;

   /* Remove leading/trailing zeroes */
   _decimal_strip( result ) ;

done:
   return rc ;
error:
   if ( NULL != res_buf )
   {
      bson_free( res_buf ) ;
   }
   goto done ;
}

int _decimal_cmp_abs( const bson_decimal *left, const bson_decimal *right )
{
   int i1               = 0;
   int i2               = 0;
   int weight1          = 0 ;
   int ndigit1          = 0 ;
   const short *digits1 = NULL ;

   int weight2          = 0 ;
   int ndigit2          = 0 ;
   const short *digits2 = NULL ;

   if ( NULL == left )
   {
      return -1 ;
   }

   if ( NULL == right )
   {
      return 1 ;
   }

   weight1 = left->weight ;
   ndigit1 = left->ndigits ;
   digits1 = left->digits ;

   weight2 = right->weight ;
   ndigit2 = right->ndigits ;
   digits2 = right->digits ;

   /* Check any digits before the first common digit */
   while ( weight1 > weight2 && i1 < ndigit1 )
   {
      if ( digits1[i1++] != 0 )
      {
         return 1 ;
      }

      weight1--;
   }

   while ( weight2 > weight1 && i2 < ndigit2 )
   {
      if ( digits2[i2++] != 0 )
      {
         return -1;
      }

      weight2-- ;
   }

   /* At this point, either w1 == w2 or we've run out of digits */
   if ( weight1 == weight2 )
   {
      while ( i1 < ndigit1 && i2 < ndigit2 )
      {
         int stat = digits1[i1++] - digits2[i2++];

         if ( stat )
         {
            if ( stat > 0 )
            {
               return 1;
            }

            return -1;
         }
      }
   }

   /*
   * At this point, we've run out of digits on one side or the other; so any
   * remaining nonzero digits imply that side is larger
   */
   while ( i1 < ndigit1 )
   {
      if ( digits1[i1++] != 0 )
      {
         return 1 ;
      }
   }

   while ( i2 < ndigit2 )
   {
      if ( digits2[i2++] != 0 )
      {
         return -1 ;
      }
   }

   return 0;
}

int _decimal_sub( const bson_decimal *left, const bson_decimal *right,
                  bson_decimal *result )
{
   int rc = 0 ;
   /*
    * Decide on the signs of the two variables what to do
    */
   if ( left->sign == SDB_DECIMAL_POS )
   {
      if ( right->sign == SDB_DECIMAL_NEG )
      {
         /* ----------
          * left is positive, right is negative
          * result = +(ABS(left) + ABS(right))
          * ----------
          */
         rc = _decimal_add_abs(left, right, result) ;
         result->sign = SDB_DECIMAL_POS ;
      }
      else
      {
         /* ----------
          * Both are positive
          * Must compare absolute values
          * ----------
          */
         switch ( _decimal_cmp_abs( left, right ) )
         {
            case 0:
               /* ----------
                * ABS(left) == ABS(right)
                * result = ZERO
                * ----------
                */
               sdb_decimal_set_zero( result ) ;
               result->dscale = Max( left->dscale, right->dscale ) ;
               break ;

            case 1:
               /* ----------
                * ABS(left) > ABS(right)
                * result = +(ABS(left) - ABS(right))
                * ----------
                */
               rc = _decimal_sub_abs( left, right, result ) ;
               result->sign = SDB_DECIMAL_POS ;
               break ;

            case -1:
               /* ----------
                * ABS(left) < ABS(right)
                * result = -(ABS(right) - ABS(left))
                * ----------
                */
               rc = _decimal_sub_abs( right, left, result ) ;
               result->sign = SDB_DECIMAL_NEG ;
               break ;
         }
      }
   }
   else
   {
      if ( right->sign == SDB_DECIMAL_NEG )
      {
         /* ----------
          * Both are negative
          * Must compare absolute values
          * ----------
          */
         switch ( _decimal_cmp_abs( left, right ) )
         {
            case 0:
               /* ----------
                * ABS(left) == ABS(right)
                * result = ZERO
                * ----------
                */
               sdb_decimal_set_zero( result ) ;
               result->dscale = Max( left->dscale, right->dscale ) ;
               break ;

            case 1:
               /* ----------
                * ABS(left) > ABS(right)
                * result = -(ABS(left) - ABS(right))
                * ----------
                */
               rc = _decimal_sub_abs( left, right, result ) ;
               result->sign = SDB_DECIMAL_NEG ;
               break ;

            case -1:
               /* ----------
                * ABS(left) < ABS(right)
                * result = +(ABS(right) - ABS(left))
                * ----------
                */
               rc = _decimal_sub_abs( right, left, result ) ;
               result->sign = SDB_DECIMAL_POS ;
               break ;
         }
      }
      else
      {
         /* ----------
          * left is negative, right is positive
          * result = -(ABS(left) + ABS(right))
          * ----------
          */
         rc = _decimal_add_abs( left, right, result ) ;
         result->sign = SDB_DECIMAL_NEG ;
      }
   }

   return rc ;
}

int _decimal_add( const bson_decimal *left, const bson_decimal *right,
                  bson_decimal *result )
{
   int rc = 0 ;
   /*
    * Decide on the signs of the two variables what to do
    */
   if ( left->sign == SDB_DECIMAL_POS )
   {
      if ( right->sign == SDB_DECIMAL_POS )
      {
         /*
          * Both are positive result = +(ABS(left) + ABS(right))
          */
         rc = _decimal_add_abs( left, right, result) ;
         result->sign = SDB_DECIMAL_POS ;
      }
      else
      {
         /*
          * left is positive, right is negative Must compare absolute values
          */
         switch ( _decimal_cmp_abs(left, right) )
         {
            case 0:
               /* ----------
                * ABS(left) == ABS(right)
                * result = ZERO
                * ----------
                */
               sdb_decimal_set_zero( result ) ;
               result->dscale = Max( left->dscale, right->dscale ) ;
               break;

            case 1:
               /* ----------
                * ABS(left) > ABS(right)
                * result = +(ABS(left) - ABS(right))
                * ----------
                */
               rc = _decimal_sub_abs( left, right, result ) ;
               result->sign = SDB_DECIMAL_POS ;
               break;

            case -1:
               /* ----------
                * ABS(left) < ABS(right)
                * result = -(ABS(right) - ABS(left))
                * ----------
                */
               rc = _decimal_sub_abs( right, left, result ) ;
               result->sign = SDB_DECIMAL_NEG ;
               break;
         }
      }
   }
   else
   {
      if ( right->sign == SDB_DECIMAL_POS )
      {
         /* ----------
          * left is negative, right is positive
          * Must compare absolute values
          * ----------
          */
         switch ( _decimal_cmp_abs( left, right ) )
         {
            case 0:
               /* ----------
                * ABS(left) == ABS(right)
                * result = ZERO
                * ----------
                */
               sdb_decimal_set_zero( result ) ;
               result->dscale = Max( left->dscale, right->dscale ) ;
               break;

            case 1:
               /* ----------
                * ABS(left) > ABS(right)
                * result = -(ABS(left) - ABS(right))
                * ----------
                */
               rc = _decimal_sub_abs( left, right, result ) ;
               result->sign = SDB_DECIMAL_NEG ;
               break;

            case -1:
               /* ----------
                * ABS(left) < ABS(right)
                * result = +(ABS(right) - ABS(left))
                * ----------
                */
               rc = _decimal_sub_abs( right, left, result ) ;
               result->sign = SDB_DECIMAL_POS ;
               break;
         }
      }
      else
      {
         /* ----------
          * Both are negative
          * result = -(ABS(left) + ABS(right))
          * ----------
          */
         rc = _decimal_add_abs( left, right, result ) ;
         result->sign = SDB_DECIMAL_NEG ;
      }
   }

   return rc ;
}

int _decimal_mul( const bson_decimal *left, const bson_decimal *right,
                  bson_decimal *result, int rscale )
{
   int res_ndigits   = 0 ;
   int res_sign      = 0 ;
   int res_weight    = 0 ;
   int maxdigits     = 0 ;
   int *dig          = NULL ;
   int carry         = 0 ;
   int maxdig        = 0 ;
   int newdig        = 0 ;
   short *res_digits = NULL ;
   int i  = 0 ;
   int ri = 0 ;
   int i1 = 0 ;
   int i2 = 0 ;
   int rc = 0 ;

   /* copy these values into local vars for speed in inner loop */
   int leftNdigits    = left->ndigits ;
   short *leftDigits  = left->digits ;

   int rightNdigits   = right->ndigits ;
   short *rightDigits = right->digits ;

   if ( leftNdigits == 0 || rightNdigits == 0 )
   {
      /* one or both inputs is zero; so is result */
      sdb_decimal_set_zero( result ) ;
      result->dscale = rscale ;
      goto done ;
   }

   /* Determine result sign and (maximum possible) weight */
   if ( left->sign == right->sign )
   {
      res_sign = SDB_DECIMAL_POS ;
   }
   else
   {
      res_sign = SDB_DECIMAL_NEG ;
   }

   res_weight = left->weight + right->weight + 2 ;

   /*
    * Determine number of result digits to compute.  If the exact result
    * would have more than rscale fractional digits, truncate the computation
    * with MUL_GUARD_DIGITS guard digits. We do that by pretending that one
    * or both inputs have fewer digits than they really do.
    */
   res_ndigits = leftNdigits + rightNdigits + 1 ;
   maxdigits   = res_weight + 1 + ( rscale * SDB_DECIMAL_DEC_DIGITS ) +
                 SDB_DECIMAL_MUL_GUARD_DIGITS ;
   if ( res_ndigits > maxdigits )
   {
      if ( maxdigits < 3 )
      {
         /* no useful precision at all in the result... */
         sdb_decimal_set_zero( result ) ;
         result->dscale = rscale ;
         goto done ;
      }
      /* force maxdigits odd so that input ndigits can be equal */
      if ( ( maxdigits & 1 ) == 0 )
      {
         maxdigits++ ;
      }

      if ( leftNdigits > rightNdigits )
      {
         leftNdigits -= res_ndigits - maxdigits ;
         if (leftNdigits < rightNdigits)
         {
            leftNdigits = rightNdigits = (leftNdigits + rightNdigits) / 2 ;
         }
      }
      else
      {
         rightNdigits -= res_ndigits - maxdigits ;
         if (rightNdigits < leftNdigits)
         {
            leftNdigits = rightNdigits = (leftNdigits + rightNdigits) / 2 ;
         }
      }

      res_ndigits = maxdigits ;

      if ( res_ndigits != ( leftNdigits + rightNdigits + 1 ) )
      {
         rc = -6 ;
         goto error ;
      }
   }

   /*
    * We do the arithmetic in an array "dig[]" of signed int's.  Since
    * INT_MAX is noticeably larger than NBASE*NBASE, this gives us headroom
    * to avoid normalizing carries immediately.
    *
    * maxdig tracks the maximum possible value of any dig[] entry; when this
    * threatens to exceed INT_MAX, we take the time to propagate carries. To
    * avoid overflow in maxdig itself, it actually represents the max
    * possible value divided by NBASE-1.
    */
   dig = (int *) bson_malloc( res_ndigits * sizeof( int ) ) ;
   if ( NULL == dig )
   {
      rc = -2 ;
      goto error ;
   }

   memset( dig, 0, res_ndigits * sizeof( int ) ) ;

   maxdig = 0 ;
   ri     = res_ndigits - 1 ;
   for ( i1 = leftNdigits - 1 ; i1 >= 0 ; ri--, i1-- )
   {
      int leftDigit = leftDigits[i1] ;

      if ( leftDigit == 0 )
      {
         continue ;
      }

      /* Time to normalize? */
      maxdig += leftDigit ;
      if ( maxdig > INT_MAX / ( SDB_DECIMAL_NBASE - 1 ) )
      {
         /* Yes, do it */
         carry = 0 ;
         for ( i = res_ndigits - 1 ; i >= 0 ; i-- )
         {
            newdig = dig[i] + carry ;
            if ( newdig >= SDB_DECIMAL_NBASE )
            {
               carry   = newdig / SDB_DECIMAL_NBASE ;
               newdig -= carry * SDB_DECIMAL_NBASE ;
            }
            else
            {
               carry = 0 ;
            }

            dig[i] = newdig ;
         }

         if ( carry != 0 )
         {
            rc = -6 ;
            goto error ;
         }
         /* Reset maxdig to indicate new worst-case */
         maxdig = 1 + leftDigit ;
      }

      /* Add appropriate multiple of var2 into the accumulator */
      i = ri ;
      for ( i2 = rightNdigits - 1 ; i2 >= 0 ; i2-- )
      {
         dig[i--] += leftDigit * rightDigits[i2] ;
      }
   }

   /*
    * Now we do a final carry propagation pass to normalize the result, which
    * we combine with storing the result digits into the output. Note that
    * this is still done at full precision w/guard digits.
    */
   sdb_decimal_free( result ) ;
   rc = _decimal_alloc( result, res_ndigits ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   res_digits = result->digits ;
   carry = 0 ;
   for ( i = res_ndigits - 1 ; i >= 0 ; i-- )
   {
      newdig = dig[i] + carry ;
      if ( newdig >= SDB_DECIMAL_NBASE )
      {
         carry   = newdig / SDB_DECIMAL_NBASE ;
         newdig -= carry * SDB_DECIMAL_NBASE ;
      }
      else
      {
         carry = 0 ;
      }
      res_digits[i] = newdig ;
   }

   if ( carry != 0 )
   {
      rc = -6 ;
      goto error ;
   }

   /*
    * Finally, round the result to the requested precision.
    */
   result->weight = res_weight ;
   result->sign   = res_sign ;

   /* Round to target rscale (and set result->dscale) */
   rc = sdb_decimal_round( result, rscale ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   /* Strip leading and trailing zeroes */
   _decimal_strip( result ) ;

done:
   if ( NULL != dig )
   {
      bson_free( dig ) ;
   }
   return rc ;
error:
   goto done ;
}

int _decimal_get_div_scale( const bson_decimal *left,
                            const bson_decimal *right )
{
   int weight1 = 0 ;
   int weight2 = 0 ;
   int qweight = 0 ;
   int i       = 0 ;
   short firstdigit1 = 0 ;
   short firstdigit2 = 0 ;
   int rscale        = 0 ;

   /*
    * The result scale of a division isn't specified in any SQL standard. For
    * PostgreSQL we select a result scale that will give at least
    * NUMERIC_MIN_SIG_DIGITS significant digits, so that numeric gives a
    * result no less accurate than float8; but use a scale not less than
    * either input's display scale.
    */

   /* Get the actual (normalized) weight and first digit of each input */

   weight1     = 0 ;    /* values to use if var1 is zero */
   firstdigit1 = 0 ;
   for ( i = 0 ; i < left->ndigits ; i++ )
   {
      firstdigit1 = left->digits[i] ;
      if ( firstdigit1 != 0 )
      {
         weight1 = left->weight - i ;
         break ;
      }
   }

   weight2     = 0 ;            /* values to use if var2 is zero */
   firstdigit2 = 0 ;
   for ( i = 0 ; i < right->ndigits ; i++ )
   {
      firstdigit2 = right->digits[i] ;
      if ( firstdigit2 != 0 )
      {
         weight2 = right->weight - i ;
         break ;
      }
   }

   /*
    * Estimate weight of quotient.  If the two first digits are equal, we
    * can't be sure, but assume that var1 is less than var2.
    */
   qweight = weight1 - weight2 ;
   if ( firstdigit1 <= firstdigit2 )
   {
      qweight-- ;
   }

   /* Select result scale */
   rscale = SDB_DECIMAL_MIN_SIG_DIGITS - qweight * SDB_DECIMAL_DEC_DIGITS ;
   rscale = Max( rscale, left->dscale ) ;
   rscale = Max( rscale, right->dscale ) ;
   rscale = Max( rscale, SDB_DECIMAL_MIN_DISPLAY_SCALE ) ;
   rscale = Min( rscale, SDB_DECIMAL_MAX_DISPLAY_SCALE ) ;

   return rscale ;
}

int _decimal_div( const bson_decimal *left, const bson_decimal *right,
                  bson_decimal *result, int rscale, int isRound )
{
   int div_ndigits   = 0 ;
   int res_ndigits   = 0 ;
   int res_sign      = 0 ;
   int res_weight    = 0 ;
   int carry         = 0 ;
   int borrow        = 0 ;
   int divisor1      = 0 ;
   int divisor2      = 0 ;
   short *dividend   = NULL ;
   short *divisor    = NULL ;
   short *res_digits = NULL ;
   int i  = 0 ;
   int j  = 0 ;
   int rc = 0 ;

   /* copy these values into local vars for speed in inner loop */
   int leftNdigits  = left->ndigits ;
   int rightNdigits = right->ndigits ;

   /*
    * First of all division by zero check; we must not be handed an
    * unnormalized divisor.
    */
   if ( rightNdigits == 0 || right->digits[0] == 0 )
   {
      // divisor is zero.
      rc = -6 ;
      goto error ;
   }

   /*
    * Now result zero check
    */
   if ( leftNdigits == 0 )
   {
      sdb_decimal_set_zero( result ) ;
      result->dscale = rscale ;
      goto done ;
   }

   /*
    * Determine the result sign, weight and number of digits to calculate.
    * The weight figured here is correct if the emitted quotient has no
    * leading zero digits; otherwise strip_var() will fix things up.
    */
   if ( left->sign == right->sign )
   {
      res_sign = SDB_DECIMAL_POS ;
   }
   else
   {
      res_sign = SDB_DECIMAL_NEG ;
   }

   res_weight = left->weight - right->weight ;
   /* The number of accurate result digits we need to produce: */
   res_ndigits = res_weight + 1
                 + ( rscale + SDB_DECIMAL_DEC_DIGITS - 1 ) /
                 SDB_DECIMAL_DEC_DIGITS ;
   /* ... but always at least 1 */
   res_ndigits = Max( res_ndigits, 1 ) ;
   /* If rounding needed, figure one more digit to ensure correct result */
   if ( isRound )
   {
      res_ndigits++ ;
   }

   /*
    * The working dividend normally requires res_ndigits + rightNdigits
    * digits, but make it at least leftNdigits so we can load all of var1
    * into it.  (There will be an additional digit dividend[0] in the
    * dividend space, but for consistency with Knuth's notation we don't
    * count that in div_ndigits.)
    */
   div_ndigits = res_ndigits + rightNdigits ;
   div_ndigits = Max( div_ndigits, leftNdigits ) ;

   /*
    * We need a workspace with room for the working dividend (div_ndigits+1
    * digits) plus room for the possibly-normalized divisor (rightNdigits
    * digits).  It is convenient also to have a zero at divisor[0] with the
    * actual divisor data in divisor[1 .. rightNdigits].  Transferring the
    * digits into the workspace also allows us to realloc the result (which
    * might be the same as either input var) before we begin the main loop.
    * Note that we use palloc0 to ensure that divisor[0], dividend[0], and
    * any additional dividend positions beyond leftNdigits, start out 0.
    */
   {
      int tmpDividendLen =  div_ndigits + rightNdigits + 2 ;
      dividend = ( short * )bson_malloc( tmpDividendLen * sizeof( short ) ) ;
      if ( NULL == dividend )
      {
         rc = -2 ;
         goto error ;
      }
      memset( dividend, 0, tmpDividendLen * sizeof( short ) ) ;
   }

   divisor = dividend + ( div_ndigits + 1 ) ;
   memcpy( dividend + 1, left->digits, leftNdigits * sizeof( short ) ) ;
   memcpy( divisor + 1, right->digits, rightNdigits * sizeof( short ) ) ;

   /*
    * Now we can realloc the result to hold the generated quotient digits.
    */
   rc = _decimal_alloc( result, res_ndigits ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   res_digits = result->digits ;

   if ( rightNdigits == 1 )
   {
      /*
       * If there's only a single divisor digit, we can use a fast path (cf.
       * Knuth section 4.3.1 exercise 16).
       */
      divisor1 = divisor[1] ;
      carry = 0 ;
      for ( i = 0 ; i < res_ndigits ; i++ )
      {
         carry         = carry * SDB_DECIMAL_NBASE + dividend[i + 1] ;
         res_digits[i] = carry / divisor1 ;
         carry         = carry % divisor1 ;
      }
   }
   else
   {
      /*
       * The full multiple-place algorithm is taken from Knuth volume 2,
       * Algorithm 4.3.1D.
       *
       * We need the first divisor digit to be >= NBASE/2.  If it isn't,
       * make it so by scaling up both the divisor and dividend by the
       * factor "d". (The reason for allocating dividend[0] above is to
       * leave room for possible carry here.)
       */
      if ( divisor[1] < SDB_DECIMAL_HALF_NBASE )
      {
         int d = SDB_DECIMAL_NBASE / ( divisor[1] + 1 ) ;
         carry = 0 ;
         for ( i = rightNdigits ; i > 0 ; i-- )
         {
            carry     += divisor[i] * d ;
            divisor[i] = carry % SDB_DECIMAL_NBASE ;
            carry      = carry / SDB_DECIMAL_NBASE ;
         }

         carry = 0 ;
         /* at this point only leftNdigits of dividend can be nonzero */
         for ( i = leftNdigits; i >= 0 ; i-- )
         {
            carry      += dividend[i] * d ;
            dividend[i] = carry % SDB_DECIMAL_NBASE ;
            carry       = carry / SDB_DECIMAL_NBASE ;
         }
      }
      /* First 2 divisor digits are used repeatedly in main loop */
      divisor1 = divisor[1] ;
      divisor2 = divisor[2] ;

      /*
       * Begin the main loop.  Each iteration of this loop produces the j'th
       * quotient digit by dividing dividend[j .. j + rightNdigits] by the
       * divisor; this is essentially the same as the common manual
       * procedure for long division.
       */
      for ( j = 0 ; j < res_ndigits ; j++ )
      {
         /* Estimate quotient digit from the first two dividend digits */
         int next2digits = dividend[j] * SDB_DECIMAL_NBASE + dividend[j + 1];
         int qhat        = 0 ;

         /*
          * If next2digits are 0, then quotient digit must be 0 and there's
          * no need to adjust the working dividend.   It's worth testing
          * here to fall out ASAP when processing trailing zeroes in a
          * dividend.
          */
         if ( next2digits == 0 )
         {
            res_digits[j] = 0 ;
            continue ;
         }

         if ( dividend[j] == divisor1 )
         {
            qhat = SDB_DECIMAL_NBASE - 1 ;
         }
         else
         {
            qhat = next2digits / divisor1 ;
         }

         /*
          * Adjust quotient digit if it's too large.  Knuth proves that
          * after this step, the quotient digit will be either correct or
          * just one too large.  (Note: it's OK to use dividend[j+2] here
          * because we know the divisor length is at least 2.)
          */
         while ( divisor2 * qhat >
                     (next2digits - qhat * divisor1) * SDB_DECIMAL_NBASE +
                     dividend[j + 2] )
         {
            qhat-- ;
         }

         /* As above, need do nothing more when quotient digit is 0 */
         if ( qhat > 0 )
         {
            /*
             * Multiply the divisor by qhat, and subtract that from the
             * working dividend.  "carry" tracks the multiplication,
             * "borrow" the subtraction (could we fold these together?)
             */
            carry  = 0 ;
            borrow = 0 ;
            for ( i = rightNdigits ; i >= 0 ; i-- )
            {
               carry  += divisor[i] * qhat ;
               borrow -= carry % SDB_DECIMAL_NBASE ;
               carry   = carry / SDB_DECIMAL_NBASE ;
               borrow += dividend[j + i] ;
               if ( borrow < 0 )
               {
                  dividend[j + i] = borrow + SDB_DECIMAL_NBASE ;
                  borrow          = -1 ;
               }
               else
               {
                  dividend[j + i] = borrow ;
                  borrow          = 0 ;
               }
            }

            /*
             * If we got a borrow out of the top dividend digit, then
             * indeed qhat was one too large.  Fix it, and add back the
             * divisor to correct the working dividend.  (Knuth proves
             * that this will occur only about 3/NBASE of the time; hence,
             * it's a good idea to test this code with small NBASE to be
             * sure this section gets exercised.)
             */
            if ( borrow )
            {
               qhat-- ;
               carry = 0 ;
               for ( i = rightNdigits ; i >= 0 ; i-- )
               {
                  carry += dividend[j + i] + divisor[i] ;
                  if ( carry >= SDB_DECIMAL_NBASE )
                  {
                     dividend[j + i] = carry - SDB_DECIMAL_NBASE ;
                     carry           = 1 ;
                  }
                  else
                  {
                     dividend[j + i] = carry ;
                     carry           = 0 ;
                  }
               }
               /* A carry should occur here to cancel the borrow above */
            }
         }

         /* And we're done with this quotient digit */
         res_digits[j] = qhat ;
      }
   }

   /*
    * Finally, round or truncate the result to the requested precision.
    */
   result->weight = res_weight ;
   result->sign   = res_sign ;

   /* Round or truncate to target rscale (and set result->dscale) */
   if ( isRound )
   {
      rc = sdb_decimal_round( result, rscale ) ;
      if ( 0 != rc )
      {
         goto error ;
      }
   }
   else
   {
      _decimal_trunc( result, rscale ) ;
   }

   /* Strip leading and trailing zeroes */
   _decimal_strip( result ) ;

done:
   if ( NULL != dividend )
   {
      bson_free( dividend ) ;
   }
   return rc ;
error:
   goto done ;
}

int _decimal_sprint_len( int sign, int weight, int scale )
{
   int tmpSize = 0 ;
   if ( SDB_DECIMAL_SPECIAL_SIGN == sign && SDB_DECIMAL_SPECIAL_NAN == scale )
   {
      return ( 3 + 1 ) ;   // "NAN" + 1
   }

   if ( SDB_DECIMAL_SPECIAL_SIGN == sign && SDB_DECIMAL_SPECIAL_MIN == scale )
   {
      return ( 3 + 1 ) ;   // "MIN" + 1
   }

   if ( SDB_DECIMAL_SPECIAL_SIGN == sign && SDB_DECIMAL_SPECIAL_MAX == scale )
   {
      return ( 3 + 1 ) ;   // "MAX" + 1
   }

   /*
   * Allocate space for the result.
   *
   * tmpSize is set to the # of decimal digits before decimal point. dscale is the
   * # of decimal digits we will print after decimal point. We may generate
   * as many as DEC_DIGITS-1 excess digits at the end, and in addition we
   * need room for sign, decimal point, null terminator.
   */
   tmpSize = ( weight + 1 ) * SDB_DECIMAL_DEC_DIGITS ;
   if ( tmpSize <= 0 )
   {
      tmpSize = 1 ;
   }

   tmpSize += scale + SDB_DECIMAL_DEC_DIGITS + 2 ;

   return tmpSize ;
}

int _decimal_alloc( bson_decimal *decimal, int ndigits )
{
   int rc = 0 ;
   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }
   _decimal_free_buff( decimal ) ;

   decimal->buff = bson_malloc( (ndigits + 1) * sizeof(short) ) ;
   if ( NULL == decimal->buff )
   {
      rc = -2 ;
      goto error ;
   }

   decimal->buff[0] = 0 ;
   decimal->digits  = decimal->buff + 1 ;
   decimal->ndigits = ndigits ;
   decimal->isOwn   = 1 ;

done:
   return rc ;
error:
   goto done ;
}

int _decimal_is_out_of_bound( bson_decimal *decimal )
{
   if ( decimal->weight >= SDB_DECIMAL_MAX_WEIGHT )
   {
      return 1 ;
   }

   if ( decimal->dscale > SDB_DECIMAL_MAX_DSCALE )
   {
      return 1 ;
   }

   return 0 ;
}

void sdb_decimal_init( bson_decimal *decimal )
{
   decimal->typemod = -1 ;
   decimal->ndigits = 0 ;
   decimal->sign    = SDB_DECIMAL_POS ;
   decimal->dscale  = 0 ;
   decimal->weight  = 0 ;
   decimal->isOwn   = 0 ;
   decimal->buff    = NULL ;
   decimal->digits  = NULL ;
}

int sdb_decimal_init1( bson_decimal *decimal, int precision, int scale )
{
   int rc = 0 ;
   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }

   if ( precision < 1 || precision > SDB_DECIMAL_MAX_PRECISION )
   {
      rc = -6 ;
      goto error ;
   }

   if ( scale < 0 || scale > precision )
   {
      rc = -6 ;
      goto error ;
   }

   sdb_decimal_init( decimal ) ;
   decimal->typemod = ( ( precision << 16 ) | scale ) ;

done:
   return rc ;
error:
   goto done ;
}

void sdb_decimal_free( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   _decimal_free_buff( decimal ) ;
   sdb_decimal_init( decimal ) ;
}

void sdb_decimal_set_zero( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   sdb_decimal_free( decimal ) ;
   decimal->ndigits = 0 ;
   decimal->weight  = 0 ;           /* by convention; doesn't really matter */
   decimal->sign    = SDB_DECIMAL_POS ;
}

int sdb_decimal_is_zero( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return 1 ;
   }

   if ( sdb_decimal_is_special( decimal ) )
   {
      return 0 ;
   }

   if ( decimal->ndigits == 0 || decimal->digits[0] == 0 )
   {
      return 1 ;
   }

   return 0 ;
}

int sdb_decimal_is_special( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return 1 ;
   }

   if ( decimal->sign == SDB_DECIMAL_SPECIAL_SIGN )
   {
      return 1 ;
   }

   return 0 ;
}

void sdb_decimal_set_nan( bson_decimal *decimal )
{
   _decimal_set_nan( decimal ) ;
}

int sdb_decimal_is_nan( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return 1 ;
   }

   if ( sdb_decimal_is_special( decimal ) &&
        decimal->dscale == SDB_DECIMAL_SPECIAL_NAN )
   {
      return 1 ;
   }

   return 0 ;
}

void sdb_decimal_set_min( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   sdb_decimal_free( decimal ) ;

   decimal->ndigits = 0 ;
   decimal->weight  = 0 ;
   decimal->sign    = SDB_DECIMAL_SPECIAL_SIGN ;
   decimal->dscale  = SDB_DECIMAL_SPECIAL_MIN ;
}

int sdb_decimal_is_min( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return 0 ;
   }

   if ( sdb_decimal_is_special( decimal ) &&
        decimal->dscale == SDB_DECIMAL_SPECIAL_MIN )
   {
      return 1 ;
   }

   return 0 ;
}

void sdb_decimal_set_max( bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return ;
   }

   sdb_decimal_free( decimal ) ;

   decimal->ndigits = 0 ;
   decimal->weight  = 0 ;
   decimal->sign    = SDB_DECIMAL_SPECIAL_SIGN ;
   decimal->dscale  = SDB_DECIMAL_SPECIAL_MAX ;
}

int sdb_decimal_is_max( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return 0 ;
   }

   if ( sdb_decimal_is_special( decimal ) &&
        decimal->dscale == SDB_DECIMAL_SPECIAL_MAX )
   {
      return 1 ;
   }

   return 0 ;
}

int sdb_decimal_round( bson_decimal *decimal, int rscale )
{
   short *digits = NULL ;
   int di      = 0 ;
   int ndigits = 0 ;
   int carry   = 0 ;
   int rc      = 0 ;

   if ( NULL == decimal || !decimal->isOwn )
   {
      rc = -6 ;
      goto error ;
   }

   digits          = decimal->digits ;
   decimal->dscale = rscale;

   /* decimal digits wanted */
   di = ( decimal->weight + 1 ) * SDB_DECIMAL_DEC_DIGITS + rscale ;

   /*
   * If di = 0, the value loses all digits, but could round up to 1 if its
   * first extra digit is >= 5.  If di < 0 the result must be 0.
   */
   if (di < 0)
   {
      decimal->ndigits = 0 ;
      decimal->weight  = 0 ;
      decimal->sign    = SDB_DECIMAL_POS ;
   }
   else
   {
      /* NBASE digits wanted */
      ndigits = ( di + SDB_DECIMAL_DEC_DIGITS - 1 ) / SDB_DECIMAL_DEC_DIGITS ;

      /* 0, or number of decimal digits to keep in last NBASE digit */
      di %= SDB_DECIMAL_DEC_DIGITS ;

      if ( ndigits < decimal->ndigits ||
         ( ndigits == decimal->ndigits && di > 0 ) )
      {
         decimal->ndigits = ndigits;

         if (di == 0)
         {
            carry = ( digits[ ndigits ] >= SDB_DECIMAL_HALF_NBASE ) ? 1 : 0;
         }
         else
         {
            /* Must round within last NBASE digit */
            int extra = 0 ;
            int pow10 = 0 ;
            pow10 = round_powers[ di ];

            extra = digits[ --ndigits ] % pow10 ;
            digits[ ndigits ] -= extra ;
            carry = 0;
            if ( extra >= pow10/2 )
            {
               pow10 += digits[ ndigits ] ;
               if ( pow10 >= SDB_DECIMAL_NBASE )
               {
                  pow10 -= SDB_DECIMAL_NBASE ;
                  carry = 1;
               }

               digits[ ndigits ] = pow10 ;
            }
         }

         /* Propagate carry if needed */
         while (carry)
         {
            carry += digits[ --ndigits ] ;
            if ( carry >= SDB_DECIMAL_NBASE )
            {
               digits[ndigits] = carry - SDB_DECIMAL_NBASE ;
               carry = 1;
            }
            else
            {
               digits[ ndigits ] = carry;
               carry = 0;
            }
         }

         if ( ndigits < 0 )
         {
            /* better not have added > 1 digit */
            if ( -1 != ndigits  || decimal->digits <= decimal->buff )
            {
               rc = -10 ;
               goto error ;
            }
            decimal->digits-- ;
            decimal->ndigits++ ;
            decimal->weight++ ;
         }
      }
   }

done:
   return rc ;
error:
   goto done ;
}

int sdb_decimal_to_int( const bson_decimal *decimal )
{
   int64_t tmpVal ;
   if ( NULL == decimal )
   {
      return 0 ;
   }

   tmpVal = sdb_decimal_to_long( decimal ) ;

   return ( int ) tmpVal ;
}

int64_t sdb_decimal_to_long( const bson_decimal *decimal )
{
   int rc      = 0 ;
   short *digits = NULL ;
   int ndigits = 0 ;
   int weight  = 0 ;
   int i       = 0 ;
   int64_t val     = 0 ;
   int64_t oldval  = 0 ;
   int neg     = 0 ;
   bson_decimal rounded = SDB_DECIMAL_DEFAULT_VALUE ;

   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( decimal ) )
   {
      rc = -6 ;
      goto error ;
   }

   /* Round to nearest integer */
   rc = sdb_decimal_copy( decimal, &rounded ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   rc = sdb_decimal_round( &rounded, 0 ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   /* Check for zero input */
   _decimal_strip( &rounded ) ;
   ndigits = rounded.ndigits ;
   if ( ndigits == 0 )
   {
      val = 0 ;
      goto done ;
   }

   /*
   * For input like 10000000000, we must treat stripped digits as real. So
   * the loop assumes there are weight+1 digits before the decimal point.
   */
   weight = rounded.weight;
   if ( weight < 0 || ndigits > weight + 1 )
   {
      rc = -10 ;
      goto error ;
   }

   /* Construct the result */
   digits = rounded.digits ;
   neg    = ( rounded.sign == SDB_DECIMAL_NEG ) ;
   val    = digits[0] ;
   for ( i = 1 ; i <= weight ; i++ )
   {
      oldval = val ;
      val *= SDB_DECIMAL_NBASE ;
      if ( i < ndigits )
      {
         val += digits[i] ;
      }

      /*
      * The overflow check is a bit tricky because we want to accept
      * int64_t_MIN, which will overflow the positive accumulator.  We can
      * detect this case easily though because int64_t_MIN is the only
      * nonzero value for which -val == val (on a two's complement machine,
      * anyway).
      */
      if ( ( val / SDB_DECIMAL_NBASE ) != oldval )  /* possible overflow? */
      {
         if ( !neg || ( ( -val ) != val ) || ( val == 0 ) || ( oldval < 0 ) )
         {
            rc = -10 ;
            goto error ;
         }
      }
   }

done:
   sdb_decimal_free( &rounded ) ;
   if ( 0 == rc )
   {
      return ( neg ? -val : val ) ;
   }
   else
   {
      return 0 ;
   }
error:
   goto done ;
}

double sdb_decimal_to_double( const bson_decimal *decimal )
{
   int rc       = 0 ;
   char *strValue = NULL ;
   double d      = 0.0 ;
   int size     = 0 ;

   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }

   rc = sdb_decimal_to_str_get_len( decimal, &size ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   strValue = bson_malloc( size + 1 ) ;
   if ( NULL == strValue )
   {
      rc = -2 ;
      goto error ;
   }

   rc = sdb_decimal_to_str( decimal, strValue, size ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   strValue[ size ] = '\0' ;
   d = atof( strValue ) ;

done:
   if ( NULL != strValue )
   {
      bson_free( strValue ) ;
   }

   return d ;
error:
   goto done ;
}

int sdb_decimal_to_str_get_len( const bson_decimal *decimal, int *size )
{
   int rc      = 0 ;
   if ( NULL == decimal || NULL == size )
   {
      rc = -6 ;
      goto error ;
   }

   *size = _decimal_sprint_len( decimal->sign, decimal->weight,
                                decimal->dscale ) ;

done:
   return rc ;
error:
   goto done ;
}

// the caller is responsible for freeing this value(bson_free)
int sdb_decimal_to_str( const bson_decimal *decimal, char *value, int value_size )
{
   int dscale      = 0 ;
   char *cp        = NULL ;
   char *endcp     = NULL ;
   int i           = 0 ;
   int d           = 0 ;
   short dig       = 0 ;
   short d1        = 0 ;
   int expect_size = 0 ;
   int rc          = 0 ;

   if ( NULL == decimal || NULL == value )
   {
      rc = -6 ;
      goto error ;
   }

   dscale = decimal->dscale ;

   rc = sdb_decimal_to_str_get_len( decimal, &expect_size ) ;
   if ( 0 != rc || expect_size > value_size )
   {
      rc = -6 ;
      goto error ;
   }

   cp = value ;

   if ( sdb_decimal_is_nan( decimal ) )
   {
      *cp++ = 'N' ;
      *cp++ = 'a' ;
      *cp++ = 'N' ;
      *cp   = '\0' ;
      goto done ;
   }

   if ( sdb_decimal_is_min( decimal ) )
   {
      *cp++ = 'M' ;
      *cp++ = 'I' ;
      *cp++ = 'N' ;
      *cp   = '\0' ;
      goto done ;
   }

   if ( sdb_decimal_is_max( decimal ) )
   {
      *cp++ = 'M' ;
      *cp++ = 'A' ;
      *cp++ = 'X' ;
      *cp   = '\0' ;
      goto done ;
   }

   /*
   * Output a dash for negative values
   */
   if ( decimal->sign == SDB_DECIMAL_NEG )
   {
      *cp++ = '-' ;
   }

   /*
   * Output all digits before the decimal point
   */
   if ( decimal->weight < 0 )
   {
      d = decimal->weight + 1 ;
      *cp++ = '0' ;
   }
   else
   {
      for ( d = 0 ; d <= decimal->weight ; d++ )
      {
         dig = (d < decimal->ndigits) ? decimal->digits[d] : 0 ;
         /* In the first digit, suppress extra leading decimal zeroes */
         {
            int putit = ( d > 0 ) ;

            d1    =  dig / 1000 ;
            dig   -= d1 * 1000 ;
            putit |= ( d1 > 0 ) ;
            if ( putit )
            {
               *cp++ = d1 + '0' ;
            }

            d1    = dig / 100 ;
            dig   -= d1 * 100 ;
            putit |= ( d1 > 0 ) ;
            if ( putit )
            {
               *cp++ = d1 + '0' ;
            }

            d1    = dig / 10 ;
            dig   -= d1 * 10 ;
            putit |= (d1 > 0) ;
            if ( putit )
            {
               *cp++ = d1 + '0' ;
            }

            *cp++ = dig + '0' ;
         }
      }
   }

   /*
   * If requested, output a decimal point and all the digits that follow it.
   * We initially put out a multiple of DEC_DIGITS digits, then truncate if
   * needed.
   */
   if ( dscale > 0 )
   {
      *cp++ = '.' ;
      endcp = cp + dscale ;
      for ( i = 0; i < dscale; d++, i += SDB_DECIMAL_DEC_DIGITS )
      {
         dig = (d >= 0 && d < decimal->ndigits) ? decimal->digits[d] : 0 ;
         d1  = dig / 1000 ;
         dig -= d1 * 1000 ;
         *cp++ = d1 + '0' ;

         d1  = dig / 100 ;
         dig -= d1 * 100 ;
         *cp++ = d1 + '0' ;

         d1  = dig / 10 ;
         dig -= d1 * 10 ;
         *cp++ = d1 + '0' ;

         *cp++ = dig + '0' ;
      }

      while ( cp < endcp )
      {
         *cp++ = '0' ;
      }

      cp = endcp ;
   }

   /*
   * terminate the string and return it
   */
   *cp = '\0' ;

done:
   return rc ;
error:
   goto done ;
}

// the caller is responsible for freeing this decimal( sdb_decimal_free )
int sdb_decimal_from_int( int value, bson_decimal *decimal )
{
   return sdb_decimal_from_long( ( int64_t ) value, decimal ) ;
}

// the caller is responsible for freeing this decimal( sdb_decimal_free )
int sdb_decimal_from_long( int64_t value, bson_decimal *decimal )
{
   uint64_t uval    = 0 ;
   uint64_t newuval = 0 ;
   short *ptr     = NULL ;
   int ndigits  = 0 ;
   int rc       = 0 ;

   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }

   /* int8 can require at most 19 decimal digits; add one for safety */
   rc = _decimal_alloc( decimal, 20 / SDB_DECIMAL_DEC_DIGITS ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   if ( value < 0 )
   {
      decimal->sign = SDB_DECIMAL_NEG ;
      uval = -value ;
   }
   else
   {
      decimal->sign = SDB_DECIMAL_POS ;
      uval = value ;
   }

   decimal->dscale = 0 ;
   if ( value == 0 )
   {
      decimal->ndigits = 0 ;
      decimal->weight  = 0 ;
      goto done ;
   }

   ptr = decimal->digits + decimal->ndigits ;
   ndigits = 0 ;
   do
   {
      ptr-- ;
      ndigits++ ;
      newuval = uval / SDB_DECIMAL_NBASE ;
      *ptr = uval - newuval * SDB_DECIMAL_NBASE ;
      uval = newuval ;
   } while (uval) ;

   decimal->digits  = ptr ;
   decimal->ndigits = ndigits ;
   decimal->weight  = ndigits - 1 ;

   rc = _decimal_apply_typmod( decimal, decimal->typemod ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   sdb_decimal_free( decimal ) ;
   goto done ;
}

// the caller is responsible for freeing this decimal( sdb_decimal_free )
int sdb_decimal_from_double( double value, bson_decimal *decimal )
{
   char buf[ SDB_DECIMAL_DBL_DIG + 100 ] = "" ;

   if ( NULL == decimal )
   {
      return -6 ;
   }

   bson_sprintf( buf, "%.*g", SDB_DECIMAL_DBL_DIG, value ) ;

  return sdb_decimal_from_str( buf, decimal ) ;
}

/*
 * sdb_decimal_from_str()
 *
 *  Parse a string and put the number into a variable
 *  the caller is responsible for freeing this decimal( sdb_decimal_free )
 */
int sdb_decimal_from_str( const char *value, bson_decimal *decimal )

{
   int have_dp = 0 ;
   int i       = 0 ;
   int sign    = SDB_DECIMAL_POS ;
   int dweight = -1 ;
   int ddigits = 0 ;
   int dscale  = 0 ;
   int weight  = -1 ;
   int ndigits = 0 ;
   int offset  = 0 ;
   int rc      = 0 ;
   unsigned int len ;

   short *digits = NULL ;
   unsigned char *decdigits = NULL ;

   const char * cp = value ;

   if ( NULL == decimal || NULL == value )
   {
      rc = -6 ;
      goto error ;
   }
   len = strlen( cp ) ;

   if ( len == 3 )
   {
      if ( ( cp[0] == 'n' || cp[0] == 'N' ) &&
           ( cp[1] == 'a' || cp[1] == 'A' ) &&
           ( cp[2] == 'n' || cp[2] == 'N' ) )
      {
         _decimal_set_nan( decimal ) ;
         goto done ;
      }

      if ( ( cp[0] == 'm' || cp[0] == 'M' ) &&
           ( cp[1] == 'i' || cp[1] == 'I' ) &&
           ( cp[2] == 'n' || cp[2] == 'N' ) )
      {
         sdb_decimal_set_min( decimal ) ;
         goto done ;
      }

      if ( ( cp[0] == 'm' || cp[0] == 'M' ) &&
           ( cp[1] == 'a' || cp[1] == 'A' ) &&
           ( cp[2] == 'x' || cp[2] == 'X' ) )
      {
         sdb_decimal_set_max( decimal ) ;
         goto done ;
      }

      if ( ( cp[0] == 'i' || cp[0] == 'I' ) &&
           ( cp[1] == 'n' || cp[1] == 'N' ) &&
           ( cp[2] == 'f' || cp[2] == 'F' ) )
      {
         sdb_decimal_set_max( decimal ) ;
         goto done ;
      }
   }

   if ( len == 4 )
   {
      if (   cp[0] == '-' &&
           ( cp[1] == 'i' || cp[1] == 'I' ) &&
           ( cp[2] == 'n' || cp[2] == 'N' ) &&
           ( cp[3] == 'f' || cp[3] == 'F' ) )
      {
         sdb_decimal_set_min( decimal ) ;
         goto done ;
      }
   }

   /*
   * We first parse the string to extract decimal digits and determine the
   * correct decimal weight.  Then convert to NBASE representation.
   */
   switch (*cp)
   {
      case '+':
         sign = SDB_DECIMAL_POS ;
         cp++;
      break;

      case '-':
         sign = SDB_DECIMAL_NEG ;
         cp++;
      break;
   }

   if ( *cp == '.' )
   {
      have_dp = 1 ;
      cp++ ;
   }

   if ( !_decimal_is_digit( *cp ) )
   {
      rc = -6 ;
      goto error ;
   }

   decdigits = (unsigned char *) bson_malloc( strlen(cp) +
                                                 SDB_DECIMAL_DEC_DIGITS * 2 ) ;

   /* leading padding for digit alignment later */
   memset( decdigits, 0, SDB_DECIMAL_DEC_DIGITS ) ;
   i = SDB_DECIMAL_DEC_DIGITS ;

   while (*cp)
   {
      if ( _decimal_is_digit( *cp ) )
      {
         decdigits[i++] = *cp++ - '0' ;
         if ( !have_dp )
         {
            dweight++ ;
            if ( dweight > SDB_DECIMAL_MAX_DWEIGHT +
                           SDB_DECIMAL_MAX_PRECISION )
            {
               rc = -6 ;
               goto error ;
            }
         }
         else
         {
            dscale++ ;
            if ( dscale > SDB_DECIMAL_MAX_DSCALE +
                          SDB_DECIMAL_MAX_PRECISION )
            {
               rc = -6 ;
               goto error ;
            }
         }
      }
      else if ( *cp == '.' )
      {
         if ( have_dp )
         {
            rc = -6 ;
            goto error ;
         }

         have_dp = 1 ;
         cp++ ;
      }
      else
      {
         break;
      }
   }

   ddigits = i - SDB_DECIMAL_DEC_DIGITS ;
   /* trailing padding for digit alignment later */
   memset( decdigits + i, 0, SDB_DECIMAL_DEC_DIGITS - 1 ) ;

   /* Handle exponent, if any */
   if (*cp == 'e' || *cp == 'E' )
   {
      long exponent = 0 ;
      char *pEndPtr = NULL ;

      cp++ ;
#if defined (_WIN32)
      exponent = _strtoi64( cp, &pEndPtr, 10 ) ;
#else
      exponent = strtoll( cp, &pEndPtr, 10 ) ;
#endif

      cp = pEndPtr ;
      if ( exponent > SDB_DECIMAL_MAX_PRECISION ||
           exponent < -SDB_DECIMAL_MAX_PRECISION )
      {
         //meet the limit
         rc = -6 ;
         goto error ;
      }

      dweight += (int) exponent ;
      dscale  -= (int) exponent ;
      if ( dscale < 0 )
      {
         dscale = 0 ;
      }
   }

   // make sure the count of digits is in the bound
   if ( dweight >= SDB_DECIMAL_MAX_DWEIGHT ||
        dscale > SDB_DECIMAL_MAX_DSCALE )
   {
      rc = -6 ;
      goto error ;
   }

   if ( *cp != '\0' )
   {
      // exist not digits value
      rc = -6 ;
      goto error ;
   }

   /*
   * Okay, convert pure-decimal representation to base NBASE.  First we need
   * to determine the converted weight and ndigits.  offset is the number of
   * decimal zeroes to insert before the first given digit to have a
   * correctly aligned first NBASE digit.
   */
   if ( dweight >= 0 )
   {
      weight = (dweight + 1 + SDB_DECIMAL_DEC_DIGITS - 1) /
               SDB_DECIMAL_DEC_DIGITS - 1 ;
   }
   else
   {
      weight = -( (-dweight - 1)/SDB_DECIMAL_DEC_DIGITS + 1 ) ;
   }

   offset  = (weight + 1) * SDB_DECIMAL_DEC_DIGITS - ( dweight + 1 ) ;
   ndigits = (ddigits + offset + SDB_DECIMAL_DEC_DIGITS - 1) /
             SDB_DECIMAL_DEC_DIGITS ;

   rc = _decimal_alloc( decimal, ndigits ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   decimal->sign   = sign ;
   decimal->weight = weight ;
   decimal->dscale = dscale ;

   i = SDB_DECIMAL_DEC_DIGITS - offset ;
   digits = decimal->digits ;

   while (ndigits-- > 0)
   {
      *digits++ = ( ( decdigits[i] * 10 + decdigits[i + 1] ) * 10 +
                  decdigits[i + 2] ) * 10 + decdigits[i + 3] ;
      i += SDB_DECIMAL_DEC_DIGITS ;
   }

   /* Strip any leading/trailing zeroes, and normalize weight if zero */
   _decimal_strip( decimal ) ;

   rc = _decimal_apply_typmod( decimal, decimal->typemod ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   if ( _decimal_is_out_of_bound( decimal ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   if ( NULL != decdigits )
   {
      bson_free( decdigits ) ;
   }
   return rc ;
error:
   sdb_decimal_free( decimal ) ;
   goto done ;
}

int sdb_decimal_from_bsonvalue( const char *value, bson_decimal *decimal )
{
   int size     = 0 ;
   int typemod  = 0 ;
   short scale  = 0 ;
   short weight = 0 ;
   short dig    = 0 ;
   int ndig     = 0 ;
   int index    = 0 ;
   int rc       = 0 ;

   //define in common_decimal.h __sdb_decimal
   bson_little_endian32( &size, value ) ;
   value += 4 ;

   bson_little_endian32( &typemod, value ) ;
   value += 4 ;

   bson_little_endian16( &scale, value ) ;
   value += 2 ;

   bson_little_endian16( &weight, value ) ;
   value += 2 ;

   ndig = ( size - SDB_DECIMAL_HEADER_SIZE ) / sizeof( short ) ;
   rc = _decimal_alloc( decimal, ndig ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   decimal->typemod = typemod ;
   decimal->weight  = weight ;
   decimal->sign    = scale & SDB_DECIMAL_SIGN_MASK ;
   decimal->dscale  = scale & SDB_DECIMAL_DSCALE_MASK ;
   for ( index = 0 ; index < ndig ; index++ )
   {
      bson_little_endian16( &dig, value ) ;
      decimal->digits[index] = dig ;
      value += 2 ;
   }

done:
   return rc ;
error:
   sdb_decimal_free( decimal ) ;
   goto done ;
}


int sdb_decimal_get_typemod( const bson_decimal *decimal, int *precision,
                             int *scale )
{
   int rc        = 0 ;

   /* Do nothing if we have a default typmod (-1) */
   if ( NULL == decimal || NULL == precision || NULL == scale )
   {
      rc = -6 ;
      goto error ;
   }

   if ( decimal->typemod == -1 )
   {
      *precision = -1 ;
      *scale     = -1 ;
      goto done ;
   }

   *precision = ( decimal->typemod >> 16 ) & 0xffff ;
   *scale     = decimal->typemod & 0xffff ;

done:
   return rc ;
error:
   goto done ;
}

int sdb_decimal_get_typemod2( const bson_decimal *decimal )
{
   if ( NULL == decimal )
   {
      return -1 ;
   }

   return decimal->typemod ;
}

int sdb_decimal_copy( const bson_decimal *source, bson_decimal *target )
{
   int rc      = 0 ;
   short *newbuf = NULL ;

   newbuf    = bson_malloc( (source->ndigits + 1) * sizeof(short) );
   if ( NULL == newbuf )
   {
      rc = -2 ;
      goto error ;
   }

   newbuf[0] = 0;          /* spare digit for rounding */
   memcpy( newbuf + 1, source->digits, source->ndigits * sizeof(short) ) ;

   _decimal_free_buff( target ) ;

   target->dscale  = source->dscale ;
   target->ndigits = source->ndigits ;
   target->sign    = source->sign ;
   target->typemod = source->typemod ;
   target->weight  = source->weight ;

   target->isOwn   = 1 ;
   target->buff    = newbuf ;
   target->digits  = newbuf + 1 ;

done:
   return rc ;
error:
   if ( NULL != newbuf )
   {
      bson_free( newbuf ) ;
   }
   goto done ;
}

int sdb_decimal_to_jsonstr_len( int sign, int weight, int dscale,
                                int typemod, int *size )
{
   int rc         = 0 ;
   int tmpSize    = 0 ;
   int simpleSize = 0 ;

   if ( NULL == size )
   {
      goto error ;
   }

   // get the simple decimal string len.  like "123.45678"
   simpleSize = _decimal_sprint_len( sign, weight, dscale ) ;

   tmpSize = strlen( decimal_str_start ) + simpleSize +
             strlen( decimal_str_end ) + strlen( json_str_end ) + 1 ;

   if ( typemod != -1 )
   {
      int precision   = 0 ;
      int scale       = 0 ;
      char preStr[20] = "" ;
      precision = ( typemod >> 16 ) & 0xffff ;
      scale     = typemod & 0xffff ;
      sprintf( preStr, "%d, %d", precision, scale ) ;

      tmpSize += strlen( precision_str_start ) +
                 strlen( preStr ) + strlen( precision_str_end ) ;
   }

   *size = tmpSize ;

done:
   return rc ;
error:
   goto done ;
}

int sdb_decimal_to_jsonstr( const bson_decimal *decimal, char *value,
                            int value_size )
{
   int rc          = 0 ;
   int expect_size = 0 ;
   int simple_size = 0 ;
   int decimal_len = 0 ;
   rc = sdb_decimal_to_jsonstr_len( decimal->sign, decimal->weight,
                                    decimal->dscale,
                                    decimal->typemod, &expect_size ) ;
   if ( 0 != rc || expect_size > value_size )
   {
      rc = -6 ;
      goto error ;
   }

   rc = sdb_decimal_to_str_get_len( decimal, &simple_size ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   memcpy( value, decimal_str_start, strlen( decimal_str_start ) ) ;
   value += strlen( decimal_str_start ) ;
   value_size -= strlen( decimal_str_start ) ;

   rc = sdb_decimal_to_str( decimal, value, simple_size ) ;
   if ( 0 != rc )
   {
      goto error ;
   }
   decimal_len = strlen( value ) ;
   value += decimal_len ;
   value_size -= decimal_len ;

   memcpy( value, decimal_str_end, strlen( decimal_str_end ) ) ;
   value += strlen( decimal_str_end ) ;
   value_size -= strlen( decimal_str_end ) ;

   if ( decimal->typemod != -1 )
   {
      int precision   = 0 ;
      int scale       = 0 ;
      char preStr[20] = "" ;
      sdb_decimal_get_typemod( decimal, &precision, &scale ) ;
      sprintf( preStr, "%d, %d", precision, scale ) ;

      memcpy( value, precision_str_start, strlen( precision_str_start ) ) ;
      value += strlen( precision_str_start ) ;
      value_size -= strlen( precision_str_start ) ;

      memcpy( value, preStr, strlen( preStr ) ) ;
      value += strlen( preStr ) ;
      value_size -= strlen( preStr ) ;

      memcpy( value, precision_str_end, strlen( precision_str_end ) ) ;
      value += strlen( precision_str_end ) ;
      value_size -= strlen( precision_str_end ) ;
   }

   memcpy( value, json_str_end, strlen( json_str_end ) ) ;
   value += strlen( json_str_end ) ;
   value_size -= strlen( json_str_end ) ;

   *value = '\0' ;

done:
   return rc ;
error:
   goto done ;
}

int sdb_decimal_cmp( const bson_decimal *left, const bson_decimal *right )
{
   if ( NULL == left )
   {
      return -1 ;
   }

   if ( NULL == right )
   {
      return 1 ;
   }

   /*
    * postgresql's define:
    *    We consider all NANs to be equal and larger than any non-NAN. This is
    *    somewhat arbitrary; the important thing is to have a consistent sort
    *    order.
    *
    * while bson's define is the opposite:
    *    NAN's is smaller than any non-NAN.  bsonobj.cpp:compareElementValues
    *
    * conclusion:  we use bson's define!
    */
   if ( sdb_decimal_is_nan( left ) )
   {
      if ( sdb_decimal_is_nan( right ) )
      {
         return 0 ;       /* NAN = NAN */
      }
      else
      {
         return -1 ;       /* NAN < non-NAN */
      }
   }
   else if ( sdb_decimal_is_nan( right ) )
   {
      return 1 ;         /* non-NAN > NAN */
   }

   // min is equal min;  min is less than any non-min.
   if ( sdb_decimal_is_min( left ) )
   {
      if ( sdb_decimal_is_min( right ) )
      {
         return 0 ;
      }
      else
      {
         return -1 ;
      }
   }
   else if ( sdb_decimal_is_min( right ) )
   {
      return 1 ;
   }

   // max is equal max;  max is larger than any non-max.
   if ( sdb_decimal_is_max( left ) )
   {
      if ( sdb_decimal_is_max( right ) )
      {
         return 0 ;
      }
      else
      {
         return 1 ;
      }
   }
   else if ( sdb_decimal_is_max( right ) )
   {
      return -1 ;
   }

   if ( left->ndigits == 0 )
   {
      if ( right->ndigits == 0 )
      {
         return 0 ;
      }

      if ( right->sign == SDB_DECIMAL_NEG )
      {
         return 1 ;
      }

      return -1 ;
   }

   if ( right->ndigits == 0 )
   {
      if ( left->sign == SDB_DECIMAL_POS )
      {
         return 1 ;
      }

      return -1 ;
   }

   if ( left->sign == SDB_DECIMAL_POS )
   {
      if ( right->sign == SDB_DECIMAL_NEG )
      {
         return 1 ;
      }

      return _decimal_cmp_abs( left, right ) ;
   }

   if ( right->sign == SDB_DECIMAL_POS )
   {
      return -1 ;
   }

   return _decimal_cmp_abs( right, left ) ;
}

SDB_EXPORT int sdb_decimal_add( const bson_decimal *left,
                                const bson_decimal *right,
                                bson_decimal *result )
{
   int rc = 0 ;
   if ( NULL == left || NULL == right || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( left ) || sdb_decimal_is_special( right ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rc = _decimal_add( left, right, result ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   _decimal_strip( result ) ;

   if ( _decimal_is_out_of_bound( result ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   return rc ;
error:
   //do not free result. result may point to left or right
   goto done ;
}

SDB_EXPORT int sdb_decimal_sub( const bson_decimal *left,
                                const bson_decimal *right,
                                bson_decimal *result )
{
   int rc = 0 ;
   if ( NULL == left || NULL == right || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( left ) || sdb_decimal_is_special( right ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rc = _decimal_sub( left, right, result ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   _decimal_strip( result ) ;

   if ( _decimal_is_out_of_bound( result ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   return rc ;
error:
   //do not free result. result may point to left or right
   goto done ;
}

SDB_EXPORT int sdb_decimal_mul( const bson_decimal *left,
                                const bson_decimal *right,
                                bson_decimal *result )
{
   int rc = 0 ;
   if ( NULL == left || NULL == right || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( left ) || sdb_decimal_is_special( right ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rc = _decimal_mul( left, right, result, left->dscale + right->dscale ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   if ( _decimal_is_out_of_bound( result ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT int sdb_decimal_div( const bson_decimal *left,
                                const bson_decimal *right,
                                bson_decimal *result )
{
   int rc     = 0 ;
   int rscale = 0 ;
   if ( NULL == left || NULL == right || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( left ) || sdb_decimal_is_special( right ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rscale = _decimal_get_div_scale( left, right ) ;

   rc = _decimal_div( left, right, result, rscale, 1 ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   if ( _decimal_is_out_of_bound( result ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT int sdb_decimal_abs( bson_decimal *decimal )
{
   int rc = 0 ;
   if ( NULL == decimal || sdb_decimal_is_special( decimal ) )
   {
      rc = -6 ;
      goto error ;
   }

   decimal->sign = SDB_DECIMAL_POS ;

done:
   return rc ;
error:
   goto done ;
}


SDB_EXPORT int sdb_decimal_ceil( const bson_decimal *decimal,
                                 bson_decimal *result )
{
   int rc = 0 ;
   bson_decimal tmp = SDB_DECIMAL_DEFAULT_VALUE ;

   if ( NULL == decimal || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( decimal ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rc = sdb_decimal_copy( decimal, &tmp ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   _decimal_trunc( &tmp, 0 ) ;

   if ( decimal->sign == SDB_DECIMAL_POS &&
        sdb_decimal_cmp( decimal, &tmp ) != 0 )
   {
      rc = sdb_decimal_add( &tmp, &const_one, &tmp ) ;
      if ( 0 != rc )
      {
         goto error ;
      }
   }

   rc = sdb_decimal_copy( &tmp, result ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

done:
   sdb_decimal_free( &tmp ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT int sdb_decimal_floor( const bson_decimal *decimal,
                                  bson_decimal *result )
{
   int rc = 0 ;
   bson_decimal tmp = SDB_DECIMAL_DEFAULT_VALUE ;

   if ( NULL == decimal || NULL == result )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( decimal ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   rc = sdb_decimal_copy( decimal, &tmp ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   _decimal_trunc( &tmp, 0 ) ;

   if ( decimal->sign == SDB_DECIMAL_NEG &&
        sdb_decimal_cmp(decimal, &tmp) != 0 )
   {
      rc = sdb_decimal_sub( &tmp, &const_one, &tmp ) ;
      if ( 0 != rc )
      {
         goto error ;
      }
   }

   rc = sdb_decimal_copy( &tmp, result ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   if ( _decimal_is_out_of_bound( result ) )
   {
      rc = -6 ;
      goto error ;
   }

done:
   sdb_decimal_free( &tmp ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT int sdb_decimal_mod( const bson_decimal *left,
                                const bson_decimal *right,
                                bson_decimal *result )
{
   int rc = 0 ;
   bson_decimal tmp = SDB_DECIMAL_DEFAULT_VALUE ;

   if ( NULL == left || NULL == right )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_special( left ) || sdb_decimal_is_special( right ) )
   {
      _decimal_set_nan( result ) ;
      goto done ;
   }

   /* ---------
    * We do this using the equation
    *    mod(x,y) = x - trunc(x/y)*y
    * div_var can be persuaded to give us trunc(x/y) directly.
    * ----------
    */
   rc = _decimal_div( left, right, &tmp, 0, 0 ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   rc = _decimal_mul( right, &tmp, &tmp, right->dscale ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   rc = _decimal_sub( left, &tmp, result ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

done:
   sdb_decimal_free( &tmp ) ;
   return rc ;
error:
   goto done ;
}

int sdb_decimal_is_out_of_precision( bson_decimal *decimal, int typemod )
{
   int precision = 0 ;
   int scale     = 0 ;
   int maxdigits = 0 ;
   int ddigits   = 0 ;
   int i         = 0 ;
   int out_of_precision = 0 ;

   if ( typemod == -1 )
   {
      /* return false if we have a default typmod (-1) */
      goto done ;
   }

   precision = ( typemod >> 16 ) & 0xffff ;
   scale     = typemod & 0xffff ;
   maxdigits = precision - scale ;

   if ( decimal->dscale > scale )
   {
      out_of_precision = 1 ;
      goto done ;
   }

   /*
   * Check for overflow - note we can't do this before rounding, because
   * rounding could raise the weight.  Also note that the var's weight could
   * be inflated by leading zeroes, which will be stripped before storage
   * but perhaps might not have been yet. In any case, we must recognize a
   * true zero, whose weight doesn't mean anything.
   */
   ddigits = ( decimal->weight + 1) * SDB_DECIMAL_DEC_DIGITS ;
   if ( ddigits > maxdigits )
   {
      /* Determine true weight; and check for all-zero result */
      for ( i = 0 ; i < decimal->ndigits ; i++ )
      {
         short dig = decimal->digits[i] ;
         if ( dig )
         {
            /* Adjust for any high-order decimal zero digits */
            if ( dig < 10 )
            {
               ddigits -= 3 ;
            }
            else if ( dig < 100 )
            {
               ddigits -= 2 ;
            }
            else if (dig < 1000)
            {
               ddigits -= 1 ;
            }

            if ( ddigits > maxdigits )
            {
               out_of_precision = 1 ;
               goto done ;
            }
            break ;
         }

         ddigits -= SDB_DECIMAL_DEC_DIGITS ;
      }
   }

done:
   return out_of_precision ;
}

int sdb_decimal_update_typemod( bson_decimal *decimal, int typemod )
{
   int rc         = 0 ;
   if ( NULL == decimal )
   {
      rc = -6 ;
      goto error ;
   }

   if ( sdb_decimal_is_out_of_precision( decimal, typemod ) )
   {
      //if out of precision define. remove the precision define
      decimal->typemod = -1 ;
      goto done ;
   }

   if ( sdb_decimal_is_special( decimal ) )
   {
      goto done ;
   }

   if ( sdb_decimal_is_zero( decimal ) )
   {
      decimal->typemod = typemod ;
      goto done ;
   }

   rc = _decimal_apply_typmod( decimal, typemod ) ;
   if ( 0 != rc )
   {
      goto error ;
   }

   decimal->typemod = typemod ;

done:
   return rc ;
error:
   goto done ;
}

int sdb_decimal_view_from_bsonvalue( const char *value,
                                     bson_decimal *decimal )
{
#ifdef SDB_BIG_ENDIAN
   return sdb_decimal_from_bsonvalue( value, decimal ) ;
#else
   int size     = 0 ;
   int typemod  = 0 ;
   short scale  = 0 ;
   short weight = 0 ;

   if ( NULL == decimal )
   {
      return -6 ;
   }

   //define in common_decimal.h __sdb_decimal
   size = *(int *)value ;
   value += 4 ;

   typemod = *(int *)value ;
   value += 4 ;

   scale = *(short *)value ;
   value += 2 ;

   weight = *(short *)value ;
   value += 2 ;

   decimal->typemod = typemod ;
   decimal->ndigits = ( size - SDB_DECIMAL_HEADER_SIZE ) / sizeof( short ) ;
   decimal->sign    = scale & SDB_DECIMAL_SIGN_MASK ;
   decimal->dscale  = scale & SDB_DECIMAL_DSCALE_MASK ;
   decimal->weight  = weight ;
   decimal->isOwn = 0 ;
   decimal->buff = NULL ;
   decimal->digits = (short *)value  ;

   return 0 ;
#endif
}
