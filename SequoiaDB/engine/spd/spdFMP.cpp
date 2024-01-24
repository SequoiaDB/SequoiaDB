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

   Source File Name = spdFMP.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "spdFMP.hpp"
#include "fmpDef.hpp"
#include "ossProc.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"

using namespace bson ;

#define SPD_READ_TIMEOUT      1000
const INT32 SPD_MSG_MAX_LEN = 17 * 1024 *1024 ;
#define SPD_READ_PAGE         4096

static const CHAR SPD_MAGIC[] = FMP_MSG_MAGIC ;
static BSONObj RESET_MSG = BSON( FMP_CONTROL_FIELD << FMP_CONTROL_STEP_RESET ) ;
static BSONObj QUIT_MSG = BSON( FMP_CONTROL_FIELD << FMP_CONTROL_STEP_QUIT ) ;

namespace engine
{
   _spdFMP::_spdFMP()
   :_seqID( 0 ),
    _id( -1 ),
    _discarded(FALSE),
    _readBuf(NULL),
    _readBufSize(0),
    _totalRead(0),
    _itr(0),
    _expect( 0 )
   {}

   _spdFMP::~_spdFMP()
   {
      if ( -1 != _id )
      {
         ossCloseNamedPipe( _in ) ;
         ossCloseNamedPipe( _out ) ;

         ossTerminateProcess( _id, TRUE ) ;
#if defined (_WINDOWS)
         ossWaitInterrupt( (HANDLE)_id, 10000 ) ;
#elif defined (_LINUX)
         ossResultCode result ;
         ossWaitChild( _id, result ) ;
#endif
      }

      if ( NULL != _readBuf )
      {
         SDB_OSS_FREE( _readBuf ) ;
      }
   }

   INT32 _spdFMP::read( BSONObj &msg, _pmdEDUCB *cb, BOOLEAN ignoreTimeout )
   {
      INT32 rc = SDB_OK ;
      INT64 read = 0 ;
      CHAR *readBuf = _readBuf ;

      while ( TRUE )
      {
         if ( ignoreTimeout && cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            PD_LOG( PDINFO, "operation is interrupted" ) ;
            goto done ;
         }

         SDB_ASSERT( _totalRead <= _readBufSize, "impossible" ) ;
         if ( _readBufSize == _totalRead )
         {
            rc = _extendReadBuf() ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         readBuf = _readBuf + _totalRead ;
         read = 0 ;
         rc = ossReadNamedPipe( _out, readBuf, _readBufSize - _totalRead,
                                &read, SPD_READ_TIMEOUT ) ;
         if ( SDB_TIMEOUT == rc )
         {
            SDB_ASSERT( 0 == read, "impossible" ) ;
            if (  ignoreTimeout )
            {
               continue ;
            }
            else
            {
               goto error ;
            }
         }
         else if ( SDB_OK == rc || SDB_INTERRUPT == rc )
         {
            _totalRead += read ;
            BOOLEAN extracted = FALSE ;
            rc = _extractMsg( msg, extracted ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extract obj msg:%d",rc ) ;
               goto error ;
            }
            else if ( extracted )
            {
               goto done ;
            }
            else
            {
               if ( SPD_MSG_MAX_LEN <= _totalRead )
               {
                  PD_LOG( PDERROR, "the length of uncompleted "
                          "msg is too long:%d", _totalRead ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               continue ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "failed to read from fmp:%d", rc ) ;
            setDiscarded() ;
            goto error ;
         }
      }
   done:
      _clear() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMP::write( const BSONObj &msg )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( !msg.isEmpty(), "impossible" ) ;

      INT64 written = 0 ;
      INT64 writeLen = msg.objsize() ;
      const CHAR *writeBuf = msg.objdata() ;
      while ( 0 < writeLen )
      {
         written = 0 ;
         rc = ossWriteNamedPipe( _in, writeBuf, writeLen, &written ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write to fmp:%d", rc ) ;
            setDiscarded() ;
            goto error ;
         }
//         PD_LOG( PDDEBUG, "write to fmp len: %lld", written ) ;
         writeLen -= written ;
         writeBuf += written ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMP::quit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj res ;
      rc = write( QUIT_MSG ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write reset msg:%d", rc ) ;
         goto error ;
      }

      rc = read( res, cb, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read reset res:%d"
                          ", kill process[%d]", rc, _id ) ;
         ossTerminateProcess( _id, TRUE ) ;
      }
      /// parse res obj if necessary in future.

      ossCloseNamedPipe( _in ) ;
      ossCloseNamedPipe( _out ) ;

#if defined (_WINDOWS)
      rc = ossWaitInterrupt( (HANDLE)_id, 10000 ) ;
#elif defined (_LINUX)
      {
      ossResultCode result ;
      rc = ossWaitChild( _id, result ) ;
      }
#endif
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to wait child process[%d], rc:%d",
                 _id, rc ) ;
         goto error ;
      }

      _id = -1 ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMP::reset( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj res ;
      rc = write( RESET_MSG ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write reset msg:%d", rc ) ;
         goto error ;
      }

      rc = read( res, cb, FALSE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read reset res:%d", rc ) ;
         goto error ;
      }

      /// parse res obj if necessary in future.
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMP::_extendReadBuf()
   {
      INT32 rc = SDB_OK ;
      CHAR *pOrgBuff = _readBuf ;
      _readBuf = ( CHAR * )SDB_OSS_REALLOC( _readBuf,
                                            _readBufSize + SPD_READ_PAGE ) ;
      if ( NULL == _readBuf )
      {
         PD_LOG( PDERROR, "failed to realloc %d bytes mem.",
                 _readBufSize + SPD_READ_PAGE ) ;
         _readBuf = pOrgBuff ;
         rc = SDB_OOM ;
         goto error ;
      }
      _readBufSize += SPD_READ_PAGE ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _spdFMP::_extractMsg( BSONObj &msg, BOOLEAN &extracted )
   {
      INT32 rc = SDB_OK ;
      extracted = FALSE ;
      SDB_ASSERT( 0 <= _expect, "impossible" ) ;

      /// magic has already been found.
      if ( sizeof( SPD_MAGIC ) == _expect )
      {
found:
         if ( (_totalRead - _itr) < (INT32)sizeof(INT32) )
         {
            extracted = FALSE ;
            goto done ;
         }
         else
         {
            SINT32 bsonLen = *((SINT32 *)(_readBuf+_itr)) ;
            if ( (_totalRead - _itr) < bsonLen )
            {
               rc = FALSE ;
               goto done ;
            }
            else if ( (_totalRead - _itr) == bsonLen )
            {
               SDB_ASSERT( _itr >= (INT32)sizeof( SPD_MAGIC ) ,
                           "impossible" ) ;
               BSONObj tmp ;

               try
               {
                  tmp = BSONObj( _readBuf + _itr ) ;
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               if ( sizeof( SPD_MAGIC ) == _itr )
               {
                  /// only a bson msg.
                  msg = tmp ;
                  extracted = TRUE ;
               }
               else
               {
                  /// not only a bson msg.
                  _readBuf[_itr - sizeof( SPD_MAGIC )] = '\0' ;
                  BSONElement retCode = tmp.getField( FMP_RES_CODE ) ;
                  BSONElement errMsg = tmp.getField( FMP_ERR_MSG ) ;

                  /// some code like 'print' may return msg more than a bsonobj.
                  /// we must parse it's return code. if it is ok, we ignore
                  /// print. else we put it into errmsg.
                  if ( !retCode.eoo() && NumberInt != retCode.type() )
                  {
                     rc = SDB_SYS ;
                     PD_LOG( PDERROR,
                             "invalid type of retCode:%d", retCode.type() ) ;
                     goto error ;
                  }
                  else if ( !retCode.eoo() ) 
                  {
                     if ( SDB_OK != retCode.Int() )
                     {
                        if ( !errMsg.eoo() )
                        {
                           msg = tmp ;
                        }
                        else
                        {
                           BSONObjBuilder builder ;
                           builder.append( FMP_ERR_MSG, _readBuf ) ;
                           builder.append( retCode ) ;
                           msg = builder.obj() ;
                        }
                     }
                     else
                     {
                        msg = tmp ;
                     }
                  }
                  else
                  {
                     /// retCode is eoo.
                     msg = tmp ;
                  }

                  extracted = TRUE ;
               }
            }
            else
            {
               SDB_ASSERT( FALSE, "impossible" ) ;
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "left len can not be lager than objsize" ) ;
               goto error ;
            }
         }
      }
      else
      {
         while ( _itr < _totalRead  && (UINT32)_expect < sizeof( SPD_MAGIC ) )
         {
            if ( SPD_MAGIC[_expect] == _readBuf[_itr] )
            {
               ++_itr ;
               ++_expect ;
               if ( sizeof( SPD_MAGIC ) == _expect )
               {
                  goto found ;
               }
            }
            else if ( 0 == _expect )
            {
               ++_itr ;
            }
            else
            {
               _expect = 0 ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

