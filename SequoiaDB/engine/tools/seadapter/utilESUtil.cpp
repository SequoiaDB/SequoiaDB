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

   Source File Name = utilESUtil.cpp

   Descriptive Name = Elasticsearch utility.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/03/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "ossUtil.hpp"
#include "utilESUtil.hpp"
#include "../../util/hex.h"
#include <sstream>

using bson::BSONObjBuilder ;

#define UTIL_ESID_ENCODE_PREFIX     'x'

namespace seadapter
{
   static const CHAR* getTypeStr( ES_DATA_TYPE type )
   {
      switch ( type )
      {
         case ES_TEXT:
            return "text" ;
         case ES_KEYWORD:
            return "keyword" ;
         case ES_DATE:
            return "date" ;
         case ES_LONG:
            return "long" ;
         case ES_DOUBLE:
            return "double" ;
         case ES_BOOLEAN:
            return "boolean" ;
         case ES_IP:
            return "ip" ;
         case ES_OBJECT:
            return "object" ;
         case ES_NESTED:
            return "nested" ;
         case ES_GEO_POINT:
            return "geo_point" ;
         case ES_GEO_SHAPE:
            return "geo_shape" ;
         case ES_COMPLETION:
            return "completion" ;
         default:
            return NULL ;
      } ;
   }

   _utilESMapProp::_utilESMapProp( const CHAR *name, ES_DATA_TYPE type )
   {
      SDB_ASSERT( name, "Name is NULL" ) ;
      _name = string( name ) ;
      _type = type ;
   }

   _utilESMapProp::~_utilESMapProp()
   {
   }

   _utilESMapping::_utilESMapping( const CHAR *index, const CHAR *type )
   {
      SDB_ASSERT( index, "Index is NULL" ) ;
      SDB_ASSERT( type, "type is NULL" ) ;

      _index = string( index ) ;
      _type = string( type ) ;
   }

   _utilESMapping::~_utilESMapping()
   {
   }

   INT32 _utilESMapping::addProperty( const CHAR *name, ES_DATA_TYPE type,
                                      BSONObj *parameters )
   {
      INT32 rc = SDB_OK ;

      if ( !name )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _properties.push_back( utilESMapProp( name, type ) ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilESMapping::toObj( BSONObj &mapObj ) const
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjBuilder builder ;

         BSONObjBuilder mapBuilder( builder.subobjStart( "mappings" ) ) ;
         BSONObjBuilder typeBuilder( mapBuilder.subobjStart( _type.c_str() ) ) ;
         BSONObjBuilder propBuilder( typeBuilder.subobjStart( "properties" ) ) ;

         for ( vector<_utilESMapProp>::const_iterator itr = _properties.begin() ;
               itr != _properties.end(); ++itr )
         {
            BSONObjBuilder fieldBuilder(
               propBuilder.subobjStart( itr->getName().c_str() ) ) ;

            /* If the string mapping option is multifields, we'll map it as:
               {
                  "field_name" : {
                     "type" : "text",
                     "fields" : {
                        "keyword" : {
                           "type" : "keyword"
                        }
                     }
                  }

               }
            */
            if ( ES_MULTI_FIELDS == itr->getType() )
            {
               fieldBuilder.append( "type", getTypeStr( ES_TEXT ) ) ;
               BSONObj fieldsObj =
                  BSON( "fields" << BSON ( "keyword" <<
                        BSON( "type" << getTypeStr( ES_KEYWORD ) ) ) ) ;
               fieldBuilder.appendElements( fieldsObj ) ;
            }
            else
            {
               fieldBuilder.append( "type", getTypeStr( itr->getType() ) ) ;
            }
            fieldBuilder.done() ;
         }

         propBuilder.done() ;
         typeBuilder.done() ;
         mapBuilder.done() ;

         mapObj = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // For OID type, the final id is the hex string of the _id value.
   // For other supported types, the format is as follows:
   //        x<type code><value code>
   // 'x' specifies this is an encoded id. The next char is its bson type.
   // After that, it's the raw data of the id value.
   // In case of any error or unsupported type, the id will be an empty string.
   // The caller need to check it.
   void encodeID( const BSONElement &idEle, string &id )
   {
      const CHAR *value = NULL ;
      INT32 valSize = 0 ;
      BOOLEAN idReady = FALSE ;
      BOOLEAN unsupport = FALSE ;

      try
      {
         switch ( idEle.type() )
         {
            case NumberDouble:
            case NumberInt:
            case NumberLong:
            case Object:
            case Bool:
            case Date:
            case Timestamp:
            case NumberDecimal:
               value = idEle.value() ;
               valSize = idEle.valuesize();
               break ;
            case String:
               value = idEle.valuestrsafe() ;
               valSize = idEle.valuestrsize() ;
               break ;
            case jstOID:
               // For oid type, it's already converted to hex. We do not encode
               // again.
               id = idEle.OID().str() ;
               idReady = TRUE ;
               break ;
            default:
               // Return empty string in case of unsupported type.
               id = "" ;
               unsupport = TRUE ;
               PD_LOG( PDWARNING, "Unsupported type[ %d ]", idEle.type() ) ;
               break ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      if ( !idReady && !unsupport )
      {
         CHAR bType = (CHAR)( idEle.type() ) ;
         id = UTIL_ESID_ENCODE_PREFIX +
              engine::toHexLower( (const void *)&bType, 1 )
              + engine::toHexLower( value, valSize ) ;
      }

   done:
      return ;
   error:
      goto done ;
   }

   INT32 decodeID( const CHAR *id, CHAR *raw, UINT32 &len, BSONType &type )
   {
      INT32 rc = SDB_OK ;
      const CHAR *p = NULL ;
      UINT32 idLen = 0 ;

      if ( !id )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "id to decode is empty" ) ;
         goto error ;
      }

      idLen = ossStrlen( id ) ;
      p = id ;
      if ( UTIL_ESID_ENCODE_PREFIX != *p )
      {
         // For compatibility with elder version(3.0).
         bson::OID oid ;
         SDB_ASSERT( 24 == idLen, "id size is not 24" ) ;
         if ( len <= sizeof( bson::OID ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[ %u ] is too small", len ) ;
            goto error ;
         }

         {
            type = bson::jstOID ;
            const string idStr( id ) ;
            bson::OID oid( idStr ) ;
            len = sizeof( bson::OID ) ;
            ossMemcpy( raw, (CHAR *)&oid, len ) ;
         }
      }
      else
      {
         CHAR bType = 0 ;
         UINT32 targetLen = 0 ;

         p++ ;
         bType = engine::fromHex( p ) ;
         type = (BSONType)bType ;
         p += 2 ;

         targetLen = ( idLen - 3 ) / 2 ;
         if ( len <= targetLen )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Buffer size[ %u ] is too small", len ) ;
            goto error ;
         }

         for ( UINT32 i = 0; i < targetLen; ++i )
         {
            raw[ i ] = engine::fromHex( p ) ;
            p += 2 ;
         }

         raw[ targetLen ] = '\0' ;
         len = targetLen ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

