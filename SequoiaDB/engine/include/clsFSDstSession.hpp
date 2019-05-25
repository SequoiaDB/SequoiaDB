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

   Source File Name = clsFSDstSession.hpp

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

#ifndef CLSFSDSTSESSION_HPP_
#define CLSFSDSTSESSION_HPP_

#include "clsDef.hpp"
#include "clsFSDef.hpp"
#include "msgReplicator.hpp"
#include "pmdAsyncSession.hpp"
#include "clsReplayer.hpp"
#include "clsSrcSelector.hpp"
#include <vector>
#include "../bson/bsonobj.h"
#include "clsTask.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   class _clsSyncManager ;
   class _clsShardMgr ;

   /*
      _clsCSInfoTuple define
   */
   struct _clsCSInfoTuple
   {
      INT32 pageSize ;
      INT32 lobPageSize ;
      DMS_STORAGE_TYPE type ;

      _clsCSInfoTuple( INT32 ps, INT32 lps, DMS_STORAGE_TYPE sType )
      :pageSize( ps ),
       lobPageSize( lps ),
       type( sType )
      {
      }

      _clsCSInfoTuple()
      :pageSize( 0 ),
       lobPageSize( 0 ),
       type( DMS_STORAGE_NORMAL )
      {
      }
   } ;
   typedef _clsCSInfoTuple clsCSInfoTuple ;

   /*
      _clsDataDstBaseSession define
   */
   class _clsDataDstBaseSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      public:
         _clsDataDstBaseSession ( UINT64 sessionID, _netRouteAgent *agent ) ;
         virtual ~_clsDataDstBaseSession () ;

      public:
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;
         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;

      protected:
         virtual void   _begin () = 0 ;
         virtual void   _end () = 0 ;

         virtual BSONObj _keyObjB () = 0 ;
         virtual BSONObj _keyObjE () = 0 ;
         virtual INT32   _needData () const = 0 ;
         virtual INT32   _dataSessionType () const = 0 ;
         virtual BOOLEAN _isReady () = 0 ;
         /*
            return FALSE, will not continue to run after
            return TRUE, will run after code
         */
         virtual BOOLEAN _onNotify ( MsgClsFSNotifyRes *pMsg ) = 0 ;

      protected:
         INT32 handleMetaRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleIndexRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleNotifyRes( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         void           _disconnect() ;
         void           _meta () ;
         void           _index() ;
         void           _notify( CLS_FS_NOTIFY_TYPE type ) ;
         BOOLEAN        _more( const MsgClsFSNotifyRes *msg,
                               const CHAR *&itr,
                               BOOLEAN isData = TRUE ) ;

         BOOLEAN        _more( const MsgClsFSNotifyRes *msg,
                               const CHAR *&itr,
                               const bson::OID *&oid,
                               const MsgLobTuple *&tuple,
                               const CHAR *&data ) ;

         INT32          _extractMeta( const CHAR *objdata,
                                      string &cs,
                                      string &collection,
                                      UINT32 &pageSize,
                                      UINT32 &attributes,
                                      INT32 &lobPageSize,
                                      DMS_STORAGE_TYPE &csType,
                                      UTIL_COMPRESSOR_TYPE &compType,
                                      BSONObj &extOptions ) ;

         INT32          _extractIndex( const CHAR *objdata,
                                       vector<BSONObj> &index,
                                       BOOLEAN &noMore ) ;

         UINT32         _addCollection ( const CHAR *pCollectionName ) ;
         UINT32         _removeCollection ( const CHAR *pCollectionName ) ;
         UINT32         _removeCS ( const CHAR *pCSName ) ;
         UINT32         _removeValidCLs( const vector<string> &validCLs ) ;

      private:
         INT32 _replayDoc( const MsgClsFSNotifyRes *msg ) ;
         INT32 _replayLog( const MsgClsFSNotifyRes *msg ) ;
         INT32 _replayLob( const MsgClsFSNotifyRes *msg ) ;

      protected:
         vector<string>       _fullNames ;
         _clsReplayer         _replayer ;
         clsSrcSelector       _selector ;
         _netRouteAgent       *_agent ;
         SINT64               _packet ;
         CLS_FS_STATUS        _status ;
         UINT32               _current ;
         UINT32               _timeout ;
         UINT32               _recvTimeout ;
         BOOLEAN              _quit ;
         UINT64               _requestID ;
         DPS_LSN              _expectLSN ;
         UINT64               _lastOprLSN ;
         BOOLEAN              _needMoreDoc ;

   };

   /*
      CLS_FULLSYNC_STEP define
   */
   enum CLS_FULLSYNC_STEP
   {
      CLS_FS_STEP_NONE = 0,
      CLS_FS_STEP_LOGBEGIN,
      CLS_FS_STEP_END
   } ;

   /*
      _clsFSDstSession define
   */
   class _clsFSDstSession : public _clsDataDstBaseSession
   {
   typedef std::map<string, clsCSInfoTuple>     CS_INFO_TUPLES ;

   DECLARE_OBJ_MSG_MAP()

   public:
      _clsFSDstSession( UINT64 sessionID,
                        _netRouteAgent *agent ) ;
      virtual ~_clsFSDstSession() ;

   public:
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR*      className() const { return "FullSync-Dest" ; }
      virtual EDU_TYPES eduType () const ;
      virtual BOOLEAN canAttachMeta() const ;

   public:
      INT32 handleBeginRes( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleEndRes( NET_HANDLE handle, MsgHeader* header ) ;
      INT32 handleSyncTransRes( NET_HANDLE handle, MsgHeader* header ) ;

   protected:
      virtual void      _onAttach () ;
      virtual void      _onDetach () ;
   protected:
      void              _pullTransLog ( DPS_LSN &begin ) ;
      INT32             _extractBeginRspBody( const BSONObj &bodyObj ) ;
      INT32             _buildBegingBody( BSONObj &bodyObj ) ;

   protected:
      virtual void      _begin() ;
      virtual void      _end() ;
      virtual INT32     _needData () const ;
      virtual BSONObj   _keyObjB () ;
      virtual BSONObj   _keyObjE () ;
      virtual INT32     _dataSessionType () const ;
      virtual BOOLEAN   _isReady () ;
      virtual BOOLEAN   _onNotify ( MsgClsFSNotifyRes *pMsg ) ;

   private:
      CLS_FULLSYNC_STEP    _fsStep ;
      CS_INFO_TUPLES       _mapEmptyCS ;
      vector<string>       _validCLs ;
      UINT32               _repeatCount ;
      BOOLEAN              _hasRegFullsyc ;

   } ;

   /*
      _clsSplitDstSession define
   */
   class _clsSplitDstSession : public _clsDataDstBaseSession
   {
      DECLARE_OBJ_MSG_MAP ()

      public:
         _clsSplitDstSession ( UINT64 sessionID, _netRouteAgent *agent,
                               void *data ) ;
         ~_clsSplitDstSession () ;

      enum SESSION_STEP
      {
         STEP_NONE      = 0,
         STEP_SYNC_DATA ,     // after sync data from peer node, and need to
         STEP_POST_SYNC ,     // after sync the last log from peer node, and
         STEP_META ,          // when cleanup notify to catalog and catalog
         STEP_END_NTY ,       // notify the peer node to update catalog and
         STEP_FINISH,         // notify catalog get all data, will to clean
         STEP_CLEANUP ,       // notify the peer node to clean up data
         STEP_REMOVE,         // remove notify to catalog
         STEP_END
      };

      public:
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "Split-Dest" ; }
         virtual EDU_TYPES eduType () const ;
         virtual BOOLEAN canAttachMeta() const ;

      protected:
         INT32 handleTaskNotifyRes ( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleBeginRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleEndRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleLEndRes ( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         virtual void      _begin () ;
         virtual void      _end ()  ;
         virtual INT32     _needData () const ;
         virtual BSONObj   _keyObjB () ;
         virtual BSONObj   _keyObjE () ;
         virtual void      _onAttach () ;
         virtual void      _onDetach () ;
         virtual INT32     _dataSessionType () const ;
         virtual BOOLEAN   _isReady () ;
         virtual BOOLEAN   _onNotify ( MsgClsFSNotifyRes *pMsg ) ;

      private:
         void              _taskNotify ( INT32 msgType ) ;
         void              _lend () ;
         void              _doneSplit() ;

      protected:
         _clsSplitTask           *_pTask ;
         BSONObj                 _taskObj ;
         _clsShardMgr            *_pShardMgr ;
         INT32                   _step ;
         INT32                   _needSyncData ;
         BOOLEAN                 _regTask ;
         UINT32                  _collectionW ;

   };

}

#endif

