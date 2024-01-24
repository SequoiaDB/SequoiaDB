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

   Source File Name = rtnLocalTaskFactory.cpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   command factory on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2020  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "rtnLocalTaskFactory.hpp"
#include "dms.hpp"
#include "ossErr.h"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{

   /*
      Common functions
   */
   const CHAR* rtnLocalTaskType2Str( RTN_LOCAL_TASK_TYPE type )
   {
      switch( type )
      {
         case RTN_LOCAL_TASK_RENAMECS :
            return "RENAMECS" ;
         case RTN_LOCAL_TASK_RENAMECL :
            return "RENAMECL" ;
         case RTN_LOCAL_TASK_RECYCLECS :
            return "RECYCLECS" ;
         case RTN_LOCAL_TASK_RECYCLECL :
            return "RECYCLECL" ;
         case RTN_LOCAL_TASK_RETURNCS :
            return "RETURNCS" ;
         case RTN_LOCAL_TASK_RETURNCL :
            return "RETURNCL" ;
         default:
            break ;
      }

      return "Unknown" ;
   }

   /*
      _rtnLocalTaskBase implement
   */
   INT32 _rtnLocalTaskBase::toBson( BSONObj &obj ) const
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      try
      {
         builder.append( DMS_ID_KEY_NAME, (INT64)_taskID ) ;
         builder.append( RTN_LT_FIELD_TASKTYPE, (INT32)getTaskType() ) ;
         builder.append( FTN_LT_FIELD_TASKTYPE_DESP,
                         rtnLocalTaskType2Str( getTaskType() ) ) ;

         rc = _toBson( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON, rc: %d", rc ) ;

         obj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   ossPoolString _rtnLocalTaskBase::toPrintString() const
   {
      BSONObj obj ;
      if ( SDB_OK == toBson( obj ) )
      {
         return obj.toPoolString() ;
      }
      return "ERROR" ;
   }

   /*
      _rtnLTFactory implement
   */
   _rtnLTFactory::_rtnLTFactory()
   {
   }

   _rtnLTFactory::~_rtnLTFactory()
   {
      _mapTask.clear() ;
   }

   INT32 _rtnLTFactory::create( INT32 taskType, rtnLocalTaskPtr &ptr )
   {
      INT32 rc = SDB_OK ;
      _rtnLocalTaskBase *pTask = NULL ;

      MAP_LT_IT it = _mapTask.find( taskType ) ;
      if ( it != _mapTask.end() )
      {
         RTN_NEW_LOCALTASK pFunc = it->second ;
         pTask = (*pFunc)() ;
         if ( !pTask )
         {
            rc = SDB_OOM ;
         }
         else
         {
            ptr = rtnLocalTaskPtr::make( pTask ) ;
            if ( !ptr.get() )
            {
               SDB_OSS_DEL pTask ;
               pTask = NULL ;
               rc = SDB_OOM ;
            }
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
      return rc ;
   }

   INT32 _rtnLTFactory::_register( INT32 taskType,
                                   RTN_NEW_LOCALTASK pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( taskType <= RTN_LOCAL_TASK_MIN ||
           taskType >= RTN_LOCAL_TASK_MAX )
      {
         SDB_ASSERT( FALSE, "Invalid parameters" ) ;
         rc = SDB_SYS ;
      }
      else
      {
         MAP_LT_IT it = _mapTask.find( taskType ) ;
         if ( it != _mapTask.end() )
         {
            SDB_ASSERT( FALSE, "Task already exist" ) ;
            rc = SDB_SYS ;
         }
         else
         {
            _mapTask[ taskType ] = pFunc ;
         }
      }
      return rc ;
   }

   _rtnLTFactory* rtnGetLTFactory()
   {
      static _rtnLTFactory s_factory ;
      return &s_factory ;
   }

   /*
      _rtnLTAssit implement
   */
   _rtnLTAssit::_rtnLTAssit( RTN_NEW_LOCALTASK pFunc )
   {
      if ( pFunc )
      {
         _rtnLocalTaskBase *pTask = (*pFunc)() ;
         if ( pTask )
         {
            rtnGetLTFactory()->_register ( pTask->getTaskType(), pFunc ) ;
            SDB_OSS_DEL pTask ;
            pTask = NULL ;
         }
         else
         {
            ossPanic() ;
         }
      }
   }

   _rtnLTAssit::~_rtnLTAssit()
   {
   }

}

