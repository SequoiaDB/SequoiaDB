/*******************************************************************************

   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobAccessManager.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobAccessManager.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnLobAccessInfo::_rtnLobAccessInfo( const bson::OID& oid,
                                         UINT32 mode,
                                         INT64 accessId )
   : _mode( mode ),
     _refCount( 0 ),
     _metaCache( NULL ),
     _lockSections( NULL )
   {
      _oid = oid ;
      if ( SDB_LOB_MODE_CREATEONLY == mode ||
           SDB_LOB_MODE_REMOVE == mode ||
           SDB_LOB_MODE_TRUNCATE == mode )
      {
         _accessId = accessId ;
      }
      else
      {
         _accessId = -1 ;
      }
   }

   _rtnLobAccessInfo::~_rtnLobAccessInfo()
   {
      SAFE_OSS_DELETE( _metaCache ) ;
      SAFE_OSS_DELETE( _lockSections ) ;
   }

   void _rtnLobAccessInfo::setMetaCache( _rtnLobMetaCache* metaCache )
   {
      SDB_ASSERT( NULL == _metaCache, "_metaCache is not null" ) ;

      _metaCache = metaCache ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_LOCKSECTION, "_rtnLobAccessInfo::lockSection" )
   INT32 _rtnLobAccessInfo::lockSection( const _rtnLobSection& section )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_LOCKSECTION ) ;

      if ( SDB_LOB_MODE_WRITE != _mode )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Only support in write mode, rc=%d", rc );
         goto error ;
      }

      if ( -1 != _accessId )
      {
         if ( section.accessId != _accessId )
         {
            rc = SDB_LOB_LOCK_CONFLICTED ;
            PD_LOG( PDERROR, "Whole LOB[%s] is already locked by [%lld], rc=%d",
                    _oid.str().c_str(), _accessId, rc ) ;
            goto error ;
         }
         else
         {
            goto done ;
         }
      }

      if ( 0 == section.offset && OSS_SINT64_MAX == section.length )
      {
         if ( NULL != _lockSections &&
              _lockSections->conflicted( section.accessId ) )
         {
            rc = SDB_LOB_LOCK_CONFLICTED ;
            PD_LOG( PDERROR, "Failed to lock whole LOB[%s], rc=%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }

         _accessId = section.accessId ;
      }

      if ( NULL == _lockSections )
      {
         _lockSections = SDB_OSS_NEW _rtnLobSections() ;
         if ( NULL == _lockSections )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create lob sections, rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = _lockSections->addSection( section ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to add section, rc=%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_LOCKSECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID, "_rtnLobAccessInfo::unlockSectionByAccessId" )
   INT32 _rtnLobAccessInfo::unlockSectionByAccessId( INT64 accessId )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID ) ;

      if ( SDB_LOB_MODE_WRITE != _mode )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Only support in write mode, rc=%d", rc );
         goto error ;
      }

      if ( -1 != _accessId && accessId == _accessId )
      {
         _accessId = -1 ;
      }

      if ( NULL == _lockSections )
      {
         goto done ;
      }

      _lockSections->delSectionById( accessId ) ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYACCESSID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnLobAccessManager::_rtnLobAccessManager()
   {
   }

   _rtnLobAccessManager::~_rtnLobAccessManager()
   {
      FOR_EACH_CMAP_BUCKET_X(RTN_LOB_MAP, _lobMap)
      {
         for ( RTN_LOB_MAP::map_const_iterator lobIter = bucket.begin() ;
               lobIter != bucket.end() ;
               lobIter++ )
         {
            _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;
            SDB_OSS_DEL lobAccessInfo ;
         }

         bucket.clear() ;
      }
      FOR_EACH_CMAP_BUCKET_END
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, "_rtnLobAccessManager::getAccessPrivilege" )
   INT32 _rtnLobAccessManager::getAccessPrivilege( const std::string& clName,
                                                   const bson::OID& oid,
                                                   UINT32 mode,
                                                   INT64 accessId,
                                                   _rtnLobAccessInfo** accessInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE ) ;

      if ( !SDB_IS_VALID_LOB_MODE( mode ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid LOB access mode: %u", mode ) ;
         goto error ;
      }

      if ( SDB_LOB_MODE_WRITE == mode && accessId <= -1 )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Invalid LOB accessId[%lld] in write mode",
                 accessId ) ;
         goto error ;
      }

      {
         _rtnLobAccessKey key( clName, oid ) ;

         RTN_LOB_MAP::Bucket& bucket = _lobMap.getBucket( key ) ;
         BUCKET_XLOCK( bucket ) ;

         RTN_LOB_MAP::map_const_iterator lobIter = bucket.find( key ) ;
         if ( lobIter != bucket.end() )
         {
            _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;
            switch ( lobAccessInfo->getMode() )
            {
            case SDB_LOB_MODE_CREATEONLY:
            case SDB_LOB_MODE_REMOVE:
            case SDB_LOB_MODE_TRUNCATE:
               rc = SDB_LOB_IS_IN_USE ;
               goto error ;
            case SDB_LOB_MODE_READ:
               if ( SDB_LOB_MODE_READ != mode )
               {
                  rc = SDB_LOB_IS_IN_USE ;
                  goto error ;
               }
               else
               {
                  lobAccessInfo->lock();
                  lobAccessInfo->incRefCount() ;
                  lobAccessInfo->unlock();
                  break ;
               }
            case SDB_LOB_MODE_WRITE:
               if ( SDB_LOB_MODE_WRITE != mode )
               {
                  rc = SDB_LOB_IS_IN_USE ;
                  goto error ;
               }

               lobAccessInfo->lock();
               lobAccessInfo->incRefCount() ;
               lobAccessInfo->unlock();
               break ;
            default:
               SDB_ASSERT( FALSE, "invalid mode" ) ;
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid LOB access mode: %u",
                       lobAccessInfo->getMode() ) ;
               goto error ;
            }

            if ( NULL != accessInfo )
            {
               *accessInfo = lobAccessInfo ;
            }
         }
         else
         {
            _rtnLobAccessInfo* lobAccessInfo =
               SDB_OSS_NEW _rtnLobAccessInfo( oid, mode, accessId ) ;
            if ( NULL == lobAccessInfo )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Failed to alloc _rtnLobAccessInfo, rc=%d", rc ) ;
               goto error ;
            }

            if ( SDB_LOB_MODE_READ == mode ||
                 SDB_LOB_MODE_WRITE == mode )
            {
               lobAccessInfo->incRefCount() ;
            }

            try
            {
               bucket.insert( RTN_LOB_MAP::value_type( key, lobAccessInfo ) ) ;
            }
            catch ( std::exception& e )
            {
               rc = SDB_SYS ;
               SAFE_OSS_DELETE( lobAccessInfo ) ;
               PD_LOG( PDERROR, "Unexpected error happened: %s", e.what() ) ;
               goto error ;
            }

            if ( NULL != accessInfo )
            {
               *accessInfo = lobAccessInfo ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, "_rtnLobAccessManager::releaseAccessPrivilege" )
   INT32 _rtnLobAccessManager::releaseAccessPrivilege( const std::string& clName,
                                                       const bson::OID& oid,
                                                       UINT32 mode,
                                                       INT64 accessId )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE ) ;
      _rtnLobAccessKey key( clName, oid ) ;

      RTN_LOB_MAP::Bucket& bucket = _lobMap.getBucket( key ) ;
      BUCKET_XLOCK( bucket ) ;

      RTN_LOB_MAP::map_const_iterator lobIter = bucket.find( key ) ;
      if ( lobIter != bucket.end() )
      {
         _rtnLobAccessInfo* lobAccessInfo = lobIter->second ;

         SDB_ASSERT( oid == lobAccessInfo->getOID(), "incorrect oid" ) ;

         if ( mode != lobAccessInfo->getMode() )
         {
            SDB_ASSERT( mode == lobAccessInfo->getMode(), "incorrect mode" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access mode: %u, %u",
                    mode, lobAccessInfo->getMode() ) ;
            goto error ;
         }

         if ( -1 != accessId &&
              -1 != lobAccessInfo->getAccessId() &&
              accessId != lobAccessInfo->getAccessId() )
         {
            SDB_ASSERT( accessId != lobAccessInfo->getAccessId(), 
                        "incorrect accessId" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access id: %lld, %lld",
                    accessId, lobAccessInfo->getAccessId() ) ;
            goto error ;
         }

         switch ( lobAccessInfo->getMode() )
         {
         case SDB_LOB_MODE_CREATEONLY:
         case SDB_LOB_MODE_REMOVE:
         case SDB_LOB_MODE_TRUNCATE:
            bucket.erase( key ) ;
            SAFE_OSS_DELETE( lobAccessInfo ) ;
            break ;
         case SDB_LOB_MODE_READ:
            lobAccessInfo->lock();
            lobAccessInfo->decRefCount() ;
            if ( lobAccessInfo->getRefCount() <= 0 )
            {
               bucket.erase( key ) ;
               lobAccessInfo->unlock();
               SAFE_OSS_DELETE( lobAccessInfo ) ;
            }
            else
            {
               lobAccessInfo->unlock();
            }
            break ;
         case SDB_LOB_MODE_WRITE:
            if ( accessId <= -1 )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Invalid LOB accessId[%lld] in write mode",
                       accessId ) ;
               goto error ;
            }
            lobAccessInfo->lock();
            rc = lobAccessInfo->unlockSectionByAccessId( accessId ) ;
            if ( SDB_OK != rc )
            {
               lobAccessInfo->unlock();
               PD_LOG( PDERROR, "Failed to unlock LOB section by access id: %lld, rc=%d",
                       accessId, rc ) ;
               goto error ;
            }
            lobAccessInfo->decRefCount() ;
            if ( lobAccessInfo->getRefCount() <= 0 )
            {
               bucket.erase( key ) ;
               lobAccessInfo->unlock();
               SAFE_OSS_DELETE( lobAccessInfo ) ;
            }
            else
            {
               lobAccessInfo->unlock();
            }
            break ;
         default:
            SDB_ASSERT( FALSE, "invalid mode" ) ;
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid LOB access mode: %u",
                    lobAccessInfo->getMode() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

