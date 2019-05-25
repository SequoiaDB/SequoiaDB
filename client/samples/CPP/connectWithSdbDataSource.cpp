/******************************************************************************
*
* Name: connectWithSdbDataSource.cpp
* Description: This program demostrates how to connect to SequoiaDB database \
* with sdbDataSource
*
* 
* Auto Compile:
* Linux: ./buildApp.sh connectWithSdbDataSource
* Win: buildApp.bat connectWithSdbDataSource
* Manual Compile:
*    Dynamic Linking:
*    Linux:
*       g++ connectWithSdbDataSource.cpp common.cpp -o connectWithSdbDataSource \
*       -I../../include -O0 -ggdb -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
*    Win:
*       cl /FoconnectWithSdbDataSource.obj /c connectWithSdbDataSource.cpp \
*       /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       link /OUT:connectWithSdbDataSource.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib \
*            connectWithSdbDataSource.obj common.obj /debug
*       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
*    Static Linking:
*    Linux: g++ connectWithSdbDataSource.cpp common.cpp -o \
*    connectWithSdbDataSource.static -I../../include -O0 -ggdb -Wno-deprecated \
*    ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
* Run:
*    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./connectWithSdbDataSource 
*    Win: connectWithSdbDataSource.exe
*
******************************************************************************/

#include "common.hpp"
#include "sdbDataSourceComm.hpp"
#include "sdbDataSource.hpp"
#include <vector>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;
using namespace sample ;

void queryTask( sdbDataSource &ds )
{
   INT32                rc = SDB_OK ;
   // define a sdb object
   // use to connect to database
   sdb*                 connection ;
   // define a sdbCursor object for query
   sdbCursor            cursor ;

   // define local variables
   // initialize them before use
   BSONObj              obj ;

   // add a coord node to sdbDataSource
   ds.addCoord( "192.168.20.166:11810" ) ;
   
   // get a connection from sdbDataSource
   rc = ds.getConnection( connection ) ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to get a connection, rc = " << rc << endl ;
      goto error ;
   }
   // list replica groups
   rc = connection->listReplicaGroups( cursor ) ; 
   if ( SDB_OK != rc )
   {
      cout << "Fail to list replica groups, rc = " << rc << endl ;
      goto error ;
   }
   // sdbCursor move forward
   rc = cursor.next( obj ) ;
   if ( SDB_OK != rc )
   {
      cout << "sdbCursor fail to move forward, rc = " << rc << endl ;
      goto error ;
   }
   // output information
   cout << obj.toString() << endl ;
   // give back a connection to sdbDataSource
   ds.releaseConnection( connection ) ;
done:  
   return ;
error:  
   goto done ; 
}


INT32 main( INT32 argc, CHAR **argv )
{
   INT32                rc = SDB_OK ;
   sdbDataSourceConf    conf ;
   sdbDataSource        ds ;

   // set configure of sdbDataSource
   // userName="",passwd=""
   conf.setUserInfo( "", "" ) ;  
   // 10 connection is created on initialized, to create 10 connection when 
   // connection number is not enough, max idle connection number is 20, 
   // sdbDataSource provide 500 connection at most
   conf.setConnCntInfo( 10, 10, 20, 500 ) ;
   // Every 60 seconds, the connection which is beyond the alive time will be 
   // destroyed(0 means do not check the alive time), then the connection beyond
   // the max idle connection number will be destroyed .
   conf.setCheckIntervalInfo( 60, 0 ) ;
   // sync coord node information every 30s, 0 means not sync
   conf.setSyncCoordInterval( 30 ) ;
   // provide connection with balance strategy
   conf.setConnectStrategy( DS_STY_BALANCE ) ;
   // whether to check connection's validation when get a connection
   conf.setValidateConnection( TRUE ) ;
   // whether enable SSL
   conf.setUseSSL( FALSE ) ;
   // provide coord node information
   vector<string>        v ;
   v.push_back( "192.168.20.165:30000" );
   v.push_back( "192.168.20,165:50000" ) ;
   // init sdbDataSource
   rc = ds.init( v, conf ) ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to init sdbDataSouce, rc = " << rc << endl ;
      goto error ;
   }

   // enable sdbDataSource
   rc = ds.enable() ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to enable sdbDataSource, rc = " << rc << endl ;
      goto error ;
   }

   // run task in single or multi thread
   queryTask( ds ) ;

   // disable sdbDataSource
   rc = ds.disable() ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to disable sdbDataSource, rc = " << rc << endl ;
      goto error ;
   }

   // close sdbDataSource
   ds.close() ;
done:  
   return 0 ;
error:  
   goto done ;   
}

