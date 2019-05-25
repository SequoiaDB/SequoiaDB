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

   Source File Name = qgmPlSort.cpp

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

******************************************************************************/

#include "qgmPlSort.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "ossUtil.hpp"
#include "monCB.hpp"
#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsTempSUMgr.hpp"
#include "rtnContextQGM.hpp"
#include "qgmUtil.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

using namespace bson ;

namespace engine
{
   _qgmPlSort::_qgmPlSort( const qgmOPFieldVec &order )
   :_qgmPlan( QGM_PLAN_TYPE_SORT, _qgmField() ),
    _contextID( -1 ),
    _contextSort( -1 ),
    _rtnCB( NULL )
   {
      BSONObjBuilder builder ;
      try
      {
         qgmOPFieldVec::const_iterator itr = order.begin() ;
         for ( ; itr != order.end(); itr++ )
         {
            builder.append( itr->value.attr().toString(),
                            SQL_GRAMMAR::ASC == itr->type?
                            1 : -1 ) ;
         }
         _orderBy = builder.obj() ;
         _rtnCB            = pmdGetKRCB()->getRTNCB () ;
         _initialized = TRUE ;
      }
      catch ( std::exception & e)
      {
         PD_LOG( PDERROR, "unexcepted err happened:%s",
                 e.what() ) ;
      }
   }

   _qgmPlSort::~_qgmPlSort()
   {
      close() ;
   }

   string _qgmPlSort::toString() const
   {
      stringstream ss ;
      ss << "Type:" << qgmPlanType( _type ) << '\n';
      ss << "Sort:" << _orderBy.toString() << '\n';
      return ss.str() ;
   }

   void _qgmPlSort::close()
   {
      if ( -1 != _contextSort )
      {
         _rtnCB->contextDelete ( _contextSort, _eduCB ) ;
         _contextSort = -1 ;
      }

      if ( -1 != _contextID )
      {
         _rtnCB->contextDelete ( _contextID, _eduCB ) ;
         _contextID = -1 ;
      }

      return ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSORT__EXEC, "_qgmPlSort::_execute" )
   INT32 _qgmPlSort::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSORT__EXEC ) ;
      INT32 rc = SDB_OK ;
      rtnContext *context = NULL ;
      _alias = input( 0 )->alias() ;

      rc = input(0)->execute ( eduCB ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to execute source, rc = %d", rc ) ;

      rc = _rtnCB->contextNew( RTN_CONTEXT_QGMSORT, &context,
                               _contextSort, eduCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create new context:%d", rc ) ;
         goto error ;
      }

      rc = ((rtnContextQgmSort *)context)->open( input(0) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open context:%d", rc ) ;
         goto error ;
      }

      rc = rtnSort( &context, _orderBy, 
                    eduCB, 0, -1, _contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to exec sort:%d", rc ) ;
         goto error ;
      }

      _contextSort = -1 ;

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSORT__EXEC, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSORT__FETCHNEXT, "_qgmPlSort::_fetchNext" )
   INT32 _qgmPlSort::_fetchNext ( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSORT__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      rtnContextBuf buffObj ;
      if ( -1 == _contextID )
      {
         PD_LOG( PDERROR, "invalid contextID" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = rtnGetMore ( _contextID, 1, buffObj, _eduCB, _rtnCB ) ;
      try
      {
         if ( SDB_OK == rc )
         {
            next.obj = BSONObj ( buffObj.data() ) ;
            if ( !_merge && !_alias.empty() )
            {
               next.alias = _alias ;
            }
         }
         else
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_RC_CHECK ( rc, PDERROR, "get more failed, rc = %d", rc ) ;
            }
            else
            {
               _contextID = -1 ;
            }
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Received exception %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSORT__FETCHNEXT, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   }

}

