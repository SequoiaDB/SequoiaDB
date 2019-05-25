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

   Source File Name = qgmPlan.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlan.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   _qgmPlan::_qgmPlan( QGM_PLAN_TYPE type, const qgmField &alias )
   :_type( type ),
   _eduCB( NULL ),
   _executed( FALSE ),
   _initialized( FALSE ),
   _merge( FALSE ),
   _param( NULL )
   {
      SDB_ASSERT( QGM_PLAN_TYPE_MAX != _type, "impossible" ) ;
      _alias = alias ;
   }

   _qgmPlan::~_qgmPlan()
   {
      QGM_PINPUT::iterator itr = _input.begin() ;
      for ( ; itr != _input.end(); itr++ )
      {
         SAFE_OSS_DELETE( *itr ) ;
      }
      _input.clear() ;
   }

   void _qgmPlan::close()
   {
      QGM_PINPUT::iterator itr = _input.begin() ;
      for ( ; itr != _input.end(); itr++ )
      {
         (*itr)->close() ;
      }
   }

   INT32 _qgmPlan::addChild( _qgmPlan *child )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != child, "impossible" ) ;
      if ( NULL == child )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         _input.push_back( child ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmPlan::needRollback() const
   {
      BOOLEAN isNeedRollback = FALSE ;
      for ( UINT32 i = 0 ; i < _input.size() ; ++i )
      {
         const _qgmPlan *pPlan = _input[i] ;
         if ( pPlan->needRollback() )
         {
            isNeedRollback = TRUE ;
            break ;
         }
      }
      return isNeedRollback ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLAN_EXECUTE, "_qgmPlan::execute" )
   INT32 _qgmPlan::execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLAN_EXECUTE ) ;
      INT32 rc = SDB_OK ;

      if ( !_initialized )
      {
         PD_LOG( PDERROR, "node is not initialized, type:%s",
                 toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL == eduCB )
      {
         SDB_ASSERT( NULL != eduCB, "can not be NULL" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _eduCB = eduCB ;
      _executed = TRUE ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLAN_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLAN_FETCHNEXT, "_qgmPlan::fetchNext" )
   INT32 _qgmPlan::fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLAN_FETCHNEXT ) ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _eduCB, "impossible" ) ;
      SDB_ASSERT( _executed, "impossible" ) ;

      if ( !_executed || NULL == _eduCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _fetchNext( next ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDDEBUG, "failed to fecth next:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLAN_FETCHNEXT, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   }

   

}
