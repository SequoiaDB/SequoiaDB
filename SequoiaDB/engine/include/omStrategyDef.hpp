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

   Source File Name = omStrategyDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OM_STRATEGY_DEF_HPP_
#define OM_STRATEGY_DEF_HPP_


#include "ossTypes.h"
#include "../bson/bson.h"
#include <boost/shared_ptr.hpp>

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      Common Define
   */
   #define OM_TASK_STRATEGY_NICE_MAX                  ( 19 )
   #define OM_TASK_STRATEGY_NICE_MIN                  ( -20 )
   #define OM_TASK_STRATEGY_NICE_DEF                  ( 0 )
   #define OM_TASK_STRATEGY_INVALID_VER               ( -1 )

   typedef set<string>                 SET_IP ;

   /*
      _omTaskStrategyInfo Define
   */
   class _omTaskStrategyInfo
   {
      friend class _omStrategyMgr ;
      public:
         _omTaskStrategyInfo() ;

         BSONObj  toBSON() const ;
         INT32    fromBSON( const BSONObj &obj ) ;

         BOOLEAN  isMatch( const string &userName, const string &ip ) const ;

         INT64    getTaskID() const { return _taskID ; }
         INT64    getID() const { return _id ; }
         INT32    getNice() const { return _nice ; }
         const string& getTaskName() const { return _taskName ; }
         const string& getUserName() const { return _userName ; }
         const SET_IP* getIPSet() const { return &_ips ; }
         UINT32   getIPCount() const { return _ips.size() ; }
         BOOLEAN  isIPInSet( const string& ip ) const ;

         void     setID( INT64 id ) ;
         void     setNice( INT32 nice ) ;
         void     setTaskName( const string& name ) ;
         void     setUserName( const string& userName ) ;
         void     clearIPSet() ;
         BOOLEAN  addIP( const string& ip ) ;
         void     delIP( const string& ip ) ;
         void     setIPSet( const SET_IP& ipSet ) ;

      protected:
         void     setTaskID( INT64 newTaskID ) ;

      private:
         INT64                      _taskID ;
         INT64                      _id ;
         INT32                      _nice ;
         string                     _taskName ;
         string                     _userName ;
         SET_IP                     _ips ;
   } ;
   typedef _omTaskStrategyInfo omTaskStrategyInfo ;

   typedef boost::shared_ptr< omTaskStrategyInfo >    taskStrategyInfoPtr ;

}

#endif // OM_STRATEGY_DEF_HPP_
