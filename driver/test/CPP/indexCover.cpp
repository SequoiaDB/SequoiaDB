/**************************************************************************
 * @Description:   test case for C++ driver
 * @Modify:        Xia qianyong Init
 *                 2020-12-11
 **************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

#define SDB_CATALOG_CS                       "collection space"
#define SDB_CATALOG_CL                       "collection"
#define FIELD_NAME_INDEX_COVER               "IndexCover"


class indexcoverTest : public testing::Test
{
protected:
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl2 ;
   const CHAR* csName = "indexcoverTestCs" ;
   const CHAR* clName = "indexcoverTestCl" ;
   const CHAR* clName2 = "indexcoverTestCl2" ;

   sdbCursor cursor ;
   sdbCursor cursor2 ;
   BSONObj matcher ;
   BSONObj select ;
   BSONObj orderby ;
   BSONObj hint ;
   BSONObj result ;
   BSONObj result2 ;

   BSONObj matcherNull ;
   BSONObj selectNull ;
   BSONObj orderbyNull ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      CHAR *hostName = HOST; //HOST ;
      const CHAR *svcPort = SERVER ;
      const CHAR *usr = USER ;
      const CHAR *passwd = PASSWD ;
      BSONObj notArray = BSON( "NotArray" << TRUE );
      BSONObj array = BSON( "NotArray" << FALSE );

      rc = db.connect( hostName, svcPort, usr, passwd ) ;
      if( SDB_OK != rc )
      {
         hostName = "localhost" ;
         rc = db.connect( hostName, svcPort, usr, passwd ) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect host " << hostName << ":" << svcPort ;

      rc = createCsCl() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

      createIndexInsertData(cl,notArray);
      createIndexInsertData(cl2,array );
   }

   void TearDown()
   {
      INT32 rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      db.disconnect();
   }

   INT32 createCsCl()
   {
      INT32 rc = SDB_OK ;

      rc = db.createCollectionSpace( csName, 4096, cs ) ;
      if( SDB_DMS_CS_EXIST == rc )
      {
         rc = db.getCollectionSpace( csName, cs ) ;
      }
      rc = cs.createCollection( clName, cl ) ;
      if( SDB_DMS_EXIST == rc )
      {
         rc = cs.dropCollection( clName ) ;
         rc = cs.createCollection( clName, cl ) ;
      }

      rc = cs.createCollection( clName2, cl2 ) ;
      if( SDB_DMS_EXIST == rc )
      {
         rc = cs.dropCollection( clName2 ) ;
         rc = cs.createCollection( clName2, cl2 ) ;
      }
   }

   void createIndexInsertData( sdbCollection &cl, BSONObj &arrayOption )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;

      indexDef = BSON( "a" << 1 ) ;
      rc = cl.createIndex( indexDef, "a", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "b" << 1 ) ;
      rc = cl.createIndex( indexDef, "b", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "c" << 1 ) ;
      rc = cl.createIndex( indexDef, "c", BSON("NotArray" << FALSE) ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a" << 1 << "b" << -1 ) ;
      rc = cl.createIndex( indexDef, "ab", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 << "c" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAndc", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 << "a" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAnda", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a" << 1 << "a.b" << 1 ) ;
      rc = cl.createIndex( indexDef, "aAndadotb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 << "b" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAndb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 << "d" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAndd", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "d" << 1 << "a.b" << 1 ) ;
      rc = cl.createIndex( indexDef, "dAndadotb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "d" << 1 << "a" << 1 << "a.b" << 1 ) ;
      rc = cl.createIndex( indexDef, "dAndaAndadotb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;


      indexDef = BSON( "a.b" << 1 << "a" << 1 << "b" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAndaAndb", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      indexDef = BSON( "a.b" << 1 << "a.c" << 1 ) ;
      rc = cl.createIndex( indexDef, "adotbAndadotc", arrayOption ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to createIndex:" << indexDef.toString().c_str() ;

      BSONObj objs[30] = {
         BSON("a" << 1 << "b" << 1 << "c" << 1 ),
         BSON("a" << 3 << "b" << 3 << "c" << 3 ),
         BSON("a" << 5 << "b" << 5 << "c" << 5 ),
         BSON("a" << 7 << "b" << 7 << "c" << 7 ),
         BSON("a" << 9 << "b" << 9 << "c" << 9 ),
         BSON("a" << 11 << "b" << 11 << "c" << 11 ),
         BSON("a" << 2 << "c" << 1 ),
         BSON("a" << 4 << "c" << 1 ),
         BSON("a" << 6 << "c" << 1 ),
         BSON("a" << 8 << "c" << 1 ),
         BSON("a" << 10 << "c" << 1 ),
         BSON("a" << 12 << "c" << 1 ),
         BSON("a" << 30 << "d" << 13 ),
         BSON("b" << 20 << "c" << 1 ),
         BSON("b" << 21 << "c" << 1 ),
         BSON("b" << 22 << "c" << 1 ),
         BSON("b" << 23 << "c" << 1 ),
         BSON("b" << 24 << "c" << 1 ),
         BSON("b" << 25 << "c" << 1 ),
         BSON("b" << 26 << "c" << 1 ),
         //{a:{b:{c:1}}}
         BSON("a" << BSON("b" << BSON("c" << 1)) ),
         BSON("a" << BSON("b" << 10) ),
         // {a:{b:{c:[1,2,3]}}}
         BSON("a" << BSON("b" << BSON("c" << 99 << "d" << 100 ))),
         // {a:{b:{c:[1,2,3]}}}
         BSON("a" << BSON("b" << BSON("c" << BSON_ARRAY(1<<2<<3)))),
         // { a:{"b":{"c1":1,"c2":2 },b2:3 }}
         BSON("a" << BSON("b" << BSON("c1" << 1  << "c2" << 2 ) << "b2" << 3) ),
         //{ a:{"b":{"c1":10,"c2":20 },b2:30 }}
         BSON("a" << BSON("b" << BSON("c1" << 10 << "c2" <<20 ) << "b2" << 30) ),
         //{ a:{"b":100,b2:300 }}
         BSON("a" << BSON("b" << 100 << "b2" << 300 )),
        // { a:{"b":11,"c":12 },"d":2}
         BSON("a" << BSON("b" << 22 << "c" << 12 ) << "d" << 2 ),
        // { a:{"b":{ "c":1 }},"d":2}
         BSON("a" << BSON("b" << BSON("c" << 1 ) ) << "d" << 2 ),
        // { a:{"b":10 },"d":2}
         BSON("a" << BSON("b" << 10 ) << "d" << 2 )
       } ;
      // insert record
      for( int i=0; i < 23; i++ )
      {
         rc = cl.insert( objs[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
   }

   void indexCoverCheck( BSONObj &matcher,BSONObj &select,
                         BSONObj &orderby,BSONObj &hint,
                         BOOLEAN expectIndexCover )
   {
      INT32 rc = SDB_OK ;
      INT32 rc2 = SDB_OK ;

      rc = cl.explain( cursor, matcher, select, orderby, hint ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
      rc = cursor.next( result ) ;
      ASSERT_EQ( SDB_OK, rc ) << "cursor.next failed" ;
      BOOLEAN indexCover = result.getField( FIELD_NAME_INDEX_COVER ).trueValue() ;
      ASSERT_EQ( expectIndexCover, indexCover ) ;

      rc = cl2.explain( cursor2, matcher, select, orderby, hint ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
      rc = cursor2.next( result ) ;
      ASSERT_EQ( SDB_OK, rc ) << "cursor2.next failed" ;
      indexCover = result.getField( FIELD_NAME_INDEX_COVER ).trueValue() ;
      ASSERT_EQ( FALSE, indexCover ) ;

      if( TRUE == expectIndexCover )
      {
         rc = cl.query(cursor, matcher, select, orderby, hint  ) ;
         ASSERT_EQ( SDB_OK, rc ) << "query failed" ;

         rc = cl2.query(cursor2, matcher, select, orderby, hint ) ;
         ASSERT_EQ( SDB_OK, rc ) << "query failed" ;

         while( 1 )
         {
            rc = cursor.next( result ) ;
            rc2 = cursor2.next( result2 ) ;
            ASSERT_TRUE( ((SDB_DMS_EOC == rc) || ( SDB_OK == rc)) && ( rc == rc2 )) ;
            if(SDB_DMS_EOC == rc )
            {
               break ;
            }

            ASSERT_EQ( 0, result.woCompare( result2 ) )
                     << " result :" << result.toString() << "\n"
                     << " result2:" << result2.toString() << "\n" ;
         }
      }
   }
} ;

TEST_F( indexcoverTest, keyc__default___false )
{
   select = BSON( "c" << "" ) ;
   hint = BSON(""<< "c") ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keya__not___true )
{
   select = BSON( "a" << "" ) ;
   hint = BSON(""<< "a") ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keya__null__null__null___false )
{
   hint = BSON(""<< "a") ;
   indexCoverCheck( matcherNull, selectNull, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keya__a__null__null___false )
{
   INT32 rc = SDB_OK ;

   hint = BSON(""<< "a") ;
   matcher = BSON("a" << 2 );
   indexCoverCheck( matcher, selectNull, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keya__null__a__null___true )
{
   hint = BSON(""<< "a") ;
   select = BSON("a" << 2 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keya__null__a_exclude__null___false )
{
   hint = BSON(""<< "a") ;
   select = BSON("a" << BSON("$include" << 0 ) );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keya__null__a_include__null___false )
{
   hint = BSON(""<< "a") ;
   select = BSON("a" << BSON("$include" << 1 ) );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

// sdbshell run find( { },{ a: { $include:0 },a:"" } ).hint({"":"a"})
TEST_F( indexcoverTest, keya__null__a_exclude_a__null___false )
{
   hint = BSON(""<< "a") ;
   select = BSON( "a" << BSON("$include" << 0) << "a" <<"" );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

//////////////////////////////// $include test
// find({},{"a.b":{"$include":0},"a":""}).hint({"":"adotbAnda"})
TEST_F( indexcoverTest, keyadotbAnda__null__adotb_exclude_a__null___false )
{
   hint = BSON("" << "adotbAnda") ;
   select = BSON( "a.b" << BSON("$include" << 0) << "a" <<"" ) ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"a.b":{"$include":0},"b":""}).hint({"":"adotbAndb"})
TEST_F( indexcoverTest, keyadotbAndb__null__adotb_exclude_a__null___false )
{
   hint = BSON("" << "adotbAndb") ;
   select = BSON( "a.b" << BSON("$include" << 0) << "b" <<"" ) ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"a.b":{"$include":0},a:"",b:""}).hint({"":"adotbAndaAndb"})
TEST_F( indexcoverTest, keyadotbAndaAndb__null__adotb_exclude_a__null___false )
{
   hint = BSON("" << "adotbAndaAndb") ;
   select = BSON( "a.b" << BSON("$include" << 0) << "a" << "" << "b" << "" ) ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"a":{"$include":0},"a.b":""}).hint({"":"adotbAnda"})
TEST_F( indexcoverTest, keyadotbAnda__null__a_exclude_adotb__null___false )
{
   hint = BSON("" << "adotbAnda") ;
   select = BSON( "a" << BSON( "$include" << 0 ) << "a.b" << "" ) ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"a":{"$include":0},"a.b":"",b:""}).hint({"":"adotbAndaAndb"})
TEST_F( indexcoverTest, keydotbAndaAndb__null__a_exclude_adotb__null___false )
{
   hint = BSON(""<< "adotbAndaAndb") ;
   select = BSON( "a" << BSON( "$include" << 0 ) << "a.b" << "" << "b" << "" ) ;
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"b":"","a.b":{"$include":0}}).hint({"":"adotbAndb"})
TEST_F( indexcoverTest, keyadotbAndb__null__a_exclude_adotb__null___false )
{
   hint = BSON(""<< "adotbAndb") ;
   select = BSON( "b" << "" << "a.b" << BSON("$include" << 0 ) );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}
// find({},{"b":"","a.b":{"$include":0},a:""}).hint({"":"adotbAndaAndb"})
TEST_F( indexcoverTest, keyadotbAndaAndb__null__b_exclude_adotb__null___false )
{
   hint = BSON(""<< "adotbAndaAndb") ;
   select = BSON( "b" << "" << "a.b" << BSON("$include" << 0 ) << "a" << "" );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keya__a__a__null___true )
{
   hint = BSON(""<< "a") ;
   matcher = BSON("a" << BSON("$et" << 2 ) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keya__b__a__null___false )
{
   hint = BSON(""<< "a") ;
   matcher = BSON("b" << BSON("$et" << 2 ) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

//({ a: { $elemMatch:{ "b": { $elemMatch: { c1: 1 } } } } },{"a":""} ).hint({"":"a"})
TEST_F( indexcoverTest, keyA__matcher_elemElemMatch_true)
{
   hint = BSON(""<< "a") ;
   matcher = BSON("a" << BSON("$elemMatch" << BSON("b" << BSON("$elemMatch" << BSON("c1" << 1 ) ))));
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyA_NULL__adotbdotc__NULL_a_false )
{
   hint = BSON(""<< "a") ;
   select = BSON("a.b.c" << "" );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

////////////////////// key(ab)
TEST_F( indexcoverTest, keyab__NULL__NULL__NULL___false )
{
   hint = BSON(""<< "ab") ;
   indexCoverCheck( matcherNull, selectNull, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__a__NULL__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON("$et" << 2 ) );
   indexCoverCheck( matcher, selectNull, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__NULL__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   select = BSON("a" << "" );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__NULL__a_exclude__NULL___false )
{
   hint = BSON(""<< "ab") ;
   select = BSON("a" << BSON("$include" << 0 ) );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__NULL__a_include__NULL___false )
{
   hint = BSON(""<< "ab") ;
   select = BSON("a" << BSON("$include" << 1 ) );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON("$et" << 2) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__abc__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << 1 );
   select = BSON("a" << "" << "b" << "" << "c" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__abc__ab__NULL___false )
{
   hint = BSON("" << "ab") ;
   matcher = BSON("a" << 1 << "b" << 1 << "c" << 1);
   select = BSON("a" << "" << "b" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__c__a__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("c" << BSON("$et" << 2) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__a__a__a___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << 1 );
   select = BSON("a" << "" );
   orderby = BSON("a" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__a__c___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON("$et" << 2) );
   select = BSON("a" << "" );
   orderby = BSON("c" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__b__a__c___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("b" << BSON("$et" << 2) );
   select = BSON("a" << "" );
   orderby = BSON("c" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__b__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("b" << BSON("$et" << 20) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__b__b__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("b" << BSON("$et" << 20) );
   select = BSON("b" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__ab__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << 1 );
   select = BSON("a" << "" << "b" << "");
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__ab__ab__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << 1 << "b" << 1 );
   select = BSON("a" << "" << "b" << "");
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}


TEST_F( indexcoverTest, keyab__a$et__a__b___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON("$et" << 2) );
   select = BSON("a" << "" );
   orderby = BSON("b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__a__b___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" <<1 );
   select = BSON("a" << "" );
   orderby = BSON("b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__b__b___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" <<1 );
   select = BSON("b" << "" );
   orderby = BSON("b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__a__ab___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" <<1 );
   select = BSON("a" << "" );
   orderby = BSON("a" << 1 << "b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a__ab__b___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" <<1 );
   select = BSON("a" << "" << "b" << "" );
   orderby = BSON("b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}


TEST_F( indexcoverTest, keyab__a$fieldb__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON( "$gt" << BSON("$field" << "b")) );
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__a$fieldb__c__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << BSON( "$gt" << BSON("$field" << "b")) );
   select = BSON("c" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__$and_ab__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("$and" << BSON_ARRAY(BSON("a" << BSON("$gt" << 2 )) <<
                                       BSON("b" << BSON("$gt" << 2 )) )) ;
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyab__$and_ab__c__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("$and" << BSON_ARRAY(BSON("a" << BSON("$gt" << 2 )) <<
                                       BSON("b" << BSON("$gt" << 2 )) ));
   select = BSON("c" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__$and_ac__a__NULL___false )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("$and" << BSON_ARRAY(BSON("a" << BSON("$gt" << 2 )) <<
                                       BSON("c" << BSON("$gt" << 2 )) ));
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyab__$or_ab__a__NULL___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("$or" << BSON_ARRAY(BSON("a" << BSON("$gt" << 2 )) <<
                                      BSON("b" << BSON("$gt" << 2 )) ));
   select = BSON("a" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

// find({a:1}, {"a":null,"b":null}).sort({a:-1,"b":1}).hint({"":"ab"})
TEST_F( indexcoverTest, keyab__a__ab__ab___true )
{
   hint = BSON(""<< "ab") ;
   matcher = BSON("a" << 1) ;
   select = BSON("a" << "null" << "b" << "null" );
   orderby = BSON("a" << -1 << "b" << 1 ) ;
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

//////////////////(a.b) nest test
TEST_F( indexcoverTest, keyAdotB_NULL__adotbdotc__NULL_a_false )
{
   hint = BSON(""<< "adotb") ;
   select = BSON("a.b.c" << "" );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyAdotB__adotbdotc__adotb__adotb_adotb_true )
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a.b" << BSON("c" << 1) );
   select = BSON("a.b" << 1 );
   orderby = BSON("a.b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,TRUE ) ;
}

TEST_F( indexcoverTest, keyAdotB__adotb_c__adotb__adotbdotc_adotb_false )
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a.b" << BSON("c" << 1) );
   select = BSON("a.b" << 1 );
   orderby = BSON("a.b.c" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,FALSE ) ;
}

TEST_F( indexcoverTest, keyAdotB__adotb_c__adotbdotc__adotb_adotb_false)
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a.b" << BSON("c" << 1) );
   select = BSON("a.b.c" << 1 );
   orderby = BSON("a.b" << 1 );
   indexCoverCheck( matcher, select, orderby, hint,FALSE ) ;
}

// { "a.b": { $gt: { "$field": "a.b" } } },{ "a.b": "" } ).hint({"":"adotb"}
TEST_F( indexcoverTest, keyAdotB__adotbdotc$fieldadotbdotd__a__NULL_adotb_true)
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a.b" << BSON("$gt" << BSON("$field" << "a.b")) );
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcher, select, orderbyNull, hint,TRUE ) ;
}

// { "a.b.c": { $gt: { "$field": "a.b.d" } } },{ "a.b.d": "" } ).hint({"":"adotb"}
TEST_F( indexcoverTest, keyAdotB__adotb$fieldadotbdotd__a__NULL_adotb_false)
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a.b" << BSON("$gt" << BSON("$field" << "a.b.d")) );
   select = BSON("a.b.d" << 1 );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

//({ a: { $elemMatch:{ "b": { $et:1 } } } },{"a.b":""} ).hint({"":"adotb"})
TEST_F( indexcoverTest, keyAdotB__matcher_elemMatch_false)
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a" << BSON("$elemMatch" << BSON("b" << BSON("$et" << 1 ))));
   select = BSON("a.b" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

//({ a: { $elemMatch:{ "b": { $elemMatch: { c1: 1 } } } } },{"a.b":""} ).hint({"":"adotb"})
TEST_F( indexcoverTest, keyAdotB__matcher_elemElemMatch_false)
{
   hint = BSON(""<< "adotb") ;
   matcher = BSON("a" << BSON("$elemMatch" << BSON("b" << BSON("$elemMatch" << BSON("c1" << 1 ) ))));
   select = BSON("a.b" << "" );
   indexCoverCheck( matcher, select, orderbyNull, hint,FALSE ) ;
}

//({},{ a: { $elemMatch:{ "b": { $et:1 } } } }).hint({"":"adotb"}).
TEST_F( indexcoverTest, keyAdotB__select_elemMatch_false)
{
   hint = BSON(""<< "adotb") ;
   select = BSON("a" << BSON("$elemMatch" << BSON("b" << BSON("$et" << 1 ))));
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

//({},{ a: { $elemMatch:{ "b": { $elemMatch: { c1: 1 } } } } }).hint({"":"adotb"})
TEST_F( indexcoverTest, keyAdotB__select_elemElemMatch_false)
{
   hint = BSON(""<< "adotb") ;
   select = BSON("a" << BSON("$elemMatch" << BSON("b" << BSON("$elemMatch" << BSON("c1" << 1 ) ))));
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

//( {},{"a":1}).hint({"":"adotbAndc"})
TEST_F( indexcoverTest, keyadotbAndc__NULL__a__NULL___false)
{
   hint = BSON(""<< "adotbAndc") ;
   select = BSON("a" << 1);
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

// ( {},{"a.b":1}).hint({"":"adotbAndc"})
TEST_F( indexcoverTest, keyadotbAndc__NULL__adotb__NULL___true)
{
   hint = BSON(""<< "adotbAndc") ;
   select = BSON("a.b" << 1);
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

// ( {},{c:""}).hint({"":"adotbAndc"})
TEST_F( indexcoverTest, keyadotbAndc__NULL__c__NULL___true)
{
   hint = BSON(""<< "adotbAndc") ;
   select = BSON("c" << 1);
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

// ( {},{"a.b":1,c:""}).hint({"":"adotbAndc"})
TEST_F( indexcoverTest, keyadotbAndc__NULL__adotb_c__NULL___true)
{
   hint = BSON(""<< "adotbAndc") ;
   select = BSON("a.b" << 1 << "c" << "");
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

// ( {},{"a.b.b1":1,c:""}).hint({"":"adotbAndc"})
TEST_F( indexcoverTest, keyadotbAndc__NULL__adotbdotb1_c__NULL___false)
{
   hint = BSON(""<< "adotbAndc") ;
   select = BSON("a.b.b1" << 1 << "c" << "");
   indexCoverCheck( matcherNull, select, orderbyNull, hint,FALSE ) ;
}

//////////////////////////////// index field merge test
//({},{"a.b":""}).hint({"":"adotb"})
TEST_F( indexcoverTest, keyadotb__NULL__adotb__NULL___true)
{
   hint = BSON(""<< "adotb") ;
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

// find({},{"a.b":""}).hint({"":"a-adotb"})
TEST_F( indexcoverTest, keyaAndadotb__NULL__adotb__NULL___true)
{
   hint = BSON(""<< "aAndadotb") ;
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}
// find({},{"a.b":""}).hint({"":"adotb-a"})
TEST_F( indexcoverTest, keyadotbAnda__NULL__adotb__NULL___true)
{
   hint = BSON(""<< "adotbAnda") ;
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}
// find({},{"a.b":""}).hint({"":"adotb-adotc"})
TEST_F( indexcoverTest, keyadotbAndadotc__NULL__adotb__NULL___true)
{
   hint = BSON(""<< "adotbAndadotc") ;
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}
// find({},{"a.b":""}).hint({"":"d-a-adotb"})
TEST_F( indexcoverTest, keyadotb__NULL__dAndaAndadotb__NULL___true)
{
   hint = BSON(""<< "dAndaAndadotb") ;
   select = BSON("a.b" << 1 );
   indexCoverCheck( matcherNull, select, orderbyNull, hint,TRUE ) ;
}

INT32 _tmain ( INT32 argc, CHAR* argv[] )
{
   testing::InitGoogleTest ( &argc, argv ) ;
   return RUN_ALL_TESTS () ;
}



