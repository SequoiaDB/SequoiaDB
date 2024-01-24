/************************************************************************
 * @Description : 通过与服务端交互获取lobID,然后创建lob
 *                listLob/listLobPieces 支持条件，选择等操作符
 * @Modify List : wenjing wang
 *                2020-05-22
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char *content= "aaaaaaaaaa" ;
class lobCreateandListTest : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   bson_oid_t lobIds[3] ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobCreateandList_22190" ;
      clName = "lobCreateandList_22190" ;
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


INT32 writeLob( sdbCollectionHandle cl, const bson_oid_t &oid, bson_oid_t& ret)
{
   sdbLobHandle lob ;
   INT32 rc = sdbOpenLob(cl, &oid, SDB_LOB_CREATEONLY, &lob) ;
   if ( rc != 0 )
   {
      return rc ;
   }
   rc = sdbWriteLob(lob, content, strlen(content)) ;
   if ( rc == 0 )
   {
      rc = sdbGetLobId(lob, &ret) ;
   }
   sdbCloseLob(&lob);
   return rc ;
}

INT32 readLob(sdbCollectionHandle cl, const bson_oid_t &oid, bson_oid_t &ret, char *buff, int len)
{
   sdbLobHandle lob ;
   UINT32 read ;
   INT32 rc = sdbOpenLob(cl, &oid, SDB_LOB_READ, &lob) ;
   if ( rc != 0 )
   {
      return rc ;
   }
   int size = len ;
   do
   {
      char *ptmp = buff ;
      rc = sdbReadLob( lob, size, ptmp, &read ) ;
      if ( rc != 0 && rc != SDB_EOF )
      {
         break ;
      }
      ptmp += read ;
      size -= read ;
   }while( size != 0 && rc != SDB_EOF ) ;
   
   if ( rc == 0 || rc == SDB_EOF )
   {
      rc = sdbGetLobId(lob, &ret) ;
   }
   sdbCloseLob(&lob);
   return rc ;
}

TEST_F( lobCreateandListTest, createLob_22190 )
{
   INT32 rc = SDB_OK ;
   bson_oid_t retId;
   char buff[10] = {0} ;
   const char *pTimeStamp = "2019-08-23-18.04.07";
   bson_oid_gen(&lobIds[0]) ;
   
   rc = sdbCreateLobID( cl , &lobIds[1]) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lobId" ;
   
   rc = sdbCreateLobID1(cl, pTimeStamp, &lobIds[2]) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lobId" ;
 
 
   for ( int i = 0; i < sizeof( lobIds ) / sizeof(lobIds[0]); ++i)
   {
      rc = writeLob( cl, lobIds[i], retId) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
      rc = memcmp( &lobIds[i], &retId, sizeof(bson_oid_t) ) ;
      ASSERT_EQ( SDB_OK, rc ) << "lob id not equal" ;
   
      rc = readLob(cl, lobIds[i], retId, buff, sizeof(buff) ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
      rc = memcmp( &lobIds[i], &retId, sizeof(bson_oid_t) ) ;
      ASSERT_EQ( SDB_OK, rc ) << "lob id not equal" ;
   
      rc = memcmp( buff, content, sizeof(buff) ) ;
      ASSERT_EQ( SDB_OK, rc ) << "lob content not equal" ;
   }
}

TEST_F( lobCreateandListTest, listLobs_22191 )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor ;
   BOOLEAN isFind = FALSE ;
   bson_oid_t lobId,retId ;
   bson_oid_gen(&lobId) ;
   rc = writeLob( cl, lobId, retId );
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;

   rc = sdbListLobs(cl, &cursor) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list lobs" ;
   bson obj ;
   bson_init(&obj) ;
   while ( (rc = sdbNext(cursor, &obj)) == 0 ){
      bson_print( &obj ) ;
      bson_iterator it ;
      bson_find( &it, &obj, "Oid") ;
      bson_oid_t *oid = bson_iterator_oid(&it) ;
      if ((rc = memcmp( oid, &lobId, sizeof(bson_oid_t) )) == 0 )
      {
         isFind = TRUE ;
      }
      bson_destroy( &obj ) ;
      bson_init(&obj) ;
   }
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to list lobs" ;
   ASSERT_EQ( isFind, TRUE) << "fail to list lobs" ;
   
   sdbCloseCursor(cursor) ;
   sdbReleaseCursor(cursor) ;
   
   isFind = FALSE ; 
   bson cond ;
   bson_init(&cond) ;
   bson_append_oid(&cond, "Oid", &lobId) ;
   bson_finish(&cond) ;

   bson selected;
   bson_init(&selected) ;
   bson_append_string(&selected, "Size", "") ;
   bson_append_string(&selected, "Oid", "") ;
   bson_finish(&selected) ;
   bson orderby ;
   bson_init(&orderby) ;
   bson_append_int(&orderby, "Size", 1) ;
   bson_finish(&orderby) ;

   bson hint;
   bson_init(&hint) ;
   bson_append_int(&hint, "ListPieces", 1) ;
   bson_finish(&hint) ;
   rc = sdbListLobs1(cl, &cond, &selected, &orderby, &hint, 0, 1, &cursor);
   ASSERT_EQ( SDB_OK, rc ) << "fail to list lob" ;
   while ( (rc = sdbNext(cursor, &obj)) == 0 )
   {
      bson_print( &obj ) ;
      bson_iterator it ;
      bson_find( &it, &obj, "Oid") ;
      bson_oid_t *oid = bson_iterator_oid(&it) ;
      if ((rc = memcmp( oid, &lobId, sizeof(bson_oid_t) )) == 0)
      {
         isFind = TRUE ;
      }
      bson_destroy( &obj ) ;
      bson_init(&obj) ;
   }
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to list lobs" ;
   ASSERT_EQ( isFind, TRUE) << "fail to list lobs" ;
   sdbCloseCursor(cursor) ;
   sdbReleaseCursor(cursor) ;
   bson_destroy( &cond );
   bson_destroy( &selected );
   bson_destroy( &orderby );
   bson_destroy( &hint );
}

TEST_F( lobCreateandListTest, listLobPieces_22192 )
{
   INT32 rc = SDB_OK ;
   bson_oid_t lobId,retId ;
   bson_oid_gen(&lobId) ;
   rc = writeLob( cl, lobId, retId );
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   sdbCursorHandle cursor ;
   rc = sdbListLobPieces(cl, &cursor);
   ASSERT_EQ( SDB_OK, rc ) << "fail to list lob pieces" ;
   bson obj ;
   bson_init(&obj) ;
   BOOLEAN isFind = FALSE ;
   while ( (rc = sdbNext(cursor, &obj)) == 0 ){
      bson_print( &obj ) ;
      bson_iterator it ;
      bson_find( &it, &obj, "Oid") ;
      bson_oid_t *oid = bson_iterator_oid(&it) ;
      if ((rc = memcmp( oid, &lobId, sizeof(bson_oid_t) )) == 0 )
      {
         isFind = TRUE ;
      }
      bson_destroy( &obj ) ;
      bson_init(&obj) ;
   }
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to list lob pieces" ;
   ASSERT_EQ( isFind, TRUE) << "fail to list lob pieces" ;
   sdbCloseCursor(cursor) ;
   sdbReleaseCursor(cursor) ;
   
   isFind = FALSE ; 
   bson cond ;
   bson_init(&cond) ;
   bson_append_oid(&cond, "Oid", &lobId) ;
   bson_finish(&cond) ;

   bson selected;
   bson_init(&selected) ;
   bson_append_string(&selected, "Size", "") ;
   bson_append_string(&selected, "Oid", "") ;
   bson_finish(&selected) ;
   bson orderby ;
   bson_init(&orderby) ;
   bson_append_int(&orderby, "Size", 1) ;
   bson_finish(&orderby) ;

   bson hint;
   bson_init(&hint) ;
   bson_append_int(&hint, "ListPieces", 1) ;
   bson_finish(&hint) ;
   rc = sdbListLobPieces1(cl, &cond, &selected, &orderby, NULL, 0, 1, &cursor);
   ASSERT_EQ( SDB_OK, rc ) << "fail to list lob pieces" ;
   bson_init(&obj) ;
   while ( (rc = sdbNext(cursor, &obj)) == 0 )
   {
      bson_print( &obj ) ;
      bson_iterator it ;
      bson_find( &it, &obj, "Oid") ;
      bson_oid_t *oid = bson_iterator_oid(&it) ;
      if (( rc = memcmp( oid, &lobId, sizeof(bson_oid_t) ) == 0))
      {
         isFind = TRUE ;
      }
      bson_destroy( &obj ) ;
      bson_init(&obj) ;
   }
   
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to list lob pieces" ;
   ASSERT_EQ( isFind, TRUE) << "fail to list lob pieces" ;
   sdbCloseCursor(cursor) ;
   sdbReleaseCursor(cursor) ;
   bson_destroy( &cond );
   bson_destroy( &selected );
   bson_destroy( &orderby );
   bson_destroy( &hint );
}
