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

   Source File Name = clsIndexJob.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          2019/09/03  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_INDEX_JOB_HPP_
#define CLS_INDEX_JOB_HPP_

#include "rtnBackgroundJob.hpp"
#include "clsMgr.hpp"
#include "pmdDummySession.hpp"

using namespace bson ;

namespace engine
{
   enum CLS_INDEX_THREAD_MODE
   {
      CLS_INDEX_NORMAL          = 0,
      CLS_INDEX_ROLLBACK        = 1,  // rollback task
      CLS_INDEX_ROLLBACK_CANCEL = 2,  // cancel task
      CLS_INDEX_RESTART         = 3,  // retry task, restart thread

      CLS_INDEX_UNKNOWN         = 255
   } ;

   class _clsIndexJob : public _rtnIndexJob
   {
      public:
         _clsIndexJob( RTN_JOB_TYPE type,
                       UINT32 locationID,
                       clsIdxTask* pTask ) ;
         _clsIndexJob( RTN_JOB_TYPE type,
                       dmsIdxTaskStatusPtr idxStatPtr,
                       CLS_INDEX_THREAD_MODE threadMod ) ;
         virtual ~_clsIndexJob() {} ;

         virtual INT32 init () ;
         virtual INT32 doit () ;

      protected:
         virtual void _onAttach() ;
         virtual void _onDetach() ;

         virtual INT32 _onDoit( INT32 resultCode ) ;
         virtual BOOLEAN _needRetry( INT32 rc, BOOLEAN &retryLater ) ;

      private:
         void _clean() ;
         INT32 _waitIndexAllInvalid( IRemoteOperator* pRemoteOpr,
                                     const CHAR* collectionName,
                                     const CHAR* indexName ) ;
         INT32 _startCatalogTask( UINT64 taskID ) ;
         INT32 _checkAndFixCLNameByID( BOOLEAN& isOk ) ;
         INT32 _buildTaskStatus() ;
         BOOLEAN _isCLNameExist() ;

      private:
         CLS_INDEX_THREAD_MODE _threadMode ;
         BOOLEAN               _hasSetIndexObj ; // protect _indexObj
         BOOLEAN               _retryLater ;
         BOOLEAN               _checkTasks ;
   };
   typedef class _clsIndexJob clsIndexJob ;

   INT32 clsStartIndexJob( RTN_JOB_TYPE jobType, UINT32 locationID,
                           clsIdxTask* pTask ) ;
   INT32 clsStartRollbackIndexJob( RTN_JOB_TYPE jobType,
                                   dmsIdxTaskStatusPtr idxStatPtr,
                                   BOOLEAN forCancel ) ;
   INT32 clsRestartIndexJob( RTN_JOB_TYPE jobType,
                             dmsIdxTaskStatusPtr idxStatPtr ) ;

   class _clsReportTaskInfoJob : public _rtnBaseJob
   {
      public:
         _clsReportTaskInfoJob() ;
         virtual ~_clsReportTaskInfoJob() {} ;

         virtual RTN_JOB_TYPE type() const ;
         virtual const CHAR* name() const ;
         virtual BOOLEAN muteXOn( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit() ;

      protected:
         virtual void _onDone() ;

      private:
         INT32 _reportTaskInfo2Cata( UINT64 cataTaskID,
                                     const BSONObj &idxStat ) ;

      private:
         clsCB*               _pClsCB ;
         shardCB*             _pShardCB ;
         dmsTaskStatusMgr*    _pTaskStatMgr ;
   } ;
   typedef class _clsReportTaskInfoJob clsReportTaskInfoJob ;

   INT32 clsStartReportTaskInfoJob( EDUID* pEDUID  = NULL ) ;
}

#endif //CLS_INDEX_JOB_HPP_

