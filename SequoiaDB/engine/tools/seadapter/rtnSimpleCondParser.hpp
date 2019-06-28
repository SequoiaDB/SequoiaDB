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

#ifndef RTN_SIMPLECONDPARSER_HPP__
#define RTN_SIMPLECONDPARSER_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "rtnSimpleCondNode.hpp"

using namespace bson ;
using namespace engine ;

namespace seadapter
{
   class _rtnCondNodeFactory : public SDBObject
   {
      public:
         _rtnCondNodeFactory() ;
         ~_rtnCondNodeFactory() ;

      public:
         rtnCondNode* createNode( rtnCondNodeAllocator *allocator,
                                  RTN_COND_NODE_TYPE type ) ;
         void releaseNode( rtnCondNode* node ) ;
   } ;
   typedef _rtnCondNodeFactory rtnCondNodeFactory ;

   class _rtnSimpleCondParseTree : public SDBObject
   {
      typedef map<string, rtnCondNode*> TAG_NODE_MAP ;

      public:
         _rtnSimpleCondParseTree() ;
         ~_rtnSimpleCondParseTree() ;

         INT32 parse( const BSONObj &condition ) ;
         void clear() ;
         BOOLEAN hasTextCond() ;
         rtnCondNode* getTextNode() ;
         BOOLEAN textNodeInNot() const { return _textNodeInNot ; }
         INT32 updateNode( rtnCondNode *node, const BSONElement &newEle ) ;
         BSONObj toBson() ;

      private:
         INT32 _parseElement( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseObjectElement( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _praseArrayElement( const BSONElement &ele,
                                   rtnCondNode *parent ) ;
         INT32 _parseNormalElement( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _parseLogicAnd( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicOr( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicNot( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseOpText( const BSONElement &ele, rtnCondNode *parent ) ;
         INT32 _parseLogicElements( const BSONElement &ele,
                                    rtnCondNode *parent ) ;
         INT32 _getElementKeysFormat( const BSONElement &ele ) ;
         void _releaseTree( rtnCondNode *root ) ;

      private:
         BSONObj              _condition ;
         rtnCondNode          *_root ;
         rtnCondNodeAllocator _allocator ;
         rtnCondNode          *_textNode ;
         BOOLEAN              _textNodeInNot ;
   } ;
   typedef _rtnSimpleCondParseTree rtnSimpleCondParseTree ;

   rtnCondNodeFactory* rtnGetCondNodeFactory() ;
}

#endif /* RTN_SIMPLECONDPARSER_HPP__ */

