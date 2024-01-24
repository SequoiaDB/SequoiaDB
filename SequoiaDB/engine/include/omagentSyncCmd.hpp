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

   Source File Name = omagentSyncCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_SYNC_CMD_HPP_
#define OMAGENT_SYNC_CMD_HPP_

#include "omagentCmdBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   class _omaTaskMgr ;

   /******************************* scan host *********************************/
   /*
      _omaScanHost
   */
   class _omaScanHost : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _omaScanHost () ;
         ~_omaScanHost () ;
         virtual const CHAR* name () { return OMA_CMD_SCAN_HOST ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /******************************* pre-check host ****************************/
   /*
      _omaPreCheckHost
   */
   class _omaPreCheckHost : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _omaPreCheckHost () ;
         ~_omaPreCheckHost () ;
         
      public:
         virtual const CHAR* name () { return OMA_CMD_PRE_CHECK_HOST ; }
         virtual INT32 init ( const CHAR *pInfo ) ;
   } ;

   /******************************* check host ********************************/
   /*
      _omaCheckHost
   */
   class _omaCheckHost : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaCheckHost () ;
         ~_omaCheckHost () ;
         virtual const CHAR* name () { return OMA_CMD_CHECK_HOST ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /******************************* post-check host ***************************/
   /*
      _omaPostCheckHost
   */
   class _omaPostCheckHost : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaPostCheckHost () ;
         ~_omaPostCheckHost () ;
         virtual const CHAR* name () { return OMA_CMD_POST_CHECK_HOST ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /******************************* remove host *******************************/
   /*
      _omaRemoveHost is background command
   */
/*
   class _omaRemoveHost : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _omaRemoveHost () ;
         ~_omaRemoveHost () ;
         
      public:
         virtual const CHAR * name () { return OMA_CMD_REMOVE_HOST ; }
         virtual INT32 init ( const CHAR *pInfo ) ;
   } ;
*/
   /***************************** update hosts table info *********************/
   /*
      _omaUpdateHostsInfo
   */
   class _omaUpdateHostsInfo : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaUpdateHostsInfo () ;
         ~_omaUpdateHostsInfo () ;

         virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         virtual const CHAR * name () { return OMA_CMD_UPDATE_HOSTS ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ; 

   /***************************** query host status ********************/
   /*
      _omaQueryHostStatus
   */
   class _omaQueryHostStatus : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaQueryHostStatus() ;
         ~_omaQueryHostStatus() ;

      public:
         virtual const CHAR* name () { return OMA_CMD_QUERY_HOST_STATUS ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
   } ;

   /***************************** handle task notify **************************/
   /*
      _omaHandleTaskNotify
   */
   class _omaHandleTaskNotify : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaHandleTaskNotify() ;
         ~_omaHandleTaskNotify() ;

      public:
         virtual const CHAR* name () { return OM_NOTIFY_TASK ; }
         virtual INT32 init ( const CHAR *pInstallInfo ) ;
         virtual INT32 doit ( BSONObj &retObj ) ;

      private:
         BSONObj   _taskIDObj ;
   } ;

   /***************************** handle interrupt task ***********************/
   /*
      _omaHandleInterruptTask
   */
   class _omaHandleInterruptTask : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaHandleInterruptTask() ;
         ~_omaHandleInterruptTask() ;

      public:
         virtual const CHAR* name () { return OM_INTERRUPT_TASK_REQ ; }
         virtual INT32 init ( const CHAR *pInterruptInfo ) ;
         virtual INT32 doit ( BSONObj &retObj ) ;

      private:
         INT64    _taskID ;
   } ;

   /***************************** handle ssql get more ************************/
   /*
      _omaHandleSsqlGetMore
   */
   class _omaHandleSsqlGetMore : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER ()
      public:
         _omaHandleSsqlGetMore() ;
         ~_omaHandleSsqlGetMore() ;

      public:
         virtual const CHAR* name () { return OM_SSQL_GET_MORE_REQ ; }
         virtual INT32 init ( const CHAR *pInterruptInfo ) ;
         virtual INT32 doit ( BSONObj &retObj ) ;

      private:
         INT64    _taskID ;
   } ;

   /************************** sync business configure ************************/
   /*
      _omaSyncBuzConfigure
   */
   class _omaSyncBuzConfigure : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaSyncBuzConfigure() ;
      ~_omaSyncBuzConfigure() ;

      virtual const CHAR* name () { return OMA_CMD_SYNC_BUSINESS_CONF ; }
      virtual INT32 init ( const CHAR *pInterruptInfo ) ;

   } ;

   /************************** create relationship ************************/
   /*
      _omaCreateRelationship
   */
   class _omaCreateRelationship : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaCreateRelationship() ;
      ~_omaCreateRelationship() ;

      virtual const CHAR* name () { return OMA_CMD_CREATE_RELATIONSHIP ; }
      virtual INT32 init ( const CHAR *pInfo ) ;

   } ;

   /************************** remove relationship ************************/
   /*
      _omaRemoveRelationship
   */
   class _omaRemoveRelationship : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaRemoveRelationship() ;
      ~_omaRemoveRelationship() ;

      virtual const CHAR* name () { return OMA_CMD_REMOVE_RELATIONSHIP ; }
      virtual INT32 init ( const CHAR *pInfo ) ;

   } ;

   /************************** modify business config ************************/
   /*
      _omaModifyBusinessConfig
   */
   class _omaModifyBusinessConfig : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaModifyBusinessConfig() ;
      ~_omaModifyBusinessConfig() ;

      virtual const CHAR* name () { return OMA_CMD_MODIFY_BUSINESS_CONFIG ; }
      virtual INT32 init ( const CHAR *pInfo ) ;

   } ;

   /************************** get node log ************************/
   /*
      _omaGetNodeLog
   */
   class _omaGetNodeLog : public _omaCommand
   {
   DECLARE_OACMD_AUTO_REGISTER()

   public:
      _omaGetNodeLog() ;
      ~_omaGetNodeLog() ;

      virtual const CHAR* name () { return OMA_CMD_GET_NODE_LOG ; }
      virtual INT32 init ( const CHAR *pInfo ) ;

   } ;

} // namespace engine


#endif // OMAGENT_SYNC_CMD_HPP_
