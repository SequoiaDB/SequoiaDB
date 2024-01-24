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

   Source File Name = optAccessPlanHelper.hpp

   Descriptive Name = Optimizer Access Plan Helper Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains structure for Helper of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/02/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPT_ACCESSPLANHELPER_HPP__
#define OPT_ACCESSPLANHELPER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "optCommon.hpp"
#include "mthMatchRuntime.hpp"
#include "rtnQueryOptions.hpp"

namespace engine
{

   /*
      _optAccessPlanConfig define
    */
   typedef struct _optAccessPlanConfig
   {
      _optAccessPlanConfig ()
      : _sortBufferSize( 0 ),
        _optCostThreshold( 0 ),
        _planCacheMainThreshold( 0 )
      {
      }

      _optAccessPlanConfig ( const _optAccessPlanConfig &config )
      : _sortBufferSize( config._sortBufferSize ),
        _optCostThreshold( config._optCostThreshold ),
        _planCacheMainThreshold( config._planCacheMainThreshold )
      {
      }

      UINT32   _sortBufferSize ;
      INT32    _optCostThreshold ;
      INT32    _planCacheMainThreshold ;
   } optAccessPlanConfig ;

   /*
      _optAccessPlanConfigHolder define
    */
   class _optAccessPlanConfigHolder
   {
      public :
         _optAccessPlanConfigHolder () ;

         _optAccessPlanConfigHolder ( const optAccessPlanConfig &config ) ;

         virtual ~_optAccessPlanConfigHolder () ;

         OSS_INLINE const optAccessPlanConfig & getPlanConfig () const
         {
            return _config ;
         }

         OSS_INLINE void setPlanConfig ( const optAccessPlanConfig & config )
         {
            _config._sortBufferSize = config._sortBufferSize ;
            _config._optCostThreshold = config._optCostThreshold ;
         }

         OSS_INLINE void setSortBufferSize ( UINT32 sortBufferSize )
         {
            _config._sortBufferSize = sortBufferSize ;
         }

         OSS_INLINE UINT32 getSortBufferSizeMB () const
         {
            return _config._sortBufferSize ;
         }

         OSS_INLINE UINT64 getSortBufferSize () const
         {
            return ( (UINT64)_config._sortBufferSize ) << 20 ;
         }

         OSS_INLINE void setOptCostThreshold ( INT32 optCostThreshold )
         {
            _config._optCostThreshold = optCostThreshold ;
         }

         OSS_INLINE INT32 getOptCostThreshold () const
         {
            return _config._optCostThreshold ;
         }

         OSS_INLINE void setPlanCacheMainCLThreshold( INT32 planCacheMainThreshold )
         {
            _config._planCacheMainThreshold = planCacheMainThreshold ;
         }

         OSS_INLINE INT32 getPlanCacheMainCLThreshold() const
         {
            return _config._planCacheMainThreshold ;
         }

      protected :
         optAccessPlanConfig _config ;
   } ;

   typedef class _optAccessPlanConfigHolder optAccessPlanConfigHolder ;

   /*
      _optAccessPlanHelper define
    */
   class _optAccessPlanHelper : public SDBObject,
                                public _mthMatchTreeHolder,
                                public _optAccessPlanConfigHolder,
                                public _mthMatchConfigHolder
   {
      public :
         _optAccessPlanHelper ( OPT_PLAN_CACHE_LEVEL cacheLevel,
                                const optAccessPlanConfig &planConfig,
                                const mthNodeConfig &mthConfig,
                                const rtnExplainOptions *expOptions ) ;

         virtual ~_optAccessPlanHelper () ;

         void clear () ;

         OSS_INLINE BSONObj getQuery ()
         {
            return _query ;
         }

         void setMatchTree ( _mthMatchTree *matchTree ) ;

         void getEstimation ( optCollectionStat *pCollectionStat,
                              double &estSelectivity, UINT32 &estCPUCost ) ;

         OSS_INLINE void getPredSelectivity ( double &predSelectivity,
                                              double &scanSelectivity )
         {
            predSelectivity = _predSelectivity ;
            scanSelectivity = _scanSelectivity ;
         }

         OSS_INLINE BOOLEAN isAutoHint() const
         {
            return _isAutoHint ;
         }

         OSS_INLINE void setAutoHint( BOOLEAN isAutoHint )
         {
            _isAutoHint = isAutoHint ;
         }

         OSS_INLINE BOOLEAN isPredEstimated () const
         {
            return _isPredEstimated ;
         }

         INT32 normalizeQuery ( const BSONObj &query,
                                BSONObjBuilder &normalBuilder,
                                rtnParamList &parameters,
                                BOOLEAN &invalidMatcher ) ;

         mthMatchNormalizer &getNormalizer ()
         {
            return _normalizer ;
         }

         OSS_INLINE const rtnPredicateSet &getPredicateSet() const
         {
            return _predicateSet ;
         }

         OSS_INLINE rtnPredicateSet &getPredicateSet ()
         {
            return _predicateSet ;
         }

         OSS_INLINE RTN_PREDICATE_MAP &getPredicates ()
         {
            return _predicateSet.predicates() ;
         }

         OSS_INLINE BOOLEAN isPredicateSetEmpty () const
         {
            return ( _predicateSet.getSize() == 0 ) ;
         }

         OSS_INLINE const rtnExplainOptions *getExplainOptions() const
         {
            return _expOptions ;
         }

         OSS_INLINE BOOLEAN isKeepPaths () const
         {
            return NULL != _expOptions && _expOptions->isNeedSearch() ;
         }

         INT32 saveSelectivityToCache( const ossPoolString &indexPath,
                                       double predSelectivity,
                                       double scanSelectivity ) ;
         BOOLEAN getSelectivityFromCache( const ossPoolString &indexPath,
                                          double &predSelectivity,
                                          double &scanSelectivity ) ;

      protected :
         void _evalEstimation ( optCollectionStat *pCollectionStat ) ;

      protected :
         BSONObj              _query ;
         OPT_PLAN_CACHE_LEVEL _cacheLevel ;
         mthMatchNormalizer   _normalizer ;
         rtnPredicateSet      _predicateSet ;

         // Flag to indicate whether is a auto hint, e.g. { "" : "" }
         BOOLEAN           _isAutoHint ;

         /// Cost or selectivity estimations
         // Flag to indicate whether the cost and selectivity are estimated
         BOOLEAN           _isPredEstimated ;
         // Selectivity of the matcher
         double            _estSelectivity ;
         // The final selectivity of the predicates
         double            _predSelectivity ;
         // The scan selectivity of the predicates
         double            _scanSelectivity ;
         // The CPU cost of the matcher
         UINT32            _estCPUCost ;

         // explain options
         const rtnExplainOptions * _expOptions ;

         // cache of selectivity
         optPlanSelectivityCache _selectivityCache ;
   } ;

   typedef class _optAccessPlanHelper optAccessPlanHelper ;
}

#endif //OPT_ACCESSPLANHELPER_HPP__
