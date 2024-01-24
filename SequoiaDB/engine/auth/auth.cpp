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

   Source File Name = auth.cpp

   Descriptive Name =

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "sdbInterface.hpp"
#include "auth.hpp"
#include "authDef.hpp"
#include "authPrivilege.hpp"
#include <boost/make_shared.hpp>
#include <boost/exception/diagnostic_information.hpp>

namespace engine
{
   using namespace bson;
   ossPoolString authGetBuiltinRoleFromOldRole( const CHAR *oldRoleName )
   {
      SDB_ASSERT( oldRoleName, "can not be null" );

      if ( 0 == ossStrcmp( oldRoleName, AUTH_ROLE_ADMIN_NAME ) )
      {
         return AUTH_ROLE_ROOT;
      }
      else if ( 0 == ossStrcmp( oldRoleName, AUTH_ROLE_MONITOR_NAME ) )
      {
         return AUTH_ROLE_CLUSTER_MONITOR;
      }
      else
      {
         return ossPoolString();
      }
   }

   INT32 authMeetRequiredPrivileges( const authRequiredPrivileges &requiredPrivileges,
                                     const authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      try
      {
         for ( authRequiredPrivileges::DATA_TYPE::const_iterator it =
                  requiredPrivileges.getData().begin();
               it != requiredPrivileges.getData().end(); ++it )
         {
            BOOLEAN meetOne = FALSE;
            for ( UINT32 actionSetIndex = 0; actionSetIndex < it->second->getSize();
                  ++actionSetIndex )
            {
               const authActionSet &actionSet = it->second->getActionSets()[ actionSetIndex ];
               meetOne = acl.isAuthorizedForActionsOnResource( *it->first, actionSet );
               if ( meetOne )
               {
                  break;
               }
               else
               {
                  continue;
               }
            }
            if ( !meetOne )
            {
               rc = SDB_NO_PRIVILEGES;
               goto error;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s. rc: %d", e.what(), rc );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Occured exception when check privileges: %s, rc: %d",
                 boost::diagnostic_information( e ).c_str(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 authMeetActionsOnCluster( const authActionSet &actions, const authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      try
      {
         boost::shared_ptr< authResource > r = authResource::forCluster();
         SDB_ASSERT( r, "Failed to get authResource for cluster" );

         if ( !acl.isAuthorizedForActionsOnResource( *r, actions ) )
         {
            rc = SDB_NO_PRIVILEGES;
            PD_LOG( PDERROR, "Actions are not authorized on cluster" );
            goto error;
         };
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Occured exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Occured exception: %s, rc: %d",
                 boost::diagnostic_information( e ).c_str(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 authMeetActionsOnExact( const CHAR *pFullName,
                                 const authActionSet &actions,
                                 const authAccessControlList &acl )
   {
      INT32 rc = SDB_OK;
      if ( !pFullName )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "pFullName can not be null" );
         goto error;
      }
      if ( !authResource::isExactName( pFullName ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR,
                     "Invalid format for collection name: %s, "
                     "Expected format: <collectionspace>.<collectionname>",
                     pFullName );
         goto error;
      }

      try
      {
         boost::shared_ptr< authResource > r = authResource::forExact( pFullName );
         if ( !r )
         {
            rc = SDB_OOM;
            PD_LOG( PDERROR, "Failed to create authResource for collection %s", pFullName );
            goto error;
         }

         if ( !acl.isAuthorizedForActionsOnResource( *r, actions ) )
         {
            rc = SDB_NO_PRIVILEGES;
            PD_LOG( PDERROR, "Actions are not authorized on collection %s", pFullName );
            goto error;
         };
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Occured exception: %s, rc: %d", e.what(), rc );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Occured exception: %s, rc: %d",
                 boost::diagnostic_information( e ).c_str(), rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN authIsBuiltinRole( const CHAR *roleName )
   {
      SDB_ASSERT( roleName, "can not be null" );
      if ( roleName[ 0 ] != BUILTIN_ROLE_PREFIX )
      {
         return FALSE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_ROOT ) )
      {
         return TRUE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_CLUSTER_ADMIN ) )
      {
         return TRUE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_CLUSTER_MONITOR ) )
      {
         return TRUE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_BACKUP ) )
      {
         return TRUE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_DB_ADMIN ) )
      {
         return TRUE;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_USER_ADMIN ) )
      {
         return TRUE;
      }

      const CHAR *dotIndex = ossStrchr( roleName, '.' );
      if ( dotIndex )
      {
         ossPoolString cs( roleName + 1, dotIndex - roleName - 1 );
         ossPoolString builtinRole( dotIndex + 1 );
         if ( builtinRole == AUTH_ROLE_CS_READ )
         {
            return TRUE;
         }
         else if ( builtinRole == AUTH_ROLE_CS_READ_WRITE )
         {
            return TRUE;
         }
         else if ( builtinRole == AUTH_ROLE_CS_ADMIN )
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   static authPrivilege AUTH_BUILTIN_ROLE_PRIVILEGES_root[ BUILTIN_ROLE_DATA_SIZE_root ];
   static authPrivilege
      AUTH_BUILTIN_ROLE_PRIVILEGES_clusterAdmin[ BUILTIN_ROLE_DATA_SIZE_clusterAdmin ];
   static authPrivilege
      AUTH_BUILTIN_ROLE_PRIVILEGES_clusterMonitor[ BUILTIN_ROLE_DATA_SIZE_clusterMonitor ];
   static authPrivilege AUTH_BUILTIN_ROLE_PRIVILEGES_backup[ BUILTIN_ROLE_DATA_SIZE_backup ];
   static authPrivilege AUTH_BUILTIN_ROLE_PRIVILEGES_dbAdmin[ BUILTIN_ROLE_DATA_SIZE_dbAdmin ];
   static authPrivilege AUTH_BUILTIN_ROLE_PRIVILEGES_userAdmin[ BUILTIN_ROLE_DATA_SIZE_userAdmin ];

   void authInitBuiltinRolePrivileges()
   {
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_root; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_root[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_root[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_root[ i ].second ) );
      }
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_clusterAdmin; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_clusterAdmin[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_clusterAdmin[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_clusterAdmin[ i ].second ) );
      }
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_clusterMonitor; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_clusterMonitor[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_clusterMonitor[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_clusterMonitor[ i ].second ) );
      }
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_backup; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_backup[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_backup[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_backup[ i ].second ) );
      }
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_dbAdmin; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_dbAdmin[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_dbAdmin[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_dbAdmin[ i ].second ) );
      }
      for ( UINT32 i = 0; i < BUILTIN_ROLE_DATA_SIZE_userAdmin; ++i )
      {
         AUTH_BUILTIN_ROLE_PRIVILEGES_userAdmin[ i ] = authPrivilege(
            authResource::forSimpleType( BUILTIN_ROLE_DATA_userAdmin[ i ].first ),
            boost::make_shared< authActionSet >( BUILTIN_ROLE_DATA_userAdmin[ i ].second ) );
      }
   }

   INT32 setCSBuiltinRole( const ossPoolString &cs,
                           const std::pair< RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY > *data,
                           INT32 size,
                           authBuiltinRole &builtinRole )
   {
      INT32 rc = SDB_OK;
      authPrivilege *privileges = SDB_OSS_NEW authPrivilege[ size ];
      if ( !privileges )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "Failed to allocate memory for authPrivilege array" );
         goto error;
      }
      for ( INT32 i = 0; i < size; ++i )
      {
         if ( data[ i ].first == RESOURCE_TYPE_COLLECTION_SPACE )
         {
            boost::shared_ptr< authResource > r = authResource::forCS( cs );
            if ( !r )
            {
               rc = SDB_OOM;
               PD_LOG( PDERROR, "Failed to create authResource for collection space %s, rc: %d",
                       cs.c_str(), rc );
               goto error;
            }
            boost::shared_ptr< authActionSet > actions =
               boost::make_shared< authActionSet >( data[ i ].second );
            if ( !actions )
            {
               rc = SDB_OOM;
               PD_LOG( PDERROR, "Failed to create authActionSet, rc: %d", rc );
               goto error;
            }
            privileges[ i ] = authPrivilege( r, actions );
         }
         else
         {
            boost::shared_ptr< authActionSet > actions =
               boost::make_shared< authActionSet >( data[ i ].second );
            if ( !actions )
            {
               rc = SDB_OOM;
               PD_LOG( PDERROR, "Failed to create authActionSet, rc: %d", rc );
               goto error;
            }
            privileges[ i ] =
               authPrivilege( authResource::forSimpleType( data[ i ].first ), actions );
         }
      }
      builtinRole.set( privileges, size, TRUE );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 authGetBuiltinRole( const CHAR *roleName, authBuiltinRole &builtinRole )
   {
      INT32 rc = SDB_OK;
      if ( !roleName || roleName[ 0 ] != BUILTIN_ROLE_PREFIX )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_ROOT ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_root, BUILTIN_ROLE_DATA_SIZE_root, FALSE );
         goto done;
      }
      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_CLUSTER_ADMIN ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_clusterAdmin,
                          BUILTIN_ROLE_DATA_SIZE_clusterAdmin, FALSE );
         goto done;
      }
      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_CLUSTER_MONITOR ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_clusterMonitor,
                          BUILTIN_ROLE_DATA_SIZE_clusterMonitor, FALSE );
         goto done;
      }
      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_BACKUP ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_backup, BUILTIN_ROLE_DATA_SIZE_backup,
                          FALSE );
         goto done;
      }
      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_DB_ADMIN ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_dbAdmin, BUILTIN_ROLE_DATA_SIZE_dbAdmin,
                          FALSE );
         goto done;
      }
      if ( 0 == ossStrcmp( roleName, AUTH_ROLE_USER_ADMIN ) )
      {
         builtinRole.set( AUTH_BUILTIN_ROLE_PRIVILEGES_userAdmin, BUILTIN_ROLE_DATA_SIZE_userAdmin,
                          FALSE );
         goto done;
      }
      {
         const CHAR *dotIndex = ossStrchr( roleName, '.' );
         if ( dotIndex )
         {
            ossPoolString cs( roleName + 1, dotIndex - roleName - 1 );
            const CHAR *partName = dotIndex + 1;
            if ( 0 == ossStrcmp( partName, AUTH_ROLE_CS_READ ) )
            {
               rc = setCSBuiltinRole( cs, BUILTIN_ROLE_DATA_cs_read, BUILTIN_ROLE_DATA_SIZE_cs_read,
                                      builtinRole );
               PD_RC_CHECK( rc, PDERROR, "Failed to set builtin role %s, rc: %d", roleName, rc );
               goto done;
            }
            else if ( 0 == ossStrcmp( partName, AUTH_ROLE_CS_READ_WRITE ) )
            {
               rc = setCSBuiltinRole( cs, BUILTIN_ROLE_DATA_cs_readWrite,
                                      BUILTIN_ROLE_DATA_SIZE_cs_readWrite, builtinRole );
               PD_RC_CHECK( rc, PDERROR, "Failed to set builtin role %s, rc: %d", roleName, rc );
               goto done;
            }
            else if ( 0 == ossStrcmp( partName, AUTH_ROLE_CS_ADMIN ) )
            {
               rc = setCSBuiltinRole( cs, BUILTIN_ROLE_DATA_cs_admin,
                                      BUILTIN_ROLE_DATA_SIZE_cs_admin, builtinRole );
               PD_RC_CHECK( rc, PDERROR, "Failed to set builtin role %s, rc: %d", roleName, rc );
               goto done;
            }
         }
      }

      rc = SDB_AUTH_ROLE_NOT_EXIST;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 authGetBuiltinRoleBsonVec( BOOLEAN showPrivileges, ossPoolVector< bson::BSONObj > &out )
   {
      INT32 rc = SDB_OK;

      static const CHAR exactCSRead[] = "_exact." AUTH_ROLE_CS_READ;
      static const CHAR exactCSReadWrite[] = "_exact." AUTH_ROLE_CS_READ_WRITE;
      static const CHAR exactCSAdmin[] = "_exact." AUTH_ROLE_CS_ADMIN;

      static const CHAR *builtRoleNames[] = {
         AUTH_ROLE_ROOT,   AUTH_ROLE_CLUSTER_ADMIN, AUTH_ROLE_CLUSTER_MONITOR,
         AUTH_ROLE_BACKUP, AUTH_ROLE_DB_ADMIN,      AUTH_ROLE_USER_ADMIN,
         exactCSRead,      exactCSReadWrite,        exactCSAdmin };

      try
      {
         for ( UINT32 i = 0; i < sizeof( builtRoleNames ) / sizeof( CHAR * ); ++i )
         {
            BSONObjBuilder builder;
            authBuiltinRole builtinRole;
            rc = authGetBuiltinRole( builtRoleNames[ i ], builtinRole );
            PD_RC_CHECK( rc, PDERROR, "Failed to get builtin role %s, rc: %d", builtRoleNames[ i ],
                         rc );
            builder.append( AUTH_FIELD_NAME_ROLENAME, builtRoleNames[ i ] );
            builtinRole.toBSONObj( showPrivileges, builder );

            out.push_back( builder.obj() );
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s. rc: %d", e.what(), rc );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Occured exception: %s, rc: %d",
                 boost::diagnostic_information( e ).c_str(), rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine