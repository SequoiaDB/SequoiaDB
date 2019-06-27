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

   Source File Name = omStrategyDef.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "omStrategyDef.hpp"
#include "omDef.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omStrategyMetaInfo implement
   */
   _omStrategyMetaInfo::_omStrategyMetaInfo()
   {
      _version = OM_TASK_STRATEGY_INVALID_VER ;
   }

   _omStrategyMetaInfo::~_omStrategyMetaInfo()
   {
   }

   void _omStrategyMetaInfo::setVersion( INT32 version )
   {
      _version = version ;
   }

   void _omStrategyMetaInfo::setClusterName( const string &name )
   {
      _clsName = name ;
   }

   void _omStrategyMetaInfo::setBusinessName( const string &name )
   {
      _bizName = name ;
   }

   BSONObj _omStrategyMetaInfo::toBSON() const
   {
      return BSON( FIELD_NAME_VERSION << getVersion() <<
                   OM_BSON_CLUSTER_NAME << getClusterName() <<
                   OM_BSON_BUSINESS_NAME << getBusinessName() <<
                   FIELD_NAME_NAME << OM_STRATEGY_BS_TASK_META_NAME ) ;
   }

   INT32 _omStrategyMetaInfo::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;

      beField = obj.getField( FIELD_NAME_VERSION ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setVersion( beField.numberInt() ) ;

      beField = obj.getField( OM_BSON_CLUSTER_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setClusterName( beField.str() ) ;

      beField = obj.getField( OM_REST_BUSINESS_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setBusinessName( beField.str() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omTaskStrategyInfo implement
   */
   _omTaskStrategyInfo::_omTaskStrategyInfo()
   {
      _ruleID     = OM_TASK_STRATEGY_INVALID_RULE_ID ;
      _nice       = OM_TASK_STRATEGY_NICE_DFT ;
      _taskID     = OM_TASK_STRATEGY_TASK_ID_DFT ;
      _sortID     = OM_TASK_STRATEGY_INVALID_SORTID ;
      _status     = OM_STRATEGY_STATUS_ENABLE ;
   }

   _omTaskStrategyInfo::~_omTaskStrategyInfo()
   {
   }

   BOOLEAN _omTaskStrategyInfo::isValid() const
   {
      if ( _clsName.empty() || _bizName.empty() || _taskName.empty() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _omTaskStrategyInfo::isEnabled() const
   {
      return OM_STRATEGY_STATUS_ENABLE == _status ? TRUE : FALSE ;
   }

   void _omTaskStrategyInfo::setTaskID( INT64 newTaskID )
   {
      _taskID = newTaskID ;
   }

   BOOLEAN _omTaskStrategyInfo::isIPInSet( const string &ip ) const
   {
      if ( _ips.find( ip ) != _ips.end() )
      {
         return TRUE ;
      }
      return TRUE ;
   }

   void _omTaskStrategyInfo::setRuleID( INT64 ruleID )
   {
      _ruleID = ruleID ;
   }

   void _omTaskStrategyInfo::setSortID( INT64 sortID )
   {
      _sortID = sortID ;
   }

   void _omTaskStrategyInfo::setStatus( INT32 status )
   {
      if ( OM_STRATEGY_STATUS_DISABLE == status )
      {
         _status = OM_STRATEGY_STATUS_DISABLE ;
      }
      else
      {
         _status = OM_STRATEGY_STATUS_ENABLE ;
      }
   }

   void _omTaskStrategyInfo::enable()
   {
      setStatus( OM_STRATEGY_STATUS_ENABLE ) ;
   }

   void _omTaskStrategyInfo::disable()
   {
      setStatus( OM_STRATEGY_STATUS_DISABLE ) ;
   }

   void _omTaskStrategyInfo::setNice( INT32 nice )
   {
      if ( nice > OM_TASK_STRATEGY_NICE_MAX )
      {
         _nice = OM_TASK_STRATEGY_NICE_MAX ;
      }
      else if ( nice < OM_TASK_STRATEGY_NICE_MIN )
      {
         _nice = OM_TASK_STRATEGY_NICE_MIN ;
      }
      else
      {
         _nice = nice ;
      }
   }

   void _omTaskStrategyInfo::setTaskName( const string &name )
   {
      _taskName = name ;
   }

   void _omTaskStrategyInfo::setUserName( const string &userName )
   {
      _userName = userName ;
   }

   void _omTaskStrategyInfo::setClusterName( const string &name )
   {
      _clsName = name ;
   }

   void _omTaskStrategyInfo::setBusinessName( const string &name )
   {
      _bizName = name ;
   }

   void _omTaskStrategyInfo::setContainerName( const string &name )
   {
      _containerName = name ;
   }

   void _omTaskStrategyInfo::clearIPSet()
   {
      _ips.clear() ;
   }

   BOOLEAN _omTaskStrategyInfo::addIP( const string &ip )
   {
      return _ips.insert( ip ).second ;
   }

   void _omTaskStrategyInfo::delIP( const string &ip )
   {
      _ips.erase( ip ) ;
   }

   void _omTaskStrategyInfo::setIPSet( const SET_IP &ipSet )
   {
      _ips = ipSet ;
   }

   BSONObj _omTaskStrategyInfo::toMatcher() const
   {
      BSONObjBuilder builder( 128 ) ;

      builder.append( OM_BSON_CLUSTER_NAME, getClusterName() ) ;
      builder.append( OM_BSON_BUSINESS_NAME, getBusinessName() ) ;
      builder.append( OM_REST_FIELD_TASK_ID, getTaskID() ) ;

      return builder.obj() ;
   }

   BSONObj _omTaskStrategyInfo::toBSON() const
   {
      BSONObjBuilder builder( 1024 ) ;

      builder.append( OM_BSON_CLUSTER_NAME, getClusterName() ) ;
      builder.append( OM_BSON_BUSINESS_NAME, getBusinessName() ) ;
      builder.append( OM_REST_FIELD_TASK_NAME, getTaskName() ) ;
      builder.append( OM_REST_FIELD_RULE_ID, getRuleID() ) ;
      builder.append( OM_REST_FIELD_TASK_ID, getTaskID() ) ;
      builder.append( OM_REST_FIELD_SORT_ID, getSortID() ) ;
      builder.append( OM_REST_FIELD_NICE, getNice() ) ;
      builder.append( OM_REST_FIELD_STATUS, getStatus() ) ;
      builder.append( OM_REST_FIELD_USER_NAME, getUserName() ) ;

      BSONArrayBuilder arr( builder.subarrayStart( OM_REST_FIELD_IPS ) ) ;

      SET_IP::const_iterator cit = _ips.begin() ;
      while( cit != _ips.end() )
      {
         if ( !cit->empty() )
         {
            arr.append( *cit ) ;
         }
         ++cit ;
      }

      arr.done() ;

      return builder.obj() ;
   }

   INT32 _omTaskStrategyInfo::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;
      BSONObj ipsObj ;

      beField = obj.getField( OM_REST_FIELD_RULE_ID ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setRuleID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_SORT_ID ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setSortID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_NICE ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setNice( beField.numberInt() ) ;

      beField = obj.getField( OM_REST_FIELD_STATUS ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setStatus( beField.numberInt() ) ;

      beField = obj.getField( OM_REST_FIELD_USER_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setUserName( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_TASK_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskName( beField.str() ) ;

      beField = obj.getField( OM_BSON_CLUSTER_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setClusterName( beField.str() ) ;

      beField = obj.getField( OM_BSON_BUSINESS_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setBusinessName( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_IPS ) ;
      if ( Array != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string array",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         clearIPSet() ;

         string tmpStr ;
         BSONElement e ;
         BSONObjIterator itr( beField.embeddedObject() ) ;
         while( itr.more() )
         {
            e = itr.next() ;
            if ( String != beField.type() )
            {
               PD_LOG( PDERROR, "Field[%s] must be string array",
                       beField.toString( TRUE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            tmpStr = e.str() ;

            if ( !tmpStr.empty() )
            {
               addIP( tmpStr ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omTaskInfo implement
   */
   _omTaskInfo::_omTaskInfo()
   {
      _taskID        = OM_TASK_STRATEGY_TASK_ID_DFT ;
      _status        = OM_STRATEGY_STATUS_ENABLE ;
   }

   _omTaskInfo::~_omTaskInfo()
   {
   }

   BOOLEAN _omTaskInfo::isValid() const
   {
      if ( _clsName.empty() || _bizName.empty() ||
           _taskName.empty() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   void _omTaskInfo::setTaskID( INT64 taskID )
   {
      _taskID = taskID ;
   }

   void _omTaskInfo::enable()
   {
      _status = OM_STRATEGY_STATUS_ENABLE ;
   }

   void _omTaskInfo::disable()
   {
      _status = OM_STRATEGY_STATUS_DISABLE ;
   }

   void _omTaskInfo::setStatus( INT32 status )
   {
      if ( OM_STRATEGY_STATUS_DISABLE == status )
      {
         _status = OM_STRATEGY_STATUS_DISABLE ;
      }
      else
      {
         _status = OM_STRATEGY_STATUS_ENABLE ;
      }
   }

   void _omTaskInfo::setCreateTime( INT64 time )
   {
      _createTime = time ;
   }

   void _omTaskInfo::makeCreateTime()
   {
      setCreateTime( time( NULL ) ) ;
   }

   void _omTaskInfo::setClusterName( const string &name )
   {
      _clsName = name ;
   }

   void _omTaskInfo::setBusinessName( const string &name )
   {
      _bizName = name ;
   }

   void _omTaskInfo::setTaskName( const string &name )
   {
      _taskName = name ;
   }

   void _omTaskInfo::setCreator( const string &name )
   {
      _createUser = name ;
   }

   BSONObj _omTaskInfo::toBSON() const
   {
      BSONObjBuilder objBuilder( 256 ) ;
      objBuilder.append( OM_REST_FIELD_TASK_ID, getTaskID() ) ;
      objBuilder.append( OM_REST_FIELD_STATUS, getStatus() ) ;
      objBuilder.append( OM_BSON_CLUSTER_NAME, getClusterName() ) ;
      objBuilder.append( OM_BSON_BUSINESS_NAME, getBusinessName() ) ;
      objBuilder.append( OM_REST_FIELD_TASK_NAME, getTaskName() ) ;
      objBuilder.append( OM_REST_FIELD_CREATE_USER, getCreator() ) ;
      objBuilder.append( OM_REST_FIELD_CREATE_TIME, getCreateTime() ) ;

      return objBuilder.obj() ;
   }

   BSONObj _omTaskInfo::toMatcher() const
   {
      BSONObjBuilder objBuilder( 128 ) ;
      objBuilder.append( OM_BSON_CLUSTER_NAME, getClusterName() ) ;
      objBuilder.append( OM_BSON_BUSINESS_NAME, getBusinessName() ) ;
      objBuilder.append( OM_REST_FIELD_TASK_NAME, getTaskName() ) ;

      return objBuilder.obj() ;
   }

   INT32 _omTaskInfo::fromBSON( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;

      beField = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_STATUS ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setStatus( beField.numberInt() ) ;

      beField = obj.getField( OM_BSON_CLUSTER_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setClusterName( beField.str() ) ;

      beField = obj.getField( OM_BSON_BUSINESS_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setBusinessName( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_TASK_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskName( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_CREATE_USER ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setCreator( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_CREATE_TIME ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setCreateTime( beField.numberLong() ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
