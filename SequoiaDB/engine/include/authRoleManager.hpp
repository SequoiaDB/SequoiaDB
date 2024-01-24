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

   Source File Name = authRoleManager.hpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef AUTH_ROLE_MANAGER_HPP__
#define AUTH_ROLE_MANAGER_HPP__

#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "pmdEDU.hpp"
#include "utilDAG.hpp"
#include "authRoleAgent.hpp"
namespace engine
{
   class _authRoleManager : public SDBObject
   {
   public:
      _authRoleManager() {}
      ~_authRoleManager() {}

      INT32 init();
      INT32 fini();
      INT32 active();
      INT32 deactive();

      void attachCB( pmdEDUCB *cb );
      void detachCB( pmdEDUCB *cb );

      INT32 getRole(  const bson::BSONObj &obj, bson::BSONObj &out );
      INT32 listRoles(  const bson::BSONObj &obj, ossPoolVector<bson::BSONObj> &out );

      INT32 createRole(  const bson::BSONObj &obj );
      INT32 dropRole(  const bson::BSONObj &obj );
      INT32 updateRole(  const bson::BSONObj &obj );

      INT32 grantPrivilegesToRole(  const bson::BSONObj &obj );
      INT32 revokePrivilegesFromRole(  const bson::BSONObj &obj );

      INT32 grantRolesToRole (  const bson::BSONObj &obj );
      INT32 revokeRolesFromRole (  const bson::BSONObj &obj );

      INT32 grantRolesToUser(  const bson::BSONObj &obj );
      INT32 revokeRolesFromUser(  const bson::BSONObj &obj );

      INT32 getUser( const bson::BSONObj &obj, bson::BSONObj &out );

      BOOLEAN hasRole( const CHAR *roleName ) const;

   private:
      INT32 _grantRolesToRole( const CHAR *roleName,
                               const bson::BSONElement &rolesELe,
                               BOOLEAN replace );
      
      INT32 _loadRoles();

   private:
      boost::shared_ptr< authRoleAgent > _agent;
      utilDAG< ossPoolString > _dag;
      pmdEDUCB *_cb;
   };
   typedef _authRoleManager authRoleManager;
} // namespace engine

#endif