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

   Source File Name = omagentRemoteUsrOma.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_USROMA_HPP_
#define OMAGENT_REMOTE_USROMA_HPP_

#include "omagentRemoteBase.hpp"
#include "pmdDef.hpp"
#include "utilNodeOpr.hpp"

using namespace bson ;

namespace engine
{
   struct _sdbToolListParam
   {
      vector< string >     _svcnames ;
      INT32                _typeFilter ;
      INT32                _modeFilter ;
      INT32                _roleFilter ;
      BOOLEAN              _showAlone ;
      BOOLEAN              _expand ;

      _sdbToolListParam()
      {
         _typeFilter    = SDB_TYPE_DB ;
         _modeFilter    = RUN_MODE_RUN ;
         _roleFilter    = -1 ;
         _showAlone     = FALSE ;
         _expand        = FALSE ;
      }
   } ;

   enum SPT_MATCH_PRED
   {
      SPT_MATCH_AND,
      SPT_MATCH_OR,
      SPT_MATCH_NOT
   } ;

   /*
      _remoteOmaGetOmaInstallFile define
   */
   class _remoteOmaGetOmaInstallFile : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetOmaInstallFile() ;

         ~_remoteOmaGetOmaInstallFile() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaGetOmaInstallInfo define
   */
   class _remoteOmaGetOmaInstallInfo : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetOmaInstallInfo() ;

         ~_remoteOmaGetOmaInstallInfo() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaGetOmaConfigFile define
   */
   class _remoteOmaGetOmaConfigFile : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetOmaConfigFile() ;

         ~_remoteOmaGetOmaConfigFile() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaGetOmaConfigs define
   */
   class _remoteOmaGetOmaConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetOmaConfigs() ;

         ~_remoteOmaGetOmaConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaNodesOperation define
   */
   class _remoteOmaNodesOperation : public _remoteExec
   {
   public:
      _remoteOmaNodesOperation() ;

      ~_remoteOmaNodesOperation() ;

      virtual const CHAR * name() = 0 ;

   protected:
      INT32 _runNodesJob( BOOLEAN isStartNodes ) ;
      void _mergeResult( BOOLEAN isStartNodes, BSONObj &retObj ) ;

   private:
      list< pair<EDUID,string> > _jobList ;
   } ;

   /*
      _remoteOmaStartNodes define
   */
   class _remoteOmaStartNodes : public _remoteOmaNodesOperation
   {
   DECLARE_OACMD_AUTO_REGISTER()
   public:
      _remoteOmaStartNodes() ;

      ~_remoteOmaStartNodes() ;

      const CHAR *name() ;

      INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaStopNodes define
   */
   class _remoteOmaStopNodes : public _remoteOmaNodesOperation
   {
   DECLARE_OACMD_AUTO_REGISTER()
   public:
      _remoteOmaStopNodes() ;

      ~_remoteOmaStopNodes() ;

      const CHAR *name() ;

      INT32 doit( BSONObj &retObj ) ;
   } ;


   /*
      _remoteOmaGetIniConfigs define
   */
   class _remoteOmaGetIniConfigs : public _remoteExec
   {
   DECLARE_OACMD_AUTO_REGISTER()
   public:
      _remoteOmaGetIniConfigs() ;

      ~_remoteOmaGetIniConfigs() ;

      const CHAR *name() ;

      INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaSetIniConfigs define
   */
   class _remoteOmaSetIniConfigs : public _remoteExec
   {
   DECLARE_OACMD_AUTO_REGISTER()
   public:
      _remoteOmaSetIniConfigs() ;

      ~_remoteOmaSetIniConfigs() ;

      const CHAR *name() ;

      INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaSetOmaConfigs define
   */
   class _remoteOmaSetOmaConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaSetOmaConfigs() ;

         ~_remoteOmaSetOmaConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaGetAOmaSvcName define
   */
   class _remoteOmaGetAOmaSvcName : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetAOmaSvcName() ;

         ~_remoteOmaGetAOmaSvcName() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaAddAOmaSvcName define
   */
   class _remoteOmaAddAOmaSvcName : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaAddAOmaSvcName() ;

         ~_remoteOmaAddAOmaSvcName() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaDelAOmaSvcName define
   */
   class _remoteOmaDelAOmaSvcName : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaDelAOmaSvcName() ;

         ~_remoteOmaDelAOmaSvcName() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaListNodes define
   */
   class _remoteOmaListNodes : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaListNodes() ;

         ~_remoteOmaListNodes() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

      protected:
         INT32 _parseListParam( const bson::BSONObj &option,
                                _sdbToolListParam &param,
                                string &errMsg ) ;
         bson::BSONObj _nodeInfo2Bson( const utilNodeInfo &info,
                                       const bson::BSONObj &conf ) ;

         bson::BSONObj _getConfObj( const CHAR *rootPath,
                                    const CHAR *localPath,
                                    const CHAR *svcname,
                                    INT32 type ) ;
   } ;

   /*
      _remoteOmaGetNodeConfigs define
   */
   class _remoteOmaGetNodeConfigs : public _remoteOmaConfigs
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaGetNodeConfigs() ;

         ~_remoteOmaGetNodeConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaSetNodeConfigs define
   */
   class _remoteOmaSetNodeConfigs : public _remoteOmaConfigs
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaSetNodeConfigs() ;

         ~_remoteOmaSetNodeConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaUpdateNodeConfigs define
   */
   class _remoteOmaUpdateNodeConfigs : public _remoteOmaConfigs
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaUpdateNodeConfigs() ;

         ~_remoteOmaUpdateNodeConfigs() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaReloadConfigs define
   */
   class _remoteOmaReloadConfigs : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaReloadConfigs() ;

         ~_remoteOmaReloadConfigs() ;

         virtual const CHAR *name() ;

         virtual INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteOmaTest define
   */
   class _remoteOmaTest : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteOmaTest() ;

         ~_remoteOmaTest() ;

         virtual const CHAR *name() ;

         virtual INT32 doit( BSONObj &retObj ) ;
   } ;
}
#endif
