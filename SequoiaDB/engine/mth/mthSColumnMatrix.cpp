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

   Source File Name = mthSColumnMatrix.hpp

   Descriptive Name = mth selector column matrix

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSColumnMatrix.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "pdTrace.hpp"
#include "utilStr.hpp"
#include "mthCommon.hpp"
#include "mthSActionParser.hpp"

using namespace bson ;

namespace engine
{
   _mthSColumnMatrix::_mthSColumnMatrix()
   {

   }

   _mthSColumnMatrix::~_mthSColumnMatrix()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX_CLEAR, "_mthSColumnMatrix::clear" )
   void _mthSColumnMatrix::clear()
   {
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX_CLEAR ) ;
      if ( !_pattern.isEmpty() )
      {
         this->_mthSColumn::clear() ;
         _columnPool.clear() ;
         _actionPool.clear() ;
         _pattern = BSONObj() ;
      }
      PD_TRACE_EXIT( SDB__MTHSCOLUMNMATRIX_CLEAR ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX_LOAD, "_mthSColumnMatrix::load" )
   INT32 _mthSColumnMatrix::load( const bson::BSONObj &obj, BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX_LOAD ) ;

      if ( !_pattern.isEmpty() )
      {
         PD_LOG( PDERROR, "clear matrix before use it" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( obj.isEmpty() )
      {
         goto done ;
      }

      _pattern = obj.copy() ;

      try
      {
         /// must be sorted.
         BSONObjIteratorSorted i( _pattern ) ;
         while ( i.more() )
         {
            rc = _load( i.next(), strictDataMode ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to load selector column[%s], rc:%d",
                       _pattern.toString( FALSE, TRUE ).c_str(), rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX_LOAD, rc ) ;
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX_SELECT, "_mthSColumnMatrix::select" )
   INT32 _mthSColumnMatrix::select( const bson::BSONObj &src,
                                    bson::BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX_SELECT ) ;
      BSONObjBuilder builder( src.objsize() * 1.2 ) ;

      rc = _buildObjFromChildren( src, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to select columns "
                 "wieh inclusion mode:%d", rc ) ;
         goto error ;
      }

      result = builder.obj() ;
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX_SELECT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__LOAD, "_mthSColumnMatrix::_load" )
   INT32 _mthSColumnMatrix::_load( const bson::BSONElement &e, BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__LOAD ) ;
      _mthSColumn *column = NULL ;

      if ( Object != e.type() )
      {
         rc = _loadDefaultValue( e ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to insert element[%s] "
                    "as a default value, rc:%d",
                    e.toString( FALSE, TRUE ).c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT32 actionNum = 0 ;
         rc = _getColumn( e.fieldName(), column ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get column[%s], rc:%d",
                    e.fieldName(), rc ) ;
            goto error ;
         }

         SDB_ASSERT( NULL != column, "can not be null" ) ;
         rc = _loadObj( column,
                        e.embeddedObject(),
                        actionNum,
                        strictDataMode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to load element[%s]"
                    " as a obj, rc:%d",
                    e.toString( FALSE, TRUE ).c_str(), rc ) ;
            goto error ;
         }

         if ( 0 == actionNum )
         {
            rc = _loadDefaultValue( e ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to insert element[%s] "
                       " as a default value, rc:%d",
                       e.toString( FALSE, TRUE ).c_str(), rc ) ;
               goto error ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__LOAD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__LOADOBJ, "_mthSColumnMatrix::_loadObj" )
   INT32 _mthSColumnMatrix::_loadObj( _mthSColumn *column,
                                      const bson::BSONObj &obj,
                                      UINT32 &actionNum,
                                      BOOLEAN strictDataMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__LOADOBJ ) ;

      _mthSAction *action = NULL ;
      const _mthSActionParser *parser = _mthSActionParser::instance() ;
      BSONObjIterator i( obj ) ;

      if ( NULL == parser )
      {
         PD_LOG( PDERROR, "failed to get mthSActionParser::instance" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      while ( i.more() )
      {
         if ( NULL == action )
         {
            rc = _actionPool.allocate( action ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to allocate action:%d", rc ) ;
               goto error ;
            }
         }

         action->clear() ;

         rc = parser->parse( i.next(), *action ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get action:%d", rc ) ;
            goto error ;
         }

         if ( action->empty() )
         {
            if ( 0 != actionNum )
            {
               PD_LOG( PDERROR, "can not have a mix of dollar and non-dollar key" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            continue ;
         }

         action->setStrictDataMode( strictDataMode ) ;
         rc = column->addAction( action ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to insert action to column:%d", rc ) ;
            goto error ;
         }

         action = NULL ;
         ++actionNum ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__LOADOBJ, rc ) ;
      return rc ;
   error:
      actionNum = 0 ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__LOADDEFAULTVALUE, "_mthSColumnMatrix::_loadDefaultValue" )
   INT32 _mthSColumnMatrix::_loadDefaultValue( const BSONElement &e )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__LOADDEFAULTVALUE ) ;
      mthSAction *action = NULL ;
      mthSColumn *column = NULL ;
      const _mthSActionParser *parser = _mthSActionParser::instance() ;

      if ( NULL == parser )
      {
         PD_LOG( PDERROR, "failed to get mthSActionParser::instance" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _actionPool.allocate( action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to allocate action:%d", rc ) ;
         goto error ;
      }

      rc = _getColumn( e.fieldName(), column ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get column[%s], rc:%d",
                 e.fieldName(), rc ) ;
         goto error ;
      }

      rc = parser->buildDefaultValueAction( e, *action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build default value action:%d", rc ) ;
         goto error ;
      }

      rc = column->addAction( action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add action to column:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__LOADDEFAULTVALUE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__GETCOLUMN, "_mthSColumnMatrix::_getColumn" )
   INT32 _mthSColumnMatrix::_getColumn( const CHAR *fieldName,
                                       _mthSColumn *&column )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__GETCOLUMN ) ;
      SDB_ASSERT( NULL != fieldName, "can not be null" ) ;
      _mthSColumn *father = this ;
      _mthSColumn *node = NULL ;
      INT32 eleNumber = 0 ;

      /// WARNING: fieldName may be changed when get next.
      /// do not use it again until i is destroyed.
      utilSplitIterator i( ( CHAR * )fieldName ) ;
      while ( i.more() )
      {
         node = NULL ;
         const CHAR *columnName = i.next() ;

         /// a.$[0].b
         if ( '$' == *columnName &&
              NULL != father &&
              SDB_OK == mthConvertSubElemToNumeric( columnName,
                                                    eleNumber ) )
         {
            rc = _addMiddleAction( father, eleNumber ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add action to column:%d", rc ) ;
               goto error ;
            }

            node = father ;
         }
         else
         {
            rc = _getColumn( columnName, father, node ) ;
            if( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get column:%d", rc ) ;
               goto error ;
            }
            SDB_ASSERT( NULL != node, "can not be null" ) ;
            father = node ;
         }
      }
      i.finish() ;

      if ( NULL != node )
      {
         column = node ;
         goto done ;
      }
      if ( '\0' == *fieldName )
      {
         rc = _getColumn( fieldName, this, node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get column of an empty fieldName:%d", rc ) ;
            goto error ;
         }
         column = node ;
      }
      else
      {
         PD_LOG( PDERROR, "unexpected error happended" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__GETCOLUMN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__GETCOLUMN2, "_mthSColumnMatrix::_getColumn" )
   INT32 _mthSColumnMatrix::_getColumn( const CHAR *name,
                                        _mthSColumn *father,
                                        _mthSColumn *&column )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__GETCOLUMN2 ) ;
      MTH_S_COLUMNS &array = father->_getSubColumns() ;

      if ( !array.empty() )
      {
         _mthSColumn *lastColumn = array[array.size() - 1] ;
         if ( 0 == ossStrcmp( name, lastColumn->getName() ) )
         {
            column = lastColumn ;       
         }
      }

      if ( NULL == column )
      {
         rc = _columnPool.allocate( column ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to allocate column:%d", rc ) ;
            goto error ;
         }

         rc = column->init( name ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to init column:%d", rc ) ;
            goto error ;
         }

         rc = array.append( column ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append column:%d", rc ) ;
            goto error ;
         }

         column->_getFather() = father ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__GETCOLUMN2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMNMATRIX__ADDMIDDLEACTION, "_mthSColumnMatrix::_addMiddleAction" )
   INT32 _mthSColumnMatrix::_addMiddleAction( _mthSColumn *column,
                                              INT32 numberic )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMNMATRIX__ADDMIDDLEACTION ) ;
      mthSAction *action = NULL ;
      const _mthSActionParser *parser = _mthSActionParser::instance() ;

      if ( NULL == parser )
      {
         PD_LOG( PDERROR, "failed to get mthSActionParser::instance" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _actionPool.allocate( action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to allocate action:%d", rc ) ;
         goto error ;
      }

      rc = parser->buildSliceAction( numberic, 1, *action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build default value action:%d", rc ) ;
         goto error ;
      }

      rc = column->addAction( action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add action to column:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMNMATRIX__ADDMIDDLEACTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

