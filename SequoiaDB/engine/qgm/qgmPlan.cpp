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
   _param( NULL ),
   _authorized( FALSE )
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

   INT32 _qgmPlan::checkTransAutoCommit( BOOLEAN dpsValid, _pmdEDUCB *eduCB )
   {
      return _checkTransAutoCommit( dpsValid, eduCB ) ;
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

   BOOLEAN _qgmPlan::canUseTrans() const
   {
      BOOLEAN canUseTransaction = FALSE ;
      for ( UINT32 i = 0 ; i < _input.size() ; ++i )
      {
         const _qgmPlan *pPlan = _input[i] ;
         if ( pPlan->canUseTrans() )
         {
            canUseTransaction = TRUE ;
            break ;
         }
      }
      return canUseTransaction ;
   }

   void _qgmPlan::buildRetInfo( BSONObjBuilder &builder ) const
   {
      for ( UINT32 i = 0 ; i < _input.size() ; ++i )
      {
         const _qgmPlan *pPlan = _input[i] ;
         pPlan->buildRetInfo( builder ) ;
      }
   }

   void _qgmPlan::setClientVersion( INT32 version )
   {
      for ( UINT32 i = 0 ; i < _input.size() ; ++i )
      {
         _qgmPlan *pPlan = _input[i] ;
         // only support one colleciton
         pPlan->setClientVersion( version ) ;
      }
   }

   INT32 _qgmPlan::getCatalogVersion() const
   {
      INT32 curPlanClVersion = CATALOG_INVALID_VERSION ;
      INT32 latestClVersion = CATALOG_INVALID_VERSION ;

      for ( UINT32 i = 0 ; i < _input.size() ; ++i )
      {
         const _qgmPlan *pPlan = _input[i] ;
         curPlanClVersion = pPlan->getCatalogVersion() ;
         if( curPlanClVersion > latestClVersion )
         {
           latestClVersion = curPlanClVersion ;
         }
      }
      // only support one colleciton
      // every plan may change the version,so return the latest
      return latestClVersion ;
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

   INT32 _qgmPlan::_checkTransOperator( BOOLEAN dpsValid, BOOLEAN autoCommit )
   {
      INT32 rc = SDB_OK ;

      if ( pmdGetDBRole() == SDB_ROLE_DATA ||
           pmdGetDBRole() == SDB_ROLE_CATALOG )
      {
         rc = SDB_RTN_CMD_NO_SERVICE_AUTH ;
         if ( !autoCommit )
         {
            PD_LOG_MSG( PDERROR, "In sharding mode, couldn't execute "
                        "transaction operation from local service" ) ;
         }
      }
      else if ( !dpsValid )
      {
         rc = SDB_PERM ;
         if ( !autoCommit )
         {
            PD_LOG_MSG( PDERROR, "Couldn't execute transaction operation when "
                        "dps log is off" ) ;
         }
      }

      return rc ;
   }

   INT32 _qgmPlan::_checkTransAutoCommit( BOOLEAN dpsValid, _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      if ( !eduCB->isTransaction() )
      {
         if ( eduCB->getTransExecutor()->isTransAutoCommit() )
         {
            rc = _checkTransOperator( dpsValid, TRUE ) ;
            if ( SDB_OK == rc )
            {
               rc = rtnTransBegin( eduCB, TRUE ) ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
      }
      else if ( eduCB->isAutoCommitTrans() )
      {
         rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
         PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                 rc ) ;
      }

      return rc ;
   }
}
