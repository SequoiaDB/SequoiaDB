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

   Source File Name = coordSqlOperator.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordSqlOperator.hpp"
#include "pmd.hpp"
#include "sqlCB.hpp"
#include "msgMessage.hpp"
#include "rtnCommandDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordSqlOperator implement
   */
   _coordSqlOperator::_coordSqlOperator()
   {
      const static string s_name( "Sql" ) ;
      setName( s_name ) ;

      _needRollback = FALSE ;
   }

   _coordSqlOperator::~_coordSqlOperator()
   {
   }

   BOOLEAN _coordSqlOperator::needRollback() const
   {
      return _needRollback ;
   }

   INT32 _coordSqlOperator::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )

   {
      INT32 rc          = SDB_OK ;
      SQL_CB *sqlcb     = pmdGetKRCB()->getSqlCB() ;
      CHAR *sql         = NULL ;

      contextID         = -1 ;

      rc = msgExtractSql( (CHAR*)pMsg, &sql ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract sql" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "%s", sql ) ;

      rc = sqlcb->exec( sql, cb, contextID, _needRollback ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}

