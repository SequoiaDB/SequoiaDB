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
      _backwardProp = NULL ;
   }

   void _sptProperty::clear()
   {
      UINT32 i = 0 ;

      if ( String == _type || Code == _type )
      {
         CHAR *p = ( CHAR * )_value ;
         SDB_OSS_FREE( p ) ;
      }
      else if ( isObject() && 0 != _value && _pReleaseFunc )
      {
         _pReleaseFunc( (void*)_value ) ;
      }

      /// clear the array
      for ( i = 0 ; i < _array.size() ; ++i )
      {
         SDB_OSS_DEL _array[ i ] ;
      }
      _array.clear() ;

      /// clear the subs
      for ( i = 0 ; i < _subs.size() ; ++i )
      {
         SDB_OSS_DEL _subs[ i ] ;
      }
      _subs.clear() ;

      if ( _backwardProp )
      {
         SDB_OSS_DEL _backwardProp ;
         _backwardProp = NULL ;
      }

      if ( isRawData() )
      {
         _sptResultVal *pRVal = (_sptResultVal*)_value ;
         SDB_OSS_DEL pRVal ;
      }

      _value = 0 ;
      _pReleaseFunc = NULL ;
      _desc = NULL ;
      _type = bson::EOO ;
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

   INT32 _sptProperty::assignJSCode( const CHAR *codeStr )
   {
      INT32 rc = SDB_OK ;

      clear() ;

      UINT32 size = ossStrlen( codeStr ) + 3 ;
       /// +3 for '(',')','\0'
      CHAR *p = ( CHAR * )SDB_OSS_MALLOC( size ) ;
      if ( NULL == p )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemset( p, 0, size ) ;
      ossSnprintf( p, size, "(%s)", codeStr ) ;

      _value = (UINT64)p ;
      _type = Code ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptProperty::assignNull()
   {
      clear() ;
      _type = jstNULL ;
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

      sptResultVal *pCopy = value->copy() ;
      if ( pCopy )
      {
         _value = ( UINT64 )pCopy ;
         _type = JSTypeMax ;
         return SDB_OK ;
      }
      else
      {
         return SDB_OOM ;
      }
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
      return ( const CHAR * )_value ;
   }

   const CHAR *_sptProperty::getJSCodeStr() const
   {
      SDB_ASSERT( Code == _type, "type must be code" ) ;
      return ( const CHAR* )_value ;
   }

   INT32 _sptProperty::getResultVal( const sptResultVal ** ppResultVal ) const
   {
      INT32 rc = SDB_OK ;
      if( !isRawData() )
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

   _sptProperty* _sptProperty::addSubProp( const std::string &name,
                                           UINT32 attr )
   {
      _sptProperty *add = SDB_OSS_NEW _sptProperty() ;
      if ( add )
      {
         add->setName( name ) ;
         add->setAttr( attr ) ;
         _subs.push_back( add ) ;
      }
      return add ;
   }

   void _sptProperty::addBackwardProp( const std::string &name,
                                       UINT32 attr )
   {
      _backwardProp = SDB_OSS_NEW _sptProperty() ;
      if ( _backwardProp )
      {
         _backwardProp->setName( name ) ;
         _backwardProp->setAttr( attr ) ;
      }
   }

}

