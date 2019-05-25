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

   Source File Name = rtnSQLBuildObj.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/16/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnSQLBuildObj.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "ossErr.h"

using namespace bson;

namespace engine
{
   rtnSQLBuildObj::rtnSQLBuildObj( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
      _hasData = FALSE;
   }

   rtnSQLBuildObj::~rtnSQLBuildObj()
   {
   }

   INT32 rtnSQLBuildObj::result( bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK;
      PD_CHECK( !_alias.empty(), SDB_INVALIDARG, error, PDERROR,
               "no aliases for function!" );
      try
      {
         builder.append( _alias.toString(), _obj ) ;
         _hasData = FALSE;
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

   INT32 rtnSQLBuildObj::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( param.size() == _param.size(), "invalid size!" );

      if ( _hasData )
      {
         goto done ;
      }

      try
      {
         BSONObjBuilder objBuilder;
         UINT32 i = 0 ;
         for ( i = 0; i < param.size(); i++ )
         {
            if ( !param[i].eoo() )
            {
               objBuilder.appendAs( param[i], _param[i].alias.toString() );
            }
         }
         _obj = objBuilder.obj() ;
         _hasData = TRUE ;
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