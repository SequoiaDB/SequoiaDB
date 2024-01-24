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

   Source File Name = aggrBuilder.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#include "aggrBuilder.hpp"
#include "qgmPlanContainer.hpp"
#include "aggrDef.hpp"
#include "optQgmOptimizer.hpp"
#include "qgmBuilder.hpp"
#include "rtnCB.hpp"
#include "rtnContextDump.hpp"
#include "rtnContextQGM.hpp"
#include "pmd.hpp"
#include "aggrGroup.hpp"
#include "aggrMatcher.hpp"
#include "aggrSkip.hpp"
#include "aggrLimit.hpp"
#include "aggrSort.hpp"
#include "aggrProject.hpp"
#include "monDump.hpp"
#include "aggrTrace.h"

using namespace bson;

namespace engine
{
   AGGR_PARSER_BEGIN
   AGGR_PARSER_ADD( AGGR_GROUP_PARSER_NAME, aggrGroupParser )
   AGGR_PARSER_ADD( AGGR_MATCH_PARSER_NAME, aggrMatchParser )
   AGGR_PARSER_ADD( AGGR_SKIP_PARSER_NAME, aggrSkipParser )
   AGGR_PARSER_ADD( AGGR_LIMIT_PARSER_NAME, aggrLimitParser )
   AGGR_PARSER_ADD( AGGR_SORT_PARSER_NAME, aggrSortParser )
   AGGR_PARSER_ADD( AGGR_PROJECT_PARSER_NAME, aggrProjectParser )
   AGGR_PARSER_END

   // PD_TRACE_DECLARE_FUNCTION ( SDB_AGGRBUILDER_AGGRBUILDER, "aggrBuilder::aggrBuilder" )
   aggrBuilder::aggrBuilder()
   {
      PD_TRACE_ENTRY ( SDB_AGGRBUILDER_AGGRBUILDER ) ;
      addParser();
      PD_TRACE_EXIT ( SDB_AGGRBUILDER_AGGRBUILDER ) ;
   }

   aggrBuilder::~aggrBuilder()
   {
      AGGR_PARSER_MAP::iterator iterMap = _parserMap.begin();
      while( iterMap != _parserMap.end() )
      {
         SDB_OSS_DEL iterMap->second;
         ++iterMap;
      }
   }

   INT32 aggrBuilder::init ()
   {
      return SDB_OK ;
   }

   INT32 aggrBuilder::active ()
   {
      return SDB_OK ;
   }

   INT32 aggrBuilder::deactive ()
   {
      return SDB_OK ;
   }

   INT32 aggrBuilder::fini ()
   {
      return SDB_OK ;
   }

   INT32 aggrBuilder::build( const BSONObj &objs,
                             INT32 objNum,
                             const CHAR *pCLName,
                             const BSONObj &hint,
                             _pmdEDUCB *cb,
                             SINT64 &contextID,
                             INT32  clientVer,
                             INT32* pCataVer )
   {
      INT32 rc = SDB_OK;
      qgmOptiTreeNode *pOptiTree = NULL;
      qgmOptiTreeNode *pExtend = NULL;
      qgmPlanContainer *pContainer = NULL;
      BOOLEAN containerOwnned = TRUE ;

      PD_CHECK( objNum > 0, SDB_INVALIDARG, error, PDERROR,
               "input error, no object input!" );

      pContainer = SDB_OSS_NEW _qgmPlanContainer() ;
      PD_CHECK( pContainer != NULL, SDB_OOM, error, PDERROR,
                "malloc failed" );

      // 1.parse the input objs and build the opti tree
      rc = buildTree( objs, objNum, pOptiTree, pContainer->ptrTable(),
                      pContainer->paramTable(), pCLName, hint );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to build the opti tree(rc=%d)",
                  rc );

      // 2.extend
      rc = pOptiTree->extend( pExtend );
      PD_RC_CHECK( rc, PDERROR,
                  "extend failed(rc=%d)", rc );

      // 3.optimize
      {
         qgmOptTree tree( pExtend );
         optQgmOptimizer optimizer;
         rc = optimizer.adjust( tree );
         PD_RC_CHECK( rc, PDERROR,
                     "failed to adjust the tree(rc=%d)",
                     rc );
         pExtend = tree.getRoot();
      }

      // 4.build physical plan
      {
         qgmBuilder builder( pContainer->ptrTable(),
                             pContainer->paramTable() );
         rc = builder.build( pExtend, pContainer->plan() );
         PD_RC_CHECK( rc, PDERROR,
                     "failed to build the physical plan(rc=%d)",
                     rc );
         SDB_ASSERT( QGM_PLAN_TYPE_MAX != pContainer->type(),
                     "invalid container type!" );
      }

      // 5. create context
      SDB_ASSERT( QGM_PLAN_TYPE_RETURN == pContainer->type(),
                  "invalid container type!" );
      rc = createContext( pContainer, cb, contextID );
      PD_RC_CHECK( rc, PDERROR, "failed to create context(rc=%d)", rc );

      containerOwnned = FALSE ;

      // 6.execute
      pContainer->setClientVersion( clientVer ) ;

      rc = pContainer->execute( cb );

      if( NULL != pCataVer )
      {
         *pCataVer = pContainer->getCatalogVersion() ;
      }

      PD_RC_CHECK( rc, PDERROR, "execute failed(rc=%d)", rc );

      /// 7. set cur trans context id
      if ( cb->isAutoCommitTrans() )
      {
         cb->setCurAutoTransCtxID( contextID ) ;
      }

   done:
      if ( NULL != pExtend )
      {
         SDB_OSS_DEL pExtend;
      }
      else
      {
         SAFE_OSS_DELETE( pOptiTree );
      }
      if ( pContainer && containerOwnned )
      {
         SDB_OSS_DEL pContainer;
      }
      return rc;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( pContainer && containerOwnned )
      {
         SDB_OSS_DEL pContainer ;
         pContainer = NULL ;
      }
      goto done;
   }

   INT32 aggrBuilder::buildTree( const BSONObj &objs, INT32 objNum,
                                 _qgmOptiTreeNode *&root,
                                 _qgmPtrTable * pPtrTable,
                                 _qgmParamTable *pParamTable,
                                 const CHAR *pCollectionName,
                                 const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;

      const CHAR *pCLNameTmp = pCollectionName ;
      BSONObj tmpHint = hint ;
      ossValuePtr pDataPos = 0 ;

      AGGR_PARSER_MAP::iterator iterMap ;

      PD_CHECK( !objs.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                "Parameter-object can't be empty!" ) ;

      pDataPos = (ossValuePtr)objs.objdata() ;
      while ( i < objNum )
      {
         try
         {
            // parse an obj, i.e:{$group:{_id: groupby, total:{$sum: "$num"}}}
            BSONObj paraObj ( (const CHAR*)pDataPos ) ;
            BSONElement bePara ;
            const CHAR *pAggrOp = NULL ;

            if ( paraObj.nFields() != 1 )
            {
               PD_LOG( PDERROR, "Only one element in one aggregate object "
                       "is allowed: %s", paraObj.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            bePara = paraObj.firstElement() ;
            pAggrOp = bePara.fieldName() ;

            iterMap = _parserMap.find( pAggrOp ) ;
            if ( iterMap == _parserMap.end() )
            {
               PD_LOG( PDERROR, "Unknow aggregation-operator name: %s",
                       pAggrOp ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = iterMap->second->parse( bePara, root, pPtrTable,
                                         pParamTable, pCLNameTmp,
                                         tmpHint ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to build the opti tree, rc: %d", rc ) ;
               goto error ;
            }

            pDataPos += ossAlignX( (ossValuePtr)paraObj.objsize(), 4 );
            i++ ;
            pCLNameTmp = NULL ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Failed to build tree, occur unexpected "
                    "error:%s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE( root ) ;
      goto done ;
   }

   INT32 aggrBuilder::createContext( _qgmPlanContainer *container,
                                     _pmdEDUCB *cb, SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      contextID = -1 ;

      rtnContextQGM::sharePtr context ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_QGM, context,
                               contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create new context" ) ;
         goto error ;
      }
      rc = context->open( container ) ;
      PD_RC_CHECK( rc, PDERROR, "Open context failed, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }


   INT32 _aggrCmdBase::appendObj( const BSONObj &obj,
                                  CHAR *&pOutputBuffer,
                                  INT32 &bufferSize,
                                  INT32 &bufUsed,
                                  INT32 &buffObjNum )
   {
      INT32 rc = SDB_OK ;
      INT32 bufEnd = bufUsed ;
      INT32 curUsedSize = 0 ;

      bufEnd = ossRoundUpToMultipleX( bufEnd, 4 ) ;
      curUsedSize = bufEnd + obj.objsize() ;
      if ( curUsedSize > bufferSize )
      {
         INT32 newBufSize = ossRoundUpToMultipleX( curUsedSize,
                                                   DMS_PAGE_SIZE4K ) ;
         CHAR *pOrgBuff = pOutputBuffer ;
         pOutputBuffer = (CHAR *)SDB_OSS_REALLOC( pOutputBuffer,
                                                  newBufSize ) ;
         if ( !pOutputBuffer )
         {
            PD_LOG( PDERROR, "Failed to realloc %d bytes memory",
                    newBufSize ) ;
            pOutputBuffer = pOrgBuff ;
            rc = SDB_OOM ;
            goto error ;
         }
         bufferSize = newBufSize ;
      }
      ossMemcpy( pOutputBuffer + bufEnd, obj.objdata(), obj.objsize() ) ;
      bufUsed = curUsedSize ;
      ++buffObjNum ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _aggrCmdBase::openContext( const CHAR *pObjBuff,
                                    INT32 objNum,
                                    const CHAR *pInnerCmd,
                                    const BSONObj &selector,
                                    const BSONObj &hint,
                                    INT64 skip,
                                    INT64 limit,
                                    _pmdEDUCB *cb,
                                    SINT64 &contextID,
                                    IRtnMonProcessorPtr monPtr )
   {
      INT32 rc = SDB_OK ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      SINT64 aggrContextID = -1 ;
      monDataSetFetch *dsFetch = NULL ;
      BSONObj obj ;

      try
      {
         obj = BSONObj( pObjBuff ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = pmdGetKRCB()->getAggrCB()->build( obj, objNum,
                                             pInnerCmd,
                                             hint,
                                             cb, aggrContextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build context, rc: %d", rc ) ;
         goto error ;
      }

      // need dump context to process selector
      if ( !selector.isEmpty() || monPtr.get() )
      {
         rtnContextDump::sharePtr context ;

         rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, context,
                                  contextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create new context, rc: %d",
                      rc ) ;

         rc = context->open( selector, BSONObj(), limit, skip ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open dump context, rc: %d",
                      rc ) ;

         dsFetch =
               (monDataSetFetch *)(
                     getRtnFetchBuilder()->create( RTN_FETCH_DATASET ) ) ;
         PD_CHECK( NULL != dsFetch, SDB_OOM, error, PDERROR,
                   "Failed to allocate fetcher for context" ) ;

         rc = dsFetch->attachContext( aggrContextID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to attach context, rc: %d", rc ) ;
         aggrContextID = -1 ;

         context->setMonFetch( dsFetch, TRUE ) ;
         dsFetch = NULL ;
         context->setMonProcessor( monPtr ) ;
      }
      else
      {
         contextID = aggrContextID ;
         aggrContextID = -1 ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( -1 != aggrContextID )
      {
         rtnCB->contextDelete ( aggrContextID, cb ) ;
         aggrContextID = -1 ;
      }
      SAFE_OSS_DELETE( dsFetch ) ;
      goto done ;
   }

   INT32 _aggrCmdBase::parseUserAggr( const BSONObj &hint,
                                      vector<BSONObj> &vecObj,
                                      BSONObj &newHint )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement e = hint.getField( FIELD_NAME_SYS_AGGR ) ;
         if ( e.eoo() )
         {
            newHint = hint ;
            goto done ;
         }
         else if ( Object == e.type() )
         {
            newHint = hint.filterFieldsUndotted( BSON( FIELD_NAME_SYS_AGGR <<
                                                 1 ), false ) ;
            vecObj.push_back( e.embeddedObject() ) ;
         }
         else if ( Array != e.type() )
         {
            PD_LOG( PDERROR, "User aggr must be Array" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            newHint = hint.filterFieldsUndotted( BSON( FIELD_NAME_SYS_AGGR <<
                                                 1 ), false ) ;
            BSONObjIterator it( e.embeddedObject() ) ;
            while( it.more() )
            {
               e = it.next() ;
               if ( Object != e.type() )
               {
                  PD_LOG( PDERROR, "User aggr must be obj" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               vecObj.push_back( e.embeddedObject() ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Parse user define aggr occur exception: %s",
                 e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      get global aggr cb
   */
   aggrBuilder* sdbGetAggrCB ()
   {
      static aggrBuilder s_aggrCB ;
      return &s_aggrCB ;
   }
}

