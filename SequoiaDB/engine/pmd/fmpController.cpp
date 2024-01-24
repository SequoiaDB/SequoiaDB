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

   Source File Name = fmpController.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "fmpController.hpp"
#include "fmpDef.hpp"
#include "fmpVM.hpp"
#include "ossUtil.hpp"
#include "ossSocket.hpp"
#include "utilStr.hpp"
#include "fmpJSVM.hpp"
#include "pd.hpp"

using namespace bson ;

#define FMP_DEFAULT_BUF_SIZE           ( 1024 )

static INT32 FMP_STATUS_M[FMP_CONTROL_SETP_MAX][FMP_CONTROL_SETP_MAX]
       = {{FMP_CONTROL_STEP_DOWNLOAD, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID},
          {FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_DOWNLOAD, FMP_CONTROL_STEP_EVAL, FMP_CONTROL_STEP_INVALID},
          {FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_FETCH},
          {FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_INVALID, FMP_CONTROL_STEP_FETCH}} ;

#define FMP_VALID_STEP( step ) \
        (FMP_CONTROL_STEP_BEGIN <= (step) && (step) <FMP_CONTROL_SETP_MAX ? \
        (FMP_STATUS_M[_step][(step)] == FMP_CONTROL_STEP_INVALID ?\
              FALSE : TRUE )\
        : FALSE)

#define FMP_STEP_AUTO_CHANGE( step )\
        do {_step = FMP_STATUS_M[_step][(step)] ;}while(FALSE)

#define FMP_STEP_ASSIGN( step )\
        do {_step = (step) ;}while(FALSE)

const CHAR *FMP_COORD_HOST = "localhost" ;
static const CHAR magicNumber[] = FMP_MSG_MAGIC ;

BSONObj OK_RES = BSON( FMP_RES_CODE << SDB_OK ) ;

_fmpController::_fmpController()
: _vm( NULL ),
  _inBuf( NULL ),
  _inBufSize(0)
{
    FMP_STEP_ASSIGN( FMP_CONTROL_STEP_BEGIN ) ;
}

_fmpController::~_fmpController()
{
   SAFE_OSS_DELETE( _vm ) ;
   if ( NULL != _inBuf )
   {
      SDB_OSS_FREE( _inBuf ) ;
   }

   PD_LOG( PDINFO, "fmp quit" ) ;
}

INT32 _fmpController::run()
{
   INT32 rc = SDB_OK ;
   _inBuf = (CHAR *)SDB_OSS_MALLOC( FMP_DEFAULT_BUF_SIZE ) ;
   if ( NULL == _inBuf )
   {
      PD_LOG( PDERROR, "failed to allocate mem." ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   _inBufSize = FMP_DEFAULT_BUF_SIZE ;

#if defined (_WINDOWS)
   _in.hFile = (HANDLE)(_fileno( stdin )) ;
   _out.hFile = (HANDLE)_fileno( stdout ) ;
#else
   _in.fd = fileno( stdin ) ;
   _out.fd = fileno( stdout ) ;
#endif

   rc = _runLoop() ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to run loop%d", rc) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpController::_runLoop()
{
   INT32 rc = SDB_OK ;
   BSONObj obj ;
   BSONElement ele ;

   while ( TRUE )
   {
      INT32 step = FMP_CONTROL_STEP_INVALID ;
      rc = _readMsg( obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read msg:%d",rc ) ;
         goto error ;
      }

      ele = obj.getField( FMP_CONTROL_FIELD ) ;
      if ( ele.eoo() )
      {
         /// we considered it be download.
         /// so func is no need to be copy in spdCB.
         step = FMP_CONTROL_STEP_DOWNLOAD ;
      }
      else if ( NumberInt != ele.type() )
      {
         PD_LOG( PDERROR, "failed to find control filed:%s",
                 obj.toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         step = ele.Int() ;
      }

      if ( FMP_CONTROL_STEP_QUIT == step )
      {
         _clear() ;
         /// should not do anything after send ok msg.
         rc = _writeMsg( OK_RES ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write res of reset:%d", rc  ) ;
            goto error ;
         }
         break ;
      }
      else if ( FMP_CONTROL_STEP_RESET == step )
      {
         _clear() ;
         rc = _writeMsg( OK_RES ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write res of reset:%d", rc  ) ;
            goto error ;
         }
         continue ;
      }
      else
      {
         /// do noting.
      }

      if ( !FMP_VALID_STEP(step) )
      {
         PD_LOG( PDERROR, "invalid step number[%d], now step[%d]",
                 ele.Int(), _step ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _handleOneLoop( obj, step ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to handle one loop:%d", rc) ;
         _clear() ;
      }

      FMP_STEP_AUTO_CHANGE( step ) ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpController::_handleOneLoop( const BSONObj &obj,
                                      INT32 step )
{
   INT32 rc = SDB_OK ;
   BSONObj res ;

   if ( FMP_CONTROL_STEP_BEGIN == step )
   {
      UINT32 seqID = 1 ;
      BSONElement beSeq = obj.getField( FMP_SEQ_ID ) ;
      if ( beSeq.isNumber() )
      {
         seqID = (UINT32)beSeq.numberInt() ;
      }
      BSONElement diag = obj.getField( FMP_DIAG_PATH ) ;
      if ( !diag.eoo() && String == diag.type() )
      {
         CHAR diaglogShort[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
         ossSnprintf( diaglogShort, OSS_MAX_PATHSIZE, "%s_%u.%s",
                      PD_FMP_DIAGLOG_PREFIX, seqID, PD_FMP_DIAGLOG_SUBFIX ) ;

         CHAR diaglog[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
         engine::utilBuildFullPath( diag.valuestrsafe(), diaglogShort,
                                    OSS_MAX_PATHSIZE, diaglog ) ;
         sdbEnablePD( diaglog ) ;
      }
      BSONElement localService = obj.getField( FMP_LOCAL_SERVICE ) ;
      if ( String == localService.type() )
      {
         _svcname = localService.valuestrsafe() ;
      }
      BSONElement localUser = obj.getField( FMP_LOCAL_USERNAME ) ;
      if ( String == localUser.type() )
      {
         _userName = localUser.valuestrsafe() ;
      }
      BSONElement localPass = obj.getField( FMP_LOCAL_PASSWORD ) ;
      if ( String == localPass.type() )
      {
         _password = localPass.valuestrsafe() ;
      }
      BSONElement fType = obj.getField( FMP_FUNC_TYPE ) ;
      if ( fType.eoo() )
      {
         rc = _createVM( FMP_FUNC_TYPE_JS ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG(PDERROR, "failed to create vm:%d", rc ) ;
            res = BSON( FMP_ERR_MSG << "failed to create vm" <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }

         rc = _vm->init( obj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG(PDERROR, "failed to init vm:%d", rc ) ;
            res = BSON( FMP_ERR_MSG << "failed to init vm" <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }
      }
      else if ( NumberInt != fType.type() )
      {
         PD_LOG( PDERROR, "invalid type of func type:%s",
                 fType.toString().c_str() ) ;
         rc = SDB_SYS ;
         res = BSON( FMP_ERR_MSG << "invalid type of func type" <<
                     FMP_RES_CODE << SDB_SYS ) ;
         goto error ;
      }
      else
      {
         rc = _createVM( fType.Int() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG(PDERROR, "failed to create vm:%d", rc ) ;
            res = BSON( FMP_ERR_MSG << "failed to create vm" <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }

         rc = _vm->init( obj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG(PDERROR, "failed to init vm:%d", rc ) ;
            res = BSON( FMP_ERR_MSG << "failed to init vm" <<
                        FMP_RES_CODE << rc ) ;
            goto error ;
         }
      }
   }
   else if ( FMP_CONTROL_STEP_DOWNLOAD == step )
   {
      SDB_ASSERT( NULL != _vm, "impossible" ) ;
      rc = _vm->eval( obj, res ) ;
      if ( SDB_OK  != rc )
      {
         PD_LOG( PDERROR, "failed to pre eval func:%s, rc:%d",
                 obj.toString(FALSE, TRUE).c_str(), rc ) ;
         if ( res.isEmpty() )
         {
            res = BSON( FMP_ERR_MSG << "failed to pre eval func" <<
                        FMP_RES_CODE << rc ) ;
         }
         goto error ;
      }
   }
   else if ( FMP_CONTROL_STEP_EVAL == step )
   {
      rc = _vm->initGlobalDB( FMP_COORD_HOST, _svcname.c_str(),
                              _userName.c_str(), _password.c_str(),
                              res ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to init global db: %s",
                 res.toString( FALSE, TRUE ).c_str() ) ;
         // continue run
      }

      rc = _vm->eval( obj, res ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to eval func:%s, rc:%d",
                 obj.toString(FALSE, TRUE).c_str(), rc ) ;
         if ( res.isEmpty() )
         {
            res = BSON( FMP_ERR_MSG << "failed to eval func" <<
                        FMP_RES_CODE << rc ) ;
         }
         goto error ;
      }
   }
   else if ( FMP_CONTROL_STEP_FETCH == step )
   {
      BSONObj next ;
      rc = _vm->fetch( next ) ;
      if ( !next.isEmpty() )
      {
         res = next ;
      }
      else
      {
         PD_LOG( PDERROR, "a empty obj was fetched out" ) ;
         rc = SDB_SYS ;
         res = BSON( FMP_ERR_MSG << "a empty obj was fetched out" <<
                     FMP_RES_CODE << rc ) ;
         goto error ;
      }

      if ( SDB_DMS_EOC == rc )
      {
         _clear() ;
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else
   {
      SDB_ASSERT( FALSE, "impossible" ) ;
   }

done:
   {
   INT32 rrc = SDB_OK ;
   if ( !res.isEmpty() )
   {
      rrc = _writeMsg( res ) ;
   }
   else
   {
      rrc = _writeMsg( BSON( FMP_RES_CODE << rc ) ) ;
   }
   if ( SDB_OK != rrc )
   {
      rc = rrc ;
      PD_LOG( PDERROR, "failed to write msg:%d", rc ) ;
   }
   }
   return rc ;
error:
   goto done ;
}

static INT32 loopRead( OSSFILE  *pFile,
                       CHAR *buffer,
                       SINT32 len )
{
   INT32 rc = SDB_OK ;
   SINT32 readLen = 0 ;
   SINT64 readOnce = 0 ;
   while ( readLen < len )
   {
      rc = ossRead( pFile, buffer, sizeof(SINT32), &readOnce ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         goto error ;
      }
      rc = SDB_OK ;
      readLen += readOnce ;
      buffer += readOnce ;
      readOnce = 0 ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpController::_readMsg( BSONObj &msg )
{
   INT32 rc = SDB_OK ;
   if ( _inBufSize < sizeof(UINT32) )
   {
      CHAR *pOldBuff = _inBuf ;
      _inBuf = (CHAR *)SDB_OSS_REALLOC(_inBuf, sizeof(SINT32)) ;
      if ( NULL == _inBuf )
      {
         PD_LOG( PDERROR, "failed to allocate mem" ) ;
         _inBuf = pOldBuff ;
         rc = SDB_OOM ;
         goto error ;
      }

      _inBufSize = sizeof(UINT32) ;
   }

   rc = loopRead( &_in, _inBuf, sizeof(UINT32) ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( _inBufSize < *((UINT32 *)_inBuf) )
   {
      CHAR *pOldBuff = _inBuf ;
      _inBuf = (CHAR *)SDB_OSS_REALLOC(_inBuf, *((SINT32 *)_inBuf)) ;
      if ( NULL == _inBuf )
      {
         PD_LOG( PDERROR, "failed to allocate mem" ) ;
         _inBuf = pOldBuff ;
         rc = SDB_OOM ;
         goto error ;
      }

      _inBufSize = *((UINT32 *)_inBuf) ;
   }

   rc = loopRead( &_in, _inBuf+sizeof(SINT32),
                  *((SINT32 *)_inBuf)-sizeof(SINT32)) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   msg = BSONObj( _inBuf ) ;
done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpController::_writeMsg( const BSONObj &msg )
{
   INT32 rc = SDB_OK ;
   SINT64 written = 0 ;
   BOOLEAN writeHead = TRUE ;

   /// to distinguish our own msg or printf.
   SINT64 objsize = sizeof( magicNumber ) ;
   const CHAR *buf = (const CHAR*)magicNumber ;

send:
   while ( 0 < objsize )
   {
      written = 0 ;
      rc = ossWrite( &_out, buf, objsize, &written ) ;
      if ( rc && SDB_INTERRUPT != rc )
      {
         PD_LOG( PDERROR, "failed to write msg:%d", rc ) ;
         goto error ;
      }
      else
      {
         objsize -= written ;
         buf += written ;
         rc = SDB_OK ;
      }
   }

   if ( writeHead )
   {
      buf = msg.objdata() ;
      objsize = msg.objsize() ;
      writeHead = FALSE ;
      goto send ;
   }
done:
   return rc ;
error:
   goto done ;
}

INT32 _fmpController::_createVM( SINT32 type )
{
   INT32 rc = SDB_OK ;
   _vm = SDB_OSS_NEW _fmpJSVM ;
   if ( NULL == _vm )
   {
      rc = SDB_OOM ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

void _fmpController::_clear()
{
   SAFE_OSS_DELETE( _vm ) ;
   FMP_STEP_ASSIGN( FMP_CONTROL_STEP_BEGIN ) ;

   _svcname.clear() ;
   _userName.clear() ;
   _password.clear() ;

   return ;
}
