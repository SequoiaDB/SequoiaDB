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

   Source File Name = dmsWTStorageService.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "interface/IOperationContext.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"

#include <boost/filesystem/operations.hpp>

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStorageService implement
    */
   _dmsWTStorageService::_dmsWTStorageService()
   : _engineOptions(),
     _engine( *this, _engineOptions )
   {
   }

   _dmsWTStorageService::~_dmsWTStorageService()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_OPENENGINE, "_dmsWTStorageService::openEngine" )
   INT32 _dmsWTStorageService::openEngine( const dmsOpenEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_OPENENGINE ) ;

      dmsWTEngineOptions engineOptions ;
      ossPoolString config ;

      rc = _initEngineOptions( engineOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init WiredTiger engine options, rc: %d", rc ) ;

      rc = _buildConfigString( engineOptions, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger config string, rc: %d", rc ) ;
      PD_LOG( PDEVENT, "WiredTiger config string: %s", config.c_str() ) ;

      rc = _engine.open( engineOptions.getDBPath(), config.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger engine, rc: %d", rc ) ;

      _engineOptions = engineOptions ;

      pmdGetSyncMgr()->registerSync( this ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_OPENENGINE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CLOSE, "_dmsWTStorageService::close" )
   INT32 _dmsWTStorageService::closeEngine( const dmsCloseEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CLOSE ) ;

      pmdGetSyncMgr()->unregSync( this ) ;

      rc = _engine.close( NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to close WiredTiger engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CLOSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CHANGECONFIG, "_dmsWTStorageService::changeConfig" )
   INT32 _dmsWTStorageService::changeConfig()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CHANGECONFIG ) ;

      dmsWTEngineOptions engineOptions ;
      ossPoolString config ;

      rc = _initEngineOptions( engineOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init WiredTiger engine options, rc: %d", rc ) ;

      rc = _buildReconfigString( engineOptions, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger reconfig string, rc: %d", rc ) ;
      PD_LOG( PDEVENT, "WiredTiger reconfig string: %s", config.c_str() ) ;

      rc = _engine.reconfig( config.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reconfig WiredTiger engine, rc: %d", rc ) ;

      // runtime configs
      if ( engineOptions.getCheckPointInterval() != _engineOptions.getCheckPointInterval() )
      {
         _engineOptions.setCheckPointInterval( engineOptions.getCheckPointInterval() ) ;
      }
      if ( engineOptions.getCacheSizeMB() != _engineOptions.getCacheSizeMB() )
      {
         _engineOptions.setCacheSizeMB( engineOptions.getCacheSizeMB() ) ;
      }
      if ( engineOptions.getEvictTarget() != _engineOptions.getEvictTarget() )
      {
         _engineOptions.setEvictTarget( engineOptions.getEvictTarget() ) ;
      }
      if ( engineOptions.getEvictTrigger() != _engineOptions.getEvictTrigger() )
      {
         _engineOptions.setEvictTrigger( engineOptions.getEvictTrigger() ) ;
      }
      if ( engineOptions.getEvictDirtyTarget() != _engineOptions.getEvictDirtyTarget() )
      {
         _engineOptions.setEvictDirtyTarget( engineOptions.getEvictDirtyTarget() ) ;
      }
      if ( engineOptions.getEvictDirtyTrigger() != _engineOptions.getEvictDirtyTrigger() )
      {
         _engineOptions.setEvictDirtyTrigger( engineOptions.getEvictDirtyTrigger() ) ;
      }
      if ( engineOptions.getEvictUpdatesTarget() != _engineOptions.getEvictUpdatesTarget() )
      {
         _engineOptions.setEvictUpdatesTarget( engineOptions.getEvictUpdatesTarget() ) ;
      }
      if ( engineOptions.getEvictUpdatesTrigger() != _engineOptions.getEvictUpdatesTrigger() )
      {
         _engineOptions.setEvictUpdatesTrigger( engineOptions.getEvictUpdatesTrigger() ) ;
      }
      if ( engineOptions.getEvictThreadsMin() != _engineOptions.getEvictThreadsMin() )
      {
         _engineOptions.setEvictThreadsMin( engineOptions.getEvictThreadsMin() ) ;
      }
      if ( engineOptions.getEvictThreadsMax() != _engineOptions.getEvictThreadsMax() )
      {
         _engineOptions.setEvictThreadsMax( engineOptions.getEvictThreadsMax() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CHANGECONFIG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_FSYNC, "_dmsWTStorageService::fsync" )
   INT32 _dmsWTStorageService::fsync( BOOLEAN isForce, BOOLEAN isSync, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_FSYNC ) ;

      lock() ;
      rc = sync( isForce, isSync, executor ) ;
      unlock() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync WiredTiger engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_FSYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_GETPERSISTUNIT, "_dmsWTStorageService::getPersistUnit" )
   INT32 _dmsWTStorageService::getPersistUnit( IExecutor *executor,
                                               IPersistUnit *&persistUnit )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_GETPERSISTUNIT ) ;

      IOperationContext *optCtx = nullptr ;
      persistUnit = nullptr ;

      if ( !executor ||
           !executor->getOperationContext() )
      {
         goto done ;
      }

      optCtx = executor->getOperationContext() ;
      persistUnit = optCtx->getPersistUnit() ;
      if ( !persistUnit )
      {
         dmsWTPersistUnit *wtUnit = SDB_OSS_NEW dmsWTPersistUnit( _engine ) ;
         PD_CHECK( wtUnit, SDB_OOM, error, PDERROR,
                   "Failed to create persist unit object" ) ;
         std::unique_ptr<IPersistUnit> puPtr = std::unique_ptr<dmsWTPersistUnit>( wtUnit ) ;
         PD_CHECK( puPtr, SDB_OOM, error, PDERROR,
                   "Failed to create persist unit pointer" ) ;
         rc = wtUnit->initUnit( executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init persist unit, rc: %d", rc ) ;

         persistUnit = puPtr.get() ;
         optCtx->setPersistUnit( std::move(puPtr) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_GETPERSISTUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CREATECS, "_dmsWTStorageService::createCS" )
   INT32 _dmsWTStorageService::createCS( const dmsCSMetadata &metadata,
                                         const dmsCreateCSOptions &options,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CREATECS ) ;

      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CREATECS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_DROPCS, "_dmsWTStorageService::dropCS" )
   INT32 _dmsWTStorageService::dropCS( const dmsCSMetadata &metadata,
                                       const dmsDropCSOptions &options,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_DROPCS ) ;

      ossPoolList<ossPoolString> uriList ;

      // make sure the collections pointers are released
      _removeCollections( metadata.getCSUID() ) ;

      rc = _dumpURIListByCS( metadata.getCSUID(), uriList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump WiredTiger URI list, rc: %d", rc ) ;

      rc = _engine.dropStores( uriList, "force,checkpoint_wait=false" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger stores, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_DROPCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CREATECL, "_dmsWTStorageService::createCL" )
   INT32 _dmsWTStorageService::createCL( const dmsCLMetadata &metadata,
                                         const dmsCreateCLOptions &options,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CREATECL ) ;

      dmsWTStore store ;
      ossPoolString dataURI, dataConfig ;
      shared_ptr<ICollection> collPtr ;

      rc = dmsWTCollection::buildDataURI( metadata.getCSUID(),
                                          metadata.getCLOrigInnerID(),
                                          metadata.getCLOrigLID(),
                                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build data URI, rc: %d", rc ) ;

      rc = dmsWTCollection::buildDataConfigString( _engineOptions, options, dataConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build data config string, rc: %d", rc ) ;

      rc = _engine.createStore( dataURI.c_str(), dataConfig.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create data store, rc: %d", rc ) ;

      rc = _addCollection( metadata, store, collPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add collection, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CREATECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_DROPCL, "_dmsWTStorageService::dropCL" )
   INT32 _dmsWTStorageService::dropCL( const dmsCLMetadata &metadata,
                                       const dmsDropCLOptions &options,
                                       IContext *context,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_DROPCL ) ;

      ossPoolString dataURI ;
      ossPoolString lobURI ;
      ossPoolList<ossPoolString> uris ;

      rc = dmsWTCollection::buildDataURI( metadata.getCSUID(),
                                          metadata.getCLOrigInnerID(),
                                          metadata.getCLOrigLID(),
                                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build data URI, rc: %d", rc ) ;
      uris.push_back( std::move( dataURI ) );

      rc = dmsWTLob::buildLobURI( metadata.getCSUID(), metadata.getCLOrigInnerID(),
                                  metadata.getCLOrigLID(), lobURI );
      PD_RC_CHECK( rc, PDERROR, "Failed to build lob URI, rc: %d", rc );
      uris.push_back( std::move( lobURI ) );

      rc = _engine.dropStores( uris, "force,checkpoint_wait=false" );
      PD_RC_CHECK( rc, PDERROR, "Failed to drop data and lob stores, rc: %d", rc );

      _removeCollection( metadata.getCLKey() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_DROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_GETCOLL, "_dmsWTStorageService::getCollection" )
   INT32 _dmsWTStorageService::getCollection( const dmsCLMetadataKey &metadataKey,
                                              IExecutor *executor,
                                              shared_ptr<ICollection> &collPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_GETCOLL ) ;

      collPtr = _getCollection( metadataKey ) ;
      PD_CHECK( collPtr, SDB_DMS_NOTEXIST, error, PDDEBUG,
                "Failed to get collection, collection [UID: %llx, LID: %x] not exist",
                metadataKey.getCLOrigUID(), metadataKey.getCLOrigLID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_GETCOLL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_LOADCOLL, "_dmsWTStorageService::loadCollection" )
   INT32 _dmsWTStorageService::loadCollection( const dmsCLMetadata &metadata,
                                               IExecutor *executor,
                                               shared_ptr<ICollection> &collPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_LOADCOLL ) ;

      dmsWTStore store ;
      ossPoolString dataURI ;

      collPtr.reset() ;

      rc = dmsWTCollection::buildDataURI( metadata.getCSUID(),
                                          metadata.getCLOrigInnerID(),
                                          metadata.getCLOrigLID(),
                                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build data URI, rc: %d", rc ) ;

      rc = _engine.loadStore( dataURI.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load data store [%s], rc: %d",
                   dataURI.c_str(), rc ) ;

      rc = _addCollection( metadata, store, collPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add collection, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_LOADCOLL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CANSYNC, "_dmsWTStorageService::canSync" )
   BOOLEAN _dmsWTStorageService::canSync( BOOLEAN &force ) const
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CANSYNC ) ;

      if ( pmdGetTickSpanTime( _lastPersistTick ) >=
                  (UINT64)( _engineOptions.getCheckPointInterval() ) * OSS_ONE_SEC )
      {
         res = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE_CANSYNC ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_SYNC, "_dmsWTStorageService::sync" )
   INT32 _dmsWTStorageService::sync( BOOLEAN force, BOOLEAN sync, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_SYNC ) ;

      // checkpoint
      rc = _engine.checkPoint( executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to checkpoint, rc: %d", rc ) ;

      rc = _syncStats( executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync stats, rc: %d", rc ) ;

      _lastPersistTick = pmdGetDBTick() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_SYNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS, "_dmsWTStorageService::_initEngineOptions" )
   INT32 _dmsWTStorageService::_initEngineOptions( dmsWTEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS ) ;

      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      boost::filesystem::path dbPath( optionCB->getDbPath() ) ;
      options.setDBPath( dbPath ) ;

      options.setCacheSizeMB( optionCB->getWTCacheSize() ) ;
      options.setEvictTarget( optionCB->getWTEvictTarget() ) ;
      options.setEvictTrigger( optionCB->getWTEvictTrigger() ) ;
      options.setEvictDirtyTarget( optionCB->getWTEvictDirtyTarget() ) ;
      options.setEvictDirtyTrigger( optionCB->getWTEvictDirtyTrigger() ) ;
      options.setEvictUpdatesTarget( optionCB->getWTEvictUpdatesTarget() ) ;
      options.setEvictUpdatesTrigger( optionCB->getWTEvictUpdatesTrigger() ) ;
      options.setEvictThreadsMin( optionCB->getWTEvictThreadsMin() ) ;
      options.setEvictThreadsMax( optionCB->getWTEvictThreadsMax() ) ;
      options.setCheckPointInterval( optionCB->getWTCheckPointInterval() ) ;

      options.fixOptions() ;


      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR, "_dmsWTStorageService::_buildConfigString" )
   INT32 _dmsWTStorageService::_buildConfigString( const dmsWTEngineOptions &options,
                                                   ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "create," ;
         ss << "cache_size=" << options.getCacheSizeMB() << "MB," ;
         ss << "session_max=33000," ;
         ss << "eviction=(threads_min=" << options.getEvictThreadsMin()
            << ",threads_max=" << options.getEvictThreadsMax() << ")," ;
         ss << "eviction_target=" << options.getEvictTarget() << "," ;
         ss << "eviction_trigger=" << options.getEvictTrigger() << "," ;
         ss << "eviction_dirty_target=" << options.getEvictDirtyTarget() << "," ;
         ss << "eviction_dirty_trigger=" << options.getEvictDirtyTrigger() << "," ;
         ss << "eviction_updates_target=" << options.getEvictUpdatesTarget() << "," ;
         ss << "eviction_updates_trigger=" << options.getEvictUpdatesTrigger() << "," ;
         ss << "config_base=false," ;
         ss << "statistics=(fast)," ;
         ss << "log=(enabled=true,remove=true,path=journal,compressor=snappy)," ;
         ss << "builtin_extension_config=(zstd=(compression_level=6))," ;
         ss << "file_manager=(close_idle_time=600,close_scan_interval=10,close_handle_minimum=2000)," ;
         ss << "statistics_log=(wait=0)," ;
         ss << "json_output=(error,message)" ;

         configString = ss.str() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger connection string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDCREONFSTR, "_dmsWTStorageService::_buildReconfigString" )
   INT32 _dmsWTStorageService::_buildReconfigString( const dmsWTEngineOptions &options,
                                                     ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDCREONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         if ( _engineOptions.getCacheSizeMB() != options.getCacheSizeMB() )
         {
            ss << "cache_size=" << options.getCacheSizeMB() << "MB," ;
         }
         if ( _engineOptions.getEvictTarget() != options.getEvictTarget() )
         {
            ss << "eviction_target=" << options.getEvictTarget() << "," ;
         }
         if ( _engineOptions.getEvictTrigger() != options.getEvictTrigger() )
         {
            ss << "eviction_trigger=" << options.getEvictTrigger() << "," ;
         }
         if ( _engineOptions.getEvictDirtyTarget() != options.getEvictDirtyTarget() )
         {
            ss << "eviction_dirty_target=" << options.getEvictDirtyTarget() << "," ;
         }
         if ( _engineOptions.getEvictDirtyTrigger() != options.getEvictDirtyTrigger() )
         {
            ss << "eviction_dirty_trigger=" << options.getEvictDirtyTrigger() << "," ;
         }
         if ( _engineOptions.getEvictUpdatesTarget() != options.getEvictUpdatesTarget() )
         {
            ss << "eviction_updates_target=" << options.getEvictUpdatesTarget() << "," ;
         }
         if ( _engineOptions.getEvictUpdatesTrigger() != options.getEvictUpdatesTrigger() )
         {
            ss << "eviction_updates_trigger=" << options.getEvictUpdatesTrigger() << "," ;
         }
         if ( _engineOptions.getEvictThreadsMin() != options.getEvictThreadsMin() )
         {
            ss << "eviction=(threads_min=" << options.getEvictThreadsMin() << ")," ;
         }
         if ( _engineOptions.getEvictThreadsMax() != options.getEvictThreadsMax() )
         {
            ss << "eviction=(threads_max=" << options.getEvictThreadsMax() << ")," ;
         }

         configString = ss.str() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger connection string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDCREONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS, "_dmsWTStorageService::_dumpURIListByCS" )
   INT32 _dmsWTStorageService::_dumpURIListByCS( utilCSUniqueID csUID,
                                                 ossPoolList< ossPoolString > &uriList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS ) ;

      ossPoolStringStream prefixSS ;
      ossPoolString prefix ;

      prefixSS << "table:" ;
      dmsWTBuildIdentPrefix( csUID, prefixSS ) ;
      prefix = prefixSS.str();

      rc = _engine.dumpURIListByPrefix( prefix.c_str(), uriList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump WiredTiger URI list, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__ADDCOLLECTION, "_dmsWTStorageService::_addCollection" )
   INT32 _dmsWTStorageService::_addCollection( const dmsCLMetadata &metadata,
                                               const dmsWTStore &store,
                                               shared_ptr<ICollection> &collPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__ADDCOLLECTION ) ;

      try
      {
         dmsCLMetadataKey key( metadata.getCLKey() ) ;
         PD_CHECK( key.isValid(), SDB_SYS, error, PDERROR,
                   "Failed to save collection, collection [UID: %llx, LID: %x] "
                   "is not valid", key.getCLOrigUID(), key.getCLOrigLID() ) ;
         collPtr = std::make_shared<dmsWTCollection>( metadata, _engine, store ) ;
         PD_CHECK( collPtr, SDB_OOM, error, PDERROR,
                   "Failed to create collection object" ) ;

         ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
         auto res = _collMap.insert( make_pair( key, collPtr ) ) ;
         if ( !res.second )
         {
            PD_LOG( PDDEBUG, "Failed to add collection, collection "
                    "[UID: %llx, LID: %x] already exist",
                    key.getCLOrigUID(), key.getCLOrigLID() ) ;
            collPtr = res.first->second ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save collection, occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__ADDCOLLECTION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTION, "_dmsWTStorageService::_removeCollection" )
   void _dmsWTStorageService::_removeCollection( const dmsCLMetadataKey &metadataKey )
   {
      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTION ) ;

      ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
      _collMap.erase( metadataKey ) ;

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTION ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTIONS, "_dmsWTStorageService::_removeCollections" )
   void _dmsWTStorageService::_removeCollections( utilCSUniqueID csUID )
   {
      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTIONS ) ;

      dmsCLMetadataKey lowBound( utilBuildCLUniqueID( csUID, 0 ), 0 ) ;
      dmsCLMetadataKey upBound( utilBuildCLUniqueID( csUID + 1, 0 ), 0 ) ;

      ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
      _dmsWTCollMapIter lowIter = _collMap.lower_bound( lowBound ) ;
      _dmsWTCollMapIter upIter = _collMap.lower_bound( upBound ) ;
      if ( lowIter != upIter )
      {
         _collMap.erase( lowIter, upIter ) ;
      }

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE__REMOVECOLLECTIONS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__GETCOLLECTION, "_dmsWTStorageService::_getCollection" )
   shared_ptr<ICollection> _dmsWTStorageService::_getCollection( const dmsCLMetadataKey &metadataKey )
   {
      shared_ptr<ICollection> collPtr ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__GETCOLLECTION ) ;

      ossScopedRWLock lock( &_collMapMutex, SHARED ) ;
      _dmsWTCollMapIter iter = _collMap.find( metadataKey ) ;
      if ( iter != _collMap.end() )
      {
         collPtr = iter->second ;
      }

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE__GETCOLLECTION ) ;

      return collPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__GETNEXTCOLLECTION, "_dmsWTStorageService::_getNextCollection" )
   shared_ptr<ICollection> _dmsWTStorageService::_getNextCollection( shared_ptr<ICollection> &collPtr )
   {
      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__GETCOLLECTION ) ;

      ossScopedRWLock lock( &_collMapMutex, SHARED ) ;
      if ( collPtr )
      {
         _dmsWTCollMapIter iter = _collMap.upper_bound( collPtr->getMetadataKey() ) ;
         if ( iter != _collMap.end() )
         {
            collPtr = iter->second ;
         }
         else
         {
            collPtr.reset() ;
         }
      }
      else
      {
         _dmsWTCollMapIter iter = _collMap.begin() ;
         if ( iter != _collMap.end() )
         {
            collPtr = iter->second ;
         }
         else
         {
            collPtr.reset() ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE__GETCOLLECTION ) ;

      return collPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__SYNCSTATS, "_dmsWTStorageService::_syncStats" )
   INT32 _dmsWTStorageService::_syncStats( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__SYNCSTATS ) ;

      shared_ptr<ICollection> curCollPtr ;

      while ( ( curCollPtr = _getNextCollection( curCollPtr ) ) )
      {
         UINT64 totalDataSize = 0, freeDataSize = 0, totalIndexSize = 0, freeIndexSize = 0 ;
         rc = curCollPtr->getDataStats( totalDataSize, freeDataSize, FALSE, executor ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         rc = curCollPtr->getIndexStats( totalIndexSize, freeIndexSize, FALSE, executor ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         _updateStats( curCollPtr,
                       totalDataSize,
                       freeDataSize,
                       totalIndexSize,
                       freeIndexSize ) ;
      }

      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__SYNCSTATS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__UPDATESTATS, "_dmsWTStorageService::_updateStats" )
   void _dmsWTStorageService::_updateStats( shared_ptr<ICollection> &collPtr,
                                            UINT64 totalDataSize,
                                            UINT64 freeDataSize,
                                            UINT64 totalIndexSize,
                                            UINT64 freeIndexSize )
   {
      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__UPDATESTATS ) ;

      ossScopedRWLock lock( &_collMapMutex, SHARED ) ;
      _dmsWTCollMapIter iter = _collMap.find( collPtr->getMetadataKey() ) ;
      if ( iter != _collMap.end() && collPtr == iter->second )
      {
         dmsMBStatInfo *mbStat = collPtr->getMetadata().getMBStat() ;
         UINT32 pageSize = collPtr->getMetadata().getSU()->getPageSize() ;
         mbStat->_totalDataPages = totalDataSize / pageSize ;
         mbStat->_totalDataFreeSpace = freeDataSize ;
         mbStat->_totalIndexPages = totalIndexSize / pageSize ;
         mbStat->_totalIndexFreeSpace = freeIndexSize ;
      }

      PD_TRACE_EXIT( SDB__DMSWTSTORAGESERVICE__UPDATESTATS ) ;
   }

}
}
