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

   Source File Name = clsCatalogMatcher.hpp

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

#ifndef CLSCATALOGMATCHER_HPP_
#define CLSCATALOGMATCHER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "clsCatalogPredicate.hpp"

using namespace bson ;

namespace engine
{

   /*
      clsCatalogMatcher define
   */
   class clsCatalogMatcher : public SDBObject
   {
   public:
      clsCatalogMatcher( const BSONObj &shardingKey,
                         BOOLEAN isHashShard ) ;

      INT32 loadPattern( const BSONObj &matcher ) ;

      INT32 calc( const _clsCatalogSet *pSet,
                  CLS_SET_CATAITEM &setItem ) ;

      BOOLEAN isUniverse() const ;
      BOOLEAN isNull() const ;

   private:
      INT32 parseAnObj( const BSONObj &matcher,
                        clsCatalogPredicateTree &predicateSet ) ;

      INT32 parseCmpOp( const BSONElement &beField,
                        clsCatalogPredicateTree &predicateSet );

      INT32 parseOpObj( const BSONElement &beField,
                        clsCatalogPredicateTree & predicateSet ) ;

      INT32 parseLogicOp( const BSONElement &beField,
                          clsCatalogPredicateTree &predicateSet ) ;

      BOOLEAN isOpObj( const BSONObj &obj ) const ;

      BOOLEAN _isExistUnreconigzeOp( const BSONObj &obj ) const ;
      BOOLEAN _isHashExistUnreconigzeOp( const BSONObj &obj ) const ;
      BOOLEAN _isRangeExistUnreconigzeOp( const BSONObj &obj ) const ;

   private:
      BOOLEAN                    _isHashShard ;
      clsCatalogPredicateTree    _predicateSet ;
      BSONObj                    _shardingKey ;
      BSONObj                    _matcher ;
   } ;

}

#endif // CLSCATALOGMATCHER_HPP_

