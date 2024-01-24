/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = rtnSQLCount.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnSQLCount.hpp"

namespace engine
{
   _rtnSQLCount::_rtnSQLCount( const CHAR *pName )
   :_rtnSQLFunc( pName ),
    _count(0)
   {
   }

   _rtnSQLCount::~_rtnSQLCount()
   {

   }

   INT32 _rtnSQLCount::_push( const RTN_FUNC_PARAMS &param  )
   {
      INT32 rc = SDB_OK ;

      if ( !param.begin()->eoo() )
      {
         ++_count ;
      }
      return rc ;
   }

   INT32 _rtnSQLCount::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         builder.append( _alias.toString(), _count ) ;
         _count = 0 ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexcepted err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}
