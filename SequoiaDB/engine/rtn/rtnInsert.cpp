/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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

namespace engine
{
   #define RTN_INSERT_ONCE_NUM         (10)

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNINSERT1, "rtnInsert" )
   INT32 rtnInsert ( const CHAR *pCollectionName, BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, INT32 *pInsertedNum,
                     INT32 *pIgnoredNum )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNINSERT1 ) ;
      pmdKRCB *krcb = pmdGetKRCB () ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB () ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB () ;

      if ( dpsCB && cb->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }
      rc = rtnInsert ( pCollectionName, objs, objNum, flags, cb,
                       dmsCB, dpsCB, 1, pInsertedNum ) ;
      PD_TRACE_EXITRC ( SDB_RTNINSERT1, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNINSERT2, "rtnInsert" )
   INT32 rtnInsert ( const CHAR *pCollectionName, BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_DPSCB *dpsCB, INT16 w, INT32 *pInsertedNum,
                     INT32 *pIgnoredNum )
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
      INT32  ignoredNum = 0 ;
      INT32  allInsertNum = 0 ;
      BOOLEAN writable = FALSE ;

      ossValuePtr pDataPos = 0 ;
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

         try
         {
            BSONObj record ( (const CHAR*)pDataPos ) ;
            rc = su->insertRecord ( pCollectionShortName, record, cb, dpsCB ) ;
            if ( rc )
            {
               if ( ( SDB_IXM_DUP_KEY == rc ) &&
                    ( FLG_INSERT_CONTONDUP & flags ) )
               {
                  ++ignoredNum ;
                  rc = SDB_OK ;
               }
               else
               {
                  PD_LOG ( PDERROR, "Failed to insert record %s into "
                           "collection: %s, rc: %d", record.toString().c_str(),
                           pCollectionName, rc ) ;
                  goto error ;
               }
            }
            ++allInsertNum ;
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
      if ( pInsertedNum )
      {
         *pInsertedNum = allInsertNum ;
      }
      if ( pIgnoredNum )
      {
         *pIgnoredNum = ignoredNum ;
      }
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
   INT32 rtnReplayInsert( const CHAR *pCollectionName, BSONObj &obj,
                          INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_DPSCB *dpsCB, INT16 w )
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
      BSONElement positionELe ;
      INT64 position = -1 ;

      rc = dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc ) ;
      writable = TRUE ;

      if ( obj.isEmpty() )
      {
         PD_LOG( PDERROR, "Insert record can't be empty" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock( pCollectionName, dmsCB, &su,
                                            &clShortName, suID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s",
                   pCollectionName ) ;

      if ( DMS_STORAGE_CAPPED == su->type() )
      {
         positionELe = obj.getField( DMS_ID_KEY_NAME ) ;
         if ( NumberLong != positionELe.type() )
         {
            PD_LOG( PDERROR, "Field _id type[ %d ] is not as expected"
                    "[ %d ]", positionELe.type(), NumberLong ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         position = positionELe.numberLong() ;
      }

      rc = su->insertRecord( clShortName, obj, cb, dpsCB, TRUE,
                             TRUE, NULL, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Insert record by position[ %lld ] failed, "
                   "rc: %d", positionELe.numberLong(), rc) ;

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
}

