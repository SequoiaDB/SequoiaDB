/**************************************************************
 * @Description: get all kind of list (storage units)
 *               seqDB-12526 : get all kind of list
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class getListStorageUnit12526 : public testBase 
{
protected:
   sdb dataDB ;
   sdbNode dataNode ;
   sdbCursor cursor ;
   BSONObj res ;
   const CHAR *pCsName ;

   void SetUp()
   {
      testBase::SetUp() ;

      // create cs cl
      pCsName = "getListStorageUnit12526" ;
      const CHAR *pClName = "getListStorageUnit12526" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      // special operation for standalone
      if( isStandalone( db ) )
      {
         rc = dataDB.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         return ;
      }

      // get cl group name
      vector<string> groupNames ;
      rc = getClGroups( db,"getListStorageUnit12526.getListStorageUnit12526", groupNames ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      // select a data group name
      INT32 groupNum = groupNames.size() ;
      string dataGroupName ;
      for( INT32 i = 0 ; i < groupNum ; ++i ) 
      {
         string currGroupName = groupNames[i] ;
         if( currGroupName != "SYSCoord" && currGroupName != "SYSCatalogGroup" )
         {
            dataGroupName = currGroupName ;
            break ;
         }
      }

      // get data node connection
      sdbReplicaGroup dataGroup ;
      rc = db.getReplicaGroup( dataGroupName.c_str(), dataGroup ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = dataGroup.getMaster( dataNode ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = dataNode.connect( dataDB ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;
      
      dataDB.disconnect() ;
      testBase::TearDown() ;
   }
} ;

TEST_F( getListStorageUnit12526, listStorageUnits )
{
   INT32 rc = SDB_OK ;
   rc = dataDB.getList( cursor, SDB_LIST_STORAGEUNITS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) << "list执行在:" << dataNode.getNodeName() ;
   ASSERT_TRUE( res.hasField( "CollectionHWM" ) ) ;
}
