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

   Source File Name = coordQueryOperator.cpp

   Descriptive Name = Coord Query

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   query on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04-17-2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordQueryOperator.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "coordCommon.hpp"
#include "coordShardKicker.hpp"
#include "coordUtil.hpp"
#include "coordFactory.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "ossUtil.hpp"

using namespace bson;

namespace engine
{
   /*
      _coordQueryOperator implement
   */
   _coordQueryOperator::_coordQueryOperator( BOOLEAN readOnly )
   {
      _pContext = NULL ;
      _processRet = SDB_OK ;
      setReadOnly( readOnly ) ;

      const static string s_name( "Query" ) ;
      setName( s_name ) ;
   }

   _coordQueryOperator::~_coordQueryOperator()
   {
      SDB_ASSERT( NULL == _pContext, "Context must be NULL" ) ;
      SDB_ASSERT( 0 == _vecBlock.size(), "Block must be empty" ) ;
   }

   INT32 _coordQueryOperator::_checkQueryModify( coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 CoordGroupSubCLMap *grpSubCl )
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )inMsg.msg() ;
      INT32 rc = SDB_OK ;

      if ( queryMsg->flags & FLG_QUERY_MODIFY )
      {
         if ( ( options._groupLst.size() > 1 ) ||
              ( grpSubCl && grpSubCl->size() >= 1 &&
                grpSubCl->begin()->second.size() > 1 ) )
         {
            if ( _pContext->getNumToReturn() > 0 ||
                 _pContext->getNumToSkip() > 0 ||
                 queryMsg->numToReturn > 0 ||
                 queryMsg->numToSkip > 0 )
            {
               rc = SDB_RTN_QUERYMODIFY_MULTI_NODES ;
               PD_LOG( PDERROR, "query and modify can't use skip and limit "
                       "in multiple nodes or sub-collections, rc: %d", rc ) ;
            }
         }

         options._primary = TRUE ;
      }

      return rc ;
   }

   void _coordQueryOperator::_optimize( coordSendMsgIn &inMsg,
                                        coordSendOptions &options,
                                        coordProcessResult &result )
   {
      if ( _pContext && 0 == result._sucGroupLst.size() )
      {
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )inMsg.msg() ;
         _pContext->optimizeReturnOptions( pQueryMsg,
                                           options._groupLst.size() ) ;
      }
   }

   INT32 _coordQueryOperator::_prepareCLOp( coordCataSel &cataSel,
                                            coordSendMsgIn &inMsg,
                                            coordSendOptions &options,
                                            pmdEDUCB *cb,
                                            coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      _optimize( inMsg, options, result ) ;

      rc = _checkQueryModify( inMsg, options, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordQueryOperator::_doneCLOp( coordCataSel &cataSel,
                                        coordSendMsgIn &inMsg,
                                        coordSendOptions &options,
                                        pmdEDUCB *cb,
                                        coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;

      ROUTE_REPLY_MAP *pOkReply = result._pOkReply ;
      SDB_ASSERT( pOkReply, "Ok reply invalid" ) ;

      BOOLEAN takeOver = FALSE ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;
      ROUTE_REPLY_MAP::iterator it = pOkReply->begin() ;
      while( it != pOkReply->end() )
      {
         takeOver = FALSE ;
         pReply = (MsgOpReply*)(it->second) ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( _pContext )
            {
               rc = _pContext->addSubContext( pReply, takeOver ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Add sub data[node: %s, context: %lld] to "
                          "context[%s] failed, rc: %d",
                          routeID2String( nodeID ).c_str(), pReply->contextID,
                          _pContext->toString().c_str(), rc ) ;
                  _processRet = rc ;
               }
            }
            else
            {
               SDB_ASSERT( pReply->contextID == -1, "Context leak" ) ;
            }
         }

         if ( !takeOver )
         {
            SDB_OSS_FREE( pReply ) ;
         }
         ++it ;
      }
      pOkReply->clear() ;
   }

   void _coordQueryOperator::_clearBlock( pmdEDUCB *cb )
   {
      for ( INT32 i = 0 ; i < (INT32)_vecBlock.size() ; ++i )
      {
         cb->releaseBuff( _vecBlock[ i ] ) ;
      }
      _vecBlock.clear() ;
   }

   INT32 _coordQueryOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                coordSendMsgIn &inMsg,
                                                coordSendOptions &options,
                                                pmdEDUCB *cb,
                                                coordProcessResult &result )
   {
      INT32 rc                = SDB_OK ;
      MsgOpQuery *pQueryMsg   = ( MsgOpQuery* )inMsg.msg() ;

      INT32 flags             = 0 ;
      CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      CHAR *pQuery            = NULL ;
      CHAR *pFieldSelector    = NULL ;
      CHAR *pOrderBy          = NULL ;
      CHAR *pHint             = NULL ;

      BSONObj objQuery ;
      BSONObj objSelector ;
      BSONObj objOrderby ;
      BSONObj objHint ;
      BSONObj newQuery ;

      CHAR *pBuff             = NULL ;
      UINT32 buffLen          = 0 ;
      UINT32 buffPos          = 0 ;

      CoordGroupSubCLMap &grpSubCl = cataSel.getGroup2SubsMap() ;
      CoordGroupSubCLMap::iterator it ;

      _optimize( inMsg, options, result ) ;

      rc = _checkQueryModify( inMsg, options, &grpSubCl ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( options._useSpecialGrp )
      {
         goto done ;
      }

      inMsg.data()->clear() ;

      rc = msgExtractQuery( (CHAR*)pQueryMsg, &flags, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      try
      {
         objQuery = BSONObj( pQuery ) ;
         objSelector = BSONObj( pFieldSelector ) ;
         objOrderby = BSONObj( pOrderBy ) ;
         objHint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      it = grpSubCl.begin() ;
      while( it != grpSubCl.end() )
      {
         CoordSubCLlist &subCLLst = it->second ;

         netIOVec &iovec = inMsg._datas[ it->first ] ;
         netIOV ioItem ;

         ioItem.iovBase = (CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
         ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpQuery, name) +
                         pQueryMsg->nameLength + 1, 4 ) - sizeof( MsgHeader ) ;
         iovec.push_back( ioItem ) ;

         newQuery = _buildNewQuery( objQuery, subCLLst ) ;
         UINT32 roundLen = ossRoundUpToMultipleX( newQuery.objsize(), 4 ) ;
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
         ossMemcpy( &pBuff[ buffPos ], newQuery.objdata(),
                    newQuery.objsize() ) ;
         ioItem.iovBase = &pBuff[ buffPos ] ;
         ioItem.iovLen = roundLen ;
         buffPos += roundLen ;
         iovec.push_back( ioItem ) ;

         ioItem.iovBase = objSelector.objdata() ;
         ioItem.iovLen = ossRoundUpToMultipleX( objSelector.objsize(), 4 ) +
                         ossRoundUpToMultipleX( objOrderby.objsize(), 4 ) +
                         objHint.objsize() ;
         iovec.push_back( ioItem ) ;

         ++it ;
      }

   done:
      return rc ;
   error:
      _clearBlock( cb ) ;
      goto done ;
   }

   void _coordQueryOperator::_doneMainCLOp( coordCataSel &cataSel,
                                            coordSendMsgIn &inMsg,
                                            coordSendOptions &options,
                                            pmdEDUCB *cb,
                                            coordProcessResult &result )
   {
      _clearBlock( cb ) ;
      inMsg._datas.clear() ;

      _doneCLOp( cataSel, inMsg, options, cb, result ) ;
   }

   BSONObj _coordQueryOperator::_buildNewQuery( const BSONObj &query,
                                                const CoordSubCLlist &subCLList )
   {
      BSONObjBuilder builder( query.objsize() +
                              subCLList.size() * COORD_SUBCL_NAME_DFT_LEN) ;
      builder.appendElements( query ) ;

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

   INT32 _coordQueryOperator::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      rtnContextCoord *pContext        = NULL ;
      coordCommandFactory *pFactory    = NULL ;
      coordOperator *pOperator         = NULL ;

      contextID                        = -1 ;

      CHAR *pCollectionName            = NULL ;
      INT32 flag                       = 0 ;
      INT64 numToSkip                  = 0 ;
      INT64 numToReturn                = 0 ;
      CHAR *pQuery                     = NULL ;
      CHAR *pSelector                  = NULL ;
      CHAR *pOrderby                   = NULL ;
      CHAR *pHint                      = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pSelector,
                            &pOrderby, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse query request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      if ( pCollectionName != NULL && '$' == pCollectionName[0] )
      {
         pFactory = coordGetFactory() ;
         rc = pFactory->create( &pCollectionName[1], pOperator ) ;
         if ( rc )
         {
            if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
            {
               PD_LOG( PDERROR, "Create operator by name[%s] failed, rc: %d",
                       pCollectionName, rc ) ;
            }
            goto error ;
         }

         rc = pOperator->init( _pResource, cb, getTimeout() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Init operator[%s] failed, rc: %d",
                    pOperator->getName(), rc ) ;
            goto error ;
         }
         rc = pOperator->execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                    pOperator->getName(), rc ) ;
            goto error ;
         }
      }
      else
      {
         coordSendOptions sendOpt ;

         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Collection:%s, Matcher:%s, Selector:%s, "
                             "OrderBy:%s, Hint:%s, Skip:%llu, Limit:%lld, "
                             "Flag:0x%08x(%u)",
                             pCollectionName,
                             BSONObj(pQuery).toString().c_str(),
                             BSONObj(pSelector).toString().c_str(),
                             BSONObj(pOrderby).toString().c_str(),
                             BSONObj(pHint).toString().c_str(),
                             numToSkip, numToReturn,
                             flag, flag ) ;

         rc = queryOrDoOnCL( pMsg, cb, &pContext, sendOpt, NULL, buf ) ;
         PD_AUDIT_OP( ( flag & FLG_QUERY_MODIFY ? AUDIT_DML : AUDIT_DQL ),
                      MSG_BS_QUERY_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc,
                      "ContextID:%lld, Matcher:%s, Selector:%s, OrderBy:%s, "
                      "Hint:%s, Skip:%llu, Limit:%lld, Flag:0x%08x(%u)",
                      pContext ? pContext->contextID() : -1,
                      BSONObj(pQuery).toString().c_str(),
                      BSONObj(pSelector).toString().c_str(),
                      BSONObj(pOrderby).toString().c_str(),
                      BSONObj(pHint).toString().c_str(),
                      numToSkip, numToReturn,
                      flag, flag ) ;
         PD_RC_CHECK( rc, PDERROR, "Query failed, rc: %d", rc ) ;

         contextID = pContext->contextID() ;

         if ( OSS_BIT_TEST(flag, FLG_QUERY_PREPARE_MORE ) &&
              !OSS_BIT_TEST(flag, FLG_QUERY_MODIFY ) )
         {
            pContext->setPrepareMoreData( TRUE ) ;
         }
      }

   done:
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordQueryOperator::_isUpdate( const BSONObj &hint,
                                           INT32 flags )
   {
      if ( flags & FLG_QUERY_MODIFY )
      {
         BSONObj updator ;
         BSONObj newUpdator ;
         BSONElement modifierEle ;
         BSONObj modifier ;

         modifierEle = hint.getField( FIELD_NAME_MODIFY ) ;
         if ( Object != modifierEle.type() )
         {
            return FALSE ;
         }

         modifier = modifierEle.Obj() ;

         BSONElement updatorEle = modifier.getField( FIELD_NAME_OP ) ;
         if ( String != updatorEle.type() ||
              0 != ossStrcmp( updatorEle.valuestr(), FIELD_OP_VALUE_UPDATE ) )
         {
            return FALSE ;
         }

         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _coordQueryOperator::_generateNewHint( const CoordCataInfoPtr &cataInfo,
                                                const BSONObj &matcher,
                                                const BSONObj &hint,
                                                BSONObj &newHint,
                                                BOOLEAN &isChanged,
                                                BOOLEAN &isEmpty,
                                                pmdEDUCB *cb,
                                                BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      coordShardKicker kicker ;

      BSONObj modifier ;
      BSONObj updator ;
      BSONObj newUpdator ;
      BSONElement ele ;

      ele = hint.getField( FIELD_NAME_MODIFY ) ;
      SDB_ASSERT( Object == ele.type(),
                  "Modifier element must be an Object" ) ;
      if ( Object != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be an Object in Object[%s]",
                 FIELD_NAME_MODIFY, hint.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      modifier = ele.Obj() ;

      ele = modifier.getField( FIELD_NAME_OP_UPDATE ) ;
      SDB_ASSERT( Object == ele.type(),
                  "Updator element must be an Object" ) ;
      if ( Object != ele.type() )
      {
         PD_LOG( PDERROR, "Field[%s] must be an Object in Object[%s]",
                 FIELD_NAME_OP_UPDATE, modifier.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      updator = ele.Obj() ;

      kicker.bind( _pResource, cataInfo ) ;

      rc = kicker.kickShardingKey( updator, newUpdator,
                                   isChanged, cb, matcher,
                                   keepShardingKey ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Kick shardingkey for updator failed, rc: %d", rc ) ;
         goto error ;
      }
      isEmpty = newUpdator.isEmpty() ? TRUE : FALSE ;

      if ( isChanged )
      {
         BSONObjBuilder builder( hint.objsize() ) ;
         BSONObjIterator itr( hint ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            if( 0 == ossStrcmp( FIELD_NAME_MODIFY, e.fieldName() ) &&
                Object == e.type() )
            {
               if ( isEmpty )
               {
                  continue ;
               }

               BSONObjBuilder subBuild( builder.subobjStart(
                  FIELD_NAME_MODIFY ) ) ;
               BSONObjIterator subItr( e.embeddedObject() ) ;
               while( subItr.more() )
               {
                  BSONElement subE = subItr.next() ;
                  if ( 0 == ossStrcmp( FIELD_NAME_OP_UPDATE,
                                       subE.fieldName() ) )
                  {
                     subBuild.append( FIELD_NAME_OP_UPDATE, newUpdator ) ;
                  }
                  else
                  {
                     subBuild.append( subE ) ;
                  }
               } /// end while ( subItr.more() )
               subBuild.done() ;
            } //end if
            else
            {
               builder.append( e ) ;
            }
         } /// end while( itr.more() )

         newHint = builder.obj() ;
      }
      else
      {
         newHint = hint ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordQueryOperator::queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **pContext,
                                             coordSendOptions & sendOpt,
                                             coordQueryConf *pQueryConf,
                                             rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, cb, pContext, sendOpt,
                             NULL, pQueryConf, buf ) ;
   }

   INT32 _coordQueryOperator::queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord **pContext,
                                             coordSendOptions &sendOpt,
                                             CoordGroupList &sucGrpLst,
                                             coordQueryConf *pQueryConf,
                                             rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, cb, pContext, sendOpt,
                             &sucGrpLst, pQueryConf, buf ) ;
   }

   INT32 _coordQueryOperator::_queryOrDoOnCL( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord **pContext,
                                              coordSendOptions &sendOpt,
                                              CoordGroupList *pSucGrpLst,
                                              coordQueryConf *pQueryConf,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      INT64 contextID = -1 ;

      coordCataSel cataSel ;
      const CHAR *pRealCLName = NULL ;
      BOOLEAN openEmptyContext = FALSE ;
      BOOLEAN updateCata = FALSE ;
      BOOLEAN allCataGroup = FALSE ;

      MsgRouteID errNodeID ;
      coordSendMsgIn inMsg( pMsg ) ;

      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;
      ROUTE_REPLY_MAP okReply ;
      result._pOkReply = &okReply ;

      if ( pQueryConf )
      {
         openEmptyContext = pQueryConf->_openEmptyContext ;
         updateCata = pQueryConf->_updateAndGetCata ;
         if ( !pQueryConf->_realCLName.empty() )
         {
            pRealCLName = pQueryConf->_realCLName.c_str() ;
         }
         allCataGroup = pQueryConf->_allCataGroups ;
      }

      SET_RC *pOldIgnoreRC = sendOpt._pIgnoreRC ;
      SET_RC ignoreRC ;
      if ( pOldIgnoreRC )
      {
         ignoreRC = *pOldIgnoreRC ;
      }
      ignoreRC.insert( SDB_DMS_EOC ) ;
      sendOpt._pIgnoreRC = &ignoreRC ;

      MsgOpQuery *pQueryMsg   = ( MsgOpQuery* )pMsg ;
      SDB_RTNCB *pRtncb       = pmdGetKRCB()->getRTNCB() ;
      CHAR *pNewMsg           = NULL ;
      INT32 newMsgSize        = 0 ;
      const CHAR* pLastMsg    = NULL ;
      CHAR *pModifyMsg        = NULL ;
      INT32 modifyMsgSize     = 0 ;

      BOOLEAN isUpdate        = FALSE ;
      INT32 flags             = 0 ;
      CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      CHAR *pQuery            = NULL ;
      CHAR *pFieldSelector    = NULL ;
      CHAR *pOrderBy          = NULL ;
      CHAR *pHint             = NULL ;

      BSONObj objQuery ;
      BSONObj objSelector ;
      BSONObj objOrderby ;
      BSONObj objHint ;
      BSONObj objNewHint ;

      rc = msgExtractQuery( (CHAR*)pMsg, &flags, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract query msg failed, rc: %d", rc ) ;

      try
      {
         if ( !allCataGroup )
         {
            objQuery = BSONObj( pQuery ) ;
         }
         objSelector = BSONObj( pFieldSelector ) ;
         objOrderby = BSONObj( pOrderBy ) ;
         objHint = BSONObj( pHint ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Extrace query msg occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( pContext )
      {
         if ( NULL == *pContext )
         {
            RTN_CONTEXT_TYPE contextType = RTN_CONTEXT_COORD ;

            if ( FLG_QUERY_EXPLAIN & pQueryMsg->flags )
            {
               contextType = RTN_CONTEXT_COORD_EXP ;
            }

            rc = pRtncb->contextNew( contextType,
                                     (rtnContext **)pContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;
         }
         else
         {
            contextID = (*pContext)->contextID() ;
         }
         _pContext = *pContext ;
      }

      if ( _pContext && !_pContext->isOpened() )
      {
         if ( openEmptyContext )
         {
            rtnQueryOptions defaultOptions ;
            rc = _pContext->open( defaultOptions ) ;
         }
         else
         {
            BOOLEAN needResetSubQuery = FALSE ;
            rtnQueryOptions options ;

            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               needResetSubQuery = TRUE ;
            }
            else
            {
               rtnNeedResetSelector( objSelector, objOrderby, needResetSubQuery ) ;
            }

            if ( needResetSubQuery )
            {
               static BSONObj emptyObj = BSONObj() ;
               rc = _buildNewMsg( (const CHAR*)pMsg, &emptyObj, NULL,
                                  pNewMsg, newMsgSize, cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to build new msg, rc: %d", rc ) ;
                  goto error ;
               }
               pQueryMsg = (MsgOpQuery *)pNewMsg ;
               inMsg._pMsg = ( MsgHeader* )pNewMsg ;
            }

            options.setCLFullName( pCollectionName ) ;
            options.setQuery( objQuery ) ;
            options.setOrderBy( objOrderby ) ;
            options.setHint( objHint ) ;
            options.setSkip( pQueryMsg->numToSkip ) ;
            options.setLimit( pQueryMsg->numToReturn ) ;
            options.resetFlag( pQueryMsg->flags ) ;

            if ( OSS_BIT_TEST( pQueryMsg->flags, FLG_QUERY_EXPLAIN ) ||
                 needResetSubQuery )
            {
               options.setSelector( objSelector ) ;
            }
            else
            {
               options.setSelector( BSONObj() ) ;
            }

            rc = _pContext->open( options,
                                  ( FLG_QUERY_MODIFY & pQueryMsg->flags )
                                  ? FALSE : TRUE ) ;

            if ( pQueryMsg->numToReturn > 0 && pQueryMsg->numToSkip > 0 )
            {
               pQueryMsg->numToReturn += pQueryMsg->numToSkip ;
            }
            pQueryMsg->numToSkip = 0 ;

            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               _pContext->getSelector().setStringOutput( TRUE ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

         if ( cb->getMonConfigCB()->timestampON )
         {
            _pContext->getMonCB()->recordStartTimestamp() ;
         }
      }

      rc = cataSel.bind( _pResource,
                         pRealCLName ? pRealCLName : pCollectionName,
                         cb, updateCata, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pRealCLName ? pRealCLName : pCollectionName,
                 rc ) ;
         goto error ;
      }

      if ( updateCata )
      {
         _groupSession.getGroupCtrl()->incRetry() ;
      }

      isUpdate = _isUpdate( objHint, pQueryMsg->flags ) ;
      pLastMsg = ( const CHAR* )pQueryMsg ;

   retry:
      do
      {
         pQueryMsg = ( MsgOpQuery* )pLastMsg ;
         inMsg._pMsg = ( MsgHeader* )pLastMsg ;

         if ( isUpdate && cataSel.getCataPtr()->isSharded() )
         {
            BOOLEAN isChanged = FALSE ;
            BSONObj tmpNewHint ;
            BOOLEAN isEmpty = FALSE ;
            BOOLEAN keepShardingKey = OSS_BIT_TEST( pQueryMsg->flags,
                                      FLG_UPDATE_KEEP_SHARDINGKEY ) ;

            rc = _generateNewHint( cataSel.getCataPtr(), objQuery,
                                   objHint, tmpNewHint, isChanged,
                                   isEmpty, cb, keepShardingKey ) ;
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

            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Generate new hint failed, rc: %d", rc ) ;
               goto error ;
            }
            else if ( isChanged )
            {
               if ( !pModifyMsg || !tmpNewHint.equal( objNewHint ) )
               {
                  rc = _buildNewMsg( (const CHAR*)inMsg._pMsg, NULL,
                                     &tmpNewHint, pModifyMsg,
                                     modifyMsgSize, cb ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to build new msg, rc: %d", rc ) ;
                     goto error ;
                  }
                  pQueryMsg   = (MsgOpQuery *)pModifyMsg ;
                  inMsg._pMsg = (MsgHeader*)pModifyMsg ;

                  if ( isEmpty )
                  {
                     pQueryMsg->flags &= ~FLG_QUERY_MODIFY ;
                  }
                  objNewHint = tmpNewHint ;
               }
               else
               {
                  pQueryMsg   = (MsgOpQuery *)pModifyMsg ;
                  inMsg._pMsg = (MsgHeader*)pModifyMsg ;
               }
            }
         }
         pQueryMsg->version = cataSel.getCataPtr()->getVersion() ;

         rcTmp = coordOperator::doOpOnCL( cataSel, objQuery, inMsg,
                                          sendOpt, cb, result ) ;
      }while( FALSE ) ;

      if ( SDB_OK != _processRet )
      {
         rc = _processRet ;
         PD_LOG( PDERROR, "Query failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         if ( _pContext )
         {
            _pContext->addSubDone( cb ) ;
         }
         goto done ;
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
         PD_LOG( PDERROR, "Query failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         msgReleaseBuffer( pNewMsg, cb ) ;
      }
      if ( pModifyMsg )
      {
         msgReleaseBuffer( pModifyMsg, cb ) ;
      }
      if ( pSucGrpLst )
      {
         *pSucGrpLst = result._sucGroupLst ;
      }
      sendOpt._pIgnoreRC = pOldIgnoreRC ;

      _pContext = NULL ;
      _processRet = SDB_OK ;
      return rc ;
   error:
      if ( SDB_CAT_NO_MATCH_CATALOG == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      if ( -1 != contextID  )
      {
         pRtncb->contextDelete( contextID, cb ) ;
         contextID = -1 ;
         *pContext = NULL ;
      }
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                   cb, &nokRC ) ) ;
      }
      goto done ;
   }

   INT32 _coordQueryOperator::_buildNewMsg( const CHAR *msg,
                                            const BSONObj *newSelector,
                                            const BSONObj *newHint,
                                            CHAR *&newMsg,
                                            INT32 &buffSize,
                                            IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      CHAR *pCollectionName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      CHAR *pQuery = NULL ;
      CHAR *pFieldSelector = NULL ;
      CHAR *pOrderBy = NULL ;
      CHAR *pHint = NULL ;
      BSONObj query ;
      BSONObj selector ;
      BSONObj orderBy ;
      BSONObj hint ;
      MsgOpQuery *pSrc = (MsgOpQuery *)msg ;

      SDB_ASSERT( newSelector || newHint, "Selector or hint is NULL" ) ;

      rc = msgExtractQuery( ( CHAR * )msg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery,
                            &pFieldSelector, &pOrderBy, &pHint );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to parse query request, rc: %d", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         selector = BSONObj( pFieldSelector ) ;
         orderBy = BSONObj( pOrderBy ) ;
         hint = BSONObj( pHint ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( newSelector )
      {
         selector = *newSelector ;
      }
      if ( newHint )
      {
         hint = *newHint ;
      }

      rc = msgBuildQueryMsg( &newMsg, &buffSize,
                             pCollectionName,
                             flag, pSrc->header.requestID,
                             numToSkip, numToReturn,
                             &query, &selector,
                             &orderBy, &hint,
                             cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build new msg, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      msgReleaseBuffer( newMsg, cb ) ;
      newMsg = NULL ;
      buffSize = 0 ;
      goto done ;
   }

}

