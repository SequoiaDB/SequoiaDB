/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = commands.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "commands.hpp"
#include "msgBuffer.hpp"
#include "parser.hpp"
#include "msg.hpp"
#include "msgDef.h"
#include <vector>
#include <string>
#include "../bson/lib/md5.hpp"

using namespace bson ;

DECLARE_COMMAND_VAR( insert )
DECLARE_COMMAND_VAR( delete )
DECLARE_COMMAND_VAR( update )
DECLARE_COMMAND_VAR( query )
DECLARE_COMMAND_VAR( find )
DECLARE_COMMAND_VAR( getMore )
DECLARE_COMMAND_VAR( killCursors )
DECLARE_COMMAND_VAR( msg )
DECLARE_COMMAND_VAR( listUsers )
DECLARE_COMMAND_VAR( create )
DECLARE_COMMAND_VAR( createCS )
DECLARE_COMMAND_VAR( listCollections )
DECLARE_COMMAND_VAR( drop )
DECLARE_COMMAND_VAR( count )
DECLARE_COMMAND_VAR( aggregate )
DECLARE_COMMAND_VAR( dropDatabase )
DECLARE_COMMAND_VAR( createIndexes )
DECLARE_COMMAND_VAR( deleteIndexes )
DECLARE_COMMAND_VAR( listIndexes )
DECLARE_COMMAND_VAR( getlasterror )
DECLARE_COMMAND_VAR( ismaster )
DECLARE_COMMAND_VAR( ping )
DECLARE_COMMAND_VAR( logout )
DECLARE_COMMAND_VAR( whatsmyuri )
DECLARE_COMMAND_VAR( buildinfo )
DECLARE_COMMAND_VAR( getLog )
DECLARE_COMMAND_VAR( listDatabases )
DECLARE_COMMAND_VAR( distinct )
DECLARE_COMMAND_VAR( createUser )
DECLARE_COMMAND_VAR( dropUser )
DECLARE_COMMAND_VAR( saslStart )
DECLARE_COMMAND_VAR( saslContinue )

void convertProjection( BSONObj &proj )
{
   BSONObjBuilder newBuilder ;
   BOOLEAN hasId = FALSE ;
   BOOLEAN addExclude = FALSE ;
   BOOLEAN hasOperator = FALSE ;
   BSONObjIterator i( proj ) ;

   if ( proj.isEmpty() )
   {
      goto done ;
   }

   while ( i.more() )
   {
      BSONElement e = i.next() ;
      if ( 0 == ossStrcmp( e.fieldName(), "_id" ) )
      {
         hasId = TRUE ;
      }
      if ( Object == e.type() && '$' == e.Obj().firstElementFieldName()[0] )
      {
         hasOperator = TRUE ;
         newBuilder.append( e ) ;
      }
      else
      {
         // { b: 1 } => { b: { $include: 1 } }
         newBuilder.append( e.fieldName(),
                            BSON( "$include" << ( e.trueValue() ? 1 : 0 ) ) ) ;
         if ( ! e.trueValue() )
         {
            addExclude = TRUE ;
         }
      }
   }

   if ( !hasOperator && !hasId && !addExclude )
   {
      newBuilder.append( "_id", BSON( "$include" << 1 ) ) ;
   }

   proj = newBuilder.obj() ;

done:
   return ;
}

/// implement of commands
INT32 insertCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 insertCommand::buildMsg( msgParser& parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpInsert *insert     = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   parser.setCurrentOp( OP_INSERT ) ;
   sdbMsg.reverse( sizeof( MsgOpInsert ) ) ;
   sdbMsg.advance( sizeof( MsgOpInsert ) - 4 ) ;

   insert = ( MsgOpInsert *)sdbMsg.data() ;
   insert->header.opCode = MSG_BS_INSERT_REQ ;
   insert->header.TID = 0 ;
   insert->header.routeID.value = 0 ;
   insert->header.requestID = packet.requestId ;
   insert->version = 0 ;
   insert->w = 0 ;
   insert->padding = 0 ;
   insert->flags |= FLG_INSERT_RETURNNUM ;

   insert->nameLength = packet.fullName.length() ;
   sdbMsg.write( packet.fullName.c_str(), insert->nameLength + 1, TRUE ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_ORDERED ) )
         {
            if ( !document.getBoolField( FAP_FIELD_NAME_ORDERED ) )
            {
               insert->flags |= FLG_INSERT_CONTONDUP ;
            }
         }
         else
         {
            sdbMsg.write( document, TRUE ) ;
         }
      }
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 insertCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 deleteCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 deleteCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   MsgOpDelete *del        = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   bson::BSONObj cond, hint ;

   parser.setCurrentOp( OP_REMOVE );
   sdbMsg.reverse( sizeof( MsgOpDelete ) ) ;
   sdbMsg.advance( sizeof( MsgOpDelete ) - 4 ) ;

   del = ( MsgOpDelete *)sdbMsg.data() ;
   del->header.opCode = MSG_BS_DELETE_REQ ;
   del->header.TID = 0 ;
   del->header.routeID.value = 0 ;
   del->header.requestID = packet.requestId ;
   del->version = 0 ;
   del->w = 0 ;
   del->padding = 0 ;
   del->flags |= FLG_DELETE_RETURNNUM ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_DELETE ) )
         {
            continue ;
         }

         // limit
         // there is field named "limit" in packet.sections.document.
         // when db.test.deleteOne is called, limit = 1
         // when db.test.deleteMany is called, limit = 0

         // cond
         if ( document.hasField( FAP_FIELD_NAME_Q ) )
         {
            cond = document.getObjectField( FAP_FIELD_NAME_Q ) ;
         }
      }
   }

   del->nameLength = packet.fullName.length() ;
   sdbMsg.write( packet.fullName.c_str(), del->nameLength + 1, TRUE ) ;
   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   return SDB_OK ;

}

INT32 deleteCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 updateCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 updateCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   MsgOpUpdate *update     = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   bson::BSONObj cond, updator, hint ;
   BOOLEAN multi  = FALSE ;
   BOOLEAN upsert = FALSE ;

   parser.setCurrentOp( OP_UPDATE ) ;
   sdbMsg.reverse( sizeof( MsgOpUpdate ) ) ;
   sdbMsg.advance( sizeof( MsgOpUpdate ) - 4 ) ;

   update = ( MsgOpUpdate *)sdbMsg.data() ;
   update->header.opCode = MSG_BS_UPDATE_REQ ;
   update->header.TID = 0 ;
   update->header.routeID.value = 0 ;
   update->header.requestID = packet.requestId ;
   update->version = 0 ;
   update->w = 0 ;
   update->padding = 0 ;
   update->flags |= FLG_UPDATE_RETURNNUM ;
   update->nameLength = packet.fullName.length() ;

   sdbMsg.write( packet.fullName.c_str(), update->nameLength + 1, TRUE ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_UPDATE ) )
         {
            continue ;
         }
         // cond
         if ( document.hasField( FAP_FIELD_NAME_Q) )
         {
            cond = document.getObjectField( FAP_FIELD_NAME_Q ) ;
         }
         // updator
         if ( document.hasField( FAP_FIELD_NAME_U ) )
         {
            updator = document.getObjectField( FAP_FIELD_NAME_U ) ;
         }
         // hint
         if ( document.hasField( FAP_FIELD_NAME_HINT ) )
         {
            hint = BSON( "" << document.getStringField( FAP_FIELD_NAME_HINT ) ) ;
         }
         // multi
         if ( document.hasField( FAP_FIELD_NAME_MULTI ) )
         {
            multi = document.getBoolField( FAP_FIELD_NAME_MULTI ) ;
         }
         // upsert
         if ( document.hasField( FAP_FIELD_NAME_UPSERT ) )
         {
            upsert = document.getBoolField( FAP_FIELD_NAME_UPSERT ) ;
         }
      }
   }

   if ( false == multi )
   {
      update->flags |= FLG_UPDATE_ONE ;
   }
   if ( upsert )
   {
      update->flags |= FLG_UPDATE_UPSERT ;
   }

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( updator, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   return SDB_OK ;

}

INT32 updateCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 queryCommand::convert( msgParser &parser )
{
   INT32 rc                = SDB_OK ;
   INT32 nToReturn         = 0 ;
   baseCommand *&cmd       = parser.command() ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.readInt( sizeof( packet.nToSkip ), ( CHAR * )&packet.nToSkip ) ;
   parser.readInt( sizeof( nToReturn ), ( CHAR * )&nToReturn ) ;
   packet.nToReturn = nToReturn < 0 ? -nToReturn : nToReturn ;

   if ( 0 == packet.nToReturn )
   {
      packet.nToReturn  = -1 ;
   }

   if ( !parser.more() )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   parser.readNextObj( packet.all ) ;

   if ( parser.more() )
   {
      parser.readNextObj( packet.fieldToReturn ) ;
   }

   if ( 0 !=  packet.optionMask )
   {
      if ( packet.with( OPTION_CMD ) )
      {
         const CHAR *cmdName = packet.all.firstElementFieldName() ;
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         packet.fullName += packet.all.getStringField( cmdName ) ;
         cmd = commandMgr::instance()->findCommand( cmdName ) ;
      }

      if ( NULL == cmd )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
         goto error ;
      }

      rc = cmd->convert( parser ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      goto done ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 queryCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_QUERY ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;
   query->nameLength = packet.fullName.length() ;

   if ( packet.all.hasField( FAP_FIELD_NAME_LIMIT ) )
   {
      query->numToReturn = packet.all.getIntField( FAP_FIELD_NAME_LIMIT ) ;
   }

   if ( packet.all.hasField( FAP_FIELD_NAME_SKIP ) )
   {
      query->numToSkip = packet.all.getIntField( FAP_FIELD_NAME_SKIP ) ;
   }

   if ( packet.all.getBoolField( "$explain" ) )
   {
      query->flags |= FLG_QUERY_EXPLAIN ;
   }

   {
      if ( parser.more() )
      {
         parser.readNextObj( packet.fieldToReturn ) ;
      }

      bson::BSONObj cond, orderby, hint ;
      cond = getQueryObj( packet.all ) ;
      orderby = getSortObj( packet.all ) ;
      hint = getHintObj( packet.all ) ;

      sdbMsg.write( packet.fullName.c_str(), query->nameLength + 1, TRUE ) ;
      sdbMsg.write( cond, TRUE ) ;
      sdbMsg.write( packet.fieldToReturn, TRUE ) ;
      sdbMsg.write( orderby, TRUE ) ;
      sdbMsg.write( hint, TRUE ) ;
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 queryCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 getMoreCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 getMoreCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpGetMore *more      = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_GETMORE ) ;
   sdbMsg.reverse( sizeof( MsgOpGetMore ) ) ;
   sdbMsg.advance( sizeof( MsgOpGetMore ) ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_GETMORE ) )
         {
            packet.cursorId = document.getIntField( FAP_FIELD_NAME_GETMORE ) ;
         }
      }
   }

   more = ( MsgOpGetMore * )sdbMsg.data() ;
   more->header.opCode = MSG_BS_GETMORE_REQ ;
   more->header.TID = 0 ;
   more->header.routeID.value = 0 ;
   more->header.requestID = packet.requestId ;
   // match to sequoiadb contextID, need decrease 1.
   // Please see the the branch with opType equal OP_GETMORE in
   // _mongoSession::_handleResponse function( the "id" in result has increased 1 )
   more->contextID = packet.cursorId - 1 ;
   more->numToReturn = -1 ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 getMoreCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 killCursorsCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 killCursorsCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   INT64 cursorId          = 0 ;
   MsgOpKillContexts *kill = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   bson::BSONElement cursors ;

   parser.setCurrentOp( OP_KILLCURSORS ) ;
   sdbMsg.reverse( sizeof( MsgOpKillContexts ) ) ;
   sdbMsg.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;

   kill = ( MsgOpKillContexts * )sdbMsg.data() ;
   kill->header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
   kill->header.TID = 0 ;
   kill->header.routeID.value = 0 ;
   kill->header.requestID = packet.requestId ;
   kill->ZERO = 0 ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_CURSORS ) )
         {
            cursors = document.getField( FAP_FIELD_NAME_CURSORS ) ;
         }
      }
   }

   kill->numContexts = 0 ;
   if ( cursors.type() == bson::Array )
   {
      bson::BSONObjIterator it ( cursors.embeddedObject() ) ;
      while ( it.more () )
      {
         bson::BSONElement ele = it.next() ;
         if ( ele.isNumber() )
         {
            cursorId = (INT64)ele.numberInt() ;
            if ( 0 != cursorId )
            {
               // match to sequoiadb contextID, need decrease 1.
               // Please see the the branch with opType equal OP_GETMORE in
               // _mongoSession::_handleResponse function
               // ( the "id" in result has increased 1 )
               cursorId -= 1 ;
               sdbMsg.write( ( CHAR * )&cursorId, sizeof( SINT64 ) ) ;
               kill->numContexts += 1 ;
            }
         }
      }
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 killCursorsCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 listUsersCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 listUsersCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_USERS ;

   parser.setCurrentOp( OP_CMD_LISTUSER ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;
   {
      bson::BSONObj cond, selector, orderby, hint ;
      sdbMsg.write( cond, TRUE ) ;
      sdbMsg.write( selector, TRUE ) ;
      sdbMsg.write( orderby, TRUE ) ;
      sdbMsg.write( hint, TRUE ) ;
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 listUsersCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 createCSCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 createCSCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;

   parser.setCurrentOp( OP_CMD_CREATE_CS ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;
   query->nameLength = ossStrlen( cmdName ) ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;
   bson::BSONObj obj, empty ;
   obj = BSON( FIELD_NAME_NAME << packet.csName
                               << FIELD_NAME_PAGE_SIZE << 65536 ) ;

   sdbMsg.write( obj, TRUE ) ;    // condition
   sdbMsg.write( empty, TRUE ) ;  // selector
   sdbMsg.write( empty, TRUE ) ;  // orderby
   sdbMsg.write( empty, TRUE ) ;  // hint

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 createCSCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 createCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 createCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;

   parser.setCurrentOp( OP_CMD_CREATE ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj obj, empty ;
   obj = BSON( FIELD_NAME_NAME << packet.fullName.c_str() ) ;

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 createCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 listCollectionsCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 listCollectionsCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;

   parser.setCurrentOp( OP_CMD_GET_CLS ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags |= 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj cond, selector, orderby, hint, filter ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_FILTER ) )
         {
            filter = document.getObjectField( FAP_FIELD_NAME_FILTER ) ;
         }
      }
   }

   // cond: { Name: { $gt: "foo.", $lt: "foo/" }, ... }
   string lowBound = packet.csName ;
   lowBound += "." ;
   string upBound = packet.csName ;
   upBound += "/" ;

   bson::BSONObjBuilder builder ;
   builder.appendElements( filter ) ;
   builder.append( "Name", BSON( "$gt" << lowBound << "$lt" << upBound ) ) ;

   sdbMsg.write( builder.obj(), TRUE ) ;
   sdbMsg.write( selector, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 listCollectionsCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 dropCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 dropCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ;

   parser.setCurrentOp( OP_CMD_DROP ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags |= 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   {
      bson::BSONObj obj, empty ;
      obj = BSON( FIELD_NAME_NAME << packet.fullName.c_str() ) ;
      sdbMsg.write( obj, TRUE ) ;    // condition
      sdbMsg.write( empty, TRUE ) ;  // selector
      sdbMsg.write( empty, TRUE ) ;  // orderby
      sdbMsg.write( empty, TRUE ) ;  // hint
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 dropCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 countCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 countCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT ;
   bson::BSONObj empty, hint, matcher, obj ;

   parser.setCurrentOp( OP_CMD_COUNT ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_QUERY ) )
         {
            matcher = document.getObjectField( FAP_FIELD_NAME_QUERY ) ;
         }
         if ( document.hasField( FAP_FIELD_NAME_HINT ) )
         {
            hint = BSON( "" << document.getStringField( FAP_FIELD_NAME_HINT ) ) ;
         }
         if ( document.hasField( FAP_FIELD_NAME_LIMIT ) )
         {
            query->numToReturn = document.getIntField( FAP_FIELD_NAME_LIMIT ) ;
         }
         if ( document.hasField( FAP_FIELD_NAME_SKIP ) )
         {
            query->numToSkip = document.getIntField( FAP_FIELD_NAME_SKIP ) ;
         }
      }
   }

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   if ( !hint.isEmpty() )
   {
      obj = BSON( FIELD_NAME_COLLECTION << packet.fullName.c_str() <<
                  FIELD_NAME_HINT << hint ) ;
   }
   else
   {
      obj = BSON( FIELD_NAME_COLLECTION << packet.fullName.c_str() ) ;
   }

   sdbMsg.write( matcher, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( obj, TRUE ) ;


   sdbMsg.doneLen() ;

   return rc ;
}

INT32 countCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 aggregateCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 aggregateCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpAggregate *aggre   = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_CMD_AGGREGATE ) ;
   sdbMsg.reverse( sizeof( MsgOpAggregate ) ) ;
   sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;

   aggre = ( MsgOpAggregate * )sdbMsg.data() ;
   aggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   aggre->header.TID = 0 ;
   aggre->header.routeID.value = 0 ;
   aggre->header.requestID = packet.requestId ;
   aggre->version = 0 ;
   aggre->w = 0 ;
   aggre->padding = 0 ;
   aggre->flags = 0 ;
   aggre->nameLength = packet.fullName.length() ;

   sdbMsg.write( packet.fullName.c_str(), aggre->nameLength + 1, TRUE ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( !document.hasField( FAP_FIELD_NAME_PIPELINE ) )
         {
            continue ;
         }
         packet.all = document.getObjectField( FAP_FIELD_NAME_PIPELINE ) ;
      }
   }

   /* eg: { pipeline: [ { $match: { b: 1 } },
                        { $group: { _id: "$a", b: { $sum: "$b" } } }
                      ] } */
   bson::BSONObj::iterator it( packet.all ) ;
   while ( it.more() )
   {
      bson::BSONElement ele = it.next() ;

      if ( ele.type() != Object )
      {
         sdbMsg.write( ele.rawdata(), ele.size(), TRUE ) ;
         continue ;
      }

      bson::BSONObj oneStage = ele.Obj() ;
      if ( 0 != ossStrcmp( oneStage.firstElement().fieldName(), "$group" ) )
      {
         sdbMsg.write( oneStage, TRUE ) ;
         continue ;
      }

      bson::BSONObj groupValue = oneStage.getObjectField( "$group" ) ;
      const CHAR* idValue = groupValue.getStringField( FAP_FIELD_NAME__ID ) ;
      if ( '$' == idValue[0] )
      {
         // { $group: { _id: "$a", total: { $sum: "$b" } } } =>
         // { $group: { _id: "$a", total: { $sum: "$b" },
        //              tmp_id_field: "$a" } },
         // { $project: { _id: "$tmp_id_field", total: 1 } }
         bson::BSONObjBuilder bobGroup, bobProj ;
         bson::BSONObj newGroup, newProject, obj ;

         bson::BSONObjIterator itr( groupValue ) ;
         while ( itr.more() )
         {
            bson::BSONElement e = itr.next() ;
            if ( e.isABSONObj() && e.Obj().getIntField( "$sum" ) == 1 )
            {
               // { $group: { _id: null, total: { $sum: 1 } } } =>
               // { $group: { _id: null, total: { $count: "$_id" } } }
               bobGroup.append( e.fieldName(), BSON( "$count" << "$_id" ) ) ;
            }
            else
            {
               bobGroup.append( e ) ;
            }
            if ( 0 == ossStrcmp( e.fieldName(), FAP_FIELD_NAME__ID ) )
            {
               bobProj.append( FAP_FIELD_NAME__ID, "$tmp_id_field" ) ;
            }
            else
            {
               bobProj.append( e.fieldName(), 1 ) ;
            }
         }
         bobGroup.append( FAP_FIELD_NAME_TMP_ID_FIELD, idValue ) ;

         newGroup = BSON( "$group" << bobGroup.done() ) ;
         newProject = BSON( "$project" << bobProj.done() ) ;

         sdbMsg.write( newGroup, TRUE ) ;
         sdbMsg.write( newProject, TRUE ) ;
      }
      else
      {
         bson::BSONObjBuilder bobGroup ;
         bson::BSONObj newGroup ;
         bson::BSONObjIterator itr( groupValue ) ;
         while ( itr.more() )
         {
            bson::BSONElement e = itr.next() ;
            if ( e.isABSONObj() && e.Obj().getIntField( "$sum" ) == 1 )
            {
               // { $group: { _id: null, total: { $sum: 1 } } } =>
               // { $group: { _id: null, total: { $count: "$_id" } } }
               bobGroup.append( e.fieldName(), BSON( "$count" << "$_id" ) ) ;
            }
            else
            {
               bobGroup.append( e ) ;
            }
         }
         newGroup = BSON( "$group" << bobGroup.done() ) ;
         sdbMsg.write( newGroup, TRUE ) ;
      }
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 aggregateCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 dropDatabaseCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 dropDatabaseCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ;

   parser.setCurrentOp( OP_CMD_DROP_DATABASE ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags |= 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;
   {
      bson::BSONObj obj, empty ;
      obj = BSON( FIELD_NAME_NAME << packet.csName ) ;

      sdbMsg.write( obj, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 dropDatabaseCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 createIndexesCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 createIndexesCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX ;
   bson::BSONObjBuilder bob, indexBob ;
   bson::BSONObj indexesArr, indexes, empty ;
   bson::BSONObj::iterator indexesIter ;
   BOOLEAN enforced = FALSE ;
   BOOLEAN unique = FALSE ;

   parser.setCurrentOp( OP_CREATE_INDEX ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_INDEXES ) )
         {
            indexesArr = document.getObjectField( FAP_FIELD_NAME_INDEXES ) ;
         }
      }
   }

   indexesIter = indexesArr.begin() ;
   while ( indexesIter.more() )
   {
      indexes = indexesIter.next().embeddedObject() ;
   }

   if ( !indexes.hasField( IXM_FIELD_NAME_KEY ) ||
        !indexes.hasField( IXM_FIELD_NAME_NAME ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   indexBob.append( IXM_FIELD_NAME_KEY,
                    indexes.getObjectField( IXM_FIELD_NAME_KEY ) ) ;
   indexBob.append( IXM_FIELD_NAME_NAME,
                    indexes.getStringField( IXM_FIELD_NAME_NAME ) ) ;

   unique = indexes.getBoolField( IXM_FIELD_NAME_UNIQUE ) ;
   if ( unique )
   {
      enforced = TRUE ;
   }

   indexBob.append( IXM_FIELD_NAME_UNIQUE, unique ) ;
   indexBob.append( IXM_FIELD_NAME_ENFORCED, enforced ) ;
   bob.append( FIELD_NAME_COLLECTION, packet.fullName.c_str() ) ;
   bob.append( FIELD_NAME_INDEX, indexBob.obj() ) ;
   sdbMsg.write( bob.obj(), TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 createIndexesCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 deleteIndexesCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 deleteIndexesCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgOpQuery *query = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ;
   string indexName ;

   parser.setCurrentOp( OP_CMD_DROP_INDEX ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = -1 ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_INDEX ) )
         {
            indexName = document.getStringField( FAP_FIELD_NAME_INDEX ) ;
         }
      }
   }

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;
   {
      bson::BSONObj obj, indexObj, empty ;
      indexObj = BSON( "" << indexName.c_str() ) ;
      obj = BSON( FIELD_NAME_COLLECTION << packet.fullName.c_str() <<
                  FIELD_NAME_INDEX << indexObj ) ;
      sdbMsg.write( obj, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
      sdbMsg.write( empty, TRUE ) ;
   }

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 deleteIndexesCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 listIndexesCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 listIndexesCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   MsgOpQuery *query = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES ;

   parser.setCurrentOp( OP_CMD_GET_INDEX ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj obj, empty ;
   obj = BSON( FIELD_NAME_COLLECTION << packet.fullName ) ;

   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( obj, TRUE ) ;

   sdbMsg.doneLen() ;

   return SDB_OK ;
}

INT32 listIndexesCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 getlasterrorCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 getlasterrorCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_GETLASTERROR ) ;
   return SDB_OK ;
}

INT32 getlasterrorCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 pingCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 pingCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_PING ) ;
   return SDB_OK ;
}

INT32 pingCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 ismasterCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 ismasterCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_ISMASTER ) ;
   return SDB_OK ;
}

INT32 ismasterCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 logoutCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 logoutCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_PING ) ;
   return SDB_OK ;
}

INT32 logoutCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

/*
*
* The structure of OP_MSG message is as follows:
*
* struct Msg
* {
*    mongoMsgHeader header ;
*    INT32 flags ;
*    Section[] sections ;
*    UINT32 checksum ;   // CRC-32C checksum
* };

* An OP_MSG message contains one or more sections. Each section starts with
* a sectionType byte indicating ite type. Everything after the sectionType byte
* constitutes the section's payload. The structture of the two types of sections
* is as follows:
*
* 1. sectionType 0 :
*
* struct Section
* {
*    CHAR sectionType = '0' ;
*    bson::BSONObj body ;
* }

* 2. sectionType 1 :
*
* struct section
* {
*    CHAR sectionType = '1' ;
*    Sequence sequence ;
* };
*
* struct Sequence
* {
*    // size = sizeof( UINT32 ) + ( name.length() + 1 ) + documents's size()
*    UINT32 size ;
*    std::string name ;
*    std::vector<bson::BSONObj> documents ;
* };
*
*/
INT32 msgCommand::convert( msgParser &parser )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
   mongoDataPacket &packet = parser.dataPacket() ;
   UINT32 i = 0 ;

   while ( parser.more() )
   {
      section sec ;
      bson::BSONObj secDoc ;

      // the last 4 bytes is checksum
      if ( sizeof( UINT32 ) == parser.bytesLeft() )
      {
         break ;
      }

      parser.readChar( &sec.sectionType ) ;
      if ( !parser.more() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SECTION_BODY == sec.sectionType )
      {
         sec.size = 0 ;
         sec.name = "" ;
         parser.readNextObj( secDoc ) ;
         sec.documents.push_back( secDoc ) ;
         packet.sections.push_back( sec ) ;
      }
      else if ( SECTION_SEQUENCE == sec.sectionType )
      {
         UINT32 needToRead = 0 ;
         UINT32 hasRead = 0 ;
         parser.readInt( sizeof( INT32 ), (CHAR *)&sec.size ) ;
         parser.readString( sec.name ) ;
         needToRead = ( sec.size - sizeof( INT32 ) - sec.name.length() - 1 ) ;

         if ( needToRead < 0 )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         // ismaster: special process
         if ( sec.size >= packet.msgLen )
         {
            sec.documents.push_back( secDoc ) ;
            packet.sections.push_back( sec ) ;
            break ;
         }

         while ( hasRead < needToRead )
         {
            if ( parser.more() )
            {
               parser.readNextObj( secDoc ) ;
               hasRead += secDoc.objsize() ;
               sec.documents.push_back( secDoc ) ;
            }
            else
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }
         packet.sections.push_back( sec ) ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

      i++ ;
   }

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( !document.hasField( "$db" ) )
         {
            continue ;
         }
         const CHAR *cmdName = document.firstElementFieldName() ;
         packet.csName = document.getStringField( "$db" ) ;

         if ( 0 == ossStrcmp( FAP_CMD_NAME_GETMORE, cmdName ) )
         {
            packet.clName =
               document.getStringField( FAP_FIELD_NAME_COLLECTION ) ;
         }
         else
         {
            packet.clName = document.getStringField( cmdName ) ;
         }

         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         packet.fullName += packet.clName ;
         cmd = commandMgr::instance()->findCommand( cmdName ) ;
      }
   }

   if ( NULL == cmd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
      goto error ;
   }

   rc = cmd->convert( parser ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 msgCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_MSG ) ;
   return SDB_OK ;
}

INT32 msgCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 whatsmyuriCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 whatsmyuriCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_WHATSMYURI ) ;
   return SDB_OK ;
}

INT32 whatsmyuriCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 getLogCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 getLogCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_GETLOG ) ;
   return SDB_OK ;
}

INT32 getLogCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 buildinfoCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 buildinfoCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   parser.setCurrentOp( OP_CMD_BUILDINFO ) ;
   return SDB_OK ;
}

INT32 buildinfoCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 findCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 findCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   INT32 nToReturn         = -1 ;
   INT32 nToSkip           = 0 ;
   bson::BSONObj cond, sel, orderby, hint ;

   parser.setCurrentOp( OP_FIND ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;
   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( !document.hasField( FAP_FIELD_NAME_FIND ) )
         {
            continue ;
         }
         // limit
         if ( document.hasField( FAP_FIELD_NAME_LIMIT ) )
         {
            nToReturn = document.getIntField( FAP_FIELD_NAME_LIMIT ) ;
         }

         // skip
         if ( document.hasField( FAP_FIELD_NAME_SKIP ) )
         {
            nToSkip = document.getIntField( FAP_FIELD_NAME_SKIP ) ;
         }

         // cond
         if ( document.hasField( FAP_FIELD_NAME_FILTER ) )
         {
            cond = document.getObjectField( FAP_FIELD_NAME_FILTER ) ;
         }

         // sel
         if ( document.hasField( FAP_FIELD_NAME_PROJECTTION ) )
         {
            sel = document.getObjectField( FAP_FIELD_NAME_PROJECTTION ) ;
            convertProjection( sel ) ;
         }
         // orderby
         if ( document.hasField( FAP_FIELD_NAME_SORT ) )
         {
            orderby = document.getObjectField( FAP_FIELD_NAME_SORT ) ;
         }
         // hint
         if ( document.hasField( FAP_FIELD_NAME_HINT ) )
         {
            hint = BSON( "" << document.getStringField( FAP_FIELD_NAME_HINT ) ) ;
         }
      }
   }

   query->numToSkip = nToSkip ;
   query->numToReturn = nToReturn ;
   query->nameLength = packet.fullName.length() ;

   sdbMsg.write( packet.fullName.c_str(), query->nameLength + 1, TRUE ) ;
   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( sel, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;

}

INT32 findCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 distinctCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 distinctCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpAggregate *aggre   = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_CMD_DISTINCT ) ;
   sdbMsg.reverse( sizeof( MsgOpAggregate ) ) ;
   sdbMsg.advance( sizeof( MsgOpAggregate ) - 4 ) ;

   aggre = ( MsgOpAggregate * )sdbMsg.data() ;
   aggre->header.opCode = MSG_BS_AGGREGATE_REQ ;
   aggre->header.TID = 0 ;
   aggre->header.routeID.value = 0 ;
   aggre->header.requestID = packet.requestId ;
   aggre->version = 0 ;
   aggre->w = 0 ;
   aggre->padding = 0 ;
   aggre->flags = 0 ;
   aggre->nameLength = packet.fullName.length() ;
   sdbMsg.write( packet.fullName.c_str(), aggre->nameLength + 1, TRUE ) ;

   std::string distinctfield = FAP_CMD_ADMIN_PREFIX ;
   bson::BSONObj match ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_KEY ) )
         {
            distinctfield += document.getStringField( FAP_FIELD_NAME_KEY ) ;
         }
         if ( document.hasField( FAP_FIELD_NAME_QUERY ) )
         {
            match = BSON( "$match" <<
                          document.getField( FAP_FIELD_NAME_QUERY ) ) ;
         }
      }
   }

   // distinct( "a", { b: 1 } ) =>
   // { $match: { b: 1 } },
   // { $group: { _id: "$a" } },
   // { $group: { _id: null, values: { $addtoset: "$a" } } }
   bson::BSONObj group1, group2 ;
   bson::BSONObjBuilder builder ;
   group1 = BSON( "$group" << BSON( FAP_FIELD_NAME__ID << distinctfield ) ) ;

   builder.appendNull( FAP_FIELD_NAME__ID ) ;
   builder.append( "values", BSON( "$addtoset" << distinctfield ) ) ;
   group2 = BSON( "$group" << builder.done() ) ;

   if ( !match.isEmpty() )
   {
      sdbMsg.write( match, TRUE ) ;
   }

   sdbMsg.write( group1, TRUE ) ;
   sdbMsg.write( group2, TRUE ) ;
   sdbMsg.doneLen() ;

   return rc ;
}

INT32 distinctCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 listDatabasesCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 listDatabasesCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName     = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES ;

   parser.setCurrentOp( OP_CMD_GET_DBS ) ;
   sdbMsg.reverse( sizeof( MsgOpQuery ) ) ;
   sdbMsg.advance( sizeof( MsgOpQuery ) - 4 ) ;

   query = ( MsgOpQuery * )sdbMsg.data() ;
   query->header.opCode = MSG_BS_QUERY_REQ ;
   query->header.TID = 0 ;
   query->header.routeID.value = 0 ;
   query->header.requestID = packet.requestId ;

   query->version = 0 ;
   query->w = 0 ;
   query->padding = 0 ;
   query->flags = 0 ;
   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = 0 ;
   query->numToReturn = -1 ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj empty ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   return rc ;
}

INT32 listDatabasesCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 createUserCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

/** \fn INT32 buildMsg( msgParser& parser, msgBuffer &sdbMsg )
    \brief Build the create user message of SequoiaDB
    \param [in] parser Parser object.
    \param [out] sdbMsg The message we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 createUserCommand::buildMsg( msgParser& parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgAuthCrtUsr *user = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   bson::BSONObj obj ;
   string pName = packet.clName ;
   string pPasswd ;
   string md5 ;

   parser.setCurrentOp( OP_CMD_CRTUSER ) ;
   sdbMsg.reverse( sizeof( MsgAuthCrtUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthCrtUsr ) ) ;

   user = ( MsgAuthCrtUsr * )sdbMsg.data() ;
   user->header.opCode = MSG_AUTH_CRTUSR_REQ ;
   user->header.TID = 0 ;
   user->header.routeID.value = 0 ;
   user->header.requestID = packet.requestId ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( !document.hasField( FAP_CMD_ADMIN_PREFIX FAP_FIELD_NAME_DB ) )
         {
            continue ;
         }
         pPasswd = document.getStringField( FAP_FIELD_NAME_PWD ) ;
      }
   }

   md5 = md5::md5simpledigest( pPasswd ) ;

   obj = BSON( SDB_AUTH_USER << pName.c_str() <<
               SDB_AUTH_PASSWD << md5.c_str() <<
               SDB_AUTH_TEXTPASSWD << pPasswd.c_str() ) ;
   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

   return rc ;
}

INT32 createUserCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 dropUserCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

/** \fn INT32 buildMsg( msgParser& parser, msgBuffer &sdbMsg )
    \brief Build the drop user message of SequoiaDB
    \param [in] parser Parser object.
    \param [out] sdbMsg The message we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 dropUserCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   MsgAuthDelUsr *auth     = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *pName = packet.clName.c_str() ;

   parser.setCurrentOp( OP_CMD_DELUSER ) ;
   sdbMsg.reverse( sizeof( MsgAuthDelUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthDelUsr ) ) ;

   auth = ( MsgAuthDelUsr * )sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_DELUSR_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = packet.requestId ;

   sdbMsg.write( BSON( SDB_AUTH_USER << pName ), TRUE ) ;
   sdbMsg.doneLen() ;

   return SDB_OK ;
}

INT32 dropUserCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 saslStartCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

/** \fn INT32 buildMsg( msgParser& parser, msgBuffer &sdbMsg )
    \brief Build the first authentication message of SequoiaDB
    \param [in] parser Parser object.
    \param [out] sdbMsg The message we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 saslStartCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   INT32 payloadLen        = 0 ;
   MsgAuthentication *auth = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   CHAR *curStr            = NULL ;
   CHAR *lastParsed        = NULL ;
   const CHAR *payload     = NULL ;
   CHAR payloadCpy[FAP_AUTH_MSG_PAYLOADD_MAX_SIZE] = { 0 } ;
   const CHAR* nonce       = NULL ;
   bson::BSONObj obj ;

   parser.setCurrentOp( OP_CMD_AUTH_STEP1 ) ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_PAYLOAD ) )
         {
            // eg: payload = "n,,n=admin,r=xxx"
            payload = document.getField( FAP_FIELD_NAME_PAYLOAD ).binData(
                      payloadLen );
         }
      }
   }

   if ( payloadLen <= 0 || payloadLen > FAP_AUTH_MSG_PAYLOADD_MAX_SIZE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossStrncpy( payloadCpy, payload, payloadLen ) ;
   curStr = ossStrtok( payloadCpy, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;

   // get user name
   curStr = ossStrtok( lastParsed, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;
   packet.userName = ( curStr + 2 ) ;

   // get client nonce
   curStr = ossStrtok( lastParsed, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;
   nonce = ( curStr + 2 ) ;

   sdbMsg.reverse( sizeof( MsgAuthentication ) ) ;
   sdbMsg.advance( sizeof( MsgAuthentication ) ) ;

   auth = ( MsgAuthentication * ) sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = packet.requestId ;

   obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_1 <<
               SDB_AUTH_USER << packet.userName.c_str() <<
               SDB_AUTH_NONCE << nonce <<
               SDB_AUTH_TYPE << SDB_AUTH_TYPE_TEXT_PWD ) ;
   sdbMsg.write( obj, TRUE ) ;

   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 saslStartCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 saslContinueCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

/** \fn INT32 buildMsg( msgParser& parser, msgBuffer &sdbMsg )
    \brief Build the second authentication message of SequoiaDB
    \param [in] parser Parser object.
    \param [out] sdbMsg The message we will build.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
INT32 saslContinueCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                       = SDB_OK ;
   INT32 payloadLen               = 0 ;
   MsgAuthentication *auth        = NULL ;
   mongoDataPacket &packet        = parser.dataPacket() ;
   CHAR  *curStr                  = NULL ;
   CHAR  *lastParsed              = NULL ;
   const CHAR *payload            = NULL ;
   const CHAR *identify           = NULL ;
   const CHAR *clientProofBase64  = NULL ;
   const CHAR *combineNonceBase64 = NULL ;
   CHAR payloadCpy[FAP_AUTH_MSG_PAYLOADD_MAX_SIZE] = { 0 } ;
   bson::BSONObj obj ;

   sdbMsg.reverse( sizeof( MsgAuthentication ) ) ;
   sdbMsg.advance( sizeof( MsgAuthentication ) ) ;

   auth = ( MsgAuthentication * ) sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_VERIFY1_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = packet.requestId ;

   for ( UINT32 i = 0; i < packet.sections.size(); i++ )
   {
      for ( UINT32 j = 0; j < packet.sections[i].documents.size(); j++ )
      {
         bson::BSONObj document = packet.sections[i].documents[j] ;
         if ( document.hasField( FAP_FIELD_NAME_PAYLOAD ) )
         {
            payload = document.getField( FAP_FIELD_NAME_PAYLOAD ).binData(
                      payloadLen );
         }
      }
   }

   if ( 0 == payloadLen )
   {
      // eg: payload = ""
      parser.setCurrentOp( OP_CMD_AUTH_STEP3 ) ;
   }
   else
   {
      // eg: payload = "c=biws,r=xxxxxx,p=xxx"
      parser.setCurrentOp( OP_CMD_AUTH_STEP2 ) ;

      if ( payloadLen < 0 || payloadLen > FAP_AUTH_MSG_PAYLOADD_MAX_SIZE )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossStrncpy( payloadCpy, payload, payloadLen ) ;

      curStr = ossStrtok( payloadCpy, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;
      identify = ( curStr + 2 ) ;
      curStr = ossStrtok( lastParsed, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;
      combineNonceBase64 = ( curStr + 2 ) ;
      curStr = ossStrtok( lastParsed, FAP_UTIL_SYMBOL_COMMA, &lastParsed ) ;
      clientProofBase64 = ( curStr + 2 ) ;

      obj = BSON( SDB_AUTH_STEP << SDB_AUTH_STEP_2 <<
                  SDB_AUTH_USER << packet.userName <<
                  SDB_AUTH_NONCE << combineNonceBase64 <<
                  SDB_AUTH_IDENTIFY << identify <<
                  SDB_AUTH_PROOF << clientProofBase64 <<
                  SDB_AUTH_TYPE << SDB_AUTH_TYPE_TEXT_PWD ) ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

done:
   return rc ;
error:
   goto done ;
}

INT32 saslContinueCommand::doCommand( void *pData )
{
   return SDB_OK ;
}