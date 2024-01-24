/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = authRBAC.cpp

   Descriptive Name = Role based access control file.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/09/2022  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "authRBAC.hpp"
#include "ossUtil.hpp"
#include "msgDef.hpp"
#include "authDef.hpp"

namespace engine
{
   // Prefixes/Complete commands for monitor operations. For a user of role
   // monitor, all these operations are allowed.
   // Note: Keep a blank after some prefix to distinguish from some possible
   // commands starting with the same prefix.
   static const CHAR *gAuthMonCmdPrefix[] = { CMD_ADMIN_PREFIX "snapshot ",
                                              CMD_ADMIN_PREFIX "list ",
                                              CMD_ADMIN_PREFIX "test ",
                                              CMD_ADMIN_PREFIX "get ",
                                              CMD_ADMIN_PREFIX "SNAPSHOT_",
                                              CMD_ADMIN_PREFIX "LIST_",

                                              CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS,
                                              CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR,
                                              CMD_ADMIN_PREFIX CMD_NAME_EVAL };

#define AUTH_MON_CMD_PREFIX_NUM ( sizeof( gAuthMonCmdPrefix ) / sizeof( const CHAR * ) )

   namespace oldRole
   {
      UINT32 authGetBuiltinRoleID( const CHAR *roleName )
      {
         UINT32 id = AUTH_INVALID_ROLE_ID;

         if ( roleName )
         {
            if ( ossStrlen( roleName ) == 0 )
            {
               id = AUTH_NULL_ROLE_ID;
            }
            else if ( 0 == ossStrcmp( roleName, AUTH_ROLE_ADMIN_NAME ) )
            {
               id = AUTH_ROLE_ADMIN;
            }
            else if ( 0 == ossStrcmp( roleName, AUTH_ROLE_MONITOR_NAME ) )
            {
               id = AUTH_ROLE_MONITOR;
            }
         }

         return id;
      }
   } // namespace oldRole

   BOOLEAN authIsMonCmd( const CHAR *cmdName )
   {
      BOOLEAN result = FALSE;

      if ( cmdName )
      {
         for ( UINT16 i = 0; i < AUTH_MON_CMD_PREFIX_NUM; ++i )
         {
            if ( 0 == ossStrncasecmp( cmdName, gAuthMonCmdPrefix[ i ],
                                      ossStrlen( gAuthMonCmdPrefix[ i ] ) ) )
            {
               result = TRUE;
               break;
            }
         }
      }

      return result;
   }
} // namespace engine
