/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-8039
 *               seqDB-25369:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
                 seqDB-25370:不开启事务使用SDB_FLG_QUERY_FOR_SHARE读数
 * @Author:      Xiao zhenfan
 * @CreateTime： 2022-02-21
 * @LastEditTime:2022.03.03
 * @LastEditors: xiao zhenfan
 *			 	  
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
class query25369_25370 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   void SetUp() 
   {
      testBase::SetUp() ;   
      INT32 rc = SDB_OK;
      csName = "cs_25369_25370" ;
      clName = "cl_25369_25370" ;
      cs = 0 ;
      cl = 0 ;
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
} ;

INT32 getLockCount( bson obj,
                    const CHAR* lockType )  //返回-1表示获取集合锁类型发生错误
{
   INT32 rc = SDB_OK;
   INT32 lockCount = 0 ;
   bson_iterator it1,sub1 ;
   bson_find( &it1, &obj, "GotLocks") ;
   bson_iterator_subiterator( &it1, &sub1 ) ;
   while( BSON_EOO != bson_iterator_next( &sub1 ) )
   {
       bson_type type ;
       bson_iterator_more( &sub1 ) ;
       bson subObj1 ;
       bson_init( &subObj1 ) ;
       bson_iterator_subobject( &sub1, &subObj1 ) ;
       bson_iterator it ;
       type = bson_find( &it, &subObj1, "CSID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'CSID' should be 'BSON_INT' " << std::endl ;
          return  -1 ; 
       }
       INT32 CSID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "CLID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'CLID' should be 'BSON_INT' " << std::endl ;
          return  -1 ;
       }
       INT32 CLID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "Mode" ) ;
       if( BSON_STRING != type )
       {
          std::cout << "type of 'Mode' should be 'BSON_STRING' " << std::endl ;
          return  -1 ;
       }
       const CHAR* mode = bson_iterator_string( &it ) ;
       type = bson_find( &it, &subObj1, "ExtentID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'ExtentID' should be 'BSON_INT' " << std::endl ;
          return  -1 ;
       }
       INT32 ExtentID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "Offset" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'Offset' should be 'BSON_INT' " << std::endl ;
          return  -1 ;
       }
       INT32 Offset = bson_iterator_int( &it ) ;
       bson_destroy( &subObj1 ) ; 
       if( CSID>=0 && CLID>=0 && ExtentID>=0 && Offset>=0 )
       {
           INT32 result = strcmp( lockType, mode ) ;
           if( result == 0 ) 
           {
               lockCount++;
           }
       }
   }
   return lockCount ;
}
BOOLEAN checkIsLockEscalated( bson obj  )
{
   INT32 rc = SDB_OK;
   BOOLEAN isLockEscalated =  false ;
   bson_type type ;
   bson_iterator it ;
   type = bson_find( &it, &obj, "IsLockEscalated") ;
   if( BSON_BOOL != type )
   {
      std::cout << "type of 'IsLockEscalated' should be 'BSON_BOOL' " << std::endl ;
      return isLockEscalated ;
   }
   isLockEscalated = bson_iterator_bool( &it ) ;
   return isLockEscalated ;
}

const CHAR* getCLLockType ( bson obj )
{
   INT32 rc = SDB_OK;
   //定义非法锁类型的值为'll'
   const CHAR* clLockType = "ll";
   bson_iterator it1,sub1 ;
   bson_find( &it1, &obj, "GotLocks") ;
   bson_iterator_subiterator( &it1, &sub1 ) ;
   while( BSON_EOO != bson_iterator_next( &sub1 ) )
   {
       bson_type type ;
       bson_iterator_more( &sub1 ) ;
       bson subObj1 ;
       bson_init( &subObj1 ) ;
       bson_iterator_subobject( &sub1, &subObj1 ) ;
       bson_iterator it ;
       type = bson_find( &it, &subObj1, "CSID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'CSID' should be 'BSON_INT' " << std::endl ;
          return  clLockType ; 
       }
       INT32 CSID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "CLID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'CLID' should be 'BSON_INT' " << std::endl ;
          return  clLockType ;
       }
       INT32 CLID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "Mode" ) ;
       if( BSON_STRING != type )
       {
          std::cout << "type of 'Mode' should be 'BSON_STRING' " << std::endl ;
          return  clLockType ;
       }
       const CHAR* mode = bson_iterator_string( &it ) ;
       type = bson_find( &it, &subObj1, "ExtentID" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'ExtentID' should be 'BSON_INT' " << std::endl ;
          return  clLockType ;
       }
       INT32 ExtentID = bson_iterator_int( &it ) ;
       type = bson_find( &it, &subObj1, "Offset" ) ;
       if( BSON_INT != type )
       {
          std::cout << "type of 'Offset' should be 'BSON_INT' " << std::endl ;
          return  clLockType ;
       }
       INT32 Offset = bson_iterator_int( &it ) ;
       bson_destroy( &subObj1 ) ; 
       if( CSID>=0 && CLID>=0 && CLID!=65535 &&ExtentID==-1 && Offset==-1 )
       {
          clLockType = mode ;
       }
   }
   return clLockType ;
}

BOOLEAN checkResult( sdbCursorHandle cursor )
{
   // check result
   INT32 count = 0 ;
   bson ret ;
   bson_init( &ret ) ;
   while( !sdbNext( cursor, &ret ) )
   {
      bson_type type ;
      bson_iterator it ; 
      type = bson_find( &it, &ret, "id" ) ;
      if( BSON_INT != type )
      {
         std::cout << "type of 'id' should be 'BSON_INT' " << std::endl ;
         return false ;
      }
      INT32 val = bson_iterator_int( &it ) ;
      if( count != val )
      {
          return false ;
      }
      count++ ;
      bson_destroy( &ret ) ;
      bson_init( &ret ) ;
   }
   bson_destroy( &ret ) ;
   if( count == 20 )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

TEST_F( query25369_25370, SDB_QUERY )
{
   //create index
   INT32 rc = SDB_OK;
   bson idxDef ;
   bson_init( &idxDef ) ;
   bson_append_int( &idxDef, "id", 1 ) ;
   bson_finish( &idxDef ) ;
   rc = sdbCreateIndex( cl, &idxDef, "id", false, false ) ;
   bson_destroy( &idxDef ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create index,rc = " << rc  ;
   bson obj ;
   bson_init ( &obj );
   bson_append_int ( &obj, "TransIsolation", 0 ) ;
   bson_append_int ( &obj, "TransMaxLockNum", 10 ) ;
   bson_finish ( &obj ) ;
   rc = sdbSetSessionAttr( db, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to set sessionAttr,rc = " << rc  ;
   bson_destroy ( &obj ) ;
   // insert datas
   bson* docs[25] ; 
   for( INT32 i = 0; i < 20; i++ )
   {   
      docs[i] = bson_create() ;
      bson_append_int( docs[i], "id", i ) ; 
      bson_finish( docs[i] ) ; 
   }   
   rc = sdbBulkInsert( cl, 0, docs, 20 ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   for( INT32 i = 0; i < 20; i++ )
   {   
      bson_dispose( docs[i] ) ; 
   }   
   //开启事务
   rc = sdbTransactionBegin( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to begin trans,rc = " << rc  ;
   
   //不指定falg，指定索引查询前10条数据，
   bson_init( &obj ) ;
   bson_append_int( &obj, "$lt", 10 ) ;
   bson_finish( &obj ) ;
   bson cond ;
   bson_init( &cond ) ;
   bson_append_bson( &cond, "id", &obj ) ;
   bson_finish( &cond ) ;
   bson hint ;
   bson_init ( &hint ) ;
   bson_append_string( &hint, "", "id" ) ;
   bson_finish( &hint ) ;
   sdbCursorHandle cursor_query ;
   rc = sdbQuery( cl, &cond, NULL, NULL, &hint, 0, -1, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   bson_destroy ( &obj ) ;
   //查看当前事务快照，没有锁
   sdbCursorHandle cursor_snapshot ;
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor_snapshot ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to dbGetSnapshot SDB_SNAP_TRANSACTIONS_CURRENT,rc = " << rc  ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor_snapshot, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext,rc = " << rc  ;
   INT32 lockCount = getLockCount( obj, "S" ) ;
   ASSERT_EQ( 0, lockCount  ) << "S锁的数量应该为0，实际数量为 " << lockCount ;
   bson_destroy ( &obj ) ;
   
   //seqDB-25369:指定flags为SDB_FLG_QUERY_FOR_SHARE查询前10条数据
   rc = sdbQuery1( cl, &cond, NULL, NULL, &hint, 0, -1, QUERY_FOR_SHARE, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   bson_destroy ( &cond ) ;
   //校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor_snapshot ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to dbGetSnapshot SDB_SNAP_TRANSACTIONS_CURRENT,rc = " << rc  ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor_snapshot, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext,rc = " << rc  ;
   BOOLEAN isLockEscalated = checkIsLockEscalated( obj ) ;
   ASSERT_EQ( false, isLockEscalated  ) << "IsLockEscalated的值应该为false,实际值为 " << isLockEscalated ;
   lockCount = getLockCount( obj, "S" ) ;
   ASSERT_EQ( 10, lockCount  ) << "S锁的数量应该为10，实际数量为 " << lockCount ;
   const CHAR* clLockType = getCLLockType( obj ) ;
   ASSERT_EQ( 0, strcmp("IS", clLockType ) ) << "集合锁的类型应该为IS,实际类型为 " << clLockType ;
   bson_destroy ( &obj ) ;
   
   // 不指定flags查询后10条数据
   bson_init( &obj ) ;
   bson_append_int( &obj, "$gt", 10 ) ;
   bson_finish( &obj ) ;
   bson_init( &cond ) ;
   bson_append_bson( &cond, "id", &obj ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery( cl, &cond, NULL, NULL, &hint, 0, -1, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   bson_destroy ( &obj ) ;
   // 记录锁数量不变，集合锁不变，没有发生锁升级
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor_snapshot ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to dbGetSnapshot SDB_SNAP_TRANSACTIONS_CURRENT,rc = " << rc  ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor_snapshot, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext,rc = " << rc  ;
   isLockEscalated = checkIsLockEscalated( obj ) ;
   ASSERT_EQ( false, isLockEscalated  ) << "IsLockEscalated的值应该为false,实际值为 " << isLockEscalated  ;
   lockCount = getLockCount( obj, "S" ) ;
   ASSERT_EQ( 10, lockCount  ) << "S锁的数量应该为10，实际数量为 " << lockCount ;
   clLockType = getCLLockType( obj ) ;
   ASSERT_EQ( 0, strcmp( "IS", clLockType ) ) << "集合锁的类型应该为IS,实际类型为 " << clLockType ;
   bson_destroy ( &obj ) ;

   //指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
   rc = sdbQuery1( cl, &cond, NULL, NULL, &hint, 0, -1, QUERY_FOR_SHARE, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor_snapshot ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to dbGetSnapshot SDB_SNAP_TRANSACTIONS_CURRENT,rc = " << rc  ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor_snapshot, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbNext,rc = " << rc  ;
   isLockEscalated = checkIsLockEscalated( obj ) ;
   ASSERT_EQ( true, isLockEscalated  ) << "IsLockEscalated的值应该为true,实际值为 " << isLockEscalated  ;
   clLockType = getCLLockType( obj ) ;
   ASSERT_EQ( 0, strcmp( "S", clLockType ) ) << "集合锁的类型应该为S,实际类型为 " << clLockType ;
   lockCount = getLockCount( obj, "S" ) ;
   ASSERT_EQ( 11, lockCount  ) << "S锁的数量应该为11，实际数量为 " << lockCount ;
   bson_destroy ( &obj ) ;

   //事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验数据正确性
   rc = sdbQuery1( cl, NULL, NULL, NULL, &hint, 0, -1, QUERY_FOR_SHARE, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   // check result
   BOOLEAN isRight = checkResult( cursor_query ) ;
   ASSERT_EQ( true, isRight ) ;

   //seqDB-25370:不开启事务使用SDB_FLG_QUERY_FOR_SHARE读数据
   rc = sdbTransactionCommit( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to commit trans,rc = " << rc  ;
   rc = sdbQuery1( cl, NULL, NULL, NULL, &hint, 0, -1, QUERY_FOR_SHARE, &cursor_query ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query,rc = " << rc  ;
   bson_destroy ( &hint ) ;
   // check result
   isRight = checkResult( cursor_query ) ;
   ASSERT_EQ( true, isRight ) ;
   
   sdbCloseCursor( cursor_query ) ;
   sdbReleaseCursor( cursor_query ) ;
   sdbCloseCursor( cursor_snapshot ) ;
   sdbReleaseCursor( cursor_snapshot ) ;
}
