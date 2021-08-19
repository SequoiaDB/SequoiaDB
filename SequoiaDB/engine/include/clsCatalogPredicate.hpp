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
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{
   class clsCatalogPredicateTree ;
   class _clsCatalogSet ;
   class _clsCatalogItem ;

   typedef ossPoolVector< clsCatalogPredicateTree * > VEC_CLSCATAPREDICATESET ;
   typedef ossPoolSet< const _clsCatalogItem* >       CLS_SET_CATAITEM ;

   /*
      _CLS_CATA_LOGIC_TYPE define
   */
   enum _CLS_CATA_LOGIC_TYPE
   {
      CLS_CATA_LOGIC_INVALID        = 0,
      CLS_CATA_LOGIC_AND            = 1,
      CLS_CATA_LOGIC_OR,
   } ;
   typedef _CLS_CATA_LOGIC_TYPE CLS_CATA_LOGIC_TYPE ;

   /*
      clsCatalogPredicateTree define
   */
   class clsCatalogPredicateTree : public utilPooledObject
   {
   public:
      clsCatalogPredicateTree( const BSONObj &shardingKey,
                               BOOLEAN isHashShard ) ;
      ~clsCatalogPredicateTree() ;

      void  upgradeToUniverse() ;
      void  upgradeToNull() ;
      void  setLogicType( CLS_CATA_LOGIC_TYPE type ) ;

      BOOLEAN              isUniverse() const ;
      BOOLEAN              isNull() const { return _isNull ; }
      CLS_CATA_LOGIC_TYPE  getLogicType() const ;

      INT32 addChild( clsCatalogPredicateTree *pChild ) ;
      INT32 addPredicate( const CHAR *pFieldName,
                          const BSONElement &beField,
                          INT32 opType ) ;

      INT32 done() ;

      INT32 calc( const _clsCatalogSet *pSet,
                  CLS_SET_CATAITEM &setItem ) ;

      ossPoolString toString() const ;

   protected:
      INT32 _pushPredset( rtnPredicateSet &predset ) ;
      void  _doneCheckUniverse() ;
      void  _doneCheckNull() ;
      void  _clearChildren() ;
      void  _clear() ;
      INT32 _done() ;

      BOOLEAN _calcNext( VEC_INT32 &vecCur ) const ;

      INT32 _calc( const _clsCatalogSet *pSet,
                   VEC_INT32 &vecCur,
                   CLS_SET_CATAITEM &setItem ) ;

      INT32 _buildStartObj( VEC_INT32 &vecCur,
                            BSONObj &obj,
                            BOOLEAN &isEqual ) ;

      void  _mergeAnd( CLS_SET_CATAITEM &setItem,
                       const CLS_SET_CATAITEM &mergeItem ) ;

      INT32 _mergeOr( CLS_SET_CATAITEM &setItem,
                      const CLS_SET_CATAITEM &mergeItem ) ;

      INT32 _compareStartWithBound( VEC_INT32 &vecCur,
                                    const BSONObj &bound,
                                    const Ordering *pOrder ) ;

      INT32 _compareStopWithBound( VEC_INT32 &vecCur,
                                   const BSONObj &bound,
                                   const Ordering *pOrder ) ;

   private:
      // forbid copy constructor
      clsCatalogPredicateTree( const clsCatalogPredicateTree &right ) {}

   private:
      VEC_CLSCATAPREDICATESET       _children ;
      rtnPredicateSet               _predicateSet ;
      rtnPredicateList              _predicateLst ;
      CLS_CATA_LOGIC_TYPE           _logicType ;
      BSONObj                       _shardingKey ;
      BOOLEAN                       _isNull ;
      BOOLEAN                       _isHashShard ;

   } ;

}

#endif // CLSCATALOGPREDICATE_HPP_

