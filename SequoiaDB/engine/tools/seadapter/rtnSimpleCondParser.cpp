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

   Source File Name = rtnSimpleCondParser.hpp

   Descriptive Name = Simple condition parser

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/18/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnSimpleCondParser.hpp"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace seadapter
{
   #define UTIL_OPERATOR_EYECATCHER          '$'
   #define FIELD_NAME_TEXT                   "$Text"
   // all elements start with $
   #define UTIL_ELEMENT_KEY_ALL_OP           0

   // all elements start without $
   #define UTIL_ELEMENT_KEY_ALL_NORMAL       1

   // mix $ and not $
   #define UTIL_ELEMENT_KEY_MIX              2

   _rtnCondNodeFactory::_rtnCondNodeFactory()
   {
   }

   _rtnCondNodeFactory::~_rtnCondNodeFactory()
   {
   }

   rtnCondNode* _rtnCondNodeFactory::createNode( rtnCondNodeAllocator *allocator,
                                                 RTN_COND_NODE_TYPE type)
   {
      rtnCondNode *node = NULL ;
      switch ( type )
      {
         case RTN_COND_NODE_LOGIC_AND:
            node = new (allocator)rtnCondLogicAndNode( allocator ) ;
            break ;
         case RTN_COND_NODE_LOGIC_OR:
            node = new (allocator)rtnCondLogicOrNode( allocator ) ;
            break ;
         case RTN_COND_NODE_LOGIC_NOT:
            node = new (allocator)rtnCondLogicNotNode( allocator ) ;
            break ;
         case RTN_COND_NODE_TEXT:
            node = new (allocator)rtnCondTextNode( allocator ) ;
            break ;
         case RTN_COND_NODE_NORMAL:
            node = new (allocator)rtnCondNormalNode( allocator ) ;
            break ;
         default:
            break ;
      }

      return node ;
   }

   void _rtnCondNodeFactory::releaseNode( rtnCondNode* node )
   {
      SAFE_OSS_DELETE( node ) ;
   }

   _rtnSimpleCondParseTree::_rtnSimpleCondParseTree()
   {
      _root = NULL ;
      _textNode = NULL ;
      _textNodeInNot = FALSE ;
      _textNodeInOr = FALSE ;
   }

   _rtnSimpleCondParseTree::~_rtnSimpleCondParseTree()
   {
      clear() ;
   }

   INT32 _rtnSimpleCondParseTree::parse( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;
      try
      {
         _condition = object.copy() ;
         _root = rtnGetCondNodeFactory()->createNode( &_allocator,
                                                      RTN_COND_NODE_LOGIC_AND ) ;
         if ( !_root )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                    RTN_COND_NODE_LOGIC_AND ) ;
            goto error ;
         }

         // Traverse all the elements in this bson.
         {
            BSONObjIterator itr( _condition ) ;
            while ( itr.more() )
            {
               BSONElement eleTmp = itr.next() ;
               rc = _parseElement( eleTmp, _root ) ;
               PD_RC_CHECK( rc, PDERROR, "Parse element[ %s ] failed[ %d ]",
                            eleTmp.toString().c_str(), rc ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   void _rtnSimpleCondParseTree::clear()
   {
      _releaseTree( _root ) ;
      _root = NULL ;
      _textNode = NULL ;
   }

   BOOLEAN _rtnSimpleCondParseTree::hasTextCond()
   {
      return ( NULL != _textNode ) ;
   }

   rtnCondNode* _rtnSimpleCondParseTree::getTextNode()
   {
      return _textNode ;
   }

   INT32 _rtnSimpleCondParseTree::updateNode( rtnCondNode *node,
                                              const BSONElement &newEle )
   {
      INT32 rc = SDB_OK ;
      rtnCondNode *parent = NULL ;

      rtnCondNormalNode *normalNode = (rtnCondNormalNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_NORMAL ) ;
      if ( !normalNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_NORMAL ) ;
         goto error ;
      }

      normalNode->init( newEle.fieldName(), newEle ) ;

      parent = node->getParent() ;
      parent->updateChild( node, normalNode, newEle ) ;
      _textNode = normalNode ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj _rtnSimpleCondParseTree::toBson()
   {
      if ( _root )
      {
         return _root->toBson() ;
      }
      else
      {
         BSONObjBuilder builder ;
         return builder.obj() ;
      }
   }

   INT32 _rtnSimpleCondParseTree::_parseElement( const BSONElement &ele,
                                                 rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      PD_LOG( PDDEBUG, "Element to parse: %s", ele.toString().c_str() ) ;
      switch( ele.type() )
      {
         case Object:
            rc = _parseObjectElement( ele, parent ) ;
            break ;
         case Array:
            rc = _praseArrayElement( ele, parent ) ;
            break ;
         default:
            rc = _parseNormalElement( ele, parent ) ;
            break ;
      }
      PD_RC_CHECK( rc, PDERROR, "Parse element[ %s ] failed[ %d ]",
                   ele.toString().c_str(), rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseObjectElement( const BSONElement &ele,
                                                       rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      INT32 keysFormat = 0 ;
      const CHAR *fieldName = ele.fieldName() ;

      if ( 0 == ossStrlen( fieldName ) )
      {
         rc = _parseOpText( ele, parent ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse text operation failed: %d", rc ) ;
         goto done ;
      }
      else if ( UTIL_OPERATOR_EYECATCHER == fieldName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Operator can not appear in the head: %s",
                 ele.toString().c_str() ) ;
         goto error ;
      }
      else
      {
         keysFormat = _getElementKeysFormat( ele ) ;
         if ( UTIL_ELEMENT_KEY_MIX == keysFormat )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "object's element do not allow exist mix op keys and "
               "normal keys:ele=%s,rc=%d", ele.toString().c_str(), rc ) ;
            goto error ;
         }
         else
         {
            rc = _parseNormalElement( ele, parent ) ;
            PD_RC_CHECK( rc, PDERROR, "Parse normal element failed[ %d ]",
                         rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_praseArrayElement( const BSONElement &ele,
                                                      rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      const CHAR *fieldName = ele.fieldName() ;

      // Array element should be in logical operation or a normal array.
      if ( UTIL_OPERATOR_EYECATCHER == fieldName[0] )
      {
         if ( 0 == ossStrcmp( RTN_OPERATOR_STR_AND, fieldName ) )
         {
            rc = _parseLogicAnd( ele, parent ) ;
         }
         else if ( 0 == ossStrcmp( RTN_OPERATOR_STR_OR, fieldName ) )
         {
            rc = _parseLogicOr( ele, parent ) ;
         }
         else if ( 0 == ossStrcmp( RTN_OPERATOR_STR_NOT, fieldName ) )
         {
            rc = _parseLogicNot( ele, parent ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unsupported logic operation: %s",
                    ele.toString().c_str() ) ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Parse logical[ %s ] element[ %s ] "
                      "failed[ %d ]", fieldName, ele.toString().c_str(), rc ) ;
      }
      else
      {
         // just normal array
         rc = _parseNormalElement( ele, parent ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse element[ %s ] failed[ %d ]" ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseNormalElement( const BSONElement &ele,
                                                       rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      const CHAR *fieldName = ele.fieldName() ;
      rtnCondNormalNode *normalNode = (rtnCondNormalNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_NORMAL ) ;
      if ( !normalNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_NORMAL ) ;
         goto error ;
      }

      normalNode->init( fieldName, ele ) ;
      rc = parent->addChild( normalNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseLogicAnd( const BSONElement &ele,
                                                  rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      // create a logical node and add to parent.
      // then parse the logical items.
      rtnCondLogicAndNode *logicAndNode = (rtnCondLogicAndNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_LOGIC_AND ) ;
      if ( !logicAndNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_LOGIC_AND ) ;
         goto error ;
      }

      logicAndNode->init( ele.fieldName() ) ;
      rc = parent->addChild( logicAndNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;

      rc = _parseLogicElements( ele, logicAndNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse logical element[ %s ] failed[ %d ]",
                   ele.toString().c_str(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseLogicOr( const BSONElement &ele,
                                                 rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;

      rtnCondLogicOrNode *logicOrNode = (rtnCondLogicOrNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_LOGIC_OR ) ;
      if ( !logicOrNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_LOGIC_OR ) ;
         goto error ;
      }

      logicOrNode->init( ele.fieldName() ) ;
      rc = parent->addChild( logicOrNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;

      rc = _parseLogicElements( ele, logicOrNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse logical element[ %s ] failed[ %d ]",
                   ele.toString().c_str(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseLogicNot( const BSONElement &ele,
                                                  rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;

      rtnCondLogicNotNode *logicNotNode = (rtnCondLogicNotNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_LOGIC_NOT ) ;
      if ( !logicNotNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_LOGIC_NOT ) ;
         goto error ;
      }

      logicNotNode->init( ele.fieldName() ) ;
      rc = parent->addChild( logicNotNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;

      rc = _parseLogicElements( ele, logicNotNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse logical element[ %s ] failed[ %d ]",
                   ele.toString().c_str(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseOpText( const BSONElement &ele,
                                                rtnCondNode *parent )
   {
      // The format of a text search condition is as follows:
      // { "" : { "Text" : { ... } } }
      INT32 rc = SDB_OK ;
      rtnCondTextNode *textNode = NULL ;
      const CHAR *fieldName = ele.fieldName() ;
      rtnCondNode *parentTmp = parent ;

      SDB_ASSERT( Object == ele.type(), "Element of text operation should be "
                  "object type" ) ;

      try
      {
         BSONObj innerObj = ele.embeddedObject() ;
         if ( 1 != innerObj.nFields() ||
              0 != ossStrcmp( FIELD_NAME_TEXT,
                              innerObj.firstElement().fieldName() ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid element in condition: %s",
                    ele.toString().c_str() ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _textNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR,
                 "Only one text search condition can be used in a query" ) ;
         goto error ;
      }

      textNode = (rtnCondTextNode *)
         rtnGetCondNodeFactory()->createNode( &_allocator,
                                              RTN_COND_NODE_TEXT ) ;
      if ( !textNode )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                 RTN_COND_NODE_TEXT ) ;
         goto error ;
      }

      textNode->init( fieldName, ele ) ;
      rc = parent->addChild( textNode ) ;
      PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;

      _textNode = textNode ;

      // Check if the text node is in a $not clause.
      while ( parentTmp )
      {
         if ( RTN_COND_NODE_LOGIC_NOT == parentTmp->getType() )
         {
            _textNodeInNot = TRUE ;
         }
         else if ( RTN_COND_NODE_LOGIC_OR == parentTmp->getType() )
         {
            _textNodeInOr = TRUE ;
         }

         if ( _textNodeInNot  && _textNodeInOr )
         {
            break ;
         }
         else
         {
            parentTmp = parentTmp->getParent() ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_parseLogicElements( const BSONElement &ele,
                                                       rtnCondNode *parent )
   {
      INT32 rc = SDB_OK ;
      BSONObjIterator itr ( ele.embeddedObject() ) ;
      while ( itr.more() )
      {
         BSONElement item = itr.next() ;

         if ( Object != item.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Iterm[ %s ] type is not Object",
                    item.toString().c_str() ) ;
            goto error ;
         }

         {
            rtnCondNode *tmpParent = parent ;
            if ( RTN_COND_NODE_LOGIC_OR == parent->getType() ||
                 RTN_COND_NODE_LOGIC_NOT == parent->getType() )
            {
               rtnCondLogicAndNode *andNode = (rtnCondLogicAndNode *)
                  rtnGetCondNodeFactory()->createNode( &_allocator,
                                                       RTN_COND_NODE_LOGIC_AND ) ;
               if ( !andNode )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Create condition node failed, type[ %d ]",
                          RTN_COND_NODE_LOGIC_AND ) ;
                  goto error ;
               }

               rc = parent->addChild( andNode ) ;
               PD_RC_CHECK( rc, PDERROR, "Add child node failed[ %d ]", rc ) ;
               tmpParent = andNode ;
            }

            BSONObjIterator iterObject( item.embeddedObject() ) ;
            while ( iterObject.more() )
            {
               BSONElement eleTemp = iterObject.next() ;
               rc = _parseElement( eleTemp, tmpParent ) ;
               PD_RC_CHECK( rc, PDERROR, "Parse element[ %s ] failed[ %d ]",
                            eleTemp.toString().c_str(), rc ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSimpleCondParseTree::_getElementKeysFormat( const BSONElement &ele )
   {
      const CHAR *eFieldName = NULL ;
      INT32 opCount = 0 ;
      INT32 normalCount = 0 ;
      BSONElement temEle ;
      INT32 eleKeyType = UTIL_ELEMENT_KEY_ALL_NORMAL ;

      if ( Object == ele.type() || Array == ele.type() )
      {
         BSONObjIterator j( ele.embeddedObject() ) ;
         while ( j.more () )
         {
            temEle     = j.next() ;
            eFieldName = temEle.fieldName() ;
            if ( UTIL_OPERATOR_EYECATCHER == eFieldName[0] )
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
            eleKeyType = UTIL_ELEMENT_KEY_MIX ;
         }
         else
         {
            eleKeyType = UTIL_ELEMENT_KEY_ALL_OP ;
         }
      }
      else
      {
         eleKeyType = UTIL_ELEMENT_KEY_ALL_NORMAL ;
      }

      return eleKeyType ;
   }

   void _rtnSimpleCondParseTree::_releaseTree( rtnCondNode *root )
   {
      if ( !root )
      {
         return ;
      }

      rtnCondNodeItr itr( root ) ;
      while ( itr.more() )
      {
         rtnCondNode *child = itr.next() ;
         _releaseTree( child ) ;
      }

      rtnGetCondNodeFactory()->releaseNode( root ) ;
   }

   rtnCondNodeFactory* rtnGetCondNodeFactory()
   {
      static rtnCondNodeFactory factory ;
      return &factory ;
   }
}
