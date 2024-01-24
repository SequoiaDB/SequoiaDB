/***************************************************
 * @Description : test case of getIndex 
 *                 seqDB-16648: 创建/删除自增字段 
 *                 seqDB-16649: 创建/删除多个自增字段
 *                 seqDB-16650: sequence快照/列表验证 
 *                 seqDB-16653: 创建集合时指定自增字段，并修改自增字段属性
 * @Modify      : wenjing wang
 *                2018-12-17
 ***************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <stdio.h>
#include "testBase.hpp"
#include "testcommon.hpp"

class autoIncrement_16648 : public testBase
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
      
      csName = "autoIncrement16648" ;
      clName = "autoIncrement16648" ;
      
      rc = db.createCollectionSpace( csName, 4096, cs ) ;
      if ( rc != -33 ){
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      }

      rc = cs.createCollection( clName,  cl ) ;
      if ( rc == -22 ) {
      	 rc = cs.dropCollection( clName ) ;
      	 ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
      	 rc = cs.createCollection( clName,  cl ) ;
      }
      
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
      
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

TEST_F( autoIncrement_16648, case16648 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }
   bson::BSONObj opt; ;
   rc = cl.createAutoIncrement( opt ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to createAutoIncrement " ;
  
   opt = BSON( "Field" << "studentID" );
   rc = cl.createAutoIncrement( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createAutoIncrement " << opt.toString() ;
   
   bson::BSONObj doc = BSON( "a" << 1 );
   rc = cl.insert( doc )  ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   rc = cl.dropAutoIncrement(NULL) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to dropAutoIncrement " ;
   
   doc = BSON( "a" << 2 );
   rc = cl.insert( doc )  ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   sdbclient::sdbCursor cursor ;
   rc = cl.query( cursor );
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << opt.toString() ;
   
   bson::BSONObj ret ;
   int small,big ;
   while ( (rc = cursor.next( ret )) == SDB_OK )
   {
      ASSERT_EQ(bson::NumberInt,ret.getField("a").type()) << "fail to getFiled" << ret.toString();
      ASSERT_EQ(bson::NumberLong,ret.getField("studentID").type()) << "fail to getFiled" << ret.toString();
      if ( ret.getIntField("a") == 1 )
      {
         small = ret.getIntField("studentID") ;  
      }
      else
      {
         big = ret.getIntField("studentID") ;
      }
   }
   
   ASSERT_GT( big, small ) << "fail to AutoIncrement" ;
   cursor.close() ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to query " ;
   
   rc = cl.dropAutoIncrement("studentID") ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to dropAutoIncrement " << opt.toString() ;  
   
   doc = BSON( "a" << 3 ) ;
   rc =  cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   rc = cl.queryOne( ret, doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << doc.toString() ;
   ASSERT_EQ (false, ret.hasField("studentID") ) << ret.toString() ;
}

TEST_F( autoIncrement_16648, case16649 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }
   std::vector<bson::BSONObj> opts ;
   rc = cl.createAutoIncrement( opts ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to createAutoIncrement " ;
   
   opts.push_back( BSON( "Field" << "studentID" ) ) ;
   opts.push_back( BSON( "Field" << "innerID" ) ) ;

   rc = cl.createAutoIncrement( opts ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createAutoIncrement " << opts[0].toString()
                           << "," << opts[1].toString() ;
   
   
   bson::BSONObj doc = BSON( "a" << 1 );
   rc = cl.insert( doc )  ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   int field1Small, field2Small, field1Big, field2Big ;
   bson::BSONObj ret ;
   rc = cl.queryOne( ret, doc ) ;
   field1Small = ret.getIntField("studentID") ;
   field2Small = ret.getIntField("innerID") ;
   
   std::vector<const CHAR*> fields ;
   rc = cl.dropAutoIncrement(fields) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to dropAutoIncrement " ;
   
   doc = BSON( "a" << 2 );
   rc = cl.insert( doc )  ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   rc = cl.queryOne( ret, doc ) ;
   field1Big = ret.getIntField("studentID") ;
   field2Big = ret.getIntField("innerID") ;
   
   ASSERT_GT( field1Big, field1Small ) << "fail to AutoIncrement" ;
   ASSERT_GT( field2Big, field2Small ) << "fail to AutoIncrement" ;
   
   fields.push_back( (CHAR*)"studentID" ) ;
   fields.push_back( (CHAR*)"innerID" ) ;
   rc = cl.dropAutoIncrement( fields ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to dropAutoIncrement " << fields[0] << "," << fields[1] ;
   
   doc = BSON( "a" << 3 ) ;
   rc = cl.insert( doc )  ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert " << doc.toString() ;
   
   rc = cl.queryOne( ret, doc ) ;
   ASSERT_EQ (false, ret.hasField("studentID") ) << ret.toString() ;
   ASSERT_EQ (false, ret.hasField("innerID") ) << ret.toString() ;
}

TEST_F( autoIncrement_16648, case16650 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }
   bson::BSONObj ret ;
   BOOLEAN isFind = false ;
   bson::BSONObj opt = BSON( "Field" << "studentID" );
   rc = cl.createAutoIncrement( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createAutoIncrement " << opt.toString() ;
   sdbCursor cursor ; 
   rc = db.getSnapshot( cursor, SDB_SNAP_SEQUENCES );
   ASSERT_EQ( SDB_OK, rc ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   while( (rc = cursor.next( ret )) == SDB_OK ) 
   {
      ASSERT_EQ( true, ret.hasField("Name") );
      ASSERT_EQ( true, ret.hasField("Version") );
      ASSERT_EQ( true, ret.hasField("CurrentValue") );
      ASSERT_EQ( true, ret.hasField("MinValue") );
      ASSERT_EQ( true, ret.hasField("MaxValue") );
      ASSERT_EQ( true, ret.hasField("Increment") );
      isFind = TRUE ;
   }
   ASSERT_EQ( isFind, TRUE ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   isFind = FALSE;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   
   rc = db.getList( cursor, SDB_LIST_SEQUENCES );
   ASSERT_EQ( SDB_OK, rc ) << "getList(" << SDB_LIST_SEQUENCES << ") failed" ;
   while( (rc = cursor.next( ret )) == SDB_OK  ) 
   {
      ASSERT_EQ( true, ret.hasField("Name") );
      isFind = TRUE ;
   }
   ASSERT_EQ( isFind, TRUE ) << "getList(" << SDB_LIST_SEQUENCES << ") failed" ;
   isFind = FALSE;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "getList(" << SDB_LIST_SEQUENCES << ") failed" ;
   rc = cl.dropAutoIncrement("studentID") ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to dropAutoIncrement " << "studentID" ;
}

long long getCLUniqueID( sdbclient::sdb &db, const std::string &clName)
{
   INT32 rc = SDB_OK ; 
   long uniqueID = -1;
   bson::BSONObj cond = BSON("Name" <<  clName ) ;
   bson::BSONObj ret ;
   sdbCursor cursor ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG,  cond);
   if ( rc != SDB_OK )
   {
      cout <<  "getSnapshot(" << SDB_SNAP_CATALOG<<"," << cond.toString() <<") failed: "  << rc;
      return uniqueID ;
   }
   
   while( (rc = cursor.next( ret )) == SDB_OK ) 
   {
      if (ret.hasField("UniqueID") )
      {
         uniqueID = ret.getField("UniqueID").Long();
         break;
      }
   }
   cursor.close() ;
   return uniqueID ;
}

TEST_F( autoIncrement_16648, case16653 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }
   bson::BSONObj opt = BSON("AutoIncrement" << BSON( "Field" << "id" )  ) ;
   bson::BSONObj ret ;
   sdbCollection cl2;
   BOOLEAN isFind = false ;
   long curVal ;
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "dropCollection(" << clName <<") failed" ;
   rc = cs.createCollection( clName, opt, cl2  ) ;
   ASSERT_EQ( SDB_OK, rc ) << "createCollection(" << clName <<") failed" ;
   

   long long CLUniqueID = getCLUniqueID( db, cl2.getFullName());
   char szName[32] = {0};
   snprintf(szName, sizeof(szName),"SYS_%lld_%s_SEQ", CLUniqueID, "id");
   bson::BSONObj cond = BSON("Name" << szName  ) ;
   
   sdbCursor cursor ; 
   rc = db.getSnapshot( cursor, SDB_SNAP_SEQUENCES, cond);
   ASSERT_EQ( SDB_OK, rc ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   while( (rc = cursor.next( ret )) == SDB_OK ) 
   {
      ASSERT_EQ( true, ret.hasField("CurrentValue") );
      isFind = TRUE ;
   }
   cursor.close() ;
   
   ASSERT_EQ( isFind, TRUE ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   isFind = FALSE ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
  
   bson::BSONObj opts = BSON("AutoIncrement" << BSON( "CurrentValue" << 10 << "Field" << "id")  ) ;
   rc = cl2.alterCollection( opts );
   ASSERT_EQ( SDB_OK, rc ) << "alterCollection" << opts.toString() ;
   rc = db.getSnapshot( cursor, SDB_SNAP_SEQUENCES , cond );
   ASSERT_EQ( SDB_OK, rc ) << "getSnapshot(" << SDB_SNAP_SEQUENCES << ") failed" ;
   while( (rc = cursor.next( ret )) == SDB_OK ) 
   {
      ASSERT_EQ( true, ret.hasField("CurrentValue") );
      isFind = TRUE ;
      curVal = ret.getIntField("CurrentValue") ;
   }
   ASSERT_EQ( curVal, 10 ) << "alterCollection" << opts.toString() ;
   
   cursor.close() ;
   
}
