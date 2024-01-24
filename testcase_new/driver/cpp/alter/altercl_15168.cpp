/**************************************************************************
 * @Description:   test case for C++ driver
 *                  seqDB-15167:alter修改CL属性
                    seqDB-15168:setAttributes修改CL属性
                    seqDB-15169:开启/关闭cl切分属性
                    seqDB-15170:开启/关闭cl压缩属性
 * @Modify:        wenjing wang Init
 *                 2018-04-27
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

const char *CompressionTypeDesc = "CompressionTypeDesc" ;
const char *lzw = "lzw";
const char *ShardingType = "ShardingType";
const char *range = "range" ;
const char *id = "id" ;
const char *ShardingKey = "ShardingKey" ;
const char *CompressionType = "CompressionType" ;

class alterCLTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      sdbCollectionSpace cs ;
      clName = "alterCS_15168";
      csName = "alterCS_15168";
      
      testBase::SetUp() ;
      
      rc = db.getCollectionSpace( csName.c_str(), cs );
      if ( rc == -34 )
      {
         rc = db.createCollectionSpace( csName.c_str(), 65536, cs );
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/getCollectionSpace " << csName ;
      
      rc = cs.getCollection( clName.c_str(), cl );
      if ( rc == -23 )
      {
         rc = cs.createCollection( clName.c_str(), cl ) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/getCollection " << clName ;
      
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName.c_str() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
protected:
   std::string clName ;
   std::string csName ;
   sdbCollection cl ;
   BSONObj getCLAttr();
} ;

BSONObj alterCLTest::getCLAttr()
{
   BSONObj ret ;
   sdbCursor cursor ;
   std::string fullName = csName + "." + clName ;
   INT32 rc = db.getSnapshot(cursor, 8, BSON("Name" << fullName.c_str()));
   if ( rc != SDB_OK )
   {
      std::cout << "fail to getSnapshot(8), rc = " << rc << endl;
      goto done ;
   }
   
   while ( SDB_DMS_EOC != cursor.next( ret ) ) ;
done:
   return ret;
}

BOOLEAN isCompressed(const BSONObj& obj)
{
   BSONElement elem = obj.getFieldDotted(CompressionTypeDesc) ;
   if ( elem.type() == EOO )
   {
      return false ;
   }
   else if( elem.type() == String && elem.String() == lzw )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

BOOLEAN isSharding(const BSONObj& obj)
{
   BSONElement elem = obj.getFieldDotted(ShardingType) ;
   BOOLEAN rettype =false, retkey = false ;
   if ( elem.type() == EOO )
   {
      rettype = false ;
   }
   else if ( elem.type() == String && elem.String() == range )
   {
      rettype = true;
   }
   
   elem = obj.getFieldDotted(ShardingKey) ;
   if ( elem.type() == EOO )
   {
      retkey = false ;
   }
   else if ( elem.type() == Object )
   {
      BSONElement subelem = elem.Obj().getFieldDotted(id) ;
      if ( subelem.type() == NumberInt && subelem.Int() == 1 )
      {
         retkey = true;
      }
   }
   
   return rettype && retkey ;
}

TEST_F( alterCLTest, enableCompression )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   rc = cl.enableCompression(BSON(CompressionType << lzw));
   ASSERT_EQ( SDB_OK, rc ) << "fail to enableCompression " << clName ;
   
   BSONObj res = getCLAttr() ;
   ASSERT_EQ( isCompressed(res), true ) << "fail to enableCompression " << clName ;
   rc = cl.disableCompression();
   ASSERT_EQ( SDB_OK, rc ) << "fail to disableCompression " << clName ;
   res = getCLAttr() ;
   ASSERT_EQ( isCompressed(res), false ) << "fail to enableCompression " << clName ;
   
}

TEST_F( alterCLTest, enableSharding )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   rc = cl.enableSharding( BSON(ShardingType << range << ShardingKey << BSON(id << 1)) );
   ASSERT_EQ( SDB_OK, rc ) << "fail to enableSharding " << clName ;
   BSONObj res = getCLAttr() ;
   ASSERT_EQ( isSharding(res), true ) << "fail to enableSharding " << clName ;
   rc = cl.disableSharding() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to disableSharding " << clName ;
   res = getCLAttr() ;
   ASSERT_EQ( isSharding(res), false ) << "fail to enableSharding " << clName ;
}

TEST_F( alterCLTest, setAttributes )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   rc = cl.setAttributes(BSON(ShardingType<<range << ShardingKey << BSON(id << 1))) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setAttributes " << clName ;
   BSONObj res = getCLAttr() ;
   ASSERT_EQ( isSharding(res), true ) << "fail to enableSharding " << clName ;
}

TEST_F( alterCLTest, alter )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   rc = cl.alterCollection (BSON(ShardingType << range << ShardingKey << BSON(id << 1)) );
   ASSERT_EQ( SDB_OK, rc ) << "fail to alterCollection " << clName ;
   BSONObj res = getCLAttr() ;
   ASSERT_EQ( isSharding(res), true ) << "fail to enableSharding " << clName ;
}

