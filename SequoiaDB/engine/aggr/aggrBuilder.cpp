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
#include "rtnDataSet.hpp"
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

   INT32 aggrBuilder::build( const BSONObj &objs, INT32 objNum,
                             const CHAR *pCLName, _pmdEDUCB *cb,
                             SINT64 &contextID )
   {
      INT32 rc = SDB_OK;
      BOOLEAN hasNew = FALSE;
      qgmOptiTreeNode *pOptiTree = NULL;
      qgmOptiTreeNode *pExtend = NULL;
      qgmPlanContainer *pContainer = NULL;

      PD_CHECK( objNum > 0, SDB_INVALIDARG, error, PDERROR,
               "input error, no object input!" );

      pContainer = SDB_OSS_NEW _qgmPlanContainer() ;
      PD_CHECK( pContainer != NULL, SDB_OOM, error, PDERROR,
               "malloc failed" );
      hasNew = TRUE;

      rc = buildTree( objs, objNum, pOptiTree, pContainer->ptrTable(),
                      pContainer->paramTable(), pCLName );
      PD_RC_CHECK( rc, PDERROR,
                  "failed to build the opti tree(rc=%d)",
                  rc );

      rc = pOptiTree->extend( pExtend );
      PD_RC_CHECK( rc, PDERROR,
                  "extend failed(rc=%d)", rc );

      {
         qgmOptTree tree( pExtend );
         optQgmOptimizer optimizer;
         rc = optimizer.adjust( tree );
         PD_RC_CHECK( rc, PDERROR,
                     "failed to adjust the tree(rc=%d)",
                     rc );
         pExtend = tree.getRoot();
      }

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

      rc = pContainer->execute( cb );
      PD_RC_CHECK( rc, PDERROR, "execute failed(rc=%d)", rc );

      SDB_ASSERT( QGM_PLAN_TYPE_RETURN == pContainer->type(),
                  "invalid container type!" );
      rc = createContext( pContainer, cb, contextID );
      PD_RC_CHECK( rc, PDERROR, "failed to create context(rc=%d)", rc );
   done:
      if ( NULL != pExtend )
      {
         SDB_OSS_DEL pExtend;
      }
      else
      {
         SAFE_OSS_DELETE( pOptiTree );
      }
      if ( hasNew
         && ( rc != SDB_OK || QGM_PLAN_TYPE_RETURN != pContainer->type()) )
      {
         SDB_OSS_DEL pContainer;
      }
      return rc;
   error:
      goto done;
   }

   INT32 aggrBuilder::buildTree( const BSONObj &objs, INT32 objNum,
                                 _qgmOptiTreeNode *&root,
                                 _qgmPtrTable * pPtrTable,
                                 _qgmParamTable *pParamTable,
                                 const CHAR *pCollectionName )
   {
      INT32 rc = SDB_OK;
      INT32 i = 0;
      const CHAR *pCLNameTmp = pCollectionName;
      ossValuePtr pDataPos = 0;
      PD_CHECK( !objs.isEmpty(), SDB_INVALIDARG, error, PDERROR,
               "Parameter-object can't be empty!" );
      pDataPos = (ossValuePtr)objs.objdata();
      while ( i < objNum )
      {
         try
         {
            BSONObj paraObj ( (const CHAR*)pDataPos );
            PD_CHECK( paraObj.nFields() == 1, SDB_INVALIDARG,
                     error, PDERROR,
                     "only one element in one aggregate object is allowed" );
            BSONElement bePara = paraObj.firstElement();
            const CHAR *pAggrOp = bePara.fieldName();
            AGGR_PARSER_MAP::iterator iterMap
                                       = _parserMap.find( pAggrOp );
            PD_CHECK( iterMap != _parserMap.end(), SDB_INVALIDARG,
                     error, PDERROR,
                     "unknow aggregation-operator name(%s)",
                     pAggrOp );
            rc = iterMap->second->parse( bePara, root, pPtrTable,
                                         pParamTable, pCLNameTmp );
            PD_RC_CHECK( rc, PDERROR,
                        "failed to build the opti tree(rc=%d)", rc );
            pDataPos += ossAlignX( (ossValuePtr)paraObj.objsize(), 4 );
            i++;
            pCLNameTmp = NULL;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR,
                  "Failed to build tree, received unexpected error:%s",
                  e.what() );
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE( root );
      goto done;
   }

   INT32 aggrBuilder::createContext( _qgmPlanContainer *container,
                                     _pmdEDUCB *cb, SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      contextID = -1 ;

      rtnContextQGM *context = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_QGM, (rtnContext**)&context,
                               contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         context = NULL ;
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
                                    _pmdEDUCB *cb,
                                    SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextDump *context = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      SINT64 aggrContextID = -1 ;
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
                                             cb, aggrContextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to build context, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnCB->contextNew ( RTN_CONTEXT_DUMP, (rtnContext**)&context,
                               contextID, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to create new context, rc: %d", rc ) ;
         goto error ;
      }

      rc = context->open( selector, BSONObj() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open dump context, rc: %d", rc ) ;
         goto error ;
      }

      {
         rtnDataSet ds( aggrContextID, cb ) ;
         while( TRUE )
         {
            rc = ds.next( obj ) ;
            if ( SDB_OK == rc )
            {
               rc = context->monAppend( obj ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to append obj to context, rc: %d",
                          rc ) ;
                  goto error ;
               }
            }
            else if ( SDB_DMS_EOC == rc )
            {
               aggrContextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to get next obj, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      if ( -1 != aggrContextID )
      {
         rtnCB->contextDelete ( aggrContextID, cb ) ;
         aggrContextID = -1 ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _aggrCmdBase::parseUserAggr( const BSONObj &hint,
                                      vector<BSONObj> &vecObj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement e = hint.getField( FIELD_NAME_SYS_AGGR ) ;
         if ( e.eoo() )
         {
            goto done ;
         }
         else if ( Object == e.type() )
         {
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

   INT32 _aggrCmdBase::parseMatcher( const BSONObj &query,
                                     BSONObj &nodesMatcher,
                                     BSONObj &newMatcher,
                                     BOOLEAN ignoreNodeParam,
                                     BOOLEAN ignoreCtrlParam )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder matcherBuilder ;
      BSONObjBuilder nodesCondBuilder ;

      try
      {
         BSONObjIterator iter( query ) ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;

            if ( Array == ele.type() &&
                 0 == ossStrcmp( ele.fieldName(), "$and" ) )
            {
               BSONObj tmpNodeMatcher ;
               BSONObj tmpNewMatcher ;
               BSONArrayBuilder subMatcher(
                  matcherBuilder.subarrayStart( ele.fieldName() ) ) ;

               BSONObjIterator subItr( ele.embeddedObject() ) ;
               while ( subItr.more() )
               {
                  BSONElement subEle = subItr.next() ;
                  if ( Object != subEle.type() )
                  {
                     PD_LOG( PDERROR, "Parse mather obj[%s] failed: "
                             "invalid $and", query.toString().c_str() ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  else
                  {
                     BSONObj tmpObj = subEle.embeddedObject() ;
                     rc = parseMatcher( tmpObj, tmpNodeMatcher, tmpNewMatcher,
                                        ignoreNodeParam, ignoreCtrlParam ) ;
                     PD_RC_CHECK( rc, PDERROR, "Parse matcher[%s] failed",
                                  query.toString().c_str() ) ;

                     subMatcher.append( tmpNewMatcher ) ;
                     nodesCondBuilder.appendElements( tmpNodeMatcher ) ;
                  }
               } /// end while
               subMatcher.done() ;
            }
            else if ( !ignoreNodeParam && (
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_GROUPID ) ||
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_GROUPNAME ) ||
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_GROUPS ) ||
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_NODEID ) ||
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_HOST ) ||
                      0 == ossStrcasecmp( ele.fieldName(), PMD_OPTION_SVCNAME ) ||
                      0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_SERVICE_NAME )
                     ) )
            {
               nodesCondBuilder.append( ele ) ;
            }
            else if ( !ignoreCtrlParam && (
                      0 == ossStrcasecmp( ele.fieldName(),
                                          FIELD_NAME_NODE_SELECT ) ||
                      0 == ossStrcasecmp( ele.fieldName(),
                                          FIELD_NAME_GLOBAL ) ||
                      0 == ossStrcasecmp( ele.fieldName(),
                                          FIELD_NAME_ROLE ) ||
                      0 == ossStrcasecmp( ele.fieldName(),
                                          FIELD_NAME_RAWDATA )
                     ) )
            {
               nodesCondBuilder.append( ele ) ;
            }
            else
            {
               matcherBuilder.append( ele );
            }
         }

         newMatcher = matcherBuilder.obj() ;
         nodesMatcher = nodesCondBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur unexpected error:%s", e.what() ) ;
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

