/************************************************************************
 * @Description : Test lob truncate operation:
 *					   seqDB-13434:truncate删除lob数据 
 *                SEQUOIADBMAINSTREAM-2975
 * @Modify List : Liang xuewang
 *                2016-11-21
 ************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class lobTruncateTest13434 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
	bson_oid_t oid ;
	const CHAR* lobWriteBuffer ;
	INT32 lobSize ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobTruncateTestCs13434" ;
      clName = "lobTruncateTestCl13434" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;   
		
		bson_oid_gen( &oid ) ;
		lobWriteBuffer = "123456789ABCDEabcde" ;
		lobSize = strlen( lobWriteBuffer ) ;
		rc = createLob( cl, oid, lobWriteBuffer, lobSize ) ;
		ASSERT_EQ( SDB_OK, rc ) ;
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

	// get lob size with listLobs
	INT32 getLobSizeWithListLobs( sdbCollectionHandle cl, bson_oid_t oid, SINT64* size )
	{
		INT32 rc = SDB_OK ;
		sdbCursorHandle cursor ;
		bson obj ;

		rc = sdbListLobs( cl, &cursor ) ;
		CHECK_RC( SDB_OK, rc, "fail to list lobs, rc = %d", rc ) ;
		bson_init( &obj ) ;
		while( !sdbNext( cursor, &obj ) )
		{
			bson_iterator it ;
			bson_find( &it, &obj, "Oid" ) ;
			bson_oid_t* oid1 = bson_iterator_oid( &it ) ;
			if( isOidEqual( *oid1, oid ) )
			{
				bson_find( &it, &obj, "Size" ) ;
				*size = bson_iterator_long( &it ) ;
			}
			// bson_print( &obj ) ;
			bson_destroy( &obj ) ;
			bson_init( &obj ) ;
		}
		bson_destroy( &obj ) ;
		sdbCloseCursor( cursor ) ;
		sdbReleaseCursor( cursor ) ;
	done:
		return rc ;
	error:
		goto done ;
	}
} ;

TEST_F( lobTruncateTest13434, eqLobSize )
{
   INT32 rc = SDB_OK ;
	
	// truncate lob with lobSize
	INT64 truncateLen = lobSize ;
	rc = sdbTruncateLob( cl, &oid, truncateLen ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to truncate lob with lobSize" ;

   // get lob size
	SINT64 size = 0 ;
	rc = getLobSize( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( lobSize, size ) << "fail to check lobSize" ;	

	// get lob size with listLobs
	size = 0 ;
	rc = getLobSizeWithListLobs( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( lobSize, size ) << "fail to check lobSize with listLobs" ;

	// read lob
	CHAR* lobReadBuffer = (CHAR*)malloc( lobSize+1 ) ;
	memset( lobReadBuffer, 0, lobSize+1 ) ;
   UINT32 readSize = 0 ;
	rc = readLob( cl, oid, lobReadBuffer, lobSize, &readSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( lobSize, readSize ) << "fail to check read lob size" ;
	ASSERT_STREQ( lobWriteBuffer, lobReadBuffer ) << "fail to check lobReadBuffer" ;

	// free buffer
   free( lobReadBuffer ) ;
}

TEST_F( lobTruncateTest13434, gtLobSize )
{
   INT32 rc = SDB_OK ;

	// truncate lob with >lobSize
	INT32 truncateLen = lobSize * 2 ;
	rc = sdbTruncateLob( cl, &oid, truncateLen ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to truncate lob with >lobSize" ;

   // get lob size
	SINT64 size = 0 ;
	rc = getLobSize( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( lobSize, size ) << "fail to check lobSize" ;	

	// get lob size with listLobs
	size = 0 ;
	rc = getLobSizeWithListLobs( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( lobSize, size ) << "fail to check lobSize with listLobs" ;

	// read lob
	CHAR* lobReadBuffer = (CHAR*)malloc( lobSize+1 ) ;
	memset( lobReadBuffer, 0, lobSize+1 ) ;
   UINT32 readSize = 0 ;
	rc = readLob( cl, oid, lobReadBuffer, lobSize, &readSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( lobSize, readSize ) << "fail to check read lob size" ;
	ASSERT_STREQ( lobWriteBuffer, lobReadBuffer ) << "fail to check lobReadBuffer" ;

	// free buffer
   free( lobReadBuffer ) ;
}

TEST_F( lobTruncateTest13434, ltLobSize )
{
   INT32 rc = SDB_OK ;

	// truncate lob with <lobSize
	INT64 truncateLen = lobSize-5 ;
	rc = sdbTruncateLob( cl, &oid, truncateLen ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to truncate lob with <lobSize" ;

   // get lob size
	SINT64 size = 0 ;
	rc = getLobSize( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( truncateLen, size ) << "fail to check lobSize" ;	

	// get lob size with listLobs
	size = 0 ;
	rc = getLobSizeWithListLobs( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( truncateLen, size ) << "fail to check lobSize with listLobs" ;

	// read lob
	CHAR* lobReadBuffer = (CHAR*)malloc( lobSize ) ;
	memset( lobReadBuffer, 0, lobSize ) ;
   UINT32 readSize = 0 ;
	rc = readLob( cl, oid, lobReadBuffer, lobSize, &readSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( truncateLen, readSize ) << "fail to check read lob size" ;
	ASSERT_STREQ( "123456789ABCDE", lobReadBuffer ) << "fail to check lobReadBuffer" ;

	// free buffer
   free( lobReadBuffer ) ;
}

TEST_F( lobTruncateTest13434, zero )
{
   INT32 rc = SDB_OK ;

	// truncate lob with 0
	INT64 truncateLen = 0 ;
	rc = sdbTruncateLob( cl, &oid, truncateLen ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to truncate lob with zero" ;

   // get lob size
	SINT64 size = 0 ;
	rc = getLobSize( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( truncateLen, size ) << "fail to check lobSize" ;	

	// get lob size with listLobs
	size = 0 ;
	rc = getLobSizeWithListLobs( cl, oid, &size ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	ASSERT_EQ( truncateLen, size ) << "fail to check lobSize with listLobs" ;

	// read lob
	CHAR* lobReadBuffer = (CHAR*)malloc( lobSize ) ;
	memset( lobReadBuffer, 0, lobSize ) ;
   UINT32 readSize = 0 ;
	rc = readLob( cl, oid, lobReadBuffer, lobSize, &readSize ) ;
   ASSERT_EQ( SDB_EOF, rc ) ;

   // free buffer
   free( lobReadBuffer ) ;
}
