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

   Source File Name = sptReturnVal.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_RETURNVAL_HPP_
#define SPT_RETURNVAL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptProperty.hpp"
#include <vector>
#include <string>

namespace engine
{
   typedef SPT_PROP_ARRAY  SPT_PROPERTIES ;

   /*
      _sptReturnVal define
   */
   class _sptReturnVal : public SDBObject
   {
   public:
      _sptReturnVal() ;
      ~_sptReturnVal() ;

      sptProperty& getReturnVal()
      {
         return _val ;
      }

      void setReturnValAttr( UINT32 attr ) ;
      void setReturnValName( const string &name ) ;

      sptProperty* addReturnValProperty( const std::string &name,
                                         UINT32 attr = SPT_PROP_DEFAULT ) ;
      sptProperty* addSelfProperty( const std::string &name,
                                    UINT32 attr = SPT_PROP_DEFAULT ) ;

      const SPT_PROPERTIES& getReturnValProperties() const
      {
         return _valProperties ;
      }

      const SPT_PROPERTIES& getSelfProperties() const
      {
         return _selfProperties ;
      }

      template< typename T >
      INT32 setUsrObjectVal( void *value )
      {
         return _val.assignUsrObject< T >( value ) ;
      }

   private:
      sptProperty       _val ;

      SPT_PROPERTIES    _valProperties ;

      SPT_PROPERTIES    _selfProperties ;
   } ;

   typedef class _sptReturnVal sptReturnVal ;
}

#endif // SPT_RETURNVAL_HPP_

