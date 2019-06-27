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

   Source File Name = omStrategyObserverJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/15/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "omStrategyObserverJob.hpp"
#include "omStrategyMgr.hpp"

namespace engine
{

   /*
      _omStrategyObserverJob imlement
   */
   _omStrategyObserverJob::_omStrategyObserverJob()
   {
   }

   _omStrategyObserverJob::~_omStrategyObserverJob()
   {
   }

   RTN_JOB_TYPE _omStrategyObserverJob::type () const
   {
      return RTN_JOB_STRATEGYOBSERVER ;
   }

   const CHAR* _omStrategyObserverJob::name () const
   {
      return "StrategyObserverJob" ;
   }

   BOOLEAN _omStrategyObserverJob::muteXOn ( const _rtnBaseJob *pOther )
   {
      return FALSE ;
   }

   INT32 _omStrategyObserverJob::doit ()
   {
      omStrategyMgr *pMgr = omGetStrategyMgr() ;
      omSdbAdaptor *pAdaptor = pMgr->getSdbAdaptor() ;
      set<omStrategyChangeKey> setBiz ;
      set<omStrategyChangeKey>::iterator it ;

      while( !eduCB()->isInterrupted() )
      {
         ossSleep( OSS_ONE_SEC ) ;

         if ( pMgr->popTimeoutBusiness( OSS_ONE_SEC, setBiz ) > 0 )
         {
            it = setBiz.begin() ;
            while( it != setBiz.end() )
            {
               const omStrategyChangeKey &item = *it ;
               ++it ;

               pAdaptor->notifyStrategyChanged( item._clsName,
                                                item._bizName,
                                                eduCB() ) ;
            }
         }
      }

      return SDB_OK ;
   }

   /*
      Global Functions Implement
   */

   INT32 omStartStrategyObserverJob( EDUID *pEduID )
   {
      INT32 rc = SDB_OK ;
      omStrategyObserverJob *pJob = NULL ;

      pJob = SDB_OSS_NEW omStrategyObserverJob() ;
      if ( !pJob )
      {
         PD_LOG( PDERROR, "Allocate strategy observer job failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = rtnGetJobMgr()->startJob( pJob, RTN_JOB_MUTEX_REUSE,
                                     pEduID, FALSE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start strategy observer job failed, rc: %d",
                 rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

