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

   Source File Name = dmsWTDataCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/12/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_LOB_CURSOR_HPP_
#define DMS_WT_LOB_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "restAdaptor.hpp"
#include "wiredtiger/dmsWTCursorHolder.hpp"

namespace engine
{
namespace wiredtiger
{
   /*
     _dmsWTLobCursor define
    */
   class _dmsWTLobCursor : public ILobCursor, public _dmsWTCursorHolder
   {
   public:
      _dmsWTLobCursor( dmsWTSession &session );
      virtual ~_dmsWTLobCursor() = default;
      _dmsWTLobCursor( const _dmsWTLobCursor & ) = delete;
      _dmsWTLobCursor &operator=( const _dmsWTLobCursor & ) = delete;

      virtual BOOLEAN isOpened() const override
      {
         return _isOpened;
      }

      virtual BOOLEAN isClosed() const override
      {
         return _isClosed;
      }

      virtual BOOLEAN isForward() const override
      {
         return _isForward;
      }

      virtual BOOLEAN isBackward() const override
      {
         return !_isForward;
      }

      virtual BOOLEAN isSample() const override
      {
         return _isSample;
      }

      virtual BOOLEAN isEOF() const override
      {
         return _isEOF;
      }

      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          const dmsLobRecord &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) override;

      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) override;

      virtual INT32 locate( const dmsLobRecord &rid,
                            BOOLEAN isAfterStartKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) override;

      virtual INT32 close() override
      {
         _resetCache();
         return _close();
      }

      virtual INT32 advance( IExecutor *executor ) override
      {
         _resetCache();
         return _advance( executor );
      }

      virtual INT32 pause( IExecutor *executor );

      virtual INT32 getCurrentLobRecord( dmsLobInfoOnPage &info, const CHAR **data ) override;

      virtual UINT64 getSnapshotID() const override
      {
         return _snapshotID;
      }

      virtual void resetSnapshotID( UINT64 snapshotID ) override
      {
         _snapshotID = snapshotID;
      }

      virtual BOOLEAN isAsync() const override
      {
         return FALSE ;
      }

      virtual IStorageSession *getSession() override
      {
         return &( _cursor.getSession() ) ;
      }

   protected:
      void _resetCache()
      {
         _recordIDCache.clear();
      }

   protected:
      std::shared_ptr< ILob > _lobPtr;
      dmsLobRecord _recordIDCache;
   };
   typedef _dmsWTLobCursor dmsWTLobCursor;

   /*
      _dmsWTLobAsyncCursor define
    */
   class _dmsWTLobAsyncCursor : public _dmsWTLobCursor
   {
   public:
      _dmsWTLobAsyncCursor();
      virtual ~_dmsWTLobAsyncCursor();
      _dmsWTLobAsyncCursor( const _dmsWTLobAsyncCursor & ) = delete;
      _dmsWTLobAsyncCursor &operator=( const _dmsWTLobAsyncCursor & ) = delete;

      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          const dmsLobRecord &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) override;
      virtual INT32 open( std::shared_ptr< ILob > lobPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) override;

      virtual BOOLEAN isAsync() const override
      {
         return TRUE ;
      }

   protected:
      dmsWTSession _asyncSession;
   };
   typedef class _dmsWTLobAsyncCursor dmsWTLobAsyncCursor;
} // namespace wiredtiger
} // namespace engine

#endif