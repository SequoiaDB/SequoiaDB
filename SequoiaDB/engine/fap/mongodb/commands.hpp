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

   Source File Name = commands.hpp

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


DECLARE_COMMAND( insert )
DECLARE_COMMAND( delete )
DECLARE_COMMAND( update )
DECLARE_COMMAND( query )
DECLARE_COMMAND( getMore )
DECLARE_COMMAND( killCursors )

DECLARE_COMMAND( getnonce )
DECLARE_COMMAND( authenticate )
DECLARE_COMMAND( createUser )
DECLARE_COMMAND( dropUser )
DECLARE_COMMAND( listUsers )
DECLARE_COMMAND( create )
DECLARE_COMMAND( createCS )
DECLARE_COMMAND( listCollection )
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

#endif
