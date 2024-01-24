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

   Source File Name = coordOmProxy.cpp

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

#include "coordOmProxy.hpp"
#include "coordOmOperator.hpp"
#include "pd.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordOmProxy implement
   */
   _coordOmProxy::_coordOmProxy()
   {
      _pResource = NULL ;
      _oprTimeout = -1 ;
   }

   _coordOmProxy::~_coordOmProxy()
   {
   }

   INT32 _coordOmProxy::init( _coordResource *pResource )
   {
      _pResource = pResource ;

      return SDB_OK ;
   }

   INT32 _coordOmProxy::queryOnOm( const rtnQueryOptions &options,
                                   pmdEDUCB *cb,
                                   SINT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOm( options, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmProxy::queryOnOm( MsgHeader *pMsg,
                                   INT32 requestType,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOm( pMsg, requestType, cb, contextID, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmProxy::queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                               pmdEDUCB *cb,
                                               vector< BSONObj > &objs,
                                               rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      coordOmOperatorBase omOpr ;

      rc = omOpr.init( _pResource, cb, _oprTimeout ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init om operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = omOpr.queryOnOmAndPushToVec( options, cb, objs, buf ) ;
      if ( rc )
      {
         goto error ;         
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _coordOmProxy::setOprTimeout( INT64 timeout )
   {
      _oprTimeout = timeout ;
   }

}


