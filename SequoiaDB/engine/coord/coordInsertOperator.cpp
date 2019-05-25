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

using namespace bson ;

namespace engine
{

   /*
      _coordInsertOperator implement
   */
   _coordInsertOperator::_coordInsertOperator()
   {
      _insertedNum = 0 ;
      _ignoredNum = 0 ;

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

   UINT32 _coordInsertOperator::getInsertedNum() const
   {
      return _insertedNum ;
   }

   UINT32 _coordInsertOperator::getIgnoredNum() const
   {
      return _ignoredNum ;
   }

   void _coordInsertOperator::clearStat()
   {
      _insertedNum = 0 ;
      _ignoredNum = 0 ;
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

      coordSendOptions sendOpt( TRUE ) ;
      sendOpt._useSpecialGrp = TRUE ;

      coordSendMsgIn inMsg( pMsg ) ;
      coordProcessResult result ;
      ROUTE_RC_MAP nokRC ;
      result._pNokRC = &nokRC ;

      coordCataSel cataSel ;
      MsgRouteID errNodeID ;

      MsgOpInsert *pInsertMsg          = (MsgOpInsert *)pMsg ;
      INT32 oldFlag                    = pInsertMsg->flags ;
      pInsertMsg->flags               |= FLG_INSERT_RETURNNUM ;
      contextID                        = -1 ;

      INT32 flag = 0 ;
      CHAR *pCollectionName = NULL ;
      CHAR *pInsertor = NULL;
      INT32 count = 0 ;
      rc = msgExtractInsert( (CHAR*)pMsg, &flag,
                             &pCollectionName, &pInsertor, count ) ;
      if( rc )
      {
         PD_LOG( PDERROR, "Failed to parse insert request, rc: %d", rc ) ;
         pCollectionName = NULL ;
         goto error ;
      }

      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Collection:%s, Insertors:%s, ObjNum:%d, "
                          "Flag:0x%08x(%u)",
                          pCollectionName,
                          BSONObj(pInsertor).toString().c_str(),
                          count, oldFlag, oldFlag ) ;

      rc = cataSel.bind( _pResource, pCollectionName, cb, FALSE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get or update collection[%s]'s catalog info "
                 "failed, rc: %d", pCollectionName, rc ) ;
         goto error ;
      }

   retry:
      pInsertMsg->version = cataSel.getCataPtr()->getVersion() ;
      pInsertMsg->w = 0 ;
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
      else
      {
         PD_LOG( PDERROR, "Insert failed on node[%s], rc: %d",
                 routeID2String( errNodeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      if ( pCollectionName )
      {
         PD_AUDIT_OP( AUDIT_DML, MSG_BS_INSERT_REQ, AUDIT_OBJ_CL,
                      pCollectionName, rc, "InsertedNum:%u, IgnoredNum:%u, "
                      "ObjNum:%u, Insertor:%s, Flag:0x%08x(%u)", _insertedNum,
                      _ignoredNum, count, BSONObj(pInsertor).toString().c_str(),
                      oldFlag, oldFlag ) ;
      }
      if ( oldFlag & FLG_INSERT_RETURNNUM )
      {
         contextID = ossPack32To64( _insertedNum, _ignoredNum ) ;
      }
      PD_TRACE_EXITRC ( COORD_INSERTOPR_EXE, rc ) ;
      return rc ;
   error:
      if ( buf && nokRC.size() > 0 )
      {
         *buf = rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                   cb, &nokRC ) ) ;
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

      options._groupLst.clear() ;

      if ( !cataSel.getCataPtr()->isSharded() )
      {
         cataSel.getCataPtr()->getGroupLst( options._groupLst ) ;
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
      else
      {
         rc = reshardData( cataSel.getCataPtr(), fixed, inMsg._datas ) ;
         PD_RC_CHECK( rc, PDERROR, "Re-shard data failed, rc: %d", rc ) ;

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
      if ( inMsg._datas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            inMsg._datas.erase( it->second ) ;
            ++it ;
         }
      }

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
      if ( pReply->contextID > 0 )
      {
         UINT32 hi1 = 0, lo1 = 0 ;
         ossUnpack32From64( pReply->contextID, hi1, lo1 ) ;
         _insertedNum += hi1 ;
         _ignoredNum += lo1 ;
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

         {
            netIOVec &iovec = datas[ groupID ] ;
            UINT32 size = iovec.size() ;
            if( size > 0 )
            {
               if ( (const CHAR*)( iovec[size-1].iovBase ) +
                    iovec[size-1].iovLen == pInsertor )
               {
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

      inMsg._datas.clear() ;

      rc = buildInsertMsg( fixed, _grpSubCLDatas, _vecObject, inMsg._datas ) ;
      PD_RC_CHECK( rc, PDERROR, "Build insert msg failed, rc: %d" ) ;

      options._groupLst.clear() ;
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
      if ( _grpSubCLDatas.size() > 0 )
      {
         CoordGroupList::iterator it = result._sucGroupLst.begin() ;
         while( it != result._sucGroupLst.end() )
         {
            _grpSubCLDatas.erase( it->second ) ;
            ++it ;
         }
      }

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

}

