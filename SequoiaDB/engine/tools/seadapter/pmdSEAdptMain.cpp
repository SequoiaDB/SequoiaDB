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

   Source File Name = pmdSEAdapterMain.cpp

   Descriptive Name = Search engine adapter main entry.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdProc.hpp"
#include "seAdptDef.hpp"
#include "seAdptMgr.hpp"
#include "omagentDef.hpp"
#include "utilProcessLock.hpp"

namespace seadapter
{
   INT32 pmdResolveArguments( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      CHAR exePath[ OSS_MAX_PATHSIZE + 1 ] = {0} ;

      rc = ossGetEWD( exePath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Get module path failed[ %d ]", rc ) ;
         goto error ;
      }

      rc = sdbGetSeAdptOptions()->init( argc, argv, exePath ) ;
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         goto done ;
      }
      else if ( rc )
      {
         ossPrintf( "Initialize configuration failed[ %d ]", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 buildDialogPath( CHAR *dialogPath, UINT32 bufSize )
   {
      // Create the log file directory.
      INT32 rc = SDB_OK ;
      CHAR currentPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      if ( bufSize < OSS_MAX_PATHSIZE + 1 )
      {
         ossPrintf( "Path buffer size is too small: %u", bufSize ) ;
         goto error ;
      }

      rc = ossGetEWD( currentPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Get working directory failed: %d", rc ) ;
         goto error ;
      }

      ossChDir( currentPath ) ;

      // conf/log
      rc = utilBuildFullPath( currentPath, SDBCM_LOG_PATH,
                              OSS_MAX_PATHSIZE, dialogPath ) ;
      if ( rc )
      {
         ossPrintf( "Build log path failed: %d", rc ) ;
         goto error ;
      }

      // conf/log/seadapterlog
      rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE, SEADPT_LOG_DIR ) ;
      if ( rc )
      {
         ossPrintf( "Build log path failed: %d", rc ) ;
         goto error ;
      }

      // conf/log/seadapterlog/SERVICE
      rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE,
                        sdbGetSeAdptOptions()->getDBService() ) ;
      if ( rc )
      {
         ossPrintf( "Build log path failed: %d", rc ) ;
         goto error ;
      }

      rc = ossMkdir( dialogPath ) ;
      if ( rc )
      {
         if ( SDB_FE != rc )
         {
            ossPrintf( "Make dialog path[ %s ] failed: %d", dialogPath, rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void pmdOnQuit()
   {
      PMD_SHUTDOWN_DB( SDB_INTERRUPT ) ;
      iPmdProc::stop( 0 ) ;
   }

   // Wait time for all daemon threads start, based on millisecond
   #define PMD_START_WAIT_TIME      ( 60000 )

   INT32 pmdThreadMainEntry( INT32 argc, CHAR** argv )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      UINT32 startTimerCount = 0 ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR dialogPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR lockFilePath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      po::variables_map vm ;
      utilProcFlockMutex procMutex ;

      rc = pmdResolveArguments( argc, argv ) ;
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         ossPrintf( "Failed resolving arguments(error=%d), exit" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      rc = buildDialogPath( dialogPath, OSS_MAX_PATHSIZE + 1 ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog path(error=%d), exit" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      {
         // To avoid multiple adapters connecting to the same data node by using
         // the same configuration file, or with the same db node service.
         ossStrncpy( lockFilePath, dialogPath, OSS_MAX_PATHSIZE + 1 ) ;
         rc = utilCatPath( lockFilePath, OSS_MAX_PATHSIZE,
                           SEADPT_LOCK_FILE_NAME ) ;
         if ( rc )
         {
            ossPrintf( "Build path for lock file failed[%d]" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = procMutex.init( "sdbseadapter", (void *)lockFilePath ) ;
         if ( rc )
         {
            ossPrintf( "Init process mutex failed[ %d ]" OSS_NEWLINE, rc ) ;
            goto error ;
         }

         rc = procMutex.tryLock() ;
         if ( rc )
         {
            ossPrintf( "Lock process mutex failed. Maybe another process is "
                       "running using the same configuration file, or try to "
                       "connect to the same data node" ) ;
            goto error ;
         }
      }

      // conf/log/seadapterlog/SERVICE/sdbseadapter.log
      rc = utilCatPath( dialogPath, OSS_MAX_PATHSIZE, SEADPT_LOG_FILE_NAME ) ;
      if ( rc )
      {
         ossPrintf( "Failed to build dialog path failed(error=%d), "
                    "exit" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      sdbEnablePD( dialogPath ) ;
      setPDLevel( (PDLEVEL)( sdbGetSeAdptOptions()->getDiagLevel() ) ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( ( getPDLevel() > PDEVENT ? PDEVENT: getPDLevel() ),
              "Start sdbseadapter[%s]...", verText ) ;

      // Print all configurations in log file.
      {
         string configs ;
         sdbGetSeAdptOptions()->toString( configs ) ;
         PD_LOG( PDEVENT, "All configs:\n%s", configs.c_str() ) ;
      }

      rc = pmdEnableSignalEvent( dialogPath, (PMD_ON_QUIT_FUNC)pmdOnQuit,
                                 NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Enable trap failed[ %d ]", rc ) ;

      PMD_REGISTER_CB( sdbGetSeAdapterCB() ) ;

      rc = krcb->init() ;
      PD_RC_CHECK( rc, PDERROR, "Initialize krcb failed[ %d ]", rc ) ;

      // wait until all daemon threads start
      while ( PMD_IS_DB_UP() && startTimerCount < PMD_START_WAIT_TIME &&
              !krcb->isBusinessOK() )
      {
         ossSleepmillis( 100 ) ;
         startTimerCount += 100 ;
      }

      if ( PMD_IS_DB_DOWN() )
      {
         rc = krcb->getShutdownCode() ;
         PD_LOG( PDERROR, "Start failed, rc: %d", rc ) ;
         goto error ;
      }
      else if ( startTimerCount >= PMD_START_WAIT_TIME )
      {
         PD_LOG( PDWARNING, "Start warning(timeout)" ) ;
      }

#if defined (_LINUX)
      {
         // Rename the process, adding the service name and role.
         CHAR pmdProcessName[OSS_RENAME_PROCESS_BUFFER_LEN + 1] = {0} ;
         ossSnprintf( pmdProcessName, OSS_RENAME_PROCESS_BUFFER_LEN,
                      "%s(%s) %s", SEADPT_PROCESS_NAME,
                      sdbGetSeAdptOptions()->getSvcName(),
                      SEADPT_ROLE_SHORT_STR ) ;
         ossEnableNameChanges( argc, argv ) ;
         ossRenameProcess( pmdProcessName ) ;
      }
#endif /* _LINUX */

      while ( PMD_IS_DB_UP() )
      {
         ossSleepsecs( 1 ) ;
      }
      rc = krcb->getShutdownCode() ;

   done:
      PMD_SHUTDOWN_DB( rc ) ;
      pmdSetQuit() ;
      krcb->destroy() ;
      pmdDisableSignalEvent() ;
      PD_LOG( PDEVENT, "Stop program, exit code: %d",
              krcb->getShutdownCode() ) ;
      procMutex.destroy() ;
      return SDB_OK == rc ? 0 : 1 ;
   error:
      goto done ;
   }
}

INT32 main( INT32 argc, CHAR** argv )
{
   return seadapter::pmdThreadMainEntry( argc, argv ) ;
}

