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

   Source File Name = qgmPIMthMatcherScan.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlMthMatcherScan.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson;

namespace engine
{
   qgmPlMthMatcherScan::qgmPlMthMatcherScan( const qgmDbAttr &collection,
                                             const qgmOPFieldVec &selector,
                                             const bson::BSONObj &orderby,
                                             const bson::BSONObj &hint,
                                             INT64 numSkip,
                                             INT64 numReturn,
                                             const qgmField &alias,
                                             const bson::BSONObj &matcher )
   : _qgmPlScan( collection, selector, orderby, hint, numSkip, numReturn,
               alias, NULL )
   {
      _condition = matcher.copy();
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLMTHMATCHERSCAN__EXEC, "qgmPlMthMatcherScan::_execute" )
   INT32 qgmPlMthMatcherScan::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLMTHMATCHERSCAN__EXEC ) ;
      INT32 rc = SDB_OK;
      SDB_ASSERT ( _input.size() == 0, "impossible" );

      _invalidPredicate = FALSE ;
      _contextID = -1 ;

      if ( SDB_ROLE_COORD == _dbRole )
      {
         rc = _executeOnCoord( eduCB ) ;
      }
      if ( SDB_COORD_UNKNOWN_OP_REQ == rc ||
           SDB_ROLE_COORD != _dbRole )
      {
         rc = _executeOnData( eduCB ) ;
      }

      if ( SDB_RTN_INVALID_PREDICATES == rc )
      {
         rc = SDB_OK ;
         _invalidPredicate = TRUE ;
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLMTHMATCHERSCAN__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
