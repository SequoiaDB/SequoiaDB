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
#include "utilEnvCheck.hpp"
#include "pmdStartupHistoryLogger.hpp"

using namespace bson ;

#define OSS_MAX_SESSIONNAME ( OSS_MAX_HOSTNAME+OSS_MAX_SERVICENAME+30 )

namespace engine
{
   #define MON_MAX_SLICE_SIZE       ( 1000 )
   #define MON_TMP_STR_SZ           ( 64 )
   #define OSS_MAX_FILE_SZ          ( 8796093022208ll )

   #define MON_DUMP_DFT_BUILDER_SZ  ( 1024 )

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
         if ( MON_MASK_SERVICE_STATUS & mask )
         {
            ob.appendBool ( FIELD_NAME_SERVICE_STATUS,
                            PMD_IS_DB_AVAILABLE() ) ;
            ob.append( FIELD_NAME_GROUP_STATUS,
                       utilDBStatusStr( krcb->getDBStatus() ) ) ;
         }

         if ( MON_MASK_LSN_INFO & mask )
         {
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

            DPS_LSN completeLSN ;
            UINT32 lsnQueSize = 0 ;
            if ( pClsCB )
            {
               clsBucket *pBucket = pClsCB->getReplCB()->getBucket() ;
               completeLSN = pBucket->completeLSN() ;
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
         PD_LOG ( PDWARNING, "Failed to append hostname and servicename, %s",
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
      PD_TRACE_ENTRY ( SDB_MONAPPENDVERSION ) ;
      ossGetVersion ( &major, &minor, &fix, &release, &pBuild ) ;
      PD_TRACE4 ( SDB_MONAPPENDVERSION,
                  PD_PACK_INT ( major ),
                  PD_PACK_INT ( minor ),
                  PD_PACK_INT ( release ),
                  PD_PACK_STRING ( pBuild ) ) ;
      try
      {
         BSONObjBuilder obVersion( ob.subobjStart( FIELD_NAME_VERSION ) ) ;
         obVersion.append ( FIELD_NAME_MAJOR, major ) ;
         obVersion.append ( FIELD_NAME_MINOR, minor ) ;
         obVersion.append ( FIELD_NAME_FIX, fix ) ;
         obVersion.append ( FIELD_NAME_RELEASE, release ) ;
         obVersion.append ( FIELD_NAME_BUILD, pBuild ) ;
         obVersion.done() ;

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

      limInfo->getLimit( OSS_LIMIT_OPEN_FILE, softLimit, hardLimit ) ;
      totalNum = softLimit ;

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
      vector<pmdStartupLog> startLogs ;
      UINT32 logNum = 10 ;

      rc = pmdGetStartupHstLogger()->getLatestLogs( logNum, startLogs ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get logs from start-up history "
                   "logger, rc = %d", rc ) ;

      {
         vector<string> startHst, abnormalHst;
         vector<pmdStartupLog>::iterator i ;
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
      INT64 memAvailPhys   = 0 ;
      INT64 memTotalPF     = 0 ;
      INT64 memAvailPF     = 0 ;
      INT64 memTotalVirtual= 0 ;
      INT64 memAvailVirtual= 0 ;

      rc = ossGetMemoryInfo( memLoadPercent, memTotalPhys, memAvailPhys,
                             memTotalPF, memAvailPF,
                             memTotalVirtual, memAvailVirtual ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info, rc = %d", rc ) ;

      {
         BSONObjBuilder memOb( ob.subobjStart( FIELD_NAME_MEMORY ) ) ;
         memOb.append( FIELD_NAME_LOADPERCENT, memLoadPercent ) ;
         memOb.append( FIELD_NAME_TOTALRAM, memTotalPhys ) ;
         memOb.append( FIELD_NAME_FREERAM, memAvailPhys ) ;
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

      rc = ossGetMemoryInfo( loadPctRAM, totalRAM, availRAM,
                             totalSwap, availSwap, totalVM, availVM,
                             overCommitMode, commitLimit, committedAS ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info of host, "
                   "rc = %d", rc ) ;

      rc = ossGetProcessMemory( ossGetCurrentProcessID(), rss, allocatedVMProc );
      PD_RC_CHECK( rc, PDERROR, "Failed to get memory info of process, "
                   "rc = %d", rc ) ;

      limInfo->getLimit( OSS_LIMIT_VIRTUAL_MEM, softLimit, hardLimit ) ;

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

            dpscb->getLsnWindow( beginLSN, currentLSN, &expectLSN, NULL ) ;

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
      INT32 loadPercent    = 0 ;
      const CHAR *dbPath   = pmdGetOptionCB()->getDbPath () ;
      CHAR fsName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      rc = ossGetDiskInfo( dbPath, diskTotalBytes, diskFreeBytes,
                           fsName, OSS_MAX_PATHSIZE + 1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get disk info, rc = %d", rc ) ;

      if ( diskTotalBytes != 0 )
      {
         loadPercent = 100 * ( diskTotalBytes - diskFreeBytes ) /
                       diskTotalBytes ;
         loadPercent = loadPercent > 100 ? 100 : loadPercent ;
         loadPercent = loadPercent < 0 ? 0 : loadPercent ;
      }
      else
      {
         loadPercent = 0 ;
      }

      {
         BSONObjBuilder diskOb( ob.subobjStart( FIELD_NAME_DISK ) ) ;
         diskOb.append( FIELD_NAME_NAME, fsName ) ;
         if( appendDbPath )
         {
            diskOb.append( FIELD_NAME_DATABASEPATH, dbPath ) ;
         }
         diskOb.append( FIELD_NAME_LOADPERCENT, loadPercent ) ;
         diskOb.append( FIELD_NAME_TOTALSPACE, diskTotalBytes ) ;
         diskOb.append( FIELD_NAME_FREESPACE, diskFreeBytes ) ;
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
                                  UINT32 relatedTID )
   {
      UINT32 ip = 0 ;
      UINT32 port = 0 ;
      CHAR szTmp[ 8 + 4 + 8 + 2 ] = { 0 } ;

      if ( 0 != relatedNID )
      {
         ossUnpack32From64( relatedNID, ip, port ) ;
      }
      else
      {
         ip = _netFrame::getLocalAddress() ;
         port = pmdGetLocalPort() ;
      }
      ossSnprintf( szTmp, sizeof(szTmp)-1, "%08x%04x%08x",
                   ip, (UINT16)port, relatedTID ) ;
      ob.append( FIELD_NAME_RELATED_ID, szTmp ) ;
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
      ob.append( FIELD_NAME_TOTALDATAREAD, (SINT64)full._monApplCB.totalDataRead ) ;
      ob.append( FIELD_NAME_TOTALINDEXREAD, (SINT64)full._monApplCB.totalIndexRead ) ;
      ob.append( FIELD_NAME_TOTALDATAWRITE, (SINT64)full._monApplCB.totalDataWrite ) ;
      ob.append( FIELD_NAME_TOTALINDEXWRITE, (SINT64)full._monApplCB.totalIndexWrite ) ;
      ob.append( FIELD_NAME_TOTALUPDATE, (SINT64)full._monApplCB.totalUpdate ) ;
      ob.append( FIELD_NAME_TOTALDELETE, (SINT64)full._monApplCB.totalDelete ) ;
      ob.append( FIELD_NAME_TOTALINSERT, (SINT64)full._monApplCB.totalInsert ) ;
      ob.append( FIELD_NAME_TOTALSELECT, (SINT64)full._monApplCB.totalSelect ) ;
      ob.append( FIELD_NAME_TOTALREAD, (SINT64)full._monApplCB.totalRead ) ;

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

      monDumpLastOpInfo( ob, full._monApplCB ) ;

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
   void monDMSCollectionFlagToString ( UINT16 flag, std::string &out )
   {
      PD_TRACE_ENTRY ( SDB_MONDMSCOLLECTIONFLAGTOSTRING ) ;
      PD_TRACE1 ( SDB_MONDMSCOLLECTIONFLAGTOSTRING, PD_PACK_USHORT(flag) ) ;
      if ( DMS_IS_MB_FREE(flag) )
      {
         out = "Free" ;
         goto done ;
      }
      if ( DMS_IS_MB_NORMAL(flag) )
      {
         out = "Normal" ;
         goto done ;
      }
      if ( DMS_IS_MB_DROPPED(flag) )
      {
         out = "Dropped" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY(flag) )
      {
         out = "Offline Reorg Shadow Copy Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_TRUNCATE(flag) )
      {
         out = "Offline Reorg Truncate Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_COPY_BACK(flag) )
      {
         out = "Offline Reorg Copy Back Phase" ;
         goto done ;
      }
      if ( DMS_IS_MB_OFFLINE_REORG_REBUILD(flag) )
      {
         out = "Offline Reorg Rebuild Phase" ;
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB_MONDMSCOLLECTIONFLAGTOSTRING ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONDUMPINDEXES, "monDumpIndexes" )
   INT32 monDumpIndexes( MON_IDX_LIST &indexes, rtnContextDump *context )
   {
      INT32 rc = SDB_OK ;
      string flagDesp ;
      SDB_ASSERT ( context, "context can't be NULL" ) ;

      PD_TRACE_ENTRY ( SDB_MONDUMPINDEXES ) ;
      try
      {
         MON_IDX_LIST::iterator it ;
         for ( it = indexes.begin(); it!=indexes.end(); ++it )
         {
            UINT16 idxType = IXM_EXTENT_TYPE_NONE ;
            monIndex &indexItem = (*it) ;
            BSONObj &indexObj = indexItem._indexDef ;
            const CHAR *extDataName = NULL ;
            BSONObj obj ;
            BSONObjBuilder builder( MON_DUMP_DFT_BUILDER_SZ ) ;
            BSONObjBuilder ob (builder.subobjStart(IXM_FIELD_NAME_INDEX_DEF )) ;
            ob.append ( IXM_NAME_FIELD,
                        indexObj.getStringField(IXM_NAME_FIELD) ) ;
            OID oid ;
            indexObj.getField(DMS_ID_KEY_NAME).Val(oid) ;
            ob.append ( DMS_ID_KEY_NAME, oid ) ;
            ob.append ( IXM_KEY_FIELD,
                        indexObj.getObjectField(IXM_KEY_FIELD) ) ;
            BSONElement e = indexObj[IXM_V_FIELD] ;
            INT32 version = ( e.type() == NumberInt ) ? e._numberInt() : 0 ;
            ob.append ( IXM_V_FIELD, version ) ;
            ob.append ( IXM_UNIQUE_FIELD,
                        indexObj[IXM_UNIQUE_FIELD].trueValue() ) ;
            ob.append ( IXM_DROPDUP_FIELD,
                        indexObj.getBoolField(IXM_DROPDUP_FIELD) ) ;
            ob.append ( IXM_ENFORCED_FIELD,
                        indexObj.getBoolField(IXM_ENFORCED_FIELD) ) ;
            BSONObj range = indexObj.getObjectField( IXM_2DRANGE_FIELD ) ;
            if ( !range.isEmpty() )
            {
               ob.append( IXM_2DRANGE_FIELD, range ) ;
            }
            ob.done () ;

            flagDesp = getIndexFlagDesp(indexItem._indexFlag) ;
            builder.append (IXM_FIELD_NAME_INDEX_FLAG, flagDesp.c_str() ) ;
            if ( IXM_INDEX_FLAG_CREATING == indexItem._indexFlag )
            {
               builder.append ( IXM_FIELD_NAME_SCAN_EXTLID,
                                indexItem._scanExtLID ) ;
            }
            indexItem.getIndexType( idxType ) ;
            builder.append( FIELD_NAME_TYPE, getIndexTypeDesp( idxType ) ) ;
            extDataName = indexItem.getExtDataName() ;
            if ( ossStrlen( extDataName ) > 0 )
            {
               builder.append( FIELD_NAME_EXTERNAL_DATA_NAME, extDataName ) ;
            }
            obj = builder.obj() ;
            rc = context->monAppend( obj ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to add object %s to collections",
                        obj.toString().c_str() ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for collections: %s",
                  e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_MONDUMPINDEXES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MONRESETMON, "monResetMon" )
   void monResetMon ( RTN_COMMAND_TYPE type, BOOLEAN resetAllEDU,
                      EDUID eduID )
   {
      PD_TRACE_ENTRY ( SDB_MONRESETMON ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      pmdEDUMgr *mgr = krcb->getEDUMgr() ;
      pmdStartupHistoryLogger *startLogger = pmdGetStartupHstLogger() ;
      switch ( type )
      {
         case CMD_SNAPSHOT_ALL :
         {
            mondbcb->reset() ;
            mgr->resetMon() ;
            mgr->resetIOService() ;
            pmdResetErrNum () ;
            startLogger->clearAll() ;
            break ;
         }
         case CMD_SNAPSHOT_DATABASE :
         {
            mondbcb->reset() ;
            mgr->resetIOService() ;
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
         default :
            break ;
      }

      PD_TRACE_EXIT ( SDB_MONRESETMON ) ;
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
            for ( INT32 i = 0; i < _pdTraceComponentNum; ++i )
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
   INT32 monDumpDatablocks( std::vector<dmsExtentID> &datablocks,
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

            rc = monAppendSystemInfo( builder, MON_MASK_HOSTNAME|
                                      MON_MASK_SERVICE_NAME|MON_MASK_NODEID ) ;
            PD_RC_CHECK( rc, PDERROR, "Append system info failed, rc: %d",
                         rc ) ;

            builder.append( FIELD_NAME_SCANTYPE, VALUE_NAME_TBSCAN ) ;
            std::vector<dmsExtentID>::iterator it = datablocks.begin() ;
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

            rc = monAppendSystemInfo( builder, MON_MASK_HOSTNAME|
                                      MON_MASK_SERVICE_NAME|MON_MASK_NODEID ) ;
            PD_RC_CHECK( rc, PDERROR, "Append system info failed, rc: %d",
                         rc ) ;

            builder.append( FIELD_NAME_SCANTYPE, VALUE_NAME_IXSCAN ) ;
            builder.append( FIELD_NAME_INDEXNAME, indexName ) ;
            builder.append( FIELD_NAME_INDEXLID, indexLID ) ;
            builder.append( FIELD_NAME_DIRECTION, direction ) ;
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
            ob.append( FIELD_NAME_FAULT, memInfo.fault ) ;
         }
         else
         {
            ob.append( FIELD_NAME_VSIZE, 0 ) ;
            ob.append( FIELD_NAME_RSS, 0 ) ;
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

   /*
      _monTransFetcher implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monTransFetcher, RTN_FETCH_TRANS )

   _monTransFetcher::_monTransFetcher()
   {
      _dumpCurrent = TRUE ;
      _detail = FALSE ;
      _hitEnd = TRUE ;
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

   BOOLEAN _monTransFetcher::isHitEnd() const
   {
      return _hitEnd ;
   }

   INT32 _monTransFetcher::_fetchNextTransInfo()
   {
      INT32 rc = SDB_OK ;
      EDUID eduID = PMD_INVALID_EDUID ;
      pmdEDUMgr *pMgr = pmdGetKRCB()->getEDUMgr() ;
      dpsTransCB *pTransCB= sdbGetTransCB() ;

   retry:
      if ( _eduList.empty() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      eduID = _eduList.front() ;
      _eduList.pop() ;

      _curTransInfo._lockList.clear() ;
      _curTransInfo._transID = DPS_INVALID_TRANS_ID ;

      if ( SDB_OK != pMgr->dumpTransInfo( eduID, _curTransInfo ) ||
           DPS_INVALID_TRANS_ID == _curTransInfo._transID )
      {
         goto retry ;
      }

      _pos = _curTransInfo._lockList.begin() ;

      try
      {
         BSONObjBuilder builder ;
         monAppendSystemInfo( builder, _addInfoMask ) ;
         builder.append( FIELD_NAME_SESSIONID,
                         (INT64)_curTransInfo._eduID ) ;
         CHAR strTransID[ 4 + 2 + 10 + 2 ] = { 0 } ;
         ossSnprintf( strTransID, sizeof( strTransID ) - 1,
                      "%04x%010x", ( _curTransInfo._transID >> 48 ),
                      ( _curTransInfo._transID & DPS_TRANSID_SN_BIT ) ) ;
         builder.append( FIELD_NAME_TRANSACTION_ID, strTransID ) ;
         builder.appendBool( FIELD_NAME_IS_ROLLBACK,
                             pTransCB->isRollback( _curTransInfo._transID ) ?
                             TRUE : FALSE ) ;
         builder.append( FIELD_NAME_TRANS_LSN_CUR,
                         (INT64)_curTransInfo._curTransLsn ) ;
         builder.append( FIELD_NAME_TRANS_WAIT_LOCK,
                         _curTransInfo._waitLock.toBson() ) ;
         builder.append( FIELD_NAME_TRANS_LOCKS_NUM,
                         (INT32)_curTransInfo._locksNum ) ;
         monAppendSessionIdentify( builder, _curTransInfo._relatedNID,
                                   _curTransInfo._relatedTID ) ;

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
            else if ( _pos->first == _curTransInfo._waitLock )
            {
               ++_pos ;
               continue ;
            }
            babLockList.append( _pos->first.toBson() ) ;
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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monContextFetcher, RTN_FETCH_CONTEXT )

   _monContextFetcher::_monContextFetcher()
   {
      _dumpCurrent = FALSE ;
      _detail = TRUE ;
      _addInfoMask = 0 ;
      _hitEnd = TRUE ;
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
            std::set <SINT64> &contextList = _contextList[ cb->getID() ] ;
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

   BOOLEAN _monContextFetcher::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob ;
         std::map<UINT64, std::set<SINT64> >::iterator it ;
         std::set<SINT64>::iterator itSet ;

         it = _contextList.begin() ;
         std::set<SINT64> &setCtx = it->second ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SESSIONID, (SINT64)it->first ) ;
         ob.append( FIELD_NAME_TOTAL_COUNT, (INT32)setCtx.size() ) ;

         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         for ( itSet = setCtx.begin(); itSet!= setCtx.end(); ++itSet )
         {
            ba.append ( (*itSet) ) ;
         }
         ba.done() ;
         obj = ob.obj() ;

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
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ ) ;
         ossTickConversionFactor factor ;

         std::map<UINT64, std::set<monContextFull> >::iterator it ;
         std::set<monContextFull>::iterator itSet ;

         CHAR timestampStr[ OSS_TIMESTAMP_STRING_LEN + 1] = { 0 } ;
         UINT32 seconds = 0 ;
         UINT32 microseconds = 0 ;

         it = _contextInfoList.begin() ;
         std::set<monContextFull> &setInfo = it->second ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SESSIONID, (INT64)it->first ) ;

         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         for ( itSet = setInfo.begin() ; itSet != setInfo.end() ; ++itSet )
         {
            const monContextFull &ctx = *itSet ;
            ossTimestamp startTime( ctx._monContext.getStartTimestamp() ) ;
            BSONObjBuilder sub( ba.subobjStart() ) ;

            sub.append( FIELD_NAME_CONTEXTID, ctx._contextID );
            sub.append( FIELD_NAME_TYPE, ctx._typeDesp ) ;
            sub.append( FIELD_NAME_DESP, ctx._info ) ;
            sub.append( FIELD_NAME_DATAREAD,
                        (SINT64)ctx._monContext.getDataRead() );
            sub.append( FIELD_NAME_INDEXREAD,
                        (SINT64)ctx._monContext.getIndexRead() ) ;
            ctx._monContext.getQueryTime().convertToTime ( factor,
                                                           seconds,
                                                           microseconds ) ;
            sub.append( FIELD_NAME_QUERYTIMESPENT,
                        (SINT64)(seconds * 1000 + microseconds / 1000 ) ) ;
            ossTimestampToString( startTime, timestampStr ) ;
            sub.append(FIELD_NAME_STARTTIMESTAMP, timestampStr ) ;
            sub.done() ;
         }
         ba.done() ;
         obj = ob.obj() ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monSessionFetcher, RTN_FETCH_SESSION )

   _monSessionFetcher::_monSessionFetcher()
   {
      _dumpCurrent = TRUE ;
      _detail = FALSE ;
      _addInfoMask = 0 ;
      _hitEnd = TRUE ;
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
            cb->getEDUMgr()->dumpInfo( _setInfoSimple ) ;
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
            cb->getEDUMgr()->dumpInfo( _setInfoDetail ) ;
         }
         _hitEnd = _setInfoDetail.empty() ? TRUE : FALSE ;
      }

      return SDB_OK ;
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

   BOOLEAN _monSessionFetcher::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob ;
         std::set<monEDUSimple>::iterator it ;

         it = _setInfoSimple.begin() ;
         const monEDUSimple &simple = *it ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append ( FIELD_NAME_SESSIONID, (SINT64)simple._eduID ) ;
         ob.append ( FIELD_NAME_TID, simple._tid ) ;
         ob.append ( FIELD_NAME_STATUS, simple._eduStatus ) ;
         ob.append ( FIELD_NAME_TYPE, simple._eduType ) ;
         ob.append ( FIELD_NAME_EDUNAME, simple._eduName ) ;
         monAppendSessionIdentify( ob, simple._relatedNID,
                                   simple._relatedTID ) ;

         obj = ob.obj () ;

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
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ ) ;
         std::set<monEDUFull>::iterator it ;

         it = _setInfoDetail.begin() ;
         const monEDUFull &full = *it ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append( FIELD_NAME_SESSIONID, (INT64)full._eduID ) ;
         ob.append( FIELD_NAME_TID, full._tid ) ;
         ob.append( FIELD_NAME_STATUS, full._eduStatus ) ;
         ob.append( FIELD_NAME_TYPE, full._eduType ) ;
         ob.append( FIELD_NAME_EDUNAME, full._eduName ) ;
         ob.append( FIELD_NAME_QUEUE_SIZE, full._queueSize ) ;
         ob.append( FIELD_NAME_PROCESS_EVENT_COUNT,
                    (SINT64)full._processEventCount ) ;
         monAppendSessionIdentify( ob, full._relatedNID,
                                   full._relatedTID ) ;
         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_CONTEXTS ) ) ;
         std::set<SINT64>::const_iterator itCtx ;
         for ( itCtx = full._eduContextList.begin() ;
               itCtx != full._eduContextList.end() ;
               ++itCtx )
         {
            ba.append( *itCtx ) ;
         }
         ba.done() ;

         ossTime userTime, sysTime ;
         ossTickConversionFactor factor ;
         ossGetCPUUsage( full._threadHdl, userTime, sysTime ) ;
         monSessionMonEDUFull( ob, full, factor, userTime, sysTime ) ;
         obj = ob.obj () ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCollectionFetch, RTN_FETCH_COLLECTION )

   _monCollectionFetch::_monCollectionFetch()
   {
      _detail = FALSE ;
      _includeSys = FALSE ;
      _addInfoMask = 0 ;
      _hitEnd = TRUE ;
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

      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _includeSys = isCurrent ;

      if ( !_detail )
      {
         dmsCB->dumpInfo( _collectionList, _includeSys ) ;
         _hitEnd = _collectionList.empty() ? TRUE : FALSE ;
      }
      else
      {
         dmsCB->dumpInfo( _collectionInfo, _includeSys ) ;
         _hitEnd = _collectionInfo.empty() ? TRUE : FALSE ;
      }

      return SDB_OK ;
   }

   const CHAR* _monCollectionFetch::getName() const
   {
      return _detail ? CMD_NAME_SNAPSHOT_COLLECTIONS :
                       CMD_NAME_LIST_COLLECTIONS ;
   }

   BOOLEAN _monCollectionFetch::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob ;
         MON_CL_SIM_LIST::iterator it ;

         it = _collectionList.begin() ;
         const monCLSimple &simple = *it ;

         ob.append ( FIELD_NAME_NAME, simple._name ) ;

         obj = ob.obj () ;

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
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;
         MON_CL_LIST::iterator it ;
         MON_CL_DETAIL_MAP::const_iterator itDetail ;

         it = _collectionInfo.begin() ;
         const monCollection &full = *it ;

         ob.append ( FIELD_NAME_NAME, full._name ) ;
         const CHAR *pDot = ossStrchr( full._name, '.' ) ;
         if ( pDot )
         {
            ob.appendStrWithNoTerminating ( FIELD_NAME_COLLECTIONSPACE,
                                            full._name,
                                            pDot - full._name ) ;
         }
         BSONArrayBuilder ba( ob.subarrayStart( FIELD_NAME_DETAILS ) ) ;
         for ( itDetail = full._details.begin() ;
               itDetail != full._details.end() ;
               ++itDetail )
         {
            const detailedInfo &detail = itDetail->second ;
            BSONObjBuilder sub( ba.subobjStart() ) ;

            UINT16 flag = detail._flag ;
            std::string status = "" ;
            CHAR tmp[ MON_TMP_STR_SZ + 1 ] = { 0 } ;

            monAppendSystemInfo( sub, _addInfoMask ) ;

            sub.append ( FIELD_NAME_ID, detail._blockID ) ;
            sub.append ( FIELD_NAME_LOGICAL_ID, detail._logicID ) ;
            sub.append ( FIELD_NAME_SEQUENCE, (INT32)itDetail->first ) ;
            sub.append ( FIELD_NAME_INDEXES, detail._numIndexes ) ;
            monDMSCollectionFlagToString ( flag, status ) ;
            sub.append ( FIELD_NAME_STATUS, status ) ;
            mbAttr2String( detail._attribute, tmp, MON_TMP_STR_SZ ) ;
            sub.append ( FIELD_NAME_ATTRIBUTE, tmp ) ;
            if ( OSS_BIT_TEST( detail._attribute, DMS_MB_ATTR_COMPRESSED ) )
            {
               sub.append ( FIELD_NAME_COMPRESSIONTYPE,
                            utilCompressType2String( detail._compressType ) ) ;
            }
            else
            {
               sub.append ( FIELD_NAME_COMPRESSIONTYPE, "" ) ;
            }
            sub.appendBool( FIELD_NAME_DICT_CREATED, detail._dictCreated ) ;
            sub.append( FIELD_NAME_DICT_VERSION, detail._dictVersion ) ;
            sub.append ( FIELD_NAME_PAGE_SIZE, detail._pageSize ) ;
            sub.append ( FIELD_NAME_LOB_PAGE_SIZE, detail._lobPageSize ) ;

            sub.append ( FIELD_NAME_TOTAL_RECORDS,
                         (long long)(detail._totalRecords )) ;
            sub.append ( FIELD_NAME_TOTAL_LOBS,
                         (long long)(detail._totalLobs) ) ;
            sub.append ( FIELD_NAME_TOTAL_DATA_PAGES,
                         detail._totalDataPages ) ;
            sub.append ( FIELD_NAME_TOTAL_INDEX_PAGES,
                         detail._totalIndexPages ) ;
            sub.append ( FIELD_NAME_TOTAL_LOB_PAGES,
                         detail._totalLobPages ) ;
            sub.append ( FIELD_NAME_TOTAL_DATA_FREESPACE,
                         (long long)(detail._totalDataFreeSpace )) ;
            sub.append ( FIELD_NAME_TOTAL_INDEX_FREESPACE,
                         (long long)(detail._totalIndexFreeSpace )) ;
            sub.append ( FIELD_NAME_CURR_COMPRESS_RATIO,
                         (FLOAT64)detail._currCompressRatio / 100.0 ) ;

            sub.append ( FIELD_NAME_DATA_COMMIT_LSN, (INT64)detail._dataCommitLSN ) ;
            sub.append ( FIELD_NAME_IDX_COMMIT_LSN, (INT64)detail._idxCommitLSN ) ;
            sub.append ( FIELD_NAME_LOB_COMMIT_LSN, (INT64)detail._lobCommitLSN ) ;
            sub.appendBool ( FIELD_NAME_DATA_COMMITTED, detail._dataIsValid ) ;
            sub.appendBool ( FIELD_NAME_IDX_COMMITTED, detail._idxIsValid ) ;
            sub.appendBool ( FIELD_NAME_LOB_COMMITTED, detail._lobIsValid ) ;

            sub.done() ;
         }
         ba.done() ;
         obj = ob.obj() ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCollectionSpaceFetch, RTN_FETCH_COLLECTIONSPACE )

   _monCollectionSpaceFetch::_monCollectionSpaceFetch()
   {
      _detail = FALSE ;
      _includeSys = FALSE ;
      _addInfoMask = 0 ;
      _hitEnd = TRUE ;
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

      _addInfoMask = addInfoMask ;
      _detail = isDetail ;
      _includeSys = isCurrent ;

      if ( !_detail )
      {
         dmsCB->dumpInfo( _csList, _includeSys, FALSE, FALSE ) ;
         _hitEnd = _csList.empty() ? TRUE : FALSE ;
      }
      else
      {
         dmsCB->dumpInfo( _csInfo, _includeSys ) ;
         _hitEnd = _csInfo.empty() ? TRUE : FALSE ;
      }

      return SDB_OK ;
   }

   const CHAR* _monCollectionSpaceFetch::getName() const
   {
      return _detail ? CMD_NAME_SNAPSHOT_COLLECTIONSPACES :
                       CMD_NAME_LIST_COLLECTIONSPACES ;
   }

   BOOLEAN _monCollectionSpaceFetch::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob ;
         MON_CS_SIM_LIST::iterator it ;

         it = _csList.begin() ;
         const monCSSimple &simple = *it ;

         ob.append ( FIELD_NAME_NAME, simple._name ) ;

         obj = ob.obj () ;

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
      INT32 rc = SDB_OK ;

      if ( _csInfo.size() == 0 )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         INT64 dataCapSize    = 0 ;
         INT64 lobCapSize     = 0 ;
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;
         MON_CS_LIST::iterator it ;
         MON_CL_DETAIL_MAP::const_iterator itDetail ;

         it = _csInfo.begin() ;
         const monCollectionSpace &full = *it ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append ( FIELD_NAME_NAME, full._name ) ;
         BSONArrayBuilder sub( ob.subarrayStart( FIELD_NAME_COLLECTION ) ) ;
         if ( ossStrcmp ( full._name, SDB_DMSTEMP_NAME ) != 0 )
         {
            MON_CL_SIM_VEC::const_iterator it1 ;
            for ( it1 = full._collections.begin();
                  it1!= full._collections.end();
                  it1++ )
            {
               sub.append (BSON ( FIELD_NAME_NAME << (*it1)._name ) ) ;
            }
         }
         sub.done() ;

         dataCapSize = (INT64)full._pageSize * DMS_MAX_PG ;
         lobCapSize  = (INT64)full._lobPageSize * DMS_MAX_PG ;
         if ( lobCapSize > OSS_MAX_FILE_SZ )
         {
            lobCapSize = OSS_MAX_FILE_SZ ;
         }
         ob.append ( FIELD_NAME_PAGE_SIZE, full._pageSize ) ;
         ob.append ( FIELD_NAME_LOB_PAGE_SIZE, full._lobPageSize ) ;
         ob.append ( FIELD_NAME_MAX_CAPACITY_SIZE,
                     2 * dataCapSize + lobCapSize ) ;
         ob.append ( FIELD_NAME_MAX_DATA_CAP_SIZE, dataCapSize ) ;
         ob.append ( FIELD_NAME_MAX_INDEX_CAP_SIZE, dataCapSize ) ;
         ob.append ( FIELD_NAME_MAX_LOB_CAP_SIZE, lobCapSize ) ;
         ob.append ( FIELD_NAME_NUMCOLLECTIONS, full._clNum ) ;
         ob.append ( FIELD_NAME_TOTAL_RECORDS, full._totalRecordNum ) ;
         ob.append ( FIELD_NAME_TOTAL_SIZE, full._totalSize ) ;
         ob.append ( FIELD_NAME_FREE_SIZE, full._freeSize ) ;
         ob.append ( FIELD_NAME_TOTAL_DATA_SIZE, full._totalDataSize ) ;
         ob.append ( FIELD_NAME_FREE_DATA_SIZE, full._freeDataSize ) ;
         ob.append ( FIELD_NAME_TOTAL_IDX_SIZE, full._totalIndexSize ) ;
         ob.append ( FIELD_NAME_FREE_IDX_SIZE, full._freeIndexSize ) ;
         ob.append ( FIELD_NAME_TOTAL_LOB_SIZE, full._totalLobSize ) ;
         ob.append ( FIELD_NAME_FREE_LOB_SIZE, full._freeLobSize ) ;

         ob.append ( FIELD_NAME_DATA_COMMIT_LSN, (INT64)full._dataCommitLsn ) ;
         ob.append ( FIELD_NAME_IDX_COMMIT_LSN, (INT64)full._idxCommitLsn ) ;
         ob.append ( FIELD_NAME_LOB_COMMIT_LSN, (INT64)full._lobCommitLsn ) ;
         ob.appendBool ( FIELD_NAME_DATA_COMMITTED, full._dataIsValid ) ;
         ob.appendBool ( FIELD_NAME_IDX_COMMITTED, full._idxIsValid ) ;
         ob.appendBool ( FIELD_NAME_LOB_COMMITTED, full._lobIsValid ) ;

         ob.append ( FIELD_NAME_DIRTY_PAGE, (INT32)full._dirtyPage ) ;
         ob.append( FIELD_NAME_TYPE, (INT32)full._type ) ;
         obj = ob.obj() ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monDataBaseFetch, RTN_FETCH_DATABASE )

   _monDataBaseFetch::_monDataBaseFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

   BOOLEAN _monDataBaseFetch::isHitEnd() const
   {
      return _hitEnd ;
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
      const CHAR *dbPath = pmdGetOptionCB()->getDbPath() ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      ossGetCPUUsage( userTime, sysTime ) ;
      ossGetDiskInfo ( dbPath, diskTotalBytes, diskFreeBytes ) ;

      try
      {
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ ) ;

         monAppendSystemInfo ( ob, _addInfoMask ) ;
         monAppendVersion ( ob ) ;

         ossTickConversionFactor factor ;
         ob.append ( FIELD_NAME_CURRENTACTIVESESSIONS,
                     (SINT32)mgr->sizeRun() ) ;
         ob.append ( FIELD_NAME_CURRENTIDLESESSIONS,
                     (SINT32)mgr->sizeIdle () ) ;
         ob.append ( FIELD_NAME_CURRENTSYSTEMSESSIONS,
                     (SINT32)mgr->sizeSystem() ) ;
         ob.append ( FIELD_NAME_CURRENTCONTEXTS, (SINT32)rtnCB->contextNum() ) ;
         ob.append ( FIELD_NAME_RECEIVECOUNT,
                     (INT64)mondbcb->getReceiveNum() ) ;
         ob.append ( FIELD_NAME_ROLE, krcb->getOptionCB()->dbroleStr() ) ;

         {
            BSONObjBuilder diskOb( ob.subobjStart( FIELD_NAME_DISK ) ) ;
            INT32 loadPercent = 0 ;
            if ( diskTotalBytes != 0 )
            {
               loadPercent = 100 * ( diskTotalBytes - diskFreeBytes ) /
                             diskTotalBytes ;
               loadPercent = loadPercent > 100 ? 100 : loadPercent ;
               loadPercent = loadPercent < 0 ? 0 : loadPercent ;
            }
            else
            {
               loadPercent = 0 ;
            }
            diskOb.append ( FIELD_NAME_DATABASEPATH, dbPath ) ;
            diskOb.append ( FIELD_NAME_LOADPERCENT, loadPercent ) ;
            diskOb.append ( FIELD_NAME_TOTALSPACE, diskTotalBytes ) ;
            diskOb.append ( FIELD_NAME_FREESPACE, diskFreeBytes ) ;
            diskOb.done() ;
         }

         monDBDump ( ob, mondbcb, factor, userTime, sysTime ) ;
         monDBDumpLogInfo( ob ) ;
         monDBDumpProcMemInfo( ob ) ;
         monDBDumpStorageInfo( ob ) ;
         monDBDumpNetInfo( ob ) ;

         obj = ob.obj () ;
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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monSystemFetch, RTN_FETCH_SYSTEM )

   _monSystemFetch::_monSystemFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

   BOOLEAN _monSystemFetch::isHitEnd() const
   {
      return _hitEnd ;
   }

   INT32 _monSystemFetch::fetch( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      INT64 cpuUser        = 0 ;
      INT64 cpuSys         = 0 ;
      INT64 cpuIdle        = 0 ;
      INT64 cpuOther       = 0 ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      rc = ossGetCPUInfo ( cpuUser, cpuSys, cpuIdle, cpuOther ) ;
       if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get cpu info, rc = %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ ) ;

         monAppendSystemInfo ( ob, _addInfoMask ) ;
         {
            BSONObjBuilder cpuOb( ob.subobjStart( FIELD_NAME_CPU ) ) ;
            cpuOb.append ( FIELD_NAME_USER, ((FLOAT64)cpuUser)/1000 ) ;
            cpuOb.append ( FIELD_NAME_SYS, ((FLOAT64)cpuSys)/1000 ) ;
            cpuOb.append ( FIELD_NAME_IDLE, ((FLOAT64)cpuIdle)/1000 ) ;
            cpuOb.append ( FIELD_NAME_OTHER, ((FLOAT64)cpuOther)/1000 ) ;
            cpuOb.done() ;
         }
         monAppendHostMemory( ob ) ;
         monAppendDisk( ob ) ;
         obj = ob.obj() ;
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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monHealthFetch, RTN_FETCH_HEALTH )

   _monHealthFetch::_monHealthFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

   BOOLEAN _monHealthFetch::isHitEnd() const
   {
      return _hitEnd ;
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

      try
      {
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ ) ;

         monAppendSystemInfo ( ob, _addInfoMask ) ;

         const CHAR* dataStatus = utilDataStatusStr( pmdGetStartup().isOK(),
                                                     krcb->getDBStatus() ) ;
         ob.append( FIELD_NAME_DATA_STATUS, dataStatus ) ;

         ob.appendBool( FIELD_NAME_SYNC_CONTROL, krcb->isInFlowControl()  ) ;

         monAppendUlimit( ob ) ;

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

         obj = ob.obj() ;
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
      _monStorageUnitFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monStorageUnitFetch, RTN_FETCH_STORAGEUNIT )

   _monStorageUnitFetch::_monStorageUnitFetch()
   {
      _includeSys    = FALSE ;
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

      _addInfoMask = addInfoMask ;
      _includeSys = isCurrent ;

      dmsCB->dumpInfo( _suInfo, _includeSys ) ;
      _hitEnd = _suInfo.empty() ? TRUE : FALSE ;

      return SDB_OK ;
   }

   const CHAR* _monStorageUnitFetch::getName() const
   {
      return CMD_NAME_LIST_STORAGEUNITS ;
   }

   BOOLEAN _monStorageUnitFetch::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;
         MON_SU_LIST::iterator it ;

         it = _suInfo.begin() ;
         const monStorageUnit &su = *it ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         ob.append ( FIELD_NAME_NAME, su._name ) ;
         ob.append ( FIELD_NAME_ID, su._CSID ) ;
         ob.append ( FIELD_NAME_LOGICAL_ID, su._logicalCSID ) ;
         ob.append ( FIELD_NAME_PAGE_SIZE, su._pageSize ) ;
         ob.append ( FIELD_NAME_LOB_PAGE_SIZE, su._lobPageSize ) ;
         ob.append ( FIELD_NAME_SEQUENCE, su._sequence ) ;
         ob.append ( FIELD_NAME_NUMCOLLECTIONS, su._numCollections ) ;
         ob.append ( FIELD_NAME_COLLECTIONHWM, su._collectionHWM ) ;
         ob.append ( FIELD_NAME_SIZE, su._size ) ;

         obj = ob.obj() ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monIndexFetch, RTN_FETCH_INDEX )

   _monIndexFetch::_monIndexFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
      _pos           = 0 ;
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
      rc = su->getIndexes ( pCollectionShortName, _indexInfo ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get indexes %s, rc: %d",
                  pCollectionName, rc ) ;
         goto error ;
      }

      _hitEnd = _indexInfo.empty() ? TRUE : FALSE ;
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

   const CHAR* _monIndexFetch::getName() const
   {
      return CMD_NAME_GET_INDEXES ;
   }

   BOOLEAN _monIndexFetch::isHitEnd() const
   {
      return _hitEnd ;
   }

   INT32 _monIndexFetch::fetch( BSONObj &obj )
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

   INT32 _monIndexFetch::_fetchNext( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      if ( _pos >= _indexInfo.size() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      try
      {
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;

         const monIndex &indexItem = _indexInfo[ _pos++ ] ;
         const BSONObj &indexObj = indexItem._indexDef ;
         OID oid ;

         monAppendSystemInfo( ob, _addInfoMask ) ;

         BSONObjBuilder sub( ob.subobjStart( IXM_FIELD_NAME_INDEX_DEF ) ) ;

         sub.append ( IXM_NAME_FIELD,
                      indexObj.getStringField( IXM_NAME_FIELD ) ) ;
         indexObj.getField( DMS_ID_KEY_NAME ).Val(oid) ;
         sub.append ( DMS_ID_KEY_NAME, oid ) ;
         sub.append ( IXM_KEY_FIELD,
                      indexObj.getObjectField( IXM_KEY_FIELD ) ) ;
         BSONElement e = indexObj[ IXM_V_FIELD ] ;
         INT32 version = ( e.type() == NumberInt ) ? e._numberInt() : 0 ;
         sub.append ( IXM_V_FIELD, version ) ;
         sub.append ( IXM_UNIQUE_FIELD,
                      indexObj[IXM_UNIQUE_FIELD].trueValue() ) ;
         sub.append ( IXM_DROPDUP_FIELD,
                      indexObj.getBoolField( IXM_DROPDUP_FIELD ) ) ;
         sub.append ( IXM_ENFORCED_FIELD,
                      indexObj.getBoolField( IXM_ENFORCED_FIELD ) ) ;
         BSONObj range = indexObj.getObjectField( IXM_2DRANGE_FIELD ) ;
         if ( !range.isEmpty() )
         {
            sub.append( IXM_2DRANGE_FIELD, range ) ;
         }
         sub.done () ;

         const CHAR *pFlagDesp = getIndexFlagDesp( indexItem._indexFlag ) ;
         ob.append ( IXM_FIELD_NAME_INDEX_FLAG, pFlagDesp ) ;
         if ( IXM_INDEX_FLAG_CREATING == indexItem._indexFlag )
         {
            ob.append ( IXM_FIELD_NAME_SCAN_EXTLID,
                        indexItem._scanExtLID ) ;
         }

         obj = ob.obj() ;

         if ( _pos >= _indexInfo.size() )
         {
            _hitEnd = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON objects for "
                  "indexes: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _monCLBlockFetch implement
   */
   IMPLEMENT_FETCH_AUTO_REGISTER( _monCLBlockFetch, RTN_FETCH_DATABLOCK )

   _monCLBlockFetch::_monCLBlockFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

   BOOLEAN _monCLBlockFetch::isHitEnd() const
   {
      return _hitEnd ;
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
         BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ * 4 ) ;

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

         obj = ob.obj() ;

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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monBackupFetch, RTN_FETCH_BACKUP )

   _monBackupFetch::_monBackupFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
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

   BOOLEAN _monBackupFetch::isHitEnd() const
   {
      return _hitEnd ;
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
            BSONObjBuilder ob( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;

            monAppendSystemInfo( ob, _addInfoMask ) ;
            ob.appendElements( _vecBackup[ _pos++ ] ) ;

            obj = ob.obj() ;
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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monAccessPlansFetch, RTN_FETCH_ACCESSPLANS )
   _monAccessPlansFetch::_monAccessPlansFetch ()
   {
      _hitEnd = TRUE ;
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

   BOOLEAN _monAccessPlansFetch::isHitEnd () const
   {
      return _hitEnd ;
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
            BSONObjBuilder builder( MON_DUMP_DFT_BUILDER_SZ / 2 ) ;

            builder.appendElements( _sysInfo ) ;
            builder.appendElements( _cachedPlanList[ _pos++ ] ) ;

            obj = builder.obj() ;
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
   IMPLEMENT_FETCH_AUTO_REGISTER( _monConfigFetch, RTN_FETCH_CONFIG )

   _monConfigFetch::_monConfigFetch()
   {
      _addInfoMask   = 0 ;
      _hitEnd        = TRUE ;
   }

   _monConfigFetch::~_monConfigFetch()
   {
   }

   INT32 _monConfigFetch::init( pmdEDUCB *cb,
                                BOOLEAN isCurrent,
                                BOOLEAN isDetail,
                                UINT32 addInfoMask,
                                const BSONObj obj )
   {
      _addInfoMask = addInfoMask ;
      _hitEnd = FALSE ;

      return SDB_OK ;
   }

   const CHAR* _monConfigFetch::getName() const
   {
      return CMD_NAME_SNAPSHOT_CONFIG ;
   }

   BOOLEAN _monConfigFetch::isHitEnd() const
   {
      return _hitEnd ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MONCONFIGFETCH_FETCH, "_monConfigFetch::fetch" )
   INT32 _monConfigFetch::fetch( BSONObj &obj )
   {
      PD_TRACE_ENTRY ( SDB__MONCONFIGFETCH_FETCH ) ;
      INT32 rc             = SDB_OK ;

      if ( _hitEnd )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }


      rc = pmdGetOptionCB()->toBSON(obj, 0);

      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "Failed to generate config snapshot.") ;
         goto error ;
      }

      _hitEnd = TRUE ;

   done:
      PD_TRACE_EXITRC ( SDB__MONCONFIGFETCH_FETCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
