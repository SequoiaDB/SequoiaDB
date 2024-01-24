/**************************************************************************
 * @Description:   seqDB-23377:验证sdbGetRunTimeDetail接口
 * @Modify:        Li Yuanyue
 *                 2021-01-14
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

class lob23377 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      pCsName = "lob23377" ;
      pClName = "lob23377" ;

      // create cs, cl
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << pCsName ;
      } 
      testBase::TearDown() ;
   }

} ;

TEST_F( lob23377, getRunTimeDetail )
{
   INT32 rc = SDB_OK ;
   sdbLob lob ;
   INT32 offset = 2 ;
   INT32 length = 20 ;
   bson::BSONObj lobRunTimeDetail ;
   OID oid ;

   // create
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;

   // get oid 
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;

   // SDB_LOB_SHAREREAD
   // open lob
   rc = cl.openLob( lob, oid, SDB_LOB_SHAREREAD ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with SDB_LOB_SHAREREAD" ;

   // lock and seek
   rc = lob.lockAndSeek( offset, length ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock and seek" ;

   // getRunTimeDetail
   rc = lob.getRunTimeDetail( lobRunTimeDetail ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getRunTimeDetail" ;

   INT32 exp_RefCount = 1 ;
   INT32 exp_ReadCount = 0 ;
   INT32 exp_WriteCount =  0 ;
   INT32 exp_ShareReadCount = 1 ;
   FLOAT64 exp_Begin = offset ;
   FLOAT64 exp_End = offset + length ;
   string exp_LockType( "S" ) ; 

   // check
   INT32 act_RefCount = lobRunTimeDetail["AccessInfo"]["RefCount"].Int() ;
   INT32 act_ReadCount = lobRunTimeDetail["AccessInfo"]["ReadCount"].Int() ;
   INT32 act_WriteCount = lobRunTimeDetail["AccessInfo"]["WriteCount"].Int() ;
   INT32 act_ShareReadCount = lobRunTimeDetail["AccessInfo"]["ShareReadCount"].Int() ;

   BSONObjIterator it = lobRunTimeDetail["AccessInfo"]["LockSections"].embeddedObject().begin() ;
   bson::BSONObj LockSections = it.next().Obj() ;
   
   FLOAT64 act_Begin = LockSections["Begin"].Number() ;
   FLOAT64 act_End = LockSections["End"].Number() ;
   string act_LockType = LockSections["LockType"].String() ;

   ASSERT_EQ( act_ReadCount, exp_ReadCount ) << "RefCount" ;
   ASSERT_EQ( act_ReadCount, exp_ReadCount ) << "ReadCount" ;
   ASSERT_EQ( act_WriteCount, exp_WriteCount ) << "WriteCount" ;
   ASSERT_EQ( act_ShareReadCount, exp_ShareReadCount ) << "ShareReadCount" ;
   ASSERT_EQ( act_Begin, exp_Begin ) << "Begin" ;
   ASSERT_EQ( act_End, exp_End ) << "End" ;
   ASSERT_EQ( act_LockType, exp_LockType ) << "LockType" ;

   // close 
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
