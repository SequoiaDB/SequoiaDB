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

   Source File Name = qgmPlanContainer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlanContainer.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{
   _qgmPlanContainer::_qgmPlanContainer()
   :_plan( NULL )
   {

   }

   _qgmPlanContainer::~_qgmPlanContainer()
   {
      if ( NULL != _plan )
      {
         _plan->close() ;
         SDB_OSS_DEL _plan ;
         _plan = NULL ;
      }
   }

   INT32 _qgmPlanContainer::execute( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == _plan )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "can not execute a empty plan" ) ;
         goto error ;
      }
#if defined (_DEBUG)
      {
      INT32 bufferSize = 1024*1024*10 ;
      CHAR *pBuffer = (CHAR*)SDB_OSS_MALLOC(bufferSize) ;
      if ( pBuffer )
      {
         rc = qgmDump ( this, pBuffer, bufferSize ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG ( PDEVENT, "Plan:"OSS_NEWLINE"%s", pBuffer ) ;
         }
         SDB_OSS_FREE ( pBuffer ) ;
      }
      }
#endif

      if ( _plan->canUseTrans() )
      {
         BOOLEAN checkAutoCommit = FALSE ;
         BOOLEAN dpsValid = TRUE ;

         /// check trans
         if ( cb->isAutoCommitTrans() )
         {
            rc = SDB_RTN_ALREADY_IN_AUTO_TRANS ;
            PD_LOG( PDWARNING, "Already in autocommit transaction, rc: %d",
                    rc ) ;
            goto error ;
         }

         if ( SDB_ROLE_COORD == pmdGetKRCB()->getDBRole() )
         {
            if ( _plan->needRollback() && _plan->inputSize() > 0 )
            {
               checkAutoCommit = TRUE ;
            }
            /// else, push down auto commit to data node
         }
         else
         {
            if ( _plan->needRollback() )
            {
               SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
               if ( dpsCB && cb->isFromLocal() && !dpsCB->isLogLocal() )
               {
                   dpsValid = FALSE ;
               }
            }
            checkAutoCommit = TRUE ;
         }

         if ( checkAutoCommit )
         {
            rc = _plan->checkTransAutoCommit( dpsValid, cb ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }

      rc = _plan->execute( cb ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmPlanContainer::needRollback() const
   {
      return _plan ? _plan->needRollback() : FALSE ;
   }

   void _qgmPlanContainer::buildRetInfo( BSONObjBuilder &builder ) const
   {
      if ( _plan )
      {
         _plan->buildRetInfo( builder ) ;
      }
   }

   INT32 _qgmPlanContainer::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      qgmFetchOut next ;

      if ( NULL == _plan )
      {
         PD_LOG( PDERROR, "can not fetch a empty plan" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _plan->fetchNext( next ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      obj = next.obj ;
   done:
      return rc ;
   error:
      goto done ;
   }
}
