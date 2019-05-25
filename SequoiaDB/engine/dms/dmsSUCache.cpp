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

   Source File Name = dmsSUCache.cpp

   Descriptive Name = DMS Storage Unit Caches

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   management of storage unit caches.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsSUCache.hpp"
#include "dmsCachedPlanUnit.hpp"
#include "optCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsStatCache implement
    */
   _dmsStatCache::_dmsStatCache ( IDmsSUCacheHolder *pHolder )
   : dmsSUCache( DMS_CACHE_TYPE_STAT, UTIL_SU_CACHE_UNIT_CLSTAT, pHolder )
   {
   }

   _dmsStatCache::~_dmsStatCache ()
   {
   }

   /*
      _dmsCachedPlanMgr implement
    */
   _dmsCachedPlanMgr::_dmsCachedPlanMgr ( IDmsSUCacheHolder *pHolder )
   : dmsSUCache( DMS_CACHE_TYPE_PLAN, UTIL_SU_CACHE_UNIT_CLPLAN, pHolder ),
     _cacheBitmap( 0 ),
     _paramInvalidBitmap(),
     _mainCLInvalidBitmap()
   {
      _setBucketModulo() ;
   }

   _dmsCachedPlanMgr::~_dmsCachedPlanMgr ()
   {
   }

   INT32 _dmsCachedPlanMgr::createCLCachedPlanUnit ( UINT16 mbID )
   {
      dmsCLCachedPlanUnit *pCachedPlanUnit =
            SDB_OSS_NEW dmsCLCachedPlanUnit( mbID, 0 ) ;
      if ( NULL != pCachedPlanUnit &&
           !addCacheUnit( pCachedPlanUnit, TRUE, FALSE ) )
      {
         SAFE_OSS_DELETE( pCachedPlanUnit ) ;
      }
      return SDB_OK ;
   }

   INT32 _dmsCachedPlanMgr::resizeBitmaps ( UINT32 bucketNum )
   {
      if ( bucketNum <= OPT_PLAN_MAX_CACHE_BUCKETS )
      {
         _cacheBitmap.resize( bucketNum ) ;
         _setBucketModulo() ;
      }
      return SDB_OK ;
   }

   void _dmsCachedPlanMgr::_setBucketModulo ()
   {
      if ( _cacheBitmap.getSize() > 0 )
      {
         _bucketModulo = _cacheBitmap.getSize() - 1 ;
      }
      else
      {
         _bucketModulo = 0 ;
      }
   }

}
