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
          01/08/2024  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTLob.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTLobCursor.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageDataCommon.hpp"
#include "ossEndian.hpp"
#include "ossUtil.h"
#include "pdTrace.hpp"

namespace engine
{
namespace wiredtiger
{
   constexpr INT32 MIN_LENGTH_FOR_DIFF = 1024;
   constexpr INT32 MAX_ENTRIES = 16;

   constexpr UINT32 LOB_KEY_SIZE = sizeof( OID ) + sizeof( UINT32 );
   using LobRecordKey = std::array< CHAR, LOB_KEY_SIZE >;
   LobRecordKey makeLobRecordKey( const OID &oid, UINT32 sequence )
   {
      LobRecordKey key;
      ossMemcpy( key.data(), oid.getData(), sizeof( OID ) );
      UINT32 sequenceBig = ossNativeToBigEndian( sequence );
      ossMemcpy( key.data() + sizeof( OID ), &sequenceBig, sizeof( UINT32 ) );
      return key;
   }
   LobRecordKey makeLobRecordKey( const dmsLobRecord &record )
   {
      return makeLobRecordKey( *record._oid, record._sequence );
   }

   std::pair< OID, UINT32 > extractLobRecordKey( const UINT8 *data, UINT32 size )
   {
      SDB_ASSERT( size == LOB_KEY_SIZE, "must be equal" );
      std::pair< OID, UINT32 > ret;
      ret.first.init( data, sizeof( OID ) );
      ret.second = ossBigEndianToNative( *(UINT32 *)( data + sizeof( OID ) ) );
      return ret;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_BLDLOBURI, "_dmsWTLob::buildLobURI")
   INT32 _dmsWTLob::buildLobURI( utilCSUniqueID csUID,
                                 utilCLInnerID clInnerID,
                                 UINT32 clLID,
                                 ossPoolString &uri )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOB_BLDLOBURI );

      try
      {
         ossPoolStringStream ss;

         ss << "table:";
         dmsWTBuildLobIdent( csUID, clInnerID, clLID, ss );
         uri = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR,
                 "Failed to build index URI, "
                 "occur exception: %s",
                 e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_BLDLOBURI, rc );
      return rc;

   error:
      goto done;
   }

   ICollection *_dmsWTLob::getCollPtr()
   {
      return _collPtr;
   }

   UINT64 _dmsWTLob::fetchSnapshotID()
   {
      return _collPtr->fetchSnapshotID();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_WRITE, "_dmsWTLob::write" )
   INT32 _dmsWTLob::write( const dmsLobRecord &record, IExecutor *executor )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__DMSWTLOB_WRITE );

      LobRecordKey key = makeLobRecordKey( record );
      CHAR *buf = nullptr;
      UINT32 len = record._offset + record._dataLen;
      std::unique_ptr< dmsWTItem > pValue;
      if ( record._offset > 0 )
      {
         rc = executor->allocBuff( len, &buf );
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate buffer[%u] , rc: %d", len, rc );
         ossMemset( buf, 0, record._offset );
         ossMemcpy( buf + record._offset, record._data, record._dataLen );
         pValue.reset( new dmsWTItem( buf, len ) );
      }
      else
      {
         pValue.reset( new dmsWTItem( record._data, len ) );
      }

      {
         dmsWTItem &value = *pValue;

         dmsWTSessionHolder sessionHolder;
         rc = _engine.getPersistSession( executor, sessionHolder );
         PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

         dmsWTCursor cursor( sessionHolder.getSession() );
         rc = cursor.open( _store.getURI(), "overwrite=false" );
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

         rc = cursor.insert( dmsWTItem( key.data(), key.size() ), value );
         PD_RC_CHECK( rc, PDERROR, "Failed to insert key to store, rc: %d", rc );
      }

   done:
      if ( buf )
      {
         executor->releaseBuff( buf );
      }
      PD_TRACE_EXITRC( SDB__DMSWTLOB_WRITE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_UPDATE, "_dmsWTLob::update" )
   INT32 _dmsWTLob::update( const dmsLobRecord &record, IExecutor *executor, updatedInfo *info )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__DMSWTLOB_UPDATE );

      const INT32 maxDiffBytes = record._dataLen / 10;

      LobRecordKey keyData = makeLobRecordKey( record );
      WT_ITEM key{ keyData.data(), keyData.size() };
      CHAR *buf = nullptr;

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      {
         WT_SESSION *session = sessionHolder.getSession().getSession();
         dmsWTCursor cursor( sessionHolder.getSession() );

         rc = cursor.open( _store.getURI(), "overwrite=false" );
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

         WT_CURSOR *c = cursor.getCursor();

         c->set_key( c, &key );

         INT32 ret = c->search( c );
         if ( 0 == ret )
         {
            dmsWTItem oldValue;
            rc = WT_CALL( c->get_value( c, oldValue.get() ), session );
            PD_RC_CHECK( rc, PDERROR, "Failed to get old lob data, rc: %d", rc );

            UINT32 len = std::max( oldValue.getSize(), record._offset + record._dataLen );
            rc = executor->allocBuff( len, &buf );
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate buffer[%u] , rc: %d", len, rc );

            ossMemcpy( buf, oldValue.getData(), std::min( oldValue.getSize(), record._offset ) );
            if ( record._offset > oldValue.getSize() )
            {
               ossMemset( buf + oldValue.getSize(), 0, record._offset - oldValue.getSize() );
            }
            ossMemcpy( buf + record._offset, record._data, record._dataLen );
            if( record._offset + record._dataLen < oldValue.getSize() )
            {
               ossMemcpy( buf + record._offset + record._dataLen,
                          (const CHAR *)oldValue.getData() + record._offset + record._dataLen,
                          oldValue.getSize() - record._offset - record._dataLen );
            }

            dmsWTItem value( buf, len );
            BOOLEAN skipUpdate = FALSE;
            if ( len > MIN_LENGTH_FOR_DIFF && len <= oldValue.getSize() + maxDiffBytes )
            {
               INT32 nentries = MAX_ENTRIES;
               std::vector< WT_MODIFY > entries( nentries );

               INT32 ret = wiredtiger_calc_modify( session, oldValue.get(), value.get(),
                                                   maxDiffBytes, entries.data(), &nentries );
               if ( ret == 0 )
               {
                  rc = WT_CALL( nentries == 0 ? c->reserve( c )
                                              : c->modify( c, entries.data(), nentries ),
                                session );
                  PD_RC_CHECK( rc, PDERROR, "Failed to modify lob value, rc: %d", rc );
                  skipUpdate = TRUE;
               }
            }
            if ( !skipUpdate )
            {
               c->set_value( c, value.get() );
               rc = WT_CALL( c->update( c ), session );
               PD_RC_CHECK( rc, PDERROR, "Failed to update lob value, rc: %d", rc );
            }
            if ( info )
            {
               info->hasUpdated = true;
               info->increasedSize = static_cast< INT32 >( value.getSize() - oldValue.getSize() );
            }
         }
         else
         {
            rc = dmsWTRCToDBRCSlow( ret, session, false );
            PD_RC_CHECK( rc, PDERROR, "Failed to search key, rc: %d", rc );
         }
      }

   done:
      if ( buf )
      {
         executor->releaseBuff( buf );
      }
      PD_TRACE_EXITRC( SDB__DMSWTLOB_UPDATE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_WRITEORUPDATE, "_dmsWTLob::writeOrUpdate" )
   INT32 _dmsWTLob::writeOrUpdate( const dmsLobRecord &record,
                                   IExecutor *executor,
                                   updatedInfo *info )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__DMSWTLOB_WRITEORUPDATE );

      const INT32 maxDiffBytes = record._dataLen / 10;

      LobRecordKey keyData = makeLobRecordKey( record );
      WT_ITEM key{ keyData.data(), keyData.size() };
      CHAR *buf = nullptr;

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      {
         WT_SESSION *session = sessionHolder.getSession().getSession();
         dmsWTCursor cursor( sessionHolder.getSession() );

         rc = cursor.open( _store.getURI(), "overwrite=false" );
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

         WT_CURSOR *c = cursor.getCursor();

         c->set_key( c, &key );

         INT32 ret = c->search( c );
         if ( WT_NOTFOUND == ret )
         {
            UINT32 len = record._offset + record._dataLen;
            if ( record._offset > 0 )
            {
               rc = executor->allocBuff( len, &buf );
               PD_RC_CHECK( rc, PDERROR, "Failed to allocate buffer[%u] , rc: %d", len, rc );
               ossMemset( buf, 0, record._offset );
               ossMemcpy( buf + record._offset, record._data, record._dataLen );
               dmsWTItem value( buf, len );
               c->set_value( c, value.get() );
            }
            else
            {
               dmsWTItem value( record._data, len );
               c->set_value( c, value.get() );
            }

            rc = WT_CALL( c->insert( c ), session );
            PD_RC_CHECK( rc, PDERROR, "Failed to insert, rc: %d", rc );
         }
         else if ( 0 == ret )
         {
            dmsWTItem oldValue;
            rc = WT_CALL( c->get_value( c, oldValue.get() ), session );
            PD_RC_CHECK( rc, PDERROR, "Failed to get old lob data, rc: %d", rc );

            UINT32 len = std::max( oldValue.getSize(), record._offset + record._dataLen );
            rc = executor->allocBuff( len, &buf );
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate buffer[%u] , rc: %d", len, rc );

            ossMemcpy( buf, oldValue.getData(), std::min( oldValue.getSize(), record._offset ) );
            if ( record._offset > oldValue.getSize() )
            {
               ossMemset( buf + oldValue.getSize(), 0, record._offset - oldValue.getSize() );
            }
            ossMemcpy( buf + record._offset, record._data, record._dataLen );
            if( record._offset + record._dataLen < oldValue.getSize() )
            {
               ossMemcpy( buf + record._offset + record._dataLen,
                          (const CHAR *)oldValue.getData() + record._offset + record._dataLen,
                          oldValue.getSize() - record._offset - record._dataLen );
            }

            dmsWTItem value( buf, len );
            BOOLEAN skipUpdate = FALSE;
            if ( len > MIN_LENGTH_FOR_DIFF && len <= oldValue.getSize() + maxDiffBytes )
            {
               INT32 nentries = MAX_ENTRIES;
               std::vector< WT_MODIFY > entries( nentries );

               INT32 ret = wiredtiger_calc_modify( session, oldValue.get(), value.get(),
                                                   maxDiffBytes, entries.data(), &nentries );
               if ( ret == 0 )
               {
                  rc = WT_CALL( nentries == 0 ? c->reserve( c )
                                              : c->modify( c, entries.data(), nentries ),
                                session );
                  PD_RC_CHECK( rc, PDERROR, "Failed to modify lob value, rc: %d", rc );
                  skipUpdate = TRUE;
               }
            }
            if ( !skipUpdate )
            {
               c->set_value( c, value.get() );
               rc = WT_CALL( c->update( c ), session );
               PD_RC_CHECK( rc, PDERROR, "Failed to update lob value, rc: %d", rc );
            }
            if ( info )
            {
               info->hasUpdated = true;
               info->increasedSize = static_cast< INT32 >( value.getSize() - oldValue.getSize() );
            }
         }
         else
         {
            rc = dmsWTRCToDBRCSlow( ret, session, false );
            PD_RC_CHECK( rc, PDERROR, "Failed to search key, rc: %d", rc );
         }
      }

   done:
      if ( buf )
      {
         executor->releaseBuff( buf );
      }
      PD_TRACE_EXITRC( SDB__DMSWTLOB_WRITEORUPDATE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_REMOVE, "_dmsWTLob::remove" )
   INT32 _dmsWTLob::remove( const dmsLobRecord &record, IExecutor *executor )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__DMSWTLOB_REMOVE );

      LobRecordKey key = makeLobRecordKey( record );

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      {
         dmsWTCursor cursor( sessionHolder.getSession() );

         rc = cursor.open( _store.getURI(), "" );
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

         rc = cursor.remove( dmsWTItem( key.data(), key.size() ) );
         PD_RC_CHECK( rc, PDERROR, "Failed to remove key from store, rc: %d", rc );
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_REMOVE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_READ, "_dmsWTLob::read" )
   INT32 _dmsWTLob::read( const dmsLobRecord &record,
                          IExecutor *executor,
                          void *buf,
                          UINT32 &readLen ) const
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__DMSWTLOB_READ );

      LobRecordKey key = makeLobRecordKey( record );

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      {
         dmsWTCursor cursor( sessionHolder.getSession() );
         dmsWTItem value;

         rc = cursor.open( _store.getURI(), "" );
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc );

         rc = cursor.searchAndGetValue( dmsWTItem( key.data(), key.size() ), value );
         PD_RC_CHECK( rc, PDERROR, "Failed to search key from store, rc: %d", rc );

         UINT32 inputLen = readLen;

         if ( value.getSize() <= record._offset )
         {
            readLen = 0;
         }
         else
         {
            if ( inputLen == 0 )
            {
               readLen = std::min( value.getSize() - record._offset, record._dataLen );
            }
            else
            {
               readLen =
                  std::min( { value.getSize() - record._offset, record._dataLen, inputLen } );
            }
            ossMemcpy( buf, (CHAR *)value.getData() + record._offset, readLen );
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_READ, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_TRUNCATE, "_dmsWTLob::truncate" )
   INT32 _dmsWTLob::truncate( IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOB_TRUNCATE );

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      rc = _engine.truncateStore( sessionHolder.getSession(), _store.getURI().c_str(), nullptr );
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate data store, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_TRUNCATE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_COMPACT, "_dmsWTLob::compact" )
   INT32 _dmsWTLob::compact( IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOB_COMPACT );

      dmsWTSessionHolder sessionHolder;
      rc = _engine.getPersistSession( executor, sessionHolder );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc );

      rc = _engine.compactStore( sessionHolder.getSession(), _store.getURI().c_str(), nullptr );
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate index store, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_COMPACT, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_VALIDATE, "_dmsWTLob::validate" )
   INT32 _dmsWTLob::validate( IExecutor *executor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOB_VALIDATE );

      UINT64 lobCount = 0, lobPieceCount = 0, totalLobDataLen = 0;

      class dmsWTLobValidator : public _dmsWTStoreValidator
      {
      public:
         dmsWTLobValidator( UINT64 &lobCount, UINT64 &lobPieceCount ,UINT64 &totalLobDataLen )
         : _lobCount( lobCount ),
           _lobPieceCount( lobPieceCount ),
           _totalLobDataLen(totalLobDataLen)
         {
         }

         virtual ~dmsWTLobValidator() = default ;

         virtual INT32 validate( const dmsWTItem &keyItem, const dmsWTItem &valueItem )
         {
            auto key = extractLobRecordKey( (UINT8 *)keyItem.getData(), keyItem.getSize() );
            if( key.second == DMS_LOB_META_SEQUENCE )
            {
               ++_lobCount;
               INT64 lobPieceLen = ( DMS_LOB_META_LENGTH >= valueItem.getSize()
                                        ? 0
                                        : ( valueItem.getSize() - DMS_LOB_META_LENGTH ) );
               _totalLobDataLen += lobPieceLen;
            }
            else
            {
               _totalLobDataLen += valueItem.getSize();
            }
            ++_lobPieceCount ;
            return SDB_OK ;
         }

      protected:
         UINT64 &_lobCount ;
         UINT64 &_lobPieceCount ;
         UINT64 &_totalLobDataLen ;
      } ;

      dmsWTLobValidator validator( lobCount, lobPieceCount, totalLobDataLen ) ;

      // validate store
      rc = _validateStore( validator, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to validate store, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Reset total lob count [%llu], lob pages count[%llu], lob size [%llu], ",
              lobCount, lobPieceCount, totalLobDataLen );

      {
         _dmsMBStatInfo *stat = getCollPtr()->getMetadata().getMBStat();
         stat->_totalLobs.poke( lobCount );
         stat->_totalLobPages.poke( lobPieceCount );
         stat->_totalLobSize.poke( totalLobDataLen );
         stat->_totalValidLobSize.poke( totalLobDataLen );
      }


   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_VALIDATE, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTLOB_LIST, "_dmsWTLob::list" )
   INT32 _dmsWTLob::list( IExecutor *executor, unique_ptr< ILobCursor > &cursor )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTLOB_LIST );

      IPersistUnit *persistUnit = nullptr;
      UINT64 snapshotID = 0;
      dmsLobRecord record;
      OID oid;

      rc = _engine.getService().getPersistUnit( executor, persistUnit );
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc );

      if ( persistUnit )
      {
         dmsWTPersistUnit *wtUnit = dynamic_cast< dmsWTPersistUnit * >( persistUnit );
         PD_CHECK( wtUnit, SDB_SYS, error, PDERROR,
                   "Failed to get persist unit, it is not a WiredTiger persist unit" );

         cursor = unique_ptr< dmsWTLobCursor >( new dmsWTLobCursor( wtUnit->getReadSession() ) );
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc );
      }
      else
      {
         cursor = unique_ptr< dmsWTLobCursor >( new dmsWTLobAsyncCursor() );
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc );
      }

      SDB_ASSERT( cursor, "cursor should be valid" );
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor" );

      snapshotID = _collPtr->fetchSnapshotID();

      record.set( &oid, DMS_LOB_META_SEQUENCE, 0, 0, nullptr );
      rc = cursor->open( shared_from_this(), record, FALSE, TRUE, snapshotID, executor );
      if ( SDB_DMS_EOC == rc )
      {
         goto error;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc );

   done:
      PD_TRACE_EXITRC( SDB__DMSWTLOB_LIST, rc );
      return rc;

   error:
      goto done;
   }
} // namespace wiredtiger
} // namespace engine