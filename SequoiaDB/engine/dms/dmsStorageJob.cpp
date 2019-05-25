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

   Source File Name = dmsStorageJob.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/10/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageJob.hpp"
#include "dmsStorageBase.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "ixmExtent.hpp"
#include "pmd.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      Local define
   */
   #define DMS_MAPPING_JOB_INTERVAL          ( 5 * OSS_ONE_SEC )
   #define DMS_MAPPING_JOB_TIMEOUT           ( 600 * OSS_ONE_SEC )
   #define DMS_MAX_MAPPING_JOB               ( 3 )
   #define DMS_MIN_MAPPING_JOB               ( 2 )

   /*
      _dmsExtendSegmentJob implement
   */

   _dmsExtendSegmentJob::_dmsExtendSegmentJob( dmsStorageBase * pSUBase )
   {
      SDB_ASSERT( pSUBase, "Storage base unit can't be NULL" ) ;
      _pSUBase = pSUBase ;

      ossSnprintf( _name, sizeof( _name ), "Job[ExtendSegment-%s]",
                   _pSUBase->getSuName() ) ;
   }

   _dmsExtendSegmentJob::~_dmsExtendSegmentJob()
   {
      _pSUBase = NULL ;
   }

   RTN_JOB_TYPE _dmsExtendSegmentJob::type() const
   {
      return RTN_JOB_EXTENDSEGMENT ;
   }

   const CHAR* _dmsExtendSegmentJob::name () const
   {
      return _name ;
   }

   BOOLEAN _dmsExtendSegmentJob::muteXOn( const _rtnBaseJob * pOther )
   {
      return FALSE ;
   }

   INT32 _dmsExtendSegmentJob::doit ()
   {
      return _pSUBase->_preExtendSegment() ;
   }

   /*
      _dmsPageMappingDispatcher implement
   */
   _dmsPageMappingDispatcher::_dmsPageMappingDispatcher()
   {
      _emptyEvent.signal() ;

      _startCtrlJob  = FALSE ;
      _curAgent      = 0 ;
      _idleAgent     = 0 ;
   }

   _dmsPageMappingDispatcher::~_dmsPageMappingDispatcher()
   {
   }

   INT32 _dmsPageMappingDispatcher::active()
   {
      INT32 rc = SDB_OK ;

      _checkAndStartJob( TRUE ) ;

      if ( FALSE == _startCtrlJob )
      {
         PD_LOG( PDERROR, "Start page mapping job controller failed" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsPageMappingDispatcher::dispatchItem( monCSName &item )
   {
      BOOLEAN hasDispatch = FALSE ;

      ossScopedLock lock( &_latch ) ;

      if ( !_vecCSName.empty() )
      {
         item = _vecCSName.back() ;
         _vecCSName.pop_back() ;

         hasDispatch = TRUE ;
         --_idleAgent ;

         if ( _vecCSName.empty() )
         {
            _emptyEvent.signal() ;
            _ntyEvent.reset() ;
         }
      }

      return hasDispatch ;
   }

   void _dmsPageMappingDispatcher::endDispatch()
   {
      _latch.get() ;
      ++_idleAgent ;
      _latch.release() ;
   }

   UINT32 _dmsPageMappingDispatcher::prepare()
   {
      UINT32 count = 0 ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      MON_CSNAME_VEC vecCS ;

      _latch.get() ;
      count = _vecCSName.size() ;
      _latch.release() ;

      if ( count > 0 )
      {
         goto done ;
      }

      dmsCB->dumpPageMapCSInfo( vecCS ) ;

      if ( !vecCS.empty() )
      {
         count = vecCS.size() ;
         ossScopedLock lock( &_latch ) ;
         _vecCSName = vecCS ;

         _ntyEvent.signalAll() ;

         while ( _curAgent < DMS_MIN_MAPPING_JOB ||
                 ( count / 10 > _idleAgent &&
                   _curAgent < DMS_MAX_MAPPING_JOB ) )
         {
            if ( SDB_OK == dmsStartMappingJob( NULL, this,
                                               DMS_MAPPING_JOB_TIMEOUT ) )
            {
               ++_curAgent ;
               ++_idleAgent ;
            }
            else
            {
               break ;
            }
         }
      }

   done:
      return count ;
   }

   ossEvent* _dmsPageMappingDispatcher::getEmptyEvent()
   {
      return &_emptyEvent ;
   }

   ossEvent* _dmsPageMappingDispatcher::getNtyEvent()
   {
      return &_ntyEvent ;
   }

   void _dmsPageMappingDispatcher::exitJob( BOOLEAN isControl )
   {
      _latch.get() ;

      --_curAgent ;
      --_idleAgent ;
      if ( isControl )
      {
         _startCtrlJob = FALSE ;
      }
      _checkAndStartJob( FALSE ) ;

      _latch.release() ;
   }

   void _dmsPageMappingDispatcher::_checkAndStartJob( BOOLEAN needLock )
   {
      if ( needLock )
      {
         _latch.get() ;
      }
      if ( FALSE == _startCtrlJob && !pmdIsQuitApp() )
      {
         if ( SDB_OK == dmsStartMappingJob( NULL, this, -1 ) )
         {
            ++_curAgent ;
            ++_idleAgent ;
            _startCtrlJob = TRUE ;
         }
      }
      if ( needLock )
      {
         _latch.release() ;
      }
   }

   /*
      _dmsPageMappingJob implement
   */
   _dmsPageMappingJob::_dmsPageMappingJob( dmsPageMappingDispatcher *pDispatcher,
                                           INT32 timeout )
   {
      _pDispatcher = pDispatcher ;
      _timeout = timeout ;
   }

   _dmsPageMappingJob::~_dmsPageMappingJob()
   {
   }

   RTN_JOB_TYPE _dmsPageMappingJob::type() const
   {
      return RTN_JOB_PAGEMAPPING ;
   }

   const CHAR* _dmsPageMappingJob::name() const
   {
      if ( isControlJob() )
      {
         return "PAGEMAPPING-JOB-D" ;
      }
      return "PAGEMAPPING-JOB" ;
   }

   BOOLEAN _dmsPageMappingJob::isControlJob() const
   {
      return _timeout < 0 ? TRUE : FALSE ;
   }

   BOOLEAN _dmsPageMappingJob::muteXOn( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _dmsPageMappingJob::doit()
   {
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      UINT32 timeout = 0 ;

      pEDUMgr->activateEDU( eduCB() ) ;

      while( !eduCB()->isForced() )
      {
         if ( isControlJob() )
         {
            _pDispatcher->getEmptyEvent()->wait( DMS_MAPPING_JOB_INTERVAL ) ;
            _pDispatcher->prepare() ;
            eduCB()->incEventCount( 1 ) ;
         }
         else
         {
            pEDUMgr->waitEDU( eduCB() ) ;
            monCSName item ;

            if ( _pDispatcher->dispatchItem( item ) )
            {
               timeout = 0 ;
               _doUnit( &item ) ;
               _pDispatcher->endDispatch() ;
            }
            else
            {
               if ( SDB_TIMEOUT == _pDispatcher->getNtyEvent()->wait(
                                   DMS_MAPPING_JOB_INTERVAL ) )
               {
                  timeout += DMS_MAPPING_JOB_INTERVAL ;
               }
               else
               {
                  timeout += 100 ;
               }

               if ( timeout >= (UINT32)_timeout )
               {
                  break ;
               }
            }
         }
      }

      _pDispatcher->exitJob( isControlJob() ) ;

      return SDB_OK ;
   }

   void _dmsPageMappingJob::_doUnit( const monCSName *pItem )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      dmsPageMapUnit *pMapUnit = NULL ;
      dmsPageMap *pPageMap = NULL ;
      UINT16 slot = 0 ;
      dmsMBContext *mbContext = NULL ;

      pEDUMgr->activateEDU( eduCB() ) ;

      rc = dmsCB->nameToSUAndLock( pItem->_csName, suID, &su ) ;
      if ( rc )
      {
         goto done ;
      }

      pMapUnit = su->index()->getPageMapUnit() ;
      pPageMap = pMapUnit->beginNonEmpty( slot ) ;
      while( pPageMap )
      {
         rc = su->data()->getMBContext( &mbContext, slot,
                                        DMS_INVALID_CLID, -1 ) ;
         if ( SDB_OK == rc )
         {
            _doACollection( su, mbContext, pPageMap ) ;
            su->data()->releaseMBContext( mbContext ) ;
         }

         pPageMap = pMapUnit->nextNonEmpty( slot ) ;
      }

   done:
      pEDUMgr->waitEDU( eduCB() ) ;
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
   }

   void _dmsPageMappingJob::_doACollection( _dmsStorageUnit *su,
                                            _dmsMBContext *mbContext,
                                            dmsPageMap *pPageMap )
   {
      INT32 rc = SDB_OK ;
      UINT64 curPageNum = 0 ;
      dmsPageMap::MAP_PAGES_IT it ;

      curPageNum = pPageMap->size() ;

      while( curPageNum > 0 && !pPageMap->isEmpty() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         if ( rc )
         {
            goto done ;
         }
         it = pPageMap->begin() ;
         if ( it == pPageMap->end() )
         {
            goto done ;
         }
         else
         {
            ixmExtent extent( it->first, su->index() ) ;
            extent.setParent( it->second, FALSE ) ;
            pPageMap->erase( it ) ;

            eduCB()->incEventCount( 1 ) ;
            --curPageNum ;
         }
         mbContext->mbUnlock() ;
      }

   done:
      mbContext->mbUnlock() ;
   }

   /*
      Function implement
   */
   INT32 startExtendSegmentJob( EDUID * pEDUID, _dmsStorageBase * pSUBase )
   {
      INT32 rc                      = SDB_OK ;
      dmsExtendSegmentJob * pJob    = NULL ;

      pJob = SDB_OSS_NEW dmsExtendSegmentJob ( pSUBase ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 dmsStartMappingJob( EDUID *pEDUID,
                             dmsPageMappingDispatcher *pDispatcher,
                             INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      dmsPageMappingJob *pJob = NULL ;

      pJob = SDB_OSS_NEW dmsPageMappingJob( pDispatcher, timeout ) ;
      if ( !pJob )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc mapping job failed" ) ;
         goto error ;
      }
      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_NONE, pEDUID  ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

