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

   Source File Name = coordAuthOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordAuthOperator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{
   /*
      _coordAuthOperator implement
   */
   _coordAuthOperator::_coordAuthOperator()
   {
      const static string s_name( "Auth" ) ;
      setName( s_name ) ;
   }

   _coordAuthOperator::~_coordAuthOperator()
   {
   }

   BOOLEAN _coordAuthOperator::isReadOnly() const
   {
      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_AUTHOPR_EXEC, "_coordAuthOperator::execute" )
   INT32 _coordAuthOperator::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_AUTHOPR_EXEC ) ;

      rc = forward( pMsg, cb, TRUE, contextID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_AUTHOPR_EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

