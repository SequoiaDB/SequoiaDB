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

   Source File Name = rtnLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLob.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsLobDef.hpp"
#include "pd.hpp"
#include "rtnContextLob.hpp"
#include "rtnTrace.hpp"
#include "rtnLocalLobStream.hpp"

using namespace bson ;

namespace engine
{

   class _rtnLobEnv : public SDBObject
   {
      public :
         _rtnLobEnv( const CHAR *fullName, _pmdEDUCB *cb,
                     dmsStorageUnit *su = NULL,
                     dmsMBContext *mbContext = NULL ) ;
         ~_rtnLobEnv() ;

         INT32 prepareOpr( INT32 lockType, BOOLEAN isWrite = TRUE ) ;
         INT32 prepareRead() ;

         void  oprDone() ;

         dmsStorageUnit    *getSU() { return _su ; }
         dmsMBContext      *getMBContext() { return _mbContext ; }

      private:
         const CHAR              *_fullName ;
         _pmdEDUCB               *_pEDUCB ;

         SDB_DMSCB               *_dmsCB ;
         dmsStorageUnit          *_su ;
         dmsStorageUnitID        _suID ;
         dmsMBContext            *_mbContext ;

         BOOLEAN                 _lockedDMS ;
         BOOLEAN                 _ownSU ;
         BOOLEAN                 _ownMB ;

   } ;
   typedef _rtnLobEnv rtnLobEnv ;

   _rtnLobEnv::_rtnLobEnv( const CHAR *fullName, _pmdEDUCB *cb,
                           dmsStorageUnit *su, dmsMBContext *mbContext )
   {
      _fullName   = fullName ;
      _pEDUCB     = cb ;
      _dmsCB      = NULL ;
      _su         = NULL ;
      _suID       = DMS_INVALID_CS ;
      _mbContext  = NULL ;

      _lockedDMS  = FALSE ;
      _ownSU      = FALSE ;
      _ownMB      = FALSE ;

      if ( su )
      {
         _su = su ;
         if ( mbContext )
         {
            _mbContext = mbContext ;
         }
      }
   }

   _rtnLobEnv::~_rtnLobEnv()
   {
      oprDone() ;
   }

   INT32 _rtnLobEnv::prepareOpr( INT32 lockType, BOOLEAN isWrite )
   {
      INT32 rc = SDB_OK ;
      const CHAR *clName = NULL ;
      _dmsCB   = sdbGetDMSCB() ;

      oprDone() ;

      if ( isWrite )
      {
         rc = _dmsCB->writable( _pEDUCB ) ;
         if ( SDB_OK !=rc )
         {
            PD_LOG ( PDERROR, "database is not writable, rc = %d", rc ) ;
            goto error ;
         }
         _lockedDMS = TRUE ;
      }

      if ( NULL == _su )
      {
         _ownSU = TRUE ;
         rc = rtnResolveCollectionNameAndLock( _fullName, _dmsCB,
                                               &_su, &clName, _suID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to resolve collection:%s, rc:%d",
                    _fullName, rc ) ;
            goto error ;
         }
      }
      else
      {
         _suID = _su->CSID() ;
      }

      if ( NULL == _mbContext )
      {
         _ownMB = TRUE ;
         rc = _su->data()->getMBContext( &_mbContext, clName, lockType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to resolve collection name:%s, rc:%d",
                    clName, rc ) ;
            goto error ;
         }
      }
      else if ( -1 != lockType )
      {
         rc = _mbContext->mbLock( lockType ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to lock collection context, rc: %d",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      oprDone() ;
      goto done ;
   }

   void _rtnLobEnv::oprDone()
   {
      if ( _mbContext && _su )
      {
         if ( _ownMB )
         {
            _su->data()->releaseMBContext( _mbContext ) ;
            _mbContext = NULL ;
            _ownMB = FALSE ;
         }
         else
         {
            _mbContext->pause() ;
         }
      }
      if ( DMS_INVALID_CS != _suID && _dmsCB )
      {
         if ( _ownSU )
         {
            _dmsCB->suUnlock( _suID, SHARED ) ;
            _su = NULL ;
            _suID = DMS_INVALID_CS ;
            _ownSU = FALSE ;
         }
      }
      if ( _lockedDMS )
      {
         _dmsCB->writeDown( _pEDUCB ) ;
         _lockedDMS = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNOPENLOB, "rtnOpenLob" )
   INT32 rtnOpenLob( const BSONObj &lob,
                     SINT32 flags,
                     _pmdEDUCB *cb,
                     SDB_DPSCB *dpsCB,
                     _rtnLobStream *pStream,
                     SINT16 w,
                     SINT64 &contextID,
                     rtnContextBuf &buffObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNOPENLOB ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_LOB,
                              (rtnContext**)(&lobContext),
                              contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob context, rc:%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( NULL != lobContext, "can not be null" ) ;
      rc = lobContext->open( lob, flags, cb, dpsCB, pStream ) ;
      /// when called open function, the pStream has been take over
      pStream = NULL ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob context, rc:%d", rc ) ;
         lobContext->getErrorInfo( rc, cb, buffObj ) ;
         goto error ;
      }

      /// get data
      rc = lobContext->getMore( -1, buffObj, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get more from context, rc: %d", rc ) ;
         lobContext->getErrorInfo( rc, cb, buffObj ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      if ( pStream )
      {
         SDB_OSS_DEL pStream ;
      }
      PD_TRACE_EXITRC( SDB_RTNOPENLOB, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREADLOB, "rtnReadLob" )
   INT32 rtnReadLob( SINT64 contextID,
                     pmdEDUCB *cb,
                     UINT32 len,
                     SINT64 offset,
                     const CHAR **buf,
                     UINT32 &read,
                     rtnContextBuf *errBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNREADLOB ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContextBuf contextBuf ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;
      rc = lobContext->read( len, offset, cb ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_EOF != rc )
         {
            PD_LOG( PDERROR, "Failed to read lob, rc:%d", rc ) ;
            if ( errBuf )
            {
               lobContext->getErrorInfo( rc, cb, *errBuf ) ;
            }
         }

         goto error ;
      }

      rc = lobContext->getMore( -1, contextBuf, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get more from context, rc:%d", rc ) ;
         if ( errBuf )
         {
            lobContext->getErrorInfo( rc, cb, *errBuf ) ;
         }
         goto error ;
      }

      *buf = contextBuf.data() ;
      read = contextBuf.size() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNREADLOB, rc ) ;
      return rc ;
   error:
      if ( SDB_EOF != rc && context )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNWRITELOB, "rtnWriteLob" )
   INT32 rtnWriteLob( SINT64 contextID,
                      pmdEDUCB *cb,
                      UINT32 len,
                      const CHAR *buf,
                      INT64 lobOffset,
                      rtnContextBuf *errBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNWRITELOB ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( lobOffset < -1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid LOB offset:%d", lobOffset ) ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;
      rc = lobContext->write( len, buf, lobOffset, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write lob, rc: %d", rc ) ;
         if ( errBuf )
         {
            lobContext->getErrorInfo( rc, cb, *errBuf ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNWRITELOB, rc ) ;
      return rc ;
   error:
      if ( -1 != contextID && context )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCKLOB, "rtnLockLob" )
   INT32 rtnLockLob( SINT64 contextID,
                     pmdEDUCB *cb,
                     INT64 offset,
                     INT64 length,
                     rtnContextBuf *errBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCKLOB ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( offset < 0 || length < -1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid LOB section(offset:%lld, length:%lld)",
                 offset, length ) ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;
      rc = lobContext->lock( cb, offset, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to lock lob, rc:%d", rc ) ;
         if ( errBuf )
         {
            lobContext->getErrorInfo( rc, cb, *errBuf ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCKLOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETLOBRTDETAIL, "rtnGetLobRTDetail" )
   INT32 rtnGetLobRTDetail( SINT64 contextID, pmdEDUCB *cb,
                            rtnContextBuf *bufObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNGETLOBRTDETAIL ) ;
      rtnContextLob *lobContext = NULL ;
      BSONObj detail ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;
      rc = lobContext->getRTDetail( cb, detail ) ;
      if ( SDB_OK != rc )
      {
         if ( NULL != bufObj )
         {
            lobContext->getErrorInfo( rc, cb, *bufObj ) ;
         }
         PD_LOG( PDERROR, "Failed to get lob runtime detail, rc:%d", rc ) ;
         goto error ;
      }

      if ( NULL != bufObj )
      {
         *bufObj = detail ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNGETLOBRTDETAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCLOSELOB, "rtnCloseLob" )
   INT32 rtnCloseLob( SINT64 contextID,
                      pmdEDUCB *cb,
                      rtnContextBuf *bufObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCLOSELOB ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         /// context has been closed.
         goto done ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;

      rc = lobContext->close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close lob, rc:%d", rc ) ;
         if ( bufObj )
         {
            lobContext->getErrorInfo( rc, cb, *bufObj ) ;
         }
         goto error ;
      }

      if ( NULL != bufObj )
      {
         INT32 mode = lobContext->mode() ;
         if ( SDB_LOB_MODE_CREATEONLY == mode || SDB_HAS_LOBWRITE_MODE( mode ) )
         {
            BSONObj meta ;
            rc = lobContext->getLobMetaData( meta ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get lob meta data, rc:%d", rc ) ;
               goto error ;
            }

            *bufObj = meta ;
         }
      }

   done:
      if ( context )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCLOSELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREMOVELOB, "rtnRemoveLob" )
   INT32 rtnRemoveLob( const BSONObj &meta,
                       INT32 flags,
                       SINT16 w,
                       _pmdEDUCB *cb,
                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNREMOVELOB ) ;
      _rtnLocalLobStream stream ;
      BSONElement fullName ;
      BSONElement oidEle ;
      bson::OID oid ;

      fullName = meta.getField( FIELD_NAME_COLLECTION ) ;
      if ( bson::String != fullName.type() )
      {
         PD_LOG( PDERROR, "Invalid type of full name:%s",
                 meta.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      oidEle = meta.getField( FIELD_NAME_LOB_OID ) ;
      if ( bson::jstOID != oidEle.type() )
      {
         PD_LOG( PDERROR, "Invalid type of full oid:%s",
                 meta.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      oid = oidEle.OID() ;

      stream.setDPSCB( dpsCB ) ;

      rc = stream.open( fullName.valuestr(),
                        oid, SDB_LOB_MODE_REMOVE,
                        flags, NULL, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to remove lob:%s, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }
      else
      {
         /// do nothing.
      }

      rc = stream.truncate( 0, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Faield to truncate lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = stream.close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to remove lob, rc:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNREMOVELOB, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNTRUNCATELOB, "rtnTruncateLob" )
   INT32 rtnTruncateLob( const BSONObj &meta,
                         INT32 flags,
                         SINT16 w,
                         _pmdEDUCB *cb,
                         SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNTRUNCATELOB ) ;
      _rtnLocalLobStream stream ;
      BSONElement ele ;
      string fullName ;
      bson::OID oid ;
      INT64 length = 0 ;

      ele = meta.getField( FIELD_NAME_COLLECTION ) ;
      if ( bson::String != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid type of full name:%s",
                 meta.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      fullName = ele.String() ;

      ele = meta.getField( FIELD_NAME_LOB_OID ) ;
      if ( bson::jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid type of full oid:%s",
                 meta.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      oid = ele.OID() ;

      ele = meta.getField( FIELD_NAME_LOB_LENGTH ) ;
      if ( bson::NumberLong != ele.type() )
      {
         PD_LOG( PDERROR, "invalid type of field \"Length\":%s",
                 meta.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      length = ele.numberLong() ;

      stream.setDPSCB( dpsCB ) ;

      rc = stream.open( fullName.c_str(),
                        oid, SDB_LOB_MODE_TRUNCATE,
                        flags, NULL, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to truncate lob:%s, rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      rc = stream.truncate( length, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Faield to truncate lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = stream.close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to truncate lob, rc:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNTRUNCATELOB, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETLOBMETADATA, "rtnGetLobMetaData" )
   INT32 rtnGetLobMetaData( SINT64 contextID,
                            pmdEDUCB *cb,
                            BSONObj &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNGETLOBMETADATA ) ;
      rtnContextLob *lobContext = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContext *context = rtnCB->contextFind ( contextID, cb ) ;
      if ( NULL == context )
      {
         PD_LOG ( PDERROR, "Context %lld does not exist", contextID ) ;
         rc = SDB_RTN_CONTEXT_NOTEXIST ;
         goto error ;
      }

      if ( RTN_CONTEXT_LOB != context->getType() )
      {
         PD_LOG( PDERROR, "It is not a lob context, invalid context type:%d"
                 ", contextID:%lld", context->getType(), contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      lobContext = ( rtnContextLob * )context ;
      rc = lobContext->getLobMetaData( meta ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_EOF != rc )
         {
            PD_LOG( PDERROR, "Failed to get lob meta data, rc:%d", rc ) ;
         }

         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNGETLOBMETADATA, rc ) ;
      return rc ;
   error:
      if ( context )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCREATELOB, "rtnCreateLob" )
   INT32 rtnCreateLob( const CHAR *fullName,
                       const bson::OID &oid,
                       _dmsLobMeta &meta,
                       pmdEDUCB *cb,
                       SINT16 w,
                       SDB_DPSCB *dpsCB,
                       dmsStorageUnit *su,
                       dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCREATELOB ) ;
      SDB_ASSERT( NULL != fullName && NULL != cb, "can not be null" ) ;
      _dmsLobMeta tmpMeta ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( EXCLUSIVE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = lobEnv.getSU()->lob()->getLobMeta( oid, lobEnv.getMBContext(),
                                              cb, tmpMeta ) ;
      if ( SDB_FNE == rc )
      {
         /// do nothing.
         rc = SDB_OK ;
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get meta data of lob, rc:%d", rc ) ;
         goto error ;
      }
      else if ( tmpMeta.isDone() )
      {
         PD_LOG( PDERROR, "Lob[%s] exists", oid.str().c_str() ) ;
         rc = SDB_FE ;
         goto error ;
      }
      else
      {
         PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                 oid.str().c_str(), tmpMeta.toString().c_str() ) ;
         rc = SDB_LOB_IS_NOT_AVAILABLE ;
         goto error ;
      }

      rc = lobEnv.getSU()->lob()->writeLobMeta( oid, lobEnv.getMBContext(),
                                                cb, meta, TRUE, dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write lob meta data, rc:%d", rc ) ;
         goto error ;
      }

      lobEnv.oprDone() ;
      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCREATELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNWRITELOB2, "rtnWriteLob" )
   INT32 rtnWriteLob( const CHAR *fullName,
                      const bson::OID &oid,
                      UINT32 sequence,
                      UINT32 offset,
                      UINT32 len,
                      const CHAR *data,
                      pmdEDUCB *cb,
                      SINT16 w,
                      SDB_DPSCB *dpsCB,
                      dmsStorageUnit *su,
                      dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNWRITELOB2 ) ;
      SDB_ASSERT( NULL != fullName && NULL != cb, "can not be null" ) ;
      _dmsLobRecord record ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      record.set( &oid, sequence, offset, len, data ) ;
      rc = lobEnv.getSU()->lob()->write( record, lobEnv.getMBContext(),
                                         cb, dpsCB ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write lob, rc:%d", rc ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNWRITELOB2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNWRITEORUPDATELOB, "rtnWriteOrUpdateLob" )
   INT32 rtnWriteOrUpdateLob( const CHAR *fullName,
                              const bson::OID &oid,
                              UINT32 sequence,
                              UINT32 offset,
                              UINT32 len,
                              const CHAR *data,
                              pmdEDUCB *cb,
                              SINT16 w,
                              SDB_DPSCB *dpsCB,
                              dmsStorageUnit *su,
                              dmsMBContext *mbContext,
                              BOOLEAN* hasUpdated )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNWRITEORUPDATELOB ) ;
      SDB_ASSERT( NULL != fullName && NULL != cb, "can not be null" ) ;
      _dmsLobRecord record ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write or update lob, rc:%d", rc ) ;
         goto error ;
      }

      record.set( &oid, sequence, offset, len, data ) ;
      rc = lobEnv.getSU()->lob()->writeOrUpdate( record, lobEnv.getMBContext(),
                                                 cb, dpsCB, hasUpdated ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write or update lob, rc:%d", rc ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNWRITEORUPDATELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCLOSELOB2, "rtnCloseLob" )
   INT32 rtnCloseLob( const CHAR *fullName,
                      const bson::OID &oid,
                      const dmsLobMeta &meta,
                      pmdEDUCB *cb,
                      SINT16 w,
                      SDB_DPSCB *dpsCB,
                      dmsStorageUnit *su,
                      dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCLOSELOB2 ) ;
      SDB_ASSERT( NULL != fullName && NULL != cb, "can not be null" ) ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = lobEnv.getSU()->lob()->writeLobMeta( oid, lobEnv.getMBContext(),
                                                cb, meta, FALSE, dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to write meta data of lob, rc:%d", rc ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCLOSELOB2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNGETLOBMETADATA2, "rtnGetLobMetaData" )
   INT32 rtnGetLobMetaData( const CHAR *fullName,
                            const bson::OID &oid,
                            pmdEDUCB *cb,
                            dmsLobMeta &meta,
                            dmsStorageUnit *su,
                            dmsMBContext *mbContext,
                            BOOLEAN allowUncompleted )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNGETLOBMETADATA2 ) ;
      _dmsLobRecord record ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to read lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = lobEnv.getSU()->lob()->getLobMeta( oid, lobEnv.getMBContext(),
                                              cb, meta ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to read lob[%s] in collection[%s], rc:%d",
                    oid.str().c_str(), fullName, rc ) ;
         }
         goto error ;
      }

      if ( !meta.isDone() && !allowUncompleted )
      {
         PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                 oid.str().c_str(), meta.toString().c_str() ) ;
         rc = SDB_LOB_IS_NOT_AVAILABLE ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNGETLOBMETADATA2, rc ) ;
      return rc ;
   error:
      meta.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREADLOB2, "rtnReadLob" )
   INT32 rtnReadLob( const CHAR *fullName,
                     const bson::OID &oid,
                     UINT32 sequence,
                     UINT32 offset,
                     UINT32 len,
                     pmdEDUCB *cb,
                     CHAR *data,
                     UINT32 &read,
                     dmsStorageUnit *su,
                     dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNREADLOB2 ) ;
      _dmsLobRecord record ;
      const CHAR *np = NULL ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to read lob, rc:%d", rc ) ;
         goto error ;
      }

      record.set( &oid, sequence, offset, len, np ) ;
      rc = lobEnv.getSU()->lob()->read( record, lobEnv.getMBContext(), cb,
                                        data, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read lob, rc:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNREADLOB2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREMOVELOBPIECE, "rtnRemoveLobPiece" )
   INT32 rtnRemoveLobPiece( const CHAR *fullName,
                            const bson::OID &oid,
                            UINT32 sequence,
                            pmdEDUCB *cb,
                            SINT16 w,
                            SDB_DPSCB *dpsCB,
                            dmsStorageUnit *su,
                            dmsMBContext *mbContext,
                            BOOLEAN onlyRemoveNewPiece )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNREMOVELOBPIECE ) ;
      _dmsLobRecord record ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      record.set( &oid, sequence, 0, 0, NULL ) ;
      rc = lobEnv.getSU()->lob()->remove( record, lobEnv.getMBContext(), cb,
                                          dpsCB, onlyRemoveNewPiece ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to remove lob[%s],"
                 "sequence:%d, rc:%d", oid.str().c_str(),
                 sequence, rc ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNREMOVELOBPIECE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNQUERYANDINVALIDAGELOB, "rtnQueryAndInvalidateLob" )
   INT32 rtnQueryAndInvalidateLob( const CHAR *fullName,
                                   const bson::OID &oid,
                                   pmdEDUCB *cb,
                                   SINT16 w,
                                   SDB_DPSCB *dpsCB,
                                   dmsLobMeta &meta,
                                   dmsStorageUnit *su,
                                   dmsMBContext *mbContext,
                                   BOOLEAN allowUncompleted )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNQUERYANDINVALIDAGELOB ) ;
      dmsLobMeta lobMeta ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( EXCLUSIVE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      rc = lobEnv.getSU()->lob()->getLobMeta( oid, lobEnv.getMBContext(), cb,
                                              lobMeta ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob meta[%s], rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      if ( !lobMeta.isDone() )
      {
         if ( allowUncompleted )
         {
            goto done ;
         }
         else
         {
            PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                    oid.str().c_str(), meta.toString().c_str() ) ;
            rc = SDB_LOB_IS_NOT_AVAILABLE ;
            goto error ;
         }
      }

      meta = lobMeta ;
      lobMeta._status = DMS_LOB_UNCOMPLETE ;

      rc = lobEnv.getSU()->lob()->writeLobMeta( oid, lobEnv.getMBContext(), cb,
                                                lobMeta, FALSE, dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to invalidate lob[%s], rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNQUERYANDINVALIDAGELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNUPDATELOB, "rtnUpdateLob" )
   INT32 rtnUpdateLob( const CHAR *fullName,
                       const bson::OID &oid,
                       UINT32 sequence,
                       UINT32 offset,
                       UINT32 len,
                       const CHAR *data,
                       pmdEDUCB *cb,
                       SINT16 w,
                       SDB_DPSCB *dpsCB,
                       dmsStorageUnit *su,
                       dmsMBContext *mbContext  )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNUPDATELOB ) ;
      SDB_ASSERT( NULL != fullName && NULL != cb, "can not be null" ) ;
      _dmsLobRecord record ;
      rtnLobEnv lobEnv( fullName, cb, su, mbContext ) ;

      rc = lobEnv.prepareOpr( -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to write lob, rc:%d", rc ) ;
         goto error ;
      }

      record.set( &oid, sequence, offset,
                  len, data ) ;

      rc = lobEnv.getSU()->lob()->update( record, lobEnv.getMBContext(), cb,
                                          dpsCB ) ;

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update lob, rc:%d", rc ) ;
         goto error ;
      }

      if ( NULL != dpsCB )
      {
         dpsCB->completeOpr( cb, w ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNUPDATELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 rtnCreateLobID( const BSONObj &createLobIDObj, bson::OID &oid )
   {
      INT32 rc = SDB_OK ;

      BSONObj obj ;
      BSONElement ele ;
      time_t seconds = 0 ;
      time_t utcTime ;
      _utilLobID lobID ;
      BYTE oidArray[UTIL_LOBID_ARRAY_LEN] ;
      _MsgRouteID routeId = pmdGetNodeID() ;

      if ( MSG_INVALID_ROUTEID == routeId.value )
      {
         rc = SDB_INVALID_ROUTEID ;
         PD_LOG( PDERROR, "Route id must be exist when create lob ID:rc=%d",
                 rc ) ;
         goto error ;
      }

      if ( createLobIDObj.isEmpty() )
      {
         seconds = ossGetCurrentMilliseconds() / 1000 ;
      }
      else
      {
         ossTimestamp timestamp ;
         INT32 parseNum = 0 ;
         ele = createLobIDObj.getField( FIELD_NAME_LOB_CREATETIME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Invalid type of field[%s]:obj=%s",
                    FIELD_NAME_LOB_CREATETIME,
                    createLobIDObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         //timestamp format:YYYY-MM-DD-HH.mm.ss  6 numbers
         ossStringToTimestamp( ele.valuestr(), timestamp, parseNum ) ;
         if ( parseNum < 6 )
         {
            PD_LOG( PDERROR, "Invalid field[%s]:obj=%s,parseNum=%d",
                    FIELD_NAME_LOB_CREATETIME,
                    createLobIDObj.toString( FALSE, TRUE ).c_str(), parseNum ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         seconds = timestamp.time ;
      }

      ossTimeLocalToUTCInSameDate( seconds, utcTime ) ;
      rc = lobID.init( (INT64)utcTime, routeId.columns.nodeID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init lobID:rc=%d", rc ) ;
         goto error ;
      }

      rc = lobID.toByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get byteArray from lobID[%s]:rc=%d",
                 lobID.toString().c_str(), rc ) ;
         goto error ;
      }

      oid.init( oidArray, UTIL_LOBID_ARRAY_LEN ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

