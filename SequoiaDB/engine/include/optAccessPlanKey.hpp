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

   Source File Name = optAccessPlanKey.hpp

   Descriptive Name = Optimizer Access Plan Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains structure for key of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPTACCESSPLANKEY_HPP__
#define OPTACCESSPLANKEY_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/oid.h"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "utilHashTable.hpp"
#include "rtnQueryOptions.hpp"
#include "dms.hpp"
#include "dmsStorageUnit.hpp"
#include "mthMatchRuntime.hpp"
#include "optAccessPlanHelper.hpp"

using namespace bson ;

namespace engine
{

   /*
      _optCollectionInfo define
    */
   class _optCollectionInfo
   {
      public :
         _optCollectionInfo ()
         : _suID( DMS_INVALID_SUID ),
           _suLID( DMS_INVALID_LOGICCSID ),
           _clLID( DMS_INVALID_CLID ),
           _mbID( DMS_INVALID_MBID )
         {
         }

         _optCollectionInfo ( const _optCollectionInfo & info )
         : _suID( info._suID ),
           _suLID( info._suLID ),
           _clLID( info._clLID ),
           _mbID( info._mbID )
         {
         }

         virtual ~_optCollectionInfo ()
         {
         }

         OSS_INLINE virtual dmsStorageUnitID getSUID () const
         {
            return _suID ;
         }

         OSS_INLINE virtual UINT32 getSULID () const
         {
            return _suLID ;
         }

         OSS_INLINE virtual UINT16 getCLMBID () const
         {
            return _mbID ;
         }

         OSS_INLINE virtual UINT32 getCLLID () const
         {
            return _clLID ;
         }

         OSS_INLINE virtual void setCSInfo ( dmsStorageUnit *su )
         {
            if ( NULL != su )
            {
               _suID = su->CSID() ;
               _suLID = su->LogicalCSID() ;
            }
         }

         OSS_INLINE virtual void setCLInfo ( dmsMBContext *mbContext )
         {
            if ( NULL != mbContext )
            {
               _mbID = mbContext->mbID() ;
               _clLID = mbContext->clLID() ;
            }
         }

      protected :
         dmsStorageUnitID        _suID ;
         UINT32                  _suLID ;
         UINT32                  _clLID ;
         UINT16                  _mbID ;
   } ;

   typedef class _optCollectionInfo optCollectionInfo ;

   /*
      _optAccessPlanKey define
    */
   class _optAccessPlanKey : public _rtnQueryOptions,
                             public _utilHashTableKey,
                             public _optCollectionInfo
   {
      public :
         _optAccessPlanKey ( const rtnQueryOptions &options,
                             OPT_PLAN_CACHE_LEVEL cacheLevel ) ;

         _optAccessPlanKey ( _optAccessPlanKey &planKey ) ;

         virtual ~_optAccessPlanKey () ;

         OSS_INLINE virtual INT32 getOwned ()
         {
            _normalizedQuery = _normalizedQuery.getOwned() ;
            return _rtnQueryOptions::getOwned() ;
         }

         OSS_INLINE virtual UINT32 getKeyCode () const
         {
            return _keyCode ;
         }

         OSS_INLINE void setValid ( BOOLEAN valid )
         {
            _isValid = valid ;
         }

         OSS_INLINE BOOLEAN getValid () const
         {
            return _isValid ;
         }

         virtual BOOLEAN isEqual ( const _optAccessPlanKey &key ) const ;

         OSS_INLINE const BSONObj &getNormalizedQuery () const
         {
            return _normalizedQuery ;
         }

         OSS_INLINE OPT_PLAN_CACHE_LEVEL getCacheLevel () const
         {
            return _cacheLevel ;
         }

         OSS_INLINE BOOLEAN isValid () const
         {
            return _isValid ;
         }

         OSS_INLINE BOOLEAN isSortedIdxRequired () const
         {
            // In below cases, we must use an index which matches order-by
            // 1. order-by is not empty
            // 2. flags is set with FLG_QUERY_MODIFY or
            //    FLG_QUERY_FORCE_IDX_BY_SORT
            return ( !isOrderByEmpty() &&
                     testFlag( FLG_QUERY_MODIFY |
                               FLG_QUERY_FORCE_IDX_BY_SORT ) ) ;
         }

         OSS_INLINE void setCollectionInfo ( dmsStorageUnit *su,
                                             dmsMBContext *mbContext )
         {
            setCSInfo( su ) ;
            setCLInfo( mbContext ) ;

            if ( _cacheLevel > OPT_PLAN_NOCACHE )
            {
               // Key code is not needed for no-cache mode
               _generateKeyCodeInternal() ;
            }

            _isValid = TRUE ;
         }

         INT32 normalize ( optAccessPlanHelper &planHelper,
                           mthMatchRuntime *matchRuntime ) ;

         OSS_INLINE static const CHAR * getCacheLevelName (
                                             OPT_PLAN_CACHE_LEVEL cacheLevel )
         {
            switch ( cacheLevel )
            {
               case OPT_PLAN_ORIGINAL :
                  return OPT_VALUE_CACHE_ORIGINAL ;
               case OPT_PLAN_NORMALIZED :
                  return OPT_VALUE_CACHE_NORMALIZED ;
               case OPT_PLAN_PARAMETERIZED :
                  return OPT_VALUE_CACHE_PARAMETERIZED ;
               case OPT_PLAN_FUZZYOPTR :
                  return OPT_VALUE_CACHE_FUZZYOPTR ;
               default :
                  break ;
            }
            return OPT_VALUE_CACHE_NOCACHE ;
         }

      protected :

         void _generateKeyCodeInternal () ;

         UINT32 _generateKeyCodeHash () ;

      protected :
         BOOLEAN                 _isValid ;
         OPT_PLAN_CACHE_LEVEL    _cacheLevel ;
         BSONObj                 _normalizedQuery ;
   } ;

   typedef class _optAccessPlanKey optAccessPlanKey ;

}

#endif //OPTACCESSPLANKEY_HPP__
