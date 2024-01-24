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

   Source File Name = clsVoteStatus.hpp

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

#ifndef CLSVOTESTATUS_HPP_
#define CLSVOTESTATUS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsDef.hpp"
#include "msgReplicator.hpp"

namespace engine
{
   #define CLS_IS_MAJORITY( now, total ) \
           ( ( total / 2) < now )

   const INT32 CLS_INVALID_VOTE_ID = -1 ;

   class _netRouteAgent ;
   class _dpsLogWrapper ;

   class _clsVoteStatus : public SDBObject
   {
   public:
      _clsVoteStatus( _clsGroupInfo *info,
                      _netRouteAgent *agent,
                      INT32 id ) ;

      virtual ~_clsVoteStatus() ;

   public:
      virtual INT32 handleInput( const MsgHeader *header,
                                 INT32 &next ) = 0 ;

      virtual void handleTimeout( const UINT32 &millisec,
                                  INT32 &next ) = 0;

      virtual void active( INT32 &next ) = 0 ;
      virtual void deactive () {}

      virtual void forceActive ( INT32 & next )
      {
         active( next ) ;
      }

      virtual const CHAR *name() const { return "" ;}

   public:
      OSS_INLINE const INT32 &id()
      {
         return _id ;
      }

      OSS_INLINE const CHAR * getScopeName() const
      {
         return isLocation() ? "Location Set" : "Replica Group" ;
      }

   protected:
      OSS_INLINE INT32 _vote()
      {
         return _launch( CLS_ELECTION_ROUND_STAGE_ONE ) ;
      }

      OSS_INLINE INT32 _announce()
      {
         return _launch( CLS_ELECTION_ROUND_STAGE_TWO ) ;
      }

      OSS_INLINE INT32 _promise( const _MsgClsElectionBallot *msg )
      {
         return _launch( msg->weights,
                         msg->identity,
                         CLS_ELECTION_ROUND_STAGE_ONE ) ;
      }

      OSS_INLINE INT32 _accept( const _MsgClsElectionBallot *msg )
      {
         return _launch( msg->weights,
                         msg->identity,
                         CLS_ELECTION_ROUND_STAGE_TWO ) ;
      }

      OSS_INLINE UINT32 &_timeout()
      {
         return _time ;
      }

      OSS_INLINE UINT32 &_accepted()
      {
         return _acceptedNum ;
      }

      OSS_INLINE UINT32 &_criticalAccepted()
      {
         return _criticalAcceptedNum ;
      }

      OSS_INLINE _clsGroupInfo *_info()
      {
         return _groupInfo ;
      }

      OSS_INLINE BOOLEAN isLocation() const
      {
         return MSG_INVALID_LOCATIONID != _groupInfo->localLocationID ;
      }

      BOOLEAN _isAccepted() ;

   private:
      INT32 _launch( const CLS_ELECTION_ROUND &round ) ;

      INT32 _launch( const DPS_LSN &lsn,
                     const _MsgRouteID &id,
                     const CLS_ELECTION_ROUND &round ) ;
      void _broadcastAlives( void *msg ) ;
   private:
      _clsGroupInfo *_groupInfo ;
      _netRouteAgent *_agent ;
      _dpsLogWrapper *_logger ;
      INT32 _id ;
      UINT32 _time ;
      UINT32 _acceptedNum ;
      UINT32 _criticalAcceptedNum ;
   } ;
}

#endif

