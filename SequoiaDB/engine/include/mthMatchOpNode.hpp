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

   Source File Name = mthMatchOpNode.hpp

   Descriptive Name = Method Match Operation Node Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for matching
   operation, which is indicating whether a record matches a given matching
   rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2016  LinYouBin    Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_MATCH_OPNODE_HPP_
#define MTH_MATCH_OPNODE_HPP_
#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilList.hpp"
#include <set>
#include <boost/shared_ptr.hpp>
#include "../bson/bson.hpp"
#include "mthMatchNode.hpp"
#include "mthCommon.hpp"
#include <vector>
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "pcrecpp.h"

using namespace bson ;
using namespace pcrecpp ;
using namespace std ;

namespace engine
{
   class _mthMatchFunc
   {
      public:
         _mthMatchFunc( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFunc() ;

      public:
         INT32 init( const CHAR *fieldName, const BSONElement &ele ) ;
         BSONObj toBson() ;
         string toString() ;

         void* operator new ( size_t size, _mthNodeAllocator *allocator ) ;
         void operator delete ( void *p ) ;
         void operator delete ( void *p, _mthNodeAllocator *allocator ) ;


         virtual void release() = 0 ;

      public:
         virtual void clear() ;
         virtual INT32 getType() = 0 ;
         virtual const CHAR* getName() = 0 ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) = 0 ;

         virtual INT32 adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                  _utilArray< INT32 > &out ) ;

         virtual UINT32 getEvalCPUCost () const
         {
            return OPT_MTH_FUNC_DEF_CPU_COST ;
         }

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) = 0 ;

      protected:
         _mthMatchFieldName<> _fieldName ;
         BSONElement _funcEle ;
         _mthNodeAllocator *_allocator ;
   } ;

   class _mthMatchFuncABS : public _mthMatchFunc
   {
      public:
         _mthMatchFuncABS( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncABS() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual void clear() ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;
   } ;

   class _mthMatchFuncCEILING : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncCEILING( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncCEILING() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncFLOOR : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncFLOOR( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncFLOOR() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncLOWER : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncLOWER( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncLOWER() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncUPPER : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncUPPER( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncUPPER() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncLTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncLTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncLTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncRTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncRTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncRTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncTRIM : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncTRIM( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncTRIM() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncSTRLEN : public _mthMatchFuncABS
   {
      public:
         _mthMatchFuncSTRLEN( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSTRLEN() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
   } ;

   class _mthMatchFuncSUBSTR : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSUBSTR( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSUBSTR() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _begin ;
         INT32 _limit ;
   } ;

   class _mthMatchFuncMOD : public _mthMatchFunc
   {
      public:
         _mthMatchFuncMOD( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncMOD() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncADD : public _mthMatchFunc
   {
      public:
         _mthMatchFuncADD( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncADD() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncSUBTRACT : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSUBTRACT( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSUBTRACT() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncMULTIPLY : public _mthMatchFunc
   {
      public:
         _mthMatchFuncMULTIPLY( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncMULTIPLY() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncDIVIDE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncDIVIDE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncDIVIDE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncCAST : public _mthMatchFunc
   {
      public:
         _mthMatchFuncCAST( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncCAST() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         BSONType _castType ;
   } ;

   class _mthMatchFuncSLICE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSLICE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSLICE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;
         virtual INT32 adjustIndexForReturnMatch( _utilArray< INT32 > &in,
                                                  _utilArray< INT32 > &out ) ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _begin ;
         INT32 _limit ;
   } ;

   class _mthMatchFuncSIZE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncSIZE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncSIZE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   class _mthMatchFuncTYPE : public _mthMatchFunc
   {
      public:
         _mthMatchFuncTYPE( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncTYPE() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _resultType ;
   } ;

   class _mthMatchFuncRETURNMATCH : public _mthMatchFunc
   {
      public:
         _mthMatchFuncRETURNMATCH( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncRETURNMATCH() ;

      public:
         INT32 getOffset() ;
         INT32 getLen() ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
         INT32 _offset ;
         INT32 _len ;
   } ;

   class _mthMatchFuncEXPAND : public _mthMatchFunc
   {
      public:
         _mthMatchFuncEXPAND( _mthNodeAllocator *allocator ) ;
         virtual ~_mthMatchFuncEXPAND() ;

      public:
         const CHAR* getFieldName() ;
         void getElement( BSONElement &ele ) ;

      public:
         virtual void release() ;
         virtual INT32 call( const BSONElement &in, BSONObj &out ) ;
         virtual INT32 getType() ;
         virtual const CHAR* getName() ;
         virtual void clear() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &ele ) ;

      private:
   } ;

   typedef _utilList< _mthMatchFunc* > MTH_FUNC_LIST ;

   class _mthMatchOpNode : public _mthMatchNode
   {
      public:
         _mthMatchOpNode( _mthNodeAllocator *allocator,
                          const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNode() ;

      public: /* from parent */
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;

         virtual INT32 addChild( _mthMatchNode *child ) ;
         virtual void delChild( _mthMatchNode *child ) ;

         virtual void clear() ;

         virtual void setWeight( UINT32 weight ) ;

         virtual void evalEstimation ( const optCollectionStat *pCollectionStat,
                                       double &selectivity, UINT32 &cpuCost ) ;

         virtual INT32 calcPredicate( rtnPredicateSet &predicateSet,
                                      const rtnParamList * paramList ) ;

         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;

         virtual BOOLEAN isTotalConverted() ;

         virtual BOOLEAN hasDollarFieldName() ;

         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;

         OSS_INLINE virtual BSONObj toBson ()
         {
            static rtnParamList s_emptyParameters ;
            return _toBson( s_emptyParameters ) ;
         }

         OSS_INLINE virtual BSONObj toParamBson ( const rtnParamList &parameters )
         {
            return _toBson( parameters ) ;
         }

         OSS_INLINE virtual void setFuzzyOpType ( EN_MATCH_OP_FUNC_TYPE nodeType )
         {
         }

        OSS_INLINE virtual void setFuzzyIndex ( INT8 fuzzyIndex )
        {
        }

        OSS_INLINE virtual void setParamIndex ( INT8 paramIndex )
        {
           _paramIndex = paramIndex ;
        }

        virtual void setDoneByPred ( BOOLEAN doneByPred ) ;

      public:
         INT32 addFunc( _mthMatchFunc *func ) ;
         INT32 addFuncList( MTH_FUNC_LIST &funcList ) ;
         void getFuncList( MTH_FUNC_LIST &funcList ) ;
         BOOLEAN hasReturnMatch() ;

         virtual INT32 getBSONOpType () = 0 ;

      protected: /* from itself */
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) = 0 ;

         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element) ;
         virtual void _clear() ;

         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

         UINT32 _evalFuncCPUCost () ;

         OSS_INLINE virtual BOOLEAN _flagExpandRegex ()
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            return FALSE ;
         }

         virtual BSONObj _toBson ( const rtnParamList &parameters ) ;
         virtual void _toParamBson ( BSONObjBuilder &builder,
                                     const rtnParamList &parameters ) ;

      protected:
         INT32 _execute( const CHAR *pFieldName, const BSONObj &obj,
                         BOOLEAN isArrayObj,
                         _mthMatchTreeContext &context,
                         BOOLEAN &result,
                         BOOLEAN &gotUndefined ) ;

         INT32 _dollarMatches( const CHAR *pFieldName,
                               const BSONElement &element,
                               _mthMatchTreeContext &context,
                               BOOLEAN &result,
                               BOOLEAN &gotUndefined ) ;

         INT32 _calculateFuncs( const BSONElement &in, BSONObj &out ) ;

         INT32 _doFuncMatch( const BSONElement &original,
                             const BSONElement &matchTarget,
                             _mthMatchTreeContext &context,
                             BOOLEAN mixCmp,
                             BOOLEAN &matchResult ) ;

         INT32 _saveElement( _mthMatchTreeContext &context,  BOOLEAN isMatch,
                             INT32 index ) ;

         BOOLEAN _isNot() ;

         OSS_INLINE virtual INT8 _getFuzzyIndex () const
         {
            return -1 ;
         }

         OSS_INLINE BOOLEAN _canSelfParameterize () const
         {
            return ( _funcList.size() == 0 &&
                     !_isCompareField &&
                     !_hasDollarFieldName &&
                     !_hasReturnMatch &&
                     !_hasExpand ) ;
         }

         virtual INT32 _addPredicate ( rtnPredicateSet & predicateSet,
                                       const CHAR * fieldName,
                                       const rtnParamList * paramList ) ;

      protected:
         MTH_FUNC_LIST _funcList ;
         BSONElement _toMatch ;
         BOOLEAN _isCompareField ;
         const CHAR *_cmpFieldName ;
         BOOLEAN _hasDollarFieldName ;

         BOOLEAN _hasReturnMatch ;
         BOOLEAN _hasExpand ;
         INT32 _offset ;
         INT32 _len ;

         INT8 _paramIndex ;

         BOOLEAN _addedToPred ;
         BOOLEAN _doneByPred ;
   } ;

   #define MTH_FUZZY_TYPE_EXCLUSIVE    ( -1 )
   #define MTH_FUZZY_TYPE_INCLUSIVE    ( -2 )
   #define MTH_FUZZY_TYPE_FUZZY_EXC    ( -3 )
   #define MTH_FUZZY_TYPE_FUZZY_INC    ( -4 )

   extern BSONObj _mthFuzzyIncOptr ;
   extern BSONObj _mthFuzzyExcOptr ;

   class _mthMatchFuzzyOpNode : public _mthMatchOpNode
   {
      public :
         _mthMatchFuzzyOpNode ( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;

         virtual ~_mthMatchFuzzyOpNode () ;

      public :
         virtual BOOLEAN isTotalConverted () ;

         OSS_INLINE virtual INT32 getType ()
         {
            return _isExclusive() ? _getExcType() : _getIncType () ;
         }

         virtual INT32 getBSONOpType ()
         {
            return _isExclusive() ? _getExcBSONOpType() : _getIncBSONOpType() ;
         }

         virtual const CHAR *getOperatorStr ()
         {
            if ( _fuzzyOpType >= 0 )
            {
               return _getIncOperatorStr() ;
            }
            else if ( MTH_FUZZY_TYPE_INCLUSIVE == _fuzzyOpType ||
                      MTH_FUZZY_TYPE_FUZZY_INC == _fuzzyOpType )
            {
               return _getIncOperatorStr() ;
            }
            return _getExcOperatorStr() ;
         }

         virtual UINT32 getWeight ()
         {
            return _isExclusive() ? _getExcWeight() : _getIncWeight() ;
         }

         virtual void setFuzzyOpType ( EN_MATCH_OP_FUNC_TYPE nodeType ) ;

         OSS_INLINE virtual void setFuzzyIndex ( INT8 fuzzyIndex )
         {
            _fuzzyOpType = fuzzyIndex ;
         }

      protected :
         virtual BOOLEAN _isExclusive ()
         {
            return MTH_FUZZY_TYPE_EXCLUSIVE == _fuzzyOpType ;
         }

         virtual INT32 _valueMatch ( const BSONElement &left,
                                     const BSONElement &right,
                                     BOOLEAN mixCmp,
                                     _mthMatchTreeContext &context,
                                     BOOLEAN &result ) ;

         virtual INT32 _getIncType () = 0 ;
         virtual INT32 _getExcType () = 0 ;
         virtual INT32 _getIncBSONOpType () = 0 ;
         virtual INT32 _getExcBSONOpType () = 0 ;
         virtual const CHAR *_getIncOperatorStr () = 0 ;
         virtual const CHAR *_getExcOperatorStr () = 0 ;
         virtual UINT32 _getIncWeight () = 0 ;
         virtual UINT32 _getExcWeight () = 0 ;

         virtual INT32 _incValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) = 0 ;

         virtual INT32 _excValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) = 0 ;

         OSS_INLINE virtual INT8 _getFuzzyIndex () const
         {
            return ( _fuzzyOpType >= 0 ? _fuzzyOpType : -1 ) ;
         }

         virtual INT32 _addPredicate ( rtnPredicateSet & predicateSet,
                                       const CHAR * fieldName,
                                       const rtnParamList * paramList ) ;

         virtual void _toParamBson ( BSONObjBuilder &builder,
                                     const rtnParamList &parameters ) ;

      protected :
         INT8 _fuzzyOpType ;
   } ;

   class _mthMatchOpNodeET : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeET( _mthNodeAllocator *allocator,
                            const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeET() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;
   } ;

   class _mthMatchOpNodeNE : public _mthMatchOpNodeET
   {
      public:
         _mthMatchOpNodeNE( _mthNodeAllocator *allocator,
                            const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeNE() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;
         virtual INT32 execute( const BSONObj &obj,
                                _mthMatchTreeContext &context,
                                BOOLEAN &result ) ;
         virtual void release() ;

      protected :
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;
   } ;

   class _mthMatchOpNodeLT : public _mthMatchFuzzyOpNode
   {
      public:
         _mthMatchOpNodeLT( _mthNodeAllocator *allocator,
                            const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeLT() ;

      public:
         virtual void release() ;

      protected:
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

         OSS_INLINE virtual INT32 _getIncType ()
         {
            return (INT32)EN_MATCH_OPERATOR_LTE ;
         }

         OSS_INLINE virtual INT32 _getExcType ()
         {
            return (INT32)EN_MATCH_OPERATOR_LT ;
         }

         OSS_INLINE virtual INT32 _getIncBSONOpType ()
         {
            return BSONObj::LTE ;
         }

         OSS_INLINE virtual INT32 _getExcBSONOpType ()
         {
            return BSONObj::LT ;
         }

         OSS_INLINE virtual const CHAR *_getIncOperatorStr ()
         {
            return MTH_OPERATOR_STR_LTE ;
         }

         OSS_INLINE virtual const CHAR *_getExcOperatorStr ()
         {
            return MTH_OPERATOR_STR_LT ;
         }

         OSS_INLINE virtual UINT32 _getIncWeight ()
         {
            return MTH_WEIGHT_LTE ;
         }

         OSS_INLINE virtual UINT32 _getExcWeight ()
         {
            return MTH_WEIGHT_LT ;
         }

         virtual INT32 _incValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) ;

         virtual INT32 _excValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeGT : public _mthMatchFuzzyOpNode
   {
      public:
         _mthMatchOpNodeGT( _mthNodeAllocator *allocator,
                            const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeGT() ;

      public:
         virtual void release() ;

      protected:
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

         OSS_INLINE virtual INT32 _getIncType ()
         {
            return (INT32)EN_MATCH_OPERATOR_GTE ;
         }

         OSS_INLINE virtual INT32 _getExcType ()
         {
            return (INT32)EN_MATCH_OPERATOR_GT ;
         }

         OSS_INLINE virtual INT32 _getIncBSONOpType ()
         {
            return BSONObj::GTE ;
         }

         OSS_INLINE virtual INT32 _getExcBSONOpType ()
         {
            return BSONObj::GT ;
         }

         OSS_INLINE virtual const CHAR *_getIncOperatorStr ()
         {
            return MTH_OPERATOR_STR_GTE ;
         }

         OSS_INLINE virtual const CHAR *_getExcOperatorStr ()
         {
            return MTH_OPERATOR_STR_GT ;
         }

         OSS_INLINE virtual UINT32 _getIncWeight ()
         {
            return MTH_WEIGHT_GTE ;
         }

         OSS_INLINE virtual UINT32 _getExcWeight ()
         {
            return MTH_WEIGHT_GT ;
         }

         virtual INT32 _incValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) ;

         virtual INT32 _excValueMatch ( const BSONElement &left,
                                        const BSONElement &right,
                                        BOOLEAN mixCmp,
                                        _mthMatchTreeContext &context,
                                        BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeRegex ;

   class _mthMatchOpNodeIN : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeIN( _mthNodeAllocator *allocator,
                            const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeIN() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual void _clear() ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

      protected:
         BOOLEAN _isMatch( const BSONElement &ele ) ;

      protected:
         typedef set<BSONElement, element_cmp_lt> VALUE_SET ;
         set<BSONElement, element_cmp_lt> _valueSet ;

         typedef vector<_mthMatchOpNodeRegex *> REGEX_VECTOR ;
         REGEX_VECTOR _regexVector ;
   } ;

   class _mthMatchOpNodeNIN : public _mthMatchOpNodeIN
   {
      public:
         _mthMatchOpNodeNIN( _mthNodeAllocator *allocator,
                             const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeNIN() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
   } ;

   class _mthMatchOpNodeALL : public _mthMatchOpNodeIN
   {
      public:
         _mthMatchOpNodeALL( _mthNodeAllocator *allocator,
                             const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeALL() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual INT32 extraEqualityMatches( BSONObjBuilder &builder,
                                             const rtnParamList *parameters ) ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         INT32 _valueMatchWithReturnMatch(  const BSONElement &left,
                                            const BSONElement &right,
                                            _mthMatchTreeContext &context,
                                            BOOLEAN &result )  ;

         BOOLEAN _valueMatchNoReturnMatch( const BSONElement &left,
                                           const BSONElement &right ) ;

         BOOLEAN _isMatchSingle( const BSONElement &ele ) ;
   } ;

   class _mthMatchOpNodeEXISTS : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeEXISTS( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeEXISTS() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            return TRUE ;
         }
   } ;

   class _mthMatchOpNodeMOD : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeMOD( _mthNodeAllocator *allocator,
                             const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeMOD() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      protected:
         BOOLEAN _isModValid( const BSONElement &modmEle ) ;

      protected:
         BSONElement _mod ;
         BSONElement _modResult ;
   } ;

   class _mthMatchOpNodeTYPE : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeTYPE( _mthNodeAllocator *allocator,
                              const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeTYPE() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

      protected:
         INT32 _type ;
   } ;

   class _mthMatchOpNodeISNULL : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeISNULL( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeISNULL() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            return TRUE ;
         }
   } ;

   class _mthMatchOpNodeEXPAND : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeEXPAND( _mthNodeAllocator *allocator,
                                const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeEXPAND() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

         virtual INT32 calcPredicate( rtnPredicateSet &predicateSet,
                                      const rtnParamList * paramList ) ;

      protected:
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;

         OSS_INLINE virtual BOOLEAN _flagAcceptUndefined ()
         {
            return TRUE ;
         }
   } ;

   class _mthMatchTree ;
   class _mthMatchOpNodeELEMMATCH : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeELEMMATCH( _mthNodeAllocator *allocator,
                                   const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeELEMMATCH() ;

      public:
         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual UINT32 getWeight() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual void release() ;

      protected:
         virtual INT32 _init( const CHAR *fieldName,
                              const BSONElement &element ) ;
         virtual void _clear() ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

      protected:
         _mthMatchTree *_subTree ;
   } ;

   class _mthMatchOpNodeRegex : public _mthMatchOpNode
   {
      public:
         _mthMatchOpNodeRegex( _mthNodeAllocator *allocator,
                               const mthNodeConfig *config ) ;
         virtual ~_mthMatchOpNodeRegex() ;

      public:
         virtual INT32 init( const CHAR *fieldName,
                             const BSONElement &element ) ;

         INT32 init( const CHAR *fieldName, const CHAR *regex,
                     const CHAR *options ) ;

         virtual INT32 getType() ;
         virtual INT32 getBSONOpType () ;
         virtual const CHAR *getOperatorStr() ;
         virtual BOOLEAN isTotalConverted() ;
         virtual UINT32 getWeight() ;
         virtual void release() ;

      public:
         BOOLEAN matches( const BSONElement &ele ) ;

      protected:
         virtual void _clear() ;
         virtual BSONObj _toBson ( const rtnParamList &parameters ) ;
         virtual INT32 _valueMatch( const BSONElement &left,
                                    const BSONElement &right,
                                    BOOLEAN mixCmp,
                                    _mthMatchTreeContext &context,
                                    BOOLEAN &result ) ;
         virtual void _evalEstimation ( const optCollectionStat *pCollectionStat,
                                        double &selectivity, UINT32 &cpuCost ) ;

      private:
         pcrecpp::RE_Options _flags2options( const char* options ) ;
         BOOLEAN _isPureWords( const char* regex, const char* options ) ;

      private:
         const CHAR *_regex ;
         const CHAR *_options ;
         boost::shared_ptr<RE> _re ;
         BOOLEAN _isSimpleMatch ;
         BSONObj _matchObj ;
   } ;
}

#endif

