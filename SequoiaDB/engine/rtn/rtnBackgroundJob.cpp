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

   Source File Name = rtnBackgroundJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/06/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnBackgroundJob.hpp"
#include "rtn.hpp"
#include "ixm.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageLoadExtent.hpp"
#include "rtnRecover.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   ////////////////////////////////////////////////////////////////////////////
   // background job implements //
   ////////////////////////////////////////////////////////////////////////////

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINDEXJOB__RTNINDEXJOB, "_rtnIndexJob::_rtnIndexJob" )
   _rtnIndexJob::_rtnIndexJob ( RTN_JOB_TYPE type, const CHAR *pCLName,
                                const BSONObj & indexObj, SDB_DPSCB * dpsCB,
                                UINT64 offset, BOOLEAN isRollBack )
   {
      PD_TRACE_ENTRY ( SDB__RTNINDEXJOB__RTNINDEXJOB ) ;
      _type = type ;
      ossStrncpy ( _clFullName, pCLName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _clFullName[DMS_COLLECTION_FULL_NAME_SZ] = 0 ;
      _indexObj = indexObj.copy() ;
      _hasAddUnique = FALSE ;
      _csLID = DMS_INVALID_LOGICCSID ;
      _clLID = DMS_INVALID_LOGICCLID ;
      _dpsCB = dpsCB ;
      _dmsCB = pmdGetKRCB()->getDMSCB() ;
      _lsn = offset ;
      _isRollback = isRollBack ;
      _regCLJob = FALSE ;
      PD_TRACE_EXIT ( SDB__RTNINDEXJOB__RTNINDEXJOB ) ;
   }

   _rtnIndexJob::~_rtnIndexJob ()
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      const CHAR *pCLShortName = NULL ;

      if ( _hasAddUnique )
      {
         rc = rtnResolveCollectionNameAndLock ( _clFullName, _dmsCB,
                                                &su, &pCLShortName,
                                                suID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                     _clFullName ) ;
            goto error ;
         }
         if ( _csLID != su->LogicalCSID() )
         {
            goto done ;
         }

         rc = su->data()->getMBContext( &mbContext, pCLShortName,
                                        EXCLUSIVE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Lock collection[%s] failed, rc = %d",
                     _clFullName, rc ) ;
            goto error ;
         }
         if ( _clLID != mbContext->clLID() )
         {
            goto done ;
         }

         mbContext->mbStat()->_uniqueIdxNum-- ;
         _hasAddUnique = FALSE ;
      }

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         _dmsCB->suUnlock( suID ) ;
      }
      // unregister collection index job if needed
      if ( _regCLJob )
      {
         rtnGetIndexJobHolder()->unregCLJob( _clFullName ) ;
         _regCLJob = FALSE ;
      }
      return ;
   error:
      goto done ;
   }

   INT32 _rtnIndexJob::checkIndexExist( const CHAR *pCLName,
                                        const CHAR *pIdxName,
                                        BOOLEAN &hasExist )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      const CHAR *pCLShortName = NULL ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsExtentID idxExtent = DMS_INVALID_EXTENT ;

      // set not exist in first
      hasExist = FALSE ;

      rc = rtnResolveCollectionNameAndLock ( pCLName, dmsCB,
                                             &su, &pCLShortName,
                                             suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  pCLName ) ;
         goto error ;
      }

      rc = su->data()->getMBContext( &mbContext, pCLShortName, SHARED ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Lock collection[%s] failed, rc = %d",
                  pCLName, rc ) ;
         goto error ;
      }

      /// get index
      rc = su->index()->getIndexCBExtent( mbContext, pIdxName, idxExtent ) ;
      if ( SDB_OK == rc )
      {
         hasExist = TRUE ;
      }
      else if ( SDB_IXM_NOTEXIST == rc )
      {
         // report not exist, and ignore error
         rc = SDB_OK ;
      }

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINDEXJOB_INIT, "_rtnIndexJob::init" )
   INT32 _rtnIndexJob::init ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNINDEXJOB_INIT ) ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      const CHAR *pCLShortName = NULL ;

      rc = rtnResolveCollectionNameAndLock ( _clFullName, _dmsCB,
                                             &su, &pCLShortName,
                                             suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  _clFullName ) ;
         goto error ;
      }

      switch ( _type )
      {
         case RTN_JOB_CREATE_INDEX :
            {
               _jobName = "CreateIndex-" ;
               // need to get the index name
               _indexName = _indexObj.getStringField( IXM_NAME_FIELD ) ;

               if ( _indexObj.getBoolField( IXM_UNIQUE_FIELD ) )
               {
                  rc = su->data()->getMBContext( &mbContext, pCLShortName,
                                                 EXCLUSIVE ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG ( PDERROR, "Lock collection[%s] failed, rc = %d",
                              _clFullName, rc ) ;
                     goto error ;
                  }

                  mbContext->mbStat()->_uniqueIdxNum++ ;
                  _hasAddUnique = TRUE ;
                  _csLID = su->LogicalCSID() ;
                  _clLID = mbContext->clLID() ;
               }
            }
            break ;
         case RTN_JOB_DROP_INDEX :
            {
               dmsExtentID idxExtent = DMS_INVALID_EXTENT ;
               _jobName = "DropIndex-" ;
               // need to get the index name
               _indexEle = _indexObj.getField( IXM_NAME_FIELD ) ;
               if ( _indexEle.eoo() )
               {
                  _indexEle = _indexObj.firstElement () ;
               }

               rc = su->data()->getMBContext( &mbContext, pCLShortName,
                                              EXCLUSIVE ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG ( PDERROR, "Lock collection[%s] failed, rc = %d",
                           _clFullName, rc ) ;
                  goto error ;
               }

               if ( jstOID == _indexEle.type() )
               {
                  OID oid ;
                  _indexEle.Val( oid ) ;
                  // get index extent
                  rc = su->index()->getIndexCBExtent( mbContext, oid,
                                                      idxExtent ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDWARNING, "Get collection[%s] indexCB[%s] extent "
                              "failed, rc: %d", _clFullName,
                              oid.str().c_str(), rc ) ;
                     /// ignore the error
                     rc = SDB_OK ;
                  }
                  else
                  {
                     ixmIndexCB indexCB ( idxExtent, su->index(), mbContext ) ;
                     _indexName = indexCB.getName() ;
                     /// first set index flag to IXM_INDEX_FLAG_INVALID
                     indexCB.setFlag( IXM_INDEX_FLAG_INVALID ) ;
                  }
               }
               else
               {
                  _indexName = _indexEle.str () ;
                  // get index extent
                  rc = su->index()->getIndexCBExtent( mbContext,
                                                      _indexName.c_str(),
                                                      idxExtent ) ;
                  if ( rc )
                  {
                     PD_LOG( PDWARNING, "Get collection[%s] indexCB[%s] extent "
                             "failed, rc: %d", _clFullName,
                             _indexName.c_str(), rc ) ;
                     /// ignore the error
                     rc = SDB_OK ;
                  }
                  else
                  {
                     ixmIndexCB indexCB ( idxExtent, su->index(), mbContext ) ;
                     /// first set index flag to IXM_INDEX_FLAG_INVALID
                     indexCB.setFlag( IXM_INDEX_FLAG_INVALID ) ;
                  }
               }

               // register drop index job to prevent other operators to be
               // executed before drop index is finished ( e.g. truncate )
               rc = rtnGetIndexJobHolder()->regCLJob( _clFullName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to register drop index job "
                            "for collection [%s], rc: %d", _clFullName, rc ) ;
               _regCLJob = TRUE ;
            }
            break ;
         default :
            _jobName = "UnknowIndexJob" ;
            PD_LOG ( PDERROR, "Index job not support this type[%d]", _type ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      if ( SDB_OK == rc )
      {
         _jobName += _clFullName ;
         _jobName += "[" ;
         _jobName += _indexName ;
         _jobName += "]" ;
      }

   done:
      if ( mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         _dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__RTNINDEXJOB_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _rtnIndexJob::getIndexName () const
   {
      return _indexName.c_str() ;
   }

   const CHAR* _rtnIndexJob::getCollectionName() const
   {
      return _clFullName ;
   }

   RTN_JOB_TYPE _rtnIndexJob::type () const
   {
      return _type ;
   }

   const CHAR* _rtnIndexJob::name () const
   {
      return _jobName.c_str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINDEXJOB_MUTEXON, "_rtnIndexJob::muteXOn" )
   BOOLEAN _rtnIndexJob::muteXOn ( const _rtnBaseJob * pOther )
   {
      PD_TRACE_ENTRY ( SDB__RTNINDEXJOB_MUTEXON ) ;
      BOOLEAN ret = FALSE;
      if ( RTN_JOB_CREATE_INDEX != pOther->type() &&
           RTN_JOB_DROP_INDEX != pOther->type() )
      {
         ret = FALSE ;
         goto done ;
      }

      {
         _rtnIndexJob *pIndexJob = ( _rtnIndexJob* )pOther ;

         if ( 0 == ossStrcmp( getIndexName(), pIndexJob->getIndexName() ) &&
              0 == ossStrcmp( getCollectionName(),
                              pIndexJob->getCollectionName() ) )
         {
            ret = TRUE ;
            goto done ;
         }
      }
   done :
      PD_TRACE_EXIT ( SDB__RTNINDEXJOB_MUTEXON ) ;
      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNINDEXJOB_DOIT , "_rtnIndexJob::doit" )
   INT32 _rtnIndexJob::doit ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNINDEXJOB_DOIT ) ;

      if ( !_dpsCB )
      {
         eduCB()->insertLsn( _lsn, _isRollback ) ;
      }

      switch ( _type )
      {
         case RTN_JOB_CREATE_INDEX :
            rc = rtnCreateIndexCommand( _clFullName, _indexObj, eduCB(),
                                        _dmsCB, _dpsCB, TRUE,
                                        SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ) ;
            break ;
         case RTN_JOB_DROP_INDEX :
            rc = rtnDropIndexCommand( _clFullName, _indexEle, eduCB(),
                                      _dmsCB, _dpsCB, TRUE ) ;
            break ;
         default :
            PD_LOG ( PDERROR, "Index job not support this type[%d]", _type ) ;
            rc = SDB_INVALIDARG ;
            break ;
      }

      PD_TRACE_EXITRC ( SDB__RTNINDEXJOB_DOIT, rc ) ;
      return rc ;
   }

   /*
      _rtnIndexJobHolder implement
    */
   _rtnIndexJobHolder::_rtnIndexJobHolder()
   {
   }

   _rtnIndexJobHolder::~_rtnIndexJobHolder()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIDXJOBHOLDER_HASCLJOB, "_rtnIndexJobHolder::hasCLJob" )
   BOOLEAN _rtnIndexJobHolder::hasCLJob( const CHAR *collection )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNIDXJOBHOLDER_HASCLJOB ) ;

      ossScopedLock lock( &_mapLatch, SHARED ) ;

      try
      {
         ossPoolString tmpCLName( collection ) ;
         res = _hasCLJob( tmpCLName ) ;
      }
      catch ( ... )
      {
         // failed to construct key, find by iterator
         res = _hasCLJobIter( collection ) ;
      }

      PD_TRACE_EXIT( SDB__RTNIDXJOBHOLDER_HASCLJOB ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNIDXJOBHOLDER_REGCLJOB, "_rtnIndexJobHolder::regCLJob" )
   INT32 _rtnIndexJobHolder::regCLJob( const CHAR *collection )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNIDXJOBHOLDER_REGCLJOB ) ;

      SDB_ASSERT( NULL != collection, "collection is invalid" ) ;

      ossScopedLock lock( &_mapLatch, EXCLUSIVE ) ;

      try
      {
         ossPoolString tmpCLName( collection ) ;
         CL_JOB_MAP::iterator iterCL = _clJobs.find( tmpCLName ) ;
         if ( iterCL != _clJobs.end() )
         {
            // found existing, increase count
            ++ ( iterCL->second ) ;
         }
         else
         {
            // not found, create new count
            _clJobs.insert( make_pair( collection, 1 ) ) ;

         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to register collection job [%s], "
                 "occur exception: %s", collection, e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNIDXJOBHOLDER_REGCLJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR_UNREGCLJOB, "_rtnIndexJobHolder::unregCLJob" )
   void _rtnIndexJobHolder::unregCLJob( const CHAR *collection )
   {
      PD_TRACE_ENTRY( SDB__RTNJOBMGR_UNREGCLJOB ) ;

      ossScopedLock lock( &_mapLatch, EXCLUSIVE ) ;

      try
      {
         ossPoolString tmpCLName( collection ) ;
         _unregCLJob( tmpCLName ) ;
      }
      catch ( ... )
      {
         // failed to construct key, find by iterator
         _unregCLJobIter( collection ) ;
      }

      PD_TRACE_EXIT( SDB__RTNJOBMGR_UNREGCLJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR_FINI, "_rtnIndexJobHolder::fini" )
   void _rtnIndexJobHolder::fini()
   {
      PD_TRACE_ENTRY( SDB__RTNJOBMGR_FINI ) ;

      ossScopedLock _lock( &_mapLatch, EXCLUSIVE ) ;
      _clJobs.clear() ;

      PD_TRACE_EXIT( SDB__RTNJOBMGR_FINI ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR__UNREGCLJOB, "_rtnIndexJobHolder::_unregCLJob" )
   void _rtnIndexJobHolder::_unregCLJob( const ossPoolString &collection )
   {
      PD_TRACE_ENTRY( SDB__RTNJOBMGR__UNREGCLJOB ) ;

      CL_JOB_MAP::iterator iterCL = _clJobs.find( collection ) ;
      SDB_ASSERT( iterCL != _clJobs.end(), "collection job is not found" ) ;
      if ( iterCL != _clJobs.end() )
      {
         if ( iterCL->second > 1 )
         {
            // decrease count
            -- ( iterCL->second ) ;
         }
         else
         {
            // no more jobs for this collection
            _clJobs.erase( iterCL ) ;
         }
      }

      PD_TRACE_EXIT( SDB__RTNJOBMGR__UNREGCLJOB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR__UNREGCLJOBITER, "_rtnIndexJobHolder::_unregCLJobIter" )
   void _rtnIndexJobHolder::_unregCLJobIter( const CHAR *collection )
   {
      PD_TRACE_ENTRY( SDB__RTNJOBMGR__UNREGCLJOBITER ) ;

      CL_JOB_MAP::iterator iterCL = _clJobs.begin() ;
      while ( iterCL != _clJobs.end() )
      {
         if ( 0 == ossStrcmp( iterCL->first.c_str(),
                              collection ) )
         {
            // found by name
            if ( iterCL->second > 1 )
            {
               // decrease count
               -- ( iterCL->second ) ;
            }
            else
            {
               // no more jobs for this collection
               _clJobs.erase( iterCL ) ;
            }
            break ;
         }
         ++ iterCL ;
      }

      PD_TRACE_EXIT( SDB__RTNJOBMGR__UNREGCLJOBITER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR__HASCLJOB, "_rtnIndexJobHolder::_hasCLJob" )
   BOOLEAN _rtnIndexJobHolder::_hasCLJob( const ossPoolString &collection )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNJOBMGR__HASCLJOB ) ;

      CL_JOB_MAP::iterator iter = _clJobs.find( collection ) ;
      if ( _clJobs.end() != iter &&
           iter->second > 0 )
      {
         res = TRUE ;
      }

      PD_TRACE_EXIT( SDB__RTNJOBMGR__HASCLJOB ) ;

      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR__HASCLJOBITER, "_rtnIndexJobHolder::_hasCLJobIter" )
   BOOLEAN _rtnIndexJobHolder::_hasCLJobIter( const CHAR *collection )
   {
      BOOLEAN res = FALSE ;

      PD_TRACE_ENTRY( SDB__RTNJOBMGR__HASCLJOBITER ) ;

      CL_JOB_MAP::iterator iterCL = _clJobs.begin() ;
      while ( iterCL != _clJobs.end() )
      {
         if ( 0 == ossStrcmp( iterCL->first.c_str(),
                              collection ) )
         {
            // found by name
            if ( iterCL->second > 0 )
            {
               res = TRUE ;
            }
            break ;
         }
         ++ iterCL ;
      }

      PD_TRACE_EXIT( SDB__RTNJOBMGR__HASCLJOBITER ) ;

      return res ;
   }

   rtnIndexJobHolder *rtnGetIndexJobHolder()
   {
      static rtnIndexJobHolder s_jobHolder ;
      return &s_jobHolder ;
   }

   /*
      _rtnLoadJob implement
   */
   RTN_JOB_TYPE _rtnLoadJob::type () const
   {
      return RTN_JOB_LOAD ;
   }

   const CHAR* _rtnLoadJob::name () const
   {
      return "Load" ;
   }

   BOOLEAN _rtnLoadJob::muteXOn ( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOADJOB_DOIT , "_rtnLoadJob::doit" )
   INT32 _rtnLoadJob::doit ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNLOADJOB_DOIT ) ;
      dmsStorageUnitID  suID     = DMS_INVALID_CS ;
      dmsStorageUnit   *su       = NULL ;
      pmdKRCB          *krcb     = pmdGetKRCB () ;
      SDB_DMSCB        *dmsCB    = krcb->getDMSCB () ;
      pmdEDUMgr        *eduMgr   = krcb->getEDUMgr () ;
      pmdEDUCB         *eduCB    = eduMgr->getEDU() ;
      dmsStorageLoadOp dmsLoadExtent ;
      MON_CS_LIST csList ;
      MON_CS_LIST::iterator it ;

      if ( SDB_ROLE_STANDALONE != krcb->getDBRole() &&
           SDB_ROLE_DATA != krcb->getDBRole() )
      {
         goto done ;
      }

      dmsCB->dumpInfo ( csList ) ;

      for ( it = csList.begin(); it != csList.end(); ++it )
      {
         MON_CL_LIST clList ;
         MON_CL_LIST::iterator itCollection ;
         rc = rtnCollectionSpaceLock ( (*it)._name,
                                       dmsCB,
                                       FALSE,
                                       &su,
                                       suID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to lock collection space, rc=%d", rc ) ;
            goto error ;
         }

         dmsLoadExtent.init ( su ) ;

         su->dumpInfo ( clList, FALSE ) ;
         for ( itCollection = clList.begin();
               itCollection != clList.end();
               ++itCollection )
         {
            dmsMBContext *mbContext = NULL ;
            UINT16 collectionFlag = 0 ;
            const CHAR *pCLNameTemp = NULL ;
            const CHAR *pCLName = (*itCollection)._name ;

            if ( ( ossStrlen ( pCLName ) > DMS_COLLECTION_FULL_NAME_SZ ) ||
                    ( NULL == ( pCLNameTemp = ossStrrchr ( pCLName, '.' ))) )
            {
               PD_LOG ( PDERROR, "collection name is not valid: %s",
                        pCLName ) ;
               continue ;
            }

            rc = su->data()->getMBContext( &mbContext, pCLNameTemp + 1,
                                           EXCLUSIVE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to lock collection: %s, rc: %d",
                       pCLName, rc ) ;
               continue ;
            }
            collectionFlag = mbContext->mb()->_flag ;

            // unlock collection

            if ( DMS_IS_MB_FLAG_LOAD_LOAD ( collectionFlag ) )
            {
               PD_LOG ( PDEVENT, "Start Rollback" ) ;
               rc = dmsLoadExtent.loadRollbackPhase ( mbContext ) ;
               if ( rc )
               {
                  su->data()->releaseMBContext( mbContext ) ;
                  PD_LOG ( PDERROR, "Failed to load Rollback Phase, rc=%d", rc ) ;
                  continue ;
               }
               dmsLoadExtent.clearFlagLoadLoad ( mbContext->mb() ) ;
            }
            if ( DMS_IS_MB_FLAG_LOAD_BUILD ( collectionFlag ) )
            {
               PD_LOG ( PDEVENT, "Start loadBuild" ) ;
               rc = dmsLoadExtent.loadBuildPhase ( mbContext,
                                                   eduCB ) ;
               if ( rc )
               {
                  su->data()->releaseMBContext( mbContext ) ;
                  PD_LOG ( PDERROR, "Failed to load build Phase, rc=%d", rc ) ;
                  continue ;
               }
               dmsLoadExtent.clearFlagLoadBuild ( mbContext->mb() ) ;
            }
            if ( DMS_IS_MB_LOAD ( collectionFlag ) )
            {
               PD_LOG ( PDEVENT, "Start clear load flag" ) ;
               dmsLoadExtent.clearFlagLoad ( mbContext->mb() ) ;
            }

            su->data()->releaseMBContext( mbContext ) ;
         }
         dmsCB->suUnlock ( suID ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNLOADJOB_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _rtnRebuildJob implement
   */
   _rtnRebuildJob::_rtnRebuildJob()
   {
      _pFunc = NULL ;
   }

   _rtnRebuildJob::~_rtnRebuildJob()
   {
   }

   void _rtnRebuildJob::setInfo( RTN_ON_REBUILD_DONE_FUNC pFunc )
   {
      _pFunc = pFunc ;
   }

   RTN_JOB_TYPE _rtnRebuildJob::type() const
   {
      return RTN_JOB_REBUILD ;
   }

   const CHAR* _rtnRebuildJob::name() const
   {
      return "Rebuild" ;
   }

   BOOLEAN _rtnRebuildJob::muteXOn( const _rtnBaseJob *pOther )
   {
      if ( type() == pOther->type() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   INT32 _rtnRebuildJob::doit()
   {
      INT32 rc = SDB_OK ;

      rtnDBRebuilder rebuilder ;
      PMD_SET_DB_STATUS( SDB_DB_REBUILDING ) ;
      rc = rebuilder.doOpr( eduCB() ) ;
      PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to rebuild database, rc: %d, "
                 "shutdown db", rc ) ;
      }

      if ( _pFunc )
      {
         _pFunc( rc ) ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNSTARTLOADJOB, "rtnStartLoadJob" )
   INT32 rtnStartLoadJob()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNSTARTLOADJOB );
      rtnLoadJob *loadJob = SDB_OSS_NEW rtnLoadJob() ;
      if ( NULL == loadJob )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for loadJob" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( loadJob, RTN_JOB_MUTEX_NONE, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start load job, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_RTNSTARTLOADJOB, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 rtnStartRebuildJob( RTN_ON_REBUILD_DONE_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      rtnRebuildJob *pJob = SDB_OSS_NEW rtnRebuildJob() ;
      if ( NULL == pJob )
      {
         PD_LOG ( PDERROR, "Failed to alloc memory for rtnRebuildJob" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      pJob->setInfo( pFunc ) ;
      /// When suc or failed, the job is hold on by job manager,
      /// so don't to release it
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_RET, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to start rebuild job, rc: %d", rc ) ;

         if ( SDB_RTN_MUTEX_JOB_EXIST != rc && pFunc )
         {
            pFunc( rc ) ;
         }
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

}

