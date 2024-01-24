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

   Source File Name = sdbOmTool.cpp

   Descriptive Name = sdbOmTool Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbstop,
   which is used to stop SequoiaDB engine.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/27/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "omToolCmdBase.hpp"
#include "utilCommon.hpp"

namespace omTool
{
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      omToolOptions options ;
      omToolCmdBase *cmd = NULL ;

      rc = options.parse( argc, argv ) ;
      if( rc )
      {
         goto error ;
      }

      if( options.hasHelp() )
      {
         options.print() ;
         goto done ;
      }

      if( options.hasVersion() )
      {
         ossPrintVersion( "sdbomtool" ) ;
         goto done ;
      }

      cmd = getOmToolCmdBuilder()->create( options.mode().c_str() ) ;
      if( NULL == cmd )
      {
         rc = SDB_INVALIDARG ;
         cout << "unreconigzed mode: " << options.mode() << endl ;
         goto error ;
      }

      cmd->setOptions( &options ) ;

      rc = cmd->doCommand() ;
      if( rc )
      {
         goto error ;
      }

   done:
      if( cmd )
      {
         getOmToolCmdBuilder()->release( cmd ) ;
         cmd = NULL ;
      }
      return SDB_OK == rc ? 0 : engine::utilRC2ShellRC( rc ) ;
   error:
      goto done ;
   }
}

INT32 main( INT32 argc, CHAR **argv )
{
   return omTool::mainEntry( argc, argv ) ;
}



