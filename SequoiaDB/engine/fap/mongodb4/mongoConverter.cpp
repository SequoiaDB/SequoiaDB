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

   Source File Name = mongoConverter.cpp

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
#include "mongoConverter.hpp"
#include "mongodef.hpp"
#include "commands.hpp"
#include "mongoReplyHelper.hpp"

INT32 mongoConverter::convert( msgBuffer &out )
{
   INT32 rc = SDB_OK ;
   baseCommand *&cmd = _parser.command() ;
   _parser.extractMsg( _msgdata, _msglen ) ;

   // convert mongodb msg to sequoiadb msg
   // for all kinds of requests available
   commandMgr *cmdMgr = commandMgr::instance() ;
   if ( NULL == cmdMgr )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( dbInsert == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_INSERT ) ;
   }
   else if ( dbDelete == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_DELETE ) ;
   }
   else if ( dbUpdate == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_UPDATE ) ;
   }
   else if ( dbQuery == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_QUERY ) ;
   }
   else if ( dbGetMore == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_GETMORE ) ;
   }
   else if ( dbKillCursors == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_KILLCURSORS ) ;
   }
   else if ( dbMsg == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_MSG ) ;
   }

   if ( NULL == cmd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      goto error ;
   }

   try
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
   }
   catch ( std::exception &e )
   {
      PD_RC_CHECK( SDB_SYS, PDERROR,
                   "Failed to process command: %s, exception occurred: %s",
                   cmd->name(), e.what() ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

