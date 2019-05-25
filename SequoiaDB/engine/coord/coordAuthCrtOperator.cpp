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

   Source File Name = coordAuthCrtOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordAuthCrtOperator.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{
   /*
      _coordAuthCrtOperator implement
   */
   _coordAuthCrtOperator::_coordAuthCrtOperator()
   {
      const static string s_name( "AuthCreate" ) ;
      setName( s_name ) ;
   }

   _coordAuthCrtOperator::~_coordAuthCrtOperator()
   {
   }

   BOOLEAN _coordAuthCrtOperator::isReadOnly() const
   {
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHCRTOPR_EXE, "_coordAuthCrtOperator::execute" )
   INT32 _coordAuthCrtOperator::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHCRTOPR_EXE ) ;
      const CHAR *pUserName = NULL ;
      const CHAR *pPassWord = NULL ;

      rc = forward( pMsg, cb, FALSE, contextID, &pUserName, &pPassWord ) ;
      if ( pUserName )
      {
         PD_AUDIT_OP( AUDIT_DCL, pMsg->opCode, AUDIT_OBJ_USER,
                      pUserName, rc, "" ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      cb->setUserInfo( pUserName, pPassWord ) ;

   done:
      PD_TRACE_EXITRC ( COORD_AUTHCRTOPR_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
