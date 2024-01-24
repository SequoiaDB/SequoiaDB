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

   Source File Name = dmsWTCursorHolder.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_CURSOR_HOLDER_HPP_
#define DMS_WT_CURSOR_HOLDER_HPP_

#include "ossMemPool.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTStorageEngine.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "wiredtiger/dmsWTItem.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCursorHolder define
    */
   class _dmsWTCursorHolder : public _utilPooledObject
   {
   public:
      _dmsWTCursorHolder( dmsWTSession &session ) ;
      virtual ~_dmsWTCursorHolder() = default ;
      _dmsWTCursorHolder( const _dmsWTCursorHolder & ) = delete ;
      _dmsWTCursorHolder &operator =( const _dmsWTCursorHolder & ) = delete ;

   protected:
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   UINT64 startKey,
                   BOOLEAN isAfterStartKey,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   const dmsWTItem &startKey,
                   BOOLEAN isAfterStartKey,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   BOOLEAN isForward,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;
      INT32 _open( dmsWTStorageEngine &engine,
                   const ossPoolString &uri,
                   const ossPoolString &config,
                   UINT64 sampelNum,
                   UINT64 snapshotID,
                   IExecutor *executor ) ;

      INT32 _advance( IExecutor *executor ) ;

      INT32 _close() ;

   protected:
      dmsWTCursor _cursor ;
      UINT64 _snapshotID = DMS_INVALID_SNAPSHOT_ID ;
      BOOLEAN _isOpened = FALSE ;
      BOOLEAN _isClosed = FALSE ;
      BOOLEAN _isForward = TRUE ;
      BOOLEAN _isSample = FALSE ;
      BOOLEAN _isEOF = FALSE ;
   } ;

   typedef class _dmsWTCursorHolder dmsWTCursorHolder ;

}
}

#endif // DMS_WT_CURSOR_HOLDER_HPP_
