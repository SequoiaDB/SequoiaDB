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

   Source File Name = optAccessPlanHelper.cpp

   Descriptive Name = Optimizer Access Plan Helper

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for helper of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/02/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optAccessPlanHelper.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"

namespace engine
{

   /*
      _optAccessPlanConfigHolder implement
    */
   _optAccessPlanConfigHolder::_optAccessPlanConfigHolder ()
   : _config()
   {
   }

   _optAccessPlanConfigHolder::_optAccessPlanConfigHolder( const optAccessPlanConfig &config )
   : _config( config )
   {

   }

   _optAccessPlanConfigHolder::~_optAccessPlanConfigHolder ()
   {
   }

   /*
      _optAccessPlanHelper implement
    */
   _optAccessPlanHelper::_optAccessPlanHelper ( OPT_PLAN_CACHE_LEVEL cacheLevel,
                                                const optAccessPlanConfig &planConfig,
                                                const mthNodeConfig &mthConfig,
                                                BOOLEAN keepSearchPaths )
   : _mthMatchTreeHolder(),
     _optAccessPlanConfigHolder( planConfig ),
     _mthMatchConfigHolder( mthConfig ),
     _cacheLevel( cacheLevel ),
     _normalizer( getMatchConfigPtr() ),
     _isEstimated( FALSE ),
     _estSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _predSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _scanSelectivity( OPT_MTH_DEFAULT_SELECTIVITY ),
     _estCPUCost( OPT_MTH_OPTR_DEFAULT_SELECTIVITY ),
     _keepSearchPaths( keepSearchPaths )
   {
      setMthEnableParameterized( mthConfig._enableParameterized &&
                                 ( cacheLevel >= OPT_PLAN_PARAMETERIZED ) ) ;
      setMthEnableFuzzyOptr( mthConfig._enableFuzzyOptr &&
                             ( cacheLevel >= OPT_PLAN_FUZZYOPTR ) ) ;
   }

   _optAccessPlanHelper::~_optAccessPlanHelper ()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_CLEAR, "_optAccessPlanHelper::clear" )
   void _optAccessPlanHelper::clear ()
   {
      _mthMatchTreeHolder::setMatchTree( NULL ) ;
      _predicateSet.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_SETMTH, "_optAccessPlanHelper::setMatcher" )
   void _optAccessPlanHelper::setMatchTree ( _mthMatchTree *matchTree )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP_SETMTH ) ;

      _mthMatchTreeHolder::setMatchTree( matchTree ) ;
      _predicateSet.clear() ;

      _isEstimated         = FALSE ;
      _estSelectivity      = OPT_MTH_DEFAULT_SELECTIVITY ;
      _predSelectivity     = OPT_MTH_DEFAULT_SELECTIVITY ;
      _scanSelectivity     = OPT_MTH_DEFAULT_SELECTIVITY ;
      _estCPUCost          = OPT_MTH_OPTR_DEFAULT_SELECTIVITY ;

      PD_TRACE_EXIT( SDB__OPTAPHELP_SETMTH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_GETEST, "_optAccessPlanHelper::getEstimation" )
   void _optAccessPlanHelper::getEstimation ( optCollectionStat *pCollectionStat,
                                              double &estSelectivity,
                                              UINT32 &estCPUCost )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP_GETEST ) ;

      if ( !_isEstimated )
      {
         _evalEstimation( pCollectionStat ) ;
      }
      estSelectivity = _estSelectivity ;
      estCPUCost = _estCPUCost ;

      PD_TRACE_EXIT( SDB__OPTAPHELP_GETEST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP__EVALEST, "_optAccessPlanHelper::_evalEstimation" )
   void _optAccessPlanHelper::_evalEstimation ( optCollectionStat *pCollectionStat )
   {
      PD_TRACE_ENTRY( SDB__OPTAPHELP__EVALEST ) ;

      double predSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      double scanSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      double tmpSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      UINT32 tmpCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

      if ( getMatchTree() != NULL )
      {
         if ( pCollectionStat )
         {
            predSelectivity = pCollectionStat->evalPredicateSet(
                  _predicateSet, mthEnabledMixCmp(), scanSelectivity ) ;
         }
         getMatchTree()->evalEstimation( pCollectionStat, tmpSelectivity,
                                         tmpCPUCost ) ;
         tmpSelectivity *= predSelectivity ;
      }

      _estSelectivity = OPT_ROUND_SELECTIVITY( tmpSelectivity ) ;
      _predSelectivity = OPT_ROUND_SELECTIVITY( predSelectivity ) ;
      _scanSelectivity = OPT_ROUND_SELECTIVITY( scanSelectivity ) ;
      _estCPUCost = tmpCPUCost ;
      _isEstimated = TRUE ;

      PD_TRACE_EXIT( SDB__OPTAPHELP__EVALEST ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__OPTAPHELP_GENSIMMTH, "_optAccessPlanHelper::generateSimpleMatcher" )
   INT32 _optAccessPlanHelper::normalizeQuery ( const BSONObj &query,
                                                BSONObjBuilder &normalBuilder,
                                                rtnParamList &parameters,
                                                BOOLEAN &invalidMatcher )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__OPTAPHELP_GENSIMMTH ) ;

      _query = query ;

      rc = _normalizer.normalize( query, normalBuilder, parameters ) ;
      invalidMatcher = _normalizer.isInvalidMatcher() ;
      PD_RC_CHECK( rc, invalidMatcher ? PDERROR : PDDEBUG,
                   "Failed to normalize query [%s] with normalizer, rc: %d",
                   query.toString( FALSE, TRUE ).c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__OPTAPHELP_GENSIMMTH, rc ) ;
      return rc ;

   error :
      _normalizer.clear() ;
      parameters.clearParams() ;
      goto done ;
   }

}
