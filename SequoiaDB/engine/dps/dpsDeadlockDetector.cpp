/******************************************************************************


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

   Source File Name = dpsDeadLock.cpp

   Descriptive Name = Deadlock detector

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/19/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsDeadlockDetector.hpp"

namespace engine
{

/*
   class dpsTDGraph implementaion
*/

// add edge into dpsTDGraph
void dpsTDGraph::addEdge( dpsTDEdge edge )
{
   try
   {
      _edges.insert( edge ) ;
   }
   catch ( std::exception &e )
   {
      PD_LOG( PDERROR,
              "Exception captured:%s while adding an edge into a graph",
              e.what() ) ;
   }
}


void dpsTDGraph::addEdge( TD_EDGE_SET & edgeSet )
{
   TD_EDGE_SET_IT it = edgeSet.begin();
   while ( it != edgeSet.end() )
   {
      try
      {
         _edges.insert( *it );
      }
      catch ( std::exception &e )
      {  
         PD_LOG( PDERROR,
                 "Exception captured:%s while adding an edge into a graph",
                 e.what() ) ;
      }
      it++;
   }
}


// find adjacent vertext of u
// input :
//   u   -- vertext u
//   cv  -- prev adjacent vertex of u
// output :
//   v   -- next adjacvent vertex of u
BOOLEAN dpsTDGraph::_getAdjacentNode( const dpsTDVertex & u,
                                      const dpsTDVertex & cv,
                                      dpsTDVertex & v )
{
   BOOLEAN found = FALSE ;
   TD_EDGE_SET_IT it ;

   if ( DPS_INVALID_TRANS_ID == cv.nodeId )
   {
      it = _edges.upper_bound( dpsTDEdge( u.nodeId, cv.nodeId ) );
   }
   else
   {
      it = _edges.find( dpsTDEdge( u.nodeId, cv.nodeId ) ) ;
   }

   while ( it != _edges.end() )
   {
      dpsTDEdge e = *it;
      if ( ( e.waiter == u.nodeId ) && ( e.holder != cv.nodeId ) )
      {
         v.nodeId = e.holder ;
         found = _find( v ) ;
         break ;
      }
      it++;
   }
   return found ;
}


BOOLEAN dpsTDGraph::_find( dpsTDVertex & u )
{
   TD_VERTEX_SET_IT it = _vertices.find( u ) ;
   if ( it != _vertices.end() )
   {
      u = *it ;
      return TRUE ;
   }
   return FALSE;
}


void dpsTDGraph::_update( dpsTDVertex & u )
{
   TD_VERTEX_SET_IT it = _vertices.find( u ) ;
   if ( it != _vertices.end() )
   {
      try
      {
         it->color = u.color ;
         it->dfn = u.dfn ;
         it->low = u.low ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR,
                 "Exception captured:%s while updating a vertex attribute",
                 e.what() ) ;
      }
   }
}


/*
   Recursively DFS traverse all adjacent vertices of vertex u
   returns true if a circle is detected.

Algorithm:
 . mark current node color to gray, indicate this node is being processed.
 . DFS traverse all the adjacent nodes and if a node is gray then return
   true as a back edge is found 
 . If any adjacent vertex is white( i.e., not be processed ), then execute
   this function recursively for that node.
 . If no adjacent node is grey set or has not returned true then mark 
   the current node to black( has been processed ) and return false.
*/

BOOLEAN dpsTDGraph::_isCyclic( dpsTDVertex & u )
{
   dpsTDVertex cv, v ;

   // dye vertex u to GRAY, i.e., this vertex is being processed.
   u.color = GRAY ;
   _update( u ) ;

   // Iterate through all adjacent vertices
   while ( _getAdjacentNode( u, cv, v ) )
   {
      // v is adjacent vertex of u 

      // if v is GRAY, i.e., a back edge is found
      if ( GRAY == v.color )
      {
         _backEdge.waiter = u.nodeId;
         _backEdge.holder = v.nodeId ;
         return TRUE ;
      }

      // if v color is WHITE, i.e., not processed,
      // then check if there is a circle in subtree rooted with v 
      if ( ( WHITE == v.color ) && _isCyclic( v ) )
      {
         return TRUE;
      }

      cv = v  ;
   }
   // set u to BLACK, i.e., this vertex has been processed
   u.color = BLACK ;
   _update( u ) ;
   return FALSE ;
}


// detect whether there is a cycle in graph
// return: true, if a cycle in graph is found 
// output: back edge ( an edge in the cycle ) when a circle is found
BOOLEAN dpsTDGraph::detectCycle( dpsTDEdge & backEdge )
{
   BOOLEAN found = FALSE ;
   dpsTDEdge edge ; 
   dpsTDVertex u ;

   if ( _edges.empty() )
   {
      return found;
   }

   _backEdge.reset() ;
   _vertices.clear() ;
   for ( TD_EDGE_SET_IT it = _edges.begin(); it != _edges.end(); it++ )
   {
      try
      {
         edge = *it; 
         u.resetAttr() ;
         u.nodeId = edge.waiter ;
         _vertices.insert( u ) ;

         u.nodeId = edge.holder ; 
         _vertices.insert( u ) ;
      }
      catch ( std::exception &e )
      {  
         PD_LOG( PDERROR,
                 "Exception captured:%s while adding a vertex into vertex set",
                 e.what() ) ;
      }
   }

   for ( TD_VERTEX_SET_IT it = _vertices.begin(); it != _vertices.end(); it++ )
   {
      u = *it ;
      if ( ( WHITE == u.color ) && _isCyclic( u ) )
      {
         found = TRUE;
         backEdge.waiter = _backEdge.waiter;
         backEdge.holder = _backEdge.holder;
         break ;
      }
   }

   return found;
}


void dpsTDGraph::_clearStack()
{
   while( !_stack.empty() )
   {
      _stack.pop();
   }
}


void dpsTDGraph::_findSCC( dpsTDVertex & u, BOOLEAN ignoreSingleton )
{
   dpsTDVertex cv, v ;

   // initialize dfn value and and low-link value
   _depth++ ;
   u.dfn   = _depth;
   u.low   = _depth;
   u.color = GRAY ; // mark this node is being processed, i.e., in stack
   _update( u );

   try
   {
     _stack.push( u ) ;
   }
   catch ( std::exception &e )
   {  
      PD_LOG( PDERROR,
              "Exception captured:%s while pushing a vertex on stack",
              e.what() ) ;
   }

   while ( _getAdjacentNode( u, cv, v ) )
   {
      // v is adjacent vertex of u 

      // If v is not visited yet, then recursively call this function for it
      if ( -1 == v.dfn )
      {
         _findSCC( v, ignoreSingleton ) ;
         u.low = OSS_MIN( u.low, v.low );
         _update( u );
      }
      else if ( GRAY == v.color )
      {
         // update low-link value of 'u' only of 'v' is still
         // in stack ( i.e. it's a back edge ).
         u.low = OSS_MIN( u.low, v.dfn ) ;
         _update( u );
      }

      cv = v ;
   }
   // found head node of substrees which forms a strongly connected component. 
   // pop everything up to u and make a SCC from it 
   if ( u.low == u.dfn )
   {
      dpsTDVertex w ;
      DPS_DEADLOCK_TX_SET * pSet = new DPS_DEADLOCK_TX_SET() ;
      while ( _stack.top() != u )
      {
         w = _stack.top();
         if ( pSet )
         {
            // save node into DPS_DEADLOCK_TX_SET, each contains a single SCC
            pSet->insert( dpsDeadlockTx( w.nodeId, 0, 0 ) ) ;
         }
         w.color = BLACK ;
         _update( w );
         _stack.pop();
      }
      w = _stack.top();

      if ( pSet )
      {
         pSet->insert( dpsDeadlockTx( w.nodeId, 0, 0 ) );
         if ( _pSetList )
         {
            try
            {
               // save this SCC into a list of SCC set
               if ( pSet->size() > 1 )
               {
                  _pSetList->push_back( pSet ) ;
               }
               else if ( ( 1 == pSet->size() ) && ( ! ignoreSingleton ) )
               {
                  _pSetList->push_back( pSet ) ;
               }
               else 
               {
                  delete pSet ;
                  pSet = NULL ;
               }
            }
            catch ( std::exception &e )
            {  
               PD_LOG( PDERROR,
                       "Exception captured:%s while saving a SCC into a list",
                       e.what() ) ;
            }
         }
         else 
         {
            delete pSet ;
            pSet = NULL ;
         }
      }

      w.color = BLACK ;
      _update( w );
      _stack.pop();
   }
}


// find dpsDeadlockTx u in DPS_DEADLOCK_TX_SET
BOOLEAN dpsTDFindTxNode( dpsDeadlockTx & u, DPS_DEADLOCK_TX_SET * pSet )
{
   if ( pSet )
   {
      DPS_DEADLOCK_TX_SET_IT it = pSet->find( u ) ;
      if ( it != pSet->end() )
      {
         u = *it ;
         return TRUE ;
      }
   }
   return FALSE;
}


// update dpsDeadlockTx atrribute of dpsDeadlockTx in DPS_DEADLOCK_TX_SET
void dpsTDUpdateTxNode( dpsDeadlockTx & u, DPS_DEADLOCK_TX_SET * pSet )
{
   if ( pSet )
   {
      DPS_DEADLOCK_TX_SET_IT it = pSet->find( u ) ;
      if ( it != pSet->end() )
      {
         try
         {
            it->degree = u.degree ;
            it->cost = u.cost ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR,
                    "Exception captured:%s while updating transaction info"
                    "in deadlock list",
                    e.what() ) ;
         }
      }
   }
}


// update dpsDeadlockTx degree in DPS_DEADLOCK_TX_SET
void dpsTDGraph::_updateTxNodeDegree( DPS_DEADLOCK_TX_SET * pSet ) 
{
   if ( pSet )
   {
      DPS_DEADLOCK_TX_SET_IT it = pSet->begin();
      while ( it != pSet->end() )
      {
         dpsDeadlockTx nodeU = *it ;
         it++ ;

         dpsTDVertex u, v, cv ;
         u.nodeId = nodeU.txId ;
         while ( _getAdjacentNode( u, cv, v ) )
         {
            // v is adjacent vertex of u
            dpsDeadlockTx nodeV( v.nodeId, 0, 0 );
            if ( dpsTDFindTxNode( nodeV, pSet ) )
            {
               nodeU.degree++ ; // increase nodeU out-degree
               nodeV.degree++ ; // increase nodeV in-degree
               dpsTDUpdateTxNode( nodeU, pSet ) ;
               dpsTDUpdateTxNode( nodeV, pSet ) ;
            }           
            cv = v ;
         }
      }
   }
}


// main driver to find strongly connect component in a directed graph
void dpsTDGraph::findSCC( DPS_DEADLOCK_TX_SET_LIST * pSetList,
                          BOOLEAN ignoreSingleton )
{
   if ( pSetList )
   {
      dpsTDEdge edge ;
      dpsTDVertex u ;
      _pSetList = pSetList;
      _vertices.clear();
      _clearStack() ;
      _depth = 0 ;

      for ( TD_EDGE_SET_IT it = _edges.begin(); it != _edges.end(); it++ )
      {
         try
         {
            edge = *it;
            u.resetAttr() ;
            u.nodeId = edge.waiter ;
            _vertices.insert( u ) ;

            u.nodeId = edge.holder ;
            _vertices.insert( u ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR,
                    "Exception captured:%s while adding a vertex "
                    "into vertex set",
                    e.what() ) ;
         }
      }

      for ( TD_VERTEX_SET_IT it = _vertices.begin();
            it != _vertices.end(); it++ )
      {
         u = *it ;
         if ( -1 == u.dfn )
         {
            _findSCC( u, ignoreSingleton ) ;
         }
      }
      
      if ( ! pSetList->empty() )
      {
         DPS_DEADLOCK_TX_SET_LIST_IT itr = pSetList->begin() ;
         while ( itr != pSetList->end() )
         {
             DPS_DEADLOCK_TX_SET * nodeSet = *itr ;
             _updateTxNodeDegree( nodeSet ) ;
             itr++ ;
         }
      }
   }
}


void clearSetList( DPS_DEADLOCK_TX_SET_LIST & setList )
{
   DPS_DEADLOCK_TX_SET_LIST::iterator it = setList.begin();
   while ( it != setList.end() )
   {
      DPS_DEADLOCK_TX_SET * pSet = *it ;
      if ( pSet )
      {  
         pSet->clear() ;
         delete pSet;
      }   
      it++;
   }
   setList.clear() ;
}


/*
     class deadlockDetector implementaion
*/
void  deadlockDetector::addSet( DPS_TRANS_WAIT_SET * pLockWaitSet )
{
   if ( pLockWaitSet && _pInfoSet )
   {
      DPS_TRANS_WAIT_SET_IT it = pLockWaitSet->begin();
      while( it != pLockWaitSet->end() )
      {
         try
         {
            _pInfoSet->insert( *it );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR,
               "Exception captured:%s while saving transaction waiting info",
               e.what() ) ;
         }
         it++;
      }
   }
}


void deadlockDetector::del( const DPS_TRANS_ID transId )
{
   DPS_TRANS_ID dummyHolderTransId = DPS_INVALID_TRANS_ID;
   DPS_TRANS_ID waiterTransId = transId;
   dpsDBNodeID    nodeId;
   dpsTransWait waitInfo( waiterTransId, dummyHolderTransId, nodeId ) ;

   DPS_TRANS_WAIT_SET_IT it = _pInfoSet->upper_bound( waitInfo );
   while( it != _pInfoSet->end() )
   {
      waitInfo = *it;
      it++;
      if ( transId == waitInfo.waiter )
      {
         _pInfoSet->erase( waitInfo );
      }
   }
}


void  deadlockDetector::del( const DPS_TRANS_ID transId,
                             const dpsDBNodeID & nodeID )

{
   DPS_TRANS_ID dummyHolderTransId = DPS_INVALID_TRANS_ID;
   dpsDBNodeID    nodeId;
   dpsTransWait waitInfo( transId, dummyHolderTransId, nodeId ) ;

   DPS_TRANS_WAIT_SET_IT it = _pInfoSet->upper_bound( waitInfo );
   while( it != _pInfoSet->end() )
   {
      waitInfo = *it;
      it++;
      if ( ( transId == waitInfo.waiter  ) &&
           ( nodeID  == waitInfo.nodeId  ) )
      {
         _pInfoSet->erase( waitInfo );
      }
   }
}


void deadlockDetector::dump( DPS_TRANS_WAIT_SET & result )
{
   DPS_TRANS_WAIT_SET_IT it = _pInfoSet->begin();
   while ( it != _pInfoSet->end() )
   {
      try
      {
         result.insert( *it ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR,
                 "Exception captured:%s while dumping transaction waiting info",
                 e.what() ) ;
      }
      it++;
   }
}


BOOLEAN deadlockDetector::detectCycle( dpsTDEdge & backEdge )
{
   BOOLEAN found = FALSE ;
   dpsTDGraph g ;

   DPS_TRANS_WAIT_SET_IT it = _pInfoSet->begin();
   while ( it != _pInfoSet->end() )
   {
      dpsTransWait info = *it;
      g.addEdge( dpsTDEdge( info.waiter, info.holder ) ) ;
      it++;
   }

   found = g.detectCycle( backEdge );
   g.reset();

   return found;
}


// sum up a given transaction cost( log space consumed ) in DPS_TRANS_WAIT_SET
UINT64 deadlockDetector::_getTxCost( const DPS_TRANS_ID   transId,
                                     DPS_TRANS_WAIT_SET * pInfoSet )
{
   UINT32 sumCost = 0;
   if ( pInfoSet )
   {
      ossPoolMap< dpsDBNodeID, UINT64 > costMap ;
      ossPoolMap< dpsDBNodeID, UINT64 >::iterator costMapItr ;

      DPS_TRANS_WAIT_SET_IT it = pInfoSet->begin() ;
      while ( it != pInfoSet->end() )
      {
         dpsTransWait waitInfo = *it;
         if ( ( transId == waitInfo.waiter ) || ( transId == waitInfo.holder ) )
         {
            costMapItr = costMap.find( waitInfo.nodeId ) ;
            if ( transId == waitInfo.waiter )
            {
               if ( waitInfo.waiterCost > 0 )
               {
                  if ( costMapItr == costMap.end() )
                  {
                     sumCost += waitInfo.waiterCost ;
                     try
                     {
                        costMap.insert( std::pair< dpsDBNodeID, UINT64 >
                           ( waitInfo.nodeId, waitInfo.waiterCost ) );
                     }
                     catch ( std::exception &e )
                     {
                        PD_LOG( PDERROR,
                                "Exception captured:%s while getting "
                                "waiter transaction cost",
                                e.what() ) ;
                     }
                  }
                  else if ( waitInfo.waiterCost > costMapItr->second )
                  {
                     sumCost += waitInfo.waiterCost - costMapItr->second ;
                     costMapItr->second = waitInfo.waiterCost ;
                  }
               }
            }
            else if ( waitInfo.holderCost > 0 )
            {
               if ( costMapItr == costMap.end() )
               {
                  sumCost += waitInfo.holderCost ;
                  try
                  {
                     costMap.insert( std::pair< dpsDBNodeID, UINT64 >
                                     ( waitInfo.nodeId, waitInfo.holderCost ) );
                  }
                  catch ( std::exception &e )
                  {  
                     PD_LOG( PDERROR,
                             "Exception captured:%s while getting "
                             "holder transaction cost",
                             e.what() ) ;
                  }
               }
               else if ( waitInfo.holderCost > costMapItr->second )
               {
                  sumCost += waitInfo.holderCost - costMapItr->second ;
                  costMapItr->second = waitInfo.holderCost ;
               }
            }
         }
         it++;
      }
      costMap.clear();
   }
   return sumCost ;
}


// update dpsDeadlockTx cost
void deadlockDetector::_updateTxNodeCost( DPS_DEADLOCK_TX_SET * pSet,
                                          DPS_TRANS_WAIT_SET  * pInfoSet )
{
   if ( pSet && pInfoSet )
   {
      DPS_DEADLOCK_TX_SET_IT it = pSet->begin();
      while ( it != pSet->end() )
      {
         dpsDeadlockTx nodeU = *it ;
         it++ ;
         nodeU.cost = _getTxCost( nodeU.txId, pInfoSet ) ;
         dpsTDUpdateTxNode( nodeU, pSet ) ;
      }
   }
}


BOOLEAN deadlockDetector::findSCC( DPS_DEADLOCK_TX_SET_LIST * pSetList,
                                   BOOLEAN                    ignoreSingleton )
{
   BOOLEAN found = FALSE ;
   dpsTDGraph g ;

   if ( ( ! _pInfoSet ) || _pInfoSet->empty() )
   {
      return found ;
   }

   DPS_TRANS_WAIT_SET_IT it = _pInfoSet->begin();
   while ( it != _pInfoSet->end() )
   {
      dpsTransWait info = *it;
      g.addEdge( dpsTDEdge( info.waiter, info.holder ) ) ;
      it++;
   }

   // find SCC in graph and SCCs found are saved in result set list
   // if there are any
   g.findSCC( pSetList, ignoreSingleton );

   // result set list is not empty, impiles a SCC is found
   if ( pSetList && ( ! pSetList->empty() ) )
   {
      found = TRUE;

      DPS_DEADLOCK_TX_SET_LIST_IT itr = pSetList->begin() ;
      while ( itr != pSetList->end() )
      {
         DPS_DEADLOCK_TX_SET * nodeSet = *itr ;
         _updateTxNodeCost( nodeSet, _pInfoSet ) ;
         itr++ ;
      }
   }

   g.reset();

   return found;
}


} // namespace engine
