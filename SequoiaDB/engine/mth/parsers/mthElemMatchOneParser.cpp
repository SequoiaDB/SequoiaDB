/******************************************************************************

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

   Source File Name = mthElemMatchOneParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthElemMatchOneParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

namespace engine
{
   _mthElemMatchOneParser::_mthElemMatchOneParser()
   {
      _name = MTH_S_ELEMMATCHONE ;
   }

   _mthElemMatchOneParser::~_mthElemMatchOneParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHONEPARSER_PARSE, "_mthElemMatchOneParser::parse" )
   INT32 _mthElemMatchOneParser::parse( const bson::BSONElement &e,
                                        _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      BOOLEAN subFieldIsOp = FALSE ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHONEPARSER_PARSE ) ;

      if ( Object != e.type() )
      {
         PD_LOG( PDERROR, "$elemMatch(One) requires object value" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL == action.getMatchTree() )
      {
         rc = action.createMatchTree() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create match tree, "
                      "rc: %d", rc ) ;
      }

      rc = mthCheckIfSubFieldIsOp( e, subFieldIsOp ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to check if the subfield name is an "
                 "operator name, rc: %d",
                 rc ) ;
         goto error ;
      }

      if ( subFieldIsOp )
      {
         action.setFunc( &mthElemMatchOneBuildSubIsOp,
                         &mthElemMatchOneGetSubIsOp ) ;
         // sub field is op, add an blank field name
         // eg: { $gte: 80 } ==> { "": { $gte: 80 } }
         rc = action.getMatchTree()->loadPattern( e.wrap( "" ) ) ;
      }
      else
      {
         action.setFunc( &mthElemMatchOneBuild,
                         &mthElemMatchOneGet ) ;
         // already have a normal field name
         rc = action.getMatchTree()->loadPattern( e.embeddedObject() ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to load match pattern, rc: %d", rc ) ;
         goto error ;
      }

      action.setName( _name.c_str() ) ;
      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHONEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

