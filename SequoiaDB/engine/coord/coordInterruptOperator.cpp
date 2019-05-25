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

   Source File Name = coordInterruptOperator.cpp

   Descriptive Name = Coord Interrupt

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   interrupt processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordInterruptOperator.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordInterrupt implement
   */
   _coordInterrupt::_coordInterrupt()
   {
      const static string s_name( "Interrupt" ) ;
      setName( s_name ) ;
   }

   _coordInterrupt::~_coordInterrupt()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INTERRUPT_EXE, "_coordInterrupt::execute" )
   INT32 _coordInterrupt::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_INTERRUPT_EXE ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      pmdRemoteSessionSite *pSite = NULL ;
      INT64 tmpContextID = -1 ;

      contextID = -1 ;

      if ( cb->getRemoteSite() )
      {
         pSite = (pmdRemoteSessionSite*)cb->getRemoteSite() ;
         pSite->interruptAllSubSession() ;
      }

      cb->interrupt() ;

      while ( -1 != ( tmpContextID = cb->contextPeek() ) )
      {
         pRtncb->contextDelete( tmpContextID, NULL ) ;
      }

      PD_TRACE_EXITRC ( COORD_INTERRUPT_EXE, rc ) ;
      return rc ;
   }

}

