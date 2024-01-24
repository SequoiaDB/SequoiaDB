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

   Source File Name = dmsWTCollection.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include <memory>

using namespace std ;
using namespace bson ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETLOBPTR, "_dmsWTCollection::getLobPtr" )
   INT32 _dmsWTCollection::getLobPtr( std::shared_ptr< ILob > &lob )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETLOBPTR );

      {
         ossScopedRWLock lock( &_lobMutex, SHARED );
         if ( _lob )
         {
            lob = _lob;
            goto done;
         }
      }

      {
         dmsWTStore store;
         ossPoolString uri, config;
         INT32 rc = dmsWTLob::buildLobURI( _metadata.getCSUID(), _metadata.getCLOrigInnerID(),
                                           _metadata.getCLOrigLID(), uri );

         rc = dmsWTCollection::buildLobConfigString( _engine.getService().getEngineOptions(),
                                                     config );
         PD_RC_CHECK( rc, PDERROR, "Failed to build lob config string, rc: %d", rc );

         pdLogShield shield;
         shield.addRC( SDB_DMS_EOC );
         rc = _engine.loadStore( uri.c_str(), store );
         if ( rc == SDB_DMS_EOC )
         {
            rc = _engine.createStore( uri.c_str(), config.c_str(), store );
            PD_RC_CHECK( rc, PDERROR, "Failed to create lob store, rc: %d", rc );
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to load lob store, rc: %d", rc );

         lob = make_shared< dmsWTLob >( _engine, store, this );
         PD_CHECK( lob, SDB_OOM, error, PDERROR, "Failed to create collection object" );

         ossScopedRWLock lock( &_lobMutex, EXCLUSIVE );
         _lob = lob;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETLOBPTR, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEIDX, "_dmsWTCollection::createIndex" )
   INT32 _dmsWTCollection::createIndex( const dmsIdxMetadata &metadata,
                                        const dmsCreateIdxOptions &options,
                                        IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEIDX ) ;

      dmsWTStore store ;
      ossPoolString idxURI, idxConfig ;
      shared_ptr<IIndex> idxPtr ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = dmsWTIndex::buildIdxConfigString( _engine.getOptions(), options, idxConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index config string, rc: %d", rc ) ;

      rc = _engine.createStore( idxURI.c_str(), idxConfig.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index store, rc: %d", rc ) ;

      rc = _addIndex( metadata, store, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add index, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_DROPIDX, "_dmsWTCollection::dropIndex" )
   INT32 _dmsWTCollection::dropIndex( const dmsIdxMetadata &metadata,
                                      const dmsDropIdxOptions &options,
                                      IContext *context,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_DROPIDX ) ;

      ossPoolString idxURI ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = _engine.dropStore( idxURI.c_str(), "force,checkpoint_wait=false", context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop index store, rc: %d", rc ) ;

      _removeIndex( metadata.getIdxKey() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_DROPIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_TRUNC, "_dmsWTCollection::truncate" )
   INT32 _dmsWTCollection::truncate( const dmsTruncCLOptions &options,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_TRUNC ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      rc = _engine.truncateStore( sessionHolder.getSession(),
                                  _store.getURI().c_str(),
                                  nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate data store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_TRUNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_COMPACT, "_dmsWTCollection::compact" )
   INT32 _dmsWTCollection::compact( const dmsCompactCLOptions &options,
                                    IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_COMPACT ) ;

      std::shared_ptr<IIndex> idxPtr ;
      std::shared_ptr<ILob> lobPtr ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get persist session, rc: %d", rc ) ;

      rc = _engine.compactStore( sessionHolder.getSession(),
                                 _store.getURI().c_str(),
                                 nullptr ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to compact data store, rc: %d", rc ) ;

      while ( ( idxPtr = _getNextIndex( idxPtr ) ) )
      {
         rc = idxPtr->compact( options, executor ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to compact index, rc: %d", rc ) ;
      }

      {
         ossScopedRWLock lock( &_lobMutex, SHARED );
         lobPtr = _lob;
      }
      if ( lobPtr )
      {
         rc = lobPtr->compact( executor );
         PD_RC_CHECK( rc, PDWARNING, "Failed to compact lob, rc: %d", rc );
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_COMPACT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETIDX, "_dmsWTCollection::getIndex" )
   INT32 _dmsWTCollection::getIndex( const dmsIdxMetadataKey &metadataKey,
                                     IExecutor *executor,
                                     shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETIDX ) ;

      idxPtr = _getIndex( metadataKey ) ;
      PD_CHECK( idxPtr, SDB_IXM_NOTEXIST, error, PDDEBUG,
                "Failed to get index, collection [UID: %llx, LID: %x] "
                "index [UID: %x] not exist", metadataKey.getCLOrigUID(),
                metadataKey.getCLOrigLID(), metadataKey.getIdxInnerID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_LOADIDX, "_dmsWTCollection::loadIndex" )
   INT32 _dmsWTCollection::loadIndex( const dmsIdxMetadata &metadata,
                                      IExecutor *executor,
                                      shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_LOADIDX ) ;

      dmsWTStore store ;
      ossPoolString idxURI ;

      idxPtr.reset() ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = _engine.loadStore( idxURI.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index store [%s], rc: %d",
                   idxURI.c_str(), rc ) ;

      rc = _addIndex( metadata, store, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add index, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_LOADIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_INSERTREC, "_dmsWTCollection::insertRecord" )
   INT32 _dmsWTCollection::insertRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_INSERTREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.insert( key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert key to store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_INSERTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_UPDATEREC, "_dmsWTCollection::updateRecord" )
   INT32 _dmsWTCollection::updateRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_UPDATEREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.update( key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update key to store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_UPDATEREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_RMREC, "_dmsWTCollection::removeRecord" )
   INT32 _dmsWTCollection::removeRecord( const dmsRecordID &rid,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_RMREC ) ;

      UINT64 key = rid.toUINT64() ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.remove( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove key from store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_RMREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_EXTRACTREC, "_dmsWTCollection::extractRecord" )
   INT32 _dmsWTCollection::extractRecord( const dmsRecordID &rid,
                                          dmsRecordData &recordData,
                                          BOOLEAN needGetOwned,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_EXTRACTREC ) ;

      UINT64 key = rid.toUINT64() ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getReadSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get read session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;
         dmsWTItem value ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.searchAndGetValue( key, value ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to search key from store, rc: %d", rc ) ;

         recordData.setData( (const CHAR *)( value.getData() ), value.getSize() ) ;
         if ( needGetOwned )
         {
            rc = recordData.getOwned() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get owned record data, rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_EXTRACTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_POPREC, "_dmsWTCollection::popRecords" )
   INT32 _dmsWTCollection::popRecords( const dmsRecordID &rid,
                                       INT32 direction,
                                       IExecutor *executor,
                                       UINT64 &popCount,
                                       UINT64 &popSize )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_POPREC ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         BOOLEAN isFound = FALSE ;

         dmsWTSession &session = sessionHolder.getSession() ;
         dmsWTCursor cursor( session ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         if ( direction > 0 )
         {
            rc = cursor.searchPrev( rid.toUINT64(), FALSE, isFound ) ;
         }
         else
         {
            rc = cursor.searchNext( rid.toUINT64(), FALSE, isFound ) ;
         }
         if ( SDB_DMS_EOC == rc )
         {
            isFound = FALSE ;
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         else if ( SDB_OK == rc && !isFound )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to search key from store, rc: %d", rc ) ;

         while ( TRUE )
         {
            dmsWTItem valueItem ;

            if ( executor->isInterrupted() )
            {
               PD_LOG( PDERROR, "Failed to pop record, executor is interrupted" ) ;
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }

            rc = cursor.getValue( valueItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

            ++ popCount ;
            popSize += valueItem.getSize() ;

            if ( direction > 0 )
            {
               rc = cursor.prev() ;
            }
            else
            {
               rc = cursor.next() ;
            }
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to move cursor, rc: %d", rc ) ;
         }

         // do truncate
         if ( direction > 0 )
         {
            rc = cursor.searchPrev( rid.toUINT64(), FALSE, isFound ) ;
         }
         else
         {
            rc = cursor.searchNext( rid.toUINT64(), FALSE, isFound ) ;
         }
         if ( SDB_DMS_EOC == rc )
         {
            isFound = FALSE ;
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         else if ( SDB_OK == rc && !isFound )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to search key from store, rc: %d", rc ) ;

         WT_SESSION *s = session.getSession() ;
         WT_CURSOR *c = cursor.getCursor() ;
         if ( direction > 0 )
         {
            rc = WT_CALL( s->truncate( s, nullptr, nullptr, c, nullptr ), s ) ;
         }
         else
         {
            rc = WT_CALL( s->truncate( s, nullptr, c, nullptr, nullptr ), s ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate store, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "Pop count [%llu], size [%llu]", popCount, popSize ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_POPREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__CREATEDATACURSOR, "_dmsWTCollection::_createDataCursor" )
   INT32 _dmsWTCollection::_createDataCursor( unique_ptr<IDataCursor> &cursor,
                                              BOOLEAN isAsync,
                                              IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__CREATEDATACURSOR ) ;

      IPersistUnit *persistUnit = nullptr ;

      if ( !isAsync )
      {
         rc = _engine.getService().getPersistUnit( executor, persistUnit ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;
      }

      if ( persistUnit )
      {
         dmsWTPersistUnit *wtUnit = dynamic_cast<dmsWTPersistUnit *>( persistUnit ) ;
         PD_CHECK( wtUnit, SDB_SYS, error, PDERROR,
                   "Failed to get persist unit, it is not a WiredTiger persist unit" ) ;

         cursor = unique_ptr<dmsWTDataCursor>( new dmsWTDataCursor( wtUnit->getReadSession() ) ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "Opened data cursor on collection [%s.%s]",
                 _metadata.getSU()->getSUName(),
                 _metadata.getMB()->_collectionName ) ;
      }
      else
      {
         cursor = unique_ptr<dmsWTDataCursor>( new dmsWTDataAsyncCursor() ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "Opened async data cursor on collection [%s.%s]",
                 _metadata.getSU()->getSUName(),
                 _metadata.getMB()->_collectionName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__CREATEDATACURSOR, rc ) ;
      return rc ;

   error:
      cursor.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, "_dmsWTCollection::createDataCursor" )
   INT32 _dmsWTCollection::createDataCursor( unique_ptr<IDataCursor> &cursor,
                                             const dmsRecordID &startRID,
                                             BOOLEAN afterStartRID,
                                             BOOLEAN isForward,
                                             BOOLEAN isAsync,
                                             IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEDATACURSOR ) ;

      UINT64 snapshotID = 0 ;

      rc = _createDataCursor( cursor, isAsync, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
      SDB_ASSERT( cursor, "cursor should be valid" ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor" ) ;

      snapshotID = _metadata.getMBStat()->_snapshotID.fetch() ;
      rc = cursor->open( shared_from_this(),
                         startRID,
                         afterStartRID,
                         isForward,
                         snapshotID,
                         executor ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEDATASAMPLECURSOR, "_dmsWTCollection::createDataSampleCursor" )
   INT32 _dmsWTCollection::createDataSampleCursor( unique_ptr<IDataCursor> &cursor,
                                                   UINT64 sampleNum,
                                                   BOOLEAN isAsync,
                                                   IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEDATASAMPLECURSOR ) ;

      UINT64 snapshotID = 0 ;

      rc = _createDataCursor( cursor, isAsync, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
      SDB_ASSERT( cursor, "cursor should be valid" ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor" ) ;

      snapshotID = _metadata.getMBStat()->_snapshotID.fetch() ;
      rc = cursor->open( shared_from_this(), sampleNum, snapshotID, executor ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open sample data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEDATASAMPLECURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETCOUNT, "_dmsWTCollection::getCount" )
   INT32 _dmsWTCollection::getCount( UINT64 &count,
                                     BOOLEAN isFast,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETCOUNT ) ;

      if ( isFast )
      {
         count = _metadata.getMBStat()->_totalRecords.fetch() ;
      }
      else
      {
         rc = _dmsWTStoreHolder::getCount( count, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to count from store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETCOUNT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETDATASTATS, "_dmsWTCollection::getDataStats" )
   INT32 _dmsWTCollection::getDataStats( UINT64 &totalSize,
                                         UINT64 &freeSize,
                                         BOOLEAN isFast,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETDATASTATS ) ;

      if ( isFast )
      {
         totalSize = _metadata.getMBStat()->_totalDataPages *
                     _metadata.getSU()->getPageSize() ;
         freeSize = _metadata.getMBStat()->_totalDataFreeSpace ;
      }
      else
      {
         rc = _dmsWTStoreHolder::getStoreTotalSize( totalSize, executor ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to store total size, rc: %d", rc ) ;
         totalSize = ossRoundUpToMultipleX( totalSize, _metadata.getSU()->getPageSize() ) ;

         rc = _dmsWTStoreHolder::getStoreFreeSize( freeSize, executor ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to store free size, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETDATASTATS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETINDEXSTATS, "_dmsWTCollection::getIndexStats" )
   INT32 _dmsWTCollection::getIndexStats( UINT64 &totalSize,
                                          UINT64 &freeSize,
                                          BOOLEAN isFast,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETINDEXSTATS ) ;

      if ( isFast )
      {
         totalSize = _metadata.getMBStat()->_totalIndexPages *
                     _metadata.getSU()->getPageSize() ;
         freeSize = _metadata.getMBStat()->_totalIndexFreeSpace ;
      }
      else
      {
         UINT32 pageSize = _metadata.getSU()->getPageSize() ;
         std::shared_ptr<IIndex> idxPtr ;
         totalSize = 0 ;
         freeSize = 0 ;
         while ( ( idxPtr = _getNextIndex( idxPtr ) ) )
         {
            UINT64 idxTotalSize = 0, idxFreeSize = 0 ;

            rc = idxPtr->getIndexStats( idxTotalSize, idxFreeSize, FALSE, executor ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_OK ;
               continue ;
            }

            totalSize += idxTotalSize ;
            // one more page for metadata
            totalSize += pageSize ;
            freeSize += idxFreeSize ;
         }
      }

      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETINDEXSTATS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_VALIDATEDATA, "_dmsWTCollection::validateData" )
   INT32 _dmsWTCollection::validateData( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_VALIDATEDATA ) ;

      std::shared_ptr<ILob> lobPtr ;

      UINT64 recordCount = 0,
             totalDataLen = 0,
             totalDataSize = 0,
             freeDataSize = 0,
             totalIndexSize = 0,
             freeIndexSize = 0 ;
      UINT32 pageSize = _metadata.getSU()->getPageSize() ;
      dmsRecordID maxRID ;

      class _dmsWTDataValidator : public _dmsWTStoreValidator
      {
      public:
         _dmsWTDataValidator( UINT64 &recordCount, UINT64 &totalDataLen )
         : _recordCount( recordCount ),
           _totalDataLen( totalDataLen )
         {
         }

         virtual ~_dmsWTDataValidator() = default ;

         virtual INT32 validate( const dmsWTItem &keyItem, const dmsWTItem &valueItem )
         {
            ++ _recordCount ;
            _totalDataLen += valueItem.getSize() ;
            return SDB_OK ;
         }

      protected:
         UINT64 &_recordCount ;
         UINT64 &_totalDataLen ;
      } ;
      _dmsWTDataValidator validator( recordCount, totalDataLen ) ;

      // validate store
      rc = _validateStore( validator, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to validate store, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Reset total record count [%llu], data length [%llu]",
              recordCount, totalDataLen ) ;
      _metadata.getMBStat()->_totalRecords.poke( recordCount ) ;
      _metadata.getMBStat()->_rcTotalRecords.poke( recordCount ) ;
      _metadata.getMBStat()->_totalDataLen.poke( totalDataLen ) ;
      _metadata.getMBStat()->_totalOrgDataLen.poke( totalDataLen ) ;

      // recover record ID generator
      rc = getMaxRecordID( maxRID, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get max record ID, rc: %d", rc ) ;

      if ( maxRID.isValid() )
      {
         PD_LOG( PDEVENT, "Move record ID to [extent: %u, offset: %u]",
                 maxRID._extent, maxRID._offset ) ;
         _metadata.getMBStat()->_ridGen.poke( maxRID.toUINT64() + 1 ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Move record ID to [extent: 0, offset: 0]" ) ;
         _metadata.getMBStat()->_ridGen.poke( 0 ) ;
      }

      // recover file size
      rc = getDataStats( totalDataSize, freeDataSize, FALSE, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get data stats, rc: %d", rc ) ;
      rc = getIndexStats( totalIndexSize, freeIndexSize, FALSE, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get index stats, rc: %d", rc ) ;

      _metadata.getMBStat()->_totalDataPages = totalDataSize / pageSize;
      _metadata.getMBStat()->_totalDataFreeSpace = freeDataSize ;
      _metadata.getMBStat()->_totalIndexPages = totalIndexSize / pageSize ;
      _metadata.getMBStat()->_totalIndexFreeSpace = freeIndexSize ;

      {
         ossScopedRWLock lock( &_lobMutex, SHARED );
         lobPtr = _lob;
      }
      if ( lobPtr )
      {
         rc = lobPtr->validate( executor );
         PD_RC_CHECK( rc, PDWARNING, "Failed to validate lob, rc: %d", rc );
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_VALIDATEDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_BLDDATACONFSTR, "_dmsWTCollection::buildDataConfigString" )
   INT32 _dmsWTCollection::buildDataConfigString( const dmsWTEngineOptions &options,
                                                  const dmsCreateCLOptions &createCLOptions,
                                                  ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_BLDDATACONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file," ;
         ss << "memory_page_max=10m," ;
         ss << "split_pct=90," ;
         ss << "leaf_value_max=64MB," ;
         ss << "checksum=on," ;

         switch ( createCLOptions._compressorType )
         {
         case UTIL_COMPRESSOR_SNAPPY:
            ss << "block_compressor=snappy," ;
            break ;
         case UTIL_COMPRESSOR_ZLIB:
            ss << "block_compressor=zlib," ;
            break ;
         case UTIL_COMPRESSOR_LZ4:
            ss << "block_compressor=lz4," ;
            break ;
         default:
            ss << "block_compressor=none," ;
            break ;
         }

         ss << "key_format=q," ;
         ss << "value_format=u," ;
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build data config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_BLDDATACONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_BLDDATAURI, "_dmsWTCollection::buildDataURI" )
   INT32 _dmsWTCollection::buildDataURI( utilCSUniqueID csUID,
                                         utilCLInnerID clInnerID,
                                         UINT32 clLID,
                                         ossPoolString &dataURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_BLDDATAURI ) ;

      try
      {
         ossPoolStringStream ss ;
         ss << "table:" ;
         dmsWTBuildDataIdent( csUID, clInnerID, clLID, ss ) ;
         dataURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build data URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_BLDDATAURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_BLDLOBCONFSTR, "_dmsWTCollection::buildLobConfigString" )
   INT32 _dmsWTCollection::buildLobConfigString( const dmsWTEngineOptions &options,
                                                 ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_BLDLOBCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file," ;
         ss << "memory_page_max=10m," ;
         ss << "split_pct=90," ;
         ss << "leaf_value_max=64MB," ;
         ss << "checksum=on," ;
         ss << "key_format=u," ;
         ss << "value_format=u," ;
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build data config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_BLDLOBCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__ADDINDEX, "_dmsWTCollection::_addIndex" )
   INT32 _dmsWTCollection::_addIndex( const dmsIdxMetadata &metadata,
                                      const dmsWTStore &store,
                                      shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__ADDINDEX ) ;

      try
      {
         dmsIdxMetadataKey key( metadata.getIdxKey() ) ;
         PD_CHECK( key.isValid(), SDB_SYS, error, PDERROR,
                   "Failed to save index, collection [UID: %llx, LID: %x], "
                   "index [UID: %x] is not valid",
                   key.getCLOrigUID(), key.getCLOrigLID(), key.getIdxInnerID() ) ;
         idxPtr = std::make_shared<dmsWTIndex>( metadata, _engine, store ) ;
         PD_CHECK( idxPtr, SDB_OOM, error, PDERROR,
                   "Failed to create index object" ) ;

         ossScopedRWLock lock( &_idxMapMutex, EXCLUSIVE ) ;
         auto res = _idxMap.insert( make_pair( key, idxPtr ) ) ;
         if ( !res.second )
         {
            PD_LOG( PDDEBUG, "Failed to add index, collection "
                    "[UID: %llx, LID: %x], index [UID: %x] already exist",
                    key.getCLOrigUID(), key.getCLOrigLID(), key.getIdxInnerID() ) ;
            idxPtr = res.first->second ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save index, occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__ADDINDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__REMOVEINDEX, "_dmsWTCollection::_removeIndex" )
   void _dmsWTCollection::_removeIndex( const dmsIdxMetadataKey &metadataKey )
   {
      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__REMOVEINDEX ) ;

      ossScopedRWLock lock( &_idxMapMutex, EXCLUSIVE ) ;
      _idxMap.erase( metadataKey ) ;

      PD_TRACE_EXIT( SDB__DMSWTCOLLECTION__REMOVEINDEX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETINDEX, "_dmsWTCollection::_getIndex" )
   shared_ptr<IIndex> _dmsWTCollection::_getIndex( const dmsIdxMetadataKey &metadataKey )
   {
      shared_ptr<IIndex> idxPtr ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETINDEX ) ;

      ossScopedRWLock lock( &_idxMapMutex, SHARED ) ;
      _dmsWTIdxMapIter iter = _idxMap.find( metadataKey ) ;
      if ( iter != _idxMap.end() )
      {
         idxPtr = iter->second ;
      }

      PD_TRACE_EXIT( SDB__DMSWTCOLLECTION__GETINDEX ) ;

      return idxPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETNEXTINDEX, "_dmsWTCollection::_getNextIndex" )
   shared_ptr<IIndex> _dmsWTCollection::_getNextIndex( shared_ptr<IIndex> &idxPtr )
   {
      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETNEXTINDEX ) ;

      ossScopedRWLock lock( &_idxMapMutex, SHARED ) ;
      if ( idxPtr )
      {
         _dmsWTIdxMapIter iter = _idxMap.upper_bound( idxPtr->getMetadataKey() ) ;
         if ( iter != _idxMap.end() )
         {
            idxPtr = iter->second ;
         }
         else
         {
            idxPtr.reset() ;
         }
      }
      else
      {
         _dmsWTIdxMapIter iter = _idxMap.begin() ;
         if ( iter != _idxMap.end() )
         {
            idxPtr = iter->second ;
         }
         else
         {
            idxPtr.reset() ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSWTCOLLECTION__GETNEXTINDEX ) ;

      return idxPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETMINRECORDID, "_dmsWTCollection::getMinRecordID" )
   INT32 _dmsWTCollection::getMinRecordID( dmsRecordID &rid, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETMINRECORDID ) ;

      dmsWTSessionHolder sessionHolder ;

      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      rc = _getMinRecordID( sessionHolder.getSession(), rid, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get min record ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETMINRECORDID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETMINRECORDID_SESS, "_dmsWTCollection::_getMinRecordID" )
   INT32 _dmsWTCollection::_getMinRecordID( dmsWTSession &session,
                                            dmsRecordID &rid,
                                            IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETMINRECORDID_SESS ) ;

      dmsWTCursor cursor( session ) ;
      UINT64 key = 0 ;

      rc = cursor.open( _store.getURI(), "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      rc = cursor.moveToHead() ;
      if ( SDB_DMS_EOC == rc )
      {
         rid.reset() ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move head of store, rc: %d", rc ) ;

      rc = cursor.getKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      rid.fromUINT64( key ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__GETMINRECORDID_SESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETMAXRECORDID, "_dmsWTCollection::getMaxRecordID" )
   INT32 _dmsWTCollection::getMaxRecordID( dmsRecordID &rid, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETMAXRECORDID ) ;

      dmsWTSessionHolder sessionHolder ;

      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      rc = _getMaxRecordID( sessionHolder.getSession(), rid, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get max record ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETMAXRECORDID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS, "_dmsWTCollection::_getMaxRecordID" )
   INT32 _dmsWTCollection::_getMaxRecordID( dmsWTSession &session,
                                            dmsRecordID &rid,
                                            IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS ) ;

      dmsWTCursor cursor( session ) ;
      UINT64 key = 0 ;

      rc = cursor.open( _store.getURI(), "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      rc = cursor.moveToTail() ;
      if ( SDB_DMS_EOC == rc )
      {
         rid.reset() ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move tail of store, rc: %d", rc ) ;

      rc = cursor.getKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      rid.fromUINT64( key ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
