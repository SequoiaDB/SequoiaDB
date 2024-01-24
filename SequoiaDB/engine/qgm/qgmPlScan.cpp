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

   Source File Name = qgmPlScan.cpp

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

#include "qgmPlScan.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "coordCB.hpp"
#include "clsResourceContainer.hpp"
#include "coordQueryOperator.hpp"
#include "ossMem.hpp"
#include "msgMessage.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "authDef.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _qgmPlScan::_qgmPlScan( const qgmDbAttr &collection,
                           const qgmOPFieldVec &selector,
                           const BSONObj &orderby,
                           const BSONObj &hint,
                           INT64 numSkip,
                           INT64 numReturn,
                           const qgmField &alias,
                           _qgmConditionNode *condition )
   :_qgmPlan( QGM_PLAN_TYPE_SCAN, alias ),
   _invalidPredicate( FALSE ),
   _contextID( -1 ),
   _collection( collection ),
   _orderby( orderby ),
   _hint( hint ),
   _skip( numSkip ),
   _return( numReturn ),
   _dmsCB( NULL ),
   _rtnCB( NULL ),
   _conditionNode( NULL ),
   _clientVersion( CATALOG_INVALID_VERSION ),
   _catalogVersion( CATALOG_INVALID_VERSION )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      _dbRole = krcb->getDBRole() ;
      _dmsCB = krcb->getDMSCB() ;
      _rtnCB = krcb->getRTNCB() ;

      if ( NULL != condition )
      {
         _conditionNode = condition ;
      }

      _selector.load( selector ) ;
      _initialized = TRUE ;
   }

   _qgmPlScan::~_qgmPlScan()
   {
      _killContext() ;
      SAFE_OSS_DELETE( _conditionNode ) ;
   }

   void _qgmPlScan::close()
   {
      _killContext() ;
      return ;
   }

   string _qgmPlScan::toString() const
   {
      stringstream ss ;

      ss << "Type:" << qgmPlanType( _type ) << '\n';
      ss << "Collection:" << _collection.toString() << '\n' ;
      if ( !_alias.empty() )
      {
         ss << "Alias:" << _alias.toString() << '\n';
      }
      if ( !_selector.empty() )
      {
         ss << "Selector:"
            << _selector.toString() << '\n';
      }
      if ( NULL != _conditionNode )
      {
         qgmConditionNodeHelper tree( _conditionNode ) ;
         ss << "Condition:"
            << tree.toJson() << '\n';
      }
      if ( !_orderby.isEmpty() )
      {
         ss << "Sort:"
            << _orderby.toString() << '\n';
      }
      if ( !_hint.isEmpty() )
      {
         ss << "Hint:"
            << _hint.toString() << '\n';
      }
      if ( 0 != _skip )
      {
         ss << "Skip:"
            << _skip << '\n';
      }
      if ( -1 != _return )
      {
         ss << "Limit:"
            << _return << '\n';
      }
      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSCAN__EXEC, "_qgmPlScan::_execute" )
   INT32 _qgmPlScan::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSCAN__EXEC ) ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( _input.size() == 0, "impossible" ) ;

      _invalidPredicate = FALSE ;
      _contextID = -1 ;

      _qgmConditionNodeHelper tree( _conditionNode ) ;
      _condition = tree.toBson( FALSE ) ;

      if ( SDB_ROLE_COORD == _dbRole )
      {
         rc = _executeOnCoord( eduCB ) ;
      }
      /// not coord or rc is SDB_COORD_UNKNOWN_OP_REQ
      if ( SDB_COORD_UNKNOWN_OP_REQ == rc ||
           SDB_ROLE_COORD != _dbRole )
      {
         rc = _executeOnData( eduCB ) ;
      }

      if ( SDB_RTN_INVALID_PREDICATES == rc )
      {
         rc = SDB_OK ;
         _invalidPredicate = TRUE ;
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSCAN__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _qgmPlScan::canUseTrans() const
   {
      ossPoolString strName = _collection.toString() ;
      const CHAR *pCLName = strName.c_str() ;

      if ( rtnIsCommand( pCLName ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSCAN__EXECONDATA, "_qgmPlScan::_executeOnData" )
   INT32 _qgmPlScan::_executeOnData( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSCAN__EXECONDATA ) ;
      INT32 rc = SDB_OK ;
      _rtnCommand *pCommand = NULL ;

      ossPoolString strName = _collection.toString() ;
      const CHAR *pCLName = strName.c_str() ;
      BSONObj selector = _selector.selector() ;

      if ( rtnIsCommand( pCLName ) )
      {
         INT32 serviceType = CMD_SPACE_SERVICE_SHARD ;
         if ( eduCB->isFromLocal() )
         {
            serviceType = CMD_SPACE_SERVICE_LOCAL ;
         }
         rc = rtnParserCommand( pCLName, &pCommand ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Parse command[%s] failed[rc:%d]",
                     pCLName, rc ) ;
            goto error ;
         }
         rc = rtnInitCommand( pCommand , 0, _skip, _return,
                              _condition.objdata(), selector.objdata(),
                              _orderby.objdata(), _hint.objdata() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         PD_LOG ( PDDEBUG, "Command: %s", pCommand->name () ) ;

         //run command
         rc = rtnRunCommand( pCommand, serviceType,
                             eduCB, _dmsCB, _rtnCB,
                             NULL, 1, &_contextID ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         // close prefetch
         rc = rtnQuery ( pCLName, selector, _condition,
                         _orderby, _hint, 0, eduCB, _skip, _return,
                         _dmsCB, _rtnCB, _contextID, NULL, FALSE ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      if ( pCommand )
      {
         rtnReleaseCommand( &pCommand ) ;
      }
      PD_TRACE_EXITRC( SDB__QGMPLSCAN__EXECONDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _qgmPlScan::setClientVersion( INT32 version )
   {
      _clientVersion = version ;
   }

   INT32 _qgmPlScan::getCatalogVersion() const
   {
      return _catalogVersion ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSCAN__EXECONCOORD, "_qgmPlScan::_executeOnCoord" )
   INT32 _qgmPlScan::_executeOnCoord( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSCAN__EXECONCOORD ) ;
      INT32 rc = SDB_OK ;
      INT32 bufSize = 0 ;
      CHAR *qMsg = NULL ;
      BSONObj selector = _selector.selector() ;

      coordQueryOperator opr ;
      rtnContextBuf buff ;

      rc = opr.init( sdbGetResourceContainer()->getResource(), eduCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                 opr.getName(), rc ) ;
         goto error ;
      }

      /// build message
      rc = msgBuildQueryMsg ( &qMsg, &bufSize,
                              _collection.toString().c_str(),
                              ( FLG_QUERY_WITH_RETURNDATA |
                                FLG_QUERY_PREPARE_MORE |
                                FLG_QUERY_CLOSE_EOF_CTX ), 0,
                              _skip, _return,
                              &_condition, &selector,
                              &_orderby, &_hint,
                              eduCB ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build message failed, rc: %d", rc ) ;
         goto error ;
      }

      ((MsgOpQuery*)qMsg)->version = _clientVersion ;

      rc = opr.execute( (MsgHeader*)qMsg, eduCB, _contextID, &buff ) ;
      if ( rc )
      {
         if( SDB_CLIENT_CATA_VER_OLD == rc )
         {
            _catalogVersion = buff.getStartFrom() ;
         }

         if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    opr.getName(), rc ) ;
         }
         goto error ;
      }

   done:
      if ( NULL != qMsg )
      {
         msgReleaseBuffer( qMsg, eduCB ) ;
      }
      PD_TRACE_EXITRC( SDB__QGMPLSCAN__EXECONCOORD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSCAN__FETCHNEXT, "_qgmPlScan::_fetchNext" )
   INT32 _qgmPlScan::_fetchNext ( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSCAN__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      const CHAR *getMoreRes = NULL ;

      if ( _invalidPredicate )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetch( getMoreRes ) ;

      if ( SDB_OK != rc && SDB_DMS_EOC != rc )
      {
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         try
         {
            if ( _selector.needSelect() )
            {
               rc = _selector.select( BSONObj(getMoreRes), next.obj ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
            else
            {
               next.obj = BSONObj( getMoreRes ) ;
            }
         }
         catch ( std::exception &e )
         {
            PD_RC_CHECK ( SDB_SYS, PDERROR,
                          "unexpected err happened when fetching: %s",
                          e.what() ) ;
         }

         if ( !_merge )
         {
            next.alias = _alias ;
         }
      }
      else
      {
         /// do nothing.
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSCAN__FETCHNEXT, rc ) ;
      return rc ;
   error:
      if ( SDB_DMS_EOC == rc )
         _contextID = -1 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSCAN__FETCH, "_qgmPlScan::_fetch" )
   INT32 _qgmPlScan::_fetch( const CHAR *&result )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSCAN__FETCH );
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( _contextID != -1,
                   "context id must be initialized" ) ;

      rtnContextBuf buffObj ;
      rc = rtnGetMore( _contextID, 1, buffObj, _eduCB, _rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to getmore from context[%lld], rc = %d",
                    _contextID, rc ) ;
         }
         goto error ;
      }
      result = buffObj.data() ;

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSCAN__FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _qgmPlScan::_killContext()
   {
      if ( -1 != _contextID && NULL != _eduCB )
      {
         rtnKillContexts( 1, &_contextID, _eduCB, _rtnCB ) ;
         _contextID = -1 ;
      }
   }
}

