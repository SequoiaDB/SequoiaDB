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

   Source File Name = rtnSQLMergeArraySet.cpp

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
#include "rtnSQLMergeArraySet.hpp"
#include "ossMem.hpp"
#include "ossTypes.h"
#include "ossErr.h"

using namespace bson;

namespace engine
{
   rtnSQLMergeArraySet::rtnSQLMergeArraySet( const CHAR *pName )
   :_rtnSQLFunc( pName )
   {
      _pArrBuilder = SDB_OSS_NEW BSONArrayBuilder();
   }

   rtnSQLMergeArraySet::~rtnSQLMergeArraySet()
   {
      if ( _pArrBuilder )
      {
         SDB_OSS_DEL _pArrBuilder;
      }
   }

   INT32 rtnSQLMergeArraySet::result( BSONObjBuilder &builder )
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

   INT32 rtnSQLMergeArraySet::_push( const RTN_FUNC_PARAMS &param )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( _pArrBuilder != NULL, "_pArrBuilder can't be NULL!" );
      try
      {
         const BSONElement &ele = *(param.begin());
         if ( ele.type() == Array )
         {
            BSONObj obj = ele.embeddedObject().copy();
            if ( obj.isEmpty() )
            {
               goto done;
            }
            _objVec.push_back( obj );
            BSONObjIterator iter( obj );
            while( iter.more())
            {
               BSONElement subEle = iter.next();
               if ( !subEle.isNull()
                  && _fieldSet.find( subEle ) == _fieldSet.end() )
               {
                  _pArrBuilder->append( subEle );
                  _fieldSet.insert( subEle );
               }
            }
         }
         else
         {
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


