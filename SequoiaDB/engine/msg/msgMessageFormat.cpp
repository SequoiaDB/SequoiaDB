/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = msgMessageFormat.cpp

   Descriptive Name = Messaging Client

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains functions for building
   and extracting data for client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/10/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "msgMessageFormat.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "msgDef.h"
#include "utilStr.hpp"
#include "pdTrace.hpp"
#include "msgTrace.hpp"
#include <stddef.h>

using namespace engine;
using namespace bson ;

const CHAR* serviceID2String( UINT32 serviceID )
{
   switch ( serviceID )
   {
      case MSG_ROUTE_LOCAL_SERVICE :
         return "LOCAL" ;
      case MSG_ROUTE_REPL_SERVICE :
         return "REPL" ;
      case MSG_ROUTE_SHARD_SERVCIE :
         return "SHARD" ;
      case MSG_ROUTE_CAT_SERVICE :
         return "CATA" ;
      case MSG_ROUTE_REST_SERVICE :
         return "REST" ;
      case MSG_ROUTE_OM_SERVICE :
         return "OM" ;
      default :
         break ;
   }
   return "UNKNOW" ;
}

string routeID2String( MsgRouteID routeID )
{
   stringstream ss ;
   ss << "{ GroupID:" << routeID.columns.groupID
      << ", NodeID:" << routeID.columns.nodeID
      << ", ServiceID:" << routeID.columns.serviceID
      << "(" << serviceID2String( routeID.columns.serviceID ) << ") }" ;
   return ss.str() ;
}

string routeID2String( UINT64 nodeID )
{
   return routeID2String( *(MsgRouteID*)&nodeID ) ;
}

const CHAR* msgType2String( MSG_TYPE msgType, BOOLEAN isCommand )
{
   switch( GET_REQUEST_TYPE( msgType ) )
   {
      case MSG_BS_INSERT_REQ :
      case MSG_BS_TRANS_INSERT_REQ :
         return "INSERT" ;
      case MSG_BS_UPDATE_REQ :
      case MSG_BS_TRANS_UPDATE_REQ :
         return "UPDATE" ;
      case MSG_BS_SQL_REQ :
         return "SQL" ;
      case MSG_BS_QUERY_REQ :
         return isCommand ? "COMMAND" : "QUERY" ;
      case MSG_BS_GETMORE_REQ :
         return "GETMORE" ;
      case MSG_BS_DELETE_REQ :
      case MSG_BS_TRANS_DELETE_REQ :
         return "DELETE" ;
      case MSG_BS_KILL_CONTEXT_REQ :
         return "KILLCONTEXT" ;
      case MSG_BS_INTERRUPTE :
         return "INTERRUPTE" ;
      case MSG_BS_TRANS_BEGIN_REQ :
         return "BEGIN" ;
      case MSG_BS_TRANS_COMMIT_REQ :
      case MSG_BS_TRANS_COMMITPRE_REQ :
         return "COMMIT" ;
      case MSG_BS_TRANS_ROLLBACK_REQ :
         return "ROLLBACK" ;
      case MSG_BS_AGGREGATE_REQ :
         return "AGGREGATE" ;
      case MSG_AUTH_VERIFY_REQ :
         return "LOGIN" ;
      case MSG_AUTH_CRTUSR_REQ :
         return "CREATE USER" ;
      case MSG_AUTH_DELUSR_REQ :
         return "DELETE USER" ;
      case MSG_BS_LOB_OPEN_REQ :
         return "LOB OPEN" ;
      case MSG_BS_LOB_WRITE_REQ :
         return "LOB WRITE" ;
      case MSG_BS_LOB_READ_REQ :
         return "LOB READ" ;
      case MSG_BS_LOB_REMOVE_REQ :
         return "LOB REMOVE" ;
      case MSG_BS_LOB_UPDATE_REQ :
         return "LOB UPDATE" ;
      case MSG_BS_LOB_CLOSE_REQ :
         return "LOB CLOSE" ;
      case MSG_BS_LOB_LOCK_REQ :
         return "LOB LOCK" ;
   } ;
   return "UNKNOW" ;
}

/*
   Message to string functions
*/
typedef struct _msgMsgFuncItem
{
   INT32                      _opCode ;
   const CHAR                 *_opDesp ;
   EXPAND_MSG_2_STRING_FUNC   _pFunc ;
} msgMsgFuncItem ;

#define MSG_MAP_2_STRING_FUNC(opCode,pFunc) \
   { (INT32)opCode, #opCode, (EXPAND_MSG_2_STRING_FUNC)pFunc }

#define MSG_MAP_2_STRING_FUNC_WITH_DESP(opCode,opDesp,pFunc) \
   { (INT32)opCode, opDesp, (EXPAND_MSG_2_STRING_FUNC)pFunc }

msgMsgFuncItem* msgGetExpand2StringFunc( const MsgHeader *pMsg )
{
   static msgMsgFuncItem s_Entry[] = {
      MSG_MAP_2_STRING_FUNC( MSG_COM_SESSION_INIT_REQ,
                             msgExpandComSessionInit2String ),
      MSG_MAP_2_STRING_FUNC( MSG_BS_QUERY_REQ,
                             msgExpandBSQuery2String ),
      { MSG_NULL, NULL }
   } ;
   static UINT32 s_EntrySize = sizeof( s_Entry ) / sizeof( msgMsgFuncItem ) ;

   for ( UINT32 i = 0 ; i < s_EntrySize ; ++i )
   {
      if ( s_Entry[ i ]._opCode == pMsg->opCode )
      {
         return &s_Entry[ i ] ;
      }
   }
   return NULL ;   
}

string msg2String( const MsgHeader *pMsg,
                   UINT32 headMask,
                   UINT32 expandMask )
{
   stringstream ss ;
   msgMsgFuncItem *pItem = msgGetExpand2StringFunc( pMsg ) ;

   if ( MSG_HEADER_MASK_LEN & headMask )
   {
      ss << "Length: " << pMsg->messageLength << ", " ;
   }
   if ( MSG_HEADER_MASK_OPCODE & headMask )
   {
      ss << "OpCode: (" << IS_REPLY_TYPE( pMsg->opCode ) << ")"
         << GET_REQUEST_TYPE( pMsg->opCode ) ;
      if ( pItem && pItem->_opDesp )
      {
         ss << "(" << pItem->_opDesp << ")" ;
      }
      ss << "," ;
   }
   if ( MSG_HEADER_MASK_TID & headMask )
   {
      ss << "TID: " << pMsg->TID << ", " ;
   }
   if ( MSG_HEADER_MASK_ROUTEID & headMask )
   {
      ss << "RouteID: { GroupID:" << pMsg->routeID.columns.groupID
         << ", NodeID:" << pMsg->routeID.columns.nodeID
         << ", ServiceID:" << pMsg->routeID.columns.serviceID
         << "(" << serviceID2String( pMsg->routeID.columns.serviceID )
         << ") }," ;
   }
   if ( MSG_HEADER_MASK_REQID & headMask )
   {
      ss << "RequestID: " << pMsg->requestID << ", " ;
   }

   if ( pItem && pItem->_pFunc && 0 != expandMask )
   {
      pItem->_pFunc( ss, pMsg, expandMask ) ;
   }

   string str = ss.str() ;
   utilStrRtrim( str ) ;
   UINT32 len = str.length() ;
   if ( len > 1 && str.at( len - 1 ) == ',' )
   {
      str.erase( len - 1 ) ;
   }
   return str ;
}

void msgExpandComSessionInit2String( stringstream &ss,
                                     const MsgHeader *pMsg,
                                     UINT32 expandMask )
{
   MsgComSessionInitReq *pReq = ( MsgComSessionInitReq* )pMsg ;

   if ( pMsg->messageLength != offsetof( MsgComSessionInitReq, srcRouteID ) &&
        pMsg->messageLength <= (INT32)sizeof( MsgComSessionInitReq ) )
   {
      ss << "Error: Size not match" ;
   }
   else
   {
      ss << "DestRouteID: " << routeID2String( pReq->dstRouteID ) << ", " ;

      if ( pMsg->messageLength > (INT32)sizeof( MsgComSessionInitReq ) )
      {
         CHAR szTmp[ 40 ] = { 0 } ;
         ossIP2Str( pReq->localIP, szTmp, sizeof( szTmp ) - 1 ) ;

         ss << "SourceRouteID: " << routeID2String( pReq->srcRouteID )
            << ", LocalIP:" << pReq->localIP
            << "(" << szTmp
            << "), LocalPort:" << pReq->localPort
            << ", LocalTID: " << pReq->localTID
            << ", LocalSessionID:" << pReq->localSessionID << ", " ;

         try
         {
            BSONObj objInfo( pReq->data ) ;
            ss << "Detail: " << objInfo.toString() ;
         }
         catch( std::exception &e )
         {
            ss << "Detail: Occur exception(" << e.what() << ")" ;
         }
      }
   }

}

void msgExpandBSQuery2String( stringstream &ss,
                              const MsgHeader *pMsg,
                              UINT32 expandMask )
{
   INT32 rc         = SDB_OK ;
   INT32 flag       = 0 ;
   CHAR *collection = NULL ;
   SINT64 skip      =  0;
   SINT64 limit     = -1 ;
   CHAR *query      = NULL ;
   CHAR *selector   = NULL ;
   CHAR *orderby    = NULL ;
   CHAR *hint       = NULL ;
   rc = msgExtractQuery( (CHAR*)pMsg, &flag, &collection,
                         &skip, &limit, &query, &selector,
                         &orderby, &hint ) ;
   if ( SDB_OK != rc )
   {
      ss << "Error: Extract failed: " << rc ;
   }
   else
   {
      try
      {
         BSONObj objQuery( query ) ;
         BSONObj objSelector( selector ) ;
         BSONObj objOrderby( orderby ) ;
         BSONObj objHint( hint ) ;

         if ( expandMask | MSG_EXP_MASK_CLNAME )
         {
            ss << "Collection Name: " << collection << "," ;
         }
         if ( expandMask | MSG_EXP_MASK_OTHER )
         {
            ss << "Limit: " << limit << ", Skip: " << skip << "," ;
         }
         if ( expandMask | MSG_EXP_MASK_MATCHER )
         {
            ss << "Matcher: " << objQuery.toString() << "," ;
         }
         if ( expandMask | MSG_EXP_MASK_SELECTOR )
         {
            ss << "Selector: " << objSelector.toString() << "," ;
         }
         if ( expandMask | MSG_EXP_MASK_ORDERBY )
         {
            ss << "Orderby: " << objOrderby.toString() << "," ;
         }
         if ( expandMask | MSG_EXP_MASK_HINT )
         {
            ss << "Hint: " << objHint.toString() << "," ;
         }
      }
      catch ( std::exception &e )
      {
         ss << "Detail: Occur exception(" << e.what() << ")" ;
      }
   }
}

