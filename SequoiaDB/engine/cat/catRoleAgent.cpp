#include "catRoleAgent.hpp"
#include "authDef.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"

namespace engine
{
   // implementation of catRoleAgent
   _catRoleAgent::_catRoleAgent( pmdEDUCB *cb, SDB_RTNCB *rtnCB, SDB_DMSCB *dmsCB )
   : _pEduCB( cb ), _pRtnCB( rtnCB ), _pDmsCB( dmsCB )
   {
      SDB_ASSERT( _pRtnCB && _pDmsCB, "can not be nullptr" );
   }

   _catRoleAgent::~_catRoleAgent() {}

   INT32 catRoleAgent::getRole( const CHAR *roleName, bson::BSONObj &out )
   {
      INT32 rc = SDB_OK;

      const BSONObj matcher = BSON( AUTH_FIELD_NAME_ROLENAME << roleName );
      const BSONObj hint = BSON( "" << AUTH_ROLE_INDEX_NAME );
      const BSONObj dummy;
      INT64 contextID = -1;
      rtnContextBuf buffObj;

      rc = rtnQuery( AUTH_ROLE_COLLECTION, dummy, matcher, dummy, hint, 0, _pEduCB, 0, 1, _pDmsCB,
                     _pRtnCB, contextID );
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, rc: %d", AUTH_ROLE_COLLECTION, rc );

      rc = rtnGetMore( contextID, 1, buffObj, _pEduCB, _pRtnCB );
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_AUTH_ROLE_NOT_EXIST;
            contextID = -1;
            goto error;
         }
         PD_LOG( PDERROR, "Failed to fetch from %s collection, rc: %d", AUTH_ROLE_COLLECTION, rc );
         goto error;
      }

      out = BSONObj( buffObj.data() ).getOwned();

   done:
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, _pEduCB );
      }
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::listRoles( ossPoolMap< ossPoolString, bson::BSONObj > &roles )
   {
      INT32 rc = SDB_OK;
      INT64 contextID = -1;
      rtnContextBuf buffObj;
      const BSONObj dummy;

      rc = rtnQuery( AUTH_ROLE_COLLECTION, dummy, dummy, dummy, dummy, 0, _pEduCB, 0, -1, _pDmsCB,
                     _pRtnCB, contextID );
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, rc: %d", AUTH_ROLE_COLLECTION, rc );

      while ( TRUE )
      {
         rc = rtnGetMore( contextID, 1, buffObj, _pEduCB, _pRtnCB );
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK;
            contextID = -1;
            break;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to fetch from %s collection, rc: %d",
                      AUTH_ROLE_COLLECTION, rc );
         BSONObj roleObj = BSONObj( buffObj.data() );
         roles.insert( std::pair< ossPoolString, BSONObj >(
            roleObj.getStringField( AUTH_FIELD_NAME_ROLENAME ), roleObj.getOwned() ) );
      }

   done:
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, _pEduCB );
      }
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::createRole( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;

      rc = rtnInsert( AUTH_ROLE_COLLECTION, obj, 1, 0, _pEduCB );
      if ( rc )
      {
         if ( SDB_IXM_DUP_KEY == rc )
         {
            PD_LOG( PDERROR, "Role already exists" );
            rc = SDB_AUTH_ROLE_EXIST;
            goto error;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to insert role object into [%s], rc = %d",
                    AUTH_ROLE_COLLECTION, rc );
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::dropRole( const CHAR *roleName )
   {
      INT32 rc = SDB_OK;
      const BSONObj matcher = BSON( AUTH_FIELD_NAME_ROLENAME << roleName );
      const BSONObj hint = BSON( "" << AUTH_ROLE_INDEX_NAME );
      utilDeleteResult result;
      rc = rtnDelete( AUTH_ROLE_COLLECTION, matcher, hint, 0, _pEduCB, &result );
      PD_RC_CHECK( rc, PDERROR, "Failed to delete role[%s], rc: %d", roleName, rc );
      if ( 0 == result.deletedNum() )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
         PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
         goto error;
      }

      {
         BSONObj updater = BSON( "$pull" << BSON( AUTH_FIELD_NAME_ROLES << roleName ) );
         BSONObj dummy = BSONObj();
         rc = rtnUpdate( AUTH_USR_COLLECTION, dummy, updater, hint, 0, _pEduCB );
         PD_RC_CHECK( rc, PDERROR, "Failed to drop role[%s] for users, rc: %d", roleName, rc );

         rc = rtnUpdate( AUTH_ROLE_COLLECTION, dummy, updater, hint, 0, _pEduCB );
         PD_RC_CHECK( rc, PDERROR, "Failed to drop role[%s] for users, rc: %d", roleName, rc );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::updateRole( const CHAR *roleName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      const BSONObj matcher = BSON( AUTH_FIELD_NAME_ROLENAME << roleName );
      const BSONObj hint = BSON( "" << AUTH_ROLE_INDEX_NAME );
      BSONObjBuilder builder;
      BSONObjBuilder subBuilder( builder.subobjStart( "$set" ) );
      BSONElement privEle = obj.getField( AUTH_FIELD_NAME_PRIVILEGES );
      BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
      if ( privEle.ok() )
      {
         subBuilder.append( privEle );
      }
      if ( rolesEle.ok() )
      {
         subBuilder.append( rolesEle );
      }
      subBuilder.doneFast();

      utilUpdateResult result;
      rc = rtnUpdate( AUTH_ROLE_COLLECTION, matcher, builder.done(), hint, 0, _pEduCB, &result );
      PD_RC_CHECK( rc, PDERROR, "Failed to update role[%s], rc: %d", roleName, rc );
      if ( 0 == result.updateNum() )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
         PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::grantRolesToRole( const CHAR *roleName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      const BSONObj matcher = BSON( AUTH_FIELD_NAME_ROLENAME << roleName );
      const BSONObj hint = BSON( "" << AUTH_ROLE_INDEX_NAME );

      BSONObjBuilder builder;
      BSONObjBuilder subBuilder( builder.subobjStart( "$addtoset" ) );
      BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
      if ( rolesEle.ok() )
      {
         subBuilder.append( rolesEle );
      }
      subBuilder.doneFast();

      utilUpdateResult result;
      rc = rtnUpdate( AUTH_ROLE_COLLECTION, matcher, builder.done(), hint, 0, _pEduCB, &result );
      PD_RC_CHECK( rc, PDERROR, "Failed to update role[%s], rc: %d", roleName, rc );
      if ( 0 == result.updateNum() )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
         PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::revokeRolesFromRole( const CHAR *roleName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      const BSONObj matcher = BSON( AUTH_FIELD_NAME_ROLENAME << roleName );
      const BSONObj hint = BSON( "" << AUTH_ROLE_INDEX_NAME );

      BSONObjBuilder builder;
      BSONObjBuilder subBuilder( builder.subobjStart( "$pull_all" ) );
      BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
      if ( rolesEle.ok() )
      {
         subBuilder.append( rolesEle );
      }
      subBuilder.doneFast();

      utilUpdateResult result;
      rc = rtnUpdate( AUTH_ROLE_COLLECTION, matcher, builder.done(), hint, 0, _pEduCB, &result );
      PD_RC_CHECK( rc, PDERROR, "Failed to update role[%s], rc: %d", roleName, rc );
      if ( 0 == result.updateNum() )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
         PD_LOG( PDERROR, "Role[%s] not exist, rc: %d", roleName, rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::grantRolesToUser( const CHAR *userName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      const BSONObj matcher = BSON( FIELD_NAME_USER << userName );

      BSONObjBuilder builder;
      BSONObjBuilder subBuilder( builder.subobjStart( "$addtoset" ) );
      BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
      if ( rolesEle.ok() )
      {
         subBuilder.append( rolesEle );
      }
      subBuilder.doneFast();

      utilUpdateResult result;
      rc =
         rtnUpdate( AUTH_USR_COLLECTION, matcher, builder.done(), BSONObj(), 0, _pEduCB, &result );
      PD_RC_CHECK( rc, PDERROR, "Failed to update roles of user[%s], rc: %d", userName, rc );
      if ( 0 == result.updateNum() )
      {
         rc = SDB_AUTH_USER_NOT_EXIST;
         PD_LOG( PDERROR, "User[%s] not exist, rc: %d", userName, rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::revokeRolesFromUser( const CHAR *userName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;

      rc = _checkRevokeLastRoot( userName, obj );
      PD_RC_CHECK( rc, PDERROR, "Failed to check revoke last root, rc: %d", rc );

      {
         const BSONObj matcher = BSON( FIELD_NAME_USER << userName );

         BSONObjBuilder builder;
         BSONObjBuilder subBuilder( builder.subobjStart( "$pull_all" ) );
         BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
         if ( rolesEle.ok() )
         {
            subBuilder.append( rolesEle );
         }
         subBuilder.doneFast();

         utilUpdateResult result;
         rc = rtnUpdate( AUTH_USR_COLLECTION, matcher, builder.done(), BSONObj(), 0, _pEduCB,
                         &result );
         PD_RC_CHECK( rc, PDERROR, "Failed to update roles of user[%s], rc: %d", userName, rc );
         if ( 0 == result.updateNum() )
         {
            rc = SDB_AUTH_USER_NOT_EXIST;
            PD_LOG( PDERROR, "User[%s] not exist, rc: %d", userName, rc );
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 catRoleAgent::getUser( const CHAR *userName, bson::BSONObj &out )
   {
      INT32 rc = SDB_OK;

      const BSONObj matcher = BSON( FIELD_NAME_USER << userName );
      const BSONObj dummy;
      INT64 contextID = -1;
      rtnContextBuf buffObj;

      rc = rtnQuery( AUTH_USR_COLLECTION, dummy, matcher, dummy, dummy, 0, _pEduCB, 0, 1, _pDmsCB,
                     _pRtnCB, contextID );
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, rc: %d", AUTH_USR_COLLECTION, rc );

      rc = rtnGetMore( contextID, 1, buffObj, _pEduCB, _pRtnCB );
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_AUTH_USER_NOT_EXIST;
            contextID = -1;
            goto error;
         }
         PD_LOG( PDERROR, "Failed to fetch from %s collection, rc: %d", AUTH_USR_COLLECTION, rc );
         goto error;
      }

      out = BSONObj( buffObj.data() ).getOwned();

   done:
      if ( -1 != contextID )
      {
         _pRtnCB->contextDelete( contextID, _pEduCB );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _catRoleAgent::_checkRevokeLastRoot( const CHAR *userName, const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK;
      BSONElement rolesEle = obj.getField( AUTH_FIELD_NAME_ROLES );
      if ( rolesEle.type() != Array )
      {
         rc = SDB_INVALIDARG;
         PD_LOG_MSG( PDERROR, "Invalid type of roles to revoke, rc: %d", rc );
         goto error;
      }

      for ( BSONObjIterator it( rolesEle.Obj() ); it.more(); )
      {
         BSONElement ele = it.next();
         if ( ele.type() != String )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG( PDERROR, "Invalid type of role name to revoke, rc: %d", rc );
            goto error;
         }
         if ( ossStrcmp( AUTH_ROLE_ROOT, ele.valuestrsafe() ) == 0 )
         {
            INT64 count = 0;
            rtnQueryOptions queryOption;

            BSONObj query =
               BSON( "$and" << BSON_ARRAY( BSON( FIELD_NAME_USER << BSON( "$ne" << userName ) )
                                           << BSON( FIELD_NAME_ROLES << AUTH_ROLE_ROOT ) ) );
            queryOption.setCLFullName( AUTH_USR_COLLECTION );
            queryOption.setQuery( query );
            rc = rtnGetCount( queryOption, _pDmsCB, _pEduCB, _pRtnCB, &count );
            PD_RC_CHECK( rc, PDERROR, "Get user number failed, rc: %d", rc );

            if ( 0 == count )
            {
               rc = SDB_OPERATION_DENIED;
               PD_LOG_MSG( PDERROR,
                           "Only users without %s role remain after "
                           "revoking roles, rc: %d",
                           AUTH_ROLE_ROOT, rc );
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      goto done;
   }
} // namespace engine