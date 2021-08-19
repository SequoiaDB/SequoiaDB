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

   Source File Name = pmdLoadWorker.cpp

   Descriptive Name =

   When/how to use: json convert bson,import to database file

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2013  JW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdEDUMgr.hpp"
#include "migLoad.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOADWORKER, "pmdLoadWorkerEntryPoint" )
   INT32 pmdLoadWorkerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOADWORKER );
      initWorker *dataWorker = NULL ;
      dmsStorageUnit *su     = NULL ;

      dataWorker = (initWorker *)pData ;
      migWorker worker( dataWorker->pMaster ) ;
      su = dataWorker->pSu ;
      rc = worker.importData( dataWorker->masterEDUID,
                              su,
                              dataWorker->collectionID,
                              dataWorker->clLID,
                              dataWorker->isAsynchr,
                              cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to import data, rc=%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_PMDLOADWORKER, rc );
      return rc ;
   error:
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_LOADWORKER, FALSE,
                          pmdLoadWorkerEntryPoint,
                          "MigLoadWork" ) ;

}
