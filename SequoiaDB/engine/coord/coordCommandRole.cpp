#include "coordCommandRole.hpp"
#include "coordTrace.hpp"
#include "rtnCB.hpp"

namespace engine
{
   /*
      _coordCommandRole define
   */
   _coordCommandRole::_coordCommandRole() {}

   _coordCommandRole::~_coordCommandRole() {}

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_ROLE_EXE, "_coordCommandRole::execute" )
   INT32 _coordCommandRole::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( COORD_CMD_ROLE_EXE );
      coordCMDArguments args;
      rc = _preProcess( pMsg, &args );
      PD_RC_CHECK( rc, PDERROR, "Pre process failed, rc: %d", rc ) ;

      rc = _execute( pMsg, cb, contextID, buf, &args );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute command [%s] on catalog, "
                   "rc: %d",
                   getName(), rc );

      rc = _postProcess( &args );
      PD_RC_CHECK( rc, PDERROR, "Post process failed, rc: %d", rc ) ;

   done:
      _doAudit( &args, rc );
      PD_TRACE_EXITRC( COORD_CMD_ROLE_EXE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_ROLE__EXECUTE, "_coordCommandRole::_execute" )
   INT32 _coordCommandRole::_execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf,
                                      coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( COORD_CMD_ROLE__EXECUTE );
      rc = executeOnCataGroup( pMsg, NULL, TRUE, NULL, NULL, buf );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to execute command [%s] on catalog, "
                   "rc: %d",
                   getName(), rc );

   done:
      PD_TRACE_EXITRC( COORD_CMD_ROLE__EXECUTE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_ROLE__PRE_PROCESS, "_coordCommandRole::_preProcess" )
   INT32 _coordCommandRole::_preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      const CHAR *pQuery = NULL;
      BSONElement roleName;
      PD_TRACE_ENTRY( COORD_CMD_ROLE__PRE_PROCESS );
      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL, &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Parse message failed, rc: %d", rc ) ;

      pArgs->_boQuery = BSONObj( pQuery ).getOwned();
      roleName = pArgs->_boQuery.getField( FIELD_NAME_ROLE );
      if ( String != roleName.type() )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid type of field %s, rc: %d", FIELD_NAME_ROLE, rc );
         goto error;
      }
      pArgs->_targetName = std::string( roleName.valuestr() );

   done:
      PD_TRACE_EXITRC( COORD_CMD_ROLE__PRE_PROCESS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_ROLE__POST_PROCESS, "_coordCommandRole::_postProcess" )
   INT32 _coordCommandRole::_postProcess( coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(COORD_CMD_ROLE__POST_PROCESS);
      sdbGetRTNCB()->getUserCacheMgr()->clear();
      PD_TRACE_EXITRC(COORD_CMD_ROLE__POST_PROCESS, rc);
      return rc;
   }


   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_ROLE__DO_AUDIT, "_coordCommandRole::_doAudit" )
   void _coordCommandRole::_doAudit( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY( COORD_CMD_ROLE__DO_AUDIT );
      /// AUDIT
      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DCL, getName(), AUDIT_OBJ_ROLE, pArgs->_targetName.c_str(), rc,
                           "Option: %s", pArgs->_boQuery.toString().c_str() );
      }
      PD_TRACE_EXIT( COORD_CMD_ROLE__DO_AUDIT );
   }

   /*
      _coordCMDCreateRole implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateRole, CMD_NAME_CREATE_ROLE, FALSE );
   _coordCMDCreateRole::_coordCMDCreateRole() {}

   _coordCMDCreateRole::~_coordCMDCreateRole() {}

   

   /*
      _coordCMDDropRole implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropRole, CMD_NAME_DROP_ROLE, FALSE );
   _coordCMDDropRole::_coordCMDDropRole() {}

   _coordCMDDropRole::~_coordCMDDropRole() {}

   /*
      _coordCMDGetRole implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetRole, CMD_NAME_GET_ROLE, TRUE );
   _coordCMDGetRole::_coordCMDGetRole() {}

   _coordCMDGetRole::~_coordCMDGetRole() {}

   /*
      _coordCMDListRoles implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListRoles, CMD_NAME_LIST_ROLES, TRUE );
   _coordCMDListRoles::_coordCMDListRoles() {}

   _coordCMDListRoles::~_coordCMDListRoles() {}

   /*
      _coordCMDUpdateRole implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDUpdateRole, CMD_NAME_UPDATE_ROLE, FALSE );
   _coordCMDUpdateRole::_coordCMDUpdateRole() {}

   _coordCMDUpdateRole::~_coordCMDUpdateRole() {}

   /*
      _coordCMDGrantPrivilegesToRole implement
   */

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGrantPrivilegesToRole,
                                      CMD_NAME_GRANT_PRIVILEGES,
                                      FALSE );
   _coordCMDGrantPrivilegesToRole::_coordCMDGrantPrivilegesToRole() {}

   _coordCMDGrantPrivilegesToRole::~_coordCMDGrantPrivilegesToRole() {}

   /*
      _coordCMDRevokePrivilegesFromRole implement
   */

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRevokePrivilegesFromRole,
                                      CMD_NAME_REVOKE_PRIVILEGES,
                                      FALSE );

   _coordCMDRevokePrivilegesFromRole::_coordCMDRevokePrivilegesFromRole() {}

   _coordCMDRevokePrivilegesFromRole::~_coordCMDRevokePrivilegesFromRole() {}

   /*
      _coordCMDGrantRolesToRole implement
   */

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGrantRolesToRole,
                                      CMD_NAME_GRANT_ROLES_TO_ROLE,
                                      FALSE );

   _coordCMDGrantRolesToRole::_coordCMDGrantRolesToRole() {}

   _coordCMDGrantRolesToRole::~_coordCMDGrantRolesToRole() {}

   /*
      _coordCMDRevokeRolesFromRole implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRevokeRolesFromRole,
                                      CMD_NAME_REVOKE_ROLES_FROM_ROLE,
                                      FALSE );

   _coordCMDRevokeRolesFromRole::_coordCMDRevokeRolesFromRole() {}

   _coordCMDRevokeRolesFromRole::~_coordCMDRevokeRolesFromRole() {}

   /*
      _coordCMDGrantRolesToUser implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGrantRolesToUser,
                                      CMD_NAME_GRANT_ROLES_TO_USER,
                                      FALSE );

   _coordCMDGrantRolesToUser::_coordCMDGrantRolesToUser() {}

   _coordCMDGrantRolesToUser::~_coordCMDGrantRolesToUser() {}

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_GRANT_ROLES_TO_USER__PRE_PROCESS, "_coordCMDGrantRolesToUser::_preProcess" )
   INT32 _coordCMDGrantRolesToUser::_preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      const CHAR *pQuery = NULL;
      BSONElement userName;
      PD_TRACE_ENTRY( COORD_CMD_GRANT_ROLES_TO_USER__PRE_PROCESS );
      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL, &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Parse message failed, rc: %d", rc ) ;

      pArgs->_boQuery = BSONObj( pQuery ).getOwned();
      userName = pArgs->_boQuery.getField( FIELD_NAME_USER );
      if ( String != userName.type() )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid type of field %s, rc: %d", FIELD_NAME_ROLE, rc );
         goto error;
      }
      pArgs->_targetName = std::string( userName.valuestr() );

   done:
      PD_TRACE_EXITRC( COORD_CMD_GRANT_ROLES_TO_USER__PRE_PROCESS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_GRANT_ROLES_TO_USER__POST_PROCESS, "_coordCMDGrantRolesToUser::_postProcess" )
   INT32 _coordCMDGrantRolesToUser::_postProcess( coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(COORD_CMD_GRANT_ROLES_TO_USER__POST_PROCESS);
      sdbGetRTNCB()->getUserCacheMgr()->remove( ossPoolString( pArgs->_targetName.data() ) );
      PD_TRACE_EXITRC(COORD_CMD_GRANT_ROLES_TO_USER__POST_PROCESS, rc);
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_GRANT_ROLES_TO_USER__DOAUDIT, "_coordCMDGrantRolesToUser::_doAudit" )
   void _coordCMDGrantRolesToUser::_doAudit( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY( COORD_CMD_GRANT_ROLES_TO_USER__DOAUDIT );
      /// AUDIT
      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DCL, getName(), AUDIT_OBJ_USER, pArgs->_targetName.c_str(), rc,
                           "Option: %s", pArgs->_boQuery.toString().c_str() );
      }
      PD_TRACE_EXIT( COORD_CMD_GRANT_ROLES_TO_USER__DOAUDIT );
   }

   /*
      _coordCMDRevokeRolesFromUser implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRevokeRolesFromUser,
                                      CMD_NAME_REVOKE_ROLES_FROM_USER,
                                      FALSE );

   _coordCMDRevokeRolesFromUser::_coordCMDRevokeRolesFromUser() {}

   _coordCMDRevokeRolesFromUser::~_coordCMDRevokeRolesFromUser() {}

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_REVOKE_ROLES_FROM_USER__PRE_PROCESS, "_coordCMDRevokeRolesFromUser::_preProcess" )
   INT32 _coordCMDRevokeRolesFromUser::_preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      const CHAR *pQuery = NULL;
      BSONElement userName;
      PD_TRACE_ENTRY( COORD_CMD_REVOKE_ROLES_FROM_USER__PRE_PROCESS );
      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL, &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Parse message failed, rc: %d", rc ) ;

      pArgs->_boQuery = BSONObj( pQuery ).getOwned();
      userName = pArgs->_boQuery.getField( FIELD_NAME_USER );
      if ( String != userName.type() )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid type of field %s, rc: %d", FIELD_NAME_ROLE, rc );
         goto error;
      }
      pArgs->_targetName = std::string( userName.valuestr() );

   done:
      PD_TRACE_EXITRC( COORD_CMD_REVOKE_ROLES_FROM_USER__PRE_PROCESS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_REVOKE_ROLES_FROM_USER__POST_PROCESS, "_coordCMDRevokeRolesFromUser::_postProcess" )
   INT32 _coordCMDRevokeRolesFromUser::_postProcess( coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY(COORD_CMD_REVOKE_ROLES_FROM_USER__POST_PROCESS);
      sdbGetRTNCB()->getUserCacheMgr()->remove( ossPoolString( pArgs->_targetName.data() ) );
      PD_TRACE_EXITRC(COORD_CMD_REVOKE_ROLES_FROM_USER__POST_PROCESS, rc);
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMD_REVOKE_ROLES_FROM_USER__DOAUDIT, "_coordCMDRevokeRolesFromUser::_doAudit" )
   void _coordCMDRevokeRolesFromUser::_doAudit( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY( COORD_CMD_REVOKE_ROLES_FROM_USER__DOAUDIT );
      /// AUDIT
      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_DCL, getName(), AUDIT_OBJ_USER, pArgs->_targetName.c_str(), rc,
                           "Option: %s", pArgs->_boQuery.toString().c_str() );
      }
      PD_TRACE_EXIT( COORD_CMD_REVOKE_ROLES_FROM_USER__DOAUDIT );
   }

   /*
      _coordCMDGetUser implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetUser, CMD_NAME_GET_USER, TRUE );
   _coordCMDGetUser::_coordCMDGetUser() {}
   
   _coordCMDGetUser::~_coordCMDGetUser() {}

   INT32 _coordCMDGetUser::_preProcess( rtnQueryOptions &queryOpt,
                                        string &clName,
                                        BSONObj &outSelector )
   {
      BSONObjBuilder builder ;

      BSONObjBuilder passBuilder( builder.subobjStart( SDB_AUTH_PASSWD ) ) ;
      passBuilder.append( "$include", 0 ) ;
      passBuilder.done() ;

      BSONObjBuilder idBuilder( builder.subobjStart( DMS_ID_KEY_NAME ) ) ;
      idBuilder.append( "$include", 0 ) ;
      idBuilder.done() ;

      BSONObjBuilder scramBuilder( builder.subobjStart( SDB_AUTH_SCRAMSHA256 ) ) ;
      scramBuilder.append( "$include", 0 ) ;
      scramBuilder.done() ;

      BSONObjBuilder scram1Builder( builder.subobjStart( SDB_AUTH_SCRAMSHA1 ) ) ;
      scram1Builder.append( "$include", 0 ) ;
      scram1Builder.done() ;

      outSelector = builder.obj() ;
      return SDB_OK ;
   }

} // namespace engine