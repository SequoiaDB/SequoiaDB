/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "ossSocket.hpp"
#include "msgMessage.hpp"
#include "msgCatalog.hpp"
#include "msg.h"
#include "msgDef.h"
#include "../bson/bson.h"
#include "../util/fromjson.hpp"
#include "ossEDU.hpp"
#include "utilLinenoiseWrapper.hpp"
#include "clsCatalogAgent.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace bson ;
using namespace std ;
using namespace boost ;

bool  dumpHex              = false ;
bool  quit                 = false ;
bool  interruptFlag        = false ;
ossSocket *sock            = NULL ;
BSONObj dummyObj ;

#define DEFAULT_BUFFER_SIZE 1024

char *pOutBuffer           = NULL ;
int outBufferSize          = 0 ;
char *pInBuffer            = NULL ;
int inBufferSize           = 0 ;
INT64 requestID            = 0 ;
INT32 g_Version            = 1 ;
INT32 g_W                  = 1 ;
INT32 g_tid                = -1 ;
BOOLEAN g_auth             = TRUE ;
ossSpinXLatch _mutex ;

vector<BSONObj> resultList ;
vector<BSONObj>::iterator it ;
long long contextID        = -1 ;
int replyFlag              = -1 ;
int numReturned            = -1 ;
int startFrom              = -1 ;

ofstream outputFile ;
#define OUTPUTNONE 0
#define OUTPUTREPLACE 1
#define OUTPUTAPPEND 2
#define OUTPUTDONE 3

int outputMode = OUTPUTNONE ;

#define RECEIVE_BUFFER_SIZE 4095
#define MAX_CS_SIZE         127
#define QUIT_COMMAND       "quit"
#define HELP_COMMAND       "help"
#define CONNECT_COMMAND    "connect"
#define INSERT_COMMAND     "put"
#define UPDATE_COMMAND     "update"
#define UPSERT_COMMAND     "upsert"
#define QUERY_COMMAND      "find"
#define DELETE_COMMAND     "delete"
#define ADMIN_COMMAND      CMD_ADMIN_PREFIX
#define ALTER_CS_COMMAND   "use"
#define ENSUREIDX_COMMAND  "ensureIndex"
#define GETIDX_COMMAND     "getIndexes"
#define DROPIDX_COMMAND    "dropIndex"
#define COUNT_COMMAND      "count"
#define RENAME_COMMAND     "rename"
#define MSG_COMMAND        "msg"
#define OFFLINEREORG_COMMAND "offlinereorg"
#define REORGRECOVER_COMMAND "reorgrecover"
#define PARTITION_COMMAND  "partition"

#define TABSPACE           "    "

char receiveBuffer [ RECEIVE_BUFFER_SIZE+1 ] ;
char defaultCS [ MAX_CS_SIZE + 1 ] ;

INT32 msgSend ( BOOLEAN incRequestID = TRUE ) ;

INT32 buildAuthMsg( CHAR **ppBuffer, INT32 *bufferSize, const CHAR *pUsrName,
                    const CHAR *pPasswd, UINT64 reqID )
{
   INT32 rc = SDB_OK ;
   INT32 msgLen = 0 ;
   BSONObj obj ;
   INT32 bsonSize = 0 ;
   MsgAuthentication *msg ;

   if ( NULL == pUsrName || NULL == pPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   obj = BSON( SDB_AUTH_USER << pUsrName << SDB_AUTH_PASSWD << pPasswd ) ;
   bsonSize = obj.objsize() ;
   msgLen = sizeof( MsgAuthentication ) + ossRoundUpToMultipleX( bsonSize, 4 ) ;
   if ( outBufferSize < msgLen )
   {
      SDB_OSS_FREE( pOutBuffer ) ;
      pOutBuffer = (CHAR *)SDB_OSS_MALLOC( msgLen ) ;
      outBufferSize = msgLen ;
   }

   msg                       = ( MsgAuthentication * )(*ppBuffer) ;
   msg->header.requestID     = reqID ;
   msg->header.opCode        = MSG_AUTH_VERIFY_REQ ;
   msg->header.messageLength = sizeof( MsgAuthentication ) + bsonSize ;
   msg->header.routeID.value = 0 ;
   msg->header.TID           = ossGetCurrentThreadID() ;
   ossMemcpy( *ppBuffer + sizeof(MsgAuthentication),
              obj.objdata(), bsonSize ) ;
done:
   return rc ;
error:
   goto done ;
}

BOOLEAN readLine ( const char* prompt, char* p, int length )
{
   BOOLEAN ret = TRUE ;
   char *readstr = NULL ;

   if ( quit )
   {
      return FALSE ;
   }

   ret = getNextCommand( prompt, &readstr ) ;
   if ( readstr )
   {
      strncpy( p, readstr, length -1 ) ;
      p[ length-1 ] = 0 ;

      SDB_OSS_FREE( readstr ) ;
      readstr = NULL ;
   }
   else
   {
      p[0] = 0 ;
   }

   return ret ;
}

string promptStr( INT32 numIndent, const char *prompt )
{
   string promptStr ;
   for ( INT32 i = 0; i< numIndent ; i++ )
   {
      promptStr += "    " ;
   }
   promptStr += prompt ;
   promptStr += "> " ;

   return promptStr ;
}

INT32 readInput ( CHAR *pPrompt, INT32 numIndent )
{
   memset ( receiveBuffer, 0, sizeof( receiveBuffer ) ) ;

   string strPrompt = promptStr( numIndent, pPrompt ) ;
   string strPrompt1 = promptStr( numIndent, "" ) ;

   if ( !readLine ( strPrompt.c_str(), receiveBuffer, sizeof(receiveBuffer) ) )
   {
      printf( " disconnect and quit\n" ) ;
      quit = true ;
      return SDB_APP_FORCED ;
   }

   while ( receiveBuffer[ strlen(receiveBuffer)-1 ] == '\\' &&
           RECEIVE_BUFFER_SIZE - strlen( receiveBuffer ) > 0 )
   {
      if ( !readLine ( strPrompt1.c_str(),
                       &receiveBuffer[ strlen(receiveBuffer)-1 ],
                       RECEIVE_BUFFER_SIZE - strlen( receiveBuffer ) ) )
      {
         memset( receiveBuffer, 0, sizeof( receiveBuffer ) ) ;
         printf(" disconnect and quit\n" ) ;
         quit = true ;
         return SDB_APP_FORCED ;
      }
   }

   if ( RECEIVE_BUFFER_SIZE == strlen( receiveBuffer ) )
   {
      printf ( "-Error: Max input length is %d bytes\n", RECEIVE_BUFFER_SIZE ) ;
      return SDB_INVALIDARG ;
   }

   return SDB_OK ;
}

INT32 msgSend ( BOOLEAN incRequestID )
{
   static UINT32 tid = ossGetCurrentThreadID() ;
   INT32 sentLen = 0 ;
   MsgHeader *header = (MsgHeader*)pOutBuffer ;

   INT32 rc = SDB_OK ;
   if ( !sock )
   {
      if ( false == quit )
      {
         printf ( "-Error: Connection does not exist\n" ) ;
      }
      rc = SDB_SYS ;
      goto error ;
   }

   if ( g_tid == -1 )
   {
      header->TID = tid ;
   }
   else
   {
      header->TID = ( UINT32 )g_tid ;
   }
   header->routeID.value = 0 ;

   if ( incRequestID )
   {
      ++requestID ;
   }
   header->requestID = (UINT64)requestID ;

   rc = sock->send ( pOutBuffer, (*(SINT32*)pOutBuffer),
                     sentLen ) ;
   if ( rc )
   {
      printf ( "-Error: Failed to send data, rc: %d\n", rc ) ;
      goto error ;
   }
   interruptFlag = false ;

done:
   return rc ;
error :
   goto done ;
}

INT32 msgReceive ()
{
   INT32 rc = SDB_OK ;
   MsgHeader *header = NULL ;
   INT32 length = 0 ;
   INT32 receivedLen = 0 ;
   INT32 totalReceivedLen = 0 ;
   if ( !sock )
   {
      printf ( "-Error: Connection does not exist\n" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while ( !quit )
   {
      rc = sock->recv ( &pInBuffer[ totalReceivedLen ],
                        sizeof(int)-totalReceivedLen,
                        receivedLen ) ;
      totalReceivedLen += receivedLen ;
      if ( rc == SDB_TIMEOUT )
      {
         continue ;
      }
      if ( rc == SDB_NETWORK_CLOSE )
      {
         printf ( "-Error: Remote connection is closed\n" ) ;
         delete ( sock ) ;
         sock = NULL ;
         goto error ;
      }
      if ( rc )
      {
         printf("-Error: Invalid bytes received, rc: %d\n", rc ) ;
         goto error ;
      }
      break ;
   }

   length = *(int*)( pInBuffer ) ;

   if ( inBufferSize < length+1 )
   {
      pInBuffer = (char*)realloc ( pInBuffer, sizeof(char)*length+1 ) ;
      if ( !pInBuffer )
      {
         printf ( "-Error: Failed to allocate buffer, size: %d\n",
                  (INT32)(sizeof(char)*length+1) ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      inBufferSize = length ;
   }

   receivedLen = 0 ;
   totalReceivedLen = 0 ;
   while ( !quit )
   {
      rc = sock->recv ( &pInBuffer[ sizeof(int)+totalReceivedLen ],
                        length-sizeof(int)-totalReceivedLen,
                        receivedLen ) ;
      totalReceivedLen += receivedLen ;
      if ( rc == SDB_TIMEOUT )
      {
         continue ;
      }
      if ( rc )
      {
         printf ("-Error: Failed to receive data, rc: %d\n", rc ) ;
         goto error ;
      }
      header = (MsgHeader*)pInBuffer ;
      if ( header->requestID != (UINT64)requestID )
      {
         printf ( "-Warning: Reply request id is not the same, expect:%lld, "
                  "recieved:%lld\n", requestID, header->requestID ) ;
      }
      break ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 msgReceiveSysInfoRes ()
{
   INT32 rc = SDB_OK ;
   INT32 length = sizeof( MsgSysInfoReply ) ;
   INT32 receivedLen = 0 ;
   INT32 totalReceivedLen = 0 ;
   if ( !sock )
   {
      printf ( "-Error: Connection does not exist\n" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while ( !quit )
   {
      rc = sock->recv ( &pInBuffer[ totalReceivedLen ],
                        length-totalReceivedLen,
                        receivedLen ) ;
      totalReceivedLen += receivedLen ;
      if ( rc == SDB_TIMEOUT )
      {
         continue ;
      }
      if ( rc == SDB_NETWORK_CLOSE )
      {
         printf ( "-Error: Remote connection is closed\n" ) ;
         delete ( sock ) ;
         sock = NULL ;
         goto error ;
      }
      if ( rc )
      {
         printf("-Error: Invalid bytes received, rc: %d\n", rc ) ;
         goto error ;
      }
      break ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 msgReceiveExtract ( BOOLEAN *querySuccess )
{
   INT32 rc = SDB_OK ;
   rc = msgReceive () ;
   if ( rc )
   {
      goto error ;
   }

   resultList.clear() ;
   rc = msgExtractReply ( pInBuffer, &replyFlag, &contextID,
                          &startFrom, &numReturned, resultList ) ;

   if ( replyFlag != SDB_OK )
   {
      if ( SDB_DMS_EOC == replyFlag )
      {
         if ( querySuccess )
         {
            *querySuccess = FALSE ;
         }
         goto done ;
      }

      printf(" Execution Failure: %d\n", replyFlag ) ;

      if ( querySuccess )
      {
         *querySuccess = FALSE ;
      }
   }
   else
   {
      printf(" Execution Successful\n") ;
      if ( querySuccess )
      {
         *querySuccess = TRUE ;
      }
   }

   for ( it = resultList.begin() ; it < resultList.end() ; it++ )
   {
      printf( "    " ) ;
      std::cout << (*it).toString() << endl ;
   }

done:
   return rc ;
error :
   goto done ;
}

INT32 readResultSet ()
{
   INT32 rc = SDB_OK ;
   UINT64 rowCount = 0 ;
   if ( -1 == contextID )
   {
      goto done ;
   }

   while ( !interruptFlag )
   {
      rc = msgBuildGetMoreMsg ( &pOutBuffer, &outBufferSize, -1, contextID,0 ) ;
      if ( rc )
      {
         printf ("-Error: Failed to build getmore message, rc: %d\n", rc ) ;
         goto error ;
      }

      rc = msgSend () ;
      if ( rc )
      {
         break ;
      }

      rc = msgReceive () ;
      if ( rc )
      {
         goto error ;
      }

      resultList.clear() ;
      rc = msgExtractReply ( pInBuffer, &replyFlag, &contextID,
                             &startFrom, &numReturned, resultList ) ;

      if ( replyFlag != SDB_OK )
      {
         if ( SDB_DMS_EOC == replyFlag )
         {
            printf ( " Totally %lld records received\n", rowCount ) ;
            goto done ;
         }

         printf("-Error: GetMore Failure: %d\n", replyFlag ) ;

         it = resultList.begin() ;
         if ( it != resultList.end() )
         {
            printf("    ") ;
            std::cout << (*it).toString() << endl ;
         }
         goto error ;
      }

      for ( it = resultList.begin() ; it < resultList.end() ; it++ )
      {
         if ( outputMode == OUTPUTDONE )
         {
            outputFile << "    " ;
            outputFile << (*it).toString()<<endl ;
         }
         else
         {
            std::cout << "    " ;
            std::cout << (*it).toString()<<endl ;
         }
         rowCount ++ ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

void displayHelp ( vector<string> subHelp )
{
   if ( subHelp.size() == 0 )
   {
      printf ( "    %s\n    %s\n    %s\n    %s\n    %s\n    %s\n    %s\n"
               "    %s\n    %s\n    %s\n    %s\n    %s\n    %s\n    %s\n"
               "    %s\n    %s\n    %s\n    %s\n    %s\n",
               QUIT_COMMAND,
               HELP_COMMAND,
               CONNECT_COMMAND,
               INSERT_COMMAND,
               UPDATE_COMMAND,
               UPSERT_COMMAND,
               QUERY_COMMAND,
               DELETE_COMMAND,
               ENSUREIDX_COMMAND,
               GETIDX_COMMAND,
               DROPIDX_COMMAND,
               COUNT_COMMAND,
               OFFLINEREORG_COMMAND,
               REORGRECOVER_COMMAND,
               RENAME_COMMAND,
               ADMIN_COMMAND,
               ALTER_CS_COMMAND,
               MSG_COMMAND,
               PARTITION_COMMAND ) ;
      return ;
   }

   vector<string>::const_iterator it ;
   for ( it = subHelp.begin() ; it != subHelp.end() ; it++ )
   {
      if ( QUIT_COMMAND == (*it) )
      {
         printf ( "%s%s: quit the program\n", TABSPACE, QUIT_COMMAND ) ;
      }
      else if ( HELP_COMMAND == (*it) )
      {
         printf ( "%s%s [suboptions]...: display help menu\n", TABSPACE,
                  HELP_COMMAND ) ;
      }
      else if ( CONNECT_COMMAND == (*it) )
      {
         printf ( "%s%s <hostname> <port> [username] [password]: "
                  "connect to a given server\n",
                  TABSPACE, CONNECT_COMMAND ) ;
      }
      else if ( INSERT_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: insert a record to "
                  "given collection\n", TABSPACE, INSERT_COMMAND ) ;
      }
      else if ( UPDATE_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: update records from "
                  "the given <collection>\n", TABSPACE, UPDATE_COMMAND ) ;
      }
      else if ( UPSERT_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: update records from "
                  "the given <collection>, if no record has been updated, it "
                  "will insert a new record\n", TABSPACE, UPSERT_COMMAND ) ;
      }
      else if ( QUERY_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: query records from "
                  "the given <collection>\n", TABSPACE, QUERY_COMMAND ) ;
      }
      else if ( DELETE_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: delete records from "
                  "the given <collection>\n", TABSPACE, DELETE_COMMAND ) ;
      }
      else if ( ENSUREIDX_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: create index for "
                  "the given <collection>\n", TABSPACE, ENSUREIDX_COMMAND ) ;
      }
      else if ( GETIDX_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: get indexes for "
                  "the given <collection>\n", TABSPACE, GETIDX_COMMAND ) ;
      }
      else if ( DROPIDX_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: drop index for the "
                  "given <collection>\n", TABSPACE, DROPIDX_COMMAND ) ;
      }
      else if ( OFFLINEREORG_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: perform offline "
                  "reorg\n", TABSPACE, OFFLINEREORG_COMMAND ) ;
      }
      else if ( REORGRECOVER_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: perform offline "
                  "reorg\n", TABSPACE, REORGRECOVER_COMMAND ) ;
      }
      else if ( RENAME_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: rename the name "
                  "of collection\n", TABSPACE, RENAME_COMMAND ) ;
      }
      else if ( COUNT_COMMAND == (*it) )
      {
         printf ( "%s[<collectionspace>.]<collection>.%s: count the number "
                  "of records in a given <collection>\n", TABSPACE,
                  COUNT_COMMAND ) ;
      }
      else if ( MSG_COMMAND == (*it) )
      {
         printf ( "%sBroad cast messages\n", TABSPACE ) ;
      }
      else if ( ADMIN_COMMAND == (*it) )
      {
         printf ( "%s%s{list {contexts [current]|sessions [current]|collections"
                  "|collectionspaces|storageunits|groups|nodes}}\n",
                  TABSPACE, ADMIN_COMMAND ) ;
         printf ( "%s%s{snapshot {contexts [current]|sessions [current]|database"
                  "|system|reset|collections|collectionspaces}}\n",
                  TABSPACE, ADMIN_COMMAND ) ;
         printf ( "%s%s{create {collection <collection name>|collectionspace "
                  "<collectionspace name>|group <group name>}}\n",
                  TABSPACE, ADMIN_COMMAND ) ;
         printf ( "%s%s{drop {collection <collection name>|collectionspace "
                  "<collectionspace name>|group <groupname>}}\n",
                  TABSPACE, ADMIN_COMMAND ) ;
         printf ( "%s%s{get datablocks < collection name >}\n", TABSPACE,
                  ADMIN_COMMAND ) ;
         printf ( "%s%s{set {pdlevel [0~5]|localver <version>|localw <w>|"
                  "tid [tid]|auth [0/1]}}\n",
                  TABSPACE, ADMIN_COMMAND ) ;
         printf ( "%s%s shutdown|backup offline [To path]|command\n", TABSPACE,
                  ADMIN_COMMAND ) ;

         printf ( "%s  list contexts: list all contexts in the database\n",
                  TABSPACE ) ;
         printf ( "%s  list contexts current: list contexts for current "
                  "session\n", TABSPACE ) ;
         printf ( "%s  list sessions: list all connections in the database\n",
                  TABSPACE) ;
         printf ( "%s  list sessions current: list the current connection\n",
                  TABSPACE) ;
         printf ( "%s  list collections: list all collections\n", TABSPACE ) ;
         printf ( "%s  list collectionspaces: list all collectionspaces\n",
                  TABSPACE) ;
         printf ( "%s  list storageunits: list all storage units\n", TABSPACE ) ;
         printf ( "%s  list groups: list all groups\n", TABSPACE );
         printf ( "%s  list nodes: list all nodes\n", TABSPACE );
         printf ( "%s  create collection <collection name>: create a new "
                  "collection\n", TABSPACE ) ;
         printf ( "%s  create collectionspace <cs name>: create a new "
                  "collection space\n", TABSPACE ) ;
         printf ( "%s  create group <group name>: create a new group\n",
                  TABSPACE );
         printf ( "%s  drop collection <collection name>: drop a collection\n",
                  TABSPACE) ;
         printf ( "%s  drop collectionspace <cs name>: drop a collection "
                  "space\n", TABSPACE ) ;
         printf ( "%s  drop group <group name>: drop a group\n", TABSPACE );
         printf ( "%s  backup offline [To path] : backup database\n", TABSPACE ) ;
         printf ( "%s  shutdown: shutdown database engine\n", TABSPACE ) ;
         printf ( "%s  command: default commands\n", TABSPACE ) ;
      }
      else if ( ALTER_CS_COMMAND == (*it) )
      {
         printf ( "%s%s <collection space name>: change default collection "
                  "space\n", TABSPACE, ALTER_CS_COMMAND ) ;
      }
      else if ( PARTITION_COMMAND == (*it) )
      {
         printf( "%s%s : calc the key object hash partition value\n",
                 TABSPACE, PARTITION_COMMAND ) ;
      }
      else
      {
         printf ( "-Error: Unknown option for %s\n", (*it).c_str() ) ;
      }
   }
}

INT32 admin ( string Command, vector<string> arg )
{
   INT32 rc = SDB_OK ;
   string command = "" ;
   INT32 argCount = arg.size() ;
   BSONObj selector ;
   BSONObj matcher ;
   BSONObj orderby ;
   BSONObj hint ;
   INT32 opcode = MSG_BS_QUERY_REQ ;

   command = string("$")+Command ;
   vector<string>::const_iterator vit ;

   BOOLEAN querySuccess = FALSE ;
   INT32 flags = 0 ;

   if ( "create" == Command || "drop" == Command )
   {
      int pagesize = 4096 ;
      int pagesizes[] = { 4096, 8192, 16384, 32768, 65536 } ;
      if ( arg.size() != 2 || ( "collection" != arg[0]
         && "collectionspace" != arg[0] && "group" != arg[0] ) )
      {
         printf ( "-Error: Unexpected format for %s command\n",
                  Command.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      argCount = 1 ;

      if ( "create"==Command && arg[0] == "collectionspace" )
      {
         rc = readInput ( "page size ( 4096/8192/16384/32768/65536 )", 1 ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( receiveBuffer[0] != 0 )
         {
            for ( unsigned int i=0; i<sizeof(pagesizes)/sizeof(int); i++ )
            {
               if ( atoi ( receiveBuffer ) == pagesizes[i] )
               {
                  pagesize = pagesizes[i] ;
               }
            }
         }
         matcher = BSON( FIELD_NAME_NAME << arg[1] <<
                         FIELD_NAME_PAGE_SIZE << pagesize ) ;
      }
      else
      {
         matcher = BSON( FIELD_NAME_NAME << arg[1] ) ;
      }
   }
   else if ( "get" == Command )
   {
      if ( arg.size() >= 1 && "datablocks" == arg[0] )
      {
         if ( arg.size() != 2 )
         {
            printf( "-Error: Unexpected format for %s command\n",
                    Command.c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         argCount = 1 ;
         hint = BSON( FIELD_NAME_COLLECTION << arg[1] ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
   else if ( "list" == Command || "snapshot" == Command )
   {
   }
   else if ( "shutdown" == Command )
   {
   }
   else if ( "set" == Command )
   {
      if ( arg.size() >= 1 && "pdlevel" == arg[0] )
      {
         INT32 pdLevel = 3 ;
         if ( arg.size() >= 2 )
         {
            argCount = 1 ;
            pdLevel = atoi( arg[1].c_str() ) ;
         }
         else
         {
            rc = readInput ( "pdlevel[0~5]", 1 ) ;
            if ( rc )
            {
               goto error ;
            }
            pdLevel = atoi ( receiveBuffer ) ;
         }
         matcher = BSON( FIELD_NAME_PDLEVEL << pdLevel ) ;
      }
      else if ( arg.size() >= 1 && "localver" == arg[0] )
      {
         if ( arg.size() < 2 )
         {
            printf("-Error: command %s need version\n", "set localver" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         g_Version = atoi( arg[1].c_str() ) ;
         printf(" change local version to %d\n", g_Version ) ;
         goto done ;
      }
      else if ( arg.size() >= 1 && "localw" == arg[0] )
      {
         if ( arg.size() < 2 )
         {
            printf("-Error: command %s need w\n", "set localw" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         g_W = atoi( arg[1].c_str() ) ;
         printf(" change lcoal w to %d\n", g_W ) ;
         goto done ;
      }
      else if ( arg.size() >= 1 && "tid" == arg[ 0 ] )
      {
         if ( arg.size() < 2 )
         {
            g_tid = -1 ;
            printf(" change local tid to %d\n", ossGetCurrentThreadID() ) ;
         }
         else
         {
            g_tid = atoi( arg[1].c_str() ) ;
            printf(" change local tid to %d\n", g_tid ) ;
         }
         goto done ;
      }
      else if ( arg.size() >= 1 && "auth" == arg[ 0 ] )
      {
         if ( arg.size() < 2 )
         {
            g_auth = TRUE ;
         }
         else
         {
            g_auth = atoi( arg[1].c_str() ) == 0 ? FALSE : TRUE ;
         }
         if ( g_auth )
         {
            printf(" enabled auth\n" ) ;
         }
         else
         {
            printf(" disabled auth\n" ) ;
         }
         goto done ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
#if defined (_DEBUG)
   else if ( "debug" == Command )
   {
      if ( arg.size() >= 1 && "query" == arg[0] )
      {
         rc = readInput ( "AccessPlan", 1 ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( 0 != strlen ( receiveBuffer ) )
         {
            if ( SDB_OK != fromjson ( receiveBuffer, matcher ) )
            {
               printf ( "-Error: Invalid plan record\n" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
   }
#endif
   else if ( "backup" == Command )
   {
      argCount = 1 ;
      if ( arg.size() < 1 )
      {
         printf ( "-Error: Unexpected format for %s command\n",
                  Command.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      vector<string>::const_iterator vit = arg.begin () ;
      if ( ++vit != arg.end() )
      {
         if ( string( FIELD_NAME_TO ) != string(*vit ) )
         {
            printf ( "-Error: Unexpected argument %s is received by $backup\n",
                     (string(*vit)).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( ++vit == arg.end() )
         {
            printf ("-Error: Expect path after \"To\" clause\n" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         matcher = BSON( FIELD_NAME_TO << string(*vit) ) ;
      }
   }
   else if ( "command" == Command )
   {
      argCount = 0 ;
      rc = readInput( "command name", 1 ) ;
      if ( rc || 0 == strlen( receiveBuffer ) )
      {
         goto error ;
      }
      command = receiveBuffer ;
      /*rc = readInput( "need get result[0/1]", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) && atoi( receiveBuffer ) == 1 )
      {
         receiveResultSet = TRUE ;
      }*/
      rc = readInput("opCode[empty use default query]", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( ( opcode = atoi( receiveBuffer ) ) <= 0 )
      {
         opcode = MSG_BS_QUERY_REQ ;
      }
      rc = readInput("flags", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) )
      {
         flags = atoi( receiveBuffer ) ;
      }
      else
      {
         flags = 0 ;
      }
      rc = readInput( "matcher", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) )
      {
         if ( SDB_OK != fromjson( receiveBuffer, matcher ) )
         {
            printf( "-Error: Invalid matcher\n" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      rc = readInput( "selector", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) )
      {
         if ( SDB_OK != fromjson( receiveBuffer, selector ) )
         {
            printf("-Error: Invalid selector\n" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      rc = readInput( "orderby", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) )
      {
         if ( SDB_OK != fromjson( receiveBuffer, orderby ) )
         {
            printf("-Error: Invalid orderby\n" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      rc = readInput( "hint", 1 ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( 0 != strlen( receiveBuffer ) )
      {
         if ( SDB_OK != fromjson( receiveBuffer, hint ) )
         {
            printf("-Error: Invalid hint\n" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
   }

   if ( rc )
   {
      printf ( "-Error: Unexpected admin command: %s\n", Command.c_str() ) ;
      goto error ;
   }

   for ( vit = arg.begin(); vit != arg.end(); vit++ )
   {
      if ( argCount-- > 0 )
      {
         command += string(" ")+(*vit) ;
      }
      else
      {
         break ;
      }
   }

   rc = msgBuildQueryMsg( &pOutBuffer, &outBufferSize, command.c_str(),
                          flags, 0, 0, -1,
                          &matcher,
                          &selector,
                          &orderby,
                          &hint ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build query message, rc: %d\n", rc ) ;
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
      if ( MSG_BS_QUERY_REQ != opcode )
      {
         queryMsg->header.opCode = opcode ;
      }
   }

   rc = msgSend() ;
   if ( rc )
   {
      goto error ;
   }

   rc = msgReceiveExtract ( &querySuccess ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( -1 != contextID && querySuccess )
   {
      rc = readResultSet () ;
      if ( rc )
      {
         goto error ;
      }
   }

done :
   contextID = -1 ;
   return rc ;
error :
   goto done ;
}

INT32 connect ( vector<string> arg )
{
   INT32 rc = 0 ;
   int port = 0 ;
   const char *pHost = NULL ;
   BOOLEAN querySuccess = FALSE ;
   string username = "" ;
   string password = "" ;

   if ( arg.size() < 2 || arg.size() > 4 )
   {
      printf ( "-Error: Invalid input for connect\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   pHost = arg[0].c_str() ;
   port = atoi(arg[1].c_str()) ;
   if ( arg.size() > 2 )
   {
      username = arg[2] ;
   }
   if ( arg.size() > 3 )
   {
      password = arg[3] ;
   }
   printf ( " Connecting to %s port %d\n", pHost, port ) ;
   if ( sock )
   {
      printf ( " Disconnect from existing connection\n" ) ;
      delete ( sock ) ;
      sock = NULL ;
   }
   sock = new ossSocket( pHost, port ) ;
   if ( !sock )
   {
      printf ( "-Error: Failed to new socket\n" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   rc = sock->initSocket () ;
   if ( rc )
   {
      printf ( "-Error: Failed to init Socket\n" ) ;
      goto error ;
   }
   rc = sock->connect () ;
   if ( rc )
   {
      printf ( "-Error: Failed to connect Socket \n" ) ;
      goto error ;
   }
   sock->disableNagle();

   {
      INT32 sentLen = 0 ;
      rc = msgBuildSysInfoRequest( &pOutBuffer, &outBufferSize ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = sock->send ( pOutBuffer, sizeof( MsgSysInfoRequest ),
                        sentLen ) ;
      if ( rc )
      {
         printf ( "-Error: Failed to send data, rc: %d\n", rc ) ;
         goto error ;
      }

      rc = msgReceiveSysInfoRes() ;
      if ( rc )
      {
         goto error ;
      }
   }

   if ( g_auth )
   {
      rc = buildAuthMsg( &pOutBuffer, &outBufferSize,
                          username.c_str(), password.c_str(), requestID ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = msgSend ( FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = msgReceiveExtract ( &querySuccess ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( FALSE == querySuccess )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   printf (" Successfully connected to server\n" ) ;

done :
   return rc ;
error :
   if ( sock )
   {
      delete sock ;
      sock = NULL ;
   }
   goto done ;
}

INT32 alter_cs ( vector<string> arg )
{
   INT32 rc = SDB_OK ;
   if ( arg.size() != 1 )
   {
      printf ( "-Error: Invalid input for alter cs\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( strlen ( arg[0].c_str() ) > MAX_CS_SIZE )
   {
      printf ( "-Error: Collectionspace length is maximum %d bytes\n",
               MAX_CS_SIZE ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   memset ( defaultCS, 0, sizeof(defaultCS) ) ;
   strcpy ( defaultCS, arg[0].c_str() ) ;
   printf ( " Default collectionspace is set to %s\n", defaultCS ) ;

done :
   return rc ;
error :
   goto done ;
}

#define COLLECTION_SPACE_MAX_SZ 127
#define COLLECTION_MAX_SZ 127

INT32 indexG ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   BOOLEAN querySuccess = FALSE ;
   std::string command ;
   std::string collectionname ;
   BSONObj obj ;
   if ( !collectionSpace || !collection )
   {
      printf ( "-Error: Unexpected format for index get\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   command = string(CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES) ;
   collectionname = string(collectionSpace)+string( ".")+string(collection) ;
   obj = BSON( FIELD_NAME_COLLECTION << collectionname ) ;
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &dummyObj, // matcher
                           &dummyObj, // selector
                           &dummyObj, // orderBy
                           &obj       // object
                           ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build query message, rc: %d\n", rc );
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &querySuccess ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( querySuccess )
   {
      rc = readResultSet () ;
      if ( rc )
      {
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 offlineR ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command = CMD_ADMIN_PREFIX CMD_NAME_REORG_OFFLINE ;
   char buffer[512] = {0} ;
   memset ( buffer, 0, sizeof(buffer) ) ;
   sprintf ( buffer, "%s.%s", collectionSpace, collection ) ;
   BOOLEAN successResult = FALSE ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   BSONObj hint ;
   BSONObjBuilder hintob ;
   rc = readInput ( "cluster index", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   ob.append ( FIELD_NAME_COLLECTION, buffer ) ;
   obj = ob.obj() ;

   if ( 0 != strlen ( receiveBuffer ) )
   {
      try
      {
         hintob.append ( "", receiveBuffer ) ;
         hint = hintob.obj () ;
      }
      catch (...)
      {
         printf ("-Error: Invalid hint record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      hintob.appendNull ( "" ) ;
      hint = hintob.obj () ;
   }
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &obj,
                           NULL, NULL, &hint ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build reorg message, rc: %d\n", rc );
      goto error ;
   }
   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &successResult ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 reorgR ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command = CMD_ADMIN_PREFIX CMD_NAME_REORG_RECOVER ;
   char buffer[512] = {0} ;
   memset ( buffer, 0, sizeof(buffer) ) ;
   sprintf ( buffer, "%s.%s", collectionSpace, collection ) ;
   BOOLEAN successResult = FALSE ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   ob.append ( FIELD_NAME_COLLECTION, buffer ) ;
   obj = ob.obj() ;
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &obj ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build reorg message, rc: %d\n", rc ) ;
      goto error ;
   }
   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &successResult ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 renameC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command = CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTION ;
   BOOLEAN successResult = FALSE ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   rc = readInput ( "new name", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      try
      {
         ob.append ( FIELD_NAME_COLLECTIONSPACE, collectionSpace ) ;
         ob.append ( FIELD_NAME_OLDNAME, collection ) ;
         ob.append ( FIELD_NAME_NEWNAME, receiveBuffer ) ;
         obj = ob.obj () ;
      }
      catch (...)
      {
         printf ("-Error: Invalid matcher record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      printf ( "-Error: new name can't be empty\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &obj ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build rename message, rc: %d\n", rc );
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &successResult ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 countC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command ;
   BSONObj matcher ;
   BOOLEAN successResult = FALSE ;
   BSONObj collectionObj, dummyObj ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   command = string(CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT) ;
   rc = readInput ( "matcherPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, matcher ) )
      {
         printf ( "-Error: Invalid matcher record\n" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   collectionObj = BSON (FIELD_NAME_COLLECTION<<collectionFullName) ;
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &matcher,
                           &dummyObj,
                           &dummyObj,
                           &collectionObj ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build countC message, rc: %d\n", rc );
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &successResult ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( successResult )
   {
      rc = readResultSet () ;
      if ( rc )
      {
         goto error ;
      }
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 indexC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command ;
   BSONObj indexDef ;
   BSONObj createObj, dummyObj ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "index definition", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( SDB_OK != fromjson ( receiveBuffer, indexDef ) )
   {
      printf ( "-Error: Invalid matcher record\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   command = string(CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX) ;
   createObj = BSON (FIELD_NAME_COLLECTION<<collectionFullName<<
                     FIELD_NAME_INDEX<<indexDef) ;
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &createObj,
                           &dummyObj ) ; // NULL/NULL for orderBy and hint
   if ( rc )
   {
      printf ("-Error: Failed to build ensureIndex message, rc: %d\n", rc ) ;
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 indexD ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   string command ;
   BSONObj indexDef ;
   BSONObj dropObj, dummyObj ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "index name or ID", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( SDB_OK != fromjson ( receiveBuffer, indexDef ) )
   {
      printf ( "-Error: Invalid index record\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   command = string(CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX) ;
   dropObj = BSON (FIELD_NAME_COLLECTION<<collectionFullName<<
                   FIELD_NAME_INDEX<<indexDef) ;
   rc = msgBuildQueryMsg ( &pOutBuffer,
                           &outBufferSize,
                           command.c_str(),
                           0, // flag
                           0, // request id
                           -1,
                           -1,
                           &dropObj,
                           &dummyObj ) ; // NULL/NULL for orderBy and hint
   if ( rc )
   {
      printf ("-Error: Failed to build ensureIndex message, rc: %d\n", rc );
      goto error ;
   }
   else
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )pOutBuffer ;
      queryMsg->version = g_Version ;
      queryMsg->w = g_W ;
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 insertC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   BSONObj insertor ;
   MsgOpInsert *pInsert = NULL ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "insertRecord", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( SDB_OK != fromjson ( receiveBuffer, insertor ) )
   {
      printf ( "-Error: Invalid insert record\n" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = msgBuildInsertMsg ( &pOutBuffer,
                            &outBufferSize,
                            collectionFullName,
                            0, // flag
                            0, // request id
                            &insertor ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build insert message, rc: %d\n", rc );
      goto error ;
   }
   pInsert = ( MsgOpInsert*)pOutBuffer ;
   pInsert->version = g_Version ;
   pInsert->w = g_W ;

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 updateC ( const CHAR *collectionSpace, const CHAR *collection,
                INT32 flag )
{
   INT32 rc = SDB_OK ;
   BSONObj modifier ;
   BSONObj matcher ;
   BSONObj hint ;
   MsgOpUpdate *pUpdate = NULL ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "modifierPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( SDB_OK != fromjson ( receiveBuffer, modifier ) )
   {
      printf ("-Error: Invalid modifier record\n") ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = readInput ( "matcherPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, matcher ) )
      {
         printf ("-Error: Invalid matcher record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }

   rc = readInput ( "hintPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, hint ) )
      {
         printf ("-Error: Invalid hint record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }

   rc = msgBuildUpdateMsg ( &pOutBuffer,
                            &outBufferSize,
                            collectionFullName,
                            flag, // flag
                            0, // request id
                            &matcher,
                            &modifier,
                            &hint ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build update message, rc: %d\n", rc );
      goto error ;
   }
   pUpdate = (MsgOpUpdate*)pOutBuffer ;
   pUpdate->version = g_Version ;
   pUpdate->w = g_W ;

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 deleteC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   BSONObj matcher ;
   BSONObj hint ;
   MsgOpDelete *pDelete = NULL ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "matcherPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, matcher ) )
      {
         printf ("-Error: Invalid matcher record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   rc = readInput ( "hintPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, hint ) )
      {
         printf ("-Error: Invalid hint record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }
   rc = msgBuildDeleteMsg ( &pOutBuffer,
                            &outBufferSize,
                            collectionFullName,
                            0, // flag
                            0, // request id
                            &matcher,
                            &hint ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build delete message, rc: %d\n", rc );
      goto error ;
   }
   pDelete = (MsgOpDelete*)pOutBuffer ;
   pDelete->version = g_Version ;
   pDelete->w = g_W ;

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 queryC ( const CHAR *collectionSpace, const CHAR *collection )
{
   INT32 rc = SDB_OK ;
   BSONObj matcher ;
   BSONObj selector ;
   BSONObj orderBy ;
   BSONObj hint ;
   BOOLEAN successResult ;
   CHAR collectionFullName [ COLLECTION_SPACE_MAX_SZ+COLLECTION_MAX_SZ+2 ];
   memset ( collectionFullName, 0, sizeof(collectionFullName)) ;
   strncpy ( collectionFullName, collectionSpace, COLLECTION_SPACE_MAX_SZ ) ;
   strncat ( collectionFullName, ".", 1 ) ;
   strncat ( collectionFullName, collection, COLLECTION_MAX_SZ ) ;
   rc = readInput ( "matcherPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, matcher ) )
      {
         printf ("-Error: Invalid matcher record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   rc = readInput ( "selectorPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, selector ) )
      {
         printf ("-Error: Invalid selector record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }

   rc = readInput ( "orderByPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, orderBy ) )
      {
         printf ("-Error: Invalid orderBy record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }

   rc = readInput ( "hintPattern", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, hint ) )
      {
         printf ("-Error: Invalid hint record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;

      }
   }
   rc = msgBuildQueryMsg ( &pOutBuffer,
                            &outBufferSize,
                            collectionFullName,
                            0, // flag
                            0, // request id
                            0, // skip rows
                            -1, // total select
                            &matcher,
                            &selector,
                            &orderBy,
                            &hint ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build query message, rc: %d\n", rc );
      goto error ;
   }
   {
      MsgOpQuery *query = (MsgOpQuery*)pOutBuffer ;
      query->version = g_Version ;      
   }

   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( &successResult ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( successResult )
   {
      rc = readResultSet () ;
      if ( rc )
      {
         goto error ;
      }
   }
   else
   {
      printf ( " no found recond\n" ) ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 calcPartition()
{
   INT32 rc = SDB_OK ;
   BSONObj keyObj ;
   UINT32 partitionNum = 4096 ;
   UINT32 squre = 12 ;
   INT32 partition = 0 ;

   rc = readInput("key object", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( 0 != strlen ( receiveBuffer ) )
   {
      if ( SDB_OK != fromjson ( receiveBuffer, keyObj ) )
      {
         printf ("-Error: Invalid matcher record\n") ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   rc = readInput("partition number", 1 ) ;
   if ( rc )
   {
      goto error ;
   }
   partitionNum = atoi( receiveBuffer ) ;
   if ( !ossIsPowerOf2( partitionNum, &squre ) )
   {
      printf("-Error: partition number must be the power of 2\n") ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   partition = engine::clsPartition( keyObj, squre, 2 ) ;
   printf(" partition: %d\n", partition ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 parseIUQD ( string collectionCommand )
{
   INT32 rc = SDB_OK ;

   char_separator<char> sep(".") ;
   tokenizer< char_separator<char> > tokens ( collectionCommand, sep ) ;
   int count = 0 ;
   string collectionSpace ;
   string collection ;
   string command ;

   BOOST_FOREACH ( const string& t, tokens )
   {
      if ( 0 == count )
      {
         collectionSpace = t ;
      }
      else if ( 1==count )
      {
         collection = t ;
      }
      else if ( 2==count )
      {
         command = t ;
      }
      else
      {
         printf ( "-Error: command is not expected\n" ) ;
         return SDB_INVALIDARG ;
      }
      count++ ;
   }
   if ( count <= 1 )
   {
      printf ( "-Error: command is not expected\n" ) ;
      return SDB_INVALIDARG ;
   }
   if ( count == 2 )
   {
      if ( strlen ( defaultCS ) == 0 )
      {
         printf ("-Error: default collection space is required\n") ;
         return SDB_INVALIDARG ;
      }
      command = collection ;
      collection = collectionSpace ;
      collectionSpace = string(defaultCS) ;
   }

   if ( command == INSERT_COMMAND )
   {
      rc = insertC ( collectionSpace.c_str(), collection.c_str() );
      if ( rc )
      {
         printf ("-Error: Failed to insert into %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == UPDATE_COMMAND )
   {
      rc = updateC ( collectionSpace.c_str(), collection.c_str(), 0);
      if ( rc )
      {
         printf ("-Error: Failed to update into %s.%s\n",
                  collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == UPSERT_COMMAND )
   {
      rc = updateC ( collectionSpace.c_str(), collection.c_str(),
                     FLG_UPDATE_UPSERT );
      if ( rc )
      {
         printf ("-Error: Failed to upsert into %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == QUERY_COMMAND )
   {
      rc = queryC ( collectionSpace.c_str(), collection.c_str() );
      if ( rc )
      {
         printf ("-Error: Failed to query from %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == DELETE_COMMAND )
   {
      rc = deleteC ( collectionSpace.c_str(), collection.c_str() );
      if ( rc )
      {
         printf ("-Error: Failed to delete from %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == ENSUREIDX_COMMAND )
   {
      rc = indexC ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ("-Error: Failed to create index in %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == GETIDX_COMMAND )
   {
      rc = indexG ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ("-Error: Failed to get index in %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == DROPIDX_COMMAND )
   {
      rc = indexD ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ("-Error: Failed to drop index in %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == RENAME_COMMAND )
   {
      rc = renameC ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ("-Error: Failed to rename collection in %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == OFFLINEREORG_COMMAND )
   {
      rc = offlineR ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ( "-Error: Failed to perform offline reorg in %s.%s\n",
                  collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == REORGRECOVER_COMMAND )
   {
      rc = reorgR ( collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ( "-Error: Failed to perform reorg recover in %s.%s\n",
                  collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else if ( command == COUNT_COMMAND )
   {
      rc = countC (  collectionSpace.c_str(), collection.c_str() ) ;
      if ( rc )
      {
         printf ("-Error: Failed to count number of records in %s.%s\n",
                 collectionSpace.c_str(), collection.c_str() ) ;
         goto error ;
      }
   }
   else
   {
      printf ("-Error: Invalid command\n") ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 sendDisconnect ()
{
   if ( outBufferSize < (INT32)sizeof(MsgHeader) )
   {
      pOutBuffer = (CHAR *)SDB_OSS_REALLOC( pOutBuffer, sizeof(MsgHeader) ) ;
      if ( pOutBuffer )
      {
         outBufferSize = sizeof(MsgHeader);
      }
   }

   if ( !pOutBuffer )
   {
      printf ( "-Error: Alloc memory failed \n" ) ;
      return SDB_OOM ;
   }

   MsgHeader *header = (MsgHeader*)pOutBuffer ;
   header->messageLength = sizeof(MsgHeader) ;
   header->opCode = MSG_BS_DISCONNECT ;

   return msgSend () ;
}

INT32 interrupt ()
{
   if ( outBufferSize < (INT32)sizeof(MsgHeader) )
   {
      pOutBuffer = (CHAR *)SDB_OSS_REALLOC( pOutBuffer, sizeof(MsgHeader) ) ;
      if ( pOutBuffer )
      {
         outBufferSize = sizeof(MsgHeader);
      }
   }

   if ( !pOutBuffer )
   {
      printf ( "-Error: Alloc memory failed \n" ) ;
      return SDB_OOM ;
   }

   MsgHeader *header = (MsgHeader*)pOutBuffer ;
   header->messageLength = sizeof(MsgHeader) ;
   header->opCode = MSG_BS_INTERRUPTE ;

   INT32 rc = msgSend ( FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   interruptFlag = true ;

done:
   return rc ;
error:
   goto done ;
}

INT32 msg ()
{
   INT32 rc = 0 ;
   rc = readInput ( "message to send", 1 ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( 0 == strlen ( receiveBuffer ) )
   {
      goto done ;
   }
   rc = msgBuildMsgMsg ( &pOutBuffer,
                         &outBufferSize,
                         0,
                         receiveBuffer ) ;
   if ( rc )
   {
      printf ("-Error: Failed to build msg message, rc: %d\n", rc );
      goto error ;
   }
   rc = msgSend () ;
   if ( rc )
   {
      goto error ;
   }
   rc = msgReceiveExtract ( NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

void receivePrompt ()
{
   INT32 rc = SDB_OK ;
   if ( outputFile.is_open() )
   {
      outputFile.close() ;
   }
   outputMode = OUTPUTNONE ;
   rc = readInput ( "sdb", 0 ) ;
   if ( rc )
   {
      return ;
   }
   else if ( 0 == strlen( receiveBuffer ) )
   {
      return ;
   }

   string textInput ( receiveBuffer ) ;

   char_separator<char> sep(" \t") ;
   tokenizer< char_separator<char> > tokens ( textInput, sep ) ;
   int count = 0 ;
   string command ;
   vector<string> options ;
   BOOST_FOREACH ( const string& t, tokens )
   {
      if ( 0 == count )
      {
         command = t ;
         count++ ;
      }
      else if ( t[0] == '>' )
      {
         if ( t.size() > 1 && t[1] == '>' )
         {
            outputMode = OUTPUTAPPEND ;
            if ( t.size() > 2 )
            {
               outputFile.open(t.substr(2).c_str(), ios::out|ios::app) ;
               if ( !outputFile.is_open() )
               {
                  printf ("-Error: Failed to open file %s\n",
                          t.substr(2).c_str()) ;
               }
               else
                  outputMode = OUTPUTDONE ;
            }
         }
         else
         {
            outputMode = OUTPUTREPLACE ;
            if ( t.size() > 1 )
            {
               outputFile.open(t.substr(2).c_str(), ios::out|ios::trunc) ;
               if ( !outputFile.is_open() )
               {
                  printf ("-Error: Failed to open file %s\n",
                          t.substr(1).c_str()) ;
               }
               else
                  outputMode = OUTPUTDONE ;
            }
         }
      }
      else if ( outputMode != OUTPUTNONE )
      {
         if ( outputMode == OUTPUTDONE )
         {
            printf ("-Waring: Only one file can be used as output\n") ;
         }
         else
         {
            outputFile.open(t.c_str(), ios::out|((outputMode == OUTPUTAPPEND)?
                               (ios::app):(ios::trunc) ) ) ;
            if ( !outputFile.is_open() )
            {
               printf ("-Error: Failed to open file %s\n", t.c_str()) ;
            }
            else
               outputMode = OUTPUTDONE ;
         }
      }
      else
      {
         options.push_back ( t ) ;
         count++;
      }
   }

   if ( QUIT_COMMAND == command )
   {
      printf( " disconnect and quit\n" ) ;
      quit = true ;
   }
   else if ( "clear" == command )
   {
      linenoiseClearScreen() ;
   }
   else if ( HELP_COMMAND == command )
   {
      displayHelp ( options ) ;
   }
   else if ( CONNECT_COMMAND == command )
   {
      rc = connect (options ) ;
   }
   else if ( ALTER_CS_COMMAND == command )
   {
      rc = alter_cs ( options ) ;
   }
   else if ( MSG_COMMAND == command )
   {
      rc = msg () ;
   }
   else if ( ossStrncmp ( command.c_str(), ADMIN_COMMAND,
                          ossStrlen(ADMIN_COMMAND)) == 0 )
   {
      rc = admin ( &(command.c_str()[1]), options ) ;
   }
   else if ( PARTITION_COMMAND == command )
   {
      rc = calcPartition() ;
   }
   else if ( 1 == count)
   {
      rc = parseIUQD ( command ) ;
   }
   else if ( 1 < count )
   {
      printf ("-Error: Unknown command\n") ;
   }
}

#ifdef _LINUX
void signalHandler ( INT32 sigNum )
{
   switch ( sigNum )
   {
      case SIGTSTP :
         printf ( " Recieved SIGTSTP signal, send interrupt\n" ) ;
         interrupt () ;
         break ;
      case SIGINT :
         printf ( " Recieved SIGINT signal, disconnect and quit\n" ) ;
         quit = true ;
         break ;
      default :
         break ;
   }
}
#endif

int main ( int argc, char** argv )
{
#ifdef _LINUX
   ossSigSet sigSet ;
   sigSet.sigAdd ( SIGTSTP ) ;
   INT32 rc = ossRegisterSignalHandle( sigSet,
      (SIG_HANDLE)signalHandler ) ;
   if ( SDB_OK != rc )
   {
      printf ( "-Error: Failed to register signal handler\n" ) ;
      return rc ;
   }
#endif //_LINUX

   linenoiseSetCompletionCallback( (linenoiseCompletionCallback*)lineComplete ) ;

   outBufferSize = DEFAULT_BUFFER_SIZE ;
   pOutBuffer = (char*)malloc(sizeof(char)*outBufferSize ) ;
   if ( !pOutBuffer )
   {
      printf ( "-Error: Failed to allocate out buffer for %d bytes",
               outBufferSize ) ;
      return -1 ;
   }
   inBufferSize = DEFAULT_BUFFER_SIZE ;
   pInBuffer = (char*)malloc(sizeof(char)*inBufferSize ) ;
   if ( !pInBuffer )
   {
      printf ( "-Error: Failed to allocate out buffer for %d bytes",
               inBufferSize ) ;
      return -1 ;
   }

   memset ( defaultCS, 0, sizeof(defaultCS)) ;
   while ( !quit )
   {
      receivePrompt () ;
   }

   sendDisconnect () ;
   ossSleep ( 100 ) ;

   if ( sock )
      delete (sock) ;
   sock = NULL ;
   return 0 ;
}

