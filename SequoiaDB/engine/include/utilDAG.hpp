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

   Source File Name = utilDAG.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for update
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_DAG_HPP__
#define UTIL_DAG_HPP__
#include "ossMemPool.hpp"
#include "ossTypes.hpp"
#include "utilSharedPtrHelper.hpp"
#include <boost/smart_ptr.hpp>
#include <queue>
#include <stack>

namespace engine
{
   /*
   Usage:
   {
      utilDAG< ossPoolString > dag;
      shared_ptr< ossPoolString > nodeA = make_shared< ossPoolString >( "a" );
      shared_ptr< ossPoolString > nodeB = make_shared< ossPoolString >( "b" );
      shared_ptr< ossPoolString > nodeC = make_shared< ossPoolString >( "c" );
      shared_ptr< ossPoolString > nodeD = make_shared< ossPoolString >( "d" );
      shared_ptr< ossPoolString > nodeE = make_shared< ossPoolString >( "e" );

      dag.addNode( nodeA )
      dag.addNode( nodeB )
      dag.addNode( nodeC )
      dag.addNode( nodeD )
      dag.addNode( nodeE )

      dag.addEdge( nodeA, nodeB )
      dag.addEdge( nodeA, nodeC )
      dag.addEdge( nodeB, nodeD )
      dag.addEdge( nodeC, nodeD )
      dag.addEdge( nodeC, nodeE )
      dag.addEdge( nodeD, nodeE )


      // graph of the above code:
      //   a
      //  / \
      // b   c
      // | / |
      // d   |
      //   \ |
      //     e


      assert( !dag.addEdge(nodeE, nodeA) ); // cycle detected

      // topological sort:
      dag.topologicalSort(v); // v = [a, b, c, d, e]

      // dfs:
      dag.dfs(nodeA, TRUE, v); // v = [a, b, d, e, c]
      dag.dfs(nodeA, FALSE, v); // v = [b, d, e, c]
      dag.dfs(nodeB, TRUE, v); // v = [b, d, e]
      dag.dfs(nodeB, FALSE, v); // v = [d, e]
      dag.dfs(nodeC, TRUE, v); // v = [c, d, e]
      dag.dfs(nodeC, FALSE, v); // v = [d, e]
      dag.dfs(nodeD, TRUE, v); // v = [d, e]
      dag.dfs(nodeD, FALSE, v); // v = [e]
      dag.dfs(nodeE, TRUE, v); // v = [e]

      // dfs with multiple start nodes:
      vector< shared_ptr< ossPoolString > > startNodes = { nodeB, nodeC };
      dag.dfs(startNodes, v); // v = [b, d, e, c]
   }
   */
   
   template < typename T > class utilDAG
   {
      typedef boost::shared_ptr< T > SHARED_TYPE;
      typedef ossPoolSet< SHARED_TYPE, SHARED_TYPE_LESS< T > > DATA_TYPE;
      typedef ossPoolMap< SHARED_TYPE, ossPoolVector< SHARED_TYPE >, SHARED_TYPE_LESS< T > >
         ADJ_LIST_TYPE;

   public:
      BOOLEAN hasNode( const SHARED_TYPE &node ) const
      {
         typename DATA_TYPE::iterator it = _data.find( node );
         if ( it == _data.end() )
         {
            return FALSE;
         }
         else
         {
            return TRUE;
         }
      }

      BOOLEAN addNode( const SHARED_TYPE &node )
      {
         typename DATA_TYPE::iterator it = _data.find( node );
         if ( it != _data.end() )
         {
            return FALSE;
         }
         return _data.insert( node ).second;
      }

      BOOLEAN addEdge( const SHARED_TYPE &source, const SHARED_TYPE &destination )
      {
         typename DATA_TYPE::iterator sourceIter = _data.find( source );
         typename DATA_TYPE::iterator destIter = _data.find( destination );

         if ( sourceIter == _data.end() || destIter == _data.end() )
         {
            return FALSE;
         }

         typename ADJ_LIST_TYPE::mapped_type &l = _adjList[ *sourceIter ];
         typename ADJ_LIST_TYPE::mapped_type::iterator it =
            std::find_if( l.begin(), l.end(), SHARED_TYPE_EQUAL< T >( destination ) );
         if ( it != l.end() )
         {
            return FALSE;
         }

         l.push_back( *destIter );

         if ( !_kahnTopoSort() )
         {
            l.pop_back();
            return FALSE;
         }
         return TRUE;
      }

#define UTIL_DAG_EDGES_RET_SUCCESS 0
#define UTIL_DAG_EDGES_RET_SRC_NOT_FOUND 1
#define UTIL_DAG_EDGES_RET_DEST_NOT_FOUND 2
#define UTIL_DAG_EDGES_RET_CYCLE_DETECTED 3
      // Return value:
      // 0: success
      // 1: source node not found
      // 2: destination node not found
      // 3: cycle detected
      std::pair< INT32, SHARED_TYPE > addEdges( const SHARED_TYPE &source,
                                                const ossPoolVector< SHARED_TYPE > &destinations )
      {
         if ( destinations.empty() )
         {
            return std::make_pair( UTIL_DAG_EDGES_RET_SUCCESS, SHARED_TYPE() );
         }
         typename DATA_TYPE::iterator sourceIter = _data.find( source );
         if ( sourceIter == _data.end() )
         {
            return std::make_pair( UTIL_DAG_EDGES_RET_SRC_NOT_FOUND, source );
         }

         UINT32 pushCount = 0;

         typename ADJ_LIST_TYPE::mapped_type &l = _adjList[ *sourceIter ];
         for ( typename ossPoolVector< SHARED_TYPE >::const_iterator it = destinations.begin();
               it != destinations.end(); ++it )
         {
            typename DATA_TYPE::iterator destIter = _data.find( *it );
            if ( destIter == _data.end() )
            {
               for ( UINT32 i = 0; i < pushCount; ++i )
               {
                  l.pop_back();
               }
               return std::make_pair( UTIL_DAG_EDGES_RET_DEST_NOT_FOUND, *it );
            }

            typename ADJ_LIST_TYPE::mapped_type::iterator it2 =
               std::find_if( l.begin(), l.end(), SHARED_TYPE_EQUAL< T >( *it ) );
            if ( it2 == l.end() )
            {
               l.push_back( *destIter );
               pushCount += 1;
            }
         }

         if ( !_kahnTopoSort() )
         {
            for ( UINT32 i = 0; i < pushCount; ++i )
            {
               l.pop_back();
            }
            return std::make_pair( UTIL_DAG_EDGES_RET_CYCLE_DETECTED, SHARED_TYPE() );
         }
         return std::make_pair( UTIL_DAG_EDGES_RET_SUCCESS, SHARED_TYPE() );
      }

      BOOLEAN delNode( const SHARED_TYPE &node )
      {
         typename DATA_TYPE::iterator it = _data.find( node );
         if ( it == _data.end() )
         {
            return FALSE;
         }

         _adjList.erase( node );

         for ( typename ADJ_LIST_TYPE::iterator it = _adjList.begin(); it != _adjList.end(); ++it )
         {
            it->second.erase( std::remove_if( it->second.begin(), it->second.end(),
                                              SHARED_TYPE_EQUAL< T >( node ) ),
                              it->second.end() );
         }

         _data.erase( it );
         return TRUE;
      }

      BOOLEAN delEdge( const SHARED_TYPE &source, const SHARED_TYPE &destination )
      {
         typename DATA_TYPE::iterator sourceIter = _data.find( source );
         typename DATA_TYPE::iterator destIter = _data.find( destination );

         if ( sourceIter == _data.end() || destIter == _data.end() )
         {
            return FALSE;
         }

         typename ADJ_LIST_TYPE::mapped_type &l = _adjList[ *sourceIter ];
         l.erase( std::remove_if( l.begin(), l.end(), SHARED_TYPE_EQUAL< T >( destination ) ),
                  l.end() );
         return TRUE;
      }

      BOOLEAN delEdges( const SHARED_TYPE &source,
                        const ossPoolVector< SHARED_TYPE > &destinations )
      {
         typename DATA_TYPE::iterator sourceIter = _data.find( source );
         if ( sourceIter == _data.end() )
         {
            return FALSE;
         }

         typename ADJ_LIST_TYPE::mapped_type &l = _adjList[ *sourceIter ];
         for ( typename ossPoolVector< SHARED_TYPE >::const_iterator it = destinations.begin();
               it != destinations.end(); ++it )
         {
            l.erase( std::remove_if( l.begin(), l.end(), SHARED_TYPE_EQUAL< T >( *it ) ), l.end() );
         }
         return TRUE;
      }

      // return FALSE if there is a cycle
      BOOLEAN dfs( const SHARED_TYPE &start,
                   BOOLEAN includeStart,
                   ossPoolVector< SHARED_TYPE > &v ) const
      {
         return _dfs( start, includeStart, v );
      }

      BOOLEAN dfs( const ossPoolVector< SHARED_TYPE > &start,
                   ossPoolVector< SHARED_TYPE > &v ) const
      {
         return _dfs( start, v );
      }

      // return FALSE if there is a cycle
      BOOLEAN topologicalSort( ossPoolVector< SHARED_TYPE > &v ) const
      {
         return _kahnTopoSort( &v );
      }

      std::string toString() const
      {
         std::stringstream ss;
         for ( typename ADJ_LIST_TYPE::const_iterator it = _adjList.begin(); it != _adjList.end();
               ++it )
         {
            ss << *( it->first ) << ": ";
            for ( typename ADJ_LIST_TYPE::mapped_type::iterator it2 = it->second.begin();
                  it2 != it->second.end(); ++it2 )
            {
               if ( it2 != it->second.begin() )
               {
                  ss << ", ";
               }
               ss << **it2;
            }
            ss << "\n";
         }

         return ss.str();
      }

      // Return value:
      // 0: success
      // 1: source node not found
      // 2: destination node not found
      // 3: cycle detected
      std::pair< INT32, SHARED_TYPE > replaceEdges(
         const SHARED_TYPE &source,
         const ossPoolVector< SHARED_TYPE > &destinations )
      {
         typename DATA_TYPE::iterator sourceIter = _data.find( source );
         if ( sourceIter == _data.end() )
         {
            return std::make_pair( UTIL_DAG_EDGES_RET_SRC_NOT_FOUND, source );
         }

         typename ADJ_LIST_TYPE::mapped_type &l = _adjList[ *sourceIter ];
         typename ADJ_LIST_TYPE::mapped_type backup;
         l.swap( backup );
         for ( typename ossPoolVector< SHARED_TYPE >::const_iterator it = destinations.begin();
               it != destinations.end(); ++it )
         {
            typename DATA_TYPE::iterator destIter = _data.find( *it );
            if ( destIter == _data.end() )
            {
               return std::make_pair( UTIL_DAG_EDGES_RET_DEST_NOT_FOUND, *it );
            }

            l.push_back( *destIter );
         }

         if ( !_kahnTopoSort() )
         {
            l.swap( backup );
            return std::make_pair( UTIL_DAG_EDGES_RET_CYCLE_DETECTED, SHARED_TYPE() );
         }
         return std::make_pair( UTIL_DAG_EDGES_RET_SUCCESS, SHARED_TYPE() );
      }

      void clear()
      {
         _data.clear();
         _adjList.clear();
      }

   private:
      // return FALSE if there is a cycle
      // make sure that start node exists
      BOOLEAN _dfs( const SHARED_TYPE &start,
                    BOOLEAN includeStart,
                    ossPoolVector< SHARED_TYPE > &v ) const
      {
         std::stack< SHARED_TYPE > s;
         std::set< SHARED_TYPE, SHARED_TYPE_LESS< T > > visited;
         std::set< SHARED_TYPE, SHARED_TYPE_LESS< T > > recursion;

         typename DATA_TYPE::iterator sourceIter = _data.find( start );
         if ( sourceIter == _data.end() )
         {
            SDB_ASSERT( FALSE, "Source node not found" );
         }
         s.push( *sourceIter );

         BOOLEAN onStart = TRUE;

         while ( !s.empty() )
         {
            const SHARED_TYPE node = s.top();
            if ( visited.find( node ) == visited.end() )
            {
               visited.insert( node );
               if ( onStart )
               {
                  if ( includeStart )
                  {
                     v.push_back( node );
                  }
                  onStart = FALSE;
               }
               else if ( !onStart )
               {
                  v.push_back( node );
               }

               recursion.insert( node );
               typename ADJ_LIST_TYPE::const_iterator it = _adjList.find( node );
               if ( it != _adjList.end() )
               {
                  const typename ADJ_LIST_TYPE::mapped_type &neighbors = it->second;
                  for ( typename ADJ_LIST_TYPE::mapped_type::const_reverse_iterator neighborIt =
                           neighbors.rbegin();
                        neighborIt != neighbors.rend(); ++neighborIt )
                  {
                     if ( recursion.find( *neighborIt ) != recursion.end() )
                     {
                        return FALSE;
                     }
                     if ( visited.find( *neighborIt ) == visited.end() )
                     {
                        s.push( *neighborIt );
                     }
                  }
               }
            }
            else
            {
               recursion.erase( node );
               s.pop();
            }
         }

         return TRUE;
      }

      BOOLEAN _dfs( const ossPoolVector< SHARED_TYPE > &start,
                    ossPoolVector< SHARED_TYPE > &v ) const
      {
         std::stack< SHARED_TYPE > s;
         std::set< SHARED_TYPE, SHARED_TYPE_LESS< T > > visited;
         std::set< SHARED_TYPE, SHARED_TYPE_LESS< T > > recursion;

         for ( typename ossPoolVector< SHARED_TYPE >::const_reverse_iterator it = start.rbegin();
               it != start.rend(); ++it )
         {
            typename DATA_TYPE::iterator sourceIter = _data.find( *it );
            if ( sourceIter == _data.end() )
            {
               SDB_ASSERT( FALSE, "Source node not found" );
            }
            s.push( *sourceIter );
         }

         while ( !s.empty() )
         {
            const SHARED_TYPE node = s.top();
            if ( visited.find( node ) == visited.end() )
            {
               visited.insert( node );
               v.push_back( node );
               recursion.insert( node );
               typename ADJ_LIST_TYPE::const_iterator it = _adjList.find( node );
               if ( it != _adjList.end() )
               {
                  const typename ADJ_LIST_TYPE::mapped_type &neighbors = it->second;
                  for ( typename ADJ_LIST_TYPE::mapped_type::const_reverse_iterator neighborIt =
                           neighbors.rbegin();
                        neighborIt != neighbors.rend(); ++neighborIt )
                  {
                     if ( recursion.find( *neighborIt ) != recursion.end() )
                     {
                        return FALSE;
                     }
                     if ( visited.find( *neighborIt ) == visited.end() )
                     {
                        s.push( *neighborIt );
                     }
                  }
               }
            }
            else
            {
               recursion.erase( node );
               s.pop();
            }
         }

         return TRUE;
      }

      // return FALSE if there is a cycle
      BOOLEAN _kahnTopoSort( ossPoolVector< SHARED_TYPE > *v = NULL ) const
      {
         ossPoolMap< SHARED_TYPE, INT32, SHARED_TYPE_LESS< T > > inDegree;

         for ( typename DATA_TYPE::iterator it = _data.begin(); it != _data.end(); ++it )
         {
            inDegree[ *it ] = 0;
         }

         for ( typename ADJ_LIST_TYPE::const_iterator it = _adjList.begin(); it != _adjList.end();
               ++it )
         {
            const typename ADJ_LIST_TYPE::mapped_type &neighbors = it->second;
            for ( typename ADJ_LIST_TYPE::mapped_type::const_iterator neighborIt =
                     neighbors.begin();
                  neighborIt != neighbors.end(); ++neighborIt )
            {
               inDegree[ *neighborIt ]++;
            }
         }

         std::queue< SHARED_TYPE > q;
         for ( typename ossPoolMap< SHARED_TYPE, INT32, SHARED_TYPE_LESS< T > >::iterator it =
                  inDegree.begin();
               it != inDegree.end(); ++it )
         {
            if ( it->second == 0 )
            {
               q.push( it->first );
            }
         }

         UINT32 count = 0; // Number of nodes visited

         while ( !q.empty() )
         {
            const SHARED_TYPE &node = q.front();
            q.pop();
            count++;
            if ( v )
            {
               v->push_back( node );
            }

            typename ADJ_LIST_TYPE::const_iterator it = _adjList.find( node );
            if ( it != _adjList.end() )
            {
               const typename ADJ_LIST_TYPE::mapped_type &neighbors = it->second;
               for ( typename ADJ_LIST_TYPE::mapped_type::const_iterator neighborIt =
                        neighbors.begin();
                     neighborIt != neighbors.end(); ++neighborIt )
               {
                  inDegree[ *neighborIt ]--;
                  if ( inDegree[ *neighborIt ] == 0 )
                  {
                     q.push( *neighborIt );
                  }
               }
            }
         }

         // If the count is not equal to the number of nodes, a cycle is present
         return count == _data.size();
      }

   private:
      DATA_TYPE _data;
      ADJ_LIST_TYPE _adjList;
   };
} // namespace engine

#endif