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

   Source File Name = rtnContextMain.cpp

   Descriptive Name = RunTime Main-Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2017   David Li  draft

   Last Changed =

*******************************************************************************/
#include "rtnContextMain.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "pd.hpp"

namespace engine
{
   _rtnContextMain::_rtnContextMain( INT64 contextID, UINT64 eduID )
      : _rtnContextBase( contextID, eduID ),
        _rtnCtxDataDispatcher(),
        _keyGen( NULL ),
        _numToReturn( -1 ),
        _numToSkip( 0 )
   {
   }

   _rtnContextMain::~_rtnContextMain()
   {
      pmdKRCB* pKrcb = pmdGetKRCB() ;
      SDB_RTNCB* rtnCB = pKrcb->getRTNCB() ;
      pmdEDUCB* eduCB = pKrcb->getEDUMgr()->getEDUByID( eduID() ) ;

      // clean ordered context
      SUB_ORDERED_CTX_MAP::iterator orderIter = _orderedContextMap.begin() ;
      while ( orderIter != _orderedContextMap.end() )
      {
         rtnCB->contextDelete( orderIter->second->contextID(), eduCB ) ;
         SDB_OSS_DEL orderIter->second ;
         ++orderIter ;
      }
      _orderedContextMap.clear() ;

      SAFE_OSS_DELETE( _keyGen ) ;
   }

   INT32 _rtnContextMain::reopenForExplain ( INT64 numToSkip,
                                             INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      _empty() ;
      _resetTotalRecords( 0 ) ;
      _isOpened = TRUE ;
      _hitEnd = FALSE ;
      _numToSkip = numToSkip ;
      _numToReturn = numToReturn ;

      return rc ;
   }

   INT32 _rtnContextMain::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;

      if ( _requireExplicitSorting() )
      {
         rc = _prepareDataByOrder( cb ) ;
      }
      else
      {
         rc = _prepareDataNormal( cb ) ;
      }

      if ( SDB_OK == rc )
      {
         rc = _doAfterPrepareData( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to do after prepare data, rc=%d", rc ) ;
         }
      }

      return rc ;
   }

   INT32 _rtnContextMain::_saveNonEmptyOrderedSubCtx( rtnSubContext* subCtx )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != subCtx, "subCtx should be not null" ) ;
      SDB_ASSERT( subCtx->recordNum() > 0, "subCtx is empty" ) ;

      rtnOrderKey orderKey ;
      rc = subCtx->getOrderKey( orderKey ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get orderKey, rc = %d", rc ) ;
         goto error ;
      }

      try
      {
         _orderedContextMap.insert(
            SUB_ORDERED_CTX_MAP::value_type( orderKey, subCtx ) ) ;
      }
      catch( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "occur unexpected error:%s", e.what() );
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextMain::_prepareDataByOrder( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;

      while ( 0 != _numToReturn )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = _prepareAllSubCtxDataByOrder( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare all sub contexts' data, rc=%d", rc ) ;
            goto error ;
         }

         if ( _orderedContextMap.size() == 0 )
         {
            _hitEnd = TRUE ;
            rc = SDB_DMS_EOC ;
            goto error ;
         }

         if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( eof() )
         {
            break ;
         }

         SUB_ORDERED_CTX_MAP::iterator iter = _orderedContextMap.begin();
         if ( _orderedContextMap.end() == iter )
         {
            _hitEnd = TRUE ;
            if ( isEmpty() )
            {
               rc = SDB_DMS_EOC;
            }
            break;
         }

         rtnSubContext* ctx = iter->second ;

         if ( _numToSkip <= 0 )
         {
            const CHAR* data = ctx->front() ;
            if ( NULL == data )
            {
               rc = SDB_SYS ;
               PD_LOG ( PDERROR, "Failed to get the data, rc: %d", rc ) ;
               goto error ;
            }

            try
            {
               BSONObj obj( data );
               BSONObj selected ;
               const BSONObj* record = NULL ;

               if ( !_selector.isInitialized() )
               {
                  record = &obj ;
               }
               else
               {
                  rc = _selector.select( obj, selected ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "failed to select fields from obj:%d", rc ) ;
                     goto error ;
                  }
                  record = &selected ;
               }

               rc = append( *record ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to append data(rc=%d)", rc ) ;
                  goto error ;
               }
            }
            catch ( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "occur unexpected error:%s", e.what() );
               goto error;
            }

            if ( _numToReturn > 0 )
            {
               --_numToReturn ;
            }
         }
         else
         {
            --_numToSkip ;
         }

         rc = ctx->pop();
         if ( SDB_OK != rc )
         {
            goto error;
         }

         if ( ctx->recordNum() <= 0 )
         {
            _orderedContextMap.erase ( iter ) ;

            rc = _saveEmptyOrderedSubCtx( ctx ) ;
            if ( SDB_OK != rc )
            {
               SDB_OSS_DEL ctx ;
               goto error ;
            }

            // if main buffer is not empty, break to return objs,
            // so this sub context can prefetch simultaneously
            if ( !isEmpty() )
            {
               break ;
            }
         }
         else
         {
            _orderedContextMap.erase ( iter ) ;

            rc = _saveNonEmptyOrderedSubCtx( ctx ) ;
            if ( SDB_OK != rc )
            {
               SDB_OSS_DEL ctx ;
               goto error ;
            }
         }

         // make sure we still have room to read another
         // record_max_sz (i.e. 16MB). if we have less than 16MB
         // to 256MB, we can't safely assume the next record we
         // read will not overflow the buffer, so let's just break
         // before reading the next record
         if ( buffEndOffset() + DMS_RECORD_MAX_SZ >
              RTN_RESULTBUFFER_SIZE_MAX )
         {
            // let's break if there's no room for another max record
            break ;
         }
      }

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextMain::_prepareDataNormal( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      while ( 0 != _numToReturn )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( eof() )
         {
            break ;
         }

         rtnSubContext* ctx = NULL ;

         rc = _getNonEmptyNormalSubCtx( cb, ctx ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               _hitEnd = TRUE ;
            }
            goto error ;
         }

         SDB_ASSERT( ctx != NULL, "ctx should not be null" ) ;

         if ( _numToSkip > 0 )
         {
            if ( _numToSkip >= ctx->recordNum() )
            {
               _numToSkip -= ctx->recordNum() ;
               rc = ctx->popAll() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pop all objs of sub ctx, rc: %d", rc ) ;

               rc = _saveEmptyNormalSubCtx( ctx ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to save empty sub ctx, rc: %d", rc ) ;

               continue ;
            }
            else
            {
               rc = ctx->popN( _numToSkip ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pop objs of sub ctx, rc: %d", rc ) ;
               _numToSkip = 0 ;
            }
         }

         if ( !_selector.isInitialized() )
         {
            if ( _numToReturn > 0 )
            {
               if ( ctx->recordNum() > _numToReturn )
               {
                  rc = ctx->truncate( _numToReturn ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to truncate sub ctx, rc: %d", rc ) ;
               }
               _numToReturn -= ctx->recordNum() ;
            }

            rc = appendObjs( ctx->front(),
                             ctx->remainLength(),
                             ctx->recordNum() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append objs, rc: %d", rc ) ;

            rc = ctx->popAll() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to pop all objs of sub ctx, rc: %d", rc ) ;
         }
         else
         {
            while ( ctx->recordNum() > 0 && 0 != _numToReturn )
            {
               const CHAR* data = ctx->front() ;
               if ( NULL == data )
               {
                  rc = SDB_SYS ;
                  PD_LOG ( PDERROR, "Failed to get the data, rc=%d", rc ) ;
                  goto error ;
               }

               try
               {
                  BSONObj obj( data ) ;
                  BSONObj selected ;

                  rc = _selector.select( obj, selected ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to select fields, rc=%d", rc ) ;
                     goto error ;
                  }

                  rc = append( selected ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to append obj, rc=%d", rc ) ;
               }
               catch ( std::exception &e )
               {
                  rc = SDB_SYS ;
                  PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
                  goto error ;
               }

               rc = ctx->pop() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pop data, rc: %d", rc ) ;

               if ( _numToReturn > 0 )
               {
                  --_numToReturn ;
               }
            }
         }

         if ( ctx->recordNum() <= 0 )
         {
            rc = _saveEmptyNormalSubCtx( ctx ) ;
         }
         else
         {
            rc = _saveNonEmptyNormalSubCtx( ctx ) ;
         }

         if ( SDB_OK != rc )
         {
            SDB_OSS_DEL ctx ;
            goto error ;
         }

         break ;
      }

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextMain::_processSubContext ( rtnSubContext * subContext,
                                               BOOLEAN & skipData )
   {
      INT32 rc = SDB_OK ;

      if ( !_hasProcessor ||
           0 == subContext->recordNum() )
      {
         goto done ;
      }

      if ( subContext->getProcessType() > (INT64)RTN_CTX_PROCESSOR_BEGIN &&
           subContext->getProcessType() < (INT64)RTN_CTX_PROCESSOR_END )
      {
         rc = _processData( subContext->getProcessType(),
                            subContext->getDataID(),
                            subContext->front(),
                            subContext->remainLength(),
                            subContext->recordNum() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to process data, rc: %d", rc ) ;
         skipData = TRUE ;
      }
      else if ( needCheckData() )
      {
         rc = _processData( RTN_CTX_PROCESSOR_NONE,
                            subContext->getDataID(),
                            subContext->front(),
                            subContext->remainLength(),
                            subContext->recordNum() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check data, rc: %d", rc ) ;
      }

      if ( skipData )
      {
         rc = subContext->popAll() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to skip context, rc: %d", rc ) ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _rtnContextMain::_checkSubContext ( rtnSubContext * subContext )
   {
      INT32 rc = SDB_OK ;

      if ( !_hasProcessor || !_needCheckSubContext )
      {
         goto done ;
      }

      rc = _rtnCtxDataDispatcher::_checkSubContext( subContext->getDataID() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub context [%lld]",
                   subContext->getDataID() ) ;

   done :
      return rc ;

   error :
      goto done ;
   }

}
