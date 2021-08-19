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

   Source File Name = qgmSelectorExpr.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGM_SELECTOREXPR_HPP_
#define QGM_SELECTOREXPR_HPP_

#include "qgmSelectorExprNode.hpp"
#include <boost/shared_ptr.hpp>

namespace engine
{
   class _qgmSelectorExpr : public SDBObject
   {
   public:
      _qgmSelectorExpr() ;
      ~_qgmSelectorExpr() ;
      _qgmSelectorExpr( const _qgmSelectorExpr &e )
      :_exprRoot( e._exprRoot )
      {

      }

      _qgmSelectorExpr &operator=(const _qgmSelectorExpr &e)
      {
         _exprRoot = e._exprRoot ;
         return *this ;
      }

   public:
      BOOLEAN isEmpty() const
      {
         return NULL == _exprRoot.get() ;
      }

      void set( _qgmSelectorExprNode *root )
      {
         _exprRoot.reset( root ) ;
         return ;
      }

      std::string toString() const ;

      INT32 getValue( const bson::BSONElement &e,
                      _qgmValueTuple &v ) const ;

   private:
      boost::shared_ptr<_qgmSelectorExprNode> _exprRoot ;
   } ;
}

#endif

