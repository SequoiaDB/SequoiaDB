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

   Source File Name = qgmSelectorExprNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#include "qgmSelectorExprNode.hpp"
#include "sqlGrammar.hpp"
#include "pd.hpp"
#include "qgmTrace.hpp"
#include "pdTrace.hpp"
#include "qgmDef.hpp"

namespace engine
{
   _qgmSelectorExprNode::_qgmSelectorExprNode()
   :_type( SQL_GRAMMAR::SQLMAX ),
    _isDouble( FALSE ),
    _left( NULL ),
    _right( NULL )
   {
      *( ( INT64 * )_data ) = 0 ;
   }

   _qgmSelectorExprNode::~_qgmSelectorExprNode()
   {
      SAFE_OSS_DELETE( _left ) ;
      SAFE_OSS_DELETE( _right ) ;
   }

   void _qgmSelectorExprNode::toString( std::stringstream &ss )const
   {
      if ( SQL_GRAMMAR::DBATTR == _type )
      {
         ss << "x" ;
      }
      else if ( SQL_GRAMMAR::DIGITAL == _type )
      {
         if ( !_isDouble )
         {
            ss << *(( INT64 *)_data) ;
         }
         else
         {
            ss << *(( FLOAT64*)_data ) ;
         }
      }
      else if ( SQL_GRAMMAR::ADD == _type )
      {
         ss << "( " ;
         _left->toString( ss ) ;
         ss << " + " ;
         _right->toString( ss ) ;
         ss << " )" ;
      }
      else if ( SQL_GRAMMAR::SUB == _type )
      {
         ss << "( " ;
         _left->toString( ss ) ;
         ss << " - " ;
         _right->toString( ss ) ;
         ss << " )" ;
      }
      else if ( SQL_GRAMMAR::MULTIPLY == _type )
      {
         ss << "( " ;
         _left->toString( ss ) ;
         ss << " * " ;
         _right->toString( ss ) ;
         ss << " )" ;
      }
      else if ( SQL_GRAMMAR::DIVIDE == _type )
      {
         ss << "( " ;
         _left->toString( ss ) ;
         ss << " / " ;
         _right->toString( ss ) ;
         ss << " )" ;
      }
      else if ( SQL_GRAMMAR::MOD == _type )
      {
         ss << "( " ;
         _left->toString( ss ) ;
         ss << " % " ;
         _right->toString( ss ) ;
         ss << " )" ;
      }
      else
      {
         ss << "error" ;
      }
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOREXPRNODE_GETVALUE, "_qgmSelectorExprNode::getValue" )
   INT32 _qgmSelectorExprNode::getValue( const bson::BSONElement &e,
                                         _qgmValueTuple *v ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMSELECTOREXPRNODE_GETVALUE ) ;
      SDB_ASSERT( NULL != v, "can not be null" ) ;
      SDB_ASSERT( e.isNumber(), "must be number" ) ;

      if ( SQL_GRAMMAR::DIGITAL == _type )
      {
         if ( !_isDouble )
         {
            rc = v->setValue( sizeof( INT64 ), _data, bson::NumberLong ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            v->setValue( sizeof( FLOAT64 ), _data, bson::NumberDouble ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
      }
      else if ( SQL_GRAMMAR::DBATTR == _type )
      {
         if ( NumberDecimal == e.type() )
         {
            rc = v->setValue( e.numberDecimal() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
         else if ( NumberDouble == e.type() )
         {
            FLOAT64 f = e.numberDouble() ;
            rc = v->setValue( sizeof( FLOAT64 ), &f, bson::NumberDouble ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            INT64 l = e.numberLong() ;
            rc = v->setValue( sizeof( INT64 ), &l, bson::NumberLong ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }

         }
      }
      else
      {
         CHAR ld[32] ;
         CHAR rd[32] ;
         _qgmValueTuple lv( ld, 32, TRUE ) ;
         _qgmValueTuple rv( rd, 32, TRUE ) ;

         if ( NULL == _left || NULL == _right )
         {
            PD_LOG( PDERROR, "children should not be null" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         rc = _left->getValue( e, &lv ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get value from left child:%d", rc ) ;
            goto error ;
         }

         rc = _right->getValue( e, &rv ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get value from right child:%d", rc ) ;
            goto error ;
         }

         rc = _calcValue( lv, rv, _type, *v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to calculate value:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOREXPRNODE_GETVALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMSELECTOREXPRNODE__CALCVALUE "_qgmSelectorExprNode::_calcValue" )
   INT32 _qgmSelectorExprNode::_calcValue( const _qgmValueTuple &lv,
                                           const _qgmValueTuple &rv,
                                           INT32 type,
                                           _qgmValueTuple &v ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMSELECTOREXPRNODE__CALCVALUE ) ;

      if ( SQL_GRAMMAR::ADD == type )
      {
         rc = lv.add( rv, v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add qgmValueTuple:%d", rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::SUB == type )
      {
         rc = lv.sub( rv, v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub qgmValueTuple:%d", rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::MULTIPLY == type )
      {
         rc = lv.multiply( rv, v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to multiply qgmValueTuple:%d", rc ) ;
            goto error ;
         }
      }
      else if ( SQL_GRAMMAR::DIVIDE == type )
      {
         rc = lv.divide( rv, v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to divide qgmValueTuple:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         //MOD
         rc = lv.mod( rv, v ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod qgmValueTuple:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMSELECTOREXPRNODE__CALCVALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

