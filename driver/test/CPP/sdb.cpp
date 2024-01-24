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
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,connect_with_serval_addr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
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
   // connect to database
   rc = connection.connect( connArr, 10, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getList( cursor, 5);
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}


TEST(sdb,disconnect)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BOOLEAN result                           = TRUE ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( FALSE, connection.isValid() ) ;
   ASSERT_EQ( TRUE, connection.isClosed() ) ;
   
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, connection.isValid() ) ;
   ASSERT_EQ( FALSE, connection.isClosed() ) ;
   // get cs , just test whether we connet to db or not
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
   ASSERT_EQ( FALSE, connection.isValid() ) ;
   ASSERT_EQ( TRUE, connection.isClosed() ) ;
   connection.disconnect() ;
   ASSERT_EQ( FALSE, connection.isValid() ) ;
   ASSERT_EQ( TRUE, connection.isClosed() ) ;
   // get cs , just test whether we connet to db or not
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   //ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
   rc = cs.dropCollection("aaaa") ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST(sdb,createUsr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "createUsr use \
in the cluser environment only" << endl ;
      return ;
   }
   // create user
   rc = connection.createUsr( USERNAME, PASSWORD ) ;
   // disconnect the connection
   connection.disconnect() ;
   // connect to database again using the new account
   rc = connection.connect( pHostName, pPort, USERNAME, PASSWORD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs , just test whether we connet to db or not
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // remove the user
   rc = connection.removeUsr( USERNAME, PASSWORD ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,removeUsr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "removeUsr use \
in the cluser environment only" << endl ;
      return ;
   }
   // create user
   rc = connection.createUsr( USERNAME, PASSWORD ) ;
   // disconnect the connection
   connection.disconnect() ;
   // connect to database again using the new account
   rc = connection.connect( pHostName, pPort, USERNAME, PASSWORD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs , just test whether we connet to db or not
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // remove the user
   rc = connection.removeUsr( USERNAME, PASSWORD ) ;
   // disconnect the connection
   connection.disconnect() ;
   // connect to database again, make sure we have removed the account
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs , just test whether we connet to db or not
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CONTEXTS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CONTEXTS_CURRENT)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SESSIONS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SESSIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SESSIONS_CURRENT)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SESSIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_COLLECTIONS)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_COLLECTIONSPACES)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_COLLECTIONSPACES ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_DATABASE)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_SYSTEM)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getSnapshot( cursor, SDB_SNAP_SYSTEM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getSnapshot_SDB_SNAP_CATALOG)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
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
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_CONTEXTS)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_CONTEXTS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_CONTEXTS_CURRENT)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_CONTEXTS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SESSIONS)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_SESSIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SESSIONS_CURRENT)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_SESSIONS_CURRENT ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_COLLECTIONS)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_COLLECTIONSPACES)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_COLLECTIONSPACES ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_STORAGEUNITS)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list
   rc = connection.getList( cursor, SDB_LIST_STORAGEUNITS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_GROUPS)
{
   sdb connection ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "snapshot() 'SDB_SNAP_CATALOG' use \
in the cluser environment only" << endl ;
      return ;
   }
   // list
   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getList_SDB_LIST_SDB_LIST_STOREPROCEDURES)
{
   sdb connection ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // test whether it is in the cluser environment
   rc = connection.getList( cursor1, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "getList() 'SDB_LIST_STOREPROCEDURES' use \
in the cluser environment only" << endl ;
      return ;
   }
   // list
   rc = connection.getList( cursor, SDB_LIST_STOREPROCEDURES ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display records
//   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getCollection)
{
   sdb connection ;
   sdbCollection cl_1 ;
   sdbCollection cl_2 ;
   sdbCollection cl_3 ;
   sdbCollection cl_4 ;
   sdbCollection cl_5 ;
   sdbCollection cl_6_1 ;
   sdbCollection cl_6_2 ;
   sdbCollection cl_7_1 ;
   sdbCollection cl_7_2 ;

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   sdbclient::sdbCursor cursor ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: get cl when checkExist is not filled in
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl_1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl_1.getDetail( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 2: get a existent cl when checkExist is true
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl_2, TRUE) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 3: get a existent cl when checkExist is false
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl_3, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl_3.getDetail( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 4: get a nonexistent cl when checkExist is true
   rc = connection.getCollection( NOT_EXIST_CL_FULL_NAME, cl_4, TRUE) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;

   // case 5: get a nonexistent cl when checkExist is false
   rc = connection.getCollection( NOT_EXIST_CL_FULL_NAME, cl_5, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl_5.getDetail( cursor ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 6 :
   // get a nonexistent cl when checkExist is false
   rc = connection.getCollection( NOT_EXIST_CL_FULL_NAME, cl_6_1, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the nonexistent cl when checkExist is true
   rc = connection.getCollection( NOT_EXIST_CL_FULL_NAME, cl_6_2, TRUE) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;

   // case 7 :
   // get a existent cl when checkExist is TRUE
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl_7_1, TRUE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the existent cl when checkExist is FALSE
   rc = connection.getCollection( COLLECTION_FULL_NAME, cl_7_2, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ(cl_7_1.getVersion(), cl_7_2.getVersion()) ;


   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,getCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs_1 ;
   sdbCollectionSpace cs_2 ;
   sdbCollectionSpace cs_3 ;
   sdbCollectionSpace cs_4 ;
   sdbCollectionSpace cs_5 ;
   sdbCollectionSpace cs_6_1 ;
   sdbCollectionSpace cs_6_2 ;
   sdbCollectionSpace cs_7_1 ;
   sdbCollectionSpace cs_7_2 ;

   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: get cs when checkExist is not filled in
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs_1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs_1.createCollection( NOT_EXIST_CL_NAME, cl ) ;
   ASSERT_NE( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 2: get a existent cs when checkExist is true
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs_2, TRUE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs_2.createCollection( NOT_EXIST_CL_NAME, cl ) ;
   ASSERT_NE( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 3: get a existent cs when checkExist is false
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs_3, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs_3.createCollection( NOT_EXIST_CL_NAME, cl ) ;
   ASSERT_NE( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 4: get a nonexistent cs when checkExist is true
   rc = connection.getCollectionSpace( NOT_EXIST_CS_NAME, cs_4, TRUE) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 5: get a nonexistent cs when checkExist is false
   rc = connection.getCollectionSpace( NOT_EXIST_CS_NAME, cs_5, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs_5.createCollection( NOT_EXIST_CL_NAME, cl ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;


   // case 6 :
   // get a nonexistent cs when checkExist is false
   rc = connection.getCollectionSpace( NOT_EXIST_CS_NAME, cs_6_1, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the nonexistent cs when checkExist is true
   rc = connection.getCollectionSpace( NOT_EXIST_CS_NAME, cs_6_2, TRUE) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;

   // case 7 :
   // get a existent cs when checkExist is TRUE
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs_7_1, TRUE) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the existent cs when checkExist is FALSE
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs_7_2, FALSE) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,createCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollectionSpace cs1 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the cs named "testfoo" first
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cs
   rc = connection.createCollectionSpace( COLLECTION_SPACE_NAME,
                                          SDB_PAGESIZE_DEFAULT,
                                          cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs, make sure it exist
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,dropCollectionSpace)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cs
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs, make sure it is no longer exist
   rc = connection.getCollectionSpace( COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,listCollectionSpaces)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cs
   rc = connection.listCollectionSpaces( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // display all the collection space
   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,execUpdate)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *sql2 = "INSERT into testfoo.testbar ( name, age )  values( \"小明\",23 )" ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.execUpdate( sql2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR *sql1 = "select * from testfoo.testbar" ;
   // connect to database
   rc = connection.exec( sql1, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   printf("The result are as below:\n") ;
   // display all the record
   displayRecord( cursor ) ;
   printf("All the collection space have been list.\n") ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(sdb,exec)
{
   sdb connection ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   const CHAR *sql1 = "select * from testfoo.testbar" ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.exec( sql1, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   printf("The result are as below:\n") ;
   // display all the record
   displayRecord( cursor ) ;
   printf("All the collection space have been list.\n") ;


   // disconnect the connection
   connection.disconnect() ;
}

TEST( sdb, flushConfigure )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   const char* str = "{Global:true}" ;
   rc = fromjson( str, obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.flushConfigure ( obj ) ;
   cout<<"rc is "<<rc<<endl ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
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
   // initialize local variables
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
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // prepare records
   insertRecords( cl, num ) ;
   // query
   rc = cl.query( cursor );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get current or next record
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
   // TODO:
   rc = connection.closeAllCursors () ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
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
   // disconnect the connection
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
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BOOLEAN result = FALSE ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   // TODO:
   // case 1
   // test before disconnect
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "before close connection, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   // case 2 ( manually )
   // test kill the server and let the connection disconnect
   result = TRUE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "after close connection manually, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   // case 3
   // test after disconnect
   connection.disconnect() ;
   result = FALSE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout<< "after close connection, result is " << result << endl ;
   ASSERT_TRUE( result == FALSE ) ;
}

TEST(sdb, getLastErrorObjTest)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl2 ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj result ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   rc = cs.getCollection( "aaaaa", cl2 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "after get cl failed, result is: " << result.toString(false, true) << endl ;

   connection.cleanLastErrorObj() ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   rc = cs.getCollection( "aaaaa", cl2 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;

   // disconnect the connection
   connection.disconnect() ;

   // case 2:
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   connection.cleanLastErrorObj() ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   rc = cs.getCollection( "aaaaa", cl2 ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   connection.cleanLastErrorObj() ;
   rc = connection.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

}

TEST(sdb, getAddressTest)
{
   INT32 rc          = SDB_OK ;
   string hostName   = HOST ;
   string port1      = SERVER ;
   string port2      = SERVER1 ;
   string port3      = port2 + "111" ;
   string expAddr1   = hostName + ":" + port1 ;
   string expAddr2   = hostName + ":" + port2 ;
   string userName   = "getAddressTestUser" ;
   string pwd        = "123";
   string errorPwd   = "111" ;
   sdb db ;
   sdb conn ;

   rc = conn.connect( hostName.c_str(), port1.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // 1: before connect
   string actAddr = db.getAddress() ;
   ASSERT_EQ( "", actAddr ) ;

   // 2: after connect
   rc = db.connect( hostName.c_str(), port1.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr1, db.getAddress() ) ;

   // 3. after disconnect
   db.disconnect() ;
   ASSERT_EQ( expAddr1, db.getAddress() ) ;

   // 4: connect again
   rc = db.connect( hostName.c_str(), port2.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( expAddr2, db.getAddress() ) ;
   db.disconnect() ;

   // 5. password error
   rc = conn.createUsr( userName.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( hostName.c_str(), port1.c_str(), userName.c_str(), errorPwd.c_str() ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) ;
   rc = conn.removeUsr( userName.c_str(), pwd.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( expAddr2, db.getAddress() ) ;

   // 6: connect fails
   rc = db.connect( hostName.c_str(), port3.c_str(), USER, PASSWD ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   ASSERT_EQ( expAddr2, db.getAddress() ) ;

   conn.disconnect() ;
}

TEST(sdb, createUserOutOfRange)
{
   sdb connection ;

   INT32 rc               = SDB_OK ;
   const CHAR *pHostName  = HOST ;
   const CHAR *pPort      = SERVER ;
   const CHAR *pUsr       = USER ;
   const CHAR *pPasswd    = PASSWD ;
   CHAR *outOfRange       = NULL ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // max size 256, use 257 + 1 for \0
   outOfRange = allocMemory( 258 ) ;
   memset(outOfRange, 'a', 257 ) ;
   rc = connection.createUsr( USERNAME, outOfRange ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc) ;
   rc = connection.createUsr( outOfRange, PASSWD ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   freeMemory( outOfRange ) ;
   connection.disconnect() ;
}

TEST(sdb, createUserWithMaxSize)
{
   sdb connection ;

   INT32 rc               = SDB_OK ;
   const CHAR *pHostName  = HOST ;
   const CHAR *pPort      = SERVER ;
   const CHAR *pUsr       = USER ;
   const CHAR *pPasswd    = PASSWD ;
   CHAR *maxCharSize      = NULL ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   maxCharSize = allocMemory( 257 ) ;
   memset(maxCharSize, 'a', 256 ) ;
   rc = connection.createUsr( maxCharSize, maxCharSize ) ;
   ASSERT_EQ( SDB_OK, rc) ;
   rc = connection.removeUsr( maxCharSize, maxCharSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   freeMemory( maxCharSize ) ;
   connection.disconnect() ;
}
