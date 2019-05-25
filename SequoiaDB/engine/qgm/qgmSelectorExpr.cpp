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

   Source File Name = qgmSelectorExpr.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmSelectorExpr.hpp"
#include "qgmTrace.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{
   _qgmSelectorExpr::_qgmSelectorExpr()
   {

   }

   _qgmSelectorExpr::~_qgmSelectorExpr()
   {

   }

   std::string _qgmSelectorExpr::toString() const
   {
      std::stringstream ss ;
      if ( NULL != _exprRoot )
      {
         _exprRoot->toString( ss ) ;
      }
      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOREXPR_GETVALUE, "_qgmSelectorExpr::getValue" )
   INT32 _qgmSelectorExpr::getValue( const bson::BSONElement &e,
                                     _qgmValueTuple &v ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMSELECTOREXPR_GETVALUE ) ;
      if ( NULL == _exprRoot )
      {
         PD_LOG( PDERROR, "expr root is null" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _exprRoot->getValue( e, &v ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get value from expr:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOREXPR_GETVALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

