/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "commands.hpp"
#include "msgBuffer.hpp"
#include "parser.hpp"
#include "msg.hpp"
#include "../../bson/lib/md5.hpp"
#include "msgDef.h"

DECLARE_COMMAND_VAR( insert )
DECLARE_COMMAND_VAR( delete )
DECLARE_COMMAND_VAR( update )
DECLARE_COMMAND_VAR( query )
DECLARE_COMMAND_VAR( getMore )
DECLARE_COMMAND_VAR( killCursors )

// other command
DECLARE_COMMAND_VAR( getnonce )
DECLARE_COMMAND_VAR( authenticate )
DECLARE_COMMAND_VAR( createUser )
DECLARE_COMMAND_VAR( dropUser )
DECLARE_COMMAND_VAR( listUsers )
DECLARE_COMMAND_VAR( create )
DECLARE_COMMAND_VAR( createCS )
DECLARE_COMMAND_VAR( listCollection )
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


void generateNonce( std::stringstream &ss )
{
   UINT64 ull = 0 ;
   ss.clear() ;
   ss << std::hex << ull ;
}

/// implement of commands
INT32 insertCommand::convert( msgParser &parser )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
   mongoDataPacket &packet = parser.dataPacket() ;

   if ( 0 != packet.optionMask && !packet.with( OPTION_CMD ) )
   {
      if ( packet.with( OPTION_IDX ) )
      {
         cmd = commandMgr::instance()->findCommand( "createIndexes" ) ;
      }
      else if ( packet.with( OPTION_USR ) )
      {
         cmd = commandMgr::instance()->findCommand( "createUser" ) ;
      }

      if ( NULL == cmd )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
         PD_LOG( PDERROR, "Unsupported command, rc: %d", rc ) ;
         goto error ;
      }

      rc = cmd->convert( parser ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to convert msg, rc: %d", rc ) ;
         goto error ;
      }

      goto done ;
   }

   if ( packet.with( OPTION_CMD ) )
   {
      // hit here means insert with write command
   }
   else
   {
      // one or more doc
   }


done:
   return rc ;
error:
   goto done ;
}

INT32 insertCommand::buildMsg( msgParser& parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
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
   insert->flags = 0 ;

   if ( packet.reservedInt & INSERT_CONTINUE_ON_ERROR )
   {
      insert->flags |= FLG_INSERT_CONTONDUP ;
   }

   try
   {
      if ( packet.with( OPTION_CMD ) )
      {
         const CHAR *clName = NULL ;
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         clName = packet.all.getStringField( "insert" ) ;
         if ( 0 == ossStrcmp( clName, "system.users" ) )
         {
            packet.optionMask |= OPTION_USR ;
            cmd = commandMgr::instance()->findCommand( "createUser" ) ;
            if ( NULL == cmd )
            {
               rc = SDB_OPTION_NOT_SUPPORT ;
               parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
               PD_LOG( PDERROR, "Unsupported command[cl: %s], rc: %d",
                       packet.fullName.c_str(), rc ) ;
               goto error ;
            }

            sdbMsg.zero() ;
            rc = cmd->buildMsg( parser, sdbMsg ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to build msg[cl: %s], rc: %d",
                       packet.fullName.c_str(), rc ) ;
               goto error ;
            }

            goto done ;
         }
         packet.fullName += packet.all.getStringField( "insert" ) ;
         insert->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(), insert->nameLength + 1, TRUE ) ;

         if ( !packet.all.getBoolField( "ordered" ) )
         {
            insert->flags |= FLG_INSERT_CONTONDUP ;
         }

         bson::BSONElement e = packet.all.getField( "documents" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of documents field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }
         {
            bson::BSONObjIterator it( e.Obj() ) ;
            while( it.more() )
            {
               bson::BSONElement be = it.next() ;
               sdbMsg.write( be.Obj(), TRUE ) ;
            }
         }
      }
      else
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg[cl: %s], rc: %d",
                    packet.fullName.c_str(), rc ) ;
            goto error ;
         }

         parser.readNextObj( packet.all ) ;

         insert->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(), insert->nameLength + 1, TRUE ) ;
         sdbMsg.write( packet.all, TRUE ) ;

         bson::BSONObj doc ;
         while ( parser.more() )
         {
            parser.readNextObj( doc ) ;
            sdbMsg.write( doc, TRUE ) ;
         }
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of insert command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of insert[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 insertCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 deleteCommand::convert( msgParser &parser )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
   mongoDataPacket &packet = parser.dataPacket() ;

   if ( packet.with( OPTION_USR ) )
   {
      cmd = commandMgr::instance()->findCommand( "dropUser" ) ;
      if ( NULL == cmd )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
         PD_LOG( PDERROR, "Unsupported command, rc: %d", rc ) ;
         goto error ;
      }

      rc = cmd->convert( parser ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to convert msg, rc: %d", rc ) ;
         goto error ;
      }

      goto done ;
   }

   if ( packet.with( OPTION_CMD ) )
   {
      //parser.skipBytes( sizeof( packet.nToSkip ) + sizeof( packet.nToReturn ) ) ;
   }
   else
   {
      // do nothing here
      parser.readInt( sizeof( INT32 ), (CHAR *)&packet.nToSkip ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 deleteCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
   MsgOpDelete *del        = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

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
   del->flags = 0 ;

   if ( packet.reservedInt & REMOVE_JUSTONE )
   {
      del->flags |= FLG_DELETE_ONE ;
   }

   try
   {
      if ( packet.with( OPTION_CMD ) )
      {
         const CHAR *clName = NULL ;
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         clName = packet.all.getStringField( "delete" ) ;
         if ( 0 == ossStrcmp( clName, "system.users" ) )
         {
            packet.optionMask |= OPTION_USR ;
            cmd = commandMgr::instance()->findCommand( "dropUser" ) ;
            if ( NULL == cmd )
            {
               rc = SDB_OPTION_NOT_SUPPORT ;
               parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
               PD_LOG( PDERROR, "Unsupported command[cl: %s], rc: %d",
                       packet.fullName.c_str(), rc ) ;
               goto error ;
            }

            sdbMsg.zero() ;
            rc = cmd->buildMsg( parser, sdbMsg ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to build msg[cl: %s], rc: %d",
                       packet.fullName.c_str(), rc ) ;
               goto error ;
            }

            goto done ;
         }
         packet.fullName += packet.all.getStringField( "delete" ) ;
         del->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(), del->nameLength + 1, TRUE ) ;

         bson::BSONElement e = packet.all.getField( "deletes" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of deletes field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         std::vector< bson::BSONElement > objList ;
         std::vector< bson::BSONElement >::const_iterator cit ;
         bson::BSONObj obj, cond, hint ;
         bson::BSONObj subObj ;
         objList = e.Array() ;
         cit = objList.begin() ;
         while ( objList.end() != cit )
         {
            subObj = (*cit).Obj() ;
            obj = subObj.getObjectField( "q" ) ;
            cond = getQueryObj( obj ) ;
            hint = getHintObj( obj ) ;
            sdbMsg.write( cond, TRUE ) ;
            sdbMsg.write( hint, TRUE ) ;
            ++cit ;
         }
      }
      else
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg[cl: %s], rc: %d",
                    packet.fullName.c_str(), rc ) ;
            goto error ;
         }

         parser.readNextObj( packet.all ) ;

         del->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(), del->nameLength + 1, TRUE ) ;

         bson::BSONObj cond, hint ;
         cond = getQueryObj( packet.all ) ;
         hint = getHintObj( packet.all ) ;
         sdbMsg.write( cond, TRUE ) ;
         sdbMsg.write( hint, TRUE ) ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of delete command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of delete[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 deleteCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 updateCommand::convert( msgParser &parser )
{
   INT32 rc                = SDB_OK ;
   baseCommand *&cmd       = parser.command() ;
   mongoDataPacket &packet = parser.dataPacket() ;

   if ( packet.with( OPTION_USR ) )
   {
      cmd = commandMgr::instance()->findCommand( "updataUser" ) ;
      if ( NULL == cmd )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
         PD_LOG( PDERROR, "Unsupported command, rc: %d", rc ) ;
         goto error ;
      }

      rc = cmd->convert( parser ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to convert msg, rc: %d", rc ) ;
         goto error ;
      }

      goto done ;
   }

   if ( packet.with( OPTION_CMD ) )
   {
      //parser.skipBytes( sizeof( packet.nToSkip ) + sizeof( packet.nToReturn ) ) ;
   }
   else
   {
      // in update option, nToSkip is used as updateFlags
      parser.readInt( sizeof( INT32 ), (CHAR *)&packet.nToSkip ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 updateCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpUpdate *update     = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
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
   update->flags = 0 ;

   try
   {
      if ( packet.with( OPTION_CMD ) )
      {
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         packet.fullName += packet.all.getStringField( "update" ) ;
         update->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(),
                       packet.fullName.length() + 1, TRUE ) ;

         bson::BSONElement e = packet.all.getField( "updates" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of updates filed must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         bson::BSONObj obj, subObj, cond, updator, hint ;
         std::vector< bson::BSONElement > objList ;
         std::vector< bson::BSONElement >::const_iterator cit ;
         objList = e.Array() ;
         cit = objList.begin() ;
         while ( objList.end() != cit )
         {
            subObj = (*cit).Obj() ;
            obj = subObj.getObjectField( "q" ) ;
            //cond = getQueryObj( obj ) ;
            updator = subObj.getObjectField( "u" ) ;
            hint = getHintObj( obj ) ;

            if( updator.nFields() > 0 &&
                updator.firstElement().fieldName()[0] != '$' )
            {
               updator = BSON( "$replace" << updator ) ;
            }

            if ( false == subObj.getBoolField( "multi" ) )
            {
               update->flags |= FLG_UPDATE_ONE ;
            }
            if ( subObj.getBoolField( "upsert" ) )
            {
               update->flags |= FLG_UPDATE_UPSERT ;
            }

            sdbMsg.write( obj, TRUE ) ;
            sdbMsg.write( updator, TRUE ) ;
            sdbMsg.write( hint, TRUE ) ;
            ++cit ;
         }
      }
      else
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg[cl: %s], rc: %d",
                    packet.fullName.c_str(), rc ) ;
            goto error ;
         }

         parser.readNextObj( packet.all ) ;

         update->nameLength = packet.fullName.length() ;
         sdbMsg.write( packet.fullName.c_str(), update->nameLength + 1, TRUE ) ;
         // in update option, nToSkip is used as updateFlags
         if ( packet.nToSkip & UPDATE_UPSERT )
         {
            update->flags |= FLG_UPDATE_UPSERT ;
         }
         if ( ! ( packet.nToSkip & UPDATE_MULTI ) )
         {
            update->flags |= FLG_UPDATE_ONE ;
         }

         bson::BSONObj cond, updator, hint ;
         if ( !parser.more() )
         {
            // lack of updator object
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg[cl: %s], rc: %d",
                    packet.fullName.c_str(), rc ) ;
            goto error ;
         }
         parser.readNextObj( updator ) ;
         cond = getQueryObj( packet.all ) ;
         hint = getHintObj( packet.all ) ;

         if( updator.nFields() > 0 &&
             updator.firstElement().fieldName()[0] != '$' )
         {
            updator = BSON( "$replace" << updator ) ;
         }

         sdbMsg.write( cond, TRUE ) ;
         sdbMsg.write( updator, TRUE ) ;
         sdbMsg.write( hint, TRUE ) ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of update command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of update[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
      PD_LOG( PDERROR, "Invalid length of msg, rc: %d", rc ) ;
      goto error ;
   }

   try
   {
      parser.readNextObj( packet.all ) ;

      if ( 0 !=  packet.optionMask )
      {
         if ( packet.with( OPTION_IDX ) )
         {
            cmd = commandMgr::instance()->findCommand( "listIndexes" ) ;
         }
         else if ( packet.with( OPTION_CLS ) )
         {
            cmd = commandMgr::instance()->findCommand( "listCollection" ) ;
         }
         else if ( packet.with( OPTION_USR ) )
         {
            cmd = commandMgr::instance()->findCommand( "listUsers" ) ;
         }
         else if ( packet.with( OPTION_CMD ) )
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
            PD_LOG( PDERROR, "Unsupported command, rc: %d", rc ) ;
            goto error ;
         }

         rc = cmd->convert( parser ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to convert msg, rc: %d", rc ) ;
            goto error ;
         }

         goto done ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Convert query msg exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
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
   bson::BSONObj cond, orderby, hint ;

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
   setQueryFlags( packet.reservedInt, query->flags ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;
   query->nameLength = packet.fullName.length() ;

   try
   {
      if ( packet.all.hasField( "limit" ) )
      {
         query->numToReturn = packet.all.getIntField( "limit" ) ;
      }

      if ( packet.all.hasField( "skip" ) )
      {
         query->numToSkip = packet.all.getIntField( "skip" ) ;
      }

      if ( packet.all.getBoolField( "$explain" ) )
      {
         query->flags |= FLG_QUERY_EXPLAIN ;
      }

      if ( parser.more() )
      {
         parser.readNextObj( packet.fieldToReturn ) ;
      }

      cond = getQueryObj( packet.all ) ;
      orderby = getSortObj( packet.all ) ;
      hint = getHintObj( packet.all ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of query command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( packet.fullName.c_str(), query->nameLength + 1, TRUE ) ;
   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( packet.fieldToReturn, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;
   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of query[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   INT32 nToReturn         = 0 ;
   MsgOpGetMore *more      = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_GETMORE ) ;
   sdbMsg.reverse( sizeof( MsgOpGetMore ) ) ;
   sdbMsg.advance( sizeof( MsgOpGetMore ) ) ;

   more = ( MsgOpGetMore * )sdbMsg.data() ;
   more->header.opCode = MSG_BS_GETMORE_REQ ;
   more->header.TID = 0 ;
   more->header.routeID.value = 0 ;
   more->header.requestID = packet.requestId ;

   parser.readInt( sizeof( nToReturn ), ( CHAR * )&nToReturn ) ;
   packet.nToReturn = nToReturn < 0 ? -nToReturn : nToReturn ;
   if ( 0 == packet.nToReturn )
   {
      packet.nToReturn = -1 ;
   }
   more->numToReturn = packet.nToReturn ;
   parser.readInt( sizeof( packet.cursorId ), ( CHAR * )&packet.cursorId ) ;
   // match to sequoiadb contextID, need decrease 1
   more->contextID = packet.cursorId - 1;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of getMore[cl: %s, contextID: %llu] "
           "command done", packet.fullName.c_str(), packet.cursorId ) ;

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
   INT32 nContext          = 0 ;
   INT64 cursorId          = 0 ;
   MsgOpKillContexts *kill = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_KILLCURSORS ) ;
   sdbMsg.reverse( sizeof( MsgOpKillContexts ) ) ;
   sdbMsg.advance( sizeof( MsgOpKillContexts ) - sizeof( SINT64 ) ) ;

   kill = ( MsgOpKillContexts * )sdbMsg.data() ;
   kill->header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
   kill->header.TID = 0 ;
   kill->header.routeID.value = 0 ;
   kill->header.requestID = packet.requestId ;
   kill->ZERO = 0 ;

   parser.readInt( sizeof( INT32 ), ( CHAR * )&nContext ) ;
   kill->numContexts = nContext ;

   while ( 0 < nContext )
   {
      cursorId = 0 ;
      parser.readInt( sizeof( SINT64 ), ( CHAR *)&cursorId ) ;
      if ( 0 != cursorId )
      {
         cursorId -= 1 ;
         sdbMsg.write( ( CHAR * )&cursorId, sizeof( SINT64 ) ) ;
      }
      --nContext ;
   }

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of killCursors command done" ) ;

   return rc ;
}

INT32 killCursorsCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 getnonceCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 getnonceCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   bson::BSONObjBuilder bob ;
   std::stringstream ss ;

   parser.setCurrentOp( OP_CMD_GETNONCE ) ;

   try
   {
      generateNonce( ss ) ;
      bob.append( "nonce", ss.str() ) ;
      sdbMsg.write( bob.obj(), TRUE ) ;

      PD_LOG( PDDEBUG, "Build the msg of getnonce command done" ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of getnonce command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 getnonceCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 authenticateCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 authenticateCommand::buildMsg( msgParser &parser,  msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgAuthentication *auth = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   bson::BSONObj obj ;

   parser.setCurrentOp( OP_CMD_AUTH ) ;
   sdbMsg.reverse( sizeof( MsgAuthentication ) ) ;
   sdbMsg.advance( sizeof( MsgAuthentication ) ) ;

   auth = ( MsgAuthentication * ) sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_VERIFY_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = packet.requestId ;

   try
   {
      packet.userName = packet.all.getStringField( "user" ) ;
      packet.password = packet.all.getStringField( "key" ) ;
      obj = BSON( SDB_AUTH_USER << packet.userName.c_str() <<
                  SDB_AUTH_PASSWD << packet.password.c_str() ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of auth command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of auth command done" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 authenticateCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 createUserCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 createUserCommand::buildMsg( msgParser& parser, msgBuffer &sdbMsg )
{
   INT32 rc = SDB_OK ;
   MsgAuthCrtUsr *user = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;

   parser.setCurrentOp( OP_CMD_CRTUSER ) ;
   sdbMsg.reverse( sizeof( MsgAuthCrtUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthCrtUsr ) ) ;

   user = ( MsgAuthCrtUsr * )sdbMsg.data() ;
   user->header.opCode = MSG_AUTH_CRTUSR_REQ ;
   user->header.TID = 0 ;
   user->header.routeID.value = 0 ;
   user->header.requestID = packet.requestId ;

   bson::BSONObj obj, cond ;
   const CHAR *pName = NULL ;
   const CHAR *pPasswd = NULL ;

   try
   {
      if ( packet.with( OPTION_CMD ) && packet.with( OPTION_USR ) )
      {
         bson::BSONElement e = packet.all.getField( "documents" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of documents field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }
         {
            bson::BSONObjIterator it( e.Obj() ) ;
            while( it.more() )
            {
               bson::BSONElement be = it.next() ;
               cond = be.Obj() ;
               break ;
            }
            pName = cond.getStringField( "user" ) ;
            pPasswd = cond.getStringField( "pwd" ) ;
         }
      }
      else if ( packet.with( OPTION_CMD ) )
      {
         bson::BSONObj cond = packet.all.getObjectField( "createUser" ) ;
         pName = cond.getStringField( "user" ) ;
         pPasswd = cond.getStringField( "pwd" ) ;
      }
      else
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg, rc: %d", rc ) ;
            goto error ;
         }

         parser.readNextObj( packet.all ) ;

         pName = packet.all.getStringField( "user" ) ;
         pPasswd = packet.all.getStringField( "pwd" ) ;
      }
      // build md5
      {
         std::stringstream ss ;
         generateNonce( ss ) ;
         md5::md5digest d ;
         md5_state_t st ;
         md5_init( &st ) ;
         md5_append( &st, ( const md5_byte_t * )ss.str().c_str(),
                     ossStrlen( ss.str().c_str() ) ) ;
         md5_append( &st, ( const md5_byte_t * )pName, ossStrlen( pName ) ) ;
         md5_append( &st, ( const md5_byte_t * )pPasswd,
                     ossStrlen( pPasswd ) ) ;
         md5_finish( &st, d ) ;

         obj = BSON( SDB_AUTH_USER << pName <<
                     SDB_AUTH_PASSWD << md5::digestToString( d ).c_str() ) ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of createUser command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of createUser command done" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 createUserCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 dropUserCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 dropUserCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgAuthDelUsr *auth     = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR* pUserName   = NULL ;

   parser.setCurrentOp( OP_CMD_DELUSER ) ;
   sdbMsg.reverse( sizeof( MsgAuthDelUsr ) ) ;
   sdbMsg.advance( sizeof( MsgAuthDelUsr ) ) ;

   auth = ( MsgAuthDelUsr * )sdbMsg.data() ;
   auth->header.opCode = MSG_AUTH_DELUSR_REQ ;
   auth->header.TID = 0 ;
   auth->header.routeID.value = 0 ;
   auth->header.requestID = packet.requestId ;

   {
      INT32 removeFlags = 0 ;
      parser.readInt( sizeof( INT32 ), (CHAR *)&removeFlags ) ;
   }

   try
   {
      if ( packet.with( OPTION_CMD ) && packet.with( OPTION_USR ) )
      {
         bson::BSONElement e = packet.all.getField( "deletes" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of deletes field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         std::vector< bson::BSONElement > objList ;
         std::vector< bson::BSONElement >::const_iterator cit ;
         bson::BSONObj cond, subObj ;
         objList = e.Array() ;
         cit = objList.begin() ;
         while ( objList.end() != cit )
         {
            subObj = (*cit).Obj() ;
            cond = subObj.getObjectField( "q" ) ;
            break ;
         }
         pUserName = cond.getStringField( "user" ) ;
      }
      else if ( packet.with( OPTION_CMD ) )
      {
         pUserName = packet.all.getStringField( "dropUser" ) ;
      }
      else
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg, rc: %d", rc ) ;
            goto error ;
         }

         parser.readNextObj( packet.all ) ;

         pUserName = packet.all.getStringField( "user" ) ;
      }

      if ( 0 != ossStrcmp( pUserName, packet.userName.c_str() ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Can not drop user[%s], only current user[%s] can be dropped",
                 pUserName, packet.userName.c_str() ) ;
         goto error ;
      }

      sdbMsg.write( BSON( SDB_AUTH_USER << pUserName <<
                          SDB_AUTH_PASSWD << packet.password.c_str() ), TRUE ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of dropUser command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of dropUser command done" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 dropUserCommand::doCommand( void *pData )
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
   setQueryFlags( packet.reservedInt, query->flags ) ;

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

   PD_LOG( PDDEBUG, "Build the msg of listUsers command done" ) ;

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
   bson::BSONObj obj, empty ;

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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;
   query->nameLength = ossStrlen( cmdName ) ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      obj = BSON( FIELD_NAME_NAME << packet.csName
                                  << FIELD_NAME_PAGE_SIZE << 65536 ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of createCS command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;    // condition
   sdbMsg.write( empty, TRUE ) ;  // selector
   sdbMsg.write( empty, TRUE ) ;  // orderby
   sdbMsg.write( empty, TRUE ) ;  // hint

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of createCS command[cs: %s] done",
           packet.csName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   bson::BSONObj obj, empty ;

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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   try
   {
      if ( packet.with( OPTION_CMD ) )
      {
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         packet.fullName += packet.all.getStringField( "create" ) ;
      }

      sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

      obj = BSON( FIELD_NAME_NAME << packet.fullName.c_str() ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of createCL command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of createCL command[cl: %s] done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 createCommand::doCommand( void *pData )
{
   return SDB_OK ;
}

INT32 listCollectionCommand::convert( msgParser &parser )
{
   return SDB_OK ;
}

INT32 listCollectionCommand::buildMsg( msgParser &parser, msgBuffer &sdbMsg )
{
   INT32 rc                = SDB_OK ;
   MsgOpQuery *query       = NULL ;
   mongoDataPacket &packet = parser.dataPacket() ;
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;
   bson::BSONObj cond, selector, orderby, hint ;

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
   query->flags = 0 ;
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      if ( parser.more() )
      {
         parser.readNextObj( packet.fieldToReturn ) ;
      }
      selector = packet.fieldToReturn ;
      cond = getQueryObj( packet.all ) ;
      orderby = getSortObj( packet.all ) ;
      hint = getHintObj( packet.all ) ;

      query->numToSkip = getFieldInt( packet.all, "skip" ) ;
      query->numToReturn = getFieldInt( packet.all, "limit" ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of listCollections command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   if ( 0 == query->numToReturn )
   {
      query->numToReturn = -1 ;
   }

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( selector, TRUE ) ;
   sdbMsg.write( orderby, TRUE ) ;
   sdbMsg.write( hint, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of listCollections command done" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 listCollectionCommand::doCommand( void *pData )
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
   bson::BSONObj obj, empty ;

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
   query->flags = 0 ;
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      packet.fullName = packet.csName ;
      packet.fullName += "." ;
      packet.fullName += packet.all.getStringField( "drop" ) ;
      obj = BSON( FIELD_NAME_NAME << packet.fullName.c_str() ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of dropCL command exception: %s, "
               "rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;    // condition
   sdbMsg.write( empty, TRUE ) ;  // selector
   sdbMsg.write( empty, TRUE ) ;  // orderby
   sdbMsg.write( empty, TRUE ) ;  // hint

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of dropCL[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   bson::BSONObj cond, obj, orderby, empty ;

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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      packet.fullName = packet.csName ;
      packet.fullName += "." ;
      packet.fullName += packet.all.getStringField( "count" ) ;

      cond = packet.all.getObjectField( "query" ) ;
      obj = BSON( FIELD_NAME_COLLECTION << packet.fullName.c_str() ) ;

      orderby = packet.all.getObjectField( "sort" ) ;
      if ( packet.all.hasField( "limit" ) )
      {
         query->numToReturn = packet.all.getIntField( "limit" ) ;
      }

      if ( packet.all.hasField( "skip" ) )
      {
         query->numToSkip = packet.all.getIntField( "skip" ) ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of count command exception: %s, rc: %d",
               ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( cond, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( obj, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of count[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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

   try
   {
      packet.fullName = packet.csName ;
      packet.fullName += "." ;
      packet.fullName += packet.all.getStringField( "aggregate" ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of aggregate command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   aggre->nameLength = packet.fullName.length() ;
   sdbMsg.write( packet.fullName.c_str(), aggre->nameLength + 1, TRUE ) ;

   sdbMsg.write( packet.all, TRUE ) ;
   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of aggregate[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   bson::BSONObj obj, empty ;

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
   query->flags = 0 ;
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      obj = BSON( FIELD_NAME_NAME << packet.csName ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of dropDatabase command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of dropDatabase[cs: %s] command done",
           packet.csName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   const CHAR *cmdName = CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX ;

   parser.setCurrentOp( OP_ENSURE_INDEX ) ;
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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj obj, subObj, indexObj, empty ;
   bson::BSONObjBuilder bob ;
   bson::BSONElement e ;
   std::vector< bson::BSONElement > objList ;
   std::vector< bson::BSONElement >::const_iterator cit ;

   try
   {
      if ( packet.with( OPTION_CMD ) )
      {
         packet.fullName = packet.csName ;
         packet.fullName += "." ;
         packet.fullName += packet.all.getStringField( "createIndexes" ) ;
         bob.append( FIELD_NAME_COLLECTION, packet.fullName.c_str() ) ;

         e = packet.all.getField( "indexes" ) ;
         if( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of indexes field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         objList = e.Array() ;
         cit = objList.begin() ;
         while ( objList.end() != cit )
         {
            subObj = (*cit).Obj() ;
            indexObj = BSON( "key" << subObj.getObjectField( "key" ) <<
                             "name" << subObj.getStringField( "name") <<
                             "unique" << subObj.getBoolField( "unique" ) );
            bob.append( "Index", indexObj ) ;
            sdbMsg.write( bob.obj(), TRUE ) ;
            ++cit ;
         }
      }
      else if ( packet.with( OPTION_IDX ) )
      {
         if ( !parser.more() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid length of msg, rc: %d", rc ) ;
            goto error ;
         }
         parser.readNextObj( packet.all ) ;

         packet.fullName = packet.all.getStringField( "ns" ) ;
         bob.append( FIELD_NAME_COLLECTION, packet.fullName.c_str() ) ;

         indexObj = BSON( "key" << packet.all.getObjectField( "key" ) <<
                          "name" << packet.all.getStringField( "name") <<
                          "unique" << packet.all.getBoolField( "unique" ) );
         bob.append( "Index", indexObj ) ;
         sdbMsg.write( bob.obj(), TRUE ) ;
      }
      else
      {
         e = packet.all.getField( "documents" ) ;
         if ( bson::Array != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The type of documents field must be array, "
                    "rc: %d", rc ) ;
            goto error ;
         }

         objList = e.Array() ;
         cit = objList.begin() ;
         while ( objList.end() != cit )
         {
            subObj = (*cit).Obj() ;
            bob.append( FIELD_NAME_COLLECTION, subObj.getStringField("ns") ) ;
            indexObj = BSON( "key" << subObj.getObjectField( "key" ) <<
                             "name" << subObj.getStringField( "name") <<
                             "unique" << subObj.getBoolField( "unique" ) );
            bob.append( "Index", indexObj ) ;
            sdbMsg.write( bob.obj(), TRUE ) ;
         }
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of createIndexes command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of createIndexes[cl: %s] command done",
           packet.fullName.c_str() ) ;

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
   bson::BSONObj obj, indexObj, empty ;

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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   try
   {
      packet.fullName = packet.csName ;
      packet.fullName += "." ;
      packet.fullName += packet.all.getStringField( "dropIndexes" ) ;

      indexObj = BSON( "" << packet.all.getStringField( "index" ) ) ;
      obj = BSON( FIELD_NAME_COLLECTION << packet.fullName.c_str() <<
                  FIELD_NAME_INDEX << indexObj ) ;
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of deleteIndexes command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( obj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of deleteIndexes[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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
   INT32 rc = SDB_OK ;
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
   setQueryFlags( packet.reservedInt, query->flags ) ;

   query->nameLength = ossStrlen( cmdName ) ;
   query->numToSkip = packet.nToSkip ;
   query->numToReturn = packet.nToReturn ;

   sdbMsg.write( cmdName, query->nameLength + 1, TRUE ) ;

   bson::BSONObj obj, cond, indexObj, empty ;

   try
   {
      if ( packet.with( OPTION_IDX ) )
      {
         cond = getQueryObj( packet.all ) ;
         if( !cond.isEmpty() )
         {
            if ( cond.hasField( "index" ) )
            {
               indexObj = BSON( "indexDef.name" <<
                                cond.getStringField( "index" ) ) ;
            }
            obj = BSON( FIELD_NAME_COLLECTION << cond.getStringField( "ns" ) ) ;
         }
         else
         {
            if ( packet.all.hasField( "index" ) )
            {
               indexObj = BSON( "indexDef.name" <<
                                packet.all.getStringField( "index" ) ) ;
            }
            obj = BSON( FIELD_NAME_COLLECTION <<
                        packet.all.getStringField( "ns" ) ) ;
         }
      }
      else if ( packet.with( OPTION_CMD ) )
      {
         rc = SDB_OPTION_NOT_SUPPORT ;
         parser.setCurrentOp( OP_CMD_NOT_SUPPORTED ) ;
         PD_LOG( PDERROR, "Unsupported command, rc: %d", rc ) ;
         goto error ;
      }
   }
   catch ( std::exception &ex )
   {
      rc = ossException2RC( &ex ) ;
      PD_LOG ( PDERROR, "Build the msg of listIndexes command exception: "
               "%s, rc: %d", ex.what(), rc ) ;
      goto error ;
   }

   sdbMsg.write( indexObj, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( empty, TRUE ) ;
   sdbMsg.write( obj, TRUE ) ;

   sdbMsg.doneLen() ;

   PD_LOG( PDDEBUG, "Build the msg of listIndexes[cl: %s] command done",
           packet.fullName.c_str() ) ;

done:
   return rc ;
error:
   goto done ;
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

