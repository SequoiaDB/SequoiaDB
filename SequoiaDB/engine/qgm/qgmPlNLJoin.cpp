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

   Source File Name = qgmPlNLJoin.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlNLJoin.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

using namespace bson ;

namespace engine
{
   _qgmPlNLJoin::_qgmPlNLJoin( INT32 type )
   :_qgmPlJoin( type ),
    _makeOuterInner( FALSE ),
    _innerEnd( TRUE ),
    _notMatched( TRUE ),
    _innerF( NULL )
   {
      _initialized = TRUE ;
   }

   _qgmPlNLJoin::~_qgmPlNLJoin()
   {
      SAFE_OSS_DELETE( _innerF ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLNLJOIN__INIT, "_qgmPlNLJoin::_init" )
   INT32 _qgmPlNLJoin::_init()
   {
      PD_TRACE_ENTRY( SDB__QGMPLNLJOIN__INIT ) ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( SQL_GRAMMAR::SQLMAX != _joinType,
                  "impossible" ) ;
      SDB_ASSERT( 2 == inputSize(), "impossible" ) ;
      if ( SQL_GRAMMAR::R_OUTERJOIN == _joinType )
      {
         _joinType = SQL_GRAMMAR::L_OUTERJOIN ;
         _outerAlias = &(input( 1 )->alias()) ;
         _outer = input( 1 ) ;
         _innerAlias = &(input( 0 )->alias()) ;
         _inner = input( 0 ) ;
      }
      else
      {
         _outerAlias = &(input( 0 )->alias()) ;
         _outer = input( 0 ) ;
         _innerAlias = &(input( 1 )->alias()) ;
         _inner = input( 1 ) ;
      }

      _innerF = SDB_OSS_NEW qgmFetchOut() ;
      if ( NULL == _innerF )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _makeOuterInner = TRUE ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLNLJOIN__INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLNLJOIN__EXEC, "_qgmPlNLJoin::_execute" )
   INT32 _qgmPlNLJoin::_execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLNLJOIN__EXEC ) ;
      INT32 rc = SDB_OK ;
      if ( !_makeOuterInner )
      {
         rc = _init() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      _innerEnd = TRUE ;
      rc = _outer->execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLNLJOIN__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLNLJOIN__FETCHNEXT, "_qgmPlNLJoin::_fetchNext" )
   INT32 _qgmPlNLJoin::_fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLNLJOIN__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      qgmFetchOut fetch ;

      do
      {
         if ( _innerEnd )
         {
            rc = _outer->fetchNext( _outerF ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = _modifyInnerCondition( _outerF.obj ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            rc = _inner->execute( _eduCB ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            _innerEnd = FALSE ;
            _notMatched = TRUE ;
            _innerF->obj = BSONObj() ;
         }

         rc = _inner->fetchNext( *_innerF ) ;

         if ( SDB_DMS_EOC == rc )
         {
            _innerEnd = TRUE ;
            if ( SQL_GRAMMAR::INNERJOIN == _joinType )
            {
               _innerF->obj = BSONObj() ;
               continue ;
            }
            else
            {
               if ( _notMatched )
               {
                  rc = SDB_OK ;
                  break ;
               }
               else
               {
                  continue ;
               }
            }
         }
         else if ( SDB_OK != rc )
         {
            goto error ;
         }
         else
         {
            if ( _notMatched )
            {
               _notMatched = FALSE ;
            }
            break ;
         }
      } while ( TRUE ) ;

      fetch.alias = *_outerAlias ;
      fetch.obj = _outerF.obj ;
      fetch.next = _innerF ;
      fetch.next->obj = _innerF->obj ;
      fetch.next->alias = *_innerAlias ;

      if ( !_merge )
      {
         next.alias = fetch.alias ;
         next.obj = fetch.obj ;
         next.next = fetch.next ;
      }
      else
      {
         next.obj = fetch.mergedObj() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLNLJOIN__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLNLJOIN__MODIFYINNERCONDITION, "_qgmPlNLJoin::_modifyInnerCondition" )
   INT32 _qgmPlNLJoin::_modifyInnerCondition( BSONObj &obj )
   {
      PD_TRACE_ENTRY( SDB__QGMPLNLJOIN__MODIFYINNERCONDITION ) ;
      SDB_ASSERT( NULL != _param, "impossible" ) ;
      INT32 rc = SDB_OK ;

      QGM_VARLIST::const_iterator itr = _varlist.begin() ;
      for ( ; itr != _varlist.end(); itr++ )
      {
         rc = _param->setVar( *itr, obj ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLNLJOIN__MODIFYINNERCONDITION, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}
