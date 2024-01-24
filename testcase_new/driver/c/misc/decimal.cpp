/*****************************************************************************
 * @Description : decimal test
 *                
 * @Modify:       Ting YU  Init
 *                2017-09-26
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "client.h"
#include "testcommon.hpp"
#include "arguments.hpp"

const CHAR* csName = "decimalTestCs" ;
const CHAR* clName = "decimalTestCl" ;

TEST( decimal, zero )
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec ); 
   INT32 rc = SDB_OK; 
   INT32 intVal = 12;
   bson_bool_t rst = 0;

   //not zero           
   rc = decimal_from_int( intVal, &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rst = decimal_is_zero( &dec );  //no --> return 0 
   ASSERT_EQ( 0, rst );

   //zero
   decimal_set_zero( &dec );

   rst = decimal_is_zero( &dec );  //yes --> return 1
   ASSERT_EQ( 1, rst );

   //clean           
   decimal_free( &dec );   
}

TEST( decimal, special_type )
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec ); 
   INT32 rc = SDB_OK; 
   bson_bool_t rst = 0;

   //init value: 0 
   rst = decimal_is_special( &dec );
   ASSERT_EQ( 0, rst );  
   rst = decimal_is_nan( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_max( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_min( &dec );
   ASSERT_EQ( 0, rst );

   //nan         
   decimal_set_nan( &dec );

   rst = decimal_is_special( &dec );
   ASSERT_EQ( 1, rst );  
   rst = decimal_is_nan( &dec );
   ASSERT_EQ( 1, rst );
   rst = decimal_is_max( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_min( &dec );
   ASSERT_EQ( 0, rst );

   //max
   decimal_set_max( &dec );

   rst = decimal_is_special( &dec );
   ASSERT_EQ( 1, rst );   
   rst = decimal_is_nan( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_max( &dec );
   ASSERT_EQ( 1, rst );
   rst = decimal_is_min( &dec );
   ASSERT_EQ( 0, rst );

   //min
   decimal_set_min( &dec );

   rst = decimal_is_special( &dec );
   ASSERT_EQ( 1, rst );   
   rst = decimal_is_nan( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_max( &dec );
   ASSERT_EQ( 0, rst );
   rst = decimal_is_min( &dec );
   ASSERT_EQ( 1, rst );

   //clean           
   decimal_free( &dec );   
}

TEST( decimal, add ) 
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775806;
   INT64 right = 2;
   CHAR expStr[] = "9223372036854775808";
   INT32 size = 0;

   //add
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rstDec );
   rc = decimal_add( &lDec, &rDec, &rstDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //check
   rc = decimal_to_str_get_len( &rstDec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rstDec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );
   decimal_free( &rstDec );   
}

TEST( decimal, sub )//long
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775806;
   INT64 right = 2;

   //sub
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rstDec );
   rc = decimal_sub( &lDec, &rDec, &rstDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //check  
   INT64 rstLong = decimal_to_long( &rstDec );
   ASSERT_EQ( left - right, rstLong );  

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );
   decimal_free( &rstDec );   
}

TEST( decimal, mul )
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775806;
   INT64 right = 2;
   CHAR expStr[] = "18446744073709551612";
   INT32 size = 0;

   //mul
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rstDec );
   rc = decimal_mul( &lDec, &rDec, &rstDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //check
   rc = decimal_to_str_get_len( &rstDec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rstDec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );
   decimal_free( &rstDec );   
}

TEST( decimal, div )
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775806;
   INT64 right = 2;
   CHAR expStr[] = "4611686018427387903";
   INT32 size = 0;

   //div
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rstDec );
   rc = decimal_div( &lDec, &rDec, &rstDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //check
   rc = decimal_to_str_get_len( &rstDec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rstDec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );
   decimal_free( &rstDec );   
}

TEST( decimal, mod )
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775807;
   INT64 right = 2;
   CHAR expStr[] = "1";
   INT32 size = 0;

   //mod
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rstDec );
   rc = decimal_mod( &lDec, &rDec, &rstDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //check
   rc = decimal_to_str_get_len( &rstDec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rstDec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );
   decimal_free( &rstDec );   
}

TEST( decimal, round )  //double
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec );     
   FLOAT64 value = 19.123456;
   INT64 scale = 4;
   CHAR expStr[] = "19.1235";
   INT32 size = 0;
   INT32 rc = SDB_OK;

   //round
   decimal_init( &dec );             
   rc = decimal_from_double( value, &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = decimal_round( &dec, scale );
   ASSERT_EQ( SDB_OK, rc ); 

   //check by str
   rc = decimal_to_str_get_len( &dec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &dec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //check by double
   FLOAT64 rstDouble = decimal_to_double( &dec );
   ASSERT_EQ( 19.1235, rstDouble );

   //clean           
   decimal_free( &dec );
}

TEST( decimal, abs )  //int
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec ); 
   INT32 rc = SDB_OK; 
   INT32 initVal = -12;
   INT32 expVal = 12;   

   //abs           
   rc = decimal_from_int( initVal, &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = decimal_abs( &dec ); 
   ASSERT_EQ( SDB_OK, rc );

   //check
   INT32 rstVal = decimal_to_int( &dec );
   ASSERT_EQ( expVal, rstVal );

   //clean           
   decimal_free( &dec );   
}

TEST( decimal, ceil )
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec ); 
   bson_decimal rst; 
   decimal_init( &rst );
   INT32 rc = SDB_OK; 
   FLOAT64 doubleVal = 9.5;
   CHAR expStr[] = "10";
   INT32 size = 0;   

   //ceil           
   rc = decimal_from_double( doubleVal, &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = decimal_ceil( &dec, &rst ); 
   ASSERT_EQ( SDB_OK, rc );

   //check   
   rc = decimal_to_str_get_len( &rst, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rst, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &dec );   
}

TEST( decimal, floor )
{ 
   //init variable    
   bson_decimal dec; 
   decimal_init( &dec ); 
   bson_decimal rst; 
   decimal_init( &rst );
   INT32 rc = SDB_OK; 
   FLOAT64 doubleVal = 8.9;
   CHAR expStr[] = "8";
   INT32 size = 0;   

   //floor           
   rc = decimal_from_double( doubleVal, &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = decimal_floor( &dec, &rst ); 
   ASSERT_EQ( SDB_OK, rc );

   //check   
   rc = decimal_to_str_get_len( &rst, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &rst, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &dec );   
}


TEST( decimal, compare_copy )
{ 
   //init variable    
   bson_decimal lDec; 
   decimal_init( &lDec ); 
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   INT32 rc = SDB_OK; 
   INT64 left = 9223372036854775806;
   INT64 right = -9223372036854775806;
   INT32 size = 0;
   INT32 rst = 0;

   //init
   decimal_init( &lDec );             
   rc = decimal_from_long( left, &lDec );
   ASSERT_EQ( SDB_OK, rc ); 

   decimal_init( &rDec );
   rc = decimal_from_long( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //compare
   rst = decimal_cmp( &lDec, &rDec );
   ASSERT_EQ( 1, rst ); 

   //copy
   rc = decimal_copy( &lDec, &rDec );
   ASSERT_EQ( SDB_OK, rc );

   //compare
   rst = decimal_cmp( &lDec, &rDec );
   ASSERT_EQ( 0, rst );    

   //clean           
   decimal_free( &lDec );
   decimal_free( &rDec );   
}


TEST( decimal, get_precision )
{ 
   //init variable    
   bson_decimal dec; 
   INT32 precision = 100;
   INT32 scale = 6;
   INT32 rstPrecision = 0;
   INT32 rstScale = 0;   
   INT32 rc = SDB_OK; 
   FLOAT64 value = 0.1234567;
   CHAR expStr[] = "0.123457";

   //init
   rc = decimal_init1( &dec, precision, scale ); 
   ASSERT_EQ( SDB_OK, rc );

   rc = decimal_from_double( value, &dec );
   ASSERT_EQ( SDB_OK, rc );            

   //get precision
   decimal_get_typemod( &dec, &rstPrecision, &rstScale );
   ASSERT_EQ( precision, rstPrecision );
   ASSERT_EQ( scale, rstScale );

   //check value
   INT32 size = 0;
   rc = decimal_to_str_get_len( &dec, &size );
   ASSERT_EQ( SDB_OK, rc );

   CHAR* strValue = (CHAR*)malloc( size );
   rc = decimal_to_str( &dec, strValue, size );
   ASSERT_EQ( SDB_OK, rc );

   ASSERT_STREQ( expStr, strValue );

   //clean           
   decimal_free( &dec );   
}

TEST( decimal, append )
{     
   bson obj;
   bson_init( &obj );
   bson_iterator it; 
   bson_decimal dec; 
   decimal_init( &dec ); 
   sdbCursorHandle cursor = 0;
   INT32 rc = SDB_OK;

   CHAR strVal[] = "922330.123451";
   INT32 precision = 20;
   INT32 scale = 5;
   INT32 i = 1;
   FLOAT64 expRst2 = 922330.12345;
   FLOAT64 expRst3 = 922330.123451;

   //create cl
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;   
   ASSERT_EQ( SDB_OK, rc ) ;

   //construct bson 
   decimal_init( &dec ); 
   rc = decimal_from_int( i, &dec );
   ASSERT_EQ( SDB_OK, rc );  
   rc = bson_append_decimal( &obj, "a", &dec );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = bson_append_decimal2( &obj, "a2", strVal, precision, scale );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = bson_append_decimal3( &obj, "a3", strVal );
   ASSERT_EQ( SDB_OK, rc ); 

   rc = bson_finish( &obj );
   ASSERT_EQ( SDB_OK, rc );

   //insert
   rc = sdbDelete ( cl, NULL, NULL );
   ASSERT_EQ( SDB_OK, rc );     
   rc = sdbInsert ( cl, &obj );
   ASSERT_EQ( SDB_OK, rc );

   //query            
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc );

   bson_init( &obj ); 
   while( !( rc=sdbNext( cursor, &obj ) ) )           //obj -->record
   { 
      //bson_append_decimal    
      bson_find( &it, &obj, "a");               //it -->field     

      decimal_init( &dec );         
      bson_iterator_decimal( &it, &dec );
      FLOAT64 iRst = decimal_to_int( &dec );
      ASSERT_EQ( i, iRst );

      //bson_append_decimal2    
      bson_find( &it, &obj, "a2");               //it -->field     

      decimal_init( &dec );         
      bson_iterator_decimal( &it, &dec );
      FLOAT64 dRst2 = decimal_to_double( &dec );
      ASSERT_EQ( expRst2, dRst2 );

      //bson_append_decimal3   
      bson_find( &it, &obj, "a3");

      decimal_init( &dec );         
      bson_iterator_decimal( &it, &dec );
      FLOAT64 dRst3 = decimal_to_double( &dec );
      ASSERT_EQ( expRst3, dRst3 );

      bson_destroy( &obj );
      bson_init( &obj );
   }

   //clean 
   rc = sdbCloseCursor( cursor );
   ASSERT_EQ( SDB_OK, rc );
   decimal_free( &dec );
   bson_destroy( &obj );
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;	   
}

TEST( decimal, boundary ) //str
{     
   bson obj;
   bson_init( &obj );
   bson_iterator it; 
   bson_decimal dec; 
   decimal_init( &dec ); 
   sdbCursorHandle cursor = 0;
   INT32 rc = SDB_OK;

   INT32 maxWeight = 131072;
   INT32 maxScale = 16383;   
   INT32 maxStrLen = maxScale + maxWeight + 1 + 1;

   //init string
   CHAR* strVal = (CHAR*)malloc( maxStrLen * sizeof(CHAR) );
   *strVal = '\0' ;
   for( INT32 i = 0; i < maxWeight; i++ )
   {
      strcat( strVal, "1" );
   }
   strcat( strVal, "." );
   for( INT32 j = 0; j < maxScale; j++ )
   {
      strcat( strVal, "2" );
   }
   printf("string len: %zd\n", strlen(strVal) );

   //create cl
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = createNormalCsCl( db, &cs, &cl, csName, clName );   
   ASSERT_EQ( SDB_OK, rc );

   //construct bson        
   decimal_init( &dec ); 
   rc = decimal_from_str( strVal, &dec );
   ASSERT_EQ( SDB_OK, rc ); 
   bson_append_decimal( &obj, "strType", &dec );

   rc = bson_finish( &obj );
   ASSERT_EQ( SDB_OK, rc );

   //insert
   rc = sdbDelete ( cl, NULL, NULL );
   ASSERT_EQ( SDB_OK, rc );     
   rc = sdbInsert ( cl, &obj );
   ASSERT_EQ( SDB_OK, rc );

   //query            
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor );
   ASSERT_EQ( SDB_OK, rc );

   bson_init( &obj ); 
   while( !( rc=sdbNext( cursor, &obj ) ) )           //obj -->record
   {       
      //strType    
      bson_find( &it, &obj, "strType");      

      decimal_init( &dec );         
      bson_iterator_decimal( &it, &dec );     

      INT32 size = 0;
      rc = decimal_to_str_get_len( &dec, &size );
      ASSERT_EQ( SDB_OK, rc );

      CHAR* rstStr = (CHAR*)malloc( size );
      rc = decimal_to_str( &dec, rstStr, size );
      ASSERT_EQ( SDB_OK, rc );

      ASSERT_STREQ( strVal, rstStr );

      bson_destroy( &obj );
      bson_init( &obj );
      free( rstStr );
   }

   //clean  
   rc = sdbCloseCursor( cursor );
   ASSERT_EQ( SDB_OK, rc );
   decimal_free( &dec );
   bson_destroy( &obj );
   free( strVal );
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

TEST( decimal, out_scale )
{     
   bson_decimal dec; 
   decimal_init( &dec ); 
   INT32 rc = SDB_OK;

   INT32 maxWeight = 131072;
   INT32 maxScale = 16383;   
   INT32 maxStrLen = maxScale + maxWeight + 1 + 1;

   //init string
   CHAR* strVal = (CHAR*)malloc( ( maxStrLen + 1 ) * sizeof(CHAR) );
   *(strVal) = '\0' ;
   for( INT32 i = 0; i < maxWeight; i++ )   
   {
      strcat( strVal, "1" );
   }   
   strcat( strVal, "." );
   for( INT32 j = 0; j < maxScale; j++ )
   {
      strcat( strVal, "2" );
   }
   strcat( strVal, "3" );  //out of max scale
   printf("string len: %zd\n", strlen(strVal) );

   //construct bson        
   decimal_init( &dec ); 
   rc = decimal_from_str( strVal, &dec );
   ASSERT_EQ( -6, rc ); 

   //clean  
   decimal_free( &dec );
   free( strVal );
}

TEST( decimal, out_weight )
{     
   bson_decimal dec; 
   decimal_init( &dec ); 
   INT32 rc = SDB_OK;

   INT32 maxWeight = 131072;
   INT32 maxScale = 16383;   
   INT32 maxStrLen = maxScale + maxWeight + 1 + 1;

   //init string
   CHAR* strVal = (CHAR*)malloc( ( maxStrLen + 1 ) * sizeof(CHAR) );
   *(strVal) = '\0' ;
   for( INT32 i = 0; i < maxWeight; i++ )   
   {
      strcat( strVal, "1" );
   }
   strcat( strVal, "3" );  //out of max weight
   strcat( strVal, "." );
   for( INT32 j = 0; j < maxScale; j++ )
   {
      strcat( strVal, "2" );
   }
   printf("string len: %zd\n", strlen(strVal) );

   //construct bson        
   decimal_init( &dec ); 
   rc = decimal_from_str( strVal, &dec );
   ASSERT_EQ( -6, rc ); 

   //clean  
   decimal_free( &dec );
   free( strVal );
}


TEST( decimal, add_out_boundary )
{     
   bson_iterator it; 
   bson_decimal dec; 
   decimal_init( &dec );
   bson_decimal rDec; 
   decimal_init( &rDec ); 
   bson_decimal rstDec; 
   decimal_init( &rstDec );  
   sdbCursorHandle cursor = 0;
   INT32 rc = SDB_OK;

   INT32 maxWeight = 131072;
   INT32 maxScale = 16383;   
   INT32 maxStrLen = maxScale + maxWeight + 1 + 1;

   //init left
   CHAR* strVal = (CHAR*)malloc( (maxStrLen + 1) * sizeof(CHAR) );
   *(strVal) = '\0' ;
   for( INT32 i = 0; i < maxWeight; i++ )
   {
      strcat( strVal, "9" );
   }
   strcat( strVal, "." );
   for( INT32 j = 0; j < maxScale; j++ )
   {
      strcat( strVal, "8" );
   }   
   printf("string len: %zd\n", strlen(strVal) );

   decimal_init( &dec ); 
   rc = decimal_from_str( strVal, &dec );
   ASSERT_EQ( SDB_OK, rc );    

   //init right
   INT32 right = 1;
   decimal_init( &rDec );
   rc = decimal_from_int( right, &rDec );
   ASSERT_EQ( SDB_OK, rc ); 

   //add
   decimal_init( &rstDec );
   rc = decimal_add( &dec, &rDec, &rstDec );
   ASSERT_EQ( -6, rc ); 

   //clean  
   decimal_free( &dec );
   decimal_free( &rDec );
   decimal_free( &rstDec );
   free( strVal );
}
