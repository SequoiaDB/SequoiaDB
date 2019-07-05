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

   Source File Name = coordOmStrategyJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordOmStrategyJob.hpp"
#include "pmd.hpp"
#include "coordCB.hpp"

namespace engine
{

   #define COORD_OM_UPDATE_OPR_TIMEOUT    ( 120 * OSS_ONE_SEC )   /// 2 mins
   #define COORD_OM_UPDATE_OPR_INTERVAL   ( 3600 * OSS_ONE_SEC )  /// 1 hour
   #define COORD_OM_UPDATE_OPR_RETRY      ( 10 * OSS_ONE_SEC )    /// 10 secs

   /*
      _coordOmStrategyJob implement
   */
   _coordOmStrategyJob::_coordOmStrategyJob()
   {
   }

   _coordOmStrategyJob::~_coordOmStrategyJob()
   {
   }

   RTN_JOB_TYPE _coordOmStrategyJob::type () const
   {
      return RTN_JOB_UPDATESTRATEGY ;
   }

   const CHAR* _coordOmStrategyJob::name () const
   {
      return "OmStrategySyncJob" ;
   }

   BOOLEAN _coordOmStrategyJob::muteXOn ( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   void _coordOmStrategyJob::_onAttach()
   {
      CoordCB *pCoord = pmdGetKRCB()->getCoordCB() ;
      pCoord->getRSManager()->registerEDU( eduCB() ) ;
   }

   void _coordOmStrategyJob::_onDetach()
   {
      CoordCB *pCoord = pmdGetKRCB()->getCoordCB() ;
      pCoord->getRSManager()->unregEUD( eduCB() ) ;
   }

   INT32 _coordOmStrategyJob::doit ()
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      CoordCB *pCoord = krcb->getCoordCB() ;
      coordResource *pResource = pCoord->getResource() ;
      coordOmStrategyAgent *pOmAgent = pResource->getOmStrategyAgent() ;
      CoordGroupInfoPtr omGroupPtr ;
      INT64 timeCount = 0 ;
      INT64 timeWait = COORD_OM_UPDATE_OPR_RETRY ;

      omGroupPtr = pResource->getOmGroupInfo() ;

      if ( 0 == omGroupPtr->nodeCount() )
      {
         PD_LOG( PDEVENT, "Om's address is not configured, stop "
                 "job[OmStrategySyncJob]" ) ;
         goto done ;
      }

      while( !eduCB()->isInterrupted() )
      {
         if ( timeCount >= timeWait )
         {
            timeCount = 0 ;
            rc = pOmAgent->update( eduCB(), COORD_OM_UPDATE_OPR_TIMEOUT ) ;
            if ( SDB_OK == rc )
            {
               timeWait = COORD_OM_UPDATE_OPR_INTERVAL ;
            }
            else
            {
               timeWait = COORD_OM_UPDATE_OPR_RETRY ;
            }
         }
         else if ( SDB_OK == pOmAgent->waitChange( OSS_ONE_SEC ) )
         {
            timeCount = timeWait ;
         }
         else
         {
            timeCount += OSS_ONE_SEC ;
         }
      }

   done:
      return rc ;
   }

   /*
      Gloable Function implement
   */
   INT32 coordStartOmStrategyJob( EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      coordOmStrategyJob *pJob = NULL ;

      pJob = SDB_OSS_NEW coordOmStrategyJob() ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate om strategy job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start om strategy job failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

