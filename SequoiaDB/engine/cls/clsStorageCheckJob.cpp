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

   Source File Name = clsStorageCheckJob.cpp

   Descriptive Name = Storage Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dms.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "monDMS.hpp"
#include "pmd.hpp"
#include "clsTrace.hpp"
#include "clsShardMgr.hpp"
#include "clsMgr.hpp"
#include "rtn.hpp"
#include "rtnContextDel.hpp"
#include "clsStorageCheckJob.hpp"

namespace engine
{
   /*
    *  _clsStorageCheckJob implement
    */
   _clsStorageCheckJob::_clsStorageCheckJob ()
   {
   }

   _clsStorageCheckJob::~_clsStorageCheckJob ()
   {
   }

   INT32 _clsStorageCheckJob::doit ()
   {
      pmdEDUCB *cb = eduCB() ;
      UINT32 checkInterval = pmdGetKRCB()->getOptionCB()->getDmsChkInterval() ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() )
      {
         pmdKRCB *krcb = pmdGetKRCB() ;
         pmdEDUMgr *pEduMgr = krcb->getEDUMgr() ;
         SDB_DMSCB *pDmsCB = krcb->getDMSCB() ;
         shardCB* pShdMgr = sdbGetShardCB() ;
         pmdEDUEvent event ;

         UINT32 secInterval = checkInterval > 0 ?
                              checkInterval * STORAGE_CHECK_UNIT_INTERVAL :
                              STORAGE_CHECK_UNIT_INTERVAL ;

         MON_CS_LIST csList ;

         /*
          * Before any one is found in the queue, the status of this thread is
          * wait. Once found, it will be changed to running.
          */
         pEduMgr->waitEDU( cb ) ;
         cb->waitEvent( event, secInterval ) ;
         pEduMgr->activateEDU( cb ) ;

         if ( PMD_IS_DB_DOWN() ||
              cb->isForced() )
         {
            break ;
         }

         checkInterval = pmdGetKRCB()->getOptionCB()->getDmsChkInterval() ;

         if ( !krcb->isPrimary() ||
              !pShdMgr ||
              checkInterval == 0 )
         {
            PD_LOG( PDDEBUG,
                    "clsStorageCheckJob: job is not enabled, interval: %u",
                    checkInterval ) ;
            continue ;
         }

         PD_LOG( PDDEBUG,
                 "clsStorageCheckJob: start job, interval: %u",
                 checkInterval ) ;

         pDmsCB->dumpInfo( csList, FALSE ) ;

         std::set<_monCollectionSpace>::const_iterator iterCS = csList.begin() ;
         for ( ; iterCS != csList.end(); ++iterCS )
         {
            INT32 rc = SDB_OK ;

            SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;

            const _monCollectionSpace &cs = *iterCS ;
            UINT32 pageSize = DMS_PAGE_SIZE_DFT ;
            UINT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
            DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL ;
            dmsStorageUnitID suID = DMS_INVALID_SUID ;
            dmsStorageUnit *su = NULL ;
            SINT64 contextID = -1 ;
            rtnContextDelCS *pDelContext = NULL ;
            rtnContextBuf buffObj ;

            PD_LOG( PDDEBUG,
                    "clsStorageCheckJob: checking space [%s]",
                    cs._name ) ;

            if ( PMD_IS_DB_DOWN() ||
                 cb->isForced() )
            {
               break ;
            }

            rc = pDmsCB->nameToSUAndLock( cs._name, suID, &su, SHARED ) ;

            if ( SDB_OK != rc )
            {
               if ( DMS_INVALID_SUID != suID )
               {
                  pDmsCB->suUnlock( suID, SHARED ) ;
                  suID = DMS_INVALID_SUID ;
                  su = NULL ;
               }
               PD_LOG( PDDEBUG,
                       "clsStorageCheckJob: "
                       "space [%s] might be dropped by another command, rc: %d",
                       cs._name, rc ) ;
               continue ;
            }

            rc = pShdMgr->rGetCSInfo( cs._name, pageSize, lobPageSize, type ) ;

            pDmsCB->suUnlock( suID, SHARED ) ;
            suID = DMS_INVALID_SUID ;
            su = NULL ;

            if ( SDB_DMS_CS_NOTEXIST != rc )
            {
               continue ;
            }

            PD_LOG( PDWARNING,
                    "clsStorageCheckJob: "
                    "space [%s] doesn't exist in catalog but exists in "
                    "storage, remove it from storage",
                    cs._name ) ;

            do
            {
               rc = pRtnCB->contextNew( RTN_CONTEXT_DELCS,
                                        (rtnContext **)&pDelContext,
                                        contextID, cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING,
                          "clsStorageCheckJob: "
                          "failed to create DelCS context [%s], rc: %d",
                          cs._name, rc ) ;
                  break ;
               }

               rc = pDelContext->open( cs._name, cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING,
                          "clsStorageCheckJob: "
                          "failed to open DelCS context [%s], rc: %d",
                          cs._name, rc ) ;
                  break ;
               }

               rc = pShdMgr->rGetCSInfo( cs._name, pageSize,
                                         lobPageSize, type ) ;
               if ( SDB_DMS_CS_NOTEXIST != rc )
               {
                  PD_LOG( PDWARNING,
                          "clsStorageCheckJob: "
                          "space [%s] exists in catalog after phase 1, kill "
                          "the context to restore the files",
                          cs._name ) ;
                  break ;
               }

               rc = pDelContext->getMore( -1, buffObj, cb ) ;
               if ( SDB_DMS_EOC == rc )
               {
                  PD_LOG( PDDEBUG,
                          "clsStorageCheckJob: "
                          "removed space [%s]",
                          cs._name ) ;
               }
               else if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING,
                          "clsStorageCheckJob: "
                          "failed to execute DelCS context [%s], rc: %d",
                          cs._name, rc ) ;
               }
            } while ( FALSE ) ;

            if ( -1 != contextID )
            {
               pRtnCB->contextDelete( contextID, cb ) ;
               contextID = -1 ;
               pDelContext = NULL ;
            }
         } // End for

         PD_LOG( PDDEBUG, "clsStorageCheckJob: end job" ) ;

      } // End while
      return SDB_OK ;
   }

   INT32 startStorageCheckJob ( EDUID *pEDUID )
   {
      INT32 rc = SDB_OK ;
      clsStorageCheckJob *pJob = NULL ;

      pJob = SDB_OSS_NEW clsStorageCheckJob() ;
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

}
