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

   Source File Name = rtnSQLSum.cpp

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

#include "rtnSQLSum.hpp"

using namespace bson ;

namespace engine
{
   _rtnSQLSum::_rtnSQLSum( const CHAR *pName )
   :_rtnSQLFunc( pName ),
    _sum(0),
    _decSum(),
    _effective( FALSE )
   {
   }

   _rtnSQLSum::~_rtnSQLSum()
   {

   }

   INT32 _rtnSQLSum::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( !_effective )
         {
            builder.appendNull( _alias.toString() ) ;
         }
         else
         {
            if ( _decSum.isZero() )
            {
               builder.append( _alias.toString(), _sum ) ;
            }
            else
            {
               bsonDecimal tmpDecimal ;

               rc = tmpDecimal.fromDouble( _sum ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "from double failed:double=%f,rc=%d", 
                          _sum, rc ) ;
                  goto error ;
               }

               rc = _decSum.add( tmpDecimal ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "decimal add failed:rc=%d", rc ) ;
                  goto error ;
               }

               builder.append( _alias.toString(), _decSum ) ;
            }

            _effective = FALSE ;
            _sum = 0 ;
            _decSum.setZero() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexcepted err happened%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSQLSum::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK ;
      const BSONElement &ele = *(param.begin()) ;
      if ( ele.isNumber() )
      {
         if ( NumberDecimal == ele.type() )
         {
            _decSum.add( ele.numberDecimal() ) ;
         }
         else
         {
            _sum += ele.Number() ;
         }

         _effective = TRUE ;
      }
      
      return rc ;
   }
}
