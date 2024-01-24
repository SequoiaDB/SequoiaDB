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

   Source File Name = pmdDef.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Event Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for events that
   used as inter-EDU communications.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_DEF_HPP__
#define PMD_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "utilCircularQueue.hpp"
#include "ossQueue.hpp"
#include "../bson/oid.h"

namespace engine
{
   /*
      pmdEDUEventTypes define
   */
   enum pmdEDUEventTypes
   {
      PMD_EDU_EVENT_NONE = 0 ,
      PMD_EDU_EVENT_TERM ,        // terminate EDU
      PMD_EDU_EVENT_RESUME,       // resume a EDU, the data is startEDU's argv
      PMD_EDU_EVENT_ACTIVE,
      PMD_EDU_EVENT_DEACTIVE,
      PMD_EDU_EVENT_MSG,          // pmd msg event
      PMD_EDU_EVENT_TIMEOUT,      // pmd edu timeout,
      PMD_EDU_EVENT_LOCKWAKEUP,   // transaction-lock wake up
      PMD_EDU_EVENT_BP_RESUME,    // break point resume
      PMD_EDU_EVENT_TRANS_STOP,   // stop transaction
      PMD_EDU_EVENT_STEP_DOWN,    // step down
      PMD_EDU_EVENT_STEP_UP,      // step up
      PMD_EDU_EVENT_KILLCONTEXT,  // kill specified context
      PMD_EDU_EVENT_UPDATE_GRPMODE, // update group mode

      PMD_EDU_EVENT_MAX
   } ;

   /*
      pmdEDUMemTypes define
   */
   enum pmdEDUMemTypes
   {
      PMD_EDU_MEM_NONE     = 0,   // Memory is not unknown
      PMD_EDU_MEM_ALLOC    = 1,   // Memory is by SDB_OSS_MALLOC
      PMD_EDU_MEM_SELF     = 2,   // Memory is by pmdEDU::allocBuff
      PMD_EDU_MEM_THREAD   = 3    // thread alloc
   } ;

   /*
      PMD_EVENT_MESSAGES define, for different event
   */
   union PMD_EVENT_MESSAGES
   {
      // for PMD_EDU_EVENT_TIMEOUT
      struct timeoutMsg
      {
         UINT64   timerID ;
         UINT32   interval ;
         UINT64   occurTime ;
      } timeoutMsg ;
   } ;

   typedef union PMD_EVENT_MESSAGES PMD_EVENT_MESSAGES ;

   /*
      _pmdEDUEvent define
   */
   class _pmdEDUEvent : public SDBObject
   {
   public :
      pmdEDUEventTypes  _eventType ;
      pmdEDUMemTypes    _dataMemType ;
      UINT64            _userData ;
      void              *_Data ;

      _pmdEDUEvent ( pmdEDUEventTypes type = PMD_EDU_EVENT_NONE,
                     pmdEDUMemTypes dataMemType = PMD_EDU_MEM_NONE,
                     void *data = NULL,
                     UINT64 userData = 0 )
      {
         _reset ( type, dataMemType, data, userData ) ;
      }
      _pmdEDUEvent( const _pmdEDUEvent &rhs )
      {
         _reset ( rhs._eventType, rhs._dataMemType, rhs._Data, rhs._userData ) ;
      }
      _pmdEDUEvent& operator=( const _pmdEDUEvent &rhs )
      {
         _reset( rhs._eventType, rhs._dataMemType, rhs._Data, rhs._userData ) ;
         return *this ;
      }
      void reset ()
      {
         _reset () ;
      }

   protected:
      void _reset ( pmdEDUEventTypes type = PMD_EDU_EVENT_NONE,
                    pmdEDUMemTypes dataMemType = PMD_EDU_MEM_NONE,
                    void *data = NULL,
                    UINT64 userData = 0 )
      {
         _eventType  = type ;
         _dataMemType= dataMemType ;
         _Data       = data ;
         _userData   = userData ;
      }

   } ;

   typedef class _pmdEDUEvent pmdEDUEvent ;

   /*
      _pmdEDUEventQueue
    */
   #define PMD_EDU_QUEUE_CAPACITY ( 5 )
   typedef _utilCircularStackBuffer< pmdEDUEvent, PMD_EDU_QUEUE_CAPACITY >
                                             PMD_EVENT_QUEUE_BUFFER ;
   typedef _utilCircularQueue< pmdEDUEvent > PMD_EVENT_QUEUE_CONTAINER ;
   class _pmdEDUEventQueue : public ossQueue< pmdEDUEvent,
                                              PMD_EVENT_QUEUE_CONTAINER >
   {
   protected:
      typedef ossQueue< pmdEDUEvent, PMD_EVENT_QUEUE_CONTAINER > _BASE ;

   public:
      _pmdEDUEventQueue()
      : _BASE( PMD_EVENT_QUEUE_CONTAINER( &_buffer ) )
      {
      }

      ~_pmdEDUEventQueue()
      {
      }

   protected:
      PMD_EVENT_QUEUE_BUFFER _buffer ;
   } ;

   typedef class _pmdEDUEventQueue pmdEDUEventQueue ;

   /*
      _pmdDataExInfo define
   */
   struct _pmdDataExInfo
   {
      _pmdDataExInfo()
      {
         _csLID   = ~0 ;
         _clLID   = ~0 ;
         _extID  = ~0 ;
         _extOffset = ~0 ;
         _lobOid = bson::OID() ;
         _lobSequence = ~0 ;
         _isValid = FALSE ;
      }

      _pmdDataExInfo& operator= ( const _pmdDataExInfo &info )
      {
         _csLID   = info._csLID ;
         _clLID   = info._clLID ;
         _extID  = info._extID ;
         _extOffset = info._extOffset ;
         _lobOid = info._lobOid ;
         _lobSequence = info._lobSequence ;
         _isValid = info._isValid ;
         return *this ;
      }

      void clear()
      {
         _csLID  = ~0 ;
         _clLID  = ~0 ;
         _extID = ~0 ;
         _extOffset = ~0 ;
         _lobOid = bson::OID() ;
         _lobSequence = ~0 ;
         _isValid = FALSE ;
      }

      UINT32  _csLID ;
      UINT32  _clLID ;
      UINT32  _extID ;
      UINT32  _extOffset ;
      bson::OID _lobOid ;
      UINT32 _lobSequence ;
      BOOLEAN _isValid ;
   } ;

   typedef struct _pmdDataExInfo pmdDataExInfo ;


   #define PMD_INVALID_EDUID              ( 0 )

   /*
      EDU_TYPES define
   */
   enum EDU_TYPES
   {
      //System EDU Type
      EDU_TYPE_TCPLISTENER                = 0,
      EDU_TYPE_RESTLISTENER,
      EDU_TYPE_REPR,
      EDU_TYPE_LOGGW,
      EDU_TYPE_LOGARCHIVEMGR,
      EDU_TYPE_DPSROLLBACK,
      EDU_TYPE_SHARDR,
      EDU_TYPE_CLUSTER,
      EDU_TYPE_CLUSTERSHARD,
      EDU_TYPE_CLSLOGNTY,
      EDU_TYPE_CATMGR,
      EDU_TYPE_CATNETWORK,
      EDU_TYPE_COORDNETWORK,
      EDU_TYPE_COORD_DS_NETWORK,
      EDU_TYPE_COORDMGR,
      EDU_TYPE_OMMGR,
      EDU_TYPE_OMNET,
      EDU_TYPE_SYNCCLOCK,
      EDU_TYPE_PIPESLISTENER,
      EDU_TYPE_FAPLISTENER,
      EDU_TYPE_DBMONITOR,
      EDU_TYPE_RTNNETWORK,
#if defined (_LINUX)
      EDU_TYPE_SIGNALTEST,
#endif // _LINUX

      // Agent EDU Type Begin
      EDU_TYPE_AGENT_BEGIN,

      EDU_TYPE_AGENT,
      EDU_TYPE_SHARDAGENT,
      EDU_TYPE_REPLAGENT,
      EDU_TYPE_RESTAGENT,
      EDU_TYPE_FAPAGENT,
      EDU_TYPE_OMAAGENT,

      // Agent EDU Type END
      EDU_TYPE_AGENT_END,

      //background job EDU Type
      EDU_TYPE_BACKGROUND_JOB,

      EDU_TYPE_LOADWORKER,
      EDU_TYPE_PREFETCHER,

      EDU_TYPE_MAIN,

      // edu for search engine adapter.
      EDU_TYPE_SEADPTMGR,
      EDU_TYPE_SE_SERVICE,
      EDU_TYPE_SE_INDEXR,
      EDU_TYPE_SE_INDEX,
      EDU_TYPE_SE_AGENT,

      EDU_TYPE_SUB_NET_AGENT,

      EDU_TYPE_FS_MCS_NET_SERVICE,
      EDU_TYPE_FS_MCS_NET_AGENT,

      EDU_TYPE_UNKNOWN,
      EDU_TYPE_MAXIMUM = EDU_TYPE_UNKNOWN
   } ;

   /*
      EDU_STATUS define
   */
   enum EDU_STATUS
   {
      // EDU Manager initialize status to this
      PMD_EDU_CREATING = 0,
      // EDU should change status to running when serve a request
      PMD_EDU_RUNNING,
      // EDU should change to wait after request result send back
      PMD_EDU_WAITING,
      // EDU should change status to idle when get into pool
      PMD_EDU_IDLE,
      // EDU should change status to destroy when removing from runmap
      PMD_EDU_DESTROY,

      PMD_EDU_UNKNOW,
      PMD_EDU_STATUS_MAXIMUM = PMD_EDU_UNKNOW
   } ;

   #define PMD_IS_EDU_CREATING(x)      ( PMD_EDU_CREATING == x )
   #define PMD_IS_EDU_RUNNING(x)       ( PMD_EDU_RUNNING  == x )
   #define PMD_IS_EDU_WAITING(x)       ( PMD_EDU_WAITING  == x )
   #define PMD_IS_EDU_IDLE(x)          ( PMD_EDU_IDLE     == x )

   /*
      EDU_BLOCK_TYPE define
   */
   typedef UINT32 EDU_BLOCK_TYPE ;

   #define EDU_BLOCK_NONE           ( 0x00000000 )
   #define EDU_BLOCK_FREEZING_WND   ( 0x00000001 )
   #define EDU_BLOCK_DMS            ( 0x00000002 )
   #define EDU_BLOCK_PRIMARY        ( 0x00000004 )
   #define EDU_BLOCK_TRANSROLLBACK  ( 0x00000008 )
   #define EDU_BLOCK_REELECT        ( 0x00000010 )
   #define EDU_BLOCK_SYNCWAIT       ( 0x00000020 )
   #define EDU_BLOCK_SYNCCONTROL    ( 0x00000040 )
   #define EDU_BLOCK_WAITREPLY      ( 0x00000080 )
   #define EDU_BLOCK_FT             ( 0x00000100 )
   #define EDU_BLOCK_RENAMECHK      ( 0x00000200 )
   #define EDU_BLOCK_ALL            ( 0xFFFFFFFF )

   /*
      SDB_TYPE_STR DEFINE
   */
   #define SDB_TYPE_DB_STR             "sequoiadb"
   #define SDB_TYPE_OM_STR             "sdbom"
   #define SDB_TYPE_OMA_STR            "sdbcm"

   /*
      SDB_DB_STATUS_STR DEFINE
   */
   #define SDB_DB_NORMAL_STR           "Normal"
   #define SDB_DB_SHUTDOWN_STR         "Shutdown"
   #define SDB_DB_REBUILDING_STR       "Rebuilding"
   #define SDB_DB_FULLSYNC_STR         "FullSync"
   #define SDB_DB_OFFLINE_BK_STR       "OfflineBackup"

   /*
      SDB_DATA_STATUS_STR DEFINE
   */
   #define SDB_DATA_NORMAL_STR           "Normal"
   #define SDB_DATA_REPAIR_STR           "Repairing"
   #define SDB_DATA_FAULT_STR            "Fault"

   /*
      SDB_DB_MODE_STR DEFINE
   */
   #define SDB_DB_MODE_READONLY_STR    "Readonly"
   #define SDB_DB_MODE_DEACTIVATED_STR "Deactivated"

   /*
      define
   */
   #define PMD_CONF_DIR_NAME           "conf"
   #define PMD_DFT_CONF                "sdb.conf"
   #define PMD_DFT_CAT                 "sdb.cat"
   #define PMD_OPTION_DIAG_PATH        "diaglog"
   #define PMD_OPTION_AUDIT_PATH       PMD_OPTION_DIAG_PATH
   #define PMD_OPTION_LOG_PATH         "replicalog"
   #define PMD_OPTION_BK_PATH          "bakfile"
   #define PMD_OPTION_WWW_PATH_DIR     "web"
   #define PMD_OPTION_TMPBLK_PATH      "tmp"
   #define PMD_OPTION_ARCHIVE_LOG_PATH "archivelog"
   #define PMD_CURRENT_PATH            "./"

   #define ENGINE_NPIPE_PREFIX         "sequoiadb_engine_"
   #if defined (_LINUX)
   #define PROC_PATH                   "/proc"
   #define PROC_CMDLINE_PATH_FORMAT    PROC_PATH"/%s/cmdline"
   #define ENGINE_NAME                 "sequoiadb"
   #define ENGINE_NPIPE_PREFIX_BW      "sequoiadb_engine_bw_"
   #define PMD_OPTION_IGNOREULIMIT     "ignoreulimit"
   #elif defined (_WINDOWS)
   #define ENGINE_NAME                 "sequoiadb.exe"
   #define PMD_OPTION_AS_PROC          "asproc"
   #endif // _LINUX
   #define PMD_OPTION_HELPFULL         "helpfull"
   #define PMD_OPTION_TYPE             "type"
   #define PMD_OPTION_MODE             "mode"
   #define PMD_OPTION_DETAIL           "detail"
   #define PMD_OPTION_EXPAND           "expand"
   #define PMD_OPTION_LONG             "long"
   #define PMD_OPTION_CURUSER          "I"
   #define PMD_OPTION_PORT             "port"
   #define PMD_OPTION_STANDALONE       "standalone"      // for om
   #define PMD_OPTION_ALIVE_TIME       "alivetime"       // for om
   #define PMD_OPTION_FORCE            "force"

   /*
      SDB_RUN_MODE_TYPE_STR DEFINE
   */
   #define SDB_RUN_MODE_TYPE_LOCAL_STR  "local"
   #define SDB_RUN_MODE_TYPE_RUN_STR    "run"

   /*
     SDBLIST_TYPE_STR
   */
   #define SDBLIST_TYPE_OMA_STR    "cm"
   #define SDBLIST_TYPE_OM_STR     "om"
   #define SDBLIST_TYPE_DB_STR     "db"
   #define SDBLIST_TYPE_ALL_STR    "all"

   /*
      SDB_RUN_MODE_TYPE define
   */
   enum SDB_RUN_MODE_TYPE
   {
      RUN_MODE_LOCAL = 1,
      RUN_MODE_RUN
   } ;

   #define ENGINE_NPIPE_MSG_PID        "$pid"
   #define ENGINE_NPIPE_MSG_SHUTDOWN   "$shutdown"
   #define ENGINE_NPIPE_MSG_TYPE       "$type"
   #define ENGINE_NPIPE_MSG_ROLE       "$role"
   #define ENGINE_NPIPE_MSG_GID        "$gid"
   #define ENGINE_NPIPE_MSG_NID        "$nid"
   #define ENGINE_NPIPE_MSG_GNAME      "$gname"
   #define ENGINE_NPIPE_MSG_PATH       "$path"
   #define ENGINE_NPIPE_MSG_PRIMARY    "$primary"
   #define ENGINE_NPIPE_MSG_ENDPIPE    "$endpipe"
   #define ENGINE_NPIPE_MSG_STARTTIME  "$starttime"
   #define ENGINE_NPIPE_MSG_DOING      "$doing"
   #define ENGINE_NPIPE_MSG_LOCATION   "$location"
   #define ENGINE_NPIPE_MSG_LOCPRIMARY "$locprimary"

   /*
      Config define
   */
   #define PMD_MIN_LOG_FILE_SZ                  64
   #define PMD_MAX_LOG_FILE_SZ                  2048
   #define PMD_DFT_LOG_FILE_SZ                  PMD_MIN_LOG_FILE_SZ
   #define PMD_DFT_LOG_FILE_NUM                 20

   #define PMD_REPL_PORT      1  // by default it's service port + 1
   #define PMD_SHARD_PORT     2  // by default it's service port + 2
   #define PMD_CAT_PORT       3  // by default it's service port + 3
   #define PMD_REST_PORT      4  // by default it's service port + 4
   #define PMD_OM_PORT        5  // by default it's service port + 5

   /*
      PMD Option define
   */
   #define PMD_ADD_PARAM_OPTIONS_BEGIN( desc )  desc.add_options()
   #define PMD_ADD_PARAM_OPTIONS_END ;
   #define PMD_COMMANDS_STRING( a, b )       (string(a) +string( b)).c_str()


   /*
      SDB_TYPE define
   */
   enum SDB_TYPE
   {
      SDB_TYPE_DB  = 1,    // sequoiadb: data, standalone, coord, catalog
      SDB_TYPE_OM,         // om
      SDB_TYPE_OMA,        // omagent

      SDB_TYPE_MAX
   } ;

   /*
      other define
   */
   #define PMD_IPADDR_LEN              ( 40 )
   #define PMD_CLIENTNAME_LEN          ( 64 )

   /*
      PMD_FT_RISK_TYPE define
   */
   enum PMD_FT_RISK_TYPE
   {
      FT_RISK_NONE            = 0,
      FT_RISK_SLOW_NODE,
      FT_RISK_DEADSYNC,
      FT_RISK_TRANSERR,

      FT_RISK_MAX
   } ;

   /*
      PMD_FT_ERR_TYPE define
   */
   enum PMD_FT_ERR_TYPE
   {
      FT_ERR_NONE             = 0,
      FT_ERR_NOSPC,
      FT_ERR_SYNC_FAILED,

      FT_ERR_MAX
   } ;

   /*
      define FT MASK
   */
   #define PMD_FT_MASK_NOSPC           0x00000001
   #define PMD_FT_MASK_DEADSYNC        0x00000002
   #define PMD_FT_MASK_SLOWNODE        0x00000004
   #define PMD_FT_MASK_TRANSERR        0x00000008

   #define PMD_FT_MASK_ALL             0xFFFFFFFF

   #define PMD_FT_MASK_DFT             ( PMD_FT_MASK_NOSPC|\
                                         PMD_FT_MASK_DEADSYNC )

   #define PMD_FT_MASK_NOSPC_STR       "NOSPC"
   #define PMD_FT_MASK_DEADSYNC_STR    "DEADSYNC"
   #define PMD_FT_MASK_SLOWNODE_STR    "SLOWNODE"
   #define PMD_FT_MASK_TRANSERR_STR    "TRANSERR"

   #define PMD_FT_MASK_DFT_STR         ( PMD_FT_MASK_NOSPC_STR"|"\
                                         PMD_FT_MASK_DEADSYNC_STR )

   #define PMD_FT_IS_FATAL_FAULT(mask) \
      ( (PMD_FT_MASK_NOSPC & (mask)) || \
        (PMD_FT_MASK_DEADSYNC & (mask)) )

   /*
      Global define
   */
   #define PMD_FT_SAMPLE_WINDOW_SZ     ( 1200 )
   #define PMD_FT_SAMPLE_INTERVAL      ( 3 )       /// second
   #define PMD_FT_CACL_INTERVAL_MIN    PMD_FT_SAMPLE_INTERVAL
   #define PMD_FT_CACL_INTERVAL_MAX    ( PMD_FT_SAMPLE_INTERVAL * \
                                         PMD_FT_SAMPLE_WINDOW_SZ )
   #define PMD_FT_CACL_WINDOW_DEF      ( 20 )
   #define PMD_FT_CACL_INTERVAL_DFT    ( PMD_FT_SAMPLE_INTERVAL * \
                                         PMD_FT_CACL_WINDOW_DEF )

   #define PMD_FT_CACL_RATIO_DFT       ( 80 )   /// %
   #define PMD_FT_CACL_RATIO_MIN       ( 1 )
   #define PMD_FT_CACL_RATIO_MAX       ( 100 )

   /*
      PMD_FT_LEVEL define
   */
   enum PMD_FT_LEVEL
   {
      FT_LEVEL_FUSING   = 1,
      FT_LEVEL_SEMI     = 2,
      FT_LEVEL_WHOLE    = 3
   } ;

   #define PMD_SVC_MASK_LOCAL            0x00000001
   #define PMD_SVC_MASK_REPLICATE        0x00000002
   #define PMD_SVC_MASK_SHARD            0x00000004
   #define PMD_SVC_MASK_CATALOG          0x00000008
   #define PMD_SVC_MASK_HTTP             0x00000010
   #define PMD_SVC_MASK_NONE             0

   #define PMD_SVC_MASK_LOCAL_STR        "LOCAL"
   #define PMD_SVC_MASK_REPLICATE_STR    "REPL"
   #define PMD_SVC_MASK_SHARD_STR        "SHARD"
   #define PMD_SVC_MASK_CATALOG_STR      "CATALOG"
   #define PMD_SVC_MASK_HTTP_STR         "HTTP"
   #define PMD_SVC_MASK_ALL_STR          "ALL"
   #define PMD_SVC_MASK_NONE_STR         "NONE"


   #define PMD_MON_GROUP_MASK_DEFT_STR   "all:off"
}

#endif // PMD_DEF_HPP__

