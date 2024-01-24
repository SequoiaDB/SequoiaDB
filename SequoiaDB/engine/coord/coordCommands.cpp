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

   Source File Name = coordCommands.cpp

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

#include "coordCommands.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSetSessionAttr implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSetSessionAttr,
                                      CMD_NAME_SETSESS_ATTR,
                                      TRUE ) ;
   _coordCMDSetSessionAttr::_coordCMDSetSessionAttr()
   {
   }

   _coordCMDSetSessionAttr::~_coordCMDSetSessionAttr()
   {
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_SETSESSIONATTR_EXE, "_coordCMDSetSessionAttr::execute" )
   INT32 _coordCMDSetSessionAttr::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_SETSESSIONATTR_EXE ) ;

      coordSessionPropSite *pPropSite = NULL ;
      pmdRemoteSessionSite *pSite = NULL ;
      const CHAR *pQuery = NULL ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL );
      PD_RC_CHECK( rc, PDERROR, "Failed to parse set session attribute "
                   "request, rc: %d", rc ) ;

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( pSite )
      {
         pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;
      }

      PD_CHECK( NULL != pPropSite, SDB_SYS, error, PDERROR,
                "Session's prop site is NULL" ) ;

      try
      {
         BSONObj property( pQuery ) ;
         rc = pPropSite->parseProperty( property ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse session property, "
                      "rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to set sessionAttr, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done :
      // fill default-reply(delete success)
      contextID = -1 ;

      PD_TRACE_EXITRC ( COORD_SETSESSIONATTR_EXE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   /*
      _coordCMDGetSessionAttr implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDGetSessionAttr,
                                      CMD_NAME_GETSESS_ATTR,
                                      TRUE ) ;
   _coordCMDGetSessionAttr::_coordCMDGetSessionAttr ()
   {
   }

   _coordCMDGetSessionAttr::~_coordCMDGetSessionAttr ()
   {
   }

   //PD_TRACE_DECLARE_FUNCTION( COORD_GETSESSIONATTR_EXE, "_coordCMDGetSessionAttr::execute" )
   INT32 _coordCMDGetSessionAttr::execute ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_GETSESSIONATTR_EXE ) ;

      coordSessionPropSite *pPropSite = NULL ;
      pmdRemoteSessionSite *pSite = NULL ;

      pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
      if ( pSite )
      {
         pPropSite = ( coordSessionPropSite* )pSite->getUserData() ;
      }
      PD_CHECK( NULL != pPropSite, SDB_SYS, error, PDERROR,
                "Session's prop site is NULL" ) ;

      ( *buf ) = rtnContextBuf( pPropSite->toBSON() ) ;

   done :
      // fill default-reply(delete success)
      contextID = -1 ;

      PD_TRACE_EXITRC ( COORD_GETSESSIONATTR_EXE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

}
