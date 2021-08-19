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

   Source File Name = pmdEDU.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Control
   Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDEDU_HPP__
#define PMDEDU_HPP__

#include "sdbInterface.hpp"
#include "ossQueue.hpp"
#include "ossMem.hpp"
#include "ossSocket.hpp"
#include "oss.hpp"
#include "pmdDef.hpp"
#include "ossRWMutex.hpp"
#include "sdbInterface.hpp"
#include "pdTrace.hpp"
#include "dpsDef.hpp"
#include "monCB.hpp"
#include "monEDU.hpp"
#include "utilUniqueID.hpp"
#include "utilMemListPool.hpp"
#include "ossMemPool.hpp"
#include "monClass.hpp"

#if defined ( SDB_ENGINE )
#include "dpsLogDef.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransLockDef.hpp"
#include "pmdTransExecutor.hpp"
#endif // SDB_ENGINE

#include <string>

using namespace std ;

namespace engine
{

   /*
      CONST VALUE DEFINE
   */
   #define PMD_EDU_NAME_LENGTH         MON_EDU_NAME_SZ
   #define EDU_ERROR_BUFF_SIZE         ( 1024 )
   #define EDU_DOING_BUFF_SIZE         MON_EDU_DOING_SZ

   /*
      TOOL FUNCTIONS
   */
   const CHAR *getEDUStatusDesp ( EDU_STATUS status ) ;

   /*
      EDU CONTROL FLAG DEFINE
   */
   #define EDU_CTRL_INTERRUPTED        0x01
   #define EDU_CTRL_DISCONNECTED       0x02
   #define EDU_CTRL_FORCED             0x04

   class _pmdEDUMgr ;

   /*
      _pmdEDUCB define
   */
   class _pmdEDUCB : public _IExecutor
   {
      friend class _SDB_KRCB ;
      friend class _pmdEDUMgr ;

   public:
      typedef ossPoolSet<INT64>              SET_CONTEXT ;

   public:
         /*
            Base Function
         */
         virtual EDUID     getID() const { return _eduID ; }
         virtual UINT32    getTID() const { return _tid ; }

         /*
            Session Related
         */
         virtual ISession*    getSession() { return _pSession ; }
         virtual IRemoteSite* getRemoteSite() { return _pRemoteSite ; }

         /*
            Status and Control
         */
         virtual BOOLEAN   isInterrupted ( BOOLEAN onlyFlag = FALSE ) ;
         virtual BOOLEAN   isDisconnected () ;
         virtual BOOLEAN   isForced () ;

         virtual BOOLEAN   isWritingDB() const { return _writingDB ; }
         virtual UINT64    getWritingID() const { return _writingID ; }
         virtual void      writingDB( BOOLEAN writing ) ;

         virtual UINT32    getProcessedNum() const { return _processEventCount ; }
         virtual void      incEventCount( UINT32 step = 1 ) ;

         virtual UINT32    getQueSize() ;

         /*
            Resource Info
         */
         virtual sdbLockItem* getLockItem( SDB_LOCK_TYPE lockType ) ;
         void                 assertLocks() ;
         void                 resetLocks() ;

         virtual INT32        appendInfo( EDU_INFO_TYPE type, const CHAR * format, ...) ;
         virtual INT32        printInfo ( EDU_INFO_TYPE type, const CHAR *format, ... ) ;
         virtual const CHAR*  getInfo ( EDU_INFO_TYPE type ) ;
         virtual void         resetInfo ( EDU_INFO_TYPE type ) ;

         /*
            Buffer Manager
         */
         virtual INT32     allocBuff( UINT32 len,
                                      CHAR **ppBuff,
                                      UINT32 *pRealSize = NULL ) ;

         virtual INT32     reallocBuff( UINT32 len,
                                        CHAR **ppBuff,
                                        UINT32 *pRealSize = NULL ) ;

         virtual void      releaseBuff( CHAR *pBuff ) ;

         virtual void*     getAlignedBuff( UINT32 size,
                                           UINT32 *pRealSize = NULL,
                                           UINT32 alignment =
                                           OSS_FILE_DIRECT_IO_ALIGNMENT ) ;

         virtual void      releaseAlignedBuff() ;

         virtual CHAR*     getBuffer( UINT32 len ) ;

         virtual void      releaseBuffer() ;

         /*
            Operation Related
         */
         virtual UINT64    getBeginLsn () const { return _beginLsn ; }
         virtual UINT64    getEndLsn() const { return _endLsn ; }
         virtual UINT32    getLsnCount () const { return _lsnNumber ; }
         virtual BOOLEAN   isDoRollback () const { return _doRollback ; }
         virtual UINT64    getTransID () const { return _curTransID ; }
         virtual UINT64    getCurTransLsn () const { return _curTransLSN ; }

         virtual void      resetLsn() ;
         virtual void      insertLsn( UINT64 lsn,
                                      BOOLEAN isRollback = FALSE ) ;
         virtual void      setTransID( UINT64 transID ) ;
         virtual void      setCurTransLsn( UINT64 lsn ) ;

         /*
            Context Related
         */
         virtual void      contextInsert( INT64 contextID ) ;
         virtual void      contextDelete( INT64 contextID ) ;
         virtual INT64     contextPeek() ;
         virtual BOOLEAN   contextFind( INT64 contextID ) ;
         virtual UINT32    contextNum() ;

         INT64             getCurAutoTransCtxID() const ;
         void              setCurAutoTransCtxID( INT64 contextID ) ;
         BOOLEAN           isTransRBPending() const ;
         void              setTransRBPending() ;
         void              clearTransRBPending() ;

         void              setBlock( EDU_BLOCK_TYPE type,
                                     const CHAR *pBlockDesp ) ;
         void              unsetBlock() ;
         EDU_BLOCK_TYPE    getBlockType() const ;
         BOOLEAN           isBlocked() const ;

   public:
      _pmdEDUCB( _pmdEDUMgr *mgr, INT32 type ) ;
      ~_pmdEDUCB() ;

      void        clear() ;
      string      toString() const ;

      EDU_STATUS  getStatus () const { return _status ; }
      INT32       getType () const { return _eduType ; }
      BOOLEAN     isLocked() const { return _isLocked ; }

      _pmdEDUMgr* getEDUMgr() { return _eduMgr ; }

   #if defined ( _LINUX )
      pthread_t   getThreadID () const { return _threadID ; }
      OSSTID      getThreadHandle() const { return _threadHdl ; }
   #elif defined ( _WINDOWS )
      HANDLE      getThreadHandle() const { return _threadHdl ; }
   #endif

   public :
      void        attachSession( ISession *pSession ) ;
      void        detachSession() ;

      void        attachRemoteSite( IRemoteSite *pSite ) ;
      void        detachRemoteSite() ;

      CHAR*       getCompressBuff( UINT32 len ) ;
      INT32       getCompressBuffLen() const { return _compressBuffLen ; }
      CHAR*       getUncompressBuff( UINT32 len ) ;
      INT32       getUncompressBuffLen() const { return _uncompressBuffLen ; }

      void        interrupt ( BOOLEAN onlySelf = FALSE,
                              INT32 interruptRC = SDB_APP_INTERRUPT ) ;
      void        resetInterrupt () ;
      void        resetDisconnect () ;
      BOOLEAN     isOnlySelfWhenInterrupt() const ;
      INT32       getInterruptRC() const ;

      void        updateTransConf() ;

      void        setUserInfo( const string &userName,
                               const string &password ) ;
      void        setName ( const CHAR *name ) ;
      void        setSource( const CHAR *pSource ) ;
      void        setClientSock ( ossSocket *pSock ) { _pClientSock = pSock ; }
      ossSocket*  getClientSock () { return _pClientSock ; }

      BOOLEAN     isFromLocal() const { return _pClientSock ? TRUE : FALSE ; }
      const CHAR* getName () const ;
      const CHAR* getSource() const ;
      const CHAR* getUserName() const { return _userName.c_str() ; }
      const CHAR* getPassword() const { return _passWord.c_str() ; }

      void postEvent ( pmdEDUEvent const &data )
      {
         // no need latch since _queue is already latched
         _queue.push ( data ) ;
      }

      UINT64   getTransWritingID() const { return _transWritingID ; }

      BOOLEAN waitEvent ( pmdEDUEvent &data, INT64 millsec,
                          BOOLEAN resetStat = FALSE )
      {
         // no need latch since _queue is already latched
         // if millsec not 0, that means we want timeout
         // otherwise it's infinite wait

         BOOLEAN waitMsg   = FALSE ;

         if ( resetStat && PMD_EDU_IDLE != _status )
         {
            _status = PMD_EDU_WAITING ;
         }

         if ( 0 > millsec )
         {
            _queue.wait_and_pop ( data ) ;
            waitMsg = TRUE ;
         }
         else
         {
            waitMsg = _queue.timed_wait_and_pop ( data, millsec ) ;
         }

         if ( waitMsg )
         {
            ++_processEventCount ;
            if ( data._eventType == PMD_EDU_EVENT_TERM )
            {
               _ctrlFlag |= ( EDU_CTRL_DISCONNECTED|EDU_CTRL_INTERRUPTED ) ;
               _isInterruptSelf = FALSE ;
            }
            else if ( resetStat )
            {
               _status = PMD_EDU_RUNNING ;
            }
         }

         return waitMsg ;
      }

      BOOLEAN waitEvent( pmdEDUEventTypes type, pmdEDUEvent &data,
                         INT64 millsec, BOOLEAN resetStat = FALSE )
      {
         BOOLEAN ret = FALSE ;
         INT64 waitTime = 0 ;
         ossQueue< pmdEDUEvent > tmpQue ;

         if ( millsec < 0 )
         {
            millsec = 0x7FFFFFFF ;
         }

         while ( !isInterrupted() )
         {
            waitTime = millsec < OSS_ONE_SEC ? millsec : OSS_ONE_SEC ;
            if ( !waitEvent( data, waitTime, resetStat ) )
            {
               millsec -= waitTime ;
               if ( millsec <= 0 )
               {
                  break ;
               }
               continue ;
            }
            if ( type != data._eventType )
            {
               tmpQue.push( data ) ;
               --_processEventCount ;
               continue ;
            }
            ret = TRUE ;
            break ;
         }

         pmdEDUEvent tmpData ;
         while ( !tmpQue.empty() )
         {
            tmpQue.try_pop( tmpData ) ;
            _queue.push( tmpData ) ;
         }
         return ret ;
      }

      void contextCopy( SET_CONTEXT &contextList ) ;

      void           resetMon() { _monApplCB.reset () ; }
      monConfigCB*   getMonConfigCB() { return & _monCfgCB ; }
      monAppCB*      getMonAppCB() { return & _monApplCB ; }
      void           setMonQueryCB ( monClassQuery *monQueryCB )
      {
         _monQueryCB = monQueryCB ;
      }
      monClassQuery* getMonQueryCB () { return _monQueryCB ; }
      void           dumpInfo( monEDUSimple &simple ) ;
      void           dumpInfo( monEDUFull &full ) ;

      void registerMonCRUDCB ( monCRUDCB * monCRUDCB )
      {
         _monApplCB.setCRUDCB( monCRUDCB ) ;
      }

      void unregisterMonCRUDCB ()
      {
         _monApplCB.setCRUDCB( NULL ) ;
      }

   #if defined ( SDB_ENGINE )

      ossEvent & getEvent () { return _event ; }

      UINT64 getCurRequestID() const { return _curRequestID ; }
      // WANRING: no lock protect, only called by eduCB thread itself
      UINT64 incCurRequestID() { return ++_curRequestID ; }

      // transaction related
      void     setRelatedTransLSN( DPS_LSN_OFFSET relatedLSN )
      {
         _relatedTransLSN = relatedLSN ;
      }
      DPS_LSN_OFFSET getRelatedTransLSN() const { return _relatedTransLSN ; }
      BOOLEAN  isTransaction() const ;
      BOOLEAN  isTransRU () const ;
      BOOLEAN  isTransRC () const ;
      BOOLEAN  isTransRS () const ;
      BOOLEAN  isAutoCommitTrans() const ;
      void     startTransRollback() { _isDoTransRollback = TRUE ; }
      void     stopTransRollback() { _isDoTransRollback = FALSE ; }
      BOOLEAN  isInTransRollback() const { return _isDoTransRollback ; }
      void     setTransRC( INT32 rc ) { _transRC = rc ; }
      INT32    getTransRC() const { return _transRC ; }
      void     setTransStatus( INT32 status ) { _transStatus = status ; }
      INT32    getTransStatus() const { return _transStatus ; }
      void     clearTransInfo() ;

      void     dumpTransInfo( monTransInfo &transInfo ) ;

      pmdTransExecutor*    getTransExecutor() ;

      void     addReservedSpace( const UINT64 len )
      {
         _transExecutor.addReservedSpace( len ) ;
      }

      UINT64   getReservedSpace() const
      {
         return _transExecutor.getReservedSpace();
      }

      void     resetLogSpace()
      {
         _transExecutor.resetLogSpace();
      }

      void     setOrgReplSize( INT16 replSize ) { _orgReplSize = replSize ; }
      INT16    getOrgReplSize() const { return _orgReplSize ; }
   #endif // SDB_ENGINE

   protected:
      void     setStatus ( EDU_STATUS status ) { _status = status ; }
      void     setID ( EDUID id ) { _eduID = id ; }
      void     setLock( BOOLEAN lock ) { _isLocked = lock ; }

      CHAR*    _getBuffInfo ( EDU_INFO_TYPE type, UINT32 &size ) ;

      void     disconnect () ;
      void     force () ;

   private:
      void        setType( INT32 type ) ;
      void        setTID ( UINT32 tid ) { _tid = tid ; }

   #if defined ( _LINUX )
      void        setThreadID ( pthread_t id ) { _threadID = id ; }
      void        setThreadHdl( OSSTID hdl ) { _threadHdl = hdl ; }
   #elif defined ( _WINDOWS )
      void        setThreadHdl( HANDLE hdl ) { _threadHdl = hdl ; }
   #endif

      void        initMonAppCB() ;

      void        initTransConf() ;

      /*
         Only for pmdEDUMgr call, and must under pmdEDUMgr::_latch protected
      */
      BOOLEAN checkAndSwapDumpTransCount( INT32 checkValue, INT32 newValue )
      {
         return _dumpTransCount.compareAndSwap( checkValue, newValue ) ;
      }

      INT32 getDumpTransCount() { return _dumpTransCount.fetch() ; }

   private :
      _pmdEDUMgr     *_eduMgr ;
      monSpinSLatch  _mutex ;
      ossQueue<pmdEDUEvent> _queue ;

      EDU_STATUS     _status ;
      ossAtomic32    _dumpTransCount ;

      INT32          _eduType ;
      BOOLEAN        _isLocked ;

      string         _userName ;
      string         _passWord ;

      // buffer related
      CHAR           *_pCompressBuff ;
      UINT32         _compressBuffLen ;
      CHAR           *_pUncompressBuff ;
      UINT32         _uncompressBuffLen ;

      // thread specific error message buffer, aka SQLCA
      CHAR              *_pErrorBuff ;
      CHAR              _doingBuff[ EDU_DOING_BUFF_SIZE + 1 ] ;
   #if defined ( _WINDOWS )
      HANDLE            _threadHdl ;
   #elif defined ( _LINUX )
      OSSTID            _threadHdl ;
      pthread_t         _threadID ;
   #endif // _WINDOWS

      CHAR              _name[ PMD_EDU_NAME_LENGTH + 1 ] ;
      CHAR              _source[ PMD_EDU_NAME_LENGTH + 1 ] ;
      ossSocket        *_pClientSock ;

      // indicate transaction is rolling back
      BOOLEAN                 _isDoTransRollback ;

      monAppCB                _monApplCB ;
      monConfigCB             _monCfgCB ;

   #if defined ( SDB_ENGINE )
      ossEvent                _event ;   // for cls replSet notify
      UINT64                  _curRequestID ;

      // transaction related variables
      DPS_LSN_OFFSET          _relatedTransLSN ;
      INT32                   _transRC ;
      INT32                   _transStatus ;

      pmdTransExecutor        _transExecutor ;
      UINT32                  _confChangeID ;

      INT16                   _orgReplSize ;

   #endif // SDB_ENGINE

      /*
         Interface members
      */
      EDUID                   _eduID ;
      UINT32                  _tid ;
      ISession                *_pSession ;
      IRemoteSite             *_pRemoteSite ;

      UINT64                  _beginLsn ;
      UINT64                  _endLsn ;
      UINT32                  _lsnNumber ;
      // indicate to rollback DPS log
      BOOLEAN                 _doRollback ;
      UINT64                  _processEventCount ;

      DPS_TRANS_ID            _curTransID ;
      DPS_LSN_OFFSET          _curTransLSN ;
      UINT64                  _transWritingID ;

      sdbLockItem             _lockInfo[ SDB_LOCK_MAX ] ;

      INT32                   _ctrlFlag ;
      BOOLEAN                 _isInterruptSelf ;
      /// return code for interrupted EDU
      INT32                   _interruptRC ;
      BOOLEAN                 _writingDB ;
      UINT64                  _writingID ;
      EDU_BLOCK_TYPE          _blockType ;
      /// aligned memory.
      void                    *_alignedMem ;
      UINT32                   _alignedMemSize ;
      /// buffer memory
      CHAR                    *_pBuffer ;
      UINT32                  _buffSize ;

      SET_CONTEXT             _contextList ;
      INT64                   _curAutoTransCtxID ;
      utilMemListPool         *_pMemPool ;
      monClassQuery           *_monQueryCB ;
   };
   typedef class _pmdEDUCB pmdEDUCB ;

   _pmdEDUCB* pmdGetThreadEDUCB() ;
   _pmdEDUCB* pmdDeclareEDUCB( _pmdEDUCB *p ) ;
   void       pmdUndeclareEDUCB() ;

   /*
      TOOL FUNCTIONS
   */
   INT32 pmdRecv ( CHAR *pBuffer, INT32 recvSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                   INT32 forceTimeout = -1 ) ;
   INT32 pmdSend ( const CHAR *pBuffer, INT32 sendSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
   /*
      NOTE: the ppRecvMsg is alloced, so need to free
      useCBMem: TRUE: the memory is allocated by cb->allocBuf, so must free
                by cb->releaseBuf
                FALSE: the memory is allocated by SDB_OSS_MALLOC, so must
                free by SDB_OSS_FREE
   */
   INT32 pmdSyncSendMsg( const MsgHeader *pMsg, pmdEDUEvent &recvEvent,
                         ossSocket *sock, pmdEDUCB *cb,
                         INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                         INT32 forceTimeout = -1 ) ;

   /*
      NOTE: recv the msg to cb queue
   */
   INT32 pmdSendAndRecv2Que( const MsgHeader *pMsg, ossSocket *sock,
                             pmdEDUCB *cb,
                             INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                             INT32 forceTimeout = -1 ) ;

   void  pmdEduEventRelease( pmdEDUEvent &event, pmdEDUCB *cb ) ;

}

#endif // PMDEDU_HPP__

