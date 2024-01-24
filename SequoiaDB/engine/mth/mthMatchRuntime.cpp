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

   Source File Name = mthMatchRuntime.cpp

   Descriptive Name = Match Tree Runtime

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains functions for runtime of
   matchers.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthMatchRuntime.hpp"
#include "mthMatchNormalizer.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"

using namespace bson ;

namespace engine
{

   /*
      _mthMatchTreeHolder implement
    */
   _mthMatchTreeHolder::_mthMatchTreeHolder ()
   : _matchTree( NULL ),
     _ownedMatchTree( FALSE )
   {
   }

   _mthMatchTreeHolder::~_mthMatchTreeHolder ()
   {
      deleteMatchTree() ;
   }

   INT32 _mthMatchTreeHolder::createMatchTree ()
   {
      INT32 rc = SDB_OK ;

      deleteMatchTree() ;

      _matchTree = SDB_OSS_NEW _mthMatchTree() ;
      PD_CHECK( _matchTree, SDB_OOM, error, PDERROR,
                "Failed to allocate match tree" ) ;
      _ownedMatchTree = TRUE ;

   done :
      return rc ;
   error :
      goto done ;
   }

   void _mthMatchTreeHolder::deleteMatchTree ()
   {
      if ( _ownedMatchTree && NULL != _matchTree )
      {
         SDB_OSS_DEL _matchTree ;
      }
      _matchTree = NULL ;
      _ownedMatchTree = FALSE ;
   }

   /*
      _mthMatchTreeStackHolder implement
    */
   _mthMatchTreeStackHolder::_mthMatchTreeStackHolder ()
   : _mthMatchTreeHolder()
   {
      _matchTree = (&_stackMatchTree ) ;
      _ownedMatchTree = TRUE ;
   }

   _mthMatchTreeStackHolder::~_mthMatchTreeStackHolder ()
   {
      _matchTree = NULL ;
      _ownedMatchTree = FALSE ;
   }

   /*
      _mthMatchRuntime implement
    */
   _mthMatchRuntime::_mthMatchRuntime ()
   : _mthMatchTreeHolder(),
     _mthParamPredListHolder()
   {
   }

   _mthMatchRuntime::~_mthMatchRuntime ()
   {
      _parameters.clearParams() ;
      clearPredList() ;
   }

   void _mthMatchRuntime::setQuery ( const BSONObj &query, BOOLEAN getOwned )
   {
      if ( getOwned )
      {
         _query = query.getOwned() ;
      }
      else
      {
         _query = query ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHRTM_GENPREDLST, "_mthMatchRuntime::generatePredList" )
   INT32 _mthMatchRuntime::generatePredList ( const rtnPredicateSet &predicateSet,
                                              const BSONObj &keyPattern,
                                              INT32 direction,
                                              mthMatchNormalizer &normalizer )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHRTM_GENPREDLST ) ;

      clearPredList() ;

      if ( NULL != getMatchTree() )
      {
         RTN_PARAM_PREDICATE_LIST *paramPredList = getParamPredList() ;

         // Create predicate list
         if ( _parameters.isEmpty() || NULL == paramPredList )
         {
            UINT32 addedLevel = 0 ;
            // Not parameterized, initialize with predicate set
            rc = _predList.initialize( predicateSet, keyPattern, direction,
                                       addedLevel ) ;
            normalizer.setDoneByPred( keyPattern, addedLevel ) ;
         }
         else if ( paramPredList->empty() )
         {
            // parameterized predicate list is empty, initialize predicate list
            // with predicate set, and generate parameterized predicate list
            // for the next query
            rc = _predList.initialize( predicateSet, keyPattern, direction,
                                       _parameters, (*paramPredList) ) ;
         }
         else
         {
            // parameterized predicate list is initialized, initialize
            // predicate list with parameterized predicate list
            rc = _predList.initialize( (*paramPredList), keyPattern, direction,
                                       _parameters ) ;
         }
         PD_RC_CHECK( rc, PDWARNING, "Failed to generate predicate list, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MTHRTM_GENPREDLST, rc ) ;
      return rc ;

   error :
      clearPredList() ;
      goto done ;
   }

   /*
      _mthMatchRuntimeHolder implement
    */
   _mthMatchRuntimeHolder::_mthMatchRuntimeHolder ()
   : _matchRuntime( NULL ),
     _ownedMatchRuntime( FALSE )
   {
   }

   _mthMatchRuntimeHolder::~_mthMatchRuntimeHolder ()
   {
      deleteMatchRuntime() ;
   }

   void _mthMatchRuntimeHolder::deleteMatchRuntime ()
   {
      if ( _ownedMatchRuntime && NULL != _matchRuntime )
      {
         SDB_OSS_DEL _matchRuntime ;
      }
      _matchRuntime = NULL ;
      _ownedMatchRuntime = FALSE ;
   }

   INT32 _mthMatchRuntimeHolder::createMatchRuntime ()
   {
      INT32 rc = SDB_OK ;

      deleteMatchRuntime() ;

      _matchRuntime = SDB_OSS_NEW mthMatchRuntime () ;
      PD_CHECK( _matchRuntime, SDB_OOM, error, PDERROR,
                "Failed to allocate match runtime" ) ;
      _ownedMatchRuntime = TRUE ;

   done :
      return rc ;
   error :
      goto done ;
   }

   void _mthMatchRuntimeHolder::getMatchRuntimeOnwed ( _mthMatchRuntimeHolder &holder )
   {
      deleteMatchRuntime() ;
      if ( holder._ownedMatchRuntime && NULL != holder._matchRuntime )
      {
         _matchRuntime = holder._matchRuntime ;
         _ownedMatchRuntime = TRUE ;
         holder._ownedMatchRuntime = FALSE ;
      }
   }

}
