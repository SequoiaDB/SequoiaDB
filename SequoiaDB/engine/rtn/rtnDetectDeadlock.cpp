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

   Source File Name = rtnDetectDeadlock.cpp

   Descriptive Name = detect deadlocks among all the transactions

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2021  JT  Initial draft

   Last Changed =

*******************************************************************************/
#include "rtnDetectDeadlock.hpp"
#include "rtn.hpp"
#include "dpsUtil.hpp"
#include "msgMessage.hpp"
#include "ossUtil.h"

using namespace bson ;

namespace engine
{
   /*
      _parseAndSaveWaitInfo, a helper function parses a BSONObj which is
      output record of snapshot, SDB_SNAP_TRANSWAITS, and saves it into
      DPS_TRANS_WAIT_SET
   */
   static INT32 _parseAndSaveWaitInfo( SDB_ROLE                & dbRole,
                                       const BSONObj           & obj,
                                       DPS_TRANS_WAIT_SET      & waitInfoSet,
                                       DPS_TRANS_RELATEDID_MAP & relatedIDMap )
   {
      INT32 rc = SDB_OK;

      if ( SDB_ROLE_MAX == dbRole )
      {
         dbRole = pmdGetDBRole() ;
      }

      DPS_TRANS_ID waiterTransId = DPS_INVALID_TRANS_ID,
                   holderTransId = DPS_INVALID_TRANS_ID;
      UINT64 waiterCost = 0, holderCost = 0 ;
      dpsDBNodeID nodeId ;
      dpsTransRelatedIDs wtrRelatedIDs, hdrRelatedIDs ;

      BSONObjIterator itr( obj ) ;
      while ( itr.more() )         
      {
         BSONElement e = itr.next() ;
         const CHAR *field = e.fieldName() ;

         if ( 0 == ossStrcmp( field, FIELD_NAME_GROUPID ) )
         {
            // groupID
            nodeId.groupID = e.numberInt() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_NODEID ) )
         {
            // nodeID
            nodeId.nodeID = e.numberInt() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_WAITER_TRANSID ) )
         {
            // waiter transID
            dpsGetTransIDFromString( e.valuestr(), waiterTransId ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_HOLDER_TRANSID ) )
         {
            // holder transID
            dpsGetTransIDFromString( e.valuestr(), holderTransId ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_WAITER_TRANS_COST ) )
         {
            // waiter cost
            waiterCost = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_HOLDER_TRANS_COST ) )
         {
            // holde cost
            holderCost = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_WAITER_RELATED_ID ) )
         { 
            // waiter RelatedID
            ossSnprintf( wtrRelatedIDs.relatedID,
                         sizeof( wtrRelatedIDs.relatedID ),
                         "%s", e.valuestr() ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_HOLDER_RELATED_ID ) )
         {
            // holder RelatedID
            ossSnprintf( hdrRelatedIDs.relatedID,
                         sizeof( hdrRelatedIDs.relatedID ),
                         "%s", e.valuestr() );
         }
         else if ( SDB_ROLE_COORD == dbRole )
         {
            if ( ! ossStrcmp( field, FIELD_NAME_WAITER_RELATED_SESSIONID ) )
            { 
               // waiter related sessionID
               wtrRelatedIDs.sessionID = e.numberLong() ;
            }
            else if ( ! ossStrcmp( field, FIELD_NAME_HOLDER_RELATED_SESSIONID ))
            {
               // holder related sessionID
               hdrRelatedIDs.sessionID = e.numberLong() ;
            }
            else if ( ! ossStrcmp(field, FIELD_NAME_WAITER_RELATED_NODEID ) )
            {
               // waiter related NodeID
               wtrRelatedIDs.relatedNID.columns.nodeID = e.numberInt() ;
            } 
            else if ( ! ossStrcmp(field, FIELD_NAME_HOLDER_RELATED_NODEID ) )
            {
               // holder related NodeID
               hdrRelatedIDs.relatedNID.columns.nodeID = e.numberInt() ;
            }
            else if ( ! ossStrcmp( field, FIELD_NAME_WAITER_RELATED_GROUPID ) )
            {
               // waiter related GroupID
               wtrRelatedIDs.relatedNID.columns.groupID = e.numberInt() ;
            } 
            else if ( ! ossStrcmp( field, FIELD_NAME_HOLDER_RELATED_GROUPID ) )
            {
               // holder related GroupID
               hdrRelatedIDs.relatedNID.columns.groupID = e.numberInt() ;
            }
         }
         else
         {
            if ( ! ossStrcmp( field, FIELD_NAME_WAITER_SESSIONID ) )
            {
               // waiter sessionID
               wtrRelatedIDs.sessionID = e.numberLong() ;
               // waiter nodeID and groupID
               wtrRelatedIDs.relatedNID.columns.nodeID  = nodeId.nodeID ;
               wtrRelatedIDs.relatedNID.columns.groupID = nodeId.groupID ;
            }
            else if ( ! ossStrcmp( field, FIELD_NAME_HOLDER_SESSIONID ) )
            {
               // holder sessionID
               hdrRelatedIDs.sessionID = e.numberLong() ;
               // holder nodeID and groupID
               hdrRelatedIDs.relatedNID.columns.nodeID  = nodeId.nodeID ;
               hdrRelatedIDs.relatedNID.columns.groupID = nodeId.groupID ;
            }
         }
      }

      if ( ( DPS_INVALID_TRANS_ID != waiterTransId ) &&
           ( DPS_INVALID_TRANS_ID != holderTransId ) )
      {
         try
         {
            relatedIDMap.insert( std::pair< DPS_TRANS_ID, dpsTransRelatedIDs >
                                 ( waiterTransId, wtrRelatedIDs ) ) ;
            relatedIDMap.insert( std::pair< DPS_TRANS_ID, dpsTransRelatedIDs >
                                 ( holderTransId, hdrRelatedIDs ) ) ;

            // save result in TRANS_WAIT_SET
            dpsTransWait waitInfo( waiterTransId, holderTransId, nodeId,
                                   0, // waitTime
                                   waiterCost, holderCost ) ;
            waitInfoSet.insert( waitInfo ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR,
                    "Exception captured:%s while parsing trans waiting info",
                    e.what() ) ;
         }
      }

      return rc ;
   }


   static BOOLEAN _getRelatedIDs( DPS_TRANS_ID              transId,
                                  DPS_TRANS_RELATEDID_MAP & relatedIDMap,
                                  dpsTransRelatedIDs      & relatedIDs )
   {
      DPS_TRANS_RELATEDID_MAP::iterator it = relatedIDMap.find( transId ) ;
      if ( it != relatedIDMap.end() )
      {
         relatedIDs = it->second ;
         return TRUE ;
      } 
      return FALSE ;
   }

   // compare function for qsort.
   //    Sort on degree field in descending order,
   //    if degree field is equal, sort on cost field in ascending order.
   static int _compareTxNode( const void * a, const void * b )
   {
      int val = ((dpsDeadlockTx*)b)->degree - ((dpsDeadlockTx*)a)->degree ;
      return ( 0 == val ) ? ( ((dpsDeadlockTx*)a)->cost -
                              ((dpsDeadlockTx*)b)->cost )
                          : val ;
   }

   static void _convertAndSort( DPS_DEADLOCK_TX_SET * pSet,
                                dpsDeadlockTx       * pDeadlockTxArray )
   {
      if ( pSet && ( ! pSet->empty() ) && pDeadlockTxArray )
      {
         UINT32 i = 0 ;
         DPS_DEADLOCK_TX_SET_IT itr = pSet->begin() ;
         while ( itr != pSet->end() )
         {
            pDeadlockTxArray[i] = *itr ;
            i++ ;
            itr++;
         }
         // sort on degree field in descending order
         qsort( pDeadlockTxArray, pSet->size(),
                sizeof( dpsDeadlockTx ), _compareTxNode ) ;
      }
   }


   /*
      _constructResult: construct result set 
   */
   #define COORD_CMD_DEADLOCK_BSON_BUILDER_DEFAULT_SZ ( 256 ) 
   static INT32 _constructResult( DPS_DEADLOCK_TX_SET_LIST & setList,
                                  DPS_TRANS_RELATEDID_MAP  & relatedIDMap,
                                  dpsDeadlockTx            * pDeadlockTxArray,
                                  RTN_DEADLOCK_RESULT_LIST & deadlockResList )
   {
      INT32 rc = SDB_OK ;
      try
      {
         UINT32 deadlockCounter = 0 ;
         DPS_DEADLOCK_TX_SET_LIST_IT it = setList.begin();
         while ( ( it != setList.end() ) && pDeadlockTxArray )
         {
            DPS_DEADLOCK_TX_SET * pSet = *it ;
            if ( pSet && ( ! pSet->empty() ) )
            {
               deadlockCounter++ ;

               // convert DPS_DEADLOCK_TX_SET to an array of dpsDeadlockTx, and
               // sort on dpsDeadlockTx.degree field in descending order
               _convertAndSort( pSet, pDeadlockTxArray );
 
               for ( UINT32 i = 0; i < pSet->size(); i++ )
               {
                  dpsDeadlockTx node = pDeadlockTxArray[i] ;

                  BSONObjBuilder tx(COORD_CMD_DEADLOCK_BSON_BUILDER_DEFAULT_SZ);

                  // deadlockID
                  tx.append( FIELD_NAME_DEADLOCKID, (INT32)deadlockCounter ) ;
                  // transID
                  CHAR strTransID[ DPS_TRANS_STR_LEN + 1 ] = { 0 } ;
                  dpsTransIDToString(node.txId, strTransID, DPS_TRANS_STR_LEN);
                  tx.append( FIELD_NAME_TRANSACTION_ID, strTransID ) ;
                  // degree
                  tx.append( FIELD_NAME_DEGREE, (INT32)node.degree ) ;
                  // cost 
                  tx.append( FIELD_NAME_COST, (INT64)node.cost ) ;
                  // RelatedID
                  dpsTransRelatedIDs relatedIDs ;
                  CHAR strRelatedID[ DPS_TRANS_RELATED_ID_STR_LEN + 1 ] = { 0 };
                  if ( _getRelatedIDs( node.txId, relatedIDMap, relatedIDs ) )
                  {
                     ossSnprintf( strRelatedID, sizeof( strRelatedID ),
                                  "%s", relatedIDs.relatedID ) ;
                  }
                  tx.append( FIELD_NAME_RELATED_ID, strRelatedID ) ;
                  // Related SessionID
                  tx.append( FIELD_NAME_SESSIONID, (INT64)relatedIDs.sessionID);
                  // Related GroupID
                  tx.append( FIELD_NAME_GROUPID, 
                             (INT32)relatedIDs.relatedNID.columns.groupID ) ;
                  // Related NodeID
                  tx.append( FIELD_NAME_NODEID, 
                             (INT32)relatedIDs.relatedNID.columns.nodeID ) ;

                  BSONObj obj = tx.obj();
                  deadlockResList.push_back( obj.getOwned() ) ;   
               }
            }
            it++;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR,
                 "Exception captured :%s when construct result",
                 e.what() ) ;
      }
      return rc ; 
   }


   // get max size of each DPS_DEADLOCK_TX_SET in the list 
   static UINT32 _getMaxSize( DPS_DEADLOCK_TX_SET_LIST & setList )
   {
      UINT32 maxSize = 0 ;
      for ( DPS_DEADLOCK_TX_SET_LIST_IT it = setList.begin();
            it != setList.end(); it++ ) 
      {
         DPS_DEADLOCK_TX_SET * pSet = *it ;
         if ( pSet && ( maxSize < pSet->size() ) )
         {
            maxSize = pSet->size() ;
         }
      }        
      return maxSize ;
   }


   /*
      _rtnDetectDeadlock implementation
   */
   _rtnDetectDeadlock::_rtnDetectDeadlock()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
      _dbRole = SDB_ROLE_MAX ;
   }

   _rtnDetectDeadlock::~_rtnDetectDeadlock()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
      _dbRole = SDB_ROLE_MAX ;
      _waitInfoSet.clear() ;
      _relatedIDMap.clear() ;
      _deadlockResList.clear() ;
   }


   INT32  _rtnDetectDeadlock::pushIn( const BSONObj & obj )
   {
      return _parseAndSaveWaitInfo( _dbRole, obj, _waitInfoSet, _relatedIDMap );
   }


   INT32  _rtnDetectDeadlock::output( BSONObj &obj, BOOLEAN & hasOut )
   {
      INT32 rc = SDB_OK ;
      try
      {
         if ( ! _deadlockResList.empty() )
         {
            obj = _deadlockResList.front().getOwned() ;
            _deadlockResList.pop_front() ;
            hasOut = TRUE ;
         }    
         else
         {
            hasOut = FALSE ;
            if ( _hasDone )
            {
               _eof = TRUE ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR,
                 "Exception captured:%s when output deadlock result",
                 e.what() ) ;
      }
      return rc ;
   }


   INT32  _rtnDetectDeadlock::done( BOOLEAN & hasOut )
   {
      INT32 rc = SDB_OK ;
      dpsDeadlockTx * pDeadlockTxArray = NULL ;

      _hasDone = TRUE ;
      hasOut   = FALSE ;

      if ( ! _waitInfoSet.empty() )
      {
         DPS_DEADLOCK_TX_SET_LIST waiterSetList ;
         deadlockDetector dl( & _waitInfoSet );
         dl.findSCC( & waiterSetList ) ;

         if ( ! waiterSetList.empty() )
         {
            // prepare a temporary array of dpsDeadlockTx for picking
            // victim transaction ( sort dpsDeadlockTx on degree field )
            UINT32 setSize = _getMaxSize( waiterSetList ) ;
            pDeadlockTxArray = (dpsDeadlockTx *)
                              SDB_THREAD_ALLOC(sizeof(dpsDeadlockTx) * setSize);
            if ( NULL == pDeadlockTxArray )
            {
               rc = SDB_OOM ;
               clearSetList( waiterSetList ) ; 
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to allocate memory to build result, rc:%d",
                         rc );
            // construct result and save
            rc = _constructResult( waiterSetList,
                                   _relatedIDMap,
                                   pDeadlockTxArray,
                                   _deadlockResList ) ;
            clearSetList( waiterSetList ) ; 
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to build deadlock trans result, rc:%d",
                         rc );
            if ( ! _deadlockResList.empty() )
            {
               hasOut = TRUE ;
            }
         }
      }
   done:
      if ( pDeadlockTxArray )
      {
         SDB_THREAD_FREE( pDeadlockTxArray ) ;
         pDeadlockTxArray = NULL ;
      }
      _waitInfoSet.clear() ;
      _relatedIDMap.clear() ;
      return rc ;
   error:
      _deadlockResList.clear();
      goto done;
   }
    
   
   BOOLEAN _rtnDetectDeadlock::eof() const
   {
      return _eof ;
   }

}  // namespace engine

