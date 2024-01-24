#include "utilTypeCast.h"
#include <math.h>

#define UTIL_NUM_INT32_MAX 2147483647
//Note: The last bit of the boundary value is not included here.
#define UTIL_NUM_INT64_MAX_THRESHOLD 922337203685477580

#define UTIL_NUM_TYPE_INT32   0
#define UTIL_NUM_TYPE_INT64   1
#define UTIL_NUM_TYPE_FLOAT64 2
#define UTIL_NUM_TYPE_DECIMAL 3

#define DOUBLE_PRECISION   (15)
#define DOUBLE_MAX_EXP     (308)
#define DOUBLE_MIN_EXP     (-308)
#define DOUBLE_BOUND       (1.79)

static FLOAT64 powersOf10[] = {
   10.,
   100.,
   1.0e4,
   1.0e8,
   1.0e16,
   1.0e32,
   1.0e64,
   1.0e128,
   1.0e256
} ;

INT32 _digit( INT32 leftPos, INT32 pointPos, INT32 rightPos ) ;

/*
 *  xxxxxxxxxxx . yyyyyyyyyyy
 *  |           |           |
 * leftPos   pointPos     rightPos
 *
 * Integer digits = max( pointPos - leftPos, 0 )
 * Decimal digits = rightPos - pointPos
 * Scientific notation offset = 
 *      1. leftPos < pointPos && pointPos < rightPos: rightPos - leftPos
 *      2. leftPos < pointPos && pointPos = rightPos: rightPos - leftPos
 *      3. leftPos > pointPos && pointPos < rightPos: rightPos - leftPos + 1
 *
 */
SDB_EXPORT INT32 utilStrToNumber( const CHAR* data, INT32 length,
                                  INT32 *type, utilNumberVal *value,
                                  INT32 *valueLength )
{
   INT32 rc       = SDB_OK ;
   INT32 len      = 0 ;
   INT32 sign     = 1 ;
   INT32 n1       = 0 ;
   INT32 numType  = UTIL_NUM_TYPE_INT32 ;

   INT32 pointPos = 0 ;
   INT32 leftPos  = -1 ;
   INT32 rightPos = 0 ;

   INT32 subscale = 0 ;
   BOOLEAN hasInteger = FALSE ;
   INT64 n2  = 0 ;
   FLOAT64 n = 0 ;
   const CHAR *pStr = data ;

   //step 1
   if ( *pStr == '#' )
   {
      //#xxx
      ++pStr ;
      ++len ;
   }

   if ( *pStr == '-' )
   {
      //-xxx
      sign = -1 ;
      ++pStr ;
      ++len ;
   }
   else if ( *pStr == '+' )
   {
      //+xxx
      sign = 1 ;
      ++pStr ;
      ++len ;
   }

   //step 2
   while ( *pStr == '0' )
   {
      //0xxxxx
      ++pStr ;
      ++len ;
      hasInteger = TRUE ;
   }

   //step 3
   {
      INT64 frac = 0 ;

      while ( *pStr >= '0' && *pStr <= '9' )
      {
         //<number>xxxx
         INT32 num = *pStr - '0' ;

         if( rightPos > 18 ||
             frac > UTIL_NUM_INT64_MAX_THRESHOLD ||
             ( frac == UTIL_NUM_INT64_MAX_THRESHOLD && sign == 1 && num > 7 ) ||
             ( frac == UTIL_NUM_INT64_MAX_THRESHOLD && sign == -1 && num > 8 ) )
         {
            //frac * 10 + num is greater than the long long range
            numType = UTIL_NUM_TYPE_DECIMAL ;

            if( 0 < length )
            {
               ++pStr ;
               ++len ;
               goto done ;
            }
         }

         frac = 10 * frac + num ;

         leftPos = 1 ;
         ++pointPos ;
         ++rightPos ;

         ++pStr ;
         ++len ;

         hasInteger = TRUE ;
      }

      n1 = (INT32)frac * sign ;
      n2 = frac * sign ;

      if( numType == UTIL_NUM_TYPE_INT32 &&
          ( 10 < rightPos || n2 != (INT64)n1 ) )
      {
         numType = UTIL_NUM_TYPE_INT64 ;
      }

      //step 4
      if ( *pStr == '.' )
      {
         //<number>.
         INT32 zeroDecimal = 0 ;

         if( 0 == pointPos )
         {
            pointPos = 1 ;
         }
         if( 0 == rightPos )
         {
            rightPos = 1 ;
         }

         ++pointPos ;
         ++rightPos ;

         ++pStr ;
         ++len ;

         if( ( *pStr < '0' || *pStr > '9' ) &&
             ( *pStr != 'e' && *pStr != 'E' ) )
         {
            if ( hasInteger )
            {
               goto finish ;
            }
            else
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
         }
         else if ( FALSE == hasInteger && ( 'e' == *pStr || 'E' == *pStr ) )
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         if( numType != UTIL_NUM_TYPE_DECIMAL )
         {
            numType = UTIL_NUM_TYPE_FLOAT64 ;
         }

         //<number>.xxx
         while ( *pStr >= '0' && *pStr <= '9' )
         {
            INT32 num = *pStr - '0' ;

            if( num > 0 )
            {
               if( 0 > leftPos )
               {
                  leftPos = pointPos + zeroDecimal + 1 ;
               }

               rightPos += ( zeroDecimal + 1 ) ;

               if( _digit( leftPos, pointPos, rightPos ) < 18 )
               {
                  for( ; zeroDecimal > 0; --zeroDecimal )
                  {
                     frac *= 10 ;
                  }

                  frac = 10 * frac + num ;
               }

               zeroDecimal = 0 ;
            }
            else
            {
               ++zeroDecimal ;
            }

            ++pStr ;
            ++len ;
         }
      }
      else
      {
         ++pointPos ;
      }

      n = frac ;

      if( 0 > leftPos )
      {
         leftPos = 1 ;
      }

      if( UTIL_NUM_TYPE_FLOAT64 == numType &&
          _digit( leftPos, pointPos, rightPos ) > DOUBLE_PRECISION )
      {
         /*
          * The effective number is greater than 15 digits
          */
         numType = UTIL_NUM_TYPE_DECIMAL ;

         if( 0 < length )
         {
            goto done ;
         }
      }
   }

   if ( pStr == data )
   {
      // no digits
      rc = SDB_INVALIDARG;
      goto error;
   }

   //step 5
   if ( *pStr == 'e' || *pStr == 'E' )
   {
      //<number>[e/E]xxx
      INT32 signsubscale = 1 ;

      if( UTIL_NUM_TYPE_DECIMAL != numType )
      {
         numType = UTIL_NUM_TYPE_FLOAT64 ;
      }

      ++pStr ;
      ++len ;

      if ( *pStr == '+' )
      {
         ++pStr ;
         ++len ;
      }
      else if ( *pStr == '-' )
      {
         signsubscale = -1 ;
         ++pStr ;
         ++len ;
      }

      while ( *pStr >= '0' && *pStr <= '9' )
      {
         subscale = ( subscale * 10 ) + ( *pStr - '0' ) ;
         ++pStr ;
         ++len ;
      }

      if( -1 == signsubscale )
      {
         subscale = -subscale ;
      }
   }

   //step 6
   if ( UTIL_NUM_TYPE_FLOAT64 == numType && n > 0.0 )
   {
      INT32 offset   = _digit( leftPos, pointPos, rightPos ) - 1 ;
      INT32 fracExp  = pointPos >= rightPos ? 0 : pointPos - rightPos ;
      INT32 exponent = 0 ;
      FLOAT64 dblExp = 1.0 ;
      FLOAT64 tmpN = 0.0 ;
      FLOAT64 *d = NULL ;

      subscale = fracExp + subscale ;
      exponent = offset + subscale ;

      if( exponent > DOUBLE_MAX_EXP || exponent < DOUBLE_MIN_EXP )
      {
         /*
          * exponent > 308 or exponent < -308
          */
         numType = UTIL_NUM_TYPE_DECIMAL ;
         goto done ;
      }
      else if( exponent == DOUBLE_MAX_EXP || exponent == DOUBLE_MIN_EXP )
      {
         INT32 tmpFracExp = offset ;
         FLOAT64 tmpDblExp = 1.0 ;

         tmpN = n ;

         for ( d = powersOf10; tmpFracExp > 0; tmpFracExp >>= 1, ++d )
         {
            if ( tmpFracExp & 01 )
            {
               tmpDblExp *= *d ;
            }
         }

         tmpN /= tmpDblExp ;

         if ( exponent == DOUBLE_MAX_EXP && tmpN > DOUBLE_BOUND )
         {
            /*
             * The maximum value of floating point number
             * should not exceed +/-1.79E+308
             */
            numType = UTIL_NUM_TYPE_DECIMAL ;
            goto done ;
         }
         else if ( exponent == DOUBLE_MIN_EXP && tmpN > DOUBLE_BOUND )
         {
            /*
             * The minimum value of floating point number
             * should not exceed +/-1.79E-308
             */
            numType = UTIL_NUM_TYPE_DECIMAL ;
            goto done ;
         }
      }

      if( subscale > DOUBLE_MAX_EXP  || subscale < DOUBLE_MIN_EXP )
      {
         n = tmpN ;
         subscale = exponent ;
      }

      {
         INT32 signsubscale = 1 ;

         if ( subscale < 0 )
         {
            signsubscale = -1 ;
            subscale = -subscale ;
         }
         else
         {
            signsubscale = 1 ;
         }

         for ( d = powersOf10; subscale > 0; subscale >>= 1, ++d )
         {
            if ( subscale & 01 )
            {
               dblExp *= *d ;
            }
         }

         if ( signsubscale == 1 )
         {
            n *= dblExp ;
         }
         else
         {
            n /= dblExp ;
         }
      }

      if ( sign < 0 )
      {
         n = -n ;
      }
   }

finish:
   if( value )
   {
      if ( numType == UTIL_NUM_TYPE_FLOAT64 )
      {
         value->doubleVal = n ;
      }
      else if( numType == UTIL_NUM_TYPE_INT32 )
      {
         value->intVal = n1 ;
      }
      else if( numType == UTIL_NUM_TYPE_INT64 )
      {
         value->longVal = n2 ;
      }
   }

done:
   if( type )
   {
      *type = numType ;
   }

   if ( valueLength )
   {
      *valueLength = len ;
   }
   return rc ;
error:
   goto done ;
}

INT32 _digit( INT32 leftPos, INT32 pointPos, INT32 rightPos )
{
   INT32 digit ;

   if( leftPos < pointPos && pointPos <= rightPos )
   {
      digit = rightPos - leftPos ;
   }
   else
   {
      digit = rightPos - leftPos + 1 ;
   }

   return digit ;
}