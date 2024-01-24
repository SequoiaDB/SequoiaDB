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

   Source File Name = commands.hpp

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
#ifndef _SDB_FAP_MONGO_COMMANDS_HPP_
#define _SDB_FAP_MONGO_COMMANDS_HPP_

#include "baseCommand.hpp"

#define __DECLARE_COMMAND( cmd, secondName, clsName )                \
class clsName : public baseCommand                                   \
{                                                                    \
public:                                                              \
   clsName() : baseCommand( cmd, secondName )                        \
   {}                                                                \
                                                                     \
   virtual INT32 convert( msgParser &parser ) ;                      \
                                                                     \
   virtual INT32 buildMsg( msgParser &parser, msgBuffer &sdbMsg ) ;  \
                                                                     \
   virtual INT32 doCommand( void *pData = NULL ) ;                   \
} ;

#define __DECLARE_COMMAND_VAR( commandClass, var )                \
      commandClass var ;

#define DECLARE_COMMAND( command )                                \
      __DECLARE_COMMAND( #command, NULL, command##Command )

#define DECLARE_COMMAND_ALAIS( command, secondName )              \
      __DECLARE_COMMAND( #command, secondName, command##Command)

#define DECLARE_COMMAND_VAR( command )                            \
      __DECLARE_COMMAND_VAR( command##Command, command##Cmd )

///////////////////////////////////////////////////////////////////
// declare all command supported

DECLARE_COMMAND( insert )
DECLARE_COMMAND( delete )
DECLARE_COMMAND( update )
DECLARE_COMMAND( query )
DECLARE_COMMAND( find )
DECLARE_COMMAND( getMore )
DECLARE_COMMAND( killCursors )
DECLARE_COMMAND( msg )
DECLARE_COMMAND( listUsers )
DECLARE_COMMAND( create ) // createCollection
DECLARE_COMMAND( createCS )
DECLARE_COMMAND( listCollections )
DECLARE_COMMAND( drop )
DECLARE_COMMAND( count )
DECLARE_COMMAND( aggregate )
DECLARE_COMMAND( dropDatabase )
DECLARE_COMMAND( createIndexes )
DECLARE_COMMAND_ALAIS( deleteIndexes, "dropIndexes")
DECLARE_COMMAND( listIndexes )
DECLARE_COMMAND( getlasterror )
DECLARE_COMMAND_ALAIS( ismaster, "isMaster" )
DECLARE_COMMAND( ping )
DECLARE_COMMAND( logout )
DECLARE_COMMAND( getLog )
DECLARE_COMMAND( whatsmyuri )
DECLARE_COMMAND_ALAIS( buildinfo, "buildInfo" )
DECLARE_COMMAND( distinct )
DECLARE_COMMAND( listDatabases )
DECLARE_COMMAND( createUser )
DECLARE_COMMAND( dropUser )
DECLARE_COMMAND( saslStart )
DECLARE_COMMAND( saslContinue )

#endif
