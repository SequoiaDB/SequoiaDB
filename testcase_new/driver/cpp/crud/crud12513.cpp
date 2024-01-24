/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12513:插入/查询/更新/删除数据（带oid，不带oid）
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class crudTest12513 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "crudTestCs12513" ;
      clName = "crudTestCl12513" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( crudTest12513, crud12513 )
{
   INT32 rc = SDB_OK ;

   // insert doc with _id
   BSONObj docs[2] ;
   BSONObj ret ;
   BSONObj doc = BSON( "_id" << 1 << "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   // insert doc without _id
   doc = BSON( "a" << 2 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   doc = BSON( "_id" << 1 << "a" << 1 ) ;
   rc = cl.insert( doc, FLG_INSERT_CONTONDUP, NULL );
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   docs[0] = BSON( "_id" << 1 << "a" << 1 ) ;
   docs[1] = BSON(  "a" << 3 ) ;
   
   rc = cl.insert( docs, sizeof(docs)/sizeof(docs[0]), FLG_INSERT_CONTONDUP|FLG_INSERT_RETURN_OID, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   std::cout << ret.toString() << std::endl;
   BSONObjIterator iter( ret ) ;
   while( iter.more() ){
      BSONElement ele = iter.next() ;
      std::cout << ele.fieldName() << std::endl; 
      if (0 != strncmp( ele.fieldName(), "_id", strlen("_id"))) {
         continue;
      }
      BSONObjIterator subItr( ele.embeddedObject() ) ;
      while ( subItr.more() )
      {
         BSONElement subEle = subItr.next() ;
         BSONType type = subEle.type() ;
         if ( type == NumberInt ) {
           ASSERT_EQ( 1, subEle.numberInt() );
         }
         ASSERT_EQ( type == jstOID || type== NumberInt, TRUE) ;
      }
   }
   
   std::vector<BSONObj> docs2;
   docs2.push_back(BSON( "_id" << 1 << "a" << 1 ) );
   docs2.push_back(BSON(  "a" << 4 )) ;
   rc = cl.insert( docs2,  FLG_INSERT_CONTONDUP|FLG_INSERT_RETURN_OID, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   std::cout << ret.toString() << std::endl;
   BSONObjIterator iter2( ret ) ;
   while( iter2.more() ){
      BSONElement ele = iter2.next() ;
      std::cout << ele.fieldName() << std::endl;
      if (0 != strncmp( ele.fieldName(), "_id", strlen("_id"))){
         continue;
      }
      BSONObjIterator subItr( ele.embeddedObject() ) ;
      while ( subItr.more() )
      {
         BSONElement subEle = subItr.next() ;
         BSONType type = subEle.type() ;
         if ( type == NumberInt ) {
           ASSERT_EQ( 1, subEle.numberInt() );
         }
         ASSERT_EQ( type == jstOID || type== NumberInt, TRUE) ;
      }
   }

   // query doc with _id
   BSONObj cond = BSON( "a" << 1 ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "_id" ).Int() ) << "fail to check _id" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;  
 
   // query doc without _id
   cond = BSON( "a" << 2 ) ;
   sdbCursor cursor1 ;
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( jstOID, obj.getField( "_id" ).type() ) << "fail to check _id" ;
   ASSERT_EQ( 2, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // query doc without _id
   cond = BSON( "a" << 3 ) ;
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( jstOID, obj.getField( "_id" ).type() ) << "fail to check _id" ;
   ASSERT_EQ( 3, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   cond = BSON( "_id" << 1 ) ;
   // update and check
   BSONObj rule = BSON( "$inc" << BSON( "a" << 1 ) ) ;
   rc = cl.update( rule, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;
   cond = BSON( "_id" << 1 ) ;
   sdbCursor cursor2 ;
   rc = cl.query( cursor2, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor2.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 2, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor2.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // del and check
   rc = cl.del( cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to del" ;
   sdbCursor cursor3 ;
   rc = cl.query( cursor3, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor3.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check del" ;
   rc = cursor3.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
