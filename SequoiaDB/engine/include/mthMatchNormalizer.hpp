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

   Source File Name = mthMatchNormalizer.hpp

   Descriptive Name = Method Match Normalizer Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for normalize
   matching operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/07/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_MATCHNORMALIZER_HPP_
#define MTH_MATCHNORMALIZER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"
#include "mthMatchNode.hpp"
#include "mthMatchOpNode.hpp"
#include "pd.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   #define MTH_NORMALIZER_ITEM_NUM  ( 32 )

   /*
      _mthMatchItemBase define
    */
   class _mthMatchItemBase : public SDBObject
   {
      public :
         _mthMatchItemBase () ;

         _mthMatchItemBase ( const CHAR *fieldName,
                             EN_MATCH_OP_FUNC_TYPE opCode,
                             const BSONElement &element ) ;

         _mthMatchItemBase ( const _mthMatchItemBase &item ) ;

         virtual ~_mthMatchItemBase () ;

         OSS_INLINE const BSONElement &getElement () const
         {
            return _element ;
         }

         OSS_INLINE const CHAR *getFieldName () const
         {
            return _fieldName ;
         }

         OSS_INLINE EN_MATCH_OP_FUNC_TYPE getOpCode () const
         {
            return _opCode ;
         }

      protected :
         const CHAR *            _fieldName ;
         EN_MATCH_OP_FUNC_TYPE   _opCode ;
         BSONElement             _element ;
   } ;

   /*
      _mthMatchFuncItem define
    */
   class _mthMatchFuncItem : public _mthMatchItemBase
   {
      public :
         _mthMatchFuncItem () ;

         _mthMatchFuncItem ( const CHAR *funcName,
                             EN_MATCH_OP_FUNC_TYPE opCode,
                             const BSONElement &element ) ;

         _mthMatchFuncItem ( const _mthMatchFuncItem &item ) ;

         virtual ~_mthMatchFuncItem () ;

         _mthMatchFuncItem &operator= ( const _mthMatchFuncItem &item ) ;

         bool operator< ( const _mthMatchFuncItem &item ) const ;

         INT32 normalize ( BSONObjBuilder &builder ) const ;
   } ;

   typedef class _mthMatchFuncItem mthMatchFuncItem ;
   typedef vector<mthMatchFuncItem> MTH_FUNC_OP_LIST ;
   /*
      _mthMatchOpItem define
    */
   class _mthMatchOpItem : public _mthMatchItemBase
   {
      public :
         _mthMatchOpItem () ;

         virtual ~_mthMatchOpItem () ;

         OSS_INLINE const CHAR *getRegex () const
         {
            return ( _opCode == EN_MATCH_OPERATOR_REGEX ) ? _opName : "" ;
         }

         OSS_INLINE const CHAR *getOptions () const
         {
            return ( _opCode == EN_MATCH_OPERATOR_REGEX ) ? _options : "" ;
         }

         OSS_INLINE const MTH_FUNC_OP_LIST &getFuncList () const
         {
            return _funcList ;
         }

         OSS_INLINE MTH_FUNC_OP_LIST &getFuncList ()
         {
            return _funcList ;
         }

         OSS_INLINE INT8 getParamIndex () const
         {
            return _paramIndex ;
         }

         OSS_INLINE INT8 getFuzzyIndex () const
         {
            return _fuzzyIndex ;
         }

         OSS_INLINE ossPoolString toString () const
         {
            return _element.toPoolString() ;
         }

         void setOpItem ( const CHAR *fieldName,
                          EN_MATCH_OP_FUNC_TYPE opCode,
                          const CHAR *opName,
                          const BSONElement &element,
                          MTH_FUNC_OP_LIST &funcList,
                          BOOLEAN fuzzyOptr ) ;

         void setRegexItem ( const CHAR *fieldName,
                             const CHAR *regex,
                             const CHAR *options,
                             MTH_FUNC_OP_LIST &funcList ) ;

         bool operator< ( const _mthMatchOpItem &item ) const ;

         INT32 comparator ( const _mthMatchOpItem &item ) const ;

         INT32 normalize ( const mthNodeConfig *config,
                           BSONArrayBuilder &builder,
                           rtnParamList &parameters ) ;

         OSS_INLINE _mthMatchOpNode *getOpNode ()
         {
            return _node ;
         }

         OSS_INLINE void setOpNode ( _mthMatchOpNode *node )
         {
            _node = node ;
         }

         void setDoneByPred () ;

      protected :
         BOOLEAN _canDoneByPred () const ;
         BOOLEAN _canParameterize () const ;
         BOOLEAN _canFuzzyOptr () const ;
         BOOLEAN _isExclusiveOperator () const ;
         const CHAR *_getFuzzyOpStr () const ;
         UINT32 _getOPWeight ( BOOLEAN fuzzyOptr ) const ;

      protected :
         INT32 _normalizeFunctions ( BSONObjBuilder & subBuilder ) ;
         INT32 _normalizeREGEX ( BSONObjBuilder & subBuilder ) ;
         INT32 _normalizeOPTR ( const mthNodeConfig * config,
                                BSONObjBuilder & subBuilder,
                                rtnParamList & parameters ) ;
         INT32 _normalizeItem ( BSONObjBuilder & subBuilder ) ;

      protected :
         UINT32                        _opWeight ;
         MTH_FUNC_OP_LIST              _funcList ;
         const CHAR *                  _opName ;
         const CHAR *                  _options ;
         INT8                          _paramIndex ;
         INT8                          _fuzzyIndex ;
         _mthMatchOpNode *             _node ;
   } ;

   typedef class _mthMatchOpItem mthMatchOpItem ;

   INT32 mthMatchOpItemComparator ( const void *a, const void *b ) ;

   /*
      _mthMatchNormalizer define
    */
   class _mthMatchNormalizer : public _mthMatchConfig
   {
      public :
         _mthMatchNormalizer ( const mthNodeConfig *config ) ;

         virtual ~_mthMatchNormalizer () ;

         void clear () ;

         INT32 normalize ( const BSONObj &matcher,
                           BSONObjBuilder &normalBuilder,
                           rtnParamList &parameters ) ;

         void setDoneByPred ( const BSONObj &keyPattern, UINT32 addedLevel ) ;

         OSS_INLINE BOOLEAN isInvalidMatcher () const
         {
            return _invalidMatcher ;
         }

         OSS_INLINE UINT8 getItemNumber () const
         {
            return _itemNumber ;
         }

         OSS_INLINE mthMatchOpItem *getOpItem ( UINT8 index )
         {
            if ( index < _itemNumber )
            {
               return _itemIndexes[ index ] ;
            }
            return NULL ;
         }

      protected :
         INT32 _parseAndOp ( const BSONElement &element ) ;

         INT32 _parseObject ( const BSONObj &object ) ;

         INT32 _parseInnerObject ( const CHAR *fieldName,
                                   const BSONElement &element ) ;
         INT32 _normalize ( BSONObjBuilder &normalBuilder,
                            rtnParamList &parameters ) ;

         INT32 _addOpItem ( const CHAR *fieldName,
                            EN_MATCH_OP_FUNC_TYPE opCode,
                            const CHAR *opName,
                            const BSONElement &element,
                            MTH_FUNC_OP_LIST &funcList ) ;

         INT32 _addRegexItem ( const CHAR *fieldName,
                               const CHAR *regex,
                               const CHAR *options,
                               MTH_FUNC_OP_LIST &funcList ) ;

      protected :
         UINT8             _itemNumber ;
         mthMatchOpItem *  _itemIndexes[ MTH_NORMALIZER_ITEM_NUM ] ;
         mthMatchOpItem    _items[ MTH_NORMALIZER_ITEM_NUM ] ;
         BOOLEAN           _invalidMatcher ;
   } ;

   typedef class _mthMatchNormalizer mthMatchNormalizer ;

}

#endif // MTH_MATCHNORMALIZER_HPP_
