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

   Source File Name = clsVoteMachine.hpp

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

#ifndef CLSVOTEMACHINE_HPP_
#define CLSVOTEMACHINE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsVoteStatus.hpp"
#include <vector>

using namespace std ;

namespace engine
{

   #define CLS_SHADOWN_TIMEOUT_DFT           ( 3000 )

   class _clsVoteMachine : public SDBObject
   {
   public:
      _clsVoteMachine( _clsGroupInfo *info,
                       _netRouteAgent *agent ) ;
      ~_clsVoteMachine() ;

   public:
      OSS_INLINE BOOLEAN primaryIsMe()
      {
         _groupInfo->mtx.lock_r() ;
         BOOLEAN res = _groupInfo->local.value == _groupInfo->primary.value &&
                       _groupInfo->primary.value != MSG_INVALID_ROUTEID ?
                       TRUE : FALSE ;
         _groupInfo->mtx.release_r() ;
         return res ;
      }

      OSS_INLINE UINT8 getShadowWeight() const
      {
         return _shadowWeight ;
      }

      OSS_INLINE void setShadowWeight( UINT8 weight,
                                       UINT32 timeout = CLS_SHADOWN_TIMEOUT_DFT )
      {
         _shadowWeight = weight ;
         _shadowTimeout = timeout ;
      }

      OSS_INLINE BOOLEAN isInStepUp() const
      {
         return 0 < _forceMillis ;
      }

      OSS_INLINE BOOLEAN isShadowTimeout() const
      {
         return 0 == _shadowTimeout ;
      }

   public:
      INT32 init() ;

      void  clear() ;

      INT32 handleInput( const MsgHeader *header ) ;

      void  handleTimeout( const UINT32 &millisec ) ;

      INT32 active() ;

      void  force( const INT32 &id, UINT32 mills = 0 ) ;
      BOOLEAN  isStatus( const INT32 &id ) const ;

   private:
      vector<_clsVoteStatus *>   _status ;
      _netRouteAgent             *_agent ;
      _clsVoteStatus             *_current ;
      _clsGroupInfo              *_groupInfo ;
      UINT8                      _shadowWeight ;
      UINT32                     _shadowTimeout ;  /// ms
      UINT32                     _forceMillis ;
   } ;

   typedef class _clsVoteMachine clsVoteMachine ;
}

#endif // CLSVOTEMACHINE_HPP_

