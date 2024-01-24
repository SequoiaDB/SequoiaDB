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

   Source File Name = dmsLobDirectOutBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLobDirectOutBuffer.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   #define DMS_SEQ_READ_FAN_THRESHOLD     ( 7 * OSS_FILE_DIRECT_IO_ALIGNMENT )

   /*
      _dmsLobDirectOutBuffer implement
   */
   _dmsLobDirectOutBuffer::_dmsLobDirectOutBuffer( const CHAR *usrBuf,
                                                   UINT32 size,
                                                   UINT32 offset,
                                                   BOOLEAN needAligned,
                                                   IExecutor *cb,
                                                   INT32 pageID,
                                                   UINT32 newestMask,
                                                   utilCachFileBase *pFile )
   :_dmsLobDirectBuffer( (CHAR*)usrBuf, size, offset, needAligned, cb )
   {
      _pageID = pageID ;
      _newestMask = newestMask ;
      _pFile = pFile ;

      SDB_ASSERT( _pFile, "_pFile is NULL" ) ;
   }

   _dmsLobDirectOutBuffer::~_dmsLobDirectOutBuffer()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTOUTBUF_DOIT, "_dmsLobDirectOutBuffer::doit" )
   INT32 _dmsLobDirectOutBuffer::doit( const tuple **pTuple )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTOUTBUF_DOIT ) ;

      rc = prepare() ;
      if ( rc )
      {
         goto error ;
      }

      if ( _aligned )
      {
         INT32 offset1 = -1 ;
         INT32 offset2 = -1 ;
         UINT32 readLen = 0 ;
         INT64 fOffset = 0 ;

         if ( _t.offset != _usrOffset )
         {
            offset1 = _t.offset ;
         }
         if ( _t.size != _usrSize )
         {
            offset2 = _t.offset + _t.size - OSS_FILE_DIRECT_IO_ALIGNMENT ;
         }

         if ( offset1 >= 0 && offset2 >= 0 &&
              offset2 - offset1 <= DMS_SEQ_READ_FAN_THRESHOLD &&
              _pageID >= 0 &&
              0 == ( _newestMask & UTIL_WRITE_NEWEST_BOTH ) )
         {
            UINT32 len = OSS_FILE_DIRECT_IO_ALIGNMENT + offset2 - offset1 ;
            fOffset = _pFile->pageID2Offset( _pageID, (UINT32)offset1 ) ;
            rc = _pFile->readRaw( fOffset, len, _t.buf, readLen, _cb, TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Read date[PageID:%u, Offset:%u, Len:%u] "
                       "failed, rc: %d", _pageID, offset1, len, rc ) ;
               goto error ;
            }
            offset1 = -1 ;
            offset2 = -1 ;
         }

         if ( offset1 >= 0 )
         {
            if ( OSS_BIT_TEST( _newestMask, UTIL_WRITE_NEWEST_HEADER ) )
            {
               ossMemset( _t.buf, 0, _usrOffset - offset1 ) ;
            }
            else
            {
               fOffset = _pFile->pageID2Offset( _pageID, (UINT32)offset1 ) ;
               rc = _pFile->readRaw( fOffset, OSS_FILE_DIRECT_IO_ALIGNMENT,
                                     _t.buf, readLen, _cb, TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read date[PageID:%u, Offset:%u, Len:%u] "
                          "failed, rc: %d", _pageID, offset1,
                          OSS_FILE_DIRECT_IO_ALIGNMENT, rc ) ;
                  goto error ;
               }
            }
         }

         if ( offset2 >= 0 )
         {
            if ( OSS_BIT_TEST( _newestMask, UTIL_WRITE_NEWEST_TAIL ) )
            {
               ossMemset( _t.buf + _usrOffset + _usrSize - _t.offset,
                          0,
                          _t.size - ( _usrSize + _usrOffset - _t.offset ) ) ;
            }
            else
            {
               fOffset = _pFile->pageID2Offset( _pageID, (UINT32)offset2 ) ;
               rc = _pFile->readRaw( fOffset, OSS_FILE_DIRECT_IO_ALIGNMENT,
                                     _t.buf + offset2 - _t.offset, readLen,
                                     _cb, TRUE ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Read date[PageID:%u, Offset:%u, Len:%u] "
                          "failed, rc: %d", _pageID, offset2,
                          OSS_FILE_DIRECT_IO_ALIGNMENT, rc ) ;
                  goto error ;
               }
            }
         }

         /// copy data
         ossMemcpy( _t.buf + _usrOffset - _t.offset,
                    _usrBuf, _usrSize ) ;
      }

      if ( pTuple )
      {
         *pTuple = &_t ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMS_LOBDIRECTOUTBUF_DOIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dmsLobDirectOutBuffer::done()
   {
   }

}

