/******************************************************************************
*
* Name: connectWithSdbConnectionPool.cpp
* Description: This program demostrates how to connect to SequoiaDB database \
* with sdbConnectionPool
*
* 
* Auto Compile:
* Linux: ./buildApp.sh connectWithSdbConnectionPool
* Win: buildApp.bat connectWithSdbConnectionPool
* Manual Compile:
*    Dynamic Linking:
*    Linux:
*       if GCC version >= 5.1
*          g++ connectWithSdbConnectionPool.cpp common.cpp -o connectWithSdbConnectionPool \
*          -I../../include -O0 -ggdb -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl -D_GLIBCXX_USE_CXX11_ABI=0
*       if GCC version < 5.1
*          g++ connectWithSdbConnectionPool.cpp common.cpp -o connectWithSdbConnectionPool \
*          -I../../include -O0 -ggdb -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
*    Win:
*       cl /FoconnectWithSdbConnectionPool.obj /c connectWithSdbConnectionPool.cpp \
*       /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 /Z7 /TP
*       link /OUT:connectWithSdbConnectionPool.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib \
*            connectWithSdbConnectionPool.obj common.obj /debug
*       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
*    Static Linking:
*    Linux: 
*       if GCC version >= 5.1
*          g++ connectWithSdbConnectionPool.cpp common.cpp -o \
*          connectWithSdbConnectionPool.static -I../../include -O0 -ggdb -Wno-deprecated \
*          ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread -D_GLIBCXX_USE_CXX11_ABI=0
*       if GCC version < 5.1
*          g++ connectWithSdbConnectionPool.cpp common.cpp -o \
*          connectWithSdbConnectionPool.static -I../../include -O0 -ggdb -Wno-deprecated \
*          ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
* Run:
*    Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./connectWithSdbConnectionPool 
*    Win: connectWithSdbConnectionPool.exe
*
******************************************************************************/

#include "common.hpp"
#include "sdbConnectionPoolComm.hpp"
#include "sdbConnectionPool.hpp"
#include <vector>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;
using namespace sample ;

void queryTask( sdbConnectionPool &connPool )
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
   
   // get a connection from sdbConnectionPool
   rc = connPool.getConnection( connection ) ;
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
   // give back a connection to sdbConnectionPool
   connPool.releaseConnection( connection ) ;
done:  
   return ;
error:  
   goto done ; 
}


INT32 main( INT32 argc, CHAR **argv )
{
   INT32                    rc = SDB_OK ;
   sdbConnectionPoolConf    conf ;
   sdbConnectionPool            connPool ;

   // set configure of sdbConnectionPool
   // userName="",passwd=""
   conf.setAuthInfo( "", "" ) ;
   // 10 connection is created on initialized, to create 10 connection when 
   // connection number is not enough, max idle connection number is 20, 
   // sdbConnectionPool provide 500 connection at most
   conf.setConnCntInfo( 10, 10, 20, 500 ) ;
   // Every 60 seconds, the connection which is beyond the alive time will be 
   // destroyed(0 means do not check the alive time), then the connection beyond
   // the max idle connection number will be destroyed .
   conf.setCheckIntervalInfo( 60, 0 ) ;
   // sync coord node information every 30s, 0 means not sync
   conf.setSyncCoordInterval( 30 ) ;
   // provide connection with serial strategy
   conf.setConnectStrategy( SDB_CONN_STY_SERIAL ) ;
   // whether to check connection's validation when get a connection
   conf.setValidateConnection( TRUE ) ;
   // whether enable SSL
   conf.setUseSSL( FALSE ) ;
   // provide coord node information
   vector<string>        v ;
   v.push_back( "server2:30000" );
   v.push_back( "server2:50000" ) ;
   // init sdbConnectionPool
   rc = connPool.init( v, conf ) ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to init sdbDataSouce, rc = " << rc << endl ;
      goto error ;
   }

   // run task in single or multi thread
   queryTask( connPool ) ;

   // close sdbConnectionPool
   rc = connPool.close() ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to close sdbConnectionPool, rc = " << rc << endl ;
      goto error ;
   }
done:  
   return 0 ;
error:  
   goto done ;   
}

