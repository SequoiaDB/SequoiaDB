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

   Source File Name = pmdLoggW.cpp

   Descriptive Name = Process MoDel Log Global Writer

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains entry point for log global
   writer thread.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include <stdio.h>

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOGGWENTPNT, "pmdLoggWEntryPoint" )
   INT32 pmdLoggWEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOGGWENTPNT );
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      SDB_DPSCB *dpsCb = ( SDB_DPSCB* )pData ;
      rc = eduMgr->activateEDU ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to activate EDU" ) ;
         goto error ;
      }

      // just sit here do nothing at the moment
      while ( !cb->isDisconnected() )
      {
         rc = dpsCb->run( cb );
         if ( rc )
         {
            PD_LOG ( PDSEVERE, "Failed to run dpsCB, rc = %d", rc ) ;
            ossPanic () ;
         }
      }

      rc = dpsCb->tearDown();
      if ( rc )
      {
         PD_LOG ( PDSEVERE, "Failed to run tearDown(), rc = %d", rc ) ;
         ossPanic() ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_PMDLOGGWENTPNT, rc );
      return rc;
   error :
      switch ( rc )
      {
      case SDB_SYS :
         PD_LOG ( PDSEVERE, "System error occured" ) ;
         break ;
      default :
         PD_LOG ( PDSEVERE, "Internal error, rc = %d", rc ) ;
      }
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_LOGGW, TRUE,
                          pmdLoggWEntryPoint,
                          "LogWriter" ) ;

}

