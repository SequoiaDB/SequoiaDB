/******************************************************************************

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

   Source File Name = mthElemMatchParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthElemMatchParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

namespace engine
{
   _mthElemMatchParser::_mthElemMatchParser()
   {
      _name = MTH_S_ELEMMATCH ;
   }

   _mthElemMatchParser::~_mthElemMatchParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHELEMMATCHPARSER_PARSE, "_mthElemMatchParser::parse" )
   INT32 _mthElemMatchParser::parse( const bson::BSONElement &e,
                                     _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHELEMMATCHPARSER_PARSE ) ;
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

      rc = action.getMatchTree()->loadPattern( e.embeddedObject() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to load match pattern:%d", rc ) ;
         goto error ;
      }

      action.setName( _name.c_str() ) ;
      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthElemMatchBuild,
                      &mthElemMatchGet ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHELEMMATCHPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

