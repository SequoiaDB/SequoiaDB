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

   Source File Name = qgmConditionNodeHelper.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMCONDITIONNODEHELPER_HPP_
#define QGMCONDITIONNODEHELPER_HPP_

#include "qgmConditionNode.hpp"
#include <sstream>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _qgmConditionNodeHelper : public SDBObject
   {
   public:
      _qgmConditionNodeHelper( _qgmConditionNode *root ) ;
      virtual ~_qgmConditionNodeHelper() ;

      _qgmConditionNode* getRoot() { return _root ; }
      void setRoot( _qgmConditionNode *pRoot ) { _root = pRoot ; }

   public:
      static void releaseNodes( qgmConditionNodePtrVec &nodes ) ;

   public:
      string toJson() const ;
      string toString() const ;

      BSONObj toBson( BOOLEAN keepAlias = TRUE ) const ;

      void getAllAttr( qgmDbAttrPtrVec &fields ) ;

      INT32 separate( qgmConditionNodePtrVec &nodes ) ;

      INT32 merge( qgmConditionNodePtrVec &nodes ) ;

      INT32 merge( _qgmConditionNode *node ) ;

   private:
      void _getAllAttr( _qgmConditionNode *node,
                        qgmDbAttrPtrVec &fields ) ;

      void  _separate( _qgmConditionNode *predicate,
                       qgmConditionNodePtrVec &nodes ) ;

      template< class Builder >
      INT32 _crtBson( const _qgmConditionNode *node,
                      Builder &bb,
                      BOOLEAN keepAlias ) const ;

   private:
      _qgmConditionNode *_root ;
   } ;
   typedef class _qgmConditionNodeHelper qgmConditionNodeHelper ;
}

#endif // QGMCONDITIONNODEHELPER_HPP_

