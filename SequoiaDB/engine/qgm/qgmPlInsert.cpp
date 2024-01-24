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
#include "clsResourceContainer.hpp"
#include "coordInsertOperator.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "utilStr.hpp"
#include "authDef.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{

   #define QGM_INSERT_MSG_BUFF_INIT_SIZE           ( 1024 * 1024 * 2 )
   #define QGM_INSERT_MSG_BUFF_MAX_SIZE            ( 1024 * 1024 * 8 )
   #define QGM_INSERT_BATCH_NUM                    ( 5000 )

   _qgmPlInsert::_qgmPlInsert( const qgmDbAttr &collection,
                               const BSONObj &record )
   :_qgmPlan( QGM_PLAN_TYPE_INSERT, _qgmField() ),
    _insertor( record ),
    _got( FALSE ),
    _clientVersion( CATALOG_INVALID_VERSION ),
    _catalogVersion( CATALOG_INVALID_VERSION )
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
      ss << "Name:" << _fullName.c_str() << '\n' ;
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

   void _qgmPlInsert::buildRetInfo( BSONObjBuilder &builder ) const
   {
      _inResult.toBSON( builder ) ;
   }


   void _qgmPlInsert::setClientVersion( INT32 version )
   {
      _clientVersion = version ;
   }

   INT32 _qgmPlInsert::getCatalogVersion() const
   {
      return _catalogVersion ;
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
      UINT32 batchNum = 0 ;
      BOOLEAN fetchEOC = FALSE ;
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

      if ( !_directInsert() )
      {
         /// ignore error
         msgCheckBuffer( &pMsg, &bufSize, QGM_INSERT_MSG_BUFF_INIT_SIZE,
                         eduCB ) ;
      }

      if ( SDB_ROLE_COORD == _role )
      {
         rc = opr.init( sdbGetResourceContainer()->getResource(), eduCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
            goto error ;
         }
      }

      while ( TRUE )
      {
         fetchEOC = FALSE ;
         rc = _nextRecord( eduCB, obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            fetchEOC = TRUE ;
         }
         else if ( SDB_OK != rc )
         {
            goto error ;
         }
         /// When insert virtual cs
         else if ( 0 == ossStrncmp( _fullName.c_str(),
                                    CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                                    SYS_VIRTUAL_CS_LEN + 1 ) )
         {
            rc = _insertVCS( _fullName.c_str(), obj, eduCB ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else if ( !_directInsert() || SDB_ROLE_COORD == _role )
         {
            MsgOpInsert *pInsertMsg = NULL ;
            if ( 0 == batchNum )
            {
               rc = msgBuildInsertMsg( &pMsg, &bufSize, _fullName.c_str(),
                                       0, 0, &obj, eduCB ) ;
            }
            else
            {
               rc = msgAppendInsertMsg( &pMsg, &bufSize, &obj, eduCB ) ;
            }
            if ( rc )
            {
               PD_LOG( PDERROR, "Build insert message failed, rc: %d", rc ) ;
               goto error ;
            }
            ++batchNum ;
            pInsertMsg = ( MsgOpInsert* )pMsg ;

            if ( batchNum < QGM_INSERT_BATCH_NUM &&
                 pInsertMsg->header.messageLength < QGM_INSERT_MSG_BUFF_MAX_SIZE )
            {
               continue ;
            }
         }
         else
         {
            _inResult.resetInfo() ;
            rc = rtnInsert ( _fullName.c_str(), obj, 1, 0, eduCB,
                             dmsCB, dpsCB, 1, NULL, &_inResult ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Insert record on node failed, rc: %d",
                       rc ) ;
               goto error ;
            }
         }

         if ( batchNum > 0 )
         {
            if ( SDB_ROLE_COORD == _role )
            {
               (( MsgOpInsert* )pMsg)->version = _clientVersion ;

               rc = opr.execute ( (MsgHeader*)pMsg, eduCB,
                                  contextID, &buff ) ;

               if( SDB_CLIENT_CATA_VER_OLD == rc )
               {
                  _catalogVersion = buff.getStartFrom() ;
               }

               /// update info
               _inResult.incInsertedNum( opr.getInsertedNum() ) ;
               _inResult.incDuplicatedNum( opr.getDuplicatedNum() ) ;
               if ( buff.recordNum() == 1 )
               {
                  BSONObj tmpResult ;
                  buff.nextObj( tmpResult ) ;
                  _inResult.setResultObj( tmpResult ) ;
               }
               opr.clearStat() ;

               if ( rc )
               {
                  PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                          opr.getName(), rc ) ;
                  goto error ;
               }
            }
            else
            {
               const CHAR *pInsertor = NULL ;
               INT32 count = 0 ;
               INT32 flag = 0 ;
               const CHAR *pCollectionName = NULL ;

               rc = msgExtractInsert( pMsg, &flag, &pCollectionName,
                                      &pInsertor, count ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Extract insert message failed, rc: %d",
                          rc ) ;
                  goto error ;
               }

               try
               {
                  BSONObj firstObj( pInsertor ) ;
                  _inResult.resetInfo() ;
                  rc = rtnInsert ( _fullName.c_str(), firstObj, count, flag,
                                   eduCB, dmsCB, dpsCB, 1, NULL, &_inResult ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Insert record on node failed, rc: %d",
                             rc ) ;
                     goto error ;
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Insert record occur exception: %s",
                          e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            }

            batchNum = 0 ;
         }

         if ( fetchEOC )
         {
            break ;
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

   INT32 _qgmPlInsert::_insertVCS( const CHAR *fullName,
                                   const BSONObj &insertor,
                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( fullName, CMD_ADMIN_PREFIX SYS_CL_SESSION_INFO ) )
      {
         schedTaskMgr *pSvcTaskMgr = pmdGetKRCB()->getSvcTaskMgr() ;
         schedItem *pItem = ( schedItem* )cb->getSession()->getSchedItemPtr() ;

         pItem->_info.fromBSON( insertor, TRUE ) ;

         /// update task info
         pItem->_ptr = pSvcTaskMgr->getTaskInfoPtr( pItem->_info.getTaskID(),
                                                    pItem->_info.getTaskName() ) ;
         /// update monApp's info
         cb->getMonAppCB()->setSvcTaskInfo( pItem->_ptr.get() ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
