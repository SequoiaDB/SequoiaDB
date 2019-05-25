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

   Source File Name = qgmMatcher.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMMATCHER_HPP_
#define QGMMATCHER_HPP_

#include "qgmDef.hpp"
#include <utilMap.hpp>

using namespace bson ;

namespace engine
{
   struct _qgmConditionNode ;

   struct _qgmMatcherDataNode
   {
      BSONObj        _obj ;
      BSONElement    _element ;

      _qgmMatcherDataNode( BSONObj obj = BSONObj() )
      {
         _obj = obj ;
         _element = _obj.firstElement() ;
      }
   } ;
   typedef _qgmMatcherDataNode qgmMatcherDataNode ;

   /*
      _qgmMatcher define
   */
   class _qgmMatcher : public SDBObject
   {
   typedef _utilMap< void*, qgmMatcherDataNode, 8 >      MAP_DATA_NODE ;
   typedef MAP_DATA_NODE::iterator                       MAP_DATA_NODE_IT ;

   public:
      _qgmMatcher( _qgmConditionNode *node ) ;
      virtual ~_qgmMatcher() ;

      void     resetDataNode() ;

   public:
      OSS_INLINE BOOLEAN ready(){return _ready ;}

      string toString() const ;

      INT32 match( const qgmFetchOut &fetch,
                   BOOLEAN &r ) ;

   private:
      INT32 _match( const _qgmConditionNode *node,
                    const qgmFetchOut &fetch,
                    BOOLEAN &r ) ;

   private:
      _qgmConditionNode *_condition ;
      BOOLEAN           _ready ;

      MAP_DATA_NODE     _mapDataNode ;

   } ;

   typedef class _qgmMatcher qgmMatcher ;
}

#endif

