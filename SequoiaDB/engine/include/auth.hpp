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

   Source File Name = auth.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A;

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef AUTH_HPP__
#define AUTH_HPP__

#include "authAccessControlList.hpp"
#include "authRequiredPrivileges.hpp"
#include "authBuiltinRole.hpp"
namespace engine
{
   ossPoolString authGetBuiltinRoleFromOldRole( const CHAR *oldRoleName );
   INT32 authMeetRequiredPrivileges( const authRequiredPrivileges &requiredPrivileges,
                                     const authAccessControlList &acl );
   INT32 authMeetActionsOnCluster( const authActionSet &actions, const authAccessControlList &acl );
   INT32 authMeetActionsOnExact( const CHAR *pFullName,
                                 const authActionSet &actions,
                                 const authAccessControlList &acl );
   
   // return FALSE if the role is not a built-in role
   BOOLEAN authIsBuiltinRole( const CHAR *roleName );
   INT32 authGetBuiltinRole( const CHAR *roleName, authBuiltinRole &builtinRole );
   void authInitBuiltinRolePrivileges();
   INT32 authGetBuiltinRoleBsonVec( BOOLEAN showPrivileges, ossPoolVector< bson::BSONObj > &out );
}

#endif