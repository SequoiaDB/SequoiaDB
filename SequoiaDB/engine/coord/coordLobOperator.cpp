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

   Source File Name = coordLobOperator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#include "coordLobOperator.hpp"
#include "rtnLob.hpp"
#include "rtnCommandDef.hpp"
#include "coordLobStream.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordCB.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordOpenLob implement
   */
   _coordOpenLob::_coordOpenLob()
   {
      const static string s_name( "LobOpen" ) ;
      setName( s_name ) ;
   }

   _coordOpenLob::~_coordOpenLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_OPENLOB_EXE, "_coordOpenLob::execute" )
   INT32 _coordOpenLob::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_OPENLOB_EXE ) ;
      SDB_ASSERT( NULL != buf, "can not be null" ) ;

      rtnLobStream *pStream = NULL ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      contextID = -1 ;

      rc = msgExtractOpenLobRequest( (const CHAR*)pMsg, &header, obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract open msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      /// pStream will free in context
      pStream = SDB_OSS_NEW _coordLobStream( _pResource, getTimeout() ) ;
      if ( !pStream )
      {
         PD_LOG( PDERROR, "Create lob stream failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnOpenLob( obj, header->flags, cb, NULL, pStream,
                       0, contextID, *buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob:%s, rc:%d",
                 obj.toString( FALSE, TRUE ).c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_OPENLOB_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordWriteLob implement
   */
   _coordWriteLob::_coordWriteLob()
   {
      const static string s_name( "WriteLob" ) ;
      setName( s_name ) ;
   }

   _coordWriteLob::~_coordWriteLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_WRITELOB_EXE, "_coordWriteLob::execute" )
   INT32 _coordWriteLob::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_WRITELOB_EXE ) ;
      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      UINT32 len = 0 ;
      INT64 offset = -1 ;
      const CHAR *data = NULL ;
      contextID = -1 ;

      rc = msgExtractWriteLobRequest( (const CHAR*)pMsg, &header, &len,
                                      &offset, &data ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, len, offset ) ;

      rc = rtnWriteLob( header->contextID, cb, len, data, offset, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_WRITELOB_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordReadLob implement
   */
   _coordReadLob::_coordReadLob()
   {
      const static string s_name( "ReadLob" ) ;
      setName( s_name ) ;
   }

   _coordReadLob::~_coordReadLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_READLOB_EXE, "_coordReadLob::execute" )
   INT32 _coordReadLob::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_READLOB_EXE ) ;
      SDB_ASSERT( NULL != buf, "can not be null" ) ;

      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      UINT32 len = 0 ;
      SINT64 offset = -1 ;
      const CHAR *data = NULL ;
      UINT32 readLen = 0 ;
      contextID = -1 ;

      rc = msgExtractReadLobRequest( (const CHAR*)pMsg, &header, &len,
                                     &offset ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld, Len:%u, Offset:%llu",
                          header->contextID, readLen, offset ) ;

      rc = rtnReadLob( header->contextID, cb, len,
                       offset, &data, readLen, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;
      }

      *buf = rtnContextBuf( data, readLen, 1 ) ;

   done:
      PD_TRACE_EXITRC( COORD_READLOB_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordLockLob implement
   */
   _coordLockLob::_coordLockLob()
   {
      const static string s_name( "LockLob" ) ;
      setName( s_name ) ;
   }

   _coordLockLob::~_coordLockLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_LOCKLOB_EXE, "_coordLockLob::execute" )
   INT32 _coordLockLob::execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_LOCKLOB_EXE ) ;
      const MsgOpLob *header = NULL ;
      INT64 offset = 0 ;
      INT64 length = -1 ;
      contextID = -1 ;

      rc = msgExtractLockLobRequest( (const CHAR*)pMsg, &header, &offset, &length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnLockLob( header->contextID, cb, offset, length, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock lob:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_LOCKLOB_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCloseLob implement
   */
   _coordCloseLob::_coordCloseLob()
   {
      const static string s_name( "CloseLob" ) ;
      setName( s_name ) ;
   }

   _coordCloseLob::~_coordCloseLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CLOSELOB_EXE, "_coordCloseLob::execute" )
   INT32 _coordCloseLob::execute( MsgHeader *pMsg,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_CLOSELOB_EXE ) ;
      const MsgOpLob *header = NULL ;
      contextID = -1 ;

      rc = msgExtractCloseLobRequest( (const CHAR*)pMsg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnCloseLob( header->contextID, cb, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( COORD_CLOSELOB_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordRemoveLob implement
   */
   _coordRemoveLob::_coordRemoveLob()
   {
      const static string s_name( "RemoveLob" ) ;
      setName( s_name ) ;
   }

   _coordRemoveLob::~_coordRemoveLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_REMOVELOB_EXE, "_coordRemoveLob::execute" )
   INT32 _coordRemoveLob::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_REMOVELOB_EXE ) ;

      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      BSONElement ele ;
      const CHAR *fullName = NULL ;
      coordLobStream stream( _pResource, getTimeout() ) ;
      contextID = -1 ;

      rc = msgExtractRemoveLobRequest( (const CHAR*)pMsg, &header,
                                       obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract remove msg:%d", rc ) ;
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"collection\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      fullName = ele.valuestr() ;

      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"oid\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      /// release operator's groupSession to improve perfermance
      _groupSession.release() ;
      /// then open stream, will init it's groupSession
      rc = stream.open( fullName,
                        ele.__oid(), SDB_LOB_MODE_REMOVE,
                        header->flags,
                        NULL,
                        cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove lob:%s, rc:%d",
                 ele.__oid().str().c_str(), rc ) ;
         goto error ;
      }

      rc = stream.truncate( 0, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "faield to remove lob pieces:%d", rc ) ;
         /// get error info
         stream.getErrorInfo( rc, cb, buf ) ;
         goto error ;
      }

      rc = stream.close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove lob:%d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_REMOVELOB_EXE, rc ) ;
      return rc ;
   error:
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = stream.closeWithException( cb ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to close lob with exception:%d", rcTmp ) ;
         }
      }
      goto done ;
   }

   /*
      _coordTruncateLob implement
   */
   _coordTruncateLob::_coordTruncateLob()
   {
      const static string s_name( "TruncateLob" ) ;
      setName( s_name ) ;
   }

   _coordTruncateLob::~_coordTruncateLob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_TRUNCATELOB_EXE, "_coordTruncateLob::execute" )
   INT32 _coordTruncateLob::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_TRUNCATELOB_EXE ) ;

      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      BSONElement ele ;
      string fullName ;
      bson::OID oid ;
      INT64 length = 0 ;
      coordLobStream stream( _pResource, getTimeout() ) ;
      contextID = -1 ;

      rc = msgExtractTruncateLobRequest( (const CHAR*)pMsg, &header,
                                         obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract truncate msg:%d", rc ) ;
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"Collection\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      fullName = ele.String() ;

      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"Oid\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      oid = ele.OID() ;

      ele = obj.getField( FIELD_NAME_LOB_LENGTH ) ;
      if ( NumberLong != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"Length\":%s",
                 obj.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      length = ele.numberLong() ;

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      /// release operator's groupSession to improve perfermance
      _groupSession.release() ;
      /// then open stream, will init it's groupSession
      rc = stream.open( fullName.c_str(),
                        oid, SDB_LOB_MODE_TRUNCATE,
                        header->flags,
                        NULL,
                        cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate lob:%s, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      rc = stream.truncate( length, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "faield to truncate lob:%d", rc ) ;
         /// get error info
         stream.getErrorInfo( rc, cb, buf ) ;
         goto error ;
      }

      rc = stream.close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to truncate lob:%d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( COORD_TRUNCATELOB_EXE, rc ) ;
      return rc ;
   error:
      {
         INT32 rcTmp = SDB_OK ;
         rcTmp = stream.closeWithException( cb ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to close lob with exception:%d", rcTmp ) ;
         }
      }
      goto done ;
   }

   /*
      _coordGetLobRTDetail
   */
   _coordGetLobRTDetail::_coordGetLobRTDetail()
   {
      const static string s_name( "GetLobRTDetail" ) ;
      setName( s_name ) ;
   }

   _coordGetLobRTDetail::~_coordGetLobRTDetail()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_GETLOBRTDETAIL_EXE, "_coordGetLobRTDetail::execute" )
   INT32 _coordGetLobRTDetail::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_GETLOBRTDETAIL_EXE ) ;
      const MsgOpLob *header = NULL ;
      contextID = -1 ;

      rc = msgExtractGetLobRTDetailRequest( (const CHAR*)pMsg, &header ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extract msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "ContextID:%lld", header->contextID ) ;

      rc = rtnGetLobRTDetail( header->contextID, cb, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob detail:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_GETLOBRTDETAIL_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCreateLobID implement
   */
   _coordCreateLobID::_coordCreateLobID()
   {
   }

   _coordCreateLobID::~_coordCreateLobID()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATELOBID_EXE, "_coordCreateLobID::execute" )
   INT32 _coordCreateLobID::execute( MsgHeader *pMsg, pmdEDUCB *cb,
                                     INT64 &contextID, rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_CREATELOBID_EXE ) ;

      const MsgOpLob *header = NULL ;
      BSONObj obj ;
      bson::OID oid ;
      contextID = -1 ;

      rc = msgExtractCreateLobIDRequest( (const CHAR*)pMsg, &header,
                                         obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to extract create LobID msg:%d", rc ) ;
         goto error ;
      }

      // add last op info
      MON_SAVE_OP_DETAIL( cb->getMonAppCB(), pMsg->opCode,
                          "Option:%s", obj.toString().c_str() ) ;

      rc = rtnCreateLobID( obj, oid ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to create lobID:rc=%d", rc ) ;
         goto error ;
      }

      if ( NULL != buf )
      {
         try
         {
            BSONObjBuilder builder ;
            builder.appendOID( FIELD_NAME_LOB_OID, &oid ) ;
            *buf = builder.obj() ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to init Object id:exception=%s,rc=%d",
                  e.what(), rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORD_CREATELOBID_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

