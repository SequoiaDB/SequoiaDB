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

   Source File Name = sptReturnVal.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptReturnVal.hpp"
#include "sptBsonobj.hpp"
#include "sptBsonobjArray.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   _sptReturnVal::_sptReturnVal()
   {
   }

   _sptReturnVal::~_sptReturnVal()
   {
      UINT32 i = 0 ;
      for ( i = 0 ; i < _valProperties.size() ; ++i )
      {
         SDB_OSS_DEL _valProperties[ i ] ;
      }
      _valProperties.clear() ;

      for ( i = 0 ; i < _selfProperties.size() ; ++i )
      {
         SDB_OSS_DEL _selfProperties[ i ] ;
      }
      _selfProperties.clear() ;
   }

   sptProperty* _sptReturnVal::addReturnValProperty( const std::string &name,
                                                     UINT32 attr )
   {
      sptProperty *add = SDB_OSS_NEW sptProperty() ;
      if ( add )
      {
         add->setName( name ) ;
         add->setAttr( attr ) ;
         _valProperties.push_back( add ) ;
      }
      return add ;
   }

   sptProperty* _sptReturnVal::addSelfProperty( const std::string &name,
                                                UINT32 attr )
   {
      sptProperty *add = SDB_OSS_NEW sptProperty() ;
      if ( add )
      {
         add->setName( name ) ;
         add->setAttr( attr ) ;
         _selfProperties.push_back( add ) ;
      }
      return add ;
   }

   void _sptReturnVal::setReturnValName( const string &name )
   {
      _val.setName( name ) ;
   }

   void _sptReturnVal::setReturnValAttr( UINT32 attr )
   {
      _val.setAttr( attr ) ;
   }

}

