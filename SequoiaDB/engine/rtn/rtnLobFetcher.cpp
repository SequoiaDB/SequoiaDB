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

   Source File Name = rtnLobFetcher.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLobFetcher.hpp"
#include "dmsCB.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   _rtnLobFetcher::_rtnLobFetcher()
   :_suID( DMS_INVALID_CS ),
    _su( NULL ),
    _mbContext( NULL ),
    _onlyMetaPage( FALSE ),
    _lastErr( SDB_OK )
   {
      ossMemset( _fullName, 0, sizeof( _fullName ) ) ;
   }

   _rtnLobFetcher::~_rtnLobFetcher()
   {
      _fini() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOBFETCHER_INIT, "_rtnLobFetcher::init" )
   INT32 _rtnLobFetcher::init( const CHAR *fullName,
                               BOOLEAN onlyMetaPage )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLOBFETCHER_INIT ) ;
      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;
      const CHAR *clName = NULL ;

      if ( NULL != _su || NULL != _mbContext )
      {
         PD_LOG( PDERROR, "do not init fetcher again" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock( fullName, dmsCB,
                                            &_su, &clName, _suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to resolve collection:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

      rc = _su->data()->getMBContext( &_mbContext, clName, -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to resolve collection name:%s",
                 clName ) ;
         goto error ;
      }

      ossStrncpy( _fullName, fullName, DMS_COLLECTION_FULL_NAME_SZ ) ;
      _lastErr = SDB_OK ;
      _onlyMetaPage = onlyMetaPage ;
   done:
      PD_TRACE_EXITRC( SDB__RTNLOBFETCHER_INIT, rc ) ;
      return rc ;
   error:
      _fini() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOBFETCHER_FETCH, "_rtnLobFetcher::fetch" )
   INT32 _rtnLobFetcher::fetch( _pmdEDUCB *cb,
                                dmsLobInfoOnPage &page,
                                _dpsMessageBlock *mb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNLOBFETCHER_FETCH ) ;

      const CHAR *data = nullptr ;
      BOOLEAN needAdvance = TRUE ;

      if ( SDB_OK != _lastErr )
      {
         rc = _lastErr ;
         goto error ;
      }

      if ( NULL == _mbContext || NULL == _su )
      {
         PD_LOG( PDERROR, "fetcher has not been initialized yet" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _mbContext->mbLock( SHARED ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get lock:%d", rc ) ;
         goto error ;
      }

      if ( !_su->lob()->isOpened() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_cursor )
      {
         std::shared_ptr< ILob > lobPtr ;
         rc = _mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage, rc: %d", rc ) ;

         rc = lobPtr->list( cb, _cursor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor to list lob, rc: %d", rc ) ;
         needAdvance = FALSE ;
      }

      do
      {
         if ( needAdvance )
         {
            rc = _cursor->advance( cb ) ;
            if ( SDB_DMS_EOC == rc )
            {
               goto error ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
         }

         rc = _cursor->getCurrentLobRecord( page, &data ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "failed to read lob pages:%d", rc ) ;
            }
            goto error ;
         }
         needAdvance = TRUE ;
      } while ( _onlyMetaPage && page._sequence != 0 ) ;

      if ( NULL != mb )
      {
         if ( mb->idleSize() < page._len )
         {
            rc = mb->extend( page._len - mb->idleSize() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extend mb block:%d", rc ) ;
               goto error ;
            }
         }

         ossMemcpy( mb->writePtr(), data, page._len ) ;
         mb->writePtr( mb->length() + page._len ) ;
      }
   done:
      if ( NULL != _mbContext && _mbContext->isMBLock() )
      {
         _mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB__RTNLOBFETCHER_FETCH, rc ) ;
      return rc ;
   error:
      /// last error valuate must in collection latch
      _lastErr = rc ;
      _fini() ;
      goto done ;
   }

   void _rtnLobFetcher::_fini()
   {
      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;

      if ( _cursor )
      {
         _cursor->close() ;
      }
      _cursor.reset() ;
      if ( NULL != _mbContext && NULL != _su )
      {
         if ( _mbContext->isMBLock() )
         {
            _mbContext->mbUnlock() ;
         }
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( NULL != _su )
      {
         dmsCB->suUnlock ( _suID ) ;
         _su = NULL ;
         _suID = DMS_INVALID_CS ;
      }

      _onlyMetaPage = FALSE ;

      return ;
   }

   void _rtnLobFetcher::close( INT32 cause )
   {
      /// last error valuate must in collection latch
      _lastErr = cause ;
      _fini() ;
   }

}

