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

   Source File Name = mthMathParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthMathParser.hpp"
#include "mthSActionFunc.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"

namespace engine
{
   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHABSPARSER_PARSE, "_mthAbsParser::parse" )
   INT32 _mthAbsParser::parse( const bson::BSONElement &e,
                               _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHABSPARSER_PARSE ) ;

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
      action.setFunc( &mthAbsBuild,
                      &mthAbsGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHABSPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCEILINGPARSER_PARSE, "_mthCeilingParser::parse" )
   INT32 _mthCeilingParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCEILINGPARSER_PARSE ) ;

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
      action.setFunc( &mthCeilingBuild,
                      &mthCeilingGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHCEILINGPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHFLOORPARSER_PARSE, "_mthFloorParser::parse" )
   INT32 _mthFloorParser::parse( const bson::BSONElement &e,
                                 _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHFLOORPARSER_PARSE ) ;

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
      action.setFunc( &mthFloorBuild,
                      &mthFloorGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHFLOORPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMODPARSER_PARSE, "_mthModParser::parse" )
   INT32 _mthModParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMODPARSER_PARSE ) ;
      BSONObjBuilder builder ;

      if ( !e.isNumber() ||
           0 == e.Number() )
      {
         PD_LOG( PDERROR, "invalid element:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthModBuild,
                      &mthModGet ) ;
      action.setName( _name.c_str() ) ;
      builder.appendAs( e, "arg1" ) ;
      action.setArg( builder.obj() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHMODPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHADDPARSER_PARSE, "_mthAddParser::parse" )
   INT32 _mthAddParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHADDPARSER_PARSE ) ;
      BSONObjBuilder builder ;

      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "invalid element:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthAddBuild,
                      &mthAddGet ) ;
      action.setName( _name.c_str() ) ;
      builder.appendAs( e, "arg1" ) ;
      action.setArg( builder.obj() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHADDPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSUBTRACTPARSER_PARSE, "_mthSubtractParser::parse" )
   INT32 _mthSubtractParser::parse( const bson::BSONElement &e,
                                    _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTPARSER_PARSE ) ;
      BSONObjBuilder builder ;

      if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "invalid element:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSubtractBuild,
                      &mthSubtractGet ) ;
      action.setName( _name.c_str() ) ;
      builder.appendAs( e, "arg1" ) ;
      action.setArg( builder.obj() ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

    ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMULTIPLYPARSER_PARSE, "_mthMultiplyParser::parse" )
    INT32 _mthMultiplyParser::parse( const bson::BSONElement &e,
                                     _mthSAction &action ) const
    {
       INT32 rc = SDB_OK ;
       PD_TRACE_ENTRY( SDB__MTHSUBTRACTPARSER_PARSE ) ;
       BSONObjBuilder builder ;

       if ( !e.isNumber() )
       {
          PD_LOG( PDERROR, "invalid element:%s",
                  e.toString( TRUE, TRUE ).c_str() ) ;
          rc = SDB_INVALIDARG ;
          goto error ;
       }

       action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
       action.setFunc( &mthMultiplyBuild,
                       &mthMultiplyGet ) ;
       action.setName( _name.c_str() ) ;
       builder.appendAs( e, "arg1" ) ;
       action.setArg( builder.obj() ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDIVIDEPARSER_PARSE, "_mthDivideParser::parse" )
   INT32 _mthDivideParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSUBTRACTPARSER_PARSE ) ;
      BSONObjBuilder builder ;

      if ( !e.isNumber() || mthIsZero(e) )
      {
         PD_LOG( PDERROR, "invalid element:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthDivideBuild,
                      &mthDivideGet ) ;
      action.setName( _name.c_str() ) ;
      builder.appendAs( e, "arg1" ) ;
      action.setArg( builder.obj() ) ;

   done:
      PD_TRACE_EXITRC( SDB__MTHSUBTRACTPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

