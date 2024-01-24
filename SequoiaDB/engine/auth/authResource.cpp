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

   Source File Name = authResource.cpp

   Descriptive Name =

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "authResource.hpp"
#include <boost/make_shared.hpp>

namespace engine
{
   using namespace bson;
   authResource::_authResource()
   {
      _t = RESOURCE_TYPE__INVALID;
   }

   _authResource::_authResource( RESOURCE_TYPE t ) : _t( t ) {}

   authResource::~_authResource() {}

   BOOLEAN authResource::operator==( const authResource &o ) const
   {
      if ( _t != o._t )
      {
         return FALSE;
      }
      if ( _t == RESOURCE_TYPE_COLLECTION_SPACE )
      {
         return _cs == o._cs;
      }
      else if ( _t == RESOURCE_TYPE_EXACT_COLLECTION )
      {
         return _cs == o._cs && _cl == o._cl;
      }
      else if ( _t == RESOURCE_TYPE_COLLECTION_NAME )
      {
         return _cl == o._cl;
      }
      return TRUE;
   }

   bool authResource::operator<( const authResource &o ) const
   {
      if ( _t < o._t )
      {
         return true;
      }
      else if ( _t > o._t )
      {
         return false;
      }
      else
      {
         if ( _t == RESOURCE_TYPE_COLLECTION_SPACE )
         {
            return _cs < o._cs;
         }
         else if ( _t == RESOURCE_TYPE_EXACT_COLLECTION )
         {
            if ( *_cs < *o._cs )
            {
               return true;
            }
            if ( *_cs > *o._cs )
            {
               return false;
            }
            else
            {
               return *_cl < *o._cl;
            }
         }
         else if ( _t == RESOURCE_TYPE_COLLECTION_NAME )
         {
            return _cl < o._cl;
         }
         else
         {
            return false;
         }
      }
   }

   RESOURCE_TYPE authResource::getType() const
   {
      return _t;
   }

   BOOLEAN isSysName( const CHAR *pName )
   {
      if ( pName && ossStrlen( pName ) >= 3 && 'S' == pName[ 0 ] && 'Y' == pName[ 1 ] &&
           'S' == pName[ 2 ] )
      {
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN authResource::isIncluded( const authResource &o ) const
   {
      if ( _t == RESOURCE_TYPE_EXACT_COLLECTION )
      {
         if ( o._t == RESOURCE_TYPE_EXACT_COLLECTION )
         {
            return _cs == o._cs && _cl == o._cl;
         }
         else if ( o._t == RESOURCE_TYPE_COLLECTION_SPACE )
         {
            return _cs == o._cs;
         }
         else if ( o._t == RESOURCE_TYPE_COLLECTION_NAME )
         {
            return _cl == o._cl;
         }
         else if ( o._t == RESOURCE_TYPE_NON_SYSTEM )
         {
            if ( isSysName( _cs->c_str() ) || isSysName( _cl->c_str() ) )
            {
               return FALSE;
            }
            else
            {
               return TRUE;
            }
         }
         else if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else if ( _t == RESOURCE_TYPE_COLLECTION_NAME )
      {
         if ( o._t == RESOURCE_TYPE_COLLECTION_NAME )
         {
            return _cl == o._cl;
         }
         else if ( o._t == RESOURCE_TYPE_NON_SYSTEM )
         {
            if ( isSysName( _cl->c_str() ) )
            {
               return FALSE;
            }
            else
            {
               return TRUE;
            }
         }
         else if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else if ( _t == RESOURCE_TYPE_COLLECTION_SPACE )
      {
         if ( o._t == RESOURCE_TYPE_COLLECTION_SPACE )
         {
            return _cs == o._cs;
         }
         else if ( o._t == RESOURCE_TYPE_NON_SYSTEM )
         {
            if ( isSysName( _cs->c_str() ) )
            {
               return FALSE;
            }
            else
            {
               return TRUE;
            }
         }
         else if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else if ( _t == RESOURCE_TYPE_NON_SYSTEM )
      {
         if ( o._t == RESOURCE_TYPE_NON_SYSTEM )
         {
            return TRUE;
         }
         else if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else if ( _t == RESOURCE_TYPE_CLUSTER )
      {
         if ( o._t == RESOURCE_TYPE_CLUSTER )
         {
            return TRUE;
         }
         if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else if ( _t == RESOURCE_TYPE_ANY )
      {
         if ( o._t == RESOURCE_TYPE_ANY )
         {
            return TRUE;
         }
         else
         {
            return FALSE;
         }
      }
      else
      {
         return FALSE;
      }
   }

   void authResource::toBSONObj( bson::BSONObjBuilder &builder ) const
   {
      if ( _t == RESOURCE_TYPE__INVALID )
      {
         builder.appendBool( AUTH_RESOURCE_INVALID_FIELD_NAME, TRUE );
      }
      else if ( _t == RESOURCE_TYPE_CLUSTER )
      {
         builder.appendBool( AUTH_RESOURCE_CLUSTER_FIELD_NAME, TRUE );
      }
      else if ( _t == RESOURCE_TYPE_COLLECTION_NAME )
      {
         builder.append( AUTH_RESOURCE_CS_FIELD_NAME, "" );
         builder.append( AUTH_RESOURCE_CL_FIELD_NAME, _cl->c_str() );
      }
      else if ( _t == RESOURCE_TYPE_COLLECTION_SPACE )
      {
         builder.append( AUTH_RESOURCE_CS_FIELD_NAME, _cs->c_str() );
         builder.append( AUTH_RESOURCE_CL_FIELD_NAME, "" );
      }
      else if ( _t == RESOURCE_TYPE_EXACT_COLLECTION )
      {
         builder.append( AUTH_RESOURCE_CS_FIELD_NAME, _cs->c_str() );
         builder.append( AUTH_RESOURCE_CL_FIELD_NAME, _cl->c_str() );
      }
      else if ( _t == RESOURCE_TYPE_NON_SYSTEM )
      {
         builder.append( AUTH_RESOURCE_CS_FIELD_NAME, "" );
         builder.append( AUTH_RESOURCE_CL_FIELD_NAME, "" );
      }
      else if ( _t == RESOURCE_TYPE_ANY )
      {
         builder.appendBool( AUTH_RESOURCE_ANY_FIELD_NAME, TRUE );
      }
      else
      {
         SDB_ASSERT( FALSE, "Impossible" );
      }
      builder.doneFast();
   }

   boost::shared_ptr< authResource > authResource::fromBson( const BSONObj &obj )
   {
      boost::shared_ptr< authResource > r = boost::make_shared< authResource >();
      if ( !r )
      {
         return r;
      }
      BSONElement clusterEle = obj.getField( AUTH_RESOURCE_CLUSTER_FIELD_NAME );
      if ( clusterEle.booleanSafe() )
      {
         r->_t = RESOURCE_TYPE_CLUSTER;
         return r;
      }

      BSONElement anyResEle = obj.getField( AUTH_RESOURCE_ANY_FIELD_NAME );
      if ( anyResEle.booleanSafe() )
      {
         r->_t = RESOURCE_TYPE_ANY;
         return r;
      }

      BSONElement csEle = obj.getField( AUTH_RESOURCE_CS_FIELD_NAME );
      BSONElement clEle = obj.getField( AUTH_RESOURCE_CL_FIELD_NAME );
      if ( csEle.type() != bson::String || clEle.type() != bson::String )
      {
         r->_t = RESOURCE_TYPE__INVALID;
         return r;
      }
      // Resource: { cs: "", cl: "" }
      else if ( csEle.valuestrsize() - 1 == 0 && clEle.valuestrsize() - 1 == 0 )
      {
         r->_t = RESOURCE_TYPE_NON_SYSTEM;
         return r;
      }
      // Resource: { cs: "exact", cl: "" }
      else if ( csEle.valuestrsize() - 1 != 0 && clEle.valuestrsize() - 1 == 0 )
      {
         r->_t = RESOURCE_TYPE_COLLECTION_SPACE;
         r->_cs.emplace( csEle.valuestr() );
         return r;
      }
      // Resource: { cs: "", cl: "exact" }
      else if ( csEle.valuestrsize() - 1 == 0 && clEle.valuestrsize() - 1 != 0 )
      {
         r->_t = RESOURCE_TYPE_COLLECTION_NAME;
         r->_cl.emplace( clEle.valuestr() );
         return r;
      }
      // Resource: { cs: "exact", cl: "exact" }
      else
      {
         r->_t = RESOURCE_TYPE_EXACT_COLLECTION;
         r->_cs.emplace( csEle.valuestr() );
         r->_cl.emplace( clEle.valuestr() );
         return r;
      }
   }

   boost::shared_ptr< authResource > authResource::forCluster()
   {
      static boost::shared_ptr< authResource > r(
         SDB_OSS_NEW authResource( RESOURCE_TYPE_CLUSTER ) );
      return r;
   }
   boost::shared_ptr< authResource > authResource::forCS( const ossPoolString &cs )
   {
      SDB_ASSERT( !cs.empty(), "Collection space name can't be empty" );
      boost::shared_ptr< authResource > r = boost::make_shared< authResource >();
      r->_t = RESOURCE_TYPE_COLLECTION_SPACE;
      r->_cs.emplace( cs );
      return r;
   }
   boost::shared_ptr< authResource > authResource::forCL( const ossPoolString &cl )
   {
      SDB_ASSERT( !cl.empty(), "Collection name can't be empty" );
      boost::shared_ptr< authResource > r = boost::make_shared< authResource >();
      r->_t = RESOURCE_TYPE_COLLECTION_NAME;
      r->_cl.emplace( cl );
      return r;
   }
   boost::shared_ptr< authResource > authResource::forExact( const ossPoolString &cs,
                                                             const ossPoolString &cl )
   {
      SDB_ASSERT( !cs.empty(), "Collection space name can't be empty" );
      SDB_ASSERT( !cl.empty(), "Collection name can't be empty" );
      boost::shared_ptr< authResource > r = boost::make_shared< authResource >();
      r->_t = RESOURCE_TYPE_EXACT_COLLECTION;
      r->_cs.emplace( cs );
      r->_cl.emplace( cl );
      return r;
   }

   BOOLEAN authResource::isExactName( const CHAR *clFullName )
   {
      return NULL != ossStrchr( clFullName, '.' );
   }

   boost::shared_ptr< _authResource > authResource::forExact( const CHAR *clFullName )
   {
      SDB_ASSERT( clFullName, "Collection full name can't be NULL" );
      const CHAR *pDot = ossStrchr( clFullName, '.' );
      SDB_ASSERT( pDot, "Invalid collection full name" );
      boost::optional< ossPoolString > cs( ossPoolString( clFullName, pDot - clFullName ) );
      boost::optional< ossPoolString > cl( ossPoolString( pDot + 1 ) );
      boost::shared_ptr< authResource > r = boost::make_shared< authResource >();
      r->_t = RESOURCE_TYPE_EXACT_COLLECTION;
      r->_cs.swap( cs );
      r->_cl.swap( cl );
      return r;
   }
   boost::shared_ptr< authResource > authResource::forNonSystem()
   {
      static boost::shared_ptr< authResource > r(
         SDB_OSS_NEW authResource( RESOURCE_TYPE_NON_SYSTEM ) );
      return r;
   }
   boost::shared_ptr< authResource > authResource::forAny()
   {
      static boost::shared_ptr< authResource > r( SDB_OSS_NEW authResource( RESOURCE_TYPE_ANY ) );
      return r;
   }

   boost::shared_ptr< _authResource > authResource::forSimpleType( RESOURCE_TYPE t )
   {
      if ( t == RESOURCE_TYPE_CLUSTER )
      {
         return forCluster();
      }
      else if ( t == RESOURCE_TYPE_NON_SYSTEM )
      {
         return forNonSystem();
      }
      else if ( t == RESOURCE_TYPE_ANY )
      {
         return forAny();
      }
      else
      {
         SDB_ASSERT( FALSE, "Impossible" );
         return boost::shared_ptr< _authResource >();
      }
   }
} // namespace engine
