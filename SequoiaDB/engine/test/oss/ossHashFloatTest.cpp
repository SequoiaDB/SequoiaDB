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

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include "../../bson/lib/md5.hpp"
#include "../../bson/lib/md5.h"
#include "ossUtil.hpp"

using namespace std;

UINT64 MD5[2] = { 0 } ;
vector<FLOAT64> nums ;
vector<FLOAT64> specialnums ;
INT32 range = 100000 ;

UINT32 nativeDouble2UINT32( FLOAT64 num )
{
   UINT32 m1 ;
   m1 = num;
   return m1 ;
}

UINT64 nativeDouble2UINT64( FLOAT64 num )
{
   UINT64 m1 ;
   m1 = num;
   return m1 ;
}

BOOLEAN _checkResult( UINT64* val )
{
   if( val[0] == MD5[0] && val[1] == MD5[1] ) return TRUE ;
   else return FALSE ;
}

void printmd5( UINT64* val )
{
   for ( int i = 0; i < 2 ; i++ )
   {
      cout << hex << UINT64( val[i] ) ;
      cout << endl;
   }
}

void setMD5( UINT64* val )
{
   val[0] = 0x65cabd0e75b1534d ;
   val[1] = 0x832ba696c67797b8 ;
}

void getExpectMD5()
{

   FLOAT64 temp;
   UINT32 n1;
   UINT64 n2;

   md5_state_t st ;
   md5::md5digest digest ;
   md5_init( &st );

   for ( UINT32 idx = 0 ; idx < nums.size() ; idx++ )
   {
      for ( INT32 i = -range ; i < range ; i++ )
      {
         temp= nums[idx] + i ;
         n1 = nativeDouble2UINT32( temp ) ;
         md5_append( &st, ( const md5_byte_t * )( &n1 ), sizeof( n1 ) ) ;
      }
   }
   for ( UINT32 idx = 0 ; idx < specialnums.size() ; idx++ )
   {
         n1 = nativeDouble2UINT32( specialnums[idx] ) ;
         md5_append( &st, ( const md5_byte_t * )( &n1 ), sizeof( n1 ) ) ;
   }
   cout << "nativeDouble2UINT32 finish\n" ;

   for ( UINT32 idx = 0 ; idx < nums.size() ; idx++ )
   {
      for ( INT32 i = -range ; i < range ; i++ )
      {
         temp = nums[idx] + i ;
         n2 = nativeDouble2UINT64( temp ) ;
         md5_append( &st, ( const md5_byte_t * )( &n2 ), sizeof( n2 ) ) ;
      }
   }
   for ( UINT32 idx = 0 ; idx < specialnums.size() ; idx++ )
   {
         n2 = nativeDouble2UINT64( specialnums[idx] ) ;
         md5_append( &st, ( const md5_byte_t * )( &n2 ), sizeof( n2 ) ) ;
   }
   cout << "nativeDouble2UINT64 finish\n" ;

   md5_finish( &st, digest ) ;
   printmd5( (UINT64*)digest ) ;

}

TEST( ossTest, ossHashFloat64_test )
{

   //testcase1: double is ten times bigger than UINT64 can represent
   nums.push_back(-39223372036854775808.12345) ;
   nums.push_back(39223372036854775808.12345) ;

   //testcase2: double is at the border of UINT64
   nums.push_back(-1.0 * FLOAT64(0xFFFFFFFFFFFFFFFF)) ;
   nums.push_back(1.0 * FLOAT64(0xFFFFFFFFFFFFFFFF)) ;

   //testcase3: double is at the border of INT64
   nums.push_back(-9223372036854775808.0) ;
   nums.push_back(9223372036854775807.0) ;

   //testcase4: double is at the border of UINT32
   nums.push_back(-1.0 * (0xFFFFFFFF)) ;
   nums.push_back(1.0 * (0xFFFFFFFF)) ;

   //testcase6: double is at the border of INT32
   nums.push_back(-2147483648.0) ;
   nums.push_back(2147483647.0) ;

   //testcase7: double is 0
   nums.push_back(0) ;

   //testcase8: in order to covering more decimals cases
   FLOAT64 e = 1;
   for ( int t = 0 ; t < 10 ; t++ )
   {
      e = e*0.8 ;
      nums.push_back( -e ) ;
      nums.push_back( e ) ;
   }

   //testcase9: in order to covering more cases
   e = 1;
   for ( int t = 0 ; t <= 15 ; t++ )
   {
      e = e*0.12345 ;
      nums.push_back(-39223372036854775808.12345 * e ) ;
      nums.push_back(39223372036854775808.12345 * e ) ;
   }

#if defined (_LINUX) || defined (_AIX)
   //in windows environment does not support divided by 0 to get NaN,inf,-inf
   //so we just add testcast10 in linux
   //testcase10: other special value NaN, inf, -inf
   specialnums.push_back( 0.0/0.0 ) ;
   specialnums.push_back( 1.0/0.0 ) ;
   specialnums.push_back( -1.0/0.0 ) ;
#endif


   //getExpectMD5();

   setMD5( MD5 ) ;

   if ( true )
   {
      FLOAT64 temp ;
      UINT32 n1 ;
      UINT64 n2 ;

      md5_state_t st ;
      md5::md5digest digest ;
      md5_init( &st ) ;

      for ( UINT32 idx = 0 ; idx < nums.size() ; idx++ )
      {
         for ( INT32 i = -range ; i < range ; i++ )
         {
            temp = nums[idx] + i ;
            n1 = ossDoubleToUINT32( temp ) ;
            md5_append( &st, ( const md5_byte_t * )( &n1 ), sizeof( n1 ) ) ;
         }
      }
      for ( UINT32 idx = 0 ; idx < specialnums.size() ; idx++ )
      {
            n1 = ossDoubleToUINT32( specialnums[idx] ) ;
            md5_append( &st, ( const md5_byte_t * )( &n1 ), sizeof( n1 ) ) ;
      }
      cout << "funcout32 finish\n" ;

      for ( UINT32 idx = 0 ; idx < nums.size() ; idx++ )
      {
         for ( INT32 i = -range ; i < range ; i++ )
         {
            temp = nums[idx] + i ;
            n2 = ossDoubleToUINT64( temp ) ;
            md5_append( &st, ( const md5_byte_t * )( &n2 ), sizeof( n2 ) ) ;
         }
      }
      for ( UINT32 idx = 0 ; idx < specialnums.size() ; idx++ )
      {
            n2 = ossDoubleToUINT64( specialnums[idx] ) ;
            md5_append( &st, ( const md5_byte_t * )( &n2 ), sizeof( n2 ) ) ;
      }
      cout << "funcout64 finish\n" ;

      md5_finish( &st, digest ) ;
      printmd5( (UINT64*)digest ) ;

      ASSERT_TRUE( _checkResult( (UINT64*)digest ) ) ;

   }
}

