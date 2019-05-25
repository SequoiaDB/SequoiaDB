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

   Source File Name = coordOperator.cpp

   Descriptive Name = Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordOperator.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      Local functions
   */
   static void _coordClearReplyMap( ROUTE_REPLY_MAP *pReply )
   {
      if ( pReply )
      {
         ROUTE_REPLY_MAP::iterator it = pReply->begin() ;
         while ( it != pReply->end() )
         {
            SDB_OSS_FREE( it->second ) ;
            ++it ;
         }
         pReply->clear() ;
      }
   }

   /*
      coordProcessResult implement
   */
   void coordProcessResult::clearError()
   {
      _coordClearReplyMap( _pNokReply ) ;
      _coordClearReplyMap( _pIgnoreReply ) ;

      if ( _pNokRC )
      {
         _pNokRC->clear() ;
      }
      if ( _pIgnoreRC )
      {
         _pIgnoreRC->clear() ;
      }
   }

   void coordProcessResult::clear()
   {
      clearError() ;

      _coordClearReplyMap( _pOkReply ) ;
      if ( _pOkRC )
      {
         _pOkRC->clear() ;
      }
      _sucGroupLst.clear() ;
   }

   /*
      _coordOperator implement
   */
   _coordOperator::_coordOperator()
   {
      _isReadOnly = FALSE ;
      _pResource = NULL ;
   }

   _coordOperator::~_coordOperator()
   {
   }

   INT64 _coordOperator::getTimeout() const
   {
      return _groupSession.getTimeout() ;
   }

   BOOLEAN _coordOperator::isReadOnly() const
   {
      return _isReadOnly ;
   }

   const CHAR* _coordOperator::getName() const
   {
      return _strName.c_str() ;
   }

   void _coordOperator::setName( const string &name )
   {
      _strName = name ;
   }

   void _coordOperator::setReadOnly( BOOLEAN isReadOnly )
   {
      _isReadOnly = isReadOnly ;
   }

   BOOLEAN _coordOperator::needRollback() const
   {
      return FALSE ;
   }

   INT32 _coordOperator::rollback( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      return SDB_OK ;
   }

   INT32 _coordOperator::init( coordResource *pResource,
                               _pmdEDUCB *cb,
                               INT64 timeout )
   {
      INT32 rc = SDB_OK ;

      _pResource = pResource ;
      rc = _groupSession.init( pResource, cb, timeout,
                               &_remoteHandler,
                               &_groupHandler ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOperator::doOnGroups( coordSendMsgIn &inMsg,
                                     coordSendOptions &options,
                                     pmdEDUCB *cb,
                                     coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      coordGroupSel *pGroupSel = _groupSession.getGroupSel() ;
      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pRemoteSession = _groupSession.getSession() ;

      pmdSubSessionItr itr ;
      pmdSubSession *pSub = NULL ;
      MsgOpReply *pReply = NULL ;
      INT32 processType = COORD_PROCESS_OK ;
      MsgRouteID routeID ;
      UINT32 primaryID = 0 ;
      UINT32 groupID = 0 ;
      BOOLEAN needRetry = FALSE ;

      _remoteHandler.enableInterruptWhenFailed( _interruptWhenFailed(),
                                                options._pIgnoreRC ) ;
      pGroupSel->setPrimary( options._primary ) ;
      pGroupSel->setServiceType( options._svcType ) ;

   retry:
      needRetry = FALSE ;
      _groupSession.resetSubSession() ;

      if ( !inMsg.hasData() )
      {
         rcTmp = _groupSession.sendMsg( inMsg.msg(), options._groupLst,
                                        NULL ) ;
      }
      else
      {
         rcTmp = _groupSession.sendMsg( inMsg.msg(), options._groupLst,
                                        *( inMsg.data() ) ) ;
      }
      if ( rcTmp )
      {
         PD_LOG( PDERROR, "Failed to send request[%s] to groups, rc: %d",
                 msg2String( inMsg.msg() ).c_str(), rcTmp ) ;
         rc = rc ? rc : rcTmp ;

         if ( _interruptWhenFailed() )
         {
            pRemoteSession->stopSubSession() ;
         }
      }

      rcTmp = pRemoteSession->waitReply1( TRUE ) ;
      if ( rcTmp )
      {
         PD_LOG( PDERROR, "Failed to recieve replys from groups, rc: %d",
                 rcTmp ) ;
         rc = rc ? rc : rcTmp ;
      }

      result.clearError() ;
      itr = pRemoteSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while( itr.more() )
      {
         pSub = itr.next() ;
         pReply = ( MsgOpReply* )pSub->getRspMsg( TRUE ) ;

         routeID.value = pReply->header.routeID.value ;
         primaryID = pReply->startFrom ;
         groupID = routeID.columns.groupID ;
         rcTmp = pReply->flags ;

         if ( rcTmp && !options.isIgnored( rcTmp ) )
         {
            if ( _isTrans( cb, (MsgHeader*)pReply ) )
            {
               processType = COORD_PROCESS_NOK ;
               if ( SDB_CLS_COORD_NODE_CAT_VER_OLD != rcTmp ||
                    !result.pushNokRC( routeID.value, pReply ) )
               {
                  PD_LOG( PDERROR, "Do trans command[%d] on data node[%s] "
                          "failed, rc: %d", inMsg.opCode(),
                          routeID2String( routeID ).c_str(), rcTmp ) ;
                  rc = rc ? rc : rcTmp ;
               }
            }
            else
            {
               if ( pCtrl->canRetry( rcTmp, routeID, primaryID,
                                     isReadOnly(), TRUE ) )
               {
                  processType = COORD_PROCESS_IGNORE ;
                  result.pushIgnoreRC( routeID.value, rcTmp ) ;
                  rcTmp = SDB_OK ;
                  needRetry = TRUE ;
               }
               else
               {
                  processType = COORD_PROCESS_NOK ;
                  if ( !result.pushNokRC( routeID.value, pReply ) )
                  {
                     rc = rc ? rc : rcTmp ;
                  }
                  PD_LOG( ( rc ? PDERROR : PDINFO ),
                          "Failed to execute command[%u] on "
                          "node[%s], rc: %d", inMsg.opCode(),
                          routeID2String( routeID ).c_str(), rcTmp ) ;
               }
            }
         }
         else
         {
            processType = COORD_PROCESS_OK ;
            result._sucGroupLst[ groupID ] = groupID ;
            result.pushOkRC( routeID.value, rcTmp ) ;
            options._groupLst.erase( groupID ) ;
         }

         _onNodeReply( processType, pReply, cb, inMsg ) ;

         if ( !result.pushReply( (MsgHeader *)pReply, processType ) )
         {
            SDB_OSS_FREE( pReply ) ;
         }
      } /// while( itr.more() )

      if ( rc )
      {
         goto error ;
      }
      else if ( needRetry && 0 == result.nokSize() )
      {
         pCtrl->incRetry() ;
         goto retry ;
      }

   done:
      _groupSession.resetSubSession() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOperator::doOpOnCL( coordCataSel &cataSel,
                                   const BSONObj &objMatch,
                                   coordSendMsgIn &inMsg,
                                   coordSendOptions &options,
                                   pmdEDUCB *cb,
                                   coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      if ( FALSE == options._useSpecialGrp )
      {
         options._groupLst.clear() ;

         rc = cataSel.getGroupLst( cb, result._sucGroupLst,
                                   options._groupLst, &objMatch ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get the groups by catalog info failed, "
                    "matcher: %s, rc: %d", objMatch.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDDEBUG, "Using specified group" ) ;
      }

      if ( cataSel.getCataPtr()->isMainCL() )
      {
         rc = _prepareMainCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      else
      {
         rc = _prepareCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Prepare collection operation failed, "
                   "rc: %d", rc ) ;

      rc = doOnGroups( inMsg, options, cb, result ) ;

      if ( cataSel.getCataPtr()->isMainCL() )
      {
         _doneMainCLOp( cataSel, inMsg, options, cb, result ) ;
      }
      else
      {
         _doneCLOp( cataSel, inMsg, options, cb, result ) ;
      }

      PD_RC_CHECK( rc, PDERROR, "Do command[%d] on groups failed, rc: %d",
                   inMsg.opCode(), rc ) ;

      if ( result.nokSize() > 0 )
      {
         goto done ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordOperator::checkRetryForCLOpr( INT32 rc,
                                               ROUTE_RC_MAP *pRC,
                                               coordCataSel &cataSel,
                                               MsgHeader *pSrcMsg,
                                               pmdEDUCB *cb,
                                               INT32 &errRC,
                                               MsgRouteID *pNodeID,
                                               BOOLEAN canUpdate )
   {
      BOOLEAN retry = FALSE ;
      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;

      errRC = SDB_OK ;
      if ( pNodeID )
      {
         pNodeID->value = 0 ;
      }

      if ( SDB_OK == rc && ( !pRC || pRC->empty() ) )
      {
         retry = FALSE ;
         goto done ;
      }

      if ( pCtrl->canRetry( rc, cataSel, canUpdate ) )
      {
         retry = TRUE ;
      }
      else if ( rc )
      {
         errRC = rc ;
         retry = FALSE ;
      }
      else if ( pRC )
      {
         retry = TRUE ;
         ROUTE_RC_MAP::iterator it = pRC->begin() ;
         while( it != pRC->end() )
         {
            retry = pCtrl->canRetry( it->second._rc, cataSel, canUpdate ) ;
            if ( !retry )
            {
               errRC = it->second._rc ;
               if ( pNodeID )
               {
                  pNodeID->value = it->first ;
               }
               break ;
            }
            ++it ;
         }
      }

      if ( retry && _isTrans( cb, pSrcMsg ) &&
           SDB_OK != cb->getTransRC() )
      {
         retry = FALSE ;
         errRC = cb->getTransRC() ;
      }

   done:
      return retry ;
   }

   INT32 _coordOperator::_prepareCLOp( coordCataSel &cataSel,
                                       coordSendMsgIn &inMsg,
                                       coordSendOptions &options,
                                       pmdEDUCB *cb,
                                       coordProcessResult &result )
   {
      return SDB_OK ;
   }

   void _coordOperator::_doneCLOp( coordCataSel &cataSel,
                                   coordSendMsgIn &inMsg,
                                   coordSendOptions &options,
                                   pmdEDUCB *cb,
                                   coordProcessResult &result )
   {
   }

   INT32 _coordOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                           coordSendMsgIn &inMsg,
                                           coordSendOptions &options,
                                           pmdEDUCB *cb,
                                           coordProcessResult &result )
   {
      return SDB_OK ;
   }

   void _coordOperator::_doneMainCLOp( coordCataSel &cataSel,
                                       coordSendMsgIn &inMsg,
                                       coordSendOptions &options,
                                       pmdEDUCB *cb,
                                       coordProcessResult &result )
   {
   }

   BOOLEAN _coordOperator::_interruptWhenFailed() const
   {
      return FALSE ;
   }

   BOOLEAN _coordOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      return FALSE ;
   }

   void _coordOperator::_onNodeReply( INT32 processType,
                                      MsgOpReply *pReply,
                                      pmdEDUCB *cb,
                                      coordSendMsgIn &inMsg )
   {
   }

}

