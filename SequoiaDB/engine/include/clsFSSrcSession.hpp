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

   Source File Name = clsFSSrcSession.hpp

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
#ifndef CLSFSSRCSESSION_HPP_
#define CLSFSSRCSESSION_HPP_

#include "clsDef.hpp"
#include "clsFSDef.hpp"
#include "pmdAsyncSession.hpp"
#include "dms.hpp"
#include "dpsLogDef.hpp"
#include "dpsMessageBlock.hpp"
#include "rtnLobFetcher.hpp"
#include "rtnContextData.hpp"
#include "rtnRecover.hpp"
#include "../bson/bsonobj.h"
#include <map>
#include "clsUtil.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   class _netRouteAgent ;
   class _SDB_DMSCB ;
   class _dmsStorageUnit ;
   class _dpsLogWrapper ;
   class _SDB_RTNCB ;
   class _MsgClsFSNotify ;
   class _clsReplicateSet ;
   class _monIndex ;
   class _clsCatalogAgent ;
   class _clsFreezingWindow ;
   class _clsSplitTask ;

   /*
      _clsDataSrcBaseSession define
   */
   class _clsDataSrcBaseSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsDataSrcBaseSession ( UINT64 sessionID,
                                  _netRouteAgent *agent ) ;
         virtual ~_clsDataSrcBaseSession () ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         // called by net io thread
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         // called by self thread
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;

         virtual void onDispatchMsgBegin( const NET_HANDLE netHandle, const MsgHeader *pHeader ) ;
         virtual void onDispatchMsgEnd( INT64 costUsecs ) ;

      public:
         virtual INT32 notifyLSN ( UINT32 suLID, UINT32 clLID,
                                   dmsExtentID extID, dmsOffset extOffset,
                                   const OID &lobOid, UINT32 lobSequence,
                                   const DPS_LSN_OFFSET &offset ) = 0 ;

      protected:
         virtual INT32       _isReady () = 0 ;
         virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                            INT32 &outSize ) = 0 ;
         virtual INT32     _onFSMeta ( const CHAR *clFullName ) = 0 ;
         virtual INT32     _onNotifyOver( const CHAR *clFullName ) = 0 ;
         virtual INT32     _scanType () const = 0 ;
         virtual BOOLEAN   _canSwitchWhenSyncLog() = 0 ;

         virtual INT32     _onLobFilter( const bson::OID &oid,
                                         UINT32 sequence,
                                         BOOLEAN &need2Send ) = 0 ;

      protected:
         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      //message function
      protected:
         INT32 handleFSMeta( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleFSIndex( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleFSNotify( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         void              _resetInfo ( BOOLEAN all = TRUE ) ;
         void              _resend( const NET_HANDLE &handle,
                                    const _MsgClsFSNotify *req ) ;
         void              _eraseDefaultIndex( BSONObj& idIdxDef ) ;
         BOOLEAN           _existIndex( const CHAR *indexName ) ;
         INT32             _openContext( CHAR *cs, CHAR *collection ) ;
         void              _constructIndex( BSONObj &obj ) ;
         void              _constructMeta( BSONObj &obj, const CHAR *cs,
                                           const CHAR *collection,
                                           utilCLUniqueID clUniqueID,
                                           const BSONObj idIdxDef,
                                           _dmsStorageUnit *su ) ;
         INT32             _getCSName( const BSONObj &obj, CHAR *cs, UINT32 len ) ;
         INT32             _getCollection( const BSONObj &obj, CHAR *collection,
                                           UINT32 len ) ;
         INT32             _getRangeKey( const BSONObj &obj ) ;
         void              _disconnect() ;

         INT32             _syncLog( const NET_HANDLE &handle,
                                     SINT64 packet,
                                     const MsgRouteID &routeID,
                                     UINT32 TID, UINT64 requestID ) ;
         INT32             _syncRecord( const NET_HANDLE &handle,
                                        SINT64 packet,
                                        const MsgRouteID &routeID,
                                        UINT32 TID, UINT64 requestID ) ;

         INT32             _syncLob( const NET_HANDLE &handle,
                                     SINT64 packet,
                                     const MsgRouteID &routeID,
                                     UINT32 TID, UINT64 requestID ) ;

         INT32             _buildCLCommitInfo( const string &fullName,
                                               BSONObj &obj ) ;

         void              _updateNtyLSN( DPS_LSN_OFFSET collectoinLSN ) ;

         void              _printLastSyncDetail( INT32 opCode ) ;
         void              _printLastSyncDetail( INT32 opCode, CLS_FS_NOTIFY_TYPE type ) ;

      protected:
         BSONObj                          _rangeKeyObj ;
         BSONObj                          _rangeEndKeyObj ;
         MON_IDX_LIST                     _indexs ;
         _netRouteAgent                   *_agent ;
         DPS_LSN                          _lsn ;
         DPS_LSN_OFFSET                   _beginLSNOffset ;
         SINT64                           _contextID ;
         rtnContextData::sharePtr         _context ;
         INT64                            _lobContextID ;
         BOOLEAN                          _findEnd ;
         const CHAR                       *_query ;
         SINT32                           _queryLen ;
         _dpsMessageBlock                 _mb ;
         SINT64                           _packetID ;
         INT32                            _dataType ;
         BOOLEAN                          _canResend ;
         BOOLEAN                          _quit ;
         BOOLEAN                          _hasMeta ;
         INT32                            _needData ;
         BOOLEAN                          _init ;
         std::string                      _curCollecitonName ;

         _clsReplicateSet                 *_pRepl ;
         MsgHeader                        _disconnectMsg ;
         UINT32                           _timeCounter ;

         map<UINT64, UINT32>              _mapOveredCLs ;
         UINT64                           _curCollection ; // suLID+clLID
         UINT32                           _curCSLID ;
         UINT16                           _curMBID ;
         dmsRecordID                      _curRID ;
         BSONObj                          _curScanKeyObj ;
         deque<DPS_LSN_OFFSET>            _deqLSN ;
         ossSpinXLatch                    _LSNlatch ;
         rtnLobFetcher                    _lobFetcher ;
         DPS_LSN_OFFSET                   _lastEndNtyOffset ;
         DPS_LSN_OFFSET                   _clLSNOffset ;
         std::pair< OID, UINT32 >         _curLobFetched = { OID(), 0 } ;

         UINT64                           _syncBeginTick ;
         UINT64                           _totalDataSync ;
         UINT64                           _totalTimeSpent ;
         MsgRouteID                       _lastSyncNode ;
         CHAR                             _lastSyncDetail[ CLS_SYNC_DETAIL_MAX_LEN + 1 ] ;
   };

   /*
      _clsFSSrcSession define
   */
   class _clsFSSrcSession : public _clsDataSrcBaseSession
   {
   DECLARE_OBJ_MSG_MAP()
   public:
      _clsFSSrcSession( UINT64 sessionID,
                        _netRouteAgent *agent ) ;
      virtual ~_clsFSSrcSession() ;

   public:
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR*      className() const { return "FullSync-Source" ; }
      virtual EDU_TYPES eduType () const ;

   public:
      virtual INT32 notifyLSN ( UINT32 suLID, UINT32 clLID,
                                dmsExtentID extID, dmsOffset extOffset,
                                const OID &lobOid, UINT32 lobSequence,
                                const DPS_LSN_OFFSET &offset ) ;

   //message function
   protected:
      INT32 handleBegin( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleEnd( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleSyncTransReq( NET_HANDLE handle, MsgHeader* header ) ;

   protected:
      virtual INT32       _isReady() ;
      virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                         INT32 &outSize ) ;
      virtual INT32 _onLobFilter( const bson::OID &oid,
                                  UINT32 sequence,
                                  BOOLEAN &need2Send ) ;
      virtual INT32   _onFSMeta ( const CHAR *clFullName ) ;
      virtual INT32   _onNotifyOver( const CHAR *clFullName ) ;
      virtual INT32   _scanType () const ;
      virtual BOOLEAN _canSwitchWhenSyncLog() ;

   protected:
      INT32 _extractBeginBody( const BSONObj &obj,
                               MAP_SU_STATUS &validCLs,
                               INT32 &nomore,
                               INT32 &slice ) ;

      INT32 _processValidCLs( MAP_SU_STATUS &validCLs ) ;

      INT32 _constructBeginRspData( INT32 slice,
                                    BSONObj &obj,
                                    MON_CS_LIST &csList,
                                    MON_CL_LIST &clList,
                                    MAP_SU_STATUS &validCLs,
                                    INT32 &nomore ) ;

      BOOLEAN _hasExternalData() const ;

   private:
      virtual void _makeName () ;


   private:
      _dpsMessageBlock           _lsnSearchMB ;
      INT32                      _lastRecvSlice ;
      MAP_SU_STATUS              _validCLs ;
      UINT64                     _beginTick ;
   } ;
   typedef class _clsFSSrcSession clsFSSrcSession ;

   /*
      _clsSplitSrcSession define
   */
   class _clsSplitSrcSession : public _clsDataSrcBaseSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsSplitSrcSession( UINT64 sessionID, _netRouteAgent *agent ) ;
         virtual ~_clsSplitSrcSession () ;

         INT32 notifyLSN ( UINT32 suLID, UINT32 clLID,
                           dmsExtentID extID, dmsOffset extOffset,
                           const OID &lobOid, UINT32 lobSequence,
                           const DPS_LSN_OFFSET &offset ) ;

      public:
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "Split-Source" ; }
         virtual EDU_TYPES eduType () const ;

      protected:
         virtual INT32       _isReady() ;
         virtual const CHAR* _onObjFilter ( const CHAR* inBuff, INT32 inSize,
                                            INT32 &outSize ) ;
         virtual INT32   _onFSMeta ( const CHAR *clFullName ) ;
         virtual INT32   _onNotifyOver( const CHAR *clFullName ) ;
         virtual INT32   _scanType () const ;
         virtual BOOLEAN _canSwitchWhenSyncLog() ;
         virtual INT32   _onLobFilter( const bson::OID &oid,
                                       UINT32 sequence,
                                       BOOLEAN &need2Send ) ;

      protected:
         INT32   _genKeyObj ( const BSONObj &obj, BSONObj &keyObj ) ;
         BOOLEAN _containMultiKey ( const BSONObj &obj ) ;
         BOOLEAN _GEThanRangeKey ( const BSONObj &keyObj ) ;
         BOOLEAN _LThanRangeEndKey( const BSONObj &keyObj ) ;
         BOOLEAN _LEThanScanObj ( const BSONObj &keyObj ) ;
         INT32   _addToFilterMB ( const CHAR *data, UINT32 size ) ;

         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

      //message function
      protected:
         INT32 handleBegin( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleEnd( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleLEnd ( NET_HANDLE handle, MsgHeader* header ) ;

      private:
      void     _updateName() ;


      protected:
         _dpsMessageBlock                 _filterMB ;
         _dpsMessageBlock                 _lsnSearchMB ;
         BSONObj                          _shardingKey ;
         _clsCatalogAgent                 *_pCatAgent ;
         _clsFreezingWindow               *_pFreezingWindow ;
         EDUID                            _cleanupJobID ;

         BOOLEAN                          _hasShardingIndex ;
         BOOLEAN                          _hashShard ;
         BOOLEAN                          _hasEndRange ;
         UINT32                           _partitionBit ;

         UINT32                           _locationID ;
         UINT64                           _ntyOverTime ;
         BOOLEAN                          _getMetaNtyOffset ;
         UINT32                           _collectionW ;
         UINT64                           _lastOprLSN ;
         UINT32                           _internalV ;
   };
}

#endif

