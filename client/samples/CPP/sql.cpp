/******************************************************************************
 *
 * Name: sql.cpp
 * Description: This program demostrates how to use the sql interface
 *              sdb::exec() and sdb::execUpdate() to work.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh sdb
 *    Win: buildApp.bat sdb
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux: 
 *       if GCC version >= 5.1
 *          g++ -c -std=c99 -o sql.o -I../../include sql.cpp
 *          g++ -c -std=c++0x -o common.o -I../../include common.cpp
 *          g++ sql.cpp common.cpp -o sql -I../../include \
 *          -L../../lib -lsdbcpp -D_GLIBCXX_USE_CXX11_ABI=0
 *       if GCC version < 5.1
 *          g++ -c -std=c99 -o sql.o -I../../include sql.cpp
 *          g++ -c -std=c++0x -o common.o -I../../include common.cpp
 *          g++ sql.cpp common.cpp -o sql -I../../include \
 *          -L../../lib -lsdbcpp
 *    Win:
 *       cl /Fosql.obj /c sql.cpp /I..\..\include /wd4047
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047
 *       link /OUT:sql.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib sql.obj common.obj
 *       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
 *    Static Linking:
 *    Linux: 
 *       if GCC version >= 5.1
 *          g++ sql.cpp common.cpp -o sql.static -I../../include -O0 \
 *          -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread -D_GLIBCXX_USE_CXX11_ABI=0
 *       if GCC version < 5.1
 *          g++ sql.cpp common.cpp -o sql.static -I../../include -O0 \
 *          -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./insert <hostname> \
 *           <servicename> <username> <password>
 *    Win: insert.exe <hostname> <servicename> <username> <password>
 *
 ******************************************************************************/

#include <iostream>
#include "common.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace sample ;

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define NUM 3

const CHAR *sql1 = "select * from foo.bar" ;
const CHAR *sql2 = "insert into foo.bar ( name, age ) values( \"Aimee\",23 )" ;

// Display Syntax Error
void displaySyntax ( CHAR *pCommand ) ;

INT32 main ( INT32 argc, CHAR **argv )
{
   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }
   // initialize local variables
   CHAR *pHostName           = (CHAR*)argv[1] ;
   CHAR *pServiceName        = (CHAR*)argv[2] ;
   CHAR *pUsr                = (CHAR*)argv[3] ;
   CHAR *pPasswd             = (CHAR*)argv[4] ;

   // define a sdb object
   // use to connect to database
   sdb connection ;
   // define a sdbCollectionSpace object
   sdbCollectionSpace collectionspace ;
   // define a sdbCollection object
   sdbCollection collection ;
   // define a cursor object
   sdbCursor cursor ;

   // define local variables
   // initialize them before use
   BSONObj obj ;
   INT32 rc = SDB_OK ;
   INT32 i = 0 ;

   // connect to database
   rc = connection.connect ( pHostName, pServiceName, pUsr, pPasswd ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to connet to database, rc = "<<rc<<endl ;
      goto error ;
   }

   // create collection space
   rc = connection.createCollectionSpace ( COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, collectionspace ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create collection space, rc = "<<rc<<endl ;
      goto error ;
   }

   // create collection
   rc = collectionspace.createCollection ( COLLECTION_NAME, collection ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create collection, rc = "<<rc<<endl ;
      goto error ;
   }
   
   // load some data
   for( i = 0; i < NUM; i++ )
   {
      obj = BSON ( "name" << "tom" << "age" << 24 ) ;
      // then,insert to the specified collection
      rc = collection.insert ( obj ) ;
      if ( rc!=SDB_OK )
      {
         cout<<"Failed to insert record, rc = "<<rc<<endl ;
         goto error ;
      }
   }

   // execute sql qurey
   rc = connection.exec( sql1, cursor ) ;
   if ( rc )
   {
      cout<<"Failed to execute, rc = "<<rc<<endl ;
      goto error ;
   }
   // display all the record
   cout<<"The result are as below: "<<endl ;
   while( !( rc = cursor.next( obj ) ) )
   {
      cout << obj.toString() << endl ;
   }

   // execute sql insert
   cout<<"The operation is :"<<endl ;
   cout<<sql2<<endl ;
   rc = connection.execUpdate ( sql2 ) ;
   if ( rc )
   {
      cout<<"Failed to execute, rc = "<<rc<<endl ;
      goto error ;
   }
   // display all the record
   rc = collection.query ( cursor  ) ;
   if( rc != SDB_OK )
   {
      cout<<"Failed to query all the records, rc = "<<endl ;
      goto error ;
   }
   cout<<"The result are as below: "<<endl ;
   while( !( rc =cursor.next( obj ) ) )
   {
      cout << obj.toString() << endl ;
   }

done:
   // drop the specified collection space
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection space,rc = "<<rc<<endl ;
   }
   // disconnect from database
   connection.disconnect () ;
   return 0;
error:
   goto done ;
}

// Display Syntax Error
void displaySyntax ( CHAR *pCommand )
{
   cout<<"Syntax:"<<pCommand<<" <hostname> <servicename>\
 <username> <password>"<<endl ;
}






