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

   Source File Name = mthCastParser.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthCastParser.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthSActionFunc.hpp"
#include "utilString.hpp"
#include "mthCommon.hpp"

namespace engine
{
   _mthCastParser::_mthCastParser()
   {
      _name = MTH_S_CAST ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHCASTPARSER_PARSE, "_mthCastParser::parse" )
   INT32 _mthCastParser::parse( const bson::BSONElement &e,
                                _mthSAction &action ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHCASTPARSER_PARSE ) ;
      BSONType castType = EOO ;
#if defined (_DEBUG)
      if ( 0 != _name.compare( e.fieldName() ) )
      {
         PD_LOG( PDERROR, "field name[%s] is not valid",
                 e.fieldName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
#endif

      if ( String == e.type() )
      {
         rc = _getCastType( e.valuestr(), castType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid cast type:%s", e.valuestr () ) ;
            goto error ;
         } 
      }
      else if ( !e.isNumber() )
      {
         PD_LOG( PDERROR, "invalid cast type:%s",
                 e.toString( TRUE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         castType = ( BSONType )( e.numberInt() ) ;
      }

      switch (castType)
      {
      case MinKey :
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      case String :
      case Object :
         break ;
      case Array :
      case BinData :
      case Undefined :
         rc = SDB_INVALIDARG ;
         break ;
      case jstOID :
      case Bool :
      case Date :
      case jstNULL :
         break ;
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberInt :
      case Timestamp :
      case NumberLong :
      case NumberDecimal :
      case MaxKey :
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "unknown cast type:%d", castType ) ;
         goto error ;
      }

      action.setAttribute( MTH_S_ATTR_PROJECTION ) ;
      action.setFunc( &mthCastBuild,
                      &mthCastGet ) ;
      action.setName( _name.c_str() ) ;
      action.setArg( BSON( "arg1" << castType ) ) ;
   done:
      PD_TRACE_EXITRC( SDB__MTHCASTPARSER_PARSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastParser::_getCastType( const CHAR *str,
                                       BSONType &type ) const
   {
      INT32 rc = SDB_OK ;
      rc = mthGetCastTranslator()->getCastType( str, type ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "unknown type:%s, check your input or"
                 "use bsontype", str ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

