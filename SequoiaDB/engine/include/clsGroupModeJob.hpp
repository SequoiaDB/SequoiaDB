/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = clsGroupModeJob.hpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/21/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSGROUPMODE_HPP_
#define CLSGROUPMODE_HPP_

#include "utilLightJobBase.hpp"
#include "clsDef.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   #define CLS_GROUPMODE_CHECK_INTERVAL      OSS_ONE_SEC * 60

   class _clsGroupInfo ;
   class _clsVoteMachine ;

   /* 
      _clsGroupModeMonitorJob
    */
   template< class T >
   class _clsGroupModeMonitorJob : public _utilLightJob
   {
   protected:
      _clsGroupModeMonitorJob( _clsGroupInfo *info,
                               const UINT32 &localVersion ) ;

      virtual ~_clsGroupModeMonitorJob() ;

   public:
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   public:
      // Use to indicate the latest thread's job version
      static ossAtomic32 version ;

   protected:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime )
      {
         return SDB_OK ;
      }

   protected:
      const _clsGroupMode     _groupMode ;
      const UINT32            _localVersion ;
      _clsGroupInfo           *_info ;
   } ;

   template< class T > ossAtomic32 _clsGroupModeMonitorJob< T >::version( 0 ) ;

   /* 
      _clsCriticalModeMonitorJob
    */
   class _clsCriticalModeMonitorJob : public _clsGroupModeMonitorJob< _clsCriticalModeMonitorJob >
   {
   public:
      _clsCriticalModeMonitorJob( _clsGroupInfo *info ) ;

      virtual ~_clsCriticalModeMonitorJob() ;

      virtual const CHAR *name() const
      {
         return "CriticalModeMonitor" ;
      }

   private:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime ) ;

      INT32 _stopCriticalMode( pmdEDUCB *cb ) ;

   } ;
   typedef _clsCriticalModeMonitorJob clsCriticalModeMonitorJob ;

   INT32 clsStartCriticalModeMonitor( _clsGroupInfo *info ) ;

   class _clsMaintenanceModeMonitorJob :
         public _clsGroupModeMonitorJob< _clsMaintenanceModeMonitorJob >
   {
   public:
      _clsMaintenanceModeMonitorJob( _clsGroupInfo *info ) ;

      virtual ~_clsMaintenanceModeMonitorJob() ;

      virtual const CHAR *name() const
      {
         return "MaintenanceModeMonitor" ;
      }

   private:
      virtual INT32 _checkGroupMode( pmdEDUCB *cb,
                                     UTIL_LJOB_DO_RESULT &result,
                                     UINT64 &sleepTime ) ;

      INT32 _stopMaintenanceMode( pmdEDUCB *cb,
                                  const CHAR *pNodeName ) ;
   } ;
   typedef _clsMaintenanceModeMonitorJob clsMaintenanceModeMonitorJob ;

   INT32 clsStartMaintenanceModeMonitor( _clsGroupInfo *info ) ;

   class _clsGroupModeReqJob : public _utilLightJob
   {
   public:
      _clsGroupModeReqJob( _clsGroupInfo *info, _clsVoteMachine *vote ) ;

      virtual ~_clsGroupModeReqJob() ;

      virtual const CHAR *name() const
      {
         return "GroupModeRequire" ;
      }

      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

   private:
      INT32 _handleGroupModeRes( const BSONObj &grpModeInfo ) ;

   private:
      // This info stores group info, not location info
      _clsGroupInfo                    *_info ;
      _clsVoteMachine                  *_vote ;
   } ;
   typedef _clsGroupModeReqJob clsGroupModeReqJob ;

   INT32 clsStartGroupModeReqJob( _clsGroupInfo *info, _clsVoteMachine *vote ) ;

}

#endif // CLSGROUPMODE_HPP_