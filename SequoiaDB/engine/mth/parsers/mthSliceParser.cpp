/******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = mthSliceParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSliceParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

using namespace bson ;

namespace engine
{
   _mthSliceParser::_mthSliceParser()
   {
      _name = MTH_S_SLICE ;
   }

   _mthSliceParser::~_mthSliceParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSLICEPARSER_PARSE, "_mthSliceParser::parse" )
   INT32 _mthSliceParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSLICEPARSER_PARSE ) ;
      BSONObjBuilder builder ;
      INT32 begin = 0 ;
      INT32 limit = -1 ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif

      if ( e.isNumber() )
      {
         begin = e.numberInt() ;
         INT32 number = e.numberInt() ;
         if ( 0 <= number )
         {
            begin = 0 ;
            limit = number ;
         }
         else
         {
            begin = number ;
         }
      }
      else if ( Array == e.type() )
      {
         UINT32 arraySize = 0 ;
         BSONObjIterator i( e.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            ++arraySize ;
            if ( !ele.isNumber() )
            {
               PD_LOG( PDERROR, "argument of $slice must be number" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( 1 == arraySize )
            {
               begin = ele.numberInt() ;
            }
            else if ( 2 == arraySize )
            {
               limit = ele.numberInt() ;
               if ( !mthIsValidLen( limit ) )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "limit is invalid:len=%d", limit ) ;
                  goto error ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "invalid array size of $slice" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         if ( 2 != arraySize )
         {
            PD_LOG( PDERROR, "invalid array size of $slice" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "invalid element type of $slice:%s",
                 e.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSliceBuild,
                      &mthSliceGet ) ;
      action.setName( _name.c_str() ) ;
      builder.append( "arg1", begin ).append( "arg2", limit ) ;
      action.setArg( builder.obj() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSLICEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

