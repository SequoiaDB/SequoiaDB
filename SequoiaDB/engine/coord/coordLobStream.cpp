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
   INT32 _coordLobStream::_prepare( _pmdEDUCB *cb )
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
      if ( SDB_OK != rc && coordCataCheckFlag( rc ) )
      {
         PD_LOG( PDEVENT, "Retry to update catalog info" ) ;
         rc = _updateCataInfo( TRUE, cb ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update catalog info of:%s, rc:%d",
                 getFullName(), rc ) ;
         goto error ;
      }

      /// set primary
      _groupSession.getGroupSel()->setPrimary(
                            !SDB_IS_LOBREADONLY_MODE(mode()) ? TRUE : FALSE ) ;
      _groupSession.getGroupCtrl()->setMaxRetryTimes( LOB_MAX_RETRYTIMES ) ;

      rc = _openMainStream( getFullName(), getOID(), mode(), cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open main streams:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_getSubCLInfo( CoordCataInfoPtr &mainCLcataPtr,
                                         const _utilLobID &lobId,
                                         _pmdEDUCB *cb,
                                         CoordCataInfoPtr &subCLcataPtr )
   {
      INT32 rc = SDB_OK ;
      string subCLName ;
      rc = mainCLcataPtr->getSubCLNameByLobID( lobId, subCLName ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING, "Couldn't find the lobId[%s]'s sub-collection "
                 "in MainCL(%s), rc: %d", lobId.toString().c_str(),
                 mainCLcataPtr->getName(), rc ) ;
         goto error ;
      }

      rc = _pResource->getOrUpdateCataInfo( subCLName.c_str(),
                                            subCLcataPtr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Get sub-collection[%s]'s catalog info "
                   "failed, rc: %d", subCLName.c_str(), rc ) ;

      if ( subCLcataPtr->isRangeSharded() )
      {
         PD_LOG( PDERROR, "Can not open a lob in range sharded cl[%s]",
                 subCLName.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordLobStream::_getGroupLst( CoordGroupList &groupLst )
   {
      if ( _cataInfo->isMainCL() )
      {
         _subCLInfo->getGroupLst( groupLst ) ;
      }
      else
      {
         _cataInfo->getGroupLst( groupLst ) ;
      }
   }

   INT32 _coordLobStream::_getLobGroupID( const OID &oid, UINT32 sequence,
                                          UINT32 &groupID)
   {
      if ( _cataInfo->isMainCL() )
      {
         return _subCLInfo->getLobGroupID( oid, sequence, groupID ) ;
      }
      else
      {
         return _cataInfo->getLobGroupID( oid, sequence, groupID ) ;
      }
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_UPDATECATAINFO, "_coordLobStream::_updateCataInfo" )
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
            PD_LOG( PDERROR, "Update collection[%s] catalog info failed, "
                    "rc: %d", getFullName(), rc ) ;
            goto error ;
         }
      }

      if ( _cataInfo->isMainCL() )
      {
         BYTE oidArray[UTIL_LOBID_ARRAY_LEN] = { 0 } ;
         const bson::OID oid = getOID() ;
         _utilLobID lobId ;
         INT32 lobShardingKeyFormat = _cataInfo->getLobShardingKeyFormat() ;
         if ( SDB_TIME_INVALID == lobShardingKeyFormat )
         {
            PD_LOG( PDERROR, "Can not open a lob in main cl without "
                    "LobShardingFormat" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         oid.toByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;

         rc = lobId.initFromByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Invalid Oid in lob's MainCL:Oid=%s,rc=%d",
                    oid.str().c_str(), rc ) ;
            goto error ;
         }

         rc  = _getSubCLInfo( _cataInfo, lobId, cb, _subCLInfo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to get subcl info:MainCL=%s,id=%s,rc=%d",
                    getFullName(), oid.str().c_str(), rc ) ;
            goto error ;
         }
      }
      else if ( _cataInfo->isRangeSharded() )
      {
         PD_LOG( PDERROR, "Can not open a lob in range sharded cl" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _getLobGroupID( getOID(), DMS_LOB_META_SEQUENCE, _metaGroup ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get meta group:%d", rc ) ;
         goto error ;
      }
      // get group info
      rc = _pResource->getOrUpdateGroupInfo( _metaGroup, _metaGroupInfo,
                                             cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get meta group info[%u] failed, rc: %d",
                 _metaGroup, rc ) ;
         goto error ;
      }
      _mapGroupInfo[ _metaGroup ] = _metaGroupInfo ;
   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_UPDATECATAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // just open one sub stream which is not main stream
   INT32 _coordLobStream::_openOtherStream( const CHAR *fullName,
                                            const bson::OID &oid,
                                            INT32 mode,
                                            _pmdEDUCB *cb,
                                            UINT32 groupID )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList gpList ;
      try
      {
         gpList[ groupID ] = groupID ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _openOtherStreams( fullName, oid, mode, cb, gpList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open stream:groupID=%u,rc=%d",
                   groupID, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // open sub streams without main stream
   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_OPENOTHERSTREAMS, "_coordLobStream::_openOtherStreams" )
   INT32 _coordLobStream::_openOtherStreams( const CHAR *fullName,
                                             const bson::OID &oid,
                                             INT32 mode,
                                             _pmdEDUCB *cb,
                                             CoordGroupList &gpLst )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_OPENOTHERSTREAMS ) ;

      SDB_ASSERT( gpLst.size() > 0, "must be greate than 0" ) ;
      SDB_ASSERT( gpLst.count( _metaGroup ) == 0, "impossible" ) ;

      MsgOpLob header ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      netIOVec iov ;
      CoordGroupMap::iterator itMap ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      coordGroupSel *pSel = _groupSession.getGroupSel() ;

      /// reset retry times
      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, FALSE ) ;
      if ( _cataInfo->isMainCL() )
      {
         builder.append( FIELD_NAME_SUBCLNAME, _subCLInfo->getName() ) ;
      }

      if ( SDB_IS_LOBREADONLY_MODE( mode ) )
      {
         /// send meta data to every group.
         builder.append( FIELD_NAME_LOB_META_DATA, _metaObj ) ;
      }

      obj = builder.obj() ;

      _initHeader( header,
                   MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      rc = _pushLobHeader( &header, obj, iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

      do
      {
         INT32 tag = RETRY_TAG_NULL ;
         _clearMsgData() ;
         CoordGroupList sendGrpLst ;
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
               PD_LOG( PDDEBUG, "ignore open substream:lobID=%s,groupID=%u",
                       getOID().toString().c_str(), itr->first ) ;
               continue ;
            }

            itMap = _mapGroupInfo.find( itr->first ) ;
            if ( _mapGroupInfo.end() == itMap )
            {
               SDB_ASSERT( FALSE, "Group info is not exist" ) ;
               rc = SDB_COOR_NO_NODEGROUP_INFO ;
               goto error ;
            }

            /// add group info
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
         /// need to add succeed nodes to subs whether successful or not,
         /// because it can close all opened contexts that on data node
         /// when some nodes failed
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

      /// reset retry times
      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, TRUE ) ;
      if ( _cataInfo->isMainCL() )
      {
         builder.append( FIELD_NAME_SUBCLNAME, _subCLInfo->getName() ) ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         builder.append( FIELD_NAME_LOB_CREATETIME, (INT64)_getMeta()._createTime ) ;
      }

      if ( _mainStreamOpened )
      {
         builder.appendBool( FIELD_NAME_LOB_REOPENED, TRUE ) ;
         if ( SDB_HAS_LOBWRITE_MODE( mode ) || SDB_LOB_MODE_SHAREREAD == mode )
         {
            if (!_getSectionMgr().isEmpty())
            {
               rc = _getSectionMgr().toBSONObjBuilder( builder ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to save LobSections, rc=%d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      obj = builder.obj() ;

      _initHeader( header, MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      rc = _pushLobHeader( &header, obj, iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         /// add group info
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
            reply = _results.empty() ? NULL :
                     (MsgOpReply*)((*_results.begin())._Data ) ;
            break ;
         }
      } while ( TRUE ) ;

      _add2Subs( reply->header.routeID.columns.groupID, reply->contextID,
                 reply->header.routeID ) ;

      if ( !_mainStreamOpened )
      {
         rc = _extractMeta( reply, _metaObj, dataTuple ) ;
         if ( dataTuple.data && dataTuple.len > 0 )
         {
            /// push data to pool
            rc = _getPool().push( dataTuple.data,
                                  dataTuple.len,
                                  dataTuple.offset ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Push data to pool failed, rc:%d", rc ) ;
               goto error ;
            }
            _getPool().pushDone() ;

            _getPool().entrust( *_results.begin() ) ;
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

   // open sub streams including main stream
   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_OPENSUBSTREAMS, "_coordLobStream::_openSubStreams" )
   INT32 _coordLobStream::_openSubStreams( CoordGroupList &gpLst,
                                           _pmdEDUCB* cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_OPENSUBSTREAMS ) ;
      CoordGroupList::iterator iter ;

      iter = gpLst.find( _metaGroup ) ;
      if ( iter != gpLst.end() )
      {
         rc = _openMainStream( getFullName(), getOID(), _getMode(), cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to open main stream in group[%d], "
                    "rc: %d", _metaGroup, rc ) ;
            goto error ;
         }

         gpLst.erase( iter ) ;
      }

      if ( gpLst.size() > 0 )
      {
         rc = _openOtherStreams( getFullName(), getOID(), _getMode(), cb,
                                 gpLst ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to open other streams, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_LOBSTREAM_OPENSUBSTREAMS, rc ) ;
      return rc ;
   error:
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
         PD_LOG( PDDEBUG, "intent to create substream:lobID=%s,groupID=%d",
                 getOID().toString().c_str(), groupID ) ;
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
            rc = _openOtherStream( getFullName(), getOID(), _getMode(), cb,
                                   groupID ) ;
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

      /// reset retry times
      pCtrl->resetRetry() ;

      _initHeader( header, opCode,
                   0, -1,
                   sizeof( header ) +
                   sizeof( _MsgLobTuple ) +
                   tuple.tuple.columns.len ) ;
      rc = _pushLobHeader( &header, BSONObj(), iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

      rc = _pushLobData( tuple.tuple.data, sizeof( tuple.tuple ), iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob data:rc=%d", rc ) ;

      rc = _pushLobData( tuple.data, tuple.tuple.columns.len, iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob data:rc=%d", rc ) ;

      if ( orUpdate && MSG_BS_LOB_WRITE_REQ == opCode )
      {
         header.flags |= FLG_LOBWRITE_OR_UPDATE ;
      }

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _getLobGroupID( getOID(), tuple.tuple.columns.sequence,
                              groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get destination:%d", rc ) ;
            goto error ;
         }

         rc = _getSubStream( groupID, &sub, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get sub stream:%d", rc ) ;
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

      /// reset retry times
      pCtrl->resetRetry() ;

      /// will reassign length
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

            /// we have pushed part of header to body.
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
                                 _pmdEDUCB *cb,
                                 pmdEDUEvent &event )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM__READ ) ;

      MsgOpLob header ;
      BOOLEAN needReshard = TRUE ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      event.reset() ;
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
            if ( !_results.empty() )
            {
               event = *( _results.begin() ) ;
               _results.erase( _results.begin() ) ;
            }
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

      /// reset retry times
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
         // Before SequoiaDB 3.0, we send UPDATE message to
         // complete lob when close lob in CREATEONLY mode.
         // And in 3.0.1 we also send UPDATE message,
         // in order to compatible with version<3.0.
         // But actually we should WRITE the meta sequence.
         // See also _rtnContextShdOfLob::update().
         rc = _update( tuple, cb ) ;
      }
      else if ( SDB_HAS_LOBWRITE_MODE( _getMode() )
                || SDB_LOB_MODE_TRUNCATE == _getMode() )
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

      /// reset retry times
      pCtrl->resetRetry() ;

      builder.append( FIELD_NAME_LOB_OFFSET, offset )
             .append( FIELD_NAME_LOB_LENGTH, length ) ;
      obj = builder.obj() ;

      _initHeader( header, MSG_BS_LOB_LOCK_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      rc = _pushLobHeader( &header, obj, iov ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _getLobGroupID( getOID(), DMS_LOB_META_SEQUENCE, groupID ) ;
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

   INT32 _coordLobStream::_getRTDetail( _pmdEDUCB *cb, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      MsgOpLob header ;
      UINT32 groupID = 0 ;
      const subStream *sub = NULL ;
      const MsgOpReply *reply = NULL ;

      coordGroupSessionCtrl *pCtrl = _groupSession.getGroupCtrl() ;
      pmdRemoteSession *pSession = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      /// reset retry times
      pCtrl->resetRetry() ;
      _initHeader( header, MSG_BS_LOB_GETRTDETAIL_REQ,
                   0, -1, sizeof( header ) ) ;
      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _getLobGroupID( getOID(), DMS_LOB_META_SEQUENCE, groupID ) ;
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
            SDB_ASSERT( 1 == _results.size(), "impossible" ) ;
            reply = _results.empty() ? NULL :
                    (MsgOpReply*)((*_results.begin())._Data ) ;
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

      rc = _extractDetail( reply, detail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract detail from reply msg:rc=%d",
                   rc ) ;

   done:
      _clearMsgData() ;
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

      /// reset retry times
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
               PD_LOG( PDDEBUG, "ignore close stream:lobID=%s,groupID=%d"
                       "nodeID=%d", getOID().toString().c_str(),
                       itr->second.id.columns.groupID,
                       itr->second.id.columns.nodeID ) ;
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
            /// here we only need to close opened sub streams.
            /// do not reopen new streams.
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

   //PD_TRACE_DECLARE_FUNCTION( COORD_LOBSTREAM_CLOSESUBSTREAMWITHEXCEP, "_coordLobStream::_closeSubStreamsWithException" )
   INT32 _coordLobStream::_closeSubStreamsWithException( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOBSTREAM_CLOSESUBSTREAMWITHEXCEP ) ;

      pmdRemoteSession *pSession = _groupSession.getSession() ;
      /// When the cb->isInterrupted(), don't kill context, because the
      /// session will send interrupt or disconnect message to peer node
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

            PD_LOG( PDDEBUG, "kill lob context:lobID=%s,groupID=%d,"
                    "nodeID=%d,contextID=%lld", getOID().toString().c_str(),
                    itr->second.id.columns.groupID,
                    itr->second.id.columns.nodeID, itr->second.contextID ) ;

            rc = pSession->sendMsg( pSub ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Kill sub-context on node[%d:%d] failed, "
                       "rc:%d", itr->second.id.columns.groupID,
                       itr->second.id.columns.nodeID, rc ) ;
               /// try to rollback all substreams, so do not goto error.
            }
         }

         /// wait reply
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

   INT32 _coordLobStream::_extractDetail( const MsgOpReply *header,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      const CHAR *detailRaw = NULL ;

      if ( NULL == header )
      {
         PD_LOG( PDERROR, "header is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( ( UINT32 )header->header.messageLength < sizeof( MsgOpReply ) + 5 )
      {
         PD_LOG( PDERROR, "invalid msg length:%d",
                 header->header.messageLength ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      detailRaw = ( const CHAR * )header + sizeof( MsgOpReply ) ;
      try
      {
         detail = BSONObj( detailRaw ).getOwned() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
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

      /// If have data, need add to pool
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

      /// reset retry times
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
      MsgOpReply *pReply = NULL ;
      std::vector<pmdEDUEvent>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         pReply = ( MsgOpReply* )(*itr)._Data ;
         if ( SDB_OK != pReply->flags )
         {
            rc = pReply->flags ;
            PD_LOG( PDERROR, "failed to open lob on node[%d:%d], rc:%d",
                    pReply->header.routeID.columns.groupID,
                    pReply->header.routeID.columns.nodeID, rc ) ;
            continue ;
         }

         if ( -1 == pReply->contextID )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "invalid context id" ) ;
            continue ;
         }

         _add2Subs( pReply->header.routeID.columns.groupID, pReply->contextID,
                    pReply->header.routeID ) ;
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
      pmdEDUEvent event ;
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
         event = pSub->getOwnedRspMsg() ;
         pReply = (MsgOpReply*)event._Data ;
         flags = pReply->flags ;
         /// Should use pSub's nodeID, because the remote node maybe isn't
         /// the same node with error SDB_INVALID_ROUTEID
         id.value = pSub->getNodeIDUInt() ;

         if ( SDB_OK == flags ||
              ( pIgoreErr && pIgoreErr->count( flags ) > 0 ) )
         {
            /// pReply will be released by _clearMsgData()
            _results.push_back( event ) ;
            continue ;
         }

         PD_LOG( PDWARNING, "Node[%d.%d] return failed, flags: %d, "
                 "new primary: %d", id.columns.groupID, id.columns.nodeID,
                 flags, pReply->startFrom ) ;

         // get group info
         CoordGroupMap::iterator it = _mapGroupInfo.find(
                                 pSub->getNodeID().columns.groupID ) ;
         if ( it == _mapGroupInfo.end() )
         {
            flags = SDB_COOR_NO_NODEGROUP_INFO ;
         }
         else if ( !nodeSpecified &&
                   pCtrl->canRetry( flags, id, pReply->startFrom,
                                    isReadonly(), TRUE ) &&
                   pSel->getGroupPtrFromMap( it->first, it->second ) )
         {
            tag |= RETRY_TAG_RETRY ;
            flags = SDB_OK ;

            /// if meta group has update, so need to update meta group
            if ( _metaGroup == it->first )
            {
               _metaGroupInfo = it->second ;
            }
         }
         // then catalog
         else if ( pCtrl->canRetry( flags, cataSel, FALSE ) &&
                   SDB_OK == ( flags = _updateCataInfo( TRUE, cb ) ) )
         {
            tag |= ( RETRY_TAG_RETRY | RETRY_TAG_REOPEN ) ;
            flags = SDB_OK ;
         }
         else
         {
            _nokRC[ id.value ] = coordErrorInfo( pReply ) ;
         }

         pmdEduEventRelease( event, cb ) ;
         pReply = NULL ;
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
      std::vector<pmdEDUEvent>::iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         pmdEduEventRelease( *itr, NULL ) ;
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
      TUPLE_GROUPID_MAP tuple2GroupIDMap ;
      CoordGroupList newGpList ;
      INT32 oldCataVertion = _cataInfo->getVersion() ;

      _dataGroups.clear() ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ; ++itr )
      {
         UINT32 groupID = 0 ;
         // _rtnLobTuple and _rtnLobTuple.tuple(MsgLobTuple)
         // share the same address
         const MsgLobTuple *tuple = ( const MsgLobTuple * )(&( *itr )) ;
         if ( 0 < doneLst.count( (ossValuePtr)tuple ) )
         {
            continue ;
         }

         rc = _getLobGroupID( getOID(), tuple->columns.sequence, groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         try
         {
            if ( _subs.count( groupID ) == 0 )
            {
               newGpList[ groupID ] = groupID ;
            }

            tuple2GroupIDMap[ (ossValuePtr)(&( *itr )) ] = groupID ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( newGpList.size() > 0 )
      {
         rc = _openSubStreams( newGpList, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get sub stream:%d", rc ) ;
            goto error ;
         }
      }

      for ( TUPLE_GROUPID_MAP::iterator iterMap = tuple2GroupIDMap.begin();
            iterMap != tuple2GroupIDMap.end(); ++iterMap )
      {
         SUB_STREAMS::iterator itrSubStream ;
         subStream* sub = NULL ;
         dataGroup *dg = NULL ;

         _rtnLobTuple *lobTuple = (_rtnLobTuple *)iterMap->first ;
         UINT32 groupID = iterMap->second ;
         if ( oldCataVertion != _cataInfo->getVersion() )
         {
            // catalog info is changed and saved groupID is not longger
            // available. let's re-calculate the groupID
            rc = _getLobGroupID( getOID(), lobTuple->tuple.columns.sequence,
                                 groupID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
               goto error ;
            }
         }

         itrSubStream = _subs.find( groupID ) ;
         if ( _subs.end() == itrSubStream )
         {
            PD_LOG( PDERROR, "group:%d is not in sub streams", groupID ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         sub = &( itrSubStream->second ) ;
         dg = &( _dataGroups[groupID] ) ;
         if ( !dg->hasData() )
         {
            rc = _pushLobHeader( &header, BSONObj(), dg->body ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;
            dg->bodyLen += sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
         }

         rc = dg->addData( lobTuple->tuple, isWrite ? lobTuple->data : NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

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

      rc = _getLobGroupID( getOID(), t.columns.sequence, groupID ) ;
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
         rc = _pushLobHeader( &header, BSONObj(), dg->body ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;
         dg->bodyLen += sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
      }

      rc = dg->addData( t, isWrite ? tuple.data : NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to push lob header:rc=%d", rc ) ;

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

      MsgOpReply *pReply = NULL ;
      std::vector<pmdEDUEvent>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         pReply = (MsgOpReply*)(*itr)._Data ;
         if ( SDB_OK != pReply->flags )
         {
            rc = pReply->flags ;
            PD_LOG( PDERROR, "failed to read lob on node[%d:%d], rc:%d",
                    pReply->header.routeID.columns.groupID,
                    pReply->header.routeID.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _push2Pool( pReply ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLst( pReply->header.routeID.columns.groupID,
                            doneLst ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add tuples to done list:%d", rc ) ;
            goto error ;
         }
      }

      /// we need to keep reply msg in memory.
      {
         std::vector<pmdEDUEvent>::const_iterator itr = _results.begin() ;
         for ( ; itr != _results.end(); ++itr )
         {
            _getPool().entrust( *itr ) ;
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
      MsgOpReply *pReply = NULL ;
      std::vector<pmdEDUEvent>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         pReply = ( MsgOpReply* )(*itr)._Data ;
         SDB_ASSERT( SDB_OK == pReply->flags, "impossible" ) ;
         rc = _add2DoneLst( pReply->header.routeID.columns.groupID,
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
      MsgOpReply *pReply = NULL ;
      std::vector<pmdEDUEvent>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         pReply = ( MsgOpReply* )(*itr)._Data ;
         SDB_ASSERT( 1 == _subs.count(
                           pReply->header.routeID.columns.groupID ),
                     "impossible" ) ;
         _subs.erase( pReply->header.routeID.columns.groupID ) ;
         SDB_ASSERT( 0 == _subs.count(
                           pReply->header.routeID.columns.groupID ),
                     "impossible" ) ;
         PD_LOG( PDDEBUG, "_removeClosedSubStreams:lobID=%s,groupID=%d,"
                 "nodeID=%d", getOID().toString().c_str(),
                 pReply->header.routeID.columns.groupID,
                 pReply->header.routeID.columns.nodeID ) ;
      }
      return rc ;
   }

   INT32 _coordLobStream::_pushLobHeader( const MsgOpLob *header,
                                          const BSONObj &obj,
                                          netIOVec &iov )
   {
      INT32 rc = SDB_OK ;
      try
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
               iov.push_back( netIOV( &_alignBuf,
                                      alignedLen - obj.objsize() ) ) ;
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
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordLobStream::_pushLobData( const void *data,
                                       UINT32 len,
                                       netIOVec &iov )
   {
      INT32 rc = SDB_OK ;
      try
      {
         iov.push_back( netIOV( data, len ) ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
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
      // throw exception outside to make sure contextID is closed( disconnect )
      _subs[groupID] = subStream( contextID, id ) ;
      PD_LOG( PDDEBUG, "_add2Subs:lobID=%s,groupIDKey=%d,groupID=%u"
           "nodeID=%d,contextID=%lld", getOID().toString().c_str(),
           groupID, id.columns.groupID,
           id.columns.nodeID, contextID ) ;
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

