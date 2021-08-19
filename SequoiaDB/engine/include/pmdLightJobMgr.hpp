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

   Source File Name = pmdLightJobMgr.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/13/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_LIGHT_JOB_MGR_HPP__
#define PMD_LIGHT_JOB_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "utilLightJobBase.hpp"
#include "rtnBackgroundJobBase.hpp"
#include <vector>

namespace engine
{

   /*
      _pmdLightJobMgr define
   */
   class _pmdLightJobMgr : public _utilLightJobMgr
   {
      typedef std::vector<utilLightJobInfo>     LIGHT_JOB_VEC ;
      typedef LIGHT_JOB_VEC::iterator           LIGHT_JOB_VEC_IT ;

      public:
         _pmdLightJobMgr() ;
         virtual ~_pmdLightJobMgr() ;

         void              setMaxExeJob( UINT64 maxExeJob ) ;

         void              exitJob( BOOLEAN isControl ) ;

         ossEvent*         getEvent() { return &_wakeUpEvent ; }

      public:
         virtual void      push( utilLightJob *pJob,
                                 BOOLEAN takeOver = TRUE,
                                 INT32 priority = UTIL_LJOB_PRI_MID,
                                 UINT64 expectAvgCost = UTIL_LJOB_DFT_AVG_COST ) ;

         virtual void      push( const utilLightJobInfo &job ) ;

      public:
         BOOLEAN           dispatchJob( utilLightJobInfo &job ) ;
         void              pushBackJob( utilLightJobInfo &job,
                                        UTIL_LJOB_DO_RESULT result ) ;

         void              checkLoad() ;
         UINT32            processPending() ;

      protected:
         void              _checkAndStartJob( BOOLEAN needLock = TRUE ) ;

      protected:
         ossSpinXLatch        _unitLatch ;
         BOOLEAN              _startCtrlJob ;

         UINT32               _curAgent ;
         UINT32               _idleAgent ;
         UINT32               _maxExeJob ;

         ossEvent             _wakeUpEvent ;

         LIGHT_JOB_VEC        _pendingJobVec ;
   } ;
   typedef _pmdLightJobMgr pmdLightJobMgr ;

   /*
      _pmdLightJobExe define
   */
   class _pmdLightJobExe : public _rtnBaseJob
   {
      public:
         _pmdLightJobExe( pmdLightJobMgr *pJobMgr, INT32 timeout = -1 ) ;
         virtual ~_pmdLightJobExe() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN isSystem() const ;
         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      protected:
         pmdLightJobMgr          *_pJobMgr ;
         INT32                   _timeout ;
   } ;
   typedef _pmdLightJobExe pmdLightJobExe ;

   /*
      Start a Light job executor
   */
   INT32 pmdStartLightJobExe( EDUID *pEDUID, pmdLightJobMgr *pJobMgr,
                              INT32 timeout ) ;

}

#endif //PMD_LIGHT_JOB_MGR_HPP__

