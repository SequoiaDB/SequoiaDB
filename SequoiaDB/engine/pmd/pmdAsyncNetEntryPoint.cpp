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

   Source File Name = pmdAsyncNetEntryPoint.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          30/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDUMgr.hpp"
#include "netRouteAgent.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDNETCALLBACKFUNC, "pmdNetCallbackFunc" )
   INT32 pmdNetCallbackFunc( _netEventSuit *pSuit )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDNETCALLBACKFUNC ) ;

      rc = SDB_SYS ;
      goto error ;

   done:
      PD_TRACE_EXITRC ( SDB_PMDNETCALLBACKFUNC, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDASYNCNETEP, "pmdAsyncNetEntryPoint" )
   INT32 pmdAsyncNetEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_PMDASYNCNETEP ) ;
      pmdEDUMgr *pEDUMgr = cb->getEDUMgr () ;
      _netRouteAgent *pRouteAgent = (_netRouteAgent *)pData;
      BOOLEAN hasReg = FALSE ;

      rc = pEDUMgr->activateEDU( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to active EDU[type:%d,ID:%lld]",
                  cb->getType(), cb->getID() ) ;
         goto error ;
      }

      PD_LOG ( PDEVENT, "Run %s[Type: %d] ...", getEDUName( cb->getType() ),
               cb->getType() ) ;

      pEDUMgr->addIOService( pRouteAgent->ioservice() ) ;
      hasReg = TRUE ;
      try
      {
         pRouteAgent->run( pmdNetCallbackFunc ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Exception during start %s[Type: %d]: %s",
                  getEDUName( cb->getType() ), cb->getType(),
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      PD_LOG ( PDEVENT, "Stop %s[Type: %d]", getEDUName( cb->getType() ),
               cb->getType() ) ;

   done:
      if ( hasReg )
      {
         pEDUMgr->delIOSerivce( pRouteAgent->ioservice() ) ;
      }
      PD_TRACE_EXITRC ( SDB_PMDASYNCNETEP, rc );
      return rc;
   error:
      goto done;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_REPR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "ReplReader" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SHARDR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "ShardReader" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_CATNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "CatalogNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_COORDNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "CoordNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_OMNET, TRUE,
                          pmdAsyncNetEntryPoint,
                          "OMNet" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_RTNNETWORK, TRUE,
                          pmdAsyncNetEntryPoint,
                          "RtnNetwork" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_SERVICE, TRUE,
                          pmdAsyncNetEntryPoint,
                          "SeService" ) ;

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_SE_INDEXR, TRUE,
                          pmdAsyncNetEntryPoint,
                          "SeIndexerReader" ) ;

}

