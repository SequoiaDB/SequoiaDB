#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include "deleteFile.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define RGNAME "group1"
#define TARGETRGNAME "targetRGGroupName"

TEST( sdb, activateShard )
{
   sdb connection ;
   sdbCollection collection ;
   sdbCursor cursor ;
   sdbShard shard ;
   sdbShard shard1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout<<"This test is for cluster only."<<endl ;
      goto done ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = connection.getShard( "group1", shard ) ;
      CHECK_MSG("%s%d\n", "rc = ", rc);
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = shard.stop();
      CHECK_MSG("%s%d\n", "rc = ", rc);
      ASSERT_EQ( SDB_OK, rc ) ;
      sleep( 3 ) ;
      rc = connection.activateShard( "group1", shard1 ) ;
      CHECK_MSG("%s%d\n", "rc = ", rc);
      ASSERT_EQ( SDB_OK, rc ) ;
      sleep( 3 ) ;
      rc = shard1.getDetail( obj ) ;
      CHECK_MSG("%s%d\n", "rc = ", rc);
      ASSERT_EQ( SDB_OK, rc ) ;
      cout << "shard detail is: " << obj.toString( false, false ) << endl ;
   }
done :
   connection.disconnect() ;
}

/*************************************
 *           test begine             *
 *************************************/
/*
TEST(sdb,listReplicaGroups)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.listReplicaGroups( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( cursor ) ;
   connection.disconnect() ;
}

TEST(sdb,getReplicaGroup_by_name)
{
   ASSERT_TRUE( 0==1 ) ;
   sdb connection ;
   sdbReplicaGroup rg ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getReplicaGroup( RGNAME, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getReplicaGroup_by_id)
{
   ASSERT_TRUE( 0==1 ) ;
   sdb connection ;
   sdbReplicaGroup rg ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getReplicaGroup( RGNAME, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,createReplicaGroup_)
{
   ASSERT_TRUE( 0==1 ) ;
   sdb connection ;
   sdbReplicaGroup rg ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,createReplicaCataGroup)
{
   ASSERT_TRUE( 0==1 ) ;
   sdb connection ;
   sdbReplicaGroup rg ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,activeReplicaGroup)
{
   ASSERT_TRUE( 0==1 ) ;
   sdb connection ;
   sdbReplicaGroup rg ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST( rg, removeReplicaGroup )
{
   sdb connection ;
   sdbCollection collection ;
   sdbCursor cursor ;
   sdbReplicaGroup rg ;
   sdbReplicaGroup rg1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   int NAMELEN = 256 ;
   char hostName[NAMELEN] ;
   char dataPath[NAMELEN] ;
   map<string, string> config ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout<<"This test is for cluster only."<<endl ;
      goto done ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = addGroup ( connection, GROUPNAME1, rg ) ;
      sleep ( 1 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = getHostName ( hostName, NAMELEN ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<"lineno:"<<__LINE__<<" host name is "<<hostName<<endl ;
      getDataPath ( dataPath, NAMELEN, _DATAPATH1, DATAPATH1 ) ;
      cout<<"data path is "<<dataPath<<endl ;
      rc = rg.createNode ( hostName, SERVER1, dataPath, config ) ;
      sleep ( 3 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = rg.start() ;
      sleep ( 3 ) ;
      cout<<"rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = connection.removeReplicaGroup ( GROUPNAME1 ) ;
      sleep ( 1 ) ;
      cout<<"rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = connection.getReplicaGroup ( GROUPNAME1, rg1 ) ;
      cout<<"rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_CLS_GRP_NOT_EXIST, rc ) ;
   } // cluster environment
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   } // something wrong

   connection.disconnect() ;
done:
   return ;
}


TEST( rg, removeNode )
{
   sdb connection ;
   sdbCollection collection ;
   sdbCursor cursor ;
   sdbReplicaGroup rg ;
   sdbReplicaGroup rg1 ;
   sdbReplicaNode node ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   int NAMELEN = 256 ;
   char hostName[NAMELEN] ;
   char dataPath1[NAMELEN] ;
   char dataPath2[NAMELEN] ;
   map<string, string> config ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout<<"This test is for cluster only."<<endl ;
      goto done ;
   } // standalone mode
   else if ( rc == SDB_OK )
   {
      rc = addGroup ( connection, GROUPNAME1, rg ) ;
      sleep ( 1 ) ;
      cout<<"rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = getHostName ( hostName, NAMELEN ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<"host name is "<<hostName<<endl ;
      getDataPath ( dataPath1, NAMELEN, _DATAPATH1, DATAPATH1 ) ;
      cout<<"data path1 is "<<dataPath1<<endl ;
      rc = rg.createNode ( hostName, SERVER1, dataPath1, config ) ;
      sleep ( 1 ) ;
      getDataPath ( dataPath2, NAMELEN, _DATAPATH2, DATAPATH2 ) ;
      cout<<"data path2 is "<<dataPath2<<endl ;
      rc = rg.createNode ( hostName, SERVER2, dataPath2, config ) ;
      sleep ( 1 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = rg.start() ;
      sleep ( 5 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ob.append ( "enforced", true ) ;
      obj = ob.obj () ;
      rc = rg.removeNode ( hostName, SERVER1 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = rg.getNode ( hostName, SERVER1, node ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      sleep ( 1 ) ;
      ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) ;

      rc = connection.removeReplicaGroup ( GROUPNAME1 ) ;
      sleep ( 1 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = connection.getReplicaGroup ( GROUPNAME1, rg1 ) ;
      cout<<"lineno:"<<__LINE__<<" rc is "<<rc<<endl ;
      ASSERT_EQ( SDB_CLS_GRP_NOT_EXIST, rc ) ;
   } // cluster environment
   else
   {
      ASSERT_EQ( SDB_OK, rc ) ;
   } // something wrong

   connection.disconnect() ;
done:
   return ;
}
*/

/*
TEST( collection, split_hash )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbReplicaGroup rg ;
   sdbReplicaNode node ;
   const CHAR *pHostName          = HOST ;
   const CHAR *pPort              = SERVER ;
   const CHAR *pUsr               = USER ;
   const CHAR *pPasswd            = PASSWD ;
   INT32 rc                       = SDB_OK ;
   INT32 NUM                      = 10000 ;
   INT32 i                        = 0 ;
   SINT64 count                   = 0 ;
   const CHAR *sourceRG           = GROUPNAME ;
   const CHAR *targetRG           = "splitTargetRG" ;
   INT32 arrLen                   = 256 ;
   CHAR hostName[arrLen] ;
   CHAR dataPath[arrLen] ;
   BSONObj obj ;
   BSONObj clInfo ;
   BSONObj cond ;
   BSONObj endCond ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   clInfo = BSON ( "ShardingKey" << BSON ( "id" << 1 ) <<
                   "ShardingType" << "hash" <<
                   "Partition" << 1024 ) ;
   rc = cs.createCollection ( COLLECTION_SPLIT, clInfo, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for ( i=0; i < NUM; i++ )
   {
      rc = cl.insert(BSON("id"<<i)) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   getHostName ( hostName, arrLen ) ;
   getDataPath ( dataPath, arrLen, _DATAPATH1, DATAPATH1 ) ;
   deleteFile ( dataPath ) ;
   rc = connection.createReplicaGroup ( targetRG, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::map<std::string,std::string> config ;
   rc = rg.createNode ( hostName, SERVER1, dataPath, config ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = rg.start () ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sleep ( 15 ) ;

   cond = BSON ( "Partition" << 0 ) ;
   endCond = BSON ( "Partition" << 512 ) ;
   rc = cl.split ( sourceRG, targetRG, cond, endCond ) ;
   cout << "rc is: " << rc  << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.getCount ( count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "count is: " << count << endl ;
   ASSERT_EQ( NUM, count ) ;
   connection.disconnect() ;

}
*/
