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

   Source File Name = rtnLobAccessManager.hpp

   Descriptive Name = LOB access manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/28/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_ACCESS_MANAGER_HPP_
#define RTN_LOB_ACCESS_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilConcurrentMap.hpp"
#include "rtnLobMetaCache.hpp"
#include "rtnLobLockSectionMgr.hpp"
#include "../bson/bson.hpp"
#include <string>

namespace engine
{
   #define RTN_LOB_ACCESS_PRIVILEGE_RETRY_TIMES     (3)
   #define RTN_LOB_ACCESS_PRIVILEGE_RETRY_INTERVAL  (50) /* ms */

   typedef ossPoolSet<INT64> RTN_LOB_CONTEXT_SET ;

   class _rtnLobAccessInfo: public SDBObject
   {
   public:
      _rtnLobAccessInfo( const bson::OID& oid ) ;
      ~_rtnLobAccessInfo() ;

   private:
      // disallow copy and assign
      _rtnLobAccessInfo( const _rtnLobAccessInfo& ) ;
      void operator=( const _rtnLobAccessInfo& ) ;

   public:
      OSS_INLINE const bson::OID& getOID() const { return _oid ; }
      OSS_INLINE INT32 getRefCount() const { return _refCount ; }

      OSS_INLINE void lock()
      {
         _lock.get() ;
      }
      OSS_INLINE void unlock()
      {
         _lock.release() ;
      }
      OSS_INLINE _rtnLobMetaCache* getMetaCache()
      {
         return _metaCache ;
      }
      void setMetaCache( _rtnLobMetaCache* metaCache ) ;

      INT32 lockSection( INT32 mode, INT64 offset, INT64 length,
                         INT64 contextID ) ;
      INT32 unlockSectionByContextId( INT64 contextID ) ;
      BOOLEAN isLockSectionEmpty() ;

      INT32 toBSONObjBuilder( BSONObjBuilder &builder ) ;

      void addContext( INT64 contextID ) ;
      void delContext( INT64 contextID ) ;
      UINT64 getContextCount() ;
      INT64 peekContextID() ;

   private:
      OSS_INLINE void incRefCount( INT32 mode ) ;
      OSS_INLINE void decRefCount( INT32 mode ) ;
      BOOLEAN checkCount() ;

   private:
      bson::OID         _oid ;
      ossSpinXLatch     _lock ;

      INT32             _refCount ;

      INT32             _createCount ;
      // remove or truncate count
      INT32             _rtCount ;
      // only for SDB_LOB_MODE_READ
      INT32             _readCount ;
      // only for SDB_LOB_MODE_SHAREREAD
      INT32             _shareReadCount ;
      INT32             _writeCount ;

      _rtnLobMetaCache* _metaCache ;
      _rtnLobLockSectionMgr _lockSectionMgr ;
      RTN_LOB_CONTEXT_SET _contextSet ;

      friend class _rtnLobAccessManager ;
   } ;
   typedef _rtnLobAccessInfo rtnLobAccessInfo ;

   struct _rtnLobAccessKey: public SDBObject
   {
      std::string clName ;
      bson::OID   oid ;

      _rtnLobAccessKey( const std::string& lobCLName, const bson::OID& lobOid )
      {
         clName = lobCLName ;
         oid = lobOid ;
      }

      bool operator<( const _rtnLobAccessKey& other ) const
      {
         INT32 result = clName.compare( other.clName ) ;
         if ( result < 0 )
         {
            return true ;
         }
         else if ( result > 0 )
         {
            return false ;
         }
         else // ( 0 == result )
         {
            return oid < other.oid ;
         }
      }
   } ;

   class _rtnLobAccessKeyHasher
   {
   public:
      _rtnLobAccessKeyHasher() {}
      ~_rtnLobAccessKeyHasher() {}

      UINT32 operator() ( const _rtnLobAccessKey& key ) const
      {
         UINT32 nameHash = ossHash( key.clName.c_str(), (UINT32)key.clName.size() ) ;
         UINT32 oidHash = ossHash( (const CHAR*)&(key.oid), (UINT32)sizeof( bson::OID ) ) ;
         return ( nameHash << 5 ) + oidHash ;
      }
   } ;

   typedef utilConcurrentMap<_rtnLobAccessKey, _rtnLobAccessInfo*, UTIL_CONCURRENT_MAP_DEFAULT_BUCKET_NUM, _rtnLobAccessKeyHasher> RTN_LOB_MAP ;

   class _rtnLobAccessManager: public SDBObject
   {
   public:
      _rtnLobAccessManager() ;
      ~_rtnLobAccessManager() ;

   public:
      INT32 getAccessPrivilege( const std::string& clName, const bson::OID& oid,
                                INT32 mode, INT64 contextID,
                                _rtnLobAccessInfo** accessInfo ) ;
      INT32 releaseAccessPrivilege( const std::string& clName,
                                    const bson::OID& oid, INT32 mode ,
                                    INT64 contextID ) ;

   private:
      INT32 _checkCompatible( _rtnLobAccessInfo* accessInfo, INT32 mode ) ;

   private:
      RTN_LOB_MAP _lobMap ;
   } ;
   typedef _rtnLobAccessManager rtnLobAccessManager ;

}

#endif /* RTN_LOB_ACCESS_MANAGER_HPP_ */

