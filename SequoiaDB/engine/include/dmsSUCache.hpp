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

   Source File Name = dmsSUCache.hpp

   Descriptive Name = Data Management Service SU Cache Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS event handler.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMS_SUCACHE_HPP_
#define DMS_SUCACHE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "utilSUCache.hpp"
#include "utilBitmap.hpp"

namespace engine
{

   #define DMS_CACHE_TYPE_STAT ( 0 )
   #define DMS_CACHE_TYPE_PLAN ( 1 )
   #define DMS_CACHE_TYPE_NUM  ( 2 )

   typedef class _utilSUCache<DMS_MME_SLOTS>          dmsSUCache ;
   typedef class _IUtilSUCacheHolder<DMS_MME_SLOTS>   IDmsSUCacheHolder ;
   typedef class _utilStackBitmap<DMS_MME_SLOTS>      dmsSUCacheBitmap ;

   /*
      _dmsStatCache define
    */
   /// _dmsStatCache stores statistics of collections and indexes
   class _dmsStatCache : public dmsSUCache
   {
      public :
         _dmsStatCache( IDmsSUCacheHolder *pHolder = NULL ) ;
         virtual ~_dmsStatCache () ;
   } ;

   typedef class _dmsStatCache dmsStatCache ;

   /*
      _dmsCachedPlanMgr define
    */
   /// _dmsCachedPlanMgr stores bitmap indexes of cached plans of collections
   class _dmsCachedPlanMgr : public dmsSUCache
   {
      public :
         _dmsCachedPlanMgr ( IDmsSUCacheHolder *pHolder ) ;
         virtual ~_dmsCachedPlanMgr () ;

         OSS_INLINE void setCacheBitmapForPlan( UINT32 hashCode )
         {
            setCacheBitmap( hashCode & _bucketModulo ) ;
         }

         OSS_INLINE void setCacheBitmap ( UINT32 bucketID )
         {
            _cacheBitmap.setBit( bucketID ) ;
         }

         OSS_INLINE void clearCacheBit ( UINT32 bucketID )
         {
            _cacheBitmap.clearBit( bucketID ) ;
         }

         OSS_INLINE BOOLEAN testCacheBitmap ( UINT32 bucketID )
         {
            return _cacheBitmap.testBit( bucketID ) ;
         }

         OSS_INLINE void resetCacheBitmap ()
         {
            _cacheBitmap.resetBitmap() ;
         }

         OSS_INLINE UINT32 getBucketNum () const
         {
            return _cacheBitmap.getSize() ;
         }

         OSS_INLINE BOOLEAN testParamInvalidBitmap ( UINT16 mbID )
         {
            return _paramInvalidBitmap.testBit( mbID ) ;
         }

         OSS_INLINE void clearParamInvalidBit ( UINT16 mbID )
         {
            _paramInvalidBitmap.clearBit( mbID ) ;
         }

         OSS_INLINE void setParamInvalidBit ( UINT16 mbID )
         {
            _paramInvalidBitmap.setBit( mbID ) ;
         }

         OSS_INLINE void resetParamInvalidBitmap ()
         {
            _paramInvalidBitmap.resetBitmap() ;
         }

         OSS_INLINE BOOLEAN testMainCLInvalidBitmap ( UINT16 mbID )
         {
            return _mainCLInvalidBitmap.testBit( mbID ) ;
         }

         OSS_INLINE void clearMainCLInvalidBit ( UINT16 mbID )
         {
            _mainCLInvalidBitmap.clearBit( mbID ) ;
         }

         OSS_INLINE void setMainCLInvalidBit ( UINT16 mbID )
         {
            _mainCLInvalidBitmap.setBit( mbID ) ;
         }

         OSS_INLINE void resetMainCLInvalidBitmap ()
         {
            _mainCLInvalidBitmap.resetBitmap() ;
         }

         INT32 createCLCachedPlanUnit ( UINT16 mbID ) ;

         INT32 resizeBitmaps ( UINT32 bucketNum ) ;

      protected :
         void _setBucketModulo () ;

      protected :
         UINT32 _bucketModulo ;
         utilBitmap _cacheBitmap ;
         dmsSUCacheBitmap _paramInvalidBitmap ;
         dmsSUCacheBitmap _mainCLInvalidBitmap ;
   } ;

   typedef class _dmsCachedPlanMgr dmsCachedPlanMgr ;

}

#endif //DMS_SUCACHE_HPP_
