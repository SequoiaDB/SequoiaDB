#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define USERNAME               "SequoiaDB"
#define PASSWORD               "SequoiaDB"

#define NUM_RECORD             5

TEST(sdb,connect)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,connect_with_serval_addr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR* connArr[10] = {"192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              "localhost:50000",
                              "192.168.20.40:12340",
                              "localhost:12340",
                              "localhost:11810"} ;
   rc = connection.connect( connArr, 10, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, 5);
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}


TEST(sdb,disconnect)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}


TEST(sdb,createUsr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "createUsr use \
in the cluser environment only" << endl ;
      return ;
   }
   rc = connection.createUsr( USERNAME, PASSWORD ) ;
   connection.disconnect() ;
   rc = connection.connect( pHostName, pPort, USERNAME, PASSWORD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.removeUsr( USERNAME, PASSWORD ) ;
   connection.disconnect() ;
}

TEST(sdb,removeUsr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "removeUsr use \
in the cluser environment only" << endl ;
      return ;
   }
   rc = connection.createUsr( USERNAME, PASSWORD ) ;
   connection.disconnect() ;
   rc = connection.connect( pHostName, pPort, USERNAME, PASSWORD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.removeUsr( USERNAME, PASSWORD ) ;
   connection.disconnect() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CONTEXTS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CONTEXTS_CURRENT)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SESSIONS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SESSIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SESSIONS_CURRENT)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SESSIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_COLLECTIONS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_COLLECTIONSPACES)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_COLLECTIONSPACES ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_DATABASE)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SYSTEM)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SYSTEM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CATALOG)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "snapshot 'SDB_SNAP_CATALOG' use \
in the cluser environment only" << endl ;
      return ;
   }
   rc = connection.getSnapshot( cursor, SDB_SNAP_CATALOG ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_CONTEXTS)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_CONTEXTS_CURRENT)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SESSIONS)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_SESSIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SESSIONS_CURRENT)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_SESSIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_COLLECTIONS)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_COLLECTIONSPACES)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_COLLECTIONSPACES ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_STORAGEUNITS)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, SDB_LIST_STORAGEUNITS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_GROUPS)
{
   sdb connection ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "snapshot() 'SDB_SNAP_CATALOG' use \
in the cluser environment only" << endl ;
      return ;
   }
   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SDB_LIST_STOREPROCEDURES)
{
   sdb connection ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "getList() 'SDB_LIST_STOREPROCEDURES' use \
in the cluser environment only" << endl ;
      return ;
   }
   rc = connection.getList( cursor, SDB_LIST_STOREPROCEDURES ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getCollection)
{
   sdb connection ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,getCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,createCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollectionSpace cs1 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.createCollectionSpace( COLLECTION_SPACE_NAME,
                                          SDB_PAGESIZE_DEFAULT,
                                          cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,dropCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;
   connection.disconnect() ;
}

TEST(sdb,listCollectionSpaces)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.listCollectionSpaces( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( cursor ) ;
   connection.disconnect() ;
}

TEST(sdb,execUpdate)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *sql2 = "INSERT into testfoo.testbar ( name, age )  values( \"小明\",23 )" ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.execUpdate( sql2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR *sql1 = "select * from testfoo.testbar" ;
   rc = connection.exec( sql1, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   printf("The result are as below:\n") ;
   displayRecord( cursor ) ;
   printf("All the collection space have been list.\n") ;

   connection.disconnect() ;
}

TEST(sdb,exec)
{
   sdb connection ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   const CHAR *sql1 = "select * from testfoo.testbar" ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.exec( sql1, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   printf("The result are as below:\n") ;
   displayRecord( cursor ) ;
   printf("All the collection space have been list.\n") ;


   connection.disconnect() ;
}

TEST( sdb, flushConfigure )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const char* str = "{Global:true}" ;
   rc = fromjson( str, obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.flushConfigure ( obj ) ;
   cout<<"rc is "<<rc<<endl ;
   ASSERT_EQ( SDB_OK, rc ) ;

   connection.disconnect() ;
}

TEST( sdb, closeAllCursors )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   sdbCursor cursor2 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 num = 10 ;
   BSONObj obj ;
   BSONObj obj1 ;
   BSONObj obj2 ;
   BSONObj obj3 ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   insertRecords( cl, num ) ;
   rc = cl.query( cursor );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.current( obj );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "obj is: " << obj.toString() << endl ;
   rc = cursor1.next( obj1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "obj1 is: " << obj1.toString() << endl ;
   rc = cursor2.current( obj2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "obj2 is: " << obj2.toString() << endl ;
   rc = connection.closeAllCursors () ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.current( obj );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor.next( obj1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor1.next( obj2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor1.next( obj3 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   connection.disconnect() ;
}

TEST( sdb, SdbIsValid )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   sdbCursor cursor2 ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BOOLEAN result = FALSE ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "before close connection, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   result = TRUE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "after close connection manually, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   connection.disconnect() ;
   result = FALSE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "after close connection, result is " << result << endl ;
   ASSERT_TRUE( result == FALSE ) ;
}


/*************************************
  the follow tests have some problems
***************************************/
/*
TEST( sdb, setSessionAttr )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbShard shard ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj conf ;
   BSONObj obj ;
   const char* str = "{PreferedInstance:\"s\"}" ;
   rc = fromjson( str, conf ) ;
    ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = initEnv() ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = fromjson( "{a:1}", obj ) ;
   rc = cl.insert( obj ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.setSessionAttr( conf ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for ( int i = 0; i < 10; i++ )
   {
      rc = cl.query( cursor );
      CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   connection.disconnect() ;
}
*/

/*
TEST(sdb,resetSnapshot)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj condition ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( 0==1 ) ;
   rc = connection.resetSnapshot( condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   connection.disconnect() ;
}


TEST(sdb,transactionBegin)
{
   sdb connection ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   bson obj ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_TRUE( 0==1 ) ;
   connection.disconnect() ;
}

TEST(sdb,transactionCommit)
{
   sdb connection ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   bson obj ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_TRUE( 0==1 ) ;
   connection.disconnect() ;
}

TEST(sdb,transactionRollback)
{
   sdb connection ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   bson obj ;
   initEnv() ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_TRUE( 0==1 ) ;
   connection.disconnect() ;
}
*/


/*
resetSnapshot
createCollectionSpace // option
listCollections
listReplicaGroups
getReplicaGroup // name
getReplicaGroup // id
createReplicaGroup
removeReplicaGroup
createReplicaCataGroup
activateReplicaGroup
transactionBegin
transactionCommit
transactionRollback
flushConfigure
crtJSProcedure
rmProcedure
listProcedures
evalJS
backupOffline
listBackup
removeBackup
listTasks
waitTasks
cancelTask
setSessionAttr
listDomains

*/
