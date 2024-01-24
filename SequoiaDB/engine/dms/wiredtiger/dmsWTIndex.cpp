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

   Source File Name = dmsWTIndex.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTIndex.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTIndexCursor.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
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

namespace
{
   static const dmsWTItem s_emptyItem( nullptr, 0 ) ;
}
   /*
      _dmsWTIndex implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_TRUNC, "_dmsWTIndex::truncate" )
   INT32 _dmsWTIndex::truncate( const dmsTruncateIdxOptions &options,
                                IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_TRUNC ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      rc = _engine.truncateStore( sessionHolder.getSession(),
                                  _store.getURI().c_str(),
                                  nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate index store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_TRUNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_COMPACT, "_dmsWTIndex::compact" )
   INT32 _dmsWTIndex::compact( const dmsCompactIdxOptions &options,
                               IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_COMPACT ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      rc = _engine.compactStore( sessionHolder.getSession(),
                                 _store.getURI().c_str(),
                                 nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate index store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_COMPACT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_INDEX, "_dmsWTIndex::index" )
   INT32 _dmsWTIndex::index( const BSONObj &key,
                             const dmsRecordID &rid,
                             BOOLEAN allowDuplicated,
                             IExecutor *executor,
                             utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_INDEX ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         // for indexing, disable overwrite to check conflicts
         rc = cursor.open( _store.getURI(), "overwrite=false" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _index( cursor, key, rid, allowDuplicated, executor, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert index key, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_INDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_UNINDEX, "_dmsWTIndex::unindex" )
   INT32 _dmsWTIndex::unindex( const BSONObj &key,
                               const dmsRecordID &rid,
                               IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_UNINDEX ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _unindex( cursor, key, rid, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove index key, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_UNINDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__CREATEINDEXCURSOR, "_dmsWTIndex::_createIndexCursor" )
   INT32 _dmsWTIndex::_createIndexCursor( unique_ptr<IIndexCursor> &cursor,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__CREATEINDEXCURSOR ) ;

      IPersistUnit *persistUnit = nullptr ;

      rc = _engine.getService().getPersistUnit( executor, persistUnit ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;

      if ( persistUnit )
      {
         dmsWTPersistUnit *wtUnit = dynamic_cast<dmsWTPersistUnit *>( persistUnit ) ;
         PD_CHECK( wtUnit, SDB_SYS, error, PDERROR,
                  "Failed to get persist unit, it is not a WiredTiger persist unit" ) ;

         cursor = unique_ptr<dmsWTIndexCursor>( new dmsWTIndexCursor( wtUnit->getReadSession() ) ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create index cursor, rc: %d", rc ) ;
      }
      else
      {
         cursor = unique_ptr<dmsWTIndexCursor>( new dmsWTIndexAsyncCursor() ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create index cursor, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__CREATEINDEXCURSOR, rc ) ;
      return rc ;

   error:
      cursor.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_CREATEINDEXCURSOR, "_dmsWTIndex::createIndexCursor" )
   INT32 _dmsWTIndex::createIndexCursor( unique_ptr<IIndexCursor> &cursor,
                                         const keyString &startKey,
                                         BOOLEAN isAfterStartKey,
                                         BOOLEAN isForward,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_CREATEINDEXCURSOR ) ;

      UINT64 snapshotID = 0 ;

      rc = _createIndexCursor( cursor, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index cursor, rc: %d", rc ) ;
      SDB_ASSERT( cursor, "cursor should be valid" ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create index cursor" ) ;

      snapshotID = _metadata.getMBStat()->_snapshotID.fetch() ;
      rc = cursor->open( shared_from_this(),
                         startKey,
                         isAfterStartKey,
                         isForward,
                         snapshotID,
                         executor ) ;
      if ( SDB_IXM_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open index cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_CREATEINDEXCURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_CREATEINDEXSAMPLECURSOR, "_dmsWTIndex::createIndexSampleCursor" )
   INT32 _dmsWTIndex::createIndexSampleCursor( unique_ptr<IIndexCursor> &cursor,
                                               UINT64 sampleNum,
                                               IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_CREATEINDEXSAMPLECURSOR ) ;

      UINT64 snapshotID = 0 ;

      rc = _createIndexCursor( cursor, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index cursor, rc: %d", rc ) ;
      SDB_ASSERT( cursor, "cursor should be valid" ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create index cursor" ) ;

      snapshotID = _metadata.getMBStat()->_snapshotID.fetch() ;
      rc = cursor->open( shared_from_this(), sampleNum, snapshotID, executor ) ;
      if ( SDB_IXM_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open index cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_CREATEINDEXSAMPLECURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_GETINDEXSTATS, "_dmsWTIndex::getIndexStats" )
   INT32 _dmsWTIndex::getIndexStats( UINT64 &totalSize,
                                     UINT64 &freeSize,
                                     BOOLEAN isFast,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_GETINDEXSTATS ) ;

      rc = _dmsWTStoreHolder::getStoreTotalSize( totalSize, executor ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to store total size, rc: %d", rc ) ;
      totalSize = ossRoundUpToMultipleX( totalSize, _metadata.getSU()->getPageSize() ) ;

      rc = _dmsWTStoreHolder::getStoreFreeSize( freeSize, executor ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to store free size, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_GETINDEXSTATS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_BLDIDXCONFSTR, "_dmsWTIndex::buildIdxConfigString" )
   INT32 _dmsWTIndex::buildIdxConfigString( const dmsWTEngineOptions &options,
                                            const dmsCreateIdxOptions &createIdxOptions,
                                            ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_BLDIDXCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file,internal_page_max=16k,leaf_page_max=16k,";
         ss << "checksum=on,";
         ss << "prefix_compression=true,";
         ss << "key_format=u,";
         ss << "value_format=u,";
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build index config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_BLDIDXCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX_BLDIDXURI, "_dmsWTIndex::buildIdxURI" )
   INT32 _dmsWTIndex::buildIdxURI( utilCSUniqueID csUID,
                                   utilCLInnerID clInnerID,
                                   UINT32 clLID,
                                   utilIdxInnerID idxInnerID,
                                   ossPoolString &idxURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_BLDIDXURI ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "table:" ;
         dmsWTBuildIndexIdent( csUID, clInnerID, clLID, idxInnerID, ss ) ;
         idxURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build index URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_BLDIDXURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__BLDKEYSTR, "_dmsWTIndex::_buildKeyString" )
   INT32 _dmsWTIndex::_buildKeyString( const BSONObj &key,
                                       const dmsRecordID &rid,
                                       keyStringBuilderImpl &builder,
                                       BOOLEAN *isAllUndefined )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__BLDKEYSTR ) ;

      rc = builder.buildIndexEntryKey( key, _metadata.getOrdering(), rid,
                                       nullptr, nullptr, isAllUndefined ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__BLDKEYSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__GETKEY, "_dmsWTIndex::_getKey" )
   INT32 _dmsWTIndex::_getKey( const keyString &ks,
                               dmsWTItem &keyItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__GETKEY ) ;

      try
      {
         if ( _metadata.isStrictUnique() )
         {
            keyItem.init( ks.getKeySliceExceptTail() ) ;
         }
         else
         {
            keyItem.init( ks.getKeySlice() ) ;
            // record ID in key
            PD_CHECK( keyItem.getSize() >= keyStringCoder::RID_ENCODING_SIZE,
                      SDB_CORRUPTED_RECORD, error, PDERROR,
                      "Failed to initialize key and value" ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize key, occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__GETKEY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__GETKEYANDVALUE, "_dmsWTIndex::_getKeyAndValue" )
   INT32 _dmsWTIndex::_getKeyAndValue( const keyString &ks,
                                       BOOLEAN isRIDInValue,
                                       dmsWTItem &keyItem,
                                       dmsWTItem &valueItem )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__GETKEYANDVALUE ) ;

      try
      {
         if ( isRIDInValue )
         {
            keyItem.init( ks.getKeySliceExceptTail() ) ;
            valueItem.init( ks.getSliceAfterKeyElements() ) ;
            // record ID in value
            PD_CHECK( valueItem.getSize() >= keyStringCoder::RID_ENCODING_SIZE,
                      SDB_CORRUPTED_RECORD, error, PDERROR,
                      "Failed to initialize key and value" ) ;
         }
         else
         {
            keyItem.init( ks.getKeySlice() ) ;
            valueItem.init( ks.getSliceAfterKey() ) ;
            // record ID in key
            PD_CHECK( keyItem.getSize() >= keyStringCoder::RID_ENCODING_SIZE,
                      SDB_CORRUPTED_RECORD, error, PDERROR,
                      "Failed to initialize key and value" ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize key and value, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__GETKEYANDVALUE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__GETRECID, "_dmsWTIndex::_getRecordID" )
   INT32 _dmsWTIndex::_getRecordID( const dmsWTItem &value,
                                    BOOLEAN isAtEnd,
                                    dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__GETKEYANDVALUE ) ;

      keyStringCoder coder ;

      PD_CHECK( value.getSize() >= keyStringCoder::RID_ENCODING_SIZE,
                SDB_CORRUPTED_RECORD, error, PDERROR,
                "Invalid value size: %d", value.getSize() ) ;

      if ( isAtEnd )
      {
         rid = coder.decodeToRID( (const CHAR *)value.getData() +
                                  value.getSize() -
                                  keyStringCoder::RID_ENCODING_SIZE ) ;
      }
      else
      {
         rid = coder.decodeToRID( (const CHAR *)value.getData() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__GETKEYANDVALUE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__INDEX, "_dmsWTIndex::_index" )
   INT32 _dmsWTIndex::_index( dmsWTCursor &cursor,
                              const BSONObj &key,
                              const dmsRecordID &rid,
                              BOOLEAN allowDuplicated,
                              IExecutor *executor,
                              utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX_INDEX ) ;

      keyStringStackBuilder builder ;
      keyString ks ;
      BOOLEAN isAllUndefined = FALSE ;

      rc = _buildKeyString( key, rid, builder, &isAllUndefined ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;

      ks = builder.getShallowKeyString() ;

      if ( _metadata.isStrictUnique() )
      {
         rc = _insertStrictUnique( cursor, ks, rid, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;
      }
      else if ( !allowDuplicated && _metadata.isUnique() )
      {
         rc = _insertUnique( cursor, ks, rid, isAllUndefined, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;
      }
      else
      {
         rc = _insertStandard( cursor, ks, rid, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX_INDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__UNINDEX, "_dmsWTIndex::_unindex" )
   INT32 _dmsWTIndex::_unindex( dmsWTCursor &cursor,
                                const BSONObj &key,
                                const dmsRecordID &rid,
                                IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__UNINDEX ) ;

      keyStringStackBuilder builder ;
      dmsWTItem keyItem ;

      rc = _buildKeyString( key, rid, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build key string, rc: %d", rc ) ;

      rc = _getKey( builder.getShallowKeyString(), keyItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key, rc: %d", rc ) ;

      rc = cursor.remove( keyItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove index key from engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__UNINDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_KS, "_dmsWTIndex::_insertStrictUnique" )
   INT32 _dmsWTIndex::_insertStrictUnique( dmsWTCursor &cursor,
                                           const keyString &ks,
                                           const dmsRecordID &rid,
                                           utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_KS ) ;

      dmsWTItem keyItem, valueItem ;

      rc = _getKeyAndValue( ks, TRUE, keyItem, valueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key and value, rc: %d", rc ) ;

      rc = _insertStrictUnique( cursor, keyItem, valueItem, rid, result ) ;
      if ( SDB_IXM_IDENTICAL_KEY == rc ||
           SDB_IXM_DUP_KEY == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_KS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_ITEM, "_dmsWTIndex::_insertStrictUnique" )
   INT32 _dmsWTIndex::_insertStrictUnique( dmsWTCursor &cursor,
                                           const dmsWTItem &keyItem,
                                           const dmsWTItem &valueItem,
                                           const dmsRecordID &rid,
                                           utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_ITEM ) ;

      rc = cursor.insert( keyItem, valueItem ) ;
      if ( SDB_IXM_DUP_KEY == rc && WT_DUPLICATE_KEY == dmsWTGetLastErrorCode() )
      {
         dmsWTItem conflictItem ;
         dmsRecordID conflictRID ;
         INT32 tmpRC = cursor.getValue( conflictItem ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to get conflict value from cursor, "
                     "rc: %d", tmpRC ) ;
            goto error ;
         }
         tmpRC = _getRecordID( conflictItem, FALSE, conflictRID ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to get conflict record ID, rc: %d", tmpRC ) ;
            goto error ;
         }
         if ( conflictRID == rid )
         {
            rc = SDB_IXM_IDENTICAL_KEY ;
            PD_LOG( PDEVENT, "Conflict record ID is identical to current record ID" ) ;
         }
         else if ( result )
         {
            result->setCurRID( rid ) ;
            result->setPeerRID( conflictRID ) ;
            PD_LOG( PDERROR, "Failed to insert index key [extent: %u, offset: %u] "
                    "to engine, conflict with [extent: %u, offset: %u]",
                    rid._extent, rid._offset, conflictRID._extent, conflictRID._offset ) ;
         }
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__INSTSTRICTUNIQUE_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__INSTUNIQUE_KS, "_dmsWTIndex::_insertUnique" )
   INT32 _dmsWTIndex::_insertUnique( dmsWTCursor &cursor,
                                     const keyString &ks,
                                     const dmsRecordID &rid,
                                     BOOLEAN isAllUndefined,
                                     utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__INSTUNIQUE_KS ) ;

      dmsWTItem keyItem, valueItem ;

      rc = _getKeyAndValue( ks, FALSE, keyItem, valueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key and value, rc: %d", rc ) ;

      // if we find duplicate, let's check whether the key includes all
      // Undefined. If this is the case, it's a special case that user
      // doesn't define those keys, so we should allow it proceed ( which
      // may violate unique definition ). If we restricted this behavior,
      // user cannot insert records that does not contains the keys twice,
      // which is very violating "schemaless"
      if ( _metadata.isEnforced() || !isAllUndefined )
      {
         rc = _checkUnique( cursor, ks, rid, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check unqiue, rc: %d", rc ) ;
      }

      rc = cursor.insert( keyItem, valueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__INSTUNIQUE_KS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__INSTSTANDARD, "_dmsWTIndex::_insertStandard" )
   INT32 _dmsWTIndex::_insertStandard( dmsWTCursor &cursor,
                                       const keyString &ks,
                                       const dmsRecordID &rid,
                                       utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__INSTSTANDARD ) ;

      dmsWTItem keyItem, valueItem ;

      rc = _getKeyAndValue( ks, FALSE, keyItem, valueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key and value, rc: %d", rc ) ;

      rc = cursor.insert( keyItem, valueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert index key to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__INSTSTANDARD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTINDEX__CHECKUNIQ, "_dmsWTIndex::_checkUnique" )
   INT32 _dmsWTIndex::_checkUnique( dmsWTCursor &cursor,
                                    const keystring::keyString &ks,
                                    const dmsRecordID &rid,
                                    utilWriteResult *result )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTINDEX__CHECKUNIQ ) ;

      dmsWTItem prefixKeyItem, prefixValueItem, existsKeyItem, existValueItem ;
      BOOLEAN isFound = FALSE, isExactMatch = FALSE ;

      rc = _getKeyAndValue( ks, TRUE, prefixKeyItem, prefixValueItem ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get prefix key and value, rc: %d", rc ) ;

      // insert the prefix to reserve unique key during write of record,
      // so other concurrent writes with the same unique key will be conflict
      rc = _insertStrictUnique( cursor, prefixKeyItem, prefixValueItem, rid, result ) ;
      if ( SDB_IXM_IDENTICAL_KEY == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve prefix key to engine, rc: %d", rc ) ;

      // remove the prefix, so the concurrent writes with the same unique key
      // will continue conflict, but any writes after this write will not be
      // conflict
      rc = cursor.remove( prefixKeyItem ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to remove prefix key from engine, rc: %d", rc ) ;

      // search if the same key exsits
      rc = cursor.searchPrefix( prefixKeyItem,
                                existsKeyItem,
                                existValueItem,
                                isFound,
                                isExactMatch ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to search prefix key, rc: %d", rc ) ;
      if ( isFound )
      {
         dmsRecordID conflictRID ;

         if ( isExactMatch )
         {
            // found key is exactly the same with prefix, record ID in value
            rc = _getRecordID( existValueItem, FALSE, conflictRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get conflict record ID, rc: %d", rc ) ;
         }
         else
         {
            // found key has the same prefix, record ID in key
            rc = _getRecordID( existsKeyItem, TRUE, conflictRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get conflict record ID, rc: %d", rc ) ;
         }

         if ( rid == conflictRID )
         {
            rc = SDB_IXM_IDENTICAL_KEY ;
            PD_LOG( PDEVENT, "Conflict record ID is identical to current "
                    "record ID [extent: %u, offset: %u]", rid._extent, rid._offset ) ;
            goto error ;
         }
         else
         {
            if ( result )
            {
               result->setCurRID( rid ) ;
               result->setPeerRID( conflictRID ) ;
            }
            rc = pdError( SDB_IXM_DUP_KEY ) ;
            PD_LOG( PDERROR, "Failed to insert index key [extent: %u, offset: %u] "
                    "to engine, conflict with [extent: %u, offset: %u]",
                    rid._extent, rid._offset, conflictRID._extent, conflictRID._offset ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTINDEX__CHECKUNIQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
