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

   Source File Name = coordDeleteOperator.cpp

   Descriptive Name = Coord Delete

   When/how to use: this program may be used on binary and text-formatted
   version of runtime component. This file contains code logic for
   data delete request from coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordDeleteOperator.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "rtn.hpp"
#include "rtnCommandDef.hpp"
#include "coordUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordDeleteOperator implement
   */
   _coordDeleteOperator::_coordDeleteOperator()
   {
      const static string s_name( "Delete" ) ;
      setName( s_name ) ;
   }

   _coordDeleteOperator::~_coordDeleteOperator()
   {
      SDB_ASSERT( 0 == _vecBlock.size(), "Block must be empty" ) ;
   }

   BOOLEAN _coordDeleteOperator::isReadOnly() const
   {
      return FALSE ;
   }

   UINT32 _coordDeleteOperator::getDeletedNum() const
   {
      return _delResult.deletedNum() ;
   }

   void _coordDeleteOperator::clearStat()
   {
      _delResult.reset() ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( COORD_OPERATORDEL_EXE, "_coordDeleteOperator::execute" )
   INT32 _coordDeleteOperator::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_OPERATORDEL_EXE ) ;

      // process define
      coordSendOptions sendOpt( TRUE ) ;
      coordSendMsgIn inMsg( pMsg ) ;
      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      coordCataSel cataSel ;
      MsgRouteID errNodeID ;

      BSONObj boDeletor ;

      BSONObjBuilder retBuilder( COORD_RET_BUILDER_DFT_SIZE ) ;

      // fill default-reply(delete success)
      MsgOpDelete *pDelMsg             = (MsgOpDelete *)pMsg ;
      INT32 oldFlag                    = pDelMsg->flags ;
      pDelMsg->flags                  |= FLG_DELETE_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0;
      CHAR *pCollectionName = NULL ;
      CHAR *pDeletor = NULL ;
      CHAR *pHint = NULL ;

      rc = msgExtractDelete( (CHAR*)pMsg, &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;

      if( rc )
      {
         PD_LOG( PDERROR,"Failed to parse delete request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      MONQUERY_SET_NAME( cb, pCollectionName ) ;

      if ( 0 == ossStrncmp( pCollectionName, CMD_ADMIN_PREFIX SYS_VIRTUAL_CS".",
                            SYS_VIRTUAL_CS_LEN + 1 ) )
      {
         rc = SDB_COORD_UNKNOWN_OP_REQ ;
         goto error ;
      }

      try
      {
         BSONObj hint = BSONObj (pHint) ;
         BSONObj clientInfo ;
         BSONObj dummy ;
         boDeletor = BSONObj( pDeletor ) ;

         if ( cb->getMonQueryCB() && !hint.getField("$"FIELD_NAME_CLIENTINFO).eoo() )
         {
            rc = rtnGetObjElement( hint, "$"FIELD_NAME_CLIENTINFO, clientInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         "$"FIELD_NAME_CLIENTINFO, rc ) ;
            cb->getMonQueryCB()->clientInfo = clientInfo.getOwned() ;
         }

         rtnQueryOptions options( boDeletor, dummy, dummy, hint, pCollectionName,
                                  0, -1, oldFlag ) ;

         // add last op info
         MON_SAVE_OP_OPTION( cb->getMonAppCB(), pMsg->opCode, options ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Delete failed, received unexpected error: %s",
                      e.what() ) ;
      }

      MONQUERY_SET_QUERY_TEXT( cb, cb->getMonAppCB()->getLastOpDetail() ) ;

      rc = cataSel.bind( _pResource, pCollectionName, cb, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pCollectionName, rc ) ;
         goto error ;
      }

   retry:
      pDelMsg->version = cataSel.getCataPtr()->getVersion() ;
      pDelMsg->w = 0 ;
      rcTmp = doOpOnCL( cataSel, boDeletor, inMsg, sendOpt, cb, result ) ;

      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         goto done ;
      }
      else if ( SDB_COORD_DELETE_MULTI_NODES == rcTmp && !cataSel.hasUpdated() )
      {
         rc = cataSel.updateCataInfo( pCollectionName, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update collection[%s]'s catalog info "
                    "failed in update operator, rc: %d",
                    pCollectionName, rc ) ;
            goto error ;
         }
         _groupSession.getGroupCtrl()->incRetry() ;
         goto retry ;
      }
      else if ( checkRetryForCLOpr( rcTmp, &nokRC, cataSel, inMsg.msg(),
                                    cb, rc, &errNodeID, TRUE ) )
      {
         nokRC.clear() ;
         _groupSession.getGroupCtrl()->incRetry() ;
         goto retry ;
      }
      else
      {
         PD_LOG( PDERROR, "Delete failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( pCollectionName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_DELETE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "DeletedNum:%u, Deletor:%s, Hint:%s, Flag:0x%08x(%u)",
                      _delResult.deletedNum(), boDeletor.toPoolString().c_str(),
                      BSONObj(pHint).toPoolString().c_str(),
                      oldFlag, oldFlag ) ;
      }
      if ( buf )
      {
         if ( ( oldFlag & FLG_DELETE_RETURNNUM ) || rc )
         {
            _delResult.toBSON( retBuilder ) ;
         }

         if ( !retBuilder.isEmpty() )
         {
            *buf = rtnContextBuf( retBuilder.obj() ) ;
         }
      }
      PD_TRACE_EXITRC ( COORD_OPERATORDEL_EXE, rc ) ;
      return rc ;
   error:
      if ( buf && ( nokRC.size() > 0 || rc ) )
      {
         coordBuildErrorObj( _pResource, rc, cb, &nokRC, retBuilder ) ;
         coordSetResultInfo( flag, nokRC, &_delResult ) ;
      }
      goto done ;
   }

   void _coordDeleteOperator::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_DELETE_REQ ;
   }

   void _coordDeleteOperator::_clearBlock( pmdEDUCB *cb )
   {
      for ( INT32 i = 0 ; i < (INT32)_vecBlock.size() ; ++i )
      {
         cb->releaseBuff( _vecBlock[ i ] ) ;
      }
      _vecBlock.clear() ;
   }

   INT32 _coordDeleteOperator::_checkDeleteOne( coordSendMsgIn &inMsg,
                                                coordSendOptions &options,
                                                CoordGroupSubCLMap *grpSubCl )
   {
      MsgOpDelete *pMsg = (MsgOpDelete*)inMsg.msg() ;
      INT32 rc = SDB_OK ;

      if ( pMsg->flags & FLG_DELETE_ONE )
      {
         if ( ( options._groupLst.size() > 1 ) ||
              ( grpSubCl && grpSubCl->size() >= 1 &&
                grpSubCl->begin()->second.size() > 1 ) )
         {
            rc = SDB_COORD_DELETE_MULTI_NODES ;
            PD_LOG( PDERROR, "Delete one can't be used "
                    "in multiple nodes or sub-collections, rc: %d", rc ) ;
         }

      }

      return rc ;
   }

   INT32 _coordDeleteOperator::_prepareCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      return _checkDeleteOne( inMsg, options, NULL ) ;
   }

   INT32 _coordDeleteOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                 coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 pmdEDUCB *cb,
                                                 coordProcessResult &result )
   {
      INT32 rc                = SDB_OK ;
      MsgOpDelete *pDelMsg    = ( MsgOpDelete* )inMsg.msg() ;

      INT32 flag              = 0 ;
      CHAR *pCollectionName   = NULL;
      CHAR *pDeletor          = NULL;
      CHAR *pHint             = NULL;

      CoordGroupSubCLMap &grpSubCl = cataSel.getGroup2SubsMap() ;
      CoordGroupSubCLMap::iterator it ;

      rc = _checkDeleteOne( inMsg, options, &grpSubCl ) ;
      if ( rc )
      {
         goto error ;
      }

      inMsg.data()->clear() ;

      rc = msgExtractDelete( (CHAR*)inMsg.msg(), &flag, &pCollectionName,
                             &pDeletor, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse delete request, rc: %d",
                   rc ) ;

      try
      {
         BSONObj boDeletor( pDeletor ) ;
         BSONObj boHint( pHint ) ;
         BSONObj boNew ;

         CHAR *pBuff             = NULL ;
         UINT32 buffLen          = 0 ;
         UINT32 buffPos          = 0 ;

         it = grpSubCl.begin() ;
         while( it != grpSubCl.end() )
         {
            CoordSubCLlist &subCLLst = it->second ;

            netIOVec &iovec = inMsg._datas[ it->first ] ;
            netIOV ioItem ;

            // 1. first vec
            ioItem.iovBase = (const CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
            ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpDelete, name) +
                                                    pDelMsg->nameLength + 1, 4 ) -
                            sizeof( MsgHeader ) ;
            iovec.push_back( ioItem ) ;

            // 2. new deletor vec
            boNew = _buildNewDeletor( boDeletor, subCLLst ) ;
            // 2.1 add to buff
            UINT32 roundLen = ossRoundUpToMultipleX( boNew.objsize(), 4 ) ;
            if ( buffPos + roundLen > buffLen )
            {
               UINT32 alignLen = ossRoundUpToMultipleX( roundLen,
                                                        DMS_PAGE_SIZE4K ) ;
               rc = cb->allocBuff( alignLen, &pBuff, &buffLen ) ;
               PD_RC_CHECK( rc, PDERROR, "Alloc buff[%u] failed, rc: %d",
                            alignLen, rc ) ;
               _vecBlock.push_back( pBuff ) ;
               buffPos = 0 ;
            }
            ossMemcpy( &pBuff[ buffPos ], boNew.objdata(), boNew.objsize() ) ;
            ioItem.iovBase = &pBuff[ buffPos ] ;
            ioItem.iovLen = roundLen ;
            buffPos += roundLen ;
            iovec.push_back( ioItem ) ;

            // 3. hinter vec
            ioItem.iovBase = boHint.objdata() ;
            ioItem.iovLen = boHint.objsize() ;
            iovec.push_back( ioItem ) ;

            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse delete message occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _clearBlock( cb ) ;
      goto done ;
   }

   void _coordDeleteOperator::_doneMainCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      _clearBlock( cb ) ;
      inMsg._datas.clear() ;
   }

   BSONObj _coordDeleteOperator::_buildNewDeletor( const BSONObj &deletor,
                                                   const CoordSubCLlist &subCLList )
   {
      BSONObjBuilder builder( deletor.objsize() +
                              subCLList.size() * COORD_SUBCL_NAME_DFT_LEN ) ;

      builder.appendElements( deletor ) ;

      /*
         Append array, as { SubCLName : [ "a.a", "b.a" ] }
      */
      BSONArrayBuilder babSubCL( builder.subarrayStart( CAT_SUBCL_NAME ) ) ;
      CoordSubCLlist::const_iterator iterCL = subCLList.begin();
      while( iterCL != subCLList.end() )
      {
         babSubCL.append( *iterCL ) ;
         ++iterCL ;
      }
      babSubCL.done() ;

      return builder.obj() ;
   }

   void _coordDeleteOperator::_onNodeReply( INT32 processType,
                                            MsgOpReply *pReply,
                                            pmdEDUCB *cb,
                                            coordSendMsgIn &inMsg )
   {
      BOOLEAN processed = FALSE ;

      if ( pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) &&
           1 == pReply->numReturned )
      {
         try
         {
            BSONObj objResult( ( const CHAR* )pReply + sizeof( MsgOpReply ) ) ;
            BSONObjIterator itr( objResult ) ;
            while ( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( !processed &&
                    0 == ossStrcmp( e.fieldName(), FIELD_NAME_DELETE_NUM ) )
               {
                  processed = TRUE ;
                  _delResult.incDeletedNum( (UINT64)e.numberLong() ) ;
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "Extract delete result exception: %s",
                    e.what() ) ;
         }
      }

      if ( !processed )
      {
         _recvNum = 0 ;
         _coordTransOperator::_onNodeReply( processType, pReply, cb, inMsg ) ;
         _delResult.incDeletedNum( _recvNum ) ;
      }
   }

}

