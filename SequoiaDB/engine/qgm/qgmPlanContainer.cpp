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
