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

   Source File Name = optAccessPlanKey.cpp

   Descriptive Name = Optimizer Access Plan Key

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for key of
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "optAccessPlanKey.hpp"
#include "pdTrace.hpp"
#include "optTrace.hpp"
#include "pmd.hpp"
#include "mthMatchTree.hpp"
#include "mthMatchNormalizer.hpp"

using namespace bson ;

namespace engine
{

   /*
      _optAccessPlanKey implement
    */
   _optAccessPlanKey::_optAccessPlanKey ( const rtnQueryOptions &options,
                                          OPT_PLAN_CACHE_LEVEL cacheLevel )
   : _rtnQueryOptions( options ),
     _utilHashTableKey(),
     _optCollectionInfo(),
     _isValid( FALSE ),
     _cacheLevel( cacheLevel )
   {
      SDB_ASSERT( NULL != getCLFullName(), "pCLFullName is invalid" ) ;

      // Selector, skip and limit is not used to generate keys, reset them
      setSelector( BSONObj() ) ;
      setSkip( 0 ) ;
      setLimit( -1 ) ;
   }

   _optAccessPlanKey::_optAccessPlanKey ( _optAccessPlanKey &planKey )
   : _rtnQueryOptions( planKey ),
     _utilHashTableKey( planKey ),
     _optCollectionInfo( planKey ),
     _isValid( FALSE ),
     _cacheLevel( planKey._cacheLevel ),
     _normalizedQuery( planKey._normalizedQuery )
   {
   }

   _optAccessPlanKey::~_optAccessPlanKey ()
   {
   }

   BOOLEAN _optAccessPlanKey::isEqual ( const _optAccessPlanKey &planKey ) const
   {
      if ( !_isValid || !planKey._isValid )
      {
         return FALSE ;
      }

      if ( _keyCode != planKey._keyCode )
      {
         return FALSE ;
      }

      // Check the IDs of Collection Space and Collection
      if ( DMS_INVALID_SUID == _suID && DMS_INVALID_SUID == planKey._suID &&
           0 != ossStrncmp( getCLFullName(), planKey.getCLFullName(),
                            DMS_COLLECTION_FULL_NAME_SZ ) )
      {
         return FALSE ;
      }
      else if ( _suID != planKey._suID || _suLID != planKey._suLID ||
                _mbID != planKey._mbID || _clLID != planKey._clLID )
      {
         return FALSE ;
      }

      if ( _cacheLevel != planKey._cacheLevel )
      {
         return FALSE ;
      }

      // User query must be identical
      if ( _normalizedQuery.isEmpty() )
      {
         if ( !getQuery().shallowEqual( planKey.getQuery() ) )
         {
            return FALSE ;
         }
      }
      else
      {
         if ( !_normalizedQuery.shallowEqual( planKey._normalizedQuery ) )
         {
            return FALSE ;
         }
      }

      // Order by must be identical
      if ( !getOrderBy().shallowEqual( planKey.getOrderBy() ) )
      {
         return FALSE ;
      }

      // Query with modifier should use index to sort or forced index
      if ( getFlag() != planKey.getFlag() )
      {
         BOOLEAN lhsFlag = isSortedIdxRequired() ;
         BOOLEAN rhsFlag = planKey.isSortedIdxRequired() ;
         if ( lhsFlag != rhsFlag )
         {
            return FALSE ;
         }
         lhsFlag = isForceHint() ;
         rhsFlag = planKey.isForceHint() ;
         if ( lhsFlag != rhsFlag )
         {
            return FALSE ;
         }
      }

      if ( getInternalFlag() != planKey.getInternalFlag() )
      {
         BOOLEAN lhsFlag = isCount() ;
         BOOLEAN rhsFlag = planKey.isCount() ;
         if ( lhsFlag != rhsFlag )
         {
            return FALSE ;
         }
      }

      /// Hint must compare field by field, and need ignore object field and
      /// field name
      BSONObjIterator itr( planKey.getHint() ) ;
      BSONObjIterator itrSelf( getHint() ) ;
      while( itr.more() )
      {
         BSONElement e2 ;
         BSONElement e1 = itr.next() ;
         if ( e1.isABSONObj() )
         {
            continue ;
         }

         while( itrSelf.more() )
         {
            e2 = itrSelf.next() ;
            if ( e2.isABSONObj() )
            {
               continue ;
            }
            break ;
         }

         if ( 0 != e1.woCompare( e2, false ) )
         {
            return FALSE ;
         }
      }

      /// If _hint has other hint field, not the same
      while( itrSelf.more() )
      {
         BSONElement e = itrSelf.next() ;
         if ( !e.isABSONObj() )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_OPTAPKEY_NORMALIZE, "_optAccessPlanKey::normalize" )
   INT32 _optAccessPlanKey::normalize ( optAccessPlanHelper &planHelper,
                                        mthMatchRuntime *matchRuntime )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_OPTAPKEY_NORMALIZE ) ;

      // normalized query is a little larger than origin query
      // no need to use default initial size of BSON builder (512)
      BSONObjBuilder normalBuilder(
                  ossRoundUpToMultipleX( getQuery().objsize(), 32 ) ) ;
      BOOLEAN invalidMatcher = FALSE ;

      SDB_ASSERT( matchRuntime, "matchRuntime is invalid" ) ;

      // Copy the query
      matchRuntime->setQuery( getQuery(), TRUE ) ;

      rc = planHelper.normalizeQuery( matchRuntime->getQuery(),
                                      normalBuilder,
                                      matchRuntime->getParameters(),
                                      invalidMatcher ) ;
      if ( SDB_OK == rc )
      {
         _normalizedQuery = normalBuilder.obj() ;

         // No parameters have been found, decrease the cache level
         if ( matchRuntime->getParameters().isEmpty() &&
              _cacheLevel >= OPT_PLAN_PARAMETERIZED )
         {
            _cacheLevel = OPT_PLAN_NORMALIZED ;
            planHelper.setMthEnableFuzzyOptr( FALSE ) ;
            planHelper.setMthEnableParameterized( FALSE ) ;
         }
         goto done ;
      }

      PD_CHECK( !invalidMatcher, rc, error, PDERROR,
                "The matcher [%s] is invalid",
                getQuery().toString( FALSE, TRUE ).c_str() ) ;

      PD_LOG( PDDEBUG, "Failed to normalize query [%s] with normalizer, change "
              "the cache level to OPT_PLAN_ORIGINAL, rc: %d",
              getQuery().toString( FALSE, TRUE ).c_str(), rc ) ;

      // Ignore errors, goto full generation for no cache level
      // NOTE: we don't cache plans which could not be normalized in
      // >= NORMALIZED levels
      _cacheLevel = OPT_PLAN_NOCACHE ;
      rc = SDB_OK ;

   done :
      PD_TRACE_EXITRC( SDB_OPTAPKEY_NORMALIZE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _optAccessPlanKey::_generateKeyCodeInternal ()
   {
      setKeyCode( _generateKeyCodeHash() ) ;
   }

   UINT32 _optAccessPlanKey::_generateKeyCodeHash ()
   {
      UINT32 keyCode = 0 ;

      // Information of collection space and collection
      if ( DMS_INVALID_SUID != _suID )
      {
         keyCode = ossHash( (CHAR *)&_suID, sizeof( _suID ), 5 ) ;
         keyCode ^= ossHash( (CHAR *)&_suLID, sizeof( _suLID ), 5 ) ;
         keyCode ^= ossHash( (CHAR *)&_mbID, sizeof( _mbID ), 5 ) ;
         keyCode ^= ossHash( (CHAR *)&_clLID, sizeof( _clLID ), 5 ) ;
      }
      else
      {
         keyCode = ossHash( getCLFullName() ) ;
      }

      keyCode ^= ossHash( (CHAR *)&_cacheLevel, sizeof( _cacheLevel ), 5 ) ;

      // Query
      if ( _normalizedQuery.isEmpty() && !isQueryEmpty() )
      {
         keyCode ^= getQueryHash() ;
      }
      else if ( !_normalizedQuery.isEmpty() )
      {
         keyCode ^= ossHash( _normalizedQuery.objdata(),
                             _normalizedQuery.objsize() ) ;
      }

      // Order-By
      if ( !isOrderByEmpty() )
      {
         keyCode ^= getOrderByHash() ;
      }

      // Hint
      BSONObjIterator itr( getHint() ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( e.isABSONObj() )
            continue ;
         keyCode ^= ossHash( e.value(), e.valuesize() ) ;
      }

      // flags
      INT32 tmpFlag = OSS_BIT_TEST( _flag,
                                    ( FLG_QUERY_MODIFY |
                                      FLG_QUERY_FORCE_IDX_BY_SORT |
                                      FLG_QUERY_FORCE_HINT ) ) ;
      keyCode ^= ossHash( (CHAR *)( &tmpFlag ), sizeof( tmpFlag ), 5 ) ;

      return keyCode ;
   }

}

