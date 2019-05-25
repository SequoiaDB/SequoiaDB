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

   Source File Name = utilSUCache.hpp

   Descriptive Name = utility of storage unit cache management header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SU cache
   management ( including plan cache, statistics cache, etc. )

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef UTILSUCACHE_HPP__
#define UTILSUCACHE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"

namespace engine
{

   #define UTIL_SU_CACHE_DFT_SIZE ( 4096 )

   #define UTIL_SU_INVALID_UNITID ( 65535 )

   template < UINT16 CACHESIZE >
   class _utilSUCache ;

   template < UINT16 CACHESIZE >
   class _IUtilSUCacheHolder ;

   #define UTIL_SU_CACHE_UNIT_STATUS_EMPTY   ( 0 )
   #define UTIL_SU_CACHE_UNIT_STATUS_CACHED  ( 1 )

   #define UTIL_SU_CACHE_UNIT_CLSTAT ( 1 )
   #define UTIL_SU_CACHE_UNIT_IXSTAT ( 2 )
   #define UTIL_SU_CACHE_UNIT_CLPLAN ( 3 )

   class _utilSUCacheUnit ;
   typedef class _utilSUCacheUnit utilSUCacheUnit ;

   /*
      _utilSUCacheUnit define
    */
   class _utilSUCacheUnit : public SDBObject
   {
      public :
         _utilSUCacheUnit ()
         {
            _unitID = UTIL_SU_INVALID_UNITID ;
            _createTime = 0 ;
         }

         _utilSUCacheUnit ( UINT16 unitID, UINT64 crtTime )
         {
            _unitID = unitID ;
            _createTime = crtTime ;
         }

         virtual ~_utilSUCacheUnit () {}

         OSS_INLINE UINT16 getUnitID () const
         {
            return _unitID ;
         }

         OSS_INLINE UINT64 getCreateTime () const
         {
            return _createTime ;
         }

         OSS_INLINE void setCreateTime ( UINT64 crtTime )
         {
            _createTime = crtTime ;
         }

         virtual UINT8 getUnitType () const = 0 ;

         virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                      BOOLEAN ignoreCrtTime ) = 0 ;

         virtual void clearSubUnits () = 0 ;

      protected :
         OSS_INLINE void _setUnitID ( UINT16 unitID )
         {
            _unitID = unitID ;
         }

      protected :
         UINT16   _unitID ;

         UINT64   _createTime ;
   } ;

   /*
      _utilSUCache define
    */
   template < UINT16 CACHESIZE = UTIL_SU_CACHE_DFT_SIZE >
   class _utilSUCache : public SDBObject
   {
      public :
         _utilSUCache ( UINT8 type, UINT8 unitType,
                        _IUtilSUCacheHolder<CACHESIZE> *pHolder = NULL )
         {
            _type = type ;
            _unitType = unitType ;
            _pHolder = pHolder ;

            ossMemset( _units, 0, sizeof( _units ) ) ;
            setStatus( UTIL_SU_CACHE_UNIT_STATUS_EMPTY ) ;
         }

         virtual ~_utilSUCache ()
         {
            clearCacheUnits() ;
         }

         OSS_INLINE UINT8 getType () const
         {
            return _type ;
         }

         OSS_INLINE const utilSUCacheUnit *getCacheUnit ( UINT16 unitID ) const
         {
            if ( unitID < CACHESIZE )
            {
               return _units[ unitID ] ;
            }
            return NULL ;
         }

         OSS_INLINE utilSUCacheUnit *getCacheUnit ( UINT16 unitID )
         {
            if ( unitID < CACHESIZE )
            {
               return _units[ unitID ] ;
            }
            return NULL ;
         }

         OSS_INLINE UINT32 getSize () const
         {
            return CACHESIZE ;
         }

         BOOLEAN addCacheUnit ( utilSUCacheUnit *pUnit,
                                BOOLEAN ignoreCrtTime,
                                BOOLEAN needCheck )
         {
            BOOLEAN added = FALSE ;

            UINT16 unitID = UTIL_SU_INVALID_UNITID ;
            utilSUCacheUnit *pTmpUnit = NULL ;

            if ( NULL ==  pUnit ||
                 pUnit->getUnitType() != _unitType )
            {
               goto done ;
            }

            if ( needCheck && NULL != _pHolder &&
                 !_pHolder->checkCacheUnit( pUnit ) )
            {
               goto done ;
            }

            unitID = pUnit->getUnitID() ;
            pTmpUnit = getCacheUnit( unitID ) ;

            if ( NULL != pTmpUnit )
            {
               if ( ignoreCrtTime ||
                    pTmpUnit->getCreateTime() < pUnit->getCreateTime() )
               {
                  SDB_OSS_DEL pTmpUnit ;
                  _units[ unitID ] = pUnit ;
                  _unitStatus[ unitID ] = UTIL_SU_CACHE_UNIT_STATUS_CACHED ;
                  added = TRUE ;
               }
            }
            else if ( unitID < CACHESIZE )
            {
               _units[ unitID ] = pUnit ;
               _unitStatus[ unitID ] = UTIL_SU_CACHE_UNIT_STATUS_CACHED ;
               added = TRUE ;
            }

         done :
            return added ;
         }

         BOOLEAN addCacheSubUnit ( utilSUCacheUnit *pSubUnit,
                                   BOOLEAN ignoreCrtTime,
                                   BOOLEAN needCheck )
         {
            BOOLEAN added = FALSE ;
            utilSUCacheUnit *pUnit = NULL ;

            if ( NULL ==  pSubUnit )
            {
               goto done ;
            }

            if ( needCheck && NULL != _pHolder &&
                 !_pHolder->checkCacheUnit( pSubUnit ) )
            {
               goto done ;
            }

            pUnit = getCacheUnit( pSubUnit->getUnitID() ) ;
            if ( NULL != pUnit )
            {
               added = pUnit->addSubUnit( pSubUnit, ignoreCrtTime ) ;
            }

         done :
            return added ;
         }

         BOOLEAN removeCacheUnit ( UINT16 unitID, BOOLEAN needDelete )
         {
            BOOLEAN deleted = FALSE ;

            if ( unitID < CACHESIZE )
            {
               utilSUCacheUnit *pTmpUnit = _units[ unitID ] ;
               if ( pTmpUnit )
               {
                  if ( needDelete )
                  {
                     SDB_OSS_DEL pTmpUnit ;
                  }
                  _units[ unitID ] = NULL ;
                  deleted = TRUE ;
               }
               _unitStatus[ unitID ] = UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
            }

            return deleted ;
         }

         BOOLEAN clearCacheUnits ()
         {
            BOOLEAN deleted = FALSE ;

            for ( UINT16 unitID = 0 ; unitID < CACHESIZE ; unitID ++ )
            {
               utilSUCacheUnit *pTmpUnit = _units[ unitID ] ;
               if ( pTmpUnit )
               {
                  SDB_OSS_DEL pTmpUnit ;
                  _units[ unitID ] = NULL ;
                  deleted = TRUE ;
               }
               _unitStatus[ unitID ] = UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
            }

            return deleted ;
         }

         UINT8 getStatus ( UINT16 unitID ) const
         {
            if ( unitID < CACHESIZE )
            {
               return _unitStatus[ unitID ] ;
            }
            return UTIL_SU_CACHE_UNIT_STATUS_EMPTY ;
         }

         void setStatus ( UINT16 unitID, UINT8 status )
         {
            if ( unitID < CACHESIZE )
            {
               _unitStatus[ unitID ] = status ;
            }
         }

         void setStatus ( UINT8 status )
         {
            for ( UINT16 unitID = 0 ; unitID < CACHESIZE ; unitID ++ )
            {
               _unitStatus[ unitID ] = status ;
            }
         }

      protected :
         UINT8                            _type ;
         UINT8                            _unitType ;
         utilSUCacheUnit *                _units[ CACHESIZE ] ;
         UINT8                            _unitStatus[ CACHESIZE ] ;
         _IUtilSUCacheHolder<CACHESIZE> * _pHolder ;
   } ;

   /*
      _IUtilSUCacheHolder define
    */
   template < UINT16 CACHESIZE = UTIL_SU_CACHE_DFT_SIZE >
   class _IUtilSUCacheHolder
   {
      public :
         _IUtilSUCacheHolder () {}

         virtual ~_IUtilSUCacheHolder () {}

         virtual const CHAR *getCSName () const = 0 ;

         virtual UINT32 getSUID () const = 0 ;

         virtual UINT32 getSULID () const = 0 ;

         virtual BOOLEAN isSysSU () const = 0 ;

         virtual BOOLEAN checkCacheUnit ( utilSUCacheUnit *pCacheUnit ) = 0 ;

         virtual BOOLEAN createSUCache ( UINT8 type ) = 0 ;

         virtual BOOLEAN deleteSUCache ( UINT8 type ) = 0 ;

         virtual void deleteAllSUCaches () = 0 ;

         virtual _utilSUCache<CACHESIZE> *getSUCache( UINT8 type ) = 0 ;
   } ;

}

#endif //UTILSUCACHE_HPP__

