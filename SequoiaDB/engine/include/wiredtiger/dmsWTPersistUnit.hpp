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

   Source File Name = dmsWTPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_PERSIST_UNIT_HPP_
#define DMS_WT_PERSIST_UNIT_HPP_

#include "dmsPersistUnit.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"
#include "wiredtiger/dmsWTUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTPersistUnit define
    */
   class _dmsWTPersistUnit : public _dmsPersistUnit
   {
   public:
      _dmsWTPersistUnit( dmsWTStorageEngine &engine ) ;
      ~_dmsWTPersistUnit() ;
      _dmsWTPersistUnit( const _dmsWTPersistUnit &o ) = delete ;
      _dmsWTPersistUnit &operator =( const _dmsWTPersistUnit & ) = delete ;

      dmsWTSession &getWriteSession()
      {
         return _writeSession ;
      }

      dmsWTSession &getReadSession()
      {
         return _readSession ;
      }

      INT32 initUnit( IExecutor *executor ) ;

   protected:
      virtual INT32 _beginUnit( IExecutor *executor ) ;
      virtual INT32 _prepareUnit( IExecutor *executor ) ;
      virtual INT32 _commitUnit( IExecutor *executor ) ;
      virtual INT32 _abortUnit( IExecutor *executor ) ;

      virtual BOOLEAN _isTransSupported() const
      {
         return FALSE ;
      }

   protected:
      dmsWTStorageEngine &_engine ;
      // session for write operators
      // NOTE: snapshot of read session will not affect write session
      dmsWTSession _writeSession ;
      // session for read operators
      // NOTE: rollback of write session will not affect read session
      dmsWTSession _readSession ;
   } ;

   typedef class _dmsWTPersistUnit dmsWTPersistUnit ;

}
}

#endif // DMS_WT_PERSIST_UNIT_HPP_
