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

   Source File Name = rtnQueryModifier.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          24/3/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_QUERYMODIFIER_HPP_
#define RTN_QUERYMODIFIER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "mthModifier.hpp"
#include "rtnHintModifier.hpp"

using std::vector ;

namespace engine
{
   class _rtnQueryModifier: public rtnHintModifier
   {
   public:
      _rtnQueryModifier() ;

      ~_rtnQueryModifier()
      {
      }

      inline BOOLEAN isUpdate() const
      {
         return RTN_MODIFY_UPDATE == _modifyOp ;
      }

      inline BOOLEAN isRemove() const
      {
         return RTN_MODIFY_REMOVE == _modifyOp ;
      }

      inline BOOLEAN returnNew() const { return _returnNew ; }
      inline mthModifier& getModifier() { return _modifier ; }
      inline vector<INT64>* getDollarList() { return &_dollarList ; }

   private:
      // disallow copy and assign
      _rtnQueryModifier( const _rtnQueryModifier& ) ;
      void operator=( const _rtnQueryModifier& ) ;

      INT32 _parseModifyEle( const BSONElement &ele ) ;
      INT32 _onInit() ;

   private:
      BOOLEAN        _returnNew ;
      mthModifier    _modifier ;
      vector<INT64>  _dollarList ;
   } ;
   typedef class _rtnQueryModifier rtnQueryModifier ;
}

#endif /* RTN_QUERYMODIFIER_HPP_ */
