#ifndef COORD_COMMAND_ROLE_HPP__
#define COORD_COMMAND_ROLE_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"
#include "coordCMDEventHandler.hpp"

using namespace bson;

namespace engine
{
   /*
      _coordCommandRole define
   */
   class _coordCommandRole : public _coordCommandBase
   {
   public:
      _coordCommandRole();
      virtual ~_coordCommandRole();

   public:
      virtual INT32 execute(MsgHeader *pMsg, pmdEDUCB *cb, INT64 &contextID, rtnContextBuf *buf );

   private:
      virtual INT32 _preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs );
      virtual INT32 _execute( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf ,
                              coordCMDArguments *pArgs);
      virtual INT32 _postProcess( coordCMDArguments *pArgs );
      virtual void _doAudit( coordCMDArguments *pArgs, INT32 rc );
      
   };

   /*
      _coordCMDCreateRole define
   */
   class _coordCMDCreateRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDCreateRole();
      virtual ~_coordCMDCreateRole();
   };

   /*
      _coordCMDDropRole define
   */
   class _coordCMDDropRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDDropRole();
      virtual ~_coordCMDDropRole();
   };

   /*
      _coordCMDGetRole define
   */
   class _coordCMDGetRole : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDGetRole();
      virtual ~_coordCMDGetRole();

      virtual INT32 _preProcess( rtnQueryOptions &queryOpt, string &clName, BSONObj &outSelector )
      {
         return SDB_OK;
      }
   };

   /*
      _coordCMDListRoles define
   */
   class _coordCMDListRoles : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDListRoles();
      virtual ~_coordCMDListRoles();

      virtual INT32 _preProcess( rtnQueryOptions &queryOpt, string &clName, BSONObj &outSelector )
      {
         return SDB_OK;
      }
   };

   /*
      _coordCMDUpdateRole define
   */
   class _coordCMDUpdateRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDUpdateRole();
      virtual ~_coordCMDUpdateRole();
   };

   /*
      _coordCMDGrantPrivilegesToRole define
   */
   class _coordCMDGrantPrivilegesToRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDGrantPrivilegesToRole();
      virtual ~_coordCMDGrantPrivilegesToRole();
   };

   /*
      _coordCMDRevokePrivilegesFromRole define
   */
   class _coordCMDRevokePrivilegesFromRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDRevokePrivilegesFromRole();
      virtual ~_coordCMDRevokePrivilegesFromRole();
   };

   /*
      _coordCMDGrantRolesToRole define
   */
   class _coordCMDGrantRolesToRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDGrantRolesToRole();
      virtual ~_coordCMDGrantRolesToRole();
   };

   /*
      _coordCMDRevokeRolesFromRole define
   */
   class _coordCMDRevokeRolesFromRole : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDRevokeRolesFromRole();
      virtual ~_coordCMDRevokeRolesFromRole();
   };

   /*
      _coordCMDGrantRolesToUser define
   */
   class _coordCMDGrantRolesToUser : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();
   public:
      _coordCMDGrantRolesToUser();
      virtual ~_coordCMDGrantRolesToUser();

   private:
      virtual INT32 _preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs );
      virtual INT32 _postProcess( coordCMDArguments *pArgs );
      virtual void _doAudit( coordCMDArguments *pArgs, INT32 rc );
   };

   /*
      _coordCMDRevokeRolesFromUser define
   */
   class _coordCMDRevokeRolesFromUser : public _coordCommandRole
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();
   public:
      _coordCMDRevokeRolesFromUser();
      virtual ~_coordCMDRevokeRolesFromUser();

   private:
      virtual INT32 _preProcess( MsgHeader *pMsg, coordCMDArguments *pArgs );
      virtual INT32 _postProcess( coordCMDArguments *pArgs );
      virtual void _doAudit( coordCMDArguments *pArgs, INT32 rc );
   };

   /*
      _coordCMDGetUser define
   */
   class _coordCMDGetUser : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER();

   public:
      _coordCMDGetUser();
      virtual ~_coordCMDGetUser();

      virtual INT32 _preProcess( rtnQueryOptions &queryOpt, string &clName, BSONObj &outSelector );
   };

} // namespace engine

#endif