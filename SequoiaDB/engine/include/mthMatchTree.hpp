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

   Source File Name = mthMatchTree.hpp

   Descriptive Name = Method MatchTree Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/25/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCHTREE_HPP_
#define MTH_MATCHTREE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "utilPooledObject.hpp"
#include "mthMatchNode.hpp"
#include "mthMatchLogicNode.hpp"
#include "mthMatchOpNode.hpp"
#include "mthMatchNormalizer.hpp"
#include "rtnPredicate.hpp"
#include <map>

using namespace bson ;
using namespace std ;

namespace engine
{
   struct mthMatchOpMapping
   {
      CHAR *opStr ;
      INT32 nodeType ;
   } ;

   class _mthMatchNodeFactory : public SDBObject
   {
   public:
      _mthMatchNodeFactory() ;
      ~_mthMatchNodeFactory() ;

   public:
      _mthMatchOpNode*        createOpNode( _mthNodeAllocator *allocator,
                                            const mthNodeConfig *configPtr,
                                            EN_MATCH_OP_FUNC_TYPE type ) ;
      _mthMatchLogicNode*     createLogicNode( _mthNodeAllocator *allocator,
                                               const mthNodeConfig *config,
                                               EN_MATCH_OP_FUNC_TYPE type ) ;
      void                    releaseNode( _mthMatchNode *node ) ;

      _mthMatchTree*          createTree() ;
      void                    releaseTree( _mthMatchTree *tree ) ;

      _mthMatchFunc*          createFunc( _mthNodeAllocator *allocator,
                                          EN_MATCH_OP_FUNC_TYPE type ) ;
      void                    releaseFunc( _mthMatchFunc *func ) ;

      EN_MATCH_OP_FUNC_TYPE   getMatchNodeType( const CHAR *opStr ) ;

   private:
      typedef map< ossPoolString, mthMatchOpMapping* >   MTH_OPSTRMAP ;
      MTH_OPSTRMAP _opstrMap ;
   } ;

   _mthMatchNodeFactory *mthGetMatchNodeFactory() ;

   class _mthMatchTree : public utilPooledObject,
                         public _mthMatchConfigHolder
   {
      public:
         _mthMatchTree() ;
         ~_mthMatchTree() ;

      public:
         INT32    loadPattern ( const BSONObj &matcher,
                                BOOLEAN needConfigMixCmp = TRUE ) ;

         INT32    loadPattern ( const BSONObj &matcher,
                                mthMatchNormalizer &normalizer ) ;

         //TODO: delete
         INT32    matches( const BSONObj &matchTarget, BOOLEAN &result,
                           _mthMatchTreeContext *context = NULL,
                           rtnParamList *parameters = NULL ) ;
         void     clear() ;
         BSONObj  getEqualityQueryObject( const rtnParamList *parameters = NULL ) ;
         BOOLEAN  isInitialized() const ;
         BOOLEAN  isMatchesAll() const ;
         BSONObj& getMatchPattern() ;
         BOOLEAN  hasDollarFieldName() ;
         BOOLEAN  totallyConverted() const ;
         void     setMatchesAll( BOOLEAN matchesAll ) ;
         BSONObj  getParsedMatcher( const rtnParamList &parameters ) const ;
         BSONObj  toBson() ;
         string   toString() ;

         BOOLEAN hasExpand() ;
         BOOLEAN hasReturnMatch() ;
         const CHAR *getAttrFieldName() ;

         void evalEstimation ( optCollectionStat *pCollectionStat,
                               double &estSelectivity, UINT32 &estCPUCost ) ;

         INT32    calcPredicate ( rtnPredicateSet &predicateSet,
                                  const rtnParamList * paramList ) ;

      private:
         INT32    _matches( const BSONObj &matchTarget, BOOLEAN &result,
                            _mthMatchTreeContext &context ) ;
         INT32    _addOperator( const CHAR *fieldName, const BSONElement &ele,
                                EN_MATCH_OP_FUNC_TYPE nodeType,
                                MTH_FUNC_LIST &funcList,
                                _mthMatchLogicNode *parent,
                                INT8 paramIndex,
                                INT8 fuzzyIndex,
                                _mthMatchOpNode **retNode = NULL ) ;
         INT32    _addFunction( const CHAR *fieldName,
                                const BSONElement &ele,
                                EN_MATCH_OP_FUNC_TYPE nodeType,
                                MTH_FUNC_LIST &funcList ) ;
         INT32    _addRegExOp( const CHAR *fieldName, const CHAR *regex,
                               const CHAR *options, MTH_FUNC_LIST &funcList,
                               _mthMatchLogicNode *parent,
                               _mthMatchOpNode **retNode = NULL ) ;
         INT32    _addExpandOp( const CHAR *fieldName,
                                const BSONElement &ele,
                                _mthMatchLogicNode *parent ) ;
         INT32    _parseOpItem ( mthMatchOpItem *opItem,
                                 _mthMatchLogicNode *parent ) ;
         INT32    _parseRegExElement( const BSONElement &ele,
                                      _mthMatchLogicNode *parent ) ;
         INT32    _parseNormalElement( const BSONElement &ele,
                                       _mthMatchLogicNode *parent ) ;
         INT32    _pareseLogicElemnts( const BSONElement ele,
                                       _mthMatchLogicNode *parent ) ;
         INT32    _pareseLogicAnd( const BSONElement ele,
                                   _mthMatchLogicNode *parent ) ;
         INT32    _pareseLogicOr( const BSONElement ele,
                                  _mthMatchLogicNode *parent ) ;
         INT32    _pareseLogicNot( const BSONElement ele,
                                   _mthMatchLogicNode *parent ) ;
         INT32    _parseArrayElement( const BSONElement &ele,
                                      _mthMatchLogicNode *parent ) ;
         INT32    _checkInnerObject ( const BSONElement &innerEle,
                                      UINT32 level, BSONElement *pOutEle ) ;
         INT32    _getElementKeysFormat( const BSONElement &ele ) ;

         void     _clearFuncList( MTH_FUNC_LIST &funcList ) ;

         INT32    _parseAttribute( const CHAR *fieldName,
                                   const BSONElement &ele,
                                   EN_MATCH_OP_FUNC_TYPE nodeType,
                                   MTH_FUNC_LIST &funcList ) ;
         INT32    _pareseObjectInnerOp( const BSONElement &ele,
                                        const BSONElement &innerEle,
                                        MTH_FUNC_LIST &funcList,
                                        _mthMatchLogicNode *parent,
                                        const char *&regex,
                                        const char *&options ) ;
         INT32    _parseObjectElement( const BSONElement &ele,
                                       _mthMatchLogicNode *parent ) ;
         INT32    _paresePrevOptions( const CHAR *fieldName,
                                      const BSONElement &ele,
                                      const CHAR *options,
                                      MTH_FUNC_LIST &funcList,
                                      _mthMatchLogicNode *parent ) ;
         INT32    _parseElement( const BSONElement &ele,
                                 _mthMatchLogicNode *parent ) ;
         void     _releaseTree( _mthMatchNode *node ) ;
         INT32    _optimizeNodeLevel() ;
         INT32    _deleteExtraLogicNode( _mthMatchNode *node ) ;
         INT32    _deleteNode( _mthMatchNode *parent, _mthMatchNode *node ) ;
         void     _setWeight( _mthMatchNode *node ) ;
         void     _sortByWeight() ;
         INT32    _optimize() ;
         void     _checkTotallyConverted() ;
         void     _checkTotallyConverted( _mthMatchNode *node,
                                          BOOLEAN &isTotallyConverted ) ;

         INT32    _createBuilder( BSONObjBuilder **builder ) ;
         void     _releaseBuilderVec( ossPoolVector< BSONObjBuilder* > &builderVec ) ;

         INT32    _adjustReturnMatchIndex( _mthMatchTreeContext &context ) ;

         INT32    _preLoadPattern ( const BSONObj &matcher,
                                    BOOLEAN enableMixCmp,
                                    BOOLEAN parameterized,
                                    BOOLEAN fuzzyOptr,
                                    BOOLEAN copyQuery ) ;
         INT32    _postLoadPattern ( BOOLEAN needOptimize ) ;

      private:
         _mthMatchNode     *_root ;
         BSONObj           _matchPattern ;
         BOOLEAN           _isInitialized ;
         BOOLEAN           _isMatchesAll ;
         BOOLEAN           _isTotallyConverted ;
         BOOLEAN           _hasDollarFieldName ;

         BOOLEAN           _hasExpand ;
         BOOLEAN           _hasReturnMatch ;
         const CHAR *      _attrFieldName ;
         _mthMatchOpNode*  _returnMatchNode ;

         _mthNodeAllocator _allocator ;
         ossPoolVector< BSONObjBuilder* > _builderVec ;
   } ;

   typedef class _mthMatchTree mthMatchTree ;

   class _mthRecordGenerator
   {
      enum EN_MATCH_RECORD_SRC_TYPE
      {
         // just return original src obj
         MTH_SRC_TYPE_ORIGINAL       = 1,

         // split src's fieldName to objs
         MTH_SRC_TYPE_ORIGINAL_SPLIT = 2,

         // set original fieldName's value to NULL
         MTH_SRC_TYPE_ORIGINAL_NULL  = 3,

         // combine all elements as one obj
         MTH_SRC_TYPE_ELEMENTS       = 4,

         // one element one obj
         MTH_SRC_TYPE_ELEMENTS_SPLIT = 5,
      } ;

   public:
      _mthRecordGenerator() ;
      ~_mthRecordGenerator() ;

   public:
      INT32 init( BOOLEAN isQueryUpdate ) ;
      INT32 resetValue( const BSONObj &src,
                        _mthMatchTreeContext *mthContext ) ;
      BOOLEAN hasNext() ;
      INT32 getNext( BSONObj &record ) ;
      INT32 getRecordNum() ;

      void popFront( UINT32 num ) ;
      void popTail( UINT32 num ) ;

      void setQueryModify( BOOLEAN isQueryModify ) ;
      void setDataPtr( ossValuePtr &dataPtr ) ;
      void getDataPtr( ossValuePtr &dataPtr ) ;

   private:
      INT32 _createArrayObj( const CHAR* name,
                             _utilArray< INT32 > &elements,
                             BSONObjBuilder &builder ) ;

      INT32 _replaceFieldObject( BSONObjIteratorSorted &iterSort,
                                 const CHAR *fieldName, BSONElement &newValue,
                                 BSONObjBuilder &builder ) ;

      INT32 _replaceFieldArray( BSONObjIteratorSorted &iterSort,
                                const CHAR *fieldName, BSONElement &newValue,
                                BSONArrayBuilder &builder ) ;

      INT32 _replaceField( BSONObj &src, const CHAR *fieldName,
                           BSONElement &newValue, BSONObjBuilder &builder ) ;

   private:
      BOOLEAN _isQueryModify ;
      ossValuePtr _dataPtr ;

      _mthMatchTreeContext *_mthContext ;
      const CHAR *_fieldName ;
      BSONObj _src ;
      BSONObj _arrayObj ;
      UINT32 _index ;
      UINT32 _totalNum ;

      UINT32 _validNum ;

      EN_MATCH_RECORD_SRC_TYPE _srcType ;

      //****_srcType = SRC_TYPE_ORIGINAL_SPLIT ****
      BSONObj _specifyObj ;
      BSONObjIterator _specifyIter ;
      //*******************************************
   } ;


}

#endif

