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

   Source File Name = pmdLogArchiveMgr.cpp

   Descriptive Name = Replica Log Archive Mgr Entry

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains entry point for log global
   writer thread.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/7/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "pmdTrace.h"
#include "pdTrace.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOGARCHIVINGENTPNT, "pmdLogArchiveMgrEntryPoint" )
   INT32 pmdLogArchiveMgrEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOGARCHIVINGENTPNT );

      pmdEDUMgr* eduMgr = cb->getEDUMgr() ;
      SDB_DPSCB* dpsCB = ( SDB_DPSCB* )pData ;

      rc = eduMgr->activateEDU ( cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to activate EDU" ) ;
         goto error ;
      }

      PD_LOG( PDEVENT, "Log archive manager start" );

      // just sit here do nothing at the moment
      while ( !cb->isDisconnected() )
      {
         rc = dpsCB->archive() ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to archive, rc = %d", rc ) ;
         }
      }

      PD_LOG( PDEVENT, "Log archive manager is stopped" );

   done :
      PD_TRACE_EXITRC ( SDB_PMDLOGARCHIVINGENTPNT, rc );
      return rc;
   error :
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_LOGARCHIVEMGR, TRUE,
                          pmdLogArchiveMgrEntryPoint,
                          "LogArchiveMgr" ) ;

}
