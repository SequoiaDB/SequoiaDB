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

   Source File Name = qgmConditionNodeHelper.cpp

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

#include "qgmConditionNodeHelper.hpp"
#include "qgmUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson ;

namespace engine
{
   #define QGM_CONDITION_BUILD_DFT_SIZE            ( 1024 )

   /*
      _qgmConditionNodeHelper implement
   */
   _qgmConditionNodeHelper::_qgmConditionNodeHelper( _qgmConditionNode *root )
   :_root( root )
   {
   }

   _qgmConditionNodeHelper::~_qgmConditionNodeHelper()
   {
      _root = NULL ;
   }

   string _qgmConditionNodeHelper::toJson() const
   {
      return toBson().toString() ;
   }

   string _qgmConditionNodeHelper::toString() const
   {
      return toJson() ;
   }

   BSONObj _qgmConditionNodeHelper::toBson( BOOLEAN keepAlias ) const
   {
      BSONObjBuilder bb( QGM_CONDITION_BUILD_DFT_SIZE ) ;
      if ( SDB_OK == _crtBson( _root, bb, keepAlias ) )
      {
         return bb.obj() ;
      }
      else
      {
         return BSONObj() ;
      }
   }

   void _qgmConditionNodeHelper::getAllAttr( qgmDbAttrPtrVec &fields )
   {
      _getAllAttr( _root, fields ) ;
   }

   void _qgmConditionNodeHelper::releaseNodes( qgmConditionNodePtrVec &nodes )
   {
      qgmConditionNodePtrVec::iterator itr = nodes.begin() ;
      for ( ; itr != nodes.end(); itr++ )
      {
         SAFE_OSS_DELETE( *itr ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMCONDITIONNODEHELPER_MERGE, "_qgmConditionNodeHelper::merge" )
   INT32 _qgmConditionNodeHelper::merge( _qgmConditionNode *node )
   {
      PD_TRACE_ENTRY( SDB__QGMCONDITIONNODEHELPER_MERGE ) ;
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != node, "node can't be NULL" ) ;
      if ( NULL == node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL != _root )
      {
         _qgmConditionNode *newRoot = NULL ;
         newRoot = SDB_OSS_NEW _qgmConditionNode( SQL_GRAMMAR::AND ) ;
         if ( NULL == newRoot )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         newRoot->left = _root ;
         newRoot->right = node ;
         _root = newRoot ;
      }
      else
      {
         _root = node ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMCONDITIONNODEHELPER_MERGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMCONDITIONNODEHELPER_SEPARATE, "_qgmConditionNodeHelper::separate" )
   INT32 _qgmConditionNodeHelper::separate( qgmConditionNodePtrVec &nodes )
   {
      PD_TRACE_ENTRY( SDB__QGMCONDITIONNODEHELPER_SEPARATE ) ;
      INT32 rc = SDB_OK ;

      if ( NULL == _root )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _separate( _root, nodes ) ;
      _root = NULL ;

   done:
      PD_TRACE_EXITRC( SDB__QGMCONDITIONNODEHELPER_SEPARATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _qgmConditionNodeHelper::merge( qgmConditionNodePtrVec &nodes )
   {
      INT32 rc = SDB_OK ;
      qgmConditionNodePtrVec::iterator itr = nodes.begin() ;
      for ( ; itr != nodes.end(); itr++ )
      {
         rc = merge( *itr ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMCONDITIONNODEHELPER_SEPARATE2, "_qgmConditionNodeHelper::_separate" )
   void _qgmConditionNodeHelper::_separate( _qgmConditionNode *predicate,
                                            qgmConditionNodePtrVec &nodes )
   {
      PD_TRACE_ENTRY( SDB__QGMCONDITIONNODEHELPER_SEPARATE2 ) ;
      SDB_ASSERT( NULL != predicate, "predicate can't be NULL" ) ;

      if ( SQL_GRAMMAR::AND == predicate->type )
      {
         _separate( predicate->left, nodes ) ;
         _separate( predicate->right, nodes ) ;

         /// release the node
         predicate->dettach() ;
         SAFE_OSS_DELETE( predicate ) ;
      }
      else if ( predicate )
      {
         nodes.push_back( predicate ) ;
      }

      PD_TRACE_EXIT( SDB__QGMCONDITIONNODEHELPER_SEPARATE2 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMCONDITIONNODEHELPER__CRTBSON, "_qgmConditionNodeHelper::_crtBson" )
   template< class Builder >
   INT32 _qgmConditionNodeHelper::_crtBson( const _qgmConditionNode *node,
                                            Builder &bb,
                                            BOOLEAN keepAlias )const
   {
      PD_TRACE_ENTRY( SDB__QGMCONDITIONNODEHELPER__CRTBSON ) ;
      INT32 rc = SDB_OK ;

      if ( NULL == node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// $and | $or
      if ( SQL_GRAMMAR::AND == node->type ||
           SQL_GRAMMAR::OR == node->type )
      {
         BSONArrayBuilder logicalBuilder(
            bb.subarrayStart( qgmGetNodeTypeStr( node->type ) ) ) ;

         if ( !node->left || !node->right )
         {
            PD_LOG( PDERROR, "Node[Type:%s]'s left node or right node is "
                    "NULL", qgmGetNodeTypeStr( node->type ) ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         /// left
         {
            BSONObjBuilder leftBuilder( logicalBuilder.subobjStart() ) ;
            rc = _crtBson( node->left, leftBuilder, keepAlias ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            leftBuilder.done() ;
         }
         /// right
         {
            BSONObjBuilder rightBuilder( logicalBuilder.subobjStart() ) ;
            rc = _crtBson( node->right, rightBuilder, keepAlias ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            rightBuilder.done() ;
         }
         logicalBuilder.done() ;
      }
      /// $not
      else if ( SQL_GRAMMAR::NOT == node->type )
      {
         if ( !node->left )
         {
            PD_LOG( PDERROR, "Node[Type:%s]'s left node is NULL", "$not" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            BSONArrayBuilder logicalBuilder( bb.subarrayStart( "$not" ) ) ;
            BSONObjBuilder leftBuilder( logicalBuilder.subobjStart() ) ;
            rc = _crtBson( node->left, leftBuilder, keepAlias ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            leftBuilder.done() ;
            logicalBuilder.done() ;
         }
      }
      /// is null ==> { $isnull:1 }
      else if ( ( SQL_GRAMMAR::IS == node->type ||
                  SQL_GRAMMAR::ISNOT == node->type ) &&
                node->left && SQL_GRAMMAR::DBATTR == node->left->type &&
                node->right &&
                SQL_GRAMMAR::NULLL == node->right->type )
      {
         BSONObjBuilder oprBuilder( bb.subobjStart(
               keepAlias ? node->left->value.toString().c_str() :
                           node->left->value.attr().toString().c_str()
                                   ) ) ;
         oprBuilder.append( "$isnull",
                            SQL_GRAMMAR::IS == node->type ? 1 : 0 ) ;
         oprBuilder.done() ;
      }
      /// like  ==> { $regex:"", $option:"" }
      else if ( SQL_GRAMMAR::LIKE == node->type &&
                node->left && SQL_GRAMMAR::DBATTR == node->left->type &&
                node->right && SQL_GRAMMAR::STR == node->right->type )
      {
         bb.appendRegex( ( keepAlias ? node->left->value.toString().c_str() :
                                       node->left->value.attr().toString().c_str() ),
                         node->right->value.toString(),
                         "s" ) ;
      }
      else if ( SQL_GRAMMAR::EG == node->type ||
                SQL_GRAMMAR::NE == node->type ||
                SQL_GRAMMAR::LT == node->type ||
                SQL_GRAMMAR::GT == node->type ||
                SQL_GRAMMAR::LTE == node->type ||
                SQL_GRAMMAR::GTE == node->type ||
                SQL_GRAMMAR::IS == node->type ||
                SQL_GRAMMAR::ISNOT == node->type ||
                SQL_GRAMMAR::INN == node->type )
      {
         if ( !node->left || SQL_GRAMMAR::DBATTR != node->left->type )
         {
            PD_LOG( PDERROR, "Node[Type:%s]'s left is NULL or not DBATTR",
                    qgmGetNodeTypeStr( node->type ) ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( !node->right )
         {
            PD_LOG( PDERROR, "Node[Type:%s]'s right is NULL",
                    qgmGetNodeTypeStr( node->type ) ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            BSONObjBuilder oprBuilder( bb.subobjStart(
                  keepAlias ? node->left->value.toString().c_str() :
                              node->left->value.attr().toString().c_str()
                                      ) )  ;
            rc = qgmBuildANodeItem( oprBuilder,
                                    qgmGetNodeTypeStr( node->type ) ,
                                    node->right, keepAlias ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Build node[Type:%s] item failed, rc: %d",
                       qgmGetNodeTypeStr( node->type ), rc ) ;
               goto error ;
            }
            oprBuilder.done() ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "invalid type: %d", node->type ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMCONDITIONNODEHELPER__CRTBSON, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMCONDITIONNODEHELPER__GETALLATTR, "_qgmConditionNodeHelper::_getAllAttr" )
   void _qgmConditionNodeHelper::_getAllAttr( _qgmConditionNode *node,
                                              qgmDbAttrPtrVec &fields )
   {
      PD_TRACE_ENTRY( SDB__QGMCONDITIONNODEHELPER__GETALLATTR ) ;

      if ( !node )
      {
         goto done ;
      }

      if ( SQL_GRAMMAR::DBATTR == node->type )
      {
         fields.push_back( &(node->value) ) ;
      }
      else
      {
         if ( node->left )
         {
            _getAllAttr( node->left, fields ) ;
         }
         if ( node->right )
         {
            _getAllAttr( node->right, fields ) ;
         }
      }

   done:
      PD_TRACE_EXIT( SDB__QGMCONDITIONNODEHELPER__GETALLATTR ) ;
      return ;
   }

}
