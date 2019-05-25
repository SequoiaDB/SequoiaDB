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
          12/09/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdWinService.hpp"
#include "ossErr.h"
#include "pd.hpp"
#include "ossUtil.h"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#if defined (_WINDOWS)

TCHAR g_service_name[ PMD_WINSVC_SVCNAME_MAX_LEN + 1 ] = {0};
PMD_WINSERVICE_FUNC g_service_fun = NULL;
SERVICE_STATUS g_service_status;
SERVICE_STATUS_HANDLE g_status_handle;
HANDLE g_wait_event = NULL;


void WINAPI pmdWinServiceMain( DWORD argc, LPTSTR *argv );
void WINAPI pmdWinServiceCtrlHandler( DWORD control );
BOOLEAN pmdWinSvcReportStatusToSCMgr( DWORD dwStatus );


//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDWINSTARTSVC, "pmdWinstartService" )
INT32 WINAPI pmdWinstartService( const CHAR *pServiceName,
                                 PMD_WINSERVICE_FUNC svcFun )
{
   INT32 rc = SDB_OK;
   PD_TRACE_ENTRY ( SDB_PMDWINSTARTSVC );
   SDB_ASSERT( pServiceName, "service name can't be null!" );
   SDB_ASSERT( svcFun, "service function can't be null!" );
   SDB_ASSERT( g_service_name[0] == 0 && NULL == g_service_fun,
               "don't dulicate register service!" );

   UINT32 nameLen = ossStrlen( pServiceName );
   SDB_ASSERT( nameLen <= PMD_WINSVC_SVCNAME_MAX_LEN,
               "invalid service name size!" );
   PD_CHECK( nameLen <= PMD_WINSVC_SVCNAME_MAX_LEN, SDB_INVALIDARG, error,
            PDERROR, "invalid service name size(%u)", nameLen );
   MultiByteToWideChar( CP_ACP, 0, pServiceName, -1,
                        g_service_name, PMD_WINSVC_SVCNAME_MAX_LEN );
   g_service_fun = svcFun;
   
   SERVICE_TABLE_ENTRY serviceTable[] = {
      { g_service_name, pmdWinServiceMain },
      { NULL, NULL }
   };
   if ( !StartServiceCtrlDispatcher ( serviceTable ) )
   {
      DWORD errorNumber = GetLastError();
      rc = SDB_SYS;
      PD_LOG( PDERROR,
            "failed to start service, GetLastError()=%d",
            errorNumber );
      goto error;
   }
done:
   PD_TRACE_EXITRC ( SDB_PMDWINSTARTSVC, rc );
   return rc;
error:
   goto done;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDWINSVC_STPSVC, "pmdWinStopService" )
VOID pmdWinStopService(LPTSTR lpszMsg)
{
    PD_TRACE_ENTRY ( SDB_PMDWINSVC_STPSVC );
    TCHAR chMsg[256];
    HANDLE hEventSource;
    LPTSTR lpszStrings[2];

    DWORD errNo = GetLastError();

    hEventSource = RegisterEventSource(NULL , g_service_name);
    wprintf_s( chMsg, 256, L"%s error:%d", g_service_name, errNo);
    lpszStrings[0] = chMsg;
    lpszStrings[1] = lpszMsg;

    if (hEventSource != NULL )
    {
        ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, 0, NULL,
                     2, 0, (LPCTSTR*)lpszStrings, NULL);
        (VOID)DeregisterEventSource(hEventSource);
    }

    SetEvent(g_wait_event);
    PD_TRACE_EXIT ( SDB_PMDWINSVC_STPSVC );
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDWINSVCMAIN, "pmdWinServiceMain" )
void WINAPI pmdWinServiceMain( DWORD argc, LPTSTR *argv )
{
   PD_TRACE_ENTRY ( SDB_PMDWINSVCMAIN );
   SDB_ASSERT( g_service_fun, "service function can't be null!" );
   DWORD dwWait;
   if ( NULL == g_service_fun )
   {
      PD_LOG ( PDERROR, "service function can't be null!" );
      goto error;
   }
   g_status_handle = RegisterServiceCtrlHandler(g_service_name,
         pmdWinServiceCtrlHandler );
   if ( (SERVICE_STATUS_HANDLE)NULL == g_status_handle )
   {
      PD_LOG ( PDERROR,
               "failed to register service(%s) control handle",
               g_service_name );
      goto error;
   }
   g_service_status.dwServiceType = SERVICE_WIN32;
   g_service_status.dwCurrentState = SERVICE_START_PENDING;
   g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                       SERVICE_ACCEPT_SHUTDOWN ;
   g_service_status.dwWin32ExitCode = 0;
   g_service_status.dwServiceSpecificExitCode = 0;
   g_service_status.dwCheckPoint = 0;
   g_service_status.dwWaitHint = 0;
   if ( !SetServiceStatus( g_status_handle, &g_service_status ) )
   {
      PD_LOG ( PDERROR, "failed to set service(%s) status",
               g_service_name );
      pmdWinSvcReportStatusToSCMgr( SERVICE_STOPPED );
      goto error;
   }

   g_wait_event = CreateEvent( NULL, TRUE, FALSE, NULL );
   if ( NULL == g_wait_event )
   {
      PD_LOG ( PDERROR, "failed to create event" );
      pmdWinSvcReportStatusToSCMgr( SERVICE_STOPPED );
      goto error;
   }

   try
   {
      boost::thread serviceThrd( g_service_fun, argc, (CHAR **)argv );
      serviceThrd.detach();
   }
   catch ( boost::exception & )
   {
      PD_LOG ( PDERROR, "occured unexpected error" );
      pmdWinSvcReportStatusToSCMgr( SERVICE_STOPPED );
      goto error;
   }
   if ( !pmdWinSvcReportStatusToSCMgr( SERVICE_RUNNING ) )
   {
      PD_LOG ( PDERROR, "failed to report running status!" );
      pmdWinSvcReportStatusToSCMgr( SERVICE_STOPPED );
      goto error;
   }

   dwWait = WaitForSingleObject( g_wait_event, INFINITE );
   if ( g_wait_event != NULL )
   {
      CloseHandle( g_wait_event );
   }

   if ( g_status_handle != NULL )
   {
      pmdWinSvcReportStatusToSCMgr( SERVICE_STOPPED );
   }
done:
   PD_TRACE_EXIT ( SDB_PMDWINSVCMAIN );
   return ;
error:
   goto done;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDWINSVCREPSTATTOMGR, "pmdWinSvcReportStatusToSCMgr" )
BOOLEAN pmdWinSvcReportStatusToSCMgr( DWORD dwStatus )
{
   BOOLEAN result = TRUE;
   PD_TRACE_ENTRY ( SDB_PMDWINSVCREPSTATTOMGR );

   if ( dwStatus == SERVICE_START_PENDING )
   {
      g_service_status.dwControlsAccepted = 0;
   }
   else
   {
      g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP ;
   }

   g_service_status.dwCurrentState = dwStatus;

   if ( !( SetServiceStatus(g_status_handle, &g_service_status) ) )
   {
      result = FALSE;
      pmdWinStopService( L"SetServiceStatus" );
   }

   PD_TRACE_EXIT ( SDB_PMDWINSVCREPSTATTOMGR );
   return result;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_PMDWINSVCCTRLHANDL, "pmdWinServiceCtrlHandler" )
void WINAPI pmdWinServiceCtrlHandler( DWORD control )
{
   PD_TRACE_ENTRY ( SDB_PMDWINSVCCTRLHANDL );
   switch( control )
   {
   case SERVICE_CONTROL_SHUTDOWN:
   case SERVICE_CONTROL_STOP:
      {
         pmdWinSvcReportStatusToSCMgr( SERVICE_STOP_PENDING );
         SetEvent( g_wait_event );
         break;
      }
   default:
      break;
   }

   PD_TRACE_EXIT ( SDB_PMDWINSVCCTRLHANDL );
   return ;
}

#endif

