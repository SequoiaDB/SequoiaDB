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
#ifndef RTN_SIMPLECONDNODE_HPP__
#define RTN_SIMPLECONDNODE_HPP__

#include "utilAllocator.hpp"
#include "../bson/bson.h"

using namespace bson ;
using namespace engine ;

namespace seadapter
{
   #define RTN_OPERATOR_STR_AND          "$and"
   #define RTN_OPERATOR_STR_OR           "$or"
   #define RTN_OPERATOR_STR_NOT          "$not"
   #define RTN_OPERATOR_STR_TEXT         "$Text"
   #define RTN_OPERATOR_STR_NORMAL       ""

   class _rtnCondNode ;

   enum RTN_COND_NODE_TYPE
   {
      RTN_COND_NODE_LOGIC_AND      = 0,
      RTN_COND_NODE_LOGIC_OR,
      RTN_COND_NODE_LOGIC_NOT,
      RTN_COND_NODE_TEXT,
      RTN_COND_NODE_NORMAL,

      RTN_COND_NODE_END
   } ;

   #define RTN_CONDNODE_ALLOCATOR_SIZE      2048
   typedef _utilAllocator<RTN_CONDNODE_ALLOCATOR_SIZE> rtnCondNodeAllocator ;
   typedef vector<_rtnCondNode *> UTIL_CONDNODE_VEC ;

   class _rtnCondNode
   {
      friend class _rtnCondNodeItr ;
      public:
         _rtnCondNode( rtnCondNodeAllocator *allocator ) ;
         virtual ~_rtnCondNode() ;

      public:
         void* operator new( size_t size, rtnCondNodeAllocator *allocator ) ;
         void operator delete( void *p ) ;
         void operator delete( void *p, rtnCondNodeAllocator *allocator ) ;

         virtual void init( const CHAR *fieldName ) ;
         virtual INT32 addChild( _rtnCondNode *child ) ;
         virtual INT32 updateChild( _rtnCondNode *child,
                                    _rtnCondNode *newChild,
                                    const BSONElement &element ) ;
         _rtnCondNode* getParent() ;

         virtual void clear() ;
         virtual void release() = 0 ;
         virtual RTN_COND_NODE_TYPE getType() = 0 ;
         virtual const CHAR* getOperatorStr() = 0 ;
         virtual BSONObj toBson() = 0 ;

      protected:
         rtnCondNodeAllocator *_allocator ;
         string _fieldName ;
         _rtnCondNode *_parent ;
         UTIL_CONDNODE_VEC _children ;
         INT32 _idxInParent ;
   } ;
   typedef _rtnCondNode rtnCondNode ;

   class _rtnCondNodeItr : public SDBObject
   {
      public:
         _rtnCondNodeItr( rtnCondNode *node ) ;
         ~_rtnCondNodeItr() ;

      private:
         _rtnCondNodeItr() ;
         _rtnCondNodeItr( const _rtnCondNodeItr &right ) ;

      public:
         BOOLEAN more() ;
         rtnCondNode *next() ;

      private:
         rtnCondNode *_node ;
         UINT32 _index ;
   } ;
   typedef _rtnCondNodeItr rtnCondNodeItr ;

   class _rtnCondNormalNode : public rtnCondNode
   {
      public:
         _rtnCondNormalNode( rtnCondNodeAllocator *allocator ) ;
         virtual ~_rtnCondNormalNode() ;

      public:
         virtual void init( const CHAR *fieldName,
                            const BSONElement &element ) ;
         virtual RTN_COND_NODE_TYPE getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual BSONObj toBson() ;
         virtual void release() ;

         BSONElement getElement() const { return _element ; }
      private:
         BSONElement _element ;
   } ;
   typedef _rtnCondNormalNode rtnCondNormalNode ;

   class _rtnCondTextNode : public rtnCondNode
   {
      public:
         _rtnCondTextNode( rtnCondNodeAllocator *allocator ) ;
         ~_rtnCondTextNode() ;
      public:
         virtual void init( const CHAR *fieldName,
                            const BSONElement &element ) ;
         virtual RTN_COND_NODE_TYPE getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual BSONObj toBson() ;
         virtual void release() ;
         BSONElement getElement() const { return _element ; }
      private:
         BSONElement _element ;
   } ;
   typedef _rtnCondTextNode rtnCondTextNode ;

   class _rtnCondLogicNode : public rtnCondNode
   {
      public:
         _rtnCondLogicNode( rtnCondNodeAllocator *allocator ) ;
         ~_rtnCondLogicNode() ;
      public:
         virtual BSONObj toBson() ;
         virtual void release() ;
   } ;
   typedef _rtnCondLogicNode rtnCondLogicNode ;

   class _rtnCondLogicAndNode : public _rtnCondLogicNode
   {
      public:
         _rtnCondLogicAndNode( rtnCondNodeAllocator *allocator ) ;
         virtual ~_rtnCondLogicAndNode() ;

      public:
         virtual RTN_COND_NODE_TYPE getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual void release() ;
   } ;
   typedef _rtnCondLogicAndNode rtnCondLogicAndNode ;

   class _rtnCondLogicOrNode : public _rtnCondLogicNode
   {
      public:
         _rtnCondLogicOrNode( rtnCondNodeAllocator *allocator ) ;
         virtual ~_rtnCondLogicOrNode() ;

      public:
         virtual RTN_COND_NODE_TYPE getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual void release() ;
   } ;
   typedef _rtnCondLogicOrNode rtnCondLogicOrNode ;

   class _rtnCondLogicNotNode : public _rtnCondLogicNode
   {
      public:
         _rtnCondLogicNotNode( rtnCondNodeAllocator *allocator ) ;
         virtual ~_rtnCondLogicNotNode() ;

      public:
         virtual RTN_COND_NODE_TYPE getType() ;
         virtual const CHAR* getOperatorStr() ;
         virtual void release() ;
   } ;
   typedef _rtnCondLogicNotNode rtnCondLogicNotNode ;

}

#endif /* RTN_SIMPLECONDNODE_HPP__ */

