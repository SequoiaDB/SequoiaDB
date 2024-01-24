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

   Source File Name = qgmMatcher.cpp

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

#include "qgmMatcher.hpp"
#include "pd.hpp"
#include "qgmConditionNodeHelper.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#define PCRE_STATIC
#include "../pcre/pcrecpp.h"

using namespace bson ;
namespace engine
{
   _qgmMatcher::_qgmMatcher( _qgmConditionNode *node )
   :_condition( NULL ),
    _ready( FALSE )
   {
      if ( NULL != node )
      {
         _condition = node ;
         _ready = TRUE ;
      }
   }

   _qgmMatcher::~_qgmMatcher()
   {
      _condition = NULL ;
      _ready = FALSE ;
   }

   void _qgmMatcher::resetDataNode()
   {
      _mapDataNode.clear() ;
   }

   string _qgmMatcher::toString() const
   {
      qgmConditionNodeHelper tree( _condition ) ;
      return tree.toJson() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMMATCHER_MATCH, "_qgmMatcher::match" )
   INT32 _qgmMatcher::match( const qgmFetchOut &fetch, BOOLEAN &r )
   {
      PD_TRACE_ENTRY( SDB__QGMMATCHER_MATCH ) ;
      INT32 rc = SDB_OK ;

      if ( !ready() )
      {
         PD_LOG( PDERROR, "matcher is not ready yet" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _match( _condition, fetch, r ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMMATCHER_MATCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMMATCHER__MATCH, "_qgmMatcher::_match" )
   INT32 _qgmMatcher::_match( const _qgmConditionNode *node,
                              const qgmFetchOut &fetch,
                              BOOLEAN &r )
   {
      PD_TRACE_ENTRY( SDB__QGMMATCHER__MATCH ) ;
      SDB_ASSERT( NULL != node, "impossible" ) ;

      INT32 rc = SDB_OK ;
      BSONElement fromFetch ;
      BSONElement fromCondition ;
      MAP_DATA_NODE_IT it ;

      try
      {
         if ( SQL_GRAMMAR::EG == node->type ||
              SQL_GRAMMAR::NE == node->type ||
              SQL_GRAMMAR::GT == node->type ||
              SQL_GRAMMAR::LT == node->type ||
              SQL_GRAMMAR::GTE == node->type ||
              SQL_GRAMMAR::LTE == node->type ||
              SQL_GRAMMAR::IS == node->type ||
              SQL_GRAMMAR::ISNOT == node->type )
         {
            rc = fetch.element( node->left->value, fromFetch ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get element from fetchout, rc=%d",
                       rc ) ;
               goto error ;
            }

            if ( fromFetch.eoo() && SQL_GRAMMAR::NULLL == node->right->type )
            {
               if ( SQL_GRAMMAR::IS == node->type )
               {
                  r = TRUE ;
                  goto done ;
               }
               else if ( SQL_GRAMMAR::ISNOT == node->type )
               {
                  r = FALSE ;
                  goto done ;
               }
            }

            /// 1. find from data nodes map
            it = _mapDataNode.find( (void*)node ) ;
            if ( it != _mapDataNode.end() )
            {
               fromCondition = it->second._element ;
            }
            else if ( SQL_GRAMMAR::DBATTR == node->right->type )
            {
               // $field element, extract specified field from fetchout
               rc = fetch.element( node->right->value, fromCondition ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get $field element from "
                            "fetchout, rc: %d", rc ) ;
            }
            else
            {
               /// create and add to map
               BSONObjBuilder bb ;
               BSONObj obj ;

               rc = qgmBuildANodeItem( bb, "", node->right, TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Build the node item to BSONObj "
                          "failed, rc: %d", rc ) ;
                  goto error ;
               }
               obj = bb.obj() ;
               _mapDataNode[ (void*)node ] = qgmMatcherDataNode( obj ) ;
               fromCondition = obj.firstElement() ;
            }

            if ( fromCondition.eoo() )
            {
               SDB_ASSERT( FALSE, "impossible" ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            if ( SQL_GRAMMAR::EG == node->type ||
                 SQL_GRAMMAR::IS == node->type )
            {
               r = (0 == fromCondition.woCompare( fromFetch, FALSE )) ?
                   TRUE : FALSE ;
            }
            else if ( SQL_GRAMMAR::NE == node->type ||
                      SQL_GRAMMAR::ISNOT == node->type )
            {
               r = (0 == fromCondition.woCompare( fromFetch, FALSE )) ?
                   FALSE : TRUE ;
            }
            else if ( SQL_GRAMMAR::LT == node->type )
            {
               r = (0 < fromCondition.woCompare( fromFetch, FALSE )) ?
                   TRUE : FALSE ;
            }
            else if ( SQL_GRAMMAR::GT == node->type )
            {
               r = (0 > fromCondition.woCompare( fromFetch, FALSE  )) ?
                   TRUE : FALSE ;
            }
            else if ( SQL_GRAMMAR::GTE == node->type )
            {
               INT32 wo = fromCondition.woCompare( fromFetch, FALSE ) ;
               r = ( 0 > wo ) || ( 0 == wo )?
                   TRUE : FALSE ;
            }
            else if ( SQL_GRAMMAR::LTE == node->type )
            {
               INT32 wo = fromCondition.woCompare( fromFetch, FALSE ) ;
               r = ( 0 < wo ) || ( 0 == wo )?
                   TRUE : FALSE ;
            }
            else
            {
               r = FALSE ;
            }
         }
         else if ( SQL_GRAMMAR::LIKE == node->type )
         {
            rc = fetch.element( node->left->value, fromFetch ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get element from fetchout, rc=%d",
                       rc ) ;
               goto error ;
            }

            if ( String != fromFetch.type() )
            {
               r = FALSE ;
               goto done ;
            }

            /// 1. find from data nodes map
            it = _mapDataNode.find( (void*)node ) ;
            if ( it != _mapDataNode.end() )
            {
               fromCondition = it->second._element ;
            }
            else
            {
               /// create and add to map
               BSONObj obj = BSON( "" << node->right->value.toString() ) ;
               _mapDataNode[ (void*)node ] = qgmMatcherDataNode( obj ) ;
               fromCondition = obj.firstElement() ;
            }

            if ( String != fromCondition.type() )
            {
               SDB_ASSERT( FALSE, "impossible" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            else
            {
               pcrecpp::RE_Options reOptions ;
               reOptions.set_utf8( true ) ;
               reOptions.set_dotall( true ) ;
               pcrecpp::RE regexMatch( fromCondition.valuestr(),
                                       reOptions ) ;
               r = regexMatch.PartialMatch( fromFetch.valuestr() ) ;
            }
         }
         else if ( SQL_GRAMMAR::INN == node->type )
         {
            SDB_ASSERT( NULL != node->right->var, "impossible" ) ;
            SDB_ASSERT( Array == node->right->var->type(), "impossible" ) ;
            rc = fetch.element( node->left->value, fromFetch ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get element from fetchout, rc=%d",
                       rc ) ;
               goto error ;
            }

            if ( fromFetch.eoo() )
            {
               r = FALSE ;
               goto done ;
            }

            r = FALSE ;

            {
               BSONObjIterator itr( node->right->var->embeddedObject() ) ;
               while ( itr.more() )
               {
                  if ( 0 == itr.next().woCompare( fromFetch, FALSE ) )
                  {
                     r = TRUE ;
                     break ;
                  }
               }
            }
         }
         else if ( SQL_GRAMMAR::NOT == node->type )
         {
            BOOLEAN rleft = FALSE ;
            rc = _match( node->left, fetch, rleft ) ;
            if ( rc )
            {
               goto error ;
            }
            r = !rleft ;
         }
         else
         {
            SDB_ASSERT( SQL_GRAMMAR::AND == node->type ||
                        SQL_GRAMMAR::OR == node->type,
                        "impossible" ) ;
            BOOLEAN rleft = FALSE ;
            BOOLEAN rright = FALSE ;
            rc = _match( node->left, fetch, rleft ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            if ( !rleft && SQL_GRAMMAR::AND == node->type )
            {
               r = FALSE ;
               goto done ;
            }
            else if ( rleft && SQL_GRAMMAR::OR == node->type )
            {
               r = TRUE ;
               goto done ;
            }

            rc = _match( node->right, fetch, rright ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            r = rright ;
         }
      }
      catch ( std::exception & e)
      {
         PD_LOG( PDERROR, "unexpected err happened:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMMATCHER__MATCH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
