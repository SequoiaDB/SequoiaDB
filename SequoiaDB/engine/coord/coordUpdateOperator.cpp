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
#include "rtnCommandDef.hpp"
#include "coordUtil.hpp"
#include "mthModifier.hpp"
#include "coordShardKicker.hpp"
#include "coordInsertOperator.hpp"
#include "mthMatchTree.hpp"
#include "mthModifier.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "ossUtil.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordUpdateOperator implement
   */
   _coordUpdateOperator::_coordUpdateOperator()
   {
      _insertedNum = 0 ;

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

   UINT32 _coordUpdateOperator::getInsertedNum() const
   {
      return _insertedNum ;
   }

   UINT32 _coordUpdateOperator::getUpdatedNum() const
   {
      return _recvNum ;
   }

   void _coordUpdateOperator::clearStat()
   {
      _insertedNum = 0 ;
      _recvNum = 0 ;
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

      try
      {
         boSelector = BSONObj( pSelector ) ;
         boHint = BSONObj( pHint ) ;
         boUpdator = BSONObj( pUpdator ) ;

         if ( boUpdator.isEmpty() )
         {
            PD_LOG( PDERROR, "modifier can't be empty" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, Matcher:%s, Updator:%s, Hint:%s, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             boSelector.toString().c_str(),
                             boUpdator.toString().c_str(),
                             boHint.toString().c_str(),
                             oldFlag, oldFlag ) ;
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

         if ( cataSel.getCataPtr()->isSharded() )
         {
            coordShardKicker shardKicker ;
            shardKicker.bind( _pResource, cataSel.getCataPtr() ) ;

            rc = shardKicker.kickShardingKey( boUpdator, tmpNewObj,
                                              isChanged, cb,
                                              boSelector,
                                              keepShardingKey ) ;
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
               PD_LOG( PDERROR, "Kick sharding key for collection[%s] "
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
                  tmpNewObj = BSON( "$null" << BSON( "null" << 1 ) ) ;
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
                  _groupSession.getGroupCtrl()->incRetry() ;
                  goto retry ;
               }
               else
               {
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
         rc = SDB_OK ;
      }
      else
      {
         PD_LOG( PDERROR, "Update failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

      if ( ( flag & FLG_UPDATE_UPSERT ) && 0 == _recvNum )
      {
         if ( OSS_BIT_TEST( cataSel.getCataPtr()->getCatalogSet()->getAttribute(),
                            DMS_MB_ATTR_STRICTDATAMODE ) )
         {
            strictDataMode = TRUE ;
         }
         rc = _upsert( pCollectionName, boSelector, boUpdator, boHint,
                       strictDataMode, cb, _insertedNum, contextID, buf ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( oldFlag & FLG_UPDATE_RETURNNUM )
      {
         contextID = ossPack32To64( _insertedNum, _recvNum ) ;
      }
      if ( pCollectionName )
      {
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_UPDATE_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "UpdatedNum:%llu, InsertedNum:%u, Matcher:%s, "
                      "Updator:%s, Hint:%s, Flag:0x%08x(%u)",
                      _recvNum, _insertedNum,
                      boSelector.toString().c_str(),
                      boUpdator.toString().c_str(),
                      boHint.toString().c_str(), oldFlag, oldFlag ) ;
      }
      if ( pMsgBuff )
      {
         cb->releaseBuff( pMsgBuff ) ;
      }
      PD_TRACE_EXITRC ( COORD_UPDATEOPR_EXEC, rc ) ;
      return rc ;
   error:
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                   cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 _coordUpdateOperator::_upsert( const CHAR *pCollectionName,
                                        const BSONObj &matcher,
                                        const BSONObj &updator,
                                        const BSONObj &hint,
                                        BOOLEAN strictDataMode,
                                        pmdEDUCB *cb,
                                        UINT32 &insertNum,
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
                       target.toString().c_str() ) ;
            }
            else
            {
               PD_LOG( PDERROR, "Insert record[%s] failed when upsert, rc: %d",
                       target.toString().c_str(), rc ) ;
            }
            goto error ;
         }
         insertNum += insertOpr.getInsertedNum() ;
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

            ioItem.iovBase = (CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
            ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpUpdate, name) +
                                                    pUpMsg->nameLength + 1, 4 ) -
                            sizeof( MsgHeader ) ;
            iovec.push_back( ioItem ) ;

            boNew = _buildNewSelector( boSelector, subCLLst ) ;
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

}

