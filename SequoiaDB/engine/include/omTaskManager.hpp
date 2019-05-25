/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = omTaskManager.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/12/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_TASKMANAGER_HPP_
#define OM_TASKMANAGER_HPP_

#include "rtnCB.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "omManager.hpp"
#include <map>
#include <string>

#include <vector>
#include <string>
#include <map>

using namespace std ;
using namespace bson ;

namespace engine
{

   class omTaskBase : public SDBObject
   {
      public:
         omTaskBase() ;
         virtual ~omTaskBase() ;
      public:
         virtual INT32     finish( BSONObj &resultInfo ) = 0 ;

         virtual INT32     getType() = 0 ;

         virtual INT64     getTaskID() = 0 ;

         virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;
   };

   class omAddHostTask : public omTaskBase
   {
      public:
         omAddHostTask( INT64 taskID ) ;
         virtual ~omAddHostTask() ;

      public:
         virtual INT32     finish( BSONObj &resultInfo ) ;

         virtual INT32     getType() ;

         virtual INT64     getTaskID() ;

         virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;

      private:
         void _getPackageVersion( const BSONObj resultInfo,
                                  const string &hostName,
                                  string &version ) ;

         void              _getOMVersion( string &version ) ;

         INT32             _getSuccessHost( BSONObj &resultInfo, 
                                            set<string> &successHostSet ) ;

      private:
         INT64             _taskID ;
         INT32             _taskType ;
   } ;

   class omRemoveHostTask : public omTaskBase
   {
      public:
         omRemoveHostTask( INT64 taskID ) ;
         virtual ~omRemoveHostTask() ;

      public:
         virtual INT32     finish( BSONObj &resultInfo ) ;

         virtual INT32     getType() ;

         virtual INT64     getTaskID() ;

         virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;

      private:
         INT32             _getSuccessHost( BSONObj &resultInfo, 
                                            set<string> &successHostSet ) ;

      private:
         INT64             _taskID ;
         INT32             _taskType ;
   } ;

   class omAddBusinessTask : public omTaskBase
   {
   public:
      omAddBusinessTask( INT64 taskID ) ;
      virtual ~omAddBusinessTask() ;

   public:
      virtual INT32     finish( BSONObj &resultInfo ) ;

      virtual INT32     getType() ;

      virtual INT64     getTaskID() ;

      virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;

   private:
      INT32             _storeBusinessInfo( BSONObj &taskInfoValue ) ;

      INT32             _updateBizHostInfo( const string &businessName ) ;

      INT32             _storeConfigInfo( BSONObj &taskInfoValue ) ;

   private:
      INT64             _taskID ;
      INT32             _taskType ;
   } ;

   class omExtendBusinessTask : public omTaskBase
   {
   public:
      omExtendBusinessTask( INT64 taskID ) ;
      virtual ~omExtendBusinessTask() ;

   public:
      virtual INT32 finish( BSONObj &resultInfo ) ;

      virtual INT32 getType() ;

      virtual INT64 getTaskID() ;

   private:
      INT32 _updateBizHostInfo( const string &businessName ) ;
      INT32 _storeConfigInfo( const BSONObj &taskInfoValue ) ;

   private:
      INT64 _taskID ;
      INT32 _taskType ;
   } ;

   class omShrinkBusinessTask : public omTaskBase
   {
   public:
      omShrinkBusinessTask( INT64 taskID ) ;
      virtual ~omShrinkBusinessTask() ;
   
   public:
      virtual INT32 finish( BSONObj &resultInfo ) ;
   
      virtual INT32 getType() ;
   
      virtual INT64 getTaskID() ;

   private:
      INT32 _removeNodeConfig( const string &businessName,
                               const string &hostName,
                               const string &svcname ) ;
      INT32 _removeConfig( const BSONObj &taskInfo,
                           const BSONObj &resultInfo ) ;
      INT32 _updateBizHostInfo( const string &businessName ) ;

   private:
      INT64 _taskID ;
      INT32 _taskType ;

   } ;

   class omDeployPackageTask : public omTaskBase
   {
   public:
      omDeployPackageTask( INT64 taskID ) ;
      virtual ~omDeployPackageTask() ;
   
   public:
      virtual INT32 finish( BSONObj &resultInfo ) ;
   
      virtual INT32 getType() ;
   
      virtual INT64 getTaskID() ;

   private:
      INT32 _addPackage( const BSONObj &taskInfo, const BSONObj &resultInfo ) ;

   private:
      INT64 _taskID ;
      INT32 _taskType ;

   } ;

   class omRemoveBusinessTask : public omTaskBase
   {
      public:
         omRemoveBusinessTask( INT64 taskID ) ;
         virtual ~omRemoveBusinessTask() ;

      public:
         virtual INT32     finish( BSONObj &resultInfo ) ;

         virtual INT32     getType() ;

         virtual INT64     getTaskID() ;

         virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;

      private:
         INT32             _removeBusinessInfo( BSONObj &taskInfoValue ) ;

         INT32             _removeConfigInfo( BSONObj &taskInfoValue ) ;

         INT32             _removeAuthInfo( BSONObj &taskInfoValue ) ;

      private:
         INT64             _taskID ;
         INT32             _taskType ;
   } ;

   class omSsqlExecTask : public omTaskBase
   {
      public:
         omSsqlExecTask( INT64 taskID ) ;
         virtual ~omSsqlExecTask() ;

      public:
         virtual INT32     finish( BSONObj &resultInfo ) ;

         virtual INT32     getType() ;

         virtual INT64     getTaskID() ;

         virtual INT32     checkUpdateInfo( const BSONObj &updateInfo ) ;

      private:
         INT64             _taskID ;
         INT32             _taskType ;
   } ;

   class omTaskManager : public SDBObject
   {
      public:
         omTaskManager() ;
         ~omTaskManager() ;

      public:
         INT32             updateTask( INT64 taskID, 
                                       const BSONObj &taskUpdateInfo ) ;

         INT32             queryOneTask( const BSONObj &selector, 
                                         const BSONObj &matcher,
                                         const BSONObj &orderBy,
                                         const BSONObj &hint , 
                                         BSONObj &oneTask ) ;

         INT32             queryTasks( const BSONObj &selector, 
                                       const BSONObj &matcher, 
                                       const BSONObj &orderBy,
                                       const BSONObj &hint, 
                                       vector< BSONObj >&tasks ) ;

      private:
         INT32             _updateTask( INT64 taskID, 
                                        const BSONObj &taskUpdateInfo ) ;

         INT32             _getTaskType( INT64 taskID, INT32 &taskType ) ;

         

         INT32             _getTaskFlag( INT64 taskID, BOOLEAN &existFlag, 
                                         BOOLEAN &isFinished, 
                                         INT32 &taskType ) ;
   } ;

}

#endif /* OM_TASKMANAGER_HPP_ */



