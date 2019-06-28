/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "rtnTrace.hpp"
#include "rtnSimpleCondNode.hpp"

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
         if ( NULL != allocator )
         {
            p = allocator->allocate( size ) ;
         }

         if ( NULL == p )
         {
            p = SDB_OSS_MALLOC( size ) ;
         }
      }

      return p ;
   }

   void _rtnCondNode::operator delete( void *p )
   {
      SAFE_OSS_FREE( p ) ;
   }

   void _rtnCondNode::operator delete( void *p,
                                       rtnCondNodeAllocator *allocator )
   {
      if ( NULL != allocator && allocator->isAllocatedByme( p ) )
      {
      }
      else
      {
         SAFE_OSS_FREE( p ) ;
      }
   }

   void _rtnCondNode::init( const CHAR *fieldName )
   {
      _fieldName = fieldName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONDNODE_ADDCHILD, "_rtnCondNode::addChild" )
   INT32 _rtnCondNode::addChild( rtnCondNode *child )
   {
      PD_TRACE_ENTRY( SDB__RTNCONDNODE_ADDCHILD ) ;
      _children.push_back( child ) ;
      child->_idxInParent = _children.size() - 1 ;
      child->_parent = this ;

      PD_TRACE_EXIT( SDB__RTNCONDNODE_ADDCHILD ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONDNODE_UPDATECHILD, "_rtnCondNode::updateChild" )
   INT32 _rtnCondNode::updateChild( rtnCondNode *child,
                                     rtnCondNode *newChild,
                                     const BSONElement &element )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONDNODE_UPDATECHILD ) ;

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
      child->release() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCONDNODE_UPDATECHILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   _rtnCondNode* _rtnCondNode::getParent()
   {
      return _parent ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONDNODE_CLEAR, "_rtnCondNode::clear" )
   void _rtnCondNode::clear()
   {
      PD_TRACE_ENTRY( SDB__RTNCONDNODE_CLEAR ) ;
      _fieldName.clear() ;
      _parent = NULL ;
      _children.clear() ;
      _idxInParent = -1 ;
      PD_TRACE_EXIT( SDB__RTNCONDNODE_CLEAR ) ;
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

   void _rtnCondNormalNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondNormalNode() ;
      }
      else
      {
         delete this ;
      }
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

   void _rtnCondTextNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondTextNode() ;
      }
      else
      {
         delete this ;
      }
   }

   _rtnCondLogicNode::_rtnCondLogicNode( rtnCondNodeAllocator *allocator )
   : _rtnCondNode( allocator )
   {

   }

   _rtnCondLogicNode::~_rtnCondLogicNode()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONDLOGICNODE_TOBSON, "_rtnCondLogicNode::toBson" )
   BSONObj _rtnCondLogicNode::toBson()
   {
      PD_TRACE_ENTRY( SDB__RTNCONDLOGICNODE_TOBSON ) ;
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( getOperatorStr() ) ) ;
      UTIL_CONDNODE_VEC::iterator itr = _children.begin() ;
      while ( itr != _children.end() )
      {
         sub << (*itr)->toBson() ;
         itr++ ;
      }

      sub.doneFast() ;

      PD_TRACE_EXIT( SDB__RTNCONDLOGICNODE_TOBSON ) ;
      return builder.obj() ;
   }

   void _rtnCondLogicNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondLogicNode() ;
      }
      else
      {
         delete this ;
      }
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

   void _rtnCondLogicAndNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondLogicAndNode() ;
      }
      else
      {
         delete this ;
      }
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

   void _rtnCondLogicOrNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondLogicOrNode() ;
      }
      else
      {
         delete this ;
      }
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

   void _rtnCondLogicNotNode::release()
   {
      if ( _allocator && _allocator->isAllocatedByme( this ) )
      {
         this->~_rtnCondLogicNotNode() ;
      }
      else
      {
         delete this ;
      }
   }
}

