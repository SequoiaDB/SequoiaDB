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

   Source File Name = dmsWTDataCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTCollection.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

namespace
{
   static dmsRecordID s_minRID = dmsRecordID::minRID() ;
   static dmsRecordID s_maxRID = dmsRecordID::maxRID() ;
}

   /*
      _dmsWTDataCursor implement
    */
   _dmsWTDataCursor::_dmsWTDataCursor( dmsWTSession &session )
   : _dmsWTCursorHolder( session )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_OPEN, "_dmsWTDataCursor::open" )
   INT32 _dmsWTDataCursor::open( shared_ptr<ICollection> collPtr,
                                 const dmsRecordID &startRID,
                                 BOOLEAN isAfterStartRID,
                                 BOOLEAN isForward,
                                 UINT64 snapshotID,
                                 IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_OPEN ) ;

      dmsWTCollection *wtCollection = dynamic_cast<dmsWTCollection *>( collPtr.get() ) ;
      UINT64 key = startRID.toUINT64() ;

      PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                "Failed to open cursor, collection is not WiredTiger collection" ) ;

      _collPtr = std::move( collPtr ) ;
      _resetCache() ;
      if ( startRID.isValid() )
      {
         // fix record ID key here, avoid move call inside WiredTiger
         if ( isAfterStartRID )
         {
            if ( isForward )
            {
               if ( s_maxRID == startRID )
               {
                  rc = SDB_DMS_EOC ;
                  _isEOF = TRUE ;
                  goto error ;
               }
               ++ key ;
            }
            else if ( !isForward )
            {
               if ( s_minRID == startRID )
               {
                  rc = SDB_DMS_EOC ;
                  _isEOF = TRUE ;
                  goto error ;
               }
               -- key ;
            }
         }

         rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(),
                     "", key, FALSE, isForward, snapshotID, executor ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }
      else
      {
         rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(),
                     "", isForward, snapshotID, executor ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_OPEN_SAMPLE, "_dmsWTDataCursor::open" )
   INT32 _dmsWTDataCursor::open( shared_ptr<ICollection> collPtr,
                                 UINT64 sampleNum,
                                 UINT64 snapshotID,
                                 IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_OPEN_SAMPLE ) ;

      dmsWTCollection *wtCollection = dynamic_cast<dmsWTCollection *>( collPtr.get() ) ;

      PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                "Failed to open cursor, collection is not WiredTiger collection" ) ;

      _collPtr = std::move( collPtr ) ;
      _resetCache() ;

      rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(),
                  "", sampleNum, snapshotID, executor ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_OPEN_SAMPLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_LOCATE, "_dmsWTDataCursor::locate" )
   INT32 _dmsWTDataCursor::locate( const dmsRecordID &rid,
                                   BOOLEAN isAfterRID,
                                   IExecutor *executor,
                                   BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_LOCATE ) ;

      UINT64 key = rid.toUINT64() ;

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current key string, cursor is not opened" ) ;

      resetSnapshotID( _collPtr->fetchSnapshotID() ) ;
      _resetCache() ;
      if ( _isForward )
      {
         rc = _cursor.searchNext( key, isAfterRID, isFound ) ;
      }
      else
      {
         rc = _cursor.searchPrev( key, isAfterRID, isFound ) ;
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to locate cursor, rc: %d", rc ) ;
         }
         goto error ;
      }
      _isEOF = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_LOCATE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_PAUSE, "_dmsWTDataCursor::pause" )
   INT32 _dmsWTDataCursor::pause( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_PAUSE ) ;

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to pause cursor, cursor is not opened" ) ;

      _resetCache() ;
      rc = _cursor.pause() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_PAUSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_GETCURRECID, "_dmsWTDataCursor::getCurrentRecordID" )
   INT32 _dmsWTDataCursor::getCurrentRecordID( dmsRecordID &recordID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_GETCURRECID ) ;

      if ( _recordIDCache.isValid() )
      {
         recordID = _recordIDCache ;
      }
      else
      {
         UINT64 key = 0 ;

         PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                   "Failed to get current record ID, cursor is hit end" ) ;
         PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                   "Failed to get current record ID, cursor is not opened" ) ;

         rc = _cursor.getKey( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

         _recordIDCache.fromUINT64( key ) ;
         recordID = _recordIDCache ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_GETCURRECID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_GETCURREC, "_dmsWTDataCursor::getCurrentRecord" )
   INT32 _dmsWTDataCursor::getCurrentRecord( dmsRecordData &data )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_GETCURREC ) ;

      dmsWTItem value ;

      PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                "Failed to get current record ID, cursor is hit end" ) ;
      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;

      rc = _cursor.getValue( value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

      data.setData( (const CHAR *)( value.get()->data ), (UINT32)( value.get()->size ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _dmsWTDataAsyncCursor implement
    */
   _dmsWTDataAsyncCursor::_dmsWTDataAsyncCursor()
   : _dmsWTDataCursor( _asyncSession )
   {
   }

   _dmsWTDataAsyncCursor::~_dmsWTDataAsyncCursor()
   {
      // should close cursor before close session
      _cursor.close() ;
      _asyncSession.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATAASYNCCURSOR_OPEN, "_dmsWTDataAsyncCursor::open" )
   INT32 _dmsWTDataAsyncCursor::open( shared_ptr<ICollection> collPtr,
                                      const dmsRecordID &startRID,
                                      BOOLEAN isAfterStartRID,
                                      BOOLEAN isForward,
                                      UINT64 snapshotID,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATAASYNCCURSOR_OPEN ) ;

      if ( !_asyncSession.isOpened() )
      {
         dmsWTCollection *wtCollection = dynamic_cast<dmsWTCollection *>( collPtr.get() ) ;
         PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, collection is not WiredTiger collection" ) ;
         rc = wtCollection->getEngine().openSession( _asyncSession,
                                                     dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

      rc = _dmsWTDataCursor::open( collPtr, startRID, isAfterStartRID,
                                   isForward, snapshotID, executor ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATAASYNCCURSOR_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATAASYNCCURSOR_OPEN_SAMPLE, "_dmsWTDataAsyncCursor::open" )
   INT32 _dmsWTDataAsyncCursor::open( shared_ptr<ICollection> collPtr,
                                      UINT64 sampleNum,
                                      UINT64 snapshotID,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATAASYNCCURSOR_OPEN_SAMPLE ) ;

      if ( !_asyncSession.isOpened() )
      {
         dmsWTCollection *wtCollection = dynamic_cast<dmsWTCollection *>( collPtr.get() ) ;
         PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, collection is not WiredTiger collection" ) ;
         rc = wtCollection->getEngine().openSession( _asyncSession,
                                                     dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

      rc = _dmsWTDataCursor::open( collPtr, sampleNum, snapshotID, executor ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATAASYNCCURSOR_OPEN_SAMPLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
