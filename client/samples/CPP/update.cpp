/******************************************************************************
 *
 * Name: update.cpp
 * Description: This program demostrates how to update the data in the database.
 *
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh update
 *    Win: buildApp.bat update
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux:
 *       g++ update.cpp common.cpp -o update -I../../include -O0 -ggdb \ 
 *       -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
 *    Win:
 *       cl /Foupdate.obj /c update.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
 *       link /OUT:update.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib update.obj common.obj /debug
 *       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
 *    Static Linking:
 *    Linux: g++ update.cpp common.cpp -o update.static -I../../include -O0
 *           -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./update <hostname> \
 *           <servicename> <username> <password>
 *    Win: update.exe <hostname> <servicename> <username> <password>
 *
 ******************************************************************************/
#include <iostream>
#include "common.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace sample ;

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"

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
   CHAR  *Port        = (CHAR*)argv[2] ;
   CHAR *pUsr         = (CHAR*)argv[3] ;
   CHAR *pPasswd      = (CHAR*)argv[4] ;

   // define a sdb object
   // use to connect to database
   sdb connection ;
   // define a sdbCollectionSpace object
   sdbCollectionSpace collectionspace ;
   // define a sdbCollection object
   sdbCollection collection ;
   // define a sdbCursor object for query
   sdbCursor cursor ;

   // define local variables
   // initialize them before use
   BSONObj obj ;
   BSONObj rule ;

   INT32 rc = SDB_OK ;

   // connect to database
   rc = connection.connect ( pHostName, Port, pUsr, pPasswd ) ;
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

   // insert records to the collection
   // insert a English record
   createEnglishRecord ( obj  ) ;
   rc = collection.insert ( obj ) ;
   if ( rc )
   {
      cout<<"Faileded to insert record, rc = "<<rc<<endl ;
   }

   // query the records
   // the result is in the cursor object
   rc = collection.query ( cursor ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to query, rc = "<<rc<<endl ;
      goto error ;
   }

   // update the record
   // let's set the rule and query condition first
   // here,we make the condition to be NULL
   // so all the records will be update
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"The update rule is: "<<endl ;
   cout << rule.toString() << endl ;
   rc = collection.update( rule ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failede to update the record, rc = "<<rc<<endl ;
      goto error ;
   }
   cout<<"Success to update!"<<endl ;

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

