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

   Source File Name = dpsLogPage.cpp

   Descriptive Name = Data Protection Services Log Page

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsLogPage.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
namespace engine
{
   _dpsLogPage::_dpsLogPage()
   {
      _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
      if ( NULL == _mb )
      {
         pdLog(PDERROR, __FUNC__, __FILE__, __LINE__,
               "new _dpsMessageBlock failed!");
      }
      _startPage = DPS_LSN_START_FROM_HEAD;
   }

   _dpsLogPage::_dpsLogPage( UINT32 size )
   {
      _mb = SDB_OSS_NEW _dpsMessageBlock( size );
      if ( NULL == _mb )
      {
         pdLog(PDERROR, __FUNC__, __FILE__, __LINE__,
               "new _dpsMessageBlock failed!");
      }

      _startPage = DPS_LSN_START_FROM_HEAD;
   }

   _dpsLogPage::~_dpsLogPage()
   {
      lock();
      if ( _mb )
         SDB_OSS_DEL _mb;
      _mb = NULL;
      unlock();
   }

   string _dpsLogPage::toString() const
   {
      stringstream ss ;
      ss << "PageNumber: " << _pageNumber
         << ", StartPage: " << (INT32)_startPage
         << ", BeginLSNV" << _beginLSN.version
         << ", BeginLSNO" << _beginLSN.offset
         << ", Length: " << getLength()
         << ", Size: " << getBufSize()
         << ", IdleSize: " << getLastSize() ;
      return ss.str() ;
   }

/*
   INT32 _dpsLogPage::insert( const CHAR *src, UINT32 len )
   {
      INT32 rc = SDB_OK;
      UINT64 offset = 0;
      rc = allocate( len, offset );
      if ( rc )
      {
         pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                 "Failed to allocate %d from offset %lld, rc = %d",
                 len, offset, rc ) ;
         goto error;
      }
      rc = fill( offset, src, len );
      if ( rc )
      {
         pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                 "Failed to fill, rc = %d", rc ) ;
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }
*/
   PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGPAGE, "_dpsLogPage::fill" )
   INT32 _dpsLogPage::fill( UINT32 offset, const CHAR *src, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DPSLGPAGE );
      if ( !_mb )
      {
         _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
         if ( NULL == _mb )
         {
            pdLog(PDERROR, __FUNC__, __FILE__, __LINE__,
                  "new _dpsMessageBlock failed!");
            rc = SDB_OOM ;
            goto error ;
         }
      }
      ossMemcpy ( _mb->offset( offset ), src, len ) ;
   done :
      PD_TRACE_EXITRC ( SDB__DPSLGPAGE, rc );
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__DPSLGPAGE2, "_dpsLogPage::allocate" )
   INT32 _dpsLogPage::allocate( UINT32 len )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DPSLGPAGE2 );
      if ( !_mb )
      {
         _mb = SDB_OSS_NEW _dpsMessageBlock( DPS_DEFAULT_PAGE_SIZE );
         if ( NULL == _mb )
         {
            pdLog(PDERROR, __FUNC__, __FILE__, __LINE__,
                  "new _dpsMessageBlock failed!");
            rc = SDB_OOM ;
            goto error ;
         }
      }
      SDB_ASSERT ( getLastSize() >= len, "len is greater than buffer size" ) ;
      _mb->writePtr( len + _mb->length() );

   done :
      PD_TRACE_EXITRC ( SDB__DPSLGPAGE2, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _dpsLogPage::allocate( UINT32 len, UINT64 &offset )
   {
      offset = getLength();

      return allocate(len);
   }
}

