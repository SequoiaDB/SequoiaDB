/************************************************************************
 * @Description : Test lob truncate operation:
 *					   seqDB-13437:truncateLob接口参数校验
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

class lobTruncateTest13437 : public testBase
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
      csName = "lobTruncateTestCs13437" ;
      clName = "lobTruncateTestCl13437" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;   
		
		// create lob
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
} ;

TEST_F( lobTruncateTest13437, illegalCl )
{
   INT32 rc = SDB_OK ;

	sdbCollectionHandle cl1 = SDB_INVALID_HANDLE ;
	rc = sdbTruncateLob( cl1, &oid, lobSize ) ;
	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test truncate lob with illegalCl" ;
}

TEST_F( lobTruncateTest13437, illegalOid )
{
   INT32 rc = SDB_OK ;

	// truncate lob with oid NULL
	rc = sdbTruncateLob( cl, NULL, lobSize ) ;
	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test truncate lob with oid NULL" ;

   // truncate lob with not exist oid
	bson_oid_t oid1 ;
	bson_oid_gen( &oid1 ) ;
	rc = sdbTruncateLob( cl, &oid1, lobSize ) ;
	ASSERT_EQ( SDB_FNE, rc ) << "fail to test truncate lob with not exist lob" ;
}

TEST_F( lobTruncateTest13437, illegalLen )
{
   INT32 rc = SDB_OK ;

	// truncate lob with illegal length
	rc = sdbTruncateLob( cl, &oid, -1 ) ;
	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test truncate lob with length -1" ;
}

TEST_F( lobTruncateTest13437, lenBoundary )
{
   INT32 rc = SDB_OK ;
	
	// truncate lob with length boundary
	rc = sdbTruncateLob( cl, &oid, 9223372036854775807 ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to test truncate lob with length boundary" ;
}
