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

   Source File Name = authTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/07/23    ZHY Initial Draft
   Last Changed =

*******************************************************************************/

#include "ossTypes.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

#include "utilDAG.hpp"
#include "auth.hpp"

namespace engine
{

   TEST( auth_resource, base_from1 )
   {
      ASSERT_EQ(
         RESOURCE_TYPE_CLUSTER,
         authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) )->getType() );
      ASSERT_EQ(
         RESOURCE_TYPE_ANY,
         authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) )->getType() );
      ASSERT_EQ( RESOURCE_TYPE_NON_SYSTEM,
                 authResource::fromBson(
                    BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) )
                    ->getType() );
      ASSERT_EQ( RESOURCE_TYPE_COLLECTION_SPACE,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                               << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) )
                    ->getType() );
      ASSERT_EQ( RESOURCE_TYPE_COLLECTION_NAME,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                               << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) )
                    ->getType() );
      ASSERT_EQ( RESOURCE_TYPE_EXACT_COLLECTION,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                               << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) )
                    ->getType() );
   }

   TEST( auth_resource, base_from2 )
   {
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( "cluster" << true ) )->getType() );
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( "anyResource" << true ) )->getType() );
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( "Cs"
                                               << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) )
                    ->getType() );
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME << ""
                                                                           << "Cl"
                                                                           << "" ) )
                    ->getType() );
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" ) )->getType() );
      ASSERT_EQ( RESOURCE_TYPE__INVALID,
                 authResource::fromBson( BSON( AUTH_RESOURCE_CL_FIELD_NAME << "" ) )->getType() );
   }

   TEST( auth_resource, base_cmp )
   {
      std::vector< boost::shared_ptr< authResource > > v;
      v.push_back( authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) );
      v.push_back( authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar2" ) ) );
      v.push_back( authResource::fromBson(
         BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo2" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) );

      if ( v.size() > 1 )
      {
         for ( std::vector< boost::shared_ptr< authResource > >::iterator it1 = v.begin();
               it1 != v.end(); ++it1 )
         {
            for ( std::vector< boost::shared_ptr< authResource > >::iterator it2 = it1 + 1;
                  it2 != v.end(); ++it2 )
            {
               ASSERT_EQ( FALSE, ( **it1 ) == ( **it2 ) );
            }
         }
      }
   }

   TEST( auth_privilege, base_resource_mask )
   {
      ACTION_SET_NUMBER_ARRAY allActions( 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL );
      ASSERT_EQ(
         *authGetActionSetMask( RESOURCE_TYPE_CLUSTER ),
         *authPrivilege( authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ),
                         boost::make_shared< authActionSet >( allActions ) )
             .getActionSet() );

      ASSERT_EQ(
         *authGetActionSetMask( RESOURCE_TYPE_ANY ),
         *authPrivilege( authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ),
                         boost::make_shared< authActionSet >( allActions ) )
             .getActionSet() );

      ASSERT_EQ( *authGetActionSetMask( RESOURCE_TYPE_NON_SYSTEM ),
                 *authPrivilege(
                     authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                                   << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
                     boost::make_shared< authActionSet >( allActions ) )
                     .getActionSet() );

      ASSERT_EQ( *authGetActionSetMask( RESOURCE_TYPE_COLLECTION_SPACE ),
                 *authPrivilege( authResource::fromBson(
                                    BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
                                 boost::make_shared< authActionSet >( allActions ) )
                     .getActionSet() );

      ASSERT_EQ( *authGetActionSetMask( RESOURCE_TYPE_COLLECTION_NAME ),
                 *authPrivilege( authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                                               << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                               << "bar" ) ),
                                 boost::make_shared< authActionSet >( allActions ) )
                     .getActionSet() );

      ASSERT_EQ( *authGetActionSetMask( RESOURCE_TYPE_EXACT_COLLECTION ),
                 *authPrivilege( authResource::fromBson(
                                    BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ),
                                 boost::make_shared< authActionSet >( allActions ) )
                     .getActionSet() );

      ASSERT_EQ( *authGetActionSetMask( RESOURCE_TYPE__INVALID ),
                 *authPrivilege( authResource::fromBson( BSON( "NotExistField"
                                                               << "foo" ) ),
                                 boost::make_shared< authActionSet >( allActions ) )
                     .getActionSet() );
   }

   TEST( util_dag, base_add )
   {
      using namespace boost;
      utilDAG< ossPoolString > dag;
      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "a" ),
                                     boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "a" ) ) );
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( TRUE, dag.addEdge( boost::make_shared< ossPoolString >( "a" ),
                                    boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "a" ),
                                     boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( FALSE, dag.addNode( boost::make_shared< ossPoolString >( "b" ) ) );

      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "c" ) ) );
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "d" ) ) );
      ASSERT_EQ( TRUE, dag.addEdge( boost::make_shared< ossPoolString >( "a" ),
                                    boost::make_shared< ossPoolString >( "c" ) ) );
      ASSERT_EQ( TRUE, dag.addEdge( boost::make_shared< ossPoolString >( "b" ),
                                    boost::make_shared< ossPoolString >( "c" ) ) );
      ASSERT_EQ( TRUE, dag.addEdge( boost::make_shared< ossPoolString >( "c" ),
                                    boost::make_shared< ossPoolString >( "d" ) ) );

      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "d" ),
                                     boost::make_shared< ossPoolString >( "a" ) ) );
      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "d" ),
                                     boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "d" ),
                                     boost::make_shared< ossPoolString >( "c" ) ) );

      ASSERT_EQ( TRUE, dag.delEdge( boost::make_shared< ossPoolString >( "a" ),
                                    boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( TRUE, dag.delNode( boost::make_shared< ossPoolString >( "c" ) ) );

      ASSERT_EQ( TRUE, dag.addEdge( boost::make_shared< ossPoolString >( "a" ),
                                    boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( FALSE, dag.addEdge( boost::make_shared< ossPoolString >( "b" ),
                                     boost::make_shared< ossPoolString >( "a" ) ) );
   }

   TEST( util_dag, base_add_edges )
   {
      using namespace boost;
      utilDAG< ossPoolString > dag;
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "a" ) ) );
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "b" ) ) );
      ASSERT_EQ( TRUE, dag.addNode( boost::make_shared< ossPoolString >( "c" ) ) );
      ossPoolVector< boost::shared_ptr< ossPoolString > > dests;
      dests.push_back( boost::make_shared< ossPoolString >( "b" ) );
      dests.push_back( boost::make_shared< ossPoolString >( "c" ) );
      dests.push_back( boost::make_shared< ossPoolString >( "d" ) );
      ASSERT_EQ( UTIL_DAG_EDGES_RET_DEST_NOT_FOUND,
                 dag.addEdges( boost::make_shared< ossPoolString >( "a" ), dests ).first );
      ossPoolVector< boost::shared_ptr< ossPoolString > > v;
      dag.dfs( boost::make_shared< ossPoolString >( "a" ), TRUE, v );
      ASSERT_EQ( 1u, v.size() );
      ASSERT_EQ( "a", *v[ 0 ] );
   }

   TEST( util_dag, base_topo_sort )
   {
      utilDAG< ossPoolString > dag;
      boost::shared_ptr< ossPoolString > nodeA = boost::make_shared< ossPoolString >( "a" );
      boost::shared_ptr< ossPoolString > nodeB = boost::make_shared< ossPoolString >( "b" );
      boost::shared_ptr< ossPoolString > nodeC = boost::make_shared< ossPoolString >( "c" );
      boost::shared_ptr< ossPoolString > nodeD = boost::make_shared< ossPoolString >( "d" );
      boost::shared_ptr< ossPoolString > nodeE = boost::make_shared< ossPoolString >( "e" );

      ASSERT_EQ( TRUE, dag.addNode( nodeA ) );
      ASSERT_EQ( TRUE, dag.addNode( nodeB ) );
      ASSERT_EQ( TRUE, dag.addNode( nodeC ) );
      ASSERT_EQ( TRUE, dag.addNode( nodeD ) );
      ASSERT_EQ( TRUE, dag.addNode( nodeE ) );

      ASSERT_EQ( TRUE, dag.addEdge( nodeA, nodeB ) );
      ASSERT_EQ( TRUE, dag.addEdge( nodeA, nodeC ) );
      ASSERT_EQ( TRUE, dag.addEdge( nodeB, nodeD ) );
      ASSERT_EQ( TRUE, dag.addEdge( nodeC, nodeD ) );
      ASSERT_EQ( TRUE, dag.addEdge( nodeD, nodeE ) );
      ASSERT_EQ( TRUE, dag.addEdge( nodeC, nodeE ) );

      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         expected.push_back( nodeA );
         expected.push_back( nodeB );
         expected.push_back( nodeC );
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         ASSERT_EQ( TRUE, dag.topologicalSort( v ) );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }

      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeA, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeA );
         }
         expected.push_back( nodeB );
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         expected.push_back( nodeC );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeA, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeA );
         }
         expected.push_back( nodeB );
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         expected.push_back( nodeC );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeB, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeB );
         }
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeC, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeC );
         }
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeD, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeD );
         }
         expected.push_back( nodeE );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      for ( INT32 includeStart = 0; includeStart < 2; ++includeStart )
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( nodeE, includeStart, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         if ( includeStart )
         {
            expected.push_back( nodeE );
         }
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
      {
         ossPoolVector< boost::shared_ptr< ossPoolString > > start;
         start.push_back( nodeB );
         start.push_back( nodeC );
         ossPoolVector< boost::shared_ptr< ossPoolString > > v;
         ASSERT_EQ( TRUE, dag.dfs( start, v ) );
         ossPoolVector< boost::shared_ptr< ossPoolString > > expected;
         expected.push_back( nodeB );
         expected.push_back( nodeD );
         expected.push_back( nodeE );
         expected.push_back( nodeC );
         for ( UINT32 i = 0; i < v.size(); ++i )
         {
            EXPECT_EQ( *expected.at( i ), *v.at( i ) );
         }
      }
   }

   TEST( auth_acl, base_order )
   {
      const ossPoolVector< engine::_authActionSet > *null = 0;
      ossPoolMap< boost::shared_ptr< authResource >, const ossPoolVector< authActionSet > *,
                  SHARED_TYPE_LESS< authResource > >
         data;
      std::vector< boost::shared_ptr< authResource > > expectedOrder;
      {
         data.insert( make_pair(
            authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ), null ) );
         data.insert( make_pair(
            authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ), null ) );

         data.insert( make_pair(
            authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ),
            null ) );
         data.insert( make_pair(
            authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "fop" << AUTH_RESOURCE_CL_FIELD_NAME << "bas" ) ),
            null ) );

         data.insert( make_pair(
            authResource::fromBson(
               BSON( AUTH_RESOURCE_CS_FIELD_NAME << "fop" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
            null ) );
         data.insert( make_pair(
            authResource::fromBson(
               BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
            null ) );

         data.insert( make_pair(
            authResource::fromBson(
               BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bas" ) ),
            null ) );
         data.insert( make_pair(
            authResource::fromBson(
               BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ),
            null ) );

         data.insert(
            make_pair( authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                                     << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
                       null ) );
      }
      {
         expectedOrder.push_back(
            authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) );

         expectedOrder.push_back( authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) );
         expectedOrder.push_back( authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bas" ) ) );

         expectedOrder.push_back( authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) );
         expectedOrder.push_back( authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "fop" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) );

         expectedOrder.push_back( authResource::fromBson( BSON(
            AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) );
         expectedOrder.push_back( authResource::fromBson( BSON(
            AUTH_RESOURCE_CS_FIELD_NAME << "fop" << AUTH_RESOURCE_CL_FIELD_NAME << "bas" ) ) );

         expectedOrder.push_back( authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) );

         expectedOrder.push_back(
            authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) );
      }

      ASSERT_EQ( data.size(), expectedOrder.size() );
      UINT32 index = 0;
      for ( ossPoolMap< boost::shared_ptr< authResource >, const ossPoolVector< authActionSet > *,
                        SHARED_TYPE_LESS< authResource > >::iterator it = data.begin();
            it != data.end(); ++it )
      {

         EXPECT_EQ( TRUE, *it->first == *expectedOrder.at( index ) );
         ++index;
      }
   }

   TEST( auth_acl, base_add_check )
   {
      authAccessControlList acl;
      {
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_find );
         actions.push_back( ACTION_TYPE_getDetail );
         actions.push_back( ACTION_TYPE_listCollections );
         boost::shared_ptr< authActionSet > actionSet =
            boost::make_shared< authActionSet >( actions );
         authPrivilege p(
            authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ),
            actionSet );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( p ) );
      }
      {
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_insert );
         actions.push_back( ACTION_TYPE_update );
         actions.push_back( ACTION_TYPE_listCollections );
         authPrivilege p(
            authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                          << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ),
            boost::make_shared< authActionSet >( actions ) );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( p ) );
      }
      {
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_analyze );
         authPrivilege p(
            authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ),
            boost::make_shared< authActionSet >( actions ) );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( p ) );
      }
      {
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_find );
         actions.push_back( ACTION_TYPE_getDetail );
         actions.push_back( ACTION_TYPE_insert );
         actions.push_back( ACTION_TYPE_update );
         ASSERT_EQ( TRUE, acl.isAuthorizedForPrivilege( authPrivilege(
                             authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                                           << "foo" << AUTH_RESOURCE_CL_FIELD_NAME
                                                           << "bar" ) ),
                             boost::make_shared< authActionSet >( actions ) ) ) );
      }
      {
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_analyze );
         ASSERT_EQ( TRUE,
                    acl.isAuthorizedForPrivilege( authPrivilege(
                       authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ),
                       boost::make_shared< authActionSet >( actions ) ) ) );
      }
   }

   TEST( auth_resource, base_included )
   {
      // check type EXAC_COLLECTION
      {
         boost::shared_ptr< authResource > sub = authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                   << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar2" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo2" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo2" << AUTH_RESOURCE_CL_FIELD_NAME << "bar2" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                   << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo2" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                               << "bar" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "bar2" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ(
            TRUE, sub->isIncluded( *authResource::fromBson( BSON(
                     AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::forExact( "SYSFOO.SYSBAR" ) ) );

         // included by CLUSTER
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
      // check type COLLECTION_NAME
      {
         boost::shared_ptr< authResource > sub = authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                               << "bar" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ(
            TRUE, sub->isIncluded( *authResource::fromBson( BSON(
                     AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::forCL( "SYSBAR" ) ) );
         // included by CLUSTER
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
      // check type COLLECTION_SPACE
      {
         boost::shared_ptr< authResource > sub = authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                   << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo2" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "bar" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ(
            TRUE, sub->isIncluded( *authResource::fromBson( BSON(
                     AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::forCS( "SYSFOO" ) ) );
         // included by CLUSTER
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
      // check type NON_SYSTEM
      {
         boost::shared_ptr< authResource > sub = authResource::fromBson(
            BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "bar" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ(
            TRUE, sub->isIncluded( *authResource::fromBson( BSON(
                     AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by CLUSTER
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
      // check type CLUSTER
      {
         boost::shared_ptr< authResource > sub =
            authResource::fromBson( BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "bar" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "" ) ) ) );
         // included by CLUSTER
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
      // check type ANY
      {
         boost::shared_ptr< authResource > sub =
            authResource::fromBson( BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) );
         // included by EXACT_COLLECTION
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "bar" ) ) ) );
         // included by COLLECTION_SPACE
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                    << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ) ) );
         // included by COLLECTION_NAME
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "bar" ) ) ) );
         // included by NON_SYSTEM
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CS_FIELD_NAME << "" << AUTH_RESOURCE_CL_FIELD_NAME
                                                                << "" ) ) ) );
         // included by CLUSTER
         ASSERT_EQ( FALSE, sub->isIncluded( *authResource::fromBson(
                              BSON( AUTH_RESOURCE_CLUSTER_FIELD_NAME << true ) ) ) );
         // included by ANY
         ASSERT_EQ( TRUE, sub->isIncluded( *authResource::fromBson(
                             BSON( AUTH_RESOURCE_ANY_FIELD_NAME << true ) ) ) );
      }
   }

   TEST( auth_acl, base_check_included )
   {
      {
         authAccessControlList acl;
         std::vector< ACTION_TYPE > actions;
         actions.push_back( ACTION_TYPE_find );
         actions.push_back( ACTION_TYPE_getDetail );
         actions.push_back( ACTION_TYPE_listCollections );
         boost::shared_ptr< authActionSet > actionSet =
            boost::make_shared< authActionSet >( actions );
         authPrivilege p(
            authResource::fromBson(
               BSON( AUTH_RESOURCE_CS_FIELD_NAME << "foo" << AUTH_RESOURCE_CL_FIELD_NAME << "" ) ),
            actionSet );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( p ) );

         ASSERT_EQ( TRUE, acl.isAuthorizedForPrivilege( authPrivilege(
                             authResource::fromBson( BSON( AUTH_RESOURCE_CS_FIELD_NAME
                                                           << "foo" << AUTH_RESOURCE_CL_FIELD_NAME
                                                           << "bar" ) ),
                             boost::make_shared< authActionSet >( actions ) ) ) );
      }
   }

   TEST( auth_acl, base_check_multi_resource )
   {
      {
         authAccessControlList acl;
         boost::shared_ptr< authActionSet > actions1 = boost::make_shared< authActionSet >();
         actions1->addAction( ACTION_TYPE_find );
         boost::shared_ptr< authActionSet > actions2 = boost::make_shared< authActionSet >();
         actions2->addAction( ACTION_TYPE_update );
         ASSERT_EQ( SDB_OK,
                    acl.addPrivilege( authPrivilege( authResource::forCS( "foo" ), actions1 ) ) );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( authPrivilege( authResource::forExact( "foo", "bar" ),
                                                             actions2 ) ) );
         authActionSet needActions;
         needActions.addAction( ACTION_TYPE_find );
         needActions.addAction( ACTION_TYPE_update );
         EXPECT_EQ( TRUE, acl.isAuthorizedForActionsOnResource(
                             *authResource::forExact( "foo", "bar" ), needActions ) );
      }

      {
         authAccessControlList acl;
         boost::shared_ptr< authActionSet > actions1 = boost::make_shared< authActionSet >();
         actions1->addAction( ACTION_TYPE_find );
         boost::shared_ptr< authActionSet > actions2 = boost::make_shared< authActionSet >();
         actions2->addAction( ACTION_TYPE_update );
         ASSERT_EQ( SDB_OK,
                    acl.addPrivilege( authPrivilege( authResource::forNonSystem(), actions1 ) ) );
         ASSERT_EQ( SDB_OK, acl.addPrivilege( authPrivilege( authResource::forExact( "foo", "bar" ),
                                                             actions2 ) ) );
         authActionSet needActions;
         needActions.addAction( ACTION_TYPE_find );
         needActions.addAction( ACTION_TYPE_update );
         EXPECT_EQ( TRUE, acl.isAuthorizedForActionsOnResource(
                             *authResource::forExact( "foo", "bar" ), needActions ) );
      }

      {
         authAccessControlList acl;
         boost::shared_ptr< authActionSet > actions1 = boost::make_shared< authActionSet >();
         actions1->addAction( ACTION_TYPE_find );
         boost::shared_ptr< authActionSet > actions2 = boost::make_shared< authActionSet >();
         actions2->addAction( ACTION_TYPE_update );
         ASSERT_EQ( SDB_OK,
                    acl.addPrivilege( authPrivilege( authResource::forNonSystem(), actions1 ) ) );
         ASSERT_EQ( SDB_OK,
                    acl.addPrivilege( authPrivilege( authResource::forCS( "foo" ), actions2 ) ) );
         authActionSet needActions;
         needActions.addAction( ACTION_TYPE_find );
         needActions.addAction( ACTION_TYPE_update );
         EXPECT_EQ( TRUE, acl.isAuthorizedForActionsOnResource(
                             *authResource::forExact( "foo", "bar" ), needActions ) );
      }
   }
} // namespace engine
