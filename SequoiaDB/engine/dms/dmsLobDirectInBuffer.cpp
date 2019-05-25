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

   Source File Name = dmsLobDirectInBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsLobDirectInBuffer.hpp"
#include "pmdEDU.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

namespace engine
{
   /*
      _dmsLobDirectInBuffer implement
   */
   _dmsLobDirectInBuffer::_dmsLobDirectInBuffer( CHAR *usrBuf,
                                                 UINT32 size,
                                                 UINT32 offset,
                                                 BOOLEAN needAligned,
                                                 IExecutor *cb )
   :_dmsLobDirectBuffer( usrBuf, size, offset, needAligned, cb )
   {
   }

   _dmsLobDirectInBuffer::~_dmsLobDirectInBuffer()
   {
   }

   INT32 _dmsLobDirectInBuffer::doit( const tuple **pTuple )
   {
      INT32 rc = prepare() ;
      if ( SDB_OK == rc && pTuple )
      {
         *pTuple = &_t ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMS_LOBDIRECTINBUF_DONE, "_dmsLobDirectInBuffer::done" )
   void _dmsLobDirectInBuffer::done()
   {
      PD_TRACE_ENTRY( SDB__DMS_LOBDIRECTINBUF_DONE ) ;

      if ( _aligned )
      {
         SDB_ASSERT( _t.offset <= _usrOffset &&
                     _usrSize <= _t.size - ( _usrOffset - _t.offset ) &&
                     NULL != _t.buf, "impossible" ) ;
         ossMemcpy( _usrBuf,
                    ( const CHAR * )_buf + ( _usrOffset - _t.offset ),
                    _usrSize ) ;
      }

      PD_TRACE_EXIT( SDB__DMS_LOBDIRECTINBUF_DONE ) ;
   }

}

