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
                       BOOLEAN &needRollback )
   {
      SDB_ASSERT( NULL != sql, "impossible" ) ;
      INT32 rc = SDB_OK ;
      qgmPlanContainer *container = NULL ;
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

      container->ast() = SQL_PARSE( trimedSql, _grammar ) ;
      if ( !container->ast().match
           || !container->ast().full )
      {
         PD_LOG( PDERROR, "syntax error [%s]", container->ast().stop ) ;
         rc = SDB_SQL_SYNTAX_ERROR ;
         goto error ;
      }

      {
      qgmBuilder builder( container->ptrTable(),
                          container->paramTable()) ;
      rc = builder.build( container->ast().trees, opti ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build qgm tree:%d", rc ) ;
         goto error ;
      }

      rc = opti->extend( extend ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extend qgm tree:%d", rc ) ;
         goto error ;
      }

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

      rc = builder.build( extend, container->plan() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to build phy tree:%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( QGM_PLAN_TYPE_MAX != container->type(),
                  "impossible" ) ;

      rc = container->execute( cb ) ;
      needRollback = container->needRollback() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute pty tree:%d", rc ) ;
         goto error ;
      }

      if ( QGM_PLAN_TYPE_RETURN == container->type() )
      {
         rc = _createContext( container, cb, contextID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create context:%d", rc ) ;
            goto error ;
         }
      }
      }
   done:
      if ( NULL != extend )
      {
         SAFE_OSS_DELETE( extend ) ;
      }
      else
      {
         SAFE_OSS_DELETE( opti ) ;
      }
      if ( NULL != container &&
           QGM_PLAN_TYPE_RETURN != container->type() )
      {
         SAFE_OSS_DELETE( container ) ;
      }
      return rc ;
   error:
      SAFE_OSS_DELETE( container ) ;
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

