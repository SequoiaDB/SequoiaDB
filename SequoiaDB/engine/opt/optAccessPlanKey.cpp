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

      if ( !getOrderBy().shallowEqual( planKey.getOrderBy() ) )
      {
         return FALSE ;
      }

      if ( getFlag() != planKey.getFlag() )
      {
         BOOLEAN lhsFlag = isSortedIdxRequired() ? TRUE : FALSE ;
         BOOLEAN rhsFlag = planKey.isSortedIdxRequired() ? TRUE : FALSE ;
         if ( lhsFlag != rhsFlag )
         {
            return FALSE ;
         }
      }

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

      BSONObjBuilder normalBuilder ;
      BOOLEAN invalidMatcher = FALSE ;

      SDB_ASSERT( matchRuntime, "matchRuntime is invalid" ) ;

      matchRuntime->setQuery( getQuery(), TRUE ) ;

      rc = planHelper.normalizeQuery( matchRuntime->getQuery(),
                                      normalBuilder,
                                      matchRuntime->getParameters(),
                                      invalidMatcher ) ;
      if ( SDB_OK == rc )
      {
         _normalizedQuery = normalBuilder.obj() ;

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

      _cacheLevel = OPT_PLAN_ORIGINAL ;
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

      if ( _normalizedQuery.isEmpty() && !isQueryEmpty() )
      {
         keyCode ^= getQueryHash() ;
      }
      else if ( !_normalizedQuery.isEmpty() )
      {
         keyCode ^= ossHash( _normalizedQuery.objdata(),
                             _normalizedQuery.objsize() ) ;
      }

      if ( !isOrderByEmpty() )
      {
         keyCode ^= getOrderByHash() ;
      }

      BSONObjIterator itr( getHint() ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( e.isABSONObj() )
            continue ;
         keyCode ^= ossHash( e.value(), e.valuesize() ) ;
      }

      return keyCode ;
   }

}

