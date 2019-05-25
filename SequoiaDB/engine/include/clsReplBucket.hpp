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
         void     push( CHAR *pData, UINT32 len ) ;
         BOOLEAN  pop( CHAR **ppData, UINT32 &len ) ;

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
   struct _clsCompleteInfo
   {
      CHAR           *_pData ;
      UINT32         _len ;
      UINT32         _unitID ;

      _clsCompleteInfo ()
      {
         _pData   = NULL ;
         _len     = 0 ;
         _unitID  = 0 ;
      }
   } ;
   typedef _clsCompleteInfo clsCompleteInfo ;

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

         INT32       init () ;
         void        reset( BOOLEAN setExpect = FALSE ) ;
         void        close() ;
         void        fini () ;

         UINT32      calcIndex( const CHAR *pData, UINT32 len ) ;

         INT32       pushData( UINT32 index, CHAR *pData, UINT32 len ) ;
         BOOLEAN     popData( UINT32 index, CHAR **ppData, UINT32 &len ) ;

         INT32       waitQueEmpty( INT64 millisec = -1 ) ;
         INT32       waitEmpty( INT64 millisec = -1 ) ;
         INT32       waitSubmit( INT64 millisec = -1 ) ;
         INT32       waitEmptyAndRollback( UINT32 *pNum = NULL ) ;
         INT32       waitEmptyWithCheck() ;

         UINT32      size () ;
         BOOLEAN     isEmpty () ;
         UINT32      idleUnitCount () ;
         UINT32      bucketSize () ;

         INT32       beginUnit( _pmdEDUCB *cb, UINT32 &unitID,
                                INT64 millisec = -1 ) ;
         INT32       endUnit( _pmdEDUCB *cb, UINT32 unitID ) ;
         INT32       submitData( UINT32 unitID, _pmdEDUCB *cb,
                                 CHAR *pData, UINT32 len,
                                 CLS_SUBMIT_RESULT &result ) ;

         void        incCurAgent() { _curAgentNum.inc() ; }
         void        decCurAgent() { _curAgentNum.dec() ; }
         UINT32      curAgentNum() { return _curAgentNum.fetch() ; }
         void        incIdleAgent() { _idleAgentNum.inc() ; }
         void        decIdelAgent() { _idleAgentNum.dec() ; }
         UINT32      idleAgentNum() { return _idleAgentNum.fetch() ; }

         DPS_LSN     completeLSN ()
         {
            ossScopedLock lock( &_bucketLatch ) ;
            _submitEvent.reset() ;
            return _expectLSN ;
         }

      protected:
         void        _submitResult( DPS_LSN_OFFSET offset, DPS_LSN_VER version,
                                    UINT32 lsnLen, CHAR *pData, UINT32 len,
                                    UINT32 unitID, CLS_SUBMIT_RESULT &result ) ;
         INT32       _pushData( UINT32 index, CHAR *pData, UINT32 len,
                                BOOLEAN incAllCount, BOOLEAN newMem ) ;

         void        _incCount( const CHAR *pData ) ;

         INT32       _doRollback( UINT32 &num ) ;

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
         ossQueue< UINT32 >               _ntyQueue ;

         ossAtomic32                      _curAgentNum ;
         ossAtomic32                      _idleAgentNum ;

         map< DPS_LSN_OFFSET, clsCompleteInfo >    _completeMap ;
         DPS_LSN                                   _expectLSN ;
         DPS_LSN_OFFSET                            _maxSubmitOffset ;
         ossSpinXLatch                             _bucketLatch ;

         INT32                            _submitRC ;

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

      private:
         clsBucket               *_pBucket ;
         INT32                   _timeout ;

   } ;
   typedef _clsBucketSyncJob clsBucketSyncJob ;

   /*
      Functions
   */
   INT32 startReplSyncJob( EDUID *pEDUID, clsBucket *pBucket, INT32 timeout ) ;

}

#endif //CLS_REPL_BUCKET_HPP__

