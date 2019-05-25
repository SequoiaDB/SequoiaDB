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

   Source File Name = pmdDaemon.hpp

   Descriptive Name = pmdDaemon

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDDAEMON_HPP__
#define PMDDAEMON_HPP__

#include "ossFeat.h"
#include "ossTypes.h"
#include "ossShMem.hpp"
#include "oss.h"
#include "pmdProc.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "omagentDef.hpp"

namespace engine
{
   /*
      Local define
   */
   #define PMDDMN_SVCNAME_DEFAULT            "sdbcmd"
   #define PMDDMN_DIALOG_FILE_NAME           PMDDMN_SVCNAME_DEFAULT".log"
   #define PMDDMN_STOP_WAIT_TIME             (60*1000)

#if defined (_LINUX)
   #define PMDDMN_SHMKEY_DEFAULT             50010
   #define PMDDMN_EXE_NAME                   PMDDMN_SVCNAME_DEFAULT
#elif defined (_WINDOWS)
   #define PMDDMN_SHMKEY_DEFAULT             "50010"
   #define PMDDMN_EXE_NAME                   PMDDMN_SVCNAME_DEFAULT".exe"
#endif

   /*
      pmdDMNSHMStat define
   */
   enum pmdDMNSHMStat
   {
      PMDDMN_SHM_STAT_DAEMON     = 0,  // the daemon-process will modify the buffer
      PMDDMN_SHM_STAT_CHILDREN   = 1   // the children-process will modify the buffer
   } ;

   /*
      pmdDMNSHMCmd define
   */
   enum pmdDMNSHMCmd
   {
      PMDDMN_SHM_CMD_INVALID     = 0,

      PMDDMN_SHM_CMD_DMN_BEGIN   = 1,
      PMDDMN_SHM_CMD_DMN_QUIT    = 2,

      PMDDMN_SHM_CMD_DMN_END     = 0x00FF,

      PMDDMN_SHM_CMD_CHL_BEGIN   = 0x0100,
      PMDDMN_SHM_CMD_CHL_QUIT    = 0x0200,

      PMDDMN_SHM_CMD_CHL_END     = 0xFF00
   } ;
   #define PMDDMN_SHM_CMD_DMN_MASK        0x00FF
   #define PMDDMN_SHM_CMD_CHL_MASK        0xFF00

   /*
      _pmdDMNProcInfo define
   */
   struct _pmdDMNProcInfo
   {
      CHAR           szTag[32] ;
      OSSPID         pid ;
      pmdDMNSHMStat  stat ;
      INT32          cmd ;
      INT32          exitCode ;
      UINT32         sn ;

      _pmdDMNProcInfo() ;
      BOOLEAN isInit() ;
      void init() ;
      INT32 setDMNCMD( pmdDMNSHMCmd newCMD ) ;
      INT32 setCHLCMD( pmdDMNSHMCmd newCMD ) ;
      INT32 getDMNCMD() ;
      INT32 getCHLCMD() ;
   } ;
   typedef _pmdDMNProcInfo pmdDMNProcInfo ;

   /*
      iPmdDMNChildProc define
   */
   class iPmdDMNChildProc : public iPmdProc
   {
   private:
      virtual const CHAR *getProgramName() = 0 ;
      virtual const CHAR *getArguments(){ return NULL ; }

   public:
      iPmdDMNChildProc() ;
      virtual ~iPmdDMNChildProc() ;
      virtual INT32 init( ossSHMKey shmKey = PMDDMN_SHMKEY_DEFAULT ) ;
      virtual INT32 fini() ;

      virtual  const CHAR *getExecuteFile() ;

      void     syncProcesserInfo() ;
      void     waitSync() ;

      void     lock() ;
      void     unlock() ;

      /*
         Following function is called by daemon
      */
      BOOLEAN  isChildRunning() ;
      INT32    start() ;
      INT32    stop() ;     

      INT32    active() ;
      void     deactive() ;

      void     signal() ;
      INT32    wait( INT64 timeout = PMDDMN_STOP_WAIT_TIME ) ;

   protected:
      INT32    DMNProcessCMD( INT32 cmd ) ;
      INT32    ChildProcessCMD( INT32 cmd ) ;

   private:
      INT32    allocSHM( ossSHMKey shmKey ) ;
      INT32    allocSHM() ;
      INT32    attachSHM( ossSHMKey shmKey ) ;
      INT32    attachSHM() ;
      void     detachSHM() ;
      void     freeSHM() ;

   private:
      pmdDMNProcInfo       *_procInfo ;
      ossSHMMid            _shmMid ;
      UINT32               _deadTime ;
      CHAR                 _execName[ OSS_MAX_PATHSIZE + 1 ] ;
      ossSHMKey            _shmKey ;
      OSSPID               _pid ;
#if defined ( _WINDOWS )
      ossSpinXLatch        _mutex ;
      ossEvent             _event ;
#endif // _WINDOWS

      ossEvent             _syncEvent ;

   } ;

   /*
      cPmdDaemon define
   */
   class cPmdDaemon : public _iPmdProc
   {
   public:
      cPmdDaemon( const CHAR *pDMNSvcName ) ;
      virtual ~cPmdDaemon() ;

      INT32 run( INT32 argc, CHAR **argv, BOOLEAN asProc ) ;
      INT32 addChildrenProcess( iPmdDMNChildProc *childProc ) ;
      void  stop() ;

   public:
      INT32 init() ;

   private:
      cPmdDaemon(){} ;
      static INT32 _run( INT32 argc, CHAR **argv ) ;

   private:
      CHAR                       _procName[OSS_MAX_PATHSIZE + 1] ;
      static iPmdDMNChildProc    *_process ;

   } ;

   /*
      cCMService define
   */
   class cCMService : public iPmdDMNChildProc
   {
   public:
      cCMService() ;
      virtual ~cCMService() ;

      void  setArgInfo( INT32 argc, CHAR **argv ) ;

      virtual INT32 init( ossSHMKey shmKey = PMDDMN_SHMKEY_DEFAULT ) ;
      virtual INT32 fini() ;

      virtual const CHAR *getProgramName() ;

   private:
      const CHAR *getArguments() ;
      CHAR        *_pArgs ;
      INT32       _argLen ;

      INT32       _argc ;
      CHAR        **_argv ;

   } ;

}

#endif // PMDDAEMON_HPP__

