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
#include "ossMemPool.hpp"

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
      utilCSUniqueID csUniqueID ;

      _clsCSInfoTuple( INT32 ps, INT32 lps, DMS_STORAGE_TYPE sType,
                       utilCSUniqueID csid )
      :pageSize( ps ),
       lobPageSize( lps ),
       type( sType ),
       csUniqueID( csid )
      {
      }

      _clsCSInfoTuple()
      :pageSize( 0 ),
       lobPageSize( 0 ),
       type( DMS_STORAGE_NORMAL ),
       csUniqueID( UTIL_UNIQUEID_NULL )
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

      struct _clMetaData
      {
         string csName ;
         string clName ;
         DMS_STORAGE_TYPE csType ;
         utilCLUniqueID clUniqueID ;
         UINT32 pageSize ;
         UINT32 attributes ;
         INT32  lobPageSize ;
         UTIL_COMPRESSOR_TYPE compType ;
         BSONObj extOptions ;
         const CHAR *dictionary ;
         UINT32 dictSize ;

         _clMetaData()
         {
            csType = DMS_STORAGE_NORMAL ;
            clUniqueID = UTIL_UNIQUEID_NULL ;
            pageSize = DMS_INVALID_PAGESIZE ;
            attributes = 0 ;
            lobPageSize = DMS_INVALID_PAGESIZE ;
            compType = UTIL_COMPRESSOR_INVALID ;
            dictionary = NULL ;
            dictSize = 0 ;
         }
      } ;

      public:
         _clsDataDstBaseSession ( UINT64 sessionID, _netRouteAgent *agent ) ;
         virtual ~_clsDataDstBaseSession () ;

      public:
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;
         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;

      protected:
         virtual INT32 _onMetaDone( const _clMetaData &meta ) ;

      protected:
         virtual void   _begin () = 0 ;
         virtual void   _end () = 0 ;

         virtual BSONObj _keyObjB () = 0 ;
         virtual BSONObj _keyObjE () = 0 ;
         virtual INT32   _needData () const = 0 ;
         virtual CLS_FS_TYPE _dataSessionType () const = 0 ;
         virtual BOOLEAN _isReady () = 0 ;
         /*
            return FALSE, will not continue to run after
            return TRUE, will run after code
         */
         virtual BOOLEAN _onNotify ( MsgClsFSNotifyRes *pMsg ) = 0 ;

      //message function
      protected:
         INT32 handleMetaRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleIndexRes( NET_HANDLE handle, MsgHeader* header ) ;
         INT32 handleNotifyRes( NET_HANDLE handle, MsgHeader* header ) ;

      protected:
         void           _disconnect() ;
         INT32          _sendTo( const MsgRouteID &id,
                                 MsgHeader *pMsg,
                                 void *pBody = NULL,
                                 UINT32 bodyLen = 0 ) ;

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

         INT32         _extractMeta( const CHAR *objdata, _clMetaData &meta ) ;

         INT32          _extractIndex( const CHAR *objdata,
                                       vector<BSONObj> &index,
                                       BOOLEAN &noMore ) ;
         // find collection in waiting list
         BOOLEAN        _findCollection( const CHAR *collection ) ;
         // find collection space in waiting list
         BOOLEAN        _findCollectionSpace( const CHAR *collectionSpace ) ;
         UINT32         _addCollection ( const CHAR *pCollectionName ) ;
         UINT32         _removeCollection ( const CHAR *pCollectionName ) ;
         vector<string> _removeCS ( const CHAR *pCSName ) ;
         INT32          _removeValidCLs( const vector<string> &validCLs,
                                         UINT32 *pHasRemoved = NULL ) ;

      private:
         INT32 _replayDoc( const MsgClsFSNotifyRes *msg ) ;
         INT32 _replayLog( const MsgClsFSNotifyRes *msg ) ;
         INT32 _replayLob( const MsgClsFSNotifyRes *msg ) ;

         INT32          _removeValidCLsFast( const vector<string> &validCLs,
                                             UINT32 *pHasRemoved = NULL ) ;

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
         /// when we begin to get lob, we do not want to sync doc any more.
         BOOLEAN              _needMoreDoc ;

      private:
         MsgRouteID           _connID ;
         NET_HANDLE           _connHandle ;

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
   typedef ossPoolMap<string, clsCSInfoTuple>   CS_INFO_TUPLES ;

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
      virtual INT32     _onMetaDone( const _clMetaData &meta ) ;

   protected:
      void              _pullTransLog ( DPS_LSN &begin ) ;
      INT32             _extractBeginRspBody( const BSONObj &bodyObj,
                                              INT32 &nomore, INT32 &slice ) ;
      INT32             _buildBegingBody( INT32 slice,
                                          BSONObj &bodyObj,
                                          MON_CS_SIM_LIST &csList,
                                          INT32 &nomore ) ;
   protected:
      virtual void      _begin() ;
      virtual void      _end() ;
      virtual INT32     _needData () const ;
      virtual BSONObj   _keyObjB () ;
      virtual BSONObj   _keyObjE () ;
      virtual CLS_FS_TYPE _dataSessionType () const ;
      virtual BOOLEAN   _isReady () ;
      virtual BOOLEAN   _onNotify ( MsgClsFSNotifyRes *pMsg ) ;

   private:
      CLS_FULLSYNC_STEP    _fsStep ;
      CS_INFO_TUPLES       _mapEmptyCS ;
      vector<string>       _validCLs ;
      UINT32               _repeatCount ;
      BOOLEAN              _hasRegFullsyc ;

      INT32                _beginSlice ;
      INT32                _beginRspSlice ;

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
                              // sync the last log from peer node
         STEP_POST_SYNC ,     // after sync the last log from peer node, and
                              // need to notify meta change to catalog
         STEP_META ,          // when cleanup notify to catalog and catalog
                              // split the catalog and response, begin to
                              // update catalog in local, and check it
         STEP_END_NTY ,       // notify the peer node to update catalog and
                              // check it
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

      //message fuction
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
         virtual CLS_FS_TYPE _dataSessionType () const ;
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

