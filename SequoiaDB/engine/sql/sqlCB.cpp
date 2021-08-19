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

   Source File Name = sqlCB.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sqlCB.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "qgmPlanContainer.hpp"
#include "qgmBuilder.hpp"
#include "utilStr.hpp"
#include "optQgmOptimizer.hpp"
#include "rtnSQLFunc.hpp"
#include "rtnSQLFuncFactory.hpp"
#include "rtnContextQGM.hpp"

namespace engine
{
   _sqlCB::_sqlCB()
   {

   }

   _sqlCB::~_sqlCB()
   {

   }

   INT32 _sqlCB::init ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::active ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::deactive ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::fini ()
   {
      return SDB_OK ;
   }

   INT32 _sqlCB::exec( const CHAR *sql, _pmdEDUCB *cb,
                       SINT64 &contextID,
                       BOOLEAN &needRollback,
                       BSONObjBuilder *pBuilder )
   {
      SDB_ASSERT( NULL != sql, "impossible" ) ;
      INT32 rc = SDB_OK ;
      qgmPlanContainer *container = NULL ;
      BOOLEAN containerOwnned = TRUE ;
      qgmOptiTreeNode *opti = NULL ;
      qgmOptiTreeNode *extend = NULL ;
      const CHAR *trimedSql = NULL ;

      PD_LOG( PDDEBUG, "sql[%s]", sql ) ;

      rc = utilStrTrim( (CHAR *)sql, trimedSql ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to trim sql:%s", sql ) ;
         goto error ;
      }

      container = SDB_OSS_NEW qgmPlanContainer() ;
      if ( NULL == container )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      /// step 1: ast parse
      container->ast() = SQL_PARSE( trimedSql, _grammar ) ;
      if ( !container->ast().match
           || !container->ast().full )
      {
         PD_LOG( PDERROR, "syntax error [%s]", container->ast().stop ) ;
         rc = SDB_SQL_SYNTAX_ERROR ;
         goto error ;
      }

      {
      /// step 2: build opti tree
      qgmBuilder builder( container->ptrTable(),
                          container->paramTable()) ;
      rc = builder.build( container->ast().trees, opti ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build qgm tree:%d", rc ) ;
         goto error ;
      }

      /// step 3: extend
      rc = opti->extend( extend ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extend qgm tree:%d", rc ) ;
         goto error ;
      }

      /// step 4: optimize
      {
      _qgmOptTree tree( extend ) ;
      _optQgmOptimizer optimizer ;
      rc = optimizer.adjust( tree ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rewrite sql:%d", rc ) ;
         goto error ;
      }

      extend = tree.getRoot() ;
      }

      /// step 5:build physical plan.
      rc = builder.build( extend, container->plan() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build phy tree:%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( QGM_PLAN_TYPE_MAX != container->type(),
                  "impossible" ) ;

      /// step 6: if it is a query. create context.
      if ( QGM_PLAN_TYPE_RETURN == container->type() )
      {
         rc = _createContext( container, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create context:%d", rc ) ;
            goto error ;
         }
         containerOwnned = FALSE ;
      }

      /// step 7: execute.
      rc = container->execute( cb ) ;
      needRollback = container->needRollback() ;
      if ( pBuilder )
      {
         container->buildRetInfo( *pBuilder ) ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to execute qgm tree, rc: %d", rc ) ;
         goto error ;
      }

      /// step 8: set cur trans context id
      if ( -1 != contextID && cb->isAutoCommitTrans() )
      {
         cb->setCurAutoTransCtxID( contextID ) ;
      }

      }
   done:
      /// if extended, we noly need release extended root.
      if ( NULL != extend )
      {
         SDB_OSS_DEL extend ;
      }
      else
      {
         SAFE_OSS_DELETE( opti ) ;
      }
      if ( container && containerOwnned &&
           QGM_PLAN_TYPE_RETURN != container->type() )
      {
         SDB_OSS_DEL container ;
      }
      return rc ;
   error:
      if ( -1 != contextID )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      if ( container && containerOwnned )
      {
         SDB_OSS_DEL container ;
         container = NULL ;
      }
      goto done ;
   }

   INT32 _sqlCB::getFunc( const CHAR *name,
                          UINT32 paramNum,
                          _rtnSQLFunc *&func )
   {
      _rtnSQLFuncFactory factory ;
      return factory.create( name, paramNum, func ) ;
   }

   INT32 _sqlCB::_createContext( _qgmPlanContainer *container,
                                 _pmdEDUCB *cb, SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      rtnContextQGM *context = NULL ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      rc = rtnCB->contextNew ( RTN_CONTEXT_QGM, (rtnContext**)&context,
                               contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create new context, rc: %d", rc ) ;
      // open
      rc = context->open( container ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open context[%lld], rc: %d",
                   context->contextID(), rc ) ;

   done:
      return rc ;
   error:
      rtnCB->contextDelete ( contextID, cb ) ;
      contextID = -1 ;
      goto done ;
   }

   /*
      get global sql cb
   */
   SQL_CB* sdbGetSQLCB ()
   {
      static SQL_CB s_sqlCB ;
      return &s_sqlCB ;
   }

}

