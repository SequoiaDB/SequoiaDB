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

   Source File Name = pmdSyncMgr.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/14/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_SYNC_MGR_HPP__
#define PMD_SYNC_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "sdbIPersistence.hpp"
#include "dpsLogWrapper.hpp"
#include "rtnBackgroundJobBase.hpp"
#include "ossMemPool.hpp"

using namespace std ;

namespace engine
{

   /*
      _pmdSyncMgr define
   */
   class _pmdSyncMgr : public IDataSyncManager
   {
      typedef ossPoolList<IDataSyncBase*>          LIST_UNIT ;

      public:
         _pmdSyncMgr() ;
         virtual ~_pmdSyncMgr() ;

         INT32             init( UINT32 maxSyncJob,
                                 BOOLEAN syncDeep ) ;
         void              fini() ;

         void              setLogAccess( ILogAccessor *pLogAccess ) ;

         void              exitJob( BOOLEAN isControl ) ;

         ossEvent*         getEvent() { return &_wakeUpEvent ; }
         ossEvent*         getNtyEvent() { return &_ntyEvent ; }

         BOOLEAN           isSyncDeep() const { return _syncDeep ; }
         void              setSyncDeep( BOOLEAN syncDeep ) ;

         void              setMaxSyncJob( UINT32 maxSyncJob ) ;

         UINT64            syncAndGetLastLSN() ;
         UINT64            getCompleteLSN() const { return _completeLSN ; }

         void              setMainUnit( IDataSyncBase *pUnit ) ;
         IDataSyncBase*    getMainUnit() { return _pMainUnit ; }

      public:
         /*
            IDataSyncManager Functions
         */
         virtual void         registerSync( IDataSyncBase *pSyncUnit ) ;
         virtual void         unregSync( IDataSyncBase *pSyncUnit ) ;
         virtual void         notifyChange() ;

         IDataSyncBase*       dispatchUnit() ;
         void                 pushBackUnit( IDataSyncBase *pUnit ) ;

         void                 checkLoad() ;

      protected:
         void                 _checkAndStartJob( BOOLEAN needLock = TRUE ) ;

      protected:
         /*
            Unit Management Members
         */
         LIST_UNIT            _unitList ;
         ossSpinXLatch        _unitLatch ;
         BOOLEAN              _startCtrlJob ;
         IDataSyncBase        *_pMainUnit ;

         UINT32               _curAgent ;
         UINT32               _idleAgent ;
         UINT32               _maxSyncJob ;
         BOOLEAN              _syncDeep ;
         ILogAccessor         *_pLogAccessor ;
         UINT64               _completeLSN ;

         ossEvent             _ntyEvent ;
         ossEvent             _wakeUpEvent ;
   } ;
   typedef _pmdSyncMgr pmdSyncMgr ;

   /*
      _pmdSyncJob define
   */
   class _pmdSyncJob : public _rtnBaseJob
   {
      public:
         _pmdSyncJob( pmdSyncMgr *pMgr, INT32 timeout = -1 ) ;
         virtual ~_pmdSyncJob() ;

         BOOLEAN isControlJob() const ;

      public:
         virtual RTN_JOB_TYPE type () const ;
         virtual const CHAR* name () const ;
         virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;
         virtual INT32 doit () ;

         virtual BOOLEAN reuseEDU() const { return TRUE ; }

      protected:
         void           _doUnit( IDataSyncBase *pUnit ) ;

      protected:
         pmdSyncMgr             *_pMgr ;
         INT32                   _timeout ;
   } ;
   typedef _pmdSyncJob pmdSyncJob ;

   /*
      Start a Sync Job
   */
   INT32 pmdStartSyncJob( EDUID *pEDUID, pmdSyncMgr *pMgr,
                          INT32 timeout ) ;


}

#endif //PMD_SYNC_MGR_HPP__

