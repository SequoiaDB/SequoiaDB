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

   Source File Name = pmdEDUEntryPoint.cpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmd.hpp"

#if defined (_LINUX)
#include "ossSignal.hpp"
#endif // _LINUX

namespace engine
{

   /*
      ENTRY POINTER FUNCTIONS
   */
   INT32 pmdSyncClockEntryPoint( pmdEDUCB * cb, void * arg )
   {
      ossTick tmp ;
      pmdKRCB *pKrcb = pmdGetKRCB() ;

      pKrcb->getEDUMgr()->activateEDU( cb ) ;

      while ( !cb->isDisconnected() )
      {
         pKrcb->syncCurTime() ;
         pmdUpdateDBTick() ;
         ossSleep( PMD_SYNC_CLOCK_INTERVAL ) ;
      }
      return SDB_OK ;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SYNCCLOCK, TRUE,
                          pmdSyncClockEntryPoint,
                          "SyncClockWorker" ) ;

#if defined (_LINUX)
   INT32 pmdSignalTestEntryPoint( pmdEDUCB *cb, void *arg )
   {
      pmdEDUCB *mainCB = ( pmdEDUCB* )arg ;
      INT32 interval = pmdGetOptionCB()->getSignalInterval() ;
      UINT32 timeCounter = 0 ;

      while( !cb->isDisconnected() )
      {
         ossSleep( OSS_ONE_SEC ) ;
         ++timeCounter ;
         interval = pmdGetOptionCB()->getSignalInterval() ;

         if ( interval > 0 && timeCounter > (UINT32)interval )
         {
            ossPThreadKill( mainCB->getThreadID(), OSS_TEST_SIGNAL ) ;
            timeCounter = 0 ;
         }
      }

      return SDB_OK ;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SIGNALTEST, TRUE,
                          pmdSignalTestEntryPoint,
                          "SignalTest" ) ;

#endif //_LINUX

}


