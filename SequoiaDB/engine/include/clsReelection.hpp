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

   Source File Name = clsReelection.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_REELECTION_HPP_
#define CLS_REELECTION_HPP_

#include "clsDef.hpp"
#include "ossEvent.hpp"

namespace engine
{
   class _clsVoteMachine ;
   class _clsSyncManager ;

   class _clsReelection : public SDBObject
   {
   public:
      _clsReelection( _clsVoteMachine *vote,
                      _clsSyncManager *syncMgr ) ;
      ~_clsReelection() ;

   public:
      INT32 run( CLS_REELECTION_LEVEL lvl,
                 UINT32 seconds,
                 pmdEDUCB *cb,
                 UINT16 destID = 0 ) ;

      INT32 wait( pmdEDUCB *cb ) ;

      void  signal() ;

   private:
      INT32 _wait4AllWriteDone( UINT32 &timePassed,
                                UINT32 timeout,
                                pmdEDUCB *cb ) ;

      INT32 _wait4Replica( UINT32 &timePassed,
                           UINT32 timeout,
                           pmdEDUCB *cb,
                           UINT16 destID ) ;

      INT32 _stepDown( UINT32 &timePassed,
                       UINT32 timeout,
                       pmdEDUCB *cb ) ;

      INT32 _wait( UINT32 &timePassed,
                   UINT32 timeout,
                   pmdEDUCB *cb,
                   BOOLEAN canSetBlock ) ;

   private:
      _clsVoteMachine *_vote ;
      _clsSyncManager *_syncMgr ;
      volatile UINT32 _level ;
      ossEvent _event ;
   } ;
   typedef class _clsReelection clsReelection ;
}

#endif

