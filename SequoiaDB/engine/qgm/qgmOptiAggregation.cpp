/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = qgmOptiAggregation.cpp

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

#include "qgmOptiAggregation.hpp"
#include "rtnSQLFuncFactory.hpp"
#include "qgmUtil.hpp"
#include "qgmOprUnit.hpp"
#include "utilStr.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   _qgmOptiAggregation::_qgmOptiAggregation( _qgmPtrTable *table,
                                             _qgmParamTable *param )
   :_qgmOptiTreeNode(QGM_OPTI_TYPE_AGGR, table, param ),
   _hasAggrFunc(FALSE)
   {

   }

   _qgmOptiAggregation::~_qgmOptiAggregation()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION_INIT, "_qgmOptiAggregation::init" )
   INT32 _qgmOptiAggregation::init()
   {
      PD_TRACE_ENTRY(SDB__QGMOPTIAGGREGATION_INIT) ;
      INT32 rc = SDB_OK ;
      qgmOprUnit *oprUnit = NULL ;
      qgmAggrUnit *aggrUnit = NULL ;

      // create a optional select oprUnit
      if ( _selector.size() > 0 )
      {
         oprUnit = SDB_OSS_NEW qgmFilterUnit( QGM_OPTI_TYPE_FILTER ) ;
         if ( !oprUnit )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         oprUnit->setOptional( TRUE ) ;
         _addFields( oprUnit ) ;
         _oprUnits.push_back( oprUnit ) ;
         oprUnit = NULL ;
      }

      // create aggr OprUnit
      aggrUnit = SDB_OSS_NEW qgmAggrUnit( QGM_OPTI_TYPE_AGGR ) ;
      if ( !aggrUnit )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      // add group by
      aggrUnit->addOpField( _groupby, FALSE ) ;
      aggrUnit->addAggrSelector( _selector ) ;
      _oprUnits.push_back( aggrUnit ) ;
      aggrUnit = NULL ;

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION_INIT, rc ) ;
      return rc ;
   error:
      if ( oprUnit )
      {
         SDB_OSS_DEL oprUnit ;
      }
      if ( aggrUnit )
      {
         SDB_OSS_DEL aggrUnit ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION_DONE, "_qgmOptiAggregation::done" )
   INT32 _qgmOptiAggregation::done()
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION_DONE ) ;
      INT32 rc = SDB_OK ;
      // if the sub node's output is same with this output, clear the output
      qgmOpStream subOpStream ;
      BOOLEAN bSame = TRUE ;

      if ( SDB_OK == getSubNode( 0 )->outputStream( subOpStream ) &&
           NULL == subOpStream.next && !subOpStream.isWildCard() &&
           subOpStream.stream.size() == _selector.size() )
      {
         UINT32 count = _selector.size() ;
         UINT32 index = 0 ;
         while ( index < count )
         {
            qgmOpField &left = _selector[index].value ;
            qgmOpField &right = subOpStream.stream[index] ;

            if ( SQL_GRAMMAR::FUNC == left.type ||
                 !left.alias.empty() )
            {
               bSame = FALSE ;
               break ;
            }
            else if ( ( !right.alias.empty() &&
                         left.value.attr() != right.alias ) ||
                      ( right.alias.empty() &&
                         left.value.attr() != right.value.attr() ) )
            {
               bSame = FALSE ;
               break ;
            }
            ++index ;
         }

         if ( bSame )
         {
            _selector.clear() ;
         }
      }

      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION_DONE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION__UPDATE2UNIT, "_qgmOptiAggregation::_update2Unit" )
   void _qgmOptiAggregation::_update2Unit( _qgmOptiAggregation::AGGR_TYPE type )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION__UPDATE2UNIT ) ;
      qgmAggrUnit *aggrUnit = (qgmAggrUnit*)
                               getOprUnitByType( QGM_OPTI_TYPE_AGGR ) ;
      if ( aggrUnit )
      {
         if ( AGGR_GROUPBY == type )
         {
            aggrUnit->setFields( _groupby, FALSE ) ;
         }
         else
         {
            aggrUnit->clearAggrSelector() ;
            aggrUnit->addAggrSelector( _selector ) ;
         }
      }

      PD_TRACE_EXIT( SDB__QGMOPTIAGGREGATION__UPDATE2UNIT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION__ADDFIELDS, "_qgmOptiAggregation::_addFields" )
   INT32 _qgmOptiAggregation::_addFields( qgmOprUnit * oprUnit )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION__ADDFIELDS ) ;
      INT32 rc = SDB_OK ;
      if ( QGM_OPTI_TYPE_FILTER != oprUnit->getType() )
      {
         return SDB_INVALIDARG ;
      }

      qgmOPFieldVec notAggrFields ;
      qgmAggrSelectorVec::iterator it = _selector.begin() ;
      while ( it != _selector.end() )
      {
         qgmOpField &field = (*it).value ;
         if ( SQL_GRAMMAR::FUNC != field.type )
         {
            notAggrFields.push_back( field ) ;
         }
         ++it ;
      }

      oprUnit->getFields()->clear() ;
      it = _selector.begin() ;
      while ( it != _selector.end() )
      {
         qgmAggrSelector &select = *it ;
         if ( select.value.type != SQL_GRAMMAR::FUNC )
         {
            oprUnit->addOpField( select.value, QGM_OPR_FILTER_COPY_FLAG ) ;
         }
         else
         {
            qgmOPFieldVec::iterator itAttr = select.param.begin() ;
            while ( itAttr != select.param.end() )
            {
               qgmDbAttr &attr = itAttr->value ;
               if ( !isFromOne( attr, notAggrFields, FALSE ) )
               {
                  oprUnit->addOpField( qgmOpField( attr, SQL_GRAMMAR::DBATTR ),
                                       TRUE ) ;
               }
               ++itAttr ;
            }
         }
         ++it ;
      }

      if ( !oprUnit->isWildCardField() && oprUnit->getFields()->size() > 0 )
      {
         oprUnit->addOpField( _groupby, TRUE ) ;
      }

      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION__ADDFIELDS, rc ) ;
      return rc ;
   }

   BOOLEAN _qgmOptiAggregation::isInAggrFieldAlias( const qgmDbAttr & field ) const
   {
      qgmAggrSelectorVec::const_iterator cit = _selector.begin() ;
      while ( cit != _selector.end() )
      {
         const qgmAggrSelector &aggrSel = *cit ;
         if ( SQL_GRAMMAR::FUNC == aggrSel.value.type &&
              aggrSel.value.alias == field.attr() )
         {
            return TRUE ;
         }
         ++cit ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION__GETFIELDALIAS, "_qgmOptiAggregation::_getFieldAlias" )
   UINT32 _qgmOptiAggregation::_getFieldAlias( qgmOPFieldPtrVec & fieldAlias,
                                               BOOLEAN getAll )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION__GETFIELDALIAS ) ;
      UINT32 count = 0 ;
      _tmpSelector.clear() ;
      qgmAggrSelectorVec::iterator it = _selector.begin() ;
      while ( it != _selector.end() )
      {
         qgmAggrSelector &select = *it ;
         if ( select.value.type == SQL_GRAMMAR::FUNC &&
              1 == select.param.size() &&
              ( getAll || !select.value.alias.empty() ) )
         {
            const qgmField &funcField = select.value.value.attr() ;
            if ( 0 == ossStrncmp( funcField.begin(), RTN_SQL_FUNC_FIRST,
                                  funcField.size() ) ||
                 0 == ossStrncmp( funcField.begin(), RTN_SQL_FUNC_LAST,
                                  funcField.size() ) )
            {
               qgmOpField tmp( select.param[0] ) ;
               tmp.alias = select.value.alias ;
               _tmpSelector.push_back( tmp ) ;

               fieldAlias.push_back( &(*(--_tmpSelector.end())) ) ;
               ++count ;
            }
         }
         else if ( select.value.type != SQL_GRAMMAR::FUNC &&
                   select.value.type != SQL_GRAMMAR::WILDCARD &&
                   ( getAll || !select.value.alias.empty() ) )
         {
            fieldAlias.push_back( &select.value ) ;
            ++count ;
         }
         ++it ;
      }

      PD_TRACE_EXIT( SDB__QGMOPTIAGGREGATION__GETFIELDALIAS ) ;
      return count ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION__PUSHOPRUNIT, "_qgmOptiAggregation::_pushOprUnit" )
   INT32 _qgmOptiAggregation::_pushOprUnit( qgmOprUnit * oprUnit,
                                            PUSH_FROM from )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION__PUSHOPRUNIT ) ;
      INT32 rc = SDB_OK ;

      if ( QGM_OPTI_TYPE_FILTER == oprUnit->getType() )
      {
         qgmOPFieldVec *fields = oprUnit->getFields() ;

         // if fields it not empty and wildcard, need make selector
         if ( fields->size() != 0 && !oprUnit->isWildCardField() )
         {
            qgmAggrSelectorVec tmpSelector = _selector ;
            _selector.clear() ;
            UINT32 count = fields->size() ;
            UINT32 index = 0 ;
            while ( index < count )
            {
               qgmOpField &field = (*fields)[index] ;
               qgmAggrSelectorVec::iterator it = tmpSelector.begin() ;
               while ( it != tmpSelector.end() )
               {
                  qgmAggrSelector &selector = *it ;
                  if ( selector.value.alias == field.value.attr() ||
                       selector.value.value == field.value ||
                       ( field.value.relegation().empty() &&
                         field.value.attr() == selector.value.value.attr() ) )
                  {
                     _selector.push_back( selector ) ;
                     if ( !field.alias.empty() )
                     {
                        (*(--_selector.end())).value.alias = field.alias ;
                     }
                     break ;
                  }
                  ++it ;
               }

               if ( it == tmpSelector.end() )
               {
                  _selector.push_back( qgmAggrSelector( field ) ) ;
               }

               ++index ;
            }

            _update2Unit( AGGR_SELECTOR ) ;
         }

         qgmFilterUnit *filterUnit = (qgmFilterUnit*)oprUnit ;
         _addFields( filterUnit ) ;

         // delete other fiterUnit
         qgmFilterUnit *otherUnit = (qgmFilterUnit*)
                                     getOprUnitByType( QGM_OPTI_TYPE_FILTER ) ;
         if ( otherUnit )
         {
            if ( otherUnit->hasCondition() )
            {
               qgmConditionNodePtrVec conds = otherUnit->getConditions() ;
               filterUnit->addCondition( conds ) ;
            }
            removeOprUnit( otherUnit, TRUE, FALSE ) ;
         }

         filterUnit->setOptional( TRUE ) ;
         _oprUnits.push_back( filterUnit ) ;
      }
      else if ( QGM_OPTI_TYPE_SORT == oprUnit->getType() )
      {
         // add the group more fields
         if ( _groupby.size() > oprUnit->getFields()->size() )
         {
            UINT32 count = _groupby.size() ;
            UINT32 index = 0 ;
            while ( index < count )
            {
               if ( !isFromOne( _groupby[index], *(oprUnit->getFields()),
                                FALSE ) )
               {
                  oprUnit->getFields()->push_back( _groupby[index] ) ;
               }
               ++index ;
            }
         }
         _groupby = *(oprUnit->getFields()) ;
         _oprUnits.insert( _oprUnits.begin(), oprUnit ) ;

         _update2Unit( AGGR_GROUPBY ) ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION__PUSHOPRUNIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmOptiAggregation::_removeOprUnit( qgmOprUnit * oprUnit )
   {
      return SDB_OK ;
   }

   INT32 _qgmOptiAggregation::_updateChange( qgmOprUnit * oprUnit )
   {
      if ( QGM_OPTI_TYPE_AGGR != oprUnit->getType() )
      {
         goto done ;
      }
      else
      {
         qgmAggrUnit *aggrUnit = (qgmAggrUnit*)oprUnit ;
         _groupby = *(aggrUnit->getFields()) ;
         _selector = *(aggrUnit->getAggrSelector()) ;
      }

   done:
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION_OURPUTSORT, "_qgmOptiAggregation::outputSort" )
   INT32 _qgmOptiAggregation::outputSort( qgmOPFieldVec & sortFields )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION_OURPUTSORT ) ;
      INT32 rc = SDB_OK ;
      // if no group, can break down all up sort
      if ( _groupby.size() == 0 )
      {
         if ( _hasAggrFunc )
         {
            qgmOpField dummyField ;
            dummyField.type = SQL_GRAMMAR::WILDCARD ;
            sortFields.push_back( dummyField ) ;
         }
      }
      else
      {
         sortFields = _groupby ;
         if ( sortFields.size() > 0 )
         {
            _upFields( sortFields ) ;
         }
      }

      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION_OURPUTSORT, rc ) ;
      return rc ;
   }

   INT32 _qgmOptiAggregation::outputStream( qgmOpStream &stream )
   {
      INT32 rc = SDB_OK ;

      qgmAggrSelectorVec::iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); itr++ )
      {
         qgmAggrSelector &selector = *itr ;
         stream.stream.push_back( selector.value ) ;
      }

      if ( !_alias.empty() )
      {
         stream.alias = _alias ;
      }

      return rc ;
   }

   string _qgmOptiAggregation::toString() const
   {
      stringstream ss ;
      ss << "{" << this->_qgmOptiTreeNode::toString() ;
      if ( !_selector.empty() )
      {
         ss << ",selector:[" ;
         qgmAggrSelectorVec::const_iterator itr =
                               _selector.begin() ;
         for ( ; itr != _selector.end(); itr++ )
         {
            ss << itr->toString() << "," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }

      if ( !_groupby.empty() )
      {
         ss << ", groupby:[" ;
         qgmOPFieldVec::const_iterator itr = _groupby.begin() ;
         for ( ; itr != _groupby.end(); itr++ )
         {
            ss << itr->value.toString() << "," ;
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }
      ss << "}" ;
      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION_PARSE, "_qgmOptiAggregation::parse" )
   INT32 _qgmOptiAggregation::parse( const qgmOpField &field,
                                     BOOLEAN &isFunc,
                                     BOOLEAN needRele )
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION_PARSE ) ;
      INT32 rc = SDB_OK ;
      _qgmAggrSelector selector ;
      if ( SQL_GRAMMAR::FUNC == field.type )
      {
         rc = qgmFindFieldFromFunc( field.value.attr().begin(),
                                    field.value.attr().size(),
                                    selector.value.value.attr(),
                                    selector.param,
                                    _table,
                                    needRele ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         isFunc = TRUE ;
         CHAR *pFuncName = NULL;
         rc = utilStrToUpper( field.value.attr().begin(), pFuncName ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         if ( 0 != ossStrcmp( "MERGEARRAYSET", pFuncName ) )
         {
            _hasAggrFunc = TRUE ;
         }
      }
      else
      {
         isFunc = FALSE ;
         selector.value.value = field.value ;
      }

      if ( !field.alias.empty() )
      {
         selector.value.alias = field.alias ;
      }

      selector.value.type = field.type ;

      _selector.push_back( selector ) ;
   done:
      PD_TRACE_EXITRC( SDB__QGMOPTIAGGREGATION_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMOPTIAGGREGATION_HASEXPR, "_qgmOptiAggregation::hasExpr" 
   BOOLEAN _qgmOptiAggregation::hasExpr() const
   {
      PD_TRACE_ENTRY( SDB__QGMOPTIAGGREGATION_HASEXPR ) ;
      BOOLEAN r = FALSE ;
      qgmAggrSelectorVec::const_iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); ++itr )
      {
         if ( SQL_GRAMMAR::DBATTR == itr->value.type &&
              !( itr->value.expr.isEmpty() ) )
         {
            r = TRUE ;
            break ;
         }
      }
      PD_TRACE_EXIT( SDB__QGMOPTIAGGREGATION_HASEXPR ) ;
      return r ;
   }

}
