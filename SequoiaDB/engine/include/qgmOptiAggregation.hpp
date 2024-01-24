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

   Source File Name = qgmOptiAggregation.hpp

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

#ifndef QGMOPTIAGGREGATION_HPP_
#define QGMOPTIAGGREGATION_HPP_

#include "qgmOptiTree.hpp"

namespace engine
{
   struct _qgmAggrSelector
   {
      qgmOpField     value ;
      qgmOPFieldVec  param ;

      _qgmAggrSelector()
      {
      }

      _qgmAggrSelector( const qgmOpField & field )
      {
         value = field ;
      }

      string toString() const
      {
         stringstream ss ;
         if ( SQL_GRAMMAR::DBATTR == value.type )
         {
            ss << "{value:" << value.value.toString()
               << ", alias:" << value.alias.toString() ;
            if ( !(value.expr.isEmpty()) )
            {
               ss << ", expr:" << value.expr.toString() ;
            }
            ss << "}" ;
         }
         else
         {
            ss << "{value:" << value.value.toString() ;
            if ( !value.alias.empty() )
            {
               ss << ", alias:" << value.alias.toString() ;
            }
            if ( !param.empty() )
            {
               ss << ", params:[" ;
               for ( qgmOPFieldVec::const_iterator itr = param.begin() ;
                     itr != param.end();
                     itr++ )
               {
                  ss << itr->value.toString() << ",";
               }
               ss.seekp((INT32)ss.tellp()-1 ) ;
               ss << "]" ;
            }
            ss << "}" ;
         }
         return ss.str() ;
      }
   } ;
   typedef struct _qgmAggrSelector qgmAggrSelector ;
   typedef ossPoolVector< qgmAggrSelector >  qgmAggrSelectorVec ;

   class _qgmOptiAggregation : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiAggregation( _qgmPtrTable *table,
                           _qgmParamTable *param ) ;
      virtual ~_qgmOptiAggregation() ;

      virtual INT32        init () ;
      virtual INT32        done () ;

   public:
      virtual INT32 outputSort( qgmOPFieldVec &sortFields ) ;
      virtual INT32 outputStream( qgmOpStream &stream ) ;

      virtual string toString() const ;

      INT32 parse( const qgmOpField &field,
                   BOOLEAN &isFunc,
                   BOOLEAN needRele ) ;

      qgmAggrSelectorVec &aggrSelector()
      {
         return _selector ;
      }

      BOOLEAN isInAggrFieldAlias( const qgmDbAttr &field ) const ;

      BOOLEAN hasExpr() const ;

   protected:
      virtual UINT32 _getFieldAlias( qgmOPFieldPtrVec &fieldAlias,
                                     BOOLEAN getAll ) ;
      virtual INT32 _pushOprUnit( qgmOprUnit *oprUnit, PUSH_FROM from ) ;
      virtual INT32 _removeOprUnit( qgmOprUnit *oprUnit ) ;
      virtual INT32 _updateChange( qgmOprUnit *oprUnit ) ;

      INT32   _addFields( qgmOprUnit *oprUnit ) ;

      enum AGGR_TYPE { AGGR_GROUPBY, AGGR_SELECTOR } ;
      void    _update2Unit( AGGR_TYPE type ) ;

   public:
      qgmAggrSelectorVec _selector ;      /// parsed
      qgmOPFieldVec      _tmpSelector ;
      qgmOPFieldVec      _groupby ;

   private:
      BOOLEAN            _hasAggrFunc;

   } ;
   typedef class _qgmOptiAggregation qgmOptiAggregation ;
}

#endif

