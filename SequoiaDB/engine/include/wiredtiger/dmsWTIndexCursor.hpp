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

   Source File Name = dmsWTCursor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_INDEX_CURSOR_HPP_
#define DMS_WT_INDEX_CURSOR_HPP_

#include "interface/ICursor.hpp"
#include "wiredtiger/dmsWTCursorHolder.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTIndexCursor define
    */
   class _dmsWTIndexCursor : public IIndexCursor, public _dmsWTCursorHolder
   {
   public:
      _dmsWTIndexCursor( dmsWTSession &session ) ;
      virtual ~_dmsWTIndexCursor() = default ;
      _dmsWTIndexCursor( const _dmsWTIndexCursor & ) = delete ;
      _dmsWTIndexCursor &operator =( const _dmsWTIndexCursor & ) = delete ;

      virtual BOOLEAN isOpened() const
      {
         return _isOpened ;
      }

      virtual BOOLEAN isClosed() const
      {
         return _isClosed ;
      }

      virtual BOOLEAN isForward() const
      {
         return _isForward ;
      }

      virtual BOOLEAN isBackward() const
      {
         return !_isForward ;
      }

      virtual BOOLEAN isSample() const
      {
         return _isSample ;
      }

      virtual BOOLEAN isEOF() const
      {
         return _isEOF ;
      }

      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          const keystring::keyString &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;
      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;
      virtual INT32 advance( IExecutor *executor ) ;
      virtual INT32 locate( const bson::BSONObj &key,
                            const dmsRecordID &recordID,
                            BOOLEAN isAfterKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) ;
      virtual INT32 locate( const keystring::keyString &key,
                            BOOLEAN isAfterKey,
                            IExecutor *executor,
                            BOOLEAN &isFound ) ;

      virtual INT32 close()
      {
         _resetCache() ;
         return _close() ;
      }

      virtual INT32 pause( IExecutor *executor ) ;

      virtual INT32 getCurrentKeyString( keystring::keyString &key ) ;
      virtual INT32 getCurrentKey( bson::BSONObj &key ) ;
      virtual INT32 getCurrentRecordID( dmsRecordID &recordID ) ;
      virtual INT32 getCurrentRecord( dmsRecordData &data ) ;

      virtual UINT64 getSnapshotID() const
      {
         return _snapshotID ;
      }

      virtual void resetSnapshotID( UINT64 snapshotID )
      {
         _snapshotID = snapshotID ;
      }

      virtual BOOLEAN isAsync() const
      {
         return FALSE ;
      }

      virtual IStorageSession *getSession()
      {
         return &( _cursor.getSession() ) ;
      }

   protected:
      void _resetCache()
      {
         _keyStringCache.reset() ;
         _keyObjCache = BSONObj() ;
         _recordIDCache.reset() ;
      }

   protected:
      std::shared_ptr<IIndex> _idxPtr ;
      keystring::keyString _keyStringCache ;
      bson::BSONObj _keyObjCache ;
      dmsRecordID _recordIDCache ;
   } ;

   typedef class _dmsWTIndexCursor dmsWTIndexCursor ;

   /*
      _dmsWTIndexAsyncCursor define
    */
   class _dmsWTIndexAsyncCursor : public _dmsWTIndexCursor
   {
   public:
      _dmsWTIndexAsyncCursor() ;
      virtual ~_dmsWTIndexAsyncCursor() ;
      _dmsWTIndexAsyncCursor( const _dmsWTIndexAsyncCursor & ) = delete ;
      _dmsWTIndexAsyncCursor &operator =( const _dmsWTIndexAsyncCursor & ) = delete ;

      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          const keystring::keyString &startKey,
                          BOOLEAN isAfterStartKey,
                          BOOLEAN isForward,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;
      virtual INT32 open( std::shared_ptr<IIndex> idxPtr,
                          UINT64 sampleNum,
                          UINT64 snapshotID,
                          IExecutor *executor ) ;

      virtual BOOLEAN isAsync() const
      {
         return TRUE ;
      }

   protected:
      dmsWTSession _asyncSession ;
   } ;

   typedef class _dmsWTIndexAsyncCursor dmsWTIndexAsyncCursor ;

}
}

#endif // DMS_WT_INDEX_CURSOR_HPP_
