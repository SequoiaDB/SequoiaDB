/******************************************************************************
 *
 * Name: index.cpp
 * Description: This program demostrates how to create index.
 *
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh index
 *    Win: buildApp.bat index
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux:
 *       g++ index.cpp common.cpp -o index -I../../include -O0 -ggdb \
 *       -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
 *    Win:
 *       cl /Foindex.obj /c index.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 \ 
 *       /Z7 /TP
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 \ 
 *       /Z7 /TP
 *       link /OUT:index.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib index.obj common.obj \
 *       /build
 *       copy ..\..\lib\cpp\debug\dll\sdbcpp.dll .
 *    Static Linking:
 *    Linux: g++ index.cpp common.cpp -o index.static -I../../include -O0
 *           -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./index <hostname> \
 *           <servicename> <username> <password>
 *    Win: index.exe <hostname> <servicename> <username> <password>
 *
 ******************************************************************************/
#include <iostream>
#include "common.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace sample ;

#define COLLECTION_SPACE_NAME "foo1"
#define COLLECTION_NAME       "bar"
#define INDEX_NAME "index"

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
   CHAR *pHostName    = (CHAR*)argv[1] ;
   CHAR *pPort        = (CHAR*)argv[2] ;
   CHAR *pUsr         = (CHAR*)argv[3] ;
   CHAR *pPasswd      = (CHAR*)argv[4] ;

   // define a sdb object
   // use to connect to database
   sdb connection ;
   // define a sdbCollectionSpace object
   sdbCollectionSpace collectionspace ;
   // define a sdbCollection object
   sdbCollection collection ;
   // define a sdbCursor object for querying the index
   sdbCursor cursor ;


   // define local variables
   // initialize them before use
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   // connect to database
   rc = connection.connect ( pHostName, pPort, pUsr, pPasswd ) ;
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
   
   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = collectionspace.createCollection ( COLLECTION_NAME, collection ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create collection, rc = "<<rc<<endl ;
      goto error ;
   }

   // build a bson for index definition
   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   cout<<"The index to build is: "<<endl ;
   cout<< obj.toString() << endl ;
   rc = collection.createIndex ( obj, INDEX_NAME, FALSE, FALSE ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create index, rc = "<<rc<<endl ;
      goto error ;
   }
   cout<<"Success to build index!"<<endl ;
   // drop the specified collection
   rc = collectionspace.dropCollection( COLLECTION_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

   // drop the specified collection space
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

done:
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


