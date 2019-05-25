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

   Source File Name = qgmOprUnit.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/04/2012  XJH Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmOprUnit.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{

   _qgmFilterUnit::_qgmFilterUnit( QGM_OPTI_TYPE type )
   :_qgmOprUnit( type )
   {
   }

   _qgmFilterUnit::_qgmFilterUnit( const _qgmFilterUnit &right,
                                   BOOLEAN delDup )
   :_qgmOprUnit( right, delDup )
   {
   }

   _qgmFilterUnit::~_qgmFilterUnit()
   {
      emptyCondition() ;
   }

   BOOLEAN _qgmFilterUnit::isOptional() const
   {
      if ( !_optional || _conds.size() > 0 || _hasFieldAlias() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _qgmFilterUnit::isFieldsOptional() const
   {
      if ( !_optional || _hasFieldAlias() )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   OPR_FILTER_TYPE _qgmFilterUnit::filterType() const
   {
      if ( 0 == _conds.size() )
      {
         return FILTER_SEC ;
      }
      else if ( 0 == _fields.size() )
      {
         return FILTER_CON ;
      }
      return FILTER_BOTH ;
   }

   INT32 _qgmFilterUnit::setCondition( qgmConditionNode * cond )
   {
      emptyCondition() ;

      if ( cond )
      {
         return _separateCond( cond, _conds ) ;
      }
      return SDB_OK ;
   }

   INT32 _qgmFilterUnit::setCondition( qgmConditionNodePtrVec &conds )
   {
      emptyCondition() ;

      return addCondition( conds ) ;
   }

   INT32 _qgmFilterUnit::addCondition( qgmConditionNode * cond )
   {
      if ( !cond )
      {
         return SDB_INVALIDARG ;
      }
      return _separateCond( cond, _conds ) ;
   }

   INT32 _qgmFilterUnit::addCondition( qgmConditionNodePtrVec & conds )
   {
      INT32 rc = SDB_OK ;
      qgmConditionNodePtrVec::iterator it = conds.begin() ;
      while ( it != conds.end() )
      {
         rc = addCondition( *it ) ;
         if ( SDB_OK != rc )
         {
            break ;
         }
         ++it ;
      }
      return rc ;
   }

   void _qgmFilterUnit::emptyCondition()
   {
      _conds.clear() ;
   }

   INT32 _qgmFilterUnit::removeCondition( qgmConditionNode *cond )
   {
      qgmConditionNodePtrVec::iterator it = _conds.begin() ;
      while ( it != _conds.end() )
      {
         if ( *it == cond )
         {
            _conds.erase( it ) ;
            return SDB_OK ;
         }
         ++it ;
      }
      return SDB_INVALIDARG ;
   }

   INT32 _qgmFilterUnit::removeCondition( qgmConditionNodePtrVec & conds )
   {
      INT32 rc = SDB_OK ;
      qgmConditionNodePtrVec::iterator it = conds.begin() ;
      while ( it != conds.end() )
      {
         rc = removeCondition( *it ) ;
         ++it ;
      }
      return rc ;
   }

   qgmConditionNode* _qgmFilterUnit::getMergeCondition()
   {
      if ( 0 == _conds.size() )
      {
         return NULL ;
      }
      else if ( 1 == _conds.size() )
      {
         return _conds[0] ;
      }
      else
      {
         qgmConditionNodeHelper condTree( NULL ) ;
         INT32 rc = condTree.merge( _conds ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "oprUnit[%s] merge condition failed, rc: %d",
                    toString().c_str(), rc ) ;
            return NULL ;
         }
         _conds.clear() ;
         return condTree.getRoot() ;
      }
   }

   qgmConditionNodePtrVec _qgmFilterUnit::getConditions()
   {
      return _conds ;
   }

   void _qgmFilterUnit::_toString( stringstream & ss ) const
   {
      if ( _conds.size() > 0 )
      {
         qgmConditionNodePtrVec::const_iterator cit = _conds.begin() ;
         while ( cit != _conds.end() )
         {
            ss << ", condition:" << qgmConditionNodeHelper( *cit ).toJson() ;
            ++cit ;
         }
      }

      ss << ", fieldOptional:" << ( isFieldsOptional() ? "TRUE" : "FALSE" ) ;
   }

   INT32 _qgmFilterUnit::_replaceRele( const qgmField & newRele )
   {
      qgmDbAttrPtrVec attrs ;
      getCondFields( attrs ) ;
      replaceAttrRele( attrs, newRele ) ;
      return SDB_OK ;
   }

   INT32 _qgmFilterUnit::_replaceFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      qgmDbAttrPtrVec attrs ;
      getCondFields( attrs ) ;
      downAttrsByFieldAlias( attrs, fieldAlias, isOptional() ) ;
      return SDB_OK ;
   }

   INT32 _qgmFilterUnit::_restoreFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      qgmDbAttrPtrVec attrs ;
      getCondFields( attrs ) ;
      upAttrsByFieldAlias( attrs, fieldAlias ) ;
      return SDB_OK ;
   }

   void _qgmFilterUnit::getCondFields( qgmDbAttrPtrVec & fields )
   {
      qgmConditionNodeHelper condHelper( NULL ) ;
      qgmConditionNodePtrVec::iterator it = _conds.begin() ;
      while ( it != _conds.end() )
      {
         condHelper.setRoot( *it ) ;
         condHelper.getAllAttr( fields ) ;
         ++it ;
      }
   }

   INT32 _qgmFilterUnit::_separateCond( qgmConditionNode * cond,
                                        qgmConditionNodePtrVec & conds )
   {
      INT32 rc = SDB_OK ;
      if ( SQL_GRAMMAR::AND != cond->type )
      {
         conds.push_back( cond ) ;
      }
      else
      {
         qgmConditionNodeHelper condTree( cond ) ;
         rc = condTree.separate( conds ) ;
      }
      return rc ;
   }

   BOOLEAN _qgmFilterUnit::_hasFieldAlias() const
   {
      qgmOPFieldVec::const_iterator cit = _fields.begin() ;
      while ( cit != _fields.end() )
      {
         const qgmOpField &field = *cit ;
         if ( !field.alias.empty() )
         {
            return TRUE ;
         }
         ++cit ;
      }
      return FALSE ;
   }

   _qgmAggrUnit::_qgmAggrUnit( QGM_OPTI_TYPE type )
   : _qgmOprUnit( type ) 
   {
   }

   _qgmAggrUnit::_qgmAggrUnit( const _qgmAggrUnit &right,
                               BOOLEAN delDup )
   :_qgmOprUnit( right, delDup )
   {
      addAggrSelector( right._aggrSelect ) ;
   }

   _qgmAggrUnit::~_qgmAggrUnit()
   {
   }

   INT32 _qgmAggrUnit::addAggrSelector( const qgmAggrSelector & selector )
   {
      _aggrSelect.push_back( selector ) ;
      return SDB_OK ;
   }

   INT32 _qgmAggrUnit::addAggrSelector( const qgmAggrSelectorVec & selectors )
   {
      qgmAggrSelectorVec::const_iterator cit = selectors.begin() ;
      while ( cit != selectors.end() )
      {
         addAggrSelector( *cit ) ;
         ++cit ;
      }
      return SDB_OK ;
   }

   void _qgmAggrUnit::_toString( stringstream & ss ) const
   {
      ss << ",aggrs:{" ;

      qgmAggrSelectorVec::const_iterator cit = _aggrSelect.begin() ;
      while ( cit != _aggrSelect.end() )
      {
         ss << (*cit).toString() ;
         ++cit ;
      }
   }

   INT32 _qgmAggrUnit::_replaceRele( const qgmField & newRele )
   {
      replaceAggrRele( _aggrSelect, newRele ) ;
      return SDB_OK ;
   }

   INT32 _qgmAggrUnit::_replaceFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      return downAggrsByFieldAlias( _aggrSelect, fieldAlias, isOptional() ) ;
   }

   INT32 _qgmAggrUnit::_restoreFieldAlias( const qgmOPFieldPtrVec & fieldAlias )
   {
      return upAggrsByFieldAlias( _aggrSelect, fieldAlias ) ;
   }

}


