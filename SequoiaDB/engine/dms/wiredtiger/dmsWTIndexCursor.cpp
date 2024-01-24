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

   Source File Name = dmsWTIndexCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTIndexCursor.hpp"
#include "keystring/utilKeyStringBuilder.hpp"
#include "wiredtiger/dmsWTIndex.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;
using namespace bson ;
using namespace engine::keystring ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTIndexCursor implement
    */
   _dmsWTIndexCursor::_dmsWTIndexCursor( dmsWTSession &session )
   : _dmsWTCursorHolder( session )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_OPEN, "_dmsWTIndexCursor::open" )
   INT32 _dmsWTIndexCursor::open( shared_ptr<IIndex> idxPtr,
                                  const keyString &startKey,
                                  BOOLEAN isAfterStartKey,
                                  BOOLEAN isForward,
                                  UINT64 snapshotID,
                                  IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_OPEN ) ;

      dmsWTIndex *wtIndex = dynamic_cast<dmsWTIndex *>( idxPtr.get() ) ;

      PD_CHECK( wtIndex, SDB_SYS, error, PDERROR,
                "Failed to open cursor, index is not WiredTiger index" ) ;

      _idxPtr = std::move( idxPtr ) ;
      _resetCache() ;
      if ( startKey.isValid() )
      {
         dmsWTItem key( startKey.getKeySlice() ) ;
         rc = _open( wtIndex->getEngine(), wtIndex->getStore().getURI(),
                     "", key, isAfterStartKey, isForward, snapshotID, executor ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }
      else
      {
         rc = _open( wtIndex->getEngine(), wtIndex->getStore().getURI(),
                     "", isForward, snapshotID, executor ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_OPEN, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_OPEN_SAMPLE, "_dmsWTIndexCursor::open" )
   INT32 _dmsWTIndexCursor::open( shared_ptr<IIndex> idxPtr,
                                  UINT64 sampleNum,
                                  UINT64 snapshotID,
                                  IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_OPEN_SAMPLE ) ;

      dmsWTIndex *wtIndex = dynamic_cast<dmsWTIndex *>( idxPtr.get() ) ;

      PD_CHECK( wtIndex, SDB_SYS, error, PDERROR,
                "Failed to open cursor, index is not WiredTiger index" ) ;

      _idxPtr = std::move( idxPtr ) ;
      _resetCache() ;

      rc = _open( wtIndex->getEngine(), wtIndex->getStore().getURI(),
                  "", sampleNum, snapshotID, executor ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_OPEN_SAMPLE, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_ADVANCE, "_dmsWTIndexCursor::advance" )
   INT32 _dmsWTIndexCursor::advance( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_ADVANCE ) ;

      _resetCache() ;
      rc = _advance( executor ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_IXM_EOC ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_LOCATE_BSON, "_dmsWTIndexCursor::locate" )
   INT32 _dmsWTIndexCursor::locate( const BSONObj &key,
                                    const dmsRecordID &recordID,
                                    BOOLEAN isAfterKey,
                                    IExecutor *executor,
                                    BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_LOCATE_BSON ) ;

      keyStringStackBuilder builder ;

      rc = builder.buildPredicate( key,
                                   _idxPtr->getMetadata().getOrdering(),
                                   _idxPtr->getMetadata().isStrictUnique() ?
                                         dmsRecordID() :
                                         recordID,
                                   _isForward,
                                   isAfterKey ?
                                         keyStringDiscriminator::EXCLUSIVE_AFTER :
                                         keyStringDiscriminator::INCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;

      rc = locate( builder.getShallowKeyString(), FALSE, executor, isFound ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to locate cursor, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_LOCATE_BSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_LOCATE, "_dmsWTIndexCursor::locate" )
   INT32 _dmsWTIndexCursor::locate( const keystring::keyString &key,
                                    BOOLEAN isAfterKey,
                                    IExecutor *executor,
                                    BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_LOCATE ) ;

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current key string, cursor is not opened" ) ;

      resetSnapshotID( _idxPtr->fetchSnapshotID() ) ;
      _resetCache() ;
      if ( _isForward )
      {
         rc = _cursor.searchNext( dmsWTItem( key.getKeySlice() ),
                                  isAfterKey,
                                  isFound ) ;
      }
      else
      {
         rc = _cursor.searchPrev( dmsWTItem( key.getKeySlice() ),
                                  isAfterKey,
                                  isFound ) ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
            rc = SDB_IXM_EOC ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to locate cursor, rc: %d", rc ) ;
         }
         goto error ;
      }
      _isEOF = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_LOCATE, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEXCURSOR_PAUSE, "_dmsWTIndexCursor::pause" )
   INT32 _dmsWTIndexCursor::pause( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEXCURSOR_PAUSE ) ;

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to pause cursor, cursor is not opened" ) ;

      _resetCache() ;
      rc = _cursor.pause() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEXCURSOR_PAUSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_GETCURKEYSTR, "_dmsWTIndexCursor::getCurrentKeyString" )
   INT32 _dmsWTIndexCursor::getCurrentKeyString( keyString &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_GETCURKEYSTR ) ;

      PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                "Failed to get current key string, cursor is hit end" ) ;
      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current key string, cursor is not opened" ) ;

      if ( _keyStringCache.isValid() )
      {
         key = _keyStringCache ;
      }
      else
      {
         dmsWTItem keyItem, valueItem ;

         rc = _cursor.getKey( keyItem ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

         rc = _cursor.getValue( valueItem ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

         rc = _keyStringCache.init(
                           utilSlice( keyItem.getSize(), keyItem.getData() ),
                           utilSlice( valueItem.getSize(), valueItem.getData() ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize key string, rc: %d", rc ) ;

         key = _keyStringCache ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_GETCURKEYSTR, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_GETCURKEY, "_dmsWTIndexCursor::getCurrentKey" )
   INT32 _dmsWTIndexCursor::getCurrentKey( BSONObj &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_GETCURKEY ) ;

      if ( !_keyObjCache.isEmpty() )
      {
         key = _keyObjCache ;
      }
      else
      {
         keyString ks ;

         rc = getCurrentKeyString( ks ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key string, rc: %d", rc ) ;

         try
         {
            _keyObjCache = ks.toBSON( _idxPtr->getMetadata().getKeyPattern() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get key object, occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         key = _keyObjCache ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_GETCURKEY, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR_GETCURRECID, "_dmsWTIndexCursor::getCurrentRecordID" )
   INT32 _dmsWTIndexCursor::getCurrentRecordID( dmsRecordID &recordID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR_GETCURRECID ) ;

      if ( _recordIDCache.isValid() )
      {
         recordID = _recordIDCache ;
      }
      else
      {
         keyString key ;

         rc = getCurrentKeyString( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key string, rc: %d", rc ) ;

         _recordIDCache = key.getRID() ;
         recordID = _recordIDCache ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR_GETCURRECID, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXCURSOR__GETCURREC, "_dmsWTIndexCursor::getCurrentRecord" )
   INT32 _dmsWTIndexCursor::getCurrentRecord( dmsRecordData &data )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXCURSOR__GETCURREC ) ;

      BSONObj key ;
      rc = getCurrentKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key, rc: %d", rc ) ;

      data.setData( key.objdata(), key.objsize() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXCURSOR__GETCURREC, rc ) ;
      return rc ;

   error:
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_EOC ;
      }
      goto done ;
   }

   /*
      _dmsWTIndexAsyncCursor implement
    */
   _dmsWTIndexAsyncCursor::_dmsWTIndexAsyncCursor()
   : _dmsWTIndexCursor( _asyncSession )
   {
   }

   _dmsWTIndexAsyncCursor::~_dmsWTIndexAsyncCursor()
   {
      // should close cursor before close session
      _cursor.close() ;
      _asyncSession.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXASYNCCURSOR_OPEN, "_dmsWTIndexAsyncCursor::open" )
   INT32 _dmsWTIndexAsyncCursor::open( shared_ptr<IIndex> idxPtr,
                                       const keyString &startKey,
                                       BOOLEAN isAfterStartKey,
                                       BOOLEAN isForward,
                                       UINT64 snapshotID,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXASYNCCURSOR_OPEN ) ;

      if ( !_asyncSession.isOpened() )
      {
         dmsWTIndex *wtIndex = dynamic_cast<dmsWTIndex *>( idxPtr.get() ) ;
         PD_CHECK( wtIndex, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, index is not WiredTiger index" ) ;
         rc = wtIndex->getEngine().openSession( _asyncSession,
                                                dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

      rc = _dmsWTIndexCursor::open( idxPtr, startKey, isAfterStartKey,
                                    isForward, snapshotID, executor ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXASYNCCURSOR_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTIDXASYNCCURSOR_OPEN_SAMPLE, "_dmsWTIndexAsyncCursor::open" )
   INT32 _dmsWTIndexAsyncCursor::open( shared_ptr<IIndex> idxPtr,
                                       UINT64 sampleNum,
                                       UINT64 snapshotID,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTIDXASYNCCURSOR_OPEN_SAMPLE ) ;

      if ( !_asyncSession.isOpened() )
      {
         dmsWTIndex *wtIndex = dynamic_cast<dmsWTIndex *>( idxPtr.get() ) ;
         PD_CHECK( wtIndex, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, index is not WiredTiger index" ) ;
         rc = wtIndex->getEngine().openSession( _asyncSession,
                                                dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

      rc = _dmsWTIndexCursor::open( idxPtr, sampleNum, snapshotID, executor ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTIDXASYNCCURSOR_OPEN_SAMPLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
