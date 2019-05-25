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

   Source File Name = mthDefaultParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthDefaultParser.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "mthSActionFunc.hpp"

namespace engine
{
   _mthDefaultParser::_mthDefaultParser()
   {
      _name = MTH_S_DEFAULT ;
   }

   _mthDefaultParser::~_mthDefaultParser()
   {

   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDEFAULTPARSER_PARSE, "_mthDefaultParser::parse" )
   INT32 _mthDefaultParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHDEFAULTPARSER_PARSE ) ;
      SDB_ASSERT( !e.eoo(), "can not be eoo" ) ;

      action.setAttribute( MTH_S_ATTR_DEFAULT ) ;
      action.setFunc( &mthDefaultBuild,
                      &mthDefaultGet ) ;
      action.setName( _name.c_str() ) ;
      action.setValue( e ) ;
      PD_TRACE_EXITRC( SDB__MTHDEFAULTPARSER_PARSE, rc ) ;
      return rc ;
   }
}

