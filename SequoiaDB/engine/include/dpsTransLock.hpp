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

   Source File Name = dpsTransLock.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSTRANSLOCK_HPP_
#define DPSTRANSLOCK_HPP_

#include "dms.hpp"
#include "ossLatch.hpp"
#include "dpsTransLockDef.hpp"
#include <vector>

namespace engine
{
   class _pmdEDUCB;
   class dpsLockBucket;


   #define MAX_LOCKBUCKET_NUM             ( 1000 )

   /*
      dpsTransLock define
   */
   class dpsTransLock : public SDBObject
   {
   typedef std::vector< dpsLockBucket *> LockBucketLst;
   public:
      dpsTransLock();

      ~dpsTransLock();

      INT32 acquireX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 acquireS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 acquireIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 acquireIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      void release( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      void releaseAll( _pmdEDUCB *eduCB );

      INT32 testS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 testIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 testX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 testIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 tryX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 tryS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 tryIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 tryIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 tryOrAppendX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 wait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      BOOLEAN hasWait( const dpsTransLockId &lockId );


   private:
      INT32 upgrade( _pmdEDUCB *eduCB,
                     const dpsTransLockId &lockId,
                     dpsTransCBLockInfo *pLockInfo,
                     DPS_TRANSLOCK_TYPE lockType );

      INT32 testUpgrade( _pmdEDUCB *eduCB,
                         const dpsTransLockId &lockId,
                         dpsTransCBLockInfo *pLockInfo,
                         DPS_TRANSLOCK_TYPE lockType );

      INT32 tryUpgrade( _pmdEDUCB *eduCB,
                        const dpsTransLockId &lockId,
                        dpsTransCBLockInfo *pLockInfo,
                        DPS_TRANSLOCK_TYPE lockType );

      INT32 upgradeCheck( DPS_TRANSLOCK_TYPE srcType,
                          DPS_TRANSLOCK_TYPE dstType );

      INT32 getBucket( const dpsTransLockId &lockId,
                       dpsLockBucket *&lockBucket );

      UINT32 getBucketNo( const dpsTransLockId &lockId );

      INT32 tryUpgradeOrAppendHead( _pmdEDUCB *eduCB,
                                    const dpsTransLockId &lockId,
                                    dpsTransCBLockInfo *pLockInfo,
                                    DPS_TRANSLOCK_TYPE lockType ) ;

   private:
      LockBucketLst           _bucketLst;
      ossSpinXLatch           _LstMutex;

   } ;

   class DPS_TRANS_WAIT_LOCK
   {
   public:

      DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, const dpsTransLockId & lockId ) ;

      DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID,
                           const _dmsRecordID *recordID ) ;

      ~DPS_TRANS_WAIT_LOCK() ;

   private:
      _pmdEDUCB         *_eduCB ;       
   };

}

#endif //DPSTRANSLOCK_HPP_

