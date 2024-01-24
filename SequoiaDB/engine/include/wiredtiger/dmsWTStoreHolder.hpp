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

   Source File Name = dmsWTStoreHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STORE_HOLDER_HPP_
#define DMS_WT_STORE_HOLDER_HPP_

#include "wiredtiger/dmsWTStore.hpp"
#include "wiredtiger/dmsWTStats.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStoreHolder define
    */
   class _dmsWTStoreHolder
   {
   public:
      _dmsWTStoreHolder( dmsWTStorageEngine &engine,
                         const dmsWTStore &store )
      : _engine( engine ),
        _store( store )
      {
      }

      ~_dmsWTStoreHolder() = default ;

      dmsWTStorageEngine &getEngine()
      {
         return _engine ;
      }

      dmsWTStore &getStore()
      {
         return _store ;
      }

      INT32 getCount( UINT64 &count, IExecutor *executor ) ;
      INT32 getStats( INT32 statsKey,
                      dmsWTStatsCatalog statsCatalog,
                      INT64 &statsValue,
                      IExecutor *executor ) ;
      INT32 getStoreTotalSize( UINT64 &totalSize, IExecutor *executor ) ;
      INT32 getStoreFreeSize( UINT64 &freeSize, IExecutor *executor ) ;

   protected:
      class _dmsWTStoreValidator
      {
      public:
         _dmsWTStoreValidator() = default ;
         virtual ~_dmsWTStoreValidator() = default ;

         virtual INT32 validate( const dmsWTItem &keyItem,
                                 const dmsWTItem &valueItem ) = 0 ;
      } ;

      INT32 _validateStore( _dmsWTStoreValidator &validator,
                            IExecutor *executor ) ;

   protected:
      dmsWTStorageEngine &_engine ;
      dmsWTStore _store ;
   } ;

   typedef class _dmsWTStoreHolder dmsWTStoreHolder ;

}
}

#endif // DMS_WT_STORE_HOLDER_HPP_
