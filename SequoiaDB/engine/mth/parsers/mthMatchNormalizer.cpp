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

   Source File Name = mthMatchNormalizer.cpp

   Descriptive Name = Method Matcher Normalizer

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for normalize
   matcher.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/07/2017  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthMatchNormalizer.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "rtnCB.hpp"
#include "msgDef.hpp"

using namespace bson ;

namespace engine
{

   /*
      _mthMatchItemBase implement
    */
   _mthMatchItemBase::_mthMatchItemBase ()
   : _fieldName( NULL ),
     _opCode( EN_MATCH_OPERATOR_ET )
   {
   }

   _mthMatchItemBase::_mthMatchItemBase ( const CHAR *fieldName,
                                          EN_MATCH_OP_FUNC_TYPE opCode,
                                          const BSONElement &element )
   : _fieldName( fieldName ),
     _opCode( opCode ),
     _element( element )
   {
   }

   _mthMatchItemBase::_mthMatchItemBase ( const _mthMatchItemBase &item )
   : _fieldName( item._fieldName ),
     _opCode( item._opCode ),
     _element( item._element )
   {
   }

   _mthMatchItemBase::~_mthMatchItemBase ()
   {
   }

   /*
      _mthMatchFuncItem implement
    */
   _mthMatchFuncItem::_mthMatchFuncItem ()
   : _mthMatchItemBase()
   {
   }

   _mthMatchFuncItem::_mthMatchFuncItem ( const CHAR *funcName,
                                          EN_MATCH_OP_FUNC_TYPE opCode,
                                          const BSONElement &element )
   : _mthMatchItemBase( funcName, opCode, element )
   {
      SDB_ASSERT( opCode >= EN_MATCH_FUNC_ABS && opCode < EN_MATCH_FUNC_END,
                  "opCode is invalid" ) ;
   }

   _mthMatchFuncItem::_mthMatchFuncItem ( const _mthMatchFuncItem &item )
   : _mthMatchItemBase( item )
   {
   }

   _mthMatchFuncItem::~_mthMatchFuncItem ()
   {
   }

   _mthMatchFuncItem &_mthMatchFuncItem::operator= ( const _mthMatchFuncItem &item )
   {
      _fieldName = item._fieldName ;
      _opCode = item._opCode ;
      _element = item._element ;
      return (*this) ;
   }

   bool _mthMatchFuncItem::operator< ( const _mthMatchFuncItem &item ) const
   {
      if ( _opCode < item._opCode )
      {
         return TRUE ;
      }
      else if ( _opCode > item._opCode )
      {
         return FALSE ;
      }
      return _element.woCompare( item._element, FALSE ) < 0 ;
   }

   INT32 _mthMatchFuncItem::normalize ( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;
      builder.appendAs( _element, _fieldName ) ;
      return rc ;
   }

   /*
      _mthMatchOpItem implement
    */
   _mthMatchOpItem::_mthMatchOpItem ()
   : _mthMatchItemBase(),
     _opWeight( 0 ),
     _opName( NULL ),
     _options( NULL ),
     _paramIndex( -1 ),
     _fuzzyIndex( -1 ),
     _node( NULL )
   {
   }

   _mthMatchOpItem::~_mthMatchOpItem ()
   {
   }

   void _mthMatchOpItem::setOpItem ( const CHAR *fieldName,
                                     EN_MATCH_OP_FUNC_TYPE opCode,
                                     const CHAR *opName,
                                     const BSONElement &element,
                                     MTH_FUNC_OP_LIST &funcList,
                                     BOOLEAN fuzzyOptr )
   {
      _fieldName = fieldName ;
      _opCode = opCode ;
      _opName = opName ;
      _element = element ;
      _options = NULL ;
      _opWeight = _getOPWeight( fuzzyOptr ) ;
      _funcList = funcList ;
      _paramIndex = -1 ;
      _fuzzyIndex = -1 ;
      _node = NULL ;
   }

   void _mthMatchOpItem::setRegexItem ( const CHAR *fieldName,
                                        const CHAR *regex,
                                        const CHAR *options,
                                        MTH_FUNC_OP_LIST &funcList )
   {
      _fieldName = fieldName ;
      _opCode = EN_MATCH_OPERATOR_REGEX ;
      // Reuse _opName as regex string
      _opName = regex ;
      _options = options ;
      _opWeight = MTH_WEIGHT_REGEX ;
      _funcList = funcList ;
      _paramIndex = -1 ;
      _fuzzyIndex = -1 ;
      _node = NULL ;
   }

   bool _mthMatchOpItem::operator< ( const _mthMatchOpItem &item ) const
   {
      // Compare order:
      // 1. weight of operator
      // 2. name of field
      // 3. code of operator
      if ( _opWeight < item._opWeight )
      {
         return TRUE ;
      }
      else if ( _opWeight > item._opWeight )
      {
         return FALSE ;
      }
      else
      {
         INT32 res = ossStrcmp( _fieldName, item._fieldName ) ;
         if ( res < 0 )
         {
            return TRUE ;
         }
         else if ( res > 0 )
         {
            return FALSE ;
         }
      }

      return _opCode < item._opCode ;
   }

   INT32 _mthMatchOpItem::comparator ( const _mthMatchOpItem &item ) const
   {
      // Compare order:
      // 1. weight of operator
      // 2. name of field
      // 3. code of operator
      if ( _opWeight < item._opWeight )
      {
         return -1 ;
      }
      else if ( _opWeight > item._opWeight )
      {
         return 1 ;
      }
      else
      {
         INT32 res = ossStrcmp( _fieldName, item._fieldName ) ;
         if ( res < 0 )
         {
            return -1 ;
         }
         else if ( res > 0 )
         {
            return 1 ;
         }
      }

      if ( _opCode < item._opCode )
      {
         return -1 ;
      }
      else if ( _opCode > item._opCode )
      {
         return 1 ;
      }
      return  0 ;
   }

   INT32 _mthMatchOpItem::normalize ( const mthNodeConfig *config,
                                      BSONArrayBuilder &builder,
                                      rtnParamList &parameters )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( config, "config is invalid" ) ;

      BSONObjBuilder opBuilder( builder.subobjStart() ) ;
      BSONObjBuilder subBuilder( opBuilder.subobjStart( _fieldName ) ) ;

      rc = _normalizeFunctions( subBuilder ) ;
      PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize functions, rc: %d", rc ) ;

      switch ( _opCode )
      {
         case EN_MATCH_OPERATOR_REGEX :
         {
            rc = _normalizeREGEX( subBuilder ) ;
            PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize $regex, "
                         "rc: %d", rc ) ;
            break ;
         }
         default :
         {
            rc = _normalizeOPTR( config, subBuilder, parameters ) ;
            PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize parameter, "
                         "rc: %d", rc ) ;
            break ;
         }
      }

      subBuilder.doneFast() ;
      opBuilder.doneFast() ;

   done :
      return rc ;

   error :
      goto done ;
   }

   void _mthMatchOpItem::setDoneByPred ()
   {
      if ( _canDoneByPred() &&
           NULL != _node )
      {
         _node->setDoneByPred( TRUE ) ;
         _node = NULL ;
      }
   }

   BOOLEAN _mthMatchOpItem::_canDoneByPred () const
   {
      if ( _funcList.empty() &&
           NULL == ossStrstr( _fieldName, ".$" ) )
      {
         switch ( _opCode )
         {
            case EN_MATCH_OPERATOR_ET :
            case EN_MATCH_OPERATOR_LT :
            case EN_MATCH_OPERATOR_LTE :
            case EN_MATCH_OPERATOR_GT :
            case EN_MATCH_OPERATOR_GTE :
               return TRUE ;
            default :
               break ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _mthCanBSONTypeParameterize ( BSONType type )
   {
      switch ( type )
      {
         case NumberDouble :
         case NumberInt :
         case NumberLong :
         case NumberDecimal :
         case Date :
         case Timestamp :
         case String :
         case jstOID :
         {
            return TRUE ;
         }
         default :
         {
            return FALSE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _mthMatchOpItem::_canParameterize () const
   {
      // Operators with below cases could be parameterized
      // 1. have no functions
      // 2. have no ".$" matches in field name
      // 3. $et, $gt, $gte, $lt, $lte, $in
      // 4. simple data types
      if ( _funcList.empty() &&
           NULL == ossStrstr( _fieldName, ".$" ) )
      {
         switch ( _opCode )
         {
            case EN_MATCH_OPERATOR_ET :
            case EN_MATCH_OPERATOR_LT :
            case EN_MATCH_OPERATOR_LTE :
            case EN_MATCH_OPERATOR_GT :
            case EN_MATCH_OPERATOR_GTE :
            {
               return _mthCanBSONTypeParameterize( _element.type() ) ;
            }
            case EN_MATCH_OPERATOR_IN :
            {
               return TRUE ;
            }
            default :
            {
               return FALSE ;
            }
         }
      }
      return FALSE ;
   }

   BOOLEAN _mthMatchOpItem::_canFuzzyOptr () const
   {
      switch ( _opCode )
      {
         case EN_MATCH_OPERATOR_LT :
         case EN_MATCH_OPERATOR_LTE :
         case EN_MATCH_OPERATOR_GT :
         case EN_MATCH_OPERATOR_GTE :
            return TRUE ;
         default :
            return FALSE ;
      }
      return FALSE ;
   }

   BOOLEAN _mthMatchOpItem::_isExclusiveOperator () const
   {
      switch ( _opCode )
      {
         case EN_MATCH_OPERATOR_LTE :
         case EN_MATCH_OPERATOR_GTE :
            return FALSE ;
         case EN_MATCH_OPERATOR_LT :
         case EN_MATCH_OPERATOR_GT :
            return TRUE ;
         default :
            break ;
      }
      SDB_ASSERT( FALSE, "Invalid code branch" ) ;
      return FALSE ;
   }

   const CHAR *_mthMatchOpItem::_getFuzzyOpStr () const
   {
      switch ( _opCode )
      {
         case EN_MATCH_OPERATOR_LT :
         case EN_MATCH_OPERATOR_LTE :
            return MTH_OPERATOR_STR_LT ;
         case EN_MATCH_OPERATOR_GT :
         case EN_MATCH_OPERATOR_GTE :
            return MTH_OPERATOR_STR_GT ;
         default :
            break ;
      }
      SDB_ASSERT( FALSE, "Invalid code branch" ) ;
      return NULL ;
   }

   UINT32 _mthMatchOpItem::_getOPWeight ( BOOLEAN fuzzyOptr ) const
   {
      switch ( _opCode )
      {
         case EN_MATCH_OPERATOR_ET :
            return MTH_WEIGHT_EQUAL ;
         case EN_MATCH_OPERATOR_LT :
            if ( fuzzyOptr )
            {
               return MTH_WEIGHT_LTE ;
            }
            return MTH_WEIGHT_LT ;
         case EN_MATCH_OPERATOR_LTE :
            return MTH_WEIGHT_LTE ;
         case EN_MATCH_OPERATOR_GT :
            if ( fuzzyOptr )
            {
               return MTH_WEIGHT_GTE ;
            }
            return MTH_WEIGHT_GT ;
         case EN_MATCH_OPERATOR_GTE :
            return MTH_WEIGHT_GTE ;
         case EN_MATCH_OPERATOR_NE :
            return MTH_WEIGHT_NE ;
         case EN_MATCH_OPERATOR_MOD :
            return MTH_WEIGHT_MOD ;
         case EN_MATCH_OPERATOR_TYPE :
            return MTH_WEIGHT_TYPE ;
         case EN_MATCH_OPERATOR_IN :
            return MTH_WEIGHT_IN ;
         case EN_MATCH_OPERATOR_NIN :
            return MTH_WEIGHT_NIN ;
         case EN_MATCH_OPERATOR_ALL :
            return MTH_WEIGHT_ALL ;
         case EN_MATCH_OPERATOR_EXISTS :
            return MTH_WEIGHT_EXISTS ;
         case EN_MATCH_OPERATOR_ELEMMATCH :
            return MTH_WEIGHT_ELEMMATCH ;
         case EN_MATCH_OPERATOR_REGEX :
            return MTH_WEIGHT_REGEX ;
         case EN_MATCH_OPERATOR_ISNULL :
            return MTH_WEIGHT_ISNULL ;
         default :
            return 0 ;
      }

      return 0 ;
   }

   INT32 _mthMatchOpItem::_normalizeFunctions ( BSONObjBuilder & subBuilder )
   {
      INT32 rc = SDB_OK ;

      for ( MTH_FUNC_OP_LIST::const_iterator iter = _funcList.begin() ;
            iter != _funcList.end() ;
            iter ++ )
      {
         rc = iter->normalize( subBuilder ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize function [%s], rc: %d",
                      iter->getFieldName(), rc ) ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _mthMatchOpItem::_normalizeREGEX ( BSONObjBuilder & subBuilder )
   {
      INT32 rc = SDB_OK ;

      subBuilder.append( MTH_OPERATOR_STR_REGEX, _opName ) ;
      subBuilder.append( MTH_OPERATOR_STR_OPTIONS, _options ) ;

      return rc ;
   }

   INT32 _mthMatchOpItem::_normalizeOPTR ( const mthNodeConfig * config,
                                           BSONObjBuilder & subBuilder,
                                           rtnParamList & parameters )
   {
      INT32 rc = SDB_OK ;

      if ( config->_enableParameterized &&
           parameters.canParameterize() &&
           _canParameterize() )
      {
         _paramIndex = parameters.addParam( _element ) ;

         const CHAR * opName = _opName ;
         if ( config->_enableFuzzyOptr &&
              parameters.canParameterize() &&
              _canFuzzyOptr() )
         {
            _fuzzyIndex = parameters.addParam(
                  _isExclusiveOperator() ? _mthFuzzyExcOptr.firstElement() :
                                           _mthFuzzyIncOptr.firstElement() ) ;
            opName = _getFuzzyOpStr() ;
         }
         else if ( EN_MATCH_OPERATOR_IN == _opCode )
         {
            rc = parameters.buildValueSet( _paramIndex ) ;
            PD_RC_CHECK( rc, PDDEBUG, "Failed to build value set, "
                         "rc: %d", rc ) ;
         }

         // Generate $param field { $param : x, $ctype : y }
         // 1. If it is not mix-compare mode, we need $ctype ( canonical type )
         //    in the normalized query, since the mix or max keys in predicates
         //    generated with different data type will be different
         // 2. If it is mix-compare mode, the $type could be ignore, since the
         //    mix or max keys are the same with different data type
         BSONObjBuilder paramBuilder( subBuilder.subobjStart( opName ) ) ;
         if ( -1 == _fuzzyIndex )
         {
            paramBuilder.append( FIELD_NAME_PARAM, (INT32)_paramIndex ) ;
         }
         else
         {
            // Generate $param field with fuzzy operator
            // $param : [ paramIndex, fuzzyIndex ]
            BSONArrayBuilder paramArrBuilder(
                  paramBuilder.subarrayStart( FIELD_NAME_PARAM ) ) ;
            paramArrBuilder.append( (INT32)_paramIndex ) ;
            paramArrBuilder.append( (INT32)_fuzzyIndex ) ;
            paramArrBuilder.doneFast() ;
         }
         if ( !config->_enableMixCmp )
         {
            paramBuilder.append( FIELD_NAME_CTYPE,
                                 (INT32)_element.canonicalType() ) ;
         }
         paramBuilder.doneFast() ;

      }
      else
      {
         rc = _normalizeItem( subBuilder ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize item, rc: %d", rc ) ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _mthMatchOpItem::_normalizeItem ( BSONObjBuilder & subBuilder )
   {
      INT32 rc = SDB_OK ;

      subBuilder.appendAs( _element, _opName ) ;

      return rc ;
   }

   INT32 mthMatchOpItemComparator ( const void *a, const void *b )
   {
      const _mthMatchOpItem *left = *((const _mthMatchOpItem **)a) ;
      const _mthMatchOpItem *right = *((const _mthMatchOpItem **)b) ;
      return left->comparator( *right ) ;
   }

   /*
      _mthMatchNormalizer implement
    */
   _mthMatchNormalizer::_mthMatchNormalizer ( const mthNodeConfig *config )
   : _mthMatchConfig( config ),
     _itemNumber( 0 ),
     _invalidMatcher( FALSE )
   {
   }

   _mthMatchNormalizer::~_mthMatchNormalizer ()
   {
   }

   void _mthMatchNormalizer::clear ()
   {
      for ( UINT8 i = 0 ; i < _itemNumber ; i++ )
      {
         _items[ i ].getFuncList().clear() ;
      }
      _itemNumber = 0 ;
   }

   INT32 _mthMatchNormalizer::normalize ( const BSONObj &matcher,
                                          BSONObjBuilder &normalBuilder,
                                          rtnParamList &parameters )
   {
      INT32 rc = SDB_OK ;

      _invalidMatcher = FALSE ;

      try
      {
         rc = _parseObject( matcher ) ;
         if ( SDB_OK != rc )
         {
            if ( _invalidMatcher )
            {
               PD_LOG( PDERROR, "Failed to parse query [%s], rc: %d",
                       matcher.toString( FALSE, TRUE ).c_str(), rc ) ;
            }
            goto error ;
         }

         if ( _itemNumber > 0 )
         {
            qsort( _itemIndexes, _itemNumber, sizeof( _mthMatchOpItem * ),
                   mthMatchOpItemComparator ) ;
            rc = _normalize( normalBuilder, parameters ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to normalize query, rc: %d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to call parse query: %s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   void _mthMatchNormalizer::setDoneByPred ( const BSONObj &keyPattern,
                                             UINT32 addedLevel )
   {
      if ( _itemNumber == 0 )
      {
         return ;
      }

      UINT32 level = 0 ;
      BSONObjIterator iter( keyPattern ) ;
      while ( iter.more() && level < addedLevel )
      {
         BSONElement e = iter.next() ;
         for ( UINT8 i = 0 ; i < _itemNumber ; i++ )
         {
            if ( NULL != _items[ i ].getOpNode() &&
                 0 == ossStrcmp( e.fieldName(), _items[ i ].getFieldName() ) )
            {
               _items[ i ].setDoneByPred() ;
            }
         }
         level ++ ;
      }
   }

   INT32 _mthMatchNormalizer::_parseObject ( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      MTH_FUNC_OP_LIST emptyFuncList ;
      BSONObjIterator iter( object ) ;

      while ( iter.more() )
      {
         BSONElement element = iter.next() ;
         const CHAR *fieldName = element.fieldName() ;

         switch ( element.type() )
         {
            case Object :
            {
               rc = _parseInnerObject( fieldName, element ) ;
               break ;
            }
            case Array :
            {
               // Parse $and:[] only
               if ( fieldName[0] == MTH_OPERATOR_EYECATCHER &&
                    fieldName[1] == 'a' && fieldName[2] == 'n' &&
                    fieldName[3] == 'd' && fieldName[4] == '\0' )
               {
                  rc = _parseAndOp( element ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
               }
               break ;
            }
            case RegEx :
            {
               rc = _addRegexItem( fieldName, element.regex(),
                                   element.regexFlags(), emptyFuncList ) ;
               break ;
            }
            default :
            {
               PD_CHECK( fieldName[0] != MTH_OPERATOR_EYECATCHER,
                         SDB_INVALIDARG, invalidate, PDERROR,
                         "Failed to parse element [%s]",
                         element.toString( FALSE, TRUE ).c_str() ) ;
               if ( Object == element.type() ||
                    Array == element.type() )
               {
                  // Too complex for normalizer to parse embedded objects
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               rc = _addOpItem( fieldName, EN_MATCH_OPERATOR_ET,
                                MTH_OPERATOR_STR_ET, element,
                                emptyFuncList ) ;
               break ;
            }
         }

         if ( SDB_OK != rc )
         {
            if ( _invalidMatcher )
            {
               PD_LOG( PDERROR, "Failed to parse element [%s], rc: %d",
                      element.toString( FALSE, TRUE ).c_str(),
                      rc ) ;
            }
            goto error ;
         }
      }

   done :
      return rc ;
   invalidate :
      _invalidMatcher = TRUE ;
   error :
      goto done ;
   }

   INT32 _mthMatchNormalizer::_parseAndOp ( const BSONElement &element )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( Array == element.type(), "Invalid element type, "
                  "Array is expected" ) ;

      try
      {
         BSONObjIterator iter( element.embeddedObject() ) ;

         while ( iter.more() )
         {
            BSONElement arrayElement = iter.next() ;

            switch ( arrayElement.type() )
            {
               case Object :
               {
                  rc = _parseObject( arrayElement.embeddedObject() ) ;
                  break ;
               }
               default :
               {
                  // Only object is allowed inside $and
                  _invalidMatcher = TRUE ;
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Item [%s] in $and must be an object",
                          arrayElement.toString( FALSE, TRUE ).c_str() ) ;
                  break ;
               }
            }

            if ( SDB_OK != rc )
            {
               if ( _invalidMatcher )
               {
                  PD_LOG( PDERROR, "Failed to parse element [%s], rc: %d",
                          arrayElement.toString( FALSE, TRUE ).c_str(),
                          rc ) ;
               }
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to call parse query: %s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _mthMatchNormalizer::_parseInnerObject ( const CHAR *fieldName,
                                                  const BSONElement &element )
   {
      INT32 rc = SDB_OK ;

      // FieldName can't start with '$'
      PD_CHECK( MTH_OPERATOR_EYECATCHER != fieldName[0],
                SDB_INVALIDARG, invalidate, PDERROR,
                "Operator can not in the head of element "
                "[%s]", element.toString( FALSE, TRUE ).c_str() ) ;

      try
      {
         BOOLEAN hasNormalEle = FALSE ;
         BOOLEAN hasOpEle = FALSE ;
         BSONObjIterator iter( element.embeddedObject() ) ;
         MTH_FUNC_OP_LIST funcList ;

         const CHAR *regex = NULL ;
         const CHAR *options = NULL ;

         while ( iter.more() )
         {
            BSONElement subElement = iter.next() ;
            const CHAR *subFieldName = subElement.fieldName() ;

            if ( subFieldName[0] != MTH_OPERATOR_EYECATCHER )
            {
               if ( hasOpEle )
               {
                  rc = SDB_INVALIDARG ;
                  goto invalidate ;
               }
               else if ( !hasNormalEle )
               {
                  hasNormalEle = TRUE ;
               }
            }
            else if ( hasNormalEle )
            {
               rc = SDB_INVALIDARG ;
               goto invalidate ;
            }
            else
            {
               EN_MATCH_OP_FUNC_TYPE opCode =
                     mthGetMatchNodeFactory()->getMatchNodeType( subFieldName ) ;

               PD_CHECK( opCode != EN_MATCH_OPERATOR_END,
                         SDB_INVALIDARG, invalidate, PDERROR,
                         "object's element do not allow exist mix op keys and "
                         "normal keys %s",
                         element.toString( TRUE, FALSE ).c_str() ) ;

               hasOpEle = TRUE ;

               if ( opCode >= EN_MATCH_OPERATOR_ET &&
                    opCode < EN_MATCH_OPERATOR_END )
               {
                  if ( opCode == EN_MATCH_OPERATOR_MOD &&
                       subElement.type() != Array &&
                       subElement.type() != Object )
                  {
                     // Special case for $mod:x, which is a function
                     _mthMatchFuncItem item( subFieldName, EN_MATCH_FUNC_MOD,
                                             subElement ) ;
                     funcList.push_back( item ) ;
                  }
                  else if ( opCode == EN_MATCH_OPERATOR_REGEX )
                  {
                     PD_CHECK( String == subElement.type() && NULL == regex,
                               SDB_INVALIDARG, invalidate, PDERROR,
                               "Failed to parse regex" ) ;
                     regex = subElement.valuestr() ;
                     if ( options != NULL )
                     {
                        // { ..., $options: 'xx', $regex : 'xx', ... }
                        rc = _addRegexItem( fieldName, regex, options, funcList ) ;
                        funcList.clear() ;
                        regex = NULL ;
                        options = NULL ;
                        if ( SDB_OK != rc )
                        {
                           goto error ;
                        }
                     }
                  }
                  else if ( opCode == EN_MATCH_OPERATOR_OPTIONS )
                  {
                     PD_CHECK( String == subElement.type() && NULL == options,
                               SDB_INVALIDARG, invalidate, PDERROR,
                               "Failed to parse regex" ) ;
                     options = subElement.valuestr() ;
                     if ( regex != NULL )
                     {
                        // { ..., $regex: 'xx', $options : 'xx', ... }
                        rc = _addRegexItem( fieldName, regex, options, funcList ) ;
                        funcList.clear() ;
                        regex = NULL ;
                        options = NULL ;
                        if ( SDB_OK != rc )
                        {
                           goto error ;
                        }
                     }
                  }
                  else
                  {
                     if ( regex != NULL )
                     {
                        // { ..., $regex : 'xx', ... }
                        SDB_ASSERT( options == NULL, "options should be NULL" ) ;
                        rc = _addRegexItem( fieldName, regex, "", funcList ) ;
                        funcList.clear() ;
                        regex = NULL ;
                        if ( SDB_OK != rc )
                        {
                           goto error ;
                        }
                     }
                     else if ( options != NULL )
                     {
                        rc = SDB_INVALIDARG ;
                        PD_LOG( PDERROR, "Failed to parse regex [%s]",
                                element.toString( FALSE, TRUE ).c_str() ) ;
                        goto invalidate ;
                     }
                     if ( EN_MATCH_OPERATOR_IN == opCode )
                     {
                        PD_CHECK( Array == subElement.type(),
                                  SDB_INVALIDARG, invalidate, PDERROR,
                                  "Failed to parse $in operator [%s], "
                                  "element is not Array",
                                  element.toString( TRUE, FALSE ).c_str() ) ;
                     }
                     else if ( Object == subElement.type() ||
                               Array == subElement.type() )
                     {
                        // Too complex for normalizer to parse embedded objects
                        rc = SDB_INVALIDARG ;
                        goto error ;
                     }
                     rc = _addOpItem( fieldName, opCode, subFieldName,
                                      subElement, funcList ) ;
                     funcList.clear() ;
                     if ( SDB_OK != rc )
                     {
                        goto error ;
                     }
                  }
               }
               else if ( opCode >= EN_MATCH_FUNC_ABS &&
                         opCode < EN_MATCH_FUNC_END )
               {
                  _mthMatchFuncItem item( subFieldName, opCode, subElement ) ;
                  funcList.push_back( item ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
         }

         if ( hasNormalEle )
         {
            PD_CHECK( funcList.empty(), SDB_INVALIDARG, invalidate, PDERROR,
                      "Failed to parse query: %s",
                      element.toString( FALSE, TRUE ).c_str() ) ;
            // Too complex for normalizer to parse embedded objects
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            if ( regex != NULL )
            {
               rc = _addRegexItem( fieldName, regex,
                                   options == NULL ? "" : options,
                                   funcList ) ;
               funcList.clear() ;
               regex = NULL ;
               options = NULL ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
            else if ( options != NULL )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Failed to parse regex [%s]",
                       element.toString( FALSE, TRUE ).c_str() ) ;
               goto invalidate ;
            }
            PD_CHECK( funcList.empty(), SDB_INVALIDARG, invalidate, PDERROR,
                      "Failed to parse query: %s",
                      element.toString( FALSE, TRUE ).c_str() ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to call parse query: %s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;
   invalidate :
      _invalidMatcher = TRUE ;
   error :
      goto done ;
   }

   INT32 _mthMatchNormalizer::_normalize ( BSONObjBuilder &normalBuilder,
                                           rtnParamList &parameters )
   {
      INT32 rc = SDB_OK ;

      BSONArrayBuilder subNormalBuilder(
            normalBuilder.subarrayStart( MTH_OPERATOR_STR_AND ) ) ;
      for ( UINT8 i = 0 ; i < _itemNumber ; i ++ )
      {
         rc = _itemIndexes[ i ]->normalize( getMatchConfigPtr(),
                                            subNormalBuilder,
                                            parameters ) ;
         PD_RC_CHECK( rc, PDDEBUG, "Failed to normalize item %s, rc: %d",
                      _itemIndexes[ i ]->toString().c_str(), rc ) ;
      }
      subNormalBuilder.done() ;

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _mthMatchNormalizer::_addOpItem ( const CHAR *fieldName,
                                           EN_MATCH_OP_FUNC_TYPE opCode,
                                           const CHAR *opName,
                                           const BSONElement &element,
                                           MTH_FUNC_OP_LIST &funcList )
   {
      if ( _itemNumber < MTH_NORMALIZER_ITEM_NUM )
      {
         _items[ _itemNumber ].setOpItem( fieldName, opCode, opName, element,
                                          funcList, mthEnabledFuzzyOptr() ) ;
         _itemIndexes[ _itemNumber ] = &(_items[ _itemNumber ]) ;
         _itemNumber ++ ;
         return SDB_OK ;
      }
      return SDB_INVALIDARG ;
   }

   INT32 _mthMatchNormalizer::_addRegexItem ( const CHAR *fieldName,
                                              const CHAR *regex,
                                              const CHAR *options,
                                              MTH_FUNC_OP_LIST &funcList )
   {
      if ( _itemNumber < MTH_NORMALIZER_ITEM_NUM )
      {
         _items[ _itemNumber ].setRegexItem( fieldName, regex, options,
                                             funcList ) ;
         _itemIndexes[ _itemNumber ] = &(_items[ _itemNumber ]) ;
         _itemNumber ++ ;
         return SDB_OK ;
      }
      return SDB_INVALIDARG ;
   }

}
