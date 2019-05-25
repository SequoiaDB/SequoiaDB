/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

using std::vector ;

namespace engine
{
   class _rtnQueryModifier: public SDBObject
   {
   public:
      _rtnQueryModifier( BOOLEAN isUpdate,
                         BOOLEAN isRemove,
                         BOOLEAN returnNew = FALSE ) ;

      ~_rtnQueryModifier()
      {
      }

      INT32 loadUpdator( const bson::BSONObj &updator ) ;

      inline BOOLEAN isUpdate() const { return _isUpdate ; }
      inline BOOLEAN isRemove() const { return _isRemove ; }
      inline BOOLEAN returnNew() const { return _returnNew ; }
      inline mthModifier& getModifier() { return _modifier ; }
      inline vector<INT64>* getDollarList() { return &_dollarList ; }

   private:
      _rtnQueryModifier( const _rtnQueryModifier& ) ;
      void operator=( const _rtnQueryModifier& ) ;

   private:
      BOOLEAN        _isUpdate ;
      BOOLEAN        _isRemove ;
      BOOLEAN        _returnNew ;
      bson::BSONObj  _updator ;
      mthModifier    _modifier ;
      vector<INT64>  _dollarList ;
   } ;
   typedef class _rtnQueryModifier rtnQueryModifier ;
}

#endif /* RTN_QUERYMODIFIER_HPP_ */
