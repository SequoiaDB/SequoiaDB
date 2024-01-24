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

   Source File Name = clsReplSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_REPL_SESSION_HPP_
#define CLS_REPL_SESSION_HPP_

#include "pmdAsyncSession.hpp"
#include "dpsMessageBlock.hpp"
#include "clsReplayer.hpp"
#include "dpsLogRecord.hpp"
#include "msgReplicator.hpp"
#include "clsSrcSelector.hpp"

namespace engine
{
   class _dpsLogWrapper ;
   class _clsSyncManager ;
   class _netRouteAgent ;
   class _clsReplicateSet ;
   class _clsBucket ;

   enum CLS_SESSION_STATUS
   {
      CLS_SESSION_STATUS_SYNC = 1,
      CLS_SESSION_STATUS_CONSULT = 2,
      CLS_SESSION_STATUS_FULL_SYNC = 3
   } ;

   /*
      _clsReplDstSession define
   */
   class _clsReplDstSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      public:
         _clsReplDstSession ( UINT64 sessionID ) ;
         virtual ~_clsReplDstSession () ;

         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "Sync-Dest" ; }

         virtual EDU_TYPES eduType () const ;
         virtual BOOLEAN canAttachMeta() const { return FALSE ; }

         // called by net io thread
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         // called by self thread
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;
         virtual void   _onAttach () ;
         virtual void   _onDetach () ;

         virtual void onDispatchMsgBegin( const NET_HANDLE netHandle, const MsgHeader *pHeader ) ;
         virtual void onDispatchMsgEnd( INT64 costUsecs ) ;

      public:
         INT32 handleSyncRes( NET_HANDLE handle, MsgHeader* header ) ;

         INT32 handleNotify( NET_HANDLE handle, MsgHeader* header ) ;

         INT32 handleConsultRes( NET_HANDLE handle, MsgHeader *header ) ;

      private:
         void _updateName() ;

      private:

         INT32 _replayLog( const CHAR *logs, const UINT32 &len, UINT32 &num ) ;

         INT32 _replay( dpsLogRecordHeader *header ) ;

         void _sendSyncReq( DPS_LSN *pCompleteLSN = NULL ) ;

         void _sendConsultReq() ;

         INT32 _rollback( const CHAR *log ) ;

         void  _fullSync() ;

      private:
         _dpsMessageBlock              _mb ;
         clsSrcSelector                _selector ;
         _dpsLogWrapper                *_logger ;
         _clsSyncManager               *_sync ;
         _clsReplicateSet              *_repl ;
         _clsBucket                    *_pReplBucket ;
         _clsReplayer                  _replayer ;
         MsgRouteID                    _syncSrc ;
         MsgRouteID                    _lastSyncNode ;
         CLS_SESSION_STATUS            _status ;
         BOOLEAN                       _quit ;
         ossAtomic32                   _addFSSession ;
         BOOLEAN                       _isFirstToSync ;
         UINT32                        _timeout ;
         UINT64                        _requestID ;
         UINT32                        _syncFailedNum ;

         DPS_LSN                       _completeLSN ;
         DPS_LSN                       _consultLsn ;
         DPS_LSN                       _lastRecvConsultLsn ;

         UINT32                        _fullSyncIgnoreTimes ;
         CHAR                          _lastSyncDetail[ CLS_SYNC_DETAIL_MAX_LEN + 1 ] ;
   } ;

   typedef _clsReplDstSession clsReplDstSession ;

   /*
      _clsReplSrcSession define
   */
   class _clsReplSrcSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      public:
         _clsReplSrcSession ( UINT64 sessionID ) ;
         virtual ~_clsReplSrcSession () ;

         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "Sync-Source" ; }

         virtual EDU_TYPES eduType () const ;
         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         // called by net io thread
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         // called by self thread
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;

         virtual void onDispatchMsgBegin( const NET_HANDLE netHandle, const MsgHeader *pHeader ) ;
         virtual void onDispatchMsgEnd( INT64 costUsecs ) ;

      public:
         INT32 handleSyncReq( NET_HANDLE handle, MsgHeader* header ) ;

         INT32 handleVirSyncReq( NET_HANDLE handle, MsgHeader* header ) ;

         INT32 handleConsultReq( NET_HANDLE handle, MsgHeader *header ) ;

      private:
         INT32 _syncLog( const NET_HANDLE &handle,
                         const MsgReplSyncReq *req ) ;

      private:
         virtual void _makeName() ;

      private:
         _dpsMessageBlock              _mb ;
         _dpsLogWrapper                *_logger ;
         _clsSyncManager               *_sync ;
         _clsReplicateSet              *_repl ;
         BOOLEAN                       _quit ;
         UINT32                        _timeout ;
         UINT64                        _lastProcRequestID ;

         UINT64                        _dbTick ;
         CHAR                          _lastSyncDetail[ CLS_SYNC_DETAIL_MAX_LEN + 1 ] ;
   } ;
   typedef _clsReplSrcSession clsReplSrcSession ;

}

#endif //CLS_REPL_SESSION_HPP_

