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

   Source File Name = qgmPtrTable.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPtrTable.hpp"
#include "qgmUtil.hpp"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   #define QGM_VALUE_PTR( itr, ptr, size ) \
        do { \
            ptr = &(*(itr->value.begin())) ; \
            size = itr->value.end() - itr->value.begin() ; \
        } while( 0 )


   /*
      _qgmPtrTable implement
   */
   _qgmPtrTable::_qgmPtrTable()
   {
      _uniqueFieldID = 0 ;
      _uniqueTableID = 0 ;
   }

   _qgmPtrTable::~_qgmPtrTable()
   {
      _table.clear() ;

      STR_TABLE::iterator it = _stringTable.begin() ;
      while ( it != _stringTable.end() )
      {
         CHAR *p = ( CHAR* )it->first ;
         SDB_THREAD_FREE( p ) ;
         ++it ;
      }
      _stringTable.clear() ;
   }

   INT32 _qgmPtrTable::getField( const SQL_CON_ITR &itr,
                                 qgmField &field )
   {
      const CHAR *begin = NULL ;
      UINT32 size = 0 ;
      QGM_VALUE_PTR( itr, begin, size ) ;
      return getField( begin, size, field ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPTRTABLE_GETFIELD, "_qgmPtrTable::getField" )
   INT32 _qgmPtrTable::getField( const CHAR *begin,
                                 UINT32 size,
                                 qgmField &field )
   {
      PD_TRACE_ENTRY( SDB__QGMPTRTABLE_GETFIELD ) ;
      INT32 rc = SDB_OK ;
      qgmField f ;
      PTR_TABLE::const_iterator itr ;

      if ( NULL == begin || 0 == size )
      {
         field._begin = "" ;
         field._size = 0 ;
         goto done ;
      }

      f._begin = begin ;
      f._size = size ;
      f._ptrTable = this ;
      itr = _table.find( f ) ;
      if ( _table.end() == itr )
      {
         field = f ;
         _table.insert( f ) ;
      }
      else
      {
         field = *itr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPTRTABLE_GETFIELD, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPTRTABLE_GETOWNFIELD, "_qgmPtrTable::getOwnField" )
   INT32 _qgmPtrTable::getOwnField( const CHAR *begin,
                                    UINT32 size,
                                    qgmField &field )
   {
      PD_TRACE_ENTRY( SDB__QGMPTRTABLE_GETOWNFIELD ) ;
      SDB_ASSERT( NULL != begin, "impossible" ) ;
      INT32 rc = SDB_OK ;
      qgmField f ;
      PTR_TABLE::const_iterator itr ;

      if ( NULL == begin || 0 == size )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      f._begin = begin ;
      f._size = size ;
      f._ptrTable = this ;
      itr = _table.find( f ) ;
      if ( _table.end() == itr )
      {
         field._begin = getOwnedString( f.toString().c_str() ) ;
         field._size = ossStrlen( field._begin ) ;
         field._ptrTable = this ;
         _table.insert( field ) ;
      }
      else
      {
         field = *itr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPTRTABLE_GETOWNFIELD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   
   INT32 _qgmPtrTable::getOwnField( const CHAR *begin,
                                    qgmField &field )
   {
      if ( begin )
      {
         return getOwnField( begin, ossStrlen( begin ), field ) ;
      }
      return SDB_INVALIDARG ;
   }

   _qgmField _qgmPtrTable::getField( const qgmField &sub1,
                                     const qgmField &sub2 )
   {
      if ( sub1.empty() )
      {
         return sub2 ;
      }
      else if ( sub2.empty() )
      {
         return sub1 ;
      }
      else
      {
         ossPoolString str( sub1.begin(), sub1.size() ) ;
         str += sub2.toString() ;
         qgmField merge ;
         getOwnField( str.c_str(), merge ) ;
         return merge ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPTRTABLE_GETATTR, "_qgmPtrTable::getAttr" )
   INT32 _qgmPtrTable::getAttr( const CHAR *begin,
                                UINT32 size,
                                qgmDbAttr &attr )
   {
      PD_TRACE_ENTRY( SDB__QGMPTRTABLE_GETATTR ) ;
      INT32 rc = SDB_OK ;
      UINT32 pos = 0 ;
      BOOLEAN hasDot = qgmUtilFirstDot( begin, size, pos ) ;
      if ( hasDot && ( 0 == pos || size - 1 == pos ) )
      {
         PD_LOG( PDERROR,
                 "the first char and the last char can not be '.'" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( hasDot )
      {
         rc = getField( begin, pos, attr.relegation() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ++pos ;
      }

      rc = getField( begin + pos, size - pos, attr.attr() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPTRTABLE_GETATTR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPTRTABLE_GETOWNATTR, "_qgmPtrTable::getOwnAttr" )
   INT32 _qgmPtrTable::getOwnAttr( const CHAR *begin,
                                   UINT32 size,
                                   qgmDbAttr &attr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__QGMPTRTABLE_GETOWNATTR ) ;

      UINT32 pos = 0 ;
      BOOLEAN hasDot = qgmUtilFirstDot( begin, size, pos ) ;
      if ( hasDot && ( 0 == pos || size - 1 == pos ) )
      {
         PD_LOG( PDERROR,
                 "the first char and the last char can not be '.'" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( hasDot )
      {
         rc = getOwnField( begin, pos, attr.relegation() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ++pos ;
      }

      rc = getOwnField( begin + pos, size - pos, attr.attr() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPTRTABLE_GETOWNATTR, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmPtrTable::getAttr( const SQL_CON_ITR &itr,
                                qgmDbAttr &attr )
   {
      const CHAR *begin = NULL ;
      UINT32 size = 0 ;
      QGM_VALUE_PTR( itr, begin, size ) ;
      return getAttr( begin, size, attr ) ;
   }

   INT32 _qgmPtrTable::getUniqueFieldAlias( qgmField &field )
   {
      CHAR uniqueName[20] = {0} ;
      ossSnprintf( uniqueName, 19, "$SYS_F%d", ++_uniqueFieldID ) ;
      return getOwnField( uniqueName, field ) ;
   }

   INT32 _qgmPtrTable::getUniqueTableAlias( qgmField & field )
   {
      CHAR uniqueName[20] = {0} ;
      ossSnprintf( uniqueName, 19, "$SYS_T%d", ++_uniqueTableID ) ;
      return getOwnField( uniqueName, field ) ;
   }

   const CHAR* _qgmPtrTable::getOwnedString( const CHAR *str )
   {
      STR_TABLE_IT it = _stringTable.find( str ) ;
      if ( it != _stringTable.end() )
      {
         ++( it->second ) ;
         return it->first ;
      }
      else
      {
         UINT32 size = ossStrlen( str ) ;
         CHAR *dup = (CHAR*)SDB_THREAD_ALLOC( size + 1 ) ;
         if ( !dup )
         {
            PD_LOG( PDERROR, "Alloc string(%s, %d) failed: out-of-memory",
                    str, size ) ;
            dup = "" ;
         }
         else
         {
            ossStrncpy( dup, str, size ) ;
            dup[ size ] = '\0' ;
            _stringTable[ dup ] = 1 ;
         }
         return dup ;
      }
   }

}

