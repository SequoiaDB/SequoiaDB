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

   Source File Name = dpsMessageBlock.cpp

   Descriptive Name = Data Protection Service Message Block

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for message block,
   which is buffer for storing log information

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dpsMessageBlock.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"

namespace engine
{
   _dpsMessageBlock::_dpsMessageBlock( UINT32 size )
   {
      _start = ( CHAR * )SDB_OSS_MALLOC( size + 1 );
      _read = _start;
      _write = _start;
      if ( _start )
         _size = size;
      else
         _size = 0 ;
      _length = 0;
   }

   _dpsMessageBlock::_dpsMessageBlock( const _dpsMessageBlock &mb )
   {
      _start =  ( CHAR * )SDB_OSS_MALLOC( mb.size() + 1 ) ;
      _read = _start ;
      _write = _start ;
      if ( NULL == _start )
      {
         _size = 0 ;
         _length = 0 ;
      }
      else
      {
         _size = mb.size() ;
         ossMemcpy( _start, mb.offset(0), mb.length() ) ;
         _length = mb.length() ;
         SDB_ASSERT( _length <= _size, "impossible" ) ;
      }
   }

   _dpsMessageBlock &_dpsMessageBlock::operator=
                                    ( const _dpsMessageBlock &mb )
    {
       _start =  ( CHAR * )SDB_OSS_MALLOC( mb.size() + 1 ) ;
      _read = _start ;
      _write = _start ;
      if ( NULL == _start )
      {
         _size = 0 ;
         _length = 0 ;
      }
      else
      {
         _size = mb.size() ;
         ossMemcpy( _start, mb.offset(0), mb.length() ) ;
         _length = mb.length() ;
         SDB_ASSERT( _length <= _size, "impossible" ) ;
      }
      return *this ;
    }

   _dpsMessageBlock::~_dpsMessageBlock()
   {
      if ( _start )
         SDB_OSS_FREE( _start );
      _start = NULL;
      _read = NULL;
      _write = NULL;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSMSGBLK_EXTEND, "_dpsMessageBlock::extend" )
   INT32 _dpsMessageBlock::extend( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSMSGBLK_EXTEND );
      SDB_ASSERT ( _write >= _start, "invalid write pointer position" ) ;
      SDB_ASSERT ( _read >= _start, "invalid read pointer position" ) ;
      ossValuePtr writeOffset = _write - _start ;
      ossValuePtr readOffset = _read - _start ;
      CHAR *pNewAddr = ( CHAR * )SDB_OSS_REALLOC( _start, _size + len + 1 ) ;
      if ( !pNewAddr )
      {
         PD_LOG ( PDERROR, "Failed to reallocate memory for %d bytes",
                  _size + len + 1 ) ;
         rc = SDB_OOM ;
         goto error;
      }
      if ( pNewAddr != _start )
      {
         _start = pNewAddr ;
         _write = _start + writeOffset ;
         _read = _start + readOffset ;
      }
      _size += len;

   done:
      PD_TRACE_EXITRC ( SDB__DPSMSGBLK_EXTEND, rc );
      return rc;
   error:
      goto done;
   }
}

