/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-15173, seqDB-15172:ʹ��sdbGetQueryMeta��ȡ��ѯԪ����     
 * @Modify:        wenjing wang Init
 *                 2018-04-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

enum Check_Res{
  Check_Res_FULL_MATCH,
  Check_Res_EXIST,
  Check_Res_NOTEXIST
};

const char *ShardingType = "ShardingType" ;
const char *ShardingKey = "ShardingKey" ;
const char *range = "range" ;
const char *CompressionType = "CompressionType" ;
const char *CompressionTypeDesc = "CompressionTypeDesc" ;
const char *lzw = "lzw" ;
const char *hashVal = "hash" ;

class alterCLTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      sdbCSHandle cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "altercl_15174";
      clName = csName;
      bson_init( &opt ) ;
      testBase::SetUp() ;
      
      rc = sdbGetCollectionSpace( db, csName.c_str(), &cs ) ;
      if ( rc == -34 )
      {
         rc = sdbCreateCollectionSpaceV2( db, csName.c_str(), NULL, &cs ) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get cs " << csName ;
      
      rc = sdbGetCollection1( cs, clName.c_str(), &cl ) ;
      if ( rc == -23 )
      {
         rc = sdbCreateCollection( cs, clName.c_str(), &cl) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get cl " << clName ;
      
      if ( cs != SDB_INVALID_HANDLE )
      {
         sdbReleaseCS( cs ) ;
      }
   }
   
   void TearDown()
   {
      bson_destroy( &opt ) ;
      INT32 rc = SDB_OK ;
      if( shouldClear() && cl != SDB_INVALID_HANDLE )
      {
         rc = sdbDropCollectionSpace( db, csName.c_str() ) ; 
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
         sdbReleaseCollection( cl ) ;
      } 
      testBase::TearDown() ;
   }
protected:
   bson opt ;
   Check_Res checkCLAttr( bson * opt) ;
   sdbCollectionHandle cl ;
   std::string csName ;
   std::string clName ;
   
};

BOOLEAN compareObj(bson *l, bson *r, int times = 1)
{
   bson_iterator lit ;
   bson_iterator rit ;
   BOOLEAN ret = FALSE ;
   bson_iterator_init( &lit, l );
   bson_type ltype ;
   while(BSON_EOO != (ltype = bson_iterator_next(&lit)))
   {
      const char *key = bson_iterator_key(&lit) ;
      bson_type rtype = bson_find( &rit, r, key );
      if ( ltype != rtype )
      {
         ret = FALSE;
         break ;
      }
      if (rtype == BSON_OBJECT && rtype == BSON_ARRAY )
      {
         bson lsubObj;
         bson rsubObj;
         bson_init( &lsubObj ) ;
         bson_init( &rsubObj ) ;
         bson_iterator_subobject( &lit, &lsubObj ) ;
         bson_iterator_subobject( &rit, &rsubObj ) ;
         compareObj( &lsubObj, &rsubObj ) ;
         bson_destroy( &lsubObj );
         bson_destroy( &rsubObj );
      }
      else if (rtype == BSON_INT )
      {
         ret = bson_iterator_int(&lit) == bson_iterator_int(&rit) ;
      }
      else if (rtype == BSON_STRING )
      {
         ret = !strcmp(bson_iterator_string(&lit), bson_iterator_string(&rit) ) ;
      }
   }
   
   if ( ret && times != 0)
   {
      return compareObj(r, l, 0);
   } 
   
   return ret ;
}

Check_Res alterCLTest::checkCLAttr( bson * opt ) 
{
   INT32 rc = SDB_OK ;
   Check_Res res = Check_Res_NOTEXIST ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   std::string fullName = csName + "." +
                          clName ;
   bson cond, ret;
   bson_init( &ret );
   bson_init( &cond );
   bson_append_string( &cond, "Name", fullName.c_str() ) ;
   bson_finish( &cond );
   
   rc = sdbGetSnapshot( db, 8, &cond, NULL, NULL, &cursor ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbGetSnapshot (" << 8 << "), rc = " << rc << std::endl ;
      goto done ;
   }
   
   res = Check_Res_NOTEXIST ;
   while ( SDB_DMS_EOC != sdbNext( cursor, &ret) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, opt );
      bson_type ctype ;
      while(BSON_EOO != (ctype = bson_iterator_next(&it)))
      {
         const char *key = bson_iterator_key(&it) ;
         if( !strcmp( key, CompressionType ) )
         {  
            bson_iterator tit ;
            bson_type type = bson_find( &tit, &ret, CompressionTypeDesc );
            if ( type == BSON_STRING && 
                 !strcmp(bson_iterator_string( &tit ), bson_iterator_string(&it)))
            {
               res = Check_Res_FULL_MATCH ;
               continue ;
            }
            else
            {
              res = type == BSON_EOO ? Check_Res_NOTEXIST : Check_Res_EXIST ;
              break ;
            }
         }
         
         bson_iterator tit ;
         bson_type type = bson_find( &tit, &ret, key);
         if ( ctype == type &&
              type == BSON_STRING && 
              !strcmp(bson_iterator_string( &tit ), bson_iterator_string(&it)))
         {
            res = Check_Res_FULL_MATCH ;
         }
         else if ( ctype == type && 
                   type == BSON_OBJECT ) 
         { 
            bson subObj;
            bson rsubObj;
            bson_init( &subObj ) ;
            bson_init( &rsubObj ) ;
            bson_iterator_subobject( &tit, &subObj ) ;
            bson_iterator_subobject( &it, &rsubObj ) ;
            if ( compareObj( &subObj, &rsubObj ) )
            {
               res = Check_Res_FULL_MATCH ;
            }
            else
            {
               res = Check_Res_EXIST ;
               break ;
            }
         }
         else
         {
            res = type == BSON_EOO ? Check_Res_NOTEXIST : Check_Res_EXIST ;
            break ;
         }
      }
   }
done: 
   return res ;
}


TEST_F( alterCLTest, EnableSharding   )
{
   INT32 rc = SDB_OK ;
   Check_Res ret ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   
   bson_append_string( &opt, ShardingType, range ) ;
   bson_append_start_object( &opt, ShardingKey );
   bson_append_int( &opt, "id", 1 );
   bson_append_finish_object( &opt );
   bson_finish( &opt );
   
   rc = sdbEnableSharding( cl, &opt ) ;
   bson_print( &opt );

   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbEnableSharding " << csName 
                           << "." << clName ;
   
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_FULL_MATCH ) << "fail to sdbEnableSharding " << csName 
                                          << "." << clName ;                       
   rc = sdbDisableSharding( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDisableSharding " << csName 
                           << "." << clName ;
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_NOTEXIST ) << "fail to sdbEnableSharding " << csName 
                                          << "." << clName ; 
}

TEST_F( alterCLTest, EnableCompression  )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   Check_Res ret ;

   bson_append_string( &opt, CompressionType, lzw ) ;
   bson_finish( &opt );
   
   rc = sdbEnableCompression( cl, &opt ) ;
   bson_print( &opt );

   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbEnableCompression " << csName 
                           << "." << clName ;
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_FULL_MATCH ) << "fail to sdbEnableCompression " << csName 
                                          << "." << clName ;                      
   rc = sdbDisableCompression( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDisableCompression " << csName 
                           << "." << clName ;
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_NOTEXIST ) << "fail to sdbEnableCompression " << csName 
                                          << "." << clName ;
}

TEST_F( alterCLTest, SetAttributes   )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   Check_Res ret ;
  
   bson_append_string( &opt, ShardingType, hashVal ) ;
   bson_append_start_object( &opt, ShardingKey );
   bson_append_int( &opt, "id", 1 );
   bson_append_finish_object( &opt );
   bson_append_string( &opt, CompressionType, lzw ) ;
   bson_finish( &opt );
   
   rc = sdbCLSetAttributes( cl, &opt ) ;
   bson_print( &opt );

   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCLSetAttributes " << csName 
                           << "." << clName ;
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_FULL_MATCH ) << "fail to sdbCLSetAttributes " << csName 
                                          << "." << clName ;
   
}

TEST_F( alterCLTest, alter  )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   Check_Res ret ;
    
   bson_append_string( &opt, ShardingType, hashVal ) ;
   bson_append_start_object( &opt, ShardingKey );
   bson_append_int( &opt, "id", 1 );
   bson_append_finish_object( &opt );
   bson_append_string( &opt, CompressionType, lzw ) ;
   bson_finish( &opt );
   
   rc = sdbAlterCollection( cl, &opt ) ;
   bson_print( &opt );
   
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbAlterCollection " << csName 
                           << "." << clName ;
   ret = checkCLAttr( &opt ) ;
   ASSERT_EQ( ret, Check_Res_FULL_MATCH ) << "fail to sdbAlterCollection " << csName 
                                          << "." << clName ;
}

