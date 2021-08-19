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

   Source File Name = sptProperty.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_PROPERTY_HPP_
#define SPT_PROPERTY_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "sptObjDesc.hpp"
#include "sptSPDef.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "sptScope.hpp"
#include <vector>

namespace engine
{
   typedef void (*SPT_RELEASE_OBJ_FUNC)(void *instance) ;

   #define SPT_MAX_NUMBER_VALUE           ( 9007199254740991LL )
   #define SPT_MIN_NUMBER_VALUE           (-SPT_MAX_NUMBER_VALUE)
   #define SPT_MAX_INT_VALUE              ( 2147483647 )
   #define SPT_MIN_INT_VALUE              (-SPT_MAX_INT_VALUE)

   class _sptProperty ;

   typedef std::vector<_sptProperty*>     SPT_PROP_ARRAY ;
   typedef SPT_PROP_ARRAY                 SPT_SUB_PROPS ;

   /*
      _sptProperty define
   */
   class _sptProperty : public SDBObject
   {
   public:
      _sptProperty() ;
      virtual ~_sptProperty() ;

      void  clear() ;

      void  setValue( INT32 value )
      {
         assignNative( bson::NumberInt, (const void*)&value ) ;
      }
      void  setValue( UINT32 value )
      {
         setValue( (INT32)value ) ;
      }
      void  setValue( FLOAT64 value )
      {
         assignNative( bson::NumberDouble, (const void*)&value ) ;
      }
      void  setValue( INT64 value )
      {
         if ( value < SPT_MIN_NUMBER_VALUE ||
              value > SPT_MAX_NUMBER_VALUE )
         {
            /// to string
            CHAR tmp[ 50 ] = { 0 } ;
            ossSnprintf( tmp, sizeof(tmp)-1, "%lld", value ) ;
            setValue( (std::string)tmp ) ;
         }
         else if ( value < SPT_MIN_INT_VALUE ||
                   value > SPT_MAX_INT_VALUE )
         {
            setValue( (FLOAT64)value ) ;
         }
         else
         {
            setValue( (INT32)value ) ;
         }
      }
      void  setValue( UINT64 value )
      {
         setValue( (INT64)value ) ;
      }
      void  setValue( bool value )
      {
         BOOLEAN valueTmp = value ? TRUE : FALSE ;
         assignNative( bson::Bool, (const void*)&valueTmp ) ;
      }
      void  setValue( const std::string &value )
      {
         assignString( value.c_str() ) ;
      }
      void  setValue( const CHAR *value )
      {
         assignString( value ) ;
      }
      void  setValue( const bson::BSONObj &value )
      {
         assignBsonobj( value ) ;
      }
      void  setValue( const std::vector< bson::BSONObj > &value )
      {
         assignBsonArray( value ) ;
      }
      void  setNull()
      {
         assignNull() ;
      }
      void  setJSCode( const CHAR *codeStr )
      {
         assignJSCode( codeStr ) ;
      }

      template< typename T >
      void  setValue( const std::vector<T> &array )
      {
         _sptProperty *item = NULL ;
         for ( UINT32 i = 0 ; i < array.size() ; ++i )
         {
            item = addArrayItem() ;
            if ( item )
            {
               item->setValue( array[i] ) ;
            }
            else
            {
               break ;
            }
         }
      }

      void setValue( const sptResultVal* value )
      {
         assignResultVal( value ) ;
      }

      void  setName( const std::string &name )
      {
         _name = name ;
      }

   public:
      /// BOOLEAN, INT32, FLOAT64
      INT32 assignNative( bson::BSONType type,
                          const void *value ) ;

      /// value should be base64 coded when
      /// it is a binary data.
      INT32 assignString( const CHAR *value ) ;

      void  assignNull() ;

      INT32 assignBsonobj( const bson::BSONObj &value ) ;

      INT32 assignBsonArray( const std::vector< bson::BSONObj > &vecObj ) ;

      INT32 assignResultVal( const sptResultVal* value ) ;

      INT32 assignJSCode( const CHAR *codeStr  ) ;

      _sptProperty* addArrayItem() ;

      _sptProperty* addSubProp( const std::string &name,
                                UINT32 attr = SPT_PROP_DEFAULT ) ;

      void          addBackwardProp( const std::string &name,
                                     UINT32 attr = SPT_PROP_DEFAULT ) ;

      void  setAttr( UINT32 attr )
      {
         _attr = attr ;
      }
      UINT32 getAttr() const
      {
         return _attr ;
      }
      void  setReadOnly( BOOLEAN readOnly )
      {
         if ( readOnly )
         {
            _attr |= SPT_PROP_READONLY ;
         }
         else
         {
            _attr &= ~SPT_PROP_READONLY ;
         }
      }
      void  setEnumerate( BOOLEAN enumerate )
      {
         if ( enumerate )
         {
            _attr |= SPT_PROP_ENUMERATE ;
         }
         else
         {
            _attr &= ~SPT_PROP_ENUMERATE ;
         }
      }
      void setPermanent( BOOLEAN permanent )
      {
         if ( permanent )
         {
            _attr |= SPT_PROP_PERMANENT ;
         }
         else
         {
            _attr &= ~SPT_PROP_PERMANENT ;
         }
      }
      /*
         The property'attr can't be set with SPT_PROP_PERMANENT
      */
      void  setDelete( BOOLEAN del = TRUE )
      {
         _deleted = del ;
      }
      BOOLEAN isNeedDelete() const
      {
         return _deleted ;
      }

      template< typename T >
      INT32 assignUsrObject( void *value )
      {
         INT32 rc = SDB_OK ;

         clear() ;

         _value = ( UINT64 )value ;
         _type = bson::Object ;
         _desc = &(T::__desc) ;
         _pReleaseFunc = &( T::releaseInstance ) ;

         return rc ;
      }

      INT32 getNative( bson::BSONType type,
                       void *value ) const ;

      /// copy value if u want to modify or keep it.
      const CHAR *getString() const ;

      const CHAR *getJSCodeStr() const ;

      INT32 getResultVal( const sptResultVal ** ppResultVal ) const ;

      inline bson::BSONType getType() const
      {
         return _type ;
      }

      inline BOOLEAN isRawData() const
      {
         return bson::JSTypeMax == _type ? TRUE : FALSE ;
      }

      inline BOOLEAN isObject() const
      {
         return bson::Object == _type ? TRUE : FALSE ;
      }
      inline BOOLEAN isArray() const
      {
         return bson::Array == _type ? TRUE : FALSE ;
      }

      inline void *getValue() const
      {
         return ( void * )_value ;
      }

      const sptObjDesc* getObjDesc() const
      {
         return _desc ;
      }

      inline void takeoverObject()
      {
         if ( isObject() )
         {
            _pReleaseFunc = NULL ;
            _value = 0 ;
            _type = bson::EOO ;
            _name = "" ;
         }
      }

      inline const std::string &getName() const
      {
         return _name ;
      }

      const SPT_PROP_ARRAY& getArray() const
      {
         return _array ;
      }

      const SPT_SUB_PROPS& getSubProps() const
      {
         return _subs ;
      }

      const _sptProperty*  getBackwardProp() const
      {
         return _backwardProp ;
      }

      BOOLEAN hasBackwardProp() const
      {
         return ( _backwardProp && !_backwardProp->getName().empty() ) ?
                TRUE : FALSE ;
      }

   private:
      std::string             _name ;
      UINT64                  _value ;
      bson::BSONType          _type ;
      SPT_RELEASE_OBJ_FUNC    _pReleaseFunc ;
      const _sptObjDesc       *_desc ;
      UINT32                  _attr ;
      BOOLEAN                 _deleted ;

      SPT_PROP_ARRAY          _array ;

      SPT_SUB_PROPS           _subs ;
      _sptProperty            *_backwardProp ;

   private:
      /// Forbidden
      _sptProperty( const _sptProperty &other ) ;
      _sptProperty &operator=(const _sptProperty &other) ;

   } ;
   typedef class _sptProperty sptProperty ;
}

#endif // SPT_PROPERTY_HPP_

