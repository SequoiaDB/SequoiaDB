/*******************************************************************************


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

   Source File Name = qgmDef.cpp

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

*******************************************************************************/

#include "qgmDef.hpp"
#include "qgmUtil.hpp"
#include "qgmPtrTable.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "mthDef.hpp"
#include "utilMath.hpp"
#include "utilMemListPool.hpp"

using namespace bson ;

namespace engine
{

   const UINT32 _qgmField::npos = 0xFFFFFFFF ;
   #define QGM_FAST_STRING_SIZE        ( 64 )

   /*
      _qgmField implement
   */
   _qgmField::_qgmField()
   :_ptrTable( NULL ), _begin( "" ), _size( 0 )
   {
   }

   _qgmField::_qgmField( const _qgmField &field )
   :_ptrTable( field._ptrTable ), _begin( field._begin ), _size( field._size)
   {
   }

   _qgmField& _qgmField::operator=(const _qgmField &field )
   {
      _ptrTable = field._ptrTable ;
      _begin = field._begin ;
      _size = field._size ;
      return *this ;
   }

   _qgmField::~_qgmField()
   {
      _begin = "" ;
      _size = 0 ;
   }

   BOOLEAN _qgmField::operator==( const _qgmField &field )const
   {
      if ( _size != field._size )
      {
         return FALSE ;
      }
      const CHAR *l = _begin ;
      const CHAR *r = field._begin ;
      UINT32 pos = 0 ;
      while( pos < _size )
      {
         if ( *l != *r )
         {
            return FALSE ;
         }
         ++l ;
         ++r ;
         ++pos ;
      }
      return TRUE ;
   }

   BOOLEAN _qgmField::operator!=( const _qgmField &field )const
   {
      if ( _size != field._size )
      {
         return TRUE ;
      }
      const CHAR *l = _begin ;
      const CHAR *r = field._begin ;
      UINT32 pos = 0 ;
      while( pos < _size )
      {
         if ( *l != *r )
         {
            return TRUE ;
         }
         ++l ;
         ++r ;
         ++pos ;
      }
      return FALSE ;
   }

   BOOLEAN _qgmField::operator<( const _qgmField &field )const
   {
      UINT32 i = 0 ;
      while ( i < this->_size && i < field._size )
      {
         if ( _begin[i] < field._begin[i] )
         {
            return TRUE ;
         }
         else if ( _begin[i] > field._begin[i] )
         {
            return FALSE ;
         }
         else
         {
            ++i ;
         }
      }

      return this->_size < field._size ? TRUE : FALSE ;
   }

   BOOLEAN _qgmField::isSubfix( const _qgmField &field,
                                BOOLEAN includeSame,
                                UINT32 *pPos ) const
   {
      if ( pPos )
      {
         *pPos = _qgmField::npos ;
      }

      UINT32 i = 0 ;
      while( i < _size && i < field._size )
      {
         if ( _begin[i] != field._begin[i] )
         {
            break ;
         }
         ++i ;
      }
      if ( i == field._size )
      {
         if ( i + 2 <= _size && '.' == _begin[i] )
         {
            if ( pPos )
            {
               *pPos = i ;
            }
            return TRUE ;
         }
         else if ( includeSame && i == _size )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   _qgmField _qgmField::subField( UINT32 pos, UINT32 size ) const
   {
      _qgmField sub ;
      sub._ptrTable = _ptrTable ;

      if ( pos < _size )
      {
         sub._begin = _begin + pos ;
         sub._size = size < ( _size - pos ) ? size : ( _size - pos ) ;
      }
      return sub ;
   }

   /// ex: self: abc.dek, return: abc
   _qgmField _qgmField::rootField() const
   {
      UINT32 pos = 0 ;
      _qgmField root( *this ) ;

      if( qgmUtilFirstDot( _begin, _size, pos ) )
      {
         root._size = pos ;
      }
      return root ;
   }

   /// ex: self: abc.dek, return: dek
   _qgmField _qgmField::lastField() const
   {
      UINT32 pos = 0 ;
      _qgmField last( *this ) ;

      if ( qgmUtilLastDot( _begin, _size, pos ) && ++pos < _size )
      {
         last._begin += pos ;
         last._size -= pos ;
      }
      return last ;
   }

   /// ex: self: abc.def.kk, cur: abc, return: def
   ///                       cur: kk,  return: (null)
   _qgmField _qgmField::nextField( const _qgmField &cur ) const
   {
      _qgmField next ;
      next._ptrTable = _ptrTable ;
      const CHAR *end = _begin + _size ;
      const CHAR *pos = cur._begin + cur._size + 1 ;

      if ( !cur.empty() &&
           cur._begin >= _begin &&
           pos < end )
      {
         next._begin = pos ;
         next._size = end - pos ;
         while( pos < end )
         {
            if ( '.' == *pos )
            {
               next._size = pos - next._begin ;
               break ;
            }
            ++pos ;
         }
      }
      return next ;
   }

   /// ex: self: abc.def.kk, cur: kk, return: def
   ///                       cur: abc,return: (null)
   _qgmField _qgmField::preField( const _qgmField &cur ) const
   {
      _qgmField next ;
      next._ptrTable = _ptrTable ;
      const CHAR *pos = cur._begin - 1 ;

      if ( !cur.empty() &&
           cur._begin + cur._size <= _begin + _size &&
           pos > _begin )
      {
         next._begin = _begin ;
         next._size = pos - _begin ;
         while( pos > _begin )
         {
            --pos ;
            if ( '.' == *pos )
            {
               next._begin = pos + 1 ;
               next._size = cur._begin - 1 - next._begin ;
               break ;
            }
         }
      }
      return next ;
   }

   void _qgmField::replace( UINT32 pos, UINT32 size, const _qgmField &field )
   {
      if ( _ptrTable )
      {
         if ( 0 == pos ) // header
         {
            *this = _ptrTable->getField( field, subField( size ) ) ;
         }
         else if ( pos + size >= _size ) // tail
         {
            *this = _ptrTable->getField( subField( pos ), field ) ;
         }
         else // middle
         {
            qgmField tmp = _ptrTable->getField( subField( 0, pos ),
                                                field ) ;
            *this = _ptrTable->getField( tmp, subField( size ) ) ;
         }
      }
   }

   BOOLEAN _qgmField::isArrayIndexFormat() const
   {
      INT32 num = 0 ;
      if ( _begin && '$' == *_begin && '[' == *(_begin+1) &&
           SDB_OK == mthConvertSubElemToNumeric( _begin, num ) )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _qgmField::isDotted() const
   {
      UINT32 i = 0 ;
      while( i < _size )
      {
         if ( _begin[ i ] == '.' )
         {
            return TRUE ;
         }
         ++i ;
      }
      return FALSE ;
   }

   ossPoolString _qgmField::toFieldName() const
   {
      StringBuilder ss( _size + 1 ) ;

      if ( _size > 0 )
      {
         CHAR *namePtr = NULL ;
         CHAR fastStr[ QGM_FAST_STRING_SIZE + 1 ] = { 0 } ;

         if ( _size > QGM_FAST_STRING_SIZE )
         {
            namePtr = (CHAR*)SDB_THREAD_ALLOC( _size + 1 ) ;
            if ( !namePtr )
            {
               return ossPoolString() ;
            }
            ossMemcpy( namePtr, _begin, _size ) ;
            namePtr[ _size ] = 0 ;
         }
         else
         {
            ossMemcpy( fastStr, _begin, _size ) ;
            namePtr = fastStr ;
         }

         INT32 pos = 0 ;
         INT32 num = 0 ;
         utilSplitIterator i( namePtr, '.' ) ;
         while ( i.more() )
         {
            if ( 0 == pos )
            {
               ++pos ;
            }
            else
            {
               ss << "." ;
            }

            const CHAR *left = i.next() ;
            if ( '$' == *left && '[' == *(left + 1) &&
                 SDB_OK == mthConvertSubElemToNumeric( left, num ) )
            {
               ss << num ;
            }
            else
            {
               ss << left ;
            }
         }
         i.finish() ;

         if ( namePtr && namePtr != fastStr )
         {
            SDB_THREAD_FREE( namePtr ) ;
            namePtr = NULL ;
         }
      }

      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMFETCHOUT_ELEMENT, "_qgmFetchOut::element" )
   INT32 _qgmFetchOut::element( const _qgmDbAttr &attr,
                                BSONElement &ele )const
   {
      PD_TRACE_ENTRY( SDB__QGMFETCHOUT_ELEMENT ) ;
      INT32 rc = SDB_OK ;
      BSONElement next, local ;

      SDB_ASSERT( !attr.empty(), "impossible" ) ;

      if ( NULL == this->next )
      {
         try
         {
            local = obj.getFieldDotted( attr.attr().toFieldName().c_str() );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happend:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         ele = local ;
      }
      else
      {
         if ( attr.relegation().empty() )
         {
            try
            {
               ossPoolString fieldName = attr.attr().toFieldName() ;
               local = obj.getFieldDotted( fieldName.c_str() ) ;
               next = this->next->obj.getFieldDotted( fieldName.c_str() ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happend:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( local.eoo() && !next.eoo() )
            {
               ele = next ;
            }
            else if ( !local.eoo() && next.eoo() )
            {
               ele = local ;
            }
            else if ( !local.eoo() && !next.eoo() )
            {
               rc = SDB_QGM_AMBIGUOUS_FIELD ;
               PD_LOG( PDERROR, "ambiguous filed name:%s, obj: %s, "
                       "next obj: %s", attr.attr().toString().c_str(),
                       obj.toString().c_str(),
                       this->next->obj.toString().c_str() ) ;
               goto error ;
            }
            else
            {
               PD_LOG( PDERROR, "field [%s] not found from fetchout, obj: %s, "
                       "next obj: %s", attr.attr().toString().c_str(),
                       obj.toString().c_str(),
                       this->next->obj.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            BSONObj srcObj ;
            if ( attr.relegation() == this->next->alias )
            {
               srcObj = this->next->obj ;
            }
            else if ( attr.relegation() == this->alias )
            {
               srcObj = this->obj ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "relegaion [%s] not found, alias: %s, "
                       "next alias: %s", attr.relegation().toString().c_str(),
                       alias.toString().c_str(),
                       this->next->alias.toString().c_str() ) ;
               goto error ;
            }

            try
            {
               local = srcObj.getFieldDotted( attr.attr().toFieldName().c_str() ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "unexpected err happend:%s", e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            ele = local ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMFETCHOUT_ELEMENT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMFETCHOUT_ELEMENTS, "_qgmFetchOut::elements" )
   void _qgmFetchOut::elements( qgmFOElementVec &eles ) const
   {
      PD_TRACE_ENTRY( SDB__QGMFETCHOUT_ELEMENTS ) ;
      BSONObjIterator itr( obj ) ;
      while ( itr.more() )
      {
         eles.push_back( itr.next() ) ;
      }

      if ( NULL != next )
      {
         next->elements( eles ) ;
      }

      PD_TRACE_EXIT( SDB__QGMFETCHOUT_ELEMENTS ) ;
      return ;
   }

   BSONObj _qgmFetchOut::mergedObj()const
   {
      return NULL == next ?
             this->obj.getOwned() : qgmMerge( obj, next->mergedObj() ) ;
   }

////////////////// _qgmValueTuple
   _qgmValueTuple::_qgmValueTuple( CHAR *data, UINT32 len, BOOLEAN format )
   :_row( data ),
    _len( len ),
    _isOwn( FALSE )
   {
      SDB_ASSERT( NULL != _row, "can not be null" ) ;
      SDB_ASSERT( sizeof( _tuple ) <= _len, "impossible" ) ;
      if ( format )
      {
         _tuple *t = ( _tuple * )_row ;
         t->len = 0 ;
         t->type = ( INT16 )bson::EOO ;
         t->flag = 0 ;
      }
   }

   _qgmValueTuple::~_qgmValueTuple()
   {
      if ( _isOwn )
      {
         SDB_THREAD_FREE( _row ) ;
         _row   = NULL ;
         _isOwn = FALSE ;
      }
   }

   INT32 _qgmValueTuple::_ensureMem( UINT32 dataLen )
   {
      INT32 rc = SDB_OK ;
      if ( _len < ( dataLen + sizeof( _tuple ) ) )
      {
         _tuple saved ;
         ossMemcpy( &saved, _row, sizeof( saved ) ) ;
         if ( _isOwn )
         {
            SDB_THREAD_FREE( _row ) ;
            _row   = NULL ;
            _isOwn = FALSE ;
         }

         _row = (CHAR *) SDB_THREAD_ALLOC( dataLen + sizeof( _tuple ) ) ;
         if ( NULL == _row )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "malloc for _row failed:rc=%d", rc ) ;
            goto error ;
         }

         _isOwn = TRUE ;
         ossMemcpy( _row, &saved, sizeof( saved ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::setValue( UINT32 dataLen, const void *data, 
                                   INT16 type )
   {
      INT32 rc = SDB_OK ;
      _tuple *t = NULL ;

      if ( NULL == _row )
      {
         PD_LOG( PDERROR, "row data is null" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _ensureMem( dataLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_ensureMem failed:rc=%d", rc ) ;
         goto error ;
      }

      t       = ( _tuple * )_row ;
      t->len  = sizeof( _tuple ) + dataLen ;
      t->type = type ;
      if ( 0 < dataLen )
      {
         ossMemcpy( _row + sizeof( _tuple ), data, dataLen ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::setValue( const bsonDecimal &decimal )
   {
      INT32 rc      = SDB_OK ;
      _tuple *t     = NULL ;
      INT32 dataLen = 0 ;
      CHAR *pData   = NULL ;
      INT32 i       = 0 ;
      const INT16 *digits = NULL ;

      if ( NULL == _row )
      {
         PD_LOG( PDERROR, "row data is null" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      dataLen = decimal.getSize() ;

      rc = _ensureMem( dataLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_ensureMem failed:rc=%d", rc ) ;
         goto error ;
      }

      t       = ( _tuple * )_row ;
      t->len  = sizeof( _tuple ) + dataLen ;
      t->type = NumberDecimal ;

      pData   = _row + sizeof( _tuple ) ;

      //define in common_decimal.h __sdb_decimal
      // size
      *( INT32 *)pData = dataLen ;
      pData += sizeof( INT32 ) ;

      // typemod
      *( INT32 *)pData = decimal.getTypemod() ;
      pData += sizeof( INT32 ) ;

      // sign + scale 
      *( INT16 *)pData = decimal.getStorageScale() ;
      pData += sizeof( INT16 ) ;

      // weight
      *( INT16 *)pData = decimal.getWeight() ;
      pData += sizeof( INT16 ) ;

      digits = decimal.getDigits() ;
      for ( i = 0 ; i < decimal.getNdigit() ; i++ )
      {
         *( INT16 *)pData = digits[i] ;
         pData += sizeof( INT16 ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::_getDecimal( bsonDecimal &decimal ) const 
   {
      INT32 rc = SDB_OK ;
      INT16 valueType = 0 ;

      valueType = getValueType() ;
      if ( ( INT16 )bson::EOO == valueType )
      {
         decimal.setZero() ;
      }
      else if ( ( INT16 )bson::NumberDecimal == valueType )
      {
         rc = decimal.fromBsonValue( (const CHAR *)getValue() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fromBsonValue:%d", rc ) ;
            goto error ;
         }
      }
      else if ( ( INT16 )bson::NumberDouble == valueType )
      {
         rc = decimal.fromDouble( *(( FLOAT64* )( getValue() ) ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fromDouble:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = decimal.fromLong( *(( INT64* )( getValue() ) ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fromDouble:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::add( const _qgmValueTuple &right, 
                              _qgmValueTuple &result ) const
   {
      INT32 rc = SDB_OK ;
      if ( ( INT16 )bson::EOO == getValueType() ||
           ( INT16 )bson::EOO == right.getValueType() )
      {
         rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
      }
      else if ( ( INT16 )bson::NumberDecimal == getValueType() ||
                ( INT16 )bson::NumberDecimal == right.getValueType() )
      {
         bsonDecimal lDecimal ;
         bsonDecimal rDecimal ;
         bsonDecimal resultDecimal ;

         rc = _getDecimal( lDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = right._getDecimal( rDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = lDecimal.add( rDecimal, resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add decimal:%d", rc ) ;
            goto error ;
         }

         rc = result.setValue( resultDecimal ) ;
      }
      else if ( ( INT16 )bson::NumberDouble == getValueType() ||
                ( INT16 )bson::NumberDouble == right.getValueType() )
      {
         FLOAT64 lNumber = ( INT16 )bson::NumberDouble == getValueType() ?
                           *(( FLOAT64* )( getValue() ) ) :
                           *(( INT64* )( getValue() ) ) ;
         FLOAT64 rNumber = ( INT16 )bson::NumberDouble == right.getValueType() ?
                           *(( FLOAT64* )( right.getValue() ) ) :
                           *(( INT64* )( right.getValue() ) ) ;

         FLOAT64 final = lNumber + rNumber ;
         rc = result.setValue( sizeof( FLOAT64 ), &final, 
                               ( INT16 )bson::NumberDouble ) ;
      }
      else if ( ( INT16 )bson::NumberLong == getValueType() ||
                ( INT16 )bson::NumberLong == right.getValueType() )
      {
         INT64 l = *(( INT64 * )( getValue() ) ) ;
         INT64 r = *(( INT64 * )( right.getValue() ) ) ;
         INT64 rst = l + r ;
         if ( !utilAddIsOverflow( l, r, rst ) )
         {
            rc = result.setValue( sizeof( INT64 ), &rst, 
                                  ( INT16 )bson::NumberLong ) ;
         }
         else 
         {
            bsonDecimal decL, decR, decRst;
            rc = decL.fromLong(l) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decR.fromLong(r) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decL.add( decR, decRst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add decimal:%d", rc ) ;
               goto error ;
            }
            rc = result.setValue( decRst ) ;
         }
      }
      else
      {
         INT32 l = *(( INT32 * )( getValue() ) ) ;
         INT32 r = *(( INT32 * )( right.getValue() ) ) ;
         INT32 rst = l + r ;
         INT64 rst64 = (INT64)l + (INT64)r ;
         if ( rst64 == (INT64)rst )
         {
            rc = result.setValue( sizeof( INT32 ), &rst, 
                                  ( INT16 )bson::NumberInt ) ;
         }
         else
         {
            rc = result.setValue( sizeof( INT64 ), &rst64, 
                                  ( INT16 )bson::NumberLong ) ;
         }       
      }
      
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::sub( const _qgmValueTuple &right, 
                              _qgmValueTuple &result ) const
   {
      INT32 rc = SDB_OK ;
      if ( ( INT16 )bson::EOO == getValueType() ||
           ( INT16 )bson::EOO == right.getValueType() )
      {
         rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
      }
      else if ( ( INT16 )bson::NumberDecimal == getValueType() ||
                ( INT16 )bson::NumberDecimal == right.getValueType() )
      {
         bsonDecimal lDecimal ;
         bsonDecimal rDecimal ;
         bsonDecimal resultDecimal ;

         rc = _getDecimal( lDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = right._getDecimal( rDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = lDecimal.sub( rDecimal, resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub decimal:%d", rc ) ;
            goto error ;
         }

         rc = result.setValue( resultDecimal ) ;
      }
      else if ( ( INT16 )bson::NumberDouble == getValueType() ||
                ( INT16 )bson::NumberDouble == right.getValueType() )
      {
         FLOAT64 lNumber = ( INT16 )bson::NumberDouble == getValueType() ?
                           *(( FLOAT64* )( getValue() ) ) :
                           *(( INT64* )( getValue() ) ) ;
         FLOAT64 rNumber = ( INT16 )bson::NumberDouble == right.getValueType() ?
                           *(( FLOAT64* )( right.getValue() ) ) :
                           *(( INT64* )( right.getValue() ) ) ;
         FLOAT64 final = lNumber - rNumber ;
         rc = result.setValue( sizeof( FLOAT64 ), &final, 
                               ( INT16 )bson::NumberDouble ) ;
      }
      else if ( ( INT16 )bson::NumberLong == getValueType() ||
                ( INT16 )bson::NumberLong == right.getValueType() )
      {
         INT64 l = *(( INT64 * )( getValue() ) ) ;
         INT64 r = *(( INT64 * )( right.getValue() ) ) ;
         INT64 rst = l - r ;
         if ( !utilSubIsOverflow( l, r, rst ) )
         {
            rc = result.setValue( sizeof( INT64 ), &rst, 
                                  ( INT16 )bson::NumberLong ) ;
         }
         else 
         {
            bsonDecimal decL, decR, decRst;
            rc = decL.fromLong(l) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decR.fromLong(r) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decL.sub( decR, decRst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add decimal:%d", rc ) ;
               goto error ;
            }
            rc = result.setValue( decRst ) ;
         }
      }
      else
      {
         INT32 l = *(( INT32 * )( getValue() ) ) ;
         INT32 r = *(( INT32 * )( right.getValue() ) ) ;
         INT32 rst = l - r ;
         INT64 rst64 = (INT64)l - (INT64)r ;
         if ( rst64 == (INT64)rst )
         {
            rc = result.setValue( sizeof( INT32 ), &rst, 
                                  ( INT16 )bson::NumberInt ) ;
         }
         else
         {
            rc = result.setValue( sizeof( INT64 ), &rst64, 
                                  ( INT16 )bson::NumberLong ) ;
         }       
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::multiply( const _qgmValueTuple &right, 
                                   _qgmValueTuple &result ) const
   {
      INT32 rc = SDB_OK ;
      if ( ( INT16 )bson::EOO == getValueType() ||
           ( INT16 )bson::EOO == right.getValueType() )
      {
         rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
      }
      else if ( ( INT16 )bson::NumberDecimal == getValueType() ||
                ( INT16 )bson::NumberDecimal == right.getValueType() )
      {
         bsonDecimal lDecimal ;
         bsonDecimal rDecimal ;
         bsonDecimal resultDecimal ;

         rc = _getDecimal( lDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = right._getDecimal( rDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = lDecimal.mul( rDecimal, resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mul decimal:%d", rc ) ;
            goto error ;
         }

         rc = result.setValue( resultDecimal ) ;
      }
      else if ( ( INT16 )bson::NumberDouble == getValueType() ||
                ( INT16 )bson::NumberDouble == right.getValueType() )
      {
         FLOAT64 lNumber = ( INT16 )bson::NumberDouble == getValueType() ?
                           *(( FLOAT64* )( getValue() ) ) :
                           *(( INT64* )( getValue() ) ) ;
         FLOAT64 rNumber = ( INT16 )bson::NumberDouble == right.getValueType() ?
                           *(( FLOAT64* )( right.getValue() ) ) :
                           *(( INT64* )( right.getValue() ) ) ;
         FLOAT64 final = lNumber * rNumber ;
         rc = result.setValue( sizeof( FLOAT64 ), &final, 
                               ( INT16 )bson::NumberDouble ) ;
      }
      else if ( ( INT16 )bson::NumberLong == getValueType() ||
                ( INT16 )bson::NumberLong == right.getValueType() )
      {
         INT64 l = *(( INT64 * )( getValue() ) ) ;
         INT64 r = *(( INT64 * )( right.getValue() ) ) ;
         INT64 rst = l * r ;
         if ( !utilMulIsOverflow( l, r, rst ) )
         {
            rc = result.setValue( sizeof( INT64 ), &rst, 
                                  ( INT16 )bson::NumberLong ) ;
         }
         else 
         {
            bsonDecimal decL, decR, decRst;
            rc = decL.fromLong(l) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decR.fromLong(r) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromLong:%d", rc ) ;
               goto error ;
            }
            rc = decL.mul( decR, decRst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add decimal:%d", rc ) ;
               goto error ;
            }
            rc = result.setValue( decRst ) ;
         }
      }
      else
      {
         INT32 l = *(( INT32 * )( getValue() ) ) ;
         INT32 r = *(( INT32 * )( right.getValue() ) ) ;
         INT32 rst = l * r ;
         INT64 rst64 = (INT64)l * (INT64)r ;
         if ( rst64 == (INT64)rst )
         {
            rc = result.setValue( sizeof( INT32 ), &rst, 
                                  ( INT16 )bson::NumberInt ) ;
         }
         else
         {
            rc = result.setValue( sizeof( INT64 ), &rst64, 
                                  ( INT16 )bson::NumberLong ) ;
         }       
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::divide( const _qgmValueTuple &right, 
                                 _qgmValueTuple &result ) const 
   {
      INT32 rc = SDB_OK ;
      if ( ( INT16 )bson::EOO == getValueType() ||
           ( INT16 )bson::EOO == right.getValueType() )
      {
         rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
      }
      else if ( ( INT16 )bson::NumberDecimal == getValueType() ||
                ( INT16 )bson::NumberDecimal == right.getValueType() )
      {
         bsonDecimal lDecimal ;
         bsonDecimal rDecimal ;
         bsonDecimal resultDecimal ;

         rc = _getDecimal( lDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = right._getDecimal( rDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = lDecimal.div( rDecimal, resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to div decimal:%d", rc ) ;
            goto error ;
         }

         rc = result.setValue( resultDecimal ) ;
      }
      else if ( ( INT16 )bson::NumberDouble == getValueType() ||
                ( INT16 )bson::NumberDouble == right.getValueType() )
      {
         FLOAT64 rNumber = ( INT16 )bson::NumberDouble == right.getValueType() ?
                           *(( FLOAT64* )( right.getValue() ) ) :
                           *(( INT64* )( right.getValue() ) ) ;
         if ( fabs( rNumber ) < OSS_EPSILON )
         {
            rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
         }
         else
         {
            FLOAT64 lNumber = ( INT16 )bson::NumberDouble == getValueType() ?
                               *(( FLOAT64* )( getValue() ) ) :
                               *(( INT64* )( getValue() ) ) ;
            FLOAT64 final = lNumber / rNumber ;
            rc = result.setValue( sizeof( FLOAT64 ), &final, 
                                  ( INT16 )bson::NumberDouble ) ;
         }
      }
      else if ( ( INT16 )bson::NumberLong == getValueType() ||
                ( INT16 )bson::NumberLong == right.getValueType() )
      {
         //NumberLong
         INT64 lNumber = *((INT64 *)(getValue() ) ) ;
         INT64 rNumber = *(( INT64 * )( right.getValue() ) ) ;
         INT64 result64 ;
         if ( 0 == rNumber )
         {
            rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
            goto done ;
         }
         if ( !utilDivIsOverflow( lNumber, rNumber ) )
         {
            result64 = lNumber / rNumber ;
            rc = result.setValue( sizeof( INT64 ), &result64, 
                                  ( INT16 )bson::NumberLong ) ;
         }
         else
         {
            //overflow
            bsonDecimal decResult ;
            rc = decResult.fromString( "9223372036854775808" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to div decimal:%lld/%lld,rc=%d",
                       lNumber, rNumber, rc ) ;
               goto error ;
            }
            rc = result.setValue( decResult ) ;
         }
      }
      else
      {
         // int32
         INT32 lNumber = *((INT32 *)(getValue() ) ) ;
         INT32 rNumber = *(( INT32 * )( right.getValue() ) ) ;
         INT32 result32 ;
         if ( 0 == rNumber )
         {
            rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
            goto done ;
         }

         if ( -1 == rNumber )
         {
            if ( lNumber != (INT32)OSS_SINT32_MIN )
            {
               result32 = -lNumber ;
               rc = result.setValue( sizeof( INT32 ), &result32, 
                                     ( INT16 )bson::NumberInt ) ;
            }
            else
            {
               INT64 result64, lNumber64 ;
               lNumber64 = (INT64)lNumber ;
               result64 = -lNumber64 ;
               
               rc = result.setValue( sizeof( INT64 ), &result64, 
                                     ( INT16 )bson::NumberLong ) ;
            }
         }
         else
         {
            result32 = lNumber / rNumber ;
            rc = result.setValue( sizeof( INT32 ), &result32, 
                                  ( INT16 )bson::NumberInt ) ;
         }
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmValueTuple::mod( const _qgmValueTuple &right, 
                              _qgmValueTuple &result ) const
   {
      INT32 rc = SDB_OK ;
      if ( ( INT16 )bson::EOO == getValueType() ||
           ( INT16 )bson::EOO == right.getValueType() )
      {
         rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
            goto error ;
         }
      }
      else if ( ( INT16 )bson::NumberDecimal == getValueType() ||
                ( INT16 )bson::NumberDecimal == right.getValueType() )
      {
         bsonDecimal lDecimal ;
         bsonDecimal rDecimal ;
         bsonDecimal resultDecimal ;

         rc = _getDecimal( lDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = right._getDecimal( rDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to _getDecimal:%d", rc ) ;
            goto error ;
         }

         rc = lDecimal.mod( rDecimal, resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod decimal:%d", rc ) ;
            goto error ;
         }

         rc = result.setValue( resultDecimal ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
            goto error ;
         }
      }
      else if ( ( INT16 )bson::NumberDouble == getValueType() ||
                ( INT16 )bson::NumberDouble == right.getValueType() )
      {
         FLOAT64 rNumber = ( INT16 )bson::NumberDouble == right.getValueType() ?
                           *(( FLOAT64* )( right.getValue() ) ) :
                           *(( INT64* )( right.getValue() ) ) ;
         if ( fabs( rNumber ) < OSS_EPSILON )
         {
            rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            FLOAT64 lNumber = ( INT16 )bson::NumberDouble == getValueType() ?
                               *(( FLOAT64* )( getValue() ) ) :
                               *(( INT64* )( getValue() ) ) ;
            FLOAT64 final = MTH_MOD( lNumber, rNumber ) ;
            rc = result.setValue( sizeof( FLOAT64 ), &final, 
                                  ( INT16 )bson::NumberDouble ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
      }
      else
      {
         //NumberLong
         INT64 lNumber = *((INT64 *)(getValue() ) ) ;
         INT64 rNumber = *(( INT64 * )( right.getValue() ) ) ;
         if ( 0 == rNumber )
         {
            rc = result.setValue( 0, NULL, ( INT16 )bson::EOO ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            INT64 final = lNumber % rNumber ;
            rc = result.setValue( sizeof( INT64 ), &final, 
                                  ( INT16 )bson::NumberLong ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to set value:%d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

