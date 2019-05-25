/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dpsArchiveMgr.hpp

   Descriptive Name = Data Protection Services Log Archive Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operationsdpsArchiveInfo

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSARCHIVER_HPP_
#define DPSARCHIVER_HPP_

#include "dpsArchiveInfo.hpp"
#include "dpsArchiveFileMgr.hpp"
#include "dpsDef.hpp"
#include "dpsLogDef.hpp"
#include "ossUtil.hpp"
#include "ossQueue.hpp"
#include <queue>

using namespace std ;

namespace engine
{
   class dpsArchiveEvent ;
   class _dpsLogWrapper ;
   class _dpsReplicaLogMgr ;

   class dpsArchiveMgr: public SDBObject,
                        public dpsEventHandler,
                        public utilStreamInterrupt
   {
   public:
      dpsArchiveMgr() ;
      virtual ~dpsArchiveMgr() ;

   public:
      INT32 init( _dpsLogWrapper* dpsCB, const CHAR* archivePath ) ;
      INT32 fini() ;
      INT32 run() ;
      BOOLEAN isInterrupted() ;

      virtual INT32 canAssignLogPage( UINT32 reqLen, _pmdEDUCB *cb ) ;
      virtual void  onPrepareLog( UINT32 csLID, UINT32 clLID,
                                  INT32 extLID, DPS_LSN_OFFSET offset )
      {
         return ;
      }
      virtual void  onWriteLog( DPS_LSN_OFFSET offset )
      {
         return ;
      }
      virtual INT32 onCompleteOpr( _pmdEDUCB *cb, INT32 w )
      {
         return SDB_OK ;
      }
      virtual void  onSwitchLogFile( UINT32 preLogicalFileId,
                                     UINT32 preFileId,
                                     UINT32 curLogicalFileId,
                                     UINT32 curFileId ) ;
      virtual void  onMoveLog( DPS_LSN_OFFSET moveToOffset,
                               DPS_LSN_VER moveToVersion,
                               DPS_LSN_OFFSET expectOffset,
                               DPS_LSN_VER expectVersion,
                               DPS_MOMENT moment,
                               INT32 errcode ) ;

   private:
      DPS_LSN  _calcStartLSN() ;
      INT32    _buildGenerateArchiveEvent( BOOLEAN allowPartial = FALSE ) ;
      INT32    _processLogEvent( dpsArchiveEvent* event ) ;
      INT32    _generateArchiveEvent( const DPS_LSN& startLSN, 
                                      const DPS_LSN& endLSN,
                                      BOOLEAN allowPartial = FALSE ) ;

      INT32    _archive( UINT32 logicalFileId, BOOLEAN isPartial,
                         const DPS_LSN& startLSN, const DPS_LSN& endLSN ) ;
      INT32    _archiveFull( UINT32 fileId, UINT32 logicalFileId ) ;
      INT32    _archivePartial( UINT32 logicalFileId,
                                DPS_LSN_OFFSET startOffset,
                                DPS_LSN_OFFSET endOffset ) ;
      INT32    _move( const DPS_LSN& lsn ) ;

      INT32    _checkArchiveTimeout() ;
      INT32    _checkArchiveExpired() ;
      INT32    _checkArchiveQuota() ;

      void     _beforeMove() ;
      void     _afterMove( const DPS_LSN& lsn, INT32 errcode ) ;
      void     _clearQueue() ;
      DPS_LSN  _getMoveLSN() ;
      void     _setMoveLSN( const DPS_LSN& lsn ) ;
      void     _popMoveLSN( const DPS_LSN& lsn ) ;


   private:
      string                        _archivePath ;
      dpsArchiveInfoMgr             _infoMgr ;
      dpsArchiveFileMgr             _fileMgr ;
      _dpsLogWrapper*               _dpsCB ;
      _dpsReplicaLogMgr*            _logMgr ;
      ossQueue<dpsArchiveEvent*>    _queue ;
      ossTimestamp                  _lastActiveTime ;
      ossTimestamp                  _lastExpiredTime ;
      INT64                         _archiveSize ;
      BOOLEAN                       _isArchiving ;
      BOOLEAN                       _isDPSMoving ;
      queue<DPS_LSN>                _moveLSN ;
      ossSpinXLatch                 _mutex ;
      BOOLEAN                       _inited ;
   } ;
}

#endif /* DPSARCHIVER_HPP_ */

