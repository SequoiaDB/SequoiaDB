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

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SYNCCLOCK, TRUE,
                          pmdSyncClockEntryPoint,
                          "SyncClockWorker" ) ;

#if defined (_LINUX)
   INT32 pmdSignalTestEntryPoint( pmdEDUCB *cb, void *arg )
   {
      pmdEDUCB *mainCB = ( pmdEDUCB* )arg ;
      UINT32 interval = pmdGetOptionCB()->getSignalInterval() ;
      UINT32 timeCounter = 0 ;

      while( !cb->isDisconnected() )
      {
         ossSleep( OSS_ONE_SEC ) ;
         ++timeCounter ;
         interval = pmdGetOptionCB()->getSignalInterval() ;

         if ( interval > 0 && timeCounter > interval )
         {
            ossPThreadKill( mainCB->getThreadID(), OSS_TEST_SIGNAL ) ;
            timeCounter = 0 ;
         }
      }

      return SDB_OK ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SIGNALTEST, TRUE,
                          pmdSignalTestEntryPoint,
                          "SignalTest" ) ;

#endif //_LINUX

}


