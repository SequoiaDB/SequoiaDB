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

   Source File Name = pmdEnv.hpp

   Descriptive Name = Process MoDel Main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for SequoiaDB,
   and all other process-initialization code.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDENV_HPP_
#define PMDENV_HPP_

#include "utilCommon.hpp"
#include "ossAtomic.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"

using namespace bson ;

namespace engine
{

   /*
      When recieve quit event or signal, will call
   */
   typedef  void (*PMD_ON_QUIT_FUNC)() ;

   /*
      Be called when pmdEDU exit
   */
   typedef  void (*PMD_ON_EDU_EXIT_FUNC)() ;

   PMD_ON_EDU_EXIT_FUNC pmdSetEDUHook( PMD_ON_EDU_EXIT_FUNC hookFunc ) ;
   PMD_ON_EDU_EXIT_FUNC pmdGetEDUHook() ;

   #define  PMD_DOING_STR_LEN          ( 256 )

   /*
      pmd system info define
   */

   // save the number of error occured
   typedef struct _pmdOccurredErr
   {
      UINT64 _oom ;
      UINT64 _noSpc ;
      UINT64 _tooManyOpenFD ;
      ossTimestamp _resetTimestamp ;

      _pmdOccurredErr()
      : _oom( 0 ), _noSpc( 0 ), _tooManyOpenFD( 0 )
      {
         ossGetCurrentTime( _resetTimestamp ) ;
      }
   } pmdOccurredErr ;

   typedef struct _pmdSysInfo
   {
      SDB_ROLE                      _dbrole ;
      MsgRouteID                    _nodeID ;
      ossAtomic32                   _isPrimary ;
      SDB_TYPE                      _dbType ;
      UINT64                        _startTime ;
      UINT16                        _localPort ;

      BOOLEAN                       _quitFlag ;
      PMD_ON_QUIT_FUNC              _pQuitFunc ;

      /// loop updated by pmdSyncClockEntryPoint
      volatile UINT64               _tick ;

      /// loop updated by clsReplicaSet
      volatile UINT64               _validationTick ;

      /// global id
      ossAtomic64                   _globalID ;

      /// doing
      CHAR                          _doing[ PMD_DOING_STR_LEN + 1 ] ;
      _ossSpinXLatch                _latch ;

      /// ulimit info
      ossProcLimits                 _limitInfo ;

      /// number of occurred error
      pmdOccurredErr                _numErr ;

      _pmdSysInfo()
      :_isPrimary( 0 ), _globalID( 1 )
      {
         _dbrole        = SDB_ROLE_STANDALONE ;
         _nodeID.value  = MSG_INVALID_ROUTEID ;
         _quitFlag      = FALSE ;
         _dbType        = SDB_TYPE_DB ;
         _pQuitFunc     = NULL ;
         _startTime     = time( NULL ) ;
         _localPort     = 0 ;
         _tick          = 0 ;
         _validationTick = 0 ;

         ossMemset( _doing, 0, sizeof( _doing ) ) ;
      }
   } pmdSysInfo ;

   SDB_ROLE       pmdGetDBRole() ;
   void           pmdSetDBRole( SDB_ROLE role ) ;
   SDB_TYPE       pmdGetDBType() ;
   void           pmdSetDBType( SDB_TYPE type ) ;
   MsgRouteID     pmdGetNodeID() ;
   void           pmdSetNodeID( MsgRouteID id ) ;
   BOOLEAN        pmdIsPrimary() ;
   void           pmdSetPrimary( BOOLEAN primary ) ;

   UINT64         pmdGetStartTime() ;

   void           pmdSetLocalPort( UINT16 port ) ;
   UINT16         pmdGetLocalPort() ;

   void           pmdSetQuit() ;
   BOOLEAN        pmdIsQuitApp() ;

   void           pmdUpdateDBTick() ;

   UINT64         pmdGetDBTick() ;

   UINT64         pmdGetTickSpanTime( UINT64 lastTick ) ;

   UINT64         pmdDBTickSpan2Time( UINT64 tickSpan ) ;

   void           pmdUpdateValidationTick() ;

   UINT64         pmdGetValidationTick() ;

   void           pmdGetTicks( UINT64 &tick,
                               UINT64 &validationTick ) ;

   BOOLEAN        pmdDBIsAbnormal() ;

   UINT64         pmdAcquireGlobalID() ;

   ossProcLimits* pmdGetLimit() ;

   void           pmdIncErrNum( INT32 rc ) ;
   void           pmdResetErrNum() ;
   pmdOccurredErr pmdGetOccurredErr() ;

   pmdSysInfo*    pmdGetSysInfo () ;

   void           pmdSetDoing( const CHAR *str ) ;
   void           pmdCleanDoing() ;
   void           pmdGetDoing( CHAR *buff, UINT32 size ) ;

   /*
      perf stat
   */
   void           pmdEnablePerfStat( BOOLEAN enablePerfStat ) ;
   BOOLEAN        pmdIsEnabledPerfStat() ;

   /*
      pmd trap functions
   */

   INT32    pmdEnableSignalEvent( const CHAR *filepath,
                                  PMD_ON_QUIT_FUNC pFunc,
                                  INT32 *pDelSig = NULL ) ;

   INT32&   pmdGetSigNum() ;

   void     pmdSleepInstance() ;

   /*
      Env define
   */
   #define  PMD_SIGNUM                 pmdGetSigNum()

}

#endif //PMDENV_HPP_

