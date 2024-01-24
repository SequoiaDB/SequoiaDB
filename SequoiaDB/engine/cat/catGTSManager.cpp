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

   Source File Name = catGTSManager.cpp

   Descriptive Name = GTS(Global Transaction Service) manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/13/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "catGTSManager.hpp"
#include "catGTSDef.hpp"
#include "catalogueCB.hpp"
#include "pmdEDU.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "../util/fromjson.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "pd.hpp"

namespace engine
{

   const utilCSUniqueID SYS_GTS_CSUID = UTIL_CSUNIQUEID_CAT_MIN + 4 ;
   const utilCLUniqueID SYS_GTS_SEQ_CLUID =
               utilBuildCLUniqueID( SYS_GTS_CSUID, UTIL_CSUNIQUEID_CAT_MIN + 1 ) ;

   _catGTSManager::_catGTSManager()
   {
      _dmsCB = NULL ;
      _eduCB = NULL ;
      _catCB = NULL ;
   }

   _catGTSManager::~_catGTSManager()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR_INIT, "_catGTSManager::init" )
   INT32 _catGTSManager::init()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;
      _dmsCB = pKrcb->getDMSCB() ;
      _catCB = pKrcb->getCATLOGUECB() ;

      PD_TRACE_ENTRY ( SDB_GTS_MGR_INIT ) ;

      rc = _ensureMetadata () ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to create GTS metadata, rc=%d", rc ) ;
         goto error ;
      }

      rc = _msgHandler.init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

      _catCB->regEventHandler( this ) ;

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR_INIT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _catGTSManager::fini()
   {
      INT32 rc = SDB_OK ;

      if ( NULL != _catCB )
      {
         _catCB->unregEventHandler( this ) ;
      }

      rc = _msgHandler.fini();
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to finalize GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _catGTSManager::attachCB( _pmdEDUCB* cb )
   {
      _eduCB = cb ;
   }

   void _catGTSManager::detachCB( _pmdEDUCB* cb )
   {
      _eduCB = NULL ;
   }

   INT32 _catGTSManager::active()
   {
      INT32 rc = SDB_OK ;

      rc = _seqMgr.active() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to active GTS sequence manager, rc=%d", rc ) ;
         goto error ;
      }

      rc = _msgHandler.active() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to active GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catGTSManager::deactive()
   {
      INT32 rc = SDB_OK ;

      rc = _msgHandler.deactive() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to deactive GTS msg handler, rc=%d", rc ) ;
         goto error ;
      }

      rc = _seqMgr.deactive() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to deactive GTS sequence manager, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _catGTSManager::handleMsg( const NET_HANDLE& handle,
                                    const MsgHeader* msg )
   {
      INT32 rc = SDB_OK ;

      PD_LOG( PDINFO, "Receive GTS msg: %d, length:%d",
              msg->opCode, msg->messageLength ) ;

      rc = _msgHandler.postMsg( handle, msg ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR_ONUPGRADE, "_catGTSManager::onUpgrade" )
   INT32 _catGTSManager::onUpgrade( UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_MGR_ONUPGRADE ) ;

      if ( CATALOG_VERSION_V2 == version )
      {
         rc = _checkAndUpgradeSequenceCLUID() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to upgrade [%s] to sequences, "
                      "rc: %d", CAT_SEQUENCE_CLUID, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_GTS_MGR_ONUPGRADE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__ENSURE_METADATA, "_catGTSManager::_ensureMetadata" )
   INT32 _catGTSManager::_ensureMetadata()
   {
      INT32 rc = SDB_OK ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      PD_TRACE_ENTRY( SDB_GTS_MGR__ENSURE_METADATA ) ;

      // create SYSGTS.SEQUENCES
      rc = _createSysCollection( GTS_SEQUENCE_COLLECTION_NAME,
                                 SYS_GTS_SEQ_CLUID, cb ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _createSysIndex( GTS_SEQUENCE_COLLECTION_NAME,
                            GTS_SEQUENCE_NAME_INDEX, cb ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _createSysIndex( GTS_SEQUENCE_COLLECTION_NAME,
                            GTS_SEQUENCE_CLUID_INDEX, cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_GTS_MGR__ENSURE_METADATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__CREATE_SYS_INDEX, "_catGTSManager::_createSysIndex" )
   INT32 _catGTSManager::_createSysIndex( const CHAR* clFullName,
                                          const CHAR* indexJson,
                                          _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj indexDef ;
      PD_TRACE_ENTRY ( SDB_GTS_MGR__CREATE_SYS_INDEX ) ;
      PD_TRACE2 ( SDB_GTS_MGR__CREATE_SYS_INDEX,
                  PD_PACK_STRING ( clFullName ),
                  PD_PACK_STRING ( indexJson ) ) ;

      rc = fromjson ( indexJson, indexDef ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to build index object, rc = %d",
                    rc ) ;

      rc = rtnTestAndCreateIndex( clFullName, indexDef, cb, _dmsCB,
                                  NULL, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR__CREATE_SYS_INDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__CREATE_SYS_CL, "_catGTSManager::_createSysCollection" )
   INT32 _catGTSManager::_createSysCollection( const CHAR* clFullName,
                                               utilCLUniqueID clUID,
                                               _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_GTS_MGR__CREATE_SYS_CL ) ;
      PD_TRACE1 ( SDB_GTS_MGR__CREATE_SYS_CL,
                  PD_PACK_STRING ( clFullName ) ) ;

      rc = rtnTestAndCreateCL( clFullName, cb, _dmsCB, NULL, clUID, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_GTS_MGR__CREATE_SYS_CL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GTS_MGR__CHKUPGSEQCLUID, "_catGTSManager::_checkAndUpgradeSequenceCLUID" )
   INT32 _catGTSManager::_checkAndUpgradeSequenceCLUID()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_GTS_MGR__CHKUPGSEQCLUID ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      BSONObj noExistMatcher, noExistCLMatcher, userSeqUpdator ;
      rtnQueryOptions options ;
      INT64 noCLUniqueIDCount = 0 ;
      INT64 contextID = -1 ;

      // upgrade sequence with CLUniqueID

      options.setCLFullName( GTS_SEQUENCE_COLLECTION_NAME ) ;

      // 1. test if sequence has CLUniqueID field
      try
      {
         noExistMatcher =
               BSON( CAT_SEQUENCE_CLUID << BSON( "$exists" << 0 ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
      options.setQuery( noExistMatcher ) ;
      rc = rtnGetCount( options, _dmsCB, _eduCB, rtnCB, &noCLUniqueIDCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to count from [%s] for sequences "
                   "without [%s], rc: %d", GTS_SEQUENCE_COLLECTION_NAME,
                   CAT_SEQUENCE_CLUID, rc ) ;
      if ( 0 == noCLUniqueIDCount )
      {
         // no sequence without collection unique ID
         goto done ;
      }

      // 2. add collection unique ID to sequences of collections
      try
      {
         // names of sequences of collections starts with "SYS_"
         noExistCLMatcher =
               BSON( CAT_SEQUENCE_CLUID << BSON( "$exists" << 0 ) <<
                     CAT_SEQUENCE_NAME <<
                                 BSON( "$gt" << "SYS_" << "$lt" << "SYS`" ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
      options.setQuery( noExistCLMatcher ) ;
      rc = rtnQuery( options, _eduCB, _dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query from [%s] for sequence "
                   "without [%s], rc: %d", GTS_SEQUENCE_COLLECTION_NAME,
                   CAT_SEQUENCE_CLUID, rc ) ;
      while ( TRUE )
      {
         rtnContextBuf buffObj ;

         rc = rtnGetMore( contextID, 1, buffObj, _eduCB, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get more from [%s] for "
                      "sequence without [%s], rc: %d",
                      GTS_SEQUENCE_COLLECTION_NAME, CAT_SEQUENCE_CLUID, rc ) ;

         try
         {
            BSONObj obj( buffObj.data() ) ;
            BSONElement eleName = obj.getField( CAT_SEQUENCE_NAME ) ;
            const CHAR *seqName = NULL ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            UINT32 parsed = 0 ;
            BSONObj matcher, updator ;
            rtnQueryOptions updateOptions ;

            // parse sequence name
            // names of sequences of collections are in format
            // "SYS_<clUnqiueID>_<field>_SEQ"
            PD_CHECK( String == eleName.type(),
                      SDB_CAT_CORRUPTION, error, PDERROR,
                      "Failed to get field [%s] from sequence [%s]",
                      CAT_SEQUENCE_NAME, obj.toPoolString().c_str() ) ;
            seqName = eleName.valuestr() ;

            parsed = ossSscanf( seqName, "SYS_%llu_", &clUniqueID ) ;
            PD_CHECK( 1 == parsed, SDB_CAT_CORRUPTION, error, PDERROR,
                      "Failed to parse collection unique ID from "
                      "sequence name [%s]", seqName ) ;

            // update sequence to add CLUniqueID field
            matcher = BSON( CAT_SEQUENCE_NAME << seqName ) ;
            updator =
                  BSON( "$set" <<
                        BSON( CAT_SEQUENCE_CLUID << (INT64)clUniqueID ) ) ;
            updateOptions.setCLFullName( GTS_SEQUENCE_COLLECTION_NAME ) ;
            updateOptions.setQuery( matcher ) ;

            rc = rtnUpdate( updateOptions, updator, _eduCB, _dmsCB, dpsCB ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add field [%s] to "
                         "sequence [%s], rc: %d", CAT_SEQUENCE_CLUID,
                         CAT_SEQUENCE_NAME, rc ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

      // 3. add NULL collection unique ID to user sequences
      try
      {
         userSeqUpdator =
               BSON( "$set" <<
                     BSON( CAT_SEQUENCE_CLUID << UTIL_UNIQUEID_NULL ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
      options.setQuery( noExistMatcher ) ;
      rc = rtnUpdate( options, userSeqUpdator, _eduCB, _dmsCB, dpsCB ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add field [%s] to "
                   "user sequences, rc: %d", CAT_SEQUENCE_CLUID, rc ) ;

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, _eduCB, rtnCB ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC( SDB_GTS_MGR__CHKUPGSEQCLUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}

