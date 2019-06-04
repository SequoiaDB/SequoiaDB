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

   Source File Name = rtnBackgroundJobBase.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/06/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnBackgroundJobBase.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   rtnJobMgr * rtnGetJobMgr ()
   {
      static rtnJobMgr _jobMgr ( pmdGetKRCB()->getEDUMgr() ) ;
      return &_jobMgr ;
   }

   /*
      _rtnJobMgr implement
   */
   _rtnJobMgr::_rtnJobMgr ( pmdEDUMgr * eduMgr )
   {
      SDB_ASSERT ( eduMgr, "EDU Mgr can't be NULL" ) ;
      _eduMgr = eduMgr ;
   }

   _rtnJobMgr::~_rtnJobMgr ()
   {
      _eduMgr = NULL ;
   }

   UINT32 _rtnJobMgr::jobsCount ()
   {
      ossScopedLock lock ( &_latch, SHARED ) ;
      return _mapJobs.size() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR_FINDJOB, "_rtnJobMgr::findJob" )
   _rtnBaseJob* _rtnJobMgr::findJob( EDUID eduID, INT32 *pResult )
   {
      PD_TRACE_ENTRY ( SDB__RTNJOBMGR_FINDJOB ) ;

      {
         ossScopedLock lock ( &_latch, SHARED ) ;
         std::map<EDUID, _rtnBaseJob*>::iterator it = _mapJobs.find( eduID ) ;
         if ( it != _mapJobs.end() )
         {
            PD_TRACE_EXIT ( SDB__RTNJOBMGR_FINDJOB ) ;
            return it->second ;
         }
      }

      INT32 res = SDB_OK ;
      {
         ossScopedLock lock ( &_latch, EXCLUSIVE ) ;
         std::map<EDUID, INT32>::iterator itRes = _mapResult.find( eduID ) ;
         if ( itRes != _mapResult.end() )
         {
            res = itRes->second ;
            _mapResult.erase( itRes ) ;
         }
      }
      if ( pResult )
      {
         *pResult = res ;
      }
      PD_TRACE_EXIT ( SDB__RTNJOBMGR_FINDJOB ) ;
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR_STARTJOB, "_rtnJobMgr::startJob" )
   INT32 _rtnJobMgr::startJob ( _rtnBaseJob * pJob,
                                RTN_JOB_MUTEX_TYPE type ,
                                EDUID * pEDUID,
                                BOOLEAN returnResult )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNJOBMGR_STARTJOB ) ;
      EDUID newEDUID = 0 ;
      BOOLEAN isMuted = FALSE ;

      ossScopedLock lock ( &_latch, EXCLUSIVE ) ;

      if ( RTN_JOB_MUTEX_NONE != type )
      {
         _rtnBaseJob *itJob = NULL ;
         std::map<EDUID, _rtnBaseJob*>::iterator it = _mapJobs.begin() ;
         while ( it != _mapJobs.end() )
         {
            itJob = it->second ;
            if ( pJob->muteXOn( itJob ) || itJob->muteXOn( pJob ) )
            {
               isMuted = TRUE ;
               PD_LOG ( PDINFO, "Exist job[%s] mutex with new job[%s]",
                        itJob->name(), pJob->name() ) ;

               if ( RTN_JOB_MUTEX_RET == type )
               {
                  rc = SDB_RTN_MUTEX_JOB_EXIST ;
                  goto error ;
               }
               else if ( RTN_JOB_MUTEX_REUSE == type )
               {
                  if ( pEDUID )
                  {
                     *pEDUID = it->first ;
                  }
                  SDB_OSS_DEL pJob ;
                  pJob = NULL ;
                  goto done ;
               }
               else
               {
                  _stopJob ( it->first ) ;
                  _latchRemove.get() ;
                  _latch.release() ;
                  itJob->waitDetach () ;
                  ossSleep( 2 ) ;
                  _latch.get() ;
                  _latchRemove.release() ;
                  it = _mapJobs.begin() ;
                  continue ;
               }
            }
            ++it ;
         }
      }

      if ( isMuted && RTN_JOB_MUTEX_STOP_RET == type )
      {
         rc = SDB_RTN_MUTEX_JOB_EXIST ;
         goto error ;
      }

      rc = _eduMgr->startEDU( EDU_TYPE_BACKGROUND_JOB, (void*)pJob,
                              &newEDUID, pJob->name() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Start background job[%s] failed, rc = %d",
                  pJob->name() , rc ) ;
         goto error ;
      }

      while( SDB_OK != pJob->waitAttach( OSS_ONE_SEC ) )
      {
         if ( !_eduMgr->getEDUByID( newEDUID ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      _mapJobs[newEDUID] = pJob ;

      if ( pEDUID )
      {
         *pEDUID = newEDUID ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNJOBMGR_STARTJOB, rc ) ;
      return rc ;
   error:
      SDB_OSS_DEL pJob ;
      goto done ;
   }

   INT32 _rtnJobMgr::_stopJob ( EDUID eduID )
   {
      return _eduMgr->forceUserEDU ( eduID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNJOBMGR__REMOVEJOB, "_rtnJobMgr::_removeJob" )
   INT32 _rtnJobMgr::_removeJob ( EDUID eduID, INT32 result )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNJOBMGR__REMOVEJOB ) ;
      rtnBaseJob *pJob = NULL ;

      _latch.get() ;

      /*std::map<EDUID, INT32>::iterator itRes = _mapResult.find( eduID ) ;
      if ( itRes != _mapResult.end() )
      {
         itRes->second = result ;
      }*/
      if ( result )
      {
         _mapResult[ eduID ] = result ;
      }

      std::map<EDUID, _rtnBaseJob*>::iterator it = _mapJobs.find ( eduID ) ;
      if ( it == _mapJobs.end() )
      {
         rc = SDB_RTN_JOB_NOT_EXIST ;
      }
      else
      {
         pJob = it->second ;
         _mapJobs.erase ( it ) ;
      }

      _latch.release() ;

      if ( pJob )
      {
         ossScopedLock lock( &_latchRemove, EXCLUSIVE ) ;
         SDB_OSS_DEL pJob ;
      }

      PD_TRACE_EXITRC ( SDB__RTNJOBMGR__REMOVEJOB, rc ) ;
      return rc ;
   }

   /*
      _rtnBaseJob implement
   */
   _rtnBaseJob::_rtnBaseJob ()
   {
      _pEDUCB = NULL ;
      _evtIn.reset() ;
      _evtOut.signal() ;
   }

   _rtnBaseJob::~_rtnBaseJob ()
   {
   }

   INT32 _rtnBaseJob::attachIn ( pmdEDUCB * cb )
   {
      _pEDUCB = cb ;
      _evtOut.reset() ;
      _evtIn.signal() ;

      _onAttach() ;

      return SDB_OK ;
   }

   INT32 _rtnBaseJob::attachOut ()
   {
      _onDetach() ;

      _evtOut.signal() ;
      _pEDUCB = NULL ;
      return SDB_OK ;      
   }

   void _rtnBaseJob::_onAttach()
   {
   }

   void _rtnBaseJob::_onDetach()
   {
   }

   INT32 _rtnBaseJob::waitAttach ( INT64 millsec )
   {
      return _evtIn.wait( millsec ) ;
   }

   INT32 _rtnBaseJob::waitDetach ( INT64 millsec )
   {
      return _evtOut.wait( millsec ) ;
   }

   pmdEDUCB * _rtnBaseJob::eduCB ()
   {
      if ( !_pEDUCB )
      {
         return pmdGetThreadEDUCB() ;
      }
      return _pEDUCB ;
   }

}

