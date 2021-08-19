/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = coordCommandProcedure.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandProcedure.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtnContextSP.hpp"
#include "utilCommon.hpp"
#include "spdSession.hpp"
#include "spdCoordDownloader.hpp"
#include "msgMessage.hpp"
#include "fmpDef.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDCrtProcedure implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCrtProcedure,
                                      CMD_NAME_CRT_PROCEDURE,
                                      FALSE ) ;
   _coordCMDCrtProcedure::_coordCMDCrtProcedure()
   {
   }

   _coordCMDCrtProcedure::~_coordCMDCrtProcedure()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CRTPROCEDURE_EXE, "_coordCMDCrtProcedure::execute" )
   INT32 _coordCMDCrtProcedure::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_CRTPROCEDURE_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_CRT_PROCEDURES_REQ ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to crt procedures, rc = %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_CRTPROCEDURE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDEval implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDEval, CMD_NAME_EVAL, TRUE ) ;
   _coordCMDEval::_coordCMDEval()
   {
   }

   _coordCMDEval::~_coordCMDEval()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_EVAL_EXE, "_coordCMDEval::execute" )
   INT32 _coordCMDEval::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_EVAL_EXE ) ;
      spdSession *session = NULL ;
      contextID           = -1 ;

      CHAR *pQuery = NULL ;
      BSONObj procedures ;
      spdCoordDownloader downloader( this, cb ) ;
      BSONObj runInfo ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery, NULL,
                            NULL, NULL );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract eval msg:%d", rc) ;
         goto error ;
      }

      try
      {
         procedures = BSONObj( pQuery ) ;
         PD_LOG( PDDEBUG, "eval:%s", procedures.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      session = SDB_OSS_NEW _spdSession() ;
      if ( NULL == session )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = session->eval( procedures, &downloader, cb ) ;
      if ( SDB_OK != rc )
      {
         const BSONObj &errmsg = session->getErrMsg() ;
         if ( !errmsg.isEmpty() )
         {
            *buf = rtnContextBuf( errmsg.getOwned() ) ;
         }
         PD_LOG( PDERROR, "failed to eval store procedure:%d", rc ) ;
         goto error ;
      }

      if ( FMP_RES_TYPE_VOID != session->resType() )
      {
         rc = _buildContext( session, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to prepare reply msg:%d", rc ) ;
            goto error ;
         }
      }

      runInfo = BSON( FIELD_NAME_RTYPE << session->resType() ) ;
      *buf = rtnContextBuf( runInfo ) ;

   done:
      /// when -1 != contextID, session will be freed
      /// in context destructor.
      if ( -1 == contextID )
      {
         SAFE_OSS_DELETE( session ) ;
      }
      PD_TRACE_EXITRC( COORD_EVAL_EXE, rc ) ;
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDEval::_buildContext( _spdSession *session,
                                       pmdEDUCB *cb,
                                       SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      const BSONObj &evalRes = session->getRetMsg() ;
      SDB_ASSERT( !evalRes.isEmpty(), "impossible" ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnContextSP *context = NULL ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_SP, (rtnContext**)&context,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new context, rc: %d", rc ) ;

      rc = context->open( session ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   context->contextID(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDRmProcedure implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRmProcedure,
                                      CMD_NAME_RM_PROCEDURE,
                                      FALSE ) ;
   _coordCMDRmProcedure::_coordCMDRmProcedure()
   {
   }

   _coordCMDRmProcedure::~_coordCMDRmProcedure()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_RMPROCEDURE_EXE, "_coordCMDRmProcedure::execute" )
   INT32 _coordCMDRmProcedure::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_RMPROCEDURE_EXE ) ;
      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_RM_PROCEDURES_REQ ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to rm procedures, rc = %d",
                  rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_RMPROCEDURE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

