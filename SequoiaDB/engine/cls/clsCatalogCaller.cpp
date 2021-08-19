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

   Source File Name = clsCatalogCaller.cpp

   Descriptive Name = clsCatalogCaller.hpp

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

#include "clsCatalogCaller.hpp"
#include "netRouteAgent.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "clsMgr.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   const INT32 CLS_CALLER_NO_SEND = -1 ;
   const INT32 CLS_CALLER_INTERVAL = 5000 ;

   _clsCatalogCaller::_clsCatalogCaller()
   {
   }

   _clsCatalogCaller::~_clsCatalogCaller()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_CALL, "_clsCatalogCaller::call" )
   INT32 _clsCatalogCaller::call( MsgHeader *header, UINT32 times )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_CALL );
      _clsCataCallerMeta &meta = _meta[MAKE_REPLY_TYPE(header->opCode)] ;
      if ( (SINT32)meta.bufLen < header->messageLength )
      {
         if ( NULL != meta.header )
         {
            SDB_OSS_FREE( meta.header ) ;
            meta.bufLen = 0 ;
         }
         // memory is free in destructor
         meta.header = ( MsgHeader *)SDB_OSS_MALLOC( header->messageLength ) ;
         if ( NULL == meta.header )
         {
            PD_LOG ( PDERROR, "Failed to allocate memory for header" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         meta.bufLen = header->messageLength ;
      }

      ossMemcpy( meta.header, header, header->messageLength ) ;
      PD_LOG( PDINFO, "send msg[%d] to catalog node.", meta.header->opCode ) ;

      /// reset info
      meta.timeout = 0 ;
      meta.sendTimes = times ;

      pmdGetKRCB()->getClsCB()->sendToCatlog( meta.header ) ;

   done:
      PD_TRACE_EXITRC ( SDB__CLSCATCLR_CALL, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_REMOVE, "_clsCatalogCaller::remove" )
   void _clsCatalogCaller::remove( const MsgHeader *header, INT32 result )
   {
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_REMOVE );
      callerMeta::iterator itr = _meta.find( header->opCode ) ;
      if ( _meta.end() != itr && SDB_OK == result )
      {
         PD_LOG( PDINFO, "response is ok, remove msg[(%d)%d]",
                 IS_REPLY_TYPE( header->opCode ),
                 GET_REQUEST_TYPE( header->opCode ) ) ;

         if ( itr->second.sendTimes > 1 )
         {
            --( itr->second.sendTimes ) ;
         }
         else
         {
            itr->second.timeout = CLS_CALLER_NO_SEND ;
            itr->second.sendTimes = 0 ;
         }
      }

      PD_TRACE_EXIT ( SDB__CLSCATCLR_REMOVE );
      return ;
   }

   void _clsCatalogCaller::remove( INT32 opCode )
   {
      callerMeta::iterator itr = _meta.find( (UINT32)opCode ) ;
      if ( _meta.end() != itr )
      {
         if ( itr->second.sendTimes > 1 )
         {
            --( itr->second.sendTimes ) ;
         }
         else
         {
            itr->second.timeout = CLS_CALLER_NO_SEND ;
            itr->second.sendTimes = 0 ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSCATCLR_HNDTMOUT, "_clsCatalogCaller::handleTimeout" )
   void _clsCatalogCaller::handleTimeout( const UINT32 &millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSCATCLR_HNDTMOUT );
      callerMeta::iterator itr = _meta.begin() ;
      for ( ; itr != _meta.end(); itr++ )
      {
         if ( CLS_CALLER_NO_SEND != itr->second.timeout )
         {
            itr->second.timeout += millisec ;
            if ( CLS_CALLER_INTERVAL <= itr->second.timeout )
            {
               /// don't need to update catalog info
               pmdGetKRCB()->getClsCB()->sendToCatlog( itr->second.header) ;
               itr->second.timeout = 0 ;
            }
         }
      }
      PD_TRACE_EXIT ( SDB__CLSCATCLR_HNDTMOUT );
      return ;
   }

}
