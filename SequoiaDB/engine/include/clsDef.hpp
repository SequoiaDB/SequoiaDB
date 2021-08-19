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

   Source File Name = clsDef.hpp

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

#ifndef CLSDEF_HPP_
#define CLSDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogDef.hpp"
#include "netDef.hpp"
#include "msgDef.h"
#include "ossMem.hpp"
#include "msg.h"
#include "pmdEDU.hpp"
#include "ossRWMutex.hpp"
#include "dms.hpp"

#include <map>

using namespace std ;

namespace engine
{
   typedef UINT32 CLS_GROUP_VERSION ;

   const UINT32 CLS_VOTE_CS_TIME = 3000 ;
   const UINT32 CLS_SHARING_BETA_INTERVAL = 2000 ;
   const UINT32 CLS_SYNC_MAX_LEN = 1024 * 1024 * 5 ;
   extern INT32  g_startShiftTime ;

   #define CLS_TID(sessionid)          ((UINT32)(sessionid & 0xFFFFFFFF))
   #define CLS_NODEID(sessionid)       ((UINT32)(sessionid >> 32))

   #define CLS_BEATID_BEGIN                     ( 1 )
   #define CLS_BEATID_INVALID                   ( 0 )

   //full sync node timeout
   #define CLS_FS_NORES_TIMEOUT                 (10000)  // 10 secs
   #define CLS_DST_SESSION_NO_MSG_TIME          (300000) // 5 mins
   #define CLS_SRC_SESSION_NO_MSG_TIME          (10000)  // 10 secs

   #define CLS_FS_MAX_BSON_SIZE                 ( 14 * 1024 * 1024 )

   enum CLS_SYNC_STATUS
   {
      CLS_SYNC_STATUS_NONE = 0,
      CLS_SYNC_STATUS_PEER = 1,
      CLS_SYNC_STATUS_RC = 2,
   } ;

   enum CLS_BEAT_STATUS
   {
      CLS_BEAT_STATUS_BREAK = 0,
      CLS_BEAT_STATUS_ALIVE = 1,
   } ;

   const UINT32 CLS_BEAT_ID_OVERTURN_WINDOW = 2 ^ 31 ;

   enum CLS_GROUP_ROLE
   {
      CLS_GROUP_ROLE_SECONDARY = 0,
      CLS_GROUP_ROLE_PRIMARY =1,
   } ;

   enum CLS_ELECTION_STATUS
   {
      CLS_ELECTION_STATUS_SILENCE = 0,
      CLS_ELECTION_STATUS_SEC = 1,
      CLS_ELECTION_STATUS_VOTE = 2,
      CLS_ELECTION_STATUS_ANNOUNCE = 3,
      CLS_ELECTION_STATUS_PRIMARY = 4,
   } ;

   enum CLS_NODE_SERVICE_STATUS
   {
      SERVICE_NORMAL          = 0,     /// node is normal, and data is normal
      SERVICE_ABNORMAL,                /// node is normal, but data is abnormal
      SERVICE_UNKNOWN                  /// node is abnormal(crashed)
   } ;

   enum CLS_NODE_RUN_STAT
   {
      CLS_NODE_RUNNING  = 0,
      CLS_NODE_CATCHUP  = 1,
      CLS_NODE_STOP     = 2
   } ;

   // after 5 times without receiving message, mark the UDP unavailable
   #define CLS_UDP_UNAVAILABLE  ( 5 )

   #define CLS_BEAT_VERSION_1       ( 1 )

   /*
      _clsGroupBeat define
   */
   class _clsGroupBeat : public SDBObject
   {
   public :
      DPS_LSN                 endLsn ;
      _MsgRouteID             identity ;
      UINT8                   weight ;
      UINT8                   beatVersion ;
      UINT8                   nodeRunStat ;
      CHAR                    pad[1] ;
      CHAR                    hashCode[4] ;
      CLS_GROUP_VERSION       version ;
      CLS_GROUP_ROLE          role ;         // self role
      CLS_SYNC_STATUS         syncStatus ;
      UINT32                  beatID ;
      CLS_NODE_SERVICE_STATUS serviceStatus ;

      /*
         >= CLS_BEAT_VERSION_1
      */
      UINT32                  ftConfirmStat ;
      INT32                   indoubtErr ;

      _clsGroupBeat(): version( 0 ),
                       role( CLS_GROUP_ROLE_SECONDARY ),
                       syncStatus( CLS_SYNC_STATUS_NONE ),
                       beatID( CLS_BEATID_INVALID ),
                       serviceStatus( SERVICE_UNKNOWN )
      {
         weight = 0 ;
         beatVersion = CLS_BEAT_VERSION_1 ;
         nodeRunStat = (UINT8)CLS_NODE_RUNNING ;
         pad[0] = 0 ;

         ftConfirmStat = 0 ;
         indoubtErr = SDB_OK ;
      }

      UINT32 getFTConfirmStat() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_1 )
         {
            return ftConfirmStat ;
         }
         return 0 ;
      }

      INT32 getIndoubtErr() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_1 )
         {
            return indoubtErr ;
         }
         return SDB_OK ;
      }

      _clsGroupBeat& operator= ( const _clsGroupBeat &rhs )
      {
         endLsn = rhs.endLsn ;
         identity = rhs.identity ;
         weight = rhs.weight ;
         nodeRunStat = rhs.nodeRunStat ;
         version = rhs.version ;
         role = rhs.role ;
         syncStatus = rhs.syncStatus ;
         beatID = rhs.beatID ;
         serviceStatus = rhs.serviceStatus ;

         ossMemcpy( pad, rhs.pad, sizeof( pad ) ) ;
         ossMemcpy( hashCode, rhs.hashCode, sizeof( hashCode ) ) ;

         if ( rhs.beatVersion >= CLS_BEAT_VERSION_1 )
         {
            beatVersion = rhs.beatVersion ;
            ftConfirmStat = rhs.ftConfirmStat ;
            indoubtErr = rhs.indoubtErr ;
         }
         else
         {
            beatVersion = CLS_BEAT_VERSION_1 ;
            ftConfirmStat = 0 ;
            indoubtErr = SDB_OK ;
         }
         return *this ;
      }

      BOOLEAN isValidID( const UINT32 &id )
      {
         if ( CLS_BEATID_INVALID == beatID )
         {
            return TRUE ;
         }
         else if ( beatID < id )
         {
            return TRUE ;
         }
         else if ( (beatID - id) >
                    CLS_BEAT_ID_OVERTURN_WINDOW )
         {
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }
   } ;

   /*
      _clsSharingStatus define
   */
   class _clsSharingStatus : public SDBObject
   {
   public:
      _clsGroupBeat beat ;
      UINT32 timeout ;
      UINT32 breakTime ;
      UINT32 deadtime ;
      UINT32 sendFailedTimes ;

   protected:
      // need test remote status, which might not support UDP
      // 1. at the beginning, send beat both by UDP and TCP
      // 2. if received UDP message, mark remote UDP supported
      // 3. if not received UDP message after 5 test beats, mark remote
      //    UDP unavailable
      BOOLEAN _supportUDP ;
      INT32   _testUDPCount ;

   public:
      _clsSharingStatus()
      {
         timeout = 0 ;
         breakTime = 0 ;
         deadtime = 0 ;
         sendFailedTimes = 0 ;
         resetUDP() ;
      }

      OSS_INLINE BOOLEAN isUDPSupported()
      {
         return _supportUDP ;
      }

      OSS_INLINE BOOLEAN isUDPUnavailable()
      {
         return _testUDPCount > CLS_UDP_UNAVAILABLE ;
      }

      OSS_INLINE void increaseUDPTest()
      {
         ++ _testUDPCount ;
      }

      OSS_INLINE void setUDPSupported()
      {
         _supportUDP = TRUE ;
         _testUDPCount = 0 ;
      }

      OSS_INLINE void setUDPUnavailable()
      {
         _supportUDP = FALSE ;
         _testUDPCount = CLS_UDP_UNAVAILABLE + 1 ;
      }

      OSS_INLINE void resetUDP()
      {
         _supportUDP = FALSE ;
         _testUDPCount = 0 ;
      }
   } ;

   #define CLS_NODE_KEEPALIVE_TIMEOUT              ( 6000 ) // ms

   /*
      _clsGroupInfo define
   */
   class _clsGroupInfo : public SDBObject
   {
   public :
      map<UINT64, _clsSharingStatus> info ;
      map<UINT64, _clsSharingStatus *> alives ;
      ossRWMutex mtx ;
      _MsgRouteID primary ;
      _MsgRouteID local ;
      UINT32 localBeatID ;
      CLS_GROUP_VERSION version ;

   private:
      UINT32 _hashCode ;

   public:
      _clsGroupInfo():localBeatID( CLS_BEATID_BEGIN ),
                      version( 0 ), _hashCode( 0 )
      {
         local.value = 0 ;
         primary.value = 0 ;
      }
      ~_clsGroupInfo()
      {
         alives.clear() ;
         info.clear() ;
      }
      UINT32 nextBeatID()
      {
         ++localBeatID ;
         if ( localBeatID <= CLS_BEATID_BEGIN  )
         {
            localBeatID = CLS_BEATID_BEGIN + 1 ;
         }
         return localBeatID ;
      }

      void     setHashCode( UINT32 hashCode ) { _hashCode = hashCode ; }
      UINT32   getHashCode() const { return _hashCode ; }

      UINT32 groupSize ()
      {
         return info.size() + 1 ;
      }

      UINT32 aliveSize ()
      {
         return alives.size() + 1 ;
      }

      BOOLEAN isAllNodeBeat()
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( 0 == status.beat.beatID )
            {
               return FALSE ;
            }
            ++it ;
         }
         return TRUE ;
      }

      BOOLEAN isAllNodeAbnormal( UINT32 timeout )
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( SERVICE_NORMAL == status.beat.serviceStatus )
            {
               return FALSE ;
            }
            else if ( SERVICE_UNKNOWN == status.beat.serviceStatus &&
                      ( 0 == timeout || status.breakTime < timeout ) )
            {
               return FALSE ;
            }
            ++it ;
         }
         return TRUE ;
      }

      UINT32 getAlivesByTimeout( UINT32 timeout = CLS_NODE_KEEPALIVE_TIMEOUT )
      {
         UINT32 count = 1 ;
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( SERVICE_UNKNOWN == status.beat.serviceStatus &&
                 ( 0 == timeout || status.breakTime > timeout ) )
            {
               ++it ;
               continue ;
            }
            ++count ;
            ++it ;
         }
         return count ;
      }

      UINT32 getNodeSendFailedTimes( UINT64 nodeID )
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.find( nodeID ) ;
         SDB_ASSERT( it != info.end(), "Node is not exist" ) ;

         if ( it != info.end() )
         {
            return it->second.sendFailedTimes ;
         }
         return (UINT32)-1 ;
      }

   } ;

   enum CLS_ELECTION_ROUND
   {
      CLS_ELECTION_ROUND_STAGE_ONE = 0,
      CLS_ELECTION_ROUND_STAGE_TWO = 1,
   } ;

   class _clsSyncSession : public SDBObject
   {
   public :
      DPS_LSN_OFFSET endLsn ;
      _pmdEDUCB *eduCB ;

      /// local write has been completed.
      /// synced starts from one.
      _clsSyncSession():endLsn(DPS_INVALID_LSN_OFFSET), eduCB( NULL)
      {}

      BOOLEAN operator<( const _clsSyncSession &session )
      {
         return endLsn < session.endLsn ;
      }

      BOOLEAN operator<=( const _clsSyncSession &session )
      {
         return endLsn <= session.endLsn ;
      }
   } ;


   #define CLS_SAME_SYNC_LSN_MAX_TIMES    (20)

   /*
      _clsSyncStatus define
   */
   class _clsSyncStatus : public SDBObject
   {
   public :
      DPS_LSN_OFFSET    offset ;
      _MsgRouteID       id ;
      BOOLEAN           valid ;
      UINT32            sameReqTimes ;

      _clsSyncStatus():offset(0)
      {
         id.value       = 0 ;
         valid          = TRUE ;
         sameReqTimes   = 0 ;
      }

      _clsSyncStatus& operator=( const _clsSyncStatus &right )
      {
         offset         = right.offset ;
         id.value       = right.id.value ;
         valid          = right.valid ;
         sameReqTimes   = right.sameReqTimes ;

         return *this ;
      }

      BOOLEAN isValid() const
      {
         // 1. already full sync
         // 2. sharing-break
         // 3. same sync req more than 20 times
         if ( DPS_INVALID_LSN_OFFSET == offset ||
              !valid ||
              sameReqTimes > CLS_SAME_SYNC_LSN_MAX_TIMES )
         {
            return FALSE ;
         }
         return TRUE ;
      }
   } ;

   /*
      _clsLSNNtyInfo define
   */
   struct _clsLSNNtyInfo
   {
      UINT32               _csLID ;
      UINT32               _clLID ;
      dmsExtentID          _extLID ;
      DPS_LSN_OFFSET       _offset ;

      _clsLSNNtyInfo ()
      {
         _csLID = ~0 ;
         _clLID = ~0 ;
         _extLID = -1 ;
         _offset = 0 ;
      }
      _clsLSNNtyInfo( UINT32 csLID, UINT32 clLID, dmsExtentID extLID,
                      DPS_LSN_OFFSET offset )
      {
         _csLID      = csLID ;
         _clLID      = clLID ;
         _extLID     = extLID ;
         _offset     = offset ;
      }
   } ;
   typedef _clsLSNNtyInfo clsLSNNtyInfo ;

   enum CLS_REELECTION_LEVEL
   {
      CLS_REELECTION_LEVEL_NONE = 0,
      CLS_REELECTION_LEVEL_1 = 1,
      CLS_REELECTION_LEVEL_3 = 3,  /// wait for at least one replication catch up
      CLS_REELECTION_LEVEL_MAX
   } ;

#define CLS_ELECTION_WEIGHT_MIN 0
#define CLS_ELECTION_WEIGHT_USR_MIN 1
#define CLS_ELECTION_WEIGHT_USR_MAX 100
#define CLS_ELECTION_WEIGHT_MAX 101

#define CLS_GET_WEIGHT( weight, shadowWeight )\
        (( CLS_ELECTION_WEIGHT_MIN == (shadowWeight) ) ?\
          (shadowWeight) :\
         ( (weight) < (shadowWeight) ?\
           (shadowWeight) : (weight) ))
}

#endif // CLSDEF_HPP_

