/************************************************************************
 * @Description : seqDB-23433:验证sdbGetRunTimeDetail接口
 * @Modify List : Li Yuanyue
 *                2020-01-18
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lob23433 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      csName = "lob_23433" ;
      clName = "lob_23433" ;

      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;   
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
   INT32 getValue( bson lobRunTimeDetail, INT32& act_RefCount, INT32& act_ReadCount, 
                   INT32& act_WriteCount, INT32& act_ShareReadCount,
                   FLOAT64& act_Begin, FLOAT64& act_End, string& act_LockType )
   {
      bson_iterator it ;
      bson_iterator sub ;
      
      // it:"AccessInfo": { "RefCount": 1, "ReadCount": 0, "WriteCount": 0, "ShareReadCount": 1, "LockSections": [ { "Begin": 2, "End": 22, "LockType": "S"} ] }
      bson_find( &it, &lobRunTimeDetail, "AccessInfo" ) ;
      // sub:"RefCount": 1, "ReadCount": 0, "WriteCount": 0, "ShareReadCount": 1, "LockSections": [ { "Begin": 2, "End": 22, "LockType": "S"} ]
      bson_iterator_subiterator( &it, &sub ) ;
      
      while ( bson_iterator_more( &sub ) )
      {
         INT32 type = bson_iterator_next( &sub ) ;
         if ( type == BSON_EOO ){
            break ;
         }

         string key = bson_iterator_key( &sub ) ;
         // printf( "Type: %d, Key: %s, value: %d\n", type, key, value );
         if ( key == "RefCount" )
         {
            act_RefCount = bson_iterator_int( &sub ) ; 
         }
         else if ( key == "ReadCount" )
         {
            act_ReadCount = bson_iterator_int( &sub ) ;
         }
         else if ( key == "WriteCount" )
         {
            act_WriteCount = bson_iterator_int( &sub ) ;
         }
         else if ( key == "ShareReadCount" )
         {
            act_ShareReadCount = bson_iterator_int( &sub ) ;
         }
         else if ( key == "LockSections" )
         {
            bson_iterator arrObj ;
            bson_iterator arr ;
            // arrObj:[ { "Begin": 2, "End": 22, "LockType": "S"} ]
            bson_iterator_subiterator( &sub, &arrObj ) ;
            // arr:{ "Begin": 2, "End": 22, "LockType": "S"}
            bson_iterator_subiterator( &arrObj, &arr ) ;
            while ( bson_iterator_more( &arr ) )
            {
               type = bson_iterator_next( &arr ) ;
               if ( type == BSON_EOO ){
                 break ;
               }
               
               key = bson_iterator_key( &arr ) ;
               if ( key == "Begin" )
               {
                  act_Begin = bson_iterator_long( &arr ) ;
               }
               else if ( key == "End" )
               {
                  act_End = bson_iterator_long( &arr ) ;
               }
               else if ( key == "LockType" )
               {
                  act_LockType = bson_iterator_string( &arr ) ;
               }
            }
         }
      }
      return SDB_OK ;
   }
} ;

TEST_F( lob23433, getRunTimeDetail )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob ;
   INT32 offset = 2 ;
   INT32 length = 20 ;
   bson lobRunTimeDetail ;
   bson_oid_t oid ;
   INT32 lobSize = 1024 ;
   
   bson_oid_gen( &oid ) ;
   bson_init( &lobRunTimeDetail ) ;
   bson_finish( &lobRunTimeDetail ) ;
   CHAR* lobWriteBuffer = (CHAR*)malloc( lobSize ) ;
   memset( lobWriteBuffer, 'a', lobSize ) ;

   // create lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_CREATEONLY" ;
   rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // open lob
   rc = sdbOpenLob( cl, &oid, SDB_LOB_SHAREREAD, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_SHAREREAD " ;

   // lock and seek
   rc = sdbLockAndSeekLob( lob, offset, length ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock and seek" ;

   // getRunTimeDetail
   rc = sdbGetRunTimeDetail( lob, &lobRunTimeDetail ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getRunTimeDetail" ;

   // check
   INT32 exp_RefCount = 1 ;
   INT32 exp_ReadCount = 0 ;
   INT32 exp_WriteCount =  0 ;
   INT32 exp_ShareReadCount = 1 ;
   FLOAT64 exp_Begin = offset ;
   FLOAT64 exp_End = offset + length ;
   string exp_LockType("S") ; 

   INT32 act_RefCount = -1 ;
   INT32 act_ReadCount = -1 ;
   INT32 act_WriteCount = -1 ;
   INT32 act_ShareReadCount = -1 ;
   FLOAT64 act_Begin = -1 ;
   FLOAT64 act_End = -1 ;
   string act_LockType = "NULL" ;

   rc = getValue( lobRunTimeDetail, act_RefCount, act_ReadCount, act_WriteCount, act_ShareReadCount,
                  act_Begin, act_End, act_LockType ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get value" ;

   ASSERT_EQ( act_ReadCount, exp_ReadCount ) << "RefCount" ;
   ASSERT_EQ( act_ReadCount, exp_ReadCount ) << "ReadCount" ;
   ASSERT_EQ( act_WriteCount, exp_WriteCount ) << "WriteCount" ;
   ASSERT_EQ( act_ShareReadCount, exp_ShareReadCount ) << "ShareReadCount" ;
   ASSERT_EQ( act_Begin, exp_Begin ) << "Begin" ;
   ASSERT_EQ( act_End, exp_End ) << "End" ;
   ASSERT_EQ( act_LockType, exp_LockType ) << "LockType" ;

   // close 
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // free
   bson_destroy( &lobRunTimeDetail ) ;
   free( lobWriteBuffer ) ;
}
