/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-26312: 测试插入操作，检查插入数量
 *                 seqDB-26313: 测试更新操作，检查更新数量
 *                 seqDB-26314: 测试删除操作，检查删除数量  
 * @Modify:        wenjingwang Init
 *                 2022-03-31
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

enum Operater {INSERT, UPDATE, DELETE} ;

class CUDTest26312_4 : public testBase
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
      csName = "cpptest" ;
      clName = "cudtest_26312" ;
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

BOOLEAN checkOperNum(const BSONObj& obj, Operater oper, int num)
{
   static const string InsertedNum = "InsertedNum";
   static const string UpdatedNum = "UpdatedNum";
   static const string DeletedNum = "DeletedNum";
   cout << obj.toString() << endl;
   if ( oper == INSERT )
   {
      if (obj.getField(InsertedNum).Long() == num )
      {
         return TRUE ;
      }
   }
   else if ( oper == UPDATE )
   {
      if (obj.getField(UpdatedNum).Long() == num )
      {
         return TRUE ;
      }
   }
   else if(oper == DELETE)
   {
      if (obj.getField(DeletedNum).Long() == num )
      {
         return TRUE ;
      }
   }
   return FALSE;
}

TEST_F( CUDTest26312_4, insertWithInsertedNum )
{
   INT32 rc = SDB_OK ;
   BSONObj result ;
   
   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc, 0, &result);
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;
   BOOLEAN ret = checkOperNum(result, INSERT, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check InsertedNum " ;

   vector<BSONObj> docs ;
   docs.push_back(BSON( "a" << 2 )) ;
   rc = cl.insert( doc, 0, &result) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;
   ret = checkOperNum(result, INSERT, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check InsertedNum " ;
   
   BSONObj objs[1];
   objs[0] = BSON( "a" << 3 ) ;
   rc = cl.insert( objs, 1, 0, &result) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;
   ret = checkOperNum(result, INSERT, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check InsertedNum " ;
}

TEST_F( CUDTest26312_4, updateWithUpdatedNum )
{
   INT32 rc = SDB_OK ;
   BSONObj result ;
   
   // upsert with cond match not exist
   BSONObj cond = BSON( "a" << 4 ) ;
   BSONObj rule = BSON( "$set" << BSON( "b" << 10 ) ) ;
   rc = cl.upsert( rule, cond, _sdbStaticObject, _sdbStaticObject, 0, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;
   
   BOOLEAN ret = checkOperNum(result, INSERT, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check InsertedNum " ;
   
   // upsert with cond match exist
   cond = BSON( "a" << 4 ) ;
   rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   rc = cl.update( rule, cond, _sdbStaticObject, 0, &result) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;
   ret = checkOperNum(result, UPDATE, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check UpdatedNum " ;
}

TEST_F( CUDTest26312_4, deleteWithDeletedNum )
{
   INT32 rc = SDB_OK ;
   BSONObj result ;
   // insert doc
   BSONObj doc = BSON( "a" << 5 ) ;
   rc = cl.insert( doc );
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;
   
   doc = BSON( "a" << 5 ) ;
   rc = cl.del( doc, _sdbStaticObject, 0, &result) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;
   BOOLEAN ret = checkOperNum(result, DELETE, 1 );
   ASSERT_EQ( ret, TRUE ) << "fail to check UpdatedNum " ;
}
