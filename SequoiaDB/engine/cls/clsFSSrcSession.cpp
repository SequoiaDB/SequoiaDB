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

   Source File Name = clsFSyncSrcSession.cpp

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
#include "clsFSSrcSession.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "rtn.hpp"
#include "clsUtil.hpp"
#include "monDMS.hpp"
#include "clsCleanupJob.hpp"
#include "rtnIXScanner.hpp"
#include "dpsLogRecord.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "dpsLogRecordDef.hpp"
#include "rtnLob.hpp"
#include "pmdStartup.hpp"
#include "rtnContextLob.hpp"
#include "rtnContextData.hpp"
#include "msgMessageFormat.hpp"
#include "rtnExtDataHandler.hpp"
#include <set>

using namespace bson ;

namespace engine
{

#define CLS_SYNC_MAX_TIME                 (5)         // second
#define CLS_FS_SRC_MAX_NO_MSG_TIME        (3600000)   // 1 hour
#define CLS_FS_DISABLE_EXTDATA_TIME       (10000)

#define CLS_IS_LOB_LOG( type )\
        ( LOG_TYPE_LOB_WRITE == ( type ) || \
          LOG_TYPE_LOB_REMOVE == ( type ) ||\
          LOG_TYPE_LOB_UPDATE == ( type) )

   /*
   _clsDataSrcBaseSession : implement
   */
   BEGIN_OBJ_MSG_MAP ( _clsDataSrcBaseSession, _pmdAsyncSession )
      //ON_MSG
      ON_MSG( MSG_CLS_FULL_SYNC_META_REQ, handleFSMeta )
      ON_MSG( MSG_CLS_FULL_SYNC_INDEX_REQ, handleFSIndex )
      ON_MSG( MSG_CLS_FULL_SYNC_NOTIFY, handleFSNotify )
   END_OBJ_MSG_MAP()

   _clsDataSrcBaseSession::_clsDataSrcBaseSession ( UINT64 sessionID,
                                                    _netRouteAgent *agent )
   :_pmdAsyncSession( sessionID ), _mb( 1024 )
   {
      _agent = agent ;
      _contextID = -1 ;
      _context   = NULL ;
      _lobContextID = -1 ;
      _findEnd = FALSE ;
      _query = NULL ;
      _queryLen = 0 ;
      _packetID = -1 ;
      _canResend= TRUE ;
      _dataType = CLS_FS_NOTIFY_TYPE_DOC ;
      _quit = FALSE ;
      _needData = 1 ;
      _hasMeta = FALSE ;
      _pRepl = sdbGetReplCB() ;
      _init = FALSE ;
      _timeCounter = 0 ;
      _curExtID = DMS_INVALID_EXTENT ;
      _curCollection = ~0 ;
      _curCSLID = DMS_INVALID_LOGICCSID ;
      _curMBID = DMS_INVALID_MBID ;
      _beginLSNOffset = 0 ;

      //init disconnect msg
      _disconnectMsg.messageLength = sizeof ( MsgHeader ) ;
      _disconnectMsg.opCode = MSG_BS_DISCONNECT ;
      _disconnectMsg.TID = CLS_TID( sessionID ) ;
      _disconnectMsg.requestID = 0 ;

      _info._info.setNice( SCHED_NICE_MIN ) ;
   }

   _clsDataSrcBaseSession::~_clsDataSrcBaseSession ()
   {
      _resetInfo () ;
   }

   void _clsDataSrcBaseSession::onRecieve ( const NET_HANDLE netHandle,
                                            MsgHeader * msg )
   {
      _disconnectMsg.routeID = msg->routeID ;

      _timeCounter = 0 ;
   }

   BOOLEAN _clsDataSrcBaseSession::timeout( UINT32 interval )
   {
      return _quit ;
   }

   void _clsDataSrcBaseSession::onTimer( UINT64 timerID, UINT32 interval )
   {
      _timeCounter += interval ;

      if ( !_quit )
      {
         INT32 rc = SDB_OK ;
         if ( SDB_OK != ( rc = _isReady() ) )
         {
            PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                    sessionName(), rc ) ;
            _disconnect () ;
            goto done ;
         }

         /// if the peer node no msg a long time, shoud to quit
         if ( CLS_FS_SRC_MAX_NO_MSG_TIME < _timeCounter )
         {
            PD_LOG ( PDWARNING, "Session[%s]: no msg a long time, quit",
                     sessionName() ) ;
            _disconnect () ;
         }
      }

   done:
      return ;
   }

   void _clsDataSrcBaseSession::_onAttach()
   {
      // register session
      _pRepl->regSession ( this ) ;
      // not use trans lock
      eduCB()->getTransExecutor()->setUseTransLock( FALSE ) ;
   }

   void _clsDataSrcBaseSession::_onDetach()
   {
      // unregister session
      _pRepl->unregSession ( this ) ;
      _resetInfo() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__RESETINFO, "_clsDataSrcBaseSession::_resetInfo" )
   void _clsDataSrcBaseSession::_resetInfo ( BOOLEAN all )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__RESETINFO );
      _indexs.clear() ;
      if ( -1 != _contextID )
      {
         rtnKillContexts( 1, &_contextID , eduCB(),
                          pmdGetKRCB()->getRTNCB() ) ;
         _contextID = -1 ;
         _context   = NULL ;
      }
      if ( -1 != _lobContextID )
      {
         rtnKillContexts( 1, &_lobContextID, eduCB(),
                          pmdGetKRCB()->getRTNCB() ) ;
         _lobContextID = -1 ;
      }
      _findEnd = FALSE ;
      _needData = 1 ;
      _hasMeta = FALSE ;
      _curExtID = DMS_INVALID_EXTENT ;
      _curScanKeyObj = BSONObj() ;
      _curCollection = ~0 ;

      if ( all )
      {
         _curCSLID = DMS_INVALID_LOGICCSID ;
         _curMBID = DMS_INVALID_MBID ;
      }

      PD_TRACE_EXIT ( SDB__CLSDSBS__RESETINFO );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__RESEND, "_clsDataSrcBaseSession::_resend" )
   void _clsDataSrcBaseSession::_resend( const NET_HANDLE & handle,
                                         const _MsgClsFSNotify * req )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__RESEND );
      MsgClsFSNotifyRes msg ;
      msg.header.res = SDB_OK ;
      msg.header.header.TID = req->header.TID ;
      msg.header.header.routeID = req->header.routeID ;
      msg.header.header.requestID = req->header.requestID ;
      msg.packet = req->packet ;
      msg.type = (CLS_FS_NOTIFY_TYPE)_dataType ;

      if ( CLS_FS_NOTIFY_TYPE_DOC == _dataType )
      {
         if ( NULL == _query )
         {
            PD_LOG( PDDEBUG, "Session[%s]: last doc sync has hit the end.",
                    sessionName() ) ;
            msg.eof = CLS_FS_EOF ;
            _agent->syncSend( handle, &msg ) ;
         }
         else
         {
            msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                              _queryLen ;
            _agent->syncSend( handle, &( msg.header.header ),
                              (void*)_query, _queryLen ) ;
         }
      }
      else if ( CLS_FS_NOTIFY_TYPE_LOG == _dataType )
      {
         if ( 0 == _mb.length() )
         {
            PD_LOG( PDDEBUG, "Session[%s]: last log sync has hit the end.",
                    sessionName() ) ;
            msg.eof = CLS_FS_EOF ;
            _agent->syncSend( handle, &msg ) ;
         }
         else
         {
            msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                              _mb.length() ;
            _agent->syncSend( handle, &( msg.header.header ),
                              _mb.offset( 0 ), _mb.length() ) ;
         }
      }
      else if ( CLS_FS_NOTIFY_TYPE_LOB == _dataType )
      {
         if ( 0 == _mb.length() )
         {
            PD_LOG( PDDEBUG, "Session[%s]: last log sync has hit the end.",
                    sessionName() ) ;
            msg.eof = CLS_FS_EOF ;
            msg.lsn = pmdGetKRCB()->getDPSCB()->expectLsn () ;
            _LSNlatch.get () ;
            if ( _deqLSN.size() > 0 )
            {
               msg.lsn.offset = _deqLSN.front() ;
            }
            else
            {
               msg.lsn.offset = _beginLSNOffset ;
            }
            _LSNlatch.release() ;
            /// build collection's commit info
            BSONObj commitObj ;
            if ( _buildCLCommitInfo( _curCollecitonName, commitObj ) )
            {
               msg.header.header.messageLength += commitObj.objsize() ;
               _agent->syncSend( handle, &(msg.header.header),
                                 (void*)commitObj.objdata(),
                                 commitObj.objsize() ) ;
            }
            else
            {
               _agent->syncSend( handle, &msg ) ;
            }
         }
         else
         {
            msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                              _mb.length() ;
            _agent->syncSend( handle, &( msg.header.header ),
                              _mb.offset( 0 ), _mb.length() ) ;
         }
      }
      else
      {
         SDB_ASSERT( FALSE, "notify over is impossible." ) ;
      }

      PD_TRACE_EXIT ( SDB__CLSDSBS__RESEND );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__ERSDFTINX, "_clsDataSrcBaseSession::_eraseDefaultIndex" )
   void _clsDataSrcBaseSession::_eraseDefaultIndex ()
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__ERSDFTINX );
      MON_IDX_LIST::iterator itr = _indexs.begin() ;
      while ( itr != _indexs.end() )
      {
         // get name
         BSONElement nameE = itr->_indexDef.getField ( IXM_NAME_FIELD ) ;
         if ( ossStrcmp( nameE.str().c_str(), IXM_ID_KEY_NAME ) == 0 )
         {
            itr = _indexs.erase( itr ) ;
            continue ;
         }
         ++itr ;
      }
      PD_TRACE_EXIT ( SDB__CLSDSBS__ERSDFTINX );
   }

   BOOLEAN _clsDataSrcBaseSession::_existIndex( const CHAR * indexName )
   {
      MON_IDX_LIST::iterator itr = _indexs.begin() ;
      while ( itr != _indexs.end() )
      {
         // get name
         BSONElement nameE = itr->_indexDef.getField ( IXM_NAME_FIELD ) ;
         if ( ossStrcmp( nameE.str().c_str(), indexName ) == 0 )
         {
            return TRUE ;
         }
         ++itr ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__OPNCONTX, "_clsDataSrcBaseSession::_openContext" )
   INT32 _clsDataSrcBaseSession::_openContext( CHAR *cs, CHAR *collection )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS__OPNCONTX );
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj orderBy ;
      BSONObjBuilder builder ;
      builder.appendNull( "" ) ;
      BSONObj hint = builder.obj() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextLobFetcher *pContextLob = NULL ;

      ossSnprintf( fullName, sizeof( fullName ), "%s.%s",
                   cs, collection ) ;

      if ( -1 != _contextID )
      {
         rtnKillContexts( 1, &_contextID , eduCB(), pRtnCB ) ;
         _contextID = -1 ;
         _context   = NULL ;
      }

      if ( -1 != _lobContextID )
      {
         rtnKillContexts( 1, &_lobContextID , eduCB(), pRtnCB ) ;
         _lobContextID = -1 ;
      }

      // not need data, don't open context
      if ( 0 == _needData )
      {
         goto done ;
      }

      // TABSCAN
      if ( TBSCAN == _scanType() )
      {
         rc = rtnQuery( fullName, selector, matcher, orderBy, hint, 0, eduCB(),
                        0, -1,  pmdGetKRCB()->getDMSCB(), pRtnCB,
                        _contextID, (rtnContextBase**)&_context ) ;
      }
      // SHARD KEY INDEX SCAN
      else
      {
         rc = rtnTraversalQuery( fullName, _rangeKeyObj, IXM_SHARD_KEY_NAME, 1,
                                 eduCB(), pmdGetKRCB()->getDMSCB(),
                                 pRtnCB, _contextID, &_context ) ;
      }

      if ( SDB_DMS_EOC == rc )
      {
         // Empty collection, hit end already
         // Make sure the context is closed
         if ( -1 != _contextID )
         {
            rtnKillContexts( 1, &_contextID , eduCB(), pRtnCB ) ;
            _contextID = -1 ;
            _context = NULL ;
         }

         _findEnd = TRUE ;

         // Ignore error
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         PD_LOG ( PDERROR, "Session[%s]: Failed to run query, rc = %d",
                  sessionName(), rc ) ;
         goto error ;
      }
      else
      {
         SDB_ASSERT( _context->scanType() == _scanType(), "scan type error" ) ;
         if ( _context->eof() )
         {
            // Empty collection, hit end already
            rtnKillContexts( 1, &_contextID , eduCB(), pRtnCB ) ;
            _contextID = -1 ;
            _context = NULL ;
            _findEnd = TRUE ;
         }
      }

      /// create lob context
      rc = pRtnCB->contextNew( RTN_CONTEXT_LOB_FETCHER,
                               (rtnContext**)&pContextLob,
                               _lobContextID, eduCB() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Create lob context failed, rc: %d", rc ) ;
         goto error ;
      }
      /// open lob context
      rc = pContextLob->open( &_lobFetcher,
                              _curCollecitonName.c_str(),
                              FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init lob fetcher, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSDSBS__OPNCONTX, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__CONSTINX, "_clsDataSrcBaseSession::_constructIndex" )
   void _clsDataSrcBaseSession::_constructIndex( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__CONSTINX );
      BSONObjBuilder builder ;
      if ( 0 == _needData || _indexs.empty() )
      {
         builder.appendBool( CLS_FS_NOMORE, TRUE ) ;
         obj = builder.obj() ;
      }
      else
      {
         builder.appendBool( CLS_FS_NOMORE, FALSE ) ;
         BSONArrayBuilder array ;
         MON_IDX_LIST::const_iterator itr = _indexs.begin() ;
         for ( ; itr != _indexs.end(); itr++ )
         {
            BSONObjBuilder index ;
            BSONObj filter = BSON( FIELD_NAME_EXT_DATA_NAME << "" ) ;
            BSONObj finalIdx =
                  itr->_indexDef.filterFieldsUndotted( filter, false ) ;
            array.append( finalIdx ) ;
         }
         builder.append( CLS_FS_INDEXES, array.arr() ) ;
         obj = builder.obj() ;
      }
      PD_TRACE_EXIT ( SDB__CLSDSBS__CONSTINX );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__CONSTMETA, "_clsDataSrcBaseSession::_constructMeta" )
   void _clsDataSrcBaseSession::_constructMeta( BSONObj &obj,
                                                const CHAR *cs,
                                                const CHAR *collection,
                                                utilCLUniqueID clUniqueID,
                                                _dmsStorageUnit *su )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__CONSTMETA );
      BSONObjBuilder builder1 ;
      BSONObjBuilder builder2 ;
      UINT32 attributes = 0 ;
      UTIL_COMPRESSOR_TYPE compType = UTIL_COMPRESSOR_INVALID ;
      BSONObj extOptions ;

      su->getCollectionAttributes( collection, attributes ) ;
      su->getCollectionCompType( collection, compType ) ;
      su->getCollectionExtOptions( collection, extOptions ) ;

      builder1.append( CLS_FS_PAGE_SIZE, su->getPageSize() ) ;
      builder1.append( CLS_FS_ATTRIBUTES, attributes ) ;
      builder1.append( CLS_FS_COMP_TYPE, (INT32)compType ) ;
      // If compression type is LZW, get the dictionary, if any.
      if ( UTIL_COMPRESSOR_LZW == compType )
      {
         dmsMBContext *context = NULL ;
         dmsStorageDataCommon* data = su->data() ;
         if ( SDB_OK == data->getMBContext( &context, collection, SHARED ) )
         {
            const CHAR *dictionary = NULL ;
            UINT32 dictLen = 0 ;
            if ( data->getDictionary( context, dictionary, dictLen ) )
            {
               // If the dictionary is not ready, nothing will be added.
               builder1.appendBinData( CLS_FS_COMP_DICT, dictLen,
                                       BinDataGeneral, dictionary ) ;
            }
            data->releaseMBContext( context ) ;
         }
      }

      if ( !extOptions.isEmpty() )
      {
         builder1.append( CLS_FS_EXT_OPTION, extOptions ) ;
      }

      builder1.append( CLS_FS_LOB_PAGE_SIZE, su->getLobPageSize() ) ;
      builder1.append( CLS_FS_CS_TYPE, (INT32)su->type() ) ;
      builder2.append( CLS_FS_CS_META_NAME, builder1.obj() ) ;
      builder2.append( CLS_FS_CS_NAME, cs ) ;
      builder2.append( CLS_FS_COLLECTION_NAME, collection ) ;
      builder2.append( CLS_FS_COLLECTION_UNIQUEID, (INT64)clUniqueID );
      obj = builder2.obj() ;
      PD_TRACE_EXIT ( SDB__CLSDSBS__CONSTMETA );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__GETCSNM, "_clsDataSrcBaseSession::_getCSName" )
   INT32 _clsDataSrcBaseSession::_getCSName( const BSONObj &obj,
                                             CHAR *cs,
                                             UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS__GETCSNM );
      try
      {
         BSONElement ele = obj.getField( CLS_FS_CS_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDWARNING, "Session[%s]: Failed to parse cs element[%s]",
                    sessionName(), ele.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( len < (UINT32)ele.valuestrsize() )
         {
            PD_LOG( PDWARNING, "Session[%s]: CS Name[%s] is too long",
                    sessionName(), ele.valuestr() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         ossStrncpy( cs, ele.valuestr(), len - 1 ) ;
         cs[ len - 1 ] = '\0' ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSDSBS__GETCSNM, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__GETCL, "_clsDataSrcBaseSession::_getCollection" )
   INT32 _clsDataSrcBaseSession::_getCollection( const BSONObj &obj,
                                                 CHAR *collection,
                                                 UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS__GETCL );
      try
      {
         BSONElement ele = obj.getField( CLS_FS_COLLECTION_NAME ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDWARNING, "Session[%s]: Failed to parse collection "
                    "element[%s]", sessionName(), ele.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if ( len < (UINT32)ele.valuestrsize() )
         {
            PD_LOG( PDWARNING, "Session[%s]: CL Name[%s] is too long",
                    sessionName(), ele.valuestr() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         ossStrncpy( collection, ele.valuestr(), len - 1 ) ;
         collection[ len - 1 ] = '\0' ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSDSBS__GETCL, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__GETRGEKEY, "_clsDataSrcBaseSession::_getRangeKey" )
   INT32 _clsDataSrcBaseSession::_getRangeKey( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS__GETRGEKEY );
      _rangeKeyObj = BSONObj() ;
      _rangeEndKeyObj = BSONObj() ;
      try
      {
         BSONElement ele = obj.getField( CLS_FS_KEYOBJ ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            PD_LOG ( PDWARNING, "Session[%s]: CLS_FS_KEYOBJ must be obj",
                     sessionName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _rangeKeyObj = ele.embeddedObject().copy() ;

         ele = obj.getField( CLS_FS_END_KEYOBJ ) ;
         if ( ele.eoo() || Object != ele.type() )
         {
            PD_LOG ( PDWARNING, "Session[%s]: CLS_FS_END_KEYOBJ must be obj",
                     sessionName() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _rangeEndKeyObj = ele.embeddedObject().copy() ;

         PD_LOG ( PDDEBUG, "Session[%s]: range key: [%s, %s]", sessionName(),
                  _rangeKeyObj.toString().c_str(),
                  _rangeEndKeyObj.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(),  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSDSBS__GETRGEKEY, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__DISCONN, "_clsDataSrcBaseSession::_disconnect" )
   void _clsDataSrcBaseSession::_disconnect()
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__DISCONN );
      PD_LOG( PDDEBUG, "Session[%s]: disconnect with peer", sessionName() ) ;
      _agent->syncSend( netHandle(), &_disconnectMsg ) ;
      _quit = TRUE ;
      PD_TRACE_EXIT ( SDB__CLSDSBS__DISCONN );
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__SYNCLOB, "_clsDataSrcBaseSession::_syncLob" )
   INT32 _clsDataSrcBaseSession::_syncLob( const NET_HANDLE &handle,
                                           SINT64 packet,
                                           const MsgRouteID &routeID,
                                           UINT32 TID, UINT64 requestID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSDSBS__SYNCLOB ) ;
      _dataType = CLS_FS_NOTIFY_TYPE_LOB ;
      MsgClsFSNotifyRes msg ;
      msg.header.header.TID = TID ;
      msg.header.header.routeID = routeID ;
      msg.header.res = SDB_OK ;
      msg.header.header.requestID = requestID ;
      msg.packet = packet ;
      msg.type = (CLS_FS_NOTIFY_TYPE)_dataType ;
      msg.eof = CLS_FS_NOT_EOF ;
      _mb.clear () ;
      dmsLobInfoOnPage page ;
      BOOLEAN need2Send = FALSE ;
      const UINT32 bmSize = sizeof( MsgLobTuple ) + sizeof( bson::OID ) ;
      time_t bTime = time( NULL ) ;

      /// | oid | MsgLobTuple | data | ... | oid | MsgLobTuple | data |
      while ( _lobContextID >= 0 )
      {
         need2Send = FALSE ;
         UINT32 finalSize = 0 ;
         UINT32 oldSize = _mb.length() ;
         UINT32 alignedLen = ossRoundUpToMultipleX( oldSize, 4 ) ;
         INT32 extendSize = bmSize + alignedLen - oldSize - _mb.idleSize() ;
         if ( 0 < extendSize )
         {
            rc = _mb.extend( (UINT32)extendSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extend mb block:%d", rc ) ;
               goto error ;
            }
         }

         _mb.writePtr( alignedLen + bmSize ) ;
         rc = _lobFetcher.fetch( eduCB(), page, &_mb ) ;
         if ( rc )
         {
            _mb.writePtr( oldSize ) ;
            if ( SDB_DMS_EOC == rc ||
                 SDB_DMS_NOTEXIST == rc ||
                 SDB_DMS_TRUNCATED == rc )
            {
               // Ignore errors:
               // 1. end of collection
               // 2. collection dropped or collection truncated
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to fetch lob:%d", rc ) ;
               goto error ;
            }
         }

         finalSize = _mb.length() ;
         rc = _onLobFilter( page._oid, page._sequence, need2Send ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to filter lob data:%d", rc ) ;
            goto error ;
         }

         if ( !need2Send )
         {
            _mb.writePtr( oldSize ) ;
            continue ;
         }
         else
         {
            UINT32 *tmp = NULL ;
            SINT64 *offset = NULL ;
            _mb.writePtr( alignedLen ) ;
            ossMemcpy( _mb.writePtr(), &( page._oid ), sizeof( page._oid ) ) ;
            _mb.writePtr( alignedLen + sizeof( page._oid ) ) ;
            tmp = (UINT32 *)_mb.writePtr() ;
            *tmp = page._len ;
            _mb.writePtr( _mb.length() + sizeof( UINT32 ) ) ;
            tmp = (UINT32 *)_mb.writePtr() ;
            *tmp = page._sequence ;
            _mb.writePtr( _mb.length() + sizeof( UINT32 ) ) ;
            offset = ( SINT64 * )_mb.writePtr() ;
            *offset = 0 ;
            _mb.writePtr( finalSize ) ;
         }

         if ( CLS_SYNC_MAX_LEN <= _mb.length() ||
              ( time( NULL ) - bTime >= CLS_SYNC_MAX_TIME &&
               _mb.length() > 0 ) )
         {
            break ;
         }
      }

      if ( 0 != _mb.length() )
      {
         msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                           _mb.length() ;
         _agent->syncSend( handle, &(msg.header.header),
                           _mb.offset( 0 ), _mb.length() ) ;
      }
      else
      {
         msg.eof = CLS_FS_EOF ;
         msg.lsn = pmdGetKRCB()->getDPSCB()->expectLsn () ;
         _LSNlatch.get () ;
         if ( _deqLSN.size() > 0 )
         {
            msg.lsn.offset = _deqLSN.front() ;
         }
         else
         {
            msg.lsn.offset = _beginLSNOffset ;
         }
         _LSNlatch.release() ;
         /// build collection's commit info
         BSONObj commitObj ;
         if ( SDB_OK == _buildCLCommitInfo( _curCollecitonName, commitObj ) )
         {
            msg.header.header.messageLength += commitObj.objsize() ;
            _agent->syncSend( handle, &(msg.header.header),
                              (void*)commitObj.objdata(),
                              commitObj.objsize() ) ;
         }
         else
         {
            _agent->syncSend( handle, &msg ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSDSBS__SYNCLOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__SYNCLOG, "_clsDataSrcBaseSession::_syncLog" )
   INT32 _clsDataSrcBaseSession::_syncLog( const NET_HANDLE &handle,
                                           SINT64 packet,
                                           const MsgRouteID &routeID,
                                           UINT32 TID, UINT64 requestID )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS__SYNCLOG );
      MsgClsFSNotifyRes msg ;
      msg.header.res = SDB_OK ;
      msg.header.header.TID = TID ;
      msg.header.header.requestID = requestID ;
      msg.header.header.routeID = routeID ;
      msg.packet = packet ;
      msg.type = CLS_FS_NOTIFY_TYPE_LOG ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      INT32 rc = SDB_OK ;
      BOOLEAN bEnd = FALSE ;
      time_t bTime = time( NULL ) ;
      _dataType = CLS_FS_NOTIFY_TYPE_LOG ;
      BOOLEAN retryTime = 0 ;

   retry:
      bEnd = FALSE ;
      _mb.clear() ;
      while ( TRUE )
      {
         _LSNlatch.get () ;
         if ( _deqLSN.size() > 0 )
         {
            _lsn.offset = _deqLSN.front() ;
            _deqLSN.pop_front() ;
         }
         else
         {
            bEnd = TRUE ;
         }
         _LSNlatch.release () ;

         if ( bEnd )
         {
            break ;
         }

         // search for a given lsn
         rc = dpsCB->search( _lsn, &_mb ) ;
         if ( SDB_OK == rc )
         {
            dpsLogRecordHeader *header = (dpsLogRecordHeader * )_mb.readPtr() ;
            _mb.readPtr( _mb.length() ) ;

            _lsn.offset += header->_length ;
            _lsn.version = header->_version ;

            /// We should update the begin lsn offset. Because in notifyLSN(),
            /// when extLID is invalid, add ZERO to _beginLSNOffset for
            /// perfermance
            if ( header->_lsn == _beginLSNOffset )
            {
               _beginLSNOffset += header->_length ;
            }

            // if we read something making the mb length exceed threshold, let's
            // break out from the loop
            if ( CLS_SYNC_MAX_LEN <= _mb.length() ||
                 ( time( NULL ) - bTime >= CLS_SYNC_MAX_TIME &&
                   _mb.length() > 0 ) )
            {
               break ;
            }
         }
         else
         {
            // if other errors happened during search, let's dump error
            PD_LOG ( PDERROR, "Session[%s]: Failed to search LSN[%lld,%d], "
                     "rc: %d", sessionName(), _lsn.offset, _lsn.version, rc ) ;
            goto error ;
         }
      }

      if ( 0 != _mb.length() )
      {
         msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                           _mb.length() ;
         _agent->syncSend( handle, &(msg.header.header),
                           _mb.offset( 0 ), _mb.length() ) ;
      }
      else if ( _canSwitchWhenSyncLog() )
      {
         if ( !_findEnd )
         {
            rc = _syncRecord( handle, packet, routeID, TID, requestID ) ;
         }
         else
         {
            rc = _syncLob( handle, packet, routeID, TID, requestID ) ;
         }

         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to sync %s, rc: %d",
                    sessionName(), ( _findEnd ? "Lob" : "Record" ), rc ) ;
            goto error ;
         }
      }
      else if ( retryTime < 5 )
      {
         ++retryTime ;
         ossSleep( 200 ) ;
         goto retry ;
      }
      else
      {
         _canResend = FALSE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSDSBS__SYNCLOG, rc );
      return rc ;
   error:
      _disconnect () ;
      goto done ;
   }

  // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS__SYNCRECD, "_clsDataSrcBaseSession::_syncRecord" )
  INT32 _clsDataSrcBaseSession::_syncRecord( const NET_HANDLE &handle,
                                             SINT64 packet,
                                             const MsgRouteID &routeID,
                                             UINT32 TID,
                                             UINT64 requestID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS__SYNCRECD );
      SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
      time_t bTime = time( NULL ) ;
      _dataType = CLS_FS_NOTIFY_TYPE_DOC ;

      MsgClsFSNotifyRes msg ;
      msg.header.header.TID = TID ;
      msg.header.header.routeID = routeID ;
      msg.header.res = SDB_OK ;
      msg.header.header.requestID = requestID ;
      msg.packet = packet ;
      msg.type = CLS_FS_NOTIFY_TYPE_DOC ;

      _mb.clear () ;
      _query = NULL ;
      _queryLen = 0 ;

      UINT32 tempSize = 0 ;
      UINT32 alignLen = 0 ;
      rtnContextBuf buffObj ;

      /// mask sure the context wether exist
      if ( -1 == _contextID )
      {
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
      }

      while ( SDB_OK == rc )
      {
         rc = _context->getMBContext()->mbLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            break ;
         }
         rc = _context->getMore( -1, buffObj, eduCB() ) ;
         if ( SDB_OK == rc )
         {
            if ( TBSCAN == _scanType() )
            {
               _curExtID = _context->lastExtLID() ;
               PD_LOG ( PDDEBUG, "Session[%s]: scan logical extent id: %d",
                        sessionName(), _curExtID ) ;
            }
            else
            {
               _curScanKeyObj = _context->getIXScanner()->getSavedObj()->copy() ;
               PD_LOG ( PDDEBUG, "Session[%s]: scan cur key obj: %s",
                        sessionName(), _curScanKeyObj.toString().c_str() ) ;
            }

            if ( _context->eof() )
            {
               _findEnd = TRUE ;
            }
            _context->getMBContext()->mbUnlock() ;

            _query = _onObjFilter( buffObj.data(), buffObj.size(), _queryLen ) ;

            if ( _queryLen > 0 && ( (UINT32)_queryLen < CLS_SYNC_MAX_LEN ||
                 _mb.length() != 0 ) )
            {
               alignLen = ossRoundUpToMultipleX (_queryLen, sizeof( UINT32 )) ;
               // add to mb block
               if ( _mb.idleSize() < alignLen )
               {
                  tempSize = ossRoundUpToMultipleX ( alignLen - _mb.idleSize(),
                                                     sizeof( UINT32 ) ) ;
                  rc = _mb.extend ( tempSize ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Session[%s]: Failed to extend mb "
                              "size[%d]", sessionName(), tempSize ) ;
                     break ;
                  }
               }

               // copy
               ossMemcpy ( _mb.writePtr(), _query, _queryLen ) ;
               _mb.writePtr ( _mb.length() + alignLen ) ;
            }

            if ( _mb.length () >= CLS_SYNC_MAX_LEN ||
                 (UINT32)_queryLen  >= CLS_SYNC_MAX_LEN ||
                 ( time( NULL ) - bTime >= CLS_SYNC_MAX_TIME &&
                   _mb.length() > 0 ) )
            {
               break ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            _findEnd = TRUE ;
         }
      }

      if ( _context )
      {
         _context->getMBContext()->mbUnlock() ;
      }

      // release context id when found error or found end
      if ( ( SDB_OK != rc || _findEnd ) && -1 != _contextID )
      {
         pRtnCB->contextDelete( _contextID, eduCB() ) ;
         _contextID = -1 ;
         _context = NULL ;
      }

      if ( _mb.length () != 0 )
      {
         _queryLen = _mb.length () ;
         _query = _mb.startPtr () ;
      }

      if ( SDB_DMS_EOC == rc ||
           SDB_RTN_CONTEXT_NOTEXIST == rc ||
           SDB_DMS_NOTEXIST == rc ||
           SDB_DMS_TRUNCATED == rc )
      {
         // End the scan in below cases :
         // 1. hit end of collection
         // 2. contextID is already closed
         // 3. collection dropped or collection truncated
         _findEnd = TRUE ;

         if ( 0 == _queryLen )
         {
            msg.eof = CLS_FS_EOF ;
            msg.lsn = pmdGetKRCB()->getDPSCB()->expectLsn () ;

            _LSNlatch.get () ;
            if ( _deqLSN.size() > 0 )
            {
               msg.lsn.offset = _deqLSN.front() ;
            }
            else
            {
               msg.lsn.offset = _beginLSNOffset ;
            }
            _LSNlatch.release() ;
            _agent->syncSend( handle, &msg ) ;
         }
         else
         {
            msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                              _queryLen ;
            _agent->syncSend( handle, &(msg.header.header),
                              (void*)_query, _queryLen ) ;
         }
         rc = SDB_OK ;
      }
      else if ( SDB_OK != rc )
      {
         // otherwise if we hit other error, let's dump it
         PD_LOG ( PDERROR, "Session[%s]: Failed to run GetMore, rc = %d",
                  sessionName(), rc ) ;
      }
      else
      {
         // if everything is fine, let's send out the data
         msg.header.header.messageLength = sizeof( MsgClsFSNotifyRes ) +
                                    _queryLen ;
         _agent->syncSend( handle, &(msg.header.header), (void*)_query,
                           _queryLen ) ;
      }

      PD_TRACE_EXITRC ( SDB__CLSDSBS__SYNCRECD, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS_HNDFSMETA, "_clsDataSrcBaseSession::handleFSMeta" )
   INT32 _clsDataSrcBaseSession::handleFSMeta( NET_HANDLE handle,
                                               MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS_HNDFSMETA );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      MsgClsFSMetaReq *msg = ( MsgClsFSMetaReq * )header ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      _dmsStorageUnit *su = NULL ;
      dmsMBContext *mbContext = NULL ;
      UINT64 curCollection = ~0 ;
      UINT32 csLID = DMS_INVALID_LOGICCSID ;
      UINT16 mbID = DMS_INVALID_MBID ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

      MsgClsFSMetaRes res ;
      res.header.header.TID = header->TID ;
      res.header.res = SDB_OK ;
      res.header.header.routeID = header->routeID ;
      res.header.header.requestID = header->requestID ;

      /// a collection sync will be done only when recv a notify
      /// over msg. then can start a new sync.
      if ( !_init )
      {
         PD_LOG( PDWARNING, "Session[%s]: not init, disconnect",
                 sessionName() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                 sessionName(), rc ) ;
         goto error ;
      }

      /// steps of a collection syncing:
      /// 1. parse msg.
      /// 2. lock su.
      /// 3. get indexs.
      /// 4. open context.
      /// 5. unlock su.
      /// any setp fails, we should release resources.
      try
      {
         BSONObj meta ;
         BSONObj obj( (CHAR *)(&(msg->header)) +
                       sizeof(MsgClsFSMetaReq) ) ;
         PD_LOG( PDEVENT, "Session[%s]: get meta req [%s]", sessionName(),
                 obj.toString().c_str() ) ;
         CHAR cs[DMS_COLLECTION_SPACE_NAME_SZ + 1] = { 0 } ;
         CHAR collection[DMS_COLLECTION_NAME_SZ + 1] = { 0 } ;

         if ( SDB_OK != ( rc = _getCSName( obj, cs,
                               DMS_COLLECTION_SPACE_NAME_SZ + 1) ) )
         {
            goto error ;
         }

         if ( SDB_OK != ( rc = _getCollection( obj, collection,
                                 DMS_COLLECTION_NAME_SZ + 1) ) )
         {
            goto error ;
         }

         if ( SDB_OK != ( rc = _getRangeKey( obj ) ) )
         {
            goto error ;
         }
         rc = rtnGetIntElement( obj, CLS_FS_NEEDDATA, _needData ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         PD_LOG ( PDEVENT, "Session[%s]: Begin collection[%s.%s], NeedData:%u",
                  sessionName(), cs, collection, _needData ) ;

         _curCollecitonName = cs ;
         _curCollecitonName += "." ;
         _curCollecitonName += collection ;

         /// clean indexes
         _indexs.clear() ;

         rc = dmsCB->nameToSUAndLock( cs, suID, &su ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Session[%s]: Lock collectionspace[%s] failed, "
                    "rc: %d", sessionName(), cs, rc ) ;
            goto error ;
         }

         // get cur cs and cl logical id, unique id
         rc = su->data()->getMBContext( &mbContext, collection, SHARED ) ;
         if ( rc )
         {
            PD_LOG ( PDWARNING, "Session[%s]: Lock collection[%s] failed, "
                     "rc: %d", sessionName(), collection, rc ) ;
            goto error ;
         }

         clUniqueID = mbContext->mb()->_clUniqueID ;
         csLID = su->LogicalCSID() ;
         mbID = mbContext->mbID() ;
         curCollection = ossPack32To64 ( su->LogicalCSID(),
                                         mbContext->clLID() ) ;
         /// get indexes
         rc = su->getIndexes( mbContext, _indexs ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Session[%s]:Failed to get indexs of "
                    "collecton[%s], rc:%d", sessionName(),
                    collection, rc ) ;
            goto error ;
         }
         {
            // Lock _LSNlatch to avoid other write operations, e.g. createIndex
            // on this collection before updating _curCollection
            ossScopedLock lock( &_LSNlatch ) ;

            su->data()->releaseMBContext( mbContext ) ;

            /// erase index of "_id"
            _eraseDefaultIndex() ;

            // must release su lock, because _openContext will get lock again
            dmsCB->suUnlock ( suID ) ;
            suID = DMS_INVALID_SUID ;

            rc = _onFSMeta( _curCollecitonName.c_str() ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( SDB_OK != ( rc = _openContext( cs, collection ) ) )
            {
               goto error ;
            }
            _curCollection = curCollection ;
            _curCSLID = csLID ;
            _curMBID = mbID ;
         }
         _constructMeta( meta, cs, collection, clUniqueID, su ) ;
         PD_LOG( PDDEBUG, "Session[%s]: get meta [%s]", sessionName(),
                 meta.toString().c_str() ) ;
         res.header.header.messageLength = sizeof( MsgClsFSMetaRes ) +
                                           meta.objsize() ;
         if ( SDB_OK == _agent->syncSend( handle, &(res.header.header),
                          ( void * )meta.objdata(), meta.objsize() ) )
         {
            _hasMeta = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected exception: %s",
                 sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( su && mbContext )
      {
         su->data()->releaseMBContext( mbContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__CLSDSBS_HNDFSMETA, rc );
      return rc ;
   error:
      if ( SDB_DMS_CS_NOTEXIST == rc || SDB_DMS_NOTEXIST == rc )
      {
         // NOTE: could not ignore SDB_DMS_TRUNCATED, in that case, meta-data
         // of indexes might not be dumped, so disconnect and let the
         // destination node to restart the sync process
         res.header.res = rc ;
         res.header.header.messageLength = sizeof ( MsgClsFSMetaRes ) ;
         if ( SDB_OK == _agent->syncSend ( handle, (void*)&res ) )
         {
            _hasMeta = TRUE ;
         }
         rc = SDB_OK ;
         _curCollection = ~0 - 1 ;
         _curCSLID = DMS_INVALID_LOGICCSID ;
         _curMBID = DMS_INVALID_MBID ;
      }
      else
      {
         _disconnect () ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS_HNDFSINX, "_clsDataSrcBaseSession::handleFSIndex" )
   INT32 _clsDataSrcBaseSession::handleFSIndex( NET_HANDLE handle,
                                                MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSDSBS_HNDFSINX );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      MsgClsFSIndexRes res ;
      BSONObj obj ;
      if ( !_hasMeta )
      {
         _disconnect() ;
         PD_LOG( PDWARNING, "Session[%s]: not get meta data yet.",
                 sessionName() ) ;
         goto done ;
      }
      else if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                 sessionName(), rc ) ;
         _disconnect () ;
         goto done ;
      }

      res.header.header.TID = header->TID ;
      res.header.res = SDB_OK ;
      res.header.header.routeID = header->routeID ;
      res.header.header.requestID = header->requestID ;
      _constructIndex( obj ) ;
      PD_LOG( PDEVENT, "Session[%s]: sync indexes [%s]", sessionName(),
              obj.toString().c_str() ) ;
      res.header.header.messageLength = sizeof( MsgClsFSIndexRes ) +
                                        obj.objsize() ;
      _agent->syncSend( handle, &(res.header.header),
                        (void *)obj.objdata(), obj.objsize() ) ;

   done:
      PD_TRACE_EXIT ( SDB__CLSDSBS_HNDFSINX );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSDSBS_HNDFSNTF, "_clsDataSrcBaseSession::handleFSNotify" )
   INT32 _clsDataSrcBaseSession::handleFSNotify( NET_HANDLE handle,
                                                 MsgHeader* header )
   {
      PD_TRACE_ENTRY ( SDB__CLSDSBS_HNDFSNTF );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      MsgClsFSNotify *msg = ( MsgClsFSNotify * )header ;
      INT32 rc = SDB_OK ;
      if ( !_init || ( !_hasMeta && CLS_FS_NOTIFY_TYPE_DOC == msg->type ) )
      {
         _disconnect() ;
         PD_LOG( PDWARNING, "Session[%s]: not init or not get meta data yet",
                 sessionName() ) ;
         goto done ;
      }
      else if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                 sessionName(), rc ) ;
         _disconnect () ;
         goto done ;
      }

      /// the sync of this collection is done.
      /// prepare for the next sync.
      if ( CLS_FS_NOTIFY_TYPE_OVER == msg->type )
      {
         PD_LOG( PDEVENT, "Session[%s]: Collection[%s] notify end",
                 sessionName(), _curCollecitonName.c_str() ) ;

         _LSNlatch.get();
         try
         {
            _mapOveredCLs[_curCollection] = 0 ;
         }
         catch( std::exception &e )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Session[%s]: Add complete collection occur "
                    "exception: %s", sessionName(), e.what() ) ;
         }
         _resetInfo( FALSE ) ;
         _LSNlatch.release() ;

         if ( rc )
         {
            _disconnect() ;
         }
         else
         {
            rc = _onNotifyOver( _curCollecitonName.c_str() ) ;
            if ( rc )
            {
               _disconnect() ;
            }
         }
         goto done ;
      }

      /// wrong packet. ignore it.
      if ( msg->packet < _packetID )
      {
         PD_LOG( PDWARNING, "Session[%s]: wrong packet id [remote:%lld]"
                 "[local:%lld]", sessionName(), msg->packet, _packetID ) ;
         goto done ;
      }
      /// msg was delayed or lost.
      /// resend last msg.
      else if ( msg->packet == _packetID && _canResend )
      {
         PD_LOG( PDWARNING, "Session[%s]: msg was delayed or lost. resend "
                 "packet. [packet:%lld][type:%d]", sessionName(),
                 msg->packet, msg->type ) ;
         _resend( handle, msg ) ;
         goto done ;
      }

      _packetID = msg->packet ;
      _canResend= TRUE ;

      if ( CLS_FS_NOTIFY_TYPE_DOC == msg->type )
      {
         rc = _syncRecord( handle, msg->packet, header->routeID,
                           header->TID, header->requestID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: failed to sync doc, rc: %d",
                    sessionName(), rc ) ;
            _disconnect() ;
            goto done ;
         }
      }
      else if ( CLS_FS_NOTIFY_TYPE_LOG == msg->type )
      {
         rc = _syncLog( handle, msg->packet,
                        header->routeID, header->TID, header->requestID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: failed to sync log, rc: %d",
                    sessionName(), rc ) ;
            _disconnect() ;
            goto done ;
         }
      }
      else
      {
         rc = _syncLob( handle, msg->packet,
                        header->routeID, header->TID, header->requestID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Session[%s]: failed to sync lob, rc: %d",
                    sessionName(), rc ) ;
            _disconnect() ;
            goto done ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSDSBS_HNDFSNTF );
      return SDB_OK ;
   }

   INT32 _clsDataSrcBaseSession::_buildCLCommitInfo( const string &fullName,
                                                     BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCLShortName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      dmsMBContext *pContext = NULL ;
      BSONObjBuilder builder ;

      rc = rtnResolveCollectionNameAndLock( fullName.c_str(), dmsCB, &su,
                                            &pCLShortName, suID ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Session[%s]: Get collectionspace lock failed "
                 "for collection[%s], rc: %d", sessionName(),
                 fullName.c_str(), rc ) ;
         goto error ;
      }
      rc = su->data()->getMBContext( &pContext, pCLShortName, SHARED ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Session[%s]: Get collection[%s]'s mblock "
                 "failed, rc: %d", sessionName(), fullName.c_str(), rc ) ;
         goto error ;
      }

      builder.append( CLS_FS_FULLNAME, fullName ) ;
      builder.append( CLS_FS_COMMITFLAG,
                      BSON_ARRAY( (INT32)pContext->mb()->_commitFlag <<
                                  (INT32)pContext->mb()->_idxCommitFlag <<
                                  (INT32)pContext->mb()->_lobCommitFlag ) ) ;
      builder.append( CLS_FS_COMMITLSN,
                      BSON_ARRAY( (INT64)pContext->mb()->_commitLSN <<
                                  (INT64)pContext->mb()->_idxCommitLSN <<
                                  (INT64)pContext->mb()->_lobCommitLSN ) ) ;
      obj = builder.obj() ;

   done:
      if ( pContext )
      {
         su->data()->releaseMBContext( pContext ) ;
      }
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
   _clsFSSrcSession : implement
   */
   BEGIN_OBJ_MSG_MAP( _clsFSSrcSession, _clsDataSrcBaseSession )
      //ON_MSG
      ON_MSG( MSG_CLS_FULL_SYNC_BEGIN, handleBegin )
      ON_MSG( MSG_CLS_FULL_SYNC_END, handleEnd )
      ON_MSG( MSG_CLS_FULL_SYNC_TRANS_REQ, handleSyncTransReq )
   END_OBJ_MSG_MAP()

   _clsFSSrcSession::_clsFSSrcSession( UINT64 sessionID,
                                       _netRouteAgent *agent )
   :_clsDataSrcBaseSession( sessionID, agent ),
    _lsnSearchMB( 1024 )
   {
      _lastRecvSlice = 1 ;
   }

   _clsFSSrcSession::~_clsFSSrcSession()
   {
      rtnGetExtDataHandler()->enable() ;
      PD_LOG( PDEVENT, "External data handler resumed" ) ;
   }

   SDB_SESSION_TYPE _clsFSSrcSession::sessionType() const
   {
      return SDB_SESSION_FS_SRC ;
   }

   EDU_TYPES _clsFSSrcSession::eduType () const
   {
      return  EDU_TYPE_REPLAGENT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSSS_HNDBEGIN, "_clsFSSrcSession::handleBegin" )
   INT32 _clsFSSrcSession::handleBegin( NET_HANDLE handle, MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSFSSS_HNDBEGIN );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      INT32 nomore = 1 ;

      SDB_DPSCB *dpscb = pmdGetKRCB()->getDPSCB() ;
      MsgClsFSBeginRes msg ;
      msg.header.header.TID = header->TID ;
      msg.header.header.requestID = header->requestID ;
      msg.header.header.routeID = header->routeID ;
      msg.header.res = SDB_OK ;

      if ( _hasMeta || _mapOveredCLs.size() > 0 )
      {
         PD_LOG( PDWARNING, "Session[%s] already hasMeta, can't begin, "
                 "disconnect", sessionName() ) ;
         _disconnect() ;
         goto done ;
      }

      //if not ready, can't be the source node of the full sync
      if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], refused",
                 sessionName(), rc ) ;
         msg.header.res = rc ;
         _quit = TRUE ;
         _agent->syncSend( handle, &msg ) ;
         goto done ;
      }

      /// When the first begin slice, need to clear validCLs
      if ( 1 == _lastRecvSlice )
      {
         _validCLs.clear() ;
      }

      /// compatiable with old version( without obj )
      if ( (UINT32)(header->messageLength) > sizeof( MsgClsFSBegin ) )
      {
         try
         {
            INT32 slice = 1 ;    /// default is 1
            BSONObj bodyObj( (const CHAR*)header + sizeof( MsgClsFSBegin ) ) ;
            rc = _extractBeginBody( bodyObj, _validCLs, nomore, slice ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Session[%s]: Extract request body data "
                       "failed, rc: %d", sessionName(), rc ) ;
               _disconnect() ;
               goto done ;
            }
            else if ( _lastRecvSlice != slice )
            {
               PD_LOG( PDERROR, "Session[%s]: Begin request with slice[%u] "
                       "is not expected[%u], disconnect session",
                       sessionName(), slice, _lastRecvSlice ) ;
               _disconnect() ;
               goto done ;
            }

            /// Expect next slice
            ++_lastRecvSlice ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Session[%s]: Extract request body data "
                    "occur exception: %s", sessionName(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            _disconnect() ;
            goto done ;
         }
      }

      if ( nomore == 0 )
      {
         /// Begin request is not complete
         goto done ;
      }

      /// steps:
      /// begin
      /// loop: meta
      ///       indexes
      ///       loop: notify
      /// end

      {
         _resetInfo() ;
         BSONObj obj ;
         INT32 sendSlice = 1 ;
         INT32 sendNomore = 1 ;
         MON_CS_LIST csList ;
         MON_CL_LIST clList ;
         BOOLEAN needDisconnect = FALSE ;

         /// get expcect lsn
         _lsn = dpscb->expectLsn() ;
         msg.lsn = _lsn ;
         _beginLSNOffset = _lsn.offset ;

         // release notify lsn que, and prevent create cs/cl, drop cs/cl,
         // rename cl, truncate cl and so on occur during construct full names
         // and set _init = TRUE
         ossScopedLock lock( &_LSNlatch ) ;
         _deqLSN.clear() ;

         /// Notify fullsync, So will kick the node from sync control nodes.
         /// In _processValidCLs, need to get lock of collection, If has some
         /// operators hold the lock and in sync control, will occur dead wait
         _pRepl->syncMgr()->notifyFullSync( header->routeID ) ;

         /// process valid collections
         rc = _processValidCLs( _validCLs ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Session[%s]: Process valid collections "
                    "failed, rc: %d", sessionName(), rc ) ;
            _disconnect() ;
            goto done ;
         }

         // New collections may be created when constructing the respond data.
         // So maybe they are not in the collection list in the respond message.
         // In that case, we rely on the replication log to create them on slave
         // nodes. Set _init to TRUE to make sure the log can be pushed to notify
         // queue( in function notifyLSN ).
         _init =  TRUE ;

         sendNomore = 0 ;
         sendSlice = 0 ;
         _lastRecvSlice = 1 ;

         while( 0 == sendNomore && SDB_OK == rc )
         {
            ++sendSlice ;
            needDisconnect = FALSE ;
            rc = _constructBeginRspData( sendSlice, obj, csList, clList,
                                         _validCLs, sendNomore ) ;
            if ( rc )
            {
               needDisconnect = TRUE ;
            }
            else
            {
               msg.header.header.messageLength = sizeof( msg ) + obj.objsize() ;
               rc = _agent->syncSend( handle, &(msg.header.header),
                                      (void*)obj.objdata(),
                                      obj.objsize() ) ;
            }
         }

         /// reset timeCounter
         _timeCounter = 0 ;

         /// Failed
         if ( rc )
         {
            if ( sendSlice != 1 || needDisconnect )
            {
               _disconnect() ;
            }
            _mapOveredCLs.clear() ;
            _validCLs.clear() ;
            _init = FALSE ;
         }
         /// Succeed
         else
         {
            PD_LOG( PDEVENT, "Session[%s]: Established the full sync session "
                    "with node[%s], Expect LSN:[%d:%lld]", sessionName(),
                    routeID2String( header->routeID ).c_str(),
                    _lsn.version, _lsn.offset ) ;

            // Disable external data handler. Currently external handler is
            // about text indices. As capped collections for text indices are
            // required to be exactly the same on primary and slave nodes, and
            // during full sync, the original collection and the capped
            // collection are syncing seperately by themselves. If any write
            // operation happened on primary capped collection, it will finally
            // result in data mismatch on primary and salve. So we directly
            // disable the external handler during the full sync. It will be
            // enabled after full sync is done or aborted.
            rc = rtnGetExtDataHandler()->disable( CLS_FS_DISABLE_EXTDATA_TIME ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Disable external data handler failed[ %d ]",
                       rc ) ;
               _disconnect() ;
               goto done ;
            }

            PD_LOG( PDEVENT, "Disable external data handler during "
                    "full sync" ) ;

            _quit = FALSE ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSFSSS_HNDBEGIN );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSSS_HNDEND, "_clsFSSrcSession::handleEnd" )
   INT32 _clsFSSrcSession::handleEnd( NET_HANDLE handle,
                                      MsgHeader* header )
   {
      PD_TRACE_ENTRY ( SDB__CLSFSSS_HNDEND );
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      MsgClsFSEndRes msg ;
      msg.header.header.TID = header->TID ;
      msg.header.header.routeID = header->routeID ;
      msg.header.header.requestID = header->requestID ;

      if ( SDB_OK == _agent->syncSend( handle, &msg ) )
      {
         PD_LOG( PDEVENT, "Session[%s]: Full sync has been done",
                 sessionName() ) ;
         _quit = TRUE ;
      }

      rtnGetExtDataHandler()->enable() ;

   //done:
      PD_TRACE_EXIT ( SDB__CLSFSSS_HNDEND );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSSS_HNDTRANSREQ, "_clsFSSrcSession::handleSyncTransReq" )
   INT32 _clsFSSrcSession::handleSyncTransReq( NET_HANDLE handle,
                                               MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSFSSS_HNDTRANSREQ );
      MsgClsFSTransSyncReq *pMsg = (MsgClsFSTransSyncReq *)header;
      MsgClsFSTransSyncRes msgRsp;
      msgRsp.header.header.requestID = pMsg->header.requestID;
      msgRsp.header.header.routeID = pMsg->header.routeID;
      msgRsp.header.header.TID = pMsg->header.TID;
      _lsn = pMsg->begin;
      time_t bTime = time( NULL ) ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;

      if ( !_init )
      {
         PD_LOG( PDWARNING, "Session[%s]: not init, disconnect",
                 sessionName() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                 sessionName(), rc ) ;
         goto error ;
      }

      if ( DPS_INVALID_LSN_OFFSET == _lsn.offset )
      {
         _lsn.offset = pmdGetKRCB()->getTransCB()->getOldestBeginLsn();
      }
      if ( DPS_INVALID_LSN_OFFSET == _lsn.offset )
      {
         _lsn.offset = pmdGetKRCB()->getDPSCB()->getStartLsn( TRUE ).offset ;
      }
      _mb.clear();
      while ( _lsn.offset != DPS_INVALID_LSN_OFFSET
            && _lsn.compareOffset( pMsg->endExpect.offset) < 0 )
      {
         rc = dpsCB->search( _lsn, &_mb );
         PD_RC_CHECK( rc, PDERROR, "Failed to search LSN[%lld,%d], rc: %d",
                      _lsn.offset, _lsn.version, rc ) ;
         dpsLogRecordHeader *header =
                     ( dpsLogRecordHeader * )_mb.readPtr();
         _mb.readPtr( _mb.length() );
         _lsn.offset += header->_length ;
         _lsn.version = header->_version ;

         if ( CLS_SYNC_MAX_LEN <= _mb.length() ||
              ( time( NULL ) - bTime >= CLS_SYNC_MAX_TIME &&
                _mb.length() > 0 ) )
         {
            break;
         }
      }
      if ( _mb.length() == 0 )
      {
         msgRsp.eof = CLS_FS_EOF;
      }

      // send reponse
      if ( _mb.length() != 0  && SDB_OK == rc )
      {
         msgRsp.header.header.messageLength
                     = sizeof( MsgClsFSTransSyncRes ) + _mb.length() ;
         _agent->syncSend( handle, &(msgRsp.header.header), _mb.offset( 0 ),
                           _mb.length() );
      }
      else
      {
         _agent->syncSend( handle, &(msgRsp.header.header) );
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSFSSS_HNDTRANSREQ ) ;
      return rc ;
   error:
      _disconnect() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSFSSS_NTFLSN, "_clsFSSrcSession::notifyLSN" )
   INT32 _clsFSSrcSession::notifyLSN ( UINT32 suLID, UINT32 clLID,
                                       dmsExtentID extLID,
                                       const DPS_LSN_OFFSET &offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSFSSS_NTFLSN );
      UINT64 fullCLLID = ossPack32To64 ( suLID, clLID ) ;
      map<UINT64, UINT32>::iterator it ;
      BOOLEAN needRelease = FALSE ;
      UINT32 lsnLen = 0 ;

      if ( !_init || _quit || offset < _beginLSNOffset )
      {
         goto done ;
      }

      _LSNlatch.get() ;
      needRelease = TRUE ;

      PD_LOG ( PDINFO, "Session[%s]: dps notify[suLID:%d, clLID:%d, "
               "extLID:%d, offset:%lld], curScan extLID:%d", sessionName(),
               suLID, clLID, extLID, offset, _curExtID ) ;

      // already complete collection
      it = _mapOveredCLs.find ( fullCLLID ) ;

      if ( DMS_INVALID_EXTENT == extLID ||
           it != _mapOveredCLs.end() )
      {
         _deqLSN.push_back ( offset ) ;
      }
      else if ( fullCLLID == _curCollection )
      {
         const dpsLogRecordHeader *rh = NULL ;
         DPS_LSN lsn ;
         lsn.offset = offset ;
         SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
         _lsnSearchMB.clear() ;
         INT32 rc = dpsCB->searchHeader( lsn, &_lsnSearchMB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Session[%s]: Failed to load dps "
                     "log[offset:%lld, rc:%d]", sessionName(), offset, rc ) ;
            goto error ;
         }

         rh = ( const dpsLogRecordHeader * )( _lsnSearchMB.startPtr() ) ;
         lsnLen = rh->_length ;
         if ( CLS_IS_LOB_LOG( rh->_type ) )
         {
            if ( !_findEnd )
            {
               goto done ;
            }
            else if ( _lobFetcher.hitEnd() )
            {
               _deqLSN.push_back ( offset ) ;
            }
            else if ( extLID < _lobFetcher.toBeFetched() )
            {
               _deqLSN.push_back ( offset ) ;
            }
            else
            {
               goto done ;
            }
         }
         else if ( _findEnd || extLID <= _curExtID )
         {
            _deqLSN.push_back ( offset ) ;
         }
         else
         {
            goto done ;
         }
      }
   done:
      if ( needRelease )
      {
         _beginLSNOffset = offset + lsnLen ;
         _LSNlatch.release () ;
      }
      PD_TRACE_EXIT ( SDB__CLSFSSS_NTFLSN );
      return SDB_OK ;
   error:
      goto done ;
   }

   INT32 _clsFSSrcSession::_onFSMeta( const CHAR * clFullName )
   {
      return SDB_OK ;
   }

   INT32 _clsFSSrcSession::_onNotifyOver( const CHAR *clFullName )
   {
      return SDB_OK ;
   }

   INT32 _clsFSSrcSession::_scanType() const
   {
      return TBSCAN ;
   }

   BOOLEAN _clsFSSrcSession::_canSwitchWhenSyncLog()
   {
      return TRUE ;
   }

   INT32 _clsFSSrcSession::_isReady()
   {
      INT32 rc = SDB_OK ;

      /// 1. not primary
      if ( !pmdIsPrimary() )
      {
         /* MSG_INVALID_ROUTEID != sdbGetReplCB()->getPrimary().value */
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is not "
                 "primary", sessionName() ) ;
      }
      /// 2. in full sync
      else if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() )
      {
         rc = SDB_CLS_FULL_SYNC ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "already in full sync", sessionName() ) ;
      }
      /// 3. in rebuild
      else if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
      {
         rc = SDB_RTN_IN_REBUILD ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "already in rebuilding", sessionName() ) ;
      }
      /// 4. business is not ok
      else if ( !pmdGetStartup().isOK () )
      {
         rc = SDB_RTN_IN_REBUILD ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "not recoverd from crash", sessionName() ) ;
      }
      /// 5. unique id upgrade is not finished
      else if ( pmdGetKRCB()->getDMSCB()->nullCSUniqueIDCnt() > 0 )
      {
         rc = SDB_DMS_UNQIUEID_UPGRADE ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Upgrade for unique id "
                 "is not finished", sessionName() ) ;
      }

      return rc ;
   }

   const CHAR* _clsFSSrcSession::_onObjFilter( const CHAR * inBuff,
                                               INT32 inSize,
                                               INT32 & outSize )
   {
      outSize = inSize ;
      return inBuff ;
   }

   INT32 _clsFSSrcSession::_onLobFilter( const bson::OID &oid,
                                         UINT32 sequence,
                                         BOOLEAN &need2Send )
   {
      need2Send = TRUE ;
      return SDB_OK ;
   }

   INT32 _clsFSSrcSession::_constructBeginRspData( INT32 slice,
                                                   BSONObj &obj,
                                                   MON_CS_LIST &csList,
                                                   MON_CL_LIST &clList,
                                                   MAP_SU_STATUS &validCLs,
                                                   INT32 &nomore )
   {
      INT32 rc = SDB_OK ;
      UINT32 count = 0 ;

      /// When the first, need to dump info from dmsCB
      if ( 1 == slice )
      {
         rc = pmdGetKRCB()->getDMSCB()->dumpInfo( csList, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = pmdGetKRCB()->getDMSCB()->dumpInfo( clList, TRUE ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      nomore = 1 ;

      try
      {
         BSONObjBuilder b( 65536 ) ;
         INT32 curLen = 0 ;
         INT32 curReserved = 0 ;

         /// reserved for CLS_FS_FULLNAMES and CLS_FS_VALIDCLS
         b.bb().reserveBytes( 100 + 100 ) ;

         b.append( CLS_FS_NOMORE, nomore ) ;
         b.append( CLS_FS_SLICE, slice ) ;

         // empty space
         BSONArrayBuilder csArrayBD( b.subarrayStart( CLS_FS_CSNAMES ) ) ;

         MON_CS_LIST::iterator itCS = csList.begin() ;
         while( itCS != csList.end() )
         {
            if ( 0 == itCS->_collections.size() )
            {
               curLen = b.bb().len() ;
               curReserved = b.bb().getReserveBytes() ;

               if ( curLen + curReserved > CLS_FS_MAX_BSON_SIZE )
               {
                  break ;
               }

               try
               {
                  BSONObjBuilder subObj( csArrayBD.subobjStart() ) ;
                  subObj.append( CLS_FS_CSNAME, itCS->_name ) ;
                  subObj.append( CLS_FS_PAGE_SIZE, itCS->_pageSize ) ;
                  subObj.append( CLS_FS_LOB_PAGE_SIZE, itCS->_lobPageSize ) ;
                  subObj.append( CLS_FS_CS_TYPE, itCS->_type ) ;
                  subObj.append( CLS_FS_CS_UNIQUEID, itCS->_csUniqueID ) ;
                  subObj.done() ;
               }
               catch( std::exception &e )
               {
                  PD_LOG( PDWARNING, "Build collectionspace information "
                          "occur exception: %s", e.what() ) ;
                  b.bb().setlen( curLen ) ;
                  b.bb().setReserveBytes( curReserved ) ;
                  break ;
               }
            }
            csList.erase( itCS++ ) ;
            ++count ;
         }
         csArrayBD.done() ;

         // collection list
         b.bb().claimReservedBytes( 100 ) ;

         BSONArrayBuilder clArrayBD( b.subarrayStart( CLS_FS_FULLNAMES ) ) ;
         MON_CL_LIST::const_iterator itrCL = clList.begin() ;
         while( itrCL != clList.end() )
         {
            curLen = b.bb().len() ;
            curReserved = b.bb().getReserveBytes() ;

            if ( curLen + curReserved > CLS_FS_MAX_BSON_SIZE )
            {
               break ;
            }

            try
            {
               BSONObjBuilder subObj( clArrayBD.subobjStart() ) ;
               subObj.append( CLS_FS_FULLNAME, itrCL->_name ) ;
               subObj.done() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "Build collection name occur exception: %s",
                       e.what() ) ;
               b.bb().setlen( curLen ) ;
               b.bb().setReserveBytes( curReserved ) ;
               break ;
            }
            clList.erase( itrCL++ ) ;
            ++count ;
         }
         clArrayBD.done() ;

         /// valid collection list
         b.bb().claimReservedBytes( 100 ) ;

         BSONArrayBuilder validArrayBD( b.subarrayStart( CLS_FS_VALIDCLS ) ) ;
         MAP_SU_STATUS::iterator itValid = validCLs.begin() ;
         while( itValid != validCLs.end() )
         {
            curLen = b.bb().len() ;
            curReserved = b.bb().getReserveBytes() ;

            if ( curLen + curReserved > CLS_FS_MAX_BSON_SIZE )
            {
               break ;
            }

            try
            {
               BSONObjBuilder subObj( validArrayBD.subobjStart() ) ;
               subObj.append( CLS_FS_FULLNAME, itValid->first ) ;
               subObj.done() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDWARNING, "Build valid collection name occur "
                       "exception: %s", e.what() ) ;
               b.bb().setlen( curLen ) ;
               b.bb().setReserveBytes( curReserved ) ;
               break ;
            }
            validCLs.erase( itValid++ ) ;
            ++count ;
         }
         validArrayBD.done() ;

         obj = b.obj() ;

         PD_LOG( PDDEBUG, "Session[%s]: get fullnames: [%s]",
                 sessionName(), obj.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: unexpected err: %s",
                 sessionName(), e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !csList.empty() || !clList.empty() || !validCLs.empty() )
      {
         if ( 0 == count )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Session[%s]: Build begin response data "
                    "failed, rc: %d", sessionName(), rc ) ;
            goto error ;
         }

         /// change nomore to 0
         nomore = 0 ;
         BSONElement e = obj.getField( CLS_FS_NOMORE ) ;
         *(INT32*)e.value() = nomore ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsFSSrcSession::_hasExternalData() const
   {
      BOOLEAN result = FALSE ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      MON_CS_SIM_LIST csList ;

      dmsCB->dumpInfo( csList, FALSE, TRUE, TRUE ) ;
      for ( MON_CS_SIM_LIST::iterator csItr = csList.begin();
            csItr != csList.end(); ++csItr )
      {
         for ( MON_CL_SIM_VEC::const_iterator clItr = csItr->_clList.begin();
               clItr != csItr->_clList.end(); ++clItr )
         {
            for ( MON_IDX_LIST::const_iterator idxItr = clItr->_idxList.begin();
                  idxItr != clItr->_idxList.end(); ++idxItr )
            {
               INT32 rcTmp = SDB_OK ;
               UINT16 idxType = IXM_EXTENT_TYPE_NONE ;
               rcTmp = idxItr->getIndexType( idxType ) ;
               if ( rcTmp )
               {
                  // Only warning, and process the remaining ones.
                  PD_LOG( PDERROR, "Get type of index failed[ %d ], cs[ %s ],"
                                   " cl[ %s ], index[ %s ]",
                          rcTmp, csItr->_name, clItr->_name,
                          idxItr->getIndexName() ) ;
                  continue;
               }
               // Only dump text indices in normal status.
               if ( IXM_EXTENT_TYPE_TEXT == idxType )
               {
                  result = TRUE;
                  goto done ;
               }
            }
         }
      }
   done:
      return result ;
   }

   INT32 _clsFSSrcSession::_extractBeginBody( const BSONObj &obj,
                                              MAP_SU_STATUS &validCLs,
                                              INT32 &nomore,
                                              INT32 &slice )
   {
      INT32 rc = SDB_OK ;
      rtnRUInfo info ;

      try
      {
         BSONElement e = obj.getField( CLS_FS_NOMORE ) ;
         if ( e.eoo() )
         {
            nomore = 1 ;
         }
         else if ( e.isNumber() )
         {
            nomore = e.numberInt() ;
         }
         else
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to parse %s form obj[%s]",
                    sessionName(), CLS_FS_NOMORE, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         e = obj.getField( CLS_FS_SLICE ) ;
         if ( e.eoo() )
         {
            slice = 1 ;    /// defaul is 1
         }
         else if ( e.isNumber() )
         {
            slice = e.numberInt() ;
         }
         else
         {
            PD_LOG( PDERROR, "Session[%s]: Failed to parse %s form obj[%s]",
                    sessionName(), CLS_FS_SLICE, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         BSONElement eleValidCL = obj.getField( CLS_FS_VALIDCLS ) ;
         if ( Array != eleValidCL.type() )
         {
            PD_LOG( PDWARNING, "Session[%s]: Parse %s from obj[%s] failed: "
                    "type error", sessionName(), CLS_FS_VALIDCLS,
                    obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         BSONObjIterator itr( eleValidCL.embeddedObject() ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( Object != e.type() )
            {
               PD_LOG( PDWARNING, "Session[%s]: Parse valid cl from obj[%s] "
                       "failed: type error", sessionName(),
                       e.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            BSONObj objCL = e.embeddedObject() ;
            BSONElement eleName = objCL.getField( CLS_FS_FULLNAME ) ;
            BSONElement eleFlag = objCL.getField( CLS_FS_COMMITFLAG ) ;
            BSONElement eleLSN = objCL.getField( CLS_FS_COMMITLSN ) ;

            if ( String != eleName.type() ||
                 Array != eleFlag.type() ||
                 Array != eleLSN.type() )
            {
               PD_LOG( PDERROR, "Session[%s]: Parse valid cl from obj[%s] "
                       "failed: type error", sessionName(),
                       e.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            vector< BSONElement > vecFlag = eleFlag.Array() ;
            vector< BSONElement > vecLSN = eleLSN.Array() ;
            if ( vecFlag.size() < 3 || vecLSN.size() < 3 )
            {
               PD_LOG( PDERROR, "Session[%s]: Parse valid cl from obj[%s] "
                       "failed: invalid commit info", sessionName(),
                       e.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            info._dataCommitFlag = vecFlag[0].numberInt() ;
            info._idxCommitFlag = vecFlag[1].numberInt() ;
            info._lobCommitFlag = vecFlag[2].numberInt() ;

            info._dataCommitLSN = vecLSN[0].numberLong() ;
            info._idxCommitLSN = vecLSN[1].numberLong() ;
            info._lobCommitLSN = vecLSN[2].numberLong() ;

            validCLs[ eleName.String() ] = info ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Session[%s]: Parse valid collections occur "
                 "exception: %s", sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsFSSrcSession::_processValidCLs( MAP_SU_STATUS &validCLs )
   {
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      INT32 rc = SDB_OK ;
      const CHAR *pCLShortName = NULL ;
      dmsMBContext *pContext = NULL ;
      UINT64 curCollection = ~0 ;

      MAP_SU_STATUS::iterator it = validCLs.begin() ;
      while( it != validCLs.end() )
      {
         rtnRUInfo &info = it->second ;

         /// not all valid, remove
         if ( !info.isAllValid() )
         {
            validCLs.erase( it++ ) ;
            continue ;
         }

         rc = rtnResolveCollectionNameAndLock( it->first.c_str(), dmsCB,
                                               &su, &pCLShortName, suID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Session[%s]: Lock collectionspace failed, "
                    "rc: %d, ignored this collection[%s]", sessionName(),
                    rc, it->first.c_str() ) ;
            validCLs.erase( it++ ) ;
            rc = SDB_OK ;
            continue ;
         }

         /// get mb context
         rc = su->data()->getMBContext( &pContext, pCLShortName, SHARED ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING, "Session[%s]: Get collection's mblock failed, "
                    "rc: %d, ignored this collection[%s]", sessionName(),
                    rc, it->first.c_str() ) ;
            dmsCB->suUnlock( suID ) ;
            validCLs.erase( it++ ) ;
            rc = SDB_OK ;
            continue ;
         }

         /// compare
         UINT64 maxLSN = pContext->mbStat()->_lastLSN.peek() ;
         if ( DPS_INVALID_LSN_OFFSET != pContext->mbStat()->_idxLastLSN.peek() &&
              pContext->mbStat()->_idxLastLSN.peek() > maxLSN )
         {
            maxLSN = pContext->mbStat()->_idxLastLSN.peek() ;
         }
         if ( DPS_INVALID_LSN_OFFSET != pContext->mbStat()->_lobLastLSN.peek() &&
              pContext->mbStat()->_lobLastLSN.peek() > maxLSN )
         {
            maxLSN = pContext->mbStat()->_lobLastLSN.peek() ;
         }

         if ( maxLSN != info.maxLSN() ||
              DPS_INVALID_LSN_OFFSET == maxLSN )
         {
            PD_LOG( PDWARNING, "Session[%s]: Collection[%s]'s lsn[%lld] is "
                    "not the same with local[%lld], ignored", sessionName(),
                    it->first.c_str(), info.maxLSN(), maxLSN ) ;
            su->data()->releaseMBContext( pContext ) ;
            dmsCB->suUnlock( suID ) ;
            validCLs.erase( it++ ) ;
            continue ;
         }
         else
         {
            PD_LOG( PDEVENT, "Session[%s]: Collection[%s]'s lsn[%lld] is the "
                    "same with local, add to valid list", sessionName(),
                    it->first.c_str(), maxLSN ) ;

            curCollection = ossPack32To64 ( su->LogicalCSID(),
                                            pContext->clLID() ) ;
            /// add to complete list
            try
            {
               _mapOveredCLs[curCollection] = 0 ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Session[%s]: Add complete collection occur "
                       "exception: %s", sessionName(), e.what() ) ;
               rc = SDB_OOM ;
            }
         }

         su->data()->releaseMBContext( pContext ) ;
         dmsCB->suUnlock( suID ) ;
         ++it ;

         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
   _clsSplitSrcSession : implement
   */
   BEGIN_OBJ_MSG_MAP( _clsSplitSrcSession, _clsDataSrcBaseSession )
      //ON_MSG
      ON_MSG( MSG_CLS_FULL_SYNC_BEGIN, handleBegin )
      ON_MSG( MSG_CLS_FULL_SYNC_END, handleEnd )
      ON_MSG( MSG_CLS_FULL_SYNC_LEND_REQ, handleLEnd )
   END_OBJ_MSG_MAP()

   _clsSplitSrcSession::_clsSplitSrcSession( UINT64 sessionID,
                                             _netRouteAgent *agent )
   :_clsDataSrcBaseSession( sessionID, agent ), _filterMB ( 1024 ),
    _lsnSearchMB( 1024 )
   {
      _pCatAgent = sdbGetShardCB()->getCataAgent() ;
      _pFreezingWindow = sdbGetShardCB()->getFreezingWindow() ;
      _cleanupJobID = PMD_INVALID_EDUID ;

      _hasShardingIndex = FALSE ;
      _hashShard        = FALSE ;
      _hasEndRange      = TRUE ;
      _partitionBit     = 0 ;

      _taskID           = 0 ;
      _ntyOverTime      = 0 ;
      _lastEndNtyOffset = DPS_INVALID_LSN_OFFSET ;
      _getLastEndNtyOffset = FALSE ;
      _collectionW      = 1 ;
      _lastOprLSN       = DPS_INVALID_LSN_OFFSET ;
      _internalV        = 0 ;
   }

   _clsSplitSrcSession::~_clsSplitSrcSession()
   {
      _pCatAgent = NULL ;
      _pFreezingWindow = NULL ;
   }

   SDB_SESSION_TYPE _clsSplitSrcSession::sessionType() const
   {
      return SDB_SESSION_SPLIT_SRC ;
   }

   EDU_TYPES _clsSplitSrcSession::eduType() const
   {
      return EDU_TYPE_SHARDAGENT ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS_NTFLSN, "_clsSplitSrcSession::notifyLSN" )
   INT32 _clsSplitSrcSession::notifyLSN( UINT32 suLID, UINT32 clLID,
                                         dmsExtentID extLID,
                                         const DPS_LSN_OFFSET & offset )
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLSS_NTFLSN );
      INT32 rc = SDB_OK ;
      UINT32 curSULID, curCLLID ;
      BOOLEAN locked = FALSE ;
      BOOLEAN inEndMap = FALSE ;
      dpsLogRecord record ;
      DPS_LSN lsn ;
      UINT32 lsnLen = 0 ;
      BSONObj recordObj ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;

      if ( !_init || _quit || PMD_INVALID_EDUID != _cleanupJobID ||
           0 == _needData || offset < _beginLSNOffset )
      {
         goto done ;
      }

      _LSNlatch.get() ;
      locked = TRUE ;

      // Access to _curCollection should be protected by _LSNlatch
      ossUnpack32From64( _curCollection, curSULID, curCLLID ) ;

      if ( (UINT64)~0 != _curCollection )
      {
         // not the cs
         if ( suLID != curSULID )
         {
            goto done ;
         }
         // not the cl or it is a drop cs
         if ( ( (UINT32)~0 != clLID && curCLLID != clLID ) ||
              ( (UINT32)~0 == clLID ) )
         {
            goto done ;
         }
      }
      else if ( _mapOveredCLs.find ( ossPack32To64 ( suLID, clLID ) ) ==
                _mapOveredCLs.end() )
      {
         goto done ;
      }
      else
      {
         inEndMap = TRUE ;
      }

      // if the lsn is in my self, and the extLID not invalid, need to read lsn
      // from file
      if ( DMS_INVALID_EXTENT == extLID )
      {
         _deqLSN.push_back( offset ) ;
         goto done ;
      }

      lsn.offset = offset ;
      _lsnSearchMB.clear() ;
      rc = dpsCB->search( lsn, &_lsnSearchMB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s]: Failed to load dps "
                  "log[offset:%lld, rc:%d]", sessionName(), offset, rc ) ;
         goto error ;
      }

      rc = record.load( _lsnSearchMB.startPtr() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s]: parse dps log failed[rc:%d]",
                  sessionName(), rc ) ;
         goto error ;
      }
      lsnLen = record.head()._length ;

      // no sharding index, scan by table
      // log of lob can be ignored, coz _findEnd is false.
      if ( TBSCAN == _scanType() && !inEndMap && !_findEnd &&
           extLID > _curExtID && !( CLS_IS_LOB_LOG( record.head()._type ) ) )
      {
         goto done ;
      }
      else if ( _hashShard && CLS_IS_LOB_LOG( record.head()._type ) )
      {
         if ( inEndMap ||
              _lobFetcher.hitEnd() ||
              extLID < _lobFetcher.toBeFetched() )
         {
            BOOLEAN need2Notify = FALSE ;
            const bson::OID *oid = NULL ;
            const UINT32 *sequence = NULL ;
            dpsLogRecord::iterator itr = record.find( DPS_LOG_LOB_OID ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: can not find oid obj in "
                      "record.", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            oid = ( const bson::OID * )( itr.value() ) ;

            itr = record.find( DPS_LOG_LOB_SEQUENCE ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: can not find oid obj in "
                      "record.", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            sequence = ( const UINT32 * )( itr.value() ) ;

            rc = _onLobFilter( *oid, *sequence, need2Notify ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Session[%s]: can not filter the log "
                       , sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( need2Notify )
            {
               _deqLSN.push_back( offset ) ;
            }
         }
         else
         {
            /// will sync this record by _syncLob.
         }

         goto done ;
      }
      else if ( !CLS_IS_LOB_LOG( record.head()._type ) )
      {
         if ( LOG_TYPE_DATA_INSERT == record.head()._type )
         {
            dpsLogRecord::iterator itr = record.find( DPS_LOG_INSERT_OBJ ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "Session[%s]: can not find insert obj in "
                       "record.", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            recordObj = BSONObj( itr.value() ) ;
         }
         else if ( LOG_TYPE_DATA_DELETE == record.head()._type )
         {
            dpsLogRecord::iterator itr = record.find( DPS_LOG_DELETE_OLDOBJ ) ;
            if ( !itr.valid() )
            {
               PD_LOG( PDERROR, "Session[%s] can not find delete obj in "
                       "record.", sessionName() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            recordObj = BSONObj( itr.value() ) ;
         }
         else if ( LOG_TYPE_DATA_UPDATE == record.head()._type )
         {
            _deqLSN.push_back( offset ) ;
            goto done ;
         }

         //jduge the object is less than the _curScanObj
         {
            BSONObj keyObj ;
            rc = _genKeyObj( recordObj, keyObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( _GEThanRangeKey( keyObj ) && _LThanRangeEndKey( keyObj ) &&
                 ( _findEnd || inEndMap ||
                  ( IXSCAN == _scanType() && _LEThanScanObj( keyObj ) ) ||
                  ( TBSCAN == _scanType() && extLID <= _curExtID ) ) )
            {
               _deqLSN.push_back( offset ) ;
               /*PD_LOG( PDERROR, "Session[%s]: push queue: %s, curObj: "
                       "%s, rangeKey: %s, findEnd: %d, contextID: %lld",
                       sessionName(), keyObj.toString().c_str(),
                       _curScanKeyObj.toString().c_str(),
                       _rangeKeyObj.toString().c_str(),
                       _findEnd,
                       _contextID ) ;*/
            }
            /*else
            {
               PD_LOG( PDERROR, "Session[%s] :dispatch: %s, curObj: %s, "
                       "rangeKey: %s, findEnd: %d, contextID: %lld",
                       sessionName(), keyObj.toString().c_str(),
                       _curScanKeyObj.toString().c_str(),
                       _rangeKeyObj.toString().c_str(),
                       _findEnd,
                       _contextID ) ;
            }*/
         }
      }

   done:
      if ( locked )
      {
         _beginLSNOffset = offset + lsnLen ;
         _LSNlatch.release() ;
      }
      PD_TRACE_EXIT ( SDB__CLSSPLSS_NTFLSN );
      return SDB_OK ;
   error:
      _disconnect() ;
      goto done ;
   }

   #define CLS_SPLIT_UPCATELOG_RETRY_TIME       ( 2 )
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ONFSMETA, "_clsSplitSrcSession::_onFSMeta" )
   INT32 _clsSplitSrcSession::_onFSMeta( const CHAR * clFullName )
   {
      //get sharding key
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__ONFSMETA );
      _clsCatalogSet *catSet = NULL ;
      UINT32 count = 0 ;
      UINT32 groupID = sdbGetShardCB()->nodeID().columns.groupID ;
      BSONObj upBound ;

      /// exist $shard index
      _hasShardingIndex = _existIndex( IXM_SHARD_KEY_NAME ) ;

      //check range key
      if ( _rangeKeyObj.isEmpty() )
      {
         PD_LOG ( PDERROR, "Session[%s] range key obj is empty",
                  sessionName() ) ;
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      //update the newest catalog,and get sharding key
      while ( count < CLS_SPLIT_UPCATELOG_RETRY_TIME )
      {
         ++count ;

         rc = sdbGetShardCB()->syncUpdateCatalog( clFullName, OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            _pCatAgent->lock_r() ;
            catSet = _pCatAgent->collectionSet( clFullName ) ;
            if ( catSet )
            {
               _shardingKey = catSet->OwnedShardingKey() ;
               _hashShard   = catSet->isHashSharding() ;
               _partitionBit = catSet->getPartitionBit() ;
               _internalV = catSet->getInternalV() ;
               catSet->getGroupUpBound( groupID , upBound ) ;
            }
            else
            {
               rc = SDB_CLS_NO_CATALOG_INFO ;
            }
            _pCatAgent->release_r() ;
            break ;
         }
      }

      if ( !_rangeEndKeyObj.isEmpty() /*&&
           ( TRUE == _hashShard ||
             _rangeEndKeyObj.woCompare( upBound, BSONObj(), false ) != 0 ) */ )
      {
         _hasEndRange = TRUE ;
      }
      else
      {
         _hasEndRange = FALSE ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSSPLSS__ONFSMETA, rc );
      return rc ;
   }

   INT32 _clsSplitSrcSession::_onNotifyOver( const CHAR *clFullName )
   {
      INT32 rc = SDB_OK ;

      if ( _ntyOverTime > 0 )
      {
         /// Blocking has been started, the sync process might be restarted
         /// by drop or truncate of collection, need to unblock the previous
         /// one
         PD_LOG( PDDEBUG, "Session[%s]: Blocking write operations of "
                 "collection [%s] has been started, end blocking for further "
                 "process", sessionName(), clFullName ) ;

         _pFreezingWindow->unregisterCL( clFullName, _ntyOverTime ) ;
         PD_LOG( PDEVENT, "Session[%s]: End to block all write operations "
                 "of collection[%s], ID: %llu", sessionName(), clFullName,
                 _ntyOverTime ) ;
      }

      _ntyOverTime = 0 ;

      /// Block collection and main-collection ( if have ) together
      rc = _pFreezingWindow->registerCL( clFullName, _ntyOverTime ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Session[%s]: Block all write operations of "
                 "collection[%s] failed, rc: %d",
                 sessionName(), clFullName, rc ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Session[%s]: Begin to block all write operations "
                 "of collection[%s], ID: %llu", sessionName(), clFullName,
                 _ntyOverTime ) ;
      }

      return rc ;
   }

   INT32 _clsSplitSrcSession::_scanType() const
   {
      if ( !_hashShard && _hasShardingIndex )
      {
         return IXSCAN ;
      }
      return TBSCAN ;
   }

   BOOLEAN _clsSplitSrcSession::_canSwitchWhenSyncLog()
   {
      if ( _ntyOverTime > 0 )
      {
         pmdEDUMgr *pEDUMgr = eduCB()->getEDUMgr() ;
         dpsTransLockId lockID( _curCSLID, _curMBID, NULL ) ;
         if ( pEDUMgr->getWritingEDUCount( -1, _ntyOverTime,
                                           EDU_BLOCK_FREEZING_WND,
                                           lockID ) > 0 )
         {
            PD_LOG( PDINFO, "Session[%s] operator ID [%llu] : Waiting for "
                    "other operations to finish", sessionName(),
                    _ntyOverTime ) ;
            return FALSE ;
         }

         if ( FALSE == _getLastEndNtyOffset )
         {
            _getLastEndNtyOffset = TRUE ;
            _lastEndNtyOffset = sdbGetReplCB()->getNtyLastOffset() ;
         }

         if ( DPS_INVALID_LSN_OFFSET != _lastEndNtyOffset )
         {
            DPS_LSN_OFFSET processed = sdbGetReplCB()->getNtyProcessedOffset() ;
            if ( DPS_INVALID_LSN_OFFSET == processed ||
                 processed < _lastEndNtyOffset )
            {
               return FALSE ;
            }
            else
            {
               ossScopedLock lock( &_LSNlatch ) ;
               if ( _deqLSN.size() > 0 )
               {
                  return FALSE ;
               }
            }
         }
      }

      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS_HNDBEGIN, "_clsSplitSrcSession::handleBegin" )
   INT32 _clsSplitSrcSession::handleBegin( NET_HANDLE handle,
                                           MsgHeader* header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS_HNDBEGIN );
      PD_LOG( PDEVENT, "Session[%s]: Begin for split", sessionName() ) ;

      MsgClsFSBeginRes msg ;
      msg.header.header.TID = header->TID ;
      msg.header.header.requestID = header->requestID ;
      msg.header.header.routeID = header->routeID ;
      msg.header.res = SDB_OK ;

      if ( _hasMeta || _mapOveredCLs.size() > 0 )
      {
         PD_LOG( PDWARNING, "Session[%s]: already hasMeta, can't begin, "
                 "disconnect", sessionName() ) ;
         _disconnect() ;
         goto done ;
      }

      //can't be self group
      if ( header->routeID.columns.groupID ==
           sdbGetShardCB()->nodeID().columns.groupID )
      {
         PD_LOG ( PDERROR, "Session[%s]: the source and dst node is in "
                  "same group", sessionName() ) ;
         _disconnect () ;
         goto done ;
      }

      //if not ready, can't be the source node of split
      if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] not ready[%d], refused",
                 sessionName(), rc ) ;
         _disconnect () ;
         goto done ;
      }

      {
         _resetInfo() ;
         _lsn = pmdGetKRCB()->getDPSCB()->expectLsn() ;
         msg.lsn = _lsn ;
         _beginLSNOffset = _lsn.offset ;

         // release notify lsn que, and prevent create cs/cl, drop cs/cl,
         // rename cl, truncate cl and so on occur during construct full names
         // and set _init = TRUE
         ossScopedLock lock( &_LSNlatch ) ;
         _deqLSN.clear() ;

         msg.header.header.messageLength = sizeof( msg ) ;
         if ( SDB_OK == _agent->syncSend( handle, &(msg.header.header) ) )
         {
            PD_LOG( PDEVENT, "Session[%s]: Established the split session "
                    "with node[%s]", sessionName(),
                    routeID2String( header->routeID ).c_str() ) ;
            _init = TRUE ;
            _quit = FALSE ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLSS_HNDBEGIN );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS_HNDEND, "_clsSplitSrcSession::handleEnd" )
   INT32 _clsSplitSrcSession::handleEnd( NET_HANDLE handle, MsgHeader * header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS_HNDEND );
      INT32 cleanResult = SDB_OK ;

      //need to start clean up job
      if ( PMD_INVALID_EDUID == _cleanupJobID )
      {
         if ( !_init )
         {
            PD_LOG( PDWARNING, "Session[%s]: not init, disconnect",
                    sessionName() ) ;
            _disconnect() ;
            goto done ;
         }
         else if ( SDB_OK != ( rc = _isReady() ) )
         {
            PD_LOG( PDWARNING, "Session[%s] is not ready[%d], "
                    "disconnect", sessionName(), rc ) ;
            _disconnect() ;
            goto done ;
         }

         if ( SDB_OK != startCleanupJob( _curCollecitonName,
                                         UTIL_UNIQUEID_NULL,
                                         _rangeKeyObj,
                                         _hasEndRange ? _rangeEndKeyObj :
                                                        BSONObj(),
                                         _hasShardingIndex, _hashShard,
                                         pmdGetKRCB()->getDPSCB(),
                                         &_cleanupJobID, TRUE ) )
         {
            _disconnect () ;
            goto done ;
         }
         // sleep one second
         ossSleep( OSS_ONE_SEC ) ;
      }

      //check cleanup whether is over
      if ( PMD_INVALID_EDUID != _cleanupJobID &&
           !rtnGetJobMgr()->findJob( _cleanupJobID, &cleanResult ) )
      {
         if ( cleanResult )
         {
            PD_LOG( PDWARNING, "Session[%s] cleanup data failed, rc: %d",
                    sessionName(), cleanResult ) ;
            _disconnect () ;
            goto done ;
         }

         if ( DPS_INVALID_LSN_OFFSET == _lastOprLSN )
         {
            _lastOprLSN = pmdGetKRCB()->getDPSCB()->getCurrentLsn().offset ;
         }

         // need wait the group other nodes sync complete
         if ( _collectionW > 1 )
         {
            INT32 rc = sdbGetReplCB()->sync( _lastOprLSN,
                                             eduCB(), _collectionW, 1 ) ;
            if ( SDB_TIMEOUT == rc )
            {
               goto done ;
            }
         }

         MsgClsFSEndRes res ;
         res.header.header.requestID = header->requestID ;
         res.header.header.routeID = header->routeID ;
         res.header.header.TID = header->TID ;
         res.header.res = SDB_OK ;

         if ( SDB_OK == _agent->syncSend( handle, (MsgHeader*)&res ) )
         {
            PD_LOG( PDEVENT, "Session[%s]: end for split",
                    sessionName() ) ;
            _quit = TRUE ;
            _cleanupJobID = PMD_INVALID_EDUID ;
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLSS_HNDEND ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS_HNDEND2, "_clsSplitSrcSession::handleLEnd" )
   INT32 _clsSplitSrcSession::handleLEnd( NET_HANDLE handle,
                                          MsgHeader * header )
   {
      //need to update catalog, and check the rangKeyObj is not in my self
      PD_TRACE_ENTRY ( SDB__CLSSPLSS_HNDEND2 );
      _clsCatalogSet *pSet = NULL ;
      shardCB *pShard = sdbGetShardCB() ;
      UINT32 groupID = pShard->nodeID().columns.groupID ;
      BOOLEAN hasSplit = FALSE ;
      INT32 rc = SDB_OK ;
      std::string mainCLName;

      if ( !_init )
      {
         PD_LOG( PDWARNING, "Session[%s]: not init, disconnect",
                 sessionName() ) ;
         rc = SDB_SYS ;
         _disconnect() ;
         goto done ;
      }
      else if ( SDB_OK != ( rc = _isReady() ) )
      {
         PD_LOG( PDWARNING, "Session[%s] is not ready[%d], disconnect",
                 sessionName(), rc ) ;
         _disconnect() ;
         goto done ;
      }

      rc = pShard->syncUpdateCatalog( _curCollecitonName.c_str(),
                                      OSS_ONE_SEC ) ;
      if ( SDB_OK != rc && SDB_DMS_NOTEXIST != rc )
      {
         goto done ;
      }

      _pCatAgent->lock_r() ;        //lock
      pSet = _pCatAgent->collectionSet( _curCollecitonName.c_str() ) ;
      if ( pSet )
      {
         mainCLName = pSet->getMainCLName() ;
         _collectionW = pSet->getW() ;
      }
      if ( !pSet || !pSet->isKeyInGroup( _rangeKeyObj, groupID ) )
      {
         hasSplit = TRUE ;
      }
      _pCatAgent->release_r() ;     //unlock
      if ( !mainCLName.empty() )
      {
         INT32 rcTmp = pShard->syncUpdateCatalog( mainCLName.c_str(),
                                                  OSS_ONE_SEC ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Session[%s]: update main-collection(%s) "
                    "failed, rc: %d", sessionName(), mainCLName.c_str(),
                    rcTmp ) ;
         }
      }

      if ( hasSplit )
      {
         /// Unblock
         _pFreezingWindow->unregisterCL( _curCollecitonName.c_str(),
                                         _ntyOverTime ) ;
         PD_LOG( PDEVENT, "Session[%s]: End to block all write operations "
                 "of collection[%s], ID: %llu", sessionName(),
                 _curCollecitonName.c_str(), _ntyOverTime ) ;

         _ntyOverTime = 0 ;

         MsgClsFSLEndRes res ;
         res.header.header.requestID = header->requestID ;
         res.header.header.routeID = header->routeID ;
         res.header.header.TID = header->TID ;
         res.header.res = SDB_OK ;

         if ( SDB_OK == _agent->syncSend( handle, (MsgHeader *)&res ) )
         {
            pmdGetKRCB()->getClsCB()->invalidateCata(
               _curCollecitonName.c_str() ) ;
            if ( !mainCLName.empty() )
            {
               pmdGetKRCB()->getClsCB()->invalidateCata(
                  mainCLName.c_str() ) ;
            }
         }
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLSS_HNDEND2 );
      return SDB_OK ;
   }

   INT32 _clsSplitSrcSession::_isReady ()
   {
      INT32 rc = SDB_OK ;

      /// 1. not primary
      if ( !pmdIsPrimary() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "not primary", sessionName() ) ;
      }
      /// 2. In full sync
      else if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() )
      {
         rc = SDB_CLS_FULL_SYNC ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "in full sync", sessionName() ) ;
      }
      /// 3. In rebuild
      else if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
      {
         rc = SDB_RTN_IN_REBUILD ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "in rebuilding", sessionName() ) ;
      }
      /// 4. not recoved from crash
      else if ( !pmdGetStartup().isOK () )
      {
         rc = SDB_RTN_IN_REBUILD ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Self node is "
                 "not recovered from crash", sessionName() ) ;
      }
      /// 5. unique id upgrade is not finished
      else if ( pmdGetKRCB()->getDMSCB()->nullCSUniqueIDCnt() > 0 )
      {
         rc = SDB_DMS_UNQIUEID_UPGRADE ;
         PD_LOG( PDWARNING, "Session[%s] not ready: Upgrade for unique id "
                 "is not finished", sessionName() ) ;
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__GENKEYOBJ, "_clsSplitSrcSession::_genKeyObj" )
   INT32 _clsSplitSrcSession::_genKeyObj( const BSONObj & obj,
                                          BSONObj & keyObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__GENKEYOBJ );
      BSONObjSet objSet ;
      ixmIndexKeyGen keyGen ( _shardingKey ) ;

      rc = keyGen.getKeys( obj , objSet ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Session[%s] gen sharding key obj from %s "
                  "failed[rc:%d]", sessionName(), obj.toString().c_str(), rc ) ;
         goto error ;
      }
      if ( objSet.size() != 1 )
      {
         PD_LOG ( PDERROR, "Session[%s]: More than one sharding key[%d] "
                  "is detected[%s]", sessionName(), objSet.size(),
                  obj.toString().c_str() ) ;
         rc = SDB_MULTI_SHARDING_KEY ;
         goto error ;
      }
      keyObj = *objSet.begin() ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSPLSS__GENKEYOBJ, rc );
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _clsSplitSrcSession::_containMultiKey ( const BSONObj & obj )
   {
      BSONObjIterator iter( _shardingKey ) ;
      while ( iter.more() )
      {
         BSONElement key = iter.next() ;
         BSONElement field = obj.getField( key.fieldName() ) ;
         if ( field.type() == Array &&
              field.embeddedObject().nFields() > 1 )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   BOOLEAN _clsSplitSrcSession::_GEThanRangeKey( const BSONObj & keyObj )
   {
      INT32 rc = 0 ;

      if ( _hashShard )
      {
         INT32 partition = clsPartition( keyObj, _partitionBit, _internalV ) ;
         INT32 beginRange = _rangeKeyObj.firstElement().numberInt() ;
         rc = partition - beginRange ;
      }
      else
      {
         Ordering order = Ordering::make( _shardingKey ) ;
         rc = keyObj.woCompare( _rangeKeyObj, order, FALSE ) ;
      }

      return rc >= 0 ? TRUE : FALSE ;
   }

   BOOLEAN _clsSplitSrcSession::_LThanRangeEndKey( const BSONObj & keyObj )
   {
      INT32 rc = -1 ;

      if ( _hasEndRange )
      {
         if ( _hashShard )
         {
            INT32 partition = clsPartition( keyObj, _partitionBit, _internalV ) ;
            INT32 endRange = _rangeEndKeyObj.firstElement().numberInt() ;
            rc = partition - endRange ;
         }
         else
         {
            Ordering order = Ordering::make( _shardingKey ) ;
            rc = keyObj.woCompare( _rangeEndKeyObj, order, FALSE ) ;
         }
      }
      return rc < 0 ? TRUE : FALSE ;
   }

   BOOLEAN _clsSplitSrcSession::_LEThanScanObj( const BSONObj &keyObj )
   {
      SDB_ASSERT( IXSCAN == _scanType(), "Must be IXSCAN" ) ;

      Ordering order = Ordering::make( _shardingKey ) ;
      INT32 rc = keyObj.woCompare( _curScanKeyObj, order, FALSE ) ;

      return rc <= 0 ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ADD2FMB, "_clsSplitSrcSession::_addToFilterMB" )
   INT32 _clsSplitSrcSession::_addToFilterMB( const CHAR * data, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__ADD2FMB );
      UINT32 alignSize = ossRoundUpToMultipleX ( size, 4 ) ;

      if ( _filterMB.idleSize() < alignSize )
      {
         UINT32 tempSize = ossRoundUpToMultipleX ( alignSize-_filterMB.idleSize(),
                                                   4 ) ;
         //alloc memry
         rc = _filterMB.extend( tempSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Session[%s] failed to extent message "
                     "block[size:%d, rc:%d]", sessionName(), tempSize, rc ) ;
            goto error ;
         }
      }

      //copy
      ossMemcpy( _filterMB.writePtr(), data, size ) ;
      _filterMB.writePtr( _filterMB.length() + alignSize ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSSPLSS__ADD2FMB, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ONOBJFLT, "_clsSplitSrcSession::_onObjFilter" )
   const CHAR* _clsSplitSrcSession::_onObjFilter( const CHAR * inBuff,
                                                  INT32 inSize,
                                                  INT32 & outSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__ONOBJFLT );

      const CHAR *outBuff = inBuff ;

      INT32 indexPos = 0 ;
      const CHAR *pBuffIndex = inBuff + indexPos;

      // split by index, all is correct, but need to check arrays
      if ( IXSCAN == _scanType() && FALSE == _hasEndRange )
      {
         try
         {
            while ( indexPos < inSize )
            {
               BSONObj recordObj ( pBuffIndex ) ;

               if ( _containMultiKey( recordObj ) )
               {
                  PD_LOG ( PDERROR, "Session[%s]: More than one sharding key "
                           "is detected[%s]", sessionName(),
                           recordObj.toString().c_str() ) ;
                  rc = SDB_MULTI_SHARDING_KEY ;
                  goto error ;
               }

               indexPos += ossRoundUpToMultipleX ( recordObj.objsize(), 4 ) ;
               pBuffIndex = inBuff + indexPos ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG ( PDERROR, "Session[%s]: filter object exception: %s",
                     sessionName(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         outSize = inSize ;
         goto done ;
      }

      _filterMB.clear() ;

      try
      {
         while ( indexPos < inSize )
         {
            BSONObj recordObj ( pBuffIndex ) ;

            //jduge the obj whether is correct, if ok push to filterMB
            BSONObj keyObj ;
            rc = _genKeyObj( recordObj, keyObj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( _GEThanRangeKey( keyObj ) && _LThanRangeEndKey( keyObj ) )
            {
               rc = _addToFilterMB ( pBuffIndex, recordObj.objsize() ) ;
            }

            if ( SDB_OK != rc )
            {
               goto error ;
            }

            indexPos += ossRoundUpToMultipleX ( recordObj.objsize(), 4 ) ;
            pBuffIndex = inBuff + indexPos ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG ( PDERROR, "Session[%s]: filter object exception: %s",
                  sessionName(), e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      outSize = (INT32)_filterMB.length() ;
      outBuff = _filterMB.startPtr() ;

   done:
      PD_TRACE_EXIT ( SDB__CLSSPLSS__ONOBJFLT );
      return outBuff ;
   error:
      PD_LOG( PDERROR, "Session[%s]: Filter obj failed[%d], disconnect",
              sessionName(), rc ) ;
      outSize = 0 ;
      _disconnect() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ONATH, "_clsSplitSrcSession::_onAttach" )
   void _clsSplitSrcSession::_onAttach ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__ONATH );
      _clsDataSrcBaseSession::_onAttach() ;

      // add empty split task to start timmer
      _clsTaskMgr *taskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
      _clsDummyTask *pTask = SDB_OSS_NEW _clsDummyTask ( taskMgr->getTaskID() ) ;
      if ( pTask && SDB_OK == taskMgr->addTask( pTask ) )
      {
         _taskID = pTask->taskID() ;
      }
      else
      {
         _quit = TRUE ;
      }
      PD_TRACE_EXIT ( SDB__CLSSPLSS__ONATH );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ONDTH, "_clsSplitSrcSession::_onDetach" )
   void _clsSplitSrcSession::_onDetach ()
   {
      PD_TRACE_ENTRY ( SDB__CLSSPLSS__ONDTH );
      _clsDataSrcBaseSession::_onDetach() ;

      // remove the empty split task
      _clsTaskMgr *taskMgr = pmdGetKRCB()->getClsCB()->getTaskMgr() ;
      if ( 0 != _taskID )
      {
         taskMgr->removeTask( _taskID ) ;
      }

      if ( _ntyOverTime > 0 )
      {
         _pFreezingWindow->unregisterCL( _curCollecitonName.c_str(),
                                         _ntyOverTime ) ;
         PD_LOG( PDEVENT, "Session[%s]: End to block all write operations "
                 "of collection[%s], ID: %llu", sessionName(),
                 _curCollecitonName.c_str(), _ntyOverTime ) ;
      }

      // wait cleanup done
      while ( PMD_INVALID_EDUID != _cleanupJobID &&
              rtnGetJobMgr()->findJob( _cleanupJobID ) )
      {
         ossSleep( OSS_ONE_SEC ) ;
      }
      _cleanupJobID = PMD_INVALID_EDUID ;

      PD_TRACE_EXIT ( SDB__CLSSPLSS__ONDTH );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSPLSS__ONLOBFILTER, "_clsSplitSrcSession::_onLobFilter" )
   INT32 _clsSplitSrcSession::_onLobFilter( const bson::OID &oid,
                                            UINT32 sequence,
                                            BOOLEAN &need2Send )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSSPLSS__ONLOBFILTER ) ;
      INT32 range = clsPartition( oid, sequence, _partitionBit ) ;
      if ( !_hashShard || !_rangeKeyObj.firstElement().isNumber() )
      {
         need2Send = FALSE ;
         goto done ;
      }

      need2Send = _rangeKeyObj.firstElement().Int() <= range &&
           ( !_hasEndRange || range < _rangeEndKeyObj.firstElement().Int() ) ;
   done:
      PD_TRACE_EXITRC( SDB__CLSSPLSS__ONLOBFILTER, rc ) ;
      return rc ;
   }

}
