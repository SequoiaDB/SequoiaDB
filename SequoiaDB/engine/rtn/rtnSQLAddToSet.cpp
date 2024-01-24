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

   Source File Name = rtnSQLAddToSet.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/05/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnSQLAddToSet.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "ossErr.h"

using namespace bson;

namespace engine
{
   rtnSQLAddToSet::rtnSQLAddToSet( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
      _pArrBuilder = SDB_OSS_NEW BSONArrayBuilder();
   }

   rtnSQLAddToSet::~rtnSQLAddToSet()
   {
      if ( _pArrBuilder )
      {
         SDB_OSS_DEL _pArrBuilder;
      }
   }

   INT32 rtnSQLAddToSet::result( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( !_alias.empty(), SDB_INVALIDARG, error, PDERROR,
               "no aliases for function!" );
      try
      {
         builder.appendArray( _alias.toString(), _pArrBuilder->arr() );
         SDB_OSS_DEL _pArrBuilder;
         _fieldSet.clear();
         _objVec.clear();
         _pArrBuilder = SDB_OSS_NEW BSONArrayBuilder();
         PD_CHECK( _pArrBuilder != NULL, SDB_OOM, error, PDERROR,
                  "malloc failed" );
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "received unexpected error:%s", e.what() );
         rc = SDB_SYS;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rtnSQLAddToSet::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( _pArrBuilder != NULL, "_pArrBuilder can't be NULL!" );
      try
      {
         const BSONElement &ele = *(param.begin());
         if ( !ele.eoo() && !ele.isNull()
               && _fieldSet.find( ele ) == _fieldSet.end() )
         {
            _pArrBuilder->append( ele );
            BSONObjBuilder builder;
            builder.append( ele );
            BSONObj obj = builder.obj();
            _fieldSet.insert( obj.firstElement() );
            _objVec.push_back( obj );
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "received unexpected error:%s", e.what() );
         rc = SDB_SYS;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}

