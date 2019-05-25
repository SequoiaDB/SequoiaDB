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

   Source File Name = coordOmOperator.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordOmOperator.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordOmOperatorBase implement
   */
   _coordOmOperatorBase::_coordOmOperatorBase()
   {
   }

   _coordOmOperatorBase::~_coordOmOperatorBase()
   {
   }

   INT32 _coordOmOperatorBase::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      return SDB_SYS ;
   }

   INT32 _coordOmOperatorBase::executeOnOm( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            BOOLEAN onPrimary,
                                            SET_RC *pIgnoreRC,
                                            rtnContextCoord **ppContext,
                                            rtnContextBuf *buf )
   {
      CoordGroupList grpList ;
      grpList[ OM_GROUPID ] = OM_GROUPID ;
      return _executeOnGroups( pMsg, cb, grpList, MSG_ROUTE_OM_SERVICE,
                               onPrimary, pIgnoreRC, NULL, ppContext,
                               buf ) ;
   }

   INT32 _coordOmOperatorBase::executeOnOm ( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             vector<BSONObj> *pReplyObjs,
                                             BOOLEAN onPrimary,
                                             SET_RC *pIgnoreRC,
                                             rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      rtnContextBuf buffObj ;
      rtnContextCoord *pContext = NULL ;

      rc = executeOnOm( pMsg, cb, onPrimary, pIgnoreRC,
                        &pContext, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command[%d] on "
                   "om, rc: %d", pMsg->opCode, rc ) ;

      while ( pContext )
      {
         rc = pContext->getMore( 1, buffObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get more from context [%lld], rc: %d",
                    pContext->contextID(), rc ) ;
            goto error ;
         }

         try
         {
            BSONObj obj( buffObj.data() ) ;

            if ( pReplyObjs )
            {
               pReplyObjs->push_back( obj.getOwned() ) ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Extrace om reply obj occur exception: %s",
                    e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      return rc ;

   error :
      goto done ;
   }

   INT32 _coordOmOperatorBase::queryOnOm( MsgHeader *pMsg,
                                          INT32 requestType,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      rtnContextCoord *pContext        = NULL ;

      contextID = -1 ;

      pMsg->opCode                     = requestType ;

      rc = executeOnOm( pMsg, cb, TRUE, NULL, &pContext, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Query[%d] on om failed, rc = %d",
                  requestType, rc ) ;
         goto error ;
      }

   done :
      if ( pContext )
      {
         contextID = pContext->contextID() ;
      }
      return rc ;
   error :
      if ( pContext )
      {
         INT64 contextID = pContext->contextID() ;
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         pContext = NULL ;
      }
      goto done ;
   }

   INT32 _coordOmOperatorBase::queryOnOm( const rtnQueryOptions &options,
                                          pmdEDUCB *cb,
                                          SINT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      CHAR *msgBuf = NULL ;
      INT32 msgBufLen = 0 ;
      contextID = -1 ;

      rc = options.toQueryMsg( &msgBuf, msgBufLen, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build query msg:%d", rc ) ;
         goto error ;
      }

      rc = queryOnOm( (MsgHeader*)msgBuf, MSG_BS_QUERY_REQ, cb,
                       contextID, buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on om failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( NULL != msgBuf )
      {
         msgReleaseBuffer( msgBuf, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmOperatorBase::queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                                      pmdEDUCB *cb,
                                                      vector< BSONObj > &objs,
                                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      rc = queryOnOm( options, cb, contextID, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to query on om: %d", rc ) ;
         goto error ;
      }

      do
      {
         rc = rtnGetMore( contextID, -1, bufObj, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            contextID = -1 ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            contextID = -1 ;
            PD_LOG( PDERROR, "Failed to getmore from context, rc: %d", rc ) ;
            goto error ;
         }
         else
         {
            while ( !bufObj.eof() )
            {
               BSONObj obj ;
               rc = bufObj.nextObj( obj ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to get obj from obj buf, rc: %d",
                          rc ) ;
                  goto error ;
               }

               objs.push_back( obj.getOwned() ) ;
            }
         }
      } while( TRUE ) ;

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

}

