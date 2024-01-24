#ifndef AUTH_ROLE_AGENT_HPP__
#define AUTH_ROLE_AGENT_HPP__

#include "ossTypes.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   typedef class _pmdEDUCB pmdEDUCB;
   class _authRoleAgent : public SDBObject
   {
   public:
      virtual ~_authRoleAgent(){};
      virtual INT32 getRole( const CHAR *roleName, bson::BSONObj &out ) = 0;
      virtual INT32 listRoles( ossPoolMap< ossPoolString, bson::BSONObj > &roles ) = 0;
      virtual INT32 createRole( const bson::BSONObj &obj ) = 0;
      virtual INT32 dropRole( const CHAR *roleName ) = 0;
      virtual INT32 updateRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 grantRolesToRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 revokeRolesFromRole( const CHAR *roleName, const bson::BSONObj &obj ) = 0;
      virtual INT32 grantRolesToUser( const CHAR *userName, const bson::BSONObj &obj ) = 0;
      virtual INT32 revokeRolesFromUser( const CHAR *userName, const bson::BSONObj &obj ) = 0;
      virtual INT32 getUser( const CHAR *userName, bson::BSONObj &out ) = 0;
   };
   typedef _authRoleAgent authRoleAgent;
} // namespace engine
#endif