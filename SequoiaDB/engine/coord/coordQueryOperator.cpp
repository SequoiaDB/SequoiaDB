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
#include "coordKeyKicker.hpp"
#include "coordUtil.hpp"
#include "coordFactory.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtnTrace.hpp"
#include "ossUtil.hpp"
#include "auth.hpp"

using namespace bson;
using namespace boost;

namespace engine
{
   /*
      _coordQueryOperator implement
   */
   _coordQueryOperator::_coordQueryOperator( BOOLEAN readOnly )
   {
      _processRet = SDB_OK ;
      setReadOnly( readOnly ) ;

      _needRollback = FALSE ;
   }

   _coordQueryOperator::~_coordQueryOperator()
   {
      SDB_ASSERT( !_pContext, "Context must be NULL" ) ;
      SDB_ASSERT( 0 == _vecBlock.size(), "Block must be empty" ) ;
   }

   const CHAR* _coordQueryOperator::getName() const
   {
      return "Query" ;
   }

   BOOLEAN _coordQueryOperator::needRollback() const
   {
      return _needRollback ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__CHECKQUERYMDY, "_coordQueryOperator::_checkQueryModify" )
   INT32 _coordQueryOperator::_checkQueryModify( coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 CoordGroupSubCLMap *grpSubCl )
   {
      MsgOpQuery *queryMsg = ( MsgOpQuery* )inMsg.msg() ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__CHECKQUERYMDY ) ;

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

         // modification can only be executed in primary node
         options._primary = TRUE ;
      }

      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__CHECKQUERYMDY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__OPTIMIZE, "_coordQueryOperator::_optimize" )
   void _coordQueryOperator::_optimize( coordSendMsgIn &inMsg,
                                        coordSendOptions &options,
                                        coordProcessResult &result )
   {
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__OPTIMIZE ) ;
      if ( _pContext && 0 == result._sucGroupLst.size() )
      {
         MsgOpQuery *pQueryMsg = ( MsgOpQuery* )inMsg.msg() ;
         _pContext->optimizeReturnOptions( pQueryMsg,
                                           options._groupLst.size() ) ;
      }
      PD_TRACE_EXIT ( COORD_QUERYOPERATOR__OPTIMIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__PREPARECLOP, "_coordQueryOperator::_prepareCLOp" )
   INT32 _coordQueryOperator::_prepareCLOp( coordCataSel &cataSel,
                                            coordSendMsgIn &inMsg,
                                            coordSendOptions &options,
                                            pmdEDUCB *cb,
                                            coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__PREPARECLOP ) ;

      _optimize( inMsg, options, result ) ;

      rc = _checkQueryModify( inMsg, options, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__PREPARECLOP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__DONECLOP, "_coordQueryOperator::_doneCLOp" )
   void _coordQueryOperator::_doneCLOp( coordCataSel &cataSel,
                                        coordSendMsgIn &inMsg,
                                        coordSendOptions &options,
                                        pmdEDUCB *cb,
                                        coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__DONECLOP ) ;

      ROUTE_REPLY_MAP *pOkReply = result._pOkReply ;
      SDB_ASSERT( pOkReply, "Ok reply invalid" ) ;

      BOOLEAN takeOver = FALSE ;
      pmdEDUEvent replyEvent ;
      MsgOpReply *pReply = NULL ;
      MsgRouteID nodeID ;
      ROUTE_REPLY_MAP::iterator it = pOkReply->begin() ;
      while( it != pOkReply->end() )
      {
         takeOver = FALSE ;
         replyEvent = it->second ;
         pReply = (MsgOpReply*)replyEvent._Data ;
         nodeID.value = pReply->header.routeID.value ;

         if ( SDB_OK == pReply->flags )
         {
            if ( _pContext )
            {
               rc = _pContext->addSubContext( replyEvent, takeOver ) ;
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
            pmdEduEventRelease( replyEvent, NULL ) ;
            pReply = NULL ;
         }
         ++it ;
      }
      pOkReply->clear() ;

      PD_TRACE_EXIT ( COORD_QUERYOPERATOR__DONECLOP ) ;
   }

   void _coordQueryOperator::_clearBlock( pmdEDUCB *cb )
   {
      for ( INT32 i = 0 ; i < (INT32)_vecBlock.size() ; ++i )
      {
         cb->releaseBuff( _vecBlock[ i ] ) ;
      }
      _vecBlock.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__PREPAREMAINCLOP, "_coordQueryOperator::_prepareMainCLOp" )
   INT32 _coordQueryOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                coordSendMsgIn &inMsg,
                                                coordSendOptions &options,
                                                pmdEDUCB *cb,
                                                coordProcessResult &result )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__PREPAREMAINCLOP ) ;
      MsgOpQuery *pQueryMsg   = ( MsgOpQuery* )inMsg.msg() ;

      INT32 flags             = 0 ;
      const CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      const CHAR *pQuery      = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy    = NULL ;
      const CHAR *pHint       = NULL ;

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

      rc = msgExtractQuery( (const CHAR*)pQueryMsg, &flags, &pCollectionName,
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

         // 1. first vec
         ioItem.iovBase = (CHAR*)inMsg.msg() + sizeof( MsgHeader ) ;
         ioItem.iovLen = ossRoundUpToMultipleX ( offsetof(MsgOpQuery, name) +
                         pQueryMsg->nameLength + 1, 4 ) - sizeof( MsgHeader ) ;
         iovec.push_back( ioItem ) ;

         // 2. new query vec
         newQuery = _buildNewQuery( objQuery, subCLLst ) ;
         // 2.1 add to buff
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

         // 3. last vec
         ioItem.iovBase = objSelector.objdata() ;
         ioItem.iovLen = ossRoundUpToMultipleX( objSelector.objsize(), 4 ) +
                         ossRoundUpToMultipleX( objOrderby.objsize(), 4 ) +
                         objHint.objsize() ;
         iovec.push_back( ioItem ) ;

         ++it ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__PREPAREMAINCLOP, rc ) ;
      return rc ;
   error:
      _clearBlock( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__DONEMAINCLOP, "_coordQueryOperator::_doneMainCLOp" )
   void _coordQueryOperator::_doneMainCLOp( coordCataSel &cataSel,
                                            coordSendMsgIn &inMsg,
                                            coordSendOptions &options,
                                            pmdEDUCB *cb,
                                            coordProcessResult &result )
   {
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__DONEMAINCLOP ) ;
      _clearBlock( cb ) ;
      inMsg._datas.clear() ;

      _doneCLOp( cataSel, inMsg, options, cb, result ) ;
      PD_TRACE_EXIT ( COORD_QUERYOPERATOR__DONEMAINCLOP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__BUILDNEWQUERY, "_coordQueryOperator::_buildNewQuery" )
   BSONObj _coordQueryOperator::_buildNewQuery( const BSONObj &query,
                                                const CoordSubCLlist &subCLList )
   {
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__BUILDNEWQUERY ) ;
      BSONObj retObj ;
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

      retObj = builder.obj() ;

      PD_TRACE_EXIT ( COORD_QUERYOPERATOR__BUILDNEWQUERY ) ;

      return retObj ;
   }

   // return <isUpdate, isRemove>
   std::pair< BOOLEAN, BOOLEAN > _isUpdateOrRemove( const BSONObj &hint, INT32 flags )
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
            return make_pair< BOOLEAN, BOOLEAN >( FALSE, FALSE ) ;
         }

         modifier = modifierEle.Obj() ;

         BSONElement updatorEle = modifier.getField( FIELD_NAME_OP ) ;
         if ( String != updatorEle.type() )
         {
            return make_pair< BOOLEAN, BOOLEAN >( FALSE, FALSE ) ;
         }
         if ( 0 == ossStrcmp( updatorEle.valuestr(), FIELD_OP_VALUE_UPDATE ) )
         {
            return make_pair< BOOLEAN, BOOLEAN >( TRUE, FALSE ) ;
         }
         else if ( 0 == ossStrcmp( updatorEle.valuestr(), FIELD_OP_VALUE_REMOVE ) )
         {
            return make_pair< BOOLEAN, BOOLEAN >( FALSE, TRUE ) ;
         }
      }

      return make_pair< BOOLEAN, BOOLEAN >( FALSE, FALSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR_EXE, "_coordQueryOperator::execute" )
   INT32 _coordQueryOperator::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR_EXE ) ;
      rtnContextCoord::sharePtr pContext ;
      coordCommandFactory *pFactory    = NULL ;
      coordOperator *pOperator         = NULL ;

      // fill default-reply(query success)
      contextID                        = -1 ;

      const CHAR *pCollectionName      = NULL ;
      INT32 flag                       = 0 ;
      INT64 numToSkip                  = 0 ;
      INT64 numToReturn                = 0 ;
      const CHAR *pQuery               = NULL ;
      const CHAR *pSelector            = NULL ;
      const CHAR *pOrderby             = NULL ;
      const CHAR *pHint                = NULL ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCollectionName,
                            &numToSkip, &numToReturn, &pQuery, &pSelector,
                            &pOrderby, &pHint ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to parse query request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      try
      {

         BSONObj hint = BSONObj (pHint) ;
         BSONObj clientInfo ;

         if ( cb->getMonQueryCB() && !hint.getField("$" FIELD_NAME_CLIENTINFO).eoo() )
         {
            rc = rtnGetObjElement( hint, "$" FIELD_NAME_CLIENTINFO, clientInfo ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                         "$" FIELD_NAME_CLIENTINFO, rc ) ;
            cb->getMonQueryCB()->clientInfo = clientInfo.getOwned() ;
         }

         // process command
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
               if ( SDB_COORD_UNKNOWN_OP_REQ != rc )
               {
                  PD_LOG( PDERROR, "Execute operator[%s] failed, rc: %d",
                          pOperator->getName(), rc ) ;
               }
               goto error ;
            }
         }
         else
         {
            coordSendOptions sendOpt( cb->isTransaction() ) ;
            rtnQueryOptions options ( BSONObj( pQuery ), BSONObj( pSelector ),
                                      BSONObj( pOrderby ), BSONObj( pHint ),
                                      pCollectionName, numToSkip, numToReturn,
                                      flag ) ;
            BSONElement ePos = hint.getField( FIELD_NAME_POSITION ) ;
            BSONElement eRange = hint.getField( FIELD_NAME_RANGE ) ;

            if ( !ePos.eoo() && !eRange.eoo() )
            {
               PD_LOG( PDERROR, "Field[%s] and Field[%s] cannot be specified at the "
                       "same time", FIELD_NAME_POSITION, FIELD_NAME_RANGE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( !ePos.eoo() && Object != ePos.type() )
            {
               PD_LOG( PDERROR, "Field[%s] is invalid", FIELD_NAME_POSITION ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( !eRange.eoo() && Object != eRange.type() )
            {
               PD_LOG( PDERROR, "Field[%s] is invalid", FIELD_NAME_RANGE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            // add last op info
            MON_SAVE_OP_OPTION( cb->getMonAppCB(), pMsg, options ) ;

            MONQUERY_SET_QUERY_TEXT( cb, cb->getMonAppCB()->getLastOpDetail() ) ;

            if ( cb->getSession()->privilegeCheckEnabled() )
            {
               authActionSet actions;
               actions.addAction( ACTION_TYPE_find );
               std::pair< BOOLEAN, BOOLEAN > res = _isUpdateOrRemove( BSONObj( pHint ), flag );
               if ( res.first )
               {
                  actions.addAction( ACTION_TYPE_update );
               }
               else if ( res.second )
               {
                  actions.addAction( ACTION_TYPE_remove );
               }
               rc = cb->getSession()->checkPrivilegesForActionsOnExact( pCollectionName, actions );
               PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
            }

            if ( OSS_BIT_TEST( flag, FLG_QUERY_MODIFY ) )
            {
               _needRollback = TRUE ;
               setReadOnly( FALSE ) ;
            }

            rc = queryOrDoOnCL( pMsg, cb, &pContext, sendOpt, NULL, buf ) ;
            /// AUDIT
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

            if ( OSS_BIT_TEST( flag, FLG_QUERY_MODIFY ) )
            {
               pContext->setModify( TRUE ) ;
            }
            if ( cb->isAutoCommitTrans() )
            {
               cb->setCurAutoTransCtxID( contextID ) ;
            }

            if ( OSS_BIT_TEST(flag, FLG_QUERY_PREPARE_MORE ) &&
                 !OSS_BIT_TEST(flag, FLG_QUERY_MODIFY ) )
            {
               pContext->setPrepareMoreData( TRUE ) ;
            }

            if ( Object == ePos.type() )
            {
               rc = pContext->locate( ePos.embeddedObject(), cb ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Do context locate failed, rc: %d", rc ) ;
                  goto error ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                      "Query failed, received unexpected error: %s",
                      e.what() ) ;
      }

   done:
      if ( pOperator )
      {
         pFactory->release( pOperator ) ;
      }
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR_EXE, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID  )
      {
         sdbGetRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
         pContext.release() ;
      }
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

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__GENNEWHINT, "_coordQueryOperator::_generateNewHint" )
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
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__GENNEWHINT ) ;
      coordKeyKicker kicker ;

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

      /// Init kicker
      kicker.bind( _pResource, cataInfo ) ;

      rc = kicker.kickKey( updator, newUpdator, isChanged, cb, matcher,
                           keepShardingKey ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Kick key for updator failed, rc: %d", rc ) ;
         goto error ;
      }
      isEmpty = newUpdator.isEmpty() ? TRUE : FALSE ;

      /// Builder new hint
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
                  /// new updator is empty, the whole $Modify will be removed
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
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__GENNEWHINT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordQueryOperator::queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *pContext,
                                             coordSendOptions & sendOpt,
                                             coordQueryConf *pQueryConf,
                                             rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, cb, pContext, sendOpt,
                             NULL, pQueryConf, buf ) ;
   }

   INT32 _coordQueryOperator::queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *pContext,
                                             coordSendOptions &sendOpt,
                                             CoordGroupList &sucGrpLst,
                                             coordQueryConf *pQueryConf,
                                             rtnContextBuf *buf )
   {
      return _queryOrDoOnCL( pMsg, cb, pContext, sendOpt,
                             &sucGrpLst, pQueryConf, buf ) ;
   }

   void _coordQueryOperator::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      if ( _canPrepareTrans( cb, pMsg ) )
      {
         pMsg->opCode = MSG_BS_TRANS_QUERY_REQ ;
      }
   }

   BOOLEAN _coordQueryOperator::_canPrepareTrans( pmdEDUCB *cb,
                                                  const MsgHeader *pMsg ) const
   {
      const MsgOpQuery *pQueryMsg = ( const MsgOpQuery* )pMsg ;
      if ( '$' != pQueryMsg->name[0] )
      {
         return TRUE ;
      }
      else if ( 0 == ossStrcmp( CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                                pQueryMsg->name ) )
      {
         // special case for "$get count"
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _coordQueryOperator::canPrepareTrans( pmdEDUCB *cb,
                                                 const MsgHeader *pMsg ) const
   {
      return _canPrepareTrans( cb, pMsg ) ;
   }

   BOOLEAN _coordQueryOperator::_isTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      if ( MSG_BS_TRANS_QUERY_REQ != GET_REQUEST_TYPE(pMsg->opCode) )
      {
         return FALSE ;
      }
      return cb->isTransaction() ;
   }

   void _coordQueryOperator::_onNodeReply( INT32 processType,
                                           MsgOpReply *pReply,
                                           pmdEDUCB *cb,
                                           coordSendMsgIn &inMsg )
   {
      /// do nothing
   }

   BOOLEAN _coordQueryOperator::_canPushDownAutoCommit( coordSendMsgIn &inMsg,
                                                        coordSendOptions &options,
                                                        pmdEDUCB *cb ) const
   {
      /// push down when not update
      MsgOpQuery *pQuery = ( MsgOpQuery* )inMsg.msg() ;
      if ( OSS_BIT_TEST( pQuery->flags, FLG_QUERY_MODIFY ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__QUERYORDOONCL, "_coordQueryOperator::_queryOrDoOnCL" )
   INT32 _coordQueryOperator::_queryOrDoOnCL( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord::sharePtr *pContext,
                                              coordSendOptions &sendOpt,
                                              CoordGroupList *pSucGrpLst,
                                              coordQueryConf *pQueryConf,
                                              rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__QUERYORDOONCL ) ;
      INT32 rcTmp = SDB_OK ;
      INT64 contextID = -1 ;

      coordCataSel cataSel ;
      const CHAR *pRealCLName = NULL ;
      BOOLEAN openEmptyContext = FALSE ;
      BOOLEAN updateCata = FALSE ;
      BOOLEAN allCataGroup = FALSE ;
      BOOLEAN preRead = TRUE ;

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
         preRead = pQueryConf->_preRead ;
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
      INT32 clientVer         = pQueryMsg->version;
      SDB_RTNCB *pRtncb       = pmdGetKRCB()->getRTNCB() ;
      CHAR *pNewMsg           = NULL ;
      INT32 newMsgSize        = 0 ;
      const CHAR* pLastMsg    = NULL ;
      CHAR *pModifyMsg        = NULL ;
      INT32 modifyMsgSize     = 0 ;

      BOOLEAN isUpdate        = FALSE ;
      INT32 flags             = 0 ;
      const CHAR *pCollectionName   = NULL ;
      INT64 numToSkip         = 0 ;
      INT64 numToReturn       = -1 ;
      const CHAR *pQuery      = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy    = NULL ;
      const CHAR *pHint       = NULL ;

      BSONObj objQuery ;
      BSONObj objSelector ;
      BSONObj objOrderby ;
      BSONObj objHint ;
      BSONObj objNewHint ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flags, &pCollectionName,
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
         if ( NULL == pContext->get() )
         {
            rtnContextCoord::sharePtr newContext ;
            RTN_CONTEXT_TYPE contextType = RTN_CONTEXT_COORD ;

            if ( FLG_QUERY_EXPLAIN & pQueryMsg->flags )
            {
               contextType = RTN_CONTEXT_COORD_EXP ;
            }

            // create context
            rc = pRtncb->contextNew( contextType,
                                     newContext,
                                     contextID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate context(rc=%d)",
                         rc ) ;

            *pContext = newContext ;
         }
         else
         {
            contextID = (*pContext)->contextID() ;
            // the context is create in out side, do nothing
         }
         _pContext = *pContext ;
      }

      if ( _pContext && !_pContext->isOpened() )
      {
         if ( openEmptyContext )
         {
            rtnQueryOptions defaultOptions ;
            rc = _pContext->open( defaultOptions, preRead ) ;
         }
         else
         {
            BOOLEAN needResetSubQuery = FALSE ;
            BSONObj mergedSelect ;
            rtnQueryOptions options ;

            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               needResetSubQuery = TRUE ;
            }
            else
            {
               rtnGetMergedSelector( objSelector, objOrderby,
                                     needResetSubQuery, &mergedSelect ) ;
            }

            // build new selector
            if ( needResetSubQuery )
            {
               rc = _buildNewMsg( (const CHAR*)pMsg, &mergedSelect, NULL,
                                  pNewMsg, newMsgSize, cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to build new msg, rc: %d", rc ) ;
                  goto error ;
               }
               pQueryMsg = (MsgOpQuery *)pNewMsg ;
               inMsg._pMsg = ( MsgHeader* )pNewMsg ;
            }

            OSS_BIT_CLEAR( pQueryMsg->flags, FLG_FORCE_INDEX_SELECTOR ) ;
            options.setCLFullName( pCollectionName ) ;
            options.setQuery( objQuery ) ;
            options.setOrderBy( objOrderby ) ;
            options.setHint( objHint ) ;
            options.setSkip( pQueryMsg->numToSkip ) ;
            options.setLimit( pQueryMsg->numToReturn ) ;
            options.resetFlag( pQueryMsg->flags ) ;

            // The explain will reset the selector itself for sub-context
            if ( OSS_BIT_TEST( pQueryMsg->flags, FLG_QUERY_EXPLAIN ) ||
                 needResetSubQuery )
            {
               options.setSelector( objSelector ) ;
            }
            else
            {
               options.setSelector( BSONObj() ) ;
            }

            // when not in transaction, query and modify can't use preRead
            if ( !cb->isTransaction() &&
                 ( FLG_QUERY_MODIFY & pQueryMsg->flags ) )
            {
               preRead = FALSE ;
            }
            // open context
            rc = _pContext->open( options, preRead ) ;

            // change some data
            if ( pQueryMsg->numToReturn > 0 && pQueryMsg->numToSkip > 0 )
            {
               // some record may skip on coord,
               // so the num of records from data-node must
               // more than "numToReturn + numToSkip"
               pQueryMsg->numToReturn += pQueryMsg->numToSkip ;
            }
            pQueryMsg->numToSkip = 0 ;

            if ( FLG_QUERY_STRINGOUT & pQueryMsg->flags )
            {
               _pContext->getSelector().setStringOutput( TRUE ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

         // sample timetamp
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

      //e.g. objHint = {"$Modify":{"OP":"update", "Update":{"$set":{a:1}} } }
      isUpdate = _isUpdate( objHint, pQueryMsg->flags ) ;
      /// save the last msg
      pLastMsg = ( const CHAR* )pQueryMsg ;

   retry:
      do
      {
         /// restore the last msg
         pQueryMsg = ( MsgOpQuery* )pLastMsg ;
         inMsg._pMsg = ( MsgHeader* )pLastMsg ;

         // if DQL not command should check version
         if ( pCollectionName && CMD_ADMIN_PREFIX[0] != pCollectionName[0] )
         {
            rc = checkCatVersion( cb, pCollectionName, clientVer, cataSel ) ;
            PD_CHECK( SDB_OK == rc, rc, error, PDWARNING,
                      "check cat version failed, rc: %d", rc ) ;
         }

         if ( isUpdate && ( cataSel.getCataPtr()->isSharded() ||
                            cataSel.getCataPtr()->hasAutoIncrement() ) )
         {
            //kick shardingKey
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

         rcTmp = doOpOnCL( cataSel, objQuery, inMsg, sendOpt, cb, result ) ;
      }while( FALSE ) ;

      if ( SDB_OK != _processRet )
      {
         rc = _processRet ;
         PD_LOG( PDERROR, "Query failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         if ( _pContext && preRead )
         {
            _pContext->addSubDone( cb ) ;
         }

         goto done ;
      }
      else if ( SDB_RTN_QUERYMODIFY_MULTI_NODES == rcTmp &&
                !cataSel.hasUpdated() )
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

      if( buf && ( SDB_CLIENT_CATA_VER_OLD == rc ) )
      {
         buf->setStartFrom( cataSel.getCataPtr()->getVersion() ) ;
      }

      /// reset info
      _pContext.release() ;
      _processRet = SDB_OK ;
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__QUERYORDOONCL, rc ) ;
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
         pContext->release() ;
      }
      if ( buf && ( nokRC.size() > 0 || SDB_CLIENT_CATA_VER_OLD == rc ) )
      {
         BSONObjBuilder retBuilder ;
         utilWriteResult wrResult ;
         coordBuildErrorObj( _pResource, rc, cb, &nokRC, retBuilder,
                             result._sucGroupLst.size() ) ;
         coordSetResultInfo( rc, nokRC, &wrResult ) ;
         wrResult.toBSON( retBuilder ) ;

         *buf = rtnContextBuf( retBuilder.obj() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_QUERYOPERATOR__BUILDNEWMSG, "_coordQueryOperator::_buildNewMsg" )
   INT32 _coordQueryOperator::_buildNewMsg( const CHAR *msg,
                                            const BSONObj *newSelector,
                                            const BSONObj *newHint,
                                            CHAR *&newMsg,
                                            INT32 &buffSize,
                                            IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_QUERYOPERATOR__BUILDNEWMSG ) ;
      INT32 flag = 0 ;
      const CHAR *pCollectionName = NULL ;
      SINT64 numToSkip = 0 ;
      SINT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;
      BSONObj query ;
      BSONObj selector ;
      BSONObj orderBy ;
      BSONObj hint ;
      MsgOpQuery *pSrc = (MsgOpQuery *)msg ;

      SDB_ASSERT( newSelector || newHint, "Selector or hint is NULL" ) ;

      rc = msgExtractQuery( ( const CHAR * )msg, &flag, &pCollectionName,
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
      PD_TRACE_EXITRC ( COORD_QUERYOPERATOR__BUILDNEWMSG, rc ) ;
   error:
      msgReleaseBuffer( newMsg, cb ) ;
      newMsg = NULL ;
      buffSize = 0 ;
      goto done ;
   }

}

