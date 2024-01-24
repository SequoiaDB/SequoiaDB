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

   Source File Name = monDump.cpp

   Descriptive Name = Monitoring Dump

   When/how to use: this program may be used on binary and text-formatted
   versions of Monitoring component. This file contains functions for
   creating resultset for a given resource.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <set>
#include <map>
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "dmsCB.hpp"
#include "monDump.hpp"
#include "monEDU.hpp"
#include "monDMS.hpp"
#include "pmdOptionsMgr.hpp"
#include "ossSocket.hpp"
#include "ossVer.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsDump.hpp"
#include "barBkupLogger.hpp"
#include "rtnCommand.hpp"
#include "msgMessage.hpp"
#include "rtn.hpp"
#include "barBkupLogger.hpp"
#include "pdTrace.hpp"
#include "monTrace.hpp"
#include "pmdEnv.hpp"
#include "rtnScannerFactory.hpp"
#include "utilEnvCheck.hpp"
#include "pmdStartupHistoryLogger.hpp"
#include "utilMemListPool.hpp"
#include "dpsUtil.hpp"
#include "msgDef.h"
#include "monMgr.hpp"
#include "utilMath.hpp"

using namespace bson ;
using namespace boost::asio::ip ;
#define OSS_MAX_SESSIONNAME ( OSS_MAX_HOSTNAME+OSS_MAX_SERVICENAME+30 )

namespace engine
{
   #define MON_MAX_SLICE_SIZE       ( 1000 )
   #define MON_TMP_STR_SZ           ( 64 )
   #define OSS_MAX_FILE_SZ          ( 8796093022208ll )

   #define MON_DUMP_DFT_BUILDER_SZ  ( 1024 )
   #define MON_DUMP_BUFF_STAT_SZ    ( 128 )

   #define MON_CL_DETAIL_VERSION_NULL ( 0 )
   #define MON_CL_DETAIL_VERSION_V1 ( 1 )
   #define MON_CL_DETAIL_VERSION_V2 ( 2 )
   #define MON_CL_DETAIL_CURRENT_V  MON_CL_DETAIL_VERSION_V2

   #define MON_CL_STAT_VERSION_NULL ( 0 )
   #define MON_CL_STAT_VERSION_V1 ( 1 )
   #define MON_CL_STAT_CURRENT_V MON_CL_STAT_VERSION_V1

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONGETNODENAME, "monGetNodeName" )
   static CHAR *monGetNodeName ( CHAR *nodeName,
                                 UINT32 size,
                                 const CHAR *hostName,
                                 const CHAR *serviceName )
   {
      CHAR *ret = NULL ;
      PD_TRACE_ENTRY ( SDB_MONGETNODENAME ) ;
      PD_TRACE4 ( SDB_MONGETNODENAME, PD_PACK_STRING ( nodeName ),
                  PD_PACK_UINT ( size ), PD_PACK_STRING ( hostName ),
                  PD_PACK_STRING ( serviceName ) ) ;
      INT32 hostSize = 0 ;
      if ( !nodeName )
      {
         goto done ;
      }
      hostSize = size < ossStrlen(hostName) ? size : ossStrlen(hostName) ;
      if ( !ossStrncpy ( nodeName, hostName, hostSize ) )
      {
         goto done ;
      }
      *( nodeName + hostSize ) = NODE_NAME_SERVICE_SEPCHAR ;
      ++hostSize ;
      size -= hostSize ;
      if ( !ossStrncpy ( nodeName + hostSize,
                         serviceName,
                         size < ossStrlen(serviceName) ?
                         size : ossStrlen(serviceName) ) )
      {
         goto done ;
      }
      ret = nodeName ;
   done :
      PD_TRACE1 ( SDB_MONGETNODENAME, PD_PACK_STRING ( ret ) ) ;
      PD_TRACE_EXIT ( SDB_MONGETNODENAME ) ;
      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONGETSESSIONNAME, "monGetSessionName" )
   static INT32 monGetSessionName( char *pSessName, UINT32 size, SINT64 sessionId )
   {
      INT32 rc = SDB_OK;
      UINT32 curPos = 0;
      PD_TRACE_ENTRY ( SDB_MONGETSESSIONNAME ) ;
      *(pSessName + size - 1) = 0;

      const CHAR* hostName = pmdGetKRCB()->getHostName();
      ossStrncpy(pSessName, hostName, size - 1);

      curPos = ossStrlen( pSessName );
      PD_CHECK( curPos < size - 1, SDB_INVALIDARG, error, PDERROR,
               "out off buffer!" );
      *(pSessName + curPos) = NODE_NAME_SERVICE_SEPCHAR;
      ++curPos;
      PD_CHECK( curPos < size - 1, SDB_INVALIDARG, error, PDERROR,
               "out off buffer!" ) ;
      ossStrncpy( pSessName + curPos, pmdGetOptionCB()->getServiceAddr(),
                  size - 1 - curPos ) ;
      curPos = ossStrlen( pSessName );
      PD_CHECK( curPos < size - 1, SDB_INVALIDARG, error, PDERROR,
               "out off buffer!" );
      *(pSessName + curPos) = NODE_NAME_SERVICE_SEPCHAR;
      ++curPos;
      PD_CHECK( curPos < size - 1, SDB_INVALIDARG, error, PDERROR,
               "out off buffer!" );
      ossSnprintf( pSessName + curPos, size - 1 - curPos,
                  OSS_LL_PRINT_FORMAT, sessionId );
   done :
      PD_TRACE_EXITRC ( SDB_MONGETSESSIONNAME, rc ) ;
      return rc;
   error :
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPSESSIONNAME, "monAppendSessionName" )
   INT32 monAppendSessionName ( BSONObjBuilder &ob, INT64 sessionId )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_MONAPPSESSIONNAME ) ;
      CHAR sessionName[OSS_MAX_SESSIONNAME + 1] = {0};
      rc = monGetSessionName( sessionName,
                              OSS_MAX_SESSIONNAME + 1,
                              sessionId );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to get session-name(rc=%d)",
                  rc );
      ob.append( FIELD_NAME_SESSIONID, sessionName );
   done:
      PD_TRACE_EXITRC ( SDB_MONAPPSESSIONNAME, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDSYSTEMINFO, "monAppendSystemInfo" )
   INT32 monAppendSystemInfo ( BSONObjBuilder &ob, UINT32 mask )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONAPPENDSYSTEMINFO ) ;

      pmdKRCB *krcb     = pmdGetKRCB() ;
      SDB_DPSCB *dpscb  = krcb->getDPSCB() ;
      dpsTransCB *transCB = krcb->getTransCB() ;
      clsCB *pClsCB     = krcb->getClsCB() ;

      const CHAR *serviceName       = pmdGetOptionCB()->getServiceAddr() ;
      const CHAR *groupName         = krcb->getGroupName() ;
      const CHAR *hostName          = krcb->getHostName() ;
      const CHAR *locationName      = pmdGetLocation() ;
      CHAR nodeName [ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1 + 1 ] = {0} ;
      UINT32 nodeNameSize = OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1 ;

      monGetNodeName( nodeName, nodeNameSize, hostName, serviceName ) ;

      PD_TRACE4 ( SDB_MONAPPENDSYSTEMINFO, PD_PACK_STRING ( hostName ),
                  PD_PACK_STRING ( serviceName ),
                  PD_PACK_STRING ( groupName ),
                  PD_PACK_STRING ( nodeName ) ) ;
      try
      {
         if ( MON_MASK_NODE_NAME & mask )
         {
            ob.append ( FIELD_NAME_NODE_NAME, nodeName ) ;
         }
         if ( MON_MASK_HOSTNAME & mask )
         {
            ob.append ( FIELD_NAME_HOST, hostName ) ;
         }
         if ( MON_MASK_SERVICE_NAME & mask )
         {
            ob.append ( FIELD_NAME_SERVICE_NAME, serviceName ) ;
         }

         if ( MON_MASK_GROUP_NAME & mask )
         {
            ob.append ( FIELD_NAME_GROUPNAME, groupName ) ;
         }
         if ( MON_MASK_IS_PRIMARY & mask )
         {
            ob.appendBool ( FIELD_NAME_IS_PRIMARY, pmdIsPrimary() ) ;
         }
         if ( MON_MASK_LOCATION & mask )
         {
            ob.append( FIELD_NAME_LOCATION, locationName ) ;
         }
         if ( MON_MASK_IS_LOC_PRIMARY & mask )
         {
            ob.appendBool( FIELD_NAME_IS_LOC_PRIMARY, pmdIsLocationPrimary() ) ;
         }
         if ( MON_MASK_SERVICE_STATUS & mask )
         {
            ob.appendBool ( FIELD_NAME_SERVICE_STATUS,
                            PMD_IS_DB_AVAILABLE() ) ;
            ob.append( FIELD_NAME_GROUP_STATUS,
                       utilDBStatusStr( krcb->getDBStatus() ) ) ;
            CHAR tmpText[ MON_DUMP_BUFF_STAT_SZ + 1 ] = { 0 } ;
            utilFTMaskToStr( pmdGetKRCB()->getFTMgr()->getConfirmedStat(),
                             tmpText, MON_DUMP_BUFF_STAT_SZ ) ;
            ob.append( FIELD_NAME_FT_STATUS, tmpText ) ;
         }

         if ( MON_MASK_LSN_INFO & mask )
         {
            /// begint lsn, current lsn, committed lsn
            DPS_LSN beginLSN ;
            DPS_LSN currentLSN ;
            DPS_LSN committed ;
            DPS_LSN expectLSN ;
            if ( dpscb )
            {
               dpscb->getLsnWindow( beginLSN, currentLSN, &expectLSN, &committed ) ;
               INT64 currentLSNOff = (INT64)currentLSN.offset ;
               PD_TRACE2 ( SDB_MONAPPENDSYSTEMINFO,
                           PD_PACK_RAW ( &currentLSN, sizeof(DPS_LSN) ),
                           PD_PACK_LONG ( currentLSNOff ) ) ;
            }
            BSONObjBuilder subBegin( ob.subobjStart( FIELD_NAME_BEGIN_LSN ) ) ;
            subBegin.append( FIELD_NAME_LSN_OFFSET, (INT64)beginLSN.offset ) ;
            subBegin.append( FIELD_NAME_LSN_VERSION, beginLSN.version ) ;
            subBegin.done() ;

            BSONObjBuilder subCur( ob.subobjStart( FIELD_NAME_CURRENT_LSN ) ) ;
            subCur.append( FIELD_NAME_LSN_OFFSET, (INT64)currentLSN.offset ) ;
            subCur.append( FIELD_NAME_LSN_VERSION, currentLSN.version ) ;
            subCur.done() ;

            BSONObjBuilder subCommit( ob.subobjStart( FIELD_NAME_COMMIT_LSN ) ) ;
            subCommit.append( FIELD_NAME_LSN_OFFSET, (INT64)committed.offset ) ;
            subCommit.append( FIELD_NAME_LSN_VERSION, committed.version ) ;
            subCommit.done() ;

            /// complete lsn and queue size
            DPS_LSN completeLSN ;
            UINT32 lsnQueSize = 0 ;
            if ( pClsCB )
            {
               clsBucket *pBucket = pClsCB->getReplCB()->getBucket() ;
               completeLSN = pBucket->fastCompleteLSN() ;
               lsnQueSize = pBucket->bucketSize() ;
               if ( pClsCB->isPrimary() || completeLSN.invalid() )
               {
                  completeLSN = expectLSN ;
               }
            }
            ob.append( FIELD_NAME_COMPLETE_LSN, (INT64)completeLSN.offset ) ;
            ob.append( FIELD_NAME_LSN_QUE_SIZE, (INT32)lsnQueSize ) ;
         }

         if ( MON_MASK_TRANSINFO & mask )
         {
            UINT32 transCount = 0 ;
            DPS_LSN_OFFSET beginLSNOff = 0 ;
            if ( transCB )
            {
               transCount = pmdIsPrimary() ? transCB->getTransCBSize() :
                            (UINT32)transCB->getTransMap()->size() ;
               beginLSNOff = transCB->getOldestBeginLsn() ;
            }
            BSONObjBuilder subTrans( ob.subobjStart( FIELD_NAME_TRANS_INFO ) ) ;
            subTrans.append( FIELD_NAME_TOTAL_COUNT, (INT32)transCount ) ;
            subTrans.append( FIELD_NAME_BEGIN_LSN, (INT64)beginLSNOff ) ;
            subTrans.done() ;
         }

         if ( MON_MASK_NODEID & mask )
         {
            NodeID selfID = pmdGetNodeID() ;
            BSONArrayBuilder subID( ob.subarrayStart( FIELD_NAME_NODEID ) ) ;
            subID.append( (INT32)selfID.columns.groupID ) ;
            subID.append( (INT32)selfID.columns.nodeID ) ;
            subID.done() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to append system information, %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONAPPENDSYSTEMINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDVERSION, "monAppendVersion" )
   void monAppendVersion ( BSONObjBuilder &ob )
   {
      INT32 major        = 0 ;
      INT32 minor        = 0 ;
      INT32 fix          = 0 ;
      INT32 release      = 0 ;
      const CHAR *pBuild = NULL ;
      const CHAR *pGitVer = NULL ;
      PD_TRACE_ENTRY ( SDB_MONAPPENDVERSION ) ;
      ossGetVersion ( &major, &minor, &fix, &release, &pBuild, &pGitVer ) ;
      if ( pGitVer )
      {
         PD_TRACE5 ( SDB_MONAPPENDVERSION,
                     PD_PACK_INT ( major ),
                     PD_PACK_INT ( minor ),
                     PD_PACK_INT ( release ),
                     PD_PACK_STRING ( pBuild ),
                     PD_PACK_STRING( pGitVer ) ) ;
      }
      else
      {
         PD_TRACE4 ( SDB_MONAPPENDVERSION,
                     PD_PACK_INT ( major ),
                     PD_PACK_INT ( minor ),
                     PD_PACK_INT ( release ),
                     PD_PACK_STRING ( pBuild ) ) ;
      }

      try
      {
         BSONObjBuilder obVersion( ob.subobjStart( FIELD_NAME_VERSION ) ) ;
         obVersion.append ( FIELD_NAME_MAJOR, major ) ;
         obVersion.append ( FIELD_NAME_MINOR, minor ) ;
         obVersion.append ( FIELD_NAME_FIX, fix ) ;
         obVersion.append ( FIELD_NAME_RELEASE, release ) ;
         if ( NULL != pGitVer )
         {
            obVersion.append ( FIELD_NAME_GITVERSION, pGitVer ) ;
         }
         obVersion.append ( FIELD_NAME_BUILD, pBuild ) ;
         obVersion.done() ;

#ifdef SDB_ENTERPRISE
         ob.append ( FIELD_NAME_EDITION, "Enterprise" ) ;
#endif // SDB_ENTERPRISE
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to append version information, %s",
                  e.what() ) ;
      }
      PD_TRACE_EXIT ( SDB_MONAPPENDVERSION ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDULIMIT, "monAppendUlimit" )
   void monAppendUlimit( BSONObjBuilder &ob )
   {
      PD_TRACE_ENTRY( SDB_MONAPPENDULIMIT ) ;

      INT64 soft = -1 ;
      INT64 hard = -1 ;
      BOOLEAN isSucc = FALSE ;
      ossProcLimits *limInfo = pmdGetLimit() ;

      BSONObjBuilder limOb( ob.subobjStart( FIELD_NAME_ULIMIT ) ) ;

      isSucc = limInfo->getLimit( OSS_LIMIT_CORE_SZ, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_CORESZ, soft ) ;
      }

      isSucc = limInfo->getLimit( OSS_LIMIT_VIRTUAL_MEM, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_VM, soft ) ;
      }

      isSucc = limInfo->getLimit( OSS_LIMIT_OPEN_FILE, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_OPENFL, soft ) ;
      }

      isSucc = limInfo->getLimit( OSS_LIMIT_PROC_NUM, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_NPROC, soft ) ;
      }

      isSucc = limInfo->getLimit( OSS_LIMIT_FILE_SZ, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_FILESZ, soft ) ;
      }

      isSucc = limInfo->getLimit( OSS_LIMIT_STACK_SIZE, soft, hard ) ;
      if ( isSucc )
      {
         limOb.append( FIELD_NAME_STACKSZ, soft ) ;
      }

      limOb.done() ;

      PD_TRACE_EXIT( SDB_MONAPPENDULIMIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDFILEDESP, "monAppendFileDesp" )
   INT32 monAppendFileDesp( BSONObjBuilder &ob )
   {
      PD_TRACE_ENTRY( SDB_MONAPPENDFILEDESP ) ;
      INT32 loadPercent       = -1 ;
      INT64 totalNum          = -1 ;
      INT64 freeNum           = -1 ;
      INT64 usedNum           = -1 ;
      INT64 softLimit         = -1 ;
      INT64 hardLimit         = -1 ;
      INT32 rc                = SDB_OK ;
      ossProcLimits *limInfo  = pmdGetLimit() ;

      // get total number in ulimit
      limInfo->getLimit( OSS_LIMIT_OPEN_FILE, softLimit, hardLimit ) ;
      totalNum = softLimit ;

      // get used number
      rc = ossGetFileDesp( usedNum ) ;
      if ( SDB_TOO_MANY_OPEN_FD == rc )
      {
         usedNum = totalNum ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get file description info, rc = %d", rc ) ;
         goto error ;
      }

      // get percent
      if ( totalNum != 0 )
      {
         freeNum = totalNum - usedNum ;
         loadPercent = 100 * usedNum / totalNum ;
         loadPercent = loadPercent > 100? 100:loadPercent ;
         loadPercent = loadPercent < 0? 0:loadPercent ;
      }
      else
      {
         freeNum = -1 ;
         loadPercent = 0 ;
      }

      // build bson object
      {
         BSONObjBuilder fdOb( ob.subobjStart( FIELD_NAME_FILEDESP ) ) ;
         fdOb.append( FIELD_NAME_LOADPERCENT, loadPercent ) ;
         fdOb.append( FIELD_NAME_TOTALNUM,    totalNum ) ;
         fdOb.append( FIELD_NAME_FREENUM,     freeNum ) ;
         fdOb.done() ;
      }
   done:
      PD_TRACE_EXITRC( SDB_MONAPPENDFILEDESP, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDSTARTINFO, "monAppendStartInfo" )
   INT32 monAppendStartInfo( BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_MONAPPENDSTARTINFO ) ;
      PMD_STARTUP_LOG_LIST startLogs ;
      UINT32 logNum = 10 ;

      rc = pmdGetStartupHstLogger()->getLatestLogs( logNum, startLogs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logs from start-up history "
                   "logger, rc = %d", rc ) ;

      {
         vector<string> startHst, abnormalHst;
         PMD_STARTUP_LOG_LIST::iterator i ;
         for ( i = startLogs.begin(); i != startLogs.end(); ++i )
         {
            pmdStartupLog log = *i ;
            CHAR time[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
            ossTimestampToString( log._time, time ) ;

            startHst.push_back( string( time ) ) ;
            if( SDB_START_NORMAL != log._type )
            {
               abnormalHst.push_back( string( time ) ) ;
            }
         }
         ob.append( FIELD_NAME_STARTHST,    startHst ) ;
         ob.append( FIELD_NAME_ABNORMALHST, abnormalHst ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONAPPENDSTARTINFO, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDHOSTMEMORY, "monAppendHostMemory" )
   INT32 monAppendHostMemory( BSONObjBuilder &ob )
   {
      PD_TRACE_ENTRY( SDB_MONAPPENDHOSTMEMORY ) ;
      INT32 rc             = SDB_OK ;
      INT32 memLoadPercent = 0 ;
      INT64 memTotalPhys   = 0 ;
      INT64 memFreePhys    = 0 ;
      INT64 memAvailPhys   = 0 ;
      INT64 memTotalPF     = 0 ;
      INT64 memAvailPF     = 0 ;
      INT64 memTotalVirtual= 0 ;
      INT64 memAvailVirtual= 0 ;

      rc = ossGetMemoryInfo( memLoadPercent, memTotalPhys, memFreePhys,
                             memAvailPhys, memTotalPF, memAvailPF,
                             memTotalVirtual, memAvailVirtual ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info, rc = %d", rc ) ;

      {
         BSONObjBuilder memOb( ob.subobjStart( FIELD_NAME_MEMORY ) ) ;
         memOb.append( FIELD_NAME_LOADPERCENT, memLoadPercent ) ;
         memOb.append( FIELD_NAME_TOTALRAM, memTotalPhys ) ;
         memOb.append( FIELD_NAME_FREERAM, memFreePhys ) ;
         memOb.append( FIELD_NAME_AVAILABLERAM, memAvailPhys ) ;
         memOb.append( FIELD_NAME_TOTALSWAP, memTotalPF ) ;
         memOb.append( FIELD_NAME_FREESWAP, memAvailPF ) ;
         memOb.append( FIELD_NAME_TOTALVIRTUAL, memTotalVirtual ) ;
         memOb.append( FIELD_NAME_FREEVIRTUAL, memAvailVirtual ) ;
         memOb.done();
      }

   done:
      PD_TRACE_EXITRC( SDB_MONAPPENDHOSTMEMORY, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDNODEMEMORY, "monAppendNodeMemory" )
   INT32 monAppendNodeMemory( BSONObjBuilder &ob )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB_MONAPPENDNODEMEMORY ) ;

      INT32 loadPctRAM        = 0 ;
      INT64 totalRAM          = 0 ;
      INT64 freeRAM           = 0 ;
      INT64 availRAM          = 0 ;
      INT64 totalSwap         = 0 ;
      INT64 availSwap         = 0 ;
      INT64 totalVM           = 0 ;
      INT64 availVM           = 0 ;
      INT64 softLimit         = -1 ;
      INT64 hardLimit         = -1 ;
      INT32 overCommitMode    = -1 ;
      INT64 commitLimit       = 0 ;
      INT64 committedAS       = 0 ;
      INT64 VMLimitProc       = 0 ;
      INT32 loadPctVM         = 0 ;
      INT64 allocatedVMProc   = 0 ;
      INT64 rss               = 0 ;
      ossProcLimits *limInfo  = pmdGetLimit() ;

      // get memory of host
      rc = ossGetMemoryInfo( loadPctRAM, totalRAM, freeRAM, availRAM,
                             totalSwap, availSwap, totalVM, availVM,
                             overCommitMode, commitLimit, committedAS ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info of host, "
                   "rc = %d", rc ) ;

      // get memory of process
      rc = ossGetProcessMemory( ossGetCurrentProcessID(), rss, allocatedVMProc );
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info of process, "
                   "rc = %d", rc ) ;

      // get limited virtual memory in ulimit
      limInfo->getLimit( OSS_LIMIT_VIRTUAL_MEM, softLimit, hardLimit ) ;

      // caculate
      if ( 2 == overCommitMode )
      {
         if ( -1 == softLimit )
         {
            VMLimitProc = commitLimit ;
         }
         else
         {
            VMLimitProc = commitLimit > softLimit ? softLimit : commitLimit ;
         }
      }
      else
      {
         VMLimitProc = softLimit ;
      }

      if ( VMLimitProc != 0 )
      {
         loadPctVM = 100 * allocatedVMProc / VMLimitProc ;
         loadPctVM = loadPctVM > 100 ? 100: loadPctVM ;
         loadPctVM = loadPctVM < 0 ? 0 : loadPctVM ;
      }
      else
      {
         loadPctVM = 0 ;
      }

      if ( totalRAM != 0 )
      {
         loadPctRAM = 100 * rss / totalRAM ;
         loadPctRAM = loadPctRAM > 100 ? 100 : loadPctRAM ;
         loadPctRAM = loadPctRAM < 0 ? 0 : loadPctRAM ;
      }
      else
      {
         loadPctRAM = 0 ;
      }

      // append bson
      {
         BSONObjBuilder memOb( ob.subobjStart( FIELD_NAME_MEMORY ) ) ;

         memOb.append( FIELD_NAME_LOADPERCENT,   loadPctRAM ) ;
         memOb.append( FIELD_NAME_TOTALRAM,      totalRAM ) ;
         memOb.append( FIELD_NAME_RSSSIZE,       rss ) ;

         memOb.append( FIELD_NAME_LOADPERCENTVM, loadPctVM ) ;
         memOb.append( FIELD_NAME_VMLIMIT,       VMLimitProc ) ;
         memOb.append( FIELD_NAME_VMSIZE,        allocatedVMProc ) ;

         memOb.done();
      }

   done:
      PD_TRACE_EXITRC( SDB_MONAPPENDNODEMEMORY, rc ) ;
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDDIFFLSN, "monAppendDiffLSN" )
   void monAppendDiffLSN( BSONObjBuilder &ob )
   {
      PD_TRACE_ENTRY( SDB_MONAPPENDDIFFLSN ) ;
      pmdKRCB *krcb        = pmdGetKRCB() ;
      SDB_DPSCB* dpscb     = krcb->getDPSCB() ;
      clsCB *clscb         = krcb->getClsCB() ;
      replCB *replcb       = NULL ;
      INT64 diffOffset     = -1 ;

      if ( clscb )
      {
         replcb = clscb->getReplCB() ;
      }

      if ( clscb && dpscb && replcb )
      {
         if ( clscb->isPrimary() )
         {
            diffOffset = 0 ;
         }
         else
         {
            _clsSharingStatus primary ;
            DPS_LSN beginLSN, currentLSN,expectLSN, primaryLSN ;

            // get self lsn
            dpscb->getLsnWindow( beginLSN, currentLSN, &expectLSN, NULL ) ;

            // get remote primary node lsn
            if ( replcb->getPrimaryInfo( primary ) )
            {
               primaryLSN = primary.beat.endLsn ;
               if ( primaryLSN.version == expectLSN.version )
               {
                  diffOffset = primaryLSN.offset - expectLSN.offset ;
                  diffOffset = diffOffset < 0 ? 0 : diffOffset ;
               }
            }
         }
      }
      ob.append( FIELD_NAME_DIFFLSNPRIMARY, diffOffset ) ;

      PD_TRACE_EXIT( SDB_MONAPPENDDIFFLSN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONAPPENDDISK, "monAppendDisk" )
   INT32 monAppendDisk( BSONObjBuilder &ob, BOOLEAN appendDbPath )
   {
      PD_TRACE_ENTRY( SDB_MONAPPENDDISK ) ;
      INT32 rc             = SDB_OK ;
      INT64 diskTotalBytes = 0 ;
      INT64 diskFreeBytes  = 0 ;
      INT64 diskAvailBytes = 0 ;
      INT32 loadPercent    = 0 ;
      const CHAR *dbPath   = pmdGetOptionCB()->getDbPath () ;
      CHAR fsName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      rc = ossGetDiskInfo( dbPath, diskTotalBytes, diskFreeBytes, diskAvailBytes,
                           loadPercent, fsName, OSS_MAX_PATHSIZE + 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get disk info, rc = %d", rc ) ;

      {
         BSONObjBuilder diskOb( ob.subobjStart( FIELD_NAME_DISK ) ) ;
         diskOb.append( FIELD_NAME_NAME, fsName ) ;
         if( appendDbPath )
         {
            diskOb.append( FIELD_NAME_DATABASEPATH, dbPath ) ;
         }
         diskOb.append( FIELD_NAME_LOADPERCENT, loadPercent ) ;
         diskOb.append( FIELD_NAME_TOTALSPACE, diskTotalBytes ) ;
         diskOb.append( FIELD_NAME_FREESPACE, diskAvailBytes ) ;
         diskOb.done();
      }

   done:
      PD_TRACE_EXITRC ( SDB_MONAPPENDDISK, rc ) ;
      return rc;
   error:
      goto done;
   }

   void monAppendSessionIdentify( BSONObjBuilder &ob,
                                  UINT64 relatedNID,
                                  UINT64 relatedEDUID,
                                  const CHAR * fieldName = NULL )
   {
      UINT32 ip = 0 ;
      UINT32 port = 0 ;
      /// IP:00000000, PORT:0000, TID:0000000000000000
      CHAR szTmp[ DPS_TRANS_RELATED_ID_STR_LEN + 1 ] = { 0 } ;

      if ( 0 != relatedNID )
      {
         ossUnpack32From64( relatedNID, ip, port ) ;
      }
      else
      {
         ip = _netFrame::getLocalAddress() ;
         port = pmdGetLocalPort() ;
      }
      ossSnprintf( szTmp, sizeof(szTmp)-1, "%08x%04x%016llx",
                   ip, (UINT16)port, relatedEDUID ) ;
      if ( NULL == fieldName )
      {
         ob.append( FIELD_NAME_RELATED_ID, szTmp ) ;
      }
      else
      {
         ob.append( fieldName, szTmp ) ;
      }
   }

   #define MON_CPU_USAGE_STR_SIZE 20
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMP, "monDBDump" )
   INT32 monDBDump ( BSONObjBuilder &ob, monDBCB *mondbcb,
                     ossTickConversionFactor &factor,
                     ossTime userTime, ossTime sysTime )
   {
      INT32 rc = SDB_OK ;
      UINT32 seconds, microseconds ;
      CHAR   timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
      CHAR   CPUTime[ MON_CPU_USAGE_STR_SIZE ] = { 0 } ;

      PD_TRACE_ENTRY ( SDB_MONDBDUMP ) ;
      ob.append( FIELD_NAME_TOTALNUMCONNECTS, (SINT64)mondbcb->getCurConns() ) ;
      ob.append( FIELD_NAME_TOTALQUERY,       (SINT64)mondbcb->totalGeneralQuery ) ;
      ob.append( FIELD_NAME_TOTALSLOWQUERY,   (SINT64)mondbcb->totalGeneralSlowQuery ) ;
      ob.append( FIELD_NAME_TOTALTRANSCOMMIT, (SINT64)mondbcb->totalTransCommit ) ;
      ob.append( FIELD_NAME_TOTALTRANSROLLBACK, (SINT64)mondbcb->totalTransRollback ) ;
      ob.append( FIELD_NAME_TOTALDATAREAD,    (SINT64)mondbcb->totalDataRead ) ;
      ob.append( FIELD_NAME_TOTALINDEXREAD,   (SINT64)mondbcb->totalIndexRead ) ;
      ob.append( FIELD_NAME_TOTALDATAWRITE,   (SINT64)mondbcb->totalDataWrite ) ;
      ob.append( FIELD_NAME_TOTALINDEXWRITE,  (SINT64)mondbcb->totalIndexWrite ) ;
      ob.append( FIELD_NAME_TOTALUPDATE,      (SINT64)mondbcb->totalUpdate ) ;
      ob.append( FIELD_NAME_TOTALDELETE,      (SINT64)mondbcb->totalDelete ) ;
      ob.append( FIELD_NAME_TOTALINSERT,      (SINT64)mondbcb->totalInsert ) ;
      ob.append( FIELD_NAME_REPLUPDATE,       (SINT64)mondbcb->replUpdate ) ;
      ob.append( FIELD_NAME_REPLDELETE,       (SINT64)mondbcb->replDelete ) ;
      ob.append( FIELD_NAME_REPLINSERT,       (SINT64)mondbcb->replInsert ) ;
      ob.append( FIELD_NAME_TOTALSELECT,      (SINT64)mondbcb->totalSelect ) ;
      ob.append( FIELD_NAME_TOTALREAD,        (SINT64)mondbcb->totalRead ) ;
      ob.append( FIELD_NAME_TOTALLOBGET,      (SINT64)mondbcb->totalLobGet ) ;
      ob.append( FIELD_NAME_TOTALLOBPUT,      (SINT64)mondbcb->totalLobPut ) ;
      ob.append( FIELD_NAME_TOTALLOBDELETE,   (SINT64)mondbcb->totalLobDelete ) ;
      ob.append( FIELD_NAME_TOTALLOBLIST,      (SINT64)mondbcb->totalLobList ) ;
      ob.append( FIELD_NAME_TOTALLOBREADSIZE,  (SINT64)mondbcb->totalLobReadSize ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITESIZE, (SINT64)mondbcb->totalLobWriteSize ) ;
      ob.append( FIELD_NAME_TOTALLOBREAD,      (SINT64)mondbcb->totalLobRead ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITE,     (SINT64)mondbcb->totalLobWrite ) ;
      ob.append( FIELD_NAME_TOTALLOBTRUNCATE,  (SINT64)mondbcb->totalLobTruncate ) ;
      ob.append( FIELD_NAME_TOTALLOBADDRESSING, (SINT64)mondbcb->totalLobAddressing ) ;

      mondbcb->totalReadTime.convertToTime ( factor, seconds, microseconds ) ;
      ob.append ( FIELD_NAME_TOTALREADTIME,
                  (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      mondbcb->totalWriteTime.convertToTime ( factor, seconds, microseconds ) ;
      ob.append ( FIELD_NAME_TOTALWRITETIME,
                  (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;
      ossTimestampToString ( mondbcb->_activateTimestamp, timestamp ) ;
      ob.append ( FIELD_NAME_ACTIVETIMESTAMP, timestamp ) ;
      ossTimestampToString ( mondbcb->_resetTimestamp, timestamp ) ;
      ob.append ( FIELD_NAME_RESETTIMESTAMP, timestamp ) ;
      ossSnprintf( CPUTime, sizeof(CPUTime), "%u.%06u",
                   userTime.seconds, userTime.microsec ) ;
      ob.append( FIELD_NAME_USERCPU, CPUTime ) ;

      ossSnprintf( CPUTime, sizeof(CPUTime), "%u.%06u",
                    sysTime.seconds, sysTime.microsec ) ;
      ob.append( FIELD_NAME_SYSCPU, CPUTime ) ;
      PD_TRACE_EXITRC ( SDB_MONDBDUMP, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONSESSIONMONEDUFULL, "monSessionMonEDUFull" )
   INT32 monSessionMonEDUFull(  BSONObjBuilder &ob, const monEDUFull &full,
                                ossTickConversionFactor &factor,
                                ossTime userTime, ossTime sysTime  )
   {
      INT32 rc = SDB_OK ;
      UINT32 seconds, microseconds ;
      CHAR   timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;

      PD_TRACE_ENTRY ( SDB_MONSESSIONMONEDUFULL ) ;
      ob.append( FIELD_NAME_TOTALQUERY, (SINT64)full._monApplCB.totalGeneralQuery ) ;
      ob.append( FIELD_NAME_TOTALSLOWQUERY, (SINT64)full._monApplCB.totalGeneralSlowQuery ) ;
      ob.append( FIELD_NAME_TOTALTRANSCOMMIT, (SINT64)full._monApplCB.totalTransCommit ) ;
      ob.append( FIELD_NAME_TOTALTRANSROLLBACK, (SINT64)full._monApplCB.totalTransRollback ) ;
      ob.append( FIELD_NAME_TOTALDATAREAD, (SINT64)full._monApplCB.totalDataRead ) ;
      ob.append( FIELD_NAME_TOTALINDEXREAD, (SINT64)full._monApplCB.totalIndexRead ) ;
      ob.append( FIELD_NAME_TOTALDATAWRITE, (SINT64)full._monApplCB.totalDataWrite ) ;
      ob.append( FIELD_NAME_TOTALINDEXWRITE, (SINT64)full._monApplCB.totalIndexWrite ) ;
      ob.append( FIELD_NAME_TOTALUPDATE, (SINT64)full._monApplCB.totalUpdate ) ;
      ob.append( FIELD_NAME_TOTALDELETE, (SINT64)full._monApplCB.totalDelete ) ;
      ob.append( FIELD_NAME_TOTALINSERT, (SINT64)full._monApplCB.totalInsert ) ;
      ob.append( FIELD_NAME_TOTALSELECT, (SINT64)full._monApplCB.totalSelect ) ;
      ob.append( FIELD_NAME_TOTALREAD, (SINT64)full._monApplCB.totalRead ) ;
      ob.append( FIELD_NAME_TOTALLOBGET, (SINT64)full._monApplCB.totalLobGet ) ;
      ob.append( FIELD_NAME_TOTALLOBPUT, (SINT64)full._monApplCB.totalLobPut ) ;
      ob.append( FIELD_NAME_TOTALLOBDELETE, (SINT64)full._monApplCB.totalLobDelete ) ;
      ob.append( FIELD_NAME_TOTALLOBLIST, (SINT64)full._monApplCB.totalLobList ) ;
      ob.append( FIELD_NAME_TOTALLOBREADSIZE, (SINT64)full._monApplCB.totalLobReadSize ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITESIZE, (SINT64)full._monApplCB.totalLobWriteSize ) ;
      ob.append( FIELD_NAME_TOTALLOBREAD, (SINT64)full._monApplCB.totalLobRead ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITE, (SINT64)full._monApplCB.totalLobWrite ) ;
      ob.append( FIELD_NAME_TOTALLOBTRUNCATE, (SINT64)full._monApplCB.totalLobTruncate ) ;
      ob.append( FIELD_NAME_TOTALLOBADDRESSING, (SINT64)full._monApplCB.totalLobAddressing ) ;

      full._monApplCB.totalReadTime.convertToTime ( factor,
                                                    seconds,
                                                    microseconds ) ;
      ob.append( FIELD_NAME_TOTALREADTIME,
                 (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;

      full._monApplCB.totalWriteTime.convertToTime ( factor,
                                                     seconds,
                                                     microseconds ) ;
      ob.append( FIELD_NAME_TOTALWRITETIME,
                 (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;

      /// the spent time is last op
      full._monApplCB._readTimeSpent.convertToTime ( factor,
                                                     seconds,
                                                     microseconds ) ;
      ob.append( FIELD_NAME_READTIMESPENT,
                 (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;

      full._monApplCB._writeTimeSpent.convertToTime ( factor,
                                                      seconds,
                                                      microseconds ) ;
      ob.append( FIELD_NAME_WRITETIMESPENT,
                 (SINT64)(seconds*1000 + microseconds / 1000 ) ) ;

      ossTimestamp tmpTm = full._monApplCB._connectTimestamp ;
      ossTimestampToString( tmpTm, timestamp ) ;
      ob.append ( FIELD_NAME_CONNECTTIMESTAMP, timestamp ) ;
      tmpTm = full._monApplCB._resetTimestamp ;
      ossTimestampToString( tmpTm, timestamp ) ;
      ob.append ( FIELD_NAME_RESETTIMESTAMP, timestamp ) ;

      /// add last op info
      monDumpLastOpInfo( ob, full._monApplCB ) ;

      /// add cpu info
      double userCpu;
      userCpu = userTime.seconds + (double)userTime.microsec / 1000000 ;
      ob.append( FIELD_NAME_USERCPU, userCpu ) ;

      double sysCpu;
      sysCpu = sysTime.seconds + (double)sysTime.microsec / 1000000 ;
      ob.append( FIELD_NAME_SYSCPU, sysCpu ) ;

      PD_TRACE_EXITRC ( SDB_MONSESSIONMONEDUFULL, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDMSCOLLECTIONFLAGTOSTRING, "monDMSCollectionFlagToString" )
   const CHAR *monDMSCollectionFlagToString ( UINT16 flag )
   {
      PD_TRACE_ENTRY ( SDB_MONDMSCOLLECTIONFLAGTOSTRING ) ;
      PD_TRACE1 ( SDB_MONDMSCOLLECTIONFLAGTOSTRING, PD_PACK_USHORT(flag) ) ;
      const CHAR *str = NULL ;
      // free flag is 0x0000
      if ( DMS_IS_MB_FREE(flag) )
      {
         str = "Free" ;
         goto done ;
      }
      // normal flag is 0x0001
      if ( DMS_IS_MB_NORMAL(flag) )
      {
         str = "Normal" ;
         goto done ;
      }
      // drop flag is 0x0002
      if ( DMS_IS_MB_DROPPED(flag) )
      {
         str = "Dropped" ;
         goto done ;
      }
      // reorg
      if ( DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY(flag) )
      {
         str = "Offline Reorg Shadow Copy Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_TRUNCATE(flag) )
      {
         str = "Offline Reorg Truncate Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_COPY_BACK(flag) )
      {
         str = "Offline Reorg Copy Back Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_REBUILD(flag) )
      {
         str = "Offline Reorg Rebuild Phase" ;
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB_MONDMSCOLLECTIONFLAGTOSTRING ) ;
      return str ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRESETMON, "monResetMon" )
   INT32 monResetMon ( RTN_COMMAND_TYPE type,
                       BOOLEAN resetAllEDU,
                       EDUID eduID,
                       const CHAR * collectionSpace,
                       const CHAR * collection )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_MONRESETMON ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      pmdEDUMgr *mgr = krcb->getEDUMgr() ;
      pmdStartupHistoryLogger *startLogger = pmdGetStartupHstLogger() ;
      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;

      switch ( type )
      {
         case CMD_SNAPSHOT_ALL :
         {
            mondbcb->reset() ;
            mgr->resetMon() ;
            mgr->resetIOService() ;
            pmdResetErrNum () ;
            startLogger->clearAll() ;
            krcb->getSvcTaskMgr()->reset() ;
            sdbGetClsCB()->resetDumpSchedInfo() ;
            dmsCB->clearAllCRUDCB() ;
            break ;
         }
         case CMD_SNAPSHOT_DATABASE :
         {
            mondbcb->reset() ;
            mgr->resetIOService() ;
            sdbGetClsCB()->resetDumpSchedInfo() ;
            break ;
         }
         case CMD_SNAPSHOT_SESSIONS :
         {
            if ( resetAllEDU )
            {
               mgr->resetMon () ;
            }
            else
            {
               mgr->resetMon( eduID ) ;
            }
            break ;
         }
         case CMD_SNAPSHOT_SESSIONS_CURRENT :
         {
            pmdGetThreadEDUCB()->resetMon() ;
            break ;
         }
         case CMD_SNAPSHOT_HEALTH :
         {
            pmdResetErrNum () ;
            startLogger->clearAll() ;
            break ;
         }
         case CMD_SNAPSHOT_SVCTASKS :
         {
            krcb->getSvcTaskMgr()->reset() ;
            break ;
         }
         case CMD_SNAPSHOT_COLLECTIONS :
         {
            if ( NULL != collectionSpace )
            {
               rc = dmsCB->clearSUCRUDCB( collectionSpace ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to reset snapshot for "
                            "collection space [%s], rc: %d", collectionSpace,
                            rc ) ;
            }
            else if ( NULL != collection )
            {
               rc = dmsCB->clearMBCRUDCB( collection ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to reset snapshot for "
                            "collection [%s], rc: %d", collection, rc ) ;
            }
            else
            {
               dmsCB->clearAllCRUDCB() ;
            }
            break ;
         }
         default :
            break ;
      }

   done :
      PD_TRACE_EXITRC( SDB_MONRESETMON, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDUMPTRACESTATUS, "monDumpTraceStatus" )
   INT32 monDumpTraceStatus ( rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDUMPTRACESTATUS ) ;
      try
      {
         BSONObj obj ;
         BSONObjBuilder builder ;
         pdTraceCB *traceCB = sdbGetPDTraceCB() ;
         BOOLEAN traceStarted = traceCB->isStarted() ;
         builder.appendBool ( FIELD_NAME_TRACESTARTED, traceStarted ) ;
         if ( traceStarted )
         {
            builder.appendBool ( FIELD_NAME_WRAPPED,
                                 traceCB->isWrapped() ) ;
            builder.appendNumber ( FIELD_NAME_SIZE,
                                   (INT32)traceCB->getSize() ) ;
            builder.append( FIELD_NAME_FREE_SIZE,
                            (INT32)traceCB->getFreeSize() ) ;
#ifdef _DEBUG
            builder.append( "PadSize", (INT64)traceCB->getPadSize() ) ;
#endif // _DEBUG
            BSONArrayBuilder arr( builder.subarrayStart( FIELD_NAME_MASK ) ) ;
            for ( UINT32 i = 0; i < pdGetTraceComponentSize() ; ++i )
            {
               UINT32 mask = ((UINT32)1)<<i ;
               if ( mask & traceCB->getMask() )
               {
                  arr.append ( pdGetTraceComponent ( i ) ) ;
               }
            }
            arr.done() ;

            BSONArrayBuilder bpArr( builder.subarrayStart( FIELD_NAME_BREAKPOINTS ) ) ;
            const UINT64 *bpList = traceCB->getBPList() ;
            UINT32 bpNum = traceCB->getBPNum () ;
            for ( UINT32 i = 0; i < bpNum; ++i )
            {
               bpArr.append( pdGetTraceFunction( bpList[i] ) ) ;
            }
            bpArr.done() ;

            BSONArrayBuilder thdArr( builder.subarrayStart( FIELD_NAME_THREADS ) ) ;
            const UINT32 *thdList = traceCB->getThreadFilterList() ;
            UINT32 thdNum = traceCB->getThreadFilterNum() ;
            for ( UINT32 i = 0 ; i < thdNum ; ++i )
            {
               thdArr.append( thdList[ i ] ) ;
            }
            thdArr.done() ;

#if defined (SDB_ENGINE)
            pmdEPFactory &factory = pmdGetEPFactory() ;
            BSONArrayBuilder threadTypeArr( builder.subarrayStart
                                            ( FIELD_NAME_THREADTYPES ) ) ;
            const INT32 *threadTypeList = traceCB->getThreadType() ;
            INT32 threadTypeNum = traceCB->getThreadTypeNum() ;
            for( INT32 i = 0; i < threadTypeNum; ++i )
            {
               threadTypeArr.append( factory.type2Name( threadTypeList[ i ] ) ) ;
            }
            threadTypeArr.done() ;
#endif

            BSONArrayBuilder functionNameArr( builder.subarrayStart
                                             ( FIELD_NAME_FUNCTIONNAMES ) ) ;
            const UINT32 *functionNameList = traceCB->getFunctionName() ;
            UINT32 functionNameNum = traceCB->getFunctionNameNum() ;
            for( UINT32 i = 0; i < functionNameNum; ++i )
            {
               functionNameArr.append( pdGetTraceFunction
                                       ( functionNameList[ i ] ) ) ;
            }
            functionNameArr.done() ;
         }
         obj = builder.obj() ;
         rc = context->monAppend( obj ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to add obj to context, rc = %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR,
                      "Failed to create trace status dump: %s",
                      e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_MONDUMPTRACESTATUS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   #define MAX_DATABLOCK_A_RECORD_NUM  (500)
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDUMPDATABLOCKS, "monDumpDatablocks" )
   INT32 monDumpDatablocks( ossPoolVector<dmsExtentID> &datablocks,
                            rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      INT32 datablockNum = 0 ;
      PD_TRACE_ENTRY ( SDB_MONDUMPDATABLOCKS ) ;
      while ( datablocks.size() > 0 )
      {
         try
         {
            datablockNum = 0 ;
            BSONObjBuilder builder( MON_DUMP_DFT_BUILDER_SZ ) ;
            BSONArrayBuilder blockArrBd ;
            BSONObj obj ;

            // add node info
            rc = monAppendSystemInfo( builder, MON_MASK_HOSTNAME|
                                      MON_MASK_SERVICE_NAME|MON_MASK_NODEID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append system info, rc: %d",
                         rc ) ;

            builder.append( FIELD_NAME_SCANTYPE, VALUE_NAME_TBSCAN ) ;
            // add datablocks
            ossPoolVector<dmsExtentID>::iterator it = datablocks.begin() ;
            while ( it != datablocks.end() &&
                    datablockNum < MAX_DATABLOCK_A_RECORD_NUM )
            {
               blockArrBd.append( *it ) ;
               it = datablocks.erase( it ) ;
               ++datablockNum ;
            }

            builder.appendArray( FIELD_NAME_DATABLOCKS, blockArrBd.arr() ) ;
            obj = builder.obj() ;
            rc = context->monAppend( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Add to obj[%s] to context failed, "
                         "rc: %d", obj.toString().c_str(), rc ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_MONDUMPDATABLOCKS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   #define MAX_INDEXBLOCK_A_RECORD_NUM  (20)
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDUMPINDEXBLOCKS, "monDumpIndexblocks" )
   INT32 monDumpIndexblocks( std::vector< BSONObj > &idxBlocks,
                             std::vector< dmsRecordID > &idxRIDs,
                             const CHAR *indexName,
                             dmsExtentID indexLID,
                             INT32 direction,
                             rtnContextDump * context )
   {
      INT32 rc = SDB_OK ;
      INT32 indexblockNum = 0 ;
      UINT32 indexPos = 0 ;
      PD_TRACE_ENTRY ( SDB_MONDUMPINDEXBLOCKS ) ;
      SDB_ASSERT( idxBlocks.size() == idxRIDs.size(), "size not same" ) ;

      if ( 1 != direction )
      {
         indexPos = idxBlocks.size() - 1 ;
      }

      while ( ( 1 == direction && indexPos + 1 < idxBlocks.size() ) ||
              ( -1 == direction && indexPos > 0 ) )
      {
         try
         {
            indexblockNum = 0 ;
            BSONObjBuilder builder( MON_DUMP_DFT_BUILDER_SZ ) ;
            BSONArrayBuilder blockArrBd ;
            BSONObj obj ;

            // add node info
            rc = monAppendSystemInfo( builder, MON_MASK_HOSTNAME|
                                      MON_MASK_SERVICE_NAME|MON_MASK_NODEID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append system info, rc: %d",
                         rc ) ;

            builder.append( FIELD_NAME_SCANTYPE, VALUE_NAME_IXSCAN ) ;
            builder.append( FIELD_NAME_INDEXNAME, indexName ) ;
            builder.append( FIELD_NAME_INDEXLID, indexLID ) ;
            builder.append( FIELD_NAME_DIRECTION, direction ) ;
            // add indexblocks
            while ( ( ( 1 == direction && indexPos + 1 < idxBlocks.size() ) ||
                      ( -1 == direction && indexPos > 0 ) ) &&
                    indexblockNum < MAX_INDEXBLOCK_A_RECORD_NUM )
            {
               blockArrBd.append( BSON( FIELD_NAME_STARTKEY <<
                                        idxBlocks[indexPos] <<
                                        FIELD_NAME_ENDKEY <<
                                        idxBlocks[indexPos+direction] <<
                                        FIELD_NAME_STARTRID <<
                                        BSON_ARRAY( idxRIDs[indexPos]._extent <<
                                                    idxRIDs[indexPos]._offset ) <<
                                        FIELD_NAME_ENDRID <<
                                        BSON_ARRAY( idxRIDs[indexPos+direction]._extent <<
                                                    idxRIDs[indexPos+direction]._offset )
                                        )
                                  ) ;
               indexPos += direction ;
               ++indexblockNum ;
            }

            builder.appendArray( FIELD_NAME_INDEXBLOCKS, blockArrBd.arr() ) ;
            obj = builder.obj() ;
            rc = context->monAppend( obj ) ;
            PD_RC_CHECK( rc, PDERROR, "Add to obj[%s] to context failed, "
                         "rc: %d", obj.toString().c_str(), rc ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB_MONDUMPINDEXBLOCKS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMPSTORINFO, "monDBDumpStorageInfo" )
   INT32 monDBDumpStorageInfo( BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDBDUMPSTORINFO ) ;
      try
      {
         INT64 totalMapped = 0 ;
         pmdGetKRCB()->getDMSCB()->dumpInfo( totalMapped ) ;
         ob.append( FIELD_NAME_TOTALMAPPED, totalMapped ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONDBDUMPSTORINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMPPROCMEMINFO, "monDBDumpProcMemInfo" )
   INT32 monDBDumpProcMemInfo( BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDBDUMPPROCMEMINFO ) ;
      try
      {
         ossProcMemInfo memInfo ;
         rc = ossGetProcMemInfo( memInfo ) ;
         if ( SDB_OK == rc )
         {
            ob.append( FIELD_NAME_VSIZE, memInfo.vSize ) ;
            ob.append( FIELD_NAME_RSS, memInfo.rss ) ;
            ob.append( FIELD_NAME_MEM_SHARED, memInfo.shr ) ;
            ob.append( FIELD_NAME_FAULT, memInfo.fault ) ;
         }
         else
         {
            ob.append( FIELD_NAME_VSIZE, 0 ) ;
            ob.append( FIELD_NAME_RSS, 0 ) ;
            ob.append( FIELD_NAME_MEM_SHARED, 0 ) ;
            ob.append( FIELD_NAME_FAULT, 0 ) ;
            PD_RC_CHECK( rc, PDERROR,
                        "failed to dump memory info(rc=%d)",
                        rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONDBDUMPPROCMEMINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMPNETINFO, "monDBDumpNetInfo" )
   INT32 monDBDumpNetInfo( BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDBDUMPNETINFO ) ;
      try
      {
         pmdKRCB *pKrcb = pmdGetKRCB() ;
         monDBCB *pdbCB = pKrcb->getMonDBCB() ;
         SDB_ROLE role = pKrcb->getDBRole() ;
         ob.append( FIELD_NAME_SVC_NETIN, (INT64)pdbCB->svcNetIn() ) ;
         ob.append( FIELD_NAME_SVC_NETOUT, (INT64)pdbCB->svcNetOut() ) ;
         if ( SDB_ROLE_DATA == role ||
              SDB_ROLE_CATALOG == role )
         {
            shardCB *pShardCB = sdbGetShardCB() ;
            ob.append( FIELD_NAME_SHARD_NETIN, pShardCB->netIn() ) ;
            ob.append( FIELD_NAME_SHARD_NETOUT, pShardCB->netOut() ) ;

            replCB *pReplCB = sdbGetReplCB() ;
            ob.append( FIELD_NAME_REPL_NETIN, pReplCB->netIn() ) ;
            ob.append( FIELD_NAME_REPL_NETOUT, pReplCB->netOut() ) ;
         }
         else if ( SDB_ROLE_COORD == role )
         {
            netRouteAgent *shardAgent = sdbGetCoordCB()->getRouteAgent() ;
            if ( shardAgent )
            {
               ob.append( FIELD_NAME_SHARD_NETIN, shardAgent->netIn() ) ;
               ob.append( FIELD_NAME_SHARD_NETOUT, shardAgent->netOut() ) ;
            }
            else
            {
               ob.append( FIELD_NAME_SHARD_NETIN, 0 ) ;
               ob.append( FIELD_NAME_SHARD_NETOUT, 0 ) ;
            }

            ob.append( FIELD_NAME_REPL_NETIN, 0 ) ;
            ob.append( FIELD_NAME_REPL_NETOUT, 0 ) ;
         }
         else
         {
            ob.append( FIELD_NAME_SHARD_NETIN, 0 ) ;
            ob.append( FIELD_NAME_SHARD_NETOUT, 0 ) ;

            ob.append( FIELD_NAME_REPL_NETIN, 0 ) ;
            ob.append( FIELD_NAME_REPL_NETOUT, 0 ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONDBDUMPNETINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMPLOGINFO, "monDBDumpLogInfo" )
   INT32 monDBDumpLogInfo( BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDBDUMPLOGINFO ) ;
      try
      {
         ob.append( FIELD_NAME_FREELOGSPACE,
                  (INT64)(pmdGetKRCB()->getTransCB()->remainLogSpace()) );
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONDBDUMPLOGINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDBDUMPLASTOPINFO, "monDumpLastOpInfo" )
   INT32 monDumpLastOpInfo( BSONObjBuilder &ob, const monAppCB &moncb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_MONDBDUMPLASTOPINFO ) ;
      try
      {
         BOOLEAN isCommand = FALSE ;
         if ( MSG_BS_QUERY_REQ == moncb._lastOpType &&
              CMD_UNKNOW != moncb._cmdType )
         {
            isCommand = TRUE ;
         }
         ob.append( FIELD_NAME_LASTOPTYPE,
                    msgType2String( (MSG_TYPE)moncb._lastOpType, isCommand ) ) ;
         CHAR   timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
         if ( ( BOOLEAN )( moncb._lastOpBeginTime ) )
         {
            ossTimestamp Tm;
            moncb._lastOpBeginTime.convertToTimestamp( Tm ) ;
            ossTimestampToString( Tm, timestamp ) ;
         }
         else
         {
            ossStrcpy(timestamp, "--") ;
         }
         ob.append( FIELD_NAME_LASTOPBEGIN, timestamp ) ;

         if ( ( BOOLEAN )( moncb._lastOpEndTime ) )
         {
            ossTimestamp Tm;
            moncb._lastOpEndTime.convertToTimestamp( Tm ) ;
            ossTimestampToString( Tm, timestamp ) ;
         }
         else
         {
            ossStrcpy(timestamp, "--") ;
         }
         ob.append( FIELD_NAME_LASTOPEND, timestamp ) ;

         SDB_ASSERT( !moncb._lastOpMsgSaved, "should not save message" ) ;
         ob.append( FIELD_NAME_LASTOPINFO, moncb._lastOpDetail ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Ocurr exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_MONDBDUMPLASTOPINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void monDumpSvcTaskInfo( BSONObjBuilder &ob,
                            const monSvcTaskInfo *pInfo,
                            BOOLEAN exceptIDName )
   {
      CHAR   timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
      ossTimestamp tmpTime ;

      if ( !exceptIDName )
      {
         ob.append( FIELD_NAME_TASK_ID, (INT64)pInfo->getTaskID() ) ;
         ob.append( FIELD_NAME_TASK_NAME, pInfo->getTaskName() ) ;
      }
      ob.append( FIELD_NAME_TOTALTIME, (INT64)( pInfo->_totalTime / 1000 ) ) ;
      ob.append( FIELD_NAME_TOTALCONTEXTS, (INT64)pInfo->_totalContexts ) ;
      ob.append( FIELD_NAME_TOTALDATAREAD, (INT64)pInfo->_totalDataRead ) ;
      ob.append( FIELD_NAME_TOTALINDEXREAD, (INT64)pInfo->_totalIndexRead ) ;
      ob.append( FIELD_NAME_TOTALDATAWRITE, (INT64)pInfo->_totalDataWrite ) ;
      ob.append( FIELD_NAME_TOTALINDEXWRITE, (INT64)pInfo->_totalIndexWrite ) ;
      ob.append( FIELD_NAME_TOTALUPDATE, (INT64)pInfo->_totalUpdate ) ;
      ob.append( FIELD_NAME_TOTALDELETE, (INT64)pInfo->_totalDelete ) ;
      ob.append( FIELD_NAME_TOTALINSERT, (INT64)pInfo->_totalInsert ) ;
      ob.append( FIELD_NAME_TOTALSELECT, (INT64)pInfo->_totalSelect ) ;
      ob.append( FIELD_NAME_TOTALREAD, (INT64)pInfo->_totalRead ) ;
      ob.append( FIELD_NAME_TOTALWRITE, (INT64)pInfo->_totalWrite ) ;

      ob.append( FIELD_NAME_TOTALLOBGET, (INT64)pInfo->_totalLobGet ) ;
      ob.append( FIELD_NAME_TOTALLOBPUT, (INT64)pInfo->_totalLobPut ) ;
      ob.append( FIELD_NAME_TOTALLOBDELETE, (INT64)pInfo->_totalLobDelete ) ;
      ob.append( FIELD_NAME_TOTALLOBLIST, (INT64)pInfo->_totalLobList ) ;
      ob.append( FIELD_NAME_TOTALLOBREADSIZE, (INT64)pInfo->_totalLobReadSize ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITESIZE, (INT64)pInfo->_totalLobWriteSize ) ;
      ob.append( FIELD_NAME_TOTALLOBREAD, (INT64)pInfo->_totalLobRead ) ;
      ob.append( FIELD_NAME_TOTALLOBWRITE, (INT64)pInfo->_totalLobWrite ) ;
      ob.append( FIELD_NAME_TOTALLOBTRUNCATE, (INT64)pInfo->_totalLobTruncate ) ;
      ob.append( FIELD_NAME_TOTALLOBADDRESSING, (INT64)pInfo->_totalLobAddressing ) ;

      tmpTime = pInfo->_startTimestamp ;
      ossTimestampToString( tmpTime, timestamp ) ;
      ob.append ( FIELD_NAME_STARTTIMESTAMP, timestamp ) ;
      tmpTime = pInfo->_resetTimestamp ;
      ossTimestampToString( tmpTime, timestamp ) ;
      ob.append ( FIELD_NAME_RESETTIMESTAMP, timestamp ) ;
   }

   INT32 monParseArchiveOpt( const BSONObj &obj, BOOLEAN &archiveOpt )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjIterator itr( obj ) ;
         BSONElement elem ;
         while ( itr.more() )
         {
            elem = itr.next() ;
            // ignore case for backward compatibility
            if ( 0 == ossStrcasecmp( elem.fieldName(), FIELD_NAME_VIEW_HISTORY ) )
            {
               if ( elem.type() == bson::Bool )
               {
                  archiveOpt = elem.boolean() ;
               }
               else if ( elem.type() == bson::String )
               {
                  ossStrToBoolean( elem.valuestr(), &archiveOpt ) ;
               }
               break ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to read configs options ",
                  e.what() ) ;
         rc = SDB_SYS ;
      }

      return rc ;
   }

   INT32 _monDetailObj2InfoV2( const BSONObj &obj, detailedInfo &info )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjIterator iter( obj ) ;

         // NodeName
         BSONElement ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_NODE_NAME ),
                     "Unexpected field here" ) ;

         // GroupName
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_GROUPNAME ),
                     "Unexpected field here" ) ;

         // InternalV
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INTERNAL_VERSION ),
                     "Unexcepted field here" ) ;
         SDB_ASSERT( MON_CL_DETAIL_CURRENT_V == ele.numberInt(), "Wrong protocal version" ) ;

         // ID
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ID ),
                     "Unexpected field here" ) ;
         info._blockID = (UINT16) ele.Int() ;

         // LogicalID
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOGICAL_ID ),
                     "Unexpected field here" ) ;
         info._logicID = ele.Int() ;

         // Sequence
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_SEQUENCE ),
                     "Unexpected field here" ) ;

         // Indexes
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_INDEXES ),
                     "Unexpected field here" ) ;
         info._numIndexes = ele.Int() ;

         // Status
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_STATUS ),
                     "Unexpected field here" ) ;

         // Attributes
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ATTRIBUTE ),
                     "Unexpected field here" ) ;

         // CompressionType
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_COMPRESSIONTYPE ),
                     "Unexpected field here" ) ;

         // DictionaryCreated
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DICT_CREATED ),
                     "Unexpected field here" ) ;
         info._dictCreated = ele.Bool() ;

         // DictionaryVersion
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DICT_VERSION ),
                     "Unexpected field here" ) ;

         // PageSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_PAGE_SIZE ),
                     "Unexpected field here" ) ;
         info._pageSize = ele.Int() ;

         // LobPageSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOB_PAGE_SIZE ),
                     "Unexpected field here" ) ;
         info._lobPageSize = ele.Int() ;

         // TotalRecords
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_RECORDS ),
                     "Unexpected field here" ) ;
         info._totalRecords = ele.Long() ;

         // TotalLobs
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_LOBS ),
                     "Unexpected field here" ) ;
         info._totalLobs = ele.Long() ;

         // TotalDataPages
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_DATA_PAGES ),
                     "Unexpected field here" ) ;
         info._totalDataPages = ele.Int() ;

         // TotalIndexPages
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_INDEX_PAGES ),
                     "Unexpected field here" ) ;
         info._totalIndexPages = ele.Int() ;

         // TotalLobPages
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_LOB_PAGES ),
                     "Unexpected field here" ) ;
         info._totalLobPages = ele.Int() ;

         // TotalUsedLobSpace
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_USED_LOB_SPACE ),
                     "Unexcepted field here" ) ;
         info._totalUsedLobSpace = ele.Long() ;

         // UsedLobSpaceRatio
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_USED_LOB_SPACE_RATIO ),
                     "Unexcepted field here" ) ;
         info._usedLobSpaceRatio = ele.Double() ;

         // TotalLobSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_LOB_SIZE ),
                     "Unexcepted field here" ) ;
         info._totalLobSize = ele.Long() ;

         // TotalValidLobSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_VALID_LOB_SIZE ),
                     "Unexcepted field here" ) ;
         info._totalValidLobSize = ele.Long() ;

         // LobUsageRate
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOB_USAGE_RATE ),
                     "Unexcepted field here" ) ;
         info._lobUsageRate = ele.Double() ;

         // AvgLobSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_AVG_LOB_SIZE ),
                     "Unexcepted field here" ) ;
         info._avgLobSize = ele.Long() ;

         // TotalDataFreeSpace
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_DATA_FREESPACE ),
                     "Unexpected field here" ) ;
         info._totalDataFreeSpace = ele.Long() ;

         // TotalIndexFreeSpace
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_INDEX_FREESPACE ),
                     "Unexpected field here" ) ;
         info._totalIndexFreeSpace = ele.Long() ;

         // CurrentCompressionRatio
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_CURR_COMPRESS_RATIO ),
                     "Unexpected field here" ) ;
         info._currCompressRatio = (UINT32)(ele.Double() * 100.0) ;

         // DataCommitLSN
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DATA_COMMIT_LSN ),
                     "Unexpected field here" ) ;
         info._dataCommitLSN = ele.Long() ;

         // IndexCommitLSN
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_IDX_COMMIT_LSN ),
                     "Unexpected field here" ) ;
         info._idxCommitLSN = ele.Long() ;

         // LobCommitLSN
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOB_COMMIT_LSN ),
                     "Unexpected field here" ) ;
         info._lobCommitLSN = ele.Long() ;

         // DataCommitted
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DATA_COMMITTED ),
                     "Unexpected field here" ) ;
         info._dataIsValid = ele.Bool() ;

         // IndexCommitted
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_IDX_COMMITTED ),
                     "Unexpected field here" ) ;
         info._idxIsValid = ele.Bool() ;

         // LobCommitted
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_LOB_COMMITTED ),
                     "Unexpected field here" ) ;
         info._lobIsValid = ele.Bool() ;

         // TotalDataRead
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALDATAREAD ),
                     "Unexpected field here" ) ;
         info._crudCB._totalDataRead = ele.Long() ;

         // TotalIndexRead
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALINDEXREAD ),
                     "Unexpected field here" ) ;
         info._crudCB._totalIndexRead = ele.Long() ;

         // TotalDataWrite
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALDATAWRITE ),
                     "Unexpected field here" ) ;
         info._crudCB._totalDataWrite = ele.Long() ;

         // TotalIndexWrite
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALINDEXWRITE ),
                     "Unexpected field here" ) ;
         info._crudCB._totalIndexWrite = ele.Long() ;

         // TotalUpdate
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALUPDATE ),
                     "Unexpected field here" ) ;
         info._crudCB._totalUpdate = ele.Long() ;

         // TotalDelete
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALDELETE ),
                     "Unexpected field here" ) ;
         info._crudCB._totalDelete = ele.Long() ;

         // TotalInsert
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALINSERT ),
                     "Unexpected field here" ) ;
         info._crudCB._totalInsert = ele.Long() ;

         // TotalSelect
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALSELECT ),
                     "Unexpected field here" ) ;
         info._crudCB._totalSelect = ele.Long() ;

         // TotalRead
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALREAD ),
                     "Unexpected field here" ) ;
         info._crudCB._totalRead = ele.Long() ;

         // TotalWrite
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALWRITE ),
                     "Unexpected field here" ) ;
         info._crudCB._totalWrite = ele.Long() ;

         // TotalTbScan
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALTBSCAN ),
                     "Unexpected field here" ) ;
         info._crudCB._totalTbScan = ele.Long() ;

         // TotalIxScan
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALIXSCAN ),
                     "Unexpected field here" ) ;
         info._crudCB._totalIxScan = ele.Long() ;

         // TotalLobGet
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBGET ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobGet = ele.Long() ;

         // TotalLobPut
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBPUT ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobPut = ele.Long() ;

         // TotalLobDelete
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBDELETE ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobDelete = ele.Long() ;

         // TotalLobList
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBLIST ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobList = ele.Long() ;

         // TotalLobReadSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBREADSIZE ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobReadSize = ele.Long() ;

         // TotalLobWriteSize
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBWRITESIZE ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobWriteSize = ele.Long() ;

         // TotalLobRead
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBREAD ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobRead = ele.Long() ;

         // TotalLobWrite
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBWRITE ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobWrite = ele.Long() ;

         // TotalLobTuncate
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBTRUNCATE ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobTruncate = ele.Long() ;

         // TotalLobAddressing
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_TOTALLOBADDRESSING ),
                     "Unexcepted field here" ) ;
         info._crudCB._totalLobAddressing = ele.Long() ;

         // ResetTimestamp
         ele = iter.next() ;
         SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_RESETTIMESTAMP ),
                     "Unexpected field here" ) ;

         if ( iter.more() )
         {
            // CreateTime
            ele = iter.next() ;
            SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_CREATE_TIME ),
                        "Unexpected field here" ) ;
            info._createTime = ossStringToMilliseconds( ele.valuestrsafe() ) ;

            // UpdateTime
            ele = iter.next() ;
            SDB_ASSERT( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_UPDATE_TIME ),
                        "Unexpected field here" ) ;
            info._updateTime = ossStringToMilliseconds( ele.valuestrsafe() ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse detail obj, exception: %s", e.what() ) ;
      }
      return rc ;
   }

   INT32 _monDetailObj2Info( const BSONObj &obj, detailedInfo &info )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      try
      {
         BSONObjIterator iter( obj ) ;
         while ( iter.more() )
         {
            ele = iter.next() ;
            if (0 == ossStrcmp( ele.fieldName(),
                                FIELD_NAME_PAGE_SIZE ))
            {
               info._pageSize = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_LOB_PAGE_SIZE ))
            {
               info._lobPageSize = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_RECORDS ))
            {
               info._totalRecords = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_LOBS ))
            {
               info._totalLobs = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_DATA_PAGES ))
            {
               info._totalDataPages = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_INDEX_PAGES ))
            {
               info._totalIndexPages = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_LOB_PAGES ))
            {
               info._totalLobPages = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_USED_LOB_SPACE ))
            {
               info._totalUsedLobSpace = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_USED_LOB_SPACE_RATIO ))
            {
               info._usedLobSpaceRatio = ele.Double() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_LOB_SIZE ))
            {
               info._totalLobSize = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_VALID_LOB_SIZE ))
            {
               info._totalValidLobSize = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_LOB_USAGE_RATE ))
            {
               info._lobUsageRate = ele.Double() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_AVG_LOB_SIZE ))
            {
               info._avgLobSize = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_DATA_FREESPACE ))
            {
               info._totalDataFreeSpace = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTAL_INDEX_FREESPACE ))
            {
               info._totalIndexFreeSpace = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALDATAREAD ))
            {
               info._crudCB._totalDataRead = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALINDEXREAD ))
            {
               info._crudCB._totalIndexRead = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALDATAWRITE ))
            {
               info._crudCB._totalDataWrite = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALINDEXWRITE ))
            {
               info._crudCB._totalIndexWrite = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALUPDATE ))
            {
               info._crudCB._totalUpdate = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALDELETE ))
            {
               info._crudCB._totalDelete = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALINSERT ))
            {
               info._crudCB._totalInsert = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALSELECT ))
            {
               info._crudCB._totalSelect = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALREAD ))
            {
               info._crudCB._totalRead = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALWRITE ))
            {
               info._crudCB._totalWrite = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALTBSCAN ))
            {
               info._crudCB._totalTbScan = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALIXSCAN ))
            {
               info._crudCB._totalIxScan = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBGET ))
            {
               info._crudCB._totalLobGet = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBPUT ))
            {
               info._crudCB._totalLobPut = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBDELETE ))
            {
               info._crudCB._totalLobDelete = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBLIST ))
            {
               info._crudCB._totalLobList = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBREADSIZE ))
            {
               info._crudCB._totalLobReadSize = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBWRITESIZE ))
            {
               info._crudCB._totalLobWriteSize = ele.Long() ;
            }

            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBREAD ))
            {
               info._crudCB._totalLobRead = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBWRITE ))
            {
               info._crudCB._totalLobWrite = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBTRUNCATE ))
            {
               info._crudCB._totalLobTruncate = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_TOTALLOBADDRESSING ))
            {
               info._crudCB._totalLobAddressing = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_INDEXES ))
            {
               info._numIndexes = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_DATA_COMMITTED ))
            {
               info._dataIsValid = ele.Bool() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_IDX_COMMITTED ))
            {
               info._idxIsValid = ele.Bool() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_LOB_COMMITTED ))
            {
               info._lobIsValid = ele.Bool() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_DICT_CREATED ))
            {
               info._dictCreated = ele.Bool() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_CURR_COMPRESS_RATIO ))
            {
               info._currCompressRatio = (UINT32)(ele.Double() * 100.0) ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_ID ))
            {
               info._blockID = (UINT16) ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_LOGICAL_ID ))
            {
               info._logicID = ele.Int() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_DATA_COMMIT_LSN ))
            {
               info._dataCommitLSN = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_IDX_COMMIT_LSN ))
            {
               info._idxCommitLSN = ele.Long() ;
            }
            else if (0 == ossStrcmp( ele.fieldName(),
                                     FIELD_NAME_LOB_COMMIT_LSN ))
            {
               info._lobCommitLSN = ele.Long() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_CREATE_TIME ) )
            {
               info._createTime = ossStringToMilliseconds( ele.valuestrsafe() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_UPDATE_TIME ) )
            {
               info._updateTime = ossStringToMilliseconds( ele.valuestrsafe() ) ;
            }
            // ignore _flag _attribute _dictVersion _compressType _maxGlobTransID
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse detail bson object. "
                 "Field[%s] has a wrong type. Detail: %s",
                 ele.fieldName(), e.what() ) ;
      }
      return rc ;
   }

   INT32 monDetailObj2Info( const BSONObj &obj, detailedInfo &info )
   {
      /*
         Generally, we parse BSON object by searching the field one by one.
         However, as the fields order is always fixed, using strcmp to
         search a field is unnecessary and with high cost. Therefore, we use
         the internal version to mark the fields order. Once the version is
         ensured, we just sequential traverse the BSON without any strcmp.
      */
      INT32 rc = SDB_OK ;
      INT32 internal_version = MON_CL_DETAIL_VERSION_NULL ;
      try
      {
         BSONElement ele = obj.getField( FIELD_NAME_INTERNAL_VERSION ) ;
         if ( ele.isNumber() )
         {
            internal_version = ele.numberInt() ;
         }
      }
      catch ( std::exception &e )
      {
         internal_version = MON_CL_DETAIL_VERSION_NULL ;
      }

      if ( MON_CL_DETAIL_VERSION_V2 == internal_version )
      {
         rc = _monDetailObj2InfoV2( obj, info ) ;
      }
      else
      {
         rc = _monDetailObj2Info( obj, info ) ;
      }
      return rc ;
   }


   INT32 monDetailInfo2Obj( const detailedInfo &info,
                            INT32 sequence,
                            BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      UINT16 flag = info._flag ;
      CHAR tmp[ MON_TMP_STR_SZ + 1 ] = { 0 } ;
      CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      try
      {
         ob.append ( FIELD_NAME_INTERNAL_VERSION, MON_CL_DETAIL_CURRENT_V ) ;
         ob.append ( FIELD_NAME_ID, info._blockID ) ;
         ob.append ( FIELD_NAME_LOGICAL_ID, info._logicID ) ;
         ob.append ( FIELD_NAME_SEQUENCE, sequence ) ;
         ob.append ( FIELD_NAME_INDEXES, info._numIndexes ) ;
         ob.append ( FIELD_NAME_STATUS, monDMSCollectionFlagToString ( flag ) ) ;
         mbAttr2String( info._attribute, tmp, MON_TMP_STR_SZ ) ;
         ob.append ( FIELD_NAME_ATTRIBUTE, tmp ) ;
         if ( OSS_BIT_TEST( info._attribute, DMS_MB_ATTR_COMPRESSED ) )
         {
            ob.append ( FIELD_NAME_COMPRESSIONTYPE,
                        utilCompressType2String( info._compressType ) ) ;
         }
         else
         {
            ob.append ( FIELD_NAME_COMPRESSIONTYPE, "" ) ;
         }
         ob.appendBool( FIELD_NAME_DICT_CREATED, info._dictCreated ) ;
         ob.append( FIELD_NAME_DICT_VERSION, info._dictVersion ) ;
         ob.append ( FIELD_NAME_PAGE_SIZE, info._pageSize ) ;
         ob.append ( FIELD_NAME_LOB_PAGE_SIZE, info._lobPageSize ) ;

         /// stat info
         ob.append ( FIELD_NAME_TOTAL_RECORDS,
                     (long long)(info._totalRecords )) ;
         ob.append ( FIELD_NAME_TOTAL_LOBS,
                     (long long)(info._totalLobs) ) ;
         ob.append ( FIELD_NAME_TOTAL_DATA_PAGES,
                     info._totalDataPages ) ;
         ob.append ( FIELD_NAME_TOTAL_INDEX_PAGES,
                     info._totalIndexPages ) ;
         ob.append ( FIELD_NAME_TOTAL_LOB_PAGES,
                     info._totalLobPages ) ;
         ob.append ( FIELD_NAME_TOTAL_USED_LOB_SPACE,
                     (INT64)(info._totalUsedLobSpace) ) ;
         ob.append ( FIELD_NAME_USED_LOB_SPACE_RATIO, info._usedLobSpaceRatio ) ;
         ob.append ( FIELD_NAME_TOTAL_LOB_SIZE, (INT64)(info._totalLobSize) ) ;
         ob.append ( FIELD_NAME_TOTAL_VALID_LOB_SIZE, (INT64)(info._totalValidLobSize) ) ;
         ob.append ( FIELD_NAME_LOB_USAGE_RATE, info._lobUsageRate ) ;
         ob.append ( FIELD_NAME_AVG_LOB_SIZE, (INT64)info._avgLobSize ) ;
         ob.append ( FIELD_NAME_TOTAL_DATA_FREESPACE,
                     (long long)(info._totalDataFreeSpace) ) ;
         ob.append ( FIELD_NAME_TOTAL_INDEX_FREESPACE,
                    (long long)(info._totalIndexFreeSpace) ) ;
         ob.append ( FIELD_NAME_CURR_COMPRESS_RATIO,
                     (FLOAT64)info._currCompressRatio / 100.0 ) ;

         /// sync info
         ob.append ( FIELD_NAME_DATA_COMMIT_LSN, (INT64)info._dataCommitLSN ) ;
         ob.append ( FIELD_NAME_IDX_COMMIT_LSN, (INT64)info._idxCommitLSN ) ;
         ob.append ( FIELD_NAME_LOB_COMMIT_LSN, (INT64)info._lobCommitLSN ) ;
         ob.appendBool ( FIELD_NAME_DATA_COMMITTED, info._dataIsValid ) ;
         ob.appendBool ( FIELD_NAME_IDX_COMMITTED, info._idxIsValid ) ;
         ob.appendBool ( FIELD_NAME_LOB_COMMITTED, info._lobIsValid ) ;
         // TODO: enble it after enable
         //ob.append ( FIELD_NAME_MAX_GTID, (INT64)info._maxGlobTransID ) ;

         /// CRUD statistics
         ob.append( FIELD_NAME_TOTALDATAREAD,
                    (INT64)info._crudCB._totalDataRead ) ;
         ob.append( FIELD_NAME_TOTALINDEXREAD,
                    (INT64)info._crudCB._totalIndexRead ) ;
         ob.append( FIELD_NAME_TOTALDATAWRITE,
                    (INT64)info._crudCB._totalDataWrite ) ;
         ob.append( FIELD_NAME_TOTALINDEXWRITE,
                     (INT64)info._crudCB._totalIndexWrite ) ;
         ob.append( FIELD_NAME_TOTALUPDATE,
                    (INT64)info._crudCB._totalUpdate ) ;
         ob.append( FIELD_NAME_TOTALDELETE,
                    (INT64)info._crudCB._totalDelete ) ;
         ob.append( FIELD_NAME_TOTALINSERT,
                    (INT64)info._crudCB._totalInsert ) ;
         ob.append( FIELD_NAME_TOTALSELECT,
                    (INT64)info._crudCB._totalSelect ) ;
         ob.append( FIELD_NAME_TOTALREAD,
                    (INT64)info._crudCB._totalRead ) ;
         ob.append( FIELD_NAME_TOTALWRITE,
                    (INT64)info._crudCB._totalWrite ) ;
         ob.append( FIELD_NAME_TOTALTBSCAN,
                    (INT64)info._crudCB._totalTbScan ) ;
         ob.append( FIELD_NAME_TOTALIXSCAN,
                    (INT64)info._crudCB._totalIxScan ) ;
         ob.append( FIELD_NAME_TOTALLOBGET,
                    (INT64)info._crudCB._totalLobGet ) ;
         ob.append( FIELD_NAME_TOTALLOBPUT,
                    (INT64)info._crudCB._totalLobPut ) ;
         ob.append( FIELD_NAME_TOTALLOBDELETE,
                    (INT64)info._crudCB._totalLobDelete ) ;
         ob.append( FIELD_NAME_TOTALLOBLIST,
                    (INT64)info._crudCB._totalLobList ) ;
         ob.append( FIELD_NAME_TOTALLOBREADSIZE,
                    (INT64)info._crudCB._totalLobReadSize ) ;
         ob.append( FIELD_NAME_TOTALLOBWRITESIZE,
                    (INT64)info._crudCB._totalLobWriteSize ) ;
         ob.append( FIELD_NAME_TOTALLOBREAD,
                    (INT64)info._crudCB._totalLobRead ) ;
         ob.append( FIELD_NAME_TOTALLOBWRITE,
                    (INT64)info._crudCB._totalLobWrite ) ;
         ob.append( FIELD_NAME_TOTALLOBTRUNCATE,
                    (INT64)info._crudCB._totalLobTruncate ) ;
         ob.append( FIELD_NAME_TOTALLOBADDRESSING,
                    (INT64)info._crudCB._totalLobAddressing ) ;
         ossTimestamp resetTimestamp =  info._crudCB._resetTimestamp ;
         ossTimestampToString( resetTimestamp, timestamp ) ;
         ob.append( FIELD_NAME_RESETTIMESTAMP, timestamp ) ;

         ossMillisecondsToString( info._createTime, timestamp ) ;
         ob.append( FIELD_NAME_CREATE_TIME, timestamp ) ;

         ossMillisecondsToString( info._updateTime, timestamp ) ;
         ob.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to build BSON object" ) ;
      }
      return rc ;
   }

   INT32 monCollection2Obj ( const monCollection &full, UINT32 addInfoMask,
                             BSONObjBuilder &ob )
   {
      INT32 rc = SDB_OK ;
      try
      {
         MON_CL_DETAIL_MAP::const_iterator itDetail ;
         /// add name & space name
         ob.append ( FIELD_NAME_NAME, full._name ) ;
         ob.append ( FIELD_NAME_UNIQUEID, (INT64)full._clUniqueID ) ;
         const CHAR *pDot = ossStrchr( full._name, '.' ) ;
         if ( pDot )
         {
            ob.appendStrWithNoTerminating ( FIELD_NAME_COLLECTIONSPACE,
                                            full._name,
                                            pDot - full._name ) ;
         }
         /// add detial
         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_DETAILS ) ) ;
         for ( itDetail = full._details.begin() ;
               itDetail != full._details.end() ;
               ++itDetail )
         {
            const detailedInfo &detail = itDetail->second ;
            BSONObjBuilder sub( ba.subobjStart() ) ;

            /// add system info
            monAppendSystemInfo( sub, addInfoMask ) ;
            rc = monDetailInfo2Obj( detail, itDetail->first, sub ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to convert detail info to BSON, rc: %d", rc ) ;
            sub.done() ;
         }
         ba.done() ;
      }
      catch ( std::bad_alloc &e )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to build BSON object" ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexcepted exception occurred: %s", e.what() ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 monCollectionStatInfo2Obj( dmsCollectionStat *collectionStat,
                                    const CHAR *clFullName,
                                    BOOLEAN isDefault,
                                    BOOLEAN isExpired,
                                    BSONObj &infoObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      CHAR timeStampStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = {};

      infoObj = BSONObj() ;
      if ( NULL == collectionStat ||
           NULL == clFullName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         UINT64 timestamp = collectionStat->getCreateTime() ;
         ossTimestamp tm( timestamp ) ;
         ossTimestampToString( tm, timeStampStr ) ;
         builder.append( FIELD_NAME_COLLECTION, clFullName ) ;
         builder.append( FIELD_NAME_INTERNAL_VERSION, MON_CL_STAT_CURRENT_V ) ;
         builder.append( FIELD_NAME_STAT_TIMESTAMP, timeStampStr )  ;
         builder.appendBool( FIELD_NAME_IS_DEFAULT, isDefault ) ;
         builder.appendBool( FIELD_NAME_IS_EXPIRED, isExpired ) ;
         builder.append( FIELD_NAME_AVGNUM_FIELDS, collectionStat->getAvgNumFields()  ) ;
         builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)collectionStat->getSampleRecords() ) ;
         builder.append( FIELD_NAME_TOTAL_RECORDS,  (INT64)collectionStat->getTotalRecords() ) ;
         builder.append( FIELD_NAME_TOTAL_DATA_PAGES, (INT64)collectionStat->getTotalDataPages() ) ;
         builder.append( FIELD_NAME_TOTAL_DATA_SIZE, (INT64)collectionStat->getTotalDataSize() ) ;

         infoObj = builder.obj() ;

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "append bson object failed, rc:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 monCollectionStatInfo2Obj( const collectionStatInfo &collectionStat,
                                    BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         builder.reset() ;
         if ( !collectionStat.inited() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         builder.append( FIELD_NAME_COLLECTION, collectionStat._collection ) ;
         builder.append ( FIELD_NAME_INTERNAL_VERSION, MON_CL_STAT_CURRENT_V ) ;
         builder.append( FIELD_NAME_STAT_TIMESTAMP, collectionStat._statTimestamp )  ;
         builder.appendBool( FIELD_NAME_IS_DEFAULT, collectionStat._isDefault ) ;
         builder.appendBool( FIELD_NAME_IS_EXPIRED, collectionStat._isExpired ) ;
         builder.append( FIELD_NAME_AVGNUM_FIELDS, (INT64)collectionStat._avgNumFields  ) ;
         builder.append( FIELD_NAME_SAMPLE_RECORDS, (INT64)collectionStat._sampleRecords ) ;
         builder.append( FIELD_NAME_TOTAL_RECORDS, (INT64)collectionStat._totalRecords ) ;
         builder.append( FIELD_NAME_TOTAL_DATA_PAGES, (INT64)collectionStat._totalDataPages ) ;
         builder.append( FIELD_NAME_TOTAL_DATA_SIZE, (INT64)collectionStat._totalDataSize ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build a collectionStat BSON object" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCheckCollectionStatObj( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( !obj.isEmpty(), "can not be empty" ) ;
      try
      {
         BSONObjIterator iter( obj ) ;

         // collection
         BSONElement ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_COLLECTION) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // internal version
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_INTERNAL_VERSION) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // statTimestamp
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_STAT_TIMESTAMP ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // isDefault
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_IS_DEFAULT ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // isExpired
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_IS_EXPIRED ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // avgNumFields
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_AVGNUM_FIELDS ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // sampleRecords
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_SAMPLE_RECORDS ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // totalRecords
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_RECORDS ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // totalDataPages
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_DATA_PAGES ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

         // totalDataSize
         ele = iter.next() ;
         if ( 0 != ossStrcmp( ele.fieldName(), FIELD_NAME_TOTAL_DATA_SIZE ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid field name[%s]", ele.fieldName() ) ;
            goto error ;
         }

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse stat obj, exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCollectionStatObj2InfoV1( const BSONObj &obj, collectionStatInfo &statInfo )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( !obj.isEmpty(), "can not be empty" ) ;
      statInfo.reset() ;

      #if defined( _DEBUG )
         rc = _monCheckCollectionStatObj( obj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Invalid collection stat bson object, rc:%d", rc ) ;
            goto error ;
         }
      #endif

      try
      {
         BSONObjIterator iter( obj ) ;
         // collection
         BSONElement ele = iter.next() ;
         ossStrncpy( statInfo._collection, ele.valuestr(), sizeof( statInfo._collection ) - 1 ) ;

         // internal version
         ele = iter.next() ;
         SDB_ASSERT( MON_CL_STAT_VERSION_V1 == ele.numberInt(), "invalid internal version" ) ;

         // statTimestamp
         ele = iter.next() ;
         ossStrncpy( statInfo._statTimestamp, ele.valuestr(),
                     sizeof( statInfo._statTimestamp ) - 1 ) ;

         // isDefault
         ele = iter.next() ;
         statInfo._isDefault = ele.boolean() ;

         // isExpired
         ele = iter.next() ;
         statInfo._isExpired = ele.boolean() ;

         // avgNumFields
         ele = iter.next() ;
         statInfo._avgNumFields = ele.numberLong() ;

         // sampleRecords
         ele = iter.next() ;
         statInfo._sampleRecords = ele.numberLong() ;

         // totalRecords
         ele = iter.next() ;
         statInfo._totalRecords = ele.numberLong() ;

         // totalDataPages
         ele = iter.next() ;
         statInfo._totalDataPages = ele.numberLong() ;

         // totalDataSize
         ele = iter.next() ;
         statInfo._totalDataSize = ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to parse stat obj, exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      statInfo.reset() ;
      goto done ;
   }

   INT32 monCollectionStatObj2Info( const BSONObj &obj, collectionStatInfo &statInfo )
   {
      INT32 rc = SDB_OK ;
      INT32 internalVersion = MON_CL_STAT_VERSION_NULL ;

      statInfo.reset() ;
      if ( obj.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONElement ele = obj.getField( FIELD_NAME_INTERNAL_VERSION ) ;
         if ( ele.isNumber() )
         {
            internalVersion = ele.numberInt() ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Get cl stat internal version failed, exception: %s", e.what() ) ;
         goto error ;
      }

      if ( MON_CL_STAT_VERSION_NULL == internalVersion )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid collection stat internal version" ) ;
         goto error ;
      }
      else if ( MON_CL_STAT_VERSION_V1 == internalVersion )
      {
         rc = _monCollectionStatObj2InfoV1( obj, statInfo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Parse cl stat bson obj to info failed, rc:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      statInfo.reset() ;
      goto done ;
   }

   INT32 monBuildIndexStatResult( BSONObj &stat, UINT32 addInfoMask,
                             BSONObjBuilder &ob, BOOLEAN detail )
   {
      // Modify the following places to the original record:
      // 1. Append system info( like "NodeName"... )
      // 2. Show features of MCV by fields:
      // "DistinctValNum", "MaxValue", "MinValue", "NullFrac", "UndefFrac".
      // 3. Rename some fields for interface unification:
      //    "CreateTime" => "StatTimestamp";
      //    "IsUnique" => "Unique";
      //    "IndexPages" => "TotalIndexPages";
      //    "IndexLevels" => "TotalIndexLevels";
      //    "CollectionSpace" + "Collection" => "Collection";
      // 4. Ignore "_id"
      // 5. If parameter detail is true, show the "MCV" field. Else ignore it.

      INT32 rc = SDB_OK ;
      BOOLEAN hasMCV = FALSE ;
      INT64 *distinctValNum = NULL ;
      INT32 keyFieldNum = 0 ;
      INT32 i = 0 ;

      try
      {
         const CHAR *clName = NULL ;
         const CHAR *csName = NULL ;
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {0} ;
         monAppendSystemInfo( ob, addInfoMask ) ;

         BSONObjIterator iter( stat ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            if ( ossStrcmp( ele.fieldName(), FIELD_NAME_MCV ) != 0 )
            {
               if ( 0 == ossStrcmp( ele.fieldName(), DMS_ID_KEY_NAME ) )
               {
                  continue ;
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         FIELD_NAME_COLLECTION ) )
               {
                  SDB_ASSERT( String == ele.type(), "cl name must be string" ) ;
                  clName = ele.valuestrsafe() ;
                  if ( clName && csName )
                  {
                     ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                                  csName, clName ) ;
                     ob.append( FIELD_NAME_COLLECTION, clFullName ) ;
                  }
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         FIELD_NAME_COLLECTIONSPACE ) )
               {
                  SDB_ASSERT( String == ele.type(), "cs name must be string" ) ;
                  csName = ele.valuestrsafe() ;
                  if ( clName && csName )
                  {
                     ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                                  csName, clName ) ;
                     ob.append( FIELD_NAME_COLLECTION, clFullName ) ;
                  }
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         DMS_STAT_CREATE_TIME ) )
               {
                  ossTimestamp tm( ele.numberLong() ) ;
                  CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
                  ossTimestampToString( tm, timestampStr ) ;
                  ob.append( FIELD_NAME_STAT_TIMESTAMP, timestampStr ) ;
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         FIELD_NAME_KEY_PATTERN ) )
               {
                  keyFieldNum = 0 ;
                  BSONObjIterator keyIter( ele.embeddedObject() ) ;
                  while ( keyIter.more() )
                  {
                     keyIter.next() ;
                     ++keyFieldNum ;
                  }
                  ob.append( ele ) ;
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         DMS_STAT_IDX_IS_UNIQUE ) )
               {
                  ob.appendAs( ele, IXM_FIELD_NAME_UNIQUE1 ) ;
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         DMS_STAT_IDX_INDEX_PAGES ) )
               {
                  ob.appendAs( ele, FIELD_NAME_TOTAL_INDEX_PAGES ) ;
               }
               else if ( 0 == ossStrcmp( ele.fieldName(),
                                         DMS_STAT_IDX_LEVELS ) )
               {
                  ob.appendAs( ele, FIELD_NAME_TOTAL_IDX_LEVELS ) ;
               }
               else
               {
                  ob.append( ele ) ;
               }
               continue ;
            }

            // Convert field "MCV" as "DistinctValNum", "MaxValue", "MinValue",
            // "NullFrac", "UndefFrac".
            hasMCV = TRUE ;

            INT32 nullFrac = 0 ;
            INT32 undefFrac = 0 ;
            BSONObj maxValue ;
            BSONObj minValue ;

            BSONObj mcv = ele.embeddedObject() ;
            BSONElement valueEle = mcv.getField( FIELD_NAME_VALUES ) ;
            SDB_ASSERT( Array == valueEle.type(), "Values of MCV must be array") ;
            BSONElement fracEle = mcv.getField( FIELD_NAME_FRAC ) ;
            SDB_ASSERT( Array == valueEle.type(), "Frac of MCV must be array") ;

            BSONObjIterator valIter( valueEle.embeddedObject() ) ;
            BSONElement val ;
            BSONObjIterator fracIter( fracEle.embeddedObject() ) ;
            BSONElement frac ;
            BSONObj lastValue ;

            // MCV is ordered. Just get the first/last one as min/max.
            // Find the first not null element as MinValue, and get the null frac.
            while ( valIter.more() )
            {
               val = valIter.next() ;
               SDB_ASSERT( Object == val.type(), "Value of MCV must be object" ) ;
               frac = fracIter.next() ;
               SDB_ASSERT( NumberInt == frac.type(), "Frac of MCV must be INT32" ) ;

               BOOLEAN isAllUndefined = TRUE ;
               BOOLEAN isAllNull = TRUE ;

               BSONObjIterator fieldIter( val.embeddedObject() ) ;
               while ( fieldIter.more() )
               {
                  BSONElement fieldEle = fieldIter.next() ;
                  if ( fieldEle.type() != Undefined )
                  {
                     isAllUndefined = FALSE ;
                  }
                  if ( fieldEle.type() != jstNULL )
                  {
                     isAllNull = FALSE ;
                  }
               }

               if ( isAllUndefined )
               {
                  undefFrac = frac.numberInt() ;
               }
               if ( isAllNull )
               {
                  nullFrac = frac.numberInt() ;
               }

               if ( !isAllUndefined && !isAllNull )
               {
                  minValue = val.embeddedObject() ;
                  lastValue = minValue ;
                  break ;
               }
            }

            // Count the distinct values.
            SDB_ASSERT( keyFieldNum > 0, "Field num must exist here" ) ;
            distinctValNum = ( INT64 *) SDB_POOL_ALLOC( keyFieldNum * sizeof( INT64 ) ) ;
            if ( !distinctValNum )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "No memory to allocate the array" ) ;
               goto error ;
            }
            for ( i = 0 ; i < keyFieldNum ; ++i )
            {
               distinctValNum[i] = 1 ;
            }

            while ( valIter.more() )
            {
               val = valIter.next() ;
               SDB_ASSERT( Object == val.type(), "Value of MCV must be object" ) ;
               BSONObj currValue = val.embeddedObject() ;
               i = 0 ;
               BSONObjIterator currIter( currValue ) ;
               BSONObjIterator lastIter( lastValue ) ;

               while ( currIter.more() )
               {
                  BSONElement currEle = currIter.next() ;
                  BSONElement lastEle = lastIter.next() ;
                  if ( !( currEle == lastEle ) )
                  {
                     break ;
                  }
                  ++i ;
               }

               while ( i < keyFieldNum )
               {
                  ++distinctValNum[i] ;
                  ++i ;
               }

               lastValue = currValue ;
            }

            if ( !minValue.isEmpty() ) // no MinValue means all samples are null.
            {
               maxValue = val.embeddedObject() ;
            }

            // Append fields to result
            {
               BSONArrayBuilder ab( ob.subarrayStart( FIELD_NAME_DISTINCT_VAL_NUM ) ) ;
               for ( i = 0 ; i < keyFieldNum ; ++i )
               {
                  ab.append( distinctValNum[i] ) ;
               }
               ab.doneFast() ;
            }

            if ( !minValue.isEmpty() )
            {
               ob.append( FIELD_NAME_MIN_VALUE, minValue ) ;
               ob.append( FIELD_NAME_MAX_VALUE, maxValue ) ;
            }
            else
            {
               ob.appendNull( FIELD_NAME_MIN_VALUE ) ;
               ob.appendNull( FIELD_NAME_MAX_VALUE ) ;
            }

            ob.append( FIELD_NAME_NULL_FRAC, nullFrac ) ;
            ob.append( FIELD_NAME_UNDEF_FRAC, undefFrac ) ;

            if ( detail )
            {
               ob.append( ele ) ;
            }
         }

         // If no MCV, append an empty info.
         if ( !hasMCV )
         {
            BSONArrayBuilder ab( ob.subarrayStart( FIELD_NAME_DISTINCT_VAL_NUM ) ) ;
            SDB_ASSERT( keyFieldNum > 0, "Field num must exist here" ) ;
            for ( i = 0 ; i < keyFieldNum ; ++i )
            {
               ab.append( 0LL ) ;
            }
            ab.doneFast() ;

            ob.appendNull( FIELD_NAME_MIN_VALUE ) ;
            ob.appendNull( FIELD_NAME_MAX_VALUE ) ;
            ob.append( FIELD_NAME_NULL_FRAC, 0 ) ;
            ob.append( FIELD_NAME_UNDEF_FRAC, 0 ) ;
            if ( detail )
            {
               BSONObjBuilder obMCV( ob.subobjStart( FIELD_NAME_MCV ) ) ;
               BSONArrayBuilder abValues(
                  obMCV.subarrayStart( FIELD_NAME_VALUES ) ) ;
               abValues.doneFast() ;
               BSONArrayBuilder abFrac(
                  obMCV.subarrayStart( FIELD_NAME_FRAC ) ) ;
               abFrac.doneFast() ;
               obMCV.doneFast() ;
            }
         }
      }
      catch ( std::bad_alloc &ba )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to build statistics result: %e", ba.what() ) ;
         goto error ;
      }


   done:
      if ( distinctValNum )
      {
         SDB_POOL_FREE( distinctValNum ) ;
      }
      return rc ;
   error:
      goto done;
   }

   /*
      _monTransFetcher implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monTransFetcher )

   _monTransFetcher::_monTransFetcher()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_TRANS )
   {
      _dumpCurrent = TRUE ;
      _detail = FALSE ;
      _addInfoMask = 0 ;
      _slice = 0 ;
   }

   _monTransFetcher::~_monTransFetcher()
   {
   }

   INT32 _monTransFetcher::init( pmdEDUCB *cb,
                                 BOOLEAN isCurrent,
                                 BOOLEAN isDetail,
                                 UINT32 addInfoMask,
                                 const BSONObj obj )
   {
      INT32 rc = SDB_OK ;
      _dumpCurrent = isCurrent ;
      _detail = isDetail ;
      _addInfoMask = addInfoMask ;

      if ( _dumpCurrent )
      {
         _eduList.push( cb->getID() ) ;
      }
      else
      {
         dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
         if ( transCB )
         {
            transCB->dumpTransEDUList( _eduList ) ;
         }
      }

      if ( _eduList.empty() )
      {
         _hitEnd = TRUE ;
      }
      else
      {
         _hitEnd = FALSE ;
         rc = _fetchNextTransInfo() ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            _hitEnd = TRUE ;
         }
      }

      return rc ;
   }

   const CHAR* _monTransFetcher::getName() const
   {
      if ( _dumpCurrent )
      {
         return _detail ? CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR :
                          CMD_NAME_LIST_TRANSACTIONS_CUR ;
      }
      return _detail ? CMD_NAME_SNAPSHOT_TRANSACTIONS :
                       CMD_NAME_LIST_TRANSACTIONS ;
   }

   INT32 _monTransFetcher::_fetchNextTransInfo()
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      DPS_LSN_OFFSET beginLSN = DPS_INVALID_LSN_OFFSET ;
      pmdEDUMgr *pMgr = pmdGetKRCB()->getEDUMgr() ;
      dpsTransCB *pTransCB = sdbGetTransCB() ;
      dpsTransLockManager *pLockMgr = pTransCB->getLockMgrHandle() ;
      pmdTransExecutor *executor = NULL ;

   retry:
      if ( !pLockMgr || _eduList.empty() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      eduID = _eduList.front() ;
      _eduList.pop() ;

      /// clear
      _curTransInfo.clear() ;

      executor = NULL ;
      if ( SDB_OK != pMgr->beginDumpEDUTrans( eduID, &executor,
                                              _curTransInfo ) )
      {
         // no session or no trans
         goto retry ;
      }

      rc = pLockMgr->dumpEDUTransInfo( executor, _curTransInfo._waitLock,
                                       _curTransInfo._lockList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to dump edu transInfo:eduID=%llu,rc=%d",
                 eduID, rc ) ;
         pMgr->endDumpEDUTrans( eduID ) ;
         goto error ;
      }

      pMgr->endDumpEDUTrans( eduID ) ;

      _curTransInfo._locksNum = _curTransInfo._lockList.size() ;
      _pos = _curTransInfo._lockList.begin() ;
      try
      {
         BSONObjBuilder builder ;
         monAppendSystemInfo( builder, _addInfoMask ) ;
         builder.append( FIELD_NAME_SESSIONID,
                         (INT64)_curTransInfo._eduID ) ;
         /// nodeID(16bit) | TAG(8bit) | SN(40bit)
         CHAR strTransID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
         dpsTransIDToString( _curTransInfo._transID, strTransID,
                             DPS_TRANS_STR_LEN ) ;
         builder.append( FIELD_NAME_TRANSACTION_ID, strTransID ) ;

         builder.append( FIELD_NAME_TRANSACTION_ID_SN,
                         (INT64)DPS_TRANS_GET_SN( _curTransInfo._transID ) ) ;
         builder.appendBool( FIELD_NAME_IS_ROLLBACK,
                             pTransCB->isRollback( _curTransInfo._transID ) ?
                             TRUE : FALSE ) ;
         builder.append( FIELD_NAME_TRANS_LSN_CUR,
                         (INT64)_curTransInfo._curTransLsn ) ;

         beginLSN = pTransCB->getBeginLsn( _curTransInfo._transID ) ;
         builder.append( FIELD_NAME_TRANS_LSN_BEGIN, (INT64)beginLSN ) ;

         /// waiter lock
         BSONObjBuilder subWaiter( builder.subobjStart(
                                   FIELD_NAME_TRANS_WAIT_LOCK ) ) ;
         if ( _curTransInfo._waitLock._id.isValid() )
         {
            _curTransInfo._waitLock.toBson( subWaiter, FALSE ) ;
         }
         subWaiter.done() ;

         builder.append( FIELD_NAME_TRANS_LOCKS_NUM,
                         (INT32)_curTransInfo._locksNum ) ;
         builder.appendBool( FIELD_NAME_TRANS_IS_LOCK_ESCALATED,
                             _curTransInfo._lockEscalated ) ;
         builder.append( FIELD_NAME_USED_LOG_SPACE,
                         (INT64)( _curTransInfo._usedLogSpace ) ) ;
         builder.append( FIELD_NAME_RESERVED_LOG_SPACE,
                         (INT64)( _curTransInfo._reservedLogSpace ) ) ;
         monAppendSessionIdentify( builder, _curTransInfo._relatedNID,
                                   _curTransInfo._relatedEDUID ) ;

         _curEduInfo = builder.obj() ;
         _slice = 0 ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monTransFetcher::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      UINT32 lockNum = 0 ;
      BOOLEAN hitThisEnd = FALSE ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_detail )
      {
         obj = _curEduInfo ;
         hitThisEnd = TRUE ;
      }
      else
      {
         BSONObjBuilder bobEduTransInfo( MON_DUMP_DFT_BUILDER_SZ ) ;
         bobEduTransInfo.appendElements( _curEduInfo ) ;

         BSONArrayBuilder babLockList( bobEduTransInfo.subarrayStart(
                                       FIELD_NAME_TRANS_LOCKS ) ) ;
         for ( ; lockNum < MON_MAX_SLICE_SIZE ; ++lockNum )
         {
            if ( _pos == _curTransInfo._lockList.end() )
            {
               break ;
            }
            else if ( (*_pos)._id == _curTransInfo._waitLock._id )
            {
               ++_pos ;
               continue ;
            }

            BSONObjBuilder subLock( babLockList.subobjStart() ) ;
            (*_pos).toBson( subLock ) ;
            subLock.done() ;
            ++_pos ;
         }
         babLockList.done() ;

         if ( _pos == _curTransInfo._lockList.end() )
         {
            hitThisEnd = TRUE ;
         }
         else if ( 0 == _slice )
         {
            _slice = 1 ;
         }

         if ( _slice > 0 )
         {
            bobEduTransInfo.append( FIELD_NAME_SLICE, (INT32)_slice ) ;
            ++_slice ;
         }
         obj = bobEduTransInfo.obj() ;
      }

      if ( hitThisEnd )
      {
         rc = _fetchNextTransInfo() ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Fetch next trans info failed, rc: %d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monContextFetcher implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monContextFetcher )

   _monContextFetcher::_monContextFetcher()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_CONTEXT )
   {
      _dumpCurrent = FALSE ;
      _detail = TRUE ;
      _addInfoMask = 0 ;
   }

   _monContextFetcher::~_monContextFetcher()
   {
   }

   INT32 _monContextFetcher::init( pmdEDUCB *cb,
                                   BOOLEAN isCurrent,
                                   BOOLEAN isDetail,
                                   UINT32 addInfoMask,
                                   const BSONObj obj )
   {
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      SDB_ASSERT( rtnCB, "RTNCB can't be NULL" ) ;
      SDB_ASSERT( cb, "CB can't be NULL" ) ;

      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _dumpCurrent = isCurrent ;

      if ( !_detail )
      {
         if ( _dumpCurrent )
         {
            pmdEDUCB::SET_CONTEXT &contextList = _contextList[ cb->getID() ] ;
            cb->contextCopy( contextList ) ;
         }
         else
         {
            rtnCB->contextDump( _contextList ) ;
         }
         _hitEnd = _contextList.empty() ? TRUE : FALSE ;
      }
      else
      {
         rtnCB->monContextSnap( _contextInfoList, _dumpCurrent ?
                                cb->getID() : PMD_INVALID_EDUID ) ;
         _hitEnd = _contextInfoList.empty() ? TRUE : FALSE ;
      }

      return SDB_OK ;
   }

   const CHAR* _monContextFetcher::getName() const
   {
      if ( _dumpCurrent )
      {
         return _detail ? CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT :
                          CMD_NAME_LIST_CONTEXTS_CURRENT ;
      }
      return _detail ? CMD_NAME_SNAPSHOT_CONTEXTS :
                       CMD_NAME_LIST_CONTEXTS ;
   }

   INT32 _monContextFetcher::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _detail )
      {
         rc = _fetchNextDetail( obj ) ;
      }
      else
      {
         rc = _fetchNextSimple( obj ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monContextFetcher::_fetchNextSimple( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _contextList.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder ) ;
         std::map<UINT64, pmdEDUCB::SET_CONTEXT>::iterator it ;
         pmdEDUCB::SET_CONTEXT::iterator itSet ;

         it = _contextList.begin() ;
         pmdEDUCB::SET_CONTEXT &setCtx = it->second ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SESSIONID, (SINT64)it->first ) ;
         ob.append( FIELD_NAME_TOTAL_COUNT, (INT32)setCtx.size() ) ;

         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         for ( itSet = setCtx.begin(); itSet!= setCtx.end(); ++itSet )
         {
            ba.append ( (*itSet) ) ;
         }
         ba.done() ;
         obj = ob.done() ;

         /// remove current edu info
         _contextList.erase( it ) ;
         if ( _contextList.size() == 0 )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for context, %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monContextFetcher::_fetchNextDetail( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _contextInfoList.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder );
         ossTickConversionFactor factor ;

         std::map<UINT64, std::set<monContextFull> >::iterator it ;
         std::set<monContextFull>::iterator itSet ;

         CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
         UINT32 seconds = 0 ;
         UINT32 microseconds = 0 ;
         CHAR queryIDStr[MSG_QUERY_ID_HEX_STR_LEN+1] = { 0 } ;

         it = _contextInfoList.begin() ;
         std::set<monContextFull> &setInfo = it->second ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         /// add session id
         ob.append( FIELD_NAME_SESSIONID, (INT64)it->first ) ;

         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         for ( itSet = setInfo.begin() ; itSet != setInfo.end() ; ++itSet )
         {
            const monContextFull &ctx = *itSet ;
            ossTimestamp startTime( ctx._monContext.getStartTimestamp() ) ;
            BSONObjBuilder sub( ba.subobjStart() ) ;

            ctx._queryID.toHexStr( queryIDStr, sizeof(queryIDStr)-1 ) ;
            sub.append( FIELD_NAME_QUERY_ID, queryIDStr ) ;

            sub.append( FIELD_NAME_CONTEXTID, ctx._contextID ) ;
            sub.append( FIELD_NAME_TYPE, ctx._typeDesp ) ;
            sub.append( FIELD_NAME_DESP, ctx._info ) ;
            sub.append( FIELD_NAME_DATAREAD,
                        (INT64)ctx._monContext.getDataRead() ) ;
            sub.append( FIELD_NAME_INDEXREAD,
                        (INT64)ctx._monContext.getIndexRead() ) ;
            sub.append( FIELD_NAME_LOBREAD,
                        (INT64)ctx._monContext.getLobRead() ) ;
            sub.append( FIELD_NAME_LOBWRITE,
                        (INT64)ctx._monContext.getLobWrite() ) ;
            sub.append( FIELD_NAME_LOBTRUNCATE,
                        (INT64)ctx._monContext.getLobTruncate() ) ;
            sub.append( FIELD_NAME_LOBADDRESSING,
                        (INT64)ctx._monContext.getLobAddressing() ) ;

            ctx._monContext.getQueryTime().convertToTime ( factor,
                                                           seconds,
                                                           microseconds ) ;
            sub.append( FIELD_NAME_QUERYTIMESPENT,
                        (SINT64)( seconds * 1000 + microseconds / 1000 ) ) ;
            ossTimestampToString( startTime, timestampStr ) ;
            sub.append( FIELD_NAME_STARTTIMESTAMP, timestampStr ) ;
            sub.done() ;
         }
         ba.done() ;
         obj = ob.done() ;

         /// remove current
         _contextInfoList.erase( it ) ;
         if ( _contextInfoList.size() == 0 )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for context: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monSessionFetcher implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monSessionFetcher )

   _monSessionFetcher::_monSessionFetcher()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_SESSION )
   {
      _dumpCurrent = TRUE ;
      _detail = FALSE ;
      _addInfoMask = 0 ;
   }

   _monSessionFetcher::~_monSessionFetcher()
   {
   }

   INT32 _monSessionFetcher::init( pmdEDUCB *cb,
                                   BOOLEAN isCurrent,
                                   BOOLEAN isDetail,
                                   UINT32 addInfoMask,
                                   const BSONObj obj )
   {
      SDB_ASSERT( cb, "cb can't be NULL" ) ;
      INT32 rc = SDB_OK ;
      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _dumpCurrent = isCurrent ;

      if ( !_detail )
      {
         if ( _dumpCurrent )
         {
            monEDUSimple info ;
            cb->dumpInfo( info ) ;
            _setInfoSimple.insert( info ) ;
         }
         else
         {
            rc = cb->getEDUMgr()->dumpInfo( _setInfoSimple ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         _hitEnd = _setInfoSimple.empty() ? TRUE : FALSE ;
      }
      else
      {
         if ( _dumpCurrent )
         {
            monEDUFull info ;
            cb->dumpInfo( info ) ;
            _setInfoDetail.insert( info ) ;
         }
         else
         {
            rc = cb->getEDUMgr()->dumpInfo( _setInfoDetail ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         _hitEnd = _setInfoDetail.empty() ? TRUE : FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monSessionFetcher::getName() const
   {
      if ( _dumpCurrent )
      {
         return _detail ? CMD_NAME_SNAPSHOT_SESSIONS_CURRENT :
                          CMD_NAME_LIST_SESSIONS_CURRENT ;
      }
      return _detail ? CMD_NAME_SNAPSHOT_SESSIONS :
                       CMD_NAME_LIST_SESSIONS ;
   }

   INT32 _monSessionFetcher::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _detail )
      {
         rc = _fetchNextDetail( obj ) ;
      }
      else
      {
         rc = _fetchNextSimple( obj ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monSessionFetcher::_fetchNextSimple( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _setInfoSimple.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob(_builder);
         std::set<monEDUSimple>::iterator it ;

         it = _setInfoSimple.begin() ;
         const monEDUSimple &simple = *it ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append ( FIELD_NAME_SESSIONID, (SINT64)simple._eduID ) ;
         ob.append ( FIELD_NAME_TID, simple._tid ) ;
         ob.append ( FIELD_NAME_STATUS, simple._eduStatus ) ;
         ob.append ( FIELD_NAME_TYPE, simple._eduType ) ;
         ob.append ( FIELD_NAME_EDUNAME, simple._eduName ) ;
         ob.append ( FIELD_NAME_SOURCE, simple._source ) ;
         monAppendSessionIdentify( ob, simple._relatedNID,
                                   simple._relatedEDUID ) ;

         obj = ob.done () ;

         /// remove current
         _setInfoSimple.erase( it ) ;
         if ( _setInfoSimple.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for session: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monSessionFetcher::_fetchNextDetail( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _setInfoDetail.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob(_builder);
         std::set<monEDUFull>::iterator it ;

         it = _setInfoDetail.begin() ;
         const monEDUFull &full = *it ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SESSIONID, (INT64)full._eduID ) ;
         ob.append( FIELD_NAME_TID, full._tid ) ;
         ob.append( FIELD_NAME_STATUS, full._eduStatus ) ;
         ob.appendBool( FIELD_NAME_ISBLOCKED, full._isBlock ) ;
         ob.append( FIELD_NAME_TYPE, full._eduType ) ;
         ob.append( FIELD_NAME_EDUNAME, full._eduName ) ;
         ob.append( FIELD_NAME_DOING, full._doing ) ;
         ob.append( FIELD_NAME_SOURCE, full._source ) ;
         ob.append( FIELD_NAME_QUEUE_SIZE, full._queueSize ) ;
         ob.append( FIELD_NAME_PROCESS_EVENT_COUNT,
                    (SINT64)full._processEventCount ) ;
         ob.append( FIELD_NAME_MEMPOOL_SIZE, (INT32)full._memPoolSize ) ;
         monAppendSessionIdentify( ob, full._relatedNID,
                                   full._relatedEDUID ) ;
         /// add contexts
         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         ossPoolSet<SINT64>::const_iterator itCtx ;
         for ( itCtx = full._eduContextList.begin() ;
               itCtx != full._eduContextList.end() ;
               ++itCtx )
         {
            ba.append( *itCtx ) ;
         }
         ba.done() ;

         ossTime userTime, sysTime ;
         ossTickConversionFactor factor ;
         rc = ossGetCPUUsage( full._threadHdl, userTime, sysTime ) ;
         if ( rc )
         {
            pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
            pmdEDUCB *cb = eduMgr->getEDUByID( full._eduID ) ;
            if ( NULL == cb )
            {
               // The thread doesn't exist, so just ignore error
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
         /// add app cb info
         monSessionMonEDUFull( ob, full, factor, userTime, sysTime ) ;
         obj = ob.done () ;

         /// remove the current
         _setInfoDetail.erase( it ) ;
         if ( _setInfoDetail.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for session: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monCollectionFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCollectionFetch )

   _monCollectionFetch::_monCollectionFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_COLLECTION )
   {
      _detail = FALSE ;
      _includeSys = FALSE ;
      _addInfoMask = 0 ;
   }

   _monCollectionFetch::~_monCollectionFetch()
   {
   }

   INT32 _monCollectionFetch::init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj )
   {
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_ASSERT( dmsCB, "DMSCB can't be NULL" ) ;

      INT32 rc = SDB_OK ;
      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _includeSys = isCurrent ;

      if ( !_detail )
      {
         rc = dmsCB->dumpInfo( _collectionList, _includeSys ) ;
         _hitEnd = _collectionList.empty() ? TRUE : FALSE ;
      }
      else
      {
         rc = dmsCB->dumpInfo( _collectionInfo, _includeSys ) ;
         _hitEnd = _collectionInfo.empty() ? TRUE : FALSE ;
      }

      return rc ;
   }

   const CHAR* _monCollectionFetch::getName() const
   {
      return _detail ? CMD_NAME_SNAPSHOT_COLLECTIONS :
                       CMD_NAME_LIST_COLLECTIONS ;
   }

   INT32 _monCollectionFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _detail )
      {
         rc = _fetchNextDetail( obj ) ;
      }
      else
      {
         rc = _fetchNextSimple( obj ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCollectionFetch::_fetchNextSimple( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _collectionList.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder ) ;
         MON_CL_SIM_LIST::iterator it ;

         it = _collectionList.begin() ;
         const monCLSimple &simple = *it ;

         ob.append ( FIELD_NAME_NAME, simple._name ) ;

         obj = ob.done() ;

         /// remove current
         _collectionList.erase( it ) ;
         if ( _collectionList.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for collections: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCollectionFetch::_fetchNextDetail( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _collectionInfo.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;
         MON_CL_LIST::iterator it = _collectionInfo.begin() ;

         rc = monCollection2Obj( *it, _addInfoMask, ob ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON obj, rc: %d", rc ) ;
         obj = ob.done() ;

         /// remove the current
         _collectionInfo.erase( it ) ;
         if ( _collectionInfo.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for collections: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monCollectionSpaceFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCollectionSpaceFetch )

   _monCollectionSpaceFetch::_monCollectionSpaceFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_COLLECTIONSPACE )
   {
      _detail = FALSE ;
      _includeSys = FALSE ;
      _addInfoMask = 0 ;
   }

   _monCollectionSpaceFetch::~_monCollectionSpaceFetch()
   {
   }

   INT32 _monCollectionSpaceFetch::init( pmdEDUCB *cb,
                                         BOOLEAN isCurrent,
                                         BOOLEAN isDetail,
                                         UINT32 addInfoMask,
                                         const BSONObj obj )
   {
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_ASSERT( dmsCB, "DMSCB can't be NULL" ) ;

      INT32 rc = SDB_OK ;
      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _includeSys = isCurrent ;

      if ( !_detail )
      {
         rc = dmsCB->dumpInfo( _csList, _includeSys, FALSE, FALSE ) ;
         _hitEnd = _csList.empty() ? TRUE : FALSE ;
      }
      else
      {
         rc = dmsCB->dumpInfo( _csInfo, _includeSys ) ;
         _hitEnd = _csInfo.empty() ? TRUE : FALSE ;
      }

      return rc ;
   }

   const CHAR* _monCollectionSpaceFetch::getName() const
   {
      return _detail ? CMD_NAME_SNAPSHOT_COLLECTIONSPACES :
                       CMD_NAME_LIST_COLLECTIONSPACES ;
   }

   INT32 _monCollectionSpaceFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _detail )
      {
         rc = _fetchNextDetail( obj ) ;
      }
      else
      {
         rc = _fetchNextSimple( obj ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCollectionSpaceFetch::_fetchNextSimple( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _csList.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;
         MON_CS_SIM_LIST::iterator it ;

         it = _csList.begin() ;
         const monCSSimple &simple = *it ;

         ob.append ( FIELD_NAME_NAME, simple._name ) ;

         obj = ob.done() ;

         /// remove current
         _csList.erase( it ) ;
         if ( _csList.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for "
                  "collectionspaces: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCollectionSpaceFetch::_fetchNextDetail( BSONObj &obj )
   {
      INT32 rc                = SDB_OK ;
      INT64 dataCapSize       = 0 ;
      INT64 lobCapSize        = 0 ;
      INT64 totalUsedLobSpace = 0 ;
      INT64 avgLobSize        = 0 ;

      if ( _csInfo.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;
         MON_CS_LIST::iterator it ;
         MON_CL_DETAIL_MAP::const_iterator itDetail ;

         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;

         it = _csInfo.begin() ;
         const monCollectionSpace &full = *it ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         /// add name & space name
         ob.append ( FIELD_NAME_NAME, full._name ) ;
         ob.append ( FIELD_NAME_UNIQUEID, full._csUniqueID ) ;
         ob.append ( FIELD_NAME_ID, full._suID ) ;
         ob.append ( FIELD_NAME_LOGICAL_ID, full._csLID ) ;

         /// add detial
         BSONArrayBuilder sub( ob.subarrayStart( FIELD_NAME_COLLECTION ) ) ;
         // do not list detailed collections if we are on temp cs
         if ( ossStrcmp ( full._name, SDB_DMSTEMP_NAME ) != 0 )
         {
            MON_CL_SIM_VEC::const_iterator it1 ;
            for ( it1 = full._collections.begin();
                  it1!= full._collections.end();
                  it1++ )
            {
               sub.append ( BSON ( FIELD_NAME_NAME <<
                                   it1->_name <<
                                   FIELD_NAME_UNIQUEID <<
                                   (INT64)it1->_clUniqueID ) ) ;
            }
         }
         sub.done() ;

         dataCapSize = (INT64)full._pageSize * DMS_MAX_PG ;
         lobCapSize  = (INT64)full._lobPageSize * DMS_MAX_PG ;
         if ( lobCapSize > OSS_MAX_FILE_SZ )
         {
            lobCapSize = OSS_MAX_FILE_SZ ;
         }
         totalUsedLobSpace = full._lobCapacity - full._freeLobSpace ;
         if ( 0 < full._totalLobs )
         {
            avgLobSize = full._totalValidLobSize / full._totalLobs ;
         }

         ob.append ( FIELD_NAME_PAGE_SIZE, full._pageSize ) ;
         ob.append ( FIELD_NAME_LOB_PAGE_SIZE, full._lobPageSize ) ;
         ob.append ( FIELD_NAME_MAX_CAPACITY_SIZE,
                     2 * dataCapSize + lobCapSize ) ;
         ob.append ( FIELD_NAME_MAX_DATA_CAP_SIZE, dataCapSize ) ;
         ob.append ( FIELD_NAME_MAX_INDEX_CAP_SIZE, dataCapSize ) ;
         ob.append ( FIELD_NAME_MAX_LOB_CAP_SIZE, lobCapSize ) ; // deprecated
         ob.append ( FIELD_NAME_NUMCOLLECTIONS, full._clNum ) ;
         ob.append ( FIELD_NAME_TOTAL_RECORDS, full._totalRecordNum ) ;
         ob.append ( FIELD_NAME_TOTAL_SIZE, full._totalSize ) ;
         ob.append ( FIELD_NAME_FREE_SIZE, full._freeSize ) ;
         ob.append ( FIELD_NAME_TOTAL_DATA_SIZE, full._totalDataSize ) ;
         ob.append ( FIELD_NAME_FREE_DATA_SIZE, full._freeDataSize ) ;
         ob.append ( FIELD_NAME_TOTAL_IDX_SIZE, full._totalIndexSize ) ;
         ob.append ( FIELD_NAME_FREE_IDX_SIZE, full._freeIndexSize ) ;
         ob.append ( FIELD_NAME_RECYCLE_DATA_SIZE, full._recycleDataSize ) ;
         ob.append ( FIELD_NAME_RECYCLE_IDX_SIZE, full._recycleIndexSize ) ;
         ob.append ( FIELD_NAME_RECYCLE_LOB_SIZE, full._recycleLobSize ) ;
         ob.append ( FIELD_NAME_FREE_LOB_SIZE, full._freeLobSpace ) ; // deprecated
         ob.append ( FIELD_NAME_MAX_LOB_CAPACITY, lobCapSize ) ;
         ob.append ( FIELD_NAME_LOB_CAPACITY, full._lobCapacity ) ;
         ob.append ( FIELD_NAME_LOB_META_CAPACITY, full._lobMetaCapacity ) ;
         ob.append ( FIELD_NAME_TOTAL_LOBS, full._totalLobs ) ;
         ob.append ( FIELD_NAME_TOTAL_LOB_PAGES, full._totalLobPages ) ;
         ob.append ( FIELD_NAME_TOTAL_USED_LOB_SPACE, totalUsedLobSpace) ;
         ob.append ( FIELD_NAME_USED_LOB_SPACE_RATIO,
                    utilPercentage( totalUsedLobSpace, full._lobCapacity ) ) ;
         ob.append ( FIELD_NAME_FREE_LOB_SPACE, full._freeLobSpace ) ;
         ob.append ( FIELD_NAME_TOTAL_LOB_SIZE, full._totalLobSize ) ;
         ob.append ( FIELD_NAME_TOTAL_VALID_LOB_SIZE, full._totalValidLobSize ) ;
         /// Because lob page 0 is unevenly distributed on data nodes, the
         /// _totalValidLobSize may be larger than the _totalUsedLobSpace,
         /// so use _totalLobSize / _totalUsedLobSpace in data nodes.
         ob.append ( FIELD_NAME_LOB_USAGE_RATE,
                     utilPercentage( full._totalLobSize, totalUsedLobSpace ) ) ;
         ob.append ( FIELD_NAME_AVG_LOB_SIZE, avgLobSize ) ;

         ob.append ( FIELD_NAME_TOTALLOBGET, full._totalLobGet ) ;
         ob.append ( FIELD_NAME_TOTALLOBPUT, full._totalLobPut ) ;
         ob.append ( FIELD_NAME_TOTALLOBDELETE, full._totalLobDelete ) ;
         ob.append ( FIELD_NAME_TOTALLOBREADSIZE, full._totalLobReadSize ) ;
         ob.append ( FIELD_NAME_TOTALLOBWRITESIZE, full._totalLobWriteSize ) ;
         ob.append ( FIELD_NAME_TOTALLOBREAD, full._totalLobRead ) ;
         ob.append ( FIELD_NAME_TOTALLOBWRITE, full._totalLobWrite ) ;
         ob.append ( FIELD_NAME_TOTALLOBTRUNCATE, full._totalLobTruncate ) ;
         ob.append ( FIELD_NAME_TOTALLOBADDRESSING, full._totalLobAddressing ) ;
         ob.append ( FIELD_NAME_TOTALLOBLIST, full._totalLobList ) ;

         /// sync info
         ob.append ( FIELD_NAME_DATA_COMMIT_LSN, (INT64)full._dataCommitLsn ) ;
         ob.append ( FIELD_NAME_IDX_COMMIT_LSN, (INT64)full._idxCommitLsn ) ;
         ob.append ( FIELD_NAME_LOB_COMMIT_LSN, (INT64)full._lobCommitLsn ) ;
         ob.appendBool ( FIELD_NAME_DATA_COMMITTED, full._dataIsValid ) ;
         ob.appendBool ( FIELD_NAME_IDX_COMMITTED, full._idxIsValid ) ;
         ob.appendBool ( FIELD_NAME_LOB_COMMITTED, full._lobIsValid ) ;

         /// cache info
         ob.append ( FIELD_NAME_DIRTY_PAGE, (INT32)full._dirtyPage ) ;
         ob.append( FIELD_NAME_TYPE, (INT32)full._type ) ;

         ossMillisecondsToString( full._createTime, timestamp ) ;
         ob.append( FIELD_NAME_CREATE_TIME, timestamp ) ;

         ossMillisecondsToString( full._updateTime, timestamp ) ;
         ob.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

         obj = ob.done() ;

         /// remove the current
         _csInfo.erase( it ) ;
         if ( _csInfo.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for "
                  "collectionspaces: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monDataBaseFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monDataBaseFetch )

   _monDataBaseFetch::_monDataBaseFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_DATABASE )
   {
      _addInfoMask   = 0 ;
   }

   _monDataBaseFetch::~_monDataBaseFetch()
   {
   }

   INT32 _monDataBaseFetch::init( pmdEDUCB *cb,
                                  BOOLEAN isCurrent,
                                  BOOLEAN isDetail,
                                  UINT32 addInfoMask,
                                  const BSONObj obj )
   {
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      return SDB_OK ;
   }

   const CHAR* _monDataBaseFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_DATABASE ;
   }

   INT32 _monDataBaseFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb     = pmdGetKRCB() ;
      monDBCB *mondbcb  = krcb->getMonDBCB () ;
      pmdEDUMgr *mgr    = krcb->getEDUMgr() ;
      SDB_RTNCB *rtnCB  = krcb->getRTNCB() ;
      SDB_ASSERT ( mgr, "EDU Mgr can't be NULL" ) ;

      ossTime userTime, sysTime ;
      INT64 diskTotalBytes    = 0 ;
      INT64 diskFreeBytes     = 0 ;
      INT64 diskAvailBytes    = 0 ;
      INT32 loadPercent       = 0 ;
      const CHAR *dbPath = pmdGetOptionCB()->getDbPath() ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      ossGetCPUUsage ( userTime, sysTime ) ;
      ossGetDiskInfo ( dbPath, diskTotalBytes, diskFreeBytes,
                       diskAvailBytes, loadPercent ) ;

      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder );

         /// add system info
         monAppendSystemInfo ( ob, _addInfoMask ) ;
         /// add version
         monAppendVersion ( ob ) ;

         ossTickConversionFactor factor ;
         ob.append ( FIELD_NAME_CURRENTACTIVESESSIONS,
                     (SINT32)mgr->sizeRun() ) ;
         ob.append ( FIELD_NAME_CURRENTIDLESESSIONS,
                     (SINT32)mgr->sizeIdle () ) ;
         ob.append ( FIELD_NAME_CURRENTSYSTEMSESSIONS,
                     (SINT32)mgr->sizeSystem() ) ;
         ob.append( FIELD_NAME_CURRENTTASKSESSIONS,
                     (SINT32)mgr->sizeByType( EDU_TYPE_BACKGROUND_JOB ) ) ;
         ob.append ( FIELD_NAME_CURRENTCONTEXTS, (SINT32)rtnCB->contextNum() ) ;
         ob.append ( FIELD_NAME_RECEIVECOUNT,
                     (INT64)mondbcb->getReceiveNum() ) ;
         ob.append ( FIELD_NAME_ROLE, krcb->getOptionCB()->dbroleStr() ) ;

         {
            BSONObjBuilder diskOb( ob.subobjStart( FIELD_NAME_DISK ) ) ;
            diskOb.append ( FIELD_NAME_DATABASEPATH, dbPath ) ;
            diskOb.append ( FIELD_NAME_LOADPERCENT, loadPercent ) ;
            diskOb.append ( FIELD_NAME_TOTALSPACE, diskTotalBytes ) ;
            diskOb.append ( FIELD_NAME_FREESPACE, diskAvailBytes ) ;
            diskOb.done() ;
         }

         monDBDump ( ob, mondbcb, factor, userTime, sysTime ) ;
         monDBDumpLogInfo( ob ) ;
         monDBDumpProcMemInfo( ob ) ;
         monDBDumpStorageInfo( ob ) ;
         monDBDumpNetInfo( ob ) ;

         sdbGetClsCB()->dumpSchedInfo( ob ) ;

         UINT64 memTotalSize = krcb->getMemBlockPool()->getTotalSize() ;
         UINT64 memUsedSize = krcb->getMemBlockPool()->getUsedSize() ;
         ob.append ( FIELD_NAME_MEMPOOL_SIZE, (INT64)memTotalSize ) ;
         ob.append ( FIELD_NAME_MEMPOOL_USED, (INT64)memUsedSize ) ;
         if ( memTotalSize > memUsedSize )
         {
            ob.append ( FIELD_NAME_MEMPOOL_FREE, (INT64)( memTotalSize - memUsedSize ) ) ;
         }
         else
         {
            ob.append ( FIELD_NAME_MEMPOOL_FREE, (INT64)0 ) ;
         }
         ob.append ( FIELD_NAME_MEMPOOL_MAX_OOLSZ,
                     (INT64)krcb->getMemBlockPool()->getMaxOOLSize() ) ;

         obj = ob.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for db snap: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _hitEnd = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monSystemFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monSystemFetch )

   _monSystemFetch::_monSystemFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_SYSTEM )
   {
      _addInfoMask   = 0 ;
   }

   _monSystemFetch::~_monSystemFetch()
   {
   }

   INT32 _monSystemFetch::init( pmdEDUCB *cb,
                                BOOLEAN isCurrent,
                                BOOLEAN isDetail,
                                UINT32 addInfoMask,
                                const BSONObj obj )
   {
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      return SDB_OK ;
   }

   const CHAR* _monSystemFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_SYSTEM ;
   }

   INT32 _monSystemFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      // cpu
      INT64 cpuUser        = 0 ;
      INT64 cpuSys         = 0 ;
      INT64 cpuIdle        = 0 ;
      INT64 cpuIOWait      = 0 ;
      INT64 cpuOther       = 0 ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      // cpu
      rc = ossGetCPUInfo ( cpuUser, cpuSys, cpuIdle, cpuIOWait, cpuOther ) ;
       if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get cpu info, rc = %d", rc ) ;
         goto error ;
      }

      // generate BSON return obj
      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;

         /// add system info
         monAppendSystemInfo ( ob, _addInfoMask ) ;
         // cpu
         {
            BSONObjBuilder cpuOb( ob.subobjStart( FIELD_NAME_CPU ) ) ;
            cpuOb.append ( FIELD_NAME_USER, ((FLOAT64)cpuUser)/1000 ) ;
            cpuOb.append ( FIELD_NAME_SYS, ((FLOAT64)cpuSys)/1000 ) ;
            cpuOb.append ( FIELD_NAME_IDLE, ((FLOAT64)cpuIdle)/1000 ) ;
            cpuOb.append ( FIELD_NAME_IOWAIT, ((FLOAT64)cpuIOWait)/1000 ) ;
            cpuOb.append ( FIELD_NAME_OTHER, ((FLOAT64)cpuOther)/1000 ) ;
            cpuOb.done() ;
         }
         // memory
         monAppendHostMemory( ob ) ;
         // disk
         monAppendDisk( ob ) ;
         obj = ob.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to generate system snapshot: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _hitEnd = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monHealthFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monHealthFetch )

   _monHealthFetch::_monHealthFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_HEALTH )
   {
      _addInfoMask   = 0 ;
   }

   _monHealthFetch::~_monHealthFetch()
   {
   }

   INT32 _monHealthFetch::init( pmdEDUCB *cb,
                                BOOLEAN isCurrent,
                                BOOLEAN isDetail,
                                UINT32 addInfoMask,
                                const BSONObj obj )
   {
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      return SDB_OK ;
   }

   const CHAR* _monHealthFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_HEALTH ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MONHEALTHFETCH_FETCH, "_monHealthFetch::fetch" )
   INT32 _monHealthFetch::fetch( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__MONHEALTHFETCH_FETCH ) ;
      INT32 rc             = SDB_OK ;
      pmdKRCB *krcb        = pmdGetKRCB() ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      // generate BSON return obj
      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;

         monAppendSystemInfo ( ob, _addInfoMask ) ;

         const CHAR* dataStatus = utilDataStatusStr( pmdGetStartup().isOK(),
                                                     krcb->getDBStatus() ) ;
         ob.append( FIELD_NAME_DATA_STATUS, dataStatus ) ;

         ob.appendBool( FIELD_NAME_SYNC_CONTROL, krcb->isInFlowControl()  ) ;

         monAppendUlimit( ob ) ;

         /// number of occurred error
         pmdOccurredErr numErr = pmdGetOccurredErr();
         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestampToString( numErr._resetTimestamp, timestamp ) ;
         ob.append( FIELD_NAME_RESETTIMESTAMP, timestamp ) ;

         BSONObjBuilder errOb( ob.subobjStart( FIELD_NAME_ERRNUM ) ) ;
         errOb.append( FIELD_NAME_OOM,        (INT64)numErr._oom ) ;
         errOb.append( FIELD_NAME_NOSPC,      (INT64)numErr._noSpc ) ;
         errOb.append( FIELD_NAME_TOOMANY_OF, (INT64)numErr._tooManyOpenFD ) ;
         errOb.done() ;

         monAppendNodeMemory( ob ) ;

         monAppendDisk( ob, FALSE ) ;

         monAppendFileDesp( ob ) ;

         monAppendStartInfo( ob ) ;

         monAppendDiffLSN( ob ) ;

         obj = ob.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to generate health snapshot: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _hitEnd = TRUE ;

   done:
      PD_TRACE_EXITRC ( SDB__MONHEALTHFETCH_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _monTasksFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monTasksFetch )

   _monTasksFetch::_monTasksFetch()
      : rtnFetchBase( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_TASKS ),
        _addInfoMask( 0 )
   {
   }

   _monTasksFetch::~_monTasksFetch()
   {
   }

   INT32 _monTasksFetch::init( pmdEDUCB *cb,
                               BOOLEAN isCurrent,
                               BOOLEAN isDetail,
                               UINT32 addInfoMask,
                               const BSONObj obj )
   {
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      sdbGetRTNCB()->getTaskStatusMgr()->dumpInfo( _mapInfo ) ;

      return SDB_OK ;
   }

   const CHAR* _monTasksFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_TASKS ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MONTASKFETCH_FETCH, "_monTasksFetch::fetch" )
   INT32 _monTasksFetch::fetch( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__MONTASKFETCH_FETCH ) ;
      INT32 rc = SDB_OK ;

      if ( _mapInfo.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder );

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ossPoolMap<UINT64, BSONObj>::iterator it = _mapInfo.begin() ;
         ob.appendElements( it->second ) ;

         obj = ob.done();

         /// remove current
         _mapInfo.erase( it ) ;
         if ( _mapInfo.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__MONTASKFETCH_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _monStorageUnitFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monStorageUnitFetch )

   _monStorageUnitFetch::_monStorageUnitFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_STORAGEUNIT )
   {
      _includeSys    = FALSE ;
      _addInfoMask   = 0 ;
   }

   _monStorageUnitFetch::~_monStorageUnitFetch()
   {
   }

   INT32 _monStorageUnitFetch::init( pmdEDUCB *cb,
                                     BOOLEAN isCurrent,
                                     BOOLEAN isDetail,
                                     UINT32 addInfoMask,
                                     const BSONObj obj )
   {
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_ASSERT( dmsCB, "DMSCB can't be NULL" ) ;

      INT32 rc = SDB_OK ;
      _addInfoMask = addInfoMask ;
      _includeSys = isCurrent ;

      rc = dmsCB->dumpInfo( _suInfo, _includeSys ) ;
      _hitEnd = _suInfo.empty() ? TRUE : FALSE ;

      return rc ;
   }

   const CHAR* _monStorageUnitFetch::getName() const
   {
      return CMD_NAME_LIST_STORAGEUNITS ;
   }

   INT32 _monStorageUnitFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetchNext( obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monStorageUnitFetch::_fetchNext( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _suInfo.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;
         MON_SU_LIST::iterator it ;

         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;

         it = _suInfo.begin() ;
         const monStorageUnit &su = *it ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         /// add name & space name
         ob.append ( FIELD_NAME_NAME, su._name ) ;
         ob.append ( FIELD_NAME_UNIQUEID, su._csUniqueID ) ;
         ob.append ( FIELD_NAME_ID, su._CSID ) ;
         ob.append ( FIELD_NAME_LOGICAL_ID, su._logicalCSID ) ;
         ob.append ( FIELD_NAME_PAGE_SIZE, su._pageSize ) ;
         ob.append ( FIELD_NAME_LOB_PAGE_SIZE, su._lobPageSize ) ;
         ob.append ( FIELD_NAME_SEQUENCE, su._sequence ) ;
         ob.append ( FIELD_NAME_NUMCOLLECTIONS, su._numCollections ) ;
         ob.append ( FIELD_NAME_COLLECTIONHWM, su._collectionHWM ) ;
         ob.append ( FIELD_NAME_SIZE, su._size ) ;

         ossMillisecondsToString( su._createTime, timestamp ) ;
         ob.append( FIELD_NAME_CREATE_TIME, timestamp ) ;
         ossMillisecondsToString( su._updateTime, timestamp ) ;
         ob.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

         obj = ob.done() ;

         /// remove the current
         _suInfo.erase( it ) ;
         if ( _suInfo.empty() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for "
                  "storageunits: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monIndexFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monIndexFetch )

   _monIndexFetch::_monIndexFetch()
      : rtnFetchBase( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_INDEX ),
        _addInfoMask( 0 ),
        _collection( NULL )
   {
   }

   _monIndexFetch::~_monIndexFetch()
   {
   }

   INT32 _monIndexFetch::init( pmdEDUCB *cb,
                               BOOLEAN isCurrent,
                               BOOLEAN isDetail,
                               UINT32 addInfoMask,
                               const BSONObj obj )
   {
      INT32 rc = SDB_OK ;
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      rc = rtnGetStringElement( obj, FIELD_NAME_COLLECTION, &_collection ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_COLLECTION, rc ) ;

      rc = _dumpIndexInfo( _collection ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monIndexFetch::getName() const
   {
      return CMD_NAME_LIST_INDEXES ;
   }

   INT32 _monIndexFetch::_dumpIndexInfo( const CHAR* collection )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      dmsStorageUnit *su = NULL ;
      const CHAR* clShortName = NULL ;

      // lock collection space
      rc = rtnResolveCollectionNameAndLock( collection, dmsCB, &su,
                                            &clShortName, suID, SHARED ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to resolve collection[%s] and lock collection "
                   "space, rc: %d", collection, rc ) ;

      rc = su->getIndexes ( clShortName, _indexList ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get indexes from collection[%s], rc: %d",
                   collection, rc ) ;

      _it = _indexList.begin() ;

   done:
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
         suID = DMS_INVALID_CS ;
         su = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _monIndexFetch::_formatIndexInfo( const CHAR* collection,
                                          const monIndex& index,
                                          BSONObjBuilder& ob )
   {
      try
      {
         INT64 idxUniqID = 0 ;
         const BSONObj &indexDef = index._indexDef ;

         BSONObjBuilder sub( ob.subobjStart( IXM_FIELD_NAME_INDEX_DEF ) );

         sub.append( IXM_NAME_FIELD,
                     indexDef.getStringField( IXM_NAME_FIELD ) ) ;
         sub.append( DMS_ID_KEY_NAME,
                     indexDef.getField( DMS_ID_KEY_NAME ).OID() ) ;
         if ( indexDef.hasField( FIELD_NAME_UNIQUEID ) )
         {
            idxUniqID = indexDef.getField( FIELD_NAME_UNIQUEID ).numberLong() ;
            sub.append( FIELD_NAME_UNIQUEID, idxUniqID ) ;
         }
         sub.append( IXM_KEY_FIELD,
                     indexDef.getObjectField( IXM_KEY_FIELD ) ) ;
         sub.append( IXM_V_FIELD,
                     indexDef.getField( IXM_V_FIELD ).numberInt() ) ;
         sub.append( IXM_UNIQUE_FIELD,
                     indexDef.getBoolField( IXM_UNIQUE_FIELD ) ) ;
         sub.append( IXM_DROPDUP_FIELD,
                     indexDef.getBoolField( IXM_DROPDUP_FIELD ) ) ;
         sub.append( IXM_ENFORCED_FIELD,
                     indexDef.getBoolField( IXM_ENFORCED_FIELD ) ) ;
         sub.append( IXM_NOTNULL_FIELD,
                     indexDef.getBoolField( IXM_NOTNULL_FIELD ) ) ;
         if( 0 == ossStrcmp( indexDef.getStringField( IXM_NAME_FIELD ),
                             IXM_ID_KEY_NAME ) )
         {
            sub.append ( IXM_NOTARRAY_FIELD, true ) ;
         }
         else
         {
            sub.append ( IXM_NOTARRAY_FIELD,
                         indexDef.getBoolField( IXM_NOTARRAY_FIELD ) ) ;
         }

         BOOLEAN isGlobal = indexDef.getBoolField( IXM_FIELD_NAME_GLOBAL ) ;
         sub.appendBool( IXM_FIELD_NAME_GLOBAL, isGlobal ) ;
         if ( isGlobal )
         {
            sub.append( IXM_FIELD_NAME_GLOBAL_OPTION,
                        indexDef.getObjectField( IXM_FIELD_NAME_GLOBAL_OPTION )
                      ) ;
         }

         if ( indexDef.hasField( FIELD_NAME_UNIQUEID ) )
         {
            sub.appendBool( IXM_FIELD_NAME_STANDALONE,
                            utilIsStandaloneIdx( (utilIdxUniqueID)idxUniqID ) ) ;
         }

         BSONObj range = indexDef.getObjectField( IXM_2DRANGE_FIELD ) ;
         if ( !range.isEmpty() )
         {
            sub.append( IXM_2DRANGE_FIELD, range ) ;
         }
         sub.done () ;

         ob.append( IXM_FIELD_NAME_INDEX_FLAG,
                    ixmGetIndexFlagDesp( index._indexFlag ) ) ;

         if ( IXM_INDEX_FLAG_CREATING == index._indexFlag )
         {
            ob.append( IXM_FIELD_NAME_SCAN_EXTLID, index._scanRID._extent ) ;
            ob.append( IXM_FIELD_NAME_SCAN_EXTLID, index._scanRID._offset ) ;
         }

         UINT16 idxType = IXM_EXTENT_TYPE_NONE ;
         index.getIndexType( idxType ) ;
         ob.append( FIELD_NAME_TYPE, ixmGetIndexTypeDesp( idxType ) ) ;

         const CHAR *extDataName = index.getExtDataName() ;
         if ( ossStrlen( extDataName ) > 0 )
         {
            ob.append( FIELD_NAME_EXT_DATA_NAME, extDataName ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   }

   INT32 _monIndexFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd || _it == _indexList.end() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder ) ;
         monAppendSystemInfo( ob, _addInfoMask ) ;
         _formatIndexInfo( _collection, *_it, ob ) ;
         obj = ob.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      ++_it ;
      if ( _it == _indexList.end() )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monCLBlockFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCLBlockFetch )

   _monCLBlockFetch::_monCLBlockFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ * 4, RTN_FETCH_DATABLOCK )
   {
      _addInfoMask   = 0 ;
      _pos           = 0 ;
   }

   _monCLBlockFetch::~_monCLBlockFetch()
   {
   }

   INT32 _monCLBlockFetch::init( pmdEDUCB *cb,
                                 BOOLEAN isCurrent,
                                 BOOLEAN isDetail,
                                 UINT32 addInfoMask,
                                 const BSONObj obj )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_ASSERT( dmsCB, "DMSCB can't be NULL" ) ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *pCollectionName = NULL ;
      const CHAR *pCollectionShortName = NULL ;

      _addInfoMask = addInfoMask ;

      try
      {
         BSONElement e = obj.getField( FIELD_NAME_NAME ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_NAME, obj.toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         pCollectionName = e.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollectionName, rc ) ;
         goto error ;
      }
      rc = su->getSegExtents ( pCollectionShortName, _vecBlock, NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Get collection[%s] segment extents failed, "
                  "rc: %d", pCollectionShortName, rc ) ;
         goto error ;
      }

      _hitEnd = _vecBlock.empty() ? TRUE : FALSE ;
      _pos = 0 ;

   done:
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monCLBlockFetch::getName() const
   {
      return CMD_NAME_GET_DATABLOCKS ;
   }

   INT32 _monCLBlockFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetchNext( obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monCLBlockFetch::_fetchNext( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      UINT32 datablockNum = 0 ;
      const UINT32 c_maxARecordNum = 500 ;

      if ( _pos >= _vecBlock.size() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         _builder.reset();
         BSONObjBuilder ob( _builder ) ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SCANTYPE, VALUE_NAME_TBSCAN ) ;

         BSONArrayBuilder sub( ob.subarrayStart( FIELD_NAME_DATABLOCKS ) ) ;

         while( _pos < _vecBlock.size() &&
                datablockNum < c_maxARecordNum )
         {
            sub.append( _vecBlock[ _pos++ ] ) ;
            ++datablockNum ;
         }
         sub.done() ;

         obj = ob.done() ;

         if ( _pos >= _vecBlock.size() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for "
                  "data blocks: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monBackupFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monBackupFetch )

   _monBackupFetch::_monBackupFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_BACKUP )
   {
      _addInfoMask   = 0 ;
      _pos           = 0 ;
   }

   _monBackupFetch::~_monBackupFetch()
   {
   }

   INT32 _monBackupFetch::init( pmdEDUCB *cb,
                                BOOLEAN isCurrent,
                                BOOLEAN isDetail,
                                UINT32 addInfoMask,
                                const BSONObj obj )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      const CHAR *pPath = NULL ;
      const CHAR *backupName = NULL ;
      BOOLEAN isSubDir        = FALSE ;
      const CHAR *prefix      = NULL ;
      string bkpath ;
      BOOLEAN detail = FALSE ;
      barBackupMgr bkMgr( krcb->getGroupName() ) ;

      _addInfoMask = addInfoMask ;
      OSS_BIT_CLEAR( _addInfoMask, MON_MASK_NODE_NAME ) ;
      OSS_BIT_CLEAR( _addInfoMask, MON_MASK_GROUP_NAME ) ;

      rc = rtnGetStringElement( obj, FIELD_NAME_PATH, &pPath ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PATH, rc ) ;

      rc = rtnGetStringElement( obj, FIELD_NAME_NAME, &backupName ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_NAME, rc ) ;
      rc = rtnGetBooleanElement( obj, FIELD_NAME_DETAIL, detail ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_DETAIL, rc ) ;

      // option config
      rc = rtnGetBooleanElement( obj, FIELD_NAME_ISSUBDIR, isSubDir ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_ISSUBDIR, rc ) ;

      rc = rtnGetStringElement( obj, FIELD_NAME_PREFIX, &prefix ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   FIELD_NAME_PREFIX, rc ) ;

      // make path
      if ( isSubDir && pPath )
      {
         bkpath = rtnFullPathName( pmdGetOptionCB()->getBkupPath(), pPath ) ;
      }
      else if ( pPath && 0 != pPath[0] )
      {
         bkpath = pPath ;
      }
      else
      {
         bkpath = pmdGetOptionCB()->getBkupPath() ;
      }

      rc = bkMgr.init( bkpath.c_str(), backupName, prefix ) ;
      PD_RC_CHECK( rc, PDWARNING, "Init backup manager failed, rc: %d", rc ) ;

      // list
      rc = bkMgr.list( _vecBackup, detail ) ;
      PD_RC_CHECK( rc, PDWARNING, "List backup failed, rc: %d", rc ) ;

      _hitEnd = _vecBackup.empty() ? TRUE : FALSE ;
      _pos = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monBackupFetch::getName() const
   {
      return CMD_NAME_LIST_BACKUPS ;
   }

   INT32 _monBackupFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetchNext( obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monBackupFetch::_fetchNext( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _pos >= _vecBackup.size() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( 0 == _addInfoMask )
      {
         obj = _vecBackup[ _pos++ ] ;
      }
      else
      {
         try
         {
            _builder.reset() ;
            BSONObjBuilder ob( _builder ) ;

            /// add system info
            monAppendSystemInfo( ob, _addInfoMask ) ;
            ob.appendElements( _vecBackup[ _pos++ ] ) ;

            obj = ob.done() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON objects for "
                     "backups: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( _pos >= _vecBackup.size() )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monCachedPlanFetch implement
    */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monAccessPlansFetch )
   _monAccessPlansFetch::_monAccessPlansFetch ()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_ACCESSPLANS )
   {
      _pos = 0 ;
   }

   _monAccessPlansFetch::~_monAccessPlansFetch ()
   {
   }

   INT32 _monAccessPlansFetch::init ( pmdEDUCB *cb,
                                      BOOLEAN isCurrent,
                                      BOOLEAN isDetail,
                                      UINT32 addInfoMask,
                                      const BSONObj obj )
   {
      INT32 rc = SDB_OK ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      _SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      optAccessPlanManager *apm = rtnCB->getAPM() ;

      try
      {
         if ( apm->isInitialized() )
         {
            optAccessPlanCache *planCache = apm->getPlanCache() ;

            if ( addInfoMask != 0 )
            {
               BSONObjBuilder infoBuilder ;

               /// add system info
               rc = monAppendSystemInfo( infoBuilder, addInfoMask ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append system info, "
                            "rc: %d", rc ) ;

               _sysInfo = infoBuilder.obj() ;
               _addInfoMask = addInfoMask ;
            }

            rc = planCache->getCachedPlanList( _cachedPlanList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get list of cached plans, "
                         "rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to create BSON objects for access plans: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _hitEnd = _cachedPlanList.empty() ? TRUE : FALSE ;
      _pos = 0 ;

   done :
      return rc ;
   error :
      _cachedPlanList.clear() ;
      goto done ;
   }

   const CHAR* _monAccessPlansFetch::getName () const
   {
      return CMD_NAME_SNAPSHOT_ACCESSPLANS ;
   }

   INT32 _monAccessPlansFetch::fetch ( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetchNext( obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _monAccessPlansFetch::_fetchNext ( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _pos >= _cachedPlanList.size() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( 0 == _addInfoMask )
      {
         obj = _cachedPlanList[ _pos++ ] ;
      }
      else
      {
         try
         {
            _builder.reset() ;
            BSONObjBuilder builder( _builder ) ;

            /// add system info
            builder.appendElements( _sysInfo ) ;
            builder.appendElements( _cachedPlanList[ _pos++ ] ) ;

            obj = builder.done() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON objects for "
                     "backups: %s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( _pos >= _cachedPlanList.size() )
      {
         _hitEnd = TRUE ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   /*
      _monConfigFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monConfigsFetch )

   _monConfigsFetch::_monConfigsFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_CONFIGS )
   {
      _addInfoMask   = 0 ;
      _isLocalMode   = FALSE ;
      _isExpand      = TRUE ;
   }

   _monConfigsFetch::~_monConfigsFetch()
   {
   }

   INT32 _monConfigsFetch::init( pmdEDUCB *cb,
                                 BOOLEAN isCurrent,
                                 BOOLEAN isDetail,
                                 UINT32 addInfoMask,
                                 const BSONObj obj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( obj ) ;
         BSONElement elem ;
         while ( itr.more() )
         {
            elem = itr.next() ;
            // ignore case for backward compatibility
            if ( 0 == ossStrcasecmp( elem.fieldName(), FIELD_NAME_MODE ) )
            {
               if ( 0 == ossStrcmp( elem.valuestr(), VALUE_NAME_LOCAL ) )
               {
                  _isLocalMode = TRUE ;
               }
            }
            else if ( 0 == ossStrcasecmp( elem.fieldName(), FIELD_NAME_EXPAND ) )
            {
               if ( elem.type() == bson::Bool )
               {
                  _isExpand = elem.boolean() ;
               }
               else if ( elem.type() == bson::String )
               {
                  ossStrToBoolean( elem.valuestr(), &_isExpand ) ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to read configs options ",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monConfigsFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_CONFIGS ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MONCONFIGSFETCH_FETCH, "_monConfigsFetch::fetch" )
   INT32 _monConfigsFetch::fetch( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__MONCONFIGSFETCH_FETCH ) ;
      INT32 rc             = SDB_OK ;
      INT32 mask           = 0 ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else
      {
         _builder.reset() ;
         BSONObjBuilder ob( _builder ) ;
         BSONObj tmpObj ;

         /// add system info
         monAppendSystemInfo( ob, _addInfoMask ) ;
         if ( !_isExpand )
         {
            mask |= PMD_CFG_MASK_SKIP_UNFIELD ;
         }
         if ( _isLocalMode )
         {
            mask |= PMD_CFG_MASK_MODE_LOCAL ;
         }

         rc = pmdGetOptionCB()->toBSON( tmpObj, mask ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to generate config, rc: %d", rc ) ;
            goto error ;
         }
         ob.appendElements( tmpObj ) ;
         obj = ob.done() ;

         _hitEnd = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__MONCONFIGSFETCH_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _monVCLSessionInfoFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monVCLSessionInfoFetch )
   _monVCLSessionInfoFetch::_monVCLSessionInfoFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_VCL_SESSIONINFO )
   {
   }

   _monVCLSessionInfoFetch::~_monVCLSessionInfoFetch()
   {
   }

   INT32 _monVCLSessionInfoFetch::init( pmdEDUCB *cb,
                                        BOOLEAN isCurrent,
                                        BOOLEAN isDetail,
                                        UINT32 addInfoMask,
                                        const BSONObj obj )
   {
      ISession *pSession = cb->getSession() ;

      if ( pSession )
      {
         schedItem *pItem = ( schedItem* )pSession->getSchedItemPtr() ;
         _info = pItem->_info.toBSON() ;
         _hitEnd = FALSE ;

         if ( cb->getMonAppCB()->getSvcTaskInfo() )
         {
            _builder.reset() ;
            BSONObjBuilder builder( _builder ) ;
            builder.appendElements( _info ) ;
            monDumpSvcTaskInfo( builder,
                                cb->getMonAppCB()->getSvcTaskInfo(),
                                TRUE ) ;
            _info = builder.done() ;
         }
      }
      else
      {
         _hitEnd = TRUE ;
      }

      return SDB_OK ;
   }

   const CHAR* _monVCLSessionInfoFetch::getName() const
   {
      return SYS_CL_SESSION_INFO ;
   }

   INT32 _monVCLSessionInfoFetch::fetch( BSONObj & obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
      }
      else
      {
         obj = _info ;
         _hitEnd = TRUE ;
      }

      return rc ;
   }

   IMPLEMENT_FETCH_AUTO_REGISTER( _monSvcTasksFetch )
   /*
      _monSvcTasksFetch implement
   */
   _monSvcTasksFetch::_monSvcTasksFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_SVCTASKS )
   {
      _addInfoMask = 0 ;
      _isDetail= TRUE ;
   }

   _monSvcTasksFetch::~_monSvcTasksFetch()
   {
   }

   INT32 _monSvcTasksFetch::init( pmdEDUCB *cb,
                                  BOOLEAN isCurrent,
                                  BOOLEAN isDetail,
                                  UINT32 addInfoMask,
                                  const BSONObj obj )
   {
      schedTaskMgr *pMgr = pmdGetKRCB()->getSvcTaskMgr() ;

      _isDetail = isDetail ;
      _addInfoMask = addInfoMask ;
      _mapSvcTask = pMgr->getTaskInfos() ;

      if ( _mapSvcTask.empty() )
      {
         _hitEnd = TRUE ;
      }
      else
      {
         _hitEnd = FALSE ;
      }

      return SDB_OK ;
   }

   const CHAR* _monSvcTasksFetch::getName() const
   {
      return _isDetail ? CMD_NAME_SNAPSHOT_SVCTASKS :
                         CMD_NAME_LIST_SVCTASKS ;
   }

   INT32 _monSvcTasksFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      MAP_SVCTASKINFO_PTR_IT it ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
      }
      else
      {
         _builder.reset() ;
         BSONObjBuilder builder( _builder ) ;
         it = _mapSvcTask.begin() ;

         monAppendSystemInfo( builder, _addInfoMask ) ;

         if ( _isDetail )
         {
            monDumpSvcTaskInfo( builder, it->second.get() ) ;
         }
         else
         {
            const monSvcTaskInfo *pInfo = it->second.get() ;
            builder.append( FIELD_NAME_TASK_ID, (INT64)pInfo->getTaskID() ) ;
            builder.append( FIELD_NAME_TASK_NAME, pInfo->getTaskName() ) ;
         }
         obj = builder.done() ;

         _mapSvcTask.erase( it ) ;
         _hitEnd = _mapSvcTask.empty() ? TRUE : FALSE ;
      }

      return rc ;
   }

   IMPLEMENT_FETCH_AUTO_REGISTER( _monQueriesFetch )
   /*
      _monQueriesFetch implement
   */
   _monQueriesFetch::_monQueriesFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_QUERIES )
   {
      _addInfoMask = MON_MASK_NODE_NAME | MON_MASK_NODEID ;
      _viewArchive = FALSE ;
      _isDetail= TRUE ;
   }

   _monQueriesFetch::~_monQueriesFetch()
   {
   }

   INT32 _monQueriesFetch::init( pmdEDUCB *cb,
                                 BOOLEAN isCurrent,
                                 BOOLEAN isDetail,
                                 UINT32 addInfoMask,
                                 const BSONObj obj )
   {
      INT32 rc = SDB_OK ;

      monMonitorManager *monMgr = pmdGetKRCB()->getMonMgr() ;

      rc =  monParseArchiveOpt( obj, _viewArchive ) ;

      if ( rc != SDB_OK )
      {
         goto error ;
      }

      if ( _viewArchive )
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_QUERY, MON_CLASS_ARCHIVED_LIST) ;
      }
      else
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_QUERY, MON_CLASS_ACTIVE_LIST) ;
      }

      _hitEnd = _cachedMonClassList.empty() ? TRUE : FALSE ;
      _itr = _cachedMonClassList.begin() ;
      _isDetail = isDetail ;
      _addInfoMask = addInfoMask ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monQueriesFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_QUERIES ;
   }

   INT32 _monQueriesFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      ossTimestamp createTS = _itr->getCreateTS() ;
      ossTimestamp endTS = _itr->getEndTS() ;
      CHAR   timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
      UINT32 seconds, microseconds ;
      FLOAT64 responseTime ;
      FLOAT64 latchWaitTime ;
      FLOAT64 lockWaitTime ;
      ossTickConversionFactor factor ;
      SDB_ROLE role = pmdGetKRCB()->getDBRole() ;
      CHAR queryIDStr[MSG_QUERY_ID_HEX_STR_LEN+1] = { 0 } ;

      try
      {
         _builder.reset() ;
         BSONObjBuilder builder( _builder ) ;

         ossTimestampToString ( createTS, timestamp ) ;
         _itr->responseTime.convertToTime ( factor, seconds, microseconds ) ;
         responseTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

         /// add system info
         rc = monAppendSystemInfo( builder, _addInfoMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Append system info failed, rc: %d",
                      rc ) ;

         builder.append( FIELD_NAME_STARTTIMESTAMP, timestamp ) ;
         if ( endTS.time != 0 )
         {
            ossTimestampToString ( endTS, timestamp ) ;
            builder.append( FIELD_NAME_ENDTIMESTAMP, timestamp ) ;
         }
         else
         {
            builder.append( FIELD_NAME_ENDTIMESTAMP, VALUE_NAME_EMPTYENDTIMESTAMP ) ;
         }
         builder.append( FIELD_NAME_TID, _itr->tid ) ;
         //FIXME: PASSING IN FALSE AS DEFAULT
         builder.append( FIELD_NAME_OPTYPE,
                         msgType2String( (MSG_TYPE)_itr->opCode, FALSE ) ) ;

         builder.append( FIELD_NAME_NAME, _itr->name.c_str() ) ;
         builder.append( FIELD_NAME_QUERYTIMESPENT, responseTime ) ;
         builder.append( FIELD_NAME_RETURN_NUM, _itr->rowsReturned ) ;
         _itr->queryID.toHexStr( queryIDStr, sizeof(queryIDStr)-1 ) ;
         builder.append( FIELD_NAME_QUERY_ID, queryIDStr ) ;

         if ( SDB_ROLE_COORD == role )
         {
            FLOAT64 nodeWaitTime ;
            FLOAT64 msgSentTime ;
            BSONObjBuilder clientInfoBuilder ;
            _itr->remoteNodesResponseTime.convertToTime ( factor, seconds, microseconds ) ;
            nodeWaitTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

            _itr->msgSentTime.convertToTime ( factor, seconds, microseconds ) ;
            msgSentTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;
            builder.append( FIELD_NAME_NUM_MSG_SENT, _itr->numMsgSent ) ;
            builder.append( FIELD_NAME_LASTOPINFO, _itr->queryText.c_str() ) ;
            builder.append( FIELD_NAME_MSG_SENT_TIME, msgSentTime ) ;
            builder.append( FIELD_NAME_NODEWAITTIME, nodeWaitTime ) ;
            clientInfoBuilder.appendElements( _itr->clientInfo ) ;
            clientInfoBuilder.append( FIELD_NAME_CLIENTTID, _itr->clientTID ) ;
            clientInfoBuilder.append( FIELD_NAME_CLIENTHOST, _itr->clientHost.c_str() ) ;
            builder.append( FIELD_NAME_CLIENTINFO, clientInfoBuilder.obj() ) ;

            if ( _itr->nodes.size() > 0 )
            {
               /// add nodes
               BSONArrayBuilder ba( builder.subarrayStart( FIELD_NAME_RELATED_NODE ) ) ;
               ossPoolSet<UINT32>::const_iterator nodeItr ;
               for ( nodeItr = _itr->nodes.begin() ;
                     nodeItr != _itr->nodes.end() ;
                     ++nodeItr )
               {
                  ba.append( *nodeItr ) ;
               }
               ba.done() ;

            }
         }
         else
         {
            _itr->latchWaitTime.convertToTime ( factor, seconds, microseconds ) ;
            latchWaitTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

            _itr->lockWaitTime.convertToTime ( factor, seconds, microseconds ) ;
            lockWaitTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

            builder.append( FIELD_NAME_RELATED_NID, _itr->relatedNID.columns.nodeID ) ;
            builder.append( FIELD_NAME_RELATED_TID, _itr->relatedTID ) ;
            builder.append( FIELD_NAME_SESSIONID, _itr->sessionID ) ;
            builder.append( FIELD_NAME_ACCESSPLAN_ID, _itr->accessPlanID ) ;
            builder.append( FIELD_NAME_DATAREAD, _itr->dataRead ) ;
            builder.append( FIELD_NAME_DATAWRITE, _itr->dataWrite ) ;
            builder.append( FIELD_NAME_INDEXREAD, _itr->indexRead ) ;
            builder.append( FIELD_NAME_INDEXWRITE, _itr->indexWrite ) ;
            builder.append( FIELD_NAME_LOBREAD, _itr->lobRead ) ;
            builder.append( FIELD_NAME_LOBWRITE, _itr->lobWrite ) ;
            builder.append( FIELD_NAME_LOBTRUNCATE, _itr->lobTruncate ) ;
            builder.append( FIELD_NAME_LOBADDRESSING, _itr->lobAddressing ) ;
            builder.append( FIELD_NAME_TRANS_WAITLOCKTIME, lockWaitTime ) ;
            builder.append( FIELD_NAME_LATCH_WAIT_TIME, latchWaitTime ) ;
         }

         ++_itr ;
         if ( _itr == _cachedMonClassList.end() )
         {
            _hitEnd = TRUE ;
         }

         obj = builder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to create BSON for query, %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_FETCH_AUTO_REGISTER( _monLatchWaitsFetch )
   /*
      _monLatchWaitsFetch implement
   */
   _monLatchWaitsFetch::_monLatchWaitsFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_LATCHWAITS )
   {
      _addInfoMask = MON_MASK_NODE_NAME ;
      _viewArchive = FALSE ;
      _isDetail= TRUE ;
   }

   _monLatchWaitsFetch::~_monLatchWaitsFetch()
   {
   }

   INT32 _monLatchWaitsFetch::init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj )
   {
      INT32 rc = SDB_OK ;
      monMonitorManager *monMgr = pmdGetKRCB()->getMonMgr() ;

      rc =  monParseArchiveOpt( obj, _viewArchive ) ;
      if ( rc != SDB_OK )
      {
         goto error ;
      }

      if ( _viewArchive )
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_LATCH, MON_CLASS_ARCHIVED_LIST) ;
      }
      else
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_LATCH, MON_CLASS_ACTIVE_LIST) ;
      }

      _hitEnd = _cachedMonClassList.empty() ? TRUE : FALSE ;
      _itr = _cachedMonClassList.begin() ;
      _isDetail = isDetail ;
      _addInfoMask = addInfoMask ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monLatchWaitsFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_LATCHWAITS ;
   }

   INT32 _monLatchWaitsFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
      CHAR addr[16] = { 0 } ;
      UINT32 seconds, microseconds ;
      FLOAT64 waitTime ;
      ossTickConversionFactor factor ;

      try
      {
         _builder.reset() ;
         BSONObjBuilder builder( _builder ) ;

         ossTimestamp createTS = _itr->getCreateTS() ;
         ossTimestampToString ( createTS, timestamp ) ;

         if ( !_viewArchive )
         {
            ossTick now ;
            now.sample() ;
            ossTickDelta delta = now - _itr->getCreateTSTick() ;
            delta.convertToTime ( factor, seconds, microseconds ) ;
         }
         else
         {
            _itr->waitTime.convertToTime ( factor, seconds, microseconds ) ;
         }

         waitTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

         /// add system info
         rc = monAppendSystemInfo( builder, _addInfoMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Append system info failed, rc: %d",
                      rc ) ;

         builder.append( FIELD_NAME_WAITER_TID, _itr->waiterTID ) ;
         if ( EXCLUSIVE == _itr->latchMode )
         {
            builder.append( FIELD_NAME_REQUIRED_MODE, "X" ) ;
         }
         else
         {
            builder.append( FIELD_NAME_REQUIRED_MODE, "S" ) ;
         }
         builder.append( FIELD_NAME_LATCH_NAME, monLatchIDtoName(_itr->latchID) ) ;

         ossSnprintf( addr, sizeof(addr)-1, "%p", _itr->latchAddr ) ;
         builder.append( FIELD_NAME_ADDRESS, addr ) ;
         builder.append( FIELD_NAME_STARTTIMESTAMP, timestamp ) ;
         builder.append( FIELD_NAME_LATCH_WAIT_TIME, waitTime ) ;

         if ( _itr->xOwnerTID )
         {
            builder.append( FIELD_NAME_LATEST_OWNER, _itr->xOwnerTID ) ;
            builder.append( FIELD_NAME_LATEST_OWNER_MODE, "X" ) ;
         }
         else
         {
            builder.append( FIELD_NAME_LATEST_OWNER, _itr->lastSOwner ) ;
            builder.append( FIELD_NAME_LATEST_OWNER_MODE, "S" ) ;
         }
         builder.append( FIELD_NAME_NUM_OWNER, _itr->numOwner ) ;

         ++_itr ;
         if ( _itr == _cachedMonClassList.end() )
         {
             _hitEnd = TRUE ;
         }

         obj = builder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to create BSON for latch wait, %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_FETCH_AUTO_REGISTER( _monLockWaitsFetch )

   /*
      _monLockWaitsFetch implement
   */
   _monLockWaitsFetch::_monLockWaitsFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_LOCKWAITS )
   {
      _addInfoMask = MON_MASK_NODE_NAME ;
      _viewArchive = FALSE ;
      _isDetail= TRUE ;
   }

   _monLockWaitsFetch::~_monLockWaitsFetch()
   {
   }

   INT32 _monLockWaitsFetch::init( pmdEDUCB *cb,
                                   BOOLEAN isCurrent,
                                   BOOLEAN isDetail,
                                   UINT32 addInfoMask,
                                   const BSONObj obj )
   {
      INT32 rc = SDB_OK ;

      monMonitorManager *monMgr = pmdGetKRCB()->getMonMgr() ;

      rc =  monParseArchiveOpt( obj, _viewArchive ) ;
      if ( rc != SDB_OK )
      {
         goto error ;
      }
      if ( _viewArchive )
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_LOCK, MON_CLASS_ARCHIVED_LIST) ;
      }
      else
      {
         monMgr->dumpList( _cachedMonClassList, MON_CLASS_LOCK, MON_CLASS_ACTIVE_LIST) ;
      }

      _hitEnd = _cachedMonClassList.empty() ? TRUE : FALSE ;
      _itr = _cachedMonClassList.begin() ;
      _isDetail = isDetail ;
      _addInfoMask = addInfoMask ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monLockWaitsFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_LOCKWAITS ;
   }

   INT32 _monLockWaitsFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      UINT32 seconds, microseconds ;
      FLOAT64 waitTime ;
      ossTickConversionFactor factor ;
      CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;

      try
      {
         _builder.reset() ;
         BSONObjBuilder builder( _builder ) ;
         ossTimestamp createTS = _itr->getCreateTS() ;
         ossTimestampToString ( createTS, timestamp ) ;

         if ( !_viewArchive )
         {
            ossTick now ;
            now.sample() ;
            ossTickDelta delta = now - _itr->getCreateTSTick() ;
            delta.convertToTime ( factor, seconds, microseconds ) ;
         }
         else
         {
            _itr->waitTime.convertToTime ( factor, seconds, microseconds ) ;
         }

         waitTime = (FLOAT64)(seconds*1000) + ( (FLOAT64)(microseconds) / 1000) ;

         rc = monAppendSystemInfo( builder, _addInfoMask ) ;
         PD_RC_CHECK( rc, PDERROR, "Append system info failed, rc: %d",
                      rc ) ;

         builder.append( FIELD_NAME_WAITER_TID, _itr->waiterTID ) ;
         builder.append( FIELD_NAME_REQUIRED_MODE, lockModeToString( _itr->lockMode ) );
         _itr->lockID.toBson(builder) ;
         builder.append( FIELD_NAME_STARTTIMESTAMP, timestamp ) ;
         builder.append( FIELD_NAME_TRANS_WAITLOCKTIME, waitTime ) ;
         builder.append( FIELD_NAME_LATEST_OWNER, _itr->xOwnerTID ) ;
         builder.append( FIELD_NAME_LATEST_OWNER_MODE, "X" ) ;
         builder.append( FIELD_NAME_NUM_OWNER, _itr->numOwner ) ;

         _itr++ ;
         if ( _itr == _cachedMonClassList.end() )
         {
            _hitEnd = TRUE ;
         }

         obj = builder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to create BSON for lock wait, %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   IMPLEMENT_FETCH_AUTO_REGISTER( _monIndexStatsFetch )
   /*
      _monIndexStatsFetch implement
   */
   _monIndexStatsFetch::_monIndexStatsFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_INDEXSTATS )
   {
      _addInfoMask   = MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
      _isDetail      = TRUE ;
      _pos           = _statCache.end() ;
      _su            = NULL ;
      _suID          = DMS_INVALID_CS ;
      _mbContext     = NULL ;
      _scanner       = NULL ;
      _noMoreStat    = FALSE ;
   }

   _monIndexStatsFetch::~_monIndexStatsFetch()
   {
      rtnScannerFactory f ;
      f.releaseScanner( _scanner ) ;
      _scanner = NULL ;

      IDX_STAT_LIST::iterator iter ;
      for ( iter = _statCache.begin() ; iter != _statCache.end() ; ++iter )
      {
         SDB_POOL_FREE( *iter ) ;
      }
      _statCache.clear() ;

      if ( _mbContext )
      {
         if ( _mbContext->isMBLock() )
         {
            _mbContext->mbUnlock() ;
         }
         _su->data()->releaseMBContext( _mbContext ) ;
      }
      if ( DMS_INVALID_CS != _suID )
      {
         pmdGetKRCB()->getDMSCB()->suUnlock( _suID ) ;
      }
   }

   INT32 _monIndexStatsFetch::init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj )
   {
      int rc = SDB_OK ;
      const CHAR *pCollectionShortName = NULL ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_ASSERT( dmsCB, "dmsCB's never NULL" ) ;

      rtnScannerFactory f ;

      _isDetail      = isDetail ;
      _addInfoMask   = addInfoMask ;
      _hitEnd        = FALSE ;

      if ( pmdGetDBRole() != SDB_ROLE_DATA &&
           pmdGetDBRole() != SDB_ROLE_STANDALONE )
      {
         _hitEnd = TRUE ;
         goto done ;
      }

      // Check whether SYSSTAT.SYSINDEXSTAT exists. If not, nothing to do.
      rc = rtnResolveCollectionNameAndLock ( DMS_STAT_INDEX_CL_NAME, dmsCB, &_su,
                                             &pCollectionShortName, _suID, SHARED ) ;
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
         _hitEnd = TRUE ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name %s, rc: %d",
                  pCollectionShortName, rc ) ;

      rc = _su->data()->getMBContext( &_mbContext, pCollectionShortName, SHARED ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         rc = SDB_OK ;
         _hitEnd = TRUE ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;

      rc = f.createTBScanner( SCANNER_TYPE_DISK, _su, _mbContext, FALSE, cb, _scanner ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create scanner, rc: %d", rc ) ;

   done:
      if ( _mbContext )
      {
         _mbContext->pause() ;
      }
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _monIndexStatsFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_INDEXSTATS ;
   }

   // Read a bulk of statistics from SYSSTAT.SYSINDEXSTAT for once.
   INT32 _monIndexStatsFetch::_bulkFetch( IDX_STAT_LIST &result )
   {
      int rc            = SDB_OK ;
      pmdEDUCB *eduCB   = pmdGetThreadEDUCB() ;
      IDX_STAT_LIST::iterator iter ;
      BSONObj obj ;
      CHAR *buffer = NULL ;

   retry:
      if ( _noMoreStat )
      {
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      // Scan a whole extent for once
      try
      {
         dmsDataScanner scanner( _su->data(), _mbContext, _scanner, NULL ) ;
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;

         while( SDB_OK == ( rc = scanner.advance( recordID, generator, eduCB ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            BSONObj stat( (const CHAR*)recordDataPtr ) ;
            _builder.reset();
            BSONObjBuilder ob( _builder ) ;

            rc = monBuildIndexStatResult( stat, _addInfoMask, ob ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build statistics result, rc: %d", rc ) ;
            obj = ob.done() ;

            buffer = (CHAR *) SDB_POOL_ALLOC( obj.objsize() ) ;
            PD_CHECK( buffer, SDB_OOM, error, PDERROR,
                      "Failed to allocate buffer to save result" ) ;

            ossMemcpy( buffer, obj.objdata(), obj.objsize() ) ;
            result.push_back( buffer ) ;
            buffer = NULL ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            if ( _scanner->isEOF() )
            {
               _noMoreStat = TRUE ;
            }
         }

         PD_RC_CHECK( rc, PDERROR, "Failed to get next record, rc: %d", rc ) ;
      }
      catch ( std::bad_alloc )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "No memory to fetch index statistics" ) ;
         goto error ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexcepted exception occurred: %s", e.what() ) ;
         goto error ;
      }

      // If this extent is empty, try next extent.
      if ( SDB_OK == rc && result.empty() )
      {
         if ( _mbContext->isMBLock() )
         {
            _mbContext->pause() ;
         }
         goto retry ;
      }
   done:
      if ( buffer )
      {
         SDB_POOL_FREE( buffer ) ;
      }
      if ( _mbContext->isMBLock() )
      {
         _mbContext->pause() ;
      }
      return rc ;
   error:
      for ( iter = result.begin() ; iter != result.end() ; ++iter )
      {
         SDB_POOL_FREE( *iter ) ;
      }
      result.clear() ;
      goto done ;
   }

   INT32 _monIndexStatsFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      IDX_STAT_LIST::iterator iter ;

      if ( _statCache.end() == _pos )
      {
         for ( iter = _statCache.begin() ; iter != _statCache.end() ; ++iter )
         {
            SDB_POOL_FREE( *iter ) ;
         }
         _statCache.clear() ;

         rc = _bulkFetch( _statCache ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            goto done ;
         }
         else if ( rc != SDB_OK )
         {
            PD_LOG( PDERROR, "Fail to fetch index statistics, rc: %d", rc ) ;
            goto error ;
         }
         _pos = _statCache.begin() ;
      }

      obj = BSONObj( *_pos ) ;
      ++_pos ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monRecycleBinFetch implement
    */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monRecycleBinFetch )

   _monRecycleBinFetch::_monRecycleBinFetch()
   : rtnFetchBase( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_RECYCLEBIN ),
     _addInfoMask( 0 ),
     _subContextID( -1 ),
     _isDetail( FALSE )
   {
   }

   _monRecycleBinFetch::~_monRecycleBinFetch()
   {
      _closeSubContext() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH_INIT, "_monRecycleBinFetch::init" )
   INT32 _monRecycleBinFetch::init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH_INIT ) ;

      clsRecycleBinManager *recycleBinMgr = NULL ;
      BSONObj dummy ;

      _isDetail = isDetail ;
      _addInfoMask = addInfoMask ;
      _hitEnd = TRUE ;

      if ( pmdGetDBRole() != SDB_ROLE_DATA )
      {
         goto done ;
      }
      else if ( NULL == pmdGetKRCB()->getClsCB() ||
                NULL == pmdGetKRCB()->getClsCB()->getRecycleBinMgr() )
      {
         goto done ;
      }

      recycleBinMgr = pmdGetKRCB()->getClsCB()->getRecycleBinMgr() ;

      rc = recycleBinMgr->getItems( dummy, dummy, dummy, -1, cb, _subContextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open query for recycle items, "
                   "rc: %d", rc ) ;

      _hitEnd = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   const CHAR* _monRecycleBinFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_RECYCLEBIN ;
   }

   INT32 _monRecycleBinFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else if ( -1 == _subContextID && _recycleList.empty() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _recycleList.empty() )
      {
         rc = _prepareRecycleList() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare recycle list, rc: %d",
                      rc ) ;

         if ( _recycleList.empty() )
         {
            _hitEnd = TRUE ;
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }

      rc = _fetchRecycleItem( obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to fetch recycle item, rc: %d", rc ) ;

      // check hit end
      if ( -1 == _subContextID && _recycleList.empty() )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc ;

   error:
      _closeSubContext() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH_PREPARERECYLIST, "_monRecycleBinFetch::_prepareRecycleList" )
   INT32 _monRecycleBinFetch::_prepareRecycleList()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH_PREPARERECYLIST ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      rtnContextBuf buffObj ;

      while( TRUE )
      {
         rc = rtnGetMore( _subContextID, 1, buffObj, cb,
                          rtnCB ) ;
         if( SDB_DMS_EOC == rc )
         {
            _subContextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            _subContextID = -1 ;
            PD_LOG( PDERROR, "Failed to get recycle items, rc: %d", rc ) ;
            goto error ;
         }

         try
         {
            monRecycleItem recycleItem ;
            BSONObj object( buffObj.data() ) ;

            rc = recycleItem.fromBSON( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get recycle item from BSON, "
                         "rc: %d", rc ) ;

            _recycleList.push_back( recycleItem ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get recycle item, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         if ( _recycleList.size() > MON_MAX_SLICE_SIZE )
         {
            break ;
         }
      }

      if ( _isDetail )
      {
         rc = _fillRecycleStats() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to fill statistics for recycle "
                      "items, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH_PREPARERECYLIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH__FILLRECYSTAT, "_monRecycleBinFetch::_fillRecycleStats" )
   INT32 _monRecycleBinFetch::_fillRecycleStats()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH__FILLRECYSTAT ) ;

      MON_RECYCLE_LIST::iterator iter = _recycleList.begin() ;
      while ( iter != _recycleList.end() )
      {
         monRecycleItem &item = ( *iter ) ;
         if ( UTIL_RECYCLE_CS == item.getType() )
         {
            rc = _fillRecycleCSStats( item ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               // not exist anymore
               rc = SDB_OK ;
               iter = _recycleList.erase( iter ) ;
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to fill statistics for "
                         "recycle item [%s], rc: %d",
                         item.getRecycleName(), rc ) ;
         }
         else if ( UTIL_RECYCLE_CL == item.getType() )
         {
            rc = _fillRecycleCLStats( item ) ;
            if ( SDB_DMS_NOTEXIST == rc || SDB_DMS_CS_NOTEXIST == rc )
            {
               // not exist anymore
               rc = SDB_OK ;
               iter = _recycleList.erase( iter ) ;
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to fill statistics for "
                         "recycle item [%s], rc: %d",
                         item.getRecycleName(), rc ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Unsupported recycle type [%s]",
                    utilGetRecycleTypeName( item.getType() ) ) ;
            iter = _recycleList.erase( iter ) ;
            continue ;
         }
         ++ iter ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH__FILLRECYSTAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH__FILLRECYCSSTAT, "_monRecycleBinFetch::_fillRecycleCSStats" )
   INT32 _monRecycleBinFetch::_fillRecycleCSStats( monRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH__FILLRECYCSSTAT ) ;

      SDB_ASSERT( UTIL_RECYCLE_CS == item.getType(), "type is invalid" ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;

      rc = dmsCB->idToSUAndLock( (utilCSUniqueID)( item.getOriginID() ),
                                 suID, &su ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock storage unit [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

      rc = su->dumpRecycleInfo( item ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump recycle item [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH__FILLRECYCSSTAT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH__FETCHRECYCLECL, "_monRecycleBinFetch::_fillRecycleCLStats" )
   INT32 _monRecycleBinFetch::_fillRecycleCLStats( monRecycleItem &item )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH__FETCHRECYCLECL ) ;

      SDB_ASSERT( UTIL_RECYCLE_CL == item.getType(), "type is invalid" ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      utilCSUniqueID csUniqueID =
            utilGetCSUniqueID( (utilCLUniqueID)( item.getOriginID() ) ) ;

      rc = dmsCB->idToSUAndLock( csUniqueID, suID, &su ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock storage unit [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

      rc = su->dumpRecycleInfo( item ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump recycle item [%s], rc: %d",
                   item.getRecycleName(), rc ) ;

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH__FETCHRECYCLECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH_FETCHRECYCLEITEM, "_monRecycleBinFetch::_fetchRecycleItem" )
   INT32 _monRecycleBinFetch::_fetchRecycleItem( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH_FETCHRECYCLEITEM ) ;

      try
      {
         BSONObjBuilder builder ;

         monRecycleItem item = _recycleList.front() ;

         _recycleList.pop_front() ;

         rc = item.toBSON( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for recycle item, "
                      "rc: %d", rc ) ;

         if ( _isDetail )
         {
            rc = monAppendSystemInfo( builder, _addInfoMask ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build system info, rc: %d",
                         rc ) ;

            builder.append( FIELD_NAME_PAGE_SIZE, item._pageSize ) ;
            builder.append( FIELD_NAME_LOB_PAGE_SIZE, item._lobPageSize ) ;
            builder.append( FIELD_NAME_TOTAL_RECORDS, item._totalRecords ) ;
            builder.append( FIELD_NAME_TOTAL_LOBS, item._totalLobs ) ;
            builder.append( FIELD_NAME_TOTAL_DATA_SIZE, item._totalDataSize ) ;
            builder.append( FIELD_NAME_TOTAL_IDX_SIZE, item._totalIndexSize ) ;
            builder.append( FIELD_NAME_TOTAL_LOB_SIZE, item._totalLobSize ) ;
         }

         obj = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build recycle item BSON, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_MONRECYBINFETCH_FETCHRECYCLEITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRECYBINFETCH__CLOSESUBCTX, "_monRecycleBinFetch::_closeSubContext" )
   void _monRecycleBinFetch::_closeSubContext()
   {
      PD_TRACE_ENTRY( SDB_MONRECYBINFETCH__CLOSESUBCTX ) ;

      if ( -1 != _subContextID )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;

         rtnCB->contextDelete( _subContextID, cb ) ;
         _subContextID = -1 ;
      }

      PD_TRACE_EXIT( SDB_MONRECYBINFETCH__CLOSESUBCTX ) ;
   }

   /*
      _monDataSetFetch implement
    */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monDataSetFetch )

   _monDataSetFetch::_monDataSetFetch()
   : _rtnFetchBase( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_DATASET ),
     _dataSet( NULL )
   {
   }

   _monDataSetFetch::~_monDataSetFetch()
   {
      _clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDSFETCH_ATTACHCTX, "_monDataSetFetch::attachContext" )
   INT32 _monDataSetFetch::attachContext( INT64 contextID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONDSFETCH_ATTACHCTX ) ;

      _clear() ;

      _dataSet = SDB_OSS_NEW rtnDataSet( contextID, cb ) ;
      PD_CHECK( NULL != _dataSet, SDB_OOM, error, PDERROR,
                "Failed to allocate data set for context" ) ;

      _hitEnd = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB_MONDSFETCH_ATTACHCTX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDSFETCH__CLEAR, "_monDataSetFetch::_clear" )
   void _monDataSetFetch::_clear()
   {
      PD_TRACE_ENTRY( SDB_MONDSFETCH__CLEAR ) ;

      SAFE_OSS_DELETE( _dataSet ) ;
      _hitEnd = TRUE ;

      PD_TRACE_EXIT( SDB_MONDSFETCH__CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDSFETCH_FETCH, "_monDataSetFetch::fetch" )
   INT32 _monDataSetFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MONDSFETCH_FETCH ) ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else if ( NULL == _dataSet )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _dataSet->next( obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _hitEnd = TRUE ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get data from data set, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_MONDSFETCH_FETCH, rc ) ;
      return rc ;

   error:
      _clear() ;
      goto done ;
   }


   /*
      _monTransWaitsFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monTransWaitsFetch )

   _monTransWaitsFetch::_monTransWaitsFetch()
      : rtnFetchBase ( MON_DUMP_DFT_BUILDER_SZ, RTN_FETCH_TRANSWAITS )
   {
      _nodeId.reset() ;
      _addInfoMask = 0 ;
      _waitInfoSet.clear() ;
   }

   _monTransWaitsFetch::~_monTransWaitsFetch()
   {
      _waitInfoSet.clear() ;
   }

   INT32 _monTransWaitsFetch::init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj )
   {
      NodeID selfID = pmdGetNodeID() ;
      _nodeId.nodeID  = selfID.columns.nodeID ;
      _nodeId.groupID = selfID.columns.groupID ;
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      monTransInfo transInfo ;
      dpsTransCB * pTransCB = pmdGetKRCB()->getTransCB() ;
      pmdEDUMgr *  eduMgr   = pmdGetKRCB()->getEDUMgr() ;
      if ( pTransCB && eduMgr )
      {
         dpsTransLockManager *pLockMgr = pTransCB->getLockMgrHandle() ;
         if ( pLockMgr )
         {
            DPS_TX_WAIT_LRB_SET txWaitLRBSet ;
            // get lock waiter's LRB
            pTransCB->snapTransLockWaiterLRB( txWaitLRBSet ) ;
            if ( ! txWaitLRBSet.empty() )
            {
               _waitInfoSet.clear() ;

               // collect transaction waitng info for each waiting executor
               DPS_TX_WAIT_LRB_SET_IT it = txWaitLRBSet.begin() ;
               while ( it != txWaitLRBSet.end() )
               {
                  dpsTxWaitLRB txWaiterLRB = *it ;

                  pmdTransExecutor *pExecutor = NULL ;
                  if ( SDB_OK == eduMgr->beginDumpEDUTrans( txWaiterLRB.eduID,
                                                            &pExecutor,
                                                            transInfo,
                                                            FALSE ) )
                  {
                     pLockMgr->snapWaitInfo( pExecutor,
                                             txWaiterLRB.pLRB,
                                             txWaiterLRB.lockId,
                                             _waitInfoSet ) ;
                     eduMgr->endDumpEDUTrans( txWaiterLRB.eduID ) ;
                  }
                  it++;
               }
            }
            txWaitLRBSet.clear();
         }
      }
      _hitEnd = _waitInfoSet.empty() ? TRUE : FALSE ;
      return SDB_OK ;
   }

   const CHAR* _monTransWaitsFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_TRANSWAITS ;
   }

   INT32 _monTransWaitsFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = _fetchNext( obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _monTransWaitsFetch::_fetchNext( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _waitInfoSet.empty() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      // convert waitInfo set into a BSONObj
      try
      {
         _builder.reset() ;
         BSONObjBuilder ob ( _builder ) ;
         CHAR strTransID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;

         DPS_TRANS_WAIT_SET_IT itr = _waitInfoSet.begin();
         if ( itr != _waitInfoSet.end() )
         {
            dpsTransWait info = *itr ;
            // system info, nodeName
            monAppendSystemInfo( ob, _addInfoMask ) ;
            // groupID
            ob.append( FIELD_NAME_GROUPID,(INT32)_nodeId.groupID );
            // nodeID
            ob.append( FIELD_NAME_NODEID,(INT32)_nodeId.nodeID );
            // wait time
            ob.append( FIELD_NAME_WAITTIME, (INT64)info.waitTime ) ;
            // waiter transId
            dpsTransIDToString( info.waiter, strTransID, DPS_TRANS_STR_LEN ) ;
            ob.append( FIELD_NAME_WAITER_TRANSID, strTransID ) ;
            // holder transId
            dpsTransIDToString( info.holder, strTransID, DPS_TRANS_STR_LEN ) ;
            ob.append( FIELD_NAME_HOLDER_TRANSID, strTransID ) ;
            // waiter trans cost
            ob.append( FIELD_NAME_WAITER_TRANS_COST,(INT64)info.waiterCost );
            // holder trans cost
            ob.append( FIELD_NAME_HOLDER_TRANS_COST,(INT64)info.holderCost );
            // waiter sessionID
            ob.append( FIELD_NAME_WAITER_SESSIONID,(INT64)info.waiterSessionID);
            // holder sessionID
            ob.append( FIELD_NAME_HOLDER_SESSIONID,(INT64)info.holderSessionID);
            // waiter RelatedID
            monAppendSessionIdentify( ob,
                                      info.waiterRelatedID,
                                      info.waiterRelatedSessionID,
                                      FIELD_NAME_WAITER_RELATED_ID ) ;
            // holder RelatedID
            monAppendSessionIdentify( ob,
                                      info.holderRelatedID,
                                      info.holderRelatedSessionID,
                                      FIELD_NAME_HOLDER_RELATED_ID ) ;
            // waiter related sessionID
            ob.append( FIELD_NAME_WAITER_RELATED_SESSIONID,
                       (INT64)info.waiterRelatedSessionID ) ;
            // holder related sessionID
            ob.append( FIELD_NAME_HOLDER_RELATED_SESSIONID,
                       (INT64)info.holderRelatedSessionID ) ;
            // waiter related GroupID
            ob.append( FIELD_NAME_WAITER_RELATED_GROUPID,
                       (INT32)info.waiterRelatedNID.columns.groupID );
            // holder related GroupID
            ob.append( FIELD_NAME_HOLDER_RELATED_GROUPID,
                       (INT32)info.holderRelatedNID.columns.groupID );
            // waiter related NodeID
            ob.append( FIELD_NAME_WAITER_RELATED_NODEID,
                       (INT32)info.waiterRelatedNID.columns.nodeID );
            // holder related NodeID
            ob.append( FIELD_NAME_HOLDER_RELATED_NODEID,
                       (INT32)info.holderRelatedNID.columns.nodeID );

            _waitInfoSet.erase( itr ) ;
         }
         obj = ob.done();
         if ( _waitInfoSet.size() == 0 )
         {
            _hitEnd = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception captured: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}
