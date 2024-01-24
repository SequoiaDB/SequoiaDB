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

   Source File Name = qgmSelectorExprNode.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGM_SELECTOREXPRNODE_HPP_
#define QGM_SELECTOREXPRNODE_HPP_

#include "oss.hpp"
#include "core.hpp"
#include <boost/noncopyable.hpp>
#include <sstream>
#include "../bson/bson.hpp"

namespace engine
{
   class _qgmValueTuple ;

   class _qgmSelectorExprNode : public boost::noncopyable
   {
   public:
      _qgmSelectorExprNode() ;
      /// guarantee ele be valid
      ~_qgmSelectorExprNode() ;

   public:
      OSS_INLINE void setChildren( _qgmSelectorExprNode *l,
                                   _qgmSelectorExprNode *r )
      {
         _left = l ;
         _right = r ;
         return ;
      }

      OSS_INLINE void setLeft( _qgmSelectorExprNode *l )
      {
         _left = l ;
         return ;
      }

      OSS_INLINE void setRight( _qgmSelectorExprNode *r )
      {
         _right = r ;
         return ;
      }

      OSS_INLINE void setType( INT32 type )
      {
         _type = type ;
         return ;
      }

      OSS_INLINE void setValue( INT64 v )
      {
         *(( INT64 *)_data) = v ;
         _isDouble = FALSE ;
         return ;
      }

      OSS_INLINE void setValue( FLOAT64 v )
      {
         *(( FLOAT64 *)_data) = v ;
         _isDouble = TRUE ;
         return ;
      }

      INT32 getValue( const bson::BSONElement &e,
                      _qgmValueTuple *v ) const ;

      void toString( std::stringstream &ss )const ;

   private:
      INT32 _calcValue( const _qgmValueTuple &lv,
                        const _qgmValueTuple &rv,
                        INT32 type,
                        _qgmValueTuple &v ) const ;

   private:
      INT32 _type ;
      BOOLEAN _isDouble ;
      CHAR _data[8] ;
      _qgmSelectorExprNode *_left ;
      _qgmSelectorExprNode *_right ;
   } ;
}

#endif

