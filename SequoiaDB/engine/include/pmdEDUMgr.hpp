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

   Source File Name = pmdEDUMgr.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Pool, which
   include operations like creating a new EDU, reuse existing EDU, destroy EDU
   and etc...

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDEDUMGR_HPP__
#define PMDEDUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "pmdEDU.hpp"
#include "monEDU.hpp"
#include "pmdEntryPoint.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"

#include <map>
#include <set>
#include <string>

namespace engine
{

   #define PMD_EDU_WAIT_PERIOD     (200)
   #define PMD_EDU_WAIT_ROUND      (600)

   /*
      Property Define
   */
   #define EDU_SYSTEM            0x0001
   #define EDU_USER              0x0002
   #define EDU_ALL               ( EDU_SYSTEM | EDU_USER )

   typedef boost::shared_ptr<ossEvent>       pmdEventPtr ;

   #define PMD_STOP_TIMEOUT      ( 600000 )  /// 10 mins

   /*
      _pmdEDUMgr define
   */
   class _pmdEDUMgr : public IExecutorMgr
   {
      typedef vector< IIOService* >       VEC_IOSERVICE ;
      typedef map<EDUID, pmdEDUCB*>       MAP_EDUCB ;
      typedef MAP_EDUCB::iterator         MAP_EDUCB_IT ;
      typedef map<UINT32,EDUID>           MAP_TID2EDU ;
      typedef MAP_TID2EDU::iterator       MAP_TID2EDU_IT ;
      typedef map<INT32,EDUID>            MAP_SYSTEMEDU ;
      typedef MAP_SYSTEMEDU::iterator     MAP_SYSTEMEDU_IT ;

      public:
         _pmdEDUMgr () ;
         virtual ~_pmdEDUMgr () ;

         virtual INT32     startEDU( INT32 type,
                                     void *args,
                                     EDUID *pEDUID = NULL,
                                     const CHAR *pInitName = "" ) ;

         virtual void      addIOService( IIOService *pIOService ) ;
         virtual void      delIOSerivce( IIOService *pIOService ) ;

      public:
         INT32             init( IResource *pResource ) ;
         BOOLEAN           reset( INT64 timeout = PMD_STOP_TIMEOUT ) ;
         UINT32            dumpAbnormalEDU() ;

         UINT32            countIOService() ;
         UINT32            size() ;
         UINT32            sizeRun() ;
         UINT32            sizeIdle() ;
         UINT32            sizeSystem() ;
         void              sizeInfo( UINT32 &runSize,
                                     UINT32 &idleSize,
                                     UINT32 &sysSize ) ;

         EDUID             getSystemEDU( INT32 eduType ) ;
         BOOLEAN           isSystemEDU( EDUID eduID ) ;

         BOOLEAN           isQuiesced() ;
         BOOLEAN           isDestroyed() ;

         INT32             waitEDU( EDUID eduID ) ;
         INT32             waitEDU( pmdEDUCB *cb ) ;

         INT32             activateEDU( EDUID eduID ) ;
         INT32             activateEDU( pmdEDUCB *cb ) ;

         INT32             createIdleEDU( EDUID *pEDUID ) ;

         INT32             forceUserEDU( EDUID eduID ) ;
         INT32             interruptUserEDU( EDUID eduID ) ;
         INT32             disconnectUserEDU( EDUID eduID ) ;

         INT32             postEDUPost ( EDUID eduID,
                                         pmdEDUEventTypes type,
                                         pmdEDUMemTypes dataMemType = PMD_EDU_MEM_NONE,
                                         void *pData = NULL,
                                         UINT64 usrData = 0 ) ;

         UINT32            interruptWritingEDUS() ;
         UINT32            getWritingEDUCount( INT32 eduTypeFilter = -1,
                                               UINT64 idThreshold = 0 ) ;

         void              resetMon( EDUID eduID = PMD_INVALID_EDUID ) ;
         void              resetIOService() ;
         void              dumpInfo( set<monEDUSimple> &info ) ;
         void              dumpInfo( set<monEDUFull> &info ) ;

#if defined(SDB_ENGINE)
         INT32             dumpTransInfo( EDUID eduId,
                                          monTransInfo &transInfo ) ;
#endif //SDB_ENGINE

         pmdEDUCB*         getEDUByID( EDUID eduID ) ;
         INT32             getEDUTypeByID( EDUID eduID ) ;
         pmdEDUCB*         getEDU() ;
         pmdEDUCB*         getEDU( UINT32 tid ) ;

         INT32             waitUntil( EDUID eduID,
                                      EDU_STATUS status,
                                      UINT32 waitPeriod = PMD_EDU_WAIT_PERIOD,
                                      UINT32 waitRound = PMD_EDU_WAIT_ROUND ) ;
         INT32             waitUntilByType( INT32 type,
                                            EDU_STATUS status,
                                            UINT32 waitPeriod = PMD_EDU_WAIT_PERIOD,
                                            UINT32 waitRound = PMD_EDU_WAIT_ROUND ) ;

#if defined (_LINUX)
         void              killByThreadID( INT32 signo ) ;
         void              getEDUThreadID ( set<pthread_t> &tidList ) ;
#endif //_LINUX

      protected:
         void              forceIOSerivce() ;

         INT32             createNewEDU ( INT32 type,
                                          BOOLEAN isSystem,
                                          void* arg,
                                          EDUID *pEDUID,
                                          const CHAR *pInitName ) ;

         pmdEDUCB*         getFromPool( INT32 type ) ;

         void              _forceEDUs( INT32 property = EDU_ALL ) ;
         UINT32            _getEDUCount( INT32 property = EDU_ALL ) ;

         UINT32            _interruptWritingEDUs() ;
         UINT32            _getWritingEDUCount( INT32 eduTypeFilter = -1,
                                                UINT64 idThreshold = 0 ) ;

         void              setDestroyed( BOOLEAN b ) ;
         void              setQuiesced( BOOLEAN b ) ;

         BOOLEAN           destroyAll( INT64 timeout = -1 ) ;

         void              setEDU ( UINT32 tid, EDUID eduid ) ;
         void              returnEDU( pmdEDUCB *cb, BOOLEAN &destroyed ) ;
         BOOLEAN           forceDestory( pmdEDUCB *cb, UINT32 idleTime ) ;

         pmdEDUCB*         findAndRemove( EDUID eduID ) ;

         UINT32            calIdleLowSize( UINT32 *pRunSize = NULL,
                                           UINT32 *pIdleSize = NULL,
                                           UINT32 *pSysSize = NULL,
                                           UINT32 *pPoolSize = NULL ) ;

      private:
         BOOLEAN           _isSystemEDU( pmdEDUCB *cb ) ;
         pmdEDUCB*         _getEDUByID( EDUID eduID ) ;
         void              _postDestoryEDU( pmdEDUCB *cb ) ;
         UINT32            _calIdleLowSize( UINT32 runSize,
                                            UINT32 idleSize,
                                            UINT32 sysSize,
                                            UINT32 poolSize ) ;

      private:
         VEC_IOSERVICE              _vecIOServices ;
         ossSpinSLatch              _latch ;
         EDUID                      _EDUID ;

         MAP_EDUCB                  _mapRuns ;
         MAP_EDUCB                  _mapIdles ;

         MAP_TID2EDU                _mapTid2Edu ;
         MAP_SYSTEMEDU              _mapSystemEdu ;

         BOOLEAN                    _isDestroyed ;
         BOOLEAN                    _isQuiesced ;

         IResource                  *_pResource ;
         boost::thread              *_pMonitorThd ;

         ossAutoEvent               _monitorEvent ;

      private:
         /*
            Entry Functions define
         */
         INT32 pmdEDUEntryPointWrapper( pmdEDUCB *cb, pmdEventPtr ePtr ) ;
         INT32 pmdEDUEntryPoint( pmdEDUCB *cb,
                                 pmdEventPtr ePtr,
                                 BOOLEAN &quitWithException ) ;
         void  monitor() ;
   };
   typedef _pmdEDUMgr pmdEDUMgr ;

}

#endif // PMDEDUMGR_HPP__
