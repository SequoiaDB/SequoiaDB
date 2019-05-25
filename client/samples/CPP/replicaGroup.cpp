/******************************************************************************
 *
 * Name: replicaGroup.cpp
 * Description: This program demostrates how to connect to SequoiaDB database,
 *              list/get replicaGroup/node, and create new node.
 * Parameters:
 *              HostName: The hostname for database server
 *              ServiceName: The service name or port number for the database
 *                           service
 *              Username: The user name for database server
 *              Password: The password  for user
 * Auto Compile:
 *    Linux: ./buildApp.sh replicaGroup
 *    Win: buildApp.bat replicaGroup
 * Manual Compile:
 *    Dynamic Linking:
 *    Linux:
 *       g++ replicaGroup.cpp common.cpp -o replicaGroup -I../../include -O0 \ 
 *       -ggdb -Wno-deprecated -L../../lib -lsdbcpp -lm -ldl
 *    Win:
 *       cl /ForeplicaGroup.obj /c replicaGroup.cpp /I..\..\include /wd4047 \ 
 *       /Od /MDd /RTC1 /Z7 /TP
 *       cl /Focommon.obj /c common.cpp /I..\..\include /wd4047 /Od /MDd /RTC1 \
 *       /Z7 /TP
 *       link /OUT:replicaGroup.exe /LIBPATH:..\..\lib\cpp\debug\dll sdbcppd.lib \ 
 *       replicaGroup.obj common.obj /debug
 *       copy ..\..\lib\cpp\debug\dll\sdbcppd.dll .
 *    Static Linking:
 *    Linux: g++ replicaGroup.cpp common.cpp -o replicaGroup.static -I../../include -O0
 *           -ggdb -Wno-deprecated ../../lib/libstaticsdbcpp.a -lm -ldl -lpthread
 * Run:
 * Linux: LD_LIBRARY_PATH=<path for libsdbcpp.so> ./replicaGroup <hostname> \
 *        <servicename> <username> <password>
 * Win: replicaGroup.exe <hostname> <servicename> <username> <password>
 *
 ******************************************************************************/
#include <iostream>
#include <stdio.h>
#include "common.hpp"

using namespace sample ;

#define COLLECTION_NAME "SAMPLE.employee"
#define NUM_RECORD 100
#define INDEX_NAME "employee_id"
/* Display Syntax Error */
void displaySyntax ( CHAR *pCommand )
{
   printf ( "Syntax: %s <hostname> <servicename>\
 <username> <password>" OSS_NEWLINE, pCommand ) ;
}

INT32 addGroup ( sdb *connection )
{
   INT32 rc = SDB_OK ;
   CHAR newGroupName [ 128 ] = {0} ;
   sdbReplicaGroup rg ;
   printf ( "Please input the new group name: " ) ;
   scanf ( "%s", newGroupName ) ;
   rc = connection->createReplicaGroup ( newGroupName, rg ) ;
   if ( rc )
   {
      printf ( "Failed to create group, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

INT32 listGroups ( sdb *connection )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   INT32 count = 0 ;
   BSONObj obj ;
   rc = connection->listReplicaGroups ( cursor ) ;
   if ( rc )
   {
      printf ( "Failed to list replica groups, rc = %d"
               OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }

   while ( TRUE )
   {
      rc = cursor.next ( obj ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            printf ( "Failed to fetch next record from collection, rc = %d"
                     OSS_NEWLINE, rc ) ;
         }
         break ;
      }
      printf ( "replica group [ %d ]: " OSS_NEWLINE, count ) ;
      cout << obj.toString() << endl ;
      ++ count ;
   }
   rc = SDB_OK ;
done :
   return rc ;
error :
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   /* initialize local variables */
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUser                       = NULL ;
   CHAR *pPasswd                     = NULL ;
   BSONObj detail ;
   sdb connection ;
   sdbReplicaGroup rg ;
   sdbNode masternode ;
   sdbNode slavenode ;
   INT32 nodeNum = 0 ;
   INT32 rc                          = 0 ;
   /* verify syntax */
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }

   /* read argument */
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUser        = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   /* connect to database */
   rc = connectTo ( pHostName, pServiceName,
                    pUser, pPasswd, connection ) ;
   if ( rc )
   {
      printf ( "Failed to connect to database at %s:%s, rc = %d"
               OSS_NEWLINE,
               pHostName, pServiceName, rc ) ;
      exit ( 0 ) ;
   }

   rc = listGroups ( &connection ) ;
   if ( rc )
   {
      printf ( "Failed to list replica groups, rc = %d"
               OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }
   // get catalog rg by name
   rc = connection.getReplicaGroup ( "SYSCatalogGroup", rg ) ;
   if ( rc )
   {
      printf ( "Failed to get catalog group, rc = %d" OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }
   // find how many nodes in the set
   rc = rg.getNodeNum ( SDB_NODE_ALL, &nodeNum ) ;
   if ( rc )
   {
      printf ( "Failed to get number of node, rc = %d" OSS_NEWLINE,
               rc ) ;
      exit ( 0 ) ;
   }
   printf ( "There are totally %d nodes in the set" OSS_NEWLINE,
            nodeNum ) ;
   // get detailed information about the set
   rc = rg.getDetail ( detail ) ;
   if ( rc )
   {
      printf ( "Failed to get details, rc = %d" OSS_NEWLINE,
               rc ) ;
      exit ( 0 ) ;
   }
   cout << detail.toString() << endl ;
   // get the master node
   rc = rg.getMaster ( masternode ) ;
   if ( rc )
   {
      printf ( "Failed to get masternode, rc = %d" OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }
   printf ( "master host name: %s" OSS_NEWLINE,
            masternode.getHostName () ) ;
   printf ( "master service name: %s" OSS_NEWLINE,
            masternode.getServiceName () ) ;
   // get one of slave
   rc = rg.getSlave ( slavenode ) ;
   if ( rc )
   {
      printf ( "Failed to get slavenode, rc = %d" OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }
   printf ( "slave host name: %s" OSS_NEWLINE,
            slavenode.getHostName () ) ;
   printf ( "slave service name: %s" OSS_NEWLINE,
            slavenode.getServiceName () ) ;

   // add a new group
   rc = addGroup ( &connection ) ;
   if ( rc )
   {
      printf ( "Failed to add group, rc = %d" OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }

   // list groups again
   rc = listGroups ( &connection ) ;
   if ( rc )
   {
      printf ( "Failed to list replica groups, rc = %d"
               OSS_NEWLINE, rc ) ;
      exit ( 0 ) ;
   }
   /* disconnect from server */
   connection.disconnect() ;
   return 0 ;
}
