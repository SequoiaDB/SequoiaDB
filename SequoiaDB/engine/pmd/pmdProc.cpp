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

   Source File Name = pmdProc.cpp

   Descriptive Name = pmdProc

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdProc.hpp"
#include "ossEDU.hpp"
#include "pd.hpp"

#if defined (_LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#endif

namespace engine
{
   /*
      _iPmdProc implement
   */

   BOOLEAN _iPmdProc::_isRunning = FALSE;

   _iPmdProc::_iPmdProc()
   {
      _isRunning = TRUE ;
   }

   _iPmdProc::~_iPmdProc()
   {
   }

   void _iPmdProc::stop( INT32 sigNum )
   {
      if ( 0 != sigNum )
      {
         PD_LOG( PDEVENT, "Recieved signal[%d], stop...", sigNum ) ;
      }
      _isRunning = FALSE ;
   }

   INT32 _iPmdProc::regSignalHandler()
   {
      INT32 rc = SDB_OK;
#if defined (_LINUX)
      ossSigSet sigSet ;
      sigSet.sigAdd( SIGHUP ) ;
      sigSet.sigAdd( SIGINT ) ;
      sigSet.sigAdd( SIGTERM ) ;
      sigSet.sigAdd( SIGPWR ) ;
      rc = ossRegisterSignalHandle( sigSet,
               (SIG_HANDLE)(&iPmdProc::stop) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register signals, rc = %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
#else
      return rc ;
#endif
   }

   BOOLEAN _iPmdProc::isRunning()
   {
      return _isRunning ;
   }

}
