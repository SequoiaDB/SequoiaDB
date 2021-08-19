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

   Source File Name = mthStrParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthStrParser.hpp"
#include "mthSActionFunc.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"

namespace engine
{
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBSTRPARSER_PARSE, "_mthSubStrParser::parse" )
   INT32 _mthSubStrParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBSTRPARSER_PARSE ) ;
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

      if ( e.isNumber() && 0 <=  e.numberInt())
      {
         limit = e.numberInt() ;
      }
      else if ( e.isNumber() )
      {
         begin = e.numberInt() ;
      }
      else if ( Array == e.type() )
      {
         BSONObjIterator i( e.embeddedObject() ) ;
         BSONElement ele ;
         if ( !i.more() )
         {
            goto invalid_arg ;
         }

         ele = i.next() ;
         if ( !ele.isNumber() )
         {
            goto invalid_arg ;
         }

         begin = ele.numberInt() ;

         if ( !i.more() )
         {
            goto invalid_arg ;
         }

         ele = i.next() ;
         if ( !ele.isNumber() )
         {
            goto invalid_arg ;
         }

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
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSubStrBuild,
                      &mthSubStrGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << begin << "arg2" << limit ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBSTRPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   invalid_arg:
      PD_LOG( PDERROR, "invalid substr argument:%s",
                    e.toString( TRUE, TRUE ).c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSTRLENPARSER_PARSE, "_mthStrLenParser::parse" )
   INT32 _mthStrLenParser::parse( const bson::BSONElement &e,
                                  _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSTRLENPARSER_PARSE ) ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthStrLenBuild,
                      &mthStrLenGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSTRLENPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLOWERPARSER_PARSE, "_mthLowerParser::parse" )
   INT32 _mthLowerParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHLOWERPARSER_PARSE ) ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLowerBuild,
                      &mthLowerGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLOWERPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHUPPERPARSER_PARSE, "_mthUpperParser::parse" )
   INT32 _mthUpperParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHUPPERPARSER_PARSE ) ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthUpperBuild,
                      &mthUpperGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHUPPERPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHTRIMPARSER_PARSE, "_mthTrimParser::parse" )
   INT32 _mthTrimParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHTRIMPARSER_PARSE ) ;

#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthTrimBuild,
                      &mthTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHLTRIMPARSER_PARSE, "_mthLTrimParser::parse" )
   INT32 _mthLTrimParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHLTRIMPARSER_PARSE ) ;

#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthLTrimBuild,
                      &mthLTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHLTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTRIMPARSER_PARSE, "_mthRTrimParser::parse" )
   INT32 _mthRTrimParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHRTRIMPARSER_PARSE ) ;

#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif
      if ( e.eoo() )
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthRTrimBuild,
                      &mthRTrimGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHRTRIMPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

