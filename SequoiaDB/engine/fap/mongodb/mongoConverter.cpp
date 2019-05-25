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

   Source File Name = mongoConverter.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoConverter.hpp"
#include "mongodef.hpp"
#include "commands.hpp"
#include "mongoReplyHelper.hpp"

INT32 mongoConverter::convert( msgBuffer &out )
{
   INT32 rc = SDB_OK ;
   baseCommand *&cmd = _parser.command() ;
   _parser.extractMsg( _msgdata, _msglen ) ;

   commandMgr *cmdMgr = commandMgr::instance() ;
   if ( NULL == cmdMgr )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( dbInsert == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "insert" ) ;
   }
   else if ( dbDelete == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "delete" ) ;
   }
   else if ( dbUpdate == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "update" ) ;
   }
   else if ( dbQuery == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "query" ) ;
   }
   else if ( dbGetMore == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "getMore" ) ;
   }
   else if ( dbKillCursors == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "killCursors" ) ;
   }

   if ( NULL == cmd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      goto error ;
   }

   rc = cmd->convert( _parser ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = cmd->buildMsg( _parser, out ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 mongoConverter::reConvert( msgBuffer &out, MsgOpReply *reply )
{
   INT32 rc = SDB_OK ;
   INT32 numToReturn = -1 ;
   baseCommand *&cmd = _parser.command() ;
   commandMgr *cmdMgr = commandMgr::instance() ;
   if ( NULL == cmdMgr )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( OP_CMD_GET_INDEX == _parser.currentOption() ||
        OP_CMD_GET_CLS == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags )
      {
         rc = reply->flags ;
         goto done ;
      }
      else
      {
         MsgOpQuery* query = (MsgOpQuery *)out.data() ;
         numToReturn = query->numToReturn ;

         if ( 0 == reply->numReturned && 0 != reply->contextID )
         {
            out.zero() ;
            fap::mongo::buildGetMoreMsg( out ) ;
            MsgOpGetMore *msg = ( MsgOpGetMore *)out.data() ;
            msg->header.requestID = reply->header.requestID ;
            msg->contextID = reply->contextID ;
            msg->numToReturn = numToReturn ;
            goto done ;
         }
      }
   }

   out.zero() ;

   if ( OP_CMD_COUNT == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags )
      {
         rc = reply->flags ;
         goto done ;
      }

      numToReturn = 1 ;
      _parser.setCurrentOp( OP_CMD_COUNT_MORE );

      fap::mongo::buildGetMoreMsg( out ) ;
      MsgOpGetMore *msg = ( MsgOpGetMore* )out.data() ;
      msg->header.requestID = reply->header.requestID ;
      msg->contextID = reply->contextID ;
      msg->numToReturn = numToReturn ;
      goto done ;
   }

   if ( OP_CMD_CREATE == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags && SDB_DMS_CS_NOTEXIST == reply->flags )
      {
         _parser.reparse() ;
         cmd = cmdMgr->findCommand( "createCS" ) ;
         if ( NULL != cmd )
         {
            rc = cmd->convert( _parser ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rc = cmd->buildMsg( _parser, out ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            goto done ;
         }
      }
      else
      {
         rc = reply->flags ;
         goto error ;
      }
   }

   if ( OP_CMD_CREATE_CS == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags && SDB_DMS_CS_EXIST != reply->flags )
      {
         rc = reply->flags ;
         goto error ;
      }

      _parser.reparse() ;
      cmd = cmdMgr->findCommand( "create" ) ;
      if ( NULL != cmd )
      {
         rc = cmd->convert( _parser ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = cmd->buildMsg( _parser, out ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         goto done ;
      }
   }

   rc = reply->flags ;

done:
   return rc ;
error:
   goto done ;
}
