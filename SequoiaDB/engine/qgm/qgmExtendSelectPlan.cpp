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

   Source File Name = qgmExtendSelectPlan.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmExtendSelectPlan.hpp"
#include "qgmOptiSort.hpp"
#include "qgmOptiSelect.hpp"
#include "qgmUtil.hpp"
#include "qgmOptiSplit.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   _qgmExtendSelectPlan::_qgmExtendSelectPlan()
   {
      _limit = -1 ;
      _skip  = 0 ;
   }

   _qgmExtendSelectPlan::~_qgmExtendSelectPlan()
   {
      _funcSelector.clear() ;
      _original.clear() ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMEXTENDSELECTPLAN__EXTEND, "_qgmExtendSelectPlan::_extend" )
   INT32 _qgmExtendSelectPlan::_extend( UINT32 id,
                                        qgmOptiTreeNode *&extended )
   {
      PD_TRACE_ENTRY( SDB__QGMEXTENDSELECTPLAN__EXTEND ) ;
      INT32 rc = SDB_OK ;
      qgmOptiSelect *select = ( qgmOptiSelect * )_local ;

      SDB_ASSERT( NULL != select, "impossible" ) ;

      if ( QGM_EXTEND_AGGR == id )
      {
         _qgmOptiAggregation *aggr = SDB_OSS_NEW
                                     qgmOptiAggregation( select->getPtrT(),
                                                         select->getParam() ) ;
         if ( NULL == aggr )
         {
            PD_LOG( PDERROR, "failed to allocate mem" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         aggr->_alias = _aliases.front() ;
         _aliases.pop() ;

         replaceFieldRele( _groupby, _aliases.front() ) ;
         aggr->_groupby = _groupby ;

         replaceAggrRele( _funcSelector, _aliases.front() ) ;
         aggr->_selector = _funcSelector ;
         extended = aggr ;
      }
      else if ( QGM_EXTEND_ORDERBY == id )
      {
         _qgmOptiSort *sort = SDB_OSS_NEW
                              qgmOptiSort( select->getPtrT(),
                                           select->getParam() ) ;
         if ( NULL == sort )
         {
            PD_LOG( PDERROR, "failed to allocate mem" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         replaceFieldRele( _orderby, _aliases.front() ) ;
         sort->append( _orderby, TRUE ) ;
         extended = sort ;
      }
      else if ( QGM_EXTEND_GROUPBY == id )
      {
         _qgmOptiSort *sort = SDB_OSS_NEW
                              qgmOptiSort( select->getPtrT(),
                                           select->getParam()) ;
         if ( NULL == sort )
         {
            PD_LOG( PDERROR, "failed to allocate mem" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         sort->append( _groupby, TRUE ) ;
         extended = sort ;
      }
      else if ( QGM_EXTEND_ORDERFILTER == id )
      {
         _qgmOptiSelect *filter = SDB_OSS_NEW
                                   qgmOptiSelect( select->getPtrT(),
                                                  select->getParam() ) ;
         if ( NULL == filter )
         {
            PD_LOG( PDERROR, "failed to allocate mem" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         {
         filter->_type = QGM_OPTI_TYPE_FILTER ;
         filter->_limit = _limit ;
         filter->_skip  = _skip ;
         clearConstraint() ;

         filter->_alias = _aliases.front() ;
         _aliases.pop() ;
         qgmOPFieldVec::const_iterator itr = _original.begin() ;
         for ( ; itr != _original.end(); itr++ )
         {
            qgmOpField selector = *itr ;
            if ( SQL_GRAMMAR::WILDCARD != selector.type )
            {
               selector.value.attr() = itr->alias.empty() ?
                                       itr->value.attr() :
                                       itr->alias ;
               selector.type = SQL_GRAMMAR::DBATTR ;
               selector.value.relegation() = _aliases.front() ;
               selector.alias.clear() ;
            }
            selector.expr.set( NULL ) ;
            filter->_selector.push_back( selector ) ;
         }
         }
         extended = filter ;
      }
      else if ( QGM_EXTEND_SPLITBY == id )
      {
         _qgmOptiSplit *split = SDB_OSS_NEW
                               _qgmOptiSplit( select->getPtrT(),
                                              select->getParam(),
                                              _splitby ) ;
         if ( NULL == split )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         extended = split ;
      }
      else
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMEXTENDSELECTPLAN__EXTEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
