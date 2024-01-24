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
#include "rtnTrace.hpp"
#include "pmd.hpp"
#include "rtn.hpp"
#include "pd.hpp"

namespace engine
{

   #define RTN_INTERRUPT_CHECK_STEPS ( 5000 )

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
      unregisterAllProcessors() ;
      SDB_ASSERT( _orderedContexts.empty(),
                  "ordered contexts should be empty" ) ;
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

      rc = subCtx->genOrderKey() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get orderKey, rc = %d", rc ) ;
         goto error ;
      }

      try
      {
         _orderedContexts.insert( subCtx ) ;
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
      _releaseSubContext( subCtx ) ;
      goto done;
   }

   INT32 _rtnContextMain::_prepareDataByOrder( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;

      UINT32 interruptStep = 0 ;

      while ( 0 != _numToReturn )
      {
         if ( interruptStep > RTN_INTERRUPT_CHECK_STEPS )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
            interruptStep = 0 ;
         }
         else
         {
            ++ interruptStep ;
         }

         rc = _prepareAllSubCtxDataByOrder( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare all sub contexts' data, rc=%d", rc ) ;
            goto error ;
         }

         if ( _orderedContexts.size() == 0 )
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

         SUB_ORDERED_CTX_SET_IT iter = _orderedContexts.begin();
         if ( _orderedContexts.end() == iter )
         {
            _hitEnd = TRUE ;
            if ( isEmpty() )
            {
               rc = SDB_DMS_EOC;
            }
            break;
         }

         rtnSubContext* ctx = *iter ;

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
            _orderedContexts.erase ( iter ) ;

            rc = _saveEmptyOrderedSubCtx( ctx ) ;
            if ( SDB_OK != rc )
            {
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
            _orderedContexts.erase ( iter ) ;

            rc = _saveNonEmptyOrderedSubCtx( ctx ) ;
            if ( SDB_OK != rc )
            {
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

   INT32 _rtnContextMain::_getAdvanceOrderby( BSONObj &orderby,
                                              BOOLEAN isRange ) const
   {
      orderby = _options.getOrderBy() ;

      if ( orderby.isEmpty() )
      {
         PD_LOG_MSG( PDERROR, "Context does not support advance without "
                     "orderby" ) ;
         return SDB_OPTION_NOT_SUPPORT ;
      }

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAIN__DOADVANCE, "_rtnContextMain::_doAdvance" )
   INT32 _rtnContextMain::_doAdvance( INT32 type,
                                      INT32 prefixNum,
                                      const BSONObj &keyVal,
                                      const BSONObj &orderby,
                                      const BSONObj &arg,
                                      BOOLEAN isLocate,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXMAIN__DOADVANCE ) ;

      if ( isLocate )
      {
         goto done ;
      }

      // check keydef is the prefix match order by
      try
      {
         LST_SUB_CTX_PTR lstCtx ;

         /// 1. process cache data
         rc = _processCacheData( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Process cache data failed, rc: %d", rc ) ;
            goto error ;
         }

         /// 2. check order context
         rc = _checkOrderCtxsAdvance( type, prefixNum, keyVal, orderby ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Check order-contexts advance failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         /// 3. prepare sub contexts
         rc = _prepareSubCtxsAdvance( lstCtx ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Prepare sub contexts advance failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         // 4. do sub contexts
         rc = _doSubCtxsAdvance( lstCtx, arg, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Do sub contexts advance failed, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXMAIN__DOADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextMain::_checkOrderCtxsAdvance( INT32 type,
                                                  INT32 prefixNum,
                                                  const BSONObj &keyVal,
                                                  const BSONObj &orderby )
   {
      INT32 rc = SDB_OK ;

      ossPoolList< rtnSubContext* > tmpList ;

      try
      {
         SUB_ORDERED_CTX_SET_IT itOrder ;
         BOOLEAN processed = FALSE ;
         ixmIndexKeyGen keyGen( orderby ) ;

         itOrder = _orderedContexts.begin() ;
         while( itOrder != _orderedContexts.end() )
         {
            rtnSubContext *pSubCtx = *itOrder ;
            rc = _checkSubContextAdvance( pSubCtx, keyGen, type,
                                          prefixNum, keyVal, orderby,
                                          processed ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Check sub-context advance failed, rc: %d",
                       rc ) ;
               goto error ;
            }

            _orderedContexts.erase( itOrder ++ ) ;
            if ( !processed )
            {
               ///  save empty
               SDB_ASSERT( 0 == pSubCtx->recordNum(),
                           "Sub-context must be empty" ) ;
               rc = _saveEmptyOrderedSubCtx( pSubCtx ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Save empty ordered sub-context failed, "
                          "rc: %d", rc ) ;
                  goto error ;
               }
            }
            else
            {
               try
               {
                  tmpList.push_back( pSubCtx ) ;
               }
               catch ( exception &e )
               {
                  _releaseSubContext( pSubCtx ) ;
                  PD_LOG( PDERROR, "Failed to save sub context, "
                          "occur exception %s", e.what() ) ;
                  rc = ossException2RC( &e ) ;
                  goto error ;
               }
            }
         }

         // data had been pop, so need generate new keys, and save back
         // ordered contexts
         while ( !( tmpList.empty() ) )
         {
            rtnSubContext *subCtx = tmpList.front() ;
            tmpList.pop_front() ;

            rc = _saveNonEmptyOrderedSubCtx( subCtx ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save ordered context, "
                         "rc: %d", rc ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      for ( ossPoolList< rtnSubContext* >::iterator iter = tmpList.begin() ;
            iter != tmpList.end() ;
            ++ iter )
      {
         rtnSubContext *ctx = *iter ;
         _releaseSubContext( ctx ) ;
      }
      tmpList.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAIN__CHECKSUBCTXADVANCE, "_rtnContextMain::_checkSubContextAdvance" )
   INT32 _rtnContextMain::_checkSubContextAdvance( rtnSubContext *pSubCtx,
                                                   ixmIndexKeyGen &keyGen,
                                                   INT32 type,
                                                   INT32 prefixNum,
                                                   const BSONObj &keyVal,
                                                   const BSONObj &orderby,
                                                   BOOLEAN &processed )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXMAIN__CHECKSUBCTXADVANCE ) ;

      BOOLEAN matched = FALSE ;
      BOOLEAN isEqual = FALSE ;

      processed = FALSE ;

      // check sub context's data
      while( pSubCtx->recordNum() > 0 )
      {
         BSONObj obj( pSubCtx->front() ) ;

         rc = _checkAdvance( type, keyGen, prefixNum, keyVal,
                             obj, orderby, matched, isEqual ) ;
         if ( rc )
         {
            goto error ;
         }
         else if ( matched )
         {
            processed = TRUE ;
            break ;
         }

         rc = pSubCtx->pop() ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXMAIN__CHECKSUBCTXADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}
