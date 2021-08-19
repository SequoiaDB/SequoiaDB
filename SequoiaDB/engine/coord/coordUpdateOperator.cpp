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

   Source File Name = coordUpdateOperator.cpp

   Descriptive Name = Coord Update

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   update operation on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordUpdateOperator.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "rtn.hpp"
#include "rtnCommandDef.hpp"
#include "coordUtil.hpp"
#include "mthModifier.hpp"
#include "coordKeyKicker.hpp"
#include "coordInsertOperator.hpp"
#include "mthMatchTree.hpp"
#include "mthModifier.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "ossUtil.hpp"

using namespace bson;

namespace engine
{

   static BSONObj s_nullUpdator = BSON( "$null" << BSON( "null" << 1 ) ) ;

   /*
      _coordUpdateOperator implement
   */
   _coordUpdateOperator::_coordUpdateOperator()
   {
      const static string s_name( "Update" ) ;
      setName( s_name ) ;
   }

   _coordUpdateOperator::~_coordUpdateOperator()
   {
      SDB_ASSERT( 0 == _vecBlock.size(), "Block must be empty" ) ;
   }

   BOOLEAN _coordUpdateOperator::isReadOnly() const
   {
      return FALSE ;
   }

   UINT64 _coordUpdateOperator::getInsertedNum() const
   {
      return _upResult.insertedNum() ;
   }

   UINT64 _coordUpdateOperator::getUpdatedNum() const
   {
      return _upResult.updateNum() ;
   }

   UINT64 _coordUpdateOperator::getModifiedNum() const
   {
      return _upResult.modifiedNum() ;
   }

   void _coordUpdateOperator::clearStat()
   {
      _upResult.reset() ;
   }

   //PD_TRACE_DECLARE_FUNCTION ( COORD_UPDATEOPR_EXEC, "_coordUpdateOperator::execute" )
   INT32 _coordUpdateOperator::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_UPDATEOPR_EXEC ) ;

      // process define
      coordSendOptions sendOpt( TRUE ) ;
      coordSendMsgIn inMsg( pMsg ) ;
      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      coordCataSel cataSel ;
      MsgRouteID errNodeID ;

      BSONObj newUpdator ;
      CHAR *pMsgBuff = NULL ;
      INT32 buffLen  = 0 ;
      MsgOpUpdate *pNewUpdate          = NULL ;

      BSONObjBuilder retBuilder( COORD_RET_BUILDER_DFT_SIZE ) ;

      // fill default-reply(update success)
      MsgOpUpdate *pUpdate             = (MsgOpUpdate *)pMsg ;
      INT32 oldFlag                    = pUpdate->flags ;
      pUpdate->flags                  |= FLG_UPDATE_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag                       = 0 ;
      CHAR *pCollectionName            = NULL ;
      CHAR *pSelector                  = NULL ;
      CHAR *pUpdator                   = NULL ;
      CHAR *pHint                      = NULL ;
      BOOLEAN strictDataMode           = FALSE ;
      BSONObj boSelector ;
      BSONObj boHint ;
      BSONObj boUpdator ;

      rc = msgExtractUpdate( (CHAR*)pMsg, &flag, &pCollectionName,
                             &pSelector, &pUpdator, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse update request, rc: %d", rc ) ;
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
         BSONObj dummy ;
         boSelector = BSONObj( pSelector ) ;
         boHint = BSONObj( pHint ) ;
         boUpdator = BSONObj( pUpdator ) ;
         BSONObj clientInfo ;

         if ( cb->getMonQueryCB() && !boHint.getField("$"FIELD_NAME_CLIENTINFO).eoo() )
         {
            rc = rtnGetObjElement( boHint, "$"FIELD_NAME_CLIENTINFO, clientInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         "$"FIELD_NAME_CLIENTINFO, rc ) ;
            cb->getMonQueryCB()->clientInfo = clientInfo.getOwned() ;
         }

         rtnQueryOptions options( boSelector, dummy, dummy, boHint, pCollectionName,
                                  0, -1, oldFlag ) ;

         if ( boUpdator.isEmpty() )
         {
            PD_LOG( PDERROR, "modifier can't be empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         options.setUpdator( boUpdator ) ;

         // add last op info
         MON_SAVE_OP_OPTION( cb->getMonAppCB(), pMsg->opCode, options ) ;

         MONQUERY_SET_QUERY_TEXT( cb, cb->getMonAppCB()->getLastOpDetail() ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Update failed, received unexpected error: %s",
                      e.what() ) ;
      }

      rc = cataSel.bind( _pResource, pCollectionName, cb, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pCollectionName, rc ) ;
         goto error ;
      }

      pNewUpdate = pUpdate ;

   retry:
      do
      {
         BSONObj tmpNewObj = boUpdator ;
         BOOLEAN isChanged = FALSE ;
         BOOLEAN keepShardingKey = OSS_BIT_TEST( flag,
                                                 FLG_UPDATE_KEEP_SHARDINGKEY ) ;

         if ( cataSel.getCataPtr()->isSharded() ||
              cataSel.getCataPtr()->hasAutoIncrement() )
         {
            coordKeyKicker kicker ;
            kicker.bind( _pResource, cataSel.getCataPtr() ) ;

            rc = kicker.kickKey( boUpdator, tmpNewObj, isChanged, cb,
                                 boSelector, keepShardingKey ) ;
            if ( SDB_UPDATE_SHARD_KEY == rc && !cataSel.hasUpdated() )
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
            else if ( rc )
            {
               PD_LOG( PDERROR, "Kick key for collection[%s] "
                       "failed, rc: %d", pCollectionName, rc ) ;
               goto error ;
            }
         }

         if ( !isChanged )
         {
            pNewUpdate = pUpdate ;
         }
         else if ( !pMsgBuff || !tmpNewObj.equal( newUpdator ) )
         {
            if ( tmpNewObj.isEmpty() )
            {
               if ( flag & FLG_UPDATE_UPSERT )
               {
                  tmpNewObj = s_nullUpdator ;
               }
               else if ( !cataSel.hasUpdated() )
               {
                  rc = cataSel.updateCataInfo( NULL, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Update collection[%s]'s catalog info "
                             "failed in update operator, rc: %d",
                             pCollectionName, rc ) ;
                     goto error ;
                  }
                  /// inc retry time
                  _groupSession.getGroupCtrl()->incRetry() ;
                  goto retry ;
               }
               else
               {
                  // don't do anything
                  goto done ;
               }
            }

            newUpdator = tmpNewObj ;
            rc = msgBuildUpdateMsg( &pMsgBuff, &buffLen, pUpdate->name,
                                    flag, 0, &boSelector,
                                    &newUpdator, &boHint,
                                    cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build update request, rc: %d", rc ) ;
            pNewUpdate = (MsgOpUpdate *)pMsgBuff ;
         }

         pNewUpdate->version = cataSel.getCataPtr()->getVersion() ;
         pNewUpdate->w = 0 ;
         if ( pNewUpdate->flags & FLG_UPDATE_UPSERT )
         {
            pNewUpdate->flags &= ~FLG_UPDATE_UPSERT ;
         }
         inMsg._pMsg = ( MsgHeader* )pNewUpdate ;

         rcTmp = doOpOnCL( cataSel, boSelector, inMsg, sendOpt, cb, result ) ;
      }while( FALSE ) ;

      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         // do nothing, for upsert
      }
      else if ( SDB_COORD_UPDATE_MULTI_NODES == rcTmp && !cataSel.hasUpdated() )
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
      else if ( SDB_CAT_NO_MATCH_CATALOG == rcTmp )
      {
         /// ignore
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG( PDERROR, "Update failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

      // upsert
      if ( ( flag & FLG_UPDATE_UPSERT ) && 0 == _upResult.updateNum() )
      {
         if ( OSS_BIT_TEST( cataSel.getCataPtr()->getCatalogSet()->getAttribute(),
                            DMS_MB_ATTR_STRICTDATAMODE ) )
         {
            strictDataMode = TRUE ;
         }
         rc = _upsert( pCollectionName, boSelector, boUpdator, boHint,
                       strictDataMode, cb, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pCollectionName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_UPDATE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "UpdatedNum:%llu, ModifiedNum:%llu, InsertedNum:%llu, "
                      "Matcher:%s, Updator:%s, Hint:%s, Flag:0x%08x(%u)",
                      _upResult.updateNum(), _upResult.modifiedNum(),
                      _upResult.insertedNum(),
                      boSelector.toPoolString().c_str(),
                      boUpdator.toPoolString().c_str(),
                      boHint.toPoolString().c_str(),
                      oldFlag, oldFlag ) ;
      }

      if ( buf )
      {
         if ( rc || ( oldFlag & FLG_UPDATE_RETURNNUM ) )
         {
            _upResult.toBSON( retBuilder ) ;
         }

         if ( !retBuilder.isEmpty() )
         {
            *buf = rtnContextBuf( retBuilder.obj() ) ;
         }
      }

      if ( pMsgBuff )
      {
         cb->releaseBuff( pMsgBuff ) ;
      }
      PD_TRACE_EXITRC ( COORD_UPDATEOPR_EXEC, rc ) ;
      return rc ;
   error:
      if ( buf && ( nokRC.size() > 0 || rc ) )
      {
         coordBuildErrorObj( _pResource, rc, cb, &nokRC, retBuilder ) ;
         coordSetResultInfo( rc, nokRC, &_upResult ) ;
      }
      goto done ;
   }

   INT32 _coordUpdateOperator::_upsert( const CHAR *pCollectionName,
                                        const BSONObj &matcher,
                                        const BSONObj &updator,
                                        const BSONObj &hint,
                                        BOOLEAN strictDataMode,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      _mthMatchTree matcherTree ;
      mthModifier modifier ;
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;

      try
      {
         BSONElement setOnInsert ;
         BSONObj source ;
         BSONObj target ;

         coordInsertOperator insertOpr ;

         rc = matcherTree.loadPattern ( matcher ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to load matcher[%s], rc: %d",
                       matcher.toString().c_str(), rc ) ;

         source = matcherTree.getEqualityQueryObject( NULL ) ;

         rc = modifier.loadPattern( updator, NULL, TRUE, NULL, strictDataMode ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load updator[%s], rc: %d",
                      updator.toString().c_str(), rc ) ;

         rc = modifier.modify( source, target ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate upsertor, rc: %d",
                      rc ) ;

         setOnInsert = hint.getField( FIELD_NAME_SET_ON_INSERT ) ;
         if ( !setOnInsert.eoo() )
         {
            rc = rtnUpsertSet( setOnInsert, target ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to set when upsert, rc: %d",
                         rc ) ;
         }

         rc = insertOpr.init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init coord insert operator failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         /// build buff
         rc = msgBuildInsertMsg( &pBuff, &buffSize, pCollectionName,
                                 0, 0, &target, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build insert message failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = insertOpr.execute( (MsgHeader*)pBuff, cb, contextID, buf ) ;
         if ( rc )
         {
            if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG( PDWARNING, "Insert record[%s] failed because of "
                       "the record is already exist when upsert",
                       target.toPoolString().c_str() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Insert record[%s] failed when upsert, rc: %d",
                       target.toPoolString().c_str(), rc ) ;
            }
            goto error ;
         }
         _upResult.incInsertedNum( insertOpr.getInsertedNum() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when upsert: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pBuff )
      {
         cb->releaseBuff( pBuff ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _coordUpdateOperator::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_UPDATE_REQ ;
   }

   void _coordUpdateOperator::_clearBlock( pmdEDUCB *cb )
   {
      for ( INT32 i = 0 ; i < (INT32)_vecBlock.size() ; ++i )
      {
         cb->releaseBuff( _vecBlock[ i ] ) ;
      }
      _vecBlock.clear() ;
   }

   INT32 _coordUpdateOperator::_checkUpdateOne( coordSendMsgIn &inMsg,
                                                coordSendOptions &options,
                                                CoordGroupSubCLMap *grpSubCl )
   {
      MsgOpUpdate *pMsg = (MsgOpUpdate*)inMsg.msg() ;
      INT32 rc = SDB_OK ;

      if ( pMsg->flags & FLG_UPDATE_ONE )
      {
         if ( ( options._groupLst.size() > 1 ) ||
              ( grpSubCl && grpSubCl->size() >= 1 &&
                grpSubCl->begin()->second.size() > 1 ) )
         {
            rc = SDB_COORD_UPDATE_MULTI_NODES ;
            PD_LOG( PDERROR, "Update one can't be used "
                    "in multiple nodes or sub-collections, rc: %d", rc ) ;
         }

      }

      return rc ;
   }

   INT32 _coordUpdateOperator::_prepareCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      return _checkUpdateOne( inMsg, options, NULL ) ;
   }

   INT32 _coordUpdateOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                 coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 pmdEDUCB *cb,
                                                 coordProcessResult &result )
   {
      INT32 rc                = SDB_OK ;
      MsgOpUpdate *pUpMsg     = ( MsgOpUpdate* )inMsg.msg() ;

      INT32 flag              = 0 ;
      CHAR *pCollectionName   = NULL;
      CHAR *pSelector         = NULL ;
      CHAR *pUpdator          = NULL ;
      CHAR *pHint             = NULL;

      CHAR *pBuff             = NULL ;
      UINT32 buffLen          = 0 ;
      UINT32 buffPos          = 0 ;

      CoordGroupSubCLMap &grpSubCl = cataSel.getGroup2SubsMap() ;
      CoordGroupSubCLMap::iterator it ;

      rc = _checkUpdateOne( inMsg, options, &grpSubCl ) ;
      if ( rc )
      {
         goto error ;
      }

      inMsg.data()->clear() ;

      rc = msgExtractUpdate( (CHAR*)pUpMsg, &flag, &pCollectionName,
                             &pSelector, &pUpdator, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse update request, rc: %d",
                   rc ) ;

      try
      {
         BSONObj boSelector( pSelector ) ;
         BSONObj boUpdator( pUpdator ) ;
         BSONObj boHint( pHint ) ;
         BSONObj boNew ;

         it = grpSubCl.begin() ;
         while( it != grpSubCl.end() )
         {
            CoordSubCLlist &subCLLst = it->second ;

            netIOVec &iovec = inMsg._datas[ it->first ] ;
            netIOV ioItem ;

            // 1. first vec
            ioItem.iovBase = (CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
            ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpUpdate, name) +
                                                    pUpMsg->nameLength + 1, 4 ) -
                            sizeof( MsgHeader ) ;
            iovec.push_back( ioItem ) ;

            // 2. new deletor vec( selector )
            boNew = _buildNewSelector( boSelector, subCLLst ) ;
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

            // 3. for last( updator + hint )
            ioItem.iovBase = boUpdator.objdata() ;
            ioItem.iovLen = ossRoundUpToMultipleX( boUpdator.objsize(), 4 ) +
                            boHint.objsize() ;
            iovec.push_back( ioItem ) ;

            ++it ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse update message occur exception: %s",
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

   void _coordUpdateOperator::_doneMainCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      _clearBlock( cb ) ;
      inMsg._datas.clear() ;
   }

   BSONObj _coordUpdateOperator::_buildNewSelector( const BSONObj &selector,
                                                    const CoordSubCLlist &subCLList )
   {
      BSONObjBuilder builder( selector.objsize() +
                              subCLList.size() * COORD_SUBCL_NAME_DFT_LEN ) ;
      builder.appendElements( selector ) ;

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

   void _coordUpdateOperator::_onNodeReply( INT32 processType,
                                            MsgOpReply *pReply,
                                            pmdEDUCB *cb,
                                            coordSendMsgIn &inMsg )
   {
      BOOLEAN inProcessed = FALSE ;
      BOOLEAN upProcessed = FALSE ;
      BOOLEAN moProcessed = FALSE ;

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
               if ( !moProcessed &&
                    0 == ossStrcmp( e.fieldName(), FIELD_NAME_MODIFIED_NUM ) )
               {
                  moProcessed = TRUE ;
                  _upResult.incModifiedNum( (UINT64)e.numberLong() ) ;
               }
               else if ( !upProcessed &&
                         0 == ossStrcmp( e.fieldName(),
                                         FIELD_NAME_UPDATE_NUM ) )
               {
                  upProcessed = TRUE ;
                  _upResult.incUpdatedNum( (UINT64)e.numberLong() ) ;
               }
               else if ( !inProcessed &&
                         0 == ossStrcmp( e.fieldName(),
                                         FIELD_NAME_INSERT_NUM ) )
               {
                  inProcessed = TRUE ;
                  _upResult.incInsertedNum( (UINT64)e.numberLong() ) ;
               }

               if ( inProcessed && upProcessed && moProcessed )
               {
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "Extract update result exception: %s",
                    e.what() ) ;
         }
      }

      if ( !upProcessed )
      {
         _recvNum = 0 ;
         _coordTransOperator::_onNodeReply( processType, pReply, cb, inMsg ) ;
         _upResult.incUpdatedNum( _recvNum ) ;
      }
   }

}

