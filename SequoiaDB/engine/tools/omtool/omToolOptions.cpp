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

   Source File Name = omtCmdHost.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "omToolOptions.hpp"
#include <iostream>

using namespace std;

namespace omTool
{
   /*
      OPTION DEFINE
   */
   #define OMTOOL_OPTION_HELP             "help"
   #define OMTOOL_OPTION_VERSION          "version"
   #define OMTOOL_OPTION_MODE             "mode"
   #define OMTOOL_OPTION_HOSTNAME         "hostname"
   #define OMTOOL_OPTION_IP               "ip"
   #define OMTOOL_OPTION_PATH             "path"
   #define OMTOOL_OPTION_USER             "username"


   omToolOptions::omToolOptions()
   {
   }

   omToolOptions::~omToolOptions()
   {
   }

   BOOLEAN omToolOptions::hasHelp()
   {
      return has( OMTOOL_OPTION_HELP ) ;
   }

   BOOLEAN omToolOptions::hasVersion()
   {
      return has( OMTOOL_OPTION_VERSION ) ;
   }

   INT32 omToolOptions::parse( INT32 argc, CHAR* argv[] )
   {
      INT32 rc = SDB_OK ;

      engine::utilOptions::addOptions( "General options" )
         (OMTOOL_OPTION_HELP",h",      /* no arg */         "help")
         (OMTOOL_OPTION_VERSION",v",   /* no arg */         "version")
         (OMTOOL_OPTION_MODE",m",      utilOptType(string), "mode type: addhost/delhost/createdir")
      ;

      engine::utilOptions::addOptions( "addhost/delhost options" )
         (OMTOOL_OPTION_HOSTNAME,      utilOptType(string), "hostname")
         (OMTOOL_OPTION_IP,            utilOptType(string), "ip")
      ;

      engine::utilOptions::addOptions( "createdir options" )
         (OMTOOL_OPTION_PATH,          utilOptType(string), "path")
         (OMTOOL_OPTION_USER,          utilOptType(string), "user name")
      ;

      rc = engine::utilOptions::parse( argc, argv ) ;
      if ( rc )
      {
         goto error;
      }

      if ( has( OMTOOL_OPTION_HELP ) || has( OMTOOL_OPTION_VERSION ) )
      {
         goto done;
      }

      rc = _checkOptions() ;
      if ( rc )
      {
         goto error;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omToolOptions::_checkOptions()
   {
      INT32 rc = SDB_OK ;

      if( !has( OMTOOL_OPTION_MODE ) )
      {
         rc = SDB_INVALIDARG ;
         cout << "mode invalid" << endl ;
         goto error ;
      }

      _mode = get<string>(OMTOOL_OPTION_MODE) ;

      if( has( OMTOOL_OPTION_IP ) )
      {
         _ip = get<string>(OMTOOL_OPTION_IP) ;
      }

      if( has( OMTOOL_OPTION_HOSTNAME ) )
      {
         _hostname = get<string>(OMTOOL_OPTION_HOSTNAME) ;
      }

      if( has( OMTOOL_OPTION_PATH ) )
      {
         _path = get<string>(OMTOOL_OPTION_PATH) ;
      }

      if( has( OMTOOL_OPTION_USER ) )
      {
         _user = get<string>(OMTOOL_OPTION_USER) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
