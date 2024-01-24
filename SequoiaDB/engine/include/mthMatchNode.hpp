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

   Source File Name = mthMatchNode.hpp

   Descriptive Name = Method MatchNode Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCHNODE_HPP_
#define MTH_MATCHNODE_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "utilMemListPool.hpp"
#include "ossUtil.hpp"
#include "mthCommon.hpp"
#include "../bson/bson.hpp"
#include "utilArray.hpp"
#include "rtnPredicate.hpp"
#include "optCommon.hpp"
#include "optStatUnit.hpp"
#include "utilAllocator.hpp"
#include <vector>

using namespace bson ;
using namespace std ;

namespace engine
{
   enum EN_MATCH_OP_FUNC_TYPE
   {
      EN_MATCH_OPERATOR_LOGIC_AND      = 0,
      EN_MATCH_OPERATOR_LOGIC_OR,
      EN_MATCH_OPERATOR_LOGIC_NOT,

      //logic end
      EN_MATCH_OPERATOR_LOGIC_END      = 10,

      EN_MATCH_OPERATOR_ET             = 11,
      EN_MATCH_OPERATOR_LT             = 12,
      EN_MATCH_OPERATOR_LTE            = 13,
      EN_MATCH_OPERATOR_GTE            = 14,
      EN_MATCH_OPERATOR_GT             = 15,
      EN_MATCH_OPERATOR_IN             = 16,
      EN_MATCH_OPERATOR_NE             = 17,
      //EN_MATCH_OPERATOR_SIZE           = 18,   //deleted
      EN_MATCH_OPERATOR_ALL            = 19,
      EN_MATCH_OPERATOR_NIN            = 20,
      EN_MATCH_OPERATOR_EXISTS         = 21,
      EN_MATCH_OPERATOR_MOD            = 22,
      EN_MATCH_OPERATOR_TYPE           = 23,
      EN_MATCH_OPERATOR_REGEX          = 24,
      EN_MATCH_OPERATOR_OPTIONS        = 25, /*do not have really matchNode*/
      EN_MATCH_OPERATOR_ELEMMATCH      = 26,
      EN_MATCH_OPERATOR_ISNULL         = 27,
      EN_MATCH_OPERATOR_FIELD          = 28, /*do not have really matchNode*/

      EN_MATCH_OPERATOR_END            = 100,

      EN_MATCH_FUNC_ABS                = 101,
      EN_MATCH_FUNC_CEILING            = 102,
      EN_MATCH_FUNC_FLOOR              = 103,
      EN_MATCH_FUNC_MOD                = 104,
      EN_MATCH_FUNC_ADD                = 105,
      EN_MATCH_FUNC_SUBTRACT           = 106,
      EN_MATCH_FUNC_MULTIPLY           = 107,
      EN_MATCH_FUNC_DIVIDE             = 108,
      EN_MATCH_FUNC_SUBSTR             = 109,
      EN_MATCH_FUNC_STRLEN             = 110,
      EN_MATCH_FUNC_LOWER              = 111,
      EN_MATCH_FUNC_UPPER              = 112,
      EN_MATCH_FUNC_LTRIM              = 113,
      EN_MATCH_FUNC_RTRIM              = 114,
      EN_MATCH_FUNC_TRIM               = 115,
      EN_MATCH_FUNC_CAST               = 116,
      EN_MATCH_FUNC_SLICE              = 117,
      EN_MATCH_FUNC_SIZE               = 118,
      EN_MATCH_FUNC_TYPE               = 119,
      EN_MATCH_FUNC_STRLENBYTES        = 120,
      EN_MATCH_FUNC_STRLENCP           = 121,
      EN_MATCH_FUNC_KEYSTRING          = 122,

      EN_MATCH_FUNC_END                = 200,

      EN_MATCH_ATTR_RETURNMATCH        = 201,
      EN_MATCH_ATTR_EXPAND             = 202,

      EN_MATCH_OP_FUNC_END             = 250,
   } ;

   #define MTH_OPERATOR_STR_AND                 "$and"
   #define MTH_OPERATOR_STR_OR                  "$or"
   #define MTH_OPERATOR_STR_NOT                 "$not"
   #define MTH_OPERATOR_STR_ET                  "$et"
   #define MTH_OPERATOR_STR_LT                  "$lt"
   #define MTH_OPERATOR_STR_LTE                 "$lte"
   #define MTH_OPERATOR_STR_GTE                 "$gte"
   #define MTH_OPERATOR_STR_GT                  "$gt"
   #define MTH_OPERATOR_STR_IN                  "$in"
   #define MTH_OPERATOR_STR_NE                  "$ne"
   #define MTH_OPERATOR_STR_ALL                 "$all"
   #define MTH_OPERATOR_STR_NIN                 "$nin"
   #define MTH_OPERATOR_STR_EXISTS              "$exists"
   #define MTH_OPERATOR_STR_MOD                 "$mod"
   #define MTH_OPERATOR_STR_TYPE                "$type"
   #define MTH_OPERATOR_STR_ELEMMATCH           "$elemMatch"
   #define MTH_OPERATOR_STR_ISNULL              "$isnull"
   #define MTH_OPERATOR_STR_FIELD               "$field"
   #define MTH_OPERATOR_STR_REGEX               "$regex"
   #define MTH_OPERATOR_STR_OPTIONS             "$options"

   //only impact array
   #define MTH_ATTR_STR_EXPAND                  "$expand"
   #define MTH_ATTR_STR_RETURNMATCH             "$returnMatch"

   #define MTH_FUNCTION_STR_ABS                 "$abs"
   #define MTH_FUNCTION_STR_CEILING             "$ceiling"
   #define MTH_FUNCTION_STR_FLOOR               "$floor"
   #define MTH_FUNCTION_STR_MOD                 "$mod"
   #define MTH_FUNCTION_STR_ADD                 "$add"
   #define MTH_FUNCTION_STR_SUBTRACT            "$subtract"
   #define MTH_FUNCTION_STR_MULTIPLY            "$multiply"
   #define MTH_FUNCTION_STR_DIVIDE              "$divide"
   #define MTH_FUNCTION_STR_SUBSTR              "$substr"
   #define MTH_FUNCTION_STR_STRLEN              "$strlen"
   #define MTH_FUNCTION_STR_LOWER               "$lower"
   #define MTH_FUNCTION_STR_UPPER               "$upper"
   #define MTH_FUNCTION_STR_LTRIM               "$ltrim"
   #define MTH_FUNCTION_STR_RTRIM               "$rtrim"
   #define MTH_FUNCTION_STR_TRIM                "$trim"
   #define MTH_FUNCTION_STR_CAST                "$cast"
   #define MTH_FUNCTION_STR_SLICE               "$slice"
   #define MTH_FUNCTION_STR_SIZE                "$size"
   #define MTH_FUNCTION_STR_TYPE                "$type"
   #define MTH_FUNCTION_STR_STRLENBYTES         "$strlenBytes"
   #define MTH_FUNCTION_STR_STRLENCP            "$strlenCP"
   #define MTH_FUNCTION_STR_KEYSTRING           "$keyString"

   #define MTH_ALLOCATOR_SIZE                   2048
   #define MTH_MATCH_FIELD_STATIC_NAME_LEN      32
   #define MTH_FIELDNAME_SEP                    '.'

   #define MTH_WEIGHT_EQUAL                     100
   #define MTH_WEIGHT_GT                        150
   #define MTH_WEIGHT_GTE                       200
   #define MTH_WEIGHT_LT                        150
   #define MTH_WEIGHT_LTE                       200
   #define MTH_WEIGHT_NE                        500
   #define MTH_WEIGHT_MOD                       200
   #define MTH_WEIGHT_TYPE                      150
   #define MTH_WEIGHT_IN                        5000
   #define MTH_WEIGHT_NIN                       10000
   #define MTH_WEIGHT_ALL                       550
   #define MTH_WEIGHT_SIZE                      250
   #define MTH_WEIGHT_EXISTS                    250
   #define MTH_WEIGHT_ELEMMATCH                 450
   #define MTH_WEIGHT_REGEX                     1000000
   #define MTH_WEIGHT_ISNULL                    250

   #define MTH_FIELD_CONFIG                  "MatchConfig"
   #define MTH_FIELD_CONF_MIXCMP             "EnableMixCmp"
   #define MTH_FIELD_CONF_PARAM              "Parameterized"
   #define MTH_FIELD_CONF_FUZZYOPTR          "FuzzyOptr"

   template <UINT32 nameSize = MTH_MATCH_FIELD_STATIC_NAME_LEN >
   class _mthMatchFieldName
   {
      public:
         _mthMatchFieldName():_name( NULL ), _allocateName( NULL )
         {
         }

         ~_mthMatchFieldName()
         {
            clear() ;
         }

      public:
         INT32 setFieldName( const CHAR *name )
         {
            INT32 rc   = SDB_OK ;
            UINT32 len = 0 ;

            if ( NULL == name )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            len = ossStrlen( name ) ;
            if ( len <= nameSize )
            {
               ossStrcpy( _staticName, name ) ;
               _staticName[ nameSize ] = '\0' ;

               _name = _staticName ;
            }
            else
            {
               CHAR *tmp = (CHAR *)SDB_THREAD_ALLOC( len + 1 ) ;
               if ( NULL == tmp )
               {
                  rc = SDB_OOM ;
                  goto error ;
               }

               ossStrcpy( tmp, name ) ;

               if ( _allocateName )
               {
                  SDB_THREAD_FREE( _allocateName ) ;
               }

               _allocateName = tmp ;
               _name         = _allocateName ;
            }

         done:
            return rc ;
         error:
            goto done ;
         }

         const CHAR *getFieldName()
         {
            return _name ;
         }

         void clear()
         {
            if ( _allocateName )
            {
               SDB_THREAD_FREE( _allocateName ) ;
            }

            _name = NULL ;
         }

      private:
         CHAR *_name ;
         CHAR _staticName[ nameSize + 1 ] ;
         CHAR *_allocateName ;
   } ;

   /*
      _IMthMatchEventHandler define
    */
   class _IMthMatchEventHandler
   {
   public:
      _IMthMatchEventHandler() {}
      virtual ~_IMthMatchEventHandler() {}
      virtual INT32 onMatchDone( const bson::BSONObj &record,
                                 BOOLEAN isMatched ) = 0 ;
   } ;
   typedef class _IMthMatchEventHandler IMthMatchEventHandler ;

   /*
      _mthMatchTreeContext define
    */
   class _mthMatchTreeContext
   {
      public:
         _mthMatchTreeContext( IMthMatchEventHandler * eventHandler = NULL ) ;
         ~_mthMatchTreeContext() ;

         void clear() ;
         void clearRecordInfo() ;

         //****array attribute relate*******
         INT32 getDollarResult( INT32 index, INT32 &value ) ;

         INT32 setFieldName( const CHAR *name ) ;
         const CHAR *getFieldName() ;
         INT32 resolveFieldName() ;
         INT32 saveElement( INT32 index ) ;
         INT32 subElements( INT32 offset, INT32 len ) ;

         BOOLEAN hasExpand() ;
         void setHasExpand( BOOLEAN hasExpand ) ;

         BOOLEAN hasReturnMatch() ;
         void setHasReturnMatch( BOOLEAN hasReturnMatch ) ;

         BOOLEAN isUseElement() ;
         void setIsUseElement( BOOLEAN isUseElement ) ;

         BOOLEAN isReturnMatchExecuted() ;
         void setReturnMatchExecuted( BOOLEAN isExecuted ) ;

         INT32 getRecordNum() ;

         void setObj( const BSONObj &obj ) ;

         void enableDollarList() ;
         void disableDollarList() ;
         BOOLEAN isDollarListEnabled() ;
         void appendDollarList( vector<INT64> &dollarList ) ;
         void getDollarList( vector<INT64> *dollarList ) ;

         string toString() ;

         OSS_INLINE void bindParameters ( rtnParamList *parameters )
         {
            _parameters = parameters ;
         }

         BSONElement getParameter ( INT8 index ) ;
         const RTN_ELEMENT_SET * getParamValueSet ( INT8 index ) ;
         BOOLEAN paramDoneByPred ( INT8 index ) ;

         // events
         INT32 onMatchDone( const bson::BSONObj &record, BOOLEAN isMatched )
         {
            return ( NULL != _eventHandler ) ?
                   ( _eventHandler->onMatchDone( record, isMatched ) ) :
                   ( SDB_OK ) ;
         }

      private:
         INT32 _replaceDollar() ;

      public:
         BOOLEAN _isDollarListEnabled ;
         vector<INT64> _dollarList ;

         BSONObj _originalObj ;

         /* array attribute relate */
         BOOLEAN _hasExpand ;
         BOOLEAN _hasReturnMatch ;
         _mthMatchFieldName<> _fieldName ;

         // if returnMatch executed or not, if not we should execute it
         BOOLEAN _isReturnMatchExecuted ;

         // if we should use _elements as result or not
         BOOLEAN _isUseElement ;

         // record the array's index
         _utilArray< INT32 > _elements ;
         /* array attribute relate */

      protected :
         // parameters
         rtnParamList *_parameters ;
         IMthMatchEventHandler * _eventHandler ;
   } ;

   void mthContextClearRecordInfoSafe( _mthMatchTreeContext *context ) ;

   class _mthMatchNode ;
   typedef ossPoolVector< _mthMatchNode* >   MATCHNODE_VECTOR ;

   class _mthMatchNodeIterator : public SDBObject
   {
      public:
         _mthMatchNodeIterator( _mthMatchNode *node ) ;
         ~_mthMatchNodeIterator() ;

      private:
         _mthMatchNodeIterator() ;
         _mthMatchNodeIterator( const _mthMatchNodeIterator &right ) ;

      public:
         BOOLEAN more() ;
         _mthMatchNode *next() ;

      private:
         _mthMatchNode *_node ;
         UINT32 _index ;
   } ;

   // Memory for _mthMatchNode may be allocated in two ways:
   // 1. By using malloc.
   // 2. By using user specified allocator(instances of _mthNodeAllocator).
   // The actions when releasing these two kinds of node are different.
   // So a type flag is added at the head of the actual allocated memory.
   // NOTE: we only use 4 bytes in memory header, but for arm64, it requires
   //       8 bytes align for atomic variables inside struct or class, so
   //       we make the memory header 8 bytes here
   #define MTH_MEM_TYPE_SIZE            sizeof(INT64)
   #define MTH_MEM_BY_USER_ALLOCATOR    0
   #define MTH_MEM_BY_DFT_ALLOCATOR     1

   // Allocator for _mthMatchNode
   typedef _utilAllocator<MTH_ALLOCATOR_SIZE> _mthNodeAllocator ;

   // Configure arguments for _mthMatchNode
   typedef struct _mthNodeConfig
   {
      _mthNodeConfig ()
      {
         _enableMixCmp = FALSE ;
         _enableParameterized = FALSE ;
         _enableFuzzyOptr = FALSE ;
      }

      BOOLEAN _enableMixCmp ;
      BOOLEAN _enableParameterized ;
      BOOLEAN _enableFuzzyOptr ;
   } mthNodeConfig ;

   const mthNodeConfig *mthGetDefaultNodeConfigPtr () ;
   const mthNodeConfig &mthGetDefaultNodEConfig () ;

   // Wrapper for mthNodeConfig
   class _mthMatchConfig
   {
      public :
         _mthMatchConfig () ;
         _mthMatchConfig ( const mthNodeConfig *configPtr ) ;
         ~_mthMatchConfig () ;

      public :
         OSS_INLINE const mthNodeConfig *getMatchConfigPtr () const
         {
            return _matchConfig ;
         }

         OSS_INLINE BOOLEAN mthEnabledMixCmp () const
         {
            return _matchConfig->_enableMixCmp ;
         }

         OSS_INLINE BOOLEAN mthEnabledParameterized () const
         {
            return _matchConfig->_enableParameterized ;
         }

         OSS_INLINE BOOLEAN mthEnabledFuzzyOptr () const
         {
            return _matchConfig->_enableFuzzyOptr ;
         }

         OSS_INLINE INT32 confToBSON ( BSONObjBuilder &builder ) const
         {
            INT32 rc = SDB_OK ;

            BSONObjBuilder confBuilder(
                           builder.subobjStart( MTH_FIELD_CONFIG ) ) ;
            confBuilder.appendBool( MTH_FIELD_CONF_MIXCMP,
                                    mthEnabledMixCmp() ) ;
            confBuilder.appendBool( MTH_FIELD_CONF_PARAM,
                                    mthEnabledParameterized() ) ;
            confBuilder.appendBool( MTH_FIELD_CONF_FUZZYOPTR,
                                    mthEnabledFuzzyOptr() ) ;
            confBuilder.done() ;

            return rc ;
         }

      protected :
         const mthNodeConfig * _matchConfig ;
   } ;

   class _mthMatchConfigHolder : public _mthMatchConfig
   {
      public :
         _mthMatchConfigHolder () ;
         _mthMatchConfigHolder ( const mthNodeConfig &config ) ;
         ~_mthMatchConfigHolder () ;

      public :
         OSS_INLINE const mthNodeConfig &getMatchConfig () const
         {
            return _stackMatchConfig ;
         }

         OSS_INLINE mthNodeConfig &getMatchConfig ()
         {
            return _stackMatchConfig ;
         }

         OSS_INLINE void setMatchConfig ( const mthNodeConfig &config )
         {
            _stackMatchConfig._enableMixCmp = config._enableMixCmp ;
            _stackMatchConfig._enableParameterized = config._enableParameterized ;
            _stackMatchConfig._enableFuzzyOptr = config._enableFuzzyOptr ;
         }

         OSS_INLINE void setMthEnableMixCmp ( BOOLEAN enableMixCmp )
         {
            _stackMatchConfig._enableMixCmp = enableMixCmp ;
         }

         OSS_INLINE void setMthEnableParameterized ( BOOLEAN enableParameterized )
         {
            _stackMatchConfig._enableParameterized = enableParameterized ;
         }

         OSS_INLINE void setMthEnableFuzzyOptr ( BOOLEAN enableFuzzyOptr )
         {
            _stackMatchConfig._enableFuzzyOptr = enableFuzzyOptr ;
         }

      protected :
         mthNodeConfig _stackMatchConfig ;
   } ;

   class _mthMatchNode : public _mthMatchConfig
   {
      friend class _mthMatchNodeIterator ;
      public:
         _mthMatchNode( _mthNodeAllocator *allocator,
                        const mthNodeConfig *config ) ;
         virtual ~_mthMatchNode() ;

      public:
         // node *p = new ( _mthNodeAllocator *allocator ) node( _mthNodeAllocator *allocator )
         void* operator new ( size_t size, _mthNodeAllocator *allocator ) ;

         void operator delete ( void *p ) ;
         void operator delete ( void *p, _mthNodeAllocator *allocator ) ;

      public:
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;

         //just clear parameter itself
         virtual void clear() ;

         virtual INT32 getType() = 0 ;

         virtual const CHAR* getOperatorStr() = 0 ;

         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) = 0 ;
         virtual BSONObj toBson() = 0 ;

         virtual BSONObj toParamBson ( const rtnParamList &parameters ) = 0 ;

         virtual void setWeight( UINT32 weight ) = 0 ;

         virtual UINT32 getWeight() = 0 ;

         virtual void evalEstimation ( const optCollectionStat *pCollectionStat,
                                       double &selectivity, UINT32 &cpuCost ) = 0 ;

         virtual BOOLEAN isTotalConverted() = 0 ;

         virtual BOOLEAN hasDollarFieldName() ;

         virtual INT32 calcPredicate ( rtnPredicateSet &predicateSet,
                                       const rtnParamList * paramList ) ;

         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;

         virtual INT32 addChild( _mthMatchNode *child ) ;
         virtual void delChild( _mthMatchNode *child ) ;

         virtual INT32 getName ( IXM_FIELD_NAME_SET& nameSet ) = 0 ;

         void sortByWeight() ;

      public:
         const CHAR *getFieldName() ;
         string toString() ;

         void rollIsUnderLogicNot() ;

         BOOLEAN isUnderLogicNot() ;

         UINT32 getChildrenCount() ;

         _mthMatchNode *getParent() ;

      protected:
         _mthNodeAllocator *_allocator ;

         _mthMatchNode *_parent ;
         UINT32 _idx_in_parent ;
         _mthMatchFieldName<> _fieldName ;
         MATCHNODE_VECTOR _children ;

         //under two logicNot will revert to false
         BOOLEAN _isUnderLogicNot ;
   } ;

   BOOLEAN mthCompareNode ( _mthMatchNode *left, _mthMatchNode *right ) ;
}

#endif


