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

   Source File Name = rtnContextMainCL.cpp

   Descriptive Name = RunTime MainCL Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextMainCL.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   _rtnSubCLContext::_rtnSubCLContext( const BSONObj& orderBy,
                                       _ixmIndexKeyGen* keyGen,
                                       INT64 contextID )
      : _rtnSubContext( orderBy, keyGen, contextID )
   {
      _hitEnd = FALSE ;
   }

   _rtnSubCLContext::~_rtnSubCLContext()
   {
   }

   const CHAR* _rtnSubCLContext::front()
   {
      return _buffer.front();
   }

   INT32 _rtnSubCLContext::pop()
   {
      INT32 rc = SDB_OK ;

      if ( _buffer.recordNum() > 0 )
      {
         rc = _buffer.pop() ;
         if ( SDB_OK == rc )
         {
            _isOrderKeyChange = TRUE ;
         }
      }

      return rc ;
   }

   INT32 _rtnSubCLContext::pushFront( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      rc = _buffer.pushFront( obj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Push front object failed, rc: %d", rc ) ;
         goto error ;
      }

      _isOrderKeyChange = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSubCLContext::popN( INT32 num )
   {
      INT32 rc = SDB_OK;
      _isOrderKeyChange = TRUE;
      if ( num >= recordNum() )
      {
         rc = popAll();
         goto done;
      }
      while ( num > 0 )
      {
         rc = pop();
         if ( rc )
         {
            goto error;
         }
         --num;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnSubCLContext::popAll()
   {
      _isOrderKeyChange = TRUE;
      rtnContextBuf emptyBuf;
      _buffer = emptyBuf;
      return SDB_OK;
   }

   INT32 _rtnSubCLContext::recordNum()
   {
      return _buffer.recordNum() ;
   }

   INT32 _rtnSubCLContext::remainLength()
   {
      return _buffer.size() - _buffer.offset() ;
   }

   INT32 _rtnSubCLContext::truncate ( INT32 num )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( num >= 0, "num can't <0 " ) ;
      rc = _buffer.truncate( (UINT32)num  ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate context buffer failed, rc: %d",
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnSubCLContext::genOrderKey()
   {
      INT32 rc = SDB_OK;
      if ( _isOrderKeyChange )
      {
         if ( recordNum() <= 0 )
         {
            _orderKey.clear();
         }
         else
         {
            try
            {
               BSONObj boRecord( front() ) ;
               rc = _orderKey.generateKey( boRecord, _keyGen ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get order-key(rc=%d)",
                            rc );
            }
            catch ( std::exception &e )
            {
               PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                            "Occur unexpected error:%s", e.what() ) ;
            }
         }
      }
      _isOrderKeyChange = FALSE ;

   done:
      return rc;
   error:
      goto done;
   }

   rtnContextBuf _rtnSubCLContext::buffer()
   {
      return _buffer;
   }

   void _rtnSubCLContext::setBuffer( rtnContextBuf &buffer )
   {
      _buffer = buffer;
      _isOrderKeyChange = TRUE;
      _startFrom = buffer.getStartFrom() ;
   }

   void _rtnSubCLContext::releaseBuffer()
   {
      _buffer.release() ;
   }

   RTN_CTX_AUTO_REGISTER(_rtnContextMainCL, RTN_CONTEXT_MAINCL, "MAINCL")

   _rtnContextMainCL::_rtnContextMainCL( INT64 contextID, UINT64 eduID )
      : _rtnContextMain( contextID, eduID ),
        _includeShardingOrder( FALSE )
   {
      _isWrite = FALSE ;
   }

   _rtnContextMainCL::~_rtnContextMainCL()
   {
      _deleteSubContexts() ;
   }

   const CHAR* _rtnContextMainCL::name() const
   {
      return "MAINCL" ;
   }

   RTN_CONTEXT_TYPE _rtnContextMainCL::getType () const
   {
      return RTN_CONTEXT_MAINCL;
   }

   INT32 _rtnContextMainCL::open( const rtnQueryOptions &options,
                                  const CLS_SUBCL_LIST &subs,
                                  BOOLEAN shardSort,
                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = _initArgs( options, subs, shardSort ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init args:%d", rc ) ;
         goto error ;
      }

      rc = _initSubCLContext( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init cl buf:%d", rc ) ;
         goto error ;
      }

      _isOpened = TRUE ;
      _hitEnd = ( 0 == _options.getLimit() ) ||
                ( _subContextMap.empty() ) ;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextMainCL::open( const CHAR *mainCLName,
                                  const bson::BSONObj & orderBy,
                                  INT64 numToReturn,
                                  INT64 numToSkip )
   {
      INT32 rc = SDB_OK ;

      _options.setCLFullName( mainCLName ) ;

      _options.setOrderBy( orderBy ) ;
      _options.setSkip( numToSkip ) ;
      _options.setLimit( numToReturn ) ;

      rc = _options.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get query options owned, "
                   "rc: %d", rc ) ;

      _keyGen = SDB_OSS_NEW _ixmIndexKeyGen( _options.getOrderBy() ) ;
      PD_CHECK( _keyGen != NULL, SDB_OOM, error, PDERROR,
                "Failed to allocate index key generator" ) ;

      _numToSkip = _options.getSkip() ;
      _numToReturn = _options.getLimit() ;

      _isOpened = TRUE;
      _hitEnd = ( 0 == numToReturn ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnContextMainCL::_initSubCLContext( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 loop = 0 ;
      SINT64 contextID = -1 ;

      loop = requireOrder() ? _subs.size() : 5 ;

      while ( 0 < loop-- )
      {
         contextID = -1 ;
         rc = _getNextContext( cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get next context:%d", rc ) ;
            goto error ;
         }

         if ( -1 == contextID )
         {
            break ;
         }
         else
         {
            rc = addSubContext( contextID ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add context to main context:%d", rc ) ;
               goto error ;
            }
         }
      }
   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         sdbGetRTNCB()->contextDelete( contextID, cb ) ;
      }
      goto done ;
   }

   INT32 _rtnContextMainCL::_getNextContext( _pmdEDUCB *cb,
                                             INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      INT64 context = -1 ;
      _SDB_RTNCB *rtnCB = sdbGetRTNCB() ;
      rtnContextPtr contextObj ;
      if ( !_subs.empty() )
      {
         // Construct query options of sub-collection
         const string &clName = *( _subs.begin() ) ;
         rtnQueryOptions subCLOptions( _options ) ;
         subCLOptions.setMainCLQuery( _options.getCLFullName(),
                                      clName.c_str() ) ;

         rc = rtnQuery( subCLOptions, cb, sdbGetDMSCB(), rtnCB, context,
                        &contextObj, TRUE, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to query on cl:%s, rc:%d",
                    clName.c_str(), rc ) ;
            goto error ;
         }

         if ( contextObj && contextObj->isWrite() )
         {
            _isWrite = TRUE ;
            contextObj->setWriteInfo( this->getDPSCB(),
                                      this->getW() ) ;
         }

         _subs.pop_front() ;
         /// do not use clName again.

         if ( contextObj && !_advancePosition.isEmpty() )
         {
            rc = contextObj->advance( _advancePosition, NULL, 0, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Advance context failed, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      contextID = context ;
      return rc ;
   error:
      if ( -1 != context )
      {
         rtnCB->contextDelete( context, cb ) ;
         context = -1 ;
      }
      goto done ;
   }

   INT32 _rtnContextMainCL::_initArgs( const _rtnQueryOptions &options,
                                       const CLS_SUBCL_LIST &subs,
                                       BOOLEAN shardSort )
   {
      INT32 rc = SDB_OK ;

      _options = options ;
      rc = _options.getOwned() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get query options owned, "
                   "rc: %d", rc ) ;

      _numToReturn = _options.getLimit() ;
      _numToSkip = _options.getSkip() ;
      _includeShardingOrder = shardSort ;

      /// _options._skip will be used in sub query.
      _options.setSkip( 0 ) ;
      _keyGen = SDB_OSS_NEW _ixmIndexKeyGen( _options.getOrderBy() ) ;
      PD_CHECK( _keyGen != NULL, SDB_OOM, error, PDERROR,
                "malloc failed!" ) ;

      if ( subs.size() <= 1 )
      {
         _includeShardingOrder = FALSE ;
         _options.setSkip( _numToSkip ) ;
         _numToSkip = 0 ;
      }
      else
      {
         if ( 0 < _numToSkip && 0 < _numToReturn )
         {
            _options.setLimit( _numToSkip + _numToReturn ) ;
         }
      }

      for ( CLS_SUBCL_LIST::const_iterator itr = subs.begin() ;
            itr != subs.end() ;
            ++itr )
      {
         _subs.push_back( *itr ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextMainCL::addSubContext( INT64 contextID )
   {
      INT32 rc = SDB_OK ;
      SUBCL_CTX_MAP::iterator iter ;
      rtnSubCLContext* subCtx = NULL ;

      iter = _subContextMap.find( contextID ) ;
      if ( iter != _subContextMap.end() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Repeat to add sub-context (ContextID=%lld)",
                 contextID ) ;
         goto error ;
      }

      subCtx = SDB_OSS_NEW _rtnSubCLContext( _options.getOrderBy(), _keyGen,
                                             contextID ) ;
      if ( NULL == subCtx )
      {
         rc = SDB_OOM;
         PD_LOG ( PDERROR, "Failed to alloc subcl context" ) ;
         goto error ;
      }

      try
      {
         _subContextMap.insert( SUBCL_CTX_MAP::value_type( contextID, subCtx ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save sub context, occur exception %s",
                 e.what() ) ;
         _releaseSubContext( subCtx ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _checkSubContext( subCtx ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check sub context, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCL_PREPARESUBDATA, "_rtnContextMainCL::_prepareSubCLData" )
   INT32 _rtnContextMainCL::_prepareSubCLData( SINT64 contextID,
                                               _pmdEDUCB * cb,
                                               INT32 maxNumToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXMAINCL_PREPARESUBDATA ) ;

      _SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB();
      rtnContextPtr pContext ;
      rtnContextBuf contextBuf;
      SUBCL_CTX_MAP::iterator iterSubCTX = _subContextMap.find( contextID ) ;
      _rtnSubCLContext *subCtx = NULL ;
      if ( _subContextMap.end() == iterSubCTX )
      {
         PD_LOG( PDERROR, "can not find context[%lld] in local context buf",
                 contextID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( iterSubCTX->second->recordNum() > 0 )
      {
         goto done;
      }

      rc = pRtnCB->contextFind( contextID, pContext );
      PD_RC_CHECK( rc, PDERROR, "Context %lld does not exist, rc: %d",
                   contextID, rc ) ;

      subCtx = iterSubCTX->second ;

      for ( ; ; )
      {
         rc = pContext->getMore( maxNumToReturn, contextBuf, cb ) ;
         if ( SDB_OK == rc )
         {
            BOOLEAN skipBuffer = FALSE ;
            subCtx->setBuffer( contextBuf ) ;
            rc = _processSubContext( subCtx, skipBuffer ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to process sub-context [%lld], "
                       "rc: %d", subCtx->contextID(), rc ) ;
               sdbGetRTNCB()->contextDelete( subCtx->contextID(), cb ) ;
               goto error ;
            }
            if ( skipBuffer )
            {
               subCtx->popAll() ;
               continue ;
            }
         }
         break ;
      }

      if ( SDB_DMS_EOC == rc || pContext->eof() )
      {
         INT32 rcTmp = SDB_OK ;
         SINT64 nextContextID = -1 ;

         // set sub-context hit end
         subCtx->setHitEnd() ;

         rcTmp = _getNextContext( cb, nextContextID ) ;
         if ( SDB_OK != rcTmp )
         {
            PD_LOG( PDERROR, "failed to get next context:%d", rcTmp ) ;
            rc = rcTmp ;
            goto error ;
         }
         else if ( -1 != nextContextID )
         {
            rcTmp = addSubContext( nextContextID ) ;
            if ( SDB_OK != rcTmp )
            {
               PD_LOG( PDERROR, "failed to add context:%d", rc ) ;
               rc = rcTmp ;
               sdbGetRTNCB()->contextDelete( nextContextID, cb ) ;
               goto error ;
            }
         }
         else
         {
            SDB_ASSERT( _subs.empty(), "must be empty" ) ;
            /// do nothing.
         }
      }
      else if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "getmore failed(rc=%d)", rc );
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCL_PREPARESUBDATA, rc ) ;
      return rc;
   error:
      goto done;
   }

   void _rtnContextMainCL::_toString( stringstream &ss )
   {
      if ( !_options.isOrderByEmpty() )
      {
         ss << ",Orderby:" << _options.getOrderBy().toString().c_str()
            << ",IsShardingOrder:" << _includeShardingOrder ;
      }
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCL_PREPAREALLSUBCTXDATA, "_rtnContextMainCL::_prepareAllSubCtxDataByOrder" )
   INT32 _rtnContextMainCL::_prepareAllSubCtxDataByOrder( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXMAINCL_PREPAREALLSUBCTXDATA ) ;

      _SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB();

      if ( !_subs.empty() )
      {
         SDB_ASSERT( FALSE, "should be empty" ) ;
         PD_LOG( PDERROR, "subs should be empty" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _subContextMap.size() == 0 )
      {
         goto done ;
      }

      for ( SUBCL_CTX_MAP::iterator iter = _subContextMap.begin() ;
            iter != _subContextMap.end() ; )
      {
         rtnSubCLContext* subCtx = iter->second ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( subCtx->recordNum() <= 0 )
         {
            rtnContextBuf contextBuf;
            rtnContextPtr rtnCtx ;
            rc = rtnCB->contextFind( subCtx->contextID(), rtnCtx );
            PD_RC_CHECK( rc, PDERROR, "Context %lld does not exist, rc: %d",
                         subCtx->contextID(), rc );

            for ( ; ; )
            {
               rc = rtnCtx->getMore( -1, contextBuf, cb );
               if ( SDB_OK == rc )
               {
                  BOOLEAN skipBuffer = FALSE ;
                  subCtx->setBuffer( contextBuf ) ;
                  rc = _processSubContext( subCtx, skipBuffer ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to process sub-context [%lld], "
                             "rc: %d", subCtx->contextID(), rc ) ;
                     sdbGetRTNCB()->contextDelete( subCtx->contextID(), cb ) ;
                     goto error ;
                  }
                  if ( skipBuffer )
                  {
                     continue ;
                  }
               }
               break ;
            }

            if ( SDB_DMS_EOC == rc )
            {
               _releaseSubContext( subCtx ) ;
               _subContextMap.erase( iter++ ) ;
               rc = SDB_OK ;
               continue ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "getmore failed(rc=%d)", rc );
               goto error;
            }
            else if ( rtnCtx->eof() )
            {
               subCtx->setHitEnd() ;
            }
         }

         SDB_ASSERT( subCtx->recordNum() > 0, "no data for sub ctx" ) ;

         _subContextMap.erase( iter++ ) ;

         rc = _saveNonEmptyOrderedSubCtx( subCtx ) ;
         if ( rc != SDB_OK )
         {
            PD_LOG ( PDERROR, "Failed to get orderKey failed, rc: %d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCL_PREPAREALLSUBCTXDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextMainCL::_getNonEmptyNormalSubCtx( _pmdEDUCB* cb, rtnSubContext*& subCtx )
   {
      INT32 rc = SDB_OK ;

      subCtx = NULL ;

      for ( ;; )
      {
         SUBCL_CTX_MAP::iterator iter = _subContextMap.begin();
         if ( _subContextMap.end() == iter )
         {
            rc = SDB_DMS_EOC;
            goto error ;
         }

         rtnSubCLContext* ctx = iter->second ;

         if ( ctx->recordNum() <= 0 )
         {
            rc = _prepareSubCLData( ctx->contextID(), cb, -1 ) ;
            if ( rc != SDB_OK )
            {
               _subContextMap.erase( ctx->contextID() );
               _releaseSubContext( ctx ) ;
               if ( SDB_DMS_EOC != rc )
               {
                  goto error;
               }
               else
               {
                  rc = SDB_OK ;
                  continue ;
               }
            }
         }

         subCtx = ctx ;
         break ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _rtnContextMainCL::_saveEmptyOrderedSubCtx( rtnSubContext* subCtx )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != subCtx, "subCtx should be not null" ) ;

      _rtnSubCLContext *tmpCtx = dynamic_cast<_rtnSubCLContext*>( subCtx ) ;
      SDB_ASSERT( NULL != tmpCtx, "sub-context is invalid" ) ;

      if ( tmpCtx->isHitEnd() )
      {
         // move from ordered context map
         // no need to erase from sub-context map
         _releaseSubContext( tmpCtx ) ;
      }
      else
      {
         // make sure all data are popped
         tmpCtx->releaseBuffer() ;

         // save to context map
         try
         {
            _subContextMap.insert(
               SUBCL_CTX_MAP::value_type( subCtx->contextID(), tmpCtx ) ) ;
         }
         catch( std::exception& e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "occur unexpected error:%s", e.what() );
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      _releaseSubContext( tmpCtx ) ;
      goto done ;
   }

   INT32 _rtnContextMainCL::_saveEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      SDB_ASSERT( NULL != subCtx, "subCtx can't be NULL" ) ;
      SDB_ASSERT( subCtx->recordNum() == 0, "sub ctx is not empty" ) ;

      _rtnSubCLContext *tmpCtx = dynamic_cast<_rtnSubCLContext*>( subCtx ) ;
      SDB_ASSERT( NULL != tmpCtx, "sub-context is invalid" ) ;

      // normal sub ctx is in _subContextMap,
      // if sub-context is ended, remove it from context map
      if ( tmpCtx->isHitEnd() )
      {
         _subContextMap.erase( subCtx->contextID() ) ;
         _releaseSubContext( subCtx ) ;
      }
      else
      {
         // make sure all data are popped
         tmpCtx->releaseBuffer() ;
      }

      return SDB_OK ;
   }

   INT32 _rtnContextMainCL::_saveNonEmptyNormalSubCtx( rtnSubContext* subCtx )
   {
      SDB_ASSERT( NULL != subCtx, "subCtx can't be NULL" ) ;
      SDB_ASSERT( subCtx->recordNum() > 0, "sub ctx is empty" ) ;

      // normal sub ctx is in _subContextMap,
      // no need to do anything
      return SDB_OK ;
   }

   INT32 _rtnContextMainCL::_doAfterPrepareData( _pmdEDUCB *cb )
   {
      if ( _subs.empty() &&
           _subContextMap.empty() &&
           _orderedContexts.empty() )
      {
         _hitEnd = TRUE ;
      }
      return SDB_OK ;
   }

   void _rtnContextMainCL::_deleteSubContexts ()
   {
      // clean normal context
      for ( SUBCL_CTX_MAP::iterator iter = _subContextMap.begin() ;
            iter != _subContextMap.end() ;
            ++ iter )
      {
         _releaseSubContext( iter->second ) ;
      }
      _subContextMap.clear();

      for ( SUB_ORDERED_CTX_SET_IT iter = _orderedContexts.begin() ;
            iter != _orderedContexts.end() ;
            ++ iter )
      {
         _releaseSubContext( *iter ) ;
      }
      _orderedContexts.clear() ;
   }

   INT32 _rtnContextMainCL::_prepareSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx )
   {
      INT32 rc = SDB_OK ;
      SUBCL_CTX_MAP::iterator it ;

      try
      {
         it = _subContextMap.begin() ;
         while( it != _subContextMap.end() )
         {
            lstCtx.push_back( it->second ) ;
            ++it ;
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
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCL_DOSUBCTXSADVANCE, "_rtnContextMainCL::_doSubCtxsAdvance" )
   INT32 _rtnContextMainCL::_doSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx,
                                               const BSONObj &arg,
                                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXMAINCL_DOSUBCTXSADVANCE ) ;

      _SDB_RTNCB *pRtnCB = pmdGetKRCB()->getRTNCB() ;
      rtnSubContext *pSubCtx = NULL ;

      LST_SUB_CTX_PTR::iterator it = lstCtx.begin() ;
      while( it != lstCtx.end() )
      {
         rtnContextPtr pContext ;
         pSubCtx = *it ;
         ++it ;
         rc = pRtnCB->contextFind( pSubCtx->contextID(), pContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Context %lld does not exist, rc: %d",
                    pSubCtx->contextID(), rc ) ;
            goto error ;
         }

         rc = pContext->advance( arg, NULL, 0, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCL_DOSUBCTXSADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCL__PRERELEASESUBCTX, "_rtnContextMainCL::_preReleaseSubContext" )
   void _rtnContextMainCL::_preReleaseSubContext( rtnSubContext *subCtx )
   {
      PD_TRACE_ENTRY( SDB_RTNCTXMAINCL__PRERELEASESUBCTX ) ;

      if ( NULL != subCtx && -1 != subCtx->contextID() )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         rtnCB->contextDelete( subCtx->contextID(), pmdGetThreadEDUCB() ) ;
      }

      PD_TRACE_EXIT( SDB_RTNCTXMAINCL__PRERELEASESUBCTX ) ;
   }

   /*
      _rtnContextMainCLExplain implement
    */
   RTN_CTX_AUTO_REGISTER( _rtnContextMainCLExplain, RTN_CONTEXT_MAINCL_EXP, "MAINCL_EXPLAIN" )

   _rtnContextMainCLExplain::_rtnContextMainCLExplain ( INT64 contextID,
                                                        UINT64 eduID )
   : _rtnContextBase( contextID, eduID ),
     _explainMergePath()
   {
   }

   _rtnContextMainCLExplain::~_rtnContextMainCLExplain ()
   {
   }

   const CHAR* _rtnContextMainCLExplain::name () const
   {
      return "MAINCL_EXPLAIN" ;
   }

   RTN_CONTEXT_TYPE _rtnContextMainCLExplain::getType () const
   {
      return RTN_CONTEXT_MAINCL_EXP ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCLEXP_OPEN, "_rtnContextMainCLExplain::open" )
   INT32 _rtnContextMainCLExplain::open ( const rtnQueryOptions & options,
                                          const CLS_SUBCL_LIST & subCollections,
                                          BOOLEAN shardSort,
                                          pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXMAINCLEXP_OPEN ) ;

      _subCollections = subCollections ;
      _shardSort = shardSort ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      rc = _openExplain( options, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open explain, rc: %d", rc ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCLEXP_OPEN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCLEXP__PREPAREDATA, "_rtnContextMainCLExplain::_prepareData" )
   INT32 _rtnContextMainCLExplain::_prepareData ( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXMAINCLEXP__PREPAREDATA ) ;

      rc = _prepareExplain( this, cb ) ;
      if ( SDB_DMS_EOC != rc &&
           SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare explain, rc: %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCLEXP__PREPAREDATA, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCLEXP__OPENSUBCTX, "_rtnContextMainCLExplain::_openSubContext" )
   INT32 _rtnContextMainCLExplain::_openSubContext ( rtnQueryOptions & options,
                                                     pmdEDUCB * cb,
                                                     rtnContextPtr *ppContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXMAINCLEXP__OPENSUBCTX ) ;

      SDB_ASSERT( ppContext, "context pointer is invalid" ) ;

      SDB_RTNCB * rtnCB = sdbGetRTNCB() ;

      INT64 queryContextID = -1 ;
      rtnContextMainCL::sharePtr queryContext ;

      rc = rtnCB->contextNew( RTN_CONTEXT_MAINCL, queryContext,
                              queryContextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new main-collection "
                   "context, rc: %d", rc ) ;

      PD_CHECK( queryContext, SDB_SYS, error, PDERROR,
                "Failed to get the context of query" ) ;

      if ( options.canPrepareMore() )
      {
         queryContext->setPrepareMoreData( TRUE ) ;
      }

      rc = _registerExplainProcessor( queryContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to register explain processor, "
                   "rc: %d", rc ) ;

      rc = queryContext->open( options, _subCollections, _shardSort, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open main-collection context, "
                   "rc: %d", rc ) ;

      rc = _explainMergePath.createMergePath( queryContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create MERGE node, rc: %d", rc ) ;

      _explainMergePath.setCollectionName( options.getCLFullName() ) ;

      if ( _expOptions.isNeedRun() )
      {
         queryContext->setEnableMonContext( TRUE ) ;
      }

      if ( NULL != ppContext )
      {
         *ppContext = queryContext ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCLEXP__OPENSUBCTX, rc ) ;
      return rc ;

   error :
      if ( -1 != queryContextID )
      {
         rtnCB->contextDelete( queryContextID, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXMAINCLEXP__PREPAREEXPPATH, "_rtnContextMainCLExplain::_prepareExplainPath" )
   INT32 _rtnContextMainCLExplain::_prepareExplainPath ( rtnContext * context,
                                                         pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_RTNCTXMAINCLEXP__PREPAREEXPPATH ) ;

      SDB_ASSERT( NULL != context, "query context is invalid" ) ;

      if ( _expOptions.isNeedDetail() )
      {
         rc = _explainMergePath.evaluate() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to evaluate MERGE path, "
                      "rc: %d", rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_RTNCTXMAINCLEXP__PREPAREEXPPATH, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXMAINCLEXP__BLDSIMPEXP, "_rtnContextMainCLExplain::_buildSimpleExplain" )
   INT32 _rtnContextMainCLExplain::_buildSimpleExplain ( rtnContext * explainContext,
                                                         BOOLEAN & hasMore )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXMAINCLEXP__BLDSIMPEXP ) ;

      BSONObjBuilder builder ;

      rc = _buildBSONNodeInfo( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for node info, "
                   "rc: %d", rc ) ;

      rc = _explainMergePath.toSimpleBSON( builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build simple explain result, "
                   "rc: %d", rc ) ;

      rc = explainContext->append( builder.obj() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed append explain result to context "
                   "[%lld], rc: %d", explainContext->contextID(), rc ) ;

      hasMore = FALSE ;

   done :
      PD_TRACE_EXITRC( SDB_CTXMAINCLEXP__BLDSIMPEXP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CTXMAINCLEXP__PARSELOCFILTER, "_rtnContextMainCLExplain::_parseLocationOption" )
   INT32 _rtnContextMainCLExplain::_parseLocationOption ( const BSONObj & explainOptions,
                                                          BOOLEAN & hasOption )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CTXMAINCLEXP__PARSELOCFILTER ) ;

      try
      {
         hasOption = FALSE ;

         // Get sub-collections option
         BSONElement ele = explainOptions.getField( FIELD_NAME_SUB_COLLECTIONS ) ;
         if ( ele.eoo() )
         {
            if ( explainOptions.hasField( FIELD_NAME_CMD_LOCATION ) ||
                 explainOptions.hasField( FIELD_NAME_LOCATION ) )
            {
               // The MERGE doesn't need "Location" option,
               // but it need to make sure "Detail" option is enabled
               hasOption = TRUE ;
            }
            goto done ;
         }
         else if ( Array == ele.type() )
         {
            BSONObjIterator iter( ele.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement subEle = iter.next() ;
               if ( String == subEle.type() )
               {
                  _subCollectionFilter.insert( subEle.valuestrsafe() ) ;
               }
            }
         }
         else if ( String == ele.type() )
         {
            _subCollectionFilter.insert( ele.valuestrsafe() ) ;
         }

         hasOption = TRUE ;
      }
      catch ( std::exception & e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CTXMAINCLEXP__PARSELOCFILTER, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   BOOLEAN _rtnContextMainCLExplain::_needChildExplain ( INT64 dataID,
                                                         const BSONObj & childExplain )
   {
      if ( _subCollectionFilter.size() > 0 )
      {
         BSONElement nameEle = childExplain.getField( OPT_FIELD_COLLECTION ) ;
         if ( String == nameEle.type() )
         {
            if ( _subCollectionFilter.end() ==
                 _subCollectionFilter.find( nameEle.String() ) )
            {
               return FALSE ;
            }
         }
      }
      return TRUE ;
   }

}
