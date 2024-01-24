/******************************************************************************
 *
 * Name: lob.cpp
 * Description: This program demostrates how to large object in the database.
 *
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh lob
 *    Win: buildApp.bat lob
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux:
 *       if GCC version >= 5.1
 *          g++ lob.cpp common.cpp -o lob -I../../include -O0 -ggdb -Wno-deprecated \
 *          -L../../lib -lsdbcpp -lm -ldl -D_GLIBCXX_USE_CXX11_ABI=0 
 *       if GCC version < 5.1
 *          g++ lob.cpp common.cpp -o lob -I../../include -O0 -ggdb -Wno-deprecated \
 *          -L../../lib -lsdbcpp -lm -ldl
 *    Win:
 *       cl /Foupdate.obj /c lob.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 \
 *       /Z7 /TP
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 \
 *       /Z7 /TP
 *       link /OUT:lob.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib lob.obj common.obj /debug
 *       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
 *    Static Linking:
 *    Linux: 
 *       if GCC version >= 5.1
            g++ lob.cpp common.cpp -o lob.static -I../../include -O0 \
 *          -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread -D_GLIBCXX_USE_CXX11_ABI=0 
 *       if GCC version < 5.1
 *          g++ lob.cpp common.cpp -o lob.static -I../../include -O0 \
 *          -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 *    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./lob <hostname> \
 *           <servicename> <username> <password>
 *    Win: lob.exe <hostname> <servicename> <username> <password>
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

   // define objects
   sdb                db ;
   sdbCollectionSpace cs ;
   sdbCollection      cl ;
   sdbLob             lob ;
   bson::OID          oid ;

   // define local variables
   const UINT32 bufSize = 1000 ;
   CHAR buf[bufSize] = { 'a' } ;
   BOOLEAN flag = FALSE ;

   INT32 rc = SDB_OK ;

   // connect to database
   rc = db.connect ( pHostName, Port, pUsr, pPasswd ) ;
   if( rc != SDB_OK )
   {
      cout << "Failed to connet to database, rc = " << rc << endl ;
      goto error ;
   }

   // create collection space
   rc = db.createCollectionSpace ( COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, cs ) ;
   if( rc != SDB_OK )
   {
      cout << "Failed to create collection space, rc = " << rc << endl ;
      goto error ;
   }

   // create collection in a specified colletion space
   rc = cs.createCollection ( COLLECTION_NAME, cl ) ;
   if( rc != SDB_OK )
   {
      cout << "Failed to create collection, rc = " << rc << endl ;
      goto error ;
   }

   // create large object for writing
   rc = cl.createLob( lob ) ;
   if ( rc != SDB_OK )
   {
      cout << "Failed to create large object, rc = " << rc << endl ;
      goto error ;
   }

   // get large object's oid
   rc = lob.getOid( oid ) ;
   if ( rc != SDB_OK )
   {
      cout << "Failed to get large object's oid, rc = " << rc << endl ;
      goto error ;
   }

   // write some data to the newly create large object
   rc = lob.write( buf, bufSize ) ;
   if ( rc != SDB_OK )
   {
      cout << "Failed to write to large object, rc = " << rc << endl ;
      goto error ;
   }

   // close the large object( when create the large object, don't forget to 
   // close it to release resource )
   rc = lob.close() ;
   if ( rc != SDB_OK )
   {
      cout << "Failed to close large object, rc = " << rc << endl ;
      goto error ;
   }
   
   cout<<"Success to create a large object and write something to it!"<<endl ;

   // drop the collection
   rc = cs.dropCollection( COLLECTION_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

   // drop the collection space
   rc = db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

done:
   // disconnect from database
   db.disconnect () ;
   return 0;
error:
   goto done ;
}

// Display Syntax Error
void displaySyntax ( CHAR *pCommand )
{
   cout << "Syntax:" << pCommand << " <hostname> <servicename>"
       "<username> <password>" << endl ;
}


