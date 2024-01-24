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

   Source File Name = mthTypeParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/21/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthTypeParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

using namespace bson ;

namespace engine
{
   _mthTypeParser::_mthTypeParser()
   {
      _name = MTH_S_TYPE ;
   }

   _mthTypeParser::~_mthTypeParser() {}

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTYPEPARSER_PARSE, "_mthTypeParser::parse" )
   INT32 _mthTypeParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHTYPEPARSER_PARSE ) ;

      if ( !e.isNumber() || ( e.numberInt() != 1 && e.numberInt() != 2 ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "invalid element:e=%s",
                      e.toString().c_str() ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthTypeBuild, &mthTypeGet ) ;
      action.setName( _name.c_str() ) ;

      try
      {
         action.setArg( BSON( "arg1" << e.numberInt() ) ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "An exception occurred when set the arg of "
                 "$type: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

