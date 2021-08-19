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

   Source File Name = coordLobStream.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_LOB_STREAM_HPP__
#define COORD_LOB_STREAM_HPP__

#include "rtnLobStream.hpp"
#include "coordRemoteSession.hpp"
#include "coordGroupHandle.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _coordLobStream define
   */
   class _coordLobStream : public _rtnLobStream
   {
      public:
         _coordLobStream( coordResource *pResource, INT64 timeout ) ;
         virtual ~_coordLobStream() ;

      public:
         virtual _dmsStorageUnit *getSU()
         {
            return NULL ;
         }

         virtual void   getErrorInfo( INT32 rc,
                                      _pmdEDUCB *cb,
                                      _rtnContextBuf *buf ) ;

      private:
         virtual INT32 _prepare( _pmdEDUCB *cb ) ;

         virtual INT32 _queryLobMeta( _pmdEDUCB *cb,
                                      _dmsLobMeta &meta,
                                      BOOLEAN allowUncompleted = FALSE,
                                      _rtnLobPiecesInfo* piecesInfo = NULL ) ;

         virtual INT32 _ensureLob( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta,
                                   BOOLEAN &isNew ) ;

         virtual INT32 _getLobPageSize( INT32 &pageSize ) ;

         virtual INT32 _write( const _rtnLobTuple &tuple,
                               _pmdEDUCB *cb,
                               BOOLEAN orUpdate = FALSE ) ;

         virtual INT32 _writev( const RTN_LOB_TUPLES &tuples,
                                _pmdEDUCB *cb,
                                BOOLEAN orUpdate = FALSE ) ;

         virtual INT32 _update( const _rtnLobTuple &tuple,
                                _pmdEDUCB *cb ) ;

         virtual INT32 _updatev( const RTN_LOB_TUPLES &tuples,
                                _pmdEDUCB *cb ) ;

         virtual INT32 _readv( const RTN_LOB_TUPLES &tuples,
                               _pmdEDUCB *cb,
                               const _rtnLobPiecesInfo* piecesInfo = NULL ) ;

         virtual INT32 _completeLob( const _rtnLobTuple &tuple,
                                     _pmdEDUCB *cb ) ;

         virtual INT32 _rollback( _pmdEDUCB *cb ) ;

         virtual INT32 _queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                    _dmsLobMeta &meta ) ;

         virtual INT32 _removev( const RTN_LOB_TUPLES &tuples,
                                 _pmdEDUCB *cb ) ;

         virtual INT32 _getRTDetail( _pmdEDUCB *cb, bson::BSONObj &detail ) ;

         virtual INT32 _lock( _pmdEDUCB *cb,
                              INT64 offset,
                              INT64 length ) ;

         virtual INT32 _close( _pmdEDUCB *cb ) ;

      private:
         struct subStream
         {
            SINT64 contextID ;
            MsgRouteID id ;

            subStream()
            :contextID( -1 )
            {
               id.value = MSG_INVALID_ROUTEID ;
            }

            subStream( SINT64 context, MsgRouteID route )
            :contextID( context ),
             id( route )
            {
            }
         } ;

         struct dataGroup
         {
            SINT64 contextID ;
            MsgRouteID id ;
            netIOVec body ;
            UINT32 bodyLen ;
            std::list<ossValuePtr> tuples ;

            dataGroup()
            :contextID( -1 ),
             bodyLen( 0 )
            {
               id.value = MSG_INVALID_ROUTEID ;
            }

            BOOLEAN hasData() const
            {
               return 0 < bodyLen ;
            }

            INT32 addData( const MsgLobTuple &tuple, const void *data )
            {
               INT32 rc = SDB_OK ;
               try
               {
                  body.push_back( netIOV( tuple.data, sizeof( tuple ) ) ) ;
                  bodyLen += sizeof( tuple ) ;
                  tuples.push_back( ( ossValuePtr )( &tuple ) ) ;
                  if ( NULL != data )
                  {
                     body.push_back( netIOV( data, tuple.columns.len ) ) ;
                     bodyLen += tuple.columns.len ;
                  }
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "Unexpected err happened:%s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

            done:
               return rc ;
            error:
               goto done ;
            }

            void clearData()
            {
               body.clear() ;
               bodyLen = 0 ;
               tuples.clear() ;
            }
         } ;


         enum RETRY_TAG
         {
            RETRY_TAG_NULL = 0,
            RETRY_TAG_RETRY = 0x00001,
            RETRY_TAG_REOPEN = 0x00002,
         } ;

         typedef ossPoolMap<UINT32, subStream>     SUB_STREAMS ;
         typedef ossPoolSet<ossValuePtr>           DONE_LST ;
         typedef ossPoolMap<UINT32, dataGroup>     DATA_GROUPS ;

         typedef ossPoolMap<ossValuePtr, UINT32>   TUPLE_GROUPID_MAP ;

      private:
         INT32 _openSubStreams( CoordGroupList &gpLst, _pmdEDUCB* cb ) ;

         INT32 _openMainStream( const CHAR *fullName,
                                const bson::OID &oid,
                                INT32 mode,
                                _pmdEDUCB *cb ) ;

         INT32 _openOtherStreams( const CHAR *fullName,
                                  const bson::OID &oid,
                                  INT32 mode,
                                  _pmdEDUCB *cb,
                                  CoordGroupList &gpList ) ;

         INT32 _openOtherStream( const CHAR *fullName,
                                 const bson::OID &oid,
                                 INT32 mode,
                                 _pmdEDUCB *cb,
                                 UINT32 pSpecGroupID ) ;

         INT32 _getSubStream( UINT32 groupID,
                              const subStream** sub,
                              _pmdEDUCB *cb ) ;

         INT32 _extractMeta( const MsgOpReply *header,
                             bson::BSONObj &obj,
                             _rtnLobDataPool::tuple &dataTuple ) ;

         INT32 _extractDetail( const MsgOpReply *header,
                               bson::BSONObj &detail ) ;

         INT32 _closeSubStreams( _pmdEDUCB *cb, BOOLEAN exceptMeta ) ;

         INT32 _closeSubStreamsWithException( _pmdEDUCB *cb ) ;

         INT32 _push2Pool( const MsgOpReply *header ) ;

         INT32 _getReply( _pmdEDUCB *cb,
                          BOOLEAN nodeSpecified,
                          INT32 &tag,
                          set< INT32 > *pIgoreErr = NULL ) ;

         void _clearMsgData() ;

         INT32 _reopenSubStreams( _pmdEDUCB *cb ) ;

         INT32 _addSubStreamsFromReply() ;

         INT32 _updateCataInfo( BOOLEAN refresh,
                                _pmdEDUCB *cb ) ;

         INT32 _shardData( const MsgOpLob &header,
                           const RTN_LOB_TUPLES &tuples,
                           BOOLEAN isWrite,
                           const DONE_LST &doneLst,
                           _pmdEDUCB *cb ) ;

         INT32 _shardSingleData( const MsgOpLob &header,
                                 const _rtnLobTuple& tuple,
                                 BOOLEAN isWrite,
                                 _pmdEDUCB *cb ) ;

         INT32 _handleReadResults( _pmdEDUCB *cb, DONE_LST &doneLst ) ;

         INT32 _add2DoneLstFromReply( DONE_LST &doneLst ) ;

         INT32 _add2DoneLst( UINT32 groupID, DONE_LST &doneLst ) ;

         INT32 _removeClosedSubStreams() ;

         void _add2Subs( UINT32 groupID, SINT64 contextID, MsgRouteID id ) ;

         INT32 _pushLobHeader( const MsgOpLob *header,
                              const BSONObj &obj,
                              netIOVec &iov ) ;

         INT32 _pushLobData( const void *data,
                            UINT32 len,
                            netIOVec &iov ) ;

         void _initHeader( MsgOpLob &header,
                           INT32 opCode,
                           INT32 bsonLen,
                           INT64 contextID,
                           INT32 msgLen = -1 ) ;

         INT32 _writeOp( const _rtnLobTuple &tuple, INT32 opCode,
                         _pmdEDUCB *cb, BOOLEAN orUpdate = FALSE ) ;

         INT32 _writevOp( const RTN_LOB_TUPLES &tuples, INT32 opCode,
                          _pmdEDUCB *cb, BOOLEAN orUpdate = FALSE ) ;

         INT32 _read( const _rtnLobTuple& tuple,
                      _pmdEDUCB *cb,
                      pmdEDUEvent &event ) ;

         INT32 _ensureEmptyPageBuf( INT32 pageSize ) ;

         void  _releaseEmptyPageBuf() ;

         INT32 _getSubCLInfo( CoordCataInfoPtr &mainCLcataPtr,
                              const _utilLobID &lobId, _pmdEDUCB *cb,
                              CoordCataInfoPtr &subCLcataPtr ) ;

         void _getGroupLst( CoordGroupList &groupLst ) ;
         INT32 _getLobGroupID( const OID &oid, UINT32 sequence,
                               UINT32 &groupID) ;

      private:
         CoordCataInfoPtr _cataInfo ;
         CoordCataInfoPtr _subCLInfo ;
         CoordGroupMap    _mapGroupInfo ;
         UINT32           _pageSize ;

         std::vector<pmdEDUEvent> _results ;
         DATA_GROUPS       _dataGroups ;

         SUB_STREAMS       _subs;
         bson::BSONObj     _metaObj ;
         CoordGroupInfoPtr _metaGroupInfo ;
         UINT32            _metaGroup ;

         UINT32            _alignBuf ;
         /// error info
         ROUTE_RC_MAP      _nokRC ;

         coordResource        *_pResource ;
         INT64                _timeout ;
         coordGroupSession    _groupSession ;
         coordGroupHandler    _groupHandler ;
         coordRemoteHandler   _remoteHandler ;

         CHAR*                _emptyPageBuf ;
         BOOLEAN              _mainStreamOpened ;
   } ;
   typedef _coordLobStream coordLobStream ;

}

#endif // COORD_LOB_STREAM_HPP__

