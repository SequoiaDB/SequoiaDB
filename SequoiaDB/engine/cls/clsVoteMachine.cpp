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

   Source File Name = clsVoteMachine.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsVoteMachine.hpp"
#include "netRouteAgent.hpp"
#include "clsVSSilence.hpp"
#include "clsVSSecondary.hpp"
#include "clsVSVote.hpp"
#include "clsVSAnnounce.hpp"
#include "clsVSPrimary.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   #define CLS_VOTE_REGISGER_STATUS( status, rc ) \
           {\
              _clsVoteStatus *s = SDB_OSS_NEW status( _groupInfo,\
                                                      _agent ) ;\
              if ( NULL == s ) \
              {\
                 clear() ;\
                 PD_LOG( PDERROR, "allocate mem failed") ;\
                 rc = SDB_OOM ;\
                 goto error ;\
              }\
              _status.push_back( s ) ;\
           }

   #define CLS_VOTE_FIRST_STATUS 0

   #define CLS_VOTE_ACTIVE_STATUS( next ) \
           {\
              SDB_ASSERT( next < (INT32)_status.size() &&\
                     0 <= next,\
                     "valid: 0 <= status < status.size()" ) ;\
              INT32 now = CLS_INVALID_VOTE_ID ;\
              INT32 nextStatus = next ;\
              _clsVoteStatus *prevVS = _current ; \
              if ( NULL != _current )\
              {\
                 now = _current->id() ;\
              }\
              _current = _status.at( next ) ;\
              while ( now != nextStatus )\
              {\
                 if ( prevVS ) { prevVS->deactive() ; } \
                 prevVS = _current ; \
                 PD_LOG( PDINFO, "vote change to %s", _current->name() ) ;\
                 now = _current->id() ;\
                 _current->active( nextStatus ) ;\
                 SDB_ASSERT( nextStatus < (INT32)_status.size() &&\
                     0 <= nextStatus,\
                     "valid: 0 <= status < status.size()" ) ;\
                 _current = _status.at( nextStatus ) ;\
              }\
              if ( 0 != _forceMillis ) \
              {\
                 _forceMillis = 0 ;\
              }\
           }


   _clsVoteMachine::_clsVoteMachine( _clsGroupInfo *info,
                                     _netRouteAgent *agent )
   :_agent( agent ),
    _current( NULL ),
    _groupInfo( info ),
    _shadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ),
    _forceMillis( 0 )
   {

   }

   _clsVoteMachine::~_clsVoteMachine()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_INIT, "_clsVoteMachine::init" )
   INT32 _clsVoteMachine::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSVTMH_INIT ) ;
      CLS_VOTE_REGISGER_STATUS( _clsVSSilence, rc )
      CLS_VOTE_REGISGER_STATUS( _clsVSSecondary, rc )
      CLS_VOTE_REGISGER_STATUS( _clsVSVote, rc )
      CLS_VOTE_REGISGER_STATUS( _clsVSAnnounce, rc )
      CLS_VOTE_REGISGER_STATUS( _clsVSPrimary, rc )
      CLS_VOTE_ACTIVE_STATUS( CLS_VOTE_FIRST_STATUS )
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTMH_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_CLEAR, "_clsVoteMachine::clear" )
   void _clsVoteMachine::clear()
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_CLEAR ) ;
      vector<_clsVoteStatus *>::iterator itr =
                                     _status.begin() ;
      for ( ; itr != _status.end(); itr++ )
      {
         SDB_OSS_DEL *itr ;
         *itr = NULL ;
      }
      _status.clear() ;
      PD_TRACE_EXIT ( SDB__CLSVTMH_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_HDINPUT, "_clsVoteMachine::handleInput" )
   INT32 _clsVoteMachine::handleInput( const MsgHeader *header )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "msg should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      INT32 next = CLS_INVALID_VOTE_ID ;
      
      if ( !_current )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      {
         rc = _current->handleInput( header, next ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         CLS_VOTE_ACTIVE_STATUS( next )
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTMH_HDINPUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_HDTMOUT, "_clsVoteMachine::handleTimeout" )
   void _clsVoteMachine::handleTimeout( const UINT32 &millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_HDTMOUT ) ;
      if ( !_current )
      {
         goto done ;
      }
      else if ( 0 < _forceMillis )
      {
         if ( _forceMillis <= millisec )
         {
            _forceMillis = 0 ;
         }
         else
         {
            _forceMillis -= millisec ;
         }
      }
      else
      {
         INT32 next = CLS_INVALID_VOTE_ID ;
         _current->handleTimeout( millisec, next ) ;
         CLS_VOTE_ACTIVE_STATUS( next )
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSVTMH_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_FORCE, "_clsVoteMachine::force" )
   void _clsVoteMachine::force( const INT32 &id,
                                UINT32 millis )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_FORCE ) ;
      CLS_VOTE_ACTIVE_STATUS( id )
      if ( 0 < millis )
      {
         _forceMillis = millis ;
      }
      PD_TRACE_EXIT ( SDB__CLSVTMH_FORCE ) ;
      return ;
   }

}
