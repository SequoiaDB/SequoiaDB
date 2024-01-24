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
#include "pd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnLob.hpp"

using namespace bson ;

namespace engine
{
   _rtnLobAccessInfo::_rtnLobAccessInfo( const bson::OID& oid )
   : _metaCache( NULL ), _lockSectionMgr( oid )
   {
      _oid = oid ;
      _refCount = 0 ;
      _createCount = 0 ;
      _rtCount = 0 ;
      _readCount = 0 ;
      _shareReadCount = 0 ;
      _writeCount = 0 ;
   }

   _rtnLobAccessInfo::~_rtnLobAccessInfo()
   {
      SAFE_OSS_DELETE( _metaCache ) ;
   }

   void _rtnLobAccessInfo::setMetaCache( _rtnLobMetaCache* metaCache )
   {
      SDB_ASSERT( NULL == _metaCache, "_metaCache is not null" ) ;

      _metaCache = metaCache ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_LOCKSECTION, "_rtnLobAccessInfo::lockSection" )
   INT32 _rtnLobAccessInfo::lockSection( INT32 mode, INT64 offset, INT64 length,
                                         INT64 contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_LOCKSECTION ) ;
      LOB_LOCK_MODE lockMode ;

      if ( mode == SDB_LOB_MODE_CREATEONLY || mode == SDB_LOB_MODE_REMOVE
           || mode == SDB_LOB_MODE_TRUNCATE || mode == SDB_LOB_MODE_READ )
      {
         // these modes have the whole lob's lock, and not need to hold sections
         // see more detail from _rtnLobAccessManager::_checkCompatible
         goto done ;
      }

      if ( SDB_HAS_LOBWRITE_MODE( mode ) )
      {
         lockMode = LOB_LOCK_MODE_EXCLUDE ;
      }
      else if ( SDB_LOB_MODE_SHAREREAD == mode )
      {
         lockMode = LOB_LOCK_MODE_SHARE ;
      }
      else
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unreconigzed mode[%d]", mode ) ;
         goto error ;
      }

      rc = _lockSectionMgr.lockSection( offset, length, lockMode,
                                        contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock section[begin:%lld, end:%lld"
                   ",lockMode:%d, contextID:%lld]", offset, offset + length,
                   lockMode, contextID ) ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_LOCKSECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYCONTEXTID, "_rtnLobAccessInfo::unlockSectionByContextId" )
   INT32 _rtnLobAccessInfo::unlockSectionByContextId( INT64 contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYCONTEXTID ) ;
      _lockSectionMgr.removeSectionByOwnerID( contextID ) ;

      PD_TRACE_EXITRC( SDB_RTNLOBACCESSINFO_UNLOCKSECTIONBYCONTEXTID, rc ) ;
      return rc ;
   }

   BOOLEAN _rtnLobAccessInfo::isLockSectionEmpty()
   {
      return _lockSectionMgr.isEmpty() ;
   }

   INT32 _rtnLobAccessInfo::toBSONObjBuilder( BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      try
      {
         builder.append( FIELD_NAME_LOB_OID, _oid.toString().c_str() ) ;

         BSONObjBuilder accessBuilder(
                     builder.subobjStart( FIELD_NAME_LOB_ACCESSINFO ) ) ;
         accessBuilder.append( FIELD_NAME_LOB_REFCOUNT, _refCount ) ;
         accessBuilder.append( FIELD_NAME_LOB_READCOUNT, _readCount ) ;
         accessBuilder.append( FIELD_NAME_LOB_WRITECOUNT, _writeCount ) ;
         accessBuilder.append( FIELD_NAME_LOB_SHAREREADCOUNT, _shareReadCount ) ;
         rc = _lockSectionMgr.toBSONBuilder( accessBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build lockSectionMgr:rc=%d",
                      rc ) ;
         accessBuilder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to build _rtnLobAccessInfo, occur unexpected "
                 "error:%s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnLobAccessInfo::addContext( INT64 contextID )
   {
      _contextSet.insert( contextID ) ;
   }

   void _rtnLobAccessInfo::delContext( INT64 contextID )
   {
      _contextSet.erase( contextID ) ;
   }

   UINT64 _rtnLobAccessInfo::getContextCount()
   {
      return _contextSet.size() ;
   }

   INT64 _rtnLobAccessInfo::peekContextID()
   {
      SDB_ASSERT( _contextSet.size() > 0, "should be greater than 0" ) ;
      if ( _contextSet.size() > 0 )
      {
         return *_contextSet.begin() ;
      }
      else
      {
         return -1 ;
      }
   }

   void _rtnLobAccessInfo::incRefCount( INT32 mode )
   {
      ++_refCount ;

      if ( SDB_LOB_MODE_READ == mode )
      {
         ++_readCount ;
      }
      else if ( SDB_HAS_LOBWRITE_MODE( mode ) )
      {
         ++_writeCount ;
      }
      else if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         ++_createCount ;
         SDB_ASSERT( (_refCount == 1 && _createCount == 1), "must be 1" ) ;
      }
      else if ( SDB_LOB_MODE_REMOVE == mode || SDB_LOB_MODE_TRUNCATE == mode )
      {
         ++_rtCount ;
         SDB_ASSERT( (_refCount == 1 && _rtCount == 1), "must be 1" ) ;
      }
      else
      {
         SDB_ASSERT( SDB_LOB_MODE_SHAREREAD == mode, "mode must be shareread") ;
         ++_shareReadCount ;
      }
   }

   BOOLEAN _rtnLobAccessInfo::checkCount()
   {
      if ( _refCount == ( _createCount + _rtCount +  _readCount
                          + _writeCount + _shareReadCount ) )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   void _rtnLobAccessInfo::decRefCount( INT32 mode )
   {
      --_refCount ;
      if ( SDB_LOB_MODE_READ == mode )
      {
         --_readCount ;
      }
      else if ( SDB_HAS_LOBWRITE_MODE( mode ) )
      {
         --_writeCount ;
      }
      else if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         --_createCount ;
         SDB_ASSERT( (_refCount == 0 && _createCount == 0), "must be zero" ) ;
      }
      else if ( SDB_LOB_MODE_REMOVE == mode || SDB_LOB_MODE_TRUNCATE == mode )
      {
         --_rtCount ;
         SDB_ASSERT( (_refCount == 0 && _rtCount == 0), "must be zero" ) ;
      }
      else
      {
         SDB_ASSERT( SDB_LOB_MODE_SHAREREAD == mode, "mode must be shareread") ;
         --_shareReadCount ;
      }
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

   INT32 _rtnLobAccessManager::_checkCompatible( _rtnLobAccessInfo* accessInfo,
                                                 INT32 mode )
   {
      if ( mode == SDB_LOB_MODE_CREATEONLY && accessInfo->_createCount == 0 )
      {
         // create an exist lob
         return SDB_LOB_IS_IN_USE ;
      }

      if ( accessInfo->_writeCount > 0 )
      {
         if ( SDB_HAS_LOBWRITE_MODE( mode ) || mode == SDB_LOB_MODE_SHAREREAD )
         {
            return SDB_OK ;
         }

         return SDB_LOB_IS_IN_USE ;
      }

      if ( accessInfo->_readCount > 0 )
      {
         if ( mode == SDB_LOB_MODE_READ || mode == SDB_LOB_MODE_SHAREREAD )
         {
            return SDB_OK ;
         }

         return SDB_LOB_IS_IN_USE ;
      }

      if ( accessInfo->_shareReadCount > 0 )
      {
         if ( mode == SDB_LOB_MODE_READ || mode == SDB_LOB_MODE_SHAREREAD
              || SDB_HAS_LOBWRITE_MODE( mode ) )
         {
            return SDB_OK ;
         }
      }

      return SDB_LOB_IS_IN_USE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, "_rtnLobAccessManager::getAccessPrivilege" )
   INT32 _rtnLobAccessManager::getAccessPrivilege( const std::string& clName,
                                                const bson::OID& oid,
                                                INT32 mode, INT64 contextID,
                                                _rtnLobAccessInfo** accessInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE ) ;

      SDB_ASSERT( NULL != accessInfo, "AccessInfo can't be NULL" ) ;

      _rtnLobAccessInfo* lobAccessInfo = NULL ;
      _rtnLobAccessKey key( clName, oid ) ;
      RTN_LOB_MAP::Bucket& bucket = _lobMap.getBucket( key ) ;

      BUCKET_XLOCK( bucket ) ;
      RTN_LOB_MAP::map_const_iterator lobIter = bucket.find( key ) ;
      if ( lobIter != bucket.end() )
      {
         lobAccessInfo = lobIter->second ;
         SDB_ASSERT( lobAccessInfo->getRefCount() >= 1,
                     "should be more than 1" ) ;
         SDB_ASSERT( lobAccessInfo->checkCount(), "count should be equal" ) ;
         rc = _checkCompatible( lobAccessInfo, mode ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to check mode[%d]:existContextID=%lld,"
                    "rc=%d", mode, lobAccessInfo->peekContextID(), rc ) ;
            goto error ;
         }
      }
      else
      {
         lobAccessInfo = SDB_OSS_NEW _rtnLobAccessInfo( oid ) ;
         if ( NULL == lobAccessInfo )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc _rtnLobAccessInfo, rc=%d", rc ) ;
            goto error ;
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
      }

      lobAccessInfo->lock() ;
      lobAccessInfo->incRefCount( mode ) ;
      lobAccessInfo->addContext( contextID ) ;
      lobAccessInfo->unlock() ;
      *accessInfo = lobAccessInfo ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_GETACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, "_rtnLobAccessManager::releaseAccessPrivilege" )
   INT32 _rtnLobAccessManager::releaseAccessPrivilege(
                                                     const std::string& clName,
                                                     const bson::OID& oid,
                                                     INT32 mode,
                                                     INT64 contextID )
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

         lobAccessInfo->lock();
         lobAccessInfo->unlockSectionByContextId( contextID ) ;
         lobAccessInfo->decRefCount( mode ) ;
         lobAccessInfo->delContext( contextID ) ;
         if ( lobAccessInfo->getRefCount() <= 0 )
         {
            SDB_ASSERT( lobAccessInfo->checkCount(), "count should be equal" ) ;
            SDB_ASSERT( lobAccessInfo->isLockSectionEmpty(), "should be 0") ;
            SDB_ASSERT( lobAccessInfo->getContextCount() == 0, "should be 0") ;
            bucket.erase( key ) ;
            lobAccessInfo->unlock() ;
            SAFE_OSS_DELETE( lobAccessInfo ) ;
         }
         else
         {
            lobAccessInfo->unlock();
         }
      }

      PD_TRACE_EXITRC( SDB_RTNLOBACCESSMGR_RELEASEACCESSPRIVILEGE, rc ) ;
      return rc ;
   }
}

