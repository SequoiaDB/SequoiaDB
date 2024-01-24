/*******************************************************************************


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

   Source File Name = qgmSelector.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmSelector.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

namespace engine
{
   _qgmSelector::_qgmSelector()
   :_needSelect( FALSE )
   {

   }

   _qgmSelector::~_qgmSelector()
   {
      _selector.clear() ;
   }

   string _qgmSelector::toString() const
   {
      stringstream ss ;
      if ( !_selector.empty() )
      {
         ss << "[" ;
         qgmOPFieldVec::const_iterator itr = _selector.begin() ;
         for ( ; itr != _selector.end(); itr++ )
         {
            if ( SQL_GRAMMAR::WILDCARD == itr->type )
            {
               ss << "{value:*}," ;
            }
            else
            {
               ss << "{value:" << itr->value.toString()
                  << ",alias:" << itr->alias.toString() ;
               if ( !itr->expr.isEmpty() )
               {
                  ss << ",expr:" << itr->expr.toString() ;
               }
               ss  << "}," ;
            }
         }
         ss.seekp((INT32)ss.tellp()-1 ) ;
         ss << "]" ;
      }
      else
      {
         ss << "[*]" ;
      }
      return ss.str() ;
   }

   INT32 _qgmSelector::load( const qgmOPFieldVec &op )
   {
      INT32 rc = SDB_OK ;
      qgmOPFieldVec::const_iterator itr = op.begin() ;
      for ( ; itr != op.end(); itr++ )
      {
         _selector.push_back( *itr ) ;
         if ( !( itr->alias.empty() ) ||
              !( itr->expr.isEmpty() ) )
         {
            _needSelect = TRUE ;
         }
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOR_SELECT, "_qgmSelector::select" )
   INT32 _qgmSelector::select( const BSONObj &src, BSONObj &out ) const
   {
      PD_TRACE_ENTRY( SDB__QGMSELECTOR_SELECT ) ;
      INT32 rc = SDB_OK ;
      if ( _selector.empty() )
      {
         out = src ;
         goto done ;
      }
      try
      {
         BSONObjBuilder builder ;
         qgmOPFieldVec::const_iterator itr = _selector.begin() ;
         for ( ; itr != _selector.end(); itr++ )
         {
            if ( SQL_GRAMMAR::WILDCARD == itr->type )
            {
               out = src ;
               goto done ;
            }
            {
            const ossPoolString &fieldName = itr->value.attr().toFieldName() ;
            BSONElement ele = src.getFieldDotted( fieldName.c_str() ) ;
            if ( ele.eoo() )
            {
               if ( itr->alias.empty() )
               {
                  builder.appendNull( fieldName ) ;
               }
               else
               {
                  builder.appendNull( itr->alias.toString() ) ;
               }
            }
            else if ( itr->expr.isEmpty() )
            {
               if ( itr->alias.empty() )
               {
                  builder.append( ele ) ;
               }
               else
               {
                  builder.appendAs( ele, itr->alias.toString() ) ;
               }
            }
            else
            {
               rc = _createValueWithExpr( ele,
                                          itr->alias.empty() ?
                                          ele.fieldName() :
                                          itr->alias.toString().c_str(),
                                          itr->expr, builder ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to create value from expr:%d", rc ) ;
                  goto error ;
               }
            }
            }
         }

         out = builder.obj() ;
      }
      catch (std::exception &e)
      {
         PD_LOG( PDERROR, "unexcepted err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOR_SELECT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOR_SELECT2, "_qgmSelector::select" )
   INT32 _qgmSelector::select( const qgmFetchOut &src,
                               BSONObj &out ) const
   {
      PD_TRACE_ENTRY( SDB__QGMSELECTOR_SELECT2 ) ;
      INT32 rc = SDB_OK ;
      if ( _selector.empty() )
      {
         out = src.mergedObj() ;
         goto done ;
      }
      else
      {
         BSONObjBuilder builder ;
         BSONElement ele ;
         try
         {
            qgmOPFieldVec::const_iterator itr = _selector.begin() ;
            for ( ; itr != _selector.end(); itr++ )
            {
               if ( SQL_GRAMMAR::WILDCARD == itr->type )
               {
                  out = src.mergedObj() ;
                  goto done ;
               }

               rc = src.element( itr->value, ele ) ;
               if ( rc )
               {
                  goto error ;
               }
               else if ( ele.eoo() )
               {
                  if ( itr->alias.empty() )
                  {
                     builder.appendNull( itr->value.attr().toFieldName() ) ;
                  }
                  else
                  {
                     builder.appendNull( itr->alias.toString() ) ;
                  }
               }
               else if ( itr->expr.isEmpty() )
               {
                  if ( itr->alias.empty() )
                  {
                     builder.append( ele ) ;
                  }
                  else
                  {
                     builder.appendAs( ele, itr->alias.toString() ) ;
                  }
               }
               else
               {
                  rc = _createValueWithExpr( ele,
                                             itr->alias.empty() ?
                                             ele.fieldName() :
                                             itr->alias.toString().c_str(),
                                             itr->expr, builder ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to create value from expr:%d", rc ) ;
                     goto error ;
                  }
               }
            }

            out = builder.obj() ;
         }
         catch ( std::exception & e )
         {
            PD_LOG( PDERROR, "unexcepted err happened:%s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOR_SELECT2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj _qgmSelector::selector() const
   {
      BSONObjBuilder builder ;

      qgmOPFieldVec::const_iterator itr = _selector.begin() ;
      for ( ; itr != _selector.end(); itr++ )
      {
         if ( !itr->value.attr().empty() )
         {
            ossPoolString name = itr->value.attr().toString() ;

            /// do not pass name like 'a.$[1]' to query
            const CHAR *dollarArray = ossStrstr( name.c_str(), ".$[" ) ;
            if ( NULL == dollarArray )
            {
               builder.appendNull( name ) ;
            }
            else
            {
               INT32 at = INT32(dollarArray - name.c_str()) ;
               name.at( at ) = '\0';
               builder.appendNull( StringData( name.c_str(), UINT32(at) ) ) ;
            }
         }
      }

      return builder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOR__CREATEVALUEWITHEXPR, "_qgmSelector::_createValueWithExpr" )
   INT32 _qgmSelector::_createValueWithExpr( const BSONElement &e,
                                             const CHAR *fieldName,
                                             const _qgmSelectorExpr &expr,
                                             BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMSELECTOR__CREATEVALUEWITHEXPR ) ;
      CHAR row[32] ;
      _qgmValueTuple v( row, 32, TRUE ) ;
      INT16 vType = 0 ;

      if ( !e.isNumber() )
      {
         builder.appendNull( fieldName ) ;
         goto done ;
      }

      rc = expr.getValue( e, v ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get value from expr:%d", rc ) ;
         goto error ;
      }

      vType = v.getValueType() ;
      if ( ( INT16 )bson::EOO == vType )
      {
         builder.appendNull( fieldName ) ;
      }
      else if ( ( INT16 )bson::NumberDecimal == vType )
      {
         bson::bsonDecimal decimal ;

         rc = decimal.fromBsonValue( (const CHAR *)v.getValue() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get decimal from bsonvalue:%d", rc ) ;
            goto error ;
         }

         builder.append( fieldName, decimal ) ;
      }
      else if ( ( INT16 )bson::NumberLong == vType )
      {
         builder.appendIntOrLL( fieldName, *((INT64 *)(v.getValue() )) ) ;
      }
      else
      {
         builder.appendNumber( fieldName, *((FLOAT64 *)(v.getValue() ) ) ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOR__CREATEVALUEWITHEXPR, rc ) ;
      return rc ;
   error:
      goto done ;
   } 
}
