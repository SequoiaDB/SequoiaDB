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

   Source File Name = authRoleManager.cpp

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

#include "authRoleManager.hpp"
#include "authDef.hpp"
#include "authAccessControlList.hpp"
#include "sdbInterface.hpp"
#include "catRoleAgent.hpp"
#include "authTrace.hpp"

namespace engine
{
   using namespace bson;

   INT32 _authRoleManager::init()
   {
      return SDB_OK;
   }

   INT32 _authRoleManager::fini()
   {
      return SDB_OK;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_ACTIVE, "_authRoleManager::active" )
   INT32 _authRoleManager::active()
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_ACTIVE );
      _dag.clear();

      rc = _loadRoles();
      PD_RC_CHECK( rc, PDERROR, "Failed to load roles, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_ACTIVE, rc );
      return rc;
   error:
      _dag.clear();
      goto done;
   }

   INT32 _authRoleManager::deactive()
   {
      _dag.clear();
      return SDB_OK;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_ATTACHCB, "_authRoleManager::attachCB" )
   void _authRoleManager::attachCB( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_ATTACHCB );
      _cb = cb;

      INT32 rc = _loadRoles();
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to load roles, rc: %d", rc );
      }
      PD_TRACE_EXIT( SDB_AUTH_ROLE_MGR_ATTACHCB );
   }

   void _authRoleManager::detachCB( pmdEDUCB *cb )
   {
      _cb = NULL;
   }

   INT32 addPrivilegeArrayToACL( const BSONObj &privileges, authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      for ( BSONObjIterator it( privileges ); it.more(); )
      {
         BSONElement privElem = it.next();
         if ( privElem.type() != Object )
         {
            rc = SDB_SYS;
            PD_LOG( PDERROR, "Invalid type of privilege, rc: %d", rc );
            goto error;
         }
         BSONObj privObj = privElem.Obj();
         rc = acl.addPrivilege( privObj );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privilege to ACL, rc: %d", rc );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 removePrivilegeArrayToACL( const BSONObj &privileges, authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      for ( BSONObjIterator it( privileges ); it.more(); )
      {
         BSONElement privElem = it.next();
         if ( privElem.type() != Object )
         {
            rc = SDB_SYS;
            PD_LOG( PDERROR, "Invalid type of privilege, rc: %d", rc );
            goto error;
         }
         BSONObj privObj = privElem.Obj();
         rc = acl.removePrivilege( privObj );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privilege to ACL, rc: %d", rc );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 addPrivilegesOfBuiltinRoleToACL( const CHAR *rolename,
                                          authRoleAgent *agent,
                                          authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      authBuiltinRole builtinRole;
      rc = authGetBuiltinRole( rolename, builtinRole );
      PD_RC_CHECK( rc, PDERROR, "Failed to get builtin role %s, rc: %d", rolename, rc );
      for ( INT32 i = 0; i < builtinRole.getSize(); ++i )
      {
         rc = acl.addPrivilege( builtinRole.getPrivileges()[ i ] );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privilege to ACL, rc: %d", rc );
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 addPrivilegesOfRoleToACL( const CHAR *rolename,
                                   authRoleAgent *agent,
                                   authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      try
      {
         BSONObj roleObj;
         if ( authIsBuiltinRole( rolename ) )
         {
            rc = addPrivilegesOfBuiltinRoleToACL( rolename, agent, acl );
            PD_RC_CHECK( rc, PDERROR, "Failed to add privileges of builtin role to ACL, rc: %d",
                         rc );
         }
         else
         {
            rc = agent->getRole( rolename, roleObj );
            PD_RC_CHECK( rc, PDERROR, "Failed to get role[%s] from agent, rc: %d", rolename, rc );

            BSONObj privileges = roleObj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
            rc = addPrivilegeArrayToACL( privileges, acl );
            PD_RC_CHECK( rc, PDERROR, "Failed to add privileges to ACL, rc: %d", rc );
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void appendElementsOfRole( BSONObjBuilder &builder, const BSONObj &x, BOOLEAN showPrivileges )
   {
      BSONObjIterator it( x );
      while ( it.moreWithEOO() )
      {
         BSONElement e = it.next();
         if ( e.eoo() )
            break;
         if ( showPrivileges || 0 != ossStrcmp( e.fieldName(), AUTH_FIELD_NAME_PRIVILEGES ) )
         {
            builder.append( e );
         }
         else
         {
            continue;
         }
      }
   }

   INT32 addInheritedBuiltinRolesOfRole(
      const CHAR *roleName,
      authRoleAgent *agent,
      ossPoolSet< boost::shared_ptr< ossPoolString >, SHARED_TYPE_LESS< ossPoolString > > &roles )
   {
      INT32 rc = SDB_OK;
      BSONObj roleObj;
      rc = agent->getRole( roleName, roleObj );
      PD_RC_CHECK( rc, PDERROR, "Failed to get role from agent, rc: %d", rc );
      {
         BSONObj rolesObj = roleObj.getObjectField( AUTH_FIELD_NAME_ROLES );
         for ( BSONObjIterator it( rolesObj ); it.more(); )
         {
            BSONElement ele = it.next();
            if ( ele.type() != String )
            {
               rc = SDB_SYS;
               PD_LOG( PDERROR, "Invalid type of role name, rc: %d", rc );
               goto error;
            }
            if ( authIsBuiltinRole( ele.valuestrsafe() ) )
            {
               roles.insert( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_GET_ROLE, "_authRoleManager::getRole" )
   INT32 _authRoleManager::getRole( const bson::BSONObj &obj, bson::BSONObj &out )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_GET_ROLE );
      const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
      BSONElement showPrivilegesEle = obj.getField( AUTH_FIELD_NAME_SHOW_PRIVILEGES );
      if ( 0 == strlen( roleName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
         goto error;
      }

      try
      {
         BSONObj roleObj;
         if ( authIsBuiltinRole( roleName ) )
         {
            BSONObjBuilder builder;
            authAccessControlList acl;
            builder.append( AUTH_FIELD_NAME_ROLENAME, roleName );
            PD_RC_CHECK( rc, PDERROR, "Failed to add privileges of role to ACL, rc: %d", rc );
            builder.appendArray( AUTH_FIELD_NAME_ROLES, BSONObj() );
            builder.appendArray( AUTH_FIELD_NAME_INHERITED_ROLES, BSONObj() );
            if ( showPrivilegesEle.booleanSafe() )
            {
               rc = addPrivilegesOfBuiltinRoleToACL( roleName, _agent.get(), acl );
               BSONArrayBuilder privBuilder( builder.subarrayStart( AUTH_FIELD_NAME_PRIVILEGES ) );
               acl.toBSONArray( privBuilder );
               BSONArrayBuilder inherited(
                  builder.subarrayStart( AUTH_FIELD_NAME_INHERITED_PRIVILEGES ) );
               acl.toBSONArray( inherited );
            }
            out = builder.obj();
         }
         else
         {
            rc = _agent->getRole( roleName, roleObj );
            PD_RC_CHECK( rc, PDERROR, "Failed to get role from agent, rc: %d", rc );

            BSONObjBuilder builder;
            appendElementsOfRole( builder, roleObj, showPrivilegesEle.booleanSafe() );
            ossPoolVector< boost::shared_ptr< ossPoolString > > inheritedNoBuiltinRoles;
            boost::shared_ptr< ossPoolString > pRoleName =
               boost::make_shared< ossPoolString >( roleName );
            if ( !_dag.hasNode( pRoleName ) )
            {
               rc = SDB_AUTH_ROLE_NOT_EXIST;
               PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
               goto error;
            }

            if ( !_dag.dfs( pRoleName, FALSE, inheritedNoBuiltinRoles ) )
            {
               rc = SDB_AUTH_ROLE_CYCLE_DETECTED;
               PD_RC_CHECK( rc, PDERROR, "Detected a cycle in role graph, rc: %d", rc );
            }

            ossPoolSet< boost::shared_ptr< ossPoolString >, SHARED_TYPE_LESS< ossPoolString > >
               inheritedRoles;
            for ( UINT32 i = 0; i < inheritedNoBuiltinRoles.size(); ++i )
            {
               inheritedRoles.insert( inheritedNoBuiltinRoles[ i ] );
            }

            BSONObj rolesObj = roleObj.getObjectField( AUTH_FIELD_NAME_ROLES );
            for ( BSONObjIterator it( rolesObj ); it.more(); )
            {
               BSONElement ele = it.next();
               if ( ele.type() != String )
               {
                  rc = SDB_SYS;
                  PD_LOG( PDERROR, "Invalid type of role name, rc: %d", rc );
                  goto error;
               }
               if ( authIsBuiltinRole( ele.valuestrsafe() ) )
               {
                  inheritedRoles.insert(
                     boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
               }
            }

            for ( UINT32 i = 0; i < inheritedNoBuiltinRoles.size(); ++i )
            {
               rc = addInheritedBuiltinRolesOfRole( inheritedNoBuiltinRoles[ i ]->c_str(),
                                                    _agent.get(), inheritedRoles );
               PD_RC_CHECK( rc, PDERROR, "Failed to get inherited built-in roles of role %s",
                            inheritedNoBuiltinRoles[ i ]->c_str() );
            }

            BSONArrayBuilder inheritedRolesBuilder;
            for ( ossPoolSet< boost::shared_ptr< ossPoolString >,
                              SHARED_TYPE_LESS< ossPoolString > >::const_iterator it =
                     inheritedRoles.begin();
                  it != inheritedRoles.end(); ++it )
            {
               inheritedRolesBuilder.append( ( *it )->c_str() );
            }
            builder.appendArray( AUTH_FIELD_NAME_INHERITED_ROLES, inheritedRolesBuilder.done() );

            if ( showPrivilegesEle.booleanSafe() )
            {
               BSONArrayBuilder inheritedPrivBuilder;
               authAccessControlList acl;
               BSONObj privsObj = roleObj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
               rc = addPrivilegeArrayToACL( privsObj, acl );
               PD_RC_CHECK( rc, PDERROR, "Failed to add privileges to ACL, rc: %d", rc );

               for ( ossPoolSet< boost::shared_ptr< ossPoolString >,
                                 SHARED_TYPE_LESS< ossPoolString > >::const_iterator it =
                        inheritedRoles.begin();
                     it != inheritedRoles.end(); ++it )
               {
                  rc = addPrivilegesOfRoleToACL( ( *it )->c_str(), _agent.get(), acl );
                  PD_RC_CHECK( rc, PDERROR, "Failed to insert privileges of role to ACL, rc: %d",
                               rc );
               }
               acl.toBSONArray( inheritedPrivBuilder );
               builder.appendArray( AUTH_FIELD_NAME_INHERITED_PRIVILEGES,
                                    inheritedPrivBuilder.done() );
            }
            out = builder.obj();
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }
   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_GET_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_LIST_ROLES, "_authRoleManager::listRoles" )
   INT32 _authRoleManager::listRoles( const bson::BSONObj &obj,
                                      ossPoolVector< bson::BSONObj > &out )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_LIST_ROLES );
      BSONElement showPrivilegesEle = obj.getField( AUTH_FIELD_NAME_SHOW_PRIVILEGES );
      BSONElement showBuiltinRolesEle = obj.getField( AUTH_FIELD_NAME_SHOW_BUILTIN_ROLES );
      ossPoolMap< ossPoolString, BSONObj > roles;

      try
      {
         rc = _agent->listRoles( roles );
         PD_RC_CHECK( rc, PDERROR, "Failed to get roles, rc: %d", rc );

         for ( ossPoolMap< ossPoolString, BSONObj >::const_iterator it = roles.begin();
               it != roles.end(); ++it )
         {
            boost::shared_ptr< ossPoolString > pRoleName =
               boost::make_shared< ossPoolString >( it->first );
            if ( !authIsBuiltinRole( it->first.c_str() ) && !_dag.hasNode( pRoleName ) )
            {
               rc = SDB_AUTH_ROLE_NOT_EXIST;
               PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", it->first.c_str(), rc );
               goto error;
            }
            const BSONObj &roleObj = it->second;
            BSONObjBuilder builder;
            appendElementsOfRole( builder, roleObj, showPrivilegesEle.booleanSafe() );
            ossPoolVector< boost::shared_ptr< ossPoolString > > inheritedNoBuiltinRoles;

            if ( !_dag.dfs( pRoleName, FALSE, inheritedNoBuiltinRoles ) )
            {
               rc = SDB_AUTH_ROLE_CYCLE_DETECTED;
               PD_RC_CHECK( rc, PDERROR, "Detected a cycle in role graph, rc: %d", rc );
            }

            ossPoolSet< boost::shared_ptr< ossPoolString >, SHARED_TYPE_LESS< ossPoolString > >
               inheritedRoles;
            for ( UINT32 i = 0; i < inheritedNoBuiltinRoles.size(); ++i )
            {
               inheritedRoles.insert( inheritedNoBuiltinRoles[ i ] );
            }

            BSONObj rolesObj = roleObj.getObjectField( AUTH_FIELD_NAME_ROLES );
            for ( BSONObjIterator it( rolesObj ); it.more(); )
            {
               BSONElement ele = it.next();
               if ( ele.type() != String )
               {
                  rc = SDB_SYS;
                  PD_LOG( PDERROR, "Invalid type of role name, rc: %d", rc );
                  goto error;
               }
               if ( authIsBuiltinRole( ele.valuestrsafe() ) )
               {
                  inheritedRoles.insert(
                     boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
               }
            }

            for ( UINT32 i = 0; i < inheritedNoBuiltinRoles.size(); ++i )
            {
               const BSONObj &inherited = roles[ *inheritedNoBuiltinRoles[ i ] ];
               BSONObj rolesObj = inherited.getObjectField( AUTH_FIELD_NAME_ROLES );
               for ( BSONObjIterator it( rolesObj ); it.more(); )
               {
                  BSONElement ele = it.next();
                  if ( ele.type() != String )
                  {
                     rc = SDB_SYS;
                     PD_LOG( PDERROR, "Invalid type of role name, rc: %d", rc );
                     goto error;
                  }
                  if ( authIsBuiltinRole( ele.valuestrsafe() ) )
                  {
                     inheritedRoles.insert(
                        boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
                  }
               }
            }

            BSONArrayBuilder inheritedRolesBuilder;
            for ( ossPoolSet< boost::shared_ptr< ossPoolString >,
                              SHARED_TYPE_LESS< ossPoolString > >::const_iterator it =
                     inheritedRoles.begin();
                  it != inheritedRoles.end(); ++it )
            {
               inheritedRolesBuilder.append( ( *it )->c_str() );
            }
            builder.appendArray( AUTH_FIELD_NAME_INHERITED_ROLES, inheritedRolesBuilder.done() );
            if ( showPrivilegesEle.booleanSafe() )
            {
               BSONArrayBuilder inheritedPrivBuilder;
               authAccessControlList acl;
               BSONObj privsObj = roleObj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
               rc = addPrivilegeArrayToACL( privsObj, acl );
               PD_RC_CHECK( rc, PDERROR, "Failed to add privileges to ACL, rc: %d", rc );
               for ( ossPoolSet< boost::shared_ptr< ossPoolString >,
                                 SHARED_TYPE_LESS< ossPoolString > >::const_iterator it =
                        inheritedRoles.begin();
                     it != inheritedRoles.end(); ++it )
               {
                  if ( authIsBuiltinRole( ( *it )->c_str() ) )
                  {
                     rc = addPrivilegesOfRoleToACL( ( *it )->c_str(), _agent.get(), acl );
                     PD_RC_CHECK( rc, PDERROR,
                                  "Failed to add privileges of built-in role %s to ACL, rc: %d",
                                  ( *it )->c_str(), rc );
                  }
                  else
                  {
                     const BSONObj &inherited = roles[ **it ];
                     BSONObj privsObj = inherited.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
                     rc = addPrivilegeArrayToACL( privsObj, acl );
                     PD_RC_CHECK( rc, PDERROR, "Failed to add privileges of role %s to ACL, rc: %d",
                                  ( *it )->c_str(), rc );
                  }
               }

               acl.toBSONArray( inheritedPrivBuilder );
               builder.appendArray( AUTH_FIELD_NAME_INHERITED_PRIVILEGES,
                                    inheritedPrivBuilder.done() );
            }
            out.push_back( builder.obj() );
         }
         if ( showBuiltinRolesEle.booleanSafe() )
         {
            rc = authGetBuiltinRoleBsonVec( showPrivilegesEle.booleanSafe(), out );
            PD_RC_CHECK( rc, PDERROR, "Failed to get builtin roles, rc: %d", rc );
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_LIST_ROLES, rc );
      return rc;
   error:
      goto done;
   }

   INT32 checkPrivilegesObj( const BSONObj &privObjs )
   {
      INT32 rc = SDB_OK;
      for ( BSONObjIterator it( privObjs ); it.more(); )
      {
         BSONElement privEle = it.next();
         if ( privEle.type() != Object )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Privilege definition must be object" );
            goto error;
         }
         BSONObj privObj = privEle.Obj();
         BSONObj resObj = privObj.getObjectField( AUTH_FIELD_NAME_RESOURCE );
         boost::shared_ptr< authResource > pRes = authResource::fromBson( resObj );
         RESOURCE_TYPE resType = pRes->getType();
         if ( resType == RESOURCE_TYPE__INVALID )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "The resource definition is invalid" );
            goto error;
         }
         else
         {
            BSONElement actionsEle = privObj.getField( AUTH_FIELD_NAME_ACTIONS );
            if ( Array != actionsEle.type() )
            {
               rc = SDB_INVALIDARG;
               PD_LOG_MSG( PDERROR, "The actions definition is invalid" );
               goto error;
            }
            BSONObj actions = actionsEle.Obj();
            for ( BSONObjIterator it( actions ); it.more(); )
            {
               BSONElement actionEle = it.next();
               if ( actionEle.type() != bson::String )
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG_MSG( PDERROR, "Action definition must be string" );
                  goto error;
               }
               else
               {
                  ACTION_TYPE actionType = authActionTypeParse( actionEle.valuestrsafe() );
                  if ( actionType == ACTION_TYPE__invalid )
                  {
                     rc = SDB_INVALIDARG;
                     PD_LOG_MSG( PDERROR, "Unrecognized action: %s", actionEle.valuestrsafe() );
                     goto error;
                  }
                  if ( !authGetActionSetMask( resType )->contains( actionType ) )
                  {
                     rc = SDB_INVALIDARG;
                     PD_LOG_MSG( PDERROR, "Action %s definition is invalid on the resource %s",
                                 actionEle.valuestrsafe(), resObj.toString().c_str() );
                     goto error;
                  }
               }
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_CREATE_ROLE, "_authRoleManager::createRole" )
   INT32 _authRoleManager::createRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_CREATE_ROLE );
      const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
      BOOLEAN added = FALSE;
      if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
         goto error;
      }

      try
      {
         if ( !_dag.addNode( boost::make_shared< ossPoolString >( roleName ) ) )
         {
            rc = SDB_AUTH_ROLE_EXIST;
            PD_LOG_MSG( PDERROR, "Role %s already exists", roleName );
            goto error;
         }
         added = TRUE;

         BSONObj privObjs = obj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
         rc = checkPrivilegesObj( privObjs );
         PD_RC_CHECK( rc, PDERROR, "Invalid privileges definition, rc: %d", rc );

         BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
         rc = _grantRolesToRole( roleName, rolesEle, TRUE );
         PD_RC_CHECK( rc, PDERROR, "Failed to grant roles to role, rc: %d", rc );

         rc = _agent->createRole( obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to create role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_CREATE_ROLE, rc );
      return rc;
   error:
      if ( added )
      {
         _dag.delNode( boost::make_shared< ossPoolString >( roleName ) );
      }
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_DROP_ROLE, "_authRoleManager::dropRole" )
   INT32 _authRoleManager::dropRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_DROP_ROLE );
      try
      {
         const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
         if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
            goto error;
         }

         rc = _agent->dropRole( roleName );
         PD_RC_CHECK( rc, PDERROR, "Failed to drop role by agent, rc: %d", rc );

         BOOLEAN res = _dag.delNode( boost::make_shared< ossPoolString >( roleName ) );
         if ( !res )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
            goto error;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_DROP_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_UPDATE_ROLE, "_authRoleManager::updateRole" )
   INT32 _authRoleManager::updateRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_UPDATE_ROLE );
      const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
      if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
         goto error;
      }

      try
      {
         if ( !_dag.hasNode( boost::make_shared< ossPoolString >( roleName ) ) )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
            goto error;
         }
         BSONObj privObjs = obj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
         rc = checkPrivilegesObj( privObjs );
         PD_RC_CHECK( rc, PDERROR, "Invalid privileges definition, rc: %d", rc );

         BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
         rc = _grantRolesToRole( roleName, rolesEle, TRUE );
         PD_RC_CHECK( rc, PDERROR, "Failed to grant roles to role, rc: %d", rc );

         rc = _agent->updateRole( roleName, obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to update role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }
   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_UPDATE_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_GRANT_PRIVILEGES_TO_ROLE, "_authRoleManager::grantPrivilegesToRole" )
   INT32 _authRoleManager::grantPrivilegesToRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_GRANT_PRIVILEGES_TO_ROLE );
      const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
      if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
         goto error;
      }

      try
      {
         BSONObj privileges = obj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
         rc = checkPrivilegesObj( privileges );
         PD_RC_CHECK(rc, PDERROR, "Invalid privileges definition, rc: %d", rc);

         authAccessControlList acl;
         rc = addPrivilegesOfRoleToACL( roleName, _agent.get(), acl );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privileges of role to ACL, rc: %d", rc );

         rc = addPrivilegeArrayToACL( privileges, acl );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privileges to ACL, rc: %d", rc );

         BSONArrayBuilder newPrivBuilder;
         acl.toBSONArray( newPrivBuilder );

         rc = _agent->updateRole( roleName,
                                  BSON( AUTH_FIELD_NAME_PRIVILEGES << newPrivBuilder.arr() ) );
         PD_RC_CHECK( rc, PDERROR, "Failed to update role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_GRANT_PRIVILEGES_TO_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_REVOKE_PRIVILEGES_FROM_ROLE, "_authRoleManager::revokePrivilegesFromRole" )
   INT32 _authRoleManager::revokePrivilegesFromRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_REVOKE_PRIVILEGES_FROM_ROLE );
      const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
      if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
         goto error;
      }

      try
      {
         BSONObj privileges = obj.getObjectField( AUTH_FIELD_NAME_PRIVILEGES );
         rc = checkPrivilegesObj( privileges );
         PD_RC_CHECK(rc, PDERROR, "Invalid privileges definition, rc: %d", rc);

         authAccessControlList acl;
         rc = addPrivilegesOfRoleToACL( roleName, _agent.get(), acl );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privileges of role to ACL, rc: %d", rc );

         rc = removePrivilegeArrayToACL( privileges, acl );
         PD_RC_CHECK( rc, PDERROR, "Failed to add privileges to ACL, rc: %d", rc );

         BSONArrayBuilder newPrivBuilder;
         acl.toBSONArray( newPrivBuilder );

         rc = _agent->updateRole( roleName,
                                  BSON( AUTH_FIELD_NAME_PRIVILEGES << newPrivBuilder.arr() ) );
         PD_RC_CHECK( rc, PDERROR, "Failed to update role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_REVOKE_PRIVILEGES_FROM_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_ROLE, "_authRoleManager::grantRolesToRole" )
   INT32 _authRoleManager::grantRolesToRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_ROLE );
      try
      {
         const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
         if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
            goto error;
         }

         BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
         rc = _grantRolesToRole( roleName, rolesEle, FALSE );
         PD_RC_CHECK( rc, PDERROR, "Failed to grant roles to role, rc: %d", rc );

         rc = _agent->grantRolesToRole( roleName, obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to revoke privileges from role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_ROLE, "_authRoleManager::revokeRolesFromRole" )
   INT32 _authRoleManager::revokeRolesFromRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_ROLE );
      try
      {
         const CHAR *roleName = obj.getStringField( AUTH_FIELD_NAME_ROLENAME );
         if ( 0 == strlen( roleName ) || authIsBuiltinRole( roleName ) )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Invalid role name, rc: %d", rc );
            goto error;
         }

         ossPoolVector< boost::shared_ptr< ossPoolString > > roles;
         BSONObj rolesObj = obj.getObjectField( AUTH_FIELD_NAME_ROLES );
         for ( BSONObjIterator it( rolesObj ); it.more(); )
         {
            BSONElement ele = it.next();
            if ( ele.type() != bson::String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG_MSG( PDERROR, "Role definition must be string" );
               goto error;
            }
            if ( !authIsBuiltinRole( ele.valuestrsafe() ) )
            {
               roles.push_back( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
            }
         }

         if ( !_dag.delEdges( boost::make_shared< ossPoolString >( roleName ), roles ) )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
            goto error;
         }

         rc = _agent->revokeRolesFromRole( roleName, obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to revoke privileges from role by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR__GRANT_ROLES_TO_ROLE, "_authRoleManager::_grantRolesToRole" )
   INT32 _authRoleManager::_grantRolesToRole( const CHAR *roleName,
                                              const bson::BSONElement &rolesEle,
                                              BOOLEAN replace )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR__GRANT_ROLES_TO_ROLE );
      ossPoolVector< boost::shared_ptr< ossPoolString > > roles;
      BSONObj rolesObj;
      if ( rolesEle.eoo() )
      {
         goto done;
      }
      if ( rolesEle.type() != Array )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "%s must be array", AUTH_FIELD_NAME_ROLES );
         goto error;
      }

      rolesObj = rolesEle.Obj();
      for ( BSONObjIterator it( rolesObj ); it.more(); )
      {
         BSONElement ele = it.next();
         if ( ele.type() != bson::String )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Role definition must be string" );
            goto error;
         }
         if ( !authIsBuiltinRole( ele.valuestrsafe() ) )
         {
            roles.push_back( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
         }
      }

      {
         std::pair< INT32, boost::shared_ptr< ossPoolString > > res;
         if ( replace )
         {
            res = _dag.replaceEdges( boost::make_shared< ossPoolString >( roleName ), roles );
         }
         else
         {
            res = _dag.addEdges( boost::make_shared< ossPoolString >( roleName ), roles );
         }

         if ( UTIL_DAG_EDGES_RET_SRC_NOT_FOUND == res.first )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", res.second->c_str(), rc );
            goto error;
         }
         else if ( UTIL_DAG_EDGES_RET_DEST_NOT_FOUND == res.first )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", res.second->c_str(), rc );
            goto error;
         }
         else if ( UTIL_DAG_EDGES_RET_CYCLE_DETECTED == res.first )
         {
            rc = SDB_AUTH_ROLE_CYCLE_DETECTED;
            PD_LOG_MSG( PDERROR, "Granting roles would introduce a cycle in the role graph, rc: %d",
                        rc );
            goto error;
         }
         else if ( UTIL_DAG_EDGES_RET_SUCCESS != res.first )
         {
            rc = SDB_SYS;
            PD_LOG_MSG( PDERROR, "Failed to add edges to DAG, rc: %d", rc );
            goto error;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR__GRANT_ROLES_TO_ROLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_USER, "_authRoleManager::grantRolesToUser" )
   INT32 _authRoleManager::grantRolesToUser( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_USER );
      const CHAR *username = obj.getStringField( FIELD_NAME_USER );
      if ( 0 == strlen( username ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid argument, rc: %d", rc );
         goto error;
      }

      try
      {
         BSONObj rolesObj = obj.getObjectField( AUTH_FIELD_NAME_ROLES );
         for ( BSONObjIterator it( rolesObj ); it.more(); )
         {
            BSONElement ele = it.next();
            if ( ele.type() != bson::String )
            {
               rc = SDB_INVALIDARG;
               PD_LOG_MSG( PDERROR, "Role definition must be string" );
               goto error;
            }
            if ( !authIsBuiltinRole( ele.valuestrsafe() ) &&
                 !_dag.hasNode( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) ) )
            {
               rc = SDB_AUTH_ROLE_NOT_EXIST;
               PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", ele.valuestrsafe(), rc );
               goto error;
            }
         }

         rc = _agent->grantRolesToUser( username, obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to grant roles to user by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_GRANT_ROLES_TO_USER, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_USER, "_authRoleManager::revokeRolesFromUser" )
   INT32 _authRoleManager::revokeRolesFromUser( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_USER );
      const CHAR *username = obj.getStringField( FIELD_NAME_USER );
      if ( 0 == strlen( username ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid argument, rc: %d", rc );
         goto error;
      }

      try
      {
         rc = _agent->revokeRolesFromUser( username, obj );
         PD_RC_CHECK( rc, PDERROR, "Failed to grant roles to user by agent, rc: %d", rc );
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_REVOKE_ROLES_FROM_USER, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR_GET_USER, "_authRoleManager::getUser" )
   INT32 _authRoleManager::getUser( const bson::BSONObj &obj, bson::BSONObj &out )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR_GET_USER );
      const CHAR *username = obj.getStringField( FIELD_NAME_USER );
      BSONElement showPrivilegesEle = obj.getField( AUTH_FIELD_NAME_SHOW_PRIVILEGES );
      if ( 0 == strlen( username ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid argument, rc: %d", rc );
         goto error;
      }

      try
      {
         BSONObj userObj;
         rc = _agent->getUser( username, userObj );
         PD_RC_CHECK( rc, PDERROR, "Failed to get user[%s] from agent, rc: %d", username, rc );
         if ( showPrivilegesEle.booleanSafe() )
         {
            BSONObjBuilder builder;
            builder.appendElements( userObj );
            ossPoolVector< boost::shared_ptr< ossPoolString > > inheritedRolesNoBuiltin;
            ossPoolVector< boost::shared_ptr< ossPoolString > > rolesNoBuiltin;
            ossPoolSet< boost::shared_ptr< ossPoolString >, SHARED_TYPE_LESS< ossPoolString > >
               rolesBuiltin;
            BSONObj rolesObj = userObj.getObjectField( AUTH_FIELD_NAME_ROLES );
            for ( BSONObjIterator it( rolesObj ); it.more(); )
            {
               BSONElement ele = it.next();
               if ( ele.type() != bson::String )
               {
                  rc = SDB_SYS;
                  PD_LOG( PDERROR, "Invalid type of role name, rc: %d", rc );
                  goto error;
               }
               if ( authIsBuiltinRole( ele.valuestrsafe() ) )
               {
                  rolesBuiltin.insert( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
               }
               else
               {
                  if ( !_dag.hasNode( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) ) )
                  {
                     rc = SDB_AUTH_ROLE_NOT_EXIST;
                     PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", ele.valuestrsafe(), rc );
                     goto error;
                  }
                  rolesNoBuiltin.push_back(
                     boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
               }
            }
            if ( !_dag.dfs( rolesNoBuiltin, inheritedRolesNoBuiltin ) )
            {
               rc = SDB_AUTH_ROLE_CYCLE_DETECTED;
               PD_RC_CHECK( rc, PDERROR, "Detected a cycle in role graph, rc: %d", rc );
            }

            BSONObj optionsObj = userObj.getObjectField( FIELD_NAME_OPTIONS );
            if ( !optionsObj.isEmpty() )
            {
               BSONElement oldRoleEle = optionsObj.getField( FIELD_NAME_ROLE );
               if ( oldRoleEle.type() == bson::String )
               {
                  static boost::shared_ptr< ossPoolString > root =
                     boost::make_shared< ossPoolString >( AUTH_ROLE_ROOT );
                  static boost::shared_ptr< ossPoolString > monitor =
                     boost::make_shared< ossPoolString >( AUTH_ROLE_CLUSTER_MONITOR );
                  if ( 0 == ossStrcmp( VALUE_NAME_ADMIN, oldRoleEle.valuestrsafe() ) )
                  {
                     rolesBuiltin.insert( root );
                  }
                  else if ( 0 == ossStrcmp( VALUE_NAME_MONITOR, oldRoleEle.valuestrsafe() ) )
                  {
                     rolesBuiltin.insert( monitor );
                  }
               }
            }
            for ( UINT32 i = 0; i < inheritedRolesNoBuiltin.size(); ++i )
            {
               rc = addInheritedBuiltinRolesOfRole( inheritedRolesNoBuiltin[ i ]->c_str(),
                                                    _agent.get(), rolesBuiltin );
               PD_RC_CHECK( rc, PDERROR, "Failed to get inherited built-in roles of role %s",
                            inheritedRolesNoBuiltin[ i ]->c_str() );
            }

            BSONArrayBuilder inheritedRolesBuilder;
            BSONArrayBuilder inheritedPrivBuilder;
            authAccessControlList acl;
            for ( UINT32 i = 0; i < inheritedRolesNoBuiltin.size(); ++i )
            {
               inheritedRolesBuilder.append( inheritedRolesNoBuiltin[ i ]->c_str() );
               rc = addPrivilegesOfRoleToACL( inheritedRolesNoBuiltin[ i ]->c_str(), _agent.get(),
                                              acl );
               PD_RC_CHECK( rc, PDERROR, "Failed to insert privileges of role to ACL, rc: %d", rc );
            }
            for ( ossPoolSet< boost::shared_ptr< ossPoolString >,
                              SHARED_TYPE_LESS< ossPoolString > >::const_iterator it =
                     rolesBuiltin.begin();
                  it != rolesBuiltin.end(); ++it )
            {
               inheritedRolesBuilder.append( ( *it )->c_str() );
               rc = addPrivilegesOfBuiltinRoleToACL( ( *it )->c_str(), _agent.get(), acl );
               PD_RC_CHECK( rc, PDERROR, "Failed to insert privileges of role to ACL, rc: %d", rc );
            }
            acl.toBSONArray( inheritedPrivBuilder );
            builder.appendArray( AUTH_FIELD_NAME_INHERITED_ROLES, inheritedRolesBuilder.done() );
            builder.appendArray( AUTH_FIELD_NAME_INHERITED_PRIVILEGES,
                                 inheritedPrivBuilder.done() );
            out = builder.obj();
         }
         else
         {
            out = userObj;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR_GET_USER, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AUTH_ROLE_MGR__LOAD_ROLES, "_authRoleManager::_loadRoles" )
   INT32 _authRoleManager::_loadRoles()
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB_AUTH_ROLE_MGR__LOAD_ROLES );
      SDB_ASSERT( _cb, "can not be nullptr" );
      _agent = boost::make_shared< catRoleAgent >( _cb, sdbGetRTNCB(), sdbGetDMSCB() );
      if ( !_agent )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "Failed to construct catRoleAgent instance" );
         goto error;
      }
      try
      {
         ossPoolMap< ossPoolString, BSONObj > roles;

         rc = _agent->listRoles( roles );
         PD_RC_CHECK( rc, PDERROR, "Failed to get roles, rc: %d", rc );

         for ( ossPoolMap< ossPoolString, BSONObj >::const_iterator it = roles.begin();
               it != roles.end(); ++it )
         {
            if ( !authIsBuiltinRole( it->first.c_str() ) )
            {
               boost::shared_ptr< ossPoolString > source =
                  boost::make_shared< ossPoolString >( it->first );
               if ( !_dag.addNode( source ) )
               {
                  rc = SDB_AUTH_ROLE_EXIST;
                  PD_LOG_MSG( PDERROR, "Role %s already exists", it->first.c_str() );
                  goto error;
               }
            }
         }

         for ( ossPoolMap< ossPoolString, BSONObj >::const_iterator it = roles.begin();
               it != roles.end(); ++it )
         {
            boost::shared_ptr< ossPoolString > source =
               boost::make_shared< ossPoolString >( it->first );
            BSONObj rolesObj = it->second.getObjectField( AUTH_FIELD_NAME_ROLES );
            ossPoolVector< boost::shared_ptr< ossPoolString > > inherited;
            for ( BSONObjIterator eleIt( rolesObj ); eleIt.more(); )
            {
               BSONElement ele = eleIt.next();
               if ( ele.type() != bson::String )
               {
                  rc = SDB_SYS;
                  PD_LOG( PDERROR, "Invalid type of role name" );
                  goto error;
               }
               if ( !authIsBuiltinRole( ele.valuestrsafe() ) )
               {
                  inherited.push_back( boost::make_shared< ossPoolString >( ele.valuestrsafe() ) );
               }
            }
            std::pair< INT32, boost::shared_ptr< ossPoolString > > res;
            res = _dag.addEdges( source, inherited );
            if ( UTIL_DAG_EDGES_RET_SRC_NOT_FOUND == res.first )
            {
               rc = SDB_AUTH_ROLE_NOT_EXIST;
               PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", res.second->c_str(), rc );
               goto error;
            }
            else if ( UTIL_DAG_EDGES_RET_DEST_NOT_FOUND == res.first )
            {
               rc = SDB_AUTH_ROLE_NOT_EXIST;
               PD_LOG_MSG( PDERROR, "Role[%s] not exist, rc: %d", res.second->c_str(), rc );
               goto error;
            }
            else if ( UTIL_DAG_EDGES_RET_CYCLE_DETECTED == res.first )
            {
               rc = SDB_AUTH_ROLE_CYCLE_DETECTED;
               PD_RC_CHECK( rc, PDERROR, "Detected a cycle in role graph, rc: %d", rc );
               goto error;
            }
            else if ( UTIL_DAG_EDGES_RET_SUCCESS != res.first )
            {
               rc = SDB_SYS;
               PD_LOG_MSG( PDERROR, "Failed to add edges to DAG, rc: %d", rc );
               goto error;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() );
         goto error;
      }
   done:
      PD_TRACE_EXITRC( SDB_AUTH_ROLE_MGR__LOAD_ROLES, rc );
      return rc;
   error:
      goto done;
   }

   BOOLEAN authRoleManager::hasRole( const CHAR *roleName ) const
   {
      return authIsBuiltinRole( roleName ) ||
             _dag.hasNode( boost::make_shared< ossPoolString >( roleName ) );
   }

} // namespace engine