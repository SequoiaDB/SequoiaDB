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

   Source File Name = coordInsertOperator.cpp

   Descriptive Name = Coord Insert

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   insert options on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordInsertOperator.hpp"
#include "coordUtil.hpp"
#include "msgMessage.hpp"
#include "msgMessageFormat.hpp"
#include "rtnCommandDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordSequenceAgent.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{
   /*
      this is a simplized bson builder, it can build bson on exist
      buffer, but never check the exist buffer size. please check
      the buffer size before use it.
   */
   class _coordInsertOperator::_SimpleBSONBuilder
   {
      /*
         BSONObj struct:
         |length(UINT32)   |BSONElements(...)   |EOO(CHAR)  |

         BSONElement struct:
         |type(CHAR) |fieldName(CHAR*) |value(...) |
         value length is decided by type. if NumberLong, it's INT64.
      */
      public:
         _SimpleBSONBuilder( CHAR *pBuf )
         {
            _pBuf    = pBuf ;
            _pCur    = _pBuf ;
            _ppPos   = NULL ;
            _hasDone = FALSE ;

            init() ;
         }

         _SimpleBSONBuilder( CHAR **ppPos )
         {
            _pBuf    = *ppPos ;
            _pCur    = _pBuf ;
            _ppPos   = ppPos ;
            _hasDone = FALSE ;

            init() ;
         }

         UINT32 len() const { return _pCur - _pBuf ; }

         const CHAR* done()
         {
            if ( !_hasDone )
            {
               _hasDone = TRUE ;
               *_pCur = (CHAR) EOO ;
               ++_pCur ;
               /// set size
               *((UINT32*)_pBuf) = _pCur - _pBuf ;
               /// set pos
               if ( _ppPos )
               {
                  *_ppPos = _pCur ;
               }
            }
            return _pBuf ;
         }

         _SimpleBSONBuilder* appendElement( BSONElement &ele )
         {
            ossMemcpy( _pCur, ele.rawdata(), ele.size() ) ;
            _pCur += ele.size() ;
            return this ;
         }

         _SimpleBSONBuilder* append( const CHAR *fieldName, INT64 value )
         {
            *_pCur = (CHAR) NumberLong ;
            _pCur += 1 ;

            const INT32 fieldLen = ossStrlen( fieldName ) + 1 ;
            ossMemcpy( _pCur, fieldName, fieldLen ) ;
            _pCur += fieldLen ;

            *((INT64*)_pCur) = value ;
            _pCur += 8 ;
            return this ;
         }

         CHAR** subobjStart( const CHAR *fieldName )
         {
            *_pCur = (CHAR) Object ;
            _pCur += 1 ;

            const INT32 fieldLen = ossStrlen( fieldName ) + 1 ;
            ossMemcpy( _pCur, fieldName, fieldLen ) ;
            _pCur += fieldLen ;

            return &_pCur ;
         }

      protected:
         void init()
         {
            *((UINT32*)_pBuf) = 0 ;
            _pCur = _pBuf + 4 ;
         }

      private:
         CHAR*       _pBuf ;
         CHAR*       _pCur ;
         CHAR**      _ppPos ;
         BOOLEAN     _hasDone ;
   };

   /*
      _coordInsertOperator implement
   */
   _coordInsertOperator::_coordInsertOperator()
   {
      _hasRetry = FALSE ;

      _hasGenerated = FALSE ;
      _lastGenerateID = 0 ;

      const static string s_insertStr("Insert" ) ;
      setName( s_insertStr ) ;
   }

   _coordInsertOperator::~_coordInsertOperator()
   {
   }

   BOOLEAN _coordInsertOperator::isReadOnly() const
   {
      return FALSE ;
   }

   UINT64 _coordInsertOperator::getInsertedNum() const
   {
      return _inResult.insertedNum() ;
   }

   UINT64 _coordInsertOperator::getDuplicatedNum() const
   {
      return _inResult.duplicatedNum() ;
   }

   void _coordInsertOperator::clearStat()
   {
      _inResult.reset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR_EXE, "_coordInsertOperator::execute" )
   INT32 _coordInsertOperator::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_INSERTOPR_EXE ) ;

      // process define
      coordSendOptions sendOpt( TRUE ) ;
      sendOpt._useSpecialGrp = TRUE ;

      coordSendMsgIn inMsg( pMsg ) ;
      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      coordCataSel cataSel ;
      MsgRouteID errNodeID ;
      CoordCataInfoPtr cataPtr ;

      // for autoIncrement
      CHAR *pNewMsg = NULL ;
      INT32 newMsgSize = 0 ;
      INT32 newMsgLen = 0 ;
      INT32 orgMsgLen = 0 ;
      MsgOpInsert *pTmpInsertMsg = NULL ;
      const clsAutoIncSet *pAutoIncSet = NULL ;
      clsAutoIncIDSet curAutoIncIDs ;
      clsAutoIncIDSet lastAutoIncIDs ;
      BOOLEAN hasExplicitKey = FALSE ;
      BOOLEAN needNewAutoInc = FALSE ;

      BSONObjBuilder retBuilder ;

      // fill default-reply(insert success)
      MsgOpInsert *pInsertMsg          = (MsgOpInsert *)pMsg ;
      INT32 oldFlag                    = pInsertMsg->flags ;
      pInsertMsg->flags               |= FLG_INSERT_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pInsertor = NULL;
      INT32 count = 0 ;
      rtnQueryOptions options ;

      rc = msgExtractInsert( (CHAR*)pMsg, &flag,
                             &pCollectionName, &pInsertor, count ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to parse insert request, rc: %d", rc ) ;
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

      options.setCLFullName( pCollectionName ) ;
      options.setInsertor( BSONObj( pInsertor ) ) ;

      // add list op info
      MON_SAVE_OP_OPTION( cb->getMonAppCB(), pMsg->opCode, options ) ;

      MONQUERY_SET_QUERY_TEXT( cb, cb->getMonAppCB()->getLastOpDetail() ) ;

      rc = cataSel.bind( _pResource, pCollectionName, cb, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pCollectionName, rc ) ;
         goto error ;
      }
      orgMsgLen = pMsg->messageLength ;

   retry:
      cataPtr = cataSel.getCataPtr() ;
      if ( cataPtr->hasAutoIncrement() )
      {
         pAutoIncSet = &cataPtr->getAutoIncSet() ;
         curAutoIncIDs = pAutoIncSet->getIDs() ;

         /*
            when retry, only in two conditions we need to generate key again.
            1. auto-increment attributes are altered;
            2. auto-increment key from system is a dupplicate key.
          */
         if ( 0 == result._sucGroupLst.size() &&
              ( curAutoIncIDs != lastAutoIncIDs || needNewAutoInc ) )
         {
            _hasGenerated = FALSE ;
            needNewAutoInc = FALSE ;
            // in case of reshard, clear the last shard result.
            cataPtr->isMainCL() ? _grpSubCLDatas.clear() : inMsg.data()->clear() ;

            hasExplicitKey = FALSE ;
            rc = _addAutoIncToMsg( *pAutoIncSet, pInsertMsg, pInsertor,
                                   count, orgMsgLen, cb,
                                   &pNewMsg, newMsgSize, newMsgLen,
                                   hasExplicitKey ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to add autoIncrement fields to msg, rc: %d",
                         rc ) ;
            inMsg._pMsg = (MsgHeader*)pNewMsg ;
            lastAutoIncIDs = curAutoIncIDs ;
         }
      }
      else
      {
         inMsg._pMsg = (MsgHeader*)pInsertMsg ;
         _hasGenerated = FALSE ;
      }

      pTmpInsertMsg = (MsgOpInsert*) inMsg._pMsg ;
      pTmpInsertMsg->version = cataPtr->getVersion() ;
      pTmpInsertMsg->w = 0 ;

      /// Do on collection
      rcTmp = doOpOnCL( cataSel, BSONObj(), inMsg, sendOpt, cb, result ) ;
      if ( SDB_OK == rcTmp && nokRC.empty() )
      {
         goto done ;
      }
      else if ( checkRetryForCLOpr( rcTmp, &nokRC, cataSel, inMsg.msg(),
                                    cb, rc, &errNodeID, TRUE ) )
      {
         nokRC.clear() ;
         _groupSession.getGroupCtrl()->incRetry() ;
         goto retry ;
      }
      else if ( cataPtr->hasAutoIncrement() &&
                SDB_OK == cb->getTransRC() &&
                _canRetry( count, nokRC, cataPtr->getAutoIncSet(),
                           hasExplicitKey ) )
      {
         nokRC.clear() ;
         _removeLocalSeqCache( cataPtr->getAutoIncSet() ) ;
         needNewAutoInc = TRUE ;
         goto retry ;
      }
      else
      {
         PD_LOG( PDERROR, "Insert failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      /// AUDIT
      if ( pCollectionName )
      {
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_INSERT_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc, "InsertedNum:%llu, "
                      "DuplicatedNum:%llu, ObjNum:%u, Insertor:%s, "
                      "Flag:0x%08x(%u)",
                      _inResult.insertedNum(), _inResult.duplicatedNum(),
                      count, BSONObj(pInsertor).toPoolString().c_str(),
                      oldFlag, oldFlag ) ;
      }

      if ( buf )
      {
         if ( ( oldFlag & FLG_INSERT_RETURNNUM ) || rc )
         {
            _inResult.toBSON( retBuilder ) ;
         }

         if ( _hasGenerated )
         {
            retBuilder.append( FIELD_NAME_LAST_GENERATE_ID, _lastGenerateID ) ;
         }

         if ( !retBuilder.isEmpty() )
         {
            *buf = rtnContextBuf( retBuilder.obj() ) ;
         }
      }
      msgReleaseBuffer( pNewMsg, cb ) ;
      PD_TRACE_EXITRC ( COORD_INSERTOPR_EXE, rc ) ;
      return rc ;
   error:
      if ( buf && ( nokRC.size() > 0 || rc ) )
      {
         coordBuildErrorObj( _pResource, rc, cb, &nokRC, retBuilder ) ;
         coordSetResultInfo( rc, nokRC, &_inResult ) ;
      }
      goto done;
   }

   INT32 _coordInsertOperator::_prepareCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      MsgOpInsert *pInsertMsg = ( MsgOpInsert* )inMsg.msg() ;
      netIOV fixed( ( CHAR*)inMsg.msg() + sizeof( MsgHeader ),
                    ossRoundUpToMultipleX ( offsetof(MsgOpInsert, name) +
                                            pInsertMsg->nameLength + 1, 4 ) -
                    sizeof( MsgHeader ) ) ;

      // clear send groups
      options._groupLst.clear() ;

      if ( !cataSel.getCataPtr()->isSharded() )
      {
         // get group
         cataSel.getCataPtr()->getGroupLst( options._groupLst ) ;
         // don't change the msg
         goto done ;
      }
      else if ( inMsg.data()->size() == 0 )
      {
         INT32 flag = 0 ;
         CHAR *pCollectionName = NULL ;
         CHAR *pInsertor = NULL ;
         INT32 count = 0 ;

         rc = msgExtractInsert( (CHAR *)inMsg.msg(), &flag, &pCollectionName,
                                &pInsertor, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Extrace insert msg failed, rc: %d",
                      rc ) ;

         rc = shardDataByGroup( cataSel.getCataPtr(), count, pInsertor,
                                fixed, inMsg._datas ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shard data by group, rc: %d",
                      rc ) ;

         // only one group, send by normal
         if ( 1 == inMsg._datas.size() )
         {
            UINT32 groupID = inMsg._datas.begin()->first ;
            options._groupLst[ groupID ] = groupID ;
            inMsg._datas.clear() ;
         }
         else
         {
            GROUP_2_IOVEC::iterator it = inMsg._datas.begin() ;
            while( it != inMsg._datas.end() )
            {
               options._groupLst[ it->first ] = it->first ;
               ++it ;
            }
         }
      }
      // reshard
      else
      {
         rc = reshardData( cataSel.getCataPtr(), fixed, inMsg._datas ) ;
         PD_RC_CHECK( rc, PDERROR, "Re-shard data failed, rc: %d", rc ) ;

         // build groups
         {
            GROUP_2_IOVEC::iterator it = inMsg._datas.begin() ;
            while( it != inMsg._datas.end() )
            {
               options._groupLst[ it->first ] = it->first ;
               ++it ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordInsertOperator::_doneCLOp( coordCataSel &cataSel,
                                         coordSendMsgIn &inMsg,
                                         coordSendOptions &options,
                                         pmdEDUCB *cb,
                                         coordProcessResult &result )
   {
      // remove the datas by succeed group
      if ( inMsg._datas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            inMsg._datas.erase( it->second ) ;
            ++it ;
         }
      }

      // clear all succeed group
      result._sucGroupLst.clear() ;
   }

   void _coordInsertOperator::_prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg )
   {
      pMsg->opCode = MSG_BS_TRANS_INSERT_REQ ;
   }

   void _coordInsertOperator::_onNodeReply( INT32 processType,
                                            MsgOpReply *pReply,
                                            pmdEDUCB *cb,
                                            coordSendMsgIn &inMsg )
   {
      if ( pReply->header.messageLength > (INT32)sizeof( MsgOpReply ) &&
           1 == pReply->numReturned )
      {
         BOOLEAN insertedProcessed = FALSE ;
         BOOLEAN duplicateProcessed = FALSE ;

         try
         {
            BSONObj objResult( ( const CHAR* )pReply + sizeof( MsgOpReply ) ) ;
            BSONObjIterator itr( objResult ) ;
            while ( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( !insertedProcessed &&
                    0 == ossStrcmp( e.fieldName(), FIELD_NAME_INSERT_NUM ) )
               {
                  insertedProcessed = TRUE ;
                  _inResult.incInsertedNum( (UINT64)e.numberLong() ) ;
               }
               else if ( !duplicateProcessed &&
                         0 == ossStrcmp( e.fieldName(),
                                         FIELD_NAME_DUPLICATE_NUM ) )
               {
                  duplicateProcessed = TRUE ;
                  _inResult.incDuplicatedNum( (UINT64)e.numberLong() ) ;
               }

               if ( insertedProcessed && duplicateProcessed )
               {
                  break ;
               }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDWARNING, "Extract insert result exception: %s",
                    e.what() ) ;
         }
      }
      else if ( pReply->contextID > 0 )
      {
         UINT32 hi = 0, lo = 0 ;
         /// (UINT32)insertedNum + (UINT32)ignoredNum
         ossUnpack32From64( pReply->contextID, hi, lo ) ;
         _inResult.incInsertedNum( hi ) ;
         _inResult.incDuplicatedNum( lo ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR_SHARDANOBJ, "_coordInsertOperator::shardAnObj" )
   INT32 _coordInsertOperator::shardAnObj( const CHAR *pInsertor,
                                           CoordCataInfoPtr &cataInfo,
                                           const netIOV &fixed,
                                           GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_INSERTOPR_SHARDANOBJ ) ;

      try
      {
         BSONObj insertObj( pInsertor ) ;
         UINT32 roundLen = ossRoundUpToMultipleX( insertObj.objsize(), 4 ) ;
         UINT32 groupID = 0 ;

         rc = cataInfo->getGroupByRecord( insertObj, groupID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get the groupid for obj[%s] from "
                    "catalog info[%s], rc: %d", insertObj.toString().c_str(),
                    cataInfo->toBSON().toString().c_str(), rc ) ;
            goto error ;
         }

         // add 2 group
         {
            netIOVec &iovec = datas[ groupID ] ;
            UINT32 size = iovec.size() ;
            if( size > 0 )
            {
               if ( (const CHAR*)( iovec[size-1].iovBase ) +
                    iovec[size-1].iovLen == pInsertor )
               {
                  // only change the length
                  iovec[size-1].iovLen += roundLen ;
               }
               else
               {
                  iovec.push_back( netIOV( pInsertor, roundLen ) ) ;
               }
            }
            else
            {
               iovec.push_back( fixed ) ;
               iovec.push_back( netIOV( pInsertor, roundLen ) ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to shard the data, received unexpected "
                 "error: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_INSERTOPR_SHARDANOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR_SHARDBYGROUP, "_coordInsertOperator::shardDataByGroup" )
   INT32 _coordInsertOperator::shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                                 INT32 count,
                                                 const CHAR *pInsertor,
                                                 const netIOV &fixed,
                                                 GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_INSERTOPR_SHARDBYGROUP ) ;

      while ( count > 0 )
      {
         rc = shardAnObj( pInsertor, cataInfo, fixed, datas ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to shard the obj, rc: %d", rc ) ;
            goto error ;
         }

         try
         {
            BSONObj boInsertor( pInsertor ) ;
            pInsertor += ossRoundUpToMultipleX( boInsertor.objsize(), 4 ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Parse insert object occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         --count ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_INSERTOPR_SHARDBYGROUP, rc ) ;
      return rc ;
   error:
      datas.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR_RESHARD, "_coordInsertOperator::reshardData" )
   INT32 _coordInsertOperator::reshardData( CoordCataInfoPtr &cataInfo,
                                            const netIOV &fixed,
                                            GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_INSERTOPR_RESHARD ) ;

      const CHAR *pData = NULL ;
      UINT32 offset = 0 ;
      UINT32 roundSize = 0 ;

      GROUP_2_IOVEC newDatas ;
      GROUP_2_IOVEC::iterator it = datas.begin() ;
      while ( it != datas.end() )
      {
         netIOVec &iovec = it->second ;
         UINT32 size = iovec.size() ;
         // skip the first
         for ( UINT32 i = 1 ; i < size ; ++i )
         {
            netIOV &ioItem = iovec[ i ] ;
            pData = ( const CHAR* )ioItem.iovBase ;
            offset = 0 ;

            while( offset < ioItem.iovLen )
            {
               try
               {
                  BSONObj obInsert( pData ) ;
                  roundSize = ossRoundUpToMultipleX( obInsert.objsize(), 4 ) ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDERROR, "Parse the insert object occur "
                          "exception: %s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               rc = shardAnObj( pData, cataInfo, fixed, newDatas ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Reshard the insert record failed, rc: %d",
                          rc ) ;
                  goto error ;
               }

               pData += roundSize ;
               offset += roundSize ;
            }
         }
         ++it ;
      }
      datas = newDatas ;

   done:
      PD_TRACE_EXITRC ( COORD_INSERTOPR_RESHARD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordInsertOperator::_prepareMainCLOp( coordCataSel &cataSel,
                                                 coordSendMsgIn &inMsg,
                                                 coordSendOptions &options,
                                                 pmdEDUCB *cb,
                                                 coordProcessResult &result )
   {
      INT32 rc = SDB_OK ;
      GROUP_2_IOVEC::iterator it ;

      MsgOpInsert *pInsertMsg = ( MsgOpInsert* )inMsg.msg() ;
      netIOV fixed( ( CHAR*)inMsg.msg() + sizeof( MsgHeader ),
                    ossRoundUpToMultipleX ( offsetof(MsgOpInsert, name) +
                                            pInsertMsg->nameLength + 1, 4 ) -
                    sizeof( MsgHeader ) ) ;

      if ( _grpSubCLDatas.size() == 0 )
      {
         INT32 flag = 0 ;
         CHAR *pCollectionName = NULL ;
         CHAR *pInsertor = NULL ;
         INT32 count = 0 ;

         rc = msgExtractInsert( (CHAR *)inMsg.msg(), &flag, &pCollectionName,
                                &pInsertor, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Extrace insert msg failed, rc: %d",
                      rc ) ;

         rc = shardDataByGroup( cataSel.getCataPtr(), count, pInsertor,
                                cb, _grpSubCLDatas ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to shard data by group, rc: %d",
                      rc ) ;
      }
      else
      {
         rc = reshardData( cataSel.getCataPtr(), cb, _grpSubCLDatas ) ;
         PD_RC_CHECK( rc, PDERROR, "Re-shard data failed, rc: %d", rc ) ;
      }

      // build msg
      inMsg._datas.clear() ;

      rc = buildInsertMsg( fixed, _grpSubCLDatas, _vecObject, inMsg._datas ) ;
      PD_RC_CHECK( rc, PDERROR, "Build insert msg failed, rc: %d", rc ) ;

      // clear send groups
      options._groupLst.clear() ;
      // build group list
      it = inMsg._datas.begin() ;
      while( it != inMsg._datas.end() )
      {
         options._groupLst[ it->first ] = it->first ;
         ++it ;
      }

   done:
      return rc ;
   error:
      _vecObject.clear() ;
      goto done ;
   }

   void _coordInsertOperator::_doneMainCLOp( coordCataSel &cataSel,
                                             coordSendMsgIn &inMsg,
                                             coordSendOptions &options,
                                             pmdEDUCB *cb,
                                             coordProcessResult &result )
   {
      // remove the datas by succeed group
      if ( _grpSubCLDatas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            _grpSubCLDatas.erase( it->second ) ;
            ++it ;
         }
      }

      // clear all succeed group
      result._sucGroupLst.clear() ;

      _vecObject.clear() ;
   }

   INT32 _coordInsertOperator::shardAnObj( const CHAR *pInsertor,
                                           CoordCataInfoPtr &cataInfo,
                                           pmdEDUCB * cb,
                                           GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK ;
      string subCLName ;
      UINT32 groupID = CAT_INVALID_GROUPID ;

      try
      {
         BSONObj insertObj( pInsertor ) ;
         CoordCataInfoPtr subClCataInfo ;
         UINT32 roundLen = ossRoundUpToMultipleX( insertObj.objsize(), 4 ) ;

         rc = cataInfo->getSubCLNameByRecord( insertObj, subCLName ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Couldn't find the match[%s] sub-collection "
                    "in cl's(%s) catalog info[%s], rc: %d",
                    insertObj.toString().c_str(), cataInfo->getName(),
                    cataInfo->toBSON().toString().c_str(),
                    rc ) ;
            goto error ;
         }

         /// get sub-collection's catalog info
         rc = _pResource->getOrUpdateCataInfo( subCLName.c_str(),
                                               subClCataInfo, cb ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Get sub-collection[%s]'s catalog info "
                    "failed, rc: %d", subCLName.c_str(), rc ) ;
            goto error ;
         }

         rc = subClCataInfo->getGroupByRecord( insertObj, groupID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Couldn't find the match[%s] catalog of "
                    "sub-collection(%s), rc: %d",
                    insertObj.toString().c_str(),
                    subClCataInfo->toBSON().toString().c_str(),
                    rc ) ;
            goto error ;
         }

         /// add to group
         (groupSubCLMap[ groupID ])[ subCLName ].push_back(
            netIOV( (const void*)pInsertor, roundLen ) ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse the insert object occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordInsertOperator::shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                                 INT32 count,
                                                 const CHAR *pInsertor,
                                                 pmdEDUCB *cb,
                                                 GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK ;

      while ( count > 0 )
      {
         rc = shardAnObj( pInsertor, cataInfo, cb, groupSubCLMap ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to shard the object, rc: %d", rc ) ;
            goto error ;
         }

         try
         {
            BSONObj boInsertor( pInsertor ) ;
            pInsertor += ossRoundUpToMultipleX( boInsertor.objsize(), 4 ) ;
            --count ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Parse the insert reocrd occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      return rc;
   error:
      groupSubCLMap.clear() ;
      goto done ;
   }

   INT32 _coordInsertOperator::reshardData( CoordCataInfoPtr &cataInfo,
                                            pmdEDUCB *cb,
                                            GroupSubCLMap &groupSubCLMap )
   {
      INT32 rc = SDB_OK ;
      GroupSubCLMap groupSubCLMapNew ;

      GroupSubCLMap::iterator iterGroup = groupSubCLMap.begin() ;
      while ( iterGroup != groupSubCLMap.end() )
      {
         SubCLObjsMap::iterator iterCL = iterGroup->second.begin() ;
         while( iterCL != iterGroup->second.end() )
         {
            netIOVec &iovec = iterCL->second ;
            UINT32 size = iovec.size() ;

            for ( UINT32 i = 0 ; i < size ; ++i )
            {
               netIOV &ioItem = iovec[ i ] ;
               rc = shardAnObj( (const CHAR*)ioItem.iovBase, cataInfo,
                                cb, groupSubCLMapNew ) ;
               if ( rc )
               {
                  PD_LOG( PDWARNING, "Failed to reshard the object, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
            ++iterCL ;
         }
         ++iterGroup ;
      }
      groupSubCLMap = groupSubCLMapNew ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordInsertOperator::buildInsertMsg( const netIOV &fixed,
                                               GroupSubCLMap &groupSubCLMap,
                                               vector< BSONObj > &subClInfoLst,
                                               GROUP_2_IOVEC &datas )
   {
      INT32 rc = SDB_OK ;
      static CHAR _fillData[ 8 ] = { 0 } ;

      GroupSubCLMap::iterator iterGroup = groupSubCLMap.begin() ;
      while ( iterGroup != groupSubCLMap.end() )
      {
         UINT32 groupID = iterGroup->first ;
         netIOVec &iovec = datas[ groupID ] ;
         iovec.push_back( fixed ) ;

         SubCLObjsMap &subCLDataMap = iterGroup->second ;
         SubCLObjsMap::iterator iterCL = subCLDataMap.begin() ;
         while ( iterCL != iterGroup->second.end() )
         {
            netIOVec &subCLIOVec = iterCL->second ;
            UINT32 dataLen = netCalcIOVecSize( subCLIOVec ) ;
            UINT32 objNum = subCLIOVec.size() ;

            // first for sub cl info
            BSONObjBuilder subCLInfoBuild ;
            subCLInfoBuild.append( FIELD_NAME_SUBOBJSNUM, (INT32)objNum ) ;
            subCLInfoBuild.append( FIELD_NAME_SUBOBJSSIZE, (INT32)dataLen ) ;
            subCLInfoBuild.append( FIELD_NAME_SUBCLNAME, iterCL->first ) ;
            BSONObj subCLInfoObj = subCLInfoBuild.obj() ;
            subClInfoLst.push_back( subCLInfoObj ) ;
            netIOV ioCLInfo ;
            ioCLInfo.iovBase = (const void*)subCLInfoObj.objdata() ;
            ioCLInfo.iovLen = subCLInfoObj.objsize() ;
            iovec.push_back( ioCLInfo ) ;

            // need fill
            UINT32 infoRoundSize = ossRoundUpToMultipleX( ioCLInfo.iovLen,
                                                          4 ) ;
            if ( infoRoundSize > ioCLInfo.iovLen )
            {
               iovec.push_back( netIOV( (const void*)_fillData,
                                infoRoundSize - ioCLInfo.iovLen ) ) ;
            }

            for ( UINT32 i = 0 ; i < objNum ; ++i )
            {
               iovec.push_back( subCLIOVec[ i ] ) ;
            }
            ++iterCL ;
         }
         ++iterGroup ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR__ADD_AUTOINC_TO_MSG, "_coordInsertOperator::_addAutoIncToMsg" )
   INT32 _coordInsertOperator::_addAutoIncToMsg( const clsAutoIncSet &autoIncSet,
                                                 MsgOpInsert *pInsertMsg,
                                                 CHAR const *pInsertor,
                                                 const INT32 count,
                                                 INT32 orgMsgLen,
                                                 pmdEDUCB *cb,
                                                 CHAR **ppNewMsg,
                                                 INT32 &newMsgSize,
                                                 INT32 &newMsgLen,
                                                 BOOLEAN &hasExplicitKey )
   {
      PD_TRACE_ENTRY( COORD_INSERTOPR__ADD_AUTOINC_TO_MSG ) ;

      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      CHAR* pCurPos = NULL ;
      UINT32 estimatedSize = 0 ;
      UINT32 headerSize = 0 ;

      // 1.malloc a msg buffer which is big enough
      estimatedSize = orgMsgLen + count * ( autoIncSet.getEleSize() + 4 ) ;
      newMsgLen = 0 ;
      rc = msgCheckBuffer( ppNewMsg, &newMsgSize, estimatedSize, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to malloc buffer, rc: %d", rc ) ;

      // 2.copy the header of original msg
      headerSize = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name) +
                                          pInsertMsg->nameLength + 1,
                                          4 ) ;
      ossMemcpy( *ppNewMsg, (CHAR*) pInsertMsg, headerSize ) ;
      pCurPos = *ppNewMsg + headerSize ;

      // 3.build new bson objs with auto-increment field
      for ( i = 0 ; i < count ; ++i )
      {
         const BSONObj objIn( (const CHAR*)pInsertor ) ;
         _SimpleBSONBuilder builder( pCurPos ) ;
         rc = _addAutoIncToObj( objIn, autoIncSet, cb, builder, hasExplicitKey ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add autoIncrement field to obj[%s], rc: %d",
                      objIn.toString().c_str(), rc ) ;
         builder.done() ;

         pCurPos += ossRoundUpToMultipleX( builder.len(), 4 ) ;
         pInsertor += ossRoundUpToMultipleX( objIn.objsize(), 4 ) ;
      }

      newMsgLen = pCurPos - (*ppNewMsg) ;
      SDB_ASSERT( newMsgLen <= newMsgSize, "message is over boundary" ) ;
      ((MsgHeader*)(*ppNewMsg))->messageLength = newMsgLen ;

   done:
      PD_TRACE_EXITRC( COORD_INSERTOPR__ADD_AUTOINC_TO_MSG, rc ) ;
      return rc ;
   error:
      newMsgLen = 0 ;
      goto done ;
   }

   template <typename T>
   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR__ADD_AUTOINC_TO_OBJ, "_coordInsertOperator::_addAutoIncToObj" )
   INT32 _coordInsertOperator::_addAutoIncToObj( const BSONObj &objIn,
                                                 const T &set,
                                                 pmdEDUCB *cb,
                                                 _SimpleBSONBuilder &builder,
                                                 BOOLEAN &hasExplicitKey )
   {
      PD_TRACE_ENTRY( COORD_INSERTOPR__ADD_AUTOINC_TO_OBJ ) ;
      typedef ossPoolSet<_utilMapStringKey>    StringKeySet ;

      INT32                      rc = SDB_OK ;
      BSONElement                ele ;
      const clsAutoIncItem       *pItem = NULL ;
      StringKeySet               doneSet ;

      // 1. Handle autoIncrement fields inputted by user.
      BSONObjIterator boIt( objIn ) ;
      while( boIt.more() )
      {
         ele = boIt.next() ;
         pItem = set.findItem( ele.fieldName() ) ;
         if ( NULL == pItem )
         {
            builder.appendElement( ele ) ;
            continue ;
         }

         UINT32 oldLen = builder.len() ;
         rc = _processUserInput( pItem, ele, cb, builder, hasExplicitKey ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to process user entered field, rc: %d", rc ) ;

         if ( builder.len() > oldLen ) {
            doneSet.insert( ele.fieldName() ) ;
         }

         if ( doneSet.size() == set.itemCount() )
         {
            break ;
         }
      }

      while( boIt.more() )
      {
         ele = boIt.next() ;
         builder.appendElement( ele ) ;
      }

      // 2. Complete the rest of autoIncrement fields.
      {
         clsAutoIncIterator autoIncIt( set );
         while ( autoIncIt.more() )
         {
            /// already exist
            pItem = autoIncIt.next();
            if ( doneSet.find( pItem->fieldName() ) != doneSet.end() )
            {
               continue;
            }

            rc = _appendAutoIncField( pItem, cb, builder );
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to append auto-increment field[%s], rc: %d",
                         pItem->fieldName(), rc );
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_INSERTOPR__ADD_AUTOINC_TO_OBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR__PROCESS_USER_INPUT, "_coordInsertOperator::_processUserInput" )
   INT32 _coordInsertOperator::_processUserInput( const clsAutoIncItem *pItem,
                                                  BSONElement &ele,
                                                  pmdEDUCB *cb,
                                                  _SimpleBSONBuilder &builder,
                                                  BOOLEAN &hasExplicitKey )
   {
      INT32 rc = SDB_OK ;
      const CHAR *eleField = ele.fieldName() ;
      PD_TRACE_ENTRY( COORD_INSERTOPR__PROCESS_USER_INPUT ) ;

      if ( !pItem->hasSubField() )
      {
         BOOLEAN isNumber = FALSE ;

         if ( AUTOINC_GEN_ALWAYS == pItem->generatedType() )
         {
            goto done ;
         }

         isNumber = (NumberInt == ele.type() || NumberLong == ele.type()) ;
         if ( AUTOINC_GEN_STRICT == pItem->generatedType() )
         {
            PD_CHECK( isNumber, SDB_INVALIDARG, error, PDERROR,
                      "Wrong type[%d] of autoIncrement field[%s]",
                      ele.type(), eleField ) ;
         }

         if ( isNumber )
         {
            coordSequenceAgent *pSequenceAgent = _pResource->getSequenceAgent() ;
            rc = pSequenceAgent->adjustNextValue( pItem->sequenceName(),
                                                  pItem->sequenceID(),
                                                  ele.numberLong(),
                                                  cb ) ;
            if ( SDB_SEQUENCE_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to adjust sequence[%s] next value, rc:%d",
                       pItem->sequenceName(), rc ) ;
               goto error ;
            }
         }

         builder.appendElement( ele ) ;
         hasExplicitKey = TRUE ;
      }
      else
      {
         if ( Object == ele.type() )
         {
            _SimpleBSONBuilder subBuilder( builder.subobjStart( eleField ) ) ;
            rc = _addAutoIncToObj( ele.embeddedObject(), *pItem,
                                   cb, subBuilder, hasExplicitKey ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add autoIncrement field[%s], "
                         "rc: %d", eleField, rc ) ;
            subBuilder.done();
         }
         else
         {
            // autoIncrement field is "a.b",
            // and user just input field "a".
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Field[%s] conflicted with autoIncrement field",
                         eleField );
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_INSERTOPR__PROCESS_USER_INPUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR__APPEND_AUTOINC_FLD, "_coordInsertOperator::_appendAutoIncField" )
   INT32 _coordInsertOperator::_appendAutoIncField( const clsAutoIncItem *pItem,
                                                    pmdEDUCB *cb,
                                                    _SimpleBSONBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_INSERTOPR__APPEND_AUTOINC_FLD ) ;

      if ( !pItem->hasSubField() )
      {
         coordSequenceAgent *pSequenceAgent = _pResource->getSequenceAgent() ;
         INT64 nextValue = 0 ;

         rc = pSequenceAgent->getNextValue( pItem->sequenceName(),
                                            pItem->sequenceID(),
                                            nextValue, cb ) ;
         if ( SDB_SEQUENCE_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get sequence[%s] next value, rc: %d",
                    pItem->sequenceName(), rc ) ;
            goto error ;
         }

         if ( !_hasGenerated )
         {
            _lastGenerateID = nextValue ;
            _hasGenerated = TRUE ;
         }
         builder.append( pItem->fieldName(), nextValue ) ;
      }
      else
      {
         _SimpleBSONBuilder subBuilder(
               builder.subobjStart( pItem->fieldName() ) ) ;

         BOOLEAN hasExplicitKey = FALSE ; // it must be FALSE here, used to compile
         rc = _addAutoIncToObj( BSONObj(), *pItem, cb,
                                subBuilder, hasExplicitKey ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add autoIncrement "
                      "field[%s], rc: %d", pItem->fieldName(), rc ) ;
         subBuilder.done() ;
      }

   done:
      PD_TRACE_EXITRC( COORD_INSERTOPR__APPEND_AUTOINC_FLD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordInsertOperator::_canRetry( INT32 count,
                                            ROUTE_RC_MAP &failedNodes,
                                            const clsAutoIncSet &set,
                                            BOOLEAN hasExplicitKey )
   {
      BOOLEAN retry = FALSE ;
      ROUTE_RC_MAP::iterator iter ;

      if ( count > 1 || hasExplicitKey || _hasRetry )
      {
         goto done ;
      }

      for ( iter = failedNodes.begin() ;
            iter != failedNodes.end() ;
            ++iter )
      {
         if ( iter->second._rc != SDB_IXM_DUP_KEY )
         {
            continue ;
         }

         BSONElement ele = iter->second._obj.getField( FIELD_NAME_INDEX ) ;
         if ( ele.type() != Object )
         {
            PD_LOG( PDWARNING,
                    "Cannot recognize which index was conflicted." ) ;
            break ;
         }

         BSONObjIterator iter( ele.embeddedObject() ) ;
         while ( iter.more() )
         {
            try
            {
               if ( set.find( iter.next().fieldName() ) )
               {
                  _hasRetry = TRUE ;
                  retry = TRUE ;
                  break ;
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDWARNING,
                       "Unexpected exception occurred: %e", e.what() ) ;
               break ;
            }
         }

         if ( retry )
         {
            break ;
         }
      }
    done:
      return retry ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INSERTOPR__REMOVE_LOCAL_SEQ_CACHE, "_coordInsertOperator::_removeLocalSeqCache" )
   void _coordInsertOperator::_removeLocalSeqCache( const clsAutoIncSet &set )
   {
      PD_TRACE_ENTRY( COORD_INSERTOPR__REMOVE_LOCAL_SEQ_CACHE ) ;

      coordSequenceAgent *pSequenceAgent = _pResource->getSequenceAgent() ;
      clsAutoIncIterator iter( set, clsAutoIncIterator::RECURS ) ;
      while ( iter.more() )
      {
         const clsAutoIncItem *pItem = iter.next() ;
         pSequenceAgent->removeCache( pItem->sequenceName(),
                                      pItem->sequenceID() ) ;
      }

      PD_TRACE_EXIT( COORD_INSERTOPR__REMOVE_LOCAL_SEQ_CACHE ) ;
   }

}

