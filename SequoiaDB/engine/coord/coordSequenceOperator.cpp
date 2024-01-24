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

   Source File Name = coordSequenceOperator.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2020  LSQ Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordSequenceOperator.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordSequenceAgent.hpp"
#include "auth.hpp"

namespace engine
{

   /*
      _coordSeqFetchOperator implement
   */
   _coordSeqFetchOperator::_coordSeqFetchOperator()
   {
   }

   _coordSeqFetchOperator::~_coordSeqFetchOperator()
   {
   }

   const CHAR* _coordSeqFetchOperator::getName() const
   {
      return "SequenceFetch" ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_SEQ_FETCH_EXE, "_coordSeqFetchOperator::execute" )
   INT32 _coordSeqFetchOperator::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_SEQ_FETCH_EXE ) ;

      const CHAR *pQuery         = NULL ;
      const CHAR *sequenceName   = NULL ;
      INT32 fetchNum             = 0 ;
      INT64 nextValue            = 0 ;
      INT32 returnNum            = 0 ;
      INT32 increment            = 0 ;
      coordSequenceAgent *pSequenceAgent = _pResource->getSequenceAgent() ;
      BSONObjBuilder bob( 64 ) ;
      BSONObj result ;

      if ( cb->getSession()->privilegeCheckEnabled() )
      {
         authActionSet actions;
         actions.addAction( ACTION_TYPE_fetchSequence );
         rc = cb->getSession()->checkPrivilegesForActionsOnCluster( actions );
         PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
      }

      rc = msgExtractQuery( (const CHAR *)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query message, rc: %d", rc ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;
         sequenceName = boQuery.getField( FIELD_NAME_NAME ).valuestrsafe() ;
         fetchNum = boQuery.getField( FIELD_NAME_FETCH_NUM ).numberInt() ;

         // add last op info
         MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                             "Option:%s", boQuery.toPoolString().c_str() ) ;

         rc = pSequenceAgent->fetch( sequenceName, UTIL_SEQUENCEID_NULL,
                                     fetchNum, nextValue, returnNum, increment,
                                     cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to fetch values from sequence[%s], rc: %d",
                      sequenceName, rc ) ;

         bob.append( FIELD_NAME_NEXT_VALUE, nextValue ) ;
         bob.append( FIELD_NAME_RETURN_NUM, returnNum ) ;
         bob.append( FIELD_NAME_INCREMENT, increment ) ;
         result = bob.obj() ;

         *buf = rtnContextBuf( result ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurs: %s", e.what() ) ;
         goto error ;
      }
   done:
      contextID = -1 ;
      PD_TRACE_EXITRC ( COORD_SEQ_FETCH_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
