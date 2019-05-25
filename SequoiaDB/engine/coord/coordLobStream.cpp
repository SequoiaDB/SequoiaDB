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

   Source File Name = coordLobStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordLobStream.hpp"
#include "coordUtil.hpp"
#include "pmdEDU.hpp"
#include "msgMessage.hpp"
#include "rtnContextBuff.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   #define LOB_MAX_RETRYTIMES                ( 5 )

   _coordLobStream::_coordLobStream( coordResource *pResource, INT64 timeout )
   :_metaGroup( 0 ),
    _alignBuf( 0 ),
    _pResource( pResource ),
    _timeout( timeout ),
    _emptyPageBuf( NULL ),
    _mainStreamOpened( FALSE )
   {
      _pageSize = 0 ;
      SDB_ASSERT( _pResource, "Resource can't be NULL" ) ;
   }

   _coordLobStream::~_coordLobStream()
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      _closeSubStreamsWithException( cb ) ;
      _clearMsgData() ;
      _releaseEmptyPageBuf() ;
   }

   void _coordLobStream::getErrorInfo( INT32 rc,
                                       _pmdEDUCB *cb,
                                       _rtnContextBuf *buf )
   {
      if ( rc && buf && _nokRC.size() > 0 )
      {
         *buf = coordBuildErrorObj( _pResource, rc, cb, &_nokRC ) ;
      }
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_PREPARE, "_coordLobStream::_prepare" )
   INT32 _coordLobStream::_prepare( const CHAR *fullName,
                                    const bson::OID &oid,
                                    INT32 mode,
                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_PREPARE ) ;

      rc = _groupSession.init( _pResource, cb, _timeout,
                               &_remoteHandler, &_groupHandler ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init group session failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = _updateCataInfo( FALSE, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to update catalog info of:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

      _groupSession.getGroupSel()->setPrimary( ( SDB_LOB_MODE_READ != mode ) ?
                                               TRUE : FALSE ) ;
      _groupSession.getGroupCtrl()->setMaxRetryTimes( LOB_MAX_RETRYTIMES ) ;

      rc = _openSubStreams( fullName, oid, mode, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open sub streams:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_UPDATECATAINFO, "_coordLobStream::_openSubStreams" )
   INT32 _coordLobStream::_updateCataInfo( BOOLEAN refresh,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_UPDATECATAINFO ) ;

      if ( !refresh )
      {
         rc = _pResource->getCataInfo( getFullName(), _cataInfo ) ;
      }
      if ( rc || refresh )
      {
         rc = _pResource->updateCataInfo( getFullName(), _cataInfo, cb ) ;
         if ( rc )
         {
            if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
            {
               rc = SDB_DMS_NOTEXIST ;
            }
            PD_LOG( PDERROR, "Update collection[%s] catalog info failed, "
                    "rc: %d", getFullName(), rc ) ;
            goto error ;
         }
      }

      if ( _cataInfo->isMainCL() )
      {
         PD_LOG( PDERROR, "can not open a lob in main cl" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( _cataInfo->isRangeSharded() )
      {
         PD_LOG( PDERROR, "can not open a lob in range sharded cl" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         rc = _cataInfo->getLobGroupID( getOID(),
                                        DMS_LOB_META_SEQUENCE,
                                        _metaGroup ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get meta group:%d", rc ) ;
            goto error ;
         }
         rc = _pResource->getOrUpdateGroupInfo( _metaGroup, _metaGroupInfo,
                                                cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get meta group info[%u] failed, rc: %d",
                    _metaGroup, rc ) ;
            goto error ;
         }
         _mapGroupInfo[ _metaGroup ] = _metaGroupInfo ;
      }
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_UPDATECATAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_OPENSUBSTREAMS, "_coordLobStream::_openSubStreams" )
   INT32 _coordLobStream::_openSubStreams( const CHAR *fullName,
                                           const bson::OID &oid,
                                           INT32 mode,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_OPENSUBSTREAMS ) ;
      rc = _openMainStream( fullName, oid, mode, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open main stream:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_OPENSUBSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_OPENOTHERSTREAMS, "_coordLobStream::_openOtherStreams" )
   INT32 _coordLobStream::_openOtherStreams( const CHAR *fullName,
                                             const bson::OID &oid,
                                             INT32 mode,
                                             _pmdEDUCB *cb,
                                             UINT32 *pSpecGroupID )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_OPENOTHERSTREAMS ) ;

      MsgOpLob header ;
      CoordGroupList gpLst ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      netIOVec iov ;
      CoordGroupMap::iterator itMap ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      coordGroupSel *pSel = _groupSession.getGroupSel() ;

      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, FALSE ) ;
      if ( SDB_LOB_MODE_READ == mode )
      {
         builder.append( FIELD_NAME_LOB_META_DATA, _metaObj ) ;
      }

      obj = builder.obj() ;

      _initHeader( header,
                   MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      _pushLobHeader( &header, obj, iov ) ;

      do
      {
         INT32 tag = RETRY_TAG_NULL ;
         _clearMsgData() ;
         CoordGroupList sendGrpLst ;

         if ( !pSpecGroupID )
         {
            _cataInfo->getGroupLst( gpLst ) ;
            SDB_ASSERT( 1 == gpLst.count( _metaGroup ), "impossible" ) ;
         }
         else
         {
            gpLst[ *pSpecGroupID ] = *pSpecGroupID ;
         }

         rc = coordGroupList2GroupPtr( _pResource, cb, gpLst,
                                       _mapGroupInfo, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get groups info failed, rc: %d", rc ) ;
            goto error ;
         }

         header.version = _cataInfo->getVersion() ;

         for ( CoordGroupList::const_iterator itr = gpLst.begin();
               itr != gpLst.end();
               ++itr )
         {
            if ( 0 < _subs.count( itr->first ) )
            {
               continue ;
            }

            itMap = _mapGroupInfo.find( itr->first ) ;
            if ( _mapGroupInfo.end() == itMap )
            {
               SDB_ASSERT( FALSE, "Group info is not exist" ) ;
               rc = SDB_COOR_NO_NODEGROUP_INFO ;
               goto error ;
            }

            pSel->addGroupPtr2Map( itMap->second ) ;

            sendGrpLst[ itr->first ] = itr->second ;
         }

         rc = _groupSession.sendMsg( (MsgHeader*)&header,
                                     sendGrpLst,
                                     &iov ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to send open msg to groups, rc: %d",
                    rc ) ;
            goto error ;
         }

         rc = _getReply( cb, FALSE, tag ) ;
         pCtrl->incRetry() ;
         rcTmp = _addSubStreamsFromReply() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg:%d", rc ) ;
            goto error ;
         }
         if ( SDB_OK != rcTmp )
         {
            rc = rcTmp ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else if ( RETRY_TAG_REOPEN & tag )
         {
            rc = _closeSubStreams( cb, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to close sub streams: %d", rc ) ;
               goto error ;
            }
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ; 
      PD_TRACE_EXITRC( COORD_LOBSTREAM_OPENOTHERSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_OPENMAINSTREAM, "_coordLobStream::_openMainStream" )
   INT32 _coordLobStream::_openMainStream( const CHAR *fullName,
                                           const bson::OID &oid,
                                           INT32 mode,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_OPENMAINSTREAM ) ;

      MsgOpLob header ;
      const MsgOpReply *reply = NULL ;
      BSONObj obj ;
      BSONObjBuilder builder ;
      netIOVec iov ;

      _rtnLobDataPool::tuple dataTuple ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      coordGroupSel *pSel = _groupSession.getGroupSel() ;

      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, TRUE ) ;
      if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         builder.append( FIELD_NAME_LOB_CREATETIME, (INT64)_getMeta()._createTime ) ;
      }
      if ( _mainStreamOpened )
      {
         builder.appendBool( FIELD_NAME_LOB_REOPENED, TRUE ) ;
         if ( SDB_LOB_MODE_WRITE == _getMode() &&
              !_getLockSections().isEmpty() )
         {
            BSONArray array ;
            rc = _getLockSections().saveTo( array ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to save LobSections, rc=%d", rc ) ;
               goto error ;
            }

            builder.appendArray( FIELD_NAME_LOB_LOCK_SECTIONS, array ) ;
         }
      }
      obj = builder.obj() ;

      _initHeader( header, MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      _pushLobHeader( &header, obj, iov ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         pSel->addGroupPtr2Map( _metaGroupInfo ) ;

         rc = _groupSession.sendMsg( (MsgHeader*)&header,
                                     _metaGroup,
                                     &iov,
                                     NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to send open msg to group[%d], rc: %d",
                    _metaGroup, rc ) ;
            goto error ;
         }

         rc = _getReply( cb, FALSE, tag ) ;
         pCtrl->incRetry() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }
         else if ( RETRY_TAG_NULL == tag )
         {
            SDB_ASSERT( 1 == _results.size(), "impossible" ) ;
            reply = _results.empty() ? NULL : *( _results.begin() ) ;
            break ;
         }
      } while ( TRUE ) ;

      _add2Subs( reply->header.routeID.columns.groupID,
                 reply->contextID, reply->header.routeID ) ;

      if ( !_mainStreamOpened )
      {
         rc = _extractMeta( reply, _metaObj, dataTuple ) ;
         if ( dataTuple.data && dataTuple.len > 0 )
         {
            rc = _getPool().push( dataTuple.data,
                                  dataTuple.len,
                                  dataTuple.offset ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Push data to pool failed, rc:%d", rc ) ;
               goto error ;
            }
            _getPool().pushDone() ;

            _getPool().entrust( ( CHAR * )( *_results.begin() ) ) ;
            _results.erase( _results.begin() ) ;
         }
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract meta data from reply msg:%d",
                    rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDEVENT, "Reopened main stream" ) ;
      }

      _mainStreamOpened = TRUE ;

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_OPENMAINSTREAM, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_GETSUBSTREAM, "_coordLobStream::_getSubStream" )
   INT32 _coordLobStream::_getSubStream( UINT32 groupID, const subStream** sub, _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_GETSUBSTREAM ) ;

      SDB_ASSERT( NULL != sub, "sub is null" ) ;

      SUB_STREAMS::iterator itr = _subs.find( groupID ) ;
      if ( _subs.end() == itr )
      {
         if ( groupID == _metaGroup )
         {
            rc = _openMainStream( getFullName(), getOID(),
                                  _getMode(), cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to open main stream in group[%d], "
                       "rc: %d", groupID, rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = _openOtherStreams( getFullName(), getOID(),
                                    _getMode(), cb, &groupID ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "failed to open other stream in group[%d], "
                       "rc: %d", groupID, rc ) ;
               goto error ;
            }
         }
         itr = _subs.find( groupID ) ;
         if ( _subs.end() == itr )
         {
            PD_LOG( PDERROR, "group:%d is not in sub streams", groupID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      *sub = &( itr->second ) ;

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_GETSUBSTREAM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_QUERYLOBMETA, "_coordLobStream::_queryLobMeta" )
   INT32 _coordLobStream::_queryLobMeta( _pmdEDUCB *cb,
                                         _dmsLobMeta &meta,
                                         BOOLEAN allowUncompleted,
                                         _rtnLobPiecesInfo* piecesInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_QUERYLOBMETA ) ;
      (void)allowUncompleted ;

      try
      {
         BSONElement ele = _metaObj.getField( FIELD_NAME_LOB_SIZE ) ;
         if ( NumberLong != ele.type() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         meta._lobLen = ele.Long() ;

         ele = _metaObj.getField( FIELD_NAME_LOB_CREATETIME ) ;
         if ( NumberLong != ele.type() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         meta._createTime = ele.Long() ;

         ele = _metaObj.getField( FIELD_NAME_LOB_MODIFICATION_TIME ) ;
         if ( NumberLong == ele.type() )
         {
            meta._modificationTime = ele.Long() ;
         }
         else if ( !ele.eoo() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            meta._modificationTime = meta._createTime ;
         }

         ele = _metaObj.getField( FIELD_NAME_VERSION ) ;
         if ( NumberInt == ele.type() )
         {
            meta._version = (UINT8)ele.numberInt() ;
         }

         ele = _metaObj.getField( FIELD_NAME_LOB_FLAG ) ;
         if ( NumberInt == ele.type() )
         {
            meta._flag = (UINT32)ele.Int() ;
         }
         else if ( !ele.eoo() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         ele = _metaObj.getField( FIELD_NAME_LOB_PIECESINFONUM ) ;
         if ( NumberInt == ele.type() )
         {
            meta._piecesInfoNum = ele.Int() ;
         }
         else if ( !ele.eoo() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         meta._status = DMS_LOB_COMPLETE ;

         if ( NULL != piecesInfo &&
              meta.hasPiecesInfo() )
         {
            ele = _metaObj.getField( FIELD_NAME_LOB_PIECESINFO ) ;
            if ( Array == ele.type() )
            {
               BSONArray array( ele.embeddedObject() );
               rc = piecesInfo->readFrom( array ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "invalid pieces info array:%s",
                          array.toString( FALSE, TRUE ).c_str() ) ;
                  goto error ;
               }
            }
            else if ( !ele.eoo() )
            {
               PD_LOG( PDERROR, "invalid meta obj:%s",
                       _metaObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_QUERYLOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_ensureLob( _pmdEDUCB *cb,
                                      _dmsLobMeta &meta,
                                      BOOLEAN &isNew )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( SDB_LOB_MODE_CREATEONLY == _getMode(),
                  "should not hit here" ) ;
      isNew = TRUE ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_GETLOBPAGESIZE, "_coordLobStream::_getLobPageSize" )
   INT32 _coordLobStream::_getLobPageSize( INT32 &pageSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_GETLOBPAGESIZE ) ;

      if ( 0 == _pageSize )
      {
         try
         {
            BSONElement ele = _metaObj.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
            if ( NumberInt != ele.type() )
            {
               PD_LOG( PDERROR, "invalid meta obj:%s",
                       _metaObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            _pageSize = ele.Int() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      pageSize = _pageSize ;

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_GETLOBPAGESIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_WRITEOP, "_coordLobStream::_writeOp" )
   INT32 _coordLobStream::_writeOp( const _rtnLobTuple &tuple,
                                    INT32 opCode,
                                    _pmdEDUCB *cb,
                                    BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_WRITEOP ) ;

      SDB_ASSERT( MSG_BS_LOB_WRITE_REQ == opCode || MSG_BS_LOB_UPDATE_REQ == opCode,
                  "only support write or update operation" ) ;
      SDB_ASSERT( !orUpdate || MSG_BS_LOB_WRITE_REQ == opCode,
                  "only support orUpdate in write operation" ) ;

      MsgOpLob header ;
      UINT32 groupID = 0 ;
      const subStream *sub = NULL ;
      netIOVec iov ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      pCtrl->resetRetry() ;

      _initHeader( header, opCode,
                   0, -1,
                   sizeof( header ) +
                   sizeof( _MsgLobTuple ) +
                   tuple.tuple.columns.len ) ;
      _pushLobHeader( &header, BSONObj(), iov ) ;
      _pushLobData( tuple.tuple.data, sizeof( tuple.tuple ), iov ) ;
      _pushLobData( tuple.data, tuple.tuple.columns.len, iov ) ;

      if ( orUpdate && MSG_BS_LOB_WRITE_REQ == opCode )
      {
         header.flags |= FLG_LOBWRITE_OR_UPDATE ;
      }

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _cataInfo->getLobGroupID( getOID(),
                                        tuple.tuple.columns.sequence,
                                        groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         rc = _getSubStream( groupID, &sub, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get sub stream:%d", rc ) ;
            goto error ;
         }

         header.contextID = sub->contextID ;

         pSub = pSession->addSubSession( sub->id.value ) ;
         pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
         pSub->addIODatas( iov ) ;

         rc = pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                    sub->id.columns.groupID, sub->id.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _getReply( cb, TRUE, tag ) ;
         pCtrl->incRetry() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else 
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to reopen sub streams, rc: %d", rc ) ;
               goto error ;
            }
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_WRITEOP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_WRITEVOP, "_coordLobStream::_writevOp" )
   INT32 _coordLobStream::_writevOp( const RTN_LOB_TUPLES &tuples,
                                     INT32 opCode,
                                     _pmdEDUCB *cb,
                                     BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_WRITEVOP ) ;

      SDB_ASSERT( MSG_BS_LOB_WRITE_REQ == opCode || MSG_BS_LOB_UPDATE_REQ == opCode,
                  "only support write or update operation" ) ;
      SDB_ASSERT( !orUpdate || MSG_BS_LOB_WRITE_REQ == opCode,
                  "only support orUpdate in write operation" ) ;

      DONE_LST doneLst ;
      BOOLEAN reshard = TRUE ;
      MsgOpLob header ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      pCtrl->resetRetry() ;

      _initHeader( header, opCode,
                   0, -1 ) ;

      if ( orUpdate && ( MSG_BS_LOB_WRITE_REQ == opCode ) )
      {
         header.flags |= FLG_LOBWRITE_OR_UPDATE ;
      }

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         if ( reshard )
         {
            rc = _shardData( header, tuples, TRUE, doneLst, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }

            reshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;

            if ( !dg.hasData() )
            {
               continue ;
            }

            header.header.messageLength = sizeof( MsgHeader ) + dg.bodyLen ;
            header.contextID = dg.contextID ;

            pSub = pSession->addSubSession( dg.id.value ) ;
            pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
            pSub->addIODatas( dg.body ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                       dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( cb, TRUE, tag ) ;
         pCtrl->incRetry() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLstFromReply( doneLst ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reopen sub streams, rc: %d", rc ) ;
               goto error ;
            }
            reshard = TRUE ;
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_WRITEVOP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_WRITE, "_coordLobStream::_write" )
   INT32 _coordLobStream::_write( const _rtnLobTuple &tuple,
                                  _pmdEDUCB *cb, BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_WRITE ) ;
      rc = _writeOp( tuple, MSG_BS_LOB_WRITE_REQ, cb, orUpdate ) ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_WRITE, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_WRITEV, "_coordLobStream::_writev" )
   INT32 _coordLobStream::_writev( const RTN_LOB_TUPLES &tuples,
                                   _pmdEDUCB *cb, BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_WRITEV ) ;
      rc = _writevOp( tuples, MSG_BS_LOB_WRITE_REQ, cb, orUpdate ) ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_WRITEV, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_UPDATE, "_coordLobStream::_update" )
   INT32 _coordLobStream::_update( const _rtnLobTuple &tuple,
                                   _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_UPDATE ) ;
      rc = _writeOp( tuple, MSG_BS_LOB_UPDATE_REQ, cb ) ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_UPDATE, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_UPDATEV, "_coordLobStream::_updatev" )
   INT32 _coordLobStream::_updatev( const RTN_LOB_TUPLES &tuples,
                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_UPDATEV ) ;
      rc = _writevOp( tuples, MSG_BS_LOB_UPDATE_REQ, cb ) ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_UPDATEV, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM__READ, "_coordLobStream::_read" )
   INT32 _coordLobStream::_read( const _rtnLobTuple& tuple,
                                 _pmdEDUCB *cb, MsgOpReply** reply )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM__READ ) ;

      SDB_ASSERT( NULL != reply, "invalid reply" ) ;

      MsgOpLob header ;
      BOOLEAN needReshard = TRUE ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      *reply = NULL ;
      pCtrl->resetRetry() ;

      _initHeader( header, MSG_BS_LOB_READ_REQ, 0, -1 ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         if ( needReshard )
         {
            rc = _shardSingleData( header, tuple, FALSE, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard tuple:%d", rc ) ;
               goto error ;
            }
            needReshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;
            if ( !dg.hasData() )
            {
               continue ;
            }

            header.header.messageLength = sizeof( MsgHeader ) + dg.bodyLen ;
            header.contextID = dg.contextID ;

            pSub = pSession->addSubSession( dg.id.value ) ;
            pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
            pSub->addIODatas( dg.body ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                       dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( cb, FALSE, tag ) ;
         pCtrl->incRetry() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            SDB_ASSERT( 1 == _results.size(), "impossible" ) ;
            *reply = _results.empty() ? NULL : *( _results.begin() ) ;
            _results.erase( _results.begin() ) ;
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
            needReshard = TRUE ;
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM__READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_READV, "_coordLobStream::_readv" )
   INT32 _coordLobStream::_readv( const RTN_LOB_TUPLES &tuples,
                                  _pmdEDUCB *cb,
                                  const _rtnLobPiecesInfo* piecesInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_READV ) ;

      DONE_LST doneLst ; 
      BOOLEAN needReshard = TRUE ;
      MsgOpLob header ;
      RTN_LOB_TUPLES newTuples ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      if ( NULL != piecesInfo )
      {
         INT32 pageSize = 0 ;
         rc = _getLobPageSize( pageSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get page size of lob:%d", rc ) ;
            goto error ;
         }

         for ( RTN_LOB_TUPLES::const_iterator iter = tuples.begin() ;
               iter != tuples.end() ; iter++ )
         {
            const _rtnLobTuple& t = *iter ;
            if ( !piecesInfo->hasPiece( t.tuple.columns.sequence ) )
            {
               rc = _ensureEmptyPageBuf( pageSize ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
 
               rc = _getPool().push( _emptyPageBuf, t.tuple.columns.len,
                                     RTN_LOB_GET_OFFSET_OF_LOB(
                                         pageSize,
                                         t.tuple.columns.sequence,
                                         t.tuple.columns.offset,
                                         _getMeta()._version >= DMS_LOB_META_MERGE_DATA_VERSION ) ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
                  goto error ;
               }
            }
            else
            {
               newTuples.push_back( t ) ;
            }
         }

         if ( newTuples.empty() )
         {
            _getPool().pushDone() ;
            goto done ;
         }
      }

      pCtrl->resetRetry() ;

      _initHeader( header, MSG_BS_LOB_READ_REQ,
                   0, -1 ) ;
      
      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         if ( needReshard )
         {
            rc = _shardData( header, NULL == piecesInfo ? tuples : newTuples,
                             FALSE, doneLst, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }
            needReshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;
            if ( !dg.hasData() )
            {
               continue ;
            }

            header.header.messageLength = sizeof( MsgHeader ) + dg.bodyLen ;
            header.contextID = dg.contextID ;

            pSub = pSession->addSubSession( dg.id.value ) ;
            pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
            pSub->addIODatas( dg.body ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                       dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( cb, FALSE, tag ) ;
         pCtrl->incRetry() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         rc = _handleReadResults( cb, doneLst ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            _getPool().pushDone() ;
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
            needReshard = TRUE ;
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_READV, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_PUSH2POOL, "_coordLobStream::_push2Pool" )
   INT32 _coordLobStream::_push2Pool( const MsgOpReply *header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_PUSH2POOL ) ;

      const MsgLobTuple *begin = NULL ;
      UINT32 tupleSz = 0 ;
      const MsgLobTuple *curTuple = NULL ;
      const CHAR *data = NULL ;
      BOOLEAN got = FALSE ;
      INT32 pageSz = 0 ;
      rc = _getLobPageSize( pageSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get page size of lob:%d", rc ) ;
         goto error ;
      }

      rc = msgExtractReadResult( header, &begin, &tupleSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract read result:%d", rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = msgExtractTuplesAndData( &begin, &tupleSz,
                                       &curTuple, &data, &got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract tuple from msg:%d", rc ) ;
            goto error ;
         }
         else if ( got )
         {
            if ( _getMeta()._version <  DMS_LOB_META_MERGE_DATA_VERSION &&
                 0 == curTuple->columns.sequence )
            {
               PD_LOG( PDERROR, "we should not get sequence 0" ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            rc = _getPool().push( data, curTuple->columns.len,
                                  RTN_LOB_GET_OFFSET_OF_LOB(
                                         pageSz,
                                         curTuple->columns.sequence,
                                         curTuple->columns.offset,
                                         _getMeta()._version >= DMS_LOB_META_MERGE_DATA_VERSION ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            break ;
         }
      }
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_PUSH2POOL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_COMPLETELOB, "_coordLobStream::_completeLob" )
   INT32 _coordLobStream::_completeLob( const _rtnLobTuple &tuple,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_COMPLETELOB ) ;
      if ( SDB_LOB_MODE_CREATEONLY == _getMode() )
      {
         rc = _write( tuple, cb ) ;
      }
      else if ( SDB_LOB_MODE_WRITE == _getMode() ||
                SDB_LOB_MODE_TRUNCATE == _getMode() )
      {
         rc = _update( tuple, cb ) ;
      }
      PD_TRACE_EXITRC( COORD_LOBSTREAM_COMPLETELOB, rc ) ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_LOCK, "_coordLobStream::_lock" )
   INT32 _coordLobStream::_lock( _pmdEDUCB *cb,
                             INT64 offset,
                             INT64 length )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_LOCK ) ;

      MsgOpLob header ;
      UINT32 groupID = 0 ;
      const subStream *sub = NULL ;
      netIOVec iov ;
      BSONObjBuilder builder ;
      BSONObj obj ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_LOB_OFFSET, offset )
             .append( FIELD_NAME_LOB_LENGTH, length ) ;
      obj = builder.obj() ;

      _initHeader( header, MSG_BS_LOB_LOCK_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      _pushLobHeader( &header, obj, iov ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _cataInfo->getLobGroupID( getOID(),
                                        DMS_LOB_META_SEQUENCE,
                                        groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         rc = _getSubStream( groupID, &sub, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get sub stream:%d", rc ) ;
            goto error ;
         }

         header.contextID = sub->contextID ;

         pSub = pSession->addSubSession( sub->id.value ) ;
         pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
         pSub->addIODatas( iov ) ;

         rc = pSession->sendMsg( pSub ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                    sub->id.columns.groupID, sub->id.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _getReply( cb, TRUE, tag ) ;
         pCtrl->incRetry() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to reopen sub streams, rc: %d", rc ) ;
               goto error ;
            }
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_LOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_close( _pmdEDUCB *cb )
   {
      return _closeSubStreams( cb, FALSE ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_ROLLBACK, "_coordLobStream::_rollback" )
   INT32 _coordLobStream::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_ROLLBACK ) ;
      rc = _closeSubStreamsWithException( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "got error when rollback lob:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_ROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_CLOSESUBSTREAM, "_coordLobStream::_closeSubStreams" )
   INT32 _coordLobStream::_closeSubStreams( _pmdEDUCB *cb,
                                            BOOLEAN exceptMeta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_CLOSESUBSTREAM ) ;

      MsgOpLob header ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      pCtrl->resetRetry() ;

      _initHeader( header, MSG_BS_LOB_CLOSE_REQ,
                   0, -1, sizeof( header ) ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         for ( SUB_STREAMS::iterator itr = _subs.begin();
               itr != _subs.end();
               ++itr )
         {
            if ( exceptMeta && _metaGroup == itr->second.id.columns.groupID )
            {
               continue ;
            }
            header.contextID = itr->second.contextID ;

            pSub = pSession->addSubSession( itr->second.id.value ) ;
            pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                       itr->second.id.columns.groupID,
                       itr->second.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         if ( isReadonly() )
         {
            set< INT32 > setIgnore ;
            setIgnore.insert( SDB_RTN_CONTEXT_NOTEXIST ) ;
            rc = _getReply( cb, TRUE, tag, &setIgnore ) ;
         }
         else
         {
            rc = _getReply( cb, TRUE, tag ) ;
         }
         pCtrl->incRetry() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         rc = _removeClosedSubStreams() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to close sub streams, rc: %d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;            
         }
         else
         {
            continue ;
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_CLOSESUBSTREAM, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_CLOSESUBSTREAMWITHEXCEP, "_coordLobStream::__closeSubStreamsWithException" )
   INT32 _coordLobStream::_closeSubStreamsWithException( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_CLOSESUBSTREAMWITHEXCEP ) ;

      pmdRemoteSession *pSession = _groupSession.getSession() ;
      if ( pSession && !cb->isInterrupted() )
      {
         MsgOpKillContexts killMsg ;
         pmdSubSession *pSub = NULL ;

         killMsg.header.messageLength = sizeof ( MsgOpKillContexts ) ;
         killMsg.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
         killMsg.header.TID = cb->getTID() ;
         killMsg.header.routeID.value = 0;
         killMsg.ZERO = 0;
         killMsg.numContexts = 1 ;

         SUB_STREAMS::const_iterator itr = _subs.begin() ;
         for ( ; itr != _subs.end(); ++itr )
         {
            killMsg.contextIDs[0] = itr->second.contextID ;

            pSub = pSession->addSubSession( itr->second.id.value ) ;
            pSub->setReqMsg( &( killMsg.header ), PMD_EDU_MEM_NONE ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Kill sub-context on node[%d:%d] failed, "
                       "rc:%d", itr->second.id.columns.groupID,
                       itr->second.id.columns.nodeID, rc ) ;
            }
         }

         rc = pSession->waitReply1( TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Wait all reply failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      _groupSession.resetSubSession() ;
      _subs.clear() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_CLOSESUBSTREAMWITHEXCEP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_EXTRACTMETA, "_coordLobStream::_extractMeta" )   
   INT32 _coordLobStream::_extractMeta( const MsgOpReply *header,
                                        bson::BSONObj &metaObj,
                                        _rtnLobDataPool::tuple &dataTuple )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_EXTRACTMETA ) ;
      const CHAR *metaRaw = NULL ;
      UINT32 dataOffset = sizeof( MsgOpReply ) ;

      dataTuple.clear() ;

      if ( NULL == header )
      {
         PD_LOG( PDERROR, "header is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( ( UINT32 )header->header.messageLength <
           sizeof( MsgOpReply ) + 5 )
      {
         PD_LOG( PDERROR, "invalid msg length:%d",
                 header->header.messageLength ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      metaRaw = ( const CHAR * )header + sizeof( MsgOpReply ) ;
      try
      {
         metaObj = BSONObj( metaRaw ).getOwned() ;
         dataOffset += ossAlign4( (UINT32)metaObj.objsize() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( (UINT32)header->header.messageLength >
           dataOffset + sizeof( MsgLobTuple ) )
      {
         MsgLobTuple *rt = ( MsgLobTuple* )( (const CHAR*)header+dataOffset ) ;
         dataOffset += sizeof( MsgLobTuple ) ;

         SDB_ASSERT( DMS_LOB_META_SEQUENCE == rt->columns.sequence &&
                     DMS_LOB_META_LENGTH == rt->columns.offset,
                     "Must be the first sequence page" ) ;

         dataTuple.data = ( const CHAR* )header + dataOffset ;
         dataTuple.len = rt->columns.len ;
         dataTuple.offset = 0 ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_EXTRACTMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                       _dmsLobMeta &meta )
   {
      return _queryLobMeta( cb, meta, TRUE ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_REMOVEV, "_coordLobStream::_removev" )
   INT32 _coordLobStream::_removev( const RTN_LOB_TUPLES &tuples,
                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_REMOVEV ) ;

      BOOLEAN reshard = TRUE ;
      DONE_LST doneLst ;
      MsgOpLob header ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      pCtrl->resetRetry() ;

      _initHeader( header,
                   MSG_BS_LOB_REMOVE_REQ,
                   0, -1, sizeof( header ) ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         if ( reshard )
         {
            rc = _shardData( header, tuples, TRUE, doneLst, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }

            reshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;
            if ( !dg.hasData() )
            {
               continue ;
            }

            header.contextID = dg.contextID ;
            header.header.messageLength = sizeof( MsgHeader ) +
                                          itr->second.bodyLen ;

            pSub = pSession->addSubSession( dg.id.value ) ;
            pSub->setReqMsg( &( header.header ), PMD_EDU_MEM_NONE ) ;
            pSub->addIODatas( dg.body ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Send msg to node[%d:%d] failed, rc:%d",
                       dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( cb, TRUE, tag ) ;
         pCtrl->incRetry() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get reply msg, rc: %d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLstFromReply( doneLst ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to reopen sub streams, rc: %d", rc ) ;
               goto error ;
            }
            reshard = TRUE ;
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_REMOVEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_addSubStreamsFromReply()
   {
      INT32 rc = SDB_OK ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         if ( SDB_OK != ( *itr )->flags )
         {
            rc = ( *itr )->flags ;
            PD_LOG( PDERROR, "failed to open lob on node[%d:%d], rc:%d",
                    ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->header.routeID.columns.nodeID, rc ) ;
            continue ;
         }

         if ( -1 == ( *itr )->contextID )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "invalid context id" ) ;
            continue ;
         }

         _add2Subs( ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->contextID,
                    ( *itr )->header.routeID ) ;
      }

      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_GETREPLY, "_coordLobStream::_getReply" )
   INT32 _coordLobStream::_getReply( _pmdEDUCB *cb,
                                     BOOLEAN nodeSpecified,
                                     INT32 &tag,
                                     set< INT32 > *pIgoreErr )
   {
      INT32 rc = SDB_OK ;
      INT32 flags = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_GETREPLY ) ;

      coordCataSel cataSel ;
      MsgOpReply *pReply = NULL ;
      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      coordGroupSel *pSel = _groupSession.getGroupSel() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;
      pmdSubSessionItr itr ;
      MsgRouteID id ;

      tag = ( INT32 )RETRY_TAG_NULL ;

      cataSel.bind( _pResource, _cataInfo, FALSE ) ;

      rc = pSession->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to wait reply: %d", rc ) ;
         goto error ;
      }

      itr = pSession->getSubSessionItr( PMD_SSITR_REPLY ) ;
      while( itr.more() )
      {
         pSub = itr.next() ;
         pReply = (MsgOpReply*)pSub->getRspMsg( TRUE ) ;
         flags = pReply->flags ;
         id = pReply->header.routeID ;

         if ( SDB_OK == flags ||
              ( pIgoreErr && pIgoreErr->count( flags ) > 0 ) )
         {
            _results.push_back( pReply ) ;
            continue ;
         }

         PD_LOG( PDWARNING, "Node[%d.%d] return failed, flags: %d, "
                 "new primary: %d", id.columns.groupID, id.columns.nodeID,
                 flags, pReply->startFrom ) ;

         CoordGroupMap::iterator it = _mapGroupInfo.find( id.columns.groupID ) ;
         if ( it == _mapGroupInfo.end() )
         {
            SDB_ASSERT( FALSE, "Group info is not exist" ) ;
            flags = SDB_COOR_NO_NODEGROUP_INFO ;
         }
         else if ( !nodeSpecified &&
                   pCtrl->canRetry( flags, id, pReply->startFrom,
                                    isReadonly(), TRUE ) &&
                   pSel->getGroupPtrFromMap( it->first, it->second ) )
         {
            tag |= RETRY_TAG_RETRY ;
            flags = SDB_OK ;

            if ( _metaGroup == it->first )
            {
               _metaGroupInfo = it->second ;
            }
         }
         else if ( pCtrl->canRetry( flags, cataSel, FALSE ) &&
                   SDB_OK == ( flags = _updateCataInfo( TRUE, cb ) ) )
         {
            tag |= ( RETRY_TAG_RETRY | RETRY_TAG_REOPEN ) ;
            flags = SDB_OK ;
         }
         else
         {
            _nokRC[ pReply->header.routeID.value ] =
               coordErrorInfo( pReply ) ;
         }

         SDB_OSS_FREE( pReply ) ;
         rc = flags ? flags : rc ;
      }

      if ( rc )
      {
         goto error ;
      }

   done:
      _groupSession.resetSubSession() ;
      PD_TRACE_EXITRC( COORD_LOBSTREAM_GETREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordLobStream::_clearMsgData()
   {
      std::vector<MsgOpReply *>::iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SAFE_OSS_FREE( *itr ) ;
      }

      _results.clear() ;
      _groupSession.resetSubSession() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_REOPENSUBSTREAMS, "_coordLobStream::_reopenSubStreams" ) 
   INT32 _coordLobStream::_reopenSubStreams( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_REOPENSUBSTREAMS ) ;
      rc = _closeSubStreams( cb, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close sub streams:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_REOPENSUBSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_SHARDDATA, "_coordLobStream::_shardData" )
   INT32 _coordLobStream::_shardData( const MsgOpLob &header,
                                      const RTN_LOB_TUPLES &tuples,
                                      BOOLEAN isWrite,
                                      const DONE_LST &doneLst,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_SHARDDATA ) ;
      _dataGroups.clear() ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         const subStream *sub = NULL ;
         UINT32 groupID = 0 ;
         dataGroup *dg = NULL ;
         const MsgLobTuple *tuple = ( const MsgLobTuple * )(&( *itr )) ;

         if ( 0 < doneLst.count( (ossValuePtr)tuple ) )
         {
            continue ;
         }

         rc = _cataInfo->getLobGroupID( getOID(),
                                        tuple->columns.sequence,
                                        groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         rc = _getSubStream( groupID, &sub, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get sub stream:%d", rc ) ;
            goto error ;
         }

         dg = &( _dataGroups[groupID] ) ;
         if ( !dg->hasData() )
         {
            _pushLobHeader( &header, BSONObj(), dg->body ) ;
            dg->bodyLen += sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
         }
         dg->addData( *tuple,
                      isWrite ? itr->data : NULL ) ;
         dg->contextID = sub->contextID ;
         dg->id = sub->id ;
      }
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_SHARDDATA, rc ) ;
      return rc ;
   error:
      _dataGroups.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_SHARDSINGLEDATA, "_coordLobStream::_shardSingleData" )
   INT32 _coordLobStream::_shardSingleData( const MsgOpLob &header,
                                            const _rtnLobTuple& tuple,
                                            BOOLEAN isWrite,
                                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_SHARDSINGLEDATA ) ;

      const subStream *sub = NULL ;
      UINT32 groupID = 0 ;
      dataGroup *dg = NULL ;
      const MsgLobTuple& t = tuple.tuple ;

      _dataGroups.clear() ;

      rc = _cataInfo->getLobGroupID( getOID(),
                                     t.columns.sequence,
                                     groupID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
         goto error ;
      }

      rc = _getSubStream( groupID, &sub, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get sub stream:%d", rc ) ;
         goto error ;
      }

      dg = &( _dataGroups[groupID] ) ;
      if ( !dg->hasData() )
      {
         _pushLobHeader( &header, BSONObj(), dg->body ) ;
         dg->bodyLen += sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
      }
      dg->addData( t, isWrite ? tuple.data : NULL ) ;
      dg->contextID = sub->contextID ;
      dg->id = sub->id ;

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_SHARDSINGLEDATA, rc ) ;
      return rc ;
   error:
      _dataGroups.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_HANDLEREADRESULTS, "_coordLobStream::_handleReadResults" )
   INT32 _coordLobStream::_handleReadResults( _pmdEDUCB *cb,
                                              DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_HANDLEREADRESULTS ) ;

      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         if ( SDB_OK != ( *itr )->flags )
         {
            rc = ( *itr )->flags ;
            PD_LOG( PDERROR, "failed to read lob on node[%d:%d], rc:%d",
                    ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->header.routeID.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _push2Pool( *itr ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLst( ( *itr )->header.routeID.columns.groupID,
                            doneLst ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add tuples to done list:%d", rc ) ;
            goto error ;
         }
      }

      {
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         _getPool().entrust( ( CHAR * )( *itr ) ) ;
      }
      }
      _results.clear() ;
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_HANDLEREADRESULTS, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_ADD2DONELSTFROMREPLY, "_coordLobStream::_add2DoneLstFromReply" )
   INT32 _coordLobStream::_add2DoneLstFromReply( DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_ADD2DONELSTFROMREPLY ) ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SDB_ASSERT( SDB_OK == ( *itr )->flags, "impossible" ) ; 
         rc = _add2DoneLst( ( *itr )->header.routeID.columns.groupID,
                            doneLst ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add group to done list:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_ADD2DONELSTFROMREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_ADD2DONELST, "_coordLobStream::_add2DoneLst" )
   INT32 _coordLobStream::_add2DoneLst( UINT32 groupID, DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_ADD2DONELST ) ;
      DATA_GROUPS::iterator itr = _dataGroups.find( groupID ) ;
      if ( _dataGroups.end() == itr )
      {
         PD_LOG( PDERROR, "we can not find group:%d", groupID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
      dataGroup &dg = itr->second ;
      for ( list<ossValuePtr>::const_iterator titr =
                                               dg.tuples.begin() ;
            titr != dg.tuples.end() ;
            ++titr )
      {
         if ( !doneLst.insert( *titr ).second )
         {
            PD_LOG( PDERROR, "we already pushed tuple to pool[%d:%d:%lld]",
                    (( MsgLobTuple *)( *titr ))->columns.len,
                    (( MsgLobTuple *)( *titr ))->columns.sequence,
                    (( MsgLobTuple *)( *titr ))->columns.offset ) ;
            rc = SDB_SYS ;
         }
      }

      dg.clearData() ;
      }
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_ADD2DONELST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_removeClosedSubStreams()
   {
      INT32 rc = SDB_OK ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SDB_ASSERT( 1 == _subs.count(
                           ( *itr )->header.routeID.columns.groupID ),
                     "impossible" ) ;
         _subs.erase( ( *itr )->header.routeID.columns.groupID ) ;
      }
      return rc ;
   }

   void _coordLobStream::_pushLobHeader( const MsgOpLob *header,
                                         const BSONObj &obj,
                                         netIOVec &iov )
   {
      const CHAR *off = ( const CHAR * )header + sizeof( MsgHeader ) ;
      UINT32 len = sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
      iov.push_back( netIOV( off, len ) ) ;

      if ( !obj.isEmpty() )
      {
         iov.push_back( netIOV( obj.objdata(), obj.objsize() ) ) ;
         UINT32 alignedLen = ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         if ( ( UINT32 )obj.objsize() < alignedLen )
         {
            iov.push_back( netIOV( &_alignBuf, alignedLen - obj.objsize() ) ) ;
         }
      }
   }

   void _coordLobStream::_pushLobData( const void *data,
                                       UINT32 len,
                                       netIOVec &iov )
   {
      iov.push_back( netIOV( data, len ) ) ;
   }

   void _coordLobStream::_initHeader( MsgOpLob &header,
                                      INT32 opCode,
                                      INT32 bsonLen,
                                      SINT64 contextID,
                                      INT32 msgLen )
   {
      ossMemset( &header, 0, sizeof( header ) ) ;
      header.header.opCode = opCode ;
      header.bsonLen = bsonLen ;
      header.contextID = contextID ;
      header.flags = _getFlags() ;
      header.header.messageLength = msgLen < 0 ?
                                    sizeof( header ) + bsonLen :
                                    msgLen ;
   }

   void _coordLobStream::_add2Subs( UINT32 groupID,
                                    SINT64 contextID,
                                    MsgRouteID id )
   {
      SDB_ASSERT( 0 == _subs.count( groupID ), "impossible" ) ;
      _subs[groupID] = subStream( contextID, id ) ;
      return ;
   }

   INT32 _coordLobStream::_ensureEmptyPageBuf( INT32 pageSize )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == _emptyPageBuf )
      {
         _emptyPageBuf = (CHAR*)SDB_OSS_MALLOC( pageSize ) ;
         if ( NULL == _emptyPageBuf )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to malloc empty page buf, rc=%d", rc ) ;
         }
         ossMemset( _emptyPageBuf, 0, pageSize ) ;
      }

      return rc ;
   }

   void _coordLobStream::_releaseEmptyPageBuf()
   {
      SAFE_OSS_FREE( _emptyPageBuf ) ;
   }
}

