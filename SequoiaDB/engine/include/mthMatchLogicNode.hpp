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

   Source File Name = mthMatchLogicNode.hpp

   Descriptive Name = Method Match logic Node Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCH_LOGICNODE_HPP_
#define MTH_MATCH_LOGICNODE_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "mthMatchNode.hpp"
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{
   class _mthMatchLogicNode : public _mthMatchNode
   {
      public:
         _mthMatchLogicNode( _mthNodeAllocator *allocator,
                             const mthNodeConfig *config ) ;
         virtual ~_mthMatchLogicNode() ;

      public: /* from parent */
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;
         virtual void clear() ;
         virtual void setWeight( UINT32 weight ) ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual BSONObj toBson() ;
         virtual BSONObj toParamBson ( const rtnParamList &parameters ) ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual void _clear() ;

      protected:
         INT32 _weight ;
   } ;

   class _mthMatchLogicAndNode : public _mthMatchLogicNode
   {
      public:
         _mthMatchLogicAndNode( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;
         virtual ~_mthMatchLogicAndNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void evalEstimation ( const optCollectionStat *pCollectionStat,
                                       double &selectivity, UINT32 &cpuCost ) ;
   } ;

   class _mthMatchLogicOrNode : public _mthMatchLogicNode
   {
      public:
         _mthMatchLogicOrNode( _mthNodeAllocator *allocator,
                               const mthNodeConfig *config ) ;
         virtual ~_mthMatchLogicOrNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual INT32 calcPredicate( rtnPredicateSet &predicateSet,
                                      const rtnParamList * paramList ) ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;
         virtual void evalEstimation ( const optCollectionStat *pCollectionStat,
                                       double &selectivity, UINT32 &cpuCost ) ;
   } ;

   class _mthMatchLogicNotNode : public _mthMatchLogicAndNode
   {
      public:
         _mthMatchLogicNotNode( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;
         virtual ~_mthMatchLogicNotNode() ;

      public:
         virtual INT32 getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual INT32 calcPredicate( rtnPredicateSet &predicateSet,
                                      const rtnParamList * paramList ) ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void evalEstimation ( const optCollectionStat *pCollectionStat,
                                       double &selectivity, UINT32 &cpuCost ) ;
   } ;
}

#endif

