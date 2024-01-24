/***************************************************
 * @Description : test case of getIndex 
 *                seqDB-16781: 获取索引
 * @Modify      : wenjing wang
 *                2018-12-12
 ***************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <stdio.h>
#include "testBase.hpp"
#include <iostream>


const char* idxName = "idx_a" ;
const char* idxbName = "idx_b" ;
class GetIndexTest16780 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbclient::sdbCollectionSpace cs ;
   sdbclient::sdbCollection cl ;
   
  
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      bson::BSONObj opt = BSON("a"<<1) ;
      
      csName = "getIndex16780" ;
      clName = "getIndex16780" ;

      rc = db.createCollectionSpace( csName, 4096, cs ) ;
      if ( rc != -33 ){
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      }

      rc = cs.createCollection( clName,  cl ) ;
      if ( rc != -22 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      }
      
      rc = cl.createIndex( opt, idxName, TRUE, FALSE ) ;
      if ( rc != -247 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create index " << idxName ;
      }
      //bson::BSONObj tmp = BSON("unique"<< TRUE <<"NotNull" << TRUE ) ;
      bson::BSONObj tmp = BSON("NotNull" << true << "unique"<< TRUE) ;
      std::cout << tmp.toString() << std::endl;
      rc = cl.createIndex( BSON("b" << 1), idxbName, tmp) ;
       if ( rc != -247 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create index " << idxbName ;
      }

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

INT32 checkIdx( bson::BSONObj idx, const char* idxName, const char* idxkey, BOOLEAN isUnique, BOOLEAN isNotNull = false )
{    
    INT32 rc = 0 ;
    const char* IndexDef = "IndexDef" ;
    const char* name = "name" ;
    const char* unique = "unique" ;
    const char* key = "key" ;
    const char* notNull = "NotNull" ;
    if ( !idx.hasField(IndexDef) || bson::Object != idx.getField(IndexDef).type() ) 
    {
       std::cout << "step 1" << std::endl; 
       return -1 ;
    }

    bson::BSONObj idxDef = idx.getObjectField(IndexDef) ;
    if ( !idxDef.hasField(name) || bson::String != idxDef.getField(name).type() ) 
    {
       std::cout << "step 2" << std::endl; 
       return -1 ;
    }
    const char* getIdxName = idxDef.getStringField(name) ;
    if ( 0 != strcmp( idxName, getIdxName )) 
    {
       std::cout << "step 3" << std::endl; 
       return -2 ;
    }
    
    if ( !idxDef.hasField(unique) || bson::Bool != idxDef.getField(unique).type() ) 
    {
       std::cout << "step 4" << std::endl; 
       return -1 ;
    }
    bool getUnique = idxDef.getBoolField(unique) ;
    if ( getUnique != isUnique )
    {
       std::cout << "step 5" << std::endl; 
       return -1 ;
    }
    
    bool getNotNull = idxDef.getBoolField( notNull ) ;
    if ( getNotNull != isNotNull )
    {
       std::cout << getNotNull << "!=" << isNotNull ;
       return -1 ;
    }

    if ( !idxDef.hasField(key) || bson::Object != idxDef.getField(key).type() ) 
    {
       std::cout << "step 6" << std::endl; 
       return -1 ;
    }
    if ( !idxDef.getObjectField(key).hasField( idxkey ) )
    {
       std::cout << "step 7" << std::endl; 
       return -1 ;
    }
    
    return 0 ;
}

TEST_F( GetIndexTest16780, getSpecificIndex )
{
   INT32 rc = SDB_OK ;
   {
      bson::BSONObj rtnIdx ;
      rc = cl.getIndex( idxName, rtnIdx ) ;
      ASSERT_EQ( SDB_OK, rc ) << "get index " << idxName << " failed" ;
   
      rc = checkIdx( rtnIdx, idxName, "a", TRUE ) ;
      ASSERT_EQ( SDB_OK, rc ) << "get index " << idxName << " failed" ;
   }
   {
      bson::BSONObj rtnIdx ;
      rc = cl.getIndex( idxbName, rtnIdx ) ;
      ASSERT_EQ( SDB_OK, rc ) << "get index " << idxbName << " failed" ;
      rc = checkIdx( rtnIdx, idxbName, "b", TRUE, TRUE ) ;
      ASSERT_EQ( SDB_OK, rc ) << "get index " << idxbName << " failed" ;
   }
}

TEST_F( GetIndexTest16780, getAllIndex )
{
   INT32 rc = SDB_OK ;
   const char* indexName = "$id" ;
   std::vector<bson::BSONObj> idxs ;
    
   std::vector<bson::BSONObj>::iterator iter ;
   rc = cl.getIndexes( idxs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "get all index failed" ;
     
   ASSERT_GT( idxs.size(), 0 ) << "get all index failed" ;
   
   for ( iter = idxs.begin(); iter != idxs.end(); ++iter)
   {
      indexName = idxName ;
      rc = checkIdx( *iter, indexName, "a", TRUE ) ;
      if ( rc == -2 )
      {
         indexName = "$id" ;
         rc = checkIdx( *iter, indexName, "_id", TRUE ) ;
      }
      if ( rc == -2 )
      {
         indexName = "idx_b" ;
         rc = checkIdx( *iter, indexName, "b", TRUE,TRUE);
      }
      std::cout << iter->toString() << std::endl;
      ASSERT_EQ( SDB_OK, rc ) << "get index " << indexName << " failed" ;
   }
   
}

