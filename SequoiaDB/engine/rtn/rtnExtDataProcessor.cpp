/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "pmd.hpp"
#include "rtn.hpp"
#include "rtnTrace.hpp"
#include "../bson/lib/md5.hpp"

#define RTN_CAPPED_CL_MAXSIZE       ( 30 * 1024 * 1024 * 1024LL )
#define RTN_CAPPED_CL_MAXRECNUM     0
#define RTN_FIELD_NAME_RID          "_rid"
#define RTN_FIELD_NAME_SOURCE       "_source"

namespace engine
{
   INT32 _rtnExtProcessorMeta::init( const CHAR *csName, const CHAR *clName,
                                     const CHAR *idxName,
                                     const BSONObj &idxKeyDef )
   {
      INT32 rc = SDB_OK ;

      _csName = string( csName ) ;
      _clName = string( clName ) ;
      _idxName = string( idxName ) ;
      try
      {
         _idxKeyDef = idxKeyDef.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnExtDataProcessor::_rtnExtDataProcessor()
   {
      ossMemset( _cappedCSName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      ossMemset( _cappedCLName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
   }

   _rtnExtDataProcessor::~_rtnExtDataProcessor()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_INIT, "_rtnExtDataProcessor::init" )
   INT32 _rtnExtDataProcessor::init( const CHAR *csName, const CHAR *clName,
                                     const CHAR *idxName,
                                     const BSONObj &idxKeyDef )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_INIT ) ;

      rc = _meta.init( csName, clName, idxName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Processor meta init failed[ %d ]", rc ) ;

      getExtDataNames( csName, clName, idxName, _cappedCSName,
                       DMS_COLLECTION_SPACE_NAME_SZ + 1,
                       _cappedCLName, DMS_COLLECTION_NAME_SZ + 1 ) ;

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_INIT ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT, "_rtnExtDataProcessor::prepareInsert" )
   INT32 _rtnExtDataProcessor::prepareInsert( const BSONObj &inputObj,
                                              BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT ) ;
      BSONObjSet keySet ;
      OID oid ;

      try
      {
         BSONElement ele ;
         ixmIndexKeyGen keygen( _meta._idxKeyDef, GEN_OBJ_KEEP_FIELD_NAME ) ;
         rc = keygen.getKeys( inputObj, keySet, NULL, TRUE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate key from object failed[ %d ]",
                      rc ) ;

         SDB_ASSERT( keySet.size() <= 1, "Key set size should be 1" ) ;
         if ( 0 == keySet.size() )
         {
            goto done ;
         }

         ele = inputObj.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no _id "
                    "field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         oid = ele.OID() ;

         {
            BSONObjSet::iterator it = keySet.begin();
            BSONObj object( *it ) ;
            rc = _prepareRecord( _cappedCLName, RTN_EXT_INSERT,
                                 &oid, &object, recordObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _lock() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREINSERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE, "_rtnExtDataProcessor::prepareDelete" )
   INT32 _rtnExtDataProcessor::prepareDelete( const BSONObj &inputObj,
                                              BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE ) ;
      OID oid ;
      BSONObjSet keySet ;

      try
      {
         BSONElement ele ;
         ixmIndexKeyGen keygen( _meta._idxKeyDef, GEN_OBJ_KEEP_FIELD_NAME ) ;
         rc = keygen.getKeys( inputObj, keySet, NULL, TRUE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate key from object failed[ %d ]",
                      rc ) ;

         SDB_ASSERT( keySet.size() <= 1, "Key set size should be 1" ) ;
         if ( 0 == keySet.size() )
         {
            goto done ;
         }

         ele = inputObj.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no _id "
                  "field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         oid = ele.OID() ;

         rc = _prepareRecord( _cappedCLName, RTN_EXT_DELETE,
                              &oid, NULL, recordObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                      rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _lock() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREDELETE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE, "_rtnExtDataProcessor::prepareUpdate" )
   INT32 _rtnExtDataProcessor::prepareUpdate( const BSONObj &originalObj,
                                              const BSONObj &newObj,
                                              BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE ) ;
      BSONObjSet keySetOri ;
      BSONObjSet keySetNew ;
      OID oid ;

      try
      {
         BSONElement ele ;

         ixmIndexKeyGen keygen( _meta._idxKeyDef, GEN_OBJ_KEEP_FIELD_NAME ) ;
         rc = keygen.getKeys( originalObj, keySetOri, NULL,
                              TRUE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate key from object[ %s ] "
                      "failed[ %d ]", originalObj.toString().c_str(), rc ) ;
         rc = keygen.getKeys( newObj, keySetNew, NULL, TRUE, FALSE, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Generate key from object[ %s ] "
                      "failed[ %d ]", newObj.toString().c_str(), rc ) ;

         if ( 0 == keySetNew.size() )
         {
            rc = prepareDelete( originalObj, recordObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare for delete failed[ %d ]", rc ) ;
            goto done ;
         }

         {
            BSONObjSet::iterator origItr = keySetOri.begin() ;
            BSONObjSet::iterator newItr = keySetNew.begin() ;

            while ( origItr != keySetOri.end() && newItr != keySetNew.end() )
            {
               if ( !( *origItr == *newItr ) )
               {
                  break ;
               }
               origItr++ ;
               newItr++ ;
            }

            if ( ( origItr == keySetOri.end() ) && ( newItr == keySetNew.end() ) )
            {
               goto done ;
            }
         }

         ele = newObj.getField( DMS_ID_KEY_NAME ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDERROR, "Text index can not be used if record has no _id "
                  "field" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         oid = ele.OID() ;
         SDB_ASSERT( 1 == keySetNew.size(), "Key set size should be 1" ) ;
         {
            BSONObjSet::iterator it = keySetNew.begin();
            BSONObj object( *it ) ;
            rc = _prepareRecord( _cappedCLName, RTN_EXT_UPDATE,
                                 &oid, &object, recordObj ) ;
            PD_RC_CHECK( rc, PDERROR, "Add operation record failed[ %d ]",
                         rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

      _lock() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_PREPAREUPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOWRITE, "_rtnExtDataProcessor::doWrite" )
   INT32 _rtnExtDataProcessor::doWrite( pmdEDUCB *cb, BSONObj &record,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_DOWRITE ) ;
      INT32 insertNum = 0 ;
      INT32 ignoreNum = 0 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;

      rc = rtnInsert( _cappedCLName, record, 1, 0,
                      cb, dmsCB, dpsCB, 1, &insertNum, &ignoreNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert record failed[ %d ]", rc ) ;

   done:
      _unlock() ;
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOWRITE, rc ) ;
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

      rc = rtnDropCollectionSpaceP1( _cappedCSName, cb, dmsCB, dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;
      rtnCB->incTextIdxVersion() ;
      _lock() ;

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

      rc = rtnDropCollectionSpaceP1Cancel( _cappedCSName, cb, dmsCB,
                                           dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Cancel dropping collection space[ %s ] in "
                   "phase 1 failed[ %d ]", _cappedCSName, rc ) ;

   done:
      _unlock() ;
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

      rc = rtnDropCollectionSpaceP2( _cappedCSName, cb, dmsCB,
                                     dpsCB, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Phase 1 of dropping collection space[ %s ] "
                   "failed[ %d ]", _cappedCSName, rc ) ;

   done:
      _unlock() ;
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DODROPP2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnExtDataProcessor::doLoad()
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_DOUNLOAD, "_rtnExtDataProcessor::doUnload" )
   INT32 _rtnExtDataProcessor::doUnload( _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
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
      _unlock() ;
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
      PD_RC_CHECK( rc, PDERROR, "Create capped collection[ %s.%s ] "
                   "failed[ %d ]",
                   _cappedCSName, _cappedCLName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_DOREBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA, "_rtnExtDataProcessor::updateMeta" )
   INT32 _rtnExtDataProcessor::updateMeta(const rtnExtProcessorMeta & meta)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA ) ;
      rc = init( meta._csName.c_str(), meta._clName.c_str(),
                 meta._idxName.c_str(), meta._idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Init processor failed[ %d ]", rc ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR_UPDATEMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY, "_rtnExtDataProcessor::isOwnedBy" )
   BOOLEAN _rtnExtDataProcessor::isOwnedBy( const CHAR *csName,
                                            const CHAR *clName,
                                            const CHAR *idxName )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      BOOLEAN result = FALSE ;

      SDB_ASSERT( csName, "CS name can not be NULL" ) ;

      if ( 0 == ossStrcmp( csName, _meta._csName.c_str() ) )
      {
         result = TRUE ;
      }

      if ( clName && 0 != ossStrcmp( clName, _meta._clName.c_str() ) && result )
      {
         result = FALSE ;
      }

      if ( idxName && 0 != ossStrcmp( idxName, _meta._idxName.c_str() )
           && result )
      {
         result = FALSE ;
      }

      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR__ISOWNEDBY ) ;
      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR_GETEXTDATANAMES, "_rtnExtDataProcessor::getExtDataNames" )
   void _rtnExtDataProcessor::getExtDataNames( const CHAR *csName,
                                               const CHAR *clName,
                                               const CHAR *idxName,
                                               CHAR *extCSName,
                                               UINT32 csNameBufSize,
                                               CHAR *extCLName,
                                               UINT32 clNameBufSize )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR_GETEXTDATANAMES ) ;
      SDB_ASSERT( csName && clName && idxName, "Name is NULL" ) ;
      string srcStr = string( csName ) + string( clName ) + string( idxName ) ;
      UINT32 hashVal = ossHash( srcStr.c_str() ) ;
      string md5Val = md5::md5simpledigest( srcStr.c_str() ) ;
      ostringstream name ;
      name << SYS_PREFIX"_" << hashVal << md5Val.substr( 0, 4 ) ;
      if ( extCSName && csNameBufSize > 0 )
      {
         ossSnprintf( extCSName, csNameBufSize, name.str().c_str() ) ;
      }

      if ( extCLName && clNameBufSize )
      {
         ossSnprintf( extCLName, clNameBufSize,
                      "%s.%s", name.str().c_str(), name.str().c_str() ) ;
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSOR_GETEXTDATANAMES ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, "_rtnExtDataProcessor::_prepareRecord" )
   INT32 _rtnExtDataProcessor::_prepareRecord( const CHAR *name,
                                               _rtnExtOprType oprType,
                                               const bson::OID *dataOID,
                                               const BSONObj *dataObj,
                                               BSONObj &recordObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD ) ;
      BSONObjBuilder objBuilder ;
      BOOLEAN oidRequired = FALSE ;
      BOOLEAN dataRequired = FALSE ;

      switch ( oprType )
      {
         case RTN_EXT_INSERT:  // insert
         case RTN_EXT_UPDATE:  // update
            oidRequired = TRUE ;
            dataRequired = TRUE ;
            break ;
         case RTN_EXT_DELETE:  // delete
            oidRequired = TRUE ;
            dataRequired = FALSE ;
            break ;
         default:
            PD_LOG( PDERROR, "Invalid operation type[ %d ]", oprType ) ;
            rc = SDB_SYS ;
            goto error ;
      }

      if ( oidRequired && ( !dataOID || !dataOID->isSet() ) )
      {
         PD_LOG( PDERROR, "_id should be specified for current operation" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( dataRequired && !dataObj )
      {
         PD_LOG( PDERROR, "Data object is NULL for operation" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         objBuilder.append( FIELD_NAME_TYPE, oprType ) ;
         if ( oidRequired )
         {
            objBuilder.append( RTN_FIELD_NAME_RID, dataOID->str().c_str() ) ;
         }

         if ( dataRequired )
         {
            objBuilder.append( RTN_FIELD_NAME_SOURCE, *dataObj ) ;
         }

         recordObj = objBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSOR__PREPARERECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnExtDataProcessor::_lock()
   {
      _latch.get() ;
   }

   void _rtnExtDataProcessor::_unlock()
   {
      _latch.release() ;
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

      rc = rtnCreateCollectionSpaceCommand( csName, cb, dmsCB,
                                            dpsCB, DMS_PAGE_SIZE_DFT,
                                            DMS_DO_NOT_CREATE_LOB,
                                            DMS_STORAGE_CAPPED, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Create capped collection space failed[ %d ]",
                   rc ) ;

      csCreated = TRUE ;

      try
      {
         builder.append( FIELD_NAME_SIZE, RTN_CAPPED_CL_MAXSIZE ) ;
         builder.append( FIELD_NAME_MAX, RTN_CAPPED_CL_MAXRECNUM ) ;
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
                                       cb, dmsCB, dpsCB,
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

   _rtnExtDataProcessorMgr::_rtnExtDataProcessorMgr()
   {
   }

   _rtnExtDataProcessorMgr::~_rtnExtDataProcessorMgr()
   {
      for ( PROCESSOR_MAP_ITR itr = _processorMap.begin();
            itr != _processorMap.end(); ++itr )
      {
         SDB_OSS_DEL itr->second ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_ADDPROCESSOR, "_rtnExtDataProcessorMgr::addProcessor" )
   INT32 _rtnExtDataProcessorMgr::addProcessor( const CHAR *csName,
                                                const CHAR *clName,
                                                const CHAR *idxName,
                                                const BSONObj &idxKeyDef,
                                                rtnExtDataProcessor** processor )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_ADDPROCESSOR ) ;
      rtnExtDataProcessor *processorLocal = NULL ;

      UINT32 key = _genProcessorKey( csName, clName, idxName ) ;

      processorLocal = SDB_OSS_NEW rtnExtDataProcessor() ;
      if ( !processorLocal )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate external data processor failed" ) ;
         goto error ;
      }

      rc = processorLocal->init( csName, clName, idxName, idxKeyDef ) ;
      PD_RC_CHECK( rc, PDERROR, "Init external data processor for "
                   "collection[ %s.%s ] and index[ %s ] failed[ %d ]",
                   csName, clName, idxName, rc ) ;

      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _processorMap.insert( std::pair<UINT32, rtnExtDataProcessor *>
                               ( key, processorLocal ) ) ;
      }

      if ( processor )
      {
         *processor = processorLocal ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_ADDPROCESSOR, rc ) ;
      return rc ;
   error:
      if ( processorLocal )
      {
         SDB_OSS_DEL processorLocal ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS, "_rtnExtDataProcessorMgr::getProcessors" )
   void _rtnExtDataProcessorMgr::getProcessors( const CHAR *csName,
                                                vector<rtnExtDataProcessor *> &processorVec )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS ) ;
      ossScopedRWLock lock( &_mutex, SHARED ) ;
      for ( PROCESSOR_MAP_ITR itr = _processorMap.begin();
            itr != _processorMap.end(); ++itr )
      {
         if ( itr->second->isOwnedBy( csName ) )
         {
            processorVec.push_back( itr->second ) ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS2, "_rtnExtDataProcessorMgr::getProcessors2" )
   void _rtnExtDataProcessorMgr::getProcessors( const CHAR *csName,
                                                const CHAR *clName,
                                                vector<rtnExtDataProcessor *> &processorVec )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS2 ) ;
      ossScopedRWLock lock( &_mutex, SHARED ) ;
      for ( PROCESSOR_MAP_ITR itr = _processorMap.begin();
            itr != _processorMap.end(); ++itr )
      {
         if ( itr->second->isOwnedBy( csName, clName ) )
         {
            processorVec.push_back( itr->second ) ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSORS2 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSOR, "_rtnExtDataProcessorMgr::getProcessor" )
   INT32 _rtnExtDataProcessorMgr::getProcessor( const CHAR *csName,
                                                const CHAR *clName,
                                                const CHAR *idxName,
                                                rtnExtDataProcessor **processor )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSOR ) ;
      PROCESSOR_MAP_ITR itr ;
      UINT32 key = 0 ;

      if ( !csName || !clName || !idxName )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "cs/cl/index name invalid to get processor" ) ;
         goto error ;
      }
      key = _genProcessorKey( csName, clName, idxName ) ;

      *processor = NULL ;
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         std::pair<PROCESSOR_MAP_ITR, PROCESSOR_MAP_ITR> range =
            _processorMap.equal_range( key ) ;
         for ( PROCESSOR_MAP_ITR itr = range.first; itr != range.second; ++itr )
         {
            if ( itr->second->isOwnedBy( csName, clName, idxName ) )
            {
               *processor = itr->second ;
               break ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNEXTDATAPROCESSORMGR_GETPROCESSOR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNEXTDATAPROCESSORMGR_DELPROCESSOR, "_rtnExtDataProcessorMgr::delProcessor" )
   void _rtnExtDataProcessorMgr::delProcessor( rtnExtDataProcessor **processor )
   {
      PD_TRACE_ENTRY( SDB__RTNEXTDATAPROCESSORMGR_DELPROCESSOR ) ;
      ossScopedRWLock _lock( &_mutex, EXCLUSIVE ) ;
      for ( PROCESSOR_MAP_ITR itr = _processorMap.begin();
            itr != _processorMap.end(); ++itr )
      {
         if ( itr->second == *processor )
         {
            _processorMap.erase( itr ) ;
            *processor = NULL ;
            break ;
         }
      }
      PD_TRACE_EXIT( SDB__RTNEXTDATAPROCESSORMGR_DELPROCESSOR ) ;
   }

   UINT32 _rtnExtDataProcessorMgr::_genProcessorKey( const CHAR *csName,
                                                     const CHAR *clName,
                                                     const CHAR *idxName )
   {
      SDB_ASSERT( csName && clName && idxName, "Names can not be NULL" ) ;

      string srcStr = string( csName ) + string( clName ) + string( idxName ) ;
      return ossHash( srcStr.c_str() ) ;
   }

   rtnExtDataProcessorMgr* rtnGetExtDataProcessorMgr()
   {
      static rtnExtDataProcessorMgr s_edpMgr ;
      return &s_edpMgr ;
   }
}

