/************************************************************

 * @Description: test case for Jira question SEQUOIADBMAINSTREAM-3920
 * @author:      liuxiaoxuan init    2019-01-04
 *		 linyingting edit    2022-01-08
 *                   
 *************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
#include <string>

class getLastErrorObjTest17055 : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   sdbCollectionHandle cl2 ;
   const CHAR* csName ;
   const CHAR* clName ;

   void SetUp() 
   { 
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      cl2 = SDB_INVALID_HANDLE ;
      csName = "csname_17055";
      clName = "clname_17055" ;
      testBase::SetUp() ;
   }
   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCollection( cl2 ) ;
      testBase::TearDown() ; 
   }  
} ;

// get last errorobj
void getLastErrObj( sdbConnectionHandle db, bson &errObj, INT32 errExpect )
{
   bson_type type ;
   bson_iterator it ;
   INT32 rc, errActual ;
   bson_init( &errObj ) ;
   rc = sdbGetLastErrorObj( db, &errObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get last error obj " << rc ;
   // standalone
   if( isStandalone( db ) )
   {
      type = bson_find( &it, &errObj, "errno" ) ;
      ASSERT_NE( type, BSON_EOO ) << "fail to find the field <errno> of LastErrorObj" ;
      ASSERT_EQ( type, BSON_INT ) << "the type of the field <errno> is not integer" ;
      errActual = bson_iterator_int( &it ) ;
      ASSERT_EQ( errExpect, errActual ) << "the expect errno is : " << errExpect << ", but the errno of lastErrorObj is : " << errActual ;
   }
   // cluster
   else
   {
      bson_iterator t1,t2,t3 ;
      type = bson_find( &it, &errObj, "ErrNodes" ) ;
      ASSERT_NE( type, BSON_EOO ) << "fail to find the field <ErrNodes> of LastErrorObj" ;
      bson_iterator_subiterator( &it, &t1 ) ;
      bson errNodesObj ;
      bson_init( &errNodesObj ) ;
      bson_iterator_subobject( &t1, &errNodesObj ) ;
      type = bson_find( &t2, &errNodesObj, "ErrInfo" ) ;
      ASSERT_NE( type, BSON_EOO ) << "fail to find the field <ErrInfo> in <ErrNodes> of LastErrorObj" ;
      bson errInfoObj ;
      bson_init( &errInfoObj ) ;
      bson_iterator_subobject( &t2, &errInfoObj ) ;
      type = bson_find( &t3, &errInfoObj, "errno" ) ;
      ASSERT_NE( type, BSON_EOO ) << "fail to find the field <errno> in <ErrNodes> of LastErrorObj" ;
      ASSERT_EQ( type, BSON_INT ) << "the type of the field <errno> in <ErrNodes> is not integer" ;
      INT32 errActual = bson_iterator_int( &t3 ) ;
      ASSERT_EQ( errActual, errExpect ) << "the expect errno is : " << errExpect << ", but the errno of lastErrorObj is : " << errActual ;
      
      bson_destroy( &errNodesObj ) ;
      bson_destroy( &errInfoObj ) ;      
   }
   bson_destroy( &errObj );

   // clean error obj
   sdbCleanLastErrorObj( db );

   // get null last error obj 
   bson_init( &errObj ) ;
   rc = sdbGetLastErrorObj(db, &errObj) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get last error obj " << rc ;
   bson_destroy( &errObj ); 
}

TEST_F( getLastErrorObjTest17055, returnErrorObj )
{
   INT32 rc = SDB_OK ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
 
   // create existed cl
   rc = sdbCreateCollection( cs, clName, &cl2 ) ;
   INT32 expectErr ;
   expectErr = -22 ;
   ASSERT_EQ( expectErr, rc ) << "fail to create cl " << clName ;

   // get last errorobj from catalog
   bson errObj ;
   getLastErrObj( db, errObj, expectErr ) ;
   
   // update with invalid param
   bson rule;
   bson_init ( &rule ) ;
   bson_append_start_object( &rule, "$est" ) ;
   bson_append_int ( &rule, "a", 1 ) ;
   bson_append_finish_object( &rule ) ;
   bson_finish ( &rule ) ;
   rc = sdbUpdate( cl, &rule, NULL, NULL ) ;
   expectErr = -6 ;
   ASSERT_EQ( expectErr, rc ) << "fail to update" ;
   bson_destroy( &rule );

   // get last errorobj from data
   getLastErrObj( db, errObj, expectErr ) ;

}

TEST_F( getLastErrorObjTest17055, errorObjNULL )
{
   INT32 rc = SDB_OK ;

   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // insert record
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;  

   // get last error
   bson errObj;
   bson_init( &errObj ) ;
   rc = sdbGetLastErrorObj(db, &errObj) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get last error obj " << rc ;
   bson_destroy( &errObj );
}
