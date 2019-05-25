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

   Source File Name = qgmPlInsert.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlInsert.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "coordCB.hpp"
#include "coordInsertOperator.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "utilStr.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _qgmPlInsert::_qgmPlInsert( const qgmDbAttr &collection,
                               const BSONObj &record )
   :_qgmPlan( QGM_PLAN_TYPE_INSERT, _qgmField() ),
    _insertor( record ),
    _got( FALSE )
   {
      _fullName = collection.toString() ;
      _role = pmdGetKRCB()->getDBRole() ;
      _initialized = TRUE ;
   }

   _qgmPlInsert::~_qgmPlInsert()
   {
   }

   string _qgmPlInsert::toString() const
   {
      stringstream ss ;
      ss << "Type:" << qgmPlanType( _type ) << '\n' ;
      ss << "Name:" << _fullName << '\n' ;
      if ( !_insertor.isEmpty() )
      {
         ss << "Record:" << _insertor.toString() << '\n' ;
      }
      return ss.str() ;
   }

   BOOLEAN _qgmPlInsert::needRollback() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLINSERT__NEXTRECORD, "_qgmPlInsert::_nextRecord" )
   INT32 _qgmPlInsert::_nextRecord( _pmdEDUCB *eduCB, BSONObj &obj )
   {
      PD_TRACE_ENTRY( SDB__QGMPLINSERT__NEXTRECORD ) ;
      INT32 rc = SDB_OK ;

      if ( _directInsert() )
      {
         if ( !_got )
         {
            obj = _insertor ;
            _got = TRUE ;
         }
         else
         {
            rc = SDB_DMS_EOC ;
         }
      }
      else
      {
         if ( !_got )
         {
            rc = input( 0 )->execute( eduCB ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            _got = TRUE ;
         }

         qgmFetchOut fetch ;
         rc = input( 0 )->fetchNext( fetch ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         obj = fetch.obj ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLINSERT__NEXTRECORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLINSERT__EXEC, "_qgmPlInsert::_execute" )
   INT32 _qgmPlInsert::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLINSERT__EXEC ) ;
      INT32 rc = SDB_OK ;
      pmdKRCB *pKrcb                   = pmdGetKRCB() ;
      CHAR *pMsg    = NULL ;
      INT32 bufSize = 0 ;
      BSONObj obj ;
      INT64 contextID = -1  ;
      SDB_DMSCB *dmsCB = pKrcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = pKrcb->getDPSCB() ;
      coordInsertOperator opr ;
      rtnContextBuf buff ;

      if ( dpsCB && eduCB->isFromLocal() && !dpsCB->isLogLocal() )
      {
         dpsCB = NULL ;
      }

      if ( SDB_ROLE_COORD == _role )
      {
         CoordCB *pCoord = pKrcb->getCoordCB() ;
         rc = opr.init( pCoord->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
      }

      while ( TRUE )
      {
         rc = _nextRecord( eduCB, obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            if ( SDB_ROLE_COORD == _role )
            {
               rc = msgBuildInsertMsg ( &pMsg,
                                        &bufSize,
                                        _fullName.c_str(),
                                        0, 0,
                                        &obj,
                                        eduCB ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Build insert message failed, rc: %d",
                          rc ) ;
                  goto error ;
               }

               rc = opr.execute ( (MsgHeader*)pMsg, eduCB,
                                  contextID, &buff ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                          opr.getName(), rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = rtnInsert ( _fullName.c_str(), obj, 1, 0, eduCB,
                                dmsCB, dpsCB ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Insert record on node failed, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      if ( pMsg )
      {
         msgReleaseBuffer( pMsg, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__QGMPLINSERT__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmPlInsert::_fetchNext ( qgmFetchOut &next )
   {
      SDB_ASSERT( FALSE, "impossble" ) ;
      return SDB_SYS ;
   }
}
