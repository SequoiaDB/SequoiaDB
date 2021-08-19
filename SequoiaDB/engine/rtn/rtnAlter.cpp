/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = rtnAlter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnAlter.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsOp2Record.hpp"
#include "rtnAlterJob.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCREATEIDINDEX, "_rtnCreateIDIndex" )
   INT32 _rtnCreateIDIndex ( const CHAR * collection,
                             INT32 sortBufferSize,
                             _pmdEDUCB * cb,
                             _dpsLogWrapper * dpsCB,
                             _dmsMBContext * mbContext,
                             _dmsStorageUnit * su,
                             utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCREATEIDINDEX ) ;

      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;

      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      const CHAR * collectionShortName = mb->_collectionName ;

      rc = su->createIndex( collectionShortName, ixmGetIDIndexDefine(), cb,
                            NULL, TRUE, mbContext, sortBufferSize, pResult ) ;
      if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
      {
         /// already exists
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to create id index on collection [%s], "
                   "rc: %d", collection, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNCREATEIDINDEX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDROPIDINDEX, "_rtnDropIDIndex" )
   INT32 _rtnDropIDIndex ( const CHAR * collection,
                           _pmdEDUCB * cb,
                           _dpsLogWrapper * dpsCB,
                           _dmsMBContext * mbContext,
                           _dmsStorageUnit * su )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDROPIDINDEX ) ;

      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;
      SDB_ASSERT( NULL != cb, "cb is invalid" ) ;

      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      const CHAR * collectionShortName = mb->_collectionName ;

      rc = su->dropIndex( collectionShortName, IXM_ID_KEY_NAME, cb, NULL,
                          TRUE, mbContext ) ;
      if ( SDB_IXM_NOTEXIST == rc )
      {
         // Already dropped
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to drop id index on collection [%s], "
                   "rc: %d", collection, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNDROPIDINDEX, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETSHARD, "_rtnCollectionSetSharding" )
   INT32 _rtnCollectionSetSharding ( const CHAR * collection,
                                     const rtnCLShardingArgument & argument,
                                     _pmdEDUCB * cb,
                                     _dpsLogWrapper * dpsCB,
                                     _dmsMBContext * mbContext,
                                     _dmsStorageUnit * su,
                                     utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETSHARD ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;

      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      const CHAR * collectionShortName = mb->_collectionName ;

      BOOLEAN dropIndex = FALSE ;
      BOOLEAN createIndex = TRUE ;
      monIndex shardIndex ;
      const BSONObj & shardingKey = argument.getShardingKey() ;

      rc = su->getIndex( mbContext, IXM_SHARD_KEY_NAME, shardIndex ) ;
      if ( SDB_OK == rc )
      {
         if ( shardingKey.isEmpty() )
         {
            dropIndex = TRUE ;
            createIndex = FALSE ;
         }
         else if ( 0 == shardIndex.getKeyPattern().woCompare( shardingKey ) )
         {
            dropIndex = FALSE ;
            createIndex = FALSE ;
         }
         else
         {
            dropIndex = TRUE ;
            createIndex = TRUE ;
         }
      }
      else if ( SDB_IXM_NOTEXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get index [%s] on collection [%s], rc: %d",
                   IXM_SHARD_KEY_NAME, collection, rc ) ;

      if ( dropIndex )
      {
         rc = su->dropIndex( collectionShortName, IXM_SHARD_KEY_NAME, cb, NULL,
                             TRUE, mbContext ) ;
         if ( SDB_IXM_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop id index on collection [%s], "
                      "rc: %d", collection, rc ) ;
      }

      if ( argument.isEnsureShardingIndex() && createIndex )
      {
         BSONObj indexDef = BSON( IXM_FIELD_NAME_KEY << shardingKey <<
                                  IXM_FIELD_NAME_NAME << IXM_SHARD_KEY_NAME <<
                                  IXM_V_FIELD << 0 ) ;

         rc = su->createIndex( collectionShortName, indexDef, cb, NULL, TRUE,
                               mbContext, 0, pResult ) ;
         if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
         {
            /// sharding key index already exists.
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to create %s index on "
                      "collection [%s], rc: %d", IXM_SHARD_KEY_NAME,
                      collection, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSETSHARD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCHKSHARD, "_rtnCollectionCheckSharding" )
   INT32 _rtnCollectionCheckSharding ( const CHAR * collection,
                                       const rtnCLShardingArgument & argument,
                                       _pmdEDUCB * cb,
                                       _dmsMBContext * mbContext,
                                       _dmsStorageUnit * su )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNCHKSHARD ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;

      dmsMB * mb = NULL ;
      MON_IDX_LIST indexes ;

      BSONObj shardingKey = argument.getShardingKey() ;

      PD_CHECK( DMS_STORAGE_NORMAL == su->type(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check collection for sharding: "
                "should not be capped" ) ;

      PD_CHECK( 0LL == mbContext->mbStat()->_totalLobs ||
                argument.isHashSharding(),
                SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                "Failed to check collection for sharding: "
                "should be hash sharding for LOB data" ) ;

      mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      rc = su->getIndexes( mbContext, indexes ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get index on collection [%s], rc: %d",
                   collection, rc ) ;

      for ( MON_IDX_LIST::iterator iterIndex = indexes.begin() ;
            iterIndex != indexes.end() ;
            iterIndex ++ )
      {
         monIndex & index = (*iterIndex) ;
         UINT16 indexType = IXM_EXTENT_TYPE_NONE ;

         rc = index.getIndexType( indexType ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get index type, rc: %d", rc ) ;

         // Exclude non-unique index, text index and system index
         if ( !index.isUnique() ||
              index.getIndexName()[ 0 ] == '$' ||
              IXM_EXTENT_TYPE_TEXT == indexType )
         {
            continue ;
         }

         // Check the sharding key which should be included in keys of
         // unique index
         try
         {
            BSONObj indexKey = index.getKeyPattern() ;
            BSONObjIterator shardingItr ( shardingKey ) ;
            while ( shardingItr.more() )
            {
               BSONElement sk = shardingItr.next() ;
               PD_CHECK( indexKey.hasField( sk.fieldName() ),
                         SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY, error, PDERROR,
                         "All fields in sharding key must be included in "
                         "unique index, missing field: %s, shardingKey: %s, "
                         "indexKey: %s, collection: %s",
                         sk.fieldName(), shardingKey.toString().c_str(),
                         indexKey.toString().c_str(), collection ) ;
            }
         }
         catch( exception & e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNCHKSHARD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETCOMPRESS, "_rtnCollectionSetCompress" )
   INT32 _rtnCollectionSetCompress ( const CHAR * collection,
                                     UTIL_COMPRESSOR_TYPE compressorType,
                                     _pmdEDUCB * cb,
                                     _dmsMBContext * mbContext,
                                     _dmsStorageUnit * su,
                                     SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETCOMPRESS ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      rc = su->setCollectionCompressor( mb->_collectionName,
                                        compressorType, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on collection [%s], "
                   "rc: %d", collection, rc ) ;

      if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == mb->_compressorType  &&
           DMS_INVALID_EXTENT == mb->_dictExtentID )
      {
         /// Build the dictionary for LZW compression if needed
         dmsCB->pushDictJob( dmsDictJob( su->CSID(),
                                         su->LogicalCSID(),
                                         mbContext->mbID(),
                                         mbContext->clLID() ) ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSETCOMPRESS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETCOMPRESS_ARG, "_rtnCollectionSetCompress" )
   INT32 _rtnCollectionSetCompress ( const CHAR * collection,
                                     const rtnCLCompressArgument & argument,
                                     _pmdEDUCB * cb,
                                     _dmsMBContext * mbContext,
                                     _dmsStorageUnit * su,
                                     SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETCOMPRESS_ARG ) ;

      if ( argument.isCompressed() )
      {
         // Alter the attribute only in below cases :
         // 1. collection is no compressed
         // 2. altering the compressor type, and old type of collection is
         //    different
         if ( !OSS_BIT_TEST( mbContext->mb()->_attributes,
                             DMS_MB_ATTR_COMPRESSED ) ||
              ( argument.testArgumentMask( UTIL_CL_COMPRESSTYPE_FIELD ) &&
                mbContext->mb()->_compressorType != argument.getCompressorType()
              ) )
         {
            rc = _rtnCollectionSetCompress( collection,
                                            argument.getCompressorType(),
                                            cb, mbContext, su, dmsCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                         "collection [%s], rc: %d", collection, rc ) ;
         }
      }
      else
      {
         rc = _rtnCollectionSetCompress( collection, UTIL_COMPRESSOR_INVALID,
                                         cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNSETCOMPRESS_ARG, rc ) ;
      return rc ;

   error :
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSETEXTOPTS, "_rtnCollectionSetExtOptions" )
   INT32 _rtnCollectionSetExtOptions ( const CHAR * collection,
                                       const rtnCLExtOptionArgument & optionArgument,
                                       _pmdEDUCB * cb,
                                       _dmsMBContext * mbContext,
                                       _dmsStorageUnit * su )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSETEXTOPTS ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;

      BSONObj curExtOptions, extOptions ;
      dmsMB * mb = mbContext->mb() ;
      SDB_ASSERT( NULL != mb, "mb is invalid" ) ;

      const CHAR * clShortName = mb->_collectionName ;

      rc = su->getCollectionExtOptions( clShortName, curExtOptions,
                                        mbContext ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get extent options on collection [%s], rc: %d",
                   collection, rc ) ;

      rc = optionArgument.toBSON( curExtOptions, extOptions ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to generate new ext options, rc: %d", rc ) ;

      rc = su->setCollectionExtOptions( clShortName, extOptions, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on collection [%s], "
                   "rc: %d", collection, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__RTNSETEXTOPTS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLCHKATTR, "_rtnAlterCLCheckAttributes" )
   INT32 _rtnAlterCLCheckAttributes ( const CHAR * collection,
                                      const rtnAlterTask * task,
                                      _dmsMBContext * mbContext,
                                      _dmsStorageUnit * su,
                                      _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLCHKATTR ) ;

      const rtnCLSetAttributeTask * localTask =
            dynamic_cast<const rtnCLSetAttributeTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get alter task" ) ;

      // Check sharding
      if ( localTask->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         rc = _rtnCollectionCheckSharding( collection,
                                           localTask->getShardingArgument(),
                                           cb, mbContext, su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check sharding for "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

      // Check compress
      if ( localTask->containCompressArgument() )
      {
         rc = su->canSetCollectionCompressor( mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check compress for collection "
                      "[%s], rc: %d", rc ) ;
      }

      // Check ext options
      if ( localTask->containExtOptionArgument() )
      {
         PD_CHECK( DMS_STORAGE_CAPPED == su->type(),
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "Failed to check collection for setting ext options: "
                   "should be capped", collection ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLCHKATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCLSETATTR, "_rtnAlterCLSetAttributes" )
   INT32 _rtnAlterCLSetAttributes ( const CHAR * collection,
                                    const rtnAlterTask * task,
                                    _pmdEDUCB * cb,
                                    _dpsLogWrapper * dpsCB,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    _SDB_DMSCB * dmsCB,
                                    DMS_FILE_TYPE & dpsType,
                                    utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCLSETATTR ) ;

      SDB_ASSERT( NULL != mbContext, "mbContext is invalid" ) ;
      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != dmsCB, "dmsCB is invalid" ) ;

      const CHAR * collectionShortName = mbContext->mb()->_collectionName ;
      const rtnCLSetAttributeTask * localTask = NULL ;

      localTask = dynamic_cast<const rtnCLSetAttributeTask *>( task ) ;
      PD_CHECK( NULL != localTask, SDB_INVALIDARG, error, PDERROR,
                "Failed to get task" ) ;

      PD_CHECK( mbContext->isMBLock( EXCLUSIVE ), SDB_SYS, error, PDERROR,
                "Failed to get mbContext: should be exclusive locked" ) ;

      // $sharding index
      if ( localTask->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
      {
         OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
         const rtnCLShardingArgument & argument =
                                             localTask->getShardingArgument() ;
         rc = _rtnCollectionSetSharding( collection, argument, cb, dpsCB,
                                         mbContext, su, pResult ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set sharding on collection [%s], "
                      "rc: %d", collection, rc ) ;
      }

      // Compress
      if ( localTask->containCompressArgument() )
      {
         const rtnCLCompressArgument & compressArgument =
                                             localTask->getCompressArgument() ;
         rc = _rtnCollectionSetCompress( collectionShortName, compressArgument,
                                         cb, mbContext, su, dmsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set compressor on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

      // Ext options
      if ( localTask->containExtOptionArgument() )
      {
         rc = _rtnCollectionSetExtOptions( collectionShortName,
                                           localTask->getExtOptionArgument(),
                                           cb, mbContext, su ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set capped options on "
                      "collection [%s], rc: %d", collection, rc ) ;
      }

      // Autoincrement options
      // do nothing
      if ( localTask->containAutoincArgument() )
      {
         rc = SDB_OK ;
      }

      // Strict data mode
      if ( localTask->testArgumentMask( UTIL_CL_STRICTDATAMODE_FIELD ) )
      {
         rc = su->setCollectionStrictDataMode( collectionShortName,
                                               localTask->isStrictDataMode(),
                                               mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set strict data mode "
                      "on collection [%s], rc: %d", collection, rc ) ;
      }

      // $id index
      if ( localTask->testArgumentMask( UTIL_CL_AUTOIDXID_FIELD ) )
      {
         OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
         if ( localTask->isAutoIndexID() )
         {
            rc = _rtnCreateIDIndex( collection, 0, cb, dpsCB, mbContext,
                                    su, pResult ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to create id index on collection "
                         "[%s], rc: %d", collection, rc ) ;
         }
         else
         {
            rc = _rtnDropIDIndex( collection, cb, dpsCB, mbContext, su ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to drop id index on collection "
                         "[%s], rc: %d", collection, rc ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCLSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERCSSETATTR, "_rtnAlterCSSetAttributes" )
   INT32 _rtnAlterCSSetAttributes ( const CHAR * collectionSpace,
                                    const rtnAlterTask * task,
                                    _dmsStorageUnit * su,
                                    _pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTERCSSETATTR ) ;

      SDB_ASSERT( NULL != su, "su is invalid" ) ;

      const rtnCSSetAttributeTask * localTask = NULL ;
      INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;

      if ( !task->testArgumentMask( UTIL_CS_LOBPAGESIZE_FIELD ) )
      {
         goto done ;
      }

      localTask = dynamic_cast< const rtnCSSetAttributeTask * >( task ) ;
      PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                "Failed to get set attributes task on collection space [%s]",
                collectionSpace ) ;

      lobPageSize = localTask->getLobPageSize() ;

      if ( lobPageSize != su->getLobPageSize() )
      {
         rc = su->setLobPageSize( (UINT32)lobPageSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to set LOB page size on "
                      "storage unit [%s], rc: %d", collectionSpace, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTERCSSETATTR, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTER2DPSLOG, "_rtnAlter2DPSLog" )
   INT32 _rtnAlter2DPSLog ( const CHAR * name,
                            const rtnAlterTask * task,
                            const rtnAlterOptions * options,
                            _pmdEDUCB * cb,
                            _dpsLogWrapper * dpsCB,
                            _dmsMBContext * mbContext,
                            _dmsStorageUnit * su,
                            DMS_FILE_TYPE dpsType )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNALTER2DPSLOG ) ;

      SDB_ASSERT( NULL != name, "name is invalid" ) ;
      SDB_ASSERT( NULL != task, "task is invalid" ) ;

      if ( NULL != dpsCB )
      {
         UINT32 csLID = su->LogicalCSID() ;
         UINT32 clLID = NULL != mbContext ? mbContext->clLID() : ~0 ;

         dpsMergeInfo info ;
         info.setInfoEx( csLID, clLID, DMS_INVALID_EXTENT, cb ) ;

         dpsLogRecord & record = info.getMergeBlock().record() ;

         BSONObj alterOptions ;
         BSONObj alterObject ;
         RTN_ALTER_OBJECT_TYPE objType = task->getObjectType() ;

         if ( NULL != options )
         {
            alterOptions = options->toBSON() ;
         }
         alterObject = task->toBSON( name, alterOptions ) ;

         rc = dpsAlter2Record( name, objType, alterObject, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build alter log, rc: %d", rc ) ;

         rc = dpsCB->prepare( info ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare alter log, "
                      "rc: %d", rc ) ;

         dpsCB->writeData( info ) ;

         if ( NULL != mbContext && DMS_FILE_EMPTY != dpsType )
         {
            mbContext->mbStat()->updateLastLSNWithComp(
                        cb->getEndLsn(), dpsType, cb->isDoRollback() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB__RTNALTER2DPSLOG, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCMD, "rtnAlterCommand" )
   INT32 rtnAlterCommand ( const CHAR * name,
                           RTN_ALTER_OBJECT_TYPE objectType,
                           BSONObj alterObject,
                           _pmdEDUCB * cb,
                           _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCMD ) ;

      rtnAlterJob alterJob ;

      rc = alterJob.initialize( NULL, objectType, alterObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize alter job, rc: %d", rc ) ;

      PD_CHECK( 0 == ossStrcmp( name, alterJob.getObjectName() ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize alter job, rc: %d", rc ) ;

      if ( !alterJob.isEmpty() )
      {
         const rtnAlterOptions * options = alterJob.getOptions() ;
         const RTN_ALTER_TASK_LIST & alterTasks = alterJob.getAlterTasks() ;

         for ( RTN_ALTER_TASK_LIST::const_iterator iter = alterTasks.begin() ;
               iter != alterTasks.end() ;
               ++ iter )
         {
            const rtnAlterTask * task = ( *iter ) ;

            rc = rtnAlter( name, task, options, cb, dpsCB ) ;

            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to run alter task [%s], rc: %d",
                       task->getActionName(), rc ) ;
               if ( options->isIgnoreException() )
               {
                  rc = SDB_OK ;
                  continue ;
               }
               else
               {
                  goto error ;
               }
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNALTERCMD, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTER, "rtnAlter" )
   INT32 rtnAlter ( const CHAR * name,
                    const rtnAlterTask * task,
                    const rtnAlterOptions * options,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB,
                    utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTER ) ;

      switch ( task->getActionType() )
      {
         // alter collection actions
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         case RTN_ALTER_CL_DROP_ID_INDEX :
         case RTN_ALTER_CL_ENABLE_SHARDING :
         case RTN_ALTER_CL_DISABLE_SHARDING :
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            rc = rtnAlterCollection( name, task, options, cb, dpsCB, pResult ) ;
            break ;
         }
         // alter collection space actions
         case RTN_ALTER_CS_SET_DOMAIN :
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         case RTN_ALTER_CS_ENABLE_CAPPED :
         case RTN_ALTER_CS_DISABLE_CAPPED :
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            rc = rtnAlterCollectionSpace( name, task, options, cb, dpsCB ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s], rc: %d",
                   task->getActionName(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_RTNALTER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCHECKALTERCOLLECTION, "rtnCheckAlterCollection" )
   INT32 rtnCheckAlterCollection ( const CHAR * collection,
                                   const rtnAlterTask * task,
                                   _pmdEDUCB * cb,
                                   _dmsMBContext * mbContext,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCHECKALTERCOLLECTION ) ;

      switch ( task->getActionType() )
      {
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            const rtnCLEnableShardingTask * localTask =
                  dynamic_cast<const rtnCLEnableShardingTask *>( task ) ;
            if ( localTask->testArgumentMask( UTIL_CL_SHDKEY_FIELD ) )
            {
               rc = _rtnCollectionCheckSharding( collection,
                                                 localTask->getShardingArgument(),
                                                 cb, mbContext, su ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to check sharding for "
                            "collection [%s], rc: %d", collection, rc ) ;
            }
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = su->canSetCollectionCompressor( mbContext ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check compress for "
                         "collection [%s], rc: %d", collection, rc ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            rc = _rtnAlterCLCheckAttributes( collection, task, mbContext, su,
                                             cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check attributes for "
                         "collection [%s], rc: %d", collection, rc ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            //TODO: data group should do nothing
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCHECKALTERCOLLECTION, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCOLLECTION, "rtnAlterCollection" )
   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCOLLECTION ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;
      const CHAR * clShortName = NULL ;
      dmsMBContext * mbContext = NULL ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = rtnResolveCollectionNameAndLock ( collection, dmsCB, &su,
                                             &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                   collection, rc ) ;

      rc = su->data()->getMBContext( &mbContext, clShortName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock mb context [%s], rc: %d",
                   collection, rc ) ;

      rc = rtnCheckAlterCollection( collection, task, cb, mbContext, su,
                                    dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check alter collection [%s], rc: %d",
                   collection, rc ) ;

      rc = rtnAlterCollection( collection, task, options, cb, dpsCB, mbContext,
                               su, dmsCB, pResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to alter collection [%s], rc: %d",
                   collection, rc ) ;

   done :
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCOLLECTION, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCOLLECTION_MB, "rtnAlterCollection" )
   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              _dmsMBContext * mbContext,
                              _dmsStorageUnit * su,
                              _SDB_DMSCB * dmsCB,
                              utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCOLLECTION_MB ) ;

      DMS_FILE_TYPE dpsType = DMS_FILE_EMPTY ;

      switch ( task->getActionType() )
      {
         case RTN_ALTER_CL_CREATE_ID_INDEX :
         {
            OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
            const rtnCLCreateIDIndexTask * localTask =
                        dynamic_cast<const rtnCLCreateIDIndexTask *>( task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get create id index task" ) ;
            rc = _rtnCreateIDIndex( collection, localTask->getSortBufferSize(),
                                    cb, dpsCB, mbContext, su, pResult ) ;
            break ;
         }
         case RTN_ALTER_CL_DROP_ID_INDEX :
         {
            OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
            rc = _rtnDropIDIndex( collection, cb, dpsCB, mbContext, su ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_SHARDING :
         {
            OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
            const rtnCLEnableShardingTask * localTask =
                  dynamic_cast<const rtnCLEnableShardingTask *>( task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get alter task" ) ;
            rc = _rtnCollectionSetSharding( collection,
                                            localTask->getShardingArgument(),
                                            cb, dpsCB, mbContext, su,
                                            pResult ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_SHARDING :
         {
            OSS_BIT_SET( dpsType, DMS_FILE_IDX ) ;
            rtnCLShardingArgument argument ;
            argument.setEnsureShardingIndex( FALSE ) ;
            rc = _rtnCollectionSetSharding( collection, argument, cb, dpsCB,
                                            mbContext, su, pResult ) ;
            break ;
         }
         case RTN_ALTER_CL_ENABLE_COMPRESS :
         {
            const rtnCLEnableCompressTask * localTask =
                  dynamic_cast<const rtnCLEnableCompressTask *>( task ) ;
            PD_CHECK( NULL != localTask, SDB_SYS, error, PDERROR,
                      "Failed to get enable compress task" ) ;
            rc = _rtnCollectionSetCompress( collection,
                                            localTask->getCompressArgument(),
                                            cb, mbContext, su, dmsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_DISABLE_COMPRESS :
         {
            rc = _rtnCollectionSetCompress( collection, UTIL_COMPRESSOR_INVALID,
                                            cb, mbContext, su, dmsCB ) ;
            break ;
         }
         case RTN_ALTER_CL_SET_ATTRIBUTES :
         {
            rc = _rtnAlterCLSetAttributes( collection, task, cb, dpsCB,
                                           mbContext, su, dmsCB, dpsType,
                                           pResult ) ;
            break ;
         }
         case RTN_ALTER_CL_CREATE_AUTOINC_FLD :
         case RTN_ALTER_CL_DROP_AUTOINC_FLD :
         {
            //TODO: data group should do nothing
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s] on "
                   "collection [%s], rc: %d", task->getActionName(),
                   collection, rc ) ;

   done :
      if ( SDB_OK == rc )
      {
         rc = _rtnAlter2DPSLog( collection, task, options, cb, dpsCB,
                                mbContext, su, dpsType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write DPS log, rc: %d", rc ) ;
         }
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCOLLECTION_MB, rc ) ;
      return rc ;

   error :
      if ( options->isIgnoreException() )
      {
         PD_LOG( PDWARNING, "Ignored failure for alter task [%s] on collection "
                 "[%s], rc: %d", task->getActionName(), collection, rc ) ;
         rc = SDB_OK ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCOLLECTIONSPACE, "rtnAlterCollectionSpace" )
   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCOLLECTIONSPACE ) ;

      SDB_DMSCB * dmsCB = sdbGetDMSCB() ;

      dmsStorageUnit * su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BOOLEAN writable = FALSE ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      rc = dmsCB->nameToSUAndLock( collectionSpace, suID, &su, EXCLUSIVE,
                                   OSS_ONE_SEC ) ;
      if ( SDB_TIMEOUT == rc )
      {
         rc = SDB_LOCK_FAILED ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to lock storage unit [%s], rc: %d",
                   collectionSpace, rc ) ;

      rc = rtnAlterCollectionSpace( collectionSpace, task, options, cb, dpsCB,
                                    su, dmsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to alter collection space [%s], "
                   "rc: %d", collectionSpace, rc ) ;

   done :
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID, EXCLUSIVE ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCOLLECTIONSPACE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNALTERCOLLECTIONSPACE_SU, "rtnAlterCollectionSpace" )
   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNALTERCOLLECTIONSPACE_SU ) ;

      switch ( task->getActionType() )
      {
         case RTN_ALTER_CS_SET_ATTRIBUTES :
         {
            PD_CHECK( !task->testArgumentMask( UTIL_CS_CAPPED_FIELD |
                                               UTIL_CS_PAGESIZE_FIELD ),
                      SDB_DMS_CS_NOT_EMPTY, error, PDERROR,
                      "Failed to check collection space, the collection space "
                      "is not empty" ) ;
            rc = _rtnAlterCSSetAttributes( collectionSpace, task, su, cb ) ;
            break ;
         }
         case RTN_ALTER_CS_SET_DOMAIN :
         case RTN_ALTER_CS_REMOVE_DOMAIN :
         {
            // do nothing
            rc = SDB_OK ;
            break ;
         }
         case RTN_ALTER_CS_ENABLE_CAPPED :
         case RTN_ALTER_CS_DISABLE_CAPPED :
         {
            rc = SDB_DMS_CS_NOT_EMPTY ;
            PD_LOG( PDERROR, "Failed to check collection space, the collection "
                    "space is not empty" ) ;
            break ;
         }
         default :
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to run alter task [%s] on collection "
                   "space [%s], rc: %d", task->getActionName(),
                   collectionSpace, rc ) ;

   done :
      if ( SDB_OK == rc )
      {
         rc = _rtnAlter2DPSLog( collectionSpace, task, options, cb, dpsCB,
                                NULL, su, DMS_FILE_EMPTY ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write DPS log, rc: %d", rc ) ;
         }
      }
      PD_TRACE_EXITRC( SDB_RTNALTERCOLLECTIONSPACE_SU, rc ) ;
      return rc ;

   error :
      if ( options->isIgnoreException() )
      {
         PD_LOG( PDWARNING, "Ignored failure for alter task [%s] on collection "
                 "space [%s], rc: %d", task->getActionName(), collectionSpace,
                 rc ) ;
         rc = SDB_OK ;
      }
      goto done ;
   }

}
