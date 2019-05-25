/*******************************************************************************


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

   Source File Name = sptProperty.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptProperty.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "sptBsonobj.hpp"
#include "sptBsonobjArray.hpp"

using namespace bson ;

namespace engine
{
   _sptProperty::_sptProperty()
   :_value( 0 ),
    _type( EOO )
   {
      _pReleaseFunc = NULL ;
      _desc = NULL ;
      _attr = SPT_PROP_DEFAULT ;
      _deleted = FALSE ;
      _isRawData = FALSE ;
   }

   void _sptProperty::clear()
   {
      if ( String == _type )
      {
         CHAR *p = ( CHAR * )_value ;
         SDB_OSS_FREE( p ) ;
      }
      else if ( isObject() && 0 != _value && _pReleaseFunc )
      {
         _pReleaseFunc( (void*)_value ) ;
      }

      for ( UINT32 i = 0 ; i < _array.size() ; ++i )
      {
         SDB_OSS_DEL _array[ i ] ;
      }
      _array.clear() ;

      _value = 0 ;
      _pReleaseFunc = NULL ;
      _desc = NULL ;
      _type = bson::EOO ;
	  _isRawData = FALSE ;
   }

   _sptProperty::~_sptProperty()
   {
      clear() ;
   }

   INT32 _sptProperty::assignNative( bson::BSONType type,
                                     const void *value )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NumberDouble == type ||
                  Bool == type ||
                  NumberInt == type, "invalid value type" ) ;
      SDB_ASSERT( NULL != value, "can not be NULL" ) ;

      clear() ;

      if ( NumberDouble == type )
      {
         FLOAT64 *v = (FLOAT64 *)(&_value) ;
         *v = *((const FLOAT64 *)value) ;
      }
      else if ( Bool == type )
      {
         BOOLEAN *v = (BOOLEAN *)(&_value);
         *v = *((const BOOLEAN *)value) ;
      }
      else if ( NumberInt == type )
      {
         INT32 *v = (INT32 *)(&_value) ;
         *v = *((const INT32 *)value) ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _type = type ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptProperty::assignString( const CHAR *value )
   {
      INT32 rc = SDB_OK ;

      clear() ;

      UINT32 size = ossStrlen( value ) ;
      CHAR *p = ( CHAR * )SDB_OSS_MALLOC( size + 1 ) ; /// +1 for \0
      if ( NULL == p )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( p, value, size + 1 ) ;
      _value = (UINT64)p ;
      _type = String ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptProperty::assignBsonobj( const bson::BSONObj &value )
   {
      INT32 rc = SDB_OK ;

      clear() ;

      _sptBsonobj *bs = SDB_OSS_NEW _sptBsonobj( value ) ;
      if ( NULL == bs )
      {
         PD_LOG( PDERROR, "failed to allocate mem.") ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = assignUsrObject<_sptBsonobj>( bs ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptProperty::assignBsonArray( const std::vector < BSONObj > &vecObj )
   {
      INT32 rc = SDB_OK ;
      clear() ;

      _sptBsonobjArray *bsonarray = SDB_OSS_NEW _sptBsonobjArray( vecObj ) ;
      if ( NULL == bsonarray )
      {
         PD_LOG( PDERROR, "failed to allocate mem for bsonarray") ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = assignUsrObject<_sptBsonobjArray>( bsonarray ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptProperty::assignResultVal( const sptResultVal* value )
   {
      clear() ;
      _value = ( UINT64 )value ;
      _isRawData = TRUE ;
      return SDB_OK ;
   }

   INT32 _sptProperty::getNative( bson::BSONType type,
                                  void *value ) const
   {
      SDB_ASSERT( NULL != value, "can not be null" ) ;
      SDB_ASSERT( NumberDouble == type ||
                  Bool == type ||
                  NumberInt == type, "invalid value type" ) ;

      if ( NumberDouble == type )
      {
         FLOAT64 *v = ( FLOAT64 * )value ;
         *v = *(( FLOAT64 *)( &_value )) ;
      }
      else if ( Bool == type )
      {
         BOOLEAN *v = ( BOOLEAN * )value ;
         *v = *(( BOOLEAN *)( &_value )) ;
      }
      else if ( NumberInt == type )
      {
         INT32 *v = ( INT32 * )value ;
         *v = *(( INT32 *)( &_value )) ;
      }

      return SDB_OK ;
   }

   const CHAR *_sptProperty::getString() const
   {
      SDB_ASSERT( String == _type, "type must be string" ) ;
      return ( CHAR * )_value ;
   }

   INT32 _sptProperty::getResultVal( sptResultVal ** ppResultVal ) const
   {
      INT32 rc = SDB_OK ;
      if( FALSE == _isRawData )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *ppResultVal = (sptResultVal*) _value ;
   done:
      return rc ;
   error:
      goto done ;
   }

   _sptProperty* _sptProperty::addArrayItem()
   {
      if ( EOO == _type )
      {
         _type = Array ;
      }
      else if ( Array != _type )
      {
         clear() ;
         _type = Array ;
      }

      _sptProperty *add = SDB_OSS_NEW _sptProperty() ;
      if ( add )
      {
         _array.push_back( add ) ;
      }
      return add ;
   }

}

