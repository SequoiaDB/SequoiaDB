/**************************************************************
 * @Description : test case of sessionAttr
 * seqDB-14166  : 设置会话访问属性指定实例为instanceid，
 *                其中instanceid包含【8/9/10】
 * seqDB-14167  : 设置会话访问属性，
 *                指定instanceid和timeout属性
 * seqDB-14168  : 设置会话访问属性，单值指定访问实例为M/S/A
 * seqDB-14169  : 设置会话访问属性指定多个instanceid，
 *                其中节点选择模式为顺序选取
 * seqDB-14170  : getSessionAttr()获取驱动端缓存信息
 * seqDB-14171  : 设置会话访问属性指定实例为instanceid和[M/S/A]
 * seqDB-14172  : 设置timeout值，执行多次不同类型操作超时
 * seqDB-14173  : 设置timeout值，执行lob操作超时
 * @Modify      : Liang xuewang
 *                2018-02-12
 **************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <string>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;

void ossSleep(UINT32 milliseconds)
{
#if defined (_WINDOWS)
   Sleep(milliseconds);
#else
   usleep(milliseconds*1000);
#endif
}

#define CHECK_TIMEOUT( rc, msg ) \
do{ \
   if( SDB_TIMEOUT == rc ) \
   { \
      cout << msg << endl ; \
      goto timeout ; \
   } \
}while( 0 ) ;

class sessionAttrTest14166 : public testBase
{
protected:
   const CHAR* rgName ;
   const CHAR* csName ;
   const CHAR* clName ;
   sdbReplicaGroup rg ;
   sdbNode master ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   map<string, INT32> nodeInfo ;  // nodename instanceid
   INT32 primaryInstanceid ;

   void SetUp()
   {
      INT32 rc = 0 ;
      testBase::SetUp() ;
      if( isStandalone( db ) )
      {
         cout << "Run mode is standalone" << endl ;
         return ;
      }
      init() ;
      ASSERT_EQ( rc, SDB_OK );      
   }
   void TearDown()
   {
      if( isStandalone( db ) )
      {
         cout << "Run mode is standalone" << endl ;
         testBase::TearDown() ;
         return ;
      }
      fini();
      testBase::TearDown() ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "sessionAttrTestRg14166" ;
      csName = "sessionAttrTestCs14166" ;
      clName = "sessionAttrTestCl14166" ;
      BSONObj clOption ;
      const INT32 instanceids[] = { 8, 9, 10 } ;
      const INT32 size = sizeof(instanceids) / sizeof(instanceids[0]) ;

      // get local host name
      CHAR hostName[ MAX_NAME_SIZE+1 ] ;
      memset( hostName, 0, sizeof(hostName) ) ;
      rc = getDBHost( db, hostName, MAX_NAME_SIZE ) ;
      CHECK_RC( SDB_OK, rc, "fail to get hostname" ) ;
      
      // create rg and node with instanceid, start rg
      rc = db.getReplicaGroup( rgName, rg );
      if ( rc == 0 )
      {
         rc = db.removeReplicaGroup( rgName ) ;
         CHECK_RC( SDB_OK, rc, "fail to remove rg %s", rgName ) ;
      }
      rc = db.createReplicaGroup( rgName, rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s", rgName ) ;
      for( INT32 i = 0;i < size;i++ )
      {
         INT32 instanceid = instanceids[i] ;
         CHAR nodeSvc[ MAX_NAME_SIZE+1 ] = { 0 } ;
         INT32 port = atoi( ARGS->rsrvPortBegin() ) + 10 * i ;
         sprintf( nodeSvc, "%d", port ) ;
         CHAR nodePath[ MAX_NAME_SIZE+1 ] = { 0 } ;
         sprintf( nodePath, "%s%s%s", ARGS->rsrvNodeDir(), "data/", nodeSvc ) ;
         BSONObj nodeOption = BSON( "instanceid" << instanceid ) ;
         string nodename = hostName ;
         nodename += ":" ;
         nodename += nodeSvc ;

         cout << "create node: " << nodename << " " << nodePath 
              << " instanceid: " << instanceid << endl ;
         rc = rg.createNode( hostName, nodeSvc, nodePath, nodeOption ) ;
         CHECK_RC( SDB_OK, rc, "fail to create node" ) ;
         nodeInfo.insert( pair<string, INT32>( nodename, instanceid ) ) ;
      }
      rc = rg.start() ;
      CHECK_RC( SDB_OK, rc, "fail to start rg %s", rgName ) ;

      // get master node instanceid
      do
      {
         rc = rg.getMaster( master ) ;
         sleep( 1 ) ;
      }while( rc != SDB_OK ) ;
      primaryInstanceid = nodeInfo[ master.getNodeName() ] ;
      cout << "primary node instanceid: " << primaryInstanceid << endl ;

      // create cs and cl in rg
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cs %s", csName ) ;
      clOption = BSON( "Group" << rgName << "ReplSize" << 0 ) ;
      rc = cs.createCollection( clName, clOption, cl ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cl %s", clName ) ;
      rc = cl.insert(BSON("_id" <<1 ));
      CHECK_RC( SDB_OK, rc, "fail to insert");
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      INT32 sleepTimeLen = 2000 ;
      INT32 alreadySleepLen = 0 ;
      do{
         rc = db.dropCollectionSpace( csName ) ;
         if ( rc == SDB_OK ) break ;
         ossSleep( 10 ) ;
         alreadySleepLen += 10 ;
         if ( alreadySleepLen > sleepTimeLen ) break ;
      }while( rc == SDB_LOCK_FAILED );
      CHECK_RC( SDB_OK, rc, "fail to drop cs %s", csName ) ;
      rc = db.removeReplicaGroup( rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s", rgName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 getExplainNode( sdbCollection& cl, const BSONObj& cond, 
                         const BSONObj& option, string& nodename )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj obj ;

      rc = cl.explain( cursor, cond, _sdbStaticObject, _sdbStaticObject, 
                       _sdbStaticObject, 0, -1, 0, option ) ;
      CHECK_RC( SDB_OK, rc, "fail to explain" ) ;
      rc = cursor.next( obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to next" ) ;
      nodename = obj.getField( "NodeName" ).String() ;
      rc = cursor.close() ;
      CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( sessionAttrTest14166, mix )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   const CHAR* instances[] = { "M","m", "S","s","A","a","" } ;
   INT32 size = sizeof(instances) / sizeof(instances[0]) ;

   for( INT32 i = 0;i < size;i++ )
   {
      BSONArrayBuilder arrBuilder ;
      arrBuilder.append( 8 ) ;
      arrBuilder.append( 9 ) ;
      arrBuilder.append( 10 ) ;
      if( instances[i][0] != 0  )
      {
         arrBuilder.append( instances[i] ) ;
      }
      BSONObj obj = arrBuilder.done() ;
      BSONObjBuilder builder ;
      builder.appendArray( "PreferedInstance", obj ) ;
      builder.append( "PreferedInstanceMode", "ordered" ) ;
      BSONObj option = builder.done() ;
      cout << "sessionAttr: " << option << endl ;

      rc = db.setSessionAttr( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
      string nodename ;
      rc = getExplainNode( cl, _sdbStaticObject, _sdbStaticObject, nodename ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      INT32 instanceid = nodeInfo.at( nodename ) ;
      switch( i )
      {
      case 0:
      case 1: ASSERT_EQ( primaryInstanceid, instanceid ) ; //M|m
              break ;      
      case 2:  // S
      case 3:  // s 
      case 4:  // A
      case 5:  // a
      case 6:
              ASSERT_EQ( 8, instanceid ) ;
              break ;
      default: ASSERT_EQ( 1, 0 ) << "Wrong i value " << i ;
              break ;
      }
   }
}

