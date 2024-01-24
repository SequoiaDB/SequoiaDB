/******************************************************************************

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

   Source File Name = utilCSKeyName.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/03/2022  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_CS_KEY_NAME_HPP_
#define UTIL_CS_KEY_NAME_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _utilCSKeyName define
    */
   typedef struct _utilCSKeyName
   {
      ossPoolString        _name ;

      _utilCSKeyName( const ossPoolString &name )
      {
         _name = name ;
      }

      _utilCSKeyName( const CHAR *name )
      {
         _name.assign( name ) ;
      }

      _utilCSKeyName()
      {
      }

      bool operator< ( const _utilCSKeyName &rhs ) const
      {
         UINT32 llen = _name.length() ;
         UINT32 rlen = rhs._name.length() ;
         UINT32 len = llen <= rlen ? llen : rlen ;

         INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
         if ( 0 == cmp && llen == len && rlen != len &&
              '.' != rhs._name[len] )
         {
            cmp = -1 ;
         }

         return cmp < 0 ? true : false ;
      }

      bool operator== ( const _utilCSKeyName &rhs ) const
      {
         UINT32 llen = _name.length() ;
         UINT32 rlen = rhs._name.length() ;
         UINT32 len = llen <= rlen ? llen : rlen ;

         INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
         if ( 0 == cmp && ( llen == len || '.' == _name[len] ) &&
              ( rlen == len || '.' == rhs._name[len] ) )
         {
            return true ;
         }
         return false ;
      }
   } utilCSKeyName ;

}

#endif
