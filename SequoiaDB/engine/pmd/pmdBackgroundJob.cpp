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

   Source File Name = pmdReplay.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDBGJOBENTPNT, "pmdBackgroundJobEntryPoint" )
   INT32 pmdBackgroundJobEntryPoint( pmdEDUCB *cb, void *pData )
   {
      SDB_ASSERT( NULL != pData, "impossible" ) ;
      PD_TRACE_ENTRY ( SDB_PMDBGJOBENTPNT );
      rtnJobMgr *jobMgr = rtnGetJobMgr () ;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr() ;
      rtnBaseJob *job = (rtnBaseJob*)pData ;
      INT32 rc = SDB_OK ;
      BOOLEAN reuseEDU = job->reuseEDU() ;
      BOOLEAN isSystem = job->isSystem() ;
      string expStr ;

      PD_LOG( PDINFO, "Start a background job[%s]", job->name() ) ;

      cb->setName( job->name() ) ;
      job->attachIn( cb ) ;

      try
      {
         pEDUMgr->activateEDU( cb ) ;
         if ( isSystem )
         {
            pEDUMgr->lockEDU( cb ) ;
         }

         rc = job->doit () ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDWARNING, "Background job[%s] do failed, rc = %d",
                     job->name(), rc ) ;
         }
         else
         {
            PD_LOG ( PDINFO, "Background job[%s] finished", job->name() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         expStr = e.what() ;
      }

      job->attachOut () ;

      if ( isSystem )
      {
         if ( PMD_IS_DB_UP() )
         {
            PD_LOG( PDSEVERE, "System job[EDUID:%lld, Type:%s, Name:%s] "
                    "exit with %d. Restart DB", cb->getID(),
                    getEDUName( cb->getType() ), job->name(), rc ) ;
            PMD_RESTART_DB( rc ) ;
         }
         pEDUMgr->unlockEDU( cb ) ;
      }

      jobMgr->_removeJob ( cb->getID(), rc ) ;
      if ( !reuseEDU )
      {
         pEDUMgr->forceUserEDU( cb->getID() ) ;
      }

      if ( !expStr.empty() )
      {
         throw pdGeneralException( rc, expStr ) ;
      }

      PD_TRACE_EXITRC ( SDB_PMDBGJOBENTPNT, rc );
      return SDB_OK ;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_BACKGROUND_JOB, FALSE,
                          pmdBackgroundJobEntryPoint,
                          "Task" ) ;

}

