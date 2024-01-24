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

   Source File Name = clsReplBucket.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/11/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_REPL_BUCKET_HPP__
#define CLS_REPL_BUCKET_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "clsBase.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossQueue.hpp"
#include "pmdMemPool.hpp"
#include "rtnBackgroundJob.hpp"
#include "dpsLogDef.hpp"
#include "clsReplayer.hpp"
#include "utilCircularQueue.hpp"

#include <vector>

using namespace std ;
using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _clsReplayer ;
   class _dpsLogWrapper ;
   class _monDBCB ;

   /*
   Buff struct
   | dpsLog | size(4) | next(8) |
   */

   /*
      _clsBucketUnit define
   */
   class _clsBucketUnit : public SDBObject
   {
      public:
         _clsBucketUnit () ;
         ~_clsBucketUnit () ;

      public:

         BOOLEAN  isEmpty () const { return 0 == _number ? TRUE : FALSE ; }
         UINT32   size () const { return _number ; }
         void     push( CHAR *pData ) ;
         BOOLEAN  pop( CHAR **ppData, UINT32 &len ) ;
         BOOLEAN  pop() ;
         BOOLEAN  front( CHAR **ppData, UINT32 &len ) ;

         void     attach()
         {
            _inQue    = FALSE ;
            _attachIn = TRUE ;
         }
         void     dettach() { _attachIn = FALSE ; }
         BOOLEAN  isAttached() const { return _attachIn ; }

         void     pushToQue () { _inQue = TRUE ; }
         BOOLEAN  isInQue () const { return _inQue ; }

      private:
         CHAR                       *_pDataHeader ;
         CHAR                       *_pDataTail ;
         UINT32                     _number ;
         BOOLEAN                    _attachIn ;
         BOOLEAN                    _inQue ;

   } ;
   typedef _clsBucketUnit clsBucketUnit ;

   /*
      _clsCompleteInfo define
   */
   struct _clsReplayInfo
   {
      CHAR             *_pData ;
      UINT32            _len ;
      UINT32            _unitID ;
      CLS_PARALLA_TYPE  _parallaType ;
      UINT32            _clHash ;
      utilCLUniqueID    _clUniqueID ;
      DPS_LSN_OFFSET    _waitLSN ;
      pmdDataExInfo     _dataExInfo ;

      _clsReplayInfo()
      {
         _pData         = NULL ;
         _len           = 0 ;
         _unitID        = 0 ;
         _parallaType   = CLS_PARALLA_NULL ;
         _clHash        = 0 ;
         _clUniqueID    = UTIL_UNIQUEID_NULL ;
         _waitLSN       = DPS_INVALID_LSN_OFFSET ;
      }
   } ;
   typedef _clsReplayInfo clsReplayInfo ;

   typedef ossPoolMap< DPS_LSN_OFFSET, clsReplayInfo > CLS_COMP_MAP ;

   enum CLS_BUCKET_STATUS
   {
      CLS_BUCKET_CLOSED          = 0,
      CLS_BUCKET_NORMAL,
      CLS_BUCKET_WAIT_ROLLBACK,
      CLS_BUCKET_ROLLBACKING
   } ;

   const CHAR* clsGetReplBucketStatusDesp( INT32 status ) ;

   enum CLS_SUBMIT_RESULT
   {
      CLS_SUBMIT_EQ_EXPECT       = 1,
      CLS_SUBMIT_LT_MAX,
      CLS_SUBMIT_GT_MAX
   } ;

   #define CLS_FAST_LSN_RETRY_TIMES             ( 200 )

   /*
      _clsBucket define
   */
   class _clsBucket : public SDBObject
   {
      friend class _clsReplayer ;

      public:
         _clsBucket () ;
         ~_clsBucket () ;

         BSONObj toBson() ;
         INT32   forceCompleteAll() ;

         void enforceMaxReplSync( UINT32 maxReplSync ) ;
         UINT32 maxReplSync () const { return _maxReplSync ; }

         CLS_BUCKET_STATUS getStatus() const { return _status ; }

         INT32       init ( clsReplayEventHandler *handler = NULL ) ;
         void        reset( BOOLEAN setExpect = FALSE ) ;
         void        close() ;
         void        fini () ;

         UINT32      calcIndex( UINT32 hashValue ) ;

         INT32       pushWait( UINT32 index, DPS_LSN_OFFSET waitLSN ) ;
         INT32       pushData( UINT32 index, CHAR *pData, UINT32 len,
                               CLS_PARALLA_TYPE parallaType,
                               UINT32 clHash, utilCLUniqueID clUniqueID,
                               DPS_LSN_OFFSET waitLSN ) ;
         BOOLEAN     popData( UINT32 index, clsReplayInfo &info ) ;

         INT32       waitQueEmpty( INT64 millisec = -1 ) ;
         INT32       waitEmpty( INT64 millisec = -1 ) ;
         INT32       waitSubmit( INT64 millisec = -1 ) ;
         INT32       waitEmptyAndRollback( UINT32 *pNum = NULL,
                                           DPS_LSN *pCompleteLsn = NULL ) ;
         INT32       waitEmptyWithCheck() ;

         UINT32      size () ;
         BOOLEAN     isEmpty () ;
         UINT32      idleUnitCount () ;
         UINT32      bucketSize () ;

         INT32       beginUnit( _pmdEDUCB *cb, UINT32 &unitID,
                                INT64 millisec = -1 ) ;
         INT32       endUnit( _pmdEDUCB *cb, UINT32 unitID ) ;
         INT32       submitData( UINT32 unitID, _pmdEDUCB *cb,
                                 clsReplayInfo &info,
                                 CLS_SUBMIT_RESULT &result ) ;

         void        incCurAgent() { _curAgentNum.inc() ; }
         void        decCurAgent() { _curAgentNum.dec() ; }
         UINT32      curAgentNum() { return _curAgentNum.fetch() ; }
         void        incIdleAgent() { _idleAgentNum.inc() ; }
         void        decIdelAgent() { _idleAgentNum.dec() ; }
         UINT32      idleAgentNum() { return _idleAgentNum.fetch() ; }

         DPS_LSN     completeLSN ( BOOLEAN withRetEvent = FALSE ) ;
         DPS_LSN     fastCompleteLSN( UINT32 retryTimes = CLS_FAST_LSN_RETRY_TIMES,
                                      BOOLEAN *pDirty = NULL ) ;

         BOOLEAN     hasPending() ;

         clsCLParallaInfo *getOrCreateInfo( const CHAR *collection,
                                            utilCLUniqueID clUID ) ;
         void        setPending( utilCLUniqueID clUID,
                                 DPS_LSN_OFFSET lsn ) ;
         void        clearParallaInfo() ;

         INT32       waitForLSN( DPS_LSN_OFFSET lsn ) ;

         INT32       waitForIDLSNComp( DPS_LSN_OFFSET nidRecLSN ) ;
         INT32       waitForNIDLSNComp( DPS_LSN_OFFSET idRecLSN ) ;

         void        initUnqIdxLSN() ;
         void        resetUnqIdxLSN( BOOLEAN isEnforced ) ;
         DPS_LSN_OFFSET checkUnqIdxWaitLSN(
                                       dpsUnqIdxHashArray &newUnqIdxHashArray,
                                       dpsUnqIdxHashArray &oldUnqIdxHashArray,
                                       DPS_LSN_OFFSET currentLSN,
                                       UINT32 clHash,
                                       UINT32 bucketID ) ;

      protected:
         void        _submitResult( DPS_LSN_OFFSET offset, DPS_LSN_VER version,
                                    UINT32 lsnLen, clsReplayInfo &info,
                                    UINT32 unitID, CLS_SUBMIT_RESULT &result ) ;
         INT32       _checkAndPushData( UINT32 index, clsReplayInfo &info ) ;
         INT32       _pushData( UINT32 index, clsReplayInfo &info,
                                BOOLEAN incAllCount, BOOLEAN newMem ) ;

         void        _incCount( const CHAR *pData ) ;

         INT32       _doRollback( UINT32 &num ) ;

         INT32       _replay( UINT32 unitID,
                              pmdEDUCB *cb,
                              clsReplayInfo &info,
                              CLS_SUBMIT_RESULT &result ) ;
         INT32       _rollback( UINT32 unitID,
                                pmdEDUCB *cb,
                                clsReplayInfo &info ) ;
         BOOLEAN     _checkCompleted( DPS_LSN_OFFSET offset )
         {
            ossScopedLock lock( &_bucketLatch, SHARED ) ;
            // check both expect LSN or complete map
            return ( _expectLSN.compareOffset( offset ) > 0 ||
                     _completeMap.find( offset ) != _completeMap.end() ) ;
         }

         static void _checkUnqIdxWaitLSN( dpsUnqIdxHashArray &unqIdxHashArray,
                                          DPS_LSN_OFFSET currentLSN,
                                          UINT32 clHash,
                                          UINT32 bucketID,
                                          DPS_LSN_OFFSET &waitLSN,
                                          utilBitmap &unqIdxBitmap,
                                          DPS_LSN_OFFSET *checkLSN,
                                          INT16 *checkBucket ) ;
         static void _saveUnqIdxWaitLSN( dpsUnqIdxHashArray &unqIdxHashArray,
                                         DPS_LSN_OFFSET currentLSN,
                                         UINT32 bucketID,
                                         DPS_LSN_OFFSET *saveLSN,
                                         INT16 *saveBucket ) ;

      private:
         _dpsLogWrapper                   *_pDPSCB ;
         _monDBCB                         *_pMonDBCB ;
         pmdMemPool                       _memPool ;
         _clsReplayer                     *_replayer ;
         CLS_BUCKET_STATUS                _status ;
         UINT32                           _maxReplSync ;

         vector< clsBucketUnit* >         _dataBucket ;
         vector< ossSpinXLatch* >         _latchBucket ;
         UINT32                           _bucketSize ;
         UINT32                           _bitSize ;

         ossAtomic32                      _totalCount ;
         ossAtomic32                      _idleUnitCount ;
         ossAtomic32                      _allCount ;
         ossRWMutex                       _counterLock ;

         ossEvent                         _emptyEvent ;
         ossEvent                         _allEmptyEvent ;
         ossEvent                         _submitEvent ;

         typedef _utilCircularBuffer< UINT32 >  CLS_BUCKET_QUEUE_BUFFER ;
         typedef _utilCircularQueue< UINT32 >   CLS_BUCKET_QUEUE_CONTAINER ;
         typedef ossQueue< UINT32, CLS_BUCKET_QUEUE_CONTAINER >
                                                CLS_BUCKET_QUEUE ;


         CLS_BUCKET_QUEUE_BUFFER          _queueBuffer ;
         CLS_BUCKET_QUEUE *               _ntyQueue ;

         ossAtomic32                      _curAgentNum ;
         ossAtomic32                      _idleAgentNum ;
         ossAtomic32                      _waitAgentNum ;

         // complete map
         CLS_COMP_MAP                     _completeMap ;
         DPS_LSN                          _expectLSN ;
         DPS_LSN_OFFSET                   _maxSubmitOffset ;
         ossSpinSLatch                    _bucketLatch ;

         // result info for error
         INT32                            _submitRC ;

         // duplicated key pending
         utilCLUniqueID                   _pendingCLUniqueID ;

         // parallel info
         MAP_CL_PARALLAINFO               _mapParallaInfo ;

         DPS_LSN_OFFSET                   _lastIDRecParaLSN ;
         DPS_LSN_OFFSET                   _lastNIDRecParaLSN ;

         UINT32               _lastUnqIdxSize ;
         DPS_LSN_OFFSET *     _lastNewUnqIdxLSN ;
         INT16 *              _lastNewUnqIdxBkt ;
         DPS_LSN_OFFSET *     _lastOldUnqIdxLSN ;
         INT16 *              _lastOldUnqIdxBkt ;

         // a bitmap to remember which hash key is already tested
         utilBitmap           _unqIdxBitmap ;

         // cache for last expect LSN
         DPS_LSN_OFFSET       _lastExpectLSN ;

         // for notify full source session when parallel replay
         clsReplayEventHandler   *_replayEventHandler ;
   } ;
   typedef _clsBucket clsBucket ;

   /*
      _clsBucketSyncJob define
   */
   class _clsBucketSyncJob : public rtnBaseJob
   {
      public:
         _clsBucketSyncJob ( clsBucket *pBucket, INT32 timeout = -1 ) ;
         virtual ~_clsBucketSyncJob () ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      protected:
         virtual void _onAttach() ;
         virtual void _onDetach() ;

      private:
         clsBucket               *_pBucket ;
         INT32                   _timeout ;
         BOOLEAN                 _hasEndUnit ;

   } ;
   typedef _clsBucketSyncJob clsBucketSyncJob ;

   /*
      Functions
   */
   INT32 startReplSyncJob( EDUID *pEDUID, clsBucket *pBucket, INT32 timeout ) ;

}

#endif //CLS_REPL_BUCKET_HPP__

