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

   Source File Name = mthSActionParser.cpp

   Descriptive Name = mth selector action parser

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSActionParser.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthIncludeParser.hpp"
#include "mthDefaultParser.hpp"
#include "mthSliceParser.hpp"
#include "mthElemMatchParser.hpp"
#include "mthElemMatchOneParser.hpp"
#include "mthMathParser.hpp"
#include "mthStrParser.hpp"
#include "mthCastParser.hpp"
#include "mthSActionFunc.hpp"


#define MTH_ADD_PARSER( parser )\
        do                                                                                    \
        {                                                                                     \
           p = NULL ;                                                                         \
           p = SDB_OSS_NEW parser ;                                                           \
           if ( NULL == p )                                                                   \
           {                                                                                  \
              PD_LOG( PDERROR, "failed to allocate mem." ) ;                                  \
              rc = SDB_OOM ;                                                                  \
              goto error ;                                                                    \
           }                                                                                  \
           if ( !_parsers.insert( std::make_pair( p->getActionName(), p ) ).second )          \
           {                                                                                  \
              PD_LOG( PDERROR, "duplicate action:%s", p->getActionName().c_str() ) ;          \
              rc = SDB_SYS ;                                                                  \
              goto error ;                                                                    \
           }                                                                                  \
           p = NULL ;                                                                         \
        } while( FALSE )

namespace engine
{
   static _mthSActionParser PARSER ;
   _mthSActionParser::_mthSActionParser()
   {
      _registerParsers() ;
   }

   _mthSActionParser::~_mthSActionParser()
   {
      PARSERS::iterator itr = _parsers.begin() ;
      for ( ; itr != _parsers.end(); ++itr )
      {
         SAFE_OSS_DELETE( itr->second ) ;
      }
      _parsers.clear() ;
   }

   const _mthSActionParser *_mthSActionParser::instance()
   {
      return &PARSER ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTIONPARSER_GETACTION, "_mthSActionParser::getAction" )
   INT32 _mthSActionParser::parse( const bson::BSONElement &e,
                                   _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTIONPARSER_GETACTION ) ;
      SDB_ASSERT( !e.eoo(), "can not be invalid" ) ;
      const CHAR *fieldName = e.fieldName() ;
      action.clear() ;

      if ( '$' != *fieldName )
      {
         goto done ;
      }

      {
      PARSERS::const_iterator itr = _parsers.find( fieldName ) ;
      if ( _parsers.end() == itr )
      {
         PD_LOG( PDERROR, "can not find the parser of action[%s]",
                 fieldName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = itr->second->parse( e, action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to parse action:%d", rc ) ;
         goto error ;
      }
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSACTIONPARSER_GETACTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTIONPARSER_BUILDDEFAULTVALUEACTION, "_mthSActionParser::buildDefaultValueAction" )
   INT32 _mthSActionParser::buildDefaultValueAction( const bson::BSONElement &e,
                                                     _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTIONPARSER_BUILDDEFAULTVALUEACTION ) ;
      PARSERS::const_iterator itr = _parsers.find( MTH_S_DEFAULT ) ;
      if ( _parsers.end() == itr )
      {
         PD_LOG( PDERROR, "can not find the parser of action[%s]",
                 MTH_S_DEFAULT ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = itr->second->parse( e, action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to parse action:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSACTIONPARSER_BUILDDEFAULTVALUEACTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTIONPARSER__BUILDSLICEACTION, "_mthSActionParser::buildSliceAction" )
   INT32 _mthSActionParser::buildSliceAction( INT32 begin,
                                              INT32 limit,
                                              _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTIONPARSER__BUILDSLICEACTION ) ;
      BSONObjBuilder builder ;
      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSliceBuild,
                      &mthSliceGet ) ;
      action.setName( MTH_S_SLICE ) ;
      builder.append( "arg1", begin ).append( "arg2", limit ) ;
      action.setArg( builder.obj() ) ;
      PD_TRACE_EXITRC( SDB__MTHSACTIONPARSER__BUILDSLICEACTION, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSACTIONPARSER__REGISTERPARSERS, "_mthSActionParser::_registerParsers" )
   INT32 _mthSActionParser::_registerParsers()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSACTIONPARSER__REGISTERPARSERS ) ;


      parser *p = NULL ;

      MTH_ADD_PARSER( _mthIncludeParser ) ;

      MTH_ADD_PARSER( _mthDefaultParser ) ;

      MTH_ADD_PARSER( _mthSliceParser ) ;

      MTH_ADD_PARSER( _mthElemMatchParser ) ;

      MTH_ADD_PARSER( _mthElemMatchOneParser ) ;

      MTH_ADD_PARSER( _mthAbsParser ) ;

      MTH_ADD_PARSER( _mthCeilingParser ) ;

      MTH_ADD_PARSER( _mthFloorParser ) ;

      MTH_ADD_PARSER( _mthModParser ) ;

      MTH_ADD_PARSER( _mthSubStrParser ) ;

      MTH_ADD_PARSER( _mthStrLenParser ) ;

      MTH_ADD_PARSER( _mthLowerParser ) ;

      MTH_ADD_PARSER( _mthUpperParser ) ;

      MTH_ADD_PARSER( _mthTrimParser ) ;

      MTH_ADD_PARSER( _mthLTrimParser ) ;

      MTH_ADD_PARSER( _mthRTrimParser ) ;

      MTH_ADD_PARSER( _mthCastParser ) ;

      MTH_ADD_PARSER( _mthAddParser ) ;

      MTH_ADD_PARSER( _mthSubtractParser ) ;

      MTH_ADD_PARSER( _mthMultiplyParser ) ;

      MTH_ADD_PARSER( _mthDivideParser ) ;

      MTH_ADD_PARSER( _mthTypeParser ) ;

      MTH_ADD_PARSER( _mthSizeParser ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSACTIONPARSER__REGISTERPARSERS, rc ) ;
      return rc ;
   error:
      SAFE_OSS_DELETE( p ) ;
      goto done ;
   }

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
      action.setArg( BSON( "arg1" << e.numberInt() ) ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHTYPEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSIZEPARSER_PARSE, "_mthSizeParser::parse" )
   INT32 _mthSizeParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY(SDB__MTHSIZEPARSER_PARSE ) ;

      if ( !mthIsNumber1( e ) )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "placeholder must be 1" ) ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthSizeBuild, &mthSizeGet ) ;
      action.setName( _name.c_str() ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSIZEPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

