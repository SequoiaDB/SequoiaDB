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

   Source File Name = mthSColumn.cpp

   Descriptive Name = mth selector column

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthSColumn.hpp"
#include "pd.hpp"
#include "mthTrace.hpp"
#include "pdTrace.hpp"
#include "mthDef.hpp"

using namespace bson ;


namespace engine
{
   #define MTH_S_IS_LAST_ACTION( i )\
         ( _getSubColumns().empty() && (_actions.size() - 1 == i) )

   _mthSColumn::_mthSColumn()
   :_father( NULL ),
    _name( _staticName ),
    _dynamicName( NULL )
   {
      ossMemset( _staticName, '\0', MTH_SCOLUMN_STATIC_NAME_BUF_LEN ) ;
   }

   _mthSColumn::~_mthSColumn()
   {
      clear() ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN_INIT, "_mthSColumn::init" )
   INT32 _mthSColumn::init( const CHAR *name )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN_INIT ) ;
      SDB_ASSERT( NULL != name, "can not be null" ) ;
      UINT32 len = ossStrlen( name ) + 1 ; /// +1 for '\0'
      if ( len < MTH_SCOLUMN_STATIC_NAME_BUF_LEN )
      {
         ossStrcpy( _staticName, name ) ;
      }
      else
      {
         _dynamicName = ( CHAR * )SDB_OSS_MALLOC( len ) ;
         if ( NULL == _dynamicName )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         ossStrcpy( _dynamicName, name ) ;
         _name = _dynamicName ;
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN_ADDACTION, "_mthSColumn::addAction" )
   INT32 _mthSColumn::addAction( _mthSAction *action )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN_ADDACTION ) ;
      SDB_ASSERT( NULL != action, "can not be null" ) ;
      SDB_ASSERT( MTH_ATTR_IS_VALID( action->getAttribute() ), "must be valid" ) ;

      if ( action->empty() )
      {
         goto done ;
      }

      rc = _setAttribute( action->getAttribute() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to set attribute:%d", rc ) ;
         goto error ;
      }

      rc = _actions.append( action ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to add action:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN_ADDACTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN_CLEAR, "_mthSColumn::clear" )
   void _mthSColumn::clear()
   {
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN_CLEAR ) ;

      /// columns and actions will be freed by their pools.
      MTH_S_COLUMNS::iterator i( _subColumns ) ;
      while ( i.more() )
      {
         _mthSColumn *column = NULL ;
         i.next( column ) ;
         SDB_ASSERT( NULL != column, "can not be null" ) ;
         column->clear() ;
      }

      _actions.clear() ;
      SAFE_OSS_FREE( _dynamicName ) ;
      ossMemset( _staticName, '\0', MTH_SCOLUMN_STATIC_NAME_BUF_LEN ) ;
      _name = _staticName ;
      _attribute.clear() ;
      _father = NULL ;
      _subColumns.clear() ;
      PD_TRACE_EXIT( SDB__MTHSCOLUMN_CLEAR ) ;
      return ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN_BUILD, "_mthSColumn::build" )
   INT32 _mthSColumn::build( const bson::BSONElement &e,
                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN_BUILD ) ;
      SDB_ASSERT( _attribute.isValid(),
                  "can not be invaild" ) ;

      rc = _build( e, builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build column:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN_BUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__BUILD, "_mthSColumn::_build" )
   INT32 _mthSColumn::_build( const bson::BSONElement &e,
                              bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__BUILD ) ;
      bson::BSONElement input = e ;
      bson::BSONElement output ;
      UINT32 size = _actions.size() ;

      for ( UINT32 i = 0; i < size ; ++i )
      {
         if ( MTH_S_IS_LAST_ACTION( i ) )
         {
            rc = _actions[i]->build( _name, input, builder ) ;
            if ( SDB_OK == rc )
            {
               goto done ;
            }
            else
            {
               PD_LOG( PDERROR, "failed to build column:%d", rc ) ;
               goto error ;
            }

         }
         else
         {
            rc = _actions[i]->get( _name, input, output ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get column:%d", rc ) ;
               goto error ;
            }
            input = output ;
         }

      }

      if ( !_subColumns.empty() )
      {
         rc = _buildFromChildren( input, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build columns from children:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__BUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__BUILDFROMCHILDREN, "_mthSColumn::_buildFromChildren" )
   INT32 _mthSColumn::_buildFromChildren( const bson::BSONElement &e,
                                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__BUILDFROMCHILDREN ) ;

      if ( Object == e.type() )
      {
         BSONObjBuilder sub( builder.subobjStart( e.fieldName() ) ) ;
         rc = _buildObjFromChildren( e.embeddedObject(), sub ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
            goto error ;
         }

         sub.doneFast() ;
      }
      else if ( Array == e.type() )
      {
         BSONArrayBuilder sub( builder.subarrayStart( e.fieldName() ) ) ;
         BSONObjIterator i( e.embeddedObject() ) ;
         while ( i.more() )
         {
            rc = _buildFromChildren( i.next(), sub ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
               goto error ;
            }
         }

         sub.doneFast() ;
      }
      else if ( !e.eoo() && !_attribute.isInclude() )
      {
         builder.append( e ) ;
      }
      else if ( e.eoo() && _attribute.isDefault() )
      {
         BSONObjBuilder sub( builder.subobjStart( _name ) ) ;
         rc = _buildObjFromChildren( BSONObj(), sub ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
            goto error ;
         }

         sub.doneFast() ;
      }
      else
      {
         /// do noting.
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__BUILDFROMCHILDREN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__BUILDFROMCHILDREN2, "_mthSColumn::_buildFromChildren" )
   INT32 _mthSColumn::_buildFromChildren( const bson::BSONElement &e,
                                          bson::BSONArrayBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__BUILDFROMCHILDREN2 ) ;
      if ( Object == e.type() )
      {
         BSONObjBuilder sub( builder.subobjStart() ) ;
         rc = _buildObjFromChildren( e.embeddedObject(), sub ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
            goto error ;
         }

         sub.doneFast() ;
      }
      else if ( Array == e.type() )
      {
         BSONArrayBuilder sub( builder.subarrayStart() ) ;
         BSONObjIterator i( e.embeddedObject() ) ;
         while ( i.more() )
         {
            rc = _buildFromChildren( i.next(), sub ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
               goto error ;
            }
         }

         sub.doneFast() ;
      }
      else if ( !e.eoo() && !_attribute.isInclude() )
      {
         builder.append( e ) ;
      }
      else if ( e.eoo() && _attribute.isDefault() )
      {
         BSONObjBuilder sub( builder.subobjStart() ) ;
         rc = _buildObjFromChildren( BSONObj(), sub ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build column from children:%d", rc ) ;
            goto error ;
         }

         sub.doneFast() ;
      }
      else
      {
         /// do nothing.
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__BUILDFROMCHILDREN2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__BUILDOBJFROMCHILDREN, "_mthSColumn::_buildObjFromChildren" )
   INT32 _mthSColumn::_buildObjFromChildren( const bson::BSONObj &obj,
                                             bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__BUILDOBJFROMCHILDREN ) ;
      UINT32 found = 0 ;
      MTH_S_COLUMNS array ;
      UINT32 number = 0 ;
      BOOLEAN addOtherChild = ( _actions.size() > 0 ) ? TRUE : FALSE ;

      rc = _subColumns.copyTo( array ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to copy array:%d", rc ) ;
         goto error ;
      }

      {
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next() ;
            mthSColumn *column = NULL ;

            if ( _findColumn( e.fieldName(),
                              _subColumns,
                              column,
                              &number ) )
            {
               rc = column->build( e, builder ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to build column from obj:%d", rc ) ;
                  goto error ;
               }
               if ( array[number] )
               {
                  ++found ;
                  array[number] = NULL ;
               }
            }
            else if ( !_attribute.isInclude() )
            {
               builder.append( e ) ;
            }
            else if ( addOtherChild )
            {
               // If the field has action, we should also show its other children
               // eg: selector is {a:null,'a.b':{$add:10}}, record is {a:{b:1,c:1}
               //     result is {a:{b:11,c:1}, instead of {a:{b:11}
               builder.append( e ) ;
            }
            else if ( found >= array.size() )
            {
               break ;
            }
         }
      }

      if ( found < array.size() )
      {
         rc = _buildLastChildren( array, builder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build other colums:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__BUILDOBJFROMCHILDREN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__BUILDLASTCHILDREN, "_mthSColumn::_buildLastChildren" )
   INT32 _mthSColumn::_buildLastChildren( MTH_S_COLUMNS &array,
                                          bson::BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__BUILDLASTCHILDREN ) ;
      BSONElement e ;
      MTH_S_COLUMNS::iterator i( array ) ;
      while ( i.more() )
      {
         _mthSColumn *column = NULL ;
         i.next( column ) ;
         if ( NULL != column )
         {
            rc = column->build( e, builder ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build column from obj:%d", rc ) ;
               goto error ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__BUILDLASTCHILDREN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__FINDCOLUMN, "_mthSColumn::_findColumn" )
   BOOLEAN _mthSColumn::_findColumn( const CHAR *name,
                                     MTH_S_COLUMNS &array,
                                     _mthSColumn *&column,
                                     UINT32 *number )
   {
      BOOLEAN rc = FALSE ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__FINDCOLUMN ) ;
      UINT32 begin = 0 ;
      UINT32 end = 0 ;
      column = NULL ;

      if ( array.empty() )
      {
         goto done ;
      }

      end = array.size() - 1 ;

      while ( begin != end )
      {
         UINT32 mid = ( begin + end ) / 2 ;
         _mthSColumn * node = array[mid] ;
         SDB_ASSERT( NULL != node, "can not be null" ) ;
         FieldCompareResult compare =
                      compareDottedFieldNames( name, node->getName() ) ;
         if ( SAME == compare )
         {
            column = node ;
            rc = TRUE ;
            if ( NULL != number )
            {
               *number = mid ;
            }
            goto done ;
         }
         /// a <-> a.b   ||  a <-> d
         else if ( ( compare == RIGHT_SUBFIELD ) || ( compare == LEFT_BEFORE ) )
         {
            end = mid ;
         }
         else
         {
            begin = mid == begin ?
                    end : mid ;
         }
      }

      {
      _mthSColumn *node = array[begin] ;
      if ( SAME == compareDottedFieldNames( name, node->getName() ) )
      {
         column = node ;
         rc = TRUE ;
         if ( NULL != number )
         {
            *number = begin ;
         }
      }
      }
   done:
      PD_TRACE_EXIT( SDB__MTHSCOLUMN__FINDCOLUMN ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHSCOLUMN__SETATTRIBUTE, "_mthSColumn::_setAttribute" )
   INT32 _mthSColumn::_setAttribute( MTH_S_ATTRIBUTE attribute )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHSCOLUMN__SETATTRIBUTE ) ;
      SDB_ASSERT( MTH_S_ATTR_INCLUDE == attribute ||
                  MTH_S_ATTR_EXCLUDE == attribute ||
                  MTH_S_ATTR_DEFAULT == attribute ||
                  MTH_S_ATTR_PROJECTION == attribute, "can not be any others" ) ;
      rc = _attribute.set( attribute ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL != _father )
      {
         rc = _father->_setAttribute( attribute ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__MTHSCOLUMN__SETATTRIBUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }


}

