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

   Source File Name = coordAuthDelOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordAuthDelOperator.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{
   /*
      coordAuthDelOperator implement
   */
   _coordAuthDelOperator::_coordAuthDelOperator()
   {
      const static string s_name( "AuthDelete" ) ;
      setName( s_name ) ;
   }

   _coordAuthDelOperator::~_coordAuthDelOperator()
   {
   }

   BOOLEAN _coordAuthDelOperator::isReadOnly() const
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHDELOPR_EXE, "_coordAuthDelOperator::execute" )
   INT32 _coordAuthDelOperator::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHDELOPR_EXE ) ;
      const CHAR *pUserName = NULL ;
      const CHAR *pPass = NULL ;

      rc = forward( pMsg, cb, FALSE, contextID, &pUserName, &pPass ) ;
      if ( pUserName )
      {
         /// AUDIT
         PD_AUDIT_OP( AUDIT_DCL, pMsg->opCode, AUDIT_OBJ_USER,
                      pUserName, rc, "" ) ;
      }
      if ( rc )
      {
         goto error ;
      }
      else if ( 0 == ossStrcmp( pUserName, cb->getUserName() ) )
      {
         cb->setUserInfo( "", "" ) ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_AUTHDELOPR_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

