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

   Source File Name = pmdPreLoader.cpp

   Descriptive Name = Process MoDel Prefetcher

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for prefetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/01/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "bpsPrefetch.hpp"
#include "bps.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "ixmExtent.hpp"
#include "ixm.hpp"
#include "dmsStorageUnit.hpp"

namespace engine
{

   #define PMD_QUEUE_WAIT_TIME 100
   #define PMD_PRELOAD_UNIT    4096

   void  doPreLoad( const CHAR * pointer )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDPRELOADERENENTPNT, "pmdPreLoaderEntryPoint" )
   INT32 pmdPreLoaderEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDPRELOADERENENTPNT );
      pmdKRCB *krcb       = pmdGetKRCB() ;
      SDB_BPSCB   *bpscb  = krcb->getBPSCB() ;
      SDB_DMSCB   *dmscb  = krcb->getDMSCB() ;

      ossQueue<bpsPreLoadReq*> *prefReqQ  = bpscb->getReqQueue () ;
      ossQueue<bpsPreLoadReq*> *dropReqQ  = bpscb->getDropQueue () ;
      bpsPreLoadReq *prefReq = NULL ;

      while ( !PMD_IS_DB_DOWN() )
      {
         if ( prefReqQ->timed_wait_and_pop ( prefReq, PMD_QUEUE_WAIT_TIME ) )
         {
            dmsExtRW extRW ;
            dmsStorageUnitID csid = prefReq->_csid ;
            dmsStorageUnit *su = dmscb->suLock ( csid ) ;
            if ( su && su->LogicalCSID() == prefReq->_csLID )
            {
               const dmsExtent *pExtent = NULL ;
               const CHAR *pData = NULL ;
               UINT32 pageSizeSqureRoot = 0 ;

               if ( BPS_DMS_DATA == prefReq->_type )
               {
                  extRW = su->data()->extent2RW( prefReq->_extid, -1 ) ;
                  pageSizeSqureRoot = su->data()->pageSizeSquareRoot () ;
               }
               else if ( BPS_DMS_INDEX == prefReq->_type )
               {
                  extRW = su->index()->extent2RW( prefReq->_extid, -1 ) ;
                  pageSizeSqureRoot = su->index()->pageSizeSquareRoot () ;
               }

               extRW.setNothrow( TRUE ) ;
               pExtent = extRW.readPtr<dmsExtent>() ;
               
               if ( pExtent )
               {
                  pData = ( const CHAR* )pExtent ;
                  UINT32 totalSize = 0 ;

                  if ( pData[0] == DMS_EXTENT_EYECATCHER0 &&
                       pData[1] == DMS_EXTENT_EYECATCHER1 )
                  {
                     totalSize = (UINT32)(pExtent->_blockSize <<
                                          pageSizeSqureRoot ) ;
                  }
                  else if ( pData[0] == DMS_META_EXTENT_EYECATCHER0 &&
                            pData[1] == DMS_META_EXTENT_EYECATCHER1 )
                  {
                     totalSize = (UINT32)(pExtent->_blockSize <<
                                          pageSizeSqureRoot ) ;
                  }
                  else
                  {
                     totalSize = (UINT32)( 1 << pageSizeSqureRoot ) ;
                  }

                  totalSize = OSS_MIN ( totalSize, DMS_MAX_EXTENT_SZ ) ;

                  pData = extRW.readPtr( 0, totalSize ) ;
                  if ( pData )
                  {
                     UINT32 index = 0 ;
                     while ( index < totalSize )
                     {
                        doPreLoad( pData + index ) ;
                        index += PMD_PRELOAD_UNIT ;
                     }
                  }
               } // if ( addr )
               dmscb->suUnlock ( csid ) ;
            } // if ( su )
            dropReqQ->push ( prefReq ) ;
         } // if ( prefReqQ->timed_wait_and_pop
      } // while ( !PMD_IS_DB_DOWN() )

      PD_TRACE_EXITRC ( SDB_PMDPRELOADERENENTPNT, rc );
      return rc;
   }

   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_PREFETCHER, FALSE,
                          pmdPreLoaderEntryPoint,
                          "PreLoader" ) ;

}

