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

   Source File Name = qgmPlAggregation.cpp

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

#include "qgmPlAggregation.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "qgmUtil.hpp"
#include "rtnSQLFuncFactory.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{

   _qgmPlAggregation::_qgmPlAggregation( const qgmAggrSelectorVec &selector,
                                         const qgmOPFieldVec &groupby,
                                         const qgmField &alias,
                                         _qgmPtrTable *table )
   :_qgmPlan( QGM_PLAN_TYPE_AGGR, alias ),
    _eoc( FALSE ), _pushedAtThisTime( FALSE ),
    _pushedAtAnyTime( FALSE ), _isAggr( FALSE ), _isStat( FALSE )
   {
      INT32 rc = SDB_OK ;
      SQL_CB *sqlCB = pmdGetKRCB()->getSqlCB() ;
      qgmAggrSelectorVec::const_iterator itr = selector.begin() ;
      for ( ; itr != selector.end(); itr++ )
      {
         if ( SQL_GRAMMAR::WILDCARD == itr->value.type )
         {
            break ;
         }

         if ( SQL_GRAMMAR::FUNC == itr->value.type )
         {
            _rtnSQLFunc *func = NULL ;
            rc = sqlCB->getFunc(itr->value.value.toString().c_str(),
                                itr->param.size(),
                                func ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDDEBUG, "invalid func: [name:%s] [paramNum:%d]",
                       itr->value.value.toString().c_str(),
                       itr->param.size() ) ;
               goto done ;
            }
            else
            {
               SDB_ASSERT( NULL != func, "impossible" ) ;
               rc = func->init( itr->value.alias, itr->param ) ;
               if ( SDB_OK != rc )
               {
                  goto done ;
               }
               _func.push_back( func ) ;
               if ( !_isAggr && func->isAggr() )
               {
                  _isAggr = TRUE ;
               }
               if ( !_isStat && func->isStat() )
               {
                  _isStat = TRUE ;
               }
            }
         }
         else
         {
            _rtnSQLFunc *func = NULL ;
            _qgmOpField first ;

            rc = sqlCB->getFunc( RTN_SQL_FUNC_FIRST, 1, func ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "can not find func[%s], rc: %d",
                       RTN_SQL_FUNC_FIRST, rc ) ;
               goto done ;
            }

            {
               SDB_ASSERT( NULL != func, "impossible") ;
               vector<qgmOpField> param ;
               param.push_back( itr->value ) ;
               rc = func->init( itr->value.alias.empty()?
                                itr->value.value.attr() :
                                itr->value.alias,
                                param ) ;
               if ( SDB_OK != rc )
               {
                  goto done ;
               }
               _func.push_back( func ) ;
            }
         }
      }

      rc = _groupby.load( groupby ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Load groupby failed, rc: %d", rc ) ;
         goto done ;
      }
      if ( !_isAggr && !_groupby.empty() )
      {
         _isAggr = TRUE ;
      }
      _initialized = TRUE ;

   done:
      return ;
   }

   _qgmPlAggregation::~_qgmPlAggregation()
   {
      vector<_rtnSQLFunc *>::iterator itr = _func.begin() ;
      for ( ; itr != _func.end(); itr++ )
      {
         SAFE_OSS_DELETE( *itr ) ;
      }
      _func.clear() ;
   }

   string _qgmPlAggregation::toString() const
   {
      stringstream ss ;
      ss << "Type:" << qgmPlanType(_type) << '\n' ;
      ss << "Alias:" << _alias.toString() << '\n' ;
      if ( !_func.empty() )
      {
         ss << "Func:[" ;
         for ( vector<_rtnSQLFunc *>::const_iterator itr = _func.begin() ;
               itr != _func.end();
               itr++ )
         {
            ss << (*itr)->toString() << "," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]\n" ;
      }

      if ( !_groupby.empty() )
      {
         ss << "Groupby:" << _groupby.selector().toString() << '\n';
      }

      return ss.str() ;
   }

   INT32 _qgmPlAggregation::_execute( _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;

      _pushedAtThisTime = FALSE ;
      _pushedAtAnyTime = FALSE ; 
      _eoc = FALSE ;
      _groupbyKey = BSONObj() ;

      SDB_ASSERT( 1 == _input.size(), "impossible" ) ;
      rc = input( 0 )->execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmPlAggregation::_fetchNext( qgmFetchOut &next )
   {
      INT32 rc = SDB_OK ;
      qgmFetchOut subFetch ;
      BSONObj currentGroupBy ;

      if ( _eoc )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      do
      {
         rc = input( 0 )->fetchNext( subFetch ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _eoc = TRUE ;

            if ( !_pushedAtThisTime && ( _pushedAtAnyTime || !_isStat ) )
            {
               goto error ;
            }

            rc = _result( next ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            else
            {
               break ;
            }
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fatch next:%d", rc ) ;
            goto error ;
         }
         else
         {
            if ( !_groupby.empty() )
            {
               rc = _groupby.select( subFetch, currentGroupBy ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }

               if ( _groupbyKey.isEmpty())
               {
                  _groupbyKey = currentGroupBy ;
                  rc = _push( subFetch ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  else
                  {
                     continue ;
                  }
               }
               else if ( 0 != _groupbyKey.woCompare( currentGroupBy ) )
               {
                  _groupbyKey = currentGroupBy ;
                  rc = _result( next ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  else
                  {
                  }

                  rc = _push( subFetch ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  else
                  {
                     break ;
                  }
               }
               else
               {
                  rc = _push( subFetch ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  else
                  {
                     continue ;
                  }
               }
            }
            else
            {
               rc = _push( subFetch ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
               else
               {
                  if ( _isAggr )
                  {
                     continue ;
                  }
                  rc = _result( next ) ;
                  if ( SDB_OK != rc )
                  {
                     goto error ;
                  }
                  else
                  {
                     break ;
                  }
               }
            }
         }
      } while ( TRUE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmPlAggregation::_select( const qgmFetchOut &next,
                                     const vector<qgmOpField> &fields,
                                     RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK ;

      vector<qgmOpField>::const_iterator itr = fields.begin() ;
      for ( ; itr != fields.end(); itr++ )
      {
         if ( !itr->empty() )
         {
            BSONElement ele ;
            next.element( itr->value, ele ) ;

            if ( ele.eoo() )
            {
               PD_LOG( PDDEBUG, "element[%s] not found in fetch",
                       itr->toString().c_str() ) ;
            }
            param.push_back( ele ) ;
         }
      }
      return rc ;
   }

   INT32 _qgmPlAggregation::_push( const qgmFetchOut &next )
   {
      INT32 rc = SDB_OK ;
      RTN_FUNC_PARAMS param ;
      if ( !_func.empty() )
      {
         vector<_rtnSQLFunc *>::iterator itr = _func.begin() ;
         for ( ; itr != _func.end(); itr++ )
         {
            const vector<qgmOpField> &fields = (*itr)->param() ;
            rc = _select( next, fields, param ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            else
            {
               rc = (*itr)->push( param ) ;
               param.clear() ;
               if ( SDB_OK != rc )
              {
                  goto error ;
              }
            }
         }
     }
     else if ( !_pushedAtThisTime )
     {
        _preObj = next.mergedObj() ;
     }

     _pushedAtThisTime = TRUE ;
     _pushedAtAnyTime = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmPlAggregation::_result( qgmFetchOut &result )
   {
      INT32 rc = SDB_OK ;
      if ( !_func.empty() )
      {
         BSONObjBuilder builder ;
         vector<_rtnSQLFunc *>::iterator itr = _func.begin() ;
         for ( ; itr != _func.end(); itr++ )
         {
            rc = (*itr)->result( builder ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         result.obj = builder.obj() ;
      }
      else
      {
         result.obj = _preObj ;
      }

      if ( !_merge && !_alias.empty() )
      {
         result.alias = _alias ;
      }

      _pushedAtThisTime = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
