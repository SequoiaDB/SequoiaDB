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
          01/18/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#include "omStrategyDef.hpp"
#include "omDef.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omTaskStrategyInfo implement
   */
   _omTaskStrategyInfo::_omTaskStrategyInfo()
   {
      _id         = 0 ;
      _nice       = 0 ;
      _taskID     = 0 ;
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

   void _omTaskStrategyInfo::setID( INT64 id )
   {
      _id = id ;
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

   BSONObj _omTaskStrategyInfo::toBSON() const
   {
      BSONObjBuilder builder( 1024 ) ;

      builder.append( OM_REST_FIELD_RULE_ID, getID() ) ;
      builder.append( OM_REST_FIELD_TASK_ID, getTaskID() ) ;
      builder.append( OM_REST_FIELD_TASK_NAME, getTaskName() ) ;
      builder.append( OM_REST_FIELD_NICE, getNice() ) ;
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
      setID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_TASK_ID ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskID( beField.numberLong() ) ;

      beField = obj.getField( OM_REST_FIELD_TASK_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setTaskName( beField.str() ) ;

      beField = obj.getField( OM_REST_FIELD_NICE ) ;
      if ( !beField.isNumber() )
      {
         PD_LOG( PDERROR, "Field[%s] must be number",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setNice( beField.numberInt() ) ;

      beField = obj.getField( OM_REST_FIELD_USER_NAME ) ;
      if ( String != beField.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be string",
                 beField.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      setUserName( beField.str() ) ;

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

}
