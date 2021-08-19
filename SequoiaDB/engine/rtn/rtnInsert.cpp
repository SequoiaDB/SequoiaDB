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

   Source File Name = rtnInsert.cpp

   Descriptive Name = Runtime Insert

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageJob.hpp"
#include "ossTypes.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "utilInsertResult.hpp"

using namespace bson;

namespace engine
{
   #define RTN_INSERT_ONCE_NUM         (10)

   static BSONObj generateUpdator( const BSONObj &record ) ;

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINSERTREC, "_rtnInsertRecord" )
   static INT32 _rtnInsertRecord( dmsStorageUnit *su,
                                  const CHAR *clFullName,
                                  const CHAR *clShortName,
                                  const BSONObj &record,
                                  INT32 flags,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB,
                                  BOOLEAN mustOID,
                                  BOOLEAN canUnLock,
                                  dmsMBContext *context,
                                  INT64 position,
                                  utilInsertResult *insertResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNINSERTREC ) ;

      SDB_ASSERT( NULL != su, "su is invalid" ) ;
      SDB_ASSERT( NULL != clFullName, "collection full name is invalid" ) ;
      SDB_ASSERT( NULL != clShortName, "collection short name is invalid" ) ;
      SDB_ASSERT( NULL != insertResult, "insert result is invalid" ) ;

      rc = su->insertRecord( clShortName, record, cb, dpsCB,
                             mustOID, canUnLock, context, position,
                             insertResult ) ;
      // check return code
      if ( SDB_IXM_DUP_KEY == rc )
      {
         if ( FLG_INSERT_CONTONDUP & flags )
         {
            insertResult->incDuplicatedNum();
            insertResult->resetInfo() ;
            // skip duplicate key error
            rc = SDB_OK ;
         }
         else if ( FLG_INSERT_REPLACEONDUP & flags )
         {
            // update record when duplicate key error
            BSONObj hint ;
            BSONObj updator ;
            BSONObj matcher ;
            utilUpdateResult upResult ;

            utilIdxDupErrAssit dupErrAssit( insertResult->getIdxKeyPattern(),
                                            insertResult->getIdxValue() ) ;

            rc = dupErrAssit.getIdxMatcher( matcher ) ;
            if ( rc )
            {
               goto error ;
            }
            insertResult->resetInfo() ;

            updator = generateUpdator( record ) ;
            rc = rtnUpdate( clFullName, matcher, updator, hint,
                            0, cb, &upResult ) ;
            if ( rc )
            {
               insertResult->setErrInfo( &upResult ) ;

               PD_LOG( PDERROR, "Failed to update record[%s] in "
                       "collection[%s] when insert exists duplicate key, "
                       "rc: %d", record.toString().c_str(), clFullName,
                       rc ) ;
               goto error ;
            }
            else
            {
               // update success.
               insertResult->incDuplicatedNum( upResult.updateNum() ) ;
               rc = SDB_OK ;
            }
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to insert record %s into "
                     "collection: %s, rc: %d",
                     record.toPoolString().c_str(),
                     clFullName, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNINSERTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNINSERT1, "rtnInsert" )
   INT32 rtnInsert ( const CHAR *pCollectionName,
                     const BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, utilInsertResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNINSERT1 ) ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB () ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB () ;
      //EDUID eduId = cb->getID() ;

      if ( dpsCB && cb->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }
      rc = rtnInsert ( pCollectionName, objs, objNum, flags, cb,
                       dmsCB, dpsCB, 1, pResult ) ;
      PD_TRACE_EXITRC ( SDB_RTNINSERT1, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNINSERT2, "rtnInsert" )
   INT32 rtnInsert ( const CHAR *pCollectionName,
                     const BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_DPSCB *dpsCB, INT16 w, utilInsertResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNINSERT2 ) ;
      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;
      UINT32 insertCount = 0 ;
      BOOLEAN writable = FALSE ;
      ossValuePtr pDataPos = 0 ;
      utilInsertResult inTmpResult ;

      if ( !pResult )
      {
         pResult = &inTmpResult ;
      }

      rc = dmsCB->writable( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Database is not writable, rc = %d", rc ) ;
         goto error;
      }
      writable = TRUE;

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  pCollectionName ) ;
         goto error ;
      }

      if ( objs.isEmpty () )
      {
         PD_LOG ( PDERROR, "Insert record can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( FLG_INSERT_CONTONDUP & flags )
      {
         pResult->disableIndexErrInfo() ;
      }

      pDataPos = (ossValuePtr)objs.objdata() ;
      for ( INT32 i = 0 ; i < objNum ; ++i )
      {
         if ( ++insertCount > RTN_INSERT_ONCE_NUM )
         {
            insertCount = 0 ;
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
         }

         pResult->resetInfo() ;

         try
         {
            BSONObj record ( (const CHAR*)pDataPos ) ;

            rc = _rtnInsertRecord( su, pCollectionName, pCollectionShortName,
                                   record, flags, cb, dpsCB, TRUE, TRUE, NULL,
                                   -1, pResult ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to insert record into "
                         "collection [%s], rc: %d", pCollectionName, rc ) ;

            pDataPos += ossAlignX ( (ossValuePtr)record.objsize(), 4 ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to convert to BSON and insert to "
                     "collection: %s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb );
      }
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_RTNINSERT2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREPLAYINERT, "rtnReplayInsert" )
   INT32 rtnReplayInsert( const CHAR *pCollectionName, const BSONObj &obj,
                          INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_DPSCB *dpsCB, INT16 w,
                          utilInsertResult *pResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNREPLAYINERT ) ;
      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *clShortName = NULL ;
      BOOLEAN writable = FALSE ;
      BSONElement positionEle ;
      INT64 position = -1 ;

      utilInsertResult inTmpResult ;

      if ( !pResult )
      {
         pResult = &inTmpResult ;
      }

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      if ( obj.isEmpty() )
      {
         SDB_ASSERT( FALSE, "Insert record can't be empty" ) ;
         PD_LOG( PDERROR, "Insert record can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock( pCollectionName, dmsCB, &su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s",
                   pCollectionName ) ;

      try
      {
         // Insertion replaying on capped collection should be done by position,
         // as the record positions on primary and slavery nodes should be
         // exactly the same.
         if ( DMS_STORAGE_CAPPED == su->type() )
         {
            positionEle = obj.getField( DMS_ID_KEY_NAME ) ;
            if ( NumberLong != positionEle.type() )
            {
               PD_LOG( PDERROR, "Field _id type[ %d ] is not as expected"
                       "[ %d ]", positionEle.type(), NumberLong ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            position = positionEle.numberLong() ;
         }

         rc = _rtnInsertRecord( su, pCollectionName, clShortName, obj, flags,
                                cb, dpsCB, TRUE, TRUE, NULL, position,
                                pResult ) ;
         if ( SDB_OK != rc )
         {
            if ( DMS_STORAGE_CAPPED == su->type() )
            {
               PD_LOG( PDERROR, "Failed to insert record into collection [%s] "
                       "by position[%lld], rc: %d", pCollectionName,
                       positionEle.numberLong(), rc ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to insert record into collection [%s] "
                       "by replay, rc: %d", pCollectionName, rc ) ;
            }
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      if ( writable )
      {
         dmsCB->writeDown( cb ) ;
      }
      if ( cb )
      {
         if ( SDB_OK == rc && dpsCB )
         {
            rc = dpsCB->completeOpr( cb, w ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_RTNREPLAYINERT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj generateUpdator( const BSONObj &record )
   {
      return BSON( "$replace" << record
                   << "$keep" << BSON( DMS_ID_KEY_NAME << 1) ) ;
   }
}

