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

   Source File Name = coordCommandDomain.cpp

   Descriptive Name = Coord Commands for Data Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/27/2017  XJH Init
   Last Changed =

*******************************************************************************/

#include "coordCommandDomain.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "rtn.hpp"

using namespace bson;

namespace engine
{

   /*
      _coordCMDCreateDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateDomain,
                                      CMD_NAME_CREATE_DOMAIN,
                                      FALSE ) ;
   _coordCMDCreateDomain::_coordCMDCreateDomain()
   {
   }

   _coordCMDCreateDomain::~_coordCMDCreateDomain()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATEDOMAIN_EXE, "_coordCMDCreateDomain::execute" )
   INT32 _coordCMDCreateDomain::execute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATEDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg;
      forward->header.opCode = MSG_CAT_CREATE_DOMAIN_REQ ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Execute on catalog failed in command[%s], "
                  "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATEDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDDropDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDDropDomain,
                                      CMD_NAME_DROP_DOMAIN,
                                      FALSE ) ;
   _coordCMDDropDomain::_coordCMDDropDomain()
   {
   }

   _coordCMDDropDomain::~_coordCMDDropDomain()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_DROPDOMAIN_EXE, "_coordCMDDropDomain::execute" )
   INT32 _coordCMDDropDomain::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_DROPDOMAIN_EXE ) ;

      contextID = -1 ;

      MsgOpQuery *forward  = (MsgOpQuery *)pMsg ;
      forward->header.opCode = MSG_CAT_DROP_DOMAIN_REQ ;

      _printDebug ( (const CHAR*)pMsg, getName() ) ;

      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Execute on catalog failed in command[%s], "
                  "rc: %d", getName(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_DROPDOMAIN_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDAlterDomain implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterDomain,
                                      CMD_NAME_ALTER_DOMAIN,
                                      FALSE ) ;
   _coordCMDAlterDomain::_coordCMDAlterDomain ()
   : _coordDataCMDAlter()
   {
   }

   _coordCMDAlterDomain::~_coordCMDAlterDomain ()
   {
   }

}
