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

   Source File Name = pmdAgent.cpp

   Descriptive Name = Process MoDel Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdSession.hpp"
#include "pmdProcessor.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOCALAGENTENTPNT, "pmdLocalAgentEntryPoint" )
   INT32 pmdLocalAgentEntryPoint( pmdEDUCB *cb, void *arg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOCALAGENTENTPNT );

      SOCKET s = *(( SOCKET *) &arg ) ;
      pmdLocalSession localSession( s ) ;
      localSession.attach( cb ) ;

      if ( pmdGetDBRole() == SDB_ROLE_COORD )
      {
         pmdCoordProcessor coordProcessor ;
         localSession.attachProcessor( &coordProcessor ) ;
         rc = localSession.run() ;
         localSession.detachProcessor() ;
      }
      else
      {
         pmdDataProcessor dataProcessor ;
         localSession.attachProcessor( &dataProcessor ) ;
         rc = localSession.run() ;
         localSession.detachProcessor() ;
      }

      localSession.detach() ;

      pmdGetKRCB()->getMonDBCB ()->connDec();

      PD_TRACE_EXITRC ( SDB_PMDLOCALAGENTENTPNT, rc );
      return rc ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_AGENT, FALSE,
                          pmdLocalAgentEntryPoint,
                          "Agent" ) ;

}

