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

   Source File Name = dpsDeadlockDetector.hpp

   Descriptive Name = Deadlock detector

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/20/2021  JT  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSDEADLOCKDETECOR_HPP_
#define DPSDEADLOCKDETECOR_HPP_

#include "ossLatch.hpp"         // ossSpinXLatch
#include "dpsDef.hpp"           // DPS_TRANS_ID
#include "dpsTransLockDef.hpp"  // dpsTransLockId
#include "utilPooledObject.hpp"
#include "ossTypes.h"
#include "ossMem.hpp"
#include "msgDef.h"             // INVALID_GROUPID
#include "msg.h"                // MsgRouteID
#include "pmdDef.hpp"           // PMD_INVALID_EDUID
#include <stack>            

using namespace std ;
namespace engine
{

/*
   Deadlock detector

   We use a directed graph to describe transactions(waiters) that waiting for
   a lock held by another transactions(holders). Waiters and holders are
   vertexes in the graph, and the waiter->holder relationship is an 'edge'.
   If a cyclic path is detected when traversal the graph, it implies a deadlock
   occurred.

   The class dpsTDGraph is designed to achieve following tasks :
     1. detect a cyclic path in direct graph using Depth-first search (DFS)
        algorithm.
     2. find all Strong Connected Component ( SCC ) in direct graph using Tarjan
        algorithm.
*/


/*
   class dpsTDEdge represents a directed edge in directed graph.
*/
class dpsTDEdge : public utilPooledObject
{
public:
   void reset()
   {
      waiter = DPS_INVALID_TRANS_ID ;
      holder = DPS_INVALID_TRANS_ID ;
   }

   dpsTDEdge()
   {
      reset() ;
   }

   dpsTDEdge( const dpsTDEdge & rhs )
   {
      waiter = rhs.waiter ;
      holder = rhs.holder ;
   }

   dpsTDEdge( DPS_TRANS_ID Waiter, DPS_TRANS_ID Holder )
   {
      waiter = Waiter ;
      holder = Holder ;
   }

   virtual ~dpsTDEdge() {}

   dpsTDEdge & operator= ( const dpsTDEdge & rhs )
   {
      waiter = rhs.waiter ;
      holder = rhs.holder ;
      return *this ;
   }

   BOOLEAN isValid()
   {
      return ( ( DPS_INVALID_TRANS_ID != waiter ) &&
               ( DPS_INVALID_TRANS_ID != holder ) ) ? TRUE : FALSE ;
   }

   BOOLEAN operator== ( const dpsTDEdge & rhs ) const
   {
      if ( ( waiter == rhs.waiter ) && ( holder == rhs.holder ) )
      {
         return TRUE;
      }
      return FALSE ;
   }

   BOOLEAN operator< ( const dpsTDEdge & rhs ) const
   {
      if ( waiter < rhs.waiter )
      {
         return TRUE ;
      }
      else if ( waiter == rhs.waiter )
      {
         if ( holder < rhs.holder )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      return FALSE ;
   }

public:
   DPS_TRANS_ID  waiter ;
   DPS_TRANS_ID  holder ;
} ;
typedef ossPoolSet < dpsTDEdge >    TD_EDGE_SET ;
typedef TD_EDGE_SET::iterator       TD_EDGE_SET_IT;


enum DPS_TD_VCOLOR
{
    WHITE, // Vertex is not processed yet. Initially, all vertices are WHITE.
    GRAY,  // Vertex is being processed, but not finished which means that all
           // descendants of this vertex are not processed yet
    BLACK  // Vertex and all its descendants are processed.
} ;


/*
   class vertex represents a vertex ( or a node ) in a directed graph.
*/
class dpsTDVertex : public utilPooledObject
{
public:
   DPS_TRANS_ID nodeId; // vertex id, i.e., node number
   mutable DPS_TD_VCOLOR color; // node color, i.e., the processing status of this node

   // attributies dfn and low are used by Tarjan's algorithm in findng
   // Strongly Connected Components of a directed graph 

   mutable SINT32 dfn ;   // DFS number, the sequence when a node visited
                        // during DFS traversal  
   mutable SINT32 low ;   // low-link value, the topmost reachable ancestor(
                        // with minimum possible dfn value ) via the subtree
                        // of that node. 
public:
   void resetAttr()
   {
      color  = WHITE ;
      dfn    = -1 ;
      low    = -1 ;
   }

   dpsTDVertex()
   {
      nodeId = DPS_INVALID_TRANS_ID;
      resetAttr();
   }

   dpsTDVertex( const dpsTDVertex & rhs )
   {
      nodeId = rhs.nodeId ;
      color  = rhs.color ;
      dfn    = rhs.dfn  ;
      low    = rhs.low ;
   }

   dpsTDVertex ( DPS_TRANS_ID  id,
                 DPS_TD_VCOLOR col,
                 SINT32 dfsNum,
                 SINT32 lowVal )
   {
      nodeId = id ;
      color  = col ;
      dfn    = dfsNum ;
      low    = lowVal ;
   }

   virtual ~dpsTDVertex() {}

   dpsTDVertex & operator= ( const dpsTDVertex & rhs )
   {
      nodeId = rhs.nodeId ;
      color  = rhs.color ;
      dfn    = rhs.dfn ;
      low    = rhs.low ;
      return *this ;
   }

   BOOLEAN operator== ( const dpsTDVertex & rhs ) const
   {
      return ( nodeId == rhs.nodeId ) ;
   }

   BOOLEAN operator!= ( const dpsTDVertex & rhs ) const
   {
      return ( nodeId != rhs.nodeId ) ;
   }

   BOOLEAN operator< ( const dpsTDVertex & rhs ) const
   {
      return ( nodeId < rhs.nodeId ) ;
   }
} ;


/*
   class dpsDeadlockTx represents an element ( transaction ) in result SCC
*/
class dpsDeadlockTx : public utilPooledObject
{
public:
   DPS_TRANS_ID txId;       // transId
   mutable UINT32 degree ;    // in_degree + out_degree
   mutable UINT64 cost ;
   
public:
   dpsDeadlockTx()
   {
      txId   = DPS_INVALID_TRANS_ID;
      degree = 0;
      cost   = 0;
   }

   dpsDeadlockTx( const dpsDeadlockTx & rhs )
   {
      txId    = rhs.txId ;
      degree  = rhs.degree ;
      cost    = rhs.cost ;
   }

   dpsDeadlockTx ( DPS_TRANS_ID txID, UINT32 txDegree, UINT64 txCost )
   {
      txId   = txID ;
      degree = txDegree ;
      cost   = txCost ;
   }

   virtual ~dpsDeadlockTx() {}

   dpsDeadlockTx & operator= ( const dpsDeadlockTx & rhs )
   {
      txId    = rhs.txId ;
      degree  = rhs.degree ;
      cost    = rhs.cost ;
      return *this ;
   }

   BOOLEAN operator== ( const dpsDeadlockTx & rhs ) const
   {
      return ( txId == rhs.txId ) ;
   }

   BOOLEAN operator!= ( const dpsDeadlockTx & rhs ) const
   {
      return ( txId != rhs.txId ) ;
   }

   BOOLEAN operator< ( const dpsDeadlockTx & rhs ) const
   {
      return ( txId < rhs.txId ) ;
   }
} ;
typedef ossPoolSet < dpsDeadlockTx >  DPS_DEADLOCK_TX_SET ;
typedef DPS_DEADLOCK_TX_SET::iterator DPS_DEADLOCK_TX_SET_IT ;

typedef ossPoolList< DPS_DEADLOCK_TX_SET * > DPS_DEADLOCK_TX_SET_LIST ;
typedef DPS_DEADLOCK_TX_SET_LIST::iterator   DPS_DEADLOCK_TX_SET_LIST_IT ;

// find dpsDeadlockTx u in DPS_DEADLOCK_TX_SET
extern BOOLEAN dpsTDFindTxNode( dpsDeadlockTx & u, DPS_DEADLOCK_TX_SET * pSet );

// update dpsDeadlockTx atrribute of dpsDeadlockTx in DPS_DEADLOCK_TX_SET
extern void dpsTDUpdateTxNode( dpsDeadlockTx & u, DPS_DEADLOCK_TX_SET * pSet ) ;


/*
   class dpsTDGraph represents a directed graph
*/
class dpsTDGraph : public utilPooledObject
{
   typedef ossPoolSet < dpsTDVertex >       TD_VERTEX_SET;
   typedef TD_VERTEX_SET::iterator          TD_VERTEX_SET_IT;
   typedef std::stack< dpsTDVertex >        TD_VERTEX_STK ;
public:

   dpsTDGraph()
   {
      reset();
   }

   void reset()
   {
      _edges.clear();
      _vertices.clear();
      _backEdge.reset();
      _clearStack();
      _depth = 0;
      _pSetList = NULL;
   }

   virtual ~dpsTDGraph()
   {
      reset() ;
   }

   // add a direct edge into graph
   void addEdge( dpsTDEdge e ) ;

   void addEdge( TD_EDGE_SET & edgeSet ) ;

   // main driver to detect a cyclic path in a directed graph via DFS
   BOOLEAN detectCycle( dpsTDEdge & backEdge );

   // main driver to find strongly connect component in a directed graph
   void findSCC( DPS_DEADLOCK_TX_SET_LIST * pSetList,
                 BOOLEAN ignoreSingleton = TRUE );

private:
   // search on _edges and get an adjancent vertex of u
   BOOLEAN _getAdjacentNode( const dpsTDVertex & u,
                             const dpsTDVertex & cv,
                             dpsTDVertex & v );

   // recursively iterate a subtree rooted with vertex u,
   // returns true if a circle is detected.
   BOOLEAN _isCyclic( dpsTDVertex & u );

   // update attribute of vertex u
   void    _update( dpsTDVertex & u );

   // find vertex u in vertex set _vertices
   BOOLEAN _find( dpsTDVertex & u );

   // find strongly connected component by Tarjan's algorithm
   void    _findSCC( dpsTDVertex & u, BOOLEAN ignoreSingularity ) ;

   void    _clearStack() ;

   // update dpsDeadlockTx degree
   void _updateTxNodeDegree( DPS_DEADLOCK_TX_SET * pSet ) ;

private:
   TD_EDGE_SET     _edges ;    // edges in directed graph
   TD_VERTEX_SET   _vertices ; // vertex set

   dpsTDEdge       _backEdge ; // the back edge found when detect circle by DFS 

   TD_VERTEX_STK   _stack ;    // processing vertex stack 
   SINT32          _depth ;    // index/sequence number using DFS traversal
   DPS_DEADLOCK_TX_SET_LIST * _pSetList ; // list of sets, each set saves a SCC
} ;


/*
    class dpsDBNodeID represents a node in SequoiaDB cluster
 */
class dpsDBNodeID
{
public:
   UINT32 groupID ;
   UINT32 nodeID ;
public:
   void reset()
   {
      nodeID  = INVALID_NODEID ; 
      groupID = INVALID_GROUPID ;
   }

   dpsDBNodeID()
   {
      reset();
   }

   dpsDBNodeID( const dpsDBNodeID & rhs )
   {
      groupID = rhs.groupID;
      nodeID  = rhs.nodeID ;
   }

   dpsDBNodeID( const UINT32 groupId, const UINT32 nodeId )
   {
      groupID = groupId;
      nodeID  = nodeId;
   }

   virtual ~dpsDBNodeID() { }

   OSS_INLINE dpsDBNodeID & operator= ( const dpsDBNodeID & rhs )
   {
      groupID = rhs.groupID;
      nodeID  = rhs.nodeID ;
      return *this ;
   }

   BOOLEAN operator== ( const dpsDBNodeID &rhs ) const
   {
      if ( ( groupID == rhs.groupID ) && ( nodeID == rhs.nodeID ) )
      {
         return TRUE;
      }
      return FALSE ;
   }
   
   BOOLEAN operator< ( const dpsDBNodeID &rhs ) const
   {
      if ( groupID < rhs.groupID )
      {
         return TRUE;
      }
      else if ( groupID == rhs.groupID )
      {
         if ( nodeID < rhs.nodeID )
         {
            return TRUE;
         }
         return FALSE;
      }
      return FALSE;
   }
} ;


/*
   class dpsTransWait is transaction waiting info
   collected by lock manager at runtime
*/
class dpsTransWait : public utilPooledObject
{
public:
   void reset()
   {
      waiter  = DPS_INVALID_TRANS_ID ;
      holder  = DPS_INVALID_TRANS_ID ;
      nodeId.reset() ;
      waiterCost = 0 ;
      holderCost = 0 ;
      waitTime   = 0 ;
      waiterSessionID = PMD_INVALID_EDUID ;
      holderSessionID = PMD_INVALID_EDUID ;
      waiterRelatedID = 0;
      holderRelatedID = 0;
      waiterRelatedTID = 0;
      holderRelatedTID = 0;
      waiterRelatedSessionID = PMD_INVALID_EDUID ;
      holderRelatedSessionID = PMD_INVALID_EDUID ;
      waiterRelatedNID.value = MSG_INVALID_ROUTEID ;
      holderRelatedNID.value = MSG_INVALID_ROUTEID ;
   }

   dpsTransWait()
   {
      reset() ;
   }

   dpsTransWait ( const dpsTransWait & rhs )
   {
      waiter       = rhs.waiter ;
      holder       = rhs.holder ;
      nodeId       = rhs.nodeId ;
      waiterCost   = rhs.waiterCost ;
      holderCost   = rhs.holderCost ;
      waitTime     = rhs.waitTime ;
      waiterSessionID  = rhs.waiterSessionID;
      holderSessionID  = rhs.holderSessionID;
      waiterRelatedID  = rhs.waiterRelatedID;
      holderRelatedID  = rhs.holderRelatedID;
      waiterRelatedTID = rhs.waiterRelatedTID;
      holderRelatedTID = rhs.holderRelatedTID;
      waiterRelatedSessionID = rhs.waiterRelatedSessionID;
      holderRelatedSessionID = rhs.holderRelatedSessionID;
      waiterRelatedNID = rhs.waiterRelatedNID;
      holderRelatedNID = rhs.holderRelatedNID;
   }

   dpsTransWait( const DPS_TRANS_ID  waiterTx,
                 const DPS_TRANS_ID  holderTx,
                 const dpsDBNodeID & nodeID, 
                 UINT64 waitingTime  = 0,
	         UINT64 waiterTxCost = 0,
                 UINT64 holderTxCost = 0,
                 EDUID  waiterTxSessionID  = PMD_INVALID_EDUID,
                 EDUID  holderTxSessionID  = PMD_INVALID_EDUID,
                 UINT64 waiterTxRelatedID  = 0,
                 UINT64 holderTxRelatedID  = 0,
                 UINT32 waiterTxRelatedTID = 0,
                 UINT32 holderTxRelatedTID = 0,
                 EDUID  waiterTxRelatedSessionID = PMD_INVALID_EDUID,
                 EDUID  holderTxRelatedSessionID = PMD_INVALID_EDUID,
                 UINT64 waiterTxRelatedNIDValue = MSG_INVALID_ROUTEID,
                 UINT64 holderTxRelatedNIDValue = MSG_INVALID_ROUTEID )
   {
      waiter       = waiterTx ;
      holder       = holderTx ;
      nodeId       = nodeID ;
      waitTime     = waitingTime ;
      waiterCost   = waiterTxCost ;
      holderCost   = holderTxCost ;
      waiterSessionID = waiterTxSessionID;
      holderSessionID = holderTxSessionID;
      waiterRelatedID = waiterTxRelatedID;
      holderRelatedID = holderTxRelatedID;
      waiterRelatedTID = waiterTxRelatedTID;
      holderRelatedTID = holderTxRelatedTID;
      waiterRelatedSessionID = waiterTxRelatedSessionID;
      holderRelatedSessionID = holderTxRelatedSessionID;
      waiterRelatedNID.value = waiterTxRelatedNIDValue;
      holderRelatedNID.value = holderTxRelatedNIDValue;
   }


  dpsTransWait( const DPS_TRANS_ID   waiterTx,
                const DPS_TRANS_ID   holderTx,
                const dpsDBNodeID &  nodeID,
                UINT64 waitingTime,
                UINT64 waiterTxCost,
                UINT64 holderTxCost,
                EDUID  waiterTxSessionID,
                EDUID  holderTxSessionID,
                UINT64 waiterTxRelatedID,
                UINT64 holderTxRelatedID,
                UINT32 waiterTxRelatedTID,
                UINT32 holderTxRelatedTID,
                EDUID  waiterTxRelatedSessionID,
                EDUID  holderTxRelatedSessionID,
                MsgRouteID waiterTxRelatedNID,
                MsgRouteID holderTxRelatedNID  )
   {
      waiter       = waiterTx ;
      holder       = holderTx ;
      nodeId       = nodeID;
      waitTime     = waitingTime ;
      waiterCost   = waiterTxCost ;
      holderCost   = holderTxCost ;
      waiterSessionID = waiterTxSessionID;
      holderSessionID = holderTxSessionID;
      waiterRelatedID = waiterTxRelatedID;
      holderRelatedID = holderTxRelatedID;
      waiterRelatedTID = waiterTxRelatedTID;
      holderRelatedTID = holderTxRelatedTID;
      waiterRelatedNID = waiterTxRelatedNID;
      holderRelatedNID = holderTxRelatedNID;
      waiterRelatedSessionID = waiterTxRelatedSessionID;
      holderRelatedSessionID = holderTxRelatedSessionID;
   }

   virtual ~dpsTransWait() { }

   OSS_INLINE dpsTransWait & operator= ( const dpsTransWait & rhs )
   {
      waiter       = rhs.waiter ;
      holder       = rhs.holder ;
      nodeId       = rhs.nodeId ;
      waitTime     = rhs.waitTime ;
      waiterCost   = rhs.waiterCost ;
      holderCost   = rhs.holderCost ;
      waiterSessionID = rhs.waiterSessionID;
      holderSessionID = rhs.holderSessionID;
      waiterRelatedID = rhs.waiterRelatedID;
      holderRelatedID = rhs.holderRelatedID;
      waiterRelatedTID = rhs.waiterRelatedTID;
      holderRelatedTID = rhs.holderRelatedTID;
      waiterRelatedSessionID = rhs.waiterRelatedSessionID;
      holderRelatedSessionID = rhs.holderRelatedSessionID;
      waiterRelatedNID = rhs.waiterRelatedNID;
      holderRelatedNID = rhs.holderRelatedNID;
      return *this ;
   }

   BOOLEAN operator== ( const dpsTransWait &rhs ) const
   {
      if ( ( waiter   == rhs.waiter  ) &&
           ( holder   == rhs.holder  ) &&
           ( nodeId   == rhs.nodeId  ) )
      {
         return TRUE;
      }
      return FALSE ;
   }

   BOOLEAN operator< ( const dpsTransWait &rhs ) const
   {
      if ( waiter < rhs.waiter )
      {
         return TRUE ;
      }
      else if ( waiter == rhs.waiter )
      {
         if ( holder < rhs.holder )
         {
            return TRUE ;
         }
         else if ( holder == rhs.holder )
         {
            if ( nodeId < rhs.nodeId )
            {
               return TRUE;
            }
            return FALSE;
         }
         return FALSE ;
      }
      return FALSE ;
   }
public:
   DPS_TRANS_ID waiter ;
   DPS_TRANS_ID holder ;
   dpsDBNodeID  nodeId ;
   UINT64       waitTime ;
   UINT64       waiterCost; // log space consumed by waiter transaction
   UINT64       holderCost; // log space consumed by holder transaction
   UINT64       waiterSessionID;
   UINT64       holderSessionID;
   UINT64       waiterRelatedID;
   UINT64       holderRelatedID;
   UINT32       waiterRelatedTID;
   UINT32       holderRelatedTID;
   EDUID        waiterRelatedSessionID ;
   EDUID        holderRelatedSessionID ;
   MsgRouteID   waiterRelatedNID ; 
   MsgRouteID   holderRelatedNID ;
} ;
typedef ossPoolSet< dpsTransWait >   DPS_TRANS_WAIT_SET ;
typedef DPS_TRANS_WAIT_SET::iterator DPS_TRANS_WAIT_SET_IT ;


// IP:00000000, PORT:0000, EDUID:0000000000000000
/// SNPRINTF will truncate the last char, so need + 2
#define DPS_TRANS_RELATED_ID_STR_LEN ( 8 + 4 + 16 + 2 )

class dpsTransRelatedIDs : public utilPooledObject
{
public:
   void reset()
   {
      ossMemset( relatedID, 0, sizeof( relatedID ) ) ;
      sessionID = PMD_INVALID_EDUID ;
      relatedNID.value = MSG_INVALID_ROUTEID ;
   }

   dpsTransRelatedIDs()
   {
      reset();
   }

   dpsTransRelatedIDs( const CHAR * relatedId,
                       EDUID        sessionId,
                       MsgRouteID   relatedNId )
   {
      if ( relatedId )
      {
         ossSnprintf( relatedID, sizeof( relatedID ), "%s", relatedId ) ;
      }
      else
      {
         ossMemset( relatedID, 0, sizeof( relatedID ) ) ;
      }
      sessionID  = sessionId ;
      relatedNID = relatedNId ;
   }

   dpsTransRelatedIDs( const dpsTransRelatedIDs & rhs )
   {
      ossSnprintf( relatedID, sizeof( relatedID ), "%s", rhs.relatedID ) ;
      sessionID  = rhs.sessionID ;
      relatedNID = rhs.relatedNID ;
   }

   dpsTransRelatedIDs & operator= ( const dpsTransRelatedIDs & rhs )
   {
      ossSnprintf( relatedID, sizeof( relatedID ), "%s", rhs.relatedID ) ;
      sessionID  = rhs.sessionID ;
      relatedNID = rhs.relatedNID ;
      return *this ;
   }

   virtual ~dpsTransRelatedIDs() {}

public:
   CHAR       relatedID[ DPS_TRANS_RELATED_ID_STR_LEN + 1 ] ;
   EDUID      sessionID ;
   MsgRouteID relatedNID ;
} ;
typedef ossPoolMap< DPS_TRANS_ID, dpsTransRelatedIDs > DPS_TRANS_RELATEDID_MAP ;


//
class deadlockDetector : public utilPooledObject
{
public:
   deadlockDetector( DPS_TRANS_WAIT_SET * pInfoSet )
   {
      SDB_ASSERT( ( pInfoSet != NULL ),
                  "Invalid argument, pInfoSet must not be NULL" ) ;
      _pInfoSet = pInfoSet;
   }

   virtual ~deadlockDetector() { }

   OSS_INLINE void add( const dpsTransWait & info )
   {
      _pInfoSet->insert( info );
   }
 
   OSS_INLINE void del( const dpsTransWait & info ) 
   {
      _pInfoSet->erase( info );
   }

   void del( const DPS_TRANS_ID   waiterTransId,
             const dpsDBNodeID  & nodeId );

   void del( const DPS_TRANS_ID waiterTransId );

   void addSet( DPS_TRANS_WAIT_SET * pLockWaitSet ) ;

   void dump( DPS_TRANS_WAIT_SET & result ) ;

   BOOLEAN detectCycle( dpsTDEdge & backEdge ) ;

   BOOLEAN findSCC( DPS_DEADLOCK_TX_SET_LIST * pSetList,
                    BOOLEAN ignoreSingleton = TRUE ) ;

private :
   // update dpsDeadlockTx cost
   void   _updateTxNodeCost( DPS_DEADLOCK_TX_SET * pSet,
                             DPS_TRANS_WAIT_SET  * pInfoSet ) ;

   // sum up a transaction cost( log space consumed ) in DPS_TRANS_WAIT_SET
   UINT64 _getTxCost( const DPS_TRANS_ID txId, DPS_TRANS_WAIT_SET * pSet );

private :
   DPS_TRANS_WAIT_SET * _pInfoSet ;
} ;

extern void clearSetList( DPS_DEADLOCK_TX_SET_LIST & setList ) ;

}
#endif // DPSDEADLOCKDETECOR_HPP_
