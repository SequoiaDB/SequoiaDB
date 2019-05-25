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

   Source File Name = dmsCachedPlanUnit.hpp

   Descriptive Name = DMS Cached Access Plan Units Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   management of cached access plans of collections.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSCACHEDPLANUNIT_HPP__
#define DMSCACHEDPLANUNIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "utilList.hpp"
#include "utilSUCache.hpp"
#include "utilHashTable.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   #define DMS_PARAM_INVALID_THRESHOLD ( 5 )

   /*
      _dmsCLCachedPlanUnit define
    */
   class _dmsCLCachedPlanUnit : public _utilSUCacheUnit
   {
      public :
         _dmsCLCachedPlanUnit () ;

         _dmsCLCachedPlanUnit ( UINT16 mbID, UINT64 createTime ) ;

         virtual ~_dmsCLCachedPlanUnit () ;

         virtual UINT8 getUnitType () const
         {
            return UTIL_SU_CACHE_UNIT_CLPLAN ;
         }

         OSS_INLINE virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                                 BOOLEAN ignoreCrtTime )
         {
            return FALSE ;
         }

         OSS_INLINE virtual void clearSubUnits () {}

         OSS_INLINE void incParamInvalid ()
         {
            _paramInvalidCount ++ ;
         }

         OSS_INLINE BOOLEAN isParamInvalid () const
         {
            return _paramInvalidCount > DMS_PARAM_INVALID_THRESHOLD ;
         }

         OSS_INLINE void incMainCLInvalid ()
         {
            _mainCLInvalidCount ++ ;
         }

         OSS_INLINE void setMainCLInvalid ()
         {
            _mainCLInvalidCount = DMS_PARAM_INVALID_THRESHOLD + 1 ;
         }

         OSS_INLINE BOOLEAN isMainCLInvalid ()
         {
            return _mainCLInvalidCount > DMS_PARAM_INVALID_THRESHOLD ;
         }

      protected :
         UINT8    _paramInvalidCount ;
         UINT8    _mainCLInvalidCount ;
         CHAR     _mainCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
   } ;

   typedef class _dmsCLCachedPlanUnit dmsCLCachedPlanUnit ;

}

#endif //DMSCACHEDPLANUNIT_HPP__

