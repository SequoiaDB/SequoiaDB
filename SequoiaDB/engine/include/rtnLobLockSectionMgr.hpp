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

   Source File Name = rtnLobLockSectionMgr.hpp

   Descriptive Name = lob's lock section management

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/07/2019  LinYouBin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LOB_LOCK_SECTIONMGR_HPP_
#define UTIL_LOB_LOCK_SECTIONMGR_HPP_

#include "utilSectionMgr.hpp"
#include "../bson/bson.h"

namespace engine
{
   enum LOB_LOCK_MODE
   {
      LOB_LOCK_MODE_SHARE   = 1,
      LOB_LOCK_MODE_EXCLUDE = 2,
   } ;

   typedef ossPoolSet<INT64> OWNER_SET_TYPE ;
   typedef OWNER_SET_TYPE::iterator OWNER_SET_TYPE_ITERATOR ;

   class _rtnLockSection : public SDBObject
   {
   public:
      _rtnLockSection( INT64 begin, INT64 end, LOB_LOCK_MODE mode,
                       INT64 ownerID ) ;

      _rtnLockSection()
      {
         _mode = LOB_LOCK_MODE_SHARE ;
      }

      ~_rtnLockSection() ;

      _rtnLockSection( const _rtnLockSection &right )
      {
         _section = right._section;
         _mode = right._mode ;
         _ownerSet = right._ownerSet ;
      }

      _rtnLockSection& operator = ( const _rtnLockSection &right )
      {
         _section = right._section ;
         _mode = right._mode ;
         _ownerSet = right._ownerSet ;

         return *this ;
      }

   public:
      OSS_INLINE INT64 begin() const ;
      OSS_INLINE INT64 end() const ;
      OSS_INLINE BOOLEAN isShared() const ;
      OSS_INLINE LOB_LOCK_MODE getMode() const ;
      OSS_INLINE BOOLEAN isContain( INT64 ownerID ) const ;

      OSS_INLINE void setBegin( INT64 begin ) ;
      OSS_INLINE void setEnd( INT64 end ) ;

      void addOwnerID( INT64 ownerID ) ;
      OWNER_SET_TYPE *getOwnerSet() ;

      string toString() ;

   private:
      _utilSection _section ;
      LOB_LOCK_MODE _mode ;
      OWNER_SET_TYPE _ownerSet ;
   } ;

   typedef _rtnLockSection rtnLockSection ;

   class _rtnLobLockSectionMgr : public SDBObject
   {
   public:
      typedef ossPoolMap< INT64, _rtnLockSection > LOCKSECTION_MAP_TYPE ;
      typedef LOCKSECTION_MAP_TYPE::iterator LOCKSECTION_MAP_ITERATOR ;

      _rtnLobLockSectionMgr( const bson::OID& oid ) ;
      ~_rtnLobLockSectionMgr() ;

   public:
      INT32 lockSection( INT64 offset, INT64 length, LOB_LOCK_MODE mode,
                         INT64 ownerID ) ;

      void removeSectionByOwnerID( INT64 ownerID ) ;

      BOOLEAN isTotalContain( INT64 offset, INT64 length, LOB_LOCK_MODE mode,
                              INT64 ownerID ) ;

      BOOLEAN isEmpty() const ;

      string toString() ;

      INT32 toBSONBuilder( BSONObjBuilder &builder ) ;

   private:
      BOOLEAN _isSameOwner( OWNER_SET_TYPE *left, OWNER_SET_TYPE *right ) ;

      INT32 _lockSection( INT64 begin, INT64 end, LOB_LOCK_MODE mode,
                          INT64 newOwnerID,
                          LOCKSECTION_MAP_TYPE &addedSectionMap ) ;

      INT32 _lockSectionInsideIter( INT64 newOwnerID,
                                    _rtnLockSection &newSection,
                                    LOCKSECTION_MAP_ITERATOR &iter,
                                    LOCKSECTION_MAP_TYPE &addedSectionMap ) ;

      INT32 _lockSectionOnIterLeft( INT64 newOwnerID,
                                    _rtnLockSection &newSection,
                                    LOCKSECTION_MAP_ITERATOR &iter,
                                    LOCKSECTION_MAP_TYPE &addedSectionMap ) ;

      void _rollbackSections( INT64 ownerID,
                              LOCKSECTION_MAP_TYPE &rollbackMap ) ;

      void _rollbackSection( INT64 ownerID, _rtnLockSection &section ) ;

      void _rollbackSectionInsideIter( INT64 ownerID,
                                       _rtnLockSection &section,
                                       LOCKSECTION_MAP_ITERATOR &iter ) ;

      void _rollbackSectionOnIterLeft( INT64 ownerID,
                                       _rtnLockSection &section,
                                       LOCKSECTION_MAP_ITERATOR &iter ) ;

   private:

      LOCKSECTION_MAP_TYPE _sectionMap ;
      bson::OID _oid ;
   } ;

   typedef _rtnLobLockSectionMgr rtnLobLockSectionMgr ;
}

#endif /* UTIL_LOB_LOCK_SECTIONMGR_HPP_ */


