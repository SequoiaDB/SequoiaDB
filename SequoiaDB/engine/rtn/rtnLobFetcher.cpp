/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

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
    _pos( DMS_LOB_INVALID_PAGEID ),
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
                               BOOLEAN onlyMetaPage,
                               const ossPoolSet<OID> *pOids )
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
      _pos = 0 ;
      _onlyMetaPage = onlyMetaPage ;

      if ( pOids )
      {
         _filterOIDs = *pOids ;
      }

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
      UINT32 readCnt = 0 ;
      const UINT32 _readStepCnt = 1000 ;
      BOOLEAN doLock = FALSE ;

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

      if ( !_mbContext->isMBLock() )
      {
         rc = _mbContext->mbLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get mblock, rc: %d", rc ) ;
            goto error ;
         }
         doLock = TRUE ;
      }

      if ( !_su->lob()->isOpened() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      while( TRUE )
      {
         ++readCnt ;
         rc = _su->lob()->readPage( _pos, _onlyMetaPage, cb, _mbContext, page ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to read lob pages, rc: %d", rc ) ;
            }
            goto error ;
         }

         /// filter
         if ( _filterOIDs.empty() )
         {
            break ;
         }
         else if ( _filterOIDs.find( page._oid ) != _filterOIDs.end() )
         {
            break ;            
         }

         if ( readCnt > _readStepCnt )
         {
            readCnt = 0 ;

            if ( doLock )
            {
               _mbContext->pause() ;
            }

            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }

            if ( doLock )
            {
               rc = _mbContext->resume() ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Resume mblock failed, rc: %d", rc ) ;
                  goto error ;
               }
            }
         }
      }

      if ( NULL != mb )
      {
         _dmsLobRecord record ;
         UINT32 read = 0 ;

         if ( mb->idleSize() < page._len )
         {
            rc = mb->extend( page._len - mb->idleSize() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to extend mb block, rc: %d", rc ) ;
               goto error ;
            }
         }

         record.set( &( page._oid ), page._sequence, 0,
                     page._len, NULL, _su->lob()->hashType() ) ;
         rc = _su->lob()->read( record, _mbContext,
                                cb, mb->writePtr(), read ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read lob, rc: %d", rc ) ;
            goto error ;
         }

         if ( page._len != read )
         {
            PD_LOG( PDERROR, "length in page is:%d, but we read:%d",
                    page._len, read ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         mb->writePtr( mb->length() + read ) ;
      }
   done:
      if ( doLock )
      {
         _mbContext->mbUnlock() ;
         doLock = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__RTNLOBFETCHER_FETCH, rc ) ;
      return rc ;
   error:
      /// last error valuate must in collection latch
      _lastErr = rc ;
      goto done ;
   }

   void _rtnLobFetcher::_fini()
   {
      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;

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
      _filterOIDs.clear() ;

      return ;
   }

   void _rtnLobFetcher::close( INT32 cause )
   {
      /// last error valuate must in collection latch
      _lastErr = cause ;
      _fini() ;
   }

}

