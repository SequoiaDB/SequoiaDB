/*******************************************************************************

   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordCacheAssist.cpp

   Descriptive Name = Coord cache assistent

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/04/2021  YSD Init
   Last Changed =

*******************************************************************************/
#include "coordCacheAssist.hpp"
#include "coordCommandWithLocation.hpp"
#include "coordTrace.hpp"

namespace engine
{
   const CHAR *coordCacheTypeStr( coordCacheType type )
   {
      const CHAR *str = NULL ;
      switch ( type )
      {
         case COORD_CACHE_CATALOGUE:
            str = VALUE_NAME_CATALOG ;
            break ;
         case COORD_CACHE_GROUP:
            str = VALUE_NAME_GROUP ;
            break ;
         case COORD_CACHE_DATASOURCE:
            str = VALUE_NAME_DATASOURCE ;
            break ;
         case COORD_CACHE_STRATEGY:
            str = VALUE_NAME_STRATEGY ;
            break ;
         default:
            str = "" ;
            break ;
      }
      return str ;
   }

   coordCacheType coordStr2CacheType( const CHAR *typeStr )
   {
      coordCacheType type = COORD_CACHE_INVALID ;
      if ( 0 == ossStrcasecmp( typeStr, VALUE_NAME_CATALOG ) )
      {
         type = COORD_CACHE_CATALOGUE ;
      }
      else if ( 0 == ossStrcasecmp( typeStr, VALUE_NAME_GROUP ) )
      {
         type = COORD_CACHE_GROUP ;
      }
      else if ( 0 == ossStrcasecmp( typeStr, VALUE_NAME_DATASOURCE ) )
      {
         type = COORD_CACHE_DATASOURCE ;
      }
      else if ( 0 == ossStrcasecmp( typeStr, VALUE_NAME_STRATEGY ) )
      {
         type = COORD_CACHE_STRATEGY ;
      }
      else
      {
         type = COORD_CACHE_INVALID ;
      }
      return type ;
   }

   _coordCacheInvalidator::_coordCacheInvalidator( coordResource *resource )
   : _resource( resource )
   {
   }

   _coordCacheInvalidator::~_coordCacheInvalidator()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR_INVALIDATE, "_coordCacheInvalidator::invalidate" )
   void _coordCacheInvalidator::invalidate( coordCacheType type,
                                            const CHAR *name )
   {
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR_INVALIDATE ) ;
      switch ( type )
      {
      case COORD_CACHE_CATALOGUE:
         _invalidateCatalogue( name ) ;
         break ;
      case COORD_CACHE_GROUP:
         if ( name )
         {
            _resource->removeGroupInfo( name ) ;
         }
         else
         {
            _resource->invalidateGroupInfo() ;
         }
         break ;
      case COORD_CACHE_DATASOURCE:
         _resource->invalidateDataSourceInfo( name ) ;
         break ;
      case COORD_CACHE_STRATEGY:
         _resource->invalidateStrategy() ;
         break ;
      default:
         SDB_ASSERT( FALSE, "Cache type is invalid" ) ;
      }
      PD_TRACE_EXIT( SDB__COORDCACHEINVALIDATOR_INVALIDATE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR_INVALIDATE2, "_coordCacheInvalidator::invalidate" )
   INT32 _coordCacheInvalidator::invalidate( const BSONObj &option )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR_INVALIDATE2 ) ;
      const CHAR *name = NULL ;
      const CHAR *type = NULL ;
      coordCacheType cacheType = COORD_CACHE_INVALID ;
      try
      {
         BSONElement typeEle = option.getField( FIELD_NAME_TYPE ) ;
         BSONElement nameEle = option.getField( FIELD_NAME_NAME ) ;
         if ( typeEle.eoo() && nameEle.eoo() )
         {
            // If no type is given, invalidate all.
            invalidateAll() ;
            goto done ;
         }
         else if ( typeEle.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Name can't be used without type in invalidate "
                    "cache command[%d]", rc ) ;
            goto error ;
         }

         PD_CHECK( String == typeEle.type(), SDB_INVALIDARG, error, PDERROR,
                   "Invalidate cache type should be a string[%d]",
                   SDB_INVALIDARG ) ;
         type = typeEle.valuestr() ;
         cacheType = coordStr2CacheType( type ) ;
         PD_CHECK( COORD_CACHE_INVALID != cacheType, SDB_INVALIDARG, error,
                   PDERROR, "Invalidate cache type[%s] is invalid[%d]",
                   type, SDB_INVALIDARG ) ;
         if ( nameEle.eoo() )
         {
            // Invalidate by type.
            invalidate( cacheType ) ;
         }
         else
         {
            if ( Array == nameEle.type() )
            {
               BSONObjIterator itr( nameEle.embeddedObject() ) ;
               while ( itr.more() )
               {
                  BSONElement subEle = itr.next() ;
                  PD_CHECK( String == subEle.type(), SDB_INVALIDARG, error,
                            PDERROR, "Type of invalidate cache target[%s] is "
                            "not string[%d]", subEle.toString().c_str(),
                            SDB_INVALIDARG ) ;
                  name = subEle.valuestr() ;
                  PD_CHECK( ossStrlen( name ) > 0, SDB_INVALIDARG, error,
                            PDERROR, "Invalidate cache name length is 0[%d]",
                            SDB_INVALIDARG ) ;
                  invalidate( cacheType, name ) ;
               }
            }
            else
            {
               PD_CHECK( String == nameEle.type(), SDB_INVALIDARG, error,
                         PDERROR, "Type of invalidate cache target[%s] is "
                         "not string[%d]", nameEle.toString().c_str(),
                         SDB_INVALIDARG ) ;
               name = nameEle.valuestr() ;
               PD_CHECK( ossStrlen( name ) > 0, SDB_INVALIDARG, error,
                         PDERROR, "Invalidate cache name length is 0[%d]",
                         SDB_INVALIDARG ) ;
               invalidate( cacheType, name ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__COORDCACHEINVALIDATOR_INVALIDATE2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _coordCacheInvalidator::invalidateAll()
   {
      _resource->invalidateCataInfo() ;
      _resource->invalidateGroupInfo() ;
      _resource->invalidateDataSourceInfo() ;
      _resource->invalidateStrategy() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR_NOTIFY, "_coordCacheInvalidator::notify" )
   INT32 _coordCacheInvalidator::notify( coordCacheType type, const CHAR *name,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR_NOTIFY ) ;

      try
      {
         BSONObj arguments ;
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ROLE, "Coord" ) ;
         builder.append( FIELD_NAME_TYPE, coordCacheTypeStr( type ) ) ;

         // If name is specified, invalidate by type and name. Otherwise,
         // invalidate by type.
         if ( name )
         {
            builder.append( FIELD_NAME_NAME, name ) ;
         }
         arguments = builder.done() ;
         rc = _notify( arguments, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Notify other coordinators to invalidate "
                      "cache failed[%d]. Arguments: %s", rc,
                      arguments.toString().c_str() ) ;
         PD_LOG( PDDEBUG, "Notify other coordinators to invalidate cache. "
                 "Arguments: %s", arguments.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e )  ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDCACHEINVALIDATOR_NOTIFY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR_NOTIFY2, "_coordCacheInvalidator::notify" )
   INT32 _coordCacheInvalidator::notify( coordCacheType type,
                                         ossPoolVector<string> &names,
                                         _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR_NOTIFY2 ) ;

      if ( names.empty() )
      {
         goto done ;
      }

      try
      {
         BSONObj arguments ;
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ROLE, "Coord" ) ;
         builder.append( FIELD_NAME_TYPE, coordCacheTypeStr( type ) ) ;
         BSONArrayBuilder subBuilder( builder.subarrayStart( FIELD_NAME_NAME ) ) ;
         for ( ossPoolVector<string>::const_iterator citr = names.begin();
               citr != names.end(); ++citr )
         {
            subBuilder.append( *citr ) ;
         }
         subBuilder.done() ;
         arguments = builder.done() ;
         rc = _notify( arguments, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Notify other coordinators to invalidate "
                      "cache failed[%d]. Arguments: %s", rc,
                      arguments.toString().c_str() ) ;

         PD_LOG( PDDEBUG, "Notify other coordinators to invalidate cache. "
                 "Arguments: %s", arguments.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e )  ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__COORDCACHEINVALIDATOR_NOTIFY2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR__INVALIDATECATALOGUE, "_coordCacheInvalidator::_invalidateCatalogue" )
   void _coordCacheInvalidator::_invalidateCatalogue( const CHAR *name )
   {
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR__INVALIDATECATALOGUE ) ;
      if ( !name )
      {
         // Invalidate only by type. All catalog info will be cleared.
         _resource->invalidateCataInfo() ;
      }
      else if ( NULL == ossStrchr( name, '.' ) )
      {
         // Invalidate cache of collections related to the dropped cs.
         vector< string > subCLSet ;
         _resource->removeCataInfoByCS( name, &subCLSet ) ;

         /// clear relate sub collection's catalog info
         vector< string >::iterator it = subCLSet.begin() ;
         while( it != subCLSet.end() )
         {
            _resource->removeCataInfo( (*it).c_str() ) ;
            ++it ;
         }
      }
      else
      {
         _resource->removeCataInfoWithMain( name ) ;
      }
      PD_TRACE_EXIT( SDB__COORDCACHEINVALIDATOR__INVALIDATECATALOGUE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__COORDCACHEINVALIDATOR__NOTIFY, "_coordCacheInvalidator::_notify" )
   INT32 _coordCacheInvalidator::_notify( const BSONObj &arguments,
                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__COORDCACHEINVALIDATOR__NOTIFY ) ;
      CHAR *msg = NULL ;
      INT32 buffSize = 0 ;
      INT64 contextID = -1 ;
      coordCMDInvalidateCache invalidator ;

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "Notify all coordinators to invalidate cache. "
              "Option: %s", arguments.toString().c_str() ) ;
#endif /* _DEBUG */
      rc = msgBuildInvalidateCacheMsg( &msg, &buffSize, arguments, 0, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Build invalidate data source cache message "
                                "failed[%d]", rc ) ;

      rc = invalidator.init( _resource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Initialize invalidate cache command "
                                "failed[%d]", rc ) ;

      rc = invalidator.execute( (MsgHeader *)msg, cb, contextID, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Execute invalidate cache command "
                   "failed[%d]", rc ) ;

   done:
      if ( msg )
      {
         msgReleaseBuffer( msg, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__COORDCACHEINVALIDATOR__NOTIFY, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
