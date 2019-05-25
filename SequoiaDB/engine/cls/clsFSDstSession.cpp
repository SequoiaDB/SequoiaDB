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

   Source File Name = clsFSDstSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "clsFSDstSession.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "clsUtil.hpp"
#include "clsSyncManager.hpp"
#include "pmdStartup.hpp"
#include "msgMessage.hpp"
#include "clsCleanupJob.hpp"
#include "dpsLogRecord.hpp"
#include "dpsLogRecordDef.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "rtnLob.hpp"
#include "rtnRecover.hpp"

using namespace bson ;

namespace engine
{
   const UINT32 CLS_FS_TIMEOUT = 10000 ;
   const UINT32 CLS_SPLIT_DST_SYNC_TIME = 60 * OSS_ONE_SEC ;
   const UINT32 CLS_FS_MAX_REPEAT_CNT = 120 * OSS_ONE_SEC / CLS_FS_TIMEOUT ;

   #define CHECK_REQUEST_ID(Header,id) \
      do { \
         if ( Header.requestID != id ) \
         { \
            PD_LOG ( PDINFO, "RequestID[%lld] is not expected[%lld], ignored", \
                     Header.requestID, id ) ; \
            goto done ; \
         } \
      } while ( 0 )


   /*
   _clsDataDstBaseSession : implement
   */
   BEGIN_OBJ_MSG_MAP( _clsDataDstBaseSession, _pmdAsyncSession )
      ON_MSG( MSG_CLS_FULL_SYNC_META_RES, handleMetaRes )
      ON_MSG( MSG_CLS_FULL_SYNC_INDEX_RES, handleIndexRes )
      ON_MSG( MSG_CLS_FULL_SYNC_NOTIFY_RES, handleNotifyRes )
   END_OBJ_MSG_MAP()

   _clsDataDstBaseSession::_clsDataDstBaseSession ( UINT64 sessionID,
                                                    _netRouteAgent * agent )
   :_pmdAsyncSession( sessionID )
   {
      _agent = agent ;
      _packet = 0 ;
      _status = CLS_FS_STATUS_BEGIN ;
      _current = 0 ;
      _timeout = CLS_FS_TIMEOUT ;
      _recvTimeout = 0 ;
      _quit = FALSE ;
      _requestID = 0 ;
      _lastOprLSN = DPS_INVALID_LSN_OFFSET ;
      _needMoreDoc = TRUE ;
   }

   _clsDataDstBaseSession::~_clsDataDstBaseSession ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS_ONTIMER, "_clsDataDstBaseSession::onTimer" )
   void _clsDataDstBaseSession::onTimer( UINT64 timerID, UINT32 interval )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS_ONTIMER );
      _recvTimeout += interval ;
      if ( _quit )
      {
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }

      _timeout += interval ;
      _selector.timeout( interval ) ;
      if ( _timeout < CLS_FS_TIMEOUT )
      {
         goto done ;
      }
      _timeout = 0 ;

      if ( CLS_FS_STATUS_BEGIN == _status )
      {
         _begin() ;
      }
      else if ( CLS_FS_STATUS_META == _status )
      {
         _meta() ;
      }
      else if ( CLS_FS_STATUS_INDEX == _status )
      {
         _index() ;
      }
      else if ( CLS_FS_STATUS_NOTIFY_DOC == _status )
      {
         _notify( CLS_FS_NOTIFY_TYPE_DOC ) ;
      }
      else if ( CLS_FS_STATUS_NOTIFY_LOG == _status )
      {
         _notify( CLS_FS_NOTIFY_TYPE_LOG ) ;
      }
      else if ( CLS_FS_STATUS_NOTIFY_LOB == _status )
      {
         _notify( CLS_FS_NOTIFY_TYPE_LOB ) ;
      }
      else if ( CLS_FS_STATUS_END == _status )
      {
         _end() ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSDATADBS_ONTIMER );
      return ;
   }

   BOOLEAN _clsDataDstBaseSession::timeout( UINT32 interval )
   {
      return _quit ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS_ONRECV, "_clsDataDstBaseSession::onRecieve" )
   void _clsDataDstBaseSession::onRecieve( const NET_HANDLE netHandle,
                                           MsgHeader * msg )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS_ONRECV );
      if ( MSG_CLS_FULL_SYNC_BEGIN_RES == msg->opCode &&
           NET_INVALID_HANDLE == _netHandle )
      {
         _netHandle = netHandle ;
      }
      _recvTimeout = 0 ;
      PD_TRACE_EXIT ( SDB__CLSDATADBS_ONRECV );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__DISCONN, "_clsDataDstBaseSession::_disconnect" )
   void _clsDataDstBaseSession::_disconnect ()
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__DISCONN );

      MsgHeader msg ;
      _quit = TRUE ;

      if ( _selector.src().value != MSG_INVALID_ROUTEID )
      {
         PD_LOG( PDEVENT, "Session[%s]: Disconnect the session with node[%s]",
                 sessionName(), routeID2String( _selector.src() ).c_str() ) ;
         msg.messageLength = sizeof( MsgHeader ) ;
         msg.TID = CLS_TID( _sessionID ) ;
         msg.routeID.value = MSG_INVALID_ROUTEID ;
         msg.requestID = ++_requestID ;
         msg.opCode = MSG_BS_DISCONNECT ;
         _agent->syncSend( _selector.src(), &msg ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSDATADBS__DISCONN );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__META, "_clsDataDstBaseSession::_meta" )
   void _clsDataDstBaseSession::_meta ()
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__META );
      if ( _fullNames.size() <= _current )
      {
         _end() ;
         goto done ;
      }
      try
      {
         MsgClsFSMetaReq msg ;
         msg.header.TID = CLS_TID( _sessionID ) ;
         msg.header.requestID = ++_requestID ;
         string &fullName = _fullNames.at( _current ) ;
         BSONObjBuilder builder ;
         BSONObj obj ;
         UINT32 pos = fullName.find_first_of('.') ;
         CHAR *collecion = &(fullName.at( pos + 1 )) ;
         const CHAR *cs = fullName.c_str() ;
         fullName.replace( pos, 1, 1, '\0' ) ;
         builder.append( CLS_FS_CS_NAME, cs ) ;
         builder.append( CLS_FS_COLLECTION_NAME, collecion ) ;
         builder.append( CLS_FS_KEYOBJ, _keyObjB() ) ;
         builder.append( CLS_FS_END_KEYOBJ, _keyObjE() ) ;
         builder.append( CLS_FS_NEEDDATA, _needData() ) ;
         obj = builder.obj() ;
         msg.header.messageLength = sizeof( MsgClsFSMetaReq ) +
                                    obj.objsize() ;
         _agent->syncSend( _selector.src(),
                           &( msg.header ), ( void * )obj.objdata(),
                           obj.objsize() ) ;
         fullName.replace( pos, 1, 1, '.' ) ;
         _timeout = 0 ;
         _needMoreDoc = TRUE ;

         PD_LOG( PDDEBUG, "Session[%s]: Send collection[%s]'s meta: %s",
                 sessionName(), fullName.c_str(), obj.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSDATADBS__META );
      return ;
   error :
      _disconnect() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__INDEX, "_clsDataDstBaseSession::_index" )
   void _clsDataDstBaseSession::_index ()
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__INDEX );
      MsgClsFSIndexReq msg ;
      msg.header.TID = CLS_TID( _sessionID ) ;
      msg.header.requestID = ++_requestID ;
      _agent->syncSend( _selector.src(), &msg ) ;
      _timeout = 0 ;
      PD_TRACE_EXIT ( SDB__CLSDATADBS__INDEX );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__NOTIFY, "_clsDataDstBaseSession::_notify" )
   void _clsDataDstBaseSession::_notify( CLS_FS_NOTIFY_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__NOTIFY );
      MsgClsFSNotify msg ;
      msg.header.TID = CLS_TID( _sessionID ) ;
      msg.header.requestID = ++_requestID ;
      msg.packet = _packet ;
      msg.type = type ;
      _agent->syncSend( _selector.src(), &msg ) ;
      _timeout = 0 ;
      PD_TRACE_EXIT ( SDB__CLSDATADBS__NOTIFY );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__MORE, "_clsDataDstBaseSession::_more" )
   BOOLEAN _clsDataDstBaseSession::_more( const MsgClsFSNotifyRes * msg,
                                          const CHAR *& itr,
                                          BOOLEAN isData )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__MORE );
      BOOLEAN res = FALSE ;
      if ( NULL == itr )
      {
         itr = ( const CHAR *)( &( msg->header ) ) + sizeof( MsgClsFSNotifyRes ) ;
      }
      else if ( isData )
      {
         itr += ossRoundUpToMultipleX( *( ( UINT32 * )itr ),
                                       sizeof( UINT32 ) ) ;
      }
      else
      {
         itr += *(UINT32*)( itr + sizeof( DPS_LSN_OFFSET ) * 2 ) ;
      }

      if ( itr < ( CHAR * )
                 ( &( msg->header ) ) + msg->header.header.messageLength )
      {
         res = TRUE ;
      }
      PD_TRACE1 ( SDB__CLSDATADBS__MORE, PD_PACK_INT(res) );
      PD_TRACE_EXIT ( SDB__CLSDATADBS__MORE );
      return res ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__ADDDCL, "_clsDataDstBaseSession::_addCollection" )
   UINT32 _clsDataDstBaseSession::_addCollection ( const CHAR * pCollectionName )
   {
      SDB_ASSERT ( _current <= _fullNames.size(), "current is error" ) ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__ADDDCL );
      UINT32 index = _current + 1 ;
      while ( index < _fullNames.size() )
      {
         if ( 0 == ossStrcmp(_fullNames[index].c_str(), pCollectionName ) )
         {
            PD_TRACE_EXIT ( SDB__CLSDATADBS__ADDDCL );
            return 0 ;
         }
         ++index ;
      }
      _fullNames.push_back ( pCollectionName ) ;
      PD_TRACE_EXIT ( SDB__CLSDATADBS__ADDDCL );
      return 1 ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__RMCL, "_clsDataDstBaseSession::_removeCollection" )
   UINT32 _clsDataDstBaseSession::_removeCollection ( const CHAR * pCollectionName )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__RMCL );
      UINT32 nDelNum = 0 ;

      vector<string>::iterator it = _fullNames.begin() ;
      UINT32 index = 0 ;

      if ( _current >= _fullNames.size() )
      {
         goto done ;
      }

      while ( it != _fullNames.end() )
      {
         if ( index > _current &&
              0 == ossStrcmp( (*it).c_str(), pCollectionName ) )
         {
            _fullNames.erase ( it ) ;
            nDelNum++ ;
            break ;
         }
         ++it ;
         ++index ;
      }

   done:
      PD_TRACE1 ( SDB__CLSDATADBS__RMCL,  PD_PACK_UINT(nDelNum) );
      PD_TRACE_EXIT ( SDB__CLSDATADBS__RMCL );
      return nDelNum ;
   }

   UINT32 _clsDataDstBaseSession::_removeValidCLs( const vector< string > &validCLs )
   {
      UINT32 removedNum = 0 ;

      for ( UINT32 i = 0 ; i < validCLs.size() ; ++i )
      {
         vector<string>::iterator it = _fullNames.begin() ;
         while( it != _fullNames.end() )
         {
            if ( *it == validCLs[i] )
            {
               _fullNames.erase( it ) ;
               ++removedNum ;
               break ;
            }
            ++it ;
         }
      }

      return removedNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__RMCS, "_clsDataDstBaseSession::_removeCS" )
   UINT32 _clsDataDstBaseSession::_removeCS ( const CHAR * pCSName )
   {
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__RMCS );

      UINT32 nDelNum = 0 ;
      vector<string>::iterator it = _fullNames.begin() ;
      UINT32 index = 0 ;
      UINT32 nameLen = ossStrlen( pCSName ) ;

      if ( _current >= _fullNames.size() )
      {
         goto done ;
      }

      while ( it != _fullNames.end() )
      {
         if ( index > _current &&
              0 == ossStrncmp( (*it).c_str(), pCSName, nameLen ) &&
              '.' == (*it).c_str()[nameLen] )
         {
            it = _fullNames.erase ( it ) ;
            nDelNum++ ;
            continue ;
         }

         ++it ;
         ++index ;
      }

   done:
      PD_TRACE1 ( SDB__CLSDATADBS__RMCS, PD_PACK_UINT(nDelNum) );
      PD_TRACE_EXIT ( SDB__CLSDATADBS__RMCS );
      return nDelNum ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__EXTMETA, "_clsDataDstBaseSession::_extractMeta" )
   INT32 _clsDataDstBaseSession::_extractMeta( const CHAR *objdata,
                                               string &cs,
                                               string &collection,
                                               UINT32 &pageSize,
                                               UINT32 &attributes,
                                               INT32 &lobPageSize,
                                               DMS_STORAGE_TYPE &csType,
                                               UTIL_COMPRESSOR_TYPE &compType,
                                               BSONObj &extOptions )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__EXTMETA );
      try
      {
         BSONObj obj( objdata ) ;
         BSONElement pageEle ;
         BSONElement collecionEle ;
         BSONElement ele ;
         BSONElement attri ;
         BSONElement compressorType ;
         BSONElement lobPageEle ;
         BSONElement typeEle ;
         BSONElement extOptEle ;
         BSONElement csEle = obj.getField( CLS_FS_CS_NAME ) ;
         PD_LOG( PDDEBUG, "Session[%s]: get meta data: %s", sessionName(),
                 obj.toString().c_str() ) ;
         if ( csEle.eoo() || String != csEle.type() )
         {
            goto error ;
         }
         cs = csEle.String() ;

         collecionEle = obj.getField( CLS_FS_COLLECTION_NAME ) ;
         if ( collecionEle.eoo() || String != collecionEle.type() )
         {
            goto error ;
         }
         collection = collecionEle.String() ;

         ele = obj.getField( CLS_FS_CS_META_NAME ) ;
         if ( ele.eoo() || !ele.isABSONObj() )
         {
            goto error ;
         }
         pageEle = ele.embeddedObject().getField( CLS_FS_PAGE_SIZE ) ;
         if ( pageEle.eoo() || NumberInt != pageEle.type() )
         {
            goto error ;
         }
         pageSize = pageEle.Int() ;

         attri = ele.embeddedObject().getField( CLS_FS_ATTRIBUTES ) ;
         if ( attri.eoo() || !attri.isNumber() )
         {
            attributes = 0 ;
         }
         else
         {
            attributes = attri.Number() ;
         }

         compressorType = ele.embeddedObject().getField( CLS_FS_COMP_TYPE );
         if ( compressorType.eoo() || NumberInt != compressorType.type() )
         {
            goto error ;
         }
         compType = (UTIL_COMPRESSOR_TYPE)compressorType.Int() ;

         extOptEle = ele.embeddedObject().getField( CLS_FS_EXT_OPTION ) ;
         if ( Object == extOptEle.type() )
         {
            extOptions = extOptEle.Obj() ;
         }

         lobPageEle =  ele.embeddedObject().getField( CLS_FS_LOB_PAGE_SIZE ) ;
         if ( NumberInt != lobPageEle.type() && !lobPageEle.eoo() )
         {
            goto error ;
         }
         else if ( NumberInt == lobPageEle.type() )
         {
            lobPageSize = lobPageEle.numberInt() ;
         }
         else
         {
            lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         }

         typeEle = ele.embeddedObject().getField( CLS_FS_CS_TYPE ) ;
         if ( !typeEle.eoo() && NumberInt != typeEle.type() )
         {
            goto error ;
         }
         else if ( NumberInt == typeEle.type() )
         {
            if (  typeEle.numberInt() >= DMS_STORAGE_NORMAL &&
                  typeEle.numberInt() < DMS_STORAGE_DUMMY )
            {
               csType = (DMS_STORAGE_TYPE)( typeEle.numberInt() ) ;
            }
            else
            {
               goto error ;
            }
         }
         else
         {
            csType= DMS_STORAGE_NORMAL ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSDATADBS__EXTMETA, rc );
      return rc ;
   error:
      rc = SDB_INVALIDARG ;
      PD_LOG( PDWARNING, "Session[%s]: failed to parse msg", sessionName() ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__EXTINX, "_clsDataDstBaseSession::_extractIndex" )
   INT32 _clsDataDstBaseSession::_extractIndex( const CHAR *objdata,
                                                vector<BSONObj> &indexes,
                                                BOOLEAN &noMore )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS__EXTINX );
      try
      {
         BSONObj obj( objdata ) ;
         PD_LOG( PDDEBUG, "Session[%s]: recv indexes [%s]", sessionName(),
                 obj.toString().c_str() ) ;
         BSONElement ele = obj.getField( CLS_FS_NOMORE ) ;
         if ( ele.eoo() || Bool != ele.type() )
         {
            PD_LOG( PDDEBUG, "Session[%s]: filed to parse nomore field",
                    sessionName() ) ;
            goto error ;
         }
         noMore = ele.Bool() ;
         if ( noMore )
         {
            goto done ;
         }
         ele = obj.getField( CLS_FS_INDEXES ) ;
         if ( ele.eoo() || Array != ele.type() ||
              !ele.isABSONObj() )
         {
            PD_LOG( PDDEBUG, "Session[%s]: filed to parse indexes field",
                    sessionName() ) ;
            goto error ;
         }
         BSONObjIterator i( ele.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement index = i.next() ;
            if ( index.eoo() || !index.isABSONObj() )
            {
               PD_LOG( PDDEBUG, "Session[%s]: filed to parse index field",
                       sessionName() ) ;
               goto error ;
            }
            indexes.push_back( index.embeddedObject() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSDATADBS__EXTINX, rc );
      return rc ;
   error:
      rc = SDB_INVALIDARG ;
      PD_LOG( PDWARNING, "Session[%s]: failed to parse msg", sessionName() ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS_HNDMETARES, "_clsDataDstBaseSession::handleMetaRes" )
   INT32 _clsDataDstBaseSession::handleMetaRes( NET_HANDLE handle,
                                                MsgHeader* header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS_HNDMETARES );
      MsgClsFSMetaRes *msg = ( _MsgClsFSMetaRes * )header ;
      if ( CLS_FS_STATUS_META != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local statsus:%d",
                 sessionName(), _status ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }

      CHECK_REQUEST_ID ( msg->header.header, _requestID ) ;

      _selector.clearTime() ;

      if ( SDB_DMS_CS_NOTEXIST == msg->header.res ||
           SDB_DMS_NOTEXIST == msg->header.res )
      {
         _status = CLS_FS_STATUS_INDEX ;
         _index() ;
         goto done ;
      }

      try
      {
         INT32 rc = SDB_OK ;
         string cs ;
         string collection ;
         UINT32 pageSize = 0 ;
         UINT32 attributes = 0 ;
         UTIL_COMPRESSOR_TYPE compType = UTIL_COMPRESSOR_INVALID ;
         BSONObj extOptions ;
         CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = { 0 } ;
         CHAR *objdata = ( CHAR *)( &( msg->header ) ) +
                                    sizeof( MsgClsFSMetaRes ) ;
         INT32 lobPageSize = 0 ;
         DMS_STORAGE_TYPE csType = DMS_STORAGE_NORMAL ;
         if ( SDB_OK != _extractMeta( objdata,
                                      cs, collection,
                                      pageSize,
                                      attributes,
                                      lobPageSize,
                                      csType,
                                      compType,
                                      extOptions ) )
         {
            _disconnect() ;
            goto done ;
         }

         ossSnprintf( fullName, DMS_COLLECTION_FULL_NAME_SZ,
                      "%s.%s", cs.c_str(), collection.c_str() ) ;
         if ( 0 != _fullNames.at( _current ).compare( fullName ) )
         {
            PD_LOG( PDWARNING, "Session[%s]: ignore msg. msg meta: %s, "
                    "local: %s", sessionName(), fullName,
                    _fullNames.at( _current ).c_str() ) ;
            goto done ;
         }

         PD_LOG( PDEVENT, "Session[%s]: Begin to sync collection[%s]",
                 sessionName(), fullName ) ;

         rc = _replayer.replayCrtCS( cs.c_str(), pageSize, lobPageSize,
                                     csType, eduCB() ) ;
         rc = _replayer.replayCrtCollection( fullName, attributes,
                                             eduCB(), compType,
                                             ( extOptions.isEmpty() ?
                                               NULL : &extOptions ) ) ;
         if ( SDB_OK != rc && SDB_DMS_EXIST != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to create collection"
                    "[%s], rc: %d", sessionName(), fullName, rc ) ;
            _disconnect() ;
            goto done ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexcepted exception: %s",
                 sessionName(), e.what() ) ;
         _disconnect() ;
         goto done ;
      }
      _status = CLS_FS_STATUS_INDEX ;
      _index() ;

   done:
      if ( eduCB()->getLsnCount() > 0 )
      {
         _lastOprLSN = eduCB()->getEndLsn() ;
      }
      PD_TRACE_EXIT ( SDB__CLSDATADBS_HNDMETARES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS_HNDINXRES2, "_clsDataDstBaseSession::handleIndexRes" )
   INT32 _clsDataDstBaseSession::handleIndexRes( NET_HANDLE handle,
                                                 MsgHeader* header )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS_HNDINXRES2 );
      MsgClsFSIndexRes *msg = ( MsgClsFSIndexRes * )header ;

      CHECK_REQUEST_ID ( msg->header.header, _requestID ) ;

      if ( CLS_FS_STATUS_INDEX != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local statsus:%d",
                 sessionName(), _status ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }
      else
      {
         _selector.clearTime() ;
         BSONObj obj ;
         BOOLEAN noMore = FALSE ;
         vector<BSONObj> indexes ;
         CHAR *objdata = ( CHAR * )( &( msg->header.header ) ) +
                                     sizeof( MsgClsFSIndexRes )  ;
         if ( SDB_OK != _extractIndex( objdata,
                                       indexes, noMore ) )
         {
            _disconnect() ;
            goto done ;
         }
         if ( !noMore )
         {
            vector<BSONObj>::iterator itr = indexes.begin() ;
            for ( ; itr != indexes.end(); itr++ )
            {
               _replayer.replayIXCrt( _fullNames.at( _current ).c_str(),
                                      *itr, eduCB() ) ;
            }
         }

         ++_packet ;
         _status = CLS_FS_STATUS_NOTIFY_LOG ;
         _notify( CLS_FS_NOTIFY_TYPE_LOG ) ;
      }
   done:
      if ( eduCB()->getLsnCount() > 0 )
      {
         _lastOprLSN = eduCB()->getEndLsn() ;
      }
      PD_TRACE_EXIT ( SDB__CLSDATADBS_HNDINXRES2 );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS_HNDNTFRES, "_clsDataDstBaseSession::handleNotifyRes" )
   INT32 _clsDataDstBaseSession::handleNotifyRes( NET_HANDLE handle,
                                                  MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSDATADBS_HNDNTFRES );
      MsgClsFSNotifyRes *msg = ( MsgClsFSNotifyRes * )header ;

      CHECK_REQUEST_ID ( msg->header.header , _requestID ) ;

      if ( !_isReady() )
      {
         goto done ;
      }
      else if ( CLS_FS_STATUS_NOTIFY_LOG != _status &&
                CLS_FS_STATUS_NOTIFY_DOC != _status &&
                CLS_FS_STATUS_NOTIFY_LOB != _status &&
                CLS_FS_STATUS_END != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local status:%d",
                 sessionName(), _status ) ;
         goto done ;
      }
      else if ( msg->packet != _packet )
      {
         PD_LOG( PDDEBUG, "Session[%s]: ignore msg, invalid packetid: %d, "
                 "local:%d", sessionName(), msg->packet, _packet ) ;
         goto done ;
      }

      if ( !_onNotify( msg ) )
      {
         goto done ;
      }

      if ( CLS_FS_NOTIFY_TYPE_DOC == msg->type )
      {
         ++_packet ;
         if ( CLS_FS_EOF != msg->eof )
         {
            _status = CLS_FS_STATUS_NOTIFY_LOG ;
            _notify( CLS_FS_NOTIFY_TYPE_LOG ) ;

            rc = _replayDoc( msg ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to replay doc from remote:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            _status = CLS_FS_STATUS_NOTIFY_LOB ;
            _notify( CLS_FS_NOTIFY_TYPE_LOB ) ;
            _needMoreDoc = FALSE ;
         }
      }
      else if ( CLS_FS_NOTIFY_TYPE_LOG == msg->type )
      {
         ++_packet ;
         if ( CLS_FS_EOF != msg->eof )
         {
            _notify( CLS_FS_NOTIFY_TYPE_LOG ) ;
            rc = _replayLog( msg ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to replay log from remote:%d", rc ) ;
               goto error ;
            }
         }
         else if ( _needMoreDoc )
         {
            _status = CLS_FS_STATUS_NOTIFY_DOC ;
            _notify( CLS_FS_NOTIFY_TYPE_DOC ) ;
         }
         else
         {
            _status = CLS_FS_STATUS_NOTIFY_LOB ;
            _notify( CLS_FS_NOTIFY_TYPE_LOB ) ;
         }
      }
      else if ( CLS_FS_NOTIFY_TYPE_LOB == msg->type )
      {
         ++_packet ;
         if ( CLS_FS_EOF != msg->eof )
         {
            _status = CLS_FS_STATUS_NOTIFY_LOG ;
            _notify( CLS_FS_NOTIFY_TYPE_LOG ) ;

            rc = _replayLob( msg ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to replay lob from remote:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            _expectLSN = msg->lsn ;
            _status = CLS_FS_STATUS_META ;

            PD_LOG( PDEVENT, "Session[%s]: Sync collection[%s] finished, "
                    "expect lsn: %d.%lld", sessionName(),
                    _fullNames[ _current ].c_str(), _expectLSN.version,
                    _expectLSN.offset ) ;

            ++_current ;
            _notify( CLS_FS_NOTIFY_TYPE_OVER ) ;
            _meta() ;
         }
      }
      else
      {
          PD_LOG( PDWARNING, "Session[%s]: ignore msg", sessionName() ) ;
      }

   done:
      if ( eduCB()->getLsnCount() > 0 )
      {
         _lastOprLSN = eduCB()->getEndLsn() ;
      }
      PD_TRACE_EXIT ( SDB__CLSDATADBS_HNDNTFRES );
      return SDB_OK ;
   error:
      _disconnect () ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__REPLAYDOC, "_clsDataDstBaseSession::_replayDoc" )
   INT32 _clsDataDstBaseSession::_replayDoc( const MsgClsFSNotifyRes *msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSDATADBS__REPLAYDOC ) ;
      const CHAR *itr = NULL ;
      while ( _more( msg, itr, TRUE ) )
      {
         try
         {
            BSONObj obj( itr ) ;
            rc = _replayer.replayInsert( _fullNames.at( _current ).c_str(),
                                         obj, eduCB() ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                    sessionName(), e.what() ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSDATADBS__REPLAYDOC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__REPLAYLOB, "_clsDataDstBaseSession::_repalyLob" )
   INT32 _clsDataDstBaseSession::_replayLob( const MsgClsFSNotifyRes *msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSDATADBS__REPLAYLOB ) ;
      const CHAR *itr = ( const CHAR * )msg ;
      const MsgLobTuple *tuple = NULL ;
      const bson::OID *oid = NULL ;
      const CHAR *data = NULL ;
      const CHAR *fullName = NULL ;

      if ( ( UINT32 )msg->header.header.messageLength <=
           sizeof( MsgClsFSNotifyRes ) )
      {
         PD_LOG( PDERROR, "invalid msg length:%d",
                 msg->header.header.messageLength ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      SDB_ASSERT( _current < _fullNames.size(), "impossible" ) ;
      fullName = _fullNames.at( _current ).c_str() ;
      itr += sizeof( MsgClsFSNotifyRes ) ;
      while ( _more( msg, itr, oid,
                     tuple, data ) )
      {
         rc = _replayer.replayWriteLob( fullName,
                                        *oid, tuple->columns.sequence,
                                        0, tuple->columns.len, data,
                                        eduCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSDATADBS__REPLAYLOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsDataDstBaseSession::_more( const MsgClsFSNotifyRes *msg,
                                          const CHAR *&itr,
                                          const bson::OID *&oid,
                                          const MsgLobTuple *&tuple,
                                          const CHAR *&data )
   {
      BOOLEAN rc = FALSE ;
      UINT32 lastSize = 0 ;

      if ( NULL == itr )
      {
         goto done ;
      }

      lastSize = msg->header.header.messageLength -
                ( itr - ( const CHAR * )msg ) ;
      if ( lastSize < ( sizeof( MsgLobTuple ) + sizeof( bson::OID ) ) )
      {
         goto done ;
      }
      else
      {
         oid = ( const bson::OID * )itr ;
         tuple = ( const MsgLobTuple * )( itr + sizeof( bson::OID ) ) ;
         UINT32 realLen = sizeof( bson::OID ) +
                          sizeof( MsgLobTuple ) +
                          tuple->columns.len ;
         UINT32 alignedLen = ossRoundUpToMultipleX( realLen, 4 ) ;
         UINT32 skipLen = 0 ;

         if ( alignedLen <= lastSize )
         {
            skipLen = alignedLen ;
         }
         else if ( realLen <= lastSize )
         {
            skipLen = realLen ;
         }
         else
         {
            goto done ;
         }

         data = itr + sizeof( MsgLobTuple ) + sizeof( bson::OID ) ;
         rc = TRUE ;
         if ( lastSize == skipLen )
         {
            itr = NULL ;
         }
         else
         {
            itr += skipLen ;
         }
      }
   done:
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDATADBS__REPLAYLOG, "_clsDataDstBaseSession::_replayLog" )
   INT32 _clsDataDstBaseSession::_replayLog( const MsgClsFSNotifyRes *msg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSDATADBS__REPLAYLOG ) ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      const CHAR *itr = NULL ;
      while ( _more( msg, itr, FALSE ) )
      {
         dpsLogRecordHeader *header = (dpsLogRecordHeader *)itr;
         SDB_ASSERT( 0 == header->_reserved1, "impossible" ) ;

         if ( !_replayer.isDPSEnabled() )
         {
            if ( dpsCB->expectLsn().compareOffset( header->_lsn ) > 0 )
            {
               SDB_ASSERT( FALSE , "header lsn is less than expect" ) ;
               PD_LOG( PDWARNING, "Session[%s]: expect lsn[%lld] more "
                       "than header lsn[%lld]", sessionName(),
                       dpsCB->expectLsn().offset, header->_lsn ) ;
               goto error ;
            }

            if ( 0 != dpsCB->expectLsn().compareOffset( header->_lsn )
                 && SDB_OK != dpsCB->move ( header->_lsn,
                                            header->_version ) )
            {
               PD_LOG ( PDERROR, "Session[%s]: failed to move lsn[%d,%lld]",
                        sessionName(), header->_version,
                        header->_lsn ) ;
               goto error ;
            }
         }

         rc = _replayer.replay( header, eduCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDWARNING, "Session[%s] replay dps log record failed"
                    "[rc:%d]", sessionName(), rc ) ;
            goto error ;
         }

         if ( LOG_TYPE_CL_CRT == header->_type ||
              LOG_TYPE_CL_TRUNC == header->_type )
         {
            dpsLogRecord record ;
            dpsLogRecord::iterator itrName ;
            rc = record.load( itr ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            itrName = record.find( DPS_LOG_PUBLIC_FULLNAME ) ;
            if ( !itrName.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag "
                       "fullname", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _addCollection ( itrName.value() ) ;
         }
         else if ( LOG_TYPE_CL_DELETE == header->_type )
         {
            dpsLogRecord record ;
            dpsLogRecord::iterator itrName ;
            rc = record.load( itr ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            itrName = record.find( DPS_LOG_PUBLIC_FULLNAME) ;
            if ( !itrName.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag "
                       "fullname", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _removeCollection ( itrName.value() ) ;
         }
         else if ( LOG_TYPE_CS_DELETE == header->_type )
         {
            dpsLogRecord record ;
            dpsLogRecord::iterator itrName ;
            rc = record.load( itr ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            itrName = record.find( DPS_LOG_CSDEL_CSNAME ) ;
            if ( !itrName.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag "
                       "fullname", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            _removeCS ( itrName.value() ) ;
         }
         else if ( LOG_TYPE_CL_RENAME == header->_type )
         {
            dpsLogRecord record ;
            dpsLogRecord::iterator cs, oldname, newname ;
            std::string fullname ;
            std::string oldFullName ;
            rc = record.load( itr ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            cs = record.find( DPS_LOG_CLRENAME_CSNAME ) ;
            if ( !cs.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag cs",
                       sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            oldname = record.find( DPS_LOG_CLRENAME_CLOLDNAME ) ;
            if ( !oldname.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag oldname",
                       sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            newname = record.find( DPS_LOG_CLRENAME_CLNEWNAME ) ;
            if ( !newname.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to find tag newname",
                       sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            oldFullName = cs.value() ;
            oldFullName += "." ;
            oldFullName += oldname.value() ;
            if ( _removeCollection ( oldFullName.c_str() ) > 0 )
            {
               std::string newFullName = cs.value() ;
               newFullName += "." ;
               newFullName += newname.value() ;
               _addCollection ( newFullName.c_str() ) ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSDATADBS__REPLAYLOG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
   _clsFSDstSession : implement
   */
   BEGIN_OBJ_MSG_MAP( _clsFSDstSession, _clsDataDstBaseSession )
      ON_MSG( MSG_CLS_FULL_SYNC_BEGIN_RES, handleBeginRes )
      ON_MSG( MSG_CLS_FULL_SYNC_END_RES, handleEndRes )
      ON_MSG( MSG_CLS_FULL_SYNC_TRANS_RES, handleSyncTransRes )
   END_OBJ_MSG_MAP()

   _clsFSDstSession::_clsFSDstSession( UINT64 sessionID,
                                       _netRouteAgent *agent )
   :_clsDataDstBaseSession( sessionID, agent ),
   _fsStep( CLS_FS_STEP_NONE )
   {
      _repeatCount = CLS_FS_MAX_REPEAT_CNT ;
      _hasRegFullsyc = FALSE ;
   }

   _clsFSDstSession::~_clsFSDstSession()
   {

   }

   SDB_SESSION_TYPE _clsFSDstSession::sessionType() const
   {
      return SDB_SESSION_FS_DST ;
   }

   EDU_TYPES _clsFSDstSession::eduType() const
   {
      return  EDU_TYPE_REPLAGENT ;
   }

   BOOLEAN _clsFSDstSession::canAttachMeta() const
   {
      return _status != CLS_FS_STATUS_BEGIN ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS_HNDBGRES, "_clsFSDstSession::handleBeginRes" )
   INT32 _clsFSDstSession::handleBeginRes( NET_HANDLE handle,
                                           MsgHeader* header )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSFSDS_HNDBGRES );

      MsgClsFSBeginRes *msg = ( MsgClsFSBeginRes * )header ;
      _expectLSN = msg->lsn ;
      SDB_DPSCB *dpsCB = NULL ;
      if ( CLS_FS_STATUS_BEGIN != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local statsus:",
                 sessionName(), _status ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }

      CHECK_REQUEST_ID ( msg->header.header, _requestID ) ;

      if ( SDB_OK != msg->header.res )
      {
         PD_LOG ( PDWARNING, "Session[%s]: Node[%d] refused full sync "
                  "seq[rc:%d]", sessionName(), _selector.src().columns.nodeID,
                  msg->header.res ) ;
         _selector.addToBlakList ( _selector.src() ) ;
         _selector.clearSrc () ;
         goto done ;
      }

      _selector.clearTime() ;
      try
      {
         BSONObj dataObj( (const CHAR*)header + sizeof( MsgClsFSBeginRes ) ) ;
         if ( SDB_OK != _extractBeginRspBody( dataObj ) )
         {
            PD_LOG( PDWARNING, "Session[%s]: Failed to extract body data, "
                    "disconnect session", sessionName() ) ;
            _disconnect() ;
            goto done ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: Extract body data occur exception: %s",
                 sessionName(), e.what() ) ;
         _disconnect() ;
         goto done ;
      }

      PD_LOG( PDEVENT, "Session[%s]: Established the full sync session with "
              "node[%s], Remote Expect LSN:[%d,%lld]. "
              "Then close all shard connections", sessionName(),
              routeID2String( _selector.src() ).c_str(),
              _expectLSN.version, _expectLSN.offset ) ;
      sdbGetClsCB()->getShardRouteAgent()->disconnectAll() ;

      sdbGetShardCB()->getCataAgent()->lock_w() ;
      sdbGetShardCB()->getCataAgent()->clearAll() ;
      sdbGetShardCB()->getCataAgent()->release_w() ;

      dpsCB = pmdGetKRCB()->getDPSCB() ;
      _status = CLS_FS_STATUS_META ;
      sdbGetTransCB()->clearTransInfo() ;
      sdbGetTransCB()->setIsNeedSyncTrans( FALSE ) ;
      pmdGetStartup().ok( FALSE ) ;
      dpsCB->move ( 0, 0 ) ;
      /*
      Don't to move the lsn to expect to prevent the node change to primary
      when the primary node crashed
      if ( SDB_OK != dpsCB->move( _expectLSN.offset, _expectLSN.version ) )
      {
         PD_LOG( PDWARNING, "Session[%s]: Failed to move dps[%d,%lld], "
                 "disconnect session", sessionName(), _expectLSN.version,
                 _expectLSN.offset ) ;
         _disconnect() ;
         goto done ;
      }
      */
      {
         DPS_LSN expect = dpsCB->expectLsn() ;
         PD_LOG( PDEVENT, "Session[%s]: begin to get meta. expect lsn is "
                 "[ver:%d][offset:%lld]", sessionName(), expect.version,
                 expect.offset ) ;

         rtnDBCleaner cleanner ;
         cleanner.setUDFValidCLs( _validCLs ) ;
         if ( SDB_OK != cleanner.doOpr( eduCB() ) )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to cleanup data.",
                    sessionName() ) ;
            _disconnect () ;
            goto done ;
         }
         _removeValidCLs( cleanner.getUDFValidCLs() ) ;
      }

      {
         CS_INFO_TUPLES::iterator itCS = _mapEmptyCS.begin() ;
         while ( itCS != _mapEmptyCS.end() )
         {
            rc = _replayer.replayCrtCS( itCS->first.c_str(),
                                        itCS->second.pageSize,
                                        itCS->second.lobPageSize,
                                        itCS->second.type,
                                        eduCB() ) ;
            if ( SDB_OK != rc && SDB_DMS_CS_EXIST != rc )
            {
               PD_LOG( PDWARNING, "Session[%s]: create empty collection "
                       "space[%s] failed, rc: %d", sessionName(),
                       itCS->first.c_str(), rc ) ;
               _disconnect() ;
               goto done ;
            }
            ++itCS ;
         }
         _mapEmptyCS.clear() ;
      }
      _meta() ;

   done:
      PD_TRACE_EXIT ( SDB__CLSFSDS_HNDBGRES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS_HNDENDRES, "_clsFSDstSession::handleEndRes" )
   INT32 _clsFSDstSession::handleEndRes( NET_HANDLE handle,
                                         MsgHeader* header )
   {
      PD_TRACE_ENTRY ( SDB__CLSFSDS_HNDENDRES );

      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      DPS_LSN lsn = dpsCB->expectLsn() ;

      if ( CLS_FS_STATUS_END != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: status[%d] is not expect[%d]",
                 sessionName(), _status, CLS_FS_STATUS_END ) ;
         _disconnect () ;
         goto done ;
      }

      dmsCB->clearSUCaches( DMS_EVENT_MASK_ALL ) ;

      _quit = TRUE ;

      PD_LOG( PDEVENT, "Session[%s]: Full sync has been done, expect lsn:"
              "[%u,%llu]", sessionName(), lsn.version, lsn.offset ) ;

   done:
      PD_TRACE_EXIT ( SDB__CLSFSDS_HNDENDRES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS__BEGIN, "_clsFSDstSession::_begin" )
   void _clsFSDstSession::_begin()
   {
      PD_TRACE_ENTRY ( SDB__CLSFSDS__BEGIN ) ;

      ++_repeatCount ;
      if ( _repeatCount >= CLS_FS_MAX_REPEAT_CNT )
      {
         MsgClsFSBegin msg ;
         msg.type = CLS_FS_TYPE_IN_SET ;
         msg.header.TID = CLS_TID( _sessionID ) ;
         MsgRouteID lastID = _selector.src() ;
         MsgRouteID src = _selector.selected( TRUE ) ;

         if ( MSG_INVALID_ROUTEID != lastID.value &&
              lastID.value != src.value )
         {
            MsgHeader disMsg ;
            disMsg.messageLength = sizeof( MsgHeader ) ;
            disMsg.routeID.value = MSG_INVALID_ROUTEID ;
            disMsg.TID = CLS_TID( _sessionID ) ;
            disMsg.requestID = ++_requestID ;
            disMsg.opCode = MSG_BS_DISCONNECT ;
            _agent->syncSend( lastID, &disMsg ) ;
         }

         BSONObj bodyObj ;
         if ( MSG_INVALID_ROUTEID != src.value &&
              SDB_OK == _buildBegingBody( bodyObj ) )
         {
            msg.header.messageLength += bodyObj.objsize() ;
            msg.header.requestID = ++_requestID ;
            if ( SDB_OK == _agent->syncSend( src, &(msg.header),
                                             (void*)bodyObj.objdata(),
                                             (UINT32)bodyObj.objsize() ) )
            {
               _repeatCount = 0 ;
            }
            _timeout = 0 ;
         }
      }

      PD_TRACE_EXIT ( SDB__CLSFSDS__BEGIN );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS__END, "_clsFSDstSession::_end" )
   void _clsFSDstSession::_end()
   {
      PD_TRACE_ENTRY ( SDB__CLSFSDS__END );
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      DPS_LSN lsn = dpsCB->expectLsn() ;
      DPS_LSN invalidLsn ;
      MsgClsFSEnd msg ;

      if ( CLS_FS_STEP_END == _fsStep )
      {
         PD_LOG( PDEVENT, "Session[%s]: End to pull transaction log or "
                 "cached log", sessionName() ) ;
         goto doend ;
      }
      else if ( CLS_FS_STEP_NONE == _fsStep )
      {
         if ( !sdbGetTransCB()->isTransOn() &&
              DPS_INVALID_LSN_OFFSET != dpsCB->getCurrentLsn().offset )
         {
            _fsStep = CLS_FS_STEP_END ;
            goto doend ;
         }
         PD_LOG( PDEVENT, "Session[%s]: Begin to pull transaction log or "
                 "cached log, expect lsn:[%u,%llu]", sessionName(),
                 _expectLSN.version, _expectLSN.offset ) ;
      }
      else
      {
         invalidLsn = lsn ;
      }

      _pullTransLog( invalidLsn ) ;
      goto done;

   doend:
      if ( 0 != _expectLSN.compare(lsn) )
      {
         INT32 rcTmp = dpsCB->move ( _expectLSN.offset, _expectLSN.version ) ;
         if ( rcTmp )
         {
            PD_LOG ( PDERROR, "Session[%s]: failed to move lsn[%d,%lld]",
                     sessionName(), _expectLSN.version, _expectLSN.offset ) ;
            goto error ;
         }
         PD_LOG( PDEVENT, "Session[%s]: Move repl-log to %d.%lld",
                 sessionName(), _expectLSN.version, _expectLSN.offset ) ;
      }
      msg.header.TID = CLS_TID( _sessionID ) ;
      msg.header.requestID = ++_requestID ;
      _agent->syncSend( _selector.src(), &msg ) ;
      _timeout = 0 ;

   done :
      _status = CLS_FS_STATUS_END ;
      PD_TRACE_EXIT ( SDB__CLSFSDS__END );
      return ;
   error :
      _disconnect () ;
      goto done ;
   }

   BSONObj _clsFSDstSession::_keyObjB ()
   {
      return BSONObj() ;
   }

   BSONObj _clsFSDstSession::_keyObjE ()
   {
      return BSONObj() ;
   }

   INT32 _clsFSDstSession::_needData () const
   {
      return 1 ;
   }

   INT32 _clsFSDstSession::_dataSessionType () const
   {
      return CLS_FS_TYPE_IN_SET ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS__ISREADY, "_clsFSDstSession::_isReady" )
   BOOLEAN _clsFSDstSession::_isReady ()
   {
      BOOLEAN result = TRUE ;
      PD_TRACE_ENTRY ( SDB__CLSFSDS__ISREADY );

      if ( sdbGetReplCB()->primaryIsMe() )
      {
         PD_LOG( PDWARNING, "Session[%s] disconnect when self is primary",
                 sessionName() ) ;
         _disconnect () ;
         result = FALSE ;
         goto done ;
      }
      if ( CLS_FS_STATUS_BEGIN != _status &&
           _recvTimeout > CLS_SRC_SESSION_NO_MSG_TIME &&
           !sdbGetReplCB()->isAlive ( _selector.src() ) )
      {
         PD_LOG ( PDWARNING, "Session[%s] peer node sharing-beak, "
                  "disconnect", sessionName() ) ;
         _disconnect () ;
         result = FALSE ;
         goto done ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSFSDS__ISREADY );
      return result ;
   }

   void _clsFSDstSession::_onAttach()
   {
      if ( SDB_OK == sdbGetDMSCB()->registerFullSync( eduCB() ) )
      {
         _hasRegFullsyc = TRUE ;
      }
      else
      {
         PMD_SET_DB_STATUS( SDB_DB_FULLSYNC ) ;
      }
      sdbGetReplCB()->getFaultEvent()->signalAll( SDB_CLS_FULL_SYNC ) ;

      _begin() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSDS__ONDETACH, "_clsFSDstSession::_onDetach" )
   void _clsFSDstSession::_onDetach()
   {
      PD_TRACE_ENTRY ( SDB__CLSFSDS__ONDETACH );

      if ( CLS_FS_STATUS_END == _status && CLS_FS_STEP_END == _fsStep )
      {
         rtnDBFSPostCleaner fsCleaner ;
         fsCleaner.doOpr( eduCB() ) ;
      }
      else if ( CLS_FS_STATUS_BEGIN != _status )
      {
         pmdGetKRCB()->getDPSCB()->move( 0, 0 ) ;
      }

      PD_LOG( PDEVENT, "Session[%s]: start sync session.", sessionName() ) ;
      pmdGetKRCB()->getClsCB()->startInnerSession( CLS_REPL,
                                                   CLS_TID_REPL_SYC ) ;

      sdbGetReplCB()->getFaultEvent()->reset() ;
      if ( _hasRegFullsyc )
      {
         sdbGetDMSCB()->fullSyncDown( eduCB() ) ;
      }
      else
      {
         PMD_SET_DB_STATUS( SDB_DB_NORMAL ) ;
      }

      _disconnect() ;
      PD_TRACE_EXIT ( SDB__CLSFSDS__ONDETACH );
   }

   BOOLEAN _clsFSDstSession::_onNotify( MsgClsFSNotifyRes *pMsg )
   {
      if ( CLS_FS_NOTIFY_TYPE_LOB == pMsg->type &&
           CLS_FS_EOF == pMsg->eof &&
           (UINT32)(pMsg->header.header.messageLength) >
           sizeof( MsgClsFSNotifyRes ) )
      {
         rtnRUInfo info ;
         const CHAR *pCLName = _fullNames[_current].c_str() ;
         try
         {
            BSONObj objData( (const CHAR*)pMsg + sizeof( MsgClsFSNotifyRes ) ) ;
            BSONElement eleLSN = objData.getField( CLS_FS_COMMITLSN ) ;
            vector<BSONElement> vecLSN = eleLSN.Array() ;
            if ( vecLSN.size() < 3 )
            {
               goto done ;
            }
            info._dataCommitLSN = vecLSN[0].numberLong() ;
            info._idxCommitLSN = vecLSN[1].numberLong() ;
            info._lobCommitLSN = vecLSN[1].numberLong() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Session[%s]: Extract collection[%s]'s commit "
                    "info occur exception: %s", sessionName(), pCLName,
                    e.what() ) ;
            goto done ;
         }

         SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
         dmsStorageUnit *su = NULL ;
         const CHAR *pCLShort = NULL ;
         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         if ( SDB_OK == rtnResolveCollectionNameAndLock( pCLName, dmsCB, &su,
                                                         &pCLShort, suID ) )
         {
            dmsMBContext *pContext = NULL ;
            if ( SDB_OK == su->data()->getMBContext( &pContext,
                                                     pCLShort, SHARED ) )
            {
               pContext->mbStat()->_lastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET,
                                                            info._dataCommitLSN ) ;
               pContext->mbStat()->_idxLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET,
                                                               info._idxCommitLSN ) ;
               pContext->mbStat()->_lobLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET,
                                                               info._lobCommitLSN ) ;
               su->data()->releaseMBContext( pContext ) ;
            }
            dmsCB->suUnlock( suID ) ;
         }
      }

   done:
      return TRUE ;
   }

   void _clsFSDstSession::_pullTransLog( DPS_LSN &begin )
   {
      MsgClsFSTransSyncReq msgReq ;
      msgReq.header.TID = CLS_TID( _sessionID ) ;
      msgReq.header.requestID = ++_requestID ;
      msgReq.endExpect = _expectLSN;
      msgReq.begin = begin;
      _agent->syncSend( _selector.src(), &( msgReq.header ) ) ;
      _timeout = 0 ;
   }

   INT32 _clsFSDstSession::_extractBeginRspBody( const BSONObj &bodyObj )
   {
      INT32 rc = SDB_OK ;

      _fullNames.clear() ;
      _validCLs.clear() ;
      _mapEmptyCS.clear() ;

      BSONElement ele ;

      PD_LOG( PDDEBUG, "Session[%s]: recv begin response:[%s]",
              sessionName(), bodyObj.toString().c_str() ) ;

      try
      {
         ele = bodyObj.getField( CLS_FS_CSNAMES ) ;
         if ( Array != ele.type() )
         {
            PD_LOG( PDERROR, "Session[%s]: failed to parse csnames from "
                    "obj[%s]", sessionName(), bodyObj.toString().c_str() ) ;
            goto error ;
         }

         BSONObjIterator csIt( ele.embeddedObject() ) ;
         while ( csIt.more() )
         {
            BSONElement next = csIt.next() ;
            if ( Object != next.type() )
            {
               PD_LOG( PDERROR, "Session[%s]: parse a collection space "
                       "ele[%s] failed", sessionName(),
                       next.toString().c_str() ) ;
               goto error ;
            }
            else
            {
               BSONObj csObj = next.embeddedObject() ;
               BSONElement eleName = csObj.getField( CLS_FS_CSNAME ) ;
               BSONElement elePageSZ = csObj.getField( CLS_FS_PAGE_SIZE ) ;
               BSONElement eleLobPageSZ = csObj.getField( CLS_FS_LOB_PAGE_SIZE ) ;
               BSONElement eleType = csObj.getField( CLS_FS_CS_TYPE ) ;
               INT32 pageSz = 0 ;
               INT32 lobPageSz = DMS_DEFAULT_LOB_PAGE_SZ ;
               DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL ;

               if ( String != eleName.type() ||
                    NumberInt != elePageSZ.type() )
               {
                  PD_LOG( PDERROR, "Session[%s]: parse a collection space "
                          "ele[%s] failed", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               pageSz = elePageSZ.numberInt() ;

               if ( NumberInt != eleLobPageSZ.type() &&
                    !eleLobPageSZ.eoo() )
               {
                  PD_LOG( PDERROR, "Session[%s]: wrong type of lob "
                          "pagesize[%s] failed", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               else if ( NumberInt == eleLobPageSZ.type() )
               {
                  lobPageSz = eleLobPageSZ.numberInt() ;
               }

               if ( NumberInt != eleType.type() && !eleType.eoo() )
               {
                  PD_LOG( PDERROR, "Session[%s]: wrong type of storage "
                          "type[%s]", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               else if ( NumberInt == eleType.type() )
               {
                  INT32 typeVal = eleType.numberInt() ;
                  if ( typeVal >= DMS_STORAGE_NORMAL &&
                       typeVal < DMS_STORAGE_DUMMY )
                  {
                     type = (DMS_STORAGE_TYPE)typeVal ;
                  }
                  else
                  {
                     PD_LOG( PDERROR, "Storage type[%d] is invalid", typeVal ) ;
                     rc = SDB_SYS ;
                     goto done ;
                  }
               }

               _mapEmptyCS[ eleName.str() ] = clsCSInfoTuple( pageSz,
                                                              lobPageSz,
                                                              type ) ;
            }
         }

         ele = bodyObj.getField( CLS_FS_FULLNAMES ) ;
         if ( Array != ele.type() )
         {
            PD_LOG( PDWARNING, "Session[%s]: failed to parse collections "
                    "from obj[%s]", sessionName(),
                    bodyObj.toString().c_str() ) ;
            goto error ;
         }
         else
         {
            BSONObjIterator i( ele.embeddedObject() ) ;
            while ( i.more() )
            {
               BSONElement next = i.next() ;
               BSONElement name ;
               if ( next.eoo() || !next.isABSONObj() )
               {
                  PD_LOG( PDWARNING, "Session[%s]: failed to parse a "
                          "collection from obj[%s]", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               name = next.embeddedObject().getField( CLS_FS_FULLNAME ) ;
               if ( name.eoo() || String != name.type() )
               {
                  PD_LOG( PDWARNING, "Session[%s]: failed to parse a "
                          "collection from obj[%s]", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               _fullNames.push_back( name.String() ) ;
            }
         }

         ele = bodyObj.getField( CLS_FS_VALIDCLS ) ;
         if ( EOO == ele.type() )
         {
         }
         else if ( Array != ele.type() )
         {
            PD_LOG( PDWARNING, "Session[%s]: Failed to parse valid "
                    "collections from obj[%s]", sessionName(),
                    bodyObj.toString().c_str() ) ;
            goto error ;
         }
         else
         {
            BSONObjIterator itValid( ele.embeddedObject() ) ;
            while( itValid.more() )
            {
               BSONElement next = itValid.next() ;
               BSONElement name ;
               if ( next.eoo() || !next.isABSONObj() )
               {
                  PD_LOG( PDWARNING, "Session[%s]: Failed to parse a valid "
                          "collection from obj[%s]", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               name = next.embeddedObject().getField( CLS_FS_FULLNAME ) ;
               if ( name.eoo() || String != name.type() )
               {
                  PD_LOG( PDWARNING, "Session[%s]: Failed to parse a valid "
                          "collection from obj[%s]", sessionName(),
                          next.toString().c_str() ) ;
                  goto error ;
               }
               _validCLs.push_back( name.String() ) ;
               PD_LOG( PDEVENT, "Session[%s]: Collection[%s] is valid, don't "
                       "need to full sync", sessionName(), name.valuestr() ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: Occur exception: %s",
                 sessionName(), e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( SDB_OK == rc )
      {
         rc = SDB_INVALIDARG ;
      }
      goto done ;
   }

   INT32 _clsFSDstSession::_buildBegingBody( BSONObj &bodyObj )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      MON_CS_SIM_LIST csList ;
      MON_CS_SIM_LIST::iterator it ;

      BSONObjBuilder builder ;
      BSONArrayBuilder validCLBD( builder.subarrayStart( CLS_FS_VALIDCLS ) ) ;

      dmsCB->dumpInfo( csList, TRUE, FALSE, FALSE ) ;

      for ( it = csList.begin() ; it != csList.end() ; ++it )
      {
         const monCSSimple &csInfo = *it ;

         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            continue ;
         }
         dmsStorageUnitID suID = DMS_INVALID_SUID ;
         dmsStorageUnit *su = NULL ;
         rc = dmsCB->nameToSUAndLock( csInfo._name, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d",
                    csInfo._name, rc ) ;
            goto error ;
         }

         rtnRecoverUnit recoverUnit ;
         recoverUnit.init( su ) ;
         MAP_SU_STATUS items ;
         if ( 0 == recoverUnit.getValidCLItem( items ) )
         {
            dmsCB->suUnlock( suID ) ;
            continue ;
         }
         MAP_SU_STATUS::iterator itCL = items.begin() ;
         while( itCL != items.end() )
         {
            rtnRUInfo &info = itCL->second ;

            BSONObjBuilder clBuild( validCLBD.subobjStart() ) ;
            clBuild.append( CLS_FS_FULLNAME, itCL->first ) ;
            clBuild.append( CLS_FS_COMMITFLAG,
                            BSON_ARRAY( (INT32)info._dataCommitFlag <<
                                        (INT32)info._idxCommitFlag <<
                                        (INT32)info._lobCommitFlag ) ) ;
            clBuild.append( CLS_FS_COMMITLSN,
                            BSON_ARRAY( (INT64)info._dataCommitLSN <<
                                        (INT64)info._idxCommitLSN <<
                                        (INT64)info._lobCommitLSN ) ) ;
            clBuild.done() ;
            ++itCL ;
         }
         dmsCB->suUnlock( suID ) ;
      }

      validCLBD.done() ;
      bodyObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsFSDstSession::handleSyncTransRes( NET_HANDLE handle,
                                               MsgHeader * header )
   {
      INT32 rc = SDB_OK;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB();
      MsgClsFSTransSyncRes *pRsp = (MsgClsFSTransSyncRes *)header;
      rc = pRsp->header.res;
      CHAR *pOffset = (CHAR *)header + sizeof(MsgClsFSTransSyncRes);
      CHAR *pEnd = (CHAR *)header + header->messageLength;
      DPS_LSN expectLsn ;

      CHECK_REQUEST_ID ( pRsp->header.header , _requestID ) ;

      if ( CLS_FS_STATUS_END != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local status:%d",
                 sessionName(), _status ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }

      PD_RC_CHECK( rc, PDERROR, "Session[%s]: Failed to synchronise "
                   "transaction-log, rc: %d", sessionName(), rc );

      if ( CLS_FS_EOF == pRsp->eof )
      {
         _fsStep = CLS_FS_STEP_END;
         _end();
         goto done ;
      }

      while( pOffset < pEnd )
      {
         dpsLogRecordHeader *header = ( dpsLogRecordHeader *)pOffset ;
         if ( _expectLSN.compareOffset( header->_lsn ) <= 0 )
         {
            _fsStep = CLS_FS_STEP_END ;
            _end() ;
            goto done ;
         }

         if ( 0 != dpsCB->expectLsn().compareOffset( header->_lsn ))
         {
            SDB_ASSERT( CLS_FS_STEP_NONE == _fsStep,
                        "get unexpected log" ) ;
            rc = dpsCB->move( header->_lsn, header->_version ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Failed to move lsn to "
                       "[%u, %lld] for sync the log info, rc: %d",
                       sessionName(), header->_version, header->_lsn,
                       rc ) ;
               goto error ;
            }
            else
            {
               PD_LOG( PDEVENT, "Session[%s]: Move lsn to [%u, %lld] for "
                       "sync the log info succeed", sessionName(),
                       header->_version, header->_lsn ) ;
               _fsStep = CLS_FS_STEP_LOGBEGIN ;
            }
         }

         rc = dpsCB->recordRow( pOffset, header->_length );
         PD_RC_CHECK( rc, PDERROR, "Failed to write log record, "
                      "synchronise transaction-log, rc: %d", rc );

         pOffset += ossRoundUpToMultipleX( header->_length,
                                           sizeof(UINT32) ) ;
      }
      expectLsn = dpsCB->expectLsn() ;
      _pullTransLog( expectLsn ) ;

   done:
      return rc;
   error:
      _disconnect() ;
      goto done ;
   }

   /*
   _clsSplitDstSession : implement
   */
   BEGIN_OBJ_MSG_MAP( _clsSplitDstSession, _clsDataDstBaseSession )
      ON_MSG ( MSG_CAT_SPLIT_START_RSP, handleTaskNotifyRes )
      ON_MSG ( MSG_CAT_SPLIT_CHGMETA_RSP, handleTaskNotifyRes )
      ON_MSG ( MSG_CAT_SPLIT_CLEANUP_RSP, handleTaskNotifyRes )
      ON_MSG ( MSG_CAT_SPLIT_FINISH_RSP, handleTaskNotifyRes )
      ON_MSG ( MSG_CLS_FULL_SYNC_BEGIN_RES, handleBeginRes )
      ON_MSG ( MSG_CLS_FULL_SYNC_END_RES, handleEndRes )
      ON_MSG ( MSG_CLS_FULL_SYNC_LEND_RES, handleLEndRes )
   END_OBJ_MSG_MAP()

   _clsSplitDstSession::_clsSplitDstSession( UINT64 sessionID,
                                             _netRouteAgent *agent,
                                             void *data )
   :_clsDataDstBaseSession ( sessionID, agent )
   {
      _pTask = ( _clsSplitTask* )data ;
      _taskObj = _pTask->toBson( CLS_SPLIT_MASK_ID|CLS_SPLIT_MASK_CLNAME ) ;
      _pShardMgr = sdbGetShardCB() ;
      _step = STEP_NONE ;
      _replayer.enableDPS () ;
      _needSyncData = 1 ;
      _regTask = FALSE ;
      _collectionW = 1 ;
   }

   _clsSplitDstSession::~_clsSplitDstSession ()
   {
      _pTask = NULL ;
   }

   SDB_SESSION_TYPE _clsSplitDstSession::sessionType() const
   {
      return SDB_SESSION_SPLIT_DST ;
   }

   EDU_TYPES _clsSplitDstSession::eduType () const
   {
      return EDU_TYPE_SHARDAGENT ;
   }

   BOOLEAN _clsSplitDstSession::canAttachMeta () const
   {
      if ( ( _pTask && CLS_TASK_STATUS_FINISH == _pTask->status() ) ||
           STEP_NONE != _step )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS_ONATH, "_clsSplitDstSession::_onAttach" )
   void _clsSplitDstSession::_onAttach ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS_ONATH );
      if ( CLS_TASK_STATUS_RUN == _pTask->status() ||
           CLS_TASK_STATUS_PAUSE == _pTask->status() )
      {
         PD_LOG ( PDEVENT, "Session[%s]: Split task[%s] already start, "
                  "status:%d, need clean up data first", sessionName(),
                  _pTask->taskName(), _pTask->status() ) ;

         EDUID cleanupJobID = PMD_INVALID_EDUID ;
         startCleanupJob( _pTask->clFullName(), _pTask->splitKeyObj(),
                          _pTask->splitEndKeyObj(), FALSE,
                          _pTask->isHashSharding(),
                          pmdGetKRCB()->getDPSCB(),
                          &cleanupJobID ) ;
         while ( rtnGetJobMgr()->findJob ( cleanupJobID ) )
         {
            ossSleep ( OSS_ONE_SEC ) ;
         }
      }
      else if ( CLS_TASK_STATUS_FINISH == _pTask->status() ||
                CLS_TASK_STATUS_META == _pTask->status() )
      {
         PD_LOG ( PDEVENT, "Session[%s]: Split task[%s] already finished,"
                  "need to notify destination node to clean up data",
                  sessionName(), _pTask->taskName() ) ;

         _step = STEP_META ;
         _needSyncData = 0 ;
      }

      PD_LOG ( PDEVENT, "Session[%s]: Begin to split[%s]",
               sessionName(), _pTask->taskName() ) ;

      clsTaskMgr *pTaskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
      pTaskMgr->regCollection( _pTask->clFullName() ) ;
      _regTask = TRUE ;

      _begin() ;

      PD_TRACE_EXIT ( SDB__CLSSPLDS_ONATH );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__ONDTH, "_clsSplitDstSession::_onDetach" )
   void _clsSplitDstSession::_onDetach ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__ONDTH );
      clsCB *pClsMgr = pmdGetKRCB()->getClsCB() ;
      UINT32 splitTaskCount = 0 ;

      if ( _regTask )
      {
         pClsMgr->getTaskMgr()->unregCollection( _pTask->clFullName() ) ;
         _regTask = FALSE ;
      }

      if ( CLS_FS_STATUS_END != _status || STEP_END != _step )
      {
         if ( 0 != _needSyncData && _step <= STEP_META )
         {
            EDUID cleanupJobID = PMD_INVALID_EDUID ;
            startCleanupJob( _pTask->clFullName(), _pTask->splitKeyObj(),
                             _pTask->splitEndKeyObj(), FALSE,
                             _pTask->isHashSharding(),
                             pmdGetKRCB()->getDPSCB(),
                             &cleanupJobID ) ;
            while ( rtnGetJobMgr()->findJob( cleanupJobID ) )
            {
               ossSleep ( OSS_ONE_SEC ) ;
            }
         }

         PD_LOG ( PDEVENT, "Session[%s]: Split[%s] is not complete"
                  "[status: %d, step: %d], need restart", sessionName(),
                  _pTask->taskName(), _status, _step ) ;
         pClsMgr->startTaskCheck( _taskObj ) ;
      }

      splitTaskCount = pClsMgr->getTaskMgr()->taskCount( _pTask->taskType() ) ;
      pClsMgr->removeTask( _pTask->taskID() ) ;
      pClsMgr->getTaskMgr()->removeTask( (UINT64)CLS_TID( sessionID() ) ) ;

      if ( CLS_FS_STATUS_END == _status && STEP_END == _step &&
           1 == splitTaskCount )
      {
         BSONObj match = BSON ( CAT_TARGETID_NAME <<
                                pClsMgr->getNodeID().columns.groupID ) ;
         pClsMgr->startTaskCheck( match ) ;
      }

      _disconnect() ;

      if ( getMeta() && 1 == getMeta()->getBasedHandleNum() )
      {
         PD_LOG( PDEVENT, "Session[%s] close socket[handle: %d]",
                 sessionName(), getMeta()->getHandle() ) ;
         _agent->close( getMeta()->getHandle() ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSPLDS__ONDTH );
   }

   INT32 _clsSplitDstSession::_dataSessionType () const
   {
      return CLS_FS_TYPE_BETWEEN_SETS ;
   }

   void _clsSplitDstSession::_doneSplit()
   {
      clsCB *pClsMgr = pmdGetKRCB()->getClsCB() ;
      if ( _regTask )
      {
         pClsMgr->getTaskMgr()->unregCollection( _pTask->clFullName() ) ;
         _regTask = FALSE ;
      }

      PD_LOG( PDEVENT, "Session[%s]: Split[%s] has been done",
              sessionName(), _pTask->taskName() ) ;
      _quit = TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__ISREADY, "_clsSplitDstSession::_isReady" )
   BOOLEAN _clsSplitDstSession::_isReady ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__ISREADY );
      BOOLEAN ret = TRUE ;

      if ( STEP_END == _step )
      {
         _doneSplit() ;
         ret = FALSE ;
         goto done ;
      }
      else if ( !sdbGetReplCB()->primaryIsMe() )
      {
         PD_LOG ( PDERROR, "Session[%s]: Self node is not primary, "
                  "disconnect. task:%s", sessionName(),
                  _pTask->taskName() ) ;
         _disconnect() ;
         ret = FALSE ;
         goto done ;
      }

   done :
      PD_TRACE_EXIT ( SDB__CLSSPLDS__ISREADY );
      return ret ;
   }

   BOOLEAN _clsSplitDstSession::_onNotify ( MsgClsFSNotifyRes *pMsg )
   {
      if ( CLS_TASK_STATUS_CANCELED == _pTask->status() )
      {
         _status = CLS_FS_STATUS_END ;
         _end() ;
         return FALSE ;  /// SDB_TASK_HAS_CANCELED
      }
      else if ( CLS_FS_STATUS_END == _status &&
                CLS_FS_EOF == pMsg->eof )
      {
         _step = STEP_POST_SYNC ;
         _end() ;
         return FALSE ;
      }
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__BEGIN, "_clsSplitDstSession::_begin" )
   void _clsSplitDstSession::_begin ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__BEGIN );
      if ( STEP_NONE == _step )
      {
         _taskNotify ( MSG_CAT_SPLIT_START_REQ ) ;
      }
      else
      {
         MsgClsFSBegin msg ;
         msg.type = CLS_FS_TYPE_BETWEEN_SETS ;
         msg.header.TID = CLS_TID( _sessionID ) ;
         MsgRouteID lastID = _selector.src() ;
         MsgRouteID src = _selector.selectPrimary( _pTask->sourceID(),
                                                   MSG_ROUTE_SHARD_SERVCIE ) ;

         if ( MSG_INVALID_ROUTEID != lastID.value &&
              lastID.value != src.value )
         {
            MsgHeader disMsg ;
            disMsg.messageLength = sizeof( MsgHeader ) ;
            disMsg.routeID.value = MSG_INVALID_ROUTEID ;
            disMsg.TID = CLS_TID( _sessionID ) ;
            disMsg.requestID = ++_requestID ;
            disMsg.opCode = MSG_BS_DISCONNECT ;
            _agent->syncSend( lastID, &disMsg ) ;
         }

         if ( MSG_INVALID_ROUTEID != src.value )
         {
            msg.header.requestID = ++_requestID ;
            _agent->syncSend( src, &msg ) ;
            _timeout = 0 ;
         }
      }
      PD_TRACE_EXIT ( SDB__CLSSPLDS__BEGIN );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__LEND, "_clsSplitDstSession::_lend" )
   void _clsSplitDstSession::_lend ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__LEND );
      MsgClsFSLEnd msg ;
      msg.header.requestID = ++_requestID ;
      msg.header.TID = CLS_TID( _sessionID ) ;
      _agent->syncSend( _selector.src(), &msg ) ;
      _timeout = 0 ;
      PD_TRACE_EXIT ( SDB__CLSSPLDS__LEND );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__END, "_clsSplitDstSession::_end" )
   void _clsSplitDstSession::_end ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__END );

      if ( CLS_TASK_STATUS_CANCELED == _pTask->status() &&
           STEP_REMOVE != _step )
      {
         clsCB *pClsCB = pmdGetKRCB()->getClsCB() ;
         pClsCB->getTaskMgr()->unregCollection( _pTask->clFullName() ) ;
         _regTask = FALSE ;
         INT32 cleanRet = SDB_OK ;

         EDUID cleanupJobID = PMD_INVALID_EDUID ;
         if ( SDB_OK != startCleanupJob( _pTask->clFullName(),
                                         _pTask->splitKeyObj(),
                                         _pTask->splitEndKeyObj(), FALSE,
                                         _pTask->isHashSharding(),
                                         pmdGetKRCB()->getDPSCB(),
                                         &cleanupJobID, TRUE ) )
         {
            _disconnect() ;
            goto done ;
         }
         while ( rtnGetJobMgr()->findJob( cleanupJobID, &cleanRet ) )
         {
            ossSleep ( OSS_ONE_SEC ) ;
         }
         if ( cleanRet )
         {
            _disconnect() ;
            goto done ;
         }
         _step = STEP_REMOVE ;
      }
      else if ( STEP_SYNC_DATA == _step )
      {
         _notify ( CLS_FS_NOTIFY_TYPE_LOG ) ;
      }
      else if ( STEP_POST_SYNC == _step )
      {
         _taskNotify( MSG_CAT_SPLIT_CHGMETA_REQ ) ;
      }
      else if ( STEP_META == _step )
      {
         INT32 rc = _pShardMgr->syncUpdateCatalog( _pTask->clFullName(),
                                                   OSS_ONE_SEC ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            _step = STEP_END_NTY ;
            _lend() ;
         }
         else
         {
            std::string mainCLName ;
            BOOLEAN hasSplit = FALSE ;
            catAgent *pCatAgent = _pShardMgr->getCataAgent() ;
            pCatAgent->lock_r () ;
            _clsCatalogSet* catSet = pCatAgent->collectionSet(
               _pTask->clFullName() ) ;
            if ( catSet )
            {
               mainCLName = catSet->getMainCLName();
               NodeID selfNode = _pShardMgr->nodeID() ;
               if ( catSet->isKeyInGroup( _pTask->splitKeyObj(),
                                          selfNode.columns.groupID ) )
               {
                  hasSplit = TRUE ;
                  _collectionW = catSet->getW() ;
               }
            }
            pCatAgent->release_r() ;
            if ( !mainCLName.empty() )
            {
               INT32 rcTmp = _pShardMgr->syncUpdateCatalog( mainCLName.c_str(),
                                                            OSS_ONE_SEC ) ;
               if ( rcTmp )
               {
                  PD_LOG( PDWARNING, "Session[%s]: Update catalog info "
                          "of main-collection(%s) failed, rc: %d",
                          sessionName(), mainCLName.c_str(), rcTmp ) ;
               }
            }

            if ( hasSplit )
            {
               PD_LOG ( PDEVENT, "Session[%s]: Catalog is valid, "
                        "task: %s", sessionName(), _pTask->taskName() ) ;
               pmdGetKRCB()->getClsCB()->invalidateCata(
                  _pTask->clFullName() ) ;
               if ( !mainCLName.empty() )
               {
                  pmdGetKRCB()->getClsCB()->invalidateCata(
                     mainCLName.c_str() ) ;
               }
               _step = STEP_END_NTY ;
               _lend () ;
            }
         }
      }
      else if ( STEP_END_NTY == _step )
      {
         _lend() ;
      }
      else if ( STEP_FINISH == _step )
      {
         _taskNotify( MSG_CAT_SPLIT_CLEANUP_REQ ) ;
      }
      else if ( STEP_CLEANUP == _step )
      {
         MsgClsFSEnd msg ;

         msg.header.TID = CLS_TID( _sessionID ) ;
         msg.header.requestID = ++_requestID ;
         _agent->syncSend( _selector.src(), &msg ) ;
         _timeout = 0 ;
      }
      else if ( STEP_REMOVE == _step )
      {
         _taskNotify( MSG_CAT_SPLIT_FINISH_REQ ) ;
      }

      _status = CLS_FS_STATUS_END ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLDS__END );
      return ;
   }

   BSONObj _clsSplitDstSession::_keyObjB ()
   {
      return _pTask->splitKeyObj () ;
   }

   BSONObj _clsSplitDstSession::_keyObjE ()
   {
      return _pTask->splitEndKeyObj() ;
   }

   INT32 _clsSplitDstSession::_needData () const
   {
      return _needSyncData ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS__TSKNTF, "_clsSplitDstSession::_taskNotify" )
   void _clsSplitDstSession::_taskNotify ( INT32 msgType )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLDS__TSKNTF );
      CHAR *pBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgOpQuery *pHeader = NULL ;

      try
      {
         rc = msgBuildQueryMsg ( &pBuff, &buffSize,
                                 "CAT", 0, 0, 0, -1,
                                 &_taskObj, NULL, NULL, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Session[%s]: Failed to build start "
                     "request, rc = %d", sessionName(), rc ) ;
            goto error ;
         }

         pHeader                       = (MsgOpQuery*)pBuff ;
         pHeader->header.opCode        = msgType ;
         pHeader->header.routeID.value = 0 ;
         pHeader->header.TID           = CLS_TID ( _sessionID ) ;
         pHeader->header.requestID     = ++_requestID ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s]: Failed to build start "
                  "request: %s", sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _pShardMgr->sendToCatlog( (MsgHeader*)pHeader ) ;
      _timeout = 0 ;

   done:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
         pBuff = NULL ;
      }
      PD_TRACE_EXIT ( SDB__CLSSPLDS__TSKNTF );
      return ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS_HNDNTFRES, "_clsSplitDstSession::handleTaskNotifyRes" )
   INT32 _clsSplitDstSession::handleTaskNotifyRes( NET_HANDLE handle,
                                                   MsgHeader * header )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS_HNDNTFRES );
      MsgOpReply *msg = ( MsgOpReply* )header ;
      CHECK_REQUEST_ID ( msg->header, _requestID ) ;

      if ( SDB_CLS_NOT_PRIMARY == msg->flags )
      {
         if ( SDB_OK != _pShardMgr->updatePrimaryByReply( header ) )
         {
            _pShardMgr->updateCatGroup() ;
         }
         goto done ;
      }
      else if ( SDB_DMS_EOC == msg->flags ||
                SDB_CAT_TASK_NOTFOUND == msg->flags )
      {
         PD_LOG ( PDWARNING, "Session[%s]: The split task[%s] is removed",
                  sessionName(), _pTask->taskName() ) ;
         if ( STEP_REMOVE != _step )
         {
            _disconnect() ;
            goto done ;
         }
      }
      else if ( SDB_TASK_HAS_CANCELED == msg->flags )
      {
         PD_LOG( PDERROR, "Session[%s]: The split task[%s] has canceled",
                 sessionName(), _pTask->taskName() ) ;
         _status = CLS_FS_STATUS_END ;
         _pTask->setStatus( CLS_TASK_STATUS_CANCELED ) ;
         goto done ;
      }
      else if ( SDB_OK != msg->flags )
      {
         PD_LOG ( PDERROR, "Session[%s]: The split task[%s] notify "
                  "response failed[%d]", sessionName(),
                  _pTask->taskName(), msg->flags ) ;
         goto done ;
      }

      switch ( _step )
      {
         case STEP_NONE :
            _step = STEP_SYNC_DATA ;
            _begin () ;
            break ;
         case STEP_SYNC_DATA :
         case STEP_POST_SYNC :
            _step = STEP_META ;
            _end () ;
            break ;
         case STEP_FINISH:
            _step = STEP_CLEANUP ;
            _end() ;
            break ;
         case STEP_REMOVE :
            _step = STEP_END ;
         default :
            break ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLDS_HNDNTFRES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS_HNDBGRES, "_clsSplitDstSession::handleBeginRes" )
   INT32 _clsSplitDstSession::handleBeginRes ( NET_HANDLE handle,
                                               MsgHeader * header )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS_HNDBGRES );
      MsgClsFSBeginRes *msg = ( MsgClsFSBeginRes * )header ;
      _expectLSN = msg->lsn ;
      if ( CLS_FS_STATUS_BEGIN != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: ignore msg. local statsus: "
                 "%d, task: %s", sessionName(), _status,
                 _pTask->taskName() ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }

      CHECK_REQUEST_ID ( msg->header.header, _requestID ) ;

      if ( SDB_OK != msg->header.res )
      {
         PD_LOG ( PDWARNING, "Session[%s]: Node[%d] refused split sync "
                  "seq[rc:%d]", sessionName(), _selector.src().columns.nodeID,
                  msg->header.res ) ;
         _selector.clearSrc () ;
         goto done ;
      }

      PD_LOG( PDEVENT, "Session[%s]: Established the split session with "
              "node[%s], Remote LSN:[%u,%lld]", sessionName(),
              routeID2String( _selector.src() ).c_str(),
              _expectLSN.version, _expectLSN.offset ) ;

      _selector.clearTime() ;

      _fullNames.clear () ;
      _fullNames.push_back ( _pTask->clFullName() ) ;

      _status = CLS_FS_STATUS_META ;
      _meta() ;
   done:
      PD_TRACE_EXIT ( SDB__CLSSPLDS_HNDBGRES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS_HNDENDRES, "_clsSplitDstSession::handleEndRes" )
   INT32 _clsSplitDstSession::handleEndRes( NET_HANDLE handle,
                                            MsgHeader * header )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS_HNDENDRES );
      if ( CLS_FS_STATUS_END != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: Split[%s] status[%d] is not "
                 "expect[%d]", sessionName(), _pTask->taskName(),
                 _status, CLS_FS_STATUS_END ) ;
         goto done ;
      }

      if ( _collectionW > 1 )
      {
         sdbGetReplCB()->sync( _lastOprLSN, eduCB(),
                               _collectionW, CLS_SPLIT_DST_SYNC_TIME ) ;
      }

      _step = STEP_REMOVE ;
      _taskNotify( MSG_CAT_SPLIT_FINISH_REQ ) ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLDS_HNDENDRES );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLDS_HNDENDRES2, "_clsSplitDstSession::handleLEndRes" )
   INT32 _clsSplitDstSession::handleLEndRes( NET_HANDLE handle,
                                             MsgHeader * header )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLDS_HNDENDRES2 );
      MsgClsFSLEndRes *msg = ( MsgClsFSLEndRes*)header ;

      if ( CLS_FS_STATUS_END != _status )
      {
         PD_LOG( PDWARNING, "Session[%s]: Split[%s] status[%d] is not "
                 "expect[%d]", sessionName(), _pTask->taskName(),
                 _status, CLS_FS_STATUS_END ) ;
         goto done ;
      }
      else if ( !_isReady() )
      {
         goto done ;
      }
      CHECK_REQUEST_ID ( msg->header.header, _requestID ) ;

      _step = STEP_FINISH ;
      _end() ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLDS_HNDENDRES2 );
      return SDB_OK ;
   }

}

