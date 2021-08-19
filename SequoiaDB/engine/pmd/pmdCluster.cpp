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

   Source File Name = pmdCluster.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDUMgr.hpp"
#include "clsMgr.hpp"
#include "pmd.hpp"
#include "pmdEnv.hpp"
#include "clsLocalValidation.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   INT32 pmdClsNtyEntryPoint( pmdEDUCB * cb, void * arg )
   {
      INT32 rc = SDB_OK ;
      clsLSNNtyInfo lsnInfo ;
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      replCB *pReplCb = ( replCB* )arg ;
      ossQueue< clsLSNNtyInfo > *pNtyQue = pReplCb->getNtyQue() ;

      rc = eduMgr->activateEDU ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to activate EDU" ) ;
         goto error ;
      }

      // just sit here do nothing at the moment
      while ( !cb->isDisconnected() )
      {
         if ( !pNtyQue->timed_wait_and_pop( lsnInfo, OSS_ONE_SEC ) )
         {
            continue ;
         }
         cb->incEventCount() ;
         pReplCb->notify2Session( lsnInfo._csLID, lsnInfo._clLID,
                                  lsnInfo._extLID, lsnInfo._offset ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CLSLOGNTY, TRUE,
                          pmdClsNtyEntryPoint,
                          "ClusterLogNotify" ) ;

   INT32 pmdDBMonitorEntryPoint( pmdEDUCB *cb, void *arg )
   {
      INT32 rc = SDB_OK ;
      pmdEDUMgr *pEduMgr = cb->getEDUMgr() ;
      pmdEDUEvent data ;
      _clsLocalValidation v ;

      rc = pEduMgr->activateEDU( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to activate edu[%s], rc: %d",
                   cb->toString().c_str(), rc ) ;

      while( !cb->isDisconnected() )
      {
         if ( cb->waitEvent( data, OSS_ONE_SEC, TRUE ) )
         {
            pmdEduEventRelease( data, cb ) ;
         }
         /// set the edu to active
         pEduMgr->activateEDU( cb ) ;
         /// monitor
         rc = v.run() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to run local validation: %d", rc ) ;
         }

         if ( pmdDBIsAbnormal() )
         {
            PD_LOG( PDSEVERE, "DB is under abnormal status, we must "
                    "restart!" ) ;
            PMD_RESTART_DB( SDB_SYS ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_DBMONITOR, TRUE,
                          pmdDBMonitorEntryPoint,
                          "DBMonitor" ) ;

}

