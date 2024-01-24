/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2365
 *               seqDB-11734:使用C++驱动插入bindata数据并查询
 * @Modify:      Liang xuewang
 *			 	     2017-02-28
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class bindataTest11734 : public testBase
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
      csName = "bindataTestCs11734" ;
      clName = "bindataTestCl11734" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

// remove " ' space from string
string& removeSign( string& str )
{
   string::iterator it ;
   for( it = str.begin();it != str.end();)
   {
      if( *it == '\"' || *it == '\'' || *it == ' ' ||
            *it == '\n' )
         it = str.erase( it ) ;
      else
         it++ ;
   }
   return str ;
}	

TEST_F( bindataTest11734, bindataType11734 )
{
   INT32 rc = SDB_OK ;

   BSONObjBuilder b ;
   BSONObj obj ;
   const CHAR* data1 = "~Y4_SqlF" ;
   const CHAR* data2 = "SqlF" ;
   const CHAR* data3 = "aGVsbG8=" ;
   b.append( "a", 1 ) ;
   b.appendBinData( "bin1", strlen(data1), (BinDataType)2, data1 ) ;
   b.appendBinDataArrayDeprecated( "bin2", data2, strlen(data2) ) ;
   b.appendBinData( "bin3", strlen(data3), (BinDataType)128, data3 ) ;
   obj = b.obj() ;
   cout << obj.jsonString( Strict,1 ) << endl ;

   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj sel = BSON( "bin1" << "" << "bin2" << "" << "bin3" << "" ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond, sel ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   BSONObj result ;
   rc = cursor.next( result ) ;
   string str = result.jsonString( Strict, 1 ) ;
   cout << str << endl ;
   str = removeSign( str ) ;
   const CHAR* expect = "{bin1:{$binary:flk0X1NxbEY=,$type:02},"
                         "bin2:{$binary:U3FsRg==,$type:02},"
                         "bin3:{$binary:YUdWc2JHOD0=,$type:80}}" ;
   ASSERT_EQ( expect, str ) << "fail to check query" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
