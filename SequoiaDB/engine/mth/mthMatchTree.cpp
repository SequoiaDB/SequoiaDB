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

   Source File Name = mthMatchTree.cpp

   Descriptive Name = Method Matcher Tree

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains functions for matcher, which
   indicates whether a record matches a given matching rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/25/2016  LYB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mthMatchTree.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "rtnCB.hpp"
#include <string>

using namespace bson ;

namespace engine
{
   #define MTH_ELEMENT_KEY_ALL_OP         0

   #define MTH_ELEMENT_KEY_ALL_NORMAL     1

   #define MTH_ELEMENT_KEY_MIX            2

   static mthMatchOpMapping g_opstr_to_type_array[] =
   {
      { MTH_OPERATOR_STR_AND,         EN_MATCH_OPERATOR_LOGIC_AND },
      { MTH_OPERATOR_STR_OR,          EN_MATCH_OPERATOR_LOGIC_OR },
      { MTH_OPERATOR_STR_NOT,         EN_MATCH_OPERATOR_LOGIC_NOT },

      { MTH_OPERATOR_STR_ET,          EN_MATCH_OPERATOR_ET },
      { MTH_OPERATOR_STR_LT,          EN_MATCH_OPERATOR_LT },
      { MTH_OPERATOR_STR_LTE,         EN_MATCH_OPERATOR_LTE },
      { MTH_OPERATOR_STR_GTE,         EN_MATCH_OPERATOR_GTE },
      { MTH_OPERATOR_STR_GT,          EN_MATCH_OPERATOR_GT },
      { MTH_OPERATOR_STR_IN,          EN_MATCH_OPERATOR_IN },
      { MTH_OPERATOR_STR_NE,          EN_MATCH_OPERATOR_NE },
      { MTH_OPERATOR_STR_ALL,         EN_MATCH_OPERATOR_ALL },
      { MTH_OPERATOR_STR_NIN,         EN_MATCH_OPERATOR_NIN },
      { MTH_OPERATOR_STR_EXISTS,      EN_MATCH_OPERATOR_EXISTS },
      { MTH_OPERATOR_STR_MOD,         EN_MATCH_OPERATOR_MOD },
      { MTH_OPERATOR_STR_ELEMMATCH,   EN_MATCH_OPERATOR_ELEMMATCH },
      { MTH_OPERATOR_STR_ISNULL,      EN_MATCH_OPERATOR_ISNULL },

      { MTH_OPERATOR_STR_FIELD,       EN_MATCH_OPERATOR_FIELD },
      { MTH_OPERATOR_STR_REGEX,       EN_MATCH_OPERATOR_REGEX },
      { MTH_OPERATOR_STR_OPTIONS,     EN_MATCH_OPERATOR_OPTIONS },

      { MTH_FUNCTION_STR_ABS,         EN_MATCH_FUNC_ABS },
      { MTH_FUNCTION_STR_CEILING,     EN_MATCH_FUNC_CEILING },
      { MTH_FUNCTION_STR_FLOOR,       EN_MATCH_FUNC_FLOOR },
      { MTH_FUNCTION_STR_ADD,         EN_MATCH_FUNC_ADD },
      { MTH_FUNCTION_STR_SUBTRACT,    EN_MATCH_FUNC_SUBTRACT },
      { MTH_FUNCTION_STR_MULTIPLY,    EN_MATCH_FUNC_MULTIPLY },
      { MTH_FUNCTION_STR_DIVIDE,      EN_MATCH_FUNC_DIVIDE },
      { MTH_FUNCTION_STR_SUBSTR,      EN_MATCH_FUNC_SUBSTR },
      { MTH_FUNCTION_STR_STRLEN,      EN_MATCH_FUNC_STRLEN },
      { MTH_FUNCTION_STR_LOWER,       EN_MATCH_FUNC_LOWER },
      { MTH_FUNCTION_STR_UPPER,       EN_MATCH_FUNC_UPPER },
      { MTH_FUNCTION_STR_LTRIM,       EN_MATCH_FUNC_LTRIM },
      { MTH_FUNCTION_STR_RTRIM,       EN_MATCH_FUNC_RTRIM },
      { MTH_FUNCTION_STR_TRIM,        EN_MATCH_FUNC_TRIM },
      { MTH_FUNCTION_STR_CAST,        EN_MATCH_FUNC_CAST },
      { MTH_FUNCTION_STR_SLICE,       EN_MATCH_FUNC_SLICE },
      { MTH_FUNCTION_STR_SIZE,        EN_MATCH_FUNC_SIZE },
      { MTH_FUNCTION_STR_TYPE,        EN_MATCH_FUNC_TYPE },

      { MTH_ATTR_STR_EXPAND,          EN_MATCH_ATTR_EXPAND },
      { MTH_ATTR_STR_RETURNMATCH,     EN_MATCH_ATTR_RETURNMATCH },
   } ;

   _mthMatchNodeFactory::_mthMatchNodeFactory()
   {
      INT32 i   = 0 ;
      INT32 len = 0 ;

      len = sizeof( g_opstr_to_type_array ) / sizeof( mthMatchOpMapping ) ;
      for ( ; i < len ; i++ )
      {
         mthMatchOpMapping *ptype = &g_opstr_to_type_array[i] ;
         SDB_ASSERT( _opstrMap.find( ptype->opStr ) == _opstrMap.end(),
                     "duplicate key is not allowed" ) ;

         _opstrMap[ ptype->opStr ] = ptype ;
      }
   }

   _mthMatchNodeFactory::~_mthMatchNodeFactory()
   {
      _opstrMap.clear() ;
   }

   EN_MATCH_OP_FUNC_TYPE _mthMatchNodeFactory::getMatchNodeType(
                                                             const CHAR *opStr )
   {
      MTH_OPSTRMAP::iterator iter ;
      iter = _opstrMap.find( opStr ) ;
      if ( iter != _opstrMap.end() )
      {
         mthMatchOpMapping* mapping = iter->second ;
         return ( EN_MATCH_OP_FUNC_TYPE ) mapping->nodeType ;
      }
      else
      {
         return EN_MATCH_OP_FUNC_END ;
      }
   }

   _mthMatchOpNode* _mthMatchNodeFactory::createOpNode(
                                                   _mthNodeAllocator *allocator,
                                                   const mthNodeConfig *config,
                                                   EN_MATCH_OP_FUNC_TYPE type )
   {
      _mthMatchOpNode *opNode = NULL ;

      switch( type )
      {
      case EN_MATCH_OPERATOR_ET:
         opNode = new ( allocator ) _mthMatchOpNodeET( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_LT:
      case EN_MATCH_OPERATOR_LTE:
         opNode = new ( allocator ) _mthMatchOpNodeLT( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_GT:
      case EN_MATCH_OPERATOR_GTE:
         opNode = new ( allocator ) _mthMatchOpNodeGT( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_IN:
         opNode = new ( allocator ) _mthMatchOpNodeIN( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_NE:
         opNode = new ( allocator ) _mthMatchOpNodeNE( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_ALL:
         opNode = new ( allocator ) _mthMatchOpNodeALL( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_NIN:
         opNode = new ( allocator ) _mthMatchOpNodeNIN( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_EXISTS:
         opNode = new ( allocator ) _mthMatchOpNodeEXISTS( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_MOD:
         opNode = new ( allocator ) _mthMatchOpNodeMOD( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_ELEMMATCH:
         opNode = new( allocator ) _mthMatchOpNodeELEMMATCH( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_ISNULL:
         opNode = new ( allocator ) _mthMatchOpNodeISNULL( allocator, config ) ;
         break ;
      case EN_MATCH_OPERATOR_REGEX:
         opNode = new ( allocator ) _mthMatchOpNodeRegex( allocator, config ) ;
         break ;
      case EN_MATCH_ATTR_EXPAND:
         opNode = new ( allocator ) _mthMatchOpNodeEXPAND( allocator, config ) ;
         break ;
      default :
         break ;
      }

      return opNode ;
   }

   _mthMatchLogicNode* _mthMatchNodeFactory::createLogicNode(
                                                   _mthNodeAllocator *allocator,
                                                   const mthNodeConfig *config,
                                                   EN_MATCH_OP_FUNC_TYPE type )
   {
      _mthMatchLogicNode *logicNode = NULL ;

      switch( type )
      {
      case EN_MATCH_OPERATOR_LOGIC_AND :
         logicNode = new ( allocator ) _mthMatchLogicAndNode( allocator, config ) ;
         break ;

      case EN_MATCH_OPERATOR_LOGIC_OR :
         logicNode = new ( allocator ) _mthMatchLogicOrNode( allocator, config ) ;
         break ;

      case EN_MATCH_OPERATOR_LOGIC_NOT :
         logicNode = new ( allocator ) _mthMatchLogicNotNode( allocator, config ) ;
         break ;

      default :
         break ;
      }

      return logicNode ;
   }

   void _mthMatchNodeFactory::releaseNode( _mthMatchNode *node )
   {
      node->release() ;
   }

   _mthMatchTree* _mthMatchNodeFactory::createTree()
   {
      return SDB_OSS_NEW _mthMatchTree() ;
   }

   void _mthMatchNodeFactory::releaseTree( _mthMatchTree *tree )
   {
      if ( NULL != tree )
      {
         tree->clear() ;
         SAFE_OSS_DELETE( tree ) ;
      }
   }

   _mthMatchFunc* _mthMatchNodeFactory::createFunc(
                                                _mthNodeAllocator *allocator,
                                                EN_MATCH_OP_FUNC_TYPE type )
   {
      _mthMatchFunc *func = NULL ;

      switch( type )
      {
      case EN_MATCH_FUNC_ABS:
         func = new ( allocator ) _mthMatchFuncABS( allocator ) ;
         break ;
      case EN_MATCH_FUNC_CEILING:
         func = new ( allocator ) _mthMatchFuncCEILING( allocator ) ;
         break ;
      case EN_MATCH_FUNC_FLOOR:
         func = new ( allocator ) _mthMatchFuncFLOOR( allocator ) ;
         break ;
      case EN_MATCH_FUNC_MOD:
      case EN_MATCH_OPERATOR_MOD:
         func = new ( allocator ) _mthMatchFuncMOD( allocator ) ;
         break ;
      case EN_MATCH_FUNC_ADD:
         func = new ( allocator ) _mthMatchFuncADD( allocator ) ;
         break ;
      case EN_MATCH_FUNC_SUBTRACT:
         func = new ( allocator ) _mthMatchFuncSUBTRACT( allocator ) ;
         break ;
      case EN_MATCH_FUNC_MULTIPLY:
         func = new ( allocator ) _mthMatchFuncMULTIPLY( allocator ) ;
         break ;
      case EN_MATCH_FUNC_DIVIDE:
         func = new ( allocator ) _mthMatchFuncDIVIDE( allocator ) ;
         break ;
      case EN_MATCH_FUNC_SUBSTR:
         func = new ( allocator ) _mthMatchFuncSUBSTR( allocator ) ;
         break ;
      case EN_MATCH_FUNC_STRLEN:
         func = new ( allocator ) _mthMatchFuncSTRLEN( allocator ) ;
         break ;
      case EN_MATCH_FUNC_LOWER:
         func = new ( allocator ) _mthMatchFuncLOWER( allocator ) ;
         break ;
      case EN_MATCH_FUNC_UPPER:
         func = new ( allocator ) _mthMatchFuncUPPER( allocator ) ;
         break ;
      case EN_MATCH_FUNC_LTRIM:
         func = new ( allocator ) _mthMatchFuncLTRIM( allocator ) ;
         break ;
      case EN_MATCH_FUNC_RTRIM:
         func = new ( allocator ) _mthMatchFuncRTRIM( allocator ) ;
         break ;
      case EN_MATCH_FUNC_TRIM:
         func = new ( allocator ) _mthMatchFuncTRIM( allocator ) ;
         break ;
      case EN_MATCH_FUNC_CAST:
         func = new ( allocator ) _mthMatchFuncCAST( allocator ) ;
         break ;
      case EN_MATCH_FUNC_SLICE:
         func = new ( allocator ) _mthMatchFuncSLICE( allocator ) ;
         break ;
      case EN_MATCH_FUNC_SIZE:
         func = new ( allocator ) _mthMatchFuncSIZE( allocator ) ;
         break ;
      case EN_MATCH_FUNC_TYPE:
         func = new ( allocator ) _mthMatchFuncTYPE( allocator ) ;
         break ;
      case EN_MATCH_ATTR_RETURNMATCH:
         /*
            attribute returnMatch's behavior is like function. so we treat it
            as a function.
         */
         func = new ( allocator ) _mthMatchFuncRETURNMATCH( allocator ) ;
         break ;
      case EN_MATCH_ATTR_EXPAND:
         /*
            attribute expand's behavior is like function. so we treat it
            as a function.
         */
         func = new ( allocator ) _mthMatchFuncEXPAND( allocator ) ;
         break ;
      default :
         break ;
      }

      return func ;
   }

   void _mthMatchNodeFactory::releaseFunc( _mthMatchFunc *func )
   {
      func->release() ;
   }

   _mthMatchNodeFactory *mthGetMatchNodeFactory()
   {
      static _mthMatchNodeFactory factory ;

      return &factory ;
   }

   _mthMatchTree::_mthMatchTree()
   : _mthMatchConfigHolder()
   {
      _root = NULL ;

      _isInitialized       = FALSE ;
      _isMatchesAll        = TRUE ;
      _isTotallyConverted  = TRUE ;
      _hasDollarFieldName  = FALSE ;

      _hasExpand           = FALSE ;
      _hasReturnMatch      = FALSE ;
      _attrFieldName       = NULL ;
      _returnMatchNode     = NULL ;
   }

   _mthMatchTree::~_mthMatchTree()
   {
      clear() ;
   }

   INT32 _mthMatchTree::_addFunction( const CHAR *fieldName,
                                      const BSONElement &ele,
                                      EN_MATCH_OP_FUNC_TYPE nodeType,
                                      MTH_FUNC_LIST &funcList )
   {
      INT32 rc = SDB_OK ;
      _mthMatchFunc* func = NULL ;
      func = mthGetMatchNodeFactory()->createFunc( &_allocator, nodeType ) ;
      if ( NULL == func )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "createFunc failed:nodeType=%d,rc=%d",
                 nodeType, rc ) ;
         goto error ;
      }

      rc = func->init( fieldName, ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init function failed:ele=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

      funcList.push_back( func ) ;

   done:
      return rc ;
   error:
      if ( NULL != func )
      {
         mthGetMatchNodeFactory()->releaseFunc( func ) ;
         func = NULL ;
      }
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_ADDOPERATOR, "_mthMatchTree::_addOperator" )
   INT32 _mthMatchTree::_addOperator( const CHAR *fieldName,
                                      const BSONElement &ele,
                                      EN_MATCH_OP_FUNC_TYPE nodeType,
                                      MTH_FUNC_LIST &funcList,
                                      _mthMatchLogicNode *parent,
                                      INT8 paramIndex,
                                      INT8 fuzzyIndex,
                                      _mthMatchOpNode **retNode )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_ADDOPERATOR ) ;
      INT32 rc = SDB_OK ;
      _mthMatchOpNode *node = NULL ;
      BOOLEAN hasAddToTree  = FALSE ;

      SDB_ASSERT( EN_MATCH_OPERATOR_REGEX != nodeType,
                  "regex is processed in another branch" ) ;

      if ( EN_MATCH_OPERATOR_FIELD == nodeType )
      {
         nodeType = EN_MATCH_OPERATOR_ET ;
      }

      node = mthGetMatchNodeFactory()->createOpNode( &_allocator,
                                                     getMatchConfigPtr(),
                                                     nodeType ) ;
      if ( NULL == node )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "createOpNodeByOp failed:nodeType=%d,rc=%d",
                 nodeType, rc ) ;
         goto error ;
      }

      rc = node->init( fieldName, ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init node failed:name=%s,ele=%s,rc=%d",
                 fieldName, ele.toString().c_str(), rc ) ;
         goto error ;
      }

      rc = node->addFuncList( funcList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add function list failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( node->hasReturnMatch() )
      {
         SDB_ASSERT( NULL == _returnMatchNode, "only once" ) ;
         _returnMatchNode = node ;
      }

      if ( node->hasDollarFieldName() )
      {
         _hasDollarFieldName = TRUE ;
      }

      node->setParamIndex( paramIndex ) ;
      if ( fuzzyIndex < 0 )
      {
         node->setFuzzyOpType( nodeType ) ;
      }
      else
      {
         node->setFuzzyIndex( fuzzyIndex ) ;
      }

      rc = parent->addChild( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                 parent->toString().c_str(), node->toString().c_str(), rc ) ;
         goto error ;
      }
      hasAddToTree = TRUE ;

      if ( retNode )
      {
         (*retNode) = node ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_ADDOPERATOR, rc ) ;
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( node ) ;
      }
      goto done ;
   }

   INT32 _mthMatchTree::_addExpandOp( const CHAR *fieldName,
                                      const BSONElement &ele,
                                      _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      _mthMatchOpNode *node = NULL ;
      BOOLEAN hasAddToTree  = FALSE ;

      node = mthGetMatchNodeFactory()->createOpNode( &_allocator,
                                                     getMatchConfigPtr(),
                                                     EN_MATCH_ATTR_EXPAND ) ;
      if ( NULL == node )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "createOpNode failed:type=%d,rc=%d",
                 EN_MATCH_ATTR_EXPAND, rc ) ;
         goto error ;
      }

      rc = node->init( fieldName, ele ) ;
      PD_RC_CHECK( rc, PDERROR, "init node failed:ele=%s,rc=%d",
                   ele.toString().c_str(), rc ) ;

      rc = parent->addChild( node ) ;
      PD_RC_CHECK( rc, PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                   parent->toString().c_str(), node->toString().c_str(), rc ) ;

      hasAddToTree = TRUE ;

   done:
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( node ) ;
      }
      goto done ;
   }

   INT32 _mthMatchTree::_addRegExOp( const CHAR *fieldName,
                                     const CHAR *regex,
                                     const CHAR *options,
                                     MTH_FUNC_LIST &funcList,
                                     _mthMatchLogicNode *parent,
                                     _mthMatchOpNode **retNode )
   {
      INT32 rc = SDB_OK ;
      _mthMatchOpNode *node           = NULL ;
      _mthMatchOpNodeRegex *regexNode = NULL ;
      BOOLEAN hasAddToTree            = FALSE ;

      node = mthGetMatchNodeFactory()->createOpNode( &_allocator,
                                                     getMatchConfigPtr(),
                                                     EN_MATCH_OPERATOR_REGEX ) ;
      if ( NULL == node )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "createOpNode failed:type=%d,rc=%d",
                 EN_MATCH_OPERATOR_REGEX, rc ) ;
         goto error ;
      }

      regexNode = dynamic_cast< _mthMatchOpNodeRegex * > ( node ) ;
      if ( NULL == regexNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "dynamic_cast(OpNode -> OpNodeRegex)"
                 " failed:node=%s,rc=%d", node->toString().c_str(), rc ) ;
         goto error ;
      }

      rc = regexNode->init( fieldName, regex, options );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init regexNode failed:regex=%s,options=%s,rc=%d",
                 ( NULL == regex ) ? "" : regex,
                 ( NULL == options ) ? "" : options, rc ) ;
         goto error ;
      }

      rc = regexNode->addFuncList( funcList ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add funclist failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( node->hasReturnMatch() )
      {
         SDB_ASSERT( NULL == _returnMatchNode, "only once" ) ;
         _returnMatchNode = node ;
      }

      rc = parent->addChild( regexNode ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                 parent->toString().c_str(), node->toString().c_str(), rc ) ;
         goto error ;
      }
      hasAddToTree = TRUE ;

      if ( retNode )
      {
         (*retNode) = node ;
      }

   done:
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( node ) ;
      }
      goto done ;
   }

   INT32 _mthMatchTree::_parseOpItem ( mthMatchOpItem *opItem,
                                       _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( opItem, "opItem is invalid" ) ;

      _mthMatchOpNode *retNode = NULL ;
      MTH_FUNC_LIST funcList ;
      const MTH_FUNC_OP_LIST &funcOpList = opItem->getFuncList() ;

      for ( MTH_FUNC_OP_LIST::const_iterator funcIter = funcOpList.begin() ;
            funcIter != funcOpList.end() ;
            funcIter ++ )
      {
         rc = _addFunction( opItem->getFieldName(), funcIter->getElement(),
                            funcIter->getOpCode(), funcList ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add function, rc: %d", rc ) ;
      }

      switch ( opItem->getOpCode() )
      {
         case EN_MATCH_OPERATOR_REGEX :
            rc = _addRegExOp( opItem->getFieldName(), opItem->getRegex(),
                              opItem->getOptions(), funcList, parent,
                              &retNode ) ;
            break ;
         default :
            rc = _addOperator( opItem->getFieldName(), opItem->getElement(),
                               opItem->getOpCode(), funcList, parent,
                               opItem->getParamIndex(),
                               opItem->getFuzzyIndex(),
                               &retNode ) ;
            break ;
      }

      PD_RC_CHECK( rc, PDERROR, "Failed to create operator, rc: %d", rc ) ;

      opItem->setOpNode( retNode ) ;

   done :
      return rc ;
   error :
      _clearFuncList( funcList ) ;
      goto done ;
   }

   INT32 _mthMatchTree::_parseRegExElement( const BSONElement &ele,
                                            _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      MTH_FUNC_LIST empty ;

      rc = _addRegExOp( ele.fieldName(), ele.regex(), ele.regexFlags(), empty,
                        parent ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to _addRegExOp:ele=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_parseNormalElement( const BSONElement &ele,
                                             _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      MTH_FUNC_LIST empty ;
      const CHAR *eFieldName = ele.fieldName() ;

      if ( MTH_OPERATOR_EYECATCHER == eFieldName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "operator can not in the head:ele=%s",
                  ele.toString().c_str() ) ;
         goto error ;
      }

      rc = _addOperator( ele.fieldName(), ele, EN_MATCH_OPERATOR_ET, empty,
                         parent, -1, -1 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed inject element, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_pareseLogicElemnts( const BSONElement ele,
                                             _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;

      BSONObjIterator iterArray( ele.embeddedObject() ) ;

      while ( iterArray.more() )
      {
         BSONElement eleArrayItem = iterArray.next() ;
         if ( Object != eleArrayItem.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Array item's element type must be Object:"
                     "item=%s,rc=%d", eleArrayItem.toString().c_str(), rc ) ;
            goto error ;
         }

         {
            _mthMatchLogicNode *tmpParent = parent ;
            if ( parent->getType() == EN_MATCH_OPERATOR_LOGIC_OR ||
                 parent->getType() == EN_MATCH_OPERATOR_LOGIC_NOT )
            {
               _mthMatchLogicNode *child = NULL ;
               child = mthGetMatchNodeFactory()->createLogicNode(
                                             &_allocator, getMatchConfigPtr(),
                                             EN_MATCH_OPERATOR_LOGIC_AND ) ;
               if ( NULL == child )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "create logicAnd failed:rc=%d", rc ) ;
                  goto error ;
               }

               rc = child->init( "", ele ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "init child failed:rc=%d", rc ) ;
                  mthGetMatchNodeFactory()->releaseNode( child ) ;
                  goto error ;
               }

               rc = parent->addChild( child ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,"
                          "rc=%d", parent->toString().c_str(),
                          child->toString().c_str(), rc ) ;
                  mthGetMatchNodeFactory()->releaseNode( child ) ;
                  goto error ;
               }

               tmpParent = child ;
            }

            BSONObjIterator iterObject( eleArrayItem.embeddedObject() ) ;
            while ( iterObject.more() )
            {
               BSONElement eleTemp = iterObject.next() ;
               rc = _parseElement ( eleTemp, tmpParent ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to _parseElement:eleTemp=%s,"
                           "rc=%d", eleTemp.toString().c_str(), rc ) ;
                  goto error ;
               }
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_pareseLogicAnd( const BSONElement ele,
                                         _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      _mthMatchLogicNode *logicAnd = NULL ;
      BOOLEAN hasAddToTree = FALSE ;

      if ( ele.type() != Array )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "LogicAnd's element type must be Array:ele=%s,rc=%d",
                  ele.toString().c_str(), rc );
         goto error ;
      }

      logicAnd = mthGetMatchNodeFactory()->createLogicNode(
                                          &_allocator, getMatchConfigPtr(),
                                          EN_MATCH_OPERATOR_LOGIC_AND ) ;
      if ( !logicAnd )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate memory for "
                  "EN_MATCH_OPERATOR_LOGIC_AND:rc=%d", rc ) ;
         goto error ;
      }

      rc = logicAnd->init( "", ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init logicAnd failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = parent->addChild( logicAnd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                 parent->toString().c_str(), logicAnd->toString().c_str(),
                 rc ) ;
         goto error ;
      }

      hasAddToTree = TRUE ;

      rc = _pareseLogicElemnts( ele, logicAnd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_pareseLogicElemnts failed:ele=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( logicAnd ) ;
         logicAnd = NULL ;
      }
      /* else */
      goto done ;
   }

   INT32 _mthMatchTree::_pareseLogicOr( const BSONElement ele,
                                        _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      _mthMatchLogicNode *logicOr  = NULL ;
      BOOLEAN hasAddToTree = FALSE ;

      if ( ele.type() != Array )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "LogicAnd's element type must be Array:ele=%s,rc=%d",
                  ele.toString().c_str(), rc );
         goto error ;
      }

      logicOr = mthGetMatchNodeFactory()->createLogicNode(
                                             &_allocator, getMatchConfigPtr(),
                                             EN_MATCH_OPERATOR_LOGIC_OR ) ;
      if ( !logicOr )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate memory for "
                  "EN_MATCH_OPERATOR_LOGIC_OR:rc=%d", rc ) ;
         goto error ;
      }

      rc = logicOr->init( "", ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init logicOr failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = parent->addChild( logicOr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                 parent->toString().c_str(), logicOr->toString().c_str(),
                 rc ) ;
         goto error ;
      }

      hasAddToTree = TRUE ;

      rc = _pareseLogicElemnts( ele, logicOr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_pareseLogicElemnts failed:ele=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( logicOr ) ;
         logicOr = NULL ;
      }
      /* else */
      goto done ;
   }

   INT32 _mthMatchTree::_pareseLogicNot( const BSONElement ele,
                                         _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      _mthMatchLogicNode *logicNot = NULL ;
      BOOLEAN hasAddToTree = FALSE ;

      if ( ele.type() != Array )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "LogicAnd's element type must be Array:ele=%s,rc=%d",
                  ele.toString().c_str(), rc );
         goto error ;
      }

      logicNot = mthGetMatchNodeFactory()->createLogicNode(
                                          &_allocator, getMatchConfigPtr(),
                                          EN_MATCH_OPERATOR_LOGIC_NOT ) ;
      if ( !logicNot )
      {
         rc = SDB_OOM ;
         PD_LOG ( PDERROR, "Failed to allocate memory for "
                  "EN_MATCH_OPERATOR_LOGIC_OR:rc=%d", rc ) ;
         goto error ;
      }

      rc = logicNot->init( "", ele ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "init logicNot failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = parent->addChild( logicNot ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "add child failed:parent=%s,child=%s,rc=%d",
                 parent->toString().c_str(), logicNot->toString().c_str(),
                 rc ) ;
         goto error ;
      }

      hasAddToTree = TRUE ;

      rc = _pareseLogicElemnts( ele, logicNot ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_pareseLogicElemnts failed:ele=%s,rc=%d",
                 ele.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( !hasAddToTree )
      {
         _releaseTree( logicNot ) ;
         logicNot = NULL ;
      }
      /* else */
      goto done ;
   }

   INT32 _mthMatchTree::_parseArrayElement( const BSONElement &ele,
                                            _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      const CHAR *fieldName = ele.fieldName() ;

      if ( MTH_OPERATOR_EYECATCHER == fieldName[0] )
      {
         EN_MATCH_OP_FUNC_TYPE nodeType ;
         nodeType = mthGetMatchNodeFactory()->getMatchNodeType( fieldName ) ;
         if ( EN_MATCH_OPERATOR_LOGIC_AND == nodeType )
         {
            rc = _pareseLogicAnd( ele, parent ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_pareseLogicAnd failed:ele=%s,rc=%d",
                       ele.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         else if ( EN_MATCH_OPERATOR_LOGIC_OR == nodeType )
         {
            rc = _pareseLogicOr( ele, parent ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_pareseLogicOr failed:ele=%s,rc=%d",
                       ele.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         else if ( EN_MATCH_OPERATOR_LOGIC_NOT == nodeType )
         {
            rc = _pareseLogicNot( ele, parent ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_pareseLogicNot failed:ele=%s,rc=%d",
                       ele.toString().c_str(), rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "unsupported logic operation:ele=%s,rc=%d",
                     ele.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         MTH_FUNC_LIST empty ;
         rc = _addOperator( ele.fieldName(), ele, EN_MATCH_OPERATOR_ET,
                            empty, parent, -1, -1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "_addOperator failed:ele=%s,rc=%d",
                    ele.toString().c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_checkInnerObject ( const BSONElement &ele,
                                            UINT32 level,
                                            BSONElement *pOutEle )
   {
      INT32 rc = SDB_OK ;

      if ( NULL != pOutEle )
      {
         (*pOutEle) = ele ;
      }

      switch ( ele.type() )
      {
         case Array :
         {
            const BSONObj &boInner = ele.embeddedObject() ;
            BSONObjIterator iter( boInner ) ;
            while ( iter.more() )
            {
               const BSONElement &beSub = iter.next() ;
               rc = _checkInnerObject( beSub, level + 1, NULL ) ;
               PD_RC_CHECK( rc, PDERROR, "Inner array %s is invalid",
                            ele.toString( TRUE, TRUE ).c_str() ) ;
            }
            break ;
         }
         case Object :
         {
            BOOLEAN hasRegex = FALSE ;
            BOOLEAN hasOptions = FALSE ;

            const BSONObj &boInner = ele.embeddedObject() ;
            BSONObjIterator iter( boInner ) ;
            while ( iter.more() )
            {
               const BSONElement &beSub = iter.next() ;
               const CHAR *subName = beSub.fieldName() ;

               if ( MTH_OPERATOR_EYECATCHER == subName[0] )
               {
                  EN_MATCH_OP_FUNC_TYPE subType =
                        mthGetMatchNodeFactory()->getMatchNodeType( subName ) ;

                  if ( EN_MATCH_OPERATOR_FIELD == subType )
                  {
                     PD_CHECK( 1 == boInner.nFields() && 0 == level &&
                               String == beSub.type(),
                               SDB_INVALIDARG, error, PDERROR,
                               "Inner $field %s is invalid",
                               ele.toString( TRUE, TRUE ).c_str() ) ;
                     if ( NULL != pOutEle )
                     {
                        (*pOutEle) = beSub ;
                     }
                  }
                  else if ( EN_MATCH_OPERATOR_REGEX == subType )
                  {
                     PD_CHECK( !hasRegex && String == beSub.type(),
                               SDB_INVALIDARG, error, PDERROR,
                               "Inner $regex %s is invalid",
                               ele.toString( TRUE, TRUE ).c_str() ) ;
                     hasRegex = TRUE ;
                  }
                  else if ( EN_MATCH_OPERATOR_OPTIONS == subType )
                  {
                     PD_CHECK( !hasOptions && String == beSub.type(),
                               SDB_INVALIDARG, error, PDERROR,
                               "Inner $options %s is invalid",
                               ele.toString( TRUE, TRUE ).c_str() ) ;
                     hasOptions = TRUE ;
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG( PDERROR, "Inner operator %s is invalid",
                             ele.toString( TRUE, TRUE ).c_str() ) ;
                     goto error ;
                  }
               }
               else
               {
                  PD_CHECK( !hasRegex && !hasOptions,
                            SDB_INVALIDARG, error, PDERROR,
                            "Inner object %s is invalid",
                            ele.toString( TRUE, TRUE).c_str() ) ;
                  rc = _checkInnerObject( beSub, level + 1, NULL ) ;
                  PD_RC_CHECK( rc, PDERROR, "Inner object %s is invalid",
                               ele.toString( TRUE, TRUE ).c_str() ) ;
               }
            }

            if ( hasRegex && hasOptions )
            {
               PD_CHECK( 2 == boInner.nFields(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Inner $regex %s is invalid",
                         ele.toString( TRUE, TRUE ).c_str() ) ;
            }
            else if ( hasRegex )
            {
               PD_CHECK( 1 == boInner.nFields(),
                         SDB_INVALIDARG, error, PDERROR,
                         "Inner $regex %s is invalid",
                         ele.toString( TRUE, TRUE ).c_str() ) ;
            }
            else if ( hasOptions )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Inner $options %s is invalid",
                       ele.toString( TRUE, TRUE ).c_str() ) ;
               goto error ;
            }
            break ;
         }
         default :
            break ;
      }

      rc = SDB_OK ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _mthMatchTree::_getElementKeysFormat( const BSONElement &ele )
   {
      const CHAR *eFieldName = NULL ;
      INT32 opCount = 0 ;
      INT32 normalCount = 0 ;
      BSONElement temEle ;
      if ( Object == ele.type() || Array == ele.type() )
      {
         BSONObjIterator j( ele.embeddedObject() ) ;
         while ( j.more () )
         {
            temEle     = j.next() ;
            eFieldName = temEle.fieldName() ;
            if ( MTH_OPERATOR_EYECATCHER == eFieldName[0] )
            {
               opCount++ ;
            }
            else
            {
               normalCount++ ;
            }
         }
      }
      else
      {
         normalCount = 1 ;
      }

      if ( opCount > 0  )
      {
         if ( normalCount > 0 )
         {
            return MTH_ELEMENT_KEY_MIX ;
         }
         else
         {
            return MTH_ELEMENT_KEY_ALL_OP ;
         }
      }
      else
      {
         return MTH_ELEMENT_KEY_ALL_NORMAL ;
      }
   }

   INT32 _mthMatchTree::_parseAttribute( const CHAR *fieldName,
                                         const BSONElement &ele,
                                         EN_MATCH_OP_FUNC_TYPE nodeType,
                                         MTH_FUNC_LIST &funcList )
   {
      INT32 rc = SDB_OK ;
      if ( EN_MATCH_ATTR_EXPAND == nodeType )
      {
         if ( _hasExpand ||
              ( _hasReturnMatch && ossStrcmp( _attrFieldName, fieldName ) != 0 ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "%s and %s can't describe two fields",
                    MTH_ATTR_STR_EXPAND, MTH_ATTR_STR_RETURNMATCH ) ;
            goto error ;
         }

         rc = _addFunction( fieldName, ele, EN_MATCH_ATTR_EXPAND,
                            funcList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "add function failed:fieldName=%s,ele=%s,rc=%d",
                    fieldName, ele.toString().c_str(), rc ) ;
            goto error ;
         }

         _hasExpand     = TRUE ;
         _attrFieldName = fieldName ;
      }
      else if ( EN_MATCH_ATTR_RETURNMATCH == nodeType )
      {
         if ( _hasReturnMatch ||
              ( _hasExpand && ossStrcmp( _attrFieldName, fieldName ) != 0 ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "%s and %s can't describe two fields",
                    MTH_ATTR_STR_EXPAND, MTH_ATTR_STR_RETURNMATCH ) ;
            goto error ;
         }

         rc = _addFunction( fieldName, ele, EN_MATCH_ATTR_RETURNMATCH,
                            funcList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "add function failed:fieldName=%s,ele=%s,rc=%d",
                    fieldName, ele.toString().c_str(), rc ) ;
            goto error ;
         }

         _hasReturnMatch = TRUE ;
         _attrFieldName  = fieldName ;
      }
      else
      {
         SDB_ASSERT( FALSE, "impossible" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_pareseObjectInnerOp( const BSONElement &ele,
                                              const BSONElement &innerEle,
                                              MTH_FUNC_LIST &funcList,
                                              _mthMatchLogicNode *parent,
                                              const char *&regex,
                                              const char *&options )
   {
      INT32 rc = SDB_OK ;
      const CHAR *innerFieldName = NULL ;
      EN_MATCH_OP_FUNC_TYPE nodeType ;

      innerFieldName = innerEle.fieldName() ;
      nodeType       = mthGetMatchNodeFactory()->getMatchNodeType(
                                                              innerFieldName ) ;
      if ( EN_MATCH_OPERATOR_REGEX == nodeType ||
           EN_MATCH_OPERATOR_OPTIONS == nodeType )
      {
         if ( innerEle.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "regex's type should be String type:fieldName=%s,"
                    "innerEle=%s,type=%d", ele.fieldName(),
                    innerEle.toString().c_str(), innerEle.type() ) ;
            goto error ;
         }
         if ( EN_MATCH_OPERATOR_REGEX == nodeType )
         {
            regex = innerEle.valuestrsafe() ;
         }
         else
         {
            options = innerEle.valuestrsafe() ;
         }

         goto done ;
      }

      if ( EN_MATCH_ATTR_EXPAND == nodeType ||
           EN_MATCH_ATTR_RETURNMATCH == nodeType )
      {
         rc = _parseAttribute( ele.fieldName(), innerEle, nodeType,
                               funcList ) ;
         PD_RC_CHECK( rc, PDERROR, "_parseAttribute failed:rc=%d", rc ) ;
         goto done ;
      }

      if ( Object != innerEle.type() && Array != innerEle.type() )
      {
         if ( EN_MATCH_OPERATOR_MOD == nodeType )
         {
            rc = _addFunction( ele.fieldName(), innerEle, nodeType,
                               funcList ) ;
            PD_RC_CHECK( rc, PDERROR, "add function failed:fieldName=%s,"
                         "innerEle=%s,rc=%d", ele.fieldName(),
                         innerEle.toString().c_str(), rc ) ;
         }
         else if ( ( nodeType < EN_MATCH_OPERATOR_END &&
                     nodeType >= EN_MATCH_OPERATOR_ET ) )
         {
            rc = _addOperator( ele.fieldName(), innerEle, nodeType, funcList,
                               parent, -1, -1 ) ;
            PD_RC_CHECK( rc, PDERROR, "_addOperator failed:innerEle=%s,rc=%d",
                         innerEle.toString().c_str(), rc ) ;
         }
         else if ( nodeType < EN_MATCH_FUNC_END &&
                   nodeType >= EN_MATCH_FUNC_ABS )
         {
            rc = _addFunction( ele.fieldName(), innerEle, nodeType, funcList ) ;
            PD_RC_CHECK( rc, PDERROR, "add function failed:fieldName=%s,"
                         "innerEle=%s,rc=%d", ele.fieldName(),
                         innerEle.toString().c_str(), rc ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "unreconigzed operator:embEle=%s,rc=%d",
                    innerEle.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         BSONElement beSub ;

         if ( nodeType >= EN_MATCH_OPERATOR_LOGIC_AND &&
              nodeType < EN_MATCH_OPERATOR_LOGIC_END )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Unsupported inner matching operator [%s]",
                    innerEle.toString( TRUE, TRUE ).c_str() ) ;
            goto error ;
         }

         if ( EN_MATCH_OPERATOR_ELEMMATCH != nodeType )
         {
            rc = _checkInnerObject( innerEle, 0, &beSub ) ;
            PD_RC_CHECK( rc, PDERROR, "Unsupported inner matching operator [%s]",
                         innerEle.toString( TRUE, TRUE ).c_str() ) ;
         }
         else
         {
            beSub = innerEle ;
         }

         if ( nodeType >= EN_MATCH_OPERATOR_ET &&
              nodeType < EN_MATCH_OPERATOR_END )
         {
            rc = _addOperator( ele.fieldName(), beSub, nodeType, funcList,
                               parent, -1, -1 ) ;
            PD_RC_CHECK( rc, PDERROR, "_addOperator failed:innerEle=%s,rc=%d",
                         innerEle.toString().c_str(), rc ) ;
         }
         else
         {
            rc = _addFunction( ele.fieldName(), innerEle, nodeType,
                               funcList ) ;
            PD_RC_CHECK( rc, PDERROR, "add function failed:fieldName=%s,"
                         "innerEle=%s,rc=%d", ele.fieldName(),
                         innerEle.toString().c_str(), rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_paresePrevOptions( const CHAR *fieldName,
                                            const BSONElement &ele,
                                            const CHAR *options,
                                            MTH_FUNC_LIST &funcList,
                                            _mthMatchLogicNode *parent )
   {
      INT32 rc = SDB_OK ;
      const CHAR *tmpRegex = NULL ;
      EN_MATCH_OP_FUNC_TYPE type ;
      type = mthGetMatchNodeFactory()->getMatchNodeType( ele.fieldName() ) ;
      if ( EN_MATCH_OPERATOR_REGEX != type )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "exist extra options operator:options=%s",
                 options ) ;
         goto error ;
      }

      tmpRegex = ele.valuestrsafe() ;
      rc = _addRegExOp( fieldName, tmpRegex, options, funcList, parent ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_addRegExOp failed:fieldName=%s,"
                 "regex=%s,options=%s,rc=%d", fieldName, tmpRegex,
                 options, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchTree::_clearFuncList( MTH_FUNC_LIST &funcList )
   {
      MTH_FUNC_LIST::iterator iter = funcList.begin() ;
      while ( iter != funcList.end() )
      {
         _mthMatchFunc *func = *iter ;
         mthGetMatchNodeFactory()->releaseFunc( func ) ;
         iter++ ;
      }

      funcList.clear() ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_PARSEOBJECTELEMENT, "_mthMatchTree::_parseObjectElement" )
   INT32 _mthMatchTree::_parseObjectElement( const BSONElement &ele,
                                             _mthMatchLogicNode *parent )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_PARSEOBJECTELEMENT ) ;
      INT32 rc = SDB_OK ;
      INT32 keysFormat = 0 ;
      MTH_FUNC_LIST funcList ;
      const CHAR *fieldName = ele.fieldName() ;

      if ( MTH_OPERATOR_EYECATCHER == fieldName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "operator can not in the head:ele=%s",
                      ele.toString().c_str() ) ;
      }

      keysFormat = _getElementKeysFormat( ele ) ;
      if ( MTH_ELEMENT_KEY_ALL_NORMAL == keysFormat )
      {
         MTH_FUNC_LIST empty ;
         rc = _addOperator( ele.fieldName(), ele, EN_MATCH_OPERATOR_ET, empty,
                            parent, -1, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "_addOperator failed:ele=%s,rc=%d",
                      ele.toString().c_str(), rc ) ;

         goto done ;
      }
      else if ( MTH_ELEMENT_KEY_MIX == keysFormat )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "object's element do not allow exist mix op keys and "
                 "normal keys:ele=%s,rc=%d", ele.toString().c_str(), rc ) ;
         goto error ;
      }
      else
      {
         BSONObjIterator iter( ele.embeddedObject() ) ;
         const CHAR *regex   = NULL ;
         const CHAR *options = NULL ;
         while ( iter.more() )
         {
            BSONElement embEle       = iter.next () ;
            const CHAR *embFieldName = embEle.fieldName () ;

            SDB_ASSERT( NULL != options ? NULL == regex : TRUE,
                        "if options is not null, regex must be null" ) ;
            SDB_ASSERT( NULL != regex ? NULL == options : TRUE,
                        "if regex is not null, options must be null" ) ;
            if ( NULL != options )
            {
               rc = _paresePrevOptions( fieldName, embEle, options, funcList,
                                        parent ) ;
               PD_RC_CHECK( rc, PDERROR, "_paresePrevOptions failed:"
                            "fieldName=%s,options=%s,rc=%d",
                            fieldName, options, rc ) ;

               options = NULL ;
               continue ;
            }

            if ( NULL != regex )
            {
               EN_MATCH_OP_FUNC_TYPE type ;
               type = mthGetMatchNodeFactory()->getMatchNodeType(
                                                                embFieldName ) ;
               if ( EN_MATCH_OPERATOR_OPTIONS == type )
               {
                  const CHAR *tmpOptions = embEle.valuestrsafe() ;
                  rc = _addRegExOp( fieldName, regex, tmpOptions, funcList,
                                    parent ) ;
                  PD_RC_CHECK( rc, PDERROR, "_addRegExOp failed:fieldName=%s,"
                               "regex=%s,options=%s,rc=%d", fieldName, regex,
                               tmpOptions, rc ) ;

                  regex = NULL ;
                  continue ;
               }

               rc = _addRegExOp( fieldName, regex, NULL, funcList, parent ) ;
               PD_RC_CHECK( rc, PDERROR, "_addRegExOp failed:fieldName=%s,"
                            "regex=%s,rc=%d", fieldName, regex, rc ) ;

               regex = NULL ;
            }

            rc = _pareseObjectInnerOp( ele, embEle, funcList, parent, regex,
                                       options ) ;
            PD_RC_CHECK( rc, PDERROR, "_pareseObjectInnerOp failed:rc=%d", rc ) ;
         }

         if ( NULL != regex )
         {
            rc = _addRegExOp( fieldName, regex, options, funcList, parent ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_addRegExOp failed:fieldName=%s,"
                       "regex=%s,rc=%d", fieldName, regex, rc ) ;
               goto error ;
            }
            regex   = NULL ;
            options = NULL ;
         }

         if ( NULL != options )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "exist extra options operator:options=%s",
                    options ) ;
            goto error ;
         }

         if ( funcList.size() > 0 )
         {
            if ( funcList.size() == 1 )
            {
               MTH_FUNC_LIST::iterator iter = funcList.begin() ;
               _mthMatchFunc *func = *iter ;
               if ( func->getType() == EN_MATCH_ATTR_EXPAND )
               {
                  BSONElement expandEle ;
                  _mthMatchFuncEXPAND *expandFunc = NULL ;
                  expandFunc = dynamic_cast< _mthMatchFuncEXPAND * > (func) ;
                  if ( NULL == expandFunc )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_RC_CHECK( rc, PDERROR, "dynamic_cast(func->FuncEXPAND)"
                                  " failed:node=%s,rc=%d",
                                  func->toString().c_str(), rc ) ;
                  }

                  expandFunc->getElement( expandEle ) ;
                  rc = _addExpandOp( expandFunc->getFieldName(), expandEle,
                                     parent ) ;
                  PD_RC_CHECK( rc, PDERROR, "_addExpandOp failed:fieldName=%s,"
                               "expandObj=%s,rc=%d", expandEle.fieldName(),
                               expandEle.toString().c_str(), rc ) ;

                  _clearFuncList( funcList ) ;
                  goto done ;
               }
            }

            rc = SDB_INVALIDARG ;
            MTH_FUNC_LIST::iterator iter = funcList.begin() ;
            PD_LOG( PDERROR, "exist extra func:first func=%s",
                    ( *iter )->toString().c_str() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_PARSEOBJECTELEMENT, rc ) ;
      return rc ;
   error:
      _clearFuncList( funcList ) ;
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_PARSEELEMENT, "_mthMatchTree::_parseElement" )
   INT32 _mthMatchTree::_parseElement( const BSONElement &ele,
                                       _mthMatchLogicNode *parent )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_PARSEELEMENT ) ;

      INT32 rc = SDB_OK ;
      switch ( ele.type() )
      {
      case RegEx:
         rc = _parseRegExElement( ele, parent ) ;
         break ;

      case Object:
         rc = _parseObjectElement( ele, parent ) ;
         break ;

      case Array:
         rc = _parseArrayElement( ele, parent ) ;
         break ;

      default:
         rc = _parseNormalElement( ele, parent ) ;
      }

      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_PARSEELEMENT, rc ) ;
      return rc ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE__PRELOADPATTERN, "_mthMatchTree::_preLoadPattern" )
   INT32 _mthMatchTree::_preLoadPattern ( const BSONObj &matcher,
                                          BOOLEAN enableMixCmp,
                                          BOOLEAN parameterized,
                                          BOOLEAN fuzzyOptr,
                                          BOOLEAN copyQuery )
   {
      INT32 rc      = SDB_OK ;
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE__PRELOADPATTERN ) ;

      _matchPattern = copyQuery ? matcher.copy() : matcher ;
      _isMatchesAll = TRUE ;

      setMthEnableMixCmp( enableMixCmp ) ;
      setMthEnableParameterized( parameterized ) ;
      setMthEnableFuzzyOptr( fuzzyOptr ) ;

      _root = mthGetMatchNodeFactory()->createLogicNode(
                                             &_allocator, getMatchConfigPtr(),
                                             EN_MATCH_OPERATOR_LOGIC_AND ) ;
      PD_CHECK( NULL != _root, SDB_OOM, error, PDERROR,
                "Failed to allocate memory for EN_MATCH_OPERATOR_LOGIC_AND, "
                "rc: %d", rc ) ;

      rc = _root->init( "", BSONObj().firstElement() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init logicNode, rc: %d", rc ) ;

   done :
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE__PRELOADPATTERN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE__POSTLOADPATTERN, "_mthMatchTree::_postLoadPattern" )
   INT32 _mthMatchTree::_postLoadPattern ( BOOLEAN needOptimize )
   {
      INT32 rc      = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMATCHTREE__POSTLOADPATTERN ) ;

      if ( needOptimize )
      {
         rc = _optimize() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to optimize match tree, rc: %d",
                      rc ) ;
      }

      _checkTotallyConverted() ;

   done :
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE__POSTLOADPATTERN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_LOADPATTERN, "_mthMatchTree::loadPattern" )
   INT32 _mthMatchTree::loadPattern ( const BSONObj &matcher,
                                      BOOLEAN needConfigMixCmp /* = TRUE */ )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_LOADPATTERN ) ;
      SDB_ASSERT ( !_isInitialized, "mthMatcher can't be initialized "
                   "multiple times" ) ;
      INT32 rc      = SDB_OK ;

      BOOLEAN mixCmp = needConfigMixCmp ?
                       sdbGetRTNCB()->isEnabledMixCmp() :
                       mthEnabledMixCmp() ;

      rc = _preLoadPattern( matcher, mixCmp, FALSE, FALSE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process before loading pattern, "
                   "rc: %d", rc ) ;

      try
      {
         BSONObjIterator i( _matchPattern ) ;

         while ( i.more() )
         {
            BSONElement temp = i.next() ;
            rc = _parseElement( temp, ( _mthMatchLogicNode* )_root ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse element %s, rc: %d",
                         temp.toString().c_str(), rc ) ;
            _isMatchesAll = FALSE ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to call loadPattern: %s", e.what() ) ;
         goto error ;
      }

      rc = _postLoadPattern( TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process after loading pattern, "
                   "rc: %d", rc ) ;

      _isInitialized = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_LOADPATTERN, rc ) ;
      return rc ;
   error :
      clear() ;   /* _root is cleared in clear() */
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_LOADPATTERN_NOR, "_mthMatchTree::loadPattern" )
   INT32 _mthMatchTree::loadPattern ( const BSONObj &matcher,
                                      mthMatchNormalizer &normalizer )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_LOADPATTERN_NOR ) ;

      SDB_ASSERT ( !_isInitialized, "mthMatcher can't be initialized "
                   "multiple times" ) ;

      rc = _preLoadPattern( matcher,
                            normalizer.mthEnabledMixCmp(),
                            normalizer.mthEnabledParameterized(),
                            normalizer.mthEnabledFuzzyOptr(),
                            FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process before loading pattern, "
                   "rc: %d", rc ) ;

      try
      {
         for ( UINT8 i = 0 ; i < normalizer.getItemNumber() ; i ++ )
         {
            mthMatchOpItem *opItem = normalizer.getOpItem( i ) ;

            rc = _parseOpItem( opItem, (_mthMatchLogicNode *)_root ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse operator item, rc: %d",
                         rc ) ;

            _isMatchesAll = FALSE ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Failed to call loadPattern: %s", e.what() ) ;
         goto error ;
      }

      rc = _postLoadPattern( FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process after loading pattern, "
                   "rc: %d", rc ) ;

      _isInitialized = TRUE ;

   done :
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_LOADPATTERN_NOR, rc ) ;
      return rc ;
   error :
      clear() ;   /* _root is cleared in clear() */
      goto done ;
   }

   INT32 _mthMatchTree::_deleteNode( _mthMatchNode *parent,
                                     _mthMatchNode *node )
   {
      INT32 rc = SDB_OK ;

      while ( node->getChildrenCount() > 0 )
      {
         _mthMatchNodeIterator iter( node ) ;
         _mthMatchNode *child = iter.next() ;

         node->delChild( child ) ;

         rc = parent->addChild( child ) ;
         if ( SDB_OK != rc )
         {
            mthGetMatchNodeFactory()->releaseNode( child ) ;
            PD_LOG( PDERROR, "add child failed:rc=%d" ) ;

            goto error ;
         }
      }

      parent->delChild( node ) ;
      mthGetMatchNodeFactory()->releaseNode( node ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_deleteExtraLogicNode( _mthMatchNode *node )
   {
      INT32 rc = SDB_OK ;
      if ( node->getType() > EN_MATCH_OPERATOR_LOGIC_END )
      {
         goto done ;
      }

      {
         UINT32 idx = 0 ;
         MATCHNODE_VECTOR childrenVec ;
         _mthMatchNodeIterator iter( node ) ;
         while ( iter.more() )
         {
            _mthMatchNode *child = iter.next() ;
            childrenVec.push_back( child ) ;
         }

         for ( idx = 0 ; idx < childrenVec.size() ; idx++ )
         {
            rc = _deleteExtraLogicNode( childrenVec[idx] ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "delete extra logic node failed:rc=%d", rc ) ;
               goto error ;
            }
         }
      }

      if ( ( node->getType() == EN_MATCH_OPERATOR_LOGIC_OR ||
             node->getType() == EN_MATCH_OPERATOR_LOGIC_AND ) &&
             node->getChildrenCount() == 1 )
      {
         _mthMatchNode *parent = node->getParent() ;
         if ( NULL != parent )
         {
            SDB_ASSERT( parent->getType() >= EN_MATCH_OPERATOR_LOGIC_AND &&
                        parent->getType() < EN_MATCH_OPERATOR_LOGIC_END,
                        "parent must be logic node" ) ;

            rc = _deleteNode( parent, node ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "_deleteNode failed:rc=%d", rc ) ;
               goto error ;
            }
            goto done ;
         }
      }

      if ( node->getType() == EN_MATCH_OPERATOR_LOGIC_AND )
      {
         _mthMatchNode *parent = node->getParent() ;
         if ( NULL != parent )
         {
            if ( parent->getType() == EN_MATCH_OPERATOR_LOGIC_AND ||
                 parent->getType() == EN_MATCH_OPERATOR_LOGIC_NOT )
            {
               rc = _deleteNode( parent, node ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "_deleteNode failed:rc=%d", rc ) ;
                  goto error ;
               }

               goto done ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMatchTree::_optimizeNodeLevel()
   {
      INT32 rc = SDB_OK ;
      UINT32 childrenCount = 0 ;

      childrenCount = _root->getChildrenCount() ;
      if ( 0 == childrenCount )
      {
         goto done ;
      }

      rc = _deleteExtraLogicNode( _root ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_deleteExtraLogicNode failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( _root->getChildrenCount() == 1 &&
           _root->getType() == EN_MATCH_OPERATOR_LOGIC_AND )
      {
         _mthMatchNodeIterator iter( _root ) ;
         _mthMatchNode *child = iter.next() ;
         if ( child->getType() < EN_MATCH_OPERATOR_LOGIC_END &&
              child->getType() >= EN_MATCH_OPERATOR_LOGIC_AND )
         {
            _root->delChild( child ) ;
            mthGetMatchNodeFactory()->releaseNode( _root ) ;
            _root = child ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchTree::_setWeight( _mthMatchNode *node )
   {
      if ( node->getType() >= EN_MATCH_OPERATOR_LOGIC_AND &&
           node->getType() < EN_MATCH_OPERATOR_LOGIC_END )
      {
         _mthMatchNodeIterator iter( node ) ;
         UINT32 weight = 0 ;
         while ( iter.more() )
         {
            _mthMatchNode *child = iter.next() ;
            _setWeight( child ) ;
            weight += child->getWeight() ;
         }

         node->setWeight( weight ) ;
      }
   }

   void _mthMatchTree::_sortByWeight()
   {
      _root->sortByWeight() ;
   }

   INT32 _mthMatchTree::calcPredicate ( rtnPredicateSet &predicateSet,
                                        const rtnParamList * paramList )
   {
      INT32 rc = SDB_OK ;

      if ( _isInitialized && _root )
      {
         rc = _root->calcPredicate( predicateSet, paramList ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "calc predicate failed:rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchTree::_checkTotallyConverted( _mthMatchNode *node,
                                               BOOLEAN &isTotallyConverted )
   {
      if ( !isTotallyConverted )
      {
         return ;
      }

      if ( !node->isTotalConverted() )
      {
         isTotallyConverted = FALSE ;
         return ;
      }

      _mthMatchNodeIterator iter( node ) ;
      while ( iter.more() )
      {
         _mthMatchNode *child = iter.next() ;
         _checkTotallyConverted( child, isTotallyConverted ) ;
         if ( !isTotallyConverted )
         {
            return ;
         }
      }
   }

   void _mthMatchTree::_checkTotallyConverted()
   {
      if ( _hasDollarFieldName )
      {
         _isTotallyConverted = FALSE ;
         return ;
      }

      _checkTotallyConverted( _root, _isTotallyConverted ) ;
   }

   INT32 _mthMatchTree::_optimize ()
   {
      INT32 rc = SDB_OK ;

      rc = _optimizeNodeLevel() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "_optimizeNodeLevel failed:rc=%d", rc ) ;
         goto error ;
      }

      _setWeight( _root ) ;

      _sortByWeight() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE_MATCHES, "_mthMatchTree::matches" )
   INT32 _mthMatchTree::matches( const BSONObj &matchTarget, BOOLEAN &result,
                                 _mthMatchTreeContext *context /* = NULL */,
                                 rtnParamList *parameters /* = NULL */ )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE_MATCHES ) ;
      INT32 rc = SDB_OK ;
      _mthMatchTreeContext innerContext ;

      if ( NULL == context )
      {
         _mthMatchTreeContext innerContext ;
         if ( parameters )
         {
            innerContext.bindParameters( parameters ) ;
         }
         rc = _matches( matchTarget, result, innerContext ) ;
         PD_RC_CHECK( rc, PDERROR, "_matches failed:rc=%d", rc ) ;
      }
      else
      {
         if ( !_hasDollarFieldName )
         {
            context->disableDollarList() ;
         }
         if ( parameters )
         {
            context->bindParameters( parameters ) ;
         }
         rc = _matches( matchTarget, result, *context ) ;
         PD_RC_CHECK( rc, PDERROR, "_matches failed:rc=%d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE_MATCHES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   ///PD_TRACE_DECLARE_FUNCTION ( SDB__MTHMATCHTREE__MATCHES, "_mthMatchTree::_matches" )
   INT32 _mthMatchTree::_matches( const BSONObj &matchTarget, BOOLEAN &result,
                                  _mthMatchTreeContext &context )
   {
      PD_TRACE_ENTRY( SDB__MTHMATCHTREE__MATCHES ) ;
      SDB_ASSERT( _isInitialized, "must be init first" ) ;
      INT32 rc = SDB_OK ;

      if ( _isMatchesAll )
      {
         result = TRUE ;
         goto done ;
      }

      if ( _hasExpand || _hasReturnMatch )
      {
         if ( _hasExpand )
         {
            context.setHasExpand( TRUE ) ;
         }

         if ( _hasReturnMatch )
         {
            context.setHasReturnMatch( TRUE ) ;
         }

         context.setFieldName( _attrFieldName ) ;
         if ( ossStrstr( _attrFieldName, ".$" ) != NULL )
         {
            /* if _attrFieldName exist Dollar, we must enable DollarList to
                  get the real fieldName. context.resolveFieldName().
            */
            context.enableDollarList() ;
         }
      }

      try
      {
         context.setObj( matchTarget ) ;
         result = FALSE ;
         rc = _root->execute( matchTarget, context, result ) ;
         PD_RC_CHECK( rc, PDERROR, "execute failed:target=%s,rc=%d",
                     matchTarget.toString().c_str(), rc ) ;

         if ( result )
         {
            if ( hasReturnMatch() && !context.isReturnMatchExecuted() )
            {
               BOOLEAN tmpResult = FALSE ;
               rc = _returnMatchNode->execute( matchTarget, context,
                                               tmpResult ) ;
               PD_RC_CHECK( rc, PDERROR, "execute failed:target=%s,rc=%d",
                            matchTarget.toString().c_str(), rc ) ;
            }

            rc = context.resolveFieldName() ;
            PD_RC_CHECK( rc, PDERROR, "resolveFieldName failed:rc=%d", rc ) ;

            rc = _adjustReturnMatchIndex( context ) ;
            PD_RC_CHECK( rc, PDERROR, "_adjust Index failed:rc=%d", rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to match: %s", e.what() ) ;
         goto error ;
      }


   done:
      PD_TRACE_EXITRC( SDB__MTHMATCHTREE__MATCHES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /* this function is to adjust the elments's index, in order to correct the
      elements's index point to the real original source obj's array index.
   */

   /* let's assume source obj is {a:[3, 7, 8, 10, 11, 15]}. after func $slice:[2,3]
      the func result is {a:[8, 10, 11]}. after matcher $in:[8,11,13].
      we have got the last result:{a:[8,11]}.

      now the elements's index just record the index[0, 2] base on
      the func result({a:[8, 10, 11]}). and we should adjust elements's index to
      index[2,4] that is base on the source obj({a:[3, 7, 8, 10, 11, 15]})
   */

   /* we save the source obj's index info to src and do the same func $slice:[2,3]
      so we can get the index base on the source obj.
      first, src records the source obj's index info src[0,1,2,3,4,5]
      second, let the index info do the same func $slice:[2,3]. and we have got dst[2,3,4]
      third, we have elements' index[0,2],
      at last, the original source obj' array index is dst[0] and dst[2], it's
      value is[2,4]
   */
   INT32 _mthMatchTree::_adjustReturnMatchIndex( _mthMatchTreeContext &context )
   {
      INT32 rc    = SDB_OK ;
      INT32 i     = 0 ;
      INT32 count = 0 ;
      _utilArray< INT32 > src ;
      _utilArray< INT32 > temp ;
      _utilArray< INT32 > dst ;
      MTH_FUNC_LIST funcList ;
      MTH_FUNC_LIST::iterator iter ;
      const CHAR *fieldName = NULL ;
      BSONElement ele ;
      if ( !hasReturnMatch() || !context.isUseElement() ||
           context._elements.size() == 0 )
      {
         goto done ;
      }

      fieldName = context.getFieldName() ;
      SDB_ASSERT( NULL != fieldName, "fieldName must exists" ) ;
      ele = context._originalObj.getFieldDotted( fieldName ) ;
      if ( ele.type() != Array )
      {
         context._elements.clear() ;
         context._isUseElement = FALSE ;
         goto done ;
      }

      count = ele.embeddedObject().nFields() ;
      for ( i = 0 ; i < count ; i++ )
      {
         src.append( i ) ;
      }

      temp.clear() ;
      rc = src.copyTo( temp ) ;
      PD_RC_CHECK( rc, PDERROR, "copyTo temp failed:rc=%d", rc ) ;

      _returnMatchNode->getFuncList( funcList ) ;

      if ( funcList.size() == 0 )
      {
         dst.clear() ;
         rc = src.copyTo( dst ) ;
         PD_RC_CHECK( rc, PDERROR, "copyTo dst failed:rc=%d", rc ) ;
      }
      else
      {
         iter = funcList.begin() ;
         while ( iter != funcList.end() )
         {
            _mthMatchFunc *func = *iter ;
            dst.clear() ;
            rc = func->adjustIndexForReturnMatch( temp, dst ) ;
            PD_RC_CHECK( rc, PDERROR, "adjust index failed:rc=%d", rc ) ;
            temp.clear() ;
            rc = dst.copyTo( temp ) ;
            PD_RC_CHECK( rc, PDERROR, "copyTo temp failed:rc=%d", rc ) ;

            iter++ ;
         }
      }

      if ( dst.size() < context._elements.size() ||
           src.size() < context._elements.size() )
      {
         context._elements.clear() ;
         context._isUseElement = FALSE ;
         goto done ;
      }

      for ( i = 0 ; i < ( INT32 )context._elements.size() ; i++ )
      {
         INT32 tempIndex = context._elements[i] ;
         SDB_ASSERT( tempIndex < ( INT32 )dst.size(), "out of bound" ) ;
         SDB_ASSERT( dst[tempIndex] < ( INT32 )src.size(), "out of bound" ) ;
         context._elements[i] = dst[tempIndex] ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthMatchTree::_releaseTree( _mthMatchNode *node )
   {
      if ( NULL == node )
      {
         return ;
      }

      _mthMatchNodeIterator iter( node ) ;
      while ( iter.more() )
      {
         _mthMatchNode *child = iter.next() ;
         _releaseTree( child ) ;
      }

      mthGetMatchNodeFactory()->releaseNode( node ) ;
   }

   INT32 _mthMatchTree::_createBuilder( BSONObjBuilder **builder )
   {
      SDB_ASSERT( NULL != builder, "builder should not be null" ) ;

      INT32 rc = SDB_OK ;
      BSONObjBuilder *b = SDB_OSS_NEW BSONObjBuilder() ;
      if ( !b )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to allocate memory for BSONObjBuilder" ) ;
      }

      try
      {
         _builderVec.push_back ( b ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_RC_CHECK( rc, PDERROR, "Failed to push builder to vector: %s",
                      e.what() );
      }

      *builder = b ;

   done:
      return rc ;
   error:
      if ( b )
      {
         SDB_OSS_DEL b ;
         b = NULL ;
      }
      goto done ;
   }

   void _mthMatchTree::_releaseBuilderVec(
                                    vector< BSONObjBuilder* > &builderVec )
   {
      vector<BSONObjBuilder*>::iterator it ;
      for ( it = builderVec.begin(); it < builderVec.end(); it++ )
      {
         SDB_OSS_DEL (*it) ;
      }

      builderVec.clear() ;
   }

   void _mthMatchTree::clear()
   {
      _releaseTree( _root ) ;
      _root = NULL ;

      _releaseBuilderVec( _builderVec ) ;

      _isInitialized      = FALSE ;
      _isMatchesAll       = TRUE ;
      _isTotallyConverted = TRUE ;
      _hasDollarFieldName = FALSE ;
      _matchPattern       = BSONObj() ;

      _hasExpand          = FALSE ;
      _hasReturnMatch     = FALSE ;
      _attrFieldName      = NULL ;
      _returnMatchNode    = NULL ;
   }

   BSONObj _mthMatchTree::getEqualityQueryObject( const rtnParamList *parameters )
   {
      BSONObj obj ;
      if ( NULL == _root )
      {
         goto done ;
      }

      try
      {
         BSONObjBuilder builder ;
         _root->extraEqualityMatches( builder, parameters ) ;
         obj = dotted2nested( builder.obj() ) ;
      }
      catch (  std::exception &e )
      {
         PD_LOG ( PDWARNING, "Failed to extract equality matches: %s",
                  e.what() ) ;
         goto done ;
      }

   done:
      return obj ;
   }

   BOOLEAN _mthMatchTree::isInitialized() const
   {
      return _isInitialized ;
   }

   BOOLEAN _mthMatchTree::isMatchesAll() const
   {
      return _isMatchesAll ;
   }

   BSONObj& _mthMatchTree::getMatchPattern()
   {
      return _matchPattern ;
   }

   BOOLEAN _mthMatchTree::hasDollarFieldName()
   {
      return _hasDollarFieldName ;
   }

   BOOLEAN _mthMatchTree::totallyConverted() const
   {
      return _isTotallyConverted ;
   }

   void _mthMatchTree::setMatchesAll( BOOLEAN matchesAll )
   {
      if ( ( _hasExpand || _hasReturnMatch ) && matchesAll )
      {
         return ;
      }

      _isMatchesAll = matchesAll ;
   }

   BSONObj _mthMatchTree::getParsedMatcher( const rtnParamList &parameters ) const
   {
      return NULL != _root ? _root->toParamBson( parameters ) : BSONObj() ;
   }

   BSONObj _mthMatchTree::toBson()
   {
      if ( NULL != _root )
      {
         return _root->toBson() ;
      }
      else
      {
         BSONObjBuilder builder ;
         return builder.obj() ;
      }
   }

   string _mthMatchTree::toString()
   {
      string output ;
      BSONObj obj ;

      obj = toBson() ;

      output = "Pattern:" + _matchPattern.toString() + "\n" ;
      output += "Root:" + obj.toString() + "\n" ;
      output += string( "isInitialized:" ) +
                ( _isInitialized ? "TRUE" : "FALSE" ) + "\n" ;
      output += string( "_isMatchesAll:" ) +
                ( _isMatchesAll ? "TRUE" : "FALSE" ) + "\n" ;
      output += string( "_isTotallyConverted:" ) +
                ( _isTotallyConverted ? "TRUE" : "FALSE" ) + "\n" ;
      output += string( "_hasDollarFieldName:" ) +
                ( _hasDollarFieldName ? "TRUE" : "FALSE" ) + "\n" ;

      return output ;
   }

   BOOLEAN _mthMatchTree::hasExpand()
   {
      SDB_ASSERT( ( _hasExpand || _hasReturnMatch ) ? NULL != _attrFieldName :
                                                      NULL == _attrFieldName,
                    "impossible" ) ;
      return _hasExpand ;
   }

   BOOLEAN _mthMatchTree::hasReturnMatch()
   {
      SDB_ASSERT( _hasReturnMatch ? NULL != _returnMatchNode
                                  : NULL == _returnMatchNode, "impossible" ) ;
      return _hasReturnMatch ;
   }

   const CHAR* _mthMatchTree::getAttrFieldName()
   {
      return _attrFieldName ;
   }

   void _mthMatchTree::evalEstimation ( optCollectionStat *pCollectionStat,
                                        double &estSelectivity,
                                        UINT32 &estCPUCost )
   {
      estSelectivity = OPT_MTH_DEFAULT_SELECTIVITY ;
      estCPUCost = OPT_MTH_DEFAULT_CPU_COST ;

      if ( !_isInitialized )
      {
         return ;
      }

      if ( _isMatchesAll && !hasExpand() && !hasReturnMatch() )
      {
         return ;
      }
      else if ( _root )
      {
         _root->evalEstimation( pCollectionStat, estSelectivity, estCPUCost ) ;
      }
   }

   _mthRecordGenerator::_mthRecordGenerator()
   {
      _index         = 0 ;
      _fieldName     = NULL ;
      _mthContext    = NULL ;
      _isQueryModify = FALSE ;
      _dataPtr       = 0 ;
      _totalNum      = 0 ;
      _validNum      = 0 ;
      _src           = BSONObj() ;
      _arrayObj      = BSONObj() ;
      _srcType       = MTH_SRC_TYPE_ORIGINAL ;

      _specifyObj    = BSONObj() ;
   }

   _mthRecordGenerator::~_mthRecordGenerator()
   {
      _index         = 0 ;
      _fieldName     = NULL ;
      _mthContext    = NULL ;
      _isQueryModify = FALSE ;
      _dataPtr       = 0 ;
      _totalNum      = 0 ;
      _validNum      = 0 ;
      _src           = BSONObj() ;
      _arrayObj      = BSONObj() ;
      _srcType       = MTH_SRC_TYPE_ORIGINAL ;

      _specifyObj    = BSONObj() ;
   }

   void _mthRecordGenerator::popFront( UINT32 num )
   {
      if ( _validNum > num )
      {
         _index    += num ;
         _validNum -= num ;
         if ( MTH_SRC_TYPE_ORIGINAL_SPLIT == _srcType )
         {
            while ( num > 0 )
            {
               SDB_ASSERT( _specifyIter.more(), "must have more" ) ;
               _specifyIter.next() ;
               num-- ;
            }
         }
      }
      else
      {
         _validNum = 0 ;
      }
   }

   void _mthRecordGenerator::popTail( UINT32 num )
   {
      if ( _validNum > num )
      {
         _validNum -= num ;
      }
      else
      {
         _validNum = 0 ;
      }
   }

   void _mthRecordGenerator::setQueryModify( BOOLEAN isQueryModify )
   {
      _isQueryModify = isQueryModify ;
   }

   void _mthRecordGenerator::setDataPtr( ossValuePtr &dataPtr )
   {
      _dataPtr = dataPtr ;
   }

   void _mthRecordGenerator::getDataPtr( ossValuePtr &dataPtr )
   {
      dataPtr = _dataPtr ;
   }

   INT32 _mthRecordGenerator::resetValue( const BSONObj &src,
                                          _mthMatchTreeContext *mthContext )
   {
      INT32 rc    = SDB_OK ;
      _index      = 0 ;
      _src        = src ;

      _mthContext = mthContext ;
      if ( _isQueryModify || NULL == mthContext )
      {
         _srcType  = MTH_SRC_TYPE_ORIGINAL ;
         _totalNum = 1 ;
         goto done ;
      }

      _fieldName = _mthContext->_fieldName.getFieldName() ;

      if ( ( _mthContext->hasReturnMatch() && _mthContext->isUseElement() ) )
      {
         BSONElement ele = _src.getFieldDotted( _fieldName ) ;
         _totalNum = _mthContext->_elements.size() ;
         if ( _mthContext->hasExpand() )
         {
            if ( ele.type() == Array && _totalNum > 0 )
            {
               _srcType = MTH_SRC_TYPE_ELEMENTS_SPLIT ;
               _arrayObj = ele.embeddedObject() ;
            }
            else
            {
               _srcType  = MTH_SRC_TYPE_ORIGINAL_NULL ;
               _totalNum = 1 ;
            }
         }
         else
         {
            if ( ele.type() == Array && _totalNum > 0 )
            {
               _arrayObj = ele.embeddedObject() ;
               _srcType  = MTH_SRC_TYPE_ELEMENTS ;
               _totalNum = 1 ;
            }
            else
            {
               _srcType  = MTH_SRC_TYPE_ORIGINAL_NULL ;
               _totalNum = 1 ;
            }
         }
      }
      else if ( _mthContext->hasExpand() )
      {
         BSONElement ele = _src.getFieldDotted( _fieldName ) ;
         if ( ele.type() == Array )
         {
            _srcType    = MTH_SRC_TYPE_ORIGINAL_SPLIT ;
            _specifyObj = ele.embeddedObject() ;
            _totalNum   = _specifyObj.nFields() ;
            if ( _totalNum == 0 )
            {
               _srcType  = MTH_SRC_TYPE_ORIGINAL_NULL ;
               _totalNum = 1 ;
            }
            else
            {
               BSONObjIterator iter( _specifyObj ) ;
               _specifyIter = iter ;
            }
         }
         else
         {
            _srcType = MTH_SRC_TYPE_ORIGINAL ;
            _totalNum = 1 ;
         }
      }
      else
      {
         _srcType = MTH_SRC_TYPE_ORIGINAL ;
         _totalNum = 1 ;
      }

   done:
      _validNum = _totalNum ;
      return rc ;
   }

   BOOLEAN _mthRecordGenerator::hasNext()
   {
      if ( _validNum > 0 )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   INT32 _mthRecordGenerator::_createArrayObj( const CHAR* name,
                                               _utilArray< INT32 > &elements,
                                               BSONObjBuilder &builder )
   {
      UINT32 i = 0 ;
      BSONArrayBuilder arrayBuilder ;

      if ( elements.size() == 0 )
      {
         return SDB_OK ;
      }

      for ( i = 0 ; i < elements.size() ; i++ )
      {
         CHAR indexStr[ 32 ] = "" ;
         BSONElement ele ;
         INT32 fieldIndex = _mthContext->_elements[i] ;
         ossItoa( fieldIndex, indexStr, 31 ) ;
         ele = _arrayObj.getField( indexStr ) ;
         arrayBuilder.append( ele ) ;
      }

      builder.append( name, arrayBuilder.arr() ) ;

      return SDB_OK ;
   }

   INT32 _mthRecordGenerator::_replaceFieldObject(
                                               BSONObjIteratorSorted &iterSort,
                                               const CHAR *fieldName,
                                               BSONElement &newValue,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      _mthMatchFieldName<> mthFieldName ;
      CHAR *pTmpFieldName = NULL ;
      CHAR *p = NULL ;

      rc = mthFieldName.setFieldName( fieldName ) ;
      PD_RC_CHECK( rc, PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                   fieldName, rc ) ;

      pTmpFieldName = ( CHAR * ) mthFieldName.getFieldName() ;
      p = ossStrchr ( pTmpFieldName, MTH_FIELDNAME_SEP ) ;
      if ( NULL != p )
      {
         *p = '\0' ;
      }

      while ( iterSort.more() )
      {
         INT32 cmp = 0 ;
         BSONElement ele = iterSort.next() ;

         cmp = ossStrcmp( ele.fieldName(), pTmpFieldName ) ;
         if ( cmp == 0 )
         {
            if ( NULL == p )
            {
               if ( !newValue.eoo() )
               {
                  builder.appendAs( newValue, pTmpFieldName ) ;
               }
               else
               {
                  builder.appendNull( pTmpFieldName ) ;
               }

               break ;
            }
            else
            {
               if ( ele.type() == Object )
               {
                  BSONObjBuilder bb( builder.subobjStart( pTmpFieldName ) ) ;
                  BSONObjIteratorSorted bis( ele.embeddedObject() ) ;
                  rc = _replaceFieldObject( bis, p+1, newValue, bb ) ;
               }
               else if ( ele.type() == Array )
               {
                  BSONArrayBuilder ba( builder.subarrayStart( pTmpFieldName ) ) ;
                  BSONObjIteratorSorted bis( ele.embeddedObject() ) ;
                  rc = _replaceFieldArray( bis, p+1, newValue, ba ) ;
               }
               else
               {
                  SDB_ASSERT( FALSE, "impossible" ) ;
               }
            }
         }
         else
         {
            builder.append( ele ) ;
         }
      }

      while ( iterSort.more() )
      {
         BSONElement ele = iterSort.next() ;
         builder.append( ele ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthRecordGenerator::_replaceFieldArray(
                                              BSONObjIteratorSorted &iterSort,
                                              const CHAR *fieldName,
                                              BSONElement &newValue,
                                              BSONArrayBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      _mthMatchFieldName<> mthFieldName ;
      CHAR *pTmpFieldName = NULL ;
      CHAR *p = NULL ;

      rc = mthFieldName.setFieldName( fieldName ) ;
      PD_RC_CHECK( rc, PDERROR, "set fieldName failed:fieldName=%s,rc=%d",
                   fieldName, rc ) ;

      pTmpFieldName = ( CHAR * ) mthFieldName.getFieldName() ;
      p = ossStrchr ( pTmpFieldName, MTH_FIELDNAME_SEP ) ;
      if ( NULL != p )
      {
         *p = '\0' ;
      }

      while ( iterSort.more() )
      {
         INT32 cmp = 0 ;
         BSONElement ele = iterSort.next() ;

         cmp = ossStrcmp( ele.fieldName(), pTmpFieldName ) ;
         if ( cmp == 0 )
         {
            if ( NULL == p )
            {
               if ( !newValue.eoo() )
               {
                  builder.append( newValue ) ;
               }
               else
               {
                  builder.appendNull() ;
               }

               break ;
            }
            else
            {
               if ( ele.type() == Object )
               {
                  BSONObjBuilder bb( builder.subobjStart( pTmpFieldName ) ) ;
                  BSONObjIteratorSorted bis( ele.embeddedObject() ) ;
                  rc = _replaceFieldObject( bis, p+1, newValue, bb ) ;
               }
               else if ( ele.type() == Array )
               {
                  BSONArrayBuilder ba( builder.subarrayStart( pTmpFieldName ) ) ;
                  BSONObjIteratorSorted bis( ele.embeddedObject() ) ;
                  rc = _replaceFieldArray( bis, p+1, newValue, ba ) ;
               }
               else
               {
                  SDB_ASSERT( FALSE, "impossible" ) ;
               }
            }
         }
         else
         {
            builder.append( ele ) ;
         }
      }

      while ( iterSort.more() )
      {
         BSONElement ele = iterSort.next() ;
         builder.append( ele ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthRecordGenerator::_replaceField( BSONObj &src,
                                             const CHAR *fieldName,
                                             BSONElement &newValue,
                                             BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BSONObjIteratorSorted iterSort( src ) ;
      rc = _replaceFieldObject( iterSort, fieldName, newValue, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "_replaceFieldObject failed:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthRecordGenerator::getNext( BSONObj &record )
   {
      SDB_ASSERT( _validNum > 0, "must hasNext" ) ;

      BSONObj tmpObj ;
      INT32 rc = SDB_OK ;

      if ( MTH_SRC_TYPE_ORIGINAL == _srcType )
      {
         record = _src ;
      }
      else
      {
         BSONObjBuilder builder( _src.objsize() ) ;
         BSONElement ele ;
         if ( MTH_SRC_TYPE_ORIGINAL_SPLIT == _srcType )
         {
            SDB_ASSERT( _specifyIter.more(), "should have next" ) ;
            ele = _specifyIter.next() ;
         }
         else if ( MTH_SRC_TYPE_ORIGINAL_NULL == _srcType )
         {
         }
         else if ( MTH_SRC_TYPE_ELEMENTS == _srcType )
         {
            BSONObjBuilder tmpBuilder ;

            rc = _createArrayObj( "tmp", _mthContext->_elements, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "_createArrayObj failed:rc=%d", rc ) ;
            tmpObj = tmpBuilder.obj() ;
            ele    = tmpObj.firstElement() ;
         }
         else if ( MTH_SRC_TYPE_ELEMENTS_SPLIT == _srcType )
         {
            SDB_ASSERT( _index < _mthContext->_elements.size(), "impossible" ) ;
            CHAR indexStr[ 32 ] = "" ;
            INT32 fieldIndex = _mthContext->_elements[_index] ;
            ossItoa( fieldIndex, indexStr, 31 ) ;
            ele = _arrayObj.getField( indexStr ) ;
         }
         else
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }

         rc = _replaceField( _src, _fieldName, ele, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "_replaceField failed:rc=%d", rc ) ;
         record = builder.obj() ;
      }

      _index++ ;
      _validNum-- ;

   done:
      return rc ;
   error:
      _validNum = 0 ;
      goto done ;
   }

   INT32 _mthRecordGenerator::getRecordNum()
   {
      return _validNum ;
   }
}

