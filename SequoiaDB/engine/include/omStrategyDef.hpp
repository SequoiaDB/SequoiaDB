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

   Source File Name = omStrategyDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_STRATEGY_DEF_HPP_
#define OM_STRATEGY_DEF_HPP_


#include "ossTypes.h"
#include "schedDef.hpp"
#include "../bson/bson.h"
#include <boost/shared_ptr.hpp>

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      Common Define
   */
   #define OM_TASK_STRATEGY_NICE_MAX                  SCHED_NICE_MAX
   #define OM_TASK_STRATEGY_NICE_MIN                  SCHED_NICE_MIN
   #define OM_TASK_STRATEGY_NICE_DFT                  SCHED_NICE_DFT
   #define OM_TASK_STRATEGY_INVALID_VER               ( -1 )
   #define OM_TASK_STRATEGY_INVALID_RULE_ID           SCHED_INVALID_RULEID
   #define OM_TASK_STRATEGY_INVALID_SORTID            ( -1 )
   #define OM_TASK_STRATEGY_INVALID_TASK_ID           SCHED_TASK_ID_DFT
   #define OM_TASK_STRATEGY_TASK_NAME_DFT             SCHED_TASK_NAME_DFT
   #define OM_TASK_STRATEGY_TASK_ID_DFT               SCHED_TASK_ID_DFT

   typedef set<string>                 SET_IP ;

   #define OM_STRATEGY_BS_TASK_META_NAME              "TaskStrategyMeta"

   /*
      OM_STRATEGY_STATUS define
   */
   enum OM_STRATEGY_STATUS
   {
      OM_STRATEGY_STATUS_DISABLE       = 0,
      OM_STRATEGY_STATUS_ENABLE        = 1
   } ;

   /*
      _omStrategyMetaInfo define
   */
   class _omStrategyMetaInfo : public SDBObject
   {
      public:
         _omStrategyMetaInfo() ;
         ~_omStrategyMetaInfo() ;

         INT32          getVersion() const { return _version ; }
         const string&  getClusterName() const { return _clsName ; }
         const string&  getBusinessName() const { return _bizName ; }

         void           setVersion( INT32 version ) ;
         void           setClusterName( const string& name ) ;
         void           setBusinessName( const string& name ) ;

         BSONObj        toBSON() const ;
         INT32          fromBSON( const BSONObj &obj ) ;

      private:
         INT32             _version ;
         string            _clsName ;     /// cluster name
         string            _bizName ;     /// business name
   } ;
   typedef _omStrategyMetaInfo omStrategyMetaInfo ;

   #define OM_STRATEGY_MASK_CLSINFO             0x00000001
   #define OM_STRATEGY_MASK_BASEINFO            0x00000002
   #define OM_STRATEGY_MASK_USER                0x00000004
   #define OM_STRATEGY_MASK_IPS                 0x00000008

   #define OM_STRATEGY_MASK_ALL                 0xFFFFFFFF

   /*
      _omTaskStrategyInfo Define
   */
   class _omTaskStrategyInfo : public SDBObject
   {
      friend class _omStrategyMgr ;
      public:
         _omTaskStrategyInfo() ;
         ~_omTaskStrategyInfo() ;

         BOOLEAN  isValid() const ;

         BSONObj  toBSON( UINT32 mask = OM_STRATEGY_MASK_ALL ) const ;
         BSONObj  toMatcher() const ;
         INT32    fromBSON( const BSONObj &obj ) ;

         BOOLEAN  isMatch( const string &userName, const string &ip ) const ;

         INT64    getTaskID() const { return _taskID ; }
         INT64    getRuleID() const { return _ruleID ; }
         INT64    getSortID() const { return _sortID ; }
         INT32    getNice() const { return _nice ; }
         INT32    getStatus() const { return _status ; }
         BOOLEAN  isEnabled() const ;
         const string& getTaskName() const { return _taskName ; }
         const string& getUserName() const { return _userName ; }
         const string& getClusterName() const { return _clsName ; }
         const string& getBusinessName() const { return _bizName ; }
         const string& getContainerName() const { return _containerName ; }
         const SET_IP* getIPSet() const { return &_ips ; }
         UINT32   getIPCount() const { return _ips.size() ; }
         BOOLEAN  isIPInSet( const string& ip ) const ;

         void     setRuleID( INT64 ruleID ) ;
         void     setSortID( INT64 sortID ) ;
         void     setNice( INT32 nice ) ;
         void     setStatus( INT32 status ) ;
         void     enable() ;
         void     disable() ;
         void     setTaskName( const string& name ) ;
         void     setUserName( const string& userName ) ;
         void     setClusterName( const string& name ) ;
         void     setBusinessName( const string& name ) ;
         void     setContainerName( const string& name ) ;
         void     clearIPSet() ;
         BOOLEAN  addIP( const string& ip ) ;
         void     delIP( const string& ip ) ;
         void     setIPSet( const SET_IP& ipSet ) ;
         void     setTaskID( INT64 newTaskID ) ;

      private:
         INT64                      _ruleID ;
         INT64                      _taskID ;
         INT64                      _sortID ;
         INT32                      _nice ;
         INT32                      _status ;
         string                     _clsName ;     /// cluster name
         string                     _bizName ;     /// business name
         string                     _taskName ;
         string                     _userName ;
         string                     _containerName ;
         SET_IP                     _ips ;
   } ;
   typedef _omTaskStrategyInfo omTaskStrategyInfo ;

   typedef boost::shared_ptr< omTaskStrategyInfo >    omTaskStrategyInfoPtr ;

   /*
      _omTaskInfo define
   */
   class _omTaskInfo : public SDBObject
   {
      public:
         _omTaskInfo() ;
         ~_omTaskInfo() ;

         BOOLEAN        isValid() const ;

         INT64          getTaskID() const { return _taskID ; }
         INT32          getStatus() const { return _status ; }
         INT64          getCreateTime() const { return _createTime ; }

         const string&  getClusterName() const { return _clsName ; }
         const string&  getBusinessName() const { return _bizName ; }
         const string&  getTaskName() const { return _taskName ; }
         const string&  getCreator() const { return _createUser ; }

         void           setTaskID( INT64 taskID ) ;
         void           enable() ;
         void           disable() ;
         void           setStatus( INT32 status ) ;
         void           setCreateTime( INT64 time ) ;
         void           makeCreateTime() ;
         void           setClusterName( const string& name ) ;
         void           setBusinessName( const string& name ) ;
         void           setTaskName( const string& name ) ;
         void           setCreator( const string &name ) ;

         BSONObj        toBSON() const ;
         INT32          fromBSON( const BSONObj &obj ) ;
         BSONObj        toMatcher() const ;

      private:
         INT64             _taskID ;
         INT32             _status ;
         INT64             _createTime ;
         string            _clsName ;
         string            _bizName ;
         string            _taskName ;
         string            _createUser ;

   } ;
   typedef _omTaskInfo omTaskInfo ;

   typedef boost::shared_ptr< omTaskInfo >               omTaskInfoPtr ;

}

#endif // OM_STRATEGY_DEF_HPP_
