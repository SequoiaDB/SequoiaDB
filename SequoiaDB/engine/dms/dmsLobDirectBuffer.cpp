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

   Source File Name = dmsLobDirectBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLobDirectBuffer.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "dmsTrace.hpp"

namespace engine
{
   /*
      _dmsLobDirectBuffer implement
   */
   _dmsLobDirectBuffer::_dmsLobDirectBuffer( CHAR *usrBuf,
                                             UINT32 size,
                                             UINT32 offset,
                                             BOOLEAN needAligned,
                                             IExecutor *cb )
   {
      _aligned = needAligned ;
      _cb = cb ;
      _buf = NULL ;
      _bufSize = 0 ;

      _usrBuf = usrBuf ;
      _usrSize = size ;
      _usrOffset = offset ;

      SDB_ASSERT( NULL != _cb, "can not be NULL" ) ;
      SDB_ASSERT( NULL != _usrBuf && 0 < _usrSize, "impossible" ) ;
   }

   _dmsLobDirectBuffer::~_dmsLobDirectBuffer()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTBUF_PREPARE, "_dmsLobDirectBuffer::prepare" )
   INT32 _dmsLobDirectBuffer::prepare()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTBUF_PREPARE ) ;

      if ( !_aligned )
      {
         _t.buf = _usrBuf ;
         _t.size = _usrSize ;
         _t.offset = _usrOffset ;
      }
      else
      {
         UINT32 newSize = _usrSize ;
         UINT32 newOffset = ossRoundUpToMultipleX( _usrOffset,
                                                   OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
         if ( _usrOffset != newOffset )
         {
            newOffset -= OSS_FILE_DIRECT_IO_ALIGNMENT ;
         }

         SDB_ASSERT( newOffset <= _usrOffset, "impossible" ) ;
         newSize += ( _usrOffset - newOffset ) ;
         newSize = ossRoundUpToMultipleX( newSize,
                                          OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
         if ( _bufSize < newSize )
         {
            rc = _extendBuf( newSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to extend buf:%d", rc ) ;
               goto error ;
            }
         }

         if ( _usrOffset == newOffset &&
              _usrSize == newSize &&
              _usrBuf >= _buf &&
              0 == ( _usrBuf - _buf ) % OSS_FILE_DIRECT_IO_ALIGNMENT &&
              _usrBuf + _usrSize <= _buf + _bufSize )
         {
            _aligned = FALSE ;

            _t.buf = _usrBuf ;
            _t.size = _usrSize ;
            _t.offset = _usrOffset ;
         }
         else
         {
            _t.buf = _buf ;
            _t.size = newSize ;
            _t.offset = newOffset ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMS_LOBDIRECTBUF_PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTBUF__EXTENDBUF, "_dmsLobDirectBuffer::_extendBuf" )
   INT32 _dmsLobDirectBuffer::_extendBuf( UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTBUF__EXTENDBUF ) ;
      UINT32 realSize = 0 ;

      _buf = ( CHAR* )_cb->getAlignedBuff( size, &realSize,
                                           OSS_FILE_DIRECT_IO_ALIGNMENT ) ;
      if ( NULL == _buf )
      {
         _bufSize = 0 ;
         PD_LOG( PDERROR, "Failed to allocate mem" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _bufSize = realSize ;

   done:
      PD_TRACE_EXITRC( SDB__DMS_LOBDIRECTBUF__EXTENDBUF, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

