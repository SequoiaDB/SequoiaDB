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

   Source File Name = omagentRemoteUsrSystem.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_USRSYSTEM_HPP_
#define OMAGENT_REMOTE_USRSYSTEM_HPP_

#include "omagentCmdBase.hpp"
#include "omagentRemoteBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _remoteSystemPing define
   */
   class _remoteSystemPing : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemPing() ;

         ~_remoteSystemPing() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemType define
   */
   class _remoteSystemType : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemType() ;

         ~_remoteSystemType() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetReleaseInfo define
   */
   class _remoteSystemGetReleaseInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetReleaseInfo() ;

         ~_remoteSystemGetReleaseInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetHostsMap define
   */
   class _remoteSystemGetHostsMap : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetHostsMap() ;

         ~_remoteSystemGetHostsMap() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetAHostMap define
   */
   class _remoteSystemGetAHostMap : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetAHostMap() ;

         ~_remoteSystemGetAHostMap() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _hostname ;
   } ;

   /*
      _remoteSystemAddAHostMap define
   */
   class _remoteSystemAddAHostMap : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemAddAHostMap() ;

         ~_remoteSystemAddAHostMap() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _hostname ;
         string _ip ;
         BOOLEAN _isReplace ;
   } ;

   /*
      _remoteSystemDelAHostMap define
   */
   class _remoteSystemDelAHostMap : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemDelAHostMap() ;

         ~_remoteSystemDelAHostMap() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _hostname ;
   } ;

   /*
      _remoteSystemGetCpuInfo define
   */
   class _remoteSystemGetCpuInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetCpuInfo() ;

         ~_remoteSystemGetCpuInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemSnapshotCpuInfo define
   */
   class _remoteSystemSnapshotCpuInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemSnapshotCpuInfo() ;

         ~_remoteSystemSnapshotCpuInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetMemInfo define
   */
   class _remoteSystemGetMemInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetMemInfo() ;

         ~_remoteSystemGetMemInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetDiskInfo define
   */
   class _remoteSystemGetDiskInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetDiskInfo() ;

         ~_remoteSystemGetDiskInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetNetcardInfo define
   */
   class _remoteSystemGetNetcardInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetNetcardInfo() ;

         ~_remoteSystemGetNetcardInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemSnapshotNetcardInfo define
   */
   class _remoteSystemSnapshotNetcardInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemSnapshotNetcardInfo() ;

         ~_remoteSystemSnapshotNetcardInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetIpTablesInfo define
   */
   class _remoteSystemGetIpTablesInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetIpTablesInfo() ;

         ~_remoteSystemGetIpTablesInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetHostName define
   */
   class _remoteSystemGetHostName : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetHostName() ;

         ~_remoteSystemGetHostName() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemSniffPort define
   */
   class _remoteSystemSniffPort : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemSniffPort() ;

         ~_remoteSystemSniffPort() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         UINT32 _port ;
   } ;

   /*
      _remoteSystemListProcess define
   */
   class _remoteSystemListProcess : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemListProcess() ;

         ~_remoteSystemListProcess() ;

         INT32 init( const CHAR * pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
      private:
         BOOLEAN _showDetail ;
   } ;

   /*
      _remoteSystemAddUser define
   */
   class _remoteSystemAddUser : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemAddUser() ;

         ~_remoteSystemAddUser() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemSetUserConfigs define
   */
   class _remoteSystemSetUserConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemSetUserConfigs() ;

         ~_remoteSystemSetUserConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemDelUser define
   */
   class _remoteSystemDelUser : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemDelUser() ;

         ~_remoteSystemDelUser() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemAddGroup define
   */
   class _remoteSystemAddGroup : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemAddGroup() ;

         ~_remoteSystemAddGroup() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemDelGroup define
   */
   class _remoteSystemDelGroup : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemDelGroup() ;

         ~_remoteSystemDelGroup() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemListLoginUsers define
   */
   class _remoteSystemListLoginUsers : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemListLoginUsers() ;

         ~_remoteSystemListLoginUsers() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemListAllUsers define
   */
   class _remoteSystemListAllUsers : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemListAllUsers() ;

         ~_remoteSystemListAllUsers() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemListGroups define
   */
   class _remoteSystemListGroups : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemListGroups() ;

         ~_remoteSystemListGroups() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         INT32 _extractGroupsInfo( const CHAR *buf,
                                   BSONObjBuilder &builder,
                                   BOOLEAN showDetail ) ;
   } ;


   /*
      _remoteSystemGetCurrentUser define
   */
   class _remoteSystemGetCurrentUser : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetCurrentUser() ;

         ~_remoteSystemGetCurrentUser() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemKillProcess define
   */
   class _remoteSystemKillProcess : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemKillProcess () ;

         ~_remoteSystemKillProcess () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetProcUlimitConfigs define
   */
   class _remoteSystemGetProcUlimitConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetProcUlimitConfigs () ;

         ~_remoteSystemGetProcUlimitConfigs () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemSetProcUlimitConfigs define
   */
   class _remoteSystemSetProcUlimitConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemSetProcUlimitConfigs () ;

         ~_remoteSystemSetProcUlimitConfigs () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetSystemConfigs define
   */
   class _remoteSystemGetSystemConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetSystemConfigs () ;

         ~_remoteSystemGetSystemConfigs () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         INT32 _getSystemInfo( std::vector< std::string > typeSplit,
                               bson::BSONObjBuilder &builder ) ;
   } ;

   /*
      _remoteSystemRunService define
   */
   class _remoteSystemRunService : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemRunService () ;

         ~_remoteSystemRunService () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemBuildTrusty define
   */
   class _remoteSystemBuildTrusty : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemBuildTrusty () ;

         ~_remoteSystemBuildTrusty () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         string _pubKey ;
   } ;

   /*
      _remoteSystemRemoveTrusty define
   */
   class _remoteSystemRemoveTrusty : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemRemoveTrusty () ;

         ~_remoteSystemRemoveTrusty () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         INT32 _removeKey( const CHAR* buf, string matchStr ) ;
   } ;

   /*
      _remoteSystemGetUserEnv define
   */
   class _remoteSystemGetUserEnv : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetUserEnv () ;

         ~_remoteSystemGetUserEnv () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      private:
         INT32 _extractEnvInfo( const CHAR *buf,
                                bson::BSONObjBuilder &builder ) ;
   } ;

   /*
      _remoteSystemGetPID define
   */
   class _remoteSystemGetPID : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetPID () ;

         ~_remoteSystemGetPID () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetTID define
   */
   class _remoteSystemGetTID : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetTID () ;

         ~_remoteSystemGetTID () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteSystemGetEWD define
   */
   class _remoteSystemGetEWD : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteSystemGetEWD () ;

         ~_remoteSystemGetEWD () ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;
}
#endif
