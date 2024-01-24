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

   Source File Name = mthMatchRuntime.hpp

   Descriptive Name = Matcher Tree Runtime Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains runtime structure for
   matchers.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMATCHRUNTIME_HPP__
#define MTHMATCHRUNTIME_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "utilPooledObject.hpp"
#include "ossUtil.hpp"
#include "mthMatchTree.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _mthMatchTreeHolder define
    */
   class _mthMatchTreeHolder
   {
      public :
         _mthMatchTreeHolder () ;

         virtual ~_mthMatchTreeHolder () ;

         virtual INT32 createMatchTree () ;

         virtual void deleteMatchTree () ;

         OSS_INLINE virtual mthMatchTree *getMatchTree ()
         {
            return _matchTree ;
         }

         OSS_INLINE virtual const mthMatchTree *getMatchTree () const
         {
            return _matchTree ;
         }

         OSS_INLINE virtual void setMatchTree ( mthMatchTree *matchTree )
         {
            deleteMatchTree() ;
            _matchTree = matchTree ;
            _ownedMatchTree = FALSE ;
         }

      protected :
         mthMatchTree * _matchTree ;
         BOOLEAN        _ownedMatchTree ;
   } ;

   /*
      _mthMatchTreeStackHolder define
    */
   class _mthMatchTreeStackHolder : public _mthMatchTreeHolder
   {
      public :
         _mthMatchTreeStackHolder () ;

         virtual ~_mthMatchTreeStackHolder () ;

         OSS_INLINE virtual INT32 createMatchTree ()
         {
            // Do nothing
            return SDB_OK ;
         }

         OSS_INLINE virtual void deleteMatchTree ()
         {
            // Do nothing
         }

         OSS_INLINE virtual void setMatchTree ( mthMatchTree *matchTree )
         {
            // Do nothing
         }

      protected :
         mthMatchTree _stackMatchTree ;
   } ;

   /*
      _mthParamPredListHolder define
    */
   class _mthParamPredListHolder
   {
      public :
         _mthParamPredListHolder ()
         : _paramPredList( NULL )
         {
         }

         virtual ~_mthParamPredListHolder ()
         {
            _paramPredList = NULL ;
         }

         OSS_INLINE RTN_PARAM_PREDICATE_LIST *getParamPredList ()
         {
            return _paramPredList ;
         }

         OSS_INLINE const RTN_PARAM_PREDICATE_LIST *getParamPredList () const
         {
            return _paramPredList ;
         }

         OSS_INLINE virtual void setParamPredList ( RTN_PARAM_PREDICATE_LIST *paramPredList )
         {
            _paramPredList = paramPredList ;
         }

      protected :
         RTN_PARAM_PREDICATE_LIST *_paramPredList ;
   } ;

   class _mthParamPredListStackHolder : public _mthParamPredListHolder
   {
      public :
         _mthParamPredListStackHolder ()
         : _mthParamPredListHolder()
         {
            _paramPredList = ( &_stackParamPredList ) ;
         }

         virtual ~_mthParamPredListStackHolder () {}

         OSS_INLINE virtual void setParamPredList ( RTN_PARAM_PREDICATE_LIST *paramPredList )
         {
            // Do nothing
         }

      protected :
         // Predicate set generated from matcher tree
         RTN_PARAM_PREDICATE_LIST _stackParamPredList ;
   } ;

   typedef class _mthParamPredListHolder mthParamPredListHolder ;

   /*
      _mthMatchRuntime define
    */
   class _mthMatchRuntime : public utilPooledObject,
                            public _mthMatchTreeHolder,
                            public _mthParamPredListHolder
   {
      public :
         _mthMatchRuntime () ;

         virtual ~_mthMatchRuntime () ;

         OSS_INLINE BSONObj getQuery ()
         {
            return _query ;
         }

         void setQuery ( const BSONObj &query, BOOLEAN getOwned ) ;

         OSS_INLINE rtnParamList &getParameters ()
         {
            return _parameters ;
         }

         OSS_INLINE const rtnParamList &getParameters () const
         {
            return _parameters ;
         }

         OSS_INLINE rtnParamList *getParametersPointer ()
         {
            return _parameters.isEmpty() ? NULL : &_parameters ;
         }

         OSS_INLINE rtnPredicateList *getPredList ()
         {
            return _predList.isInitialized() ? (&_predList) : NULL ;
         }

         OSS_INLINE const rtnPredicateList *getPredList () const
         {
            return _predList.isInitialized() ? (&_predList) : NULL ;
         }

         OSS_INLINE BOOLEAN isFixedPredList () const
         {
            return _predList.isInitialized() &&
                   _predList.isFixedPredList() ;
         }

         INT32 generatePredList ( const rtnPredicateSet &predicateSet,
                                  const BSONObj &keyPattern,
                                  INT32 direction,
                                  mthMatchNormalizer &normalizer ) ;

         OSS_INLINE void clearPredList ()
         {
            _predList.clear() ;
         }

         OSS_INLINE BSONObj getEqualityQueryObject ()
         {
            if ( NULL != getMatchTree() )
            {
               return getMatchTree()->getEqualityQueryObject( &_parameters ) ;
            }
            return BSONObj() ;
         }

      protected :
         BSONObj              _query ;
         rtnParamList         _parameters ;
         rtnPredicateList     _predList ;
   } ;

   typedef class _mthMatchRuntime mthMatchRuntime ;

   /*
      _mthMatchRuntimeHolder define
    */
   class _mthMatchRuntimeHolder
   {
      public :
         _mthMatchRuntimeHolder () ;

         virtual ~_mthMatchRuntimeHolder () ;

         virtual void deleteMatchRuntime () ;

         virtual INT32 createMatchRuntime () ;

         void getMatchRuntimeOnwed ( _mthMatchRuntimeHolder &holder ) ;

         virtual const mthMatchRuntime *getMatchRuntime () const
         {
            return _matchRuntime ;
         }

         OSS_INLINE virtual mthMatchRuntime *getMatchRuntime ()
         {
            return _matchRuntime ;
         }

         OSS_INLINE virtual void setMatchRuntime ( mthMatchRuntime *matchRuntime )
         {
            deleteMatchRuntime() ;
            _matchRuntime = matchRuntime ;
         }

      protected :
         mthMatchRuntime *_matchRuntime ;
         BOOLEAN _ownedMatchRuntime ;
   } ;
}

#endif //MTHMATCHRUNTIME_HPP__
