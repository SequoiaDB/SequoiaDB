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

   Source File Name = msgReplicator.hpp

   Descriptive Name = Message Replicator Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains message structure for
   replications between multiple servers within a ReplicationGroup.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MSGREPLICATOR_HPP_
#define MSGREPLICATOR_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "clsDef.hpp"
#include "msg.hpp"
#pragma pack(4)

namespace engine
{

const UINT32 MSG_HOSTNAME_MAX = 256 ;
const UINT32 MSG_SERVICE_MAX = 64 ;

   class _MsgClsBeat : public SDBObject
   {
   public :
      _MsgHeader header ;
      _clsGroupBeat beat ;
      _MsgClsBeat()
      {
         header.messageLength = sizeof( _MsgClsBeat ) ;
         header.opCode = MSG_CLS_BEAT ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
      }
   } ;
   typedef class _MsgClsBeat MsgClsBeat ;

   class _MsgClsBeatRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgRouteID identity ;
      _MsgClsBeatRes()
      {
         header.header.opCode = MSG_CLS_BEAT_RES ;
         header.header.messageLength = sizeof( _MsgClsBeatRes ) ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
      }
   } ;
   typedef class _MsgClsBeatRes MsgClsBeatRes ;

   class _MsgClsElectionBallot : public SDBObject
   {
   public :
      _MsgHeader header ;
      DPS_LSN weights ;
      _MsgRouteID identity ;
      CLS_ELECTION_ROUND round ;
      _MsgClsElectionBallot()
      {
         header.opCode = MSG_CLS_BALLOT ;
         header.messageLength = sizeof( _MsgClsElectionBallot ) ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
      }
   } ;
   typedef class _MsgClsElectionBallot MsgClsElectionBallot ;

   class _MsgClsElectionRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgRouteID identity ;
      CLS_ELECTION_ROUND round ;
      _MsgClsElectionRes()
      {
         header.header.messageLength = sizeof( _MsgClsElectionRes ) ;
         header.header.opCode = MSG_CLS_BALLOT_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
      }
   } ;
   typedef class _MsgClsElectionRes MsgClsElectionRes ;

   class _MsgSyncNotify : public SDBObject
   {
   public :
      _MsgHeader header ;

      _MsgSyncNotify()
      {
         header.messageLength = sizeof( _MsgSyncNotify ) ;
         header.opCode = MSG_CLS_SYNC_NOTIFY ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
      }
   } ;
   typedef class _MsgSyncNotify MsgSyncNotify ;

   class _MsgReplSyncReq : public SDBObject
   {
   public :
      _MsgHeader           header ;
      _MsgRouteID          identity ;
      DPS_LSN              next;
      DPS_LSN              completeNext ;
      INT32                needData ;
      _MsgReplSyncReq()
      {
         header.messageLength = sizeof( _MsgReplSyncReq ) ;
         header.opCode = MSG_CLS_SYNC_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID = 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
         needData = 1 ;
      }
   } ;
   typedef class _MsgReplSyncReq MsgReplSyncReq ;

   class _MsgReplSyncRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgRouteID             identity ;
      DPS_LSN_OFFSET          oldestTransLsn;
      _MsgReplSyncRes()
      {
          /// not contains the length of data
         header.header.messageLength = sizeof( _MsgReplSyncRes ) ;
         header.header.opCode = MSG_CLS_SYNC_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
      }
   } ;
   typedef class _MsgReplSyncRes MsgReplSyncRes ;

   class _MsgReplVirSyncReq : public SDBObject
   {
   public :
      _MsgHeader        header ;
      DPS_LSN           next;
      MsgRouteID        from ;
      _MsgReplVirSyncReq()
      {
         header.messageLength = sizeof( _MsgReplVirSyncReq ) ;
         header.opCode = MSG_CLS_SYNC_VIR_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         from.value = MSG_INVALID_ROUTEID ;
      }
   } ;
   typedef class _MsgReplVirSyncReq MsgReplVirSyncReq ;

   class _MsgReplConsultation : public SDBObject
   {
   public :
      _MsgHeader        header ;
      DPS_LSN           current ;
      DPS_LSN           lastConsult ;
      MsgRouteID        identity ;
      UINT32            hashValue ;
      CHAR              reserved[ 16 ] ;
      _MsgReplConsultation()
      {
         header.messageLength = sizeof( _MsgReplConsultation ) ;
         header.opCode = MSG_CLS_CONSULTATION_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         identity.value = MSG_INVALID_ROUTEID ;
         hashValue = 0 ;
         ossMemset( reserved, 0, sizeof( reserved ) ) ;
      }
   } ;
   typedef class _MsgReplConsultation MsgReplConsultation ;

   class _MsgReplConsultationRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      DPS_LSN                 returnTo ;
      UINT32                  hashValue ;
      CHAR                    reserved[ 16 ] ;
      _MsgReplConsultationRes()
      {
         header.header.messageLength =
                   sizeof( _MsgReplConsultationRes ) ;
         header.header.opCode = MSG_CLS_CONSULTATION_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         hashValue = 0 ;
         ossMemset( reserved, 0, sizeof( reserved ) ) ;
      }
   } ;
   typedef class _MsgReplConsultationRes MsgReplConsultationRes ;


   enum CLS_FS_TYPE
   {
      CLS_FS_TYPE_IN_SET = 0,
      CLS_FS_TYPE_BETWEEN_SETS,
   } ;

   /// msg : | --- MsgClsFSBegin --- | --- [ bson ] --- |
   /// bson:
   ///   for fullsync: { validcls: [ { fullname:"xx.xx", commitflag:[x,y,z], commitlsn:[X,Y,Z] }, ... ],
   ///                   nomore:1/0, slice:n }
   ///   for split: no-bson
   class _MsgClsFSBegin : public SDBObject
   {
   public :
      _MsgHeader header ;
      CLS_FS_TYPE type ;
      SINT32 cataVersion ;
      _MsgClsFSBegin()
      {
         header.messageLength = sizeof( _MsgClsFSBegin ) ;
         header.opCode = MSG_CLS_FULL_SYNC_BEGIN ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         header.requestID = 0 ;
         type = CLS_FS_TYPE_IN_SET ;
         cataVersion = -1 ;
      }
   } ;
   typedef class _MsgClsFSBegin MsgClsFSBegin ;

   /// msg: | -- MsgClsFSBeginRes -- | -- [bson] -- |
   /// bson:
   ///   for fullsyc: { csnames: [ {csname:'xx', pagesize:x, logpagesize:y},... ],
   ///                  fullnames:[ {fullname:'yy'},... ],
   ///                  validcls:[ {fullname:'zz'}, ... ],
   ///                  nomore:1/0, slice:n
   ///                }
   ///   for split: no-bson
   class _MsgClsFSBeginRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      DPS_LSN lsn ;
      _MsgClsFSBeginRes()
      {
         header.header.messageLength = sizeof( _MsgClsFSBeginRes ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_BEGIN_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         header.header.requestID = 0 ;
         header.res = 0 ;
      }
   } ;
   typedef class _MsgClsFSBeginRes MsgClsFSBeginRes ;

   class _MsgClsFSLEnd : public SDBObject
   {
      public:
         _MsgHeader header ;

         _MsgClsFSLEnd()
         {
            header.messageLength = sizeof( _MsgClsFSLEnd ) ;
            header.opCode = MSG_CLS_FULL_SYNC_LEND_REQ ;
            header.routeID.value = MSG_INVALID_ROUTEID ;
            header.requestID = 0 ;
            header.TID = 0 ;
         }
   };
   typedef class _MsgClsFSLEnd MsgClsFSLEnd ;

   class _MsgClsFSLEndRes : public SDBObject
   {
      public:
         _MsgInternalReplyHeader header ;
         _MsgClsFSLEndRes()
         {
            header.header.messageLength = sizeof( _MsgClsFSLEndRes ) ;
            header.header.opCode = MSG_CLS_FULL_SYNC_LEND_RES ;
            header.header.routeID.value = MSG_INVALID_ROUTEID ;
            header.header.requestID = 0 ;
            header.header.TID = 0 ;
            header.res = 0 ;
         }
   };
   typedef class _MsgClsFSLEndRes MsgClsFSLEndRes ;

   class _MsgClsFSEnd : public SDBObject
   {
   public :
      _MsgHeader header ;
      _MsgClsFSEnd()
      {
         header.messageLength = sizeof( _MsgClsFSEnd ) ;
         header.opCode = MSG_CLS_FULL_SYNC_END;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         header.requestID = 0 ;
      }
   } ;
   typedef class _MsgClsFSEnd MsgClsFSEnd ;

   class _MsgClsFSEndRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgClsFSEndRes()
      {
         header.header.messageLength = sizeof( _MsgClsFSEndRes ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_END_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         header.header.requestID = 0 ;
         header.res = 0 ;
      }
   } ;
   typedef class _MsgClsFSEndRes MsgClsFSEndRes ;

   /// msg: | -- _MsgClsFSRequire -- | -- bson -- |
   /// bson: {cs:"xxx", collection:"xxx", keyobj:{"":value1,"":value2}, needdata:1/0}
   class _MsgClsFSMetaReq : public SDBObject
   {
   public :
      _MsgHeader header ;
      _MsgClsFSMetaReq()
      {
         header.messageLength = sizeof( _MsgClsFSMetaReq ) ;
         header.opCode = MSG_CLS_FULL_SYNC_META_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         header.requestID = 0 ;
      }
   } ;
   typedef class _MsgClsFSMetaReq MsgClsFSMetaReq ;

   /// msg: | -- _MsgClsFSRequireRes -- | -- bson -- |
   /// bson: {cs:"xxx",  csmeta:xxx}
   class _MsgClsFSMetaRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgClsFSMetaRes()
      {
         header.header.messageLength = sizeof( _MsgClsFSMetaRes ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_META_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         header.header.requestID = 0 ;
         header.res = 0 ;
      }
   } ;
   typedef class _MsgClsFSMetaRes MsgClsFSMetaRes ;

   class _MsgClsFSIndexReq : public SDBObject
   {
   public :
      _MsgHeader header ;
      _MsgClsFSIndexReq()
      {
         header.messageLength = sizeof( _MsgClsFSIndexReq ) ;
         header.opCode = MSG_CLS_FULL_SYNC_INDEX_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         header.requestID = 0 ;
      }
   } ;
   typedef class _MsgClsFSIndexReq MsgClsFSIndexReq ;

   /// msg: | -- MsgClsFSIndexRes -- | -- bson -- |
   /// bson: {nomore:xx, indexes:[index:xxx]}
   class _MsgClsFSIndexRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      _MsgClsFSIndexRes()
      {
         header.header.messageLength = sizeof( _MsgClsFSIndexRes ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_INDEX_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         header.header.requestID = 0 ;
         header.res = 0 ;
      }
   } ;
   typedef class _MsgClsFSIndexRes MsgClsFSIndexRes ;

   enum CLS_FS_NOTIFY_TYPE
   {
      CLS_FS_NOTIFY_TYPE_DOC = 0,
      CLS_FS_NOTIFY_TYPE_LOG,
      CLS_FS_NOTIFY_TYPE_OVER,
      CLS_FS_NOTIFY_TYPE_LOB,
   } ;

   class _MsgClsFSNotify : public SDBObject
   {
   public :
      _MsgHeader header ;
      SINT64 packet ;
      CLS_FS_NOTIFY_TYPE type ;
      _MsgClsFSNotify()
      {
         header.messageLength = sizeof( _MsgClsFSNotify ) ;
         header.opCode = MSG_CLS_FULL_SYNC_NOTIFY ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID= 0 ;
         header.requestID = 0 ;
         packet = 0 ;
      }
   } ;
   typedef class _MsgClsFSNotify MsgClsFSNotify ;

   #define CLS_FS_EOF -1
   #define CLS_FS_NOT_EOF 0

   /// msg: | -- _MsgClsFSNotify -- | -- data -- |
   /// data: if DOC: | record bson | record bson |...|
   ///       if LOG: | log | log |...|
   ///       if LOB: | oid | MsgLobTuple | data | ... | oid | MsgLobTuple | data |
   ///       if LOB and eof == CLS_FS_EOF : | bson |
   class _MsgClsFSNotifyRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      SINT64 packet;
      CLS_FS_NOTIFY_TYPE type ;
      SINT16 eof ;
      DPS_LSN lsn ;
      _MsgClsFSNotifyRes()
      {
         header.header.messageLength = sizeof( _MsgClsFSNotifyRes ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_NOTIFY_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID= 0 ;
         header.header.requestID = 0 ;
         header.res = 0 ;
         packet = 0 ;
         eof = CLS_FS_NOT_EOF ;
      }
   } ;
   typedef class _MsgClsFSNotifyRes MsgClsFSNotifyRes ;

   class MsgClsFSTransSyncReq : public SDBObject
   {
   public :
      _MsgHeader header ;
      DPS_LSN  begin;
      DPS_LSN  endExpect;
      MsgClsFSTransSyncReq()
      {
         header.messageLength = sizeof( MsgClsFSTransSyncReq ) ;
         header.opCode = MSG_CLS_FULL_SYNC_TRANS_REQ ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID = 0;
         header.requestID = 0;
      }
   };

   class MsgClsFSTransSyncRes : public SDBObject
   {
   public :
      _MsgInternalReplyHeader header ;
      SINT16 eof ;
      MsgClsFSTransSyncRes()
      {
         header.res = SDB_OK;
         header.header.messageLength = sizeof( MsgClsFSTransSyncReq ) ;
         header.header.opCode = MSG_CLS_FULL_SYNC_TRANS_RES ;
         header.header.routeID.value = MSG_INVALID_ROUTEID ;
         header.header.TID = 0;
         header.header.requestID = 0;
         eof = CLS_FS_NOT_EOF;
      }
   };

   class _MsgClsGInfoUpdated : public SDBObject
   {
   public:
      _MsgHeader header ;
      UINT32 groupID ;

      _MsgClsGInfoUpdated()
      {
         header.messageLength = sizeof( _MsgClsGInfoUpdated ) ;
         header.opCode = MSG_CLS_GINFO_UPDATED ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID = 0;
         header.requestID = 0;
         groupID = 0 ;
      }
   } ;

   typedef class _MsgClsGInfoUpdated MsgClsGInfoUpdated ;

   class _MsgClsNodeStatusNotify : public SDBObject
   {
   public:
      _MsgHeader header ;
      INT32      status ;  /// SDB_DB_STATUS

      _MsgClsNodeStatusNotify()
      {
         header.messageLength = sizeof( _MsgClsNodeStatusNotify ) ;
         header.opCode = MSG_CLS_NODE_STATUS_NOTIFY ;
         header.routeID.value = MSG_INVALID_ROUTEID ;
         header.TID = 0 ;
         header.requestID = 0 ;
         status = 0 ;
      }
   } ;
   typedef _MsgClsNodeStatusNotify MsgClsNodeStatusNotify ;

   class _MsgClsTransCheckReq : public SDBObject
   {
      public:
         _MsgHeader header ;
         UINT64     transID ;
         UINT32     reserved[8] ;

         _MsgClsTransCheckReq()
         {
            header.messageLength = sizeof( _MsgClsTransCheckReq ) ;
            header.opCode = MSG_CLS_TRANS_CHECK_REQ ;
            header.routeID.value = MSG_INVALID_ROUTEID ;
            header.TID = 0 ;
            header.requestID = 0 ;
            transID = 0 ;
            ossMemset( reserved, 0, sizeof( reserved ) ) ;
         }
   } ;
   typedef _MsgClsTransCheckReq MsgClsTransCheckReq ;

   /*
      MsgOpReply + BSON( { TransID:xxx, Status:xxx } )
   */
   typedef MsgOpReply MsgClsTransCheckRes ;

}

#pragma pack()

#endif
