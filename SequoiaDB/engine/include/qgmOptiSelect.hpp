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

   Source File Name = qgmOptiSelect.hpp

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

#ifndef QGMOPTISELECT_HPP_
#define QGMOPTISELECT_HPP_

#include "qgmOptiTree.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{
   class _qgmExtendSelectPlan ;

   class _qgmOptiSelect : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiSelect( _qgmPtrTable *table, _qgmParamTable *param ) ;
      virtual ~_qgmOptiSelect() ;

      BOOLEAN hasConstraint() const { return _limit != -1 || _skip != 0 ; }
      void    clearConstraint() { _limit = -1 ; _skip = 0 ; }

      virtual INT32        init () ;
      virtual INT32        done () ;

   public:
      virtual INT32     outputSort( qgmOPFieldVec &sortFields ) ;
      virtual INT32 outputStream( qgmOpStream &stream ) ;
      virtual BOOLEAN   isEmpty() ;

      virtual string toString() const ;
      virtual INT32 handleHints( QGM_HINS &hints ) ;

      BOOLEAN hasExpr() const ;

      BSONObj getHint() const ;

   protected:
      virtual INT32 _extend( _qgmOptiTreeNode *&exNode ) ;
      virtual UINT32 _getFieldAlias( qgmOPFieldPtrVec &fieldAlias,
                                     BOOLEAN getAll ) ;
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

   private:

      void _initFrom() ;

      INT32 _validateAndCrtPlan( qgmOpStream &stream,
                                 _qgmExtendSelectPlan *ePlan ) ;

      INT32 _extendLocal( _qgmOptiTreeNode *&exNode ) ;

      INT32 _extendSub() ;

      INT32 _paramExistInSelector( const qgmDbAttr &field,
                                   BOOLEAN &found,
                                   qgmOpField *&selector,
                                   UINT32 *pos ) ;

      void _handleHint( QGM_HINS &hints ) ;

   public:
      qgmOPFieldVec        _selector ;
      qgmOPFieldVec        _orderby ;
      qgmOPFieldVec        _groupby ;
      qgmDbAttr            _splitby ;
      qgmOpField           _collection ;
      qgmConditionNode     *_condition ;
      INT64 _limit ;
      INT64 _skip ;
      BOOLEAN _hasFunc ;
      _qgmOptiTreeNode *_from ;
   } ;
   typedef class _qgmOptiSelect qgmOptiSelect ;
}

#endif

