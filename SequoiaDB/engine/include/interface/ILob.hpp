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

   Source File Name = ILob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/08/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_LOB_HPP_
#define SDB_I_LOB_HPP_

#include "sdbInterface.hpp"
#include "interface/ICursor.hpp"
#include "utilPooledObject.hpp"
#include "dmsLobDef.hpp"

namespace engine
{

class ILob : public _utilPooledObject, public std::enable_shared_from_this< ILob >
{
public:
   struct updatedInfo
   {
      BOOLEAN hasUpdated;
      INT64 increasedSize;
   };

public:
   ILob() = default;
   virtual ~ILob() = default;
   ILob( const ILob & ) = delete;
   ILob &operator=( const ILob & ) = delete;

   virtual ICollection *getCollPtr() = 0;
   virtual UINT64 fetchSnapshotID() = 0;

   virtual INT32 write( const dmsLobRecord &record, IExecutor *executor ) = 0;
   virtual INT32 update( const dmsLobRecord &record,
                         IExecutor *executor,
                         updatedInfo *info = nullptr ) = 0;
   virtual INT32 writeOrUpdate( const dmsLobRecord &record,
                                IExecutor *executor,
                                updatedInfo *info = nullptr ) = 0;
   virtual INT32 remove( const dmsLobRecord &record, IExecutor *executor ) = 0;
   virtual INT32 read( const dmsLobRecord &record,
                       IExecutor *executor,
                       void *buf,
                       UINT32 &readLen ) const = 0;
   virtual INT32 list( IExecutor *executor, unique_ptr< ILobCursor > &cursor ) = 0;
   virtual INT32 truncate( IExecutor *executor ) = 0;
   virtual INT32 compact( IExecutor *executor ) = 0;
   virtual INT32 validate( IExecutor *executor ) = 0;
};
} // namespace engine
#endif