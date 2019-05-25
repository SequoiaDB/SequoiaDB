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

   Source File Name = rtnCommandMon.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCommandMon.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "monDump.hpp"
#include "aggrDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _rtnMonInnerBase implement
   */
   _rtnMonInnerBase::_rtnMonInnerBase()
   :_matcherBuff ( NULL ), _selectBuff ( NULL ), _orderByBuff ( NULL ),
    _hintBuff( NULL )
   {
      _flags = 0 ;
      _numToReturn = -1 ;
      _numToSkip = 0 ;
   }

   _rtnMonInnerBase::~_rtnMonInnerBase()
   {
   }

   INT32 _rtnMonInnerBase::init ( INT32 flags, INT64 numToSkip,
                                  INT64 numToReturn,
                                  const CHAR * pMatcherBuff,
                                  const CHAR * pSelectBuff,
                                  const CHAR * pOrderByBuff,
                                  const CHAR * pHintBuff )
   {
      _flags = flags ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip ;
      _matcherBuff = pMatcherBuff ;
      _selectBuff = pSelectBuff ;
      _orderByBuff = pOrderByBuff ;
      _hintBuff = pHintBuff ;

      return SDB_OK ;
   }

   UINT32 _rtnMonInnerBase::_addInfoMask() const
   {
      return MON_MASK_FETCH_DEFAULT ;
   }

   BSONObj _rtnMonInnerBase::_getOptObj() const
   {
      return BSONObj() ;
   }

   INT32 _rtnMonInnerBase::_createFetch( _pmdEDUCB *cb,
                                         rtnFetchBase **ppFetch )
   {
      INT32 rc = SDB_OK ;
      _rtnFetchBuilder *pFetchBuilder = getRtnFetchBuilder() ;
      rtnFetchBase *pNewFetch = NULL ;

      pNewFetch = pFetchBuilder->create( _getFetchType() ) ;
      if ( !pNewFetch )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to alloc fetch[type:%d]",
                 _getFetchType() ) ;
         goto error ;
      }

      rc = pNewFetch->init( cb, _isCurrent(), _isDetail(),
                            _addInfoMask(), _getOptObj() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init fetch, rc: %d", rc ) ;
         goto error ;
      }

      *ppFetch = pNewFetch ;
      pNewFetch = NULL ;

   done:
      if ( pNewFetch )
      {
         pFetchBuilder->release( pNewFetch ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnMonInnerBase::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                 INT16 w, INT64 *pContextID )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;

      rtnContextDump *context = NULL ;
      rtnFetchBase *pFetch = NULL ;

      BSONObj matcher ;
      BSONObj selector ;
      BSONObj orderBy ;
      BSONObj hint ;

      try
      {
         BSONObj tmpMatcher ( _matcherBuff ) ;
         BSONObj nodeMatcher ;
         selector = BSONObj( _selectBuff ) ;
         orderBy = BSONObj( _orderByBuff ) ;
         hint = BSONObj( _hintBuff ) ;

         rc = parseMatcher( tmpMatcher, nodeMatcher, matcher, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse matcher failed, rc: %d", rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse param occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                               *pContextID, cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to create new context, rc: %d", rc ) ;
         goto error ;
      }

      rc = context->open( selector,
                          matcher,
                          orderBy.isEmpty() ? _numToReturn : -1,
                          orderBy.isEmpty() ? _numToSkip : 0 ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

      if ( cb->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      rc = _createFetch( cb, &pFetch ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create fetch, rc: %d", rc ) ;

      context->setMonFetch( pFetch, TRUE ) ;
      pFetch = NULL ;

      if ( !orderBy.isEmpty() )
      {
         rc = rtnSort( (rtnContext**)&context, orderBy, cb,
                       _numToSkip, _numToReturn, *pContextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to sort, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete( *pContextID, cb ) ;
         *pContextID = -1 ;
         context = NULL ;
      }
      if ( pFetch )
      {
         SDB_OSS_DEL pFetch ;
         pFetch = NULL ;
      }
      goto done ;
   }

   /*
      _rtnMonBase implement
   */
   _rtnMonBase::_rtnMonBase ()
   {
   }

   _rtnMonBase::~_rtnMonBase ()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSNAPSHOT_DOIT, "_rtnMonBase::doit" )
   INT32 _rtnMonBase::doit( _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                            SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                            INT16 w , INT64 *pContextID )
   {
      PD_TRACE_ENTRY ( SDB__RTNSNAPSHOT_DOIT ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( pContextID, "context id can't be NULL" ) ;

      INT32 rc = SDB_OK ;
      CHAR *pOutBuff = NULL ;
      INT32 buffSize = 0 ;
      INT32 buffUsedSize = 0 ;
      INT32 buffObjNum = 0 ;

      vector< BSONObj > vecUserAggr ;
      BSONObj matcher ;
      BSONObj selector ;
      BSONObj orderBy ;
      BSONObj hint ;

      try
      {
         hint = BSONObj( _hintBuff ) ;

         rc = parseUserAggr( hint, vecUserAggr ) ;
         PD_RC_CHECK( rc, PDERROR, "Parse user define aggr failed, rc: %d",
                      rc ) ;

         if ( vecUserAggr.size() > 0 )
         {
            BSONObj tmpMatcher ( _matcherBuff ) ;
            BSONObj nodeMatcher ;
            selector = BSONObj( _selectBuff ) ;
            orderBy = BSONObj( _orderByBuff ) ;

            rc = parseMatcher( tmpMatcher, nodeMatcher, matcher, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Parse matcher failed, rc: %d", rc ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse param occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( vecUserAggr.size() > 0 )
      {
         if ( !matcher.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_MATCH_PARSER_NAME << matcher ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append new matcher failed, rc: %d",
                         rc ) ;
         }

         if ( !orderBy.isEmpty() )
         {
            rc = appendObj( BSON( AGGR_SORT_PARSER_NAME << orderBy ),
                            pOutBuff, buffSize, buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append order by failed, rc: %d",
                         rc ) ;
         }

         for ( UINT32 i = 0 ; i < vecUserAggr.size() ; ++i )
         {
            rc = appendObj( vecUserAggr[ i ], pOutBuff, buffSize,
                            buffUsedSize, buffObjNum ) ;
            PD_RC_CHECK( rc, PDERROR, "Append user define aggr[%s] failed, "
                         "rc: %d", vecUserAggr[ i ].toString().c_str(),
                         rc ) ;
         }

         rc = openContext( pOutBuff, buffObjNum, getIntrCMDName(),
                           selector, cb, *pContextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;
      }
      else
      {
         rc = _rtnMonInnerBase::doit( cb, dmsCB, rtnCB,
                                      dpsCB, w, pContextID ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      if ( pOutBuff )
      {
         SDB_OSS_FREE( pOutBuff ) ;
         pOutBuff = NULL ;
         buffSize = 0 ;
         buffUsedSize = 0 ;
      }
      PD_TRACE_EXITRC ( SDB__RTNSNAPSHOT_DOIT, rc ) ;
      return rc ;
   error:
      if ( -1 != *pContextID )
      {
         rtnCB->contextDelete ( *pContextID, cb ) ;
         *pContextID = -1 ;
      }
      goto done ;
   }

}


