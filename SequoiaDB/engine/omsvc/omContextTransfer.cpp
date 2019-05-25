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

   Source File Name = omContextTransfer.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/09/2015  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omContextTransfer.hpp"
#include "msgMessage.hpp"
#include "omDef.hpp"
#include "pmdEDU.hpp"
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   RTN_CTX_AUTO_REGISTER(_omContextTransfer, RTN_CONTEXT_OM_TRANSFER, "OM_TRANSFER")

   _omContextTransfer::_omContextTransfer( INT64 contextID, UINT64 eduID )
                      :_rtnContextBase( contextID, eduID )
   {
      _conn              = NULL ;
      _originalContextID = -1 ;
   }

   _omContextTransfer::~_omContextTransfer()
   {
      if ( NULL != _conn )
      {
         _conn->close() ;
         SDB_OSS_DEL _conn ;
         _conn = NULL ;
      }
   }

   INT32 _omContextTransfer::open( omSdbConnector *conn, MsgHeader *reply )
   {
      INT32 rc           = SDB_OK ;
      SINT32 flag        = SDB_OK ;
      SINT32 startFrom   = 0 ;
      SINT32 numReturned = 0 ;
      UINT32 i           = 0;
      vector< BSONObj > objVec ;

      _conn  = conn ;

      if ( NULL == conn || NULL == reply )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "rsmManager or reply can't be null!" ) ;
         goto error ;
      }

      rc = msgExtractReply( (CHAR *)reply, &flag, &_originalContextID, 
                            &startFrom, &numReturned, objVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "extract reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         if ( SDB_DMS_EOC == flag )
         {
            rc      = SDB_OK ;
            _hitEnd = TRUE ;
            goto done ;
         }

         rc = flag ;
         PD_LOG( PDERROR, "received failure message:flag=%d", flag ) ;
         goto error ;
      }

      for ( i = 0 ; i < objVec.size() ; i++ )
      {
         rc = append( objVec[i] ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "append obj failed:rc=%d", rc ) ;
            goto error ;
         }
      }

      _isOpened = TRUE ;
      if ( ( -1 == _originalContextID ) && ( 0 == objVec.size() ) )
      {
         _hitEnd = TRUE ;
      }
      else
      {
         _hitEnd = FALSE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   std::string _omContextTransfer::name() const
   {
      return "OM_TRANSFER" ;
   }

   RTN_CONTEXT_TYPE _omContextTransfer::getType() const
   {
      return RTN_CONTEXT_OM_TRANSFER ;
   }

   _dmsStorageUnit* _omContextTransfer::getSU() 
   { 
      return NULL ; 
   }

   INT32 _omContextTransfer::_appendReply( MsgHeader *reply )
   {
      INT32 rc           = SDB_OK ;
      SINT64 contextID   = -1 ;
      SINT32 startFrom   = 0 ;
      SINT32 numReturned = 0 ;
      SINT32 flag        = 0 ;
      UINT32 i           = 0 ;
      vector<BSONObj> objVec ;

      rc = msgExtractReply( (CHAR *)reply, &flag, &contextID, &startFrom, 
                            &numReturned, objVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "extract reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( SDB_OK != flag )
      {
         if ( SDB_DMS_EOC == flag )
         {
            rc      = SDB_OK ;
            _hitEnd = TRUE ;
            goto done ;
         }

         rc = flag ;
         PD_LOG( PDERROR, "received failure message:flag=%d", flag ) ;
         goto error ;
      }

      for ( i = 0 ; i < objVec.size() ; i++ )
      {
         rc = append( objVec[i] ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "append obj failed:rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omContextTransfer::_getMoreFromRemote( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *reply = NULL ;
      UINT32 tid = cb->getTID() ;

      MsgOpGetMore msgReq ;
      msgFillGetMoreMsg( msgReq, tid, _originalContextID, -1, 0 ) ;

      rc = _conn->sendMessage( (MsgHeader *) &msgReq ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "send msg failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _conn->recvMessage( &reply ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "receive reply failed:rc=%d", rc ) ;
         goto error ;
      }

      rc = _appendReply( reply ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "append reply failed:rc=%d", rc );
         goto error ;
      }

   done:
      if ( NULL != reply )
      {
         SDB_OSS_FREE( reply ) ;
         reply = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omContextTransfer::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( !_isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         PD_LOG( PDERROR, "context is close" ) ;
         goto error ;
      }

      if ( -1 == _originalContextID )
      {
         _hitEnd = TRUE ;
         goto done ;
      }

      rc = _getMoreFromRemote( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "get more from remote failed:rc=%d", rc );
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}


