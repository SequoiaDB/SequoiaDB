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

   Source File Name = rtnExtDataProcessor.hpp

   Descriptive Name = External data processor for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnExtDataProcessor.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"
#include "rtnTrace.hpp"
#include "dmsStorageDataCapped.hpp"

// Currently we set the size limit of capped collection to 30GB. This may change
// in the future.
#define RTN_CAPPED_CL_MAXSIZE       ( 30 * 1024 * 1024 * 1024LL )
#define RTN_CAPPED_CL_MAXRECNUM     0
#define RTN_FIELD_NAME_RID          "_rid"
#define RTN_FIELD_NAME_RID_NEW      "_ridNew"
#define RTN_FIELD_NAME_SOURCE       "_source"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR, "_rtnExtDataProcessor::_rtnExtDataProcessor" )
   _rtnExtDataProcessor::_rtnExtDataProcessor()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR ) ;
      _stat = RTN_EXT_PROCESSOR_INVALID ;
      _su = NULL ;
      _id = RTN_EXT_PROCESSOR_INVALID_ID ;
      ossMemset( _cappedCSName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      _needUpdateLSN = FALSE ;
      _freeSpace = 0 ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__RTNEXTDATAPROCESSOR ) ;
   }

   _rtnExtDataProcessor::~_rtnExtDataProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_INIT, "_rtnExtDataProcessor::init" )
   INT32 _rtnExtDataProcessor::init( INT32 id, const CHAR *csName,
                                     const CHAR *clName,
                                     const CHAR *idxName,
                                     const CHAR *targetName,
                                     const BSONObj &idxKeyDef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_INIT ) ;

      SDB_ASSERT( csName && clName && idxName && targetName,
                  "Names should not be NULL") ;

      rc = _meta.init( csName, clName, idxName, targetName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor meta init failed[ %d ]", rc ) ;

      rc = setTargetNames( targetName ) ;
      PD_RC_CHECK( rc, PDERROR, "Set target names failed[ %d ]", rc ) ;
      _id = id ;
      _stat = RTN_EXT_PROCESSOR_CREATING ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_INIT, rc ) ;
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_RESET, "_rtnExtDataProcessor::reset" )
   void _rtnExtDataProcessor::reset()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_RESET ) ;
      _stat = RTN_EXT_PROCESSOR_INVALID ;
      _su = NULL ;
      _needUpdateLSN = FALSE ;
      _freeSpace = 0 ;
      _id = RTN_EXT_PROCESSOR_INVALID_ID ;
      _meta.reset() ;
      ossMemset( _cappedCSName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_RESET ) ;
   }

   INT32 _rtnExtDataProcessor::getID() const
   {
      return _id ;
   }

   void _rtnExtDataProcessor::setStat( rtnExtProcessorStat stat )
   {
      _stat = stat ;
   }

   rtnExtProcessorStat _rtnExtDataProcessor::stat() const
   {
      return _stat ;
   }

   INT32 _rtnExtDataProcessor::active()
   {
      _stat = RTN_EXT_PROCESSOR_NORMAL ;

      return SDB_OK ;
   }

   BOOLEAN _rtnExtDataProcessor::isActive() const
   {
      return ( RTN_EXT_PROCESSOR_NORMAL == _stat ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA, "_rtnExtDataProcessor::updateMeta" )
   void _rtnExtDataProcessor::updateMeta( const CHAR *csName,
                                          const CHAR *clName,
                                          const CHAR *idxName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA ) ;
      if ( csName && ( ossStrcmp( csName, _meta._csName ) != 0 ) )
      {
         ossStrncpy( _meta._csName, csName, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      }

      if ( clName && ( ossStrcmp( clName, _meta._clName ) != 0 ) )
      {
         ossStrncpy( _meta._clName, clName, DMS_COLLECTION_NAME_SZ + 1 ) ;
      }

      if ( idxName && ( ossStrcmp( idxName, _meta._idxName ) != 0 ) )
      {
         ossStrncpy( _meta._idxName, idxName, IXM_INDEX_NAME_SIZE + 1 ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES, "_rtnExtDataProcessor::setTargetNames" )
   INT32 _rtnExtDataProcessor::setTargetNames( const CHAR *extName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES ) ;
      SDB_ASSERT( extName, "cs name is NULL" ) ;

      if ( ossStrlen( extName ) > DMS_COLLECTION_SPACE_NAME_SZ )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "External name too long: %s", extName ) ;
         goto error ;
      }

      ossStrncpy( _cappedCSName, extName, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossSnprintf( _cappedCLName, sizeof( _cappedCLName ),
                   "%s.%s", extName, extName ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_SETTARGETNAMES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPARE, "_rtnExtDataProcessor::prepare" )
   INT32 _rtnExtDataProcessor::prepare( DMS_EXTOPR_TYPE oprType,
                                        rtnExtOprData *oprData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPARE ) ;

      switch ( oprType )
      {
      case DMS_EXTOPR_TYPE_INSERT:
         rc = _prepareInsert( oprData ) ;
         break;
      case DMS_EXTOPR_TYPE_DELETE:
         rc = _prepareDelete( oprData ) ;
         break ;
      case DMS_EXTOPR_TYPE_UPDATE:
         rc = _prepareUpdate( oprData ) ;
         break ;
      default:
         SDB_ASSERT( FALSE, "Context type is wrong" ) ;
         rc = SDB_SYS ;
         break ;
      }
      PD_RC_CHECK( rc, PDERROR, "Prepare external operation failed[%d]", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSDML, "_rtnExtDataProcessor::processDML" )
   INT32 _rtnExtDataProcessor::processDML( BSONObj &oprRecord, pmdEDUCB *cb,
                                           BOOLEAN isRollback,
                                           SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSDML ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = _spaceCheck( oprRecord.objsize() ) ;
      PD_RC_CHECK( rc, PDERROR, "Space check failed[%d]", rc ) ;

      if ( isRollback )
      {
         PD_LOG( PDDEBUG, "In rollback progess, pop last record in capped "
                          "collection: %s", _cappedCLName ) ;
         rc = rtnPopCommand( _cappedCLName, 1, cb, dmsCB, dpsCB, -1, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Pop last record from collection[%s] "
                                   "failed: %d", _cappedCLName, rc ) ;
      }
      else
      {
         rc = rtnInsert( _cappedCLName, oprRecord, 1, 0,
                         cb, dmsCB, dpsCB, 1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Insert operation record into collection[%s] "
                                   "failed[%d]", _cappedCLName, rc ) ;
      }

      _needUpdateLSN = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSDML, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE, "_rtnExtDataProcessor::processTruncate" )
   INT32 _rtnExtDataProcessor::processTruncate( pmdEDUCB *cb,
                                                BOOLEAN needChangeCLID,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnTruncCollectionCommand( _cappedCLName, cb, dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate capped collection[%s] failed[%d]",
                   _cappedCLName,  rc ) ;

      // If the logical id of the original collection dose not change, insert a
      // reset record into the capped collection, for the adapter need
      // information to know the change of the collection and index.
      if ( !needChangeCLID )
      {
         rc = _addRebuildRecord( cb, dpsCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Add rebuild record failed[%d]", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PROCESSTRUNCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP1, "_rtnExtDataProcessor::doDropP1" )
   INT32 _rtnExtDataProcessor::doDropP1( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP1 ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = dmsCB->dropCollectionSpaceP1( _cappedCSName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL, "_rtnExtDataProcessor::doDropP1Cancel" )
   INT32 _rtnExtDataProcessor::doDropP1Cancel( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = dmsCB->dropCollectionSpaceP1Cancel( _cappedCSName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Cancel dropping collection space[ %s ] in "
                   "phase 1 failed[ %d ]", _cappedCSName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP1CANCEL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DODROPP2, "_rtnExtDataProcessor::doDropP2" )
   INT32 _rtnExtDataProcessor::doDropP2( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DODROPP2 ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = dmsCB->dropCollectionSpaceP2( _cappedCSName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnExtDataProcessor::doLoad()
   {
      // TODO:YSD
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD, "_rtnExtDataProcessor::doUnload" )
   INT32 _rtnExtDataProcessor::doUnload( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnUnloadCollectionSpace( _cappedCSName, cb, dmsCB ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         PD_LOG( PDWARNING, "Capped collection space[ %s ] not found when "
                 "unload", _cappedCSName ) ;
         rc = SDB_OK ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Unload capped collection space failed[ %d ]",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOREBUILD, "_rtnExtDataProcessor::doRebuild" )
   INT32 _rtnExtDataProcessor::doRebuild( pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DOREBUILD ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DB_STATUS dbStatus = krcb->getDBStatus() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;

      // In case of rebuilding, we don't know whether the capped collections are
      // valid or not. So we directly drop and re-create the capped collection
      // again.
      if ( SDB_DB_REBUILDING == dbStatus )
      {
         rc = rtnDropCollectionSpaceCommand( _cappedCSName, cb,
                                             dmsCB, dpsCB, TRUE ) ;
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            PD_LOG( PDINFO, "Capped collection space[ %s ] not found when "
                    "trying to drop it", _cappedCSName ) ;
         }
         else
         {
            PD_RC_CHECK( rc, PDERROR, "Drop collectionspace[ %s ] failed[ %d ]",
                         _cappedCSName, rc);
         }
      }
      else if ( SDB_DB_FULLSYNC == dbStatus )
      {
         goto done ;
      }

      rc = _prepareCSAndCL( _cappedCSName, _cappedCLName, cb, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection[ %s ] "
                   "failed[ %d ]", _cappedCLName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOREBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // Update the commit LSN information for the capped collection.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DONE, "_rtnExtDataProcessor::done" )
   INT32 _rtnExtDataProcessor::done( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DONE ) ;
      dmsMBContext *context = NULL ;

      if ( _needUpdateLSN )
      {
         SDB_ASSERT( _su, "Storage unit is NULL") ;
         rc = _su->data()->getMBContext( &context, _getExtCLShortName(), -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get mbcontext for collection[ %s ] "
                                   "failed[ %d ]", _cappedCLName, rc ) ;

         SDB_ASSERT( context, "mbContext should not be NULL" ) ;

         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_DATA ) ;
      }

      _needUpdateLSN = FALSE ;

   done:
      if ( context )
      {
         _su->data()->releaseMBContext( context ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_ABORT, "_rtnExtDataProcessor::abort" )
   INT32 _rtnExtDataProcessor::abort()
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_ABORT ) ;
      _needUpdateLSN = FALSE ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_ABORT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY, "_rtnExtDataProcessor::isOwnedBy" )
   BOOLEAN _rtnExtDataProcessor::isOwnedBy( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName ) const
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      BOOLEAN result = FALSE ;

      SDB_ASSERT( csName, "CS name can not be NULL" ) ;

      if ( 0 == ossStrcmp( csName, _meta._csName ) )
      {
         result = TRUE ;
      }

      if ( clName && 0 != ossStrcmp( clName, _meta._clName ) && result )
      {
         result = FALSE ;
      }

      if ( idxName && 0 != ossStrcmp( idxName, _meta._idxName )
           && result )
      {
         result = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      return result ;
   }

   BOOLEAN _rtnExtDataProcessor::isOwnedByExt( const CHAR *targetName ) const
   {
      return ( 0 == ossStrcmp( targetName, _meta._targetName ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__GENRECORDBYKEYSET, "_rtnExtDataProcessor::_genRecordByKeySet" )
   INT32 _rtnExtDataProcessor::_genRecordByKeySet( const BSONObjSet &keySet,
                                                   BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__GENRECORDBYKEYSET ) ;
      SDB_ASSERT( !keySet.empty(), "Key set is empty") ;

      try
      {
         if ( 1 == keySet.size() )
         {
            // Key set size is 1, so no array.
            record = *( keySet.begin() ) ;
         }
         else
         {
            // One of the index field is of type array. Resume the array from
            // the keys. Compare the elements of the first and second key. If
            // they are not the same, that's the array field.
            BSONObjBuilder builder ;
            BOOLEAN found = FALSE ;
            BSONObjIterator itrFirst( *(keySet.begin()) ) ;
            BSONObjIterator itrSecond( *(++keySet.begin()) ) ;

            while ( itrFirst.more() )
            {
               BSONElement lNextEle = itrFirst.next() ;
               BSONElement rNextEle = itrSecond.next() ;
               if ( found || 0 == lNextEle.woCompare( rNextEle, true) )
               {
                  // Same, it's not the array field.
                  builder.append( lNextEle ) ;
               }
               else
               {
                  // Found the array field.
                  const CHAR *arrField = lNextEle.fieldName() ;
                  BSONArrayBuilder arrBuilder( builder.subarrayStart( arrField ) ) ;
                  arrBuilder.append( lNextEle ) ;
                  arrBuilder.append( rNextEle ) ;
                  // Get the array field from all key record.
                  BSONObjSet::iterator itr = keySet.begin() ;
                  // Skip the first and second key.
                  std::advance( itr, 2 ) ;
                  while ( itr != keySet.end() )
                  {
                     arrBuilder.append( itr->getField( arrField ) ) ;
                     ++itr ;
                  }
                  arrBuilder.done() ;
                  found = TRUE ;
               }
            }
            record = builder.obj() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__GENRECORDBYKEYSET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, "_rtnExtDataProcessor::_prepareRecord" )
   INT32 _rtnExtDataProcessor::_prepareRecord( _rtnExtOprType oprType,
                                               BSONObj &recordObj,
                                               const BSONObjSet *keySet,
                                               const BSONElement &idEle,
                                               const BSONElement *newIdEle )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD ) ;

      try
      {
         BSONObjBuilder builder ;

         if ( RTN_EXT_INVALID == oprType )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Operation type is invalid[%d]", oprType ) ;
            goto error ;
         }

         // Append operation type.
         builder.append( FIELD_NAME_TYPE, oprType ) ;
         if ( RTN_EXT_DUMMY != oprType )
         {
            SDB_ASSERT( !idEle.eoo(), "OID is invalid" ) ;
            // Append the original oid.
            builder.appendAs( idEle, RTN_FIELD_NAME_RID ) ;

            // Add source data for the operation. Only these operation need the
            // source data.
            if ( RTN_EXT_INSERT == oprType || RTN_EXT_UPDATE == oprType ||
                 RTN_EXT_UPDATE_WITH_ID == oprType )
            {
               BSONObj record ;
               if ( !keySet )
               {
                  rc = SDB_SYS ;
                  PD_LOG( PDERROR, "Index key set is empty" ) ;
                  goto error ;
               }

               rc = _genRecordByKeySet( *keySet, record ) ;
               PD_RC_CHECK( rc, PDERROR, "Generate record by key set failed[%d]",
                            rc ) ;
               builder.append( RTN_FIELD_NAME_SOURCE, record ) ;

               if ( RTN_EXT_UPDATE_WITH_ID == oprType )
               {
                  builder.appendAs( *newIdEle, RTN_FIELD_NAME_RID_NEW ) ;
               }
            }
         }
         recordObj = builder.obj() ;
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL, "_rtnExtDataProcessor::_prepareCSAndCL" )
   INT32 _rtnExtDataProcessor::_prepareCSAndCL( const CHAR *csName,
                                                const CHAR *clName,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL ) ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      BOOLEAN csCreated = FALSE ;
      BSONObjBuilder builder ;
      BSONObj extOptions ;

      rc = rtnCreateCollectionSpaceCommand( csName, cb, dmsCB, dpsCB,
                                            UTIL_UNIQUEID_NULL,
                                            DMS_PAGE_SIZE_DFT,
                                            DMS_DO_NOT_CREATE_LOB,
                                            DMS_STORAGE_CAPPED, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection space failed[ %d ]",
                   rc ) ;

      csCreated = TRUE ;

      try
      {
         builder.append( FIELD_NAME_SIZE, RTN_CAPPED_CL_MAXSIZE ) ;
         builder.append( FIELD_NAME_MAX, RTN_CAPPED_CL_MAXRECNUM ) ;
         // Set the OverWrite option as false.
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      rc = rtnCreateCollectionCommand( clName,
                                       DMS_MB_ATTR_NOIDINDEX | DMS_MB_ATTR_CAPPED,
                                       cb, dmsCB, dpsCB, UTIL_UNIQUEID_NULL,
                                       UTIL_COMPRESSOR_INVALID, 0,
                                       TRUE, &extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection[ %s ] failed[ %d ]",
                   clName, rc ) ;
      rtnCB->incTextIdxVersion() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPARECSANDCL, rc ) ;
      return rc ;
   error:
      if ( csCreated )
      {
         INT32 rcTmp = rtnDropCollectionSpaceCommand( csName, cb, dmsCB,
                                                      dpsCB, TRUE ) ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Drop collectionspace[ %s ] failed[ %d ]",
                    csName, rcTmp ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPAREINSERT, "_rtnExtDataProcessor::_prepareInsert" )
   INT32 _rtnExtDataProcessor::_prepareInsert( rtnExtOprData *oprData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPAREINSERT ) ;

      const BSONObj &origRecord = oprData->getOrigRecord() ;
      if ( origRecord.isEmpty() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Original record is not found in the context" ) ;
         goto error ;
      }

      try
      {
         BSONObjSet keySet ;
         BSONElement arrayEle ;
         BSONElement oidEle ;

         // Record without field "_id" will be ignored in text index.
         oidEle = origRecord.getField( DMS_ID_KEY_NAME ) ;
         if ( oidEle.eoo() )
         {
            PD_LOG( PDWARNING, "Record has no _id field, will not be indexed "
                    "by text index: %s",
                    origRecord.toString(false, true).c_str() ) ;
            goto done ;
         }

         {
            BSONObj record ;
            // Get index keys from the record. If none is there, the record will
            // be ignored in text index.
            ixmIndexKeyGen keygen( _meta._idxKeyDef ) ;
            rc = keygen.getKeys( origRecord, keySet, &arrayEle, TRUE, TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Generate key from object[%s] failed[%d]",
                       origRecord.toString(false, true).c_str(), rc ) ;
               goto error ;
            }

            if ( 0 == keySet.size() )
            {
               goto done ;
            }

            rc = _prepareRecord( RTN_EXT_INSERT, record, &keySet, oidEle ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare insert operation record "
                         "failed[%d]. Original record: %s",
                         rc, origRecord.toString(false, true).c_str() ) ;
            rc = oprData->saveOprRecord( (void *)this, record, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Set insert operation record in context "
                         "failed[%d]", rc ) ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPAREINSERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPAREDELETE, "_rtnExtDataProcessor::_prepareDelete" )
   INT32 _rtnExtDataProcessor::_prepareDelete( rtnExtOprData *oprData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPAREDELETE ) ;

      const BSONObj &origRecord = oprData->getOrigRecord() ;
      if ( origRecord.isEmpty() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Original record is not found in the context" ) ;
         goto error ;
      }

      try
      {
         BSONObjSet keySet ;
         BSONElement arrayEle ;
         BSONElement oidEle ;

         oidEle = origRecord.getField( DMS_ID_KEY_NAME ) ;
         if ( oidEle.eoo() )
         {
            // No _id in the record. It should not have been indexed.
            goto done ;
         }

         {
            BSONObj record ;
            ixmIndexKeyGen keygen( _meta._idxKeyDef ) ;
            rc = keygen.getKeys( origRecord, keySet, &arrayEle, FALSE, TRUE ) ;
            if ( rc )
            {
               // The record which contains multiple fields of type array may be
               // insertted before the text index is created. In that case, we
               // should be sure it can be deleted.
               if ( SDB_IXM_MULTIPLE_ARRAY != rc )
               {
                  PD_LOG( PDERROR, "Generate key from object[%s] failed[%d]",
                          origRecord.toString(false, true).c_str(), rc ) ;
                  goto error ;
               }
               rc = SDB_OK ;
            }
            else if ( 0 == keySet.size() )
            {
               // No index keys in the record. It should be skipped.
               goto done ;
            }

            // For delete operation, only the operation type and oid are
            // necessary.
            rc = _prepareRecord( RTN_EXT_DELETE, record, NULL, oidEle ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare delete operation record "
                         "failed[%d]. Original record: %s",
                         rc, origRecord.toString(false, true).c_str() ) ;
            rc = oprData->saveOprRecord( (void *)this, record, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Set insert operation record in context "
                         "failed[%d]", rc ) ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPAREDELETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPAREUPDATE, "_rtnExtDataProcessor::_prepareUpdate" )
   INT32 _rtnExtDataProcessor::_prepareUpdate( rtnExtOprData *oprData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPAREUPDATE ) ;

      const BSONObj &origRecord = oprData->getOrigRecord() ;
      const BSONObj &newRecord = oprData->getNewRecord() ;
      if ( origRecord.isEmpty() || newRecord.isEmpty() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Original or new record is not found in the "
                 "context" ) ;
         goto error ;
      }

      try
      {
         BSONObjSet keySet ;
         BSONObjSet keySetNew ;
         BSONElement arrayEle ;
         ixmIndexKeyGen keygen( _meta._idxKeyDef ) ;
         rc = keygen.getKeys( origRecord, keySet, &arrayEle, TRUE, TRUE ) ;
         if ( rc )
         {
            // The record which contains multiple fields of type array may be
            // insertted before the text index is created. In that case, we
            // should be sure it can be updated.
            if ( SDB_IXM_MULTIPLE_ARRAY != rc )
            {
               PD_LOG( PDERROR, "Generate key from object[%s] failed[%d]",
                       origRecord.toString(false, true).c_str(), rc ) ;
               goto error ;
            }
            rc = SDB_OK ;
         }

         // If the new record contains multiple arraies( included in the text
         // index), report error.
         rc = keygen.getKeys( newRecord, keySetNew, NULL, TRUE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate key from object[%s] failed[%d]",
                      newRecord.toString(false, true).c_str(), rc ) ;
         {
            BSONObj record ;
            BSONElement oidEle ;
            BSONElement oidEleNew ;
            BOOLEAN idModified = FALSE ;
            BOOLEAN hasIndexed = FALSE ;
            BOOLEAN shouldIndex = FALSE ;
            _rtnExtOprType oprType = RTN_EXT_UPDATE ;

            oidEle = origRecord.getField( DMS_ID_KEY_NAME ) ;
            oidEleNew = newRecord.getField( DMS_ID_KEY_NAME ) ;
            idModified = oidEle.valuesEqual( oidEleNew ) ? FALSE : TRUE ;

            // Whether the old record has been indexed before.
            hasIndexed = ( !oidEle.eoo() && keySet.size() > 0 ) ? TRUE : FALSE ;
            // Whether the new record should be indexed this time.
            shouldIndex = ( !oidEleNew.eoo() && keySetNew.size() > 0 ) ?
                          TRUE : FALSE ;
            if ( !( hasIndexed || shouldIndex ) ||
                 ( !idModified && (keySet == keySetNew) ) )
            {
               goto done ;
            }

            if ( hasIndexed )
            {
               if ( shouldIndex )
               {
                  if ( idModified )
                  {
                     oprType = RTN_EXT_UPDATE_WITH_ID ;
                  }
               }
               else
               {
                  oprType = RTN_EXT_DELETE ;
               }
            }
            else if ( shouldIndex )
            {
               oprType = RTN_EXT_INSERT ;
               oidEle = oidEleNew ;
            }
            else
            {
               goto done ;
            }

            rc = _prepareRecord( oprType, record, &keySetNew,
                                 oidEle, &oidEleNew ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare operation record failed[%d]. "
                         "Original record: %s",
                         rc, origRecord.toString(false, true).c_str() ) ;
            rc = oprData->saveOprRecord( (void *)this, record, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Set insert operation record in context "
                         "failed[%d]", rc ) ;
         }
      }
      catch ( const std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPAREUPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME, "_rtnExtDataProcessor::_getExtCLShortName" )
   const CHAR* _rtnExtDataProcessor::_getExtCLShortName() const
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME ) ;
      UINT32 curPos = 0 ;
      UINT32 fullLen = (UINT32)ossStrlen( _cappedCLName ) ;
      const CHAR *clShortName = NULL ;

      if ( 0 == fullLen )
      {
         clShortName = NULL ;
         goto done ;
      }

      while ( ( curPos < fullLen ) && ( '.' != _cappedCLName[curPos] ) )
      {
         curPos++ ;
      }

      if ( ( curPos == fullLen ) || ( curPos == fullLen - 1 ) )
      {
         clShortName = NULL ;
         goto done ;
      }

      clShortName = _cappedCLName + curPos + 1 ;

   done:
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__GETEXTCLSHORTNAME ) ;
      return clShortName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__SPACECHECK, "_rtnExtDataProcessor::_spaceCheck" )
   INT32 _rtnExtDataProcessor::_spaceCheck( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__SPACECHECK ) ;
      if ( _freeSpace < size )
      {
         // If cached size is not enough, let's sync the information. The capped
         // collection may have been popped by the search engine adapter.
         rc = _updateSpaceInfo( _freeSpace ) ;
         PD_RC_CHECK( rc, PDERROR, "Update space information for collection"
                                   "[ %s ] failed[ %d ]",
                      _cappedCLName, rc ) ;

         // If space still not enough after update, report error.
         if ( _freeSpace < size )
         {
            rc = SDB_OSS_UP_TO_LIMIT ;
            PD_LOG( PDERROR, "Space not enough, request size[ %u ]", size ) ;
            goto error ;
         }
      }

      _freeSpace -= ossRoundUpToMultipleX( size, 4 ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__SPACECHECK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO, "_rtnExtDataProcessor::_updateSpaceInfo" )
   INT32 _rtnExtDataProcessor::_updateSpaceInfo( UINT64 &freeSpace )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO ) ;
      const dmsMBStatInfo *mbStat = NULL ;
      UINT64 remainSize = 0 ;
      UINT32 remainExtentNum = 0 ;
      dmsMBContext *context = NULL ;

      if ( !_su )
      {
         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         dmsStorageUnitID suID = DMS_INVALID_CS ;
         // Delay set in open text index case.
         rc = dmsCB->nameToSUAndLock( _cappedCSName, suID, &_su ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection space[ %s ] failed[ %d ]",
                      _cappedCSName, rc ) ;
         dmsCB->suUnlock( suID, SHARED ) ;
      }
      rc = _su->data()->getMBContext( &context, _getExtCLShortName(), -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Get mbcontext for collection[ %s ] "
                                "failed[ %d ]", _cappedCLName, rc ) ;

      mbStat = context->mbStat() ;

      remainSize = RTN_CAPPED_CL_MAXSIZE -
            ( mbStat->_totalDataPages << _su->data()->pageSize() ) ;

      remainExtentNum = remainSize / DMS_CAP_EXTENT_SZ ;
      remainSize -= ( remainExtentNum * DMS_EXTENT_METADATA_SZ ) ;

      freeSpace = mbStat->_totalDataFreeSpace + remainSize ;

   done:
      if ( context )
      {
         _su->data()->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__UPDATESPACEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__ADDREBUILDRECORD, "_rtnExtDataProcessor::_addRebuildRecord" )
   INT32 _rtnExtDataProcessor::_addRebuildRecord( pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__ADDREBUILDRECORD ) ;
      BSONObj record ;
      BSONElement dummyEle = BSONElement() ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = _prepareRecord( RTN_EXT_DUMMY,  record, NULL, dummyEle ) ;
      PD_RC_CHECK( rc, PDERROR, "Prepare rebuild record failed[%d]", rc ) ;

      rc = rtnInsert( _cappedCLName, record, 1, 0, cb, dmsCB,
                      dpsCB, 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert rebuild record into collection[%s] "
                   "failed[%d]", _cappedCLName, rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__ADDREBUILDRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDataProcessorMgr::_rtnExtDataProcessorMgr()
   : _number( 0 )
   {
   }

   _rtnExtDataProcessorMgr::~_rtnExtDataProcessorMgr()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR, "_rtnExtDataProcessorMgr::createProcessor" )
   INT32 _rtnExtDataProcessorMgr::createProcessor( const CHAR *csName,
                                                   const CHAR *clName,
                                                   const CHAR *idxName,
                                                   const CHAR *extName,
                                                   const BSONObj &idxKeyDef,
                                                   BOOLEAN newIndex,
                                                   pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB,
                                                   rtnExtDataProcessor *&processor )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR ) ;
      INT32 id = RTN_EXT_PROCESSOR_INVALID_ID ;
      rtnExtDataProcessor *processorLocal = NULL ;
      BOOLEAN metaLocked = FALSE ;

      // Check if the processor exists by try locking the processor.
      rc = _nameToProcessorAndLock( extName, &processorLocal ) ;
      if ( SDB_IXM_NOTEXIST != rc )
      {
         if ( processorLocal )
         {
            unlockProcessor( processorLocal ) ;
         }
         rc = SDB_IXM_EXIST ;
         PD_LOG( PDERROR, "Processor for index %s on collection %s.%s already"
                          " exists", idxName, csName, clName ) ;
         goto error ;
      }

      // Get the metadata lock, to avoid concurrent problem with other creating/
      // dropping text index operation.
      aquireMetaLock( EXCLUSIVE ) ;
      metaLocked = TRUE ;

      processorLocal = _getFirstFreeProcessor( &id ) ;
      if ( !processorLocal )
      {
         rc = SDB_DMS_MAX_INDEX ;
         PD_LOG( PDERROR, "Text index number has hit the limit" ) ;
         goto error ;
      }

      rc = processorLocal->init( id, csName, clName, idxName,
                                 extName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Init external data processor failed[ %d ]",
                   rc ) ;

      ++_number ;
      if ( newIndex )
      {
         // As doRebuild will create the capped collection space and capped
         // collection, locks of upper levels will be taken, e.g., cs mutex in
         // dmsCB. If we do this in the processor meta lock, it may result in
         // deadlock. Refer to defact SEQUOIADBMAINSTREAM-7224 for details.
         // After initialization, the processor is invisible to anyone else. So
         // it's safe to do the rebuild without any locks.
         releaseMetaLock( EXCLUSIVE ) ;
         metaLocked = FALSE ;
         rc = processorLocal->doRebuild( cb, dpsCB ) ;
         aquireMetaLock( EXCLUSIVE ) ;
         if ( rc )
         {
            --_number ;
            releaseMetaLock( EXCLUSIVE ) ;
            PD_LOG( PDERROR, "Rebuild of index failed[%d]. Collection space: "
                    "%s, collection: %s, index: %s", rc,
                    csName, clName, idxName ) ;
            goto error ;
         }
         metaLocked = TRUE ;
      }
      processorLocal->active() ;
      processor = processorLocal ;

   done:
      if ( metaLocked )
      {
         releaseMetaLock( EXCLUSIVE ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_CREATEPROCESSOR, rc ) ;
      return rc ;
   error:
      if ( processorLocal )
      {
         processorLocal->reset() ;
      }
      goto done ;
   }

   UINT32 _rtnExtDataProcessorMgr::number()
   {
      ossScopedLock lock( &_mutex, SHARED ) ;
      return _number ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS, "_rtnExtDataProcessorMgr::getProcessorsByCS" )
   INT32 _rtnExtDataProcessorMgr::getProcessorsByCS( const CHAR *csName,
                                                     INT32 lockType,
                                                     vector<rtnExtDataProcessor *> &processors )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS ) ;

      UINT16 checkNum = 0 ;
      BOOLEAN locked = ( SHARED == lockType || EXCLUSIVE == lockType ) ;
      vector<INT32> lockedIDs ;
      rtnExtDataProcessor *processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      // One cs may contain multiple text indices, according with multiple
      // processors. We need to lock all of them, to avoid partial failure.
      // If any one of the processors can not be locked in one second. All
      // processors who have been locked need to be unlocked.
      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processor = &_processors[i] ;
         if ( !processor->isActive() )
         {
            continue ;
         }

         if ( processor->isOwnedBy( csName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               rc = mutex->lock_r() ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               rc = mutex->lock_w() ;
            }
            else
            {
               processors.push_back( processor ) ;
               continue ;
            }

            // In case of error, all processors which have been locked need to
            // be released.
            if ( rc )
            {
               goto error ;
            }

            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processor->isOwnedBy( csName ) || !processor->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }

            if ( locked )
            {
               lockedIDs.push_back( i ) ;
            }
            processors.push_back( processor ) ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCS, rc ) ;
      return rc ;
   error:
      processors.clear() ;
      if ( locked )
      {
         for ( vector<INT32>::iterator itr = lockedIDs.begin();
               itr != lockedIDs.end(); ++itr )
         {
            if ( SHARED == lockType )
            {
               _processorLocks[*itr].release_r() ;
            }
            else
            {
               _processorLocks[*itr].release_w() ;
            }
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL, "_rtnExtDataProcessorMgr::getProcessorsByCL" )
   INT32 _rtnExtDataProcessorMgr::getProcessorsByCL( const CHAR *csName,
                                                     const CHAR *clName,
                                                     INT32 lockType,
                                                     std::vector<rtnExtDataProcessor *> &processors )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL ) ;

      UINT16 checkNum = 0 ;
      BOOLEAN locked = ( SHARED == lockType || EXCLUSIVE == lockType ) ;
      vector<INT32> lockedIDs ;
      rtnExtDataProcessor *processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processor = &_processors[i] ;
         if ( !processor->isActive() )
         {
            continue ;
         }

         if ( processor->isOwnedBy( csName, clName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               rc = mutex->lock_r() ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               rc = mutex->lock_w() ;
            }
            else
            {
               processors.push_back( processor ) ;
               continue ;
            }

            if ( rc )
            {
               goto error ;
            }

            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processor->isOwnedBy( csName, clName ) ||
                 !processor->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }

            if ( locked )
            {
               lockedIDs.push_back( i ) ;
            }
            processors.push_back( processor ) ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORSBYCL, rc ) ;
      return rc ;
   error:
      processors.clear() ;
      if ( locked )
      {
         for ( vector<INT32>::iterator itr = lockedIDs.begin();
               itr != lockedIDs.end(); ++itr )
         {
            if ( SHARED == lockType )
            {
               _processorLocks[*itr].release_r() ;
            }
            else
            {
                _processorLocks[*itr].release_w() ;
            }
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX, "_rtnExtDataProcessorMgr::getProcessorByIdx" )
   INT32 _rtnExtDataProcessorMgr::getProcessorByIdx( const CHAR *csName,
                                                     const CHAR *clName,
                                                     const CHAR *idxName,
                                                     INT32 lockType,
                                                     rtnExtDataProcessor *&processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX ) ;
      UINT16 checkNum = 0 ;
      rtnExtDataProcessor *processorLocal = NULL ;

      processor = NULL ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         processorLocal = &_processors[i] ;
         if ( !processorLocal->isActive() )
         {
            continue ;
         }

         if ( processorLocal->isOwnedBy( csName, clName, idxName ) )
         {
            ossRWMutex *mutex = &_processorLocks[i] ;
            if ( SHARED == lockType )
            {
               mutex->lock_r() ;
            }
            else if ( EXCLUSIVE == lockType )
            {
               mutex->lock_w() ;
            }
            // Double check after aquiring the lock. As when we are waiting for
            // the lock, it may have been deleted by some drop operation.
            if ( !processorLocal->isOwnedBy( csName, clName, idxName ) ||
                 !processorLocal->isActive() )
            {
               if ( SHARED == lockType )
               {
                  mutex->release_r() ;
               }
               else
               {
                  mutex->release_w() ;
               }
               continue ;
            }
            processor = processorLocal  ;
            break ;
         }
         if ( ++checkNum >= _number )
         {
            break ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYIDX ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME, "_rtnExtDataProcessorMgr::getProcessorByExtName" )
   INT32 _rtnExtDataProcessorMgr::getProcessorByExtName( const CHAR *extName,
                                                         INT32 lockType,
                                                         rtnExtDataProcessor *&processor )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME ) ;

      UINT16 checkNum = 0 ;
      processor = NULL  ;

      if ( !extName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "External data name is invalid" ) ;
         goto error ;
      }

      {
         ossScopedLock lock( &_mutex, SHARED ) ;
         for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
         {
            if ( !_processors[i].isActive() )
            {
               continue ;
            }
            if ( _processors[i].isOwnedByExt( extName ) )
            {
               ossRWMutex *mutex = &_processorLocks[i] ;
               if ( SHARED == lockType )
               {
                  mutex->lock_r() ;
               }
               else if ( EXCLUSIVE == lockType )
               {
                  mutex->lock_w() ;
               }
               processor = &_processors[i] ;
               break ;
            }
            if ( ++checkNum >= _number )
            {
               break ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORBYEXTNAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSOR, "_rtnExtDataProcessorMgr::unlockProcessor" )
   void _rtnExtDataProcessorMgr::unlockProcessor( rtnExtDataProcessor *processor,
                                                  INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSOR ) ;
      ossRWMutex *mutex = &_processorLocks[ processor->getID() ] ;

      if ( SHARED == lockType )
      {
         mutex->release_r() ;
      }
      else if ( EXCLUSIVE == lockType )
      {
         mutex->release_w() ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS, "_rtnExtDataProcessorMgr::unlockProcessors" )
   void _rtnExtDataProcessorMgr::unlockProcessors( std::vector<rtnExtDataProcessor *> &processors,
                                                   INT32 lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS ) ;
      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return ;
      }

      for ( std::vector<rtnExtDataProcessor *>::iterator itr = processors.begin() ;
            itr != processors.end(); ++itr )
      {
         ossRWMutex *mutex = &_processorLocks[ (*itr)->getID() ] ;
         if ( SHARED == lockType )
         {
            mutex->release_r() ;
         }
         else
         {
            mutex->release_w() ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_UNLOCKPROCESSORS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR, "_rtnExtDataProcessorMgr::destroyProcessor" )
   void
   _rtnExtDataProcessorMgr::destroyProcessor( rtnExtDataProcessor *processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR ) ;
      if ( processor )
      {
         INT32 id = processor->getID() ;
         if ( id < 0 || id >= RTN_EXT_PROCESSOR_MAX_NUM )
         {
            PD_LOG( PDWARNING, "Invalid processor id[%d] for destroy", id ) ;
            goto done ;
         }

         aquireMetaLock( EXCLUSIVE );
         processor->reset() ;
         --_number ;
         releaseMetaLock( EXCLUSIVE ) ;
      }
   done:
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSOR ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS, "_rtnExtDataProcessorMgr::destroyProcessors" )
   void _rtnExtDataProcessorMgr::destroyProcessors( vector<rtnExtDataProcessor*> &processors )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS ) ;
      aquireMetaLock( EXCLUSIVE ) ;

      for ( vector<rtnExtDataProcessor*>::iterator itr = processors.begin();
            itr != processors.end(); ++itr )
      {
         INT32 id = (*itr)->getID() ;
         if ( id < 0 || id >= RTN_EXT_PROCESSOR_MAX_NUM )
         {
            PD_LOG( PDWARNING, "Invalid processor id[%d] for destroy", id ) ;
            continue ;
         }
         _processors[id].reset() ;
         --_number ;
      }
      releaseMetaLock( EXCLUSIVE ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_DESTROYPROCESSORS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS, "_rtnExtDataProcessorMgr::renameCS" )
   INT32 _rtnExtDataProcessorMgr::renameCS( const CHAR *name,
                                            const CHAR *newName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS ) ;
      aquireMetaLock( EXCLUSIVE );

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         if ( _processors[i].isOwnedBy( name ) )
         {
            _processors[i].updateMeta( newName ) ;
         }
      }

      releaseMetaLock( EXCLUSIVE ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_RENAMECS ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL, "_rtnExtDataProcessorMgr::renameCL" )
   INT32 _rtnExtDataProcessorMgr::renameCL( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *newCLName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL ) ;
      aquireMetaLock( EXCLUSIVE );

      for ( INT32 i = 0; i < RTN_EXT_PROCESSOR_MAX_NUM; ++i )
      {
         if ( _processors[i].isOwnedBy( csName, clName ) )
         {
            _processors[i].updateMeta( csName, newCLName ) ;
         }
      }

      releaseMetaLock( EXCLUSIVE ) ;
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_RENAMECL ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_TRYAQUIREMETALOCK, "_rtnExtDataProcessorMgr::tryAquireMetaLock" )
   BOOLEAN _rtnExtDataProcessorMgr::tryAquireMetaLock( OSS_LATCH_MODE lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_TRYAQUIREMETALOCK ) ;
      BOOLEAN locked = FALSE;

      if ( EXCLUSIVE == lockType )
      {
         locked = _mutex.try_get() ;
      }
      else if ( SHARED == lockType )
      {
         locked = _mutex.try_get_shared() ;
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_TRYAQUIREMETALOCK ) ;
      return locked ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR__AQUIREMETALOCK, "_rtnExtDataProcessorMgr::aquireMetaLock" )
   void _rtnExtDataProcessorMgr::aquireMetaLock( OSS_LATCH_MODE lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR__AQUIREMETALOCK ) ;
      BOOLEAN locked = FALSE ;
   retry:
      if ( EXCLUSIVE == lockType )
      {
         locked = _mutex.try_get() ;
      }
      else if ( SHARED == lockType )
      {
         locked = _mutex.try_get_shared() ;
      }
      else
      {
         goto done ;
      }

      if ( !locked )
      {
         ossSleep( 10 ) ;
         goto retry ;
      }

   done:
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR__AQUIREMETALOCK ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_RELEASEMETALOCK, "_rtnExtDataProcessorMgr::releaseMetaLock" )
   void _rtnExtDataProcessorMgr::releaseMetaLock( OSS_LATCH_MODE lockType )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_RELEASEMETALOCK ) ;
      if ( EXCLUSIVE == lockType )
      {
         _mutex.release() ;
      }
      else if ( SHARED == lockType )
      {
         _mutex.release_shared() ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_RELEASEMETALOCK ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR__PROCESSORLOOKUP,"_rtnExtDataProcessorMgr::_processorLookup" )
   INT32 _rtnExtDataProcessorMgr::_processorLookup( const CHAR *extName,
                                                    rtnExtDataProcessor **processor )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR__PROCESSORLOOKUP ) ;
      rtnExtDataProcessor *processorLocal = NULL ;

      for ( INT32 id = 0 ; id < RTN_EXT_PROCESSOR_MAX_NUM; ++id )
      {
         if ( RTN_EXT_PROCESSOR_NORMAL == _processors[ id ].stat() &&
              _processors[ id ].isOwnedByExt( extName ) )
         {
            processorLocal = &_processors[ id ];
            break;
         }
      }

      if ( !processorLocal )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

      if ( processor )
      {
         *processor = processorLocal ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR__PROCESSORLOOKUP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR__NAMETOPROCESSORANDLOCK,"_rtnExtDataProcessorMgr::_nameToProcessorAndLock" )
   INT32 _rtnExtDataProcessorMgr::_nameToProcessorAndLock( const CHAR *extName,
                                                           rtnExtDataProcessor **processor,
                                                           OSS_LATCH_MODE lockType,
                                                           INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR__NAMETOPROCESSORANDLOCK ) ;
      rtnExtDataProcessor *processorLocal = NULL ;

      SDB_ASSERT( processor, "Processor pointer is NULL" ) ;
      SDB_ASSERT( extName, "External name is not null" ) ;

      ossScopedLock lock( &_mutex, SHARED ) ;

      rc = _processorLookup( extName, &processorLocal ) ;
      if ( rc )
      {
         goto error ;
      }

      SDB_ASSERT( processorLocal, "Processor is NULL" ) ;

      if ( EXCLUSIVE == lockType )
      {
         rc = _processorLocks[ processorLocal->getID() ].lock_w( millisec ) ;
      }
      else
      {
         rc = _processorLocks[ processorLocal->getID() ].lock_r( millisec ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      *processor = processorLocal ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR__NAMETOPROCESSORANDLOCK,
                       rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR__GETFIRSTFREEPROCESSOR,"_rtnExtDataProcessorMgr::_getFirstFreeProcessor" )
   rtnExtDataProcessor* _rtnExtDataProcessorMgr::_getFirstFreeProcessor( INT32 *id )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR__GETFIRSTFREEPROCESSOR ) ;
      rtnExtDataProcessor *processor = NULL ;
      if ( _number < RTN_EXT_PROCESSOR_MAX_NUM )
      {
         for ( INT32 index = 0 ; index < RTN_EXT_PROCESSOR_MAX_NUM; ++index )
         {
            if ( RTN_EXT_PROCESSOR_INVALID == _processors[ index ].stat() )
            {
               if ( id )
               {
                  *id = index ;
               }
               processor = &_processors[ index ] ;
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR__GETFIRSTFREEPROCESSOR ) ;
      return processor ;
   }

   rtnExtDataProcessorMgr* rtnGetExtDataProcessorMgr()
   {
      static rtnExtDataProcessorMgr s_edpMgr ;
      return &s_edpMgr ;
   }
}

