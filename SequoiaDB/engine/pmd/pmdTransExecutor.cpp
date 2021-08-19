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

   Source File Name = pmdTransExecutor.cpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdTransExecutor.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   /*
      _pmdTransExecutor implement
   */
   _pmdTransExecutor::_pmdTransExecutor( _pmdEDUCB *cb, monMonitorManager *monMgr )
      : _dpsTransExecutor( monMgr )
   {
      SDB_ASSERT( cb, "CB can't be NULL" ) ;
      _pEDUCB = cb ;
   }

   _pmdTransExecutor::~_pmdTransExecutor()
   {
      _pEDUCB = NULL ;
   }

   EDUID _pmdTransExecutor::getEDUID() const
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getID() ;
      }
      return PMD_INVALID_EDUID ;
   }

   UINT32 _pmdTransExecutor::getTID() const
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getTID() ;
      }
      return PMD_INVALID_EDUID ;
   }

   void _pmdTransExecutor::wakeup()
   {
      SDB_ASSERT( _pEDUCB, "EDU CB can't be NULL" ) ;
      if ( _pEDUCB )
      {
         _pEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_LOCKWAKEUP ) ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDTRANSEXECUTOR_WAIT, "_pmdTransExecutor::wait" )
   INT32 _pmdTransExecutor::wait( INT64 timeout )
   {
      INT32 rc = SDB_SYS ;
      PD_TRACE_ENTRY ( SDB__PMDTRANSEXECUTOR_WAIT );
      pmdEDUEvent data ;

      if ( _pEDUCB )
      {
         if ( _pEDUCB->waitEvent( PMD_EDU_EVENT_LOCKWAKEUP, data, timeout ) )
         {
            rc = SDB_OK ;
         }
         else if ( _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
         }
         else
         {
            rc = SDB_TIMEOUT ;
         }
      }

      PD_TRACE_EXITRC ( SDB__PMDTRANSEXECUTOR_WAIT, rc );
      return rc ;
   }

   IExecutor* _pmdTransExecutor::getExecutor()
   {
      return _pEDUCB ;
   }

   BOOLEAN _pmdTransExecutor::isInterrupted ()
   {
      if ( NULL != _pEDUCB && _pEDUCB->isInterrupted() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

}

