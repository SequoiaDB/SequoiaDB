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

   Source File Name = rtnContextSort.cpp

   Descriptive Name = RunTime Sort Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/29/2020   HGM Split from rtnContextData.cpp

   Last Changed =

*******************************************************************************/

#include "rtnContextData.hpp"
#include "rtnContextSort.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnContextSort implement
    */
   RTN_CTX_AUTO_REGISTER(_rtnContextSort, RTN_CONTEXT_SORT, "SORT")

   _rtnContextSort::_rtnContextSort( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _rtnSubContextHolder(),
    _keyBuilder( FALSE ),
    _dataSorted ( FALSE ),
    _numToSkip( 0 ),
    _numToReturn( -1 )
   {
      _enableMonContext = TRUE ;
      _enableQueryActivity = TRUE ;
   }

   _rtnContextSort::~_rtnContextSort()
   {
      setQueryActivity( _hitEnd ) ;
      _planRuntime.reset() ;
      _numToSkip = 0 ;
      _numToReturn = 0 ;
   }

   const CHAR* _rtnContextSort::name() const
   {
      return "SORT" ;
   }

   RTN_CONTEXT_TYPE _rtnContextSort::getType() const
   {
      return RTN_CONTEXT_SORT ;
   }

   INT32 _rtnContextSort::open( const BSONObj &orderby,
                                rtnContextPtr &context,
                                pmdEDUCB *cb,
                                SINT64 numToSkip,
                                SINT64 numToReturn )
   {
      SDB_ASSERT( !orderby.isEmpty(), "impossible" ) ;
      SDB_ASSERT( NULL != cb, "possible" ) ;
      SDB_ASSERT( context, "impossible" ) ;
      INT32 rc = SDB_OK ;
      UINT64 sortBufSz = sdbGetRTNCB()->getAPM()->getSortBufferSizeMB() ;
      SINT64 limit = numToReturn ;

      if ( 0 < limit && 0 < numToSkip )
      {
         limit += numToSkip ;
      }

      rc = _sorting.init( sortBufSz, orderby,
                          contextID(), limit, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init sort:%d", rc ) ;
         goto error ;
      }

      _returnOptions.setSkip( numToSkip ) ;
      _returnOptions.setLimit( numToReturn ) ;
      _numToSkip = numToSkip ;
      _numToReturn = numToReturn ;

      rc = _rebuildSrcContext( orderby, context ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rebuild src context:%d", rc ) ;
         goto error ;
      }

      if ( NULL != context->getPlanRuntime() )
      {
         _planRuntime.inheritRuntime( context->getPlanRuntime() ) ;
      }
      _setSubContext( context, cb ) ;
      _orderby = orderby.getOwned() ;

      rc = _keyGen.setKeyPattern( _orderby ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set key pattern, rc: %d", rc ) ;

      // reusable key builder
      _keyGen.setKeyBuilder( &_keyBuilder ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextSort::setQueryActivity ( BOOLEAN hitEnd )
   {
      if ( _planRuntime.canSetQueryActivity() &&
           enabledMonContext() &&
           enabledQueryActivity() )
      {
         _planRuntime.setQueryActivity( MON_SELECT, _monCtxCB, _returnOptions,
                                        hitEnd ) ;
      }
   }

   INT32 _rtnContextSort::_rebuildSrcContext( const BSONObj &orderBy,
                                              rtnContext *srcContext )
   {
      INT32 rc = SDB_OK ;
      const BSONObj &selector = srcContext->getSelector().getPattern() ;
      if ( selector.isEmpty() )
      {
         goto done ;
      }
      else
      {
         BOOLEAN needRebuild = FALSE ;
         if ( srcContext->getSelector().getStringOutput() )
         {
            needRebuild = TRUE ;
         }
         else
         {
            rtnGetMergedSelector( selector, orderBy, needRebuild ) ;
         }

         if ( needRebuild )
         {
            rc = srcContext->getSelector().move( _selector ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to rebuild selector:%d", rc ) ;
               goto error ;
            }
            _returnOptions.setSelector( _selector.getPattern() ) ;
         }
         else
         {
            _returnOptions.setSelector( selector ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextSort::_sortData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextBuf bufObj ;
      BSONObj obj ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         _getSubContext()->getMonCB()->recordStartTimestamp() ;
      }

      for ( ; ; )
      {
         rc = _getSubContext()->getMore( -1, bufObj, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            // sort data
            rc = _sorting.sort( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sort: %d", rc ) ;
               goto error ;
            }
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to getmore:%d", rc ) ;
            goto error ;
         }

         while ( SDB_OK == ( rc = bufObj.nextObj( obj ) ) )
         {
            BSONElement arrEle ;
            BSONObj keyObj ;
            rc = _keyGen.getKeys( obj, keyObj, &arrEle ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed gen sort keys:%d", rc ) ;
               goto error ;
            }

            SDB_ASSERT( !keyObj.isEmpty(), "can not be empty" ) ;
            rc = _sorting.push( keyObj,
                                obj.objdata(), obj.objsize(),
                                &arrEle, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to push obj: %d", rc ) ;
               goto error ;
            }
         }

         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "failed to get next obj from objBuf: %d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextSort::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      const INT32 maxNum = 1000000 ;
      const INT32 breakBufferSize = 2097152 ; /// 2MB
      const INT32 minRecordNum = 4 ;
      BSONObj key ;
      BSONObj obj ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_dataSorted )
      {
         rc = _sortData( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sort data:%d", rc ) ;
            goto error ;
         }
         _dataSorted = TRUE ;
      }

      for ( INT32 i = 0; i < maxNum; i++ )
      {
         const CHAR* objdata ;
         INT32 objlen ;
         rc = _sorting.fetch( key, &objdata, &objlen, cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to fetch from sorting:%d", rc ) ;
            goto error ;
         }
         else if ( 0 < _numToSkip )
         {
            -- _numToSkip ;
            /// wo do not want to break this loop when get nothing.
            -- i ;
            continue ;
         }
         else if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else
         {
            const BSONObj *record = NULL ;
            BSONObj selected ;
            obj = BSONObj( objdata ) ;
            if ( _selector.isInitialized() )
            {
               rc = _selector.select( obj, selected ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to select fields from obj:%d", rc ) ;
                  goto error ;
               }
               record = &selected ;
            }
            else
            {
               record = &obj ;
            }

            rc = append( *record ) ;
            PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                      obj.toString().c_str(), rc ) ;

            if ( 0 < _numToReturn )
            {
               -- _numToReturn ;
            }
         }

         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;

         if ( minRecordNum <= i && buffEndOffset() >= breakBufferSize )
         {
            break ;
         }

         if ( buffEndOffset() + DMS_RECORD_MAX_SZ > RTN_RESULTBUFFER_SIZE_MAX )
         {
            break ;
         }
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextSort::_toString( stringstream &ss )
   {
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
      if ( !_orderby.isEmpty() )
      {
         ss << ",Orderby:" << _orderby.toString().c_str() ;
      }
   }

}
