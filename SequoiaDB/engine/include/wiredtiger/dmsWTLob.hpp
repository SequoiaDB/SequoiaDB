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

   Source File Name = dmsWTIndex.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/08/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_LOB_HPP_
#define DMS_WT_LOB_HPP_

#include "dmsMetadata.hpp"
#include "interface/ILob.hpp"
#include "wiredtiger/dmsWTStoreHolder.hpp"

namespace engine
{
namespace wiredtiger
{
   class _dmsWTLob : public ILob, public _dmsWTStoreHolder
   {
   public:
      _dmsWTLob( dmsWTStorageEngine &engine, const dmsWTStore &store, ICollection *collPtr )
      : dmsWTStoreHolder( engine, store ), _collPtr( collPtr )
      {
      }

      static INT32 buildLobURI( utilCSUniqueID csUID,
                                utilCLInnerID clInnerID,
                                UINT32 clLID,
                                ossPoolString &uri );

      virtual ICollection *getCollPtr() override;
      virtual UINT64 fetchSnapshotID() override;

      virtual INT32 write( const dmsLobRecord &record, IExecutor *executor ) override;
      virtual INT32 update( const dmsLobRecord &record,
                            IExecutor *executor,
                            updatedInfo *info ) override;
      virtual INT32 writeOrUpdate( const dmsLobRecord &record,
                                   IExecutor *executor,
                                   updatedInfo *info ) override;
      virtual INT32 remove( const dmsLobRecord &record, IExecutor *executor ) override;
      virtual INT32 read( const dmsLobRecord &record,
                          IExecutor *executor,
                          void *buf,
                          UINT32 &readLen ) const override;
      virtual INT32 list( IExecutor *executor, unique_ptr< ILobCursor > &cursor ) override;
      virtual INT32 truncate( IExecutor *executor ) override;
      virtual INT32 compact( IExecutor *executor ) override;
      virtual INT32 validate( IExecutor *executor ) override;

   private:
      ICollection *_collPtr = nullptr;
   };
   typedef _dmsWTLob dmsWTLob;
} // namespace wiredtiger
} // namespace engine
#endif