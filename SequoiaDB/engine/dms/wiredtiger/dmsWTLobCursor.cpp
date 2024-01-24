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
          01/12/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTLobCursor.hpp"
#include "ossEndian.hpp"
#include "wiredtiger/dmsWTLob.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include "dmsLobDef.hpp"

using namespace std;

namespace engine
{
namespace wiredtiger
{
   constexpr UINT32 LOB_KEY_SIZE = sizeof( OID ) + sizeof( UINT32 );
   using LobRecordKey = std::array< CHAR, LOB_KEY_SIZE >;
   extern LobRecordKey makeLobRecordKey( const OID &oid, UINT32 sequence );
   extern LobRecordKey makeLobRecordKey( const dmsLobRecord &record );
   extern std::pair< OID, UINT32 > extractLobRecordKey( const UINT8 *data, UINT32 size );

   _dmsWTLobCursor::_dmsWTLobCursor( dmsWTSession &session ) : _dmsWTCursorHolder( session ) {}

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBCURSOR_OPEN, "_dmsWTLobCursor::open" )
   INT32 _dmsWTLobCursor::open( std::shared_ptr< ILob > lobPtr,
                                const dmsLobRecord &startKey,
                                BOOLEAN isAfterStartKey,
                                BOOLEAN isForward,
                                UINT64 snapshotID,
                                IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBCURSOR_OPEN );

      dmsWTLob *wtCollection = dynamic_cast< dmsWTLob * >( lobPtr.get() );
      LobRecordKey keyData = makeLobRecordKey( startKey );
      dmsWTItem key( keyData.data(), keyData.size() );

      PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                "Failed to open cursor, collection is not WiredTiger collection" );

      _lobPtr = std::move( lobPtr );
      _resetCache();

      rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(), "", key,
                  isAfterStartKey, isForward, snapshotID, executor );
      if ( SDB_DMS_EOC == rc )
      {
         goto error;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBCURSOR_OPEN, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBCURSOR_OPEN_SAMPLE, "_dmsWTLobCursor::open" )
   INT32 _dmsWTLobCursor::open( std::shared_ptr< ILob > lobPtr,
                                UINT64 sampleNum,
                                UINT64 snapshotID,
                                IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBCURSOR_OPEN_SAMPLE );

      dmsWTLob *wtLob = dynamic_cast< dmsWTLob * >( lobPtr.get() );

      PD_CHECK( wtLob, SDB_SYS, error, PDERROR,
                "Failed to open cursor, lob storage is not WiredTiger" );

      _lobPtr = std::move( lobPtr );
      _resetCache();

      rc = _open( wtLob->getEngine(), wtLob->getStore().getURI(), "", sampleNum, snapshotID,
                  executor );
      if ( SDB_DMS_EOC == rc )
      {
         goto error;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBCURSOR_OPEN_SAMPLE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBCURSOR_LOCATE, "_dmsWTLobCursor::locate" )
   INT32 _dmsWTLobCursor::locate( const dmsLobRecord &record,
                                  BOOLEAN isAfterStartKey,
                                  IExecutor *executor,
                                  BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBCURSOR_LOCATE );

      LobRecordKey keyData = makeLobRecordKey( record );
      dmsWTItem key( keyData.data(), keyData.size() );

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current lob record, cursor is not opened" );

      resetSnapshotID( _lobPtr->fetchSnapshotID() );
      _resetCache();
      if ( _isForward )
      {
         rc = _cursor.searchNext( key, isAfterStartKey, isFound );
      }
      else
      {
         rc = _cursor.searchPrev( key, isAfterStartKey, isFound );
      }
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to locate cursor, rc: %d", rc );
         }
         goto error;
      }
      _isEOF = FALSE;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBCURSOR_LOCATE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBCURSOR_PAUSE, "_dmsWTLobCursor::pause" )
   INT32 _dmsWTLobCursor::pause( IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBCURSOR_PAUSE );

      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to pause cursor, cursor is not opened" );

      _resetCache();
      rc = _cursor.pause();
      PD_RC_CHECK( rc, PDERROR, "Failed to pause cursor, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBCURSOR_PAUSE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBCURSOR_GETCURREC, "_dmsWTLobCursor::getCurrentLobRecord" )
   INT32 _dmsWTLobCursor::getCurrentLobRecord( dmsLobInfoOnPage &info, const CHAR **data )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBCURSOR_GETCURREC );

      SDB_ASSERT( data, "Can not be nullptr" );

      dmsWTItem key;
      dmsWTItem value;

      PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                "Failed to get current record ID, cursor is hit end" );
      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" );

      rc = _cursor.getKey( key );
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc );

      {
         std::pair< OID, UINT32 > ret =
            extractLobRecordKey( (UINT8 *)key.getData(), key.getSize() );

         rc = _cursor.getValue( value );
         PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc );

         info._oid = ret.first;
         info._sequence = ret.second;
         info._len = value.getSize();
         *data = (const CHAR *)value.getData();
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBCURSOR_GETCURREC, rc );
      return rc;

   error:
      goto done;
   }

   /*
      _dmsWTDataAsyncCursor implement
    */
   _dmsWTLobAsyncCursor::_dmsWTLobAsyncCursor() : _dmsWTLobCursor( _asyncSession ) {}

   _dmsWTLobAsyncCursor::~_dmsWTLobAsyncCursor()
   {
      // should close cursor before close session
      _cursor.close();
      _asyncSession.close();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBASYNCCURSOR_OPEN, "_dmsWTLobAsyncCursor::open" )
   INT32 _dmsWTLobAsyncCursor::open( std::shared_ptr< ILob > lobPtr,
                                     const dmsLobRecord &startKey,
                                     BOOLEAN isAfterStartKey,
                                     BOOLEAN isForward,
                                     UINT64 snapshotID,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBASYNCCURSOR_OPEN );

      if ( !_asyncSession.isOpened() )
      {
         dmsWTLob *wtLob = dynamic_cast< dmsWTLob * >( lobPtr.get() );
         PD_CHECK( wtLob, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, collection is not WiredTiger collection" );
         rc = wtLob->getEngine().openSession( _asyncSession, dmsWTSessIsolation::SNAPSHOT );
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc );
      }

      rc = _dmsWTLobCursor::open( lobPtr, startKey, isAfterStartKey, isForward, snapshotID,
                                  executor );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBASYNCCURSOR_OPEN, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOBASYNCCURSOR_OPEN_SAMPLE, "_dmsWTLobAsyncCursor::open" )
   INT32 _dmsWTLobAsyncCursor::open( std::shared_ptr< ILob > lobPtr,
                                     UINT64 sampleNum,
                                     UINT64 snapshotID,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOBASYNCCURSOR_OPEN_SAMPLE );

      if ( !_asyncSession.isOpened() )
      {
         dmsWTLob *wtLob = dynamic_cast< dmsWTLob * >( lobPtr.get() );
         PD_CHECK( wtLob, SDB_SYS, error, PDERROR,
                   "Failed to open cursor, collection is not WiredTiger collection" );
         rc = wtLob->getEngine().openSession( _asyncSession, dmsWTSessIsolation::SNAPSHOT );
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc );
      }

      rc = _dmsWTLobCursor::open( lobPtr, sampleNum, snapshotID, executor );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOBASYNCCURSOR_OPEN_SAMPLE, rc );
      return rc;

   error:
      goto done;
   }
} // namespace wiredtiger
} // namespace engine