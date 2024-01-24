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

   Source File Name = dmsWTItem.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_ITEM_HPP_
#define DMS_WT_ITEM_HPP_

#include "dmsDef.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.hpp"
#include "utilSlice.hpp"

#include <wiredtiger.h>

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTItem define
    */
   class _dmsWTItem : public SDBObject
   {
   public:
      _dmsWTItem()
      {
         _item.data = nullptr ;
         _item.size = 0 ;
      }

      _dmsWTItem( const void* d, size_t s )
      {
         _item.data = d ;
         _item.size = s ;
      }

      ~_dmsWTItem() = default ;
      _dmsWTItem( const _dmsWTItem &o ) = delete ;
      _dmsWTItem &operator =( const _dmsWTItem & ) = delete ;

      _dmsWTItem( const bson::BSONObj &obj )
      : _dmsWTItem( obj.objdata(), obj.objsize() )
      {
      }

      _dmsWTItem( const utilSlice &slice )
      : _dmsWTItem( slice.getData(), slice.getSize() )
      {
      }

      void init( const bson::BSONObj &obj )
      {
         _item.data = obj.objdata() ;
         _item.size = obj.objsize() ;
      }

      void init( const utilSlice &slice )
      {
         _item.data = slice.getData() ;
         _item.size = slice.getSize() ;
      }

      WT_ITEM *get()
      {
         return &_item ;
      }

      const WT_ITEM *get() const
      {
         return &_item ;
      }

      const void *getData() const
      {
         return _item.data ;
      }

      UINT32 getSize() const
      {
         return _item.size ;
      }

      void reset()
      {
         _item.data = nullptr ;
         _item.size = 0 ;
      }

   protected:
      WT_ITEM _item ;
   } ;

   typedef class _dmsWTItem dmsWTItem ;

}
}

#endif // DMS_WT_ITEM_HPP_
