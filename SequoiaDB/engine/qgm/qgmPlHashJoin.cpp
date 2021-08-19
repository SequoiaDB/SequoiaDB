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

   Source File Name = qgmPlHashJoin.cpp

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

#include "qgmPlHashJoin.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include "pd.hpp"
#include "qgmOptiNLJoin.hpp"

namespace engine
{
   _qgmPlHashJoin::_qgmPlHashJoin( INT32 type )
   :_qgmPlJoin( type ),
    _build( NULL ),
    _probe( NULL ),
    _buildAlias( NULL ),
    _probeAlias( NULL ),
    _hashContext( QGM_HT_INVALID_CONTEXT ),
    _state( QGM_HJ_FETCH_STATE_BUILD ),
    _hitBuildEnd(FALSE)
   {
      SDB_ASSERT( SQL_GRAMMAR::INNERJOIN == _joinType, "impossible" ) ;
      _type = QGM_PLAN_TYPE_HASHJOIN ;
   }

   _qgmPlHashJoin::~_qgmPlHashJoin()
   {

   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLHASHJOIN_INIT, "_qgmPlHashJoin::init" )
   INT32 _qgmPlHashJoin::init( _qgmOptiNLJoin *opti )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMPLHASHJOIN_INIT ) ;
      SDB_ASSERT( NULL != opti, "impossible" ) ;
      SDB_ASSERT( NULL != opti->_condition, "impossible" ) ;
      SDB_ASSERT( NULL != opti->_condition->left, "impossible" ) ;
      SDB_ASSERT( NULL != opti->_condition->right, "impossible" ) ;
      SDB_ASSERT( SQL_GRAMMAR::EG == opti->_condition->type, "impossible" ) ;
      SDB_ASSERT( SQL_GRAMMAR::DBATTR == opti->_condition->left->type, "impossible" ) ;
      SDB_ASSERT( SQL_GRAMMAR::DBATTR == opti->_condition->right->type, "impossible" ) ;

      _qgmConditionNode *left = opti->_condition->left ;
      _qgmConditionNode *right = opti->_condition->right ;

      /// TODO: put public attrs of qgmPlNLJoin and qgmPlHashJoin to qgmPlJoin.
      _outerAlias = &(input( 0 )->alias()) ;
      _innerAlias = &(input( 1 )->alias()) ;
      _inner = input( 1 ) ;
      _outer = input( 0 ) ;

      if ( left->value.relegation() == *_outerAlias &&
           right->value.relegation() == *_innerAlias )
      {
         _buildKey = right->value.attr().toString() ;
         _probeKey = left->value.attr().toString() ;
      }
      else if ( left->value.relegation() == *_innerAlias &&
                right->value.relegation() == *_outerAlias )
      {
         _buildKey = left->value.attr().toString() ;
         _probeKey = right->value.attr().toString() ;
      }
      else
      {
         PD_LOG( PDERROR, "left key or right key is not found, "
                 "left:%s, right:%s", left->value.toString().c_str(),
                 right->value.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _build = _inner ;
      _probe = _outer ;
      _buildAlias = _innerAlias ;
      _probeAlias = _outerAlias ;

      _initialized = TRUE ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLHASHJOIN_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   string _qgmPlHashJoin::toString()const
   {
      stringstream ss ;
      ss << "Build Key: " << _buildKey.c_str() << "\n" ;
      ss << "Probe Key: " << _probeKey.c_str() << "\n" ;
      return ss.str() ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLHASHJOIN__EXEC, "_qgmPlHashJoin::_execute" )
   INT32 _qgmPlHashJoin::_execute( _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMPLHASHJOIN__EXEC ) ;
      SDB_ASSERT( NULL != _build && NULL != _probe, "can not be NULL" ) ;

      _hashTbl.release() ;

      _hitBuildEnd = FALSE ;

      UINT64 bufSize = ( ( UINT64 )pmdGetOptionCB()->getHjBufSize() ) * 1024 * 1024 ;
      rc = _hashTbl.init( bufSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init hash table:%d", rc ) ;
         goto error ;
      }

      rc = _build->execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLHASHJOIN__EXEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLHASHJOIN__FETCHNEXT, "_qgmPlHashJoin::_fetchNext" )
   INT32 _qgmPlHashJoin::_fetchNext ( qgmFetchOut &next )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMPLHASHJOIN__FETCHNEXT ) ;

      do
      {
         if ( QGM_HJ_FETCH_STATE_BUILD == _state )
         {
            rc = _buildHashTbl() ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  PD_LOG( PDEVENT, "hash join done." ) ;
               }
               else
               {
                  PD_LOG( PDERROR, "failed to build hash tbl:%d", rc ) ;
               }
               goto error ;
            }

            rc = _probe->execute( _eduCB ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to exec probe:%d", rc ) ;
               goto error ;
            }

            _state = QGM_HJ_FETCH_STATE_PROBE ;
         }
         else if ( QGM_HJ_FETCH_STATE_PROBE == _state )
         {
            rc = _probe->fetchNext( _probeF ) ;
            if ( SDB_DMS_EOC == rc )
            {
               _state = QGM_HJ_FETCH_STATE_BUILD ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fetch from probe:%d" ,rc ) ;
               goto error ;
            }
            else
            {
               SDB_ASSERT( NULL == _probeF.next, "impossible" ) ;
               _probeEle = _probeF.obj.getField( _probeKey.c_str() ) ;
               rc = _hashTbl.find( _probeEle, _hashContext ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to find from hash tbl:%d", rc ) ;
                  goto error ;
               }
               _state = QGM_HJ_FETCH_STATE_FIND ;
            }
         }
         else
         {
            SDB_ASSERT( !_probeF.obj.isEmpty(), "impossible" ) ;
            BSONObj obj ;
            rc = _hashTbl.getMore( _probeEle, _hashContext, obj ) ;
            if ( SDB_DMS_EOC == rc )
            {
               SDB_ASSERT( QGM_HT_INVALID_CONTEXT == _hashContext, "impossible" ) ;
               _state = QGM_HJ_FETCH_STATE_PROBE ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fetch next from hashtbl:%d", rc ) ;
               goto error ;
            }
            else
            {
               next.obj = obj ;
               if ( NULL != _buildAlias )
               {
                  next.alias = *_buildAlias ;
               }
               if ( NULL != _probeAlias )
               {
                  _probeF.alias = *_probeAlias ;
               }
               next.next = &_probeF ;
               break ;
            }
         }
      } while (TRUE) ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLHASHJOIN__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLHASHJOIN__BUILDHASNTBL, "_qgmPlHashJoin::_buildHashTbl")
   INT32 _qgmPlHashJoin::_buildHashTbl()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__QGMPLHASHJOIN__BUILDHASNTBL ) ;
      _hashTbl.clear() ;
      UINT64 builded = 0 ;
      while ( TRUE )
      {
         if ( _buildF.obj.isEmpty() )
         {
            if ( _hitBuildEnd )
            {
               rc = SDB_DMS_EOC ;
               break ;
            }

            rc = _build->fetchNext( _buildF ) ;
            if ( SDB_DMS_EOC == rc )
            {
               _hitBuildEnd = TRUE ;
               break ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fetch from build:%d", rc ) ;
               goto error ;
            }
            else
            {
               /// do noting.
            }
         }

         SDB_ASSERT( NULL == _buildF.next, "impossible" ) ;
         rc = _hashTbl.push( _buildKey.c_str(), _buildF.obj ) ;
         if ( SDB_HIT_HIGH_WATERMARK == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to push obj to hashtbl:%d", rc ) ;
            goto error ;
         }
         else
         {
            ++builded ;
            _buildF.obj = BSONObj() ;
         }
      }

      if ( SDB_DMS_EOC == rc )
      {
         if ( 0 != builded )
         {
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

      PD_LOG( PDDEBUG, "%lld records were pushed into hash table", builded ) ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLHASHJOIN__BUILDHASNTBL, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

