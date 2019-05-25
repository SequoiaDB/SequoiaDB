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

   Source File Name = clsCatalogPredicate.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CLSCATALOGPREDICATE_HPP_
#define CLSCATALOGPREDICATE_HPP_

#include "rtnPredicate.hpp"
#include "utilMap.hpp"

namespace engine
{
   class clsCatalogPredicateTree;
   class _clsCatalogItem;

   typedef _utilMap< std::string, rtnStartStopKey*, 10 > MAP_CLSCATAPREDICATEFIELD ;
   typedef std::vector< clsCatalogPredicateTree * >      VEC_CLSCATAPREDICATESET ;

   /*
      _CLS_CATA_LOGIC_TYPE define
   */
   typedef enum _CLS_CATA_LOGIC_TYPE
   {
      CLS_CATA_LOGIC_INVALID        = 0,
      CLS_CATA_LOGIC_AND            = 1,
      CLS_CATA_LOGIC_OR,
   }CLS_CATA_LOGIC_TYPE ;

   /*
      clsCatalogPredicateTree define
   */
   class clsCatalogPredicateTree : public SDBObject
   {
   public:
      clsCatalogPredicateTree( bson::BSONObj shardingKey ) ;
      ~clsCatalogPredicateTree() ;

      void upgradeToUniverse() ;
      BOOLEAN isUniverse() ;
      CLS_CATA_LOGIC_TYPE getLogicType() ;
      void setLogicType( CLS_CATA_LOGIC_TYPE type ) ;
      void addChild( clsCatalogPredicateTree *pChild ) ;
      INT32 addPredicate( const CHAR *pFieldName, bson::BSONElement beField,
                          INT32 opType );
      void adjustByShardingKey() ;
      void clear() ;
      INT32 matches( _clsCatalogItem * pCatalogItem, BOOLEAN & result ) ;

      string toString() const ;

   protected:
      INT32 _matches( bson::BSONObjIterator itrSK,
                      bson::BSONObjIterator itrLB,
                      bson::BSONObjIterator itrUB,
                      BOOLEAN & result,
                      BOOLEAN isCloseInterval,
                      INT32 compareLU ) ;

   private:
      clsCatalogPredicateTree( clsCatalogPredicateTree &right ){}
   private:
      VEC_CLSCATAPREDICATESET       _children ;
      rtnPredicateSet               _predicateSet ;
      CLS_CATA_LOGIC_TYPE           _logicType ;
      bson::BSONObj                 _shardingKey ;

   } ;

}

#endif // CLSCATALOGPREDICATE_HPP_

