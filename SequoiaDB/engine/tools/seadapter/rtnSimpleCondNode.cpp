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

   Source File Name = rtnSimpleNode.hpp

   Descriptive Name = Simple condition node in a parse tree.

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

#include "pd.hpp"
#include "pdTrace.hpp"
#include "rtnSimpleCondNode.hpp"

// Memory for _rtnCondNode may be allocated in two ways:
// 1. By using malloc.
// 2. By using user specified allocator(instances of rtnCondNodeAllocator).
// The actions when releasing these two kinds of node are different.
// So a type flag is added at the head of the actual allocated memory.
#define RTN_MEM_TYPE_SIZE            sizeof(INT32)
#define RTN_MEM_BY_USER_ALLOCATOR    0
#define RTN_MEM_BY_DFT_ALLOCATOR     1

namespace seadapter
{
   _rtnCondNode::_rtnCondNode( rtnCondNodeAllocator *allocator )
   {
      _allocator = allocator ;
      _parent = NULL ;
      _idxInParent = -1 ;
   }

   _rtnCondNode::~_rtnCondNode()
   {
      clear() ;
   }

   void* _rtnCondNode::operator new( size_t size,
                                     rtnCondNodeAllocator *allocator )
   {
      void *p = NULL ;
      if ( size > 0 )
      {
         // In order to know if the memory is allocated by malloc() when
         // deleting the object, reserve space for a flag at the head of the
         // allocated space.
         size_t reserveSize = size + RTN_MEM_TYPE_SIZE ;
         if ( allocator )
         {
            p = allocator->allocate( reserveSize ) ;
         }

         if ( NULL == p )
         {
            p = SDB_OSS_MALLOC( reserveSize ) ;
            if ( NULL == p )
            {
               goto error ;
            }
            *(INT32 *)p = RTN_MEM_BY_DFT_ALLOCATOR ;
         }
         else
         {
            *(INT32 *)p = RTN_MEM_BY_USER_ALLOCATOR ;
         }
         // Seek address which can actually be used by the user.
         p = (CHAR *)p + RTN_MEM_TYPE_SIZE ;
      }

   done:
      return p ;
   error:
      goto done ;
   }

   void _rtnCondNode::operator delete( void *p )
   {
      if ( p )
      {
         void *beginAddr = (void *)( (CHAR *)p - RTN_MEM_TYPE_SIZE ) ;
         // Only release memory allocted by SDB_OSS_MALLOC().
         // Objects allocated by instances of _utilAllocator(allocator is not
         // NULL in new) will not be released seperately, as they are allocated
         // in a stack. They space is released when the allocator is destroyed.
         if ( RTN_MEM_BY_DFT_ALLOCATOR == *(INT32 *)beginAddr )
         {
            SDB_OSS_FREE( beginAddr ) ;
         }
      }
   }

   void _rtnCondNode::operator delete( void *p, rtnCondNodeAllocator *allocator )
   {
      _rtnCondNode::operator delete( p ) ;
   }

   void _rtnCondNode::init( const CHAR *fieldName )
   {
      _fieldName = fieldName ;
   }

   INT32 _rtnCondNode::addChild( rtnCondNode *child )
   {
      _children.push_back( child ) ;
      child->_idxInParent = _children.size() - 1 ;
      child->_parent = this ;

      return SDB_OK ;
   }

   INT32 _rtnCondNode::updateChild( rtnCondNode *child,
                                    rtnCondNode *newChild,
                                    const BSONElement &element )
   {
      INT32 rc = SDB_OK ;

      if ( !child || !newChild ||
           child->_idxInParent >= (INT32)_children.size() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The old or new child node is invalid" ) ;
         goto error ;
      }

      _children[ child->_idxInParent ] = newChild ;
      newChild->_idxInParent = child->_idxInParent ;
      newChild->_parent = this ;
      // Release the old child.
      SDB_OSS_DEL child ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnCondNode* _rtnCondNode::getParent()
   {
      return _parent ;
   }

   void _rtnCondNode::clear()
   {
      _fieldName.clear() ;
      _parent = NULL ;
      _children.clear() ;
      _idxInParent = -1 ;
   }

   _rtnCondNodeItr::_rtnCondNodeItr( rtnCondNode *node )
   {
      _node  = node ;
      _index = 0 ;
   }

   _rtnCondNodeItr::~_rtnCondNodeItr()
   {
      _node  = NULL ;
      _index = 0 ;
   }

   _rtnCondNodeItr::_rtnCondNodeItr()
   {
   }

   _rtnCondNodeItr::_rtnCondNodeItr( const _rtnCondNodeItr &right )
   {
   }

   BOOLEAN _rtnCondNodeItr::more()
   {
      return _index < _node->_children.size() ;
   }

   rtnCondNode* _rtnCondNodeItr::next()
   {
      UINT32 i = _index++ ;
      if ( i < _node->_children.size() )
      {
         return _node->_children[ i ] ;
      }

      return NULL ;
   }

   _rtnCondNormalNode::_rtnCondNormalNode( rtnCondNodeAllocator *allocator )
   : _rtnCondNode( allocator )
   {
   }

   _rtnCondNormalNode::~_rtnCondNormalNode()
   {
      clear() ;
   }

   void _rtnCondNormalNode::init( const CHAR *fieldName,
                                  const BSONElement &element )
   {
      _rtnCondNode::init( fieldName ) ;
      // For normal node, directory use the element.
      _element = element ;
   }

   RTN_COND_NODE_TYPE _rtnCondNormalNode::getType()
   {
      return RTN_COND_NODE_NORMAL ;
   }

   const CHAR* _rtnCondNormalNode::getOperatorStr()
   {
      return RTN_OPERATOR_STR_NORMAL ;
   }

   BSONObj _rtnCondNormalNode::toBson()
   {
      BSONObjBuilder builder ;
      builder.append( _element ) ;

      return builder.obj() ;
   }

   _rtnCondTextNode::_rtnCondTextNode( rtnCondNodeAllocator *allocator )
   : _rtnCondNode( allocator )
   {
   }

   _rtnCondTextNode::~_rtnCondTextNode()
   {
      clear() ;
   }

   void _rtnCondTextNode::init( const CHAR *fieldName,
                                const BSONElement &element )
   {
      _rtnCondNode::init( fieldName ) ;
      // For text node, directly use the element.
      _element = element ;
   }

   RTN_COND_NODE_TYPE _rtnCondTextNode::getType()
   {
      return RTN_COND_NODE_TEXT ;
   }

   const CHAR* _rtnCondTextNode::getOperatorStr()
   {
      return RTN_OPERATOR_STR_TEXT ;
   }

   BSONObj _rtnCondTextNode::toBson()
   {
      BSONObjBuilder builder ;
      builder.append( _element ) ;

      return builder.obj() ;
   }

   _rtnCondLogicNode::_rtnCondLogicNode( rtnCondNodeAllocator *allocator )
   : _rtnCondNode( allocator )
   {

   }

   _rtnCondLogicNode::~_rtnCondLogicNode()
   {
      clear() ;
   }

   BSONObj _rtnCondLogicNode::toBson()
   {
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( getOperatorStr() ) ) ;
      UTIL_CONDNODE_VEC::iterator itr = _children.begin() ;
      while ( itr != _children.end() )
      {
         sub << (*itr)->toBson() ;
         itr++ ;
      }

      sub.doneFast() ;

      return builder.obj() ;
   }

   _rtnCondLogicAndNode::_rtnCondLogicAndNode( rtnCondNodeAllocator *allocator )
   : _rtnCondLogicNode( allocator )
   {
   }

   _rtnCondLogicAndNode::~_rtnCondLogicAndNode()
   {
      clear() ;
   }

   RTN_COND_NODE_TYPE _rtnCondLogicAndNode::getType()
   {
      return RTN_COND_NODE_LOGIC_AND ;
   }

   const CHAR* _rtnCondLogicAndNode::getOperatorStr()
   {
      return RTN_OPERATOR_STR_AND ;
   }

   _rtnCondLogicOrNode::_rtnCondLogicOrNode( rtnCondNodeAllocator *allocator )
   : _rtnCondLogicNode( allocator )
   {
   }

   _rtnCondLogicOrNode::~_rtnCondLogicOrNode()
   {
      clear() ;
   }

   RTN_COND_NODE_TYPE _rtnCondLogicOrNode::getType()
   {
      return RTN_COND_NODE_LOGIC_OR ;
   }

   const CHAR* _rtnCondLogicOrNode::getOperatorStr()
   {
      return RTN_OPERATOR_STR_OR ;
   }

   _rtnCondLogicNotNode::_rtnCondLogicNotNode( rtnCondNodeAllocator *allocator )
   : _rtnCondLogicNode( allocator )
   {
   }

   _rtnCondLogicNotNode::~_rtnCondLogicNotNode()
   {
      clear() ;
   }

   RTN_COND_NODE_TYPE _rtnCondLogicNotNode::getType()
   {
      return RTN_COND_NODE_LOGIC_NOT ;
   }

   const CHAR* _rtnCondLogicNotNode::getOperatorStr()
   {
      return RTN_OPERATOR_STR_NOT ;
   }
}

