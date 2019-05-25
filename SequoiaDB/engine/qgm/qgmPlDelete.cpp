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

   Source File Name = qgmPlDelete.cpp

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

******************************************************************************/

#include "qgmPlDelete.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "msgDef.h"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "coordCB.hpp"
#include "coordDeleteOperator.hpp"
#include "msgMessage.hpp"
#include "rtn.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _qgmPlDelete::_qgmPlDelete( const qgmDbAttr &collection,
                               _qgmConditionNode *condition )
   :_qgmPlan( QGM_PLAN_TYPE_DELETE, _qgmField() ),
    _collection( collection )
   {
      if ( NULL != condition )
      {
         _qgmConditionNodeHelper tree( condition ) ;
         _condition = tree.toBson( TRUE ) ;
         if ( !_condition.isEmpty() )
         {
            _initialized = TRUE ;
         }
      }
      else
      {
         _initialized = TRUE ;
      }
   }

   _qgmPlDelete::~_qgmPlDelete()
   {

   }

   string _qgmPlDelete::toString() const
   {
      stringstream ss ;
      ss << "Type:" << qgmPlanType( _type ) << '\n';
      ss << "Collection:" << _collection.toString() << '\n';
      if ( !_condition.isEmpty() )
      {
         ss << "Condition:" << _condition.toString() << '\n';
      }
      return ss.str() ;
   }

   BOOLEAN _qgmPlDelete::needRollback() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLDELETE__EXEC, "_qgmPlDelete::_execute" )
   INT32 _qgmPlDelete::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLDELETE__EXEC ) ;
      INT32 rc = SDB_OK ;

      _SDB_KRCB *krcb = pmdGetKRCB() ;
      SDB_ROLE role = krcb->getDBRole() ;
      CHAR *msg = NULL ;
      INT32 bufSize = 0 ;

      if ( SDB_ROLE_COORD == role )
      {
         CoordCB *pCoord = krcb->getCoordCB() ;
         INT64 contextID = -1 ;
         rtnContextBuf buff ;

         coordDeleteOperator opr ;
         rc = msgBuildDeleteMsg( &msg, &bufSize,
                                 _collection.toString().c_str(),
                                 0, 0,
                                 _condition.isEmpty() ? NULL : &_condition,
                                 NULL, eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build delete message failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = opr.init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
         rc = opr.execute( (MsgHeader*)msg, eduCB, contextID, &buff ) ;
      }
      else
      {
         SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

         if ( dpsCB && eduCB->isFromLocal() && !dpsCB->isLogLocal() )
         {
            dpsCB = NULL ;
         }
         SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
         BSONObj empty ;
         rc = rtnDelete( _collection.toString().c_str(),
                         _condition, empty, 0, eduCB,
                         dmsCB, dpsCB ) ;
      }

      if ( rc )
      {
         goto error ;
      }

   done:
      if ( NULL != msg )
      {
         msgReleaseBuffer( msg, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__QGMPLDELETE__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
