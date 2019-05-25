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

   Source File Name = dpsTransLockBucket.hpp

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

#ifndef DPSTRANSLOCKBUCKET_HPP_
#define DPSTRANSLOCKBUCKET_HPP_

#include "dpsTransLockDef.hpp"
#include "ossLatch.hpp"
#include <map>

namespace engine
{
   class dpsTransLockUnit;
   typedef std::map< dpsTransLockId, dpsTransLockUnit * > dpsTransLockUnitList;

   /*
      dpsLockBucket define
   */
   class dpsLockBucket : public SDBObject
   {
   public:
      friend class dpsTransLock;

      static void setLockTimeout( UINT32 timeout ) { _lockTimeout = timeout ; }

   protected:
      dpsLockBucket();
      ~dpsLockBucket();
      INT32 acquire( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );
      INT32 upgrade( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );
      void release( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 test( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                  DPS_TRANSLOCK_TYPE lockType );

      INT32 tryAcquire( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );

      INT32 tryAcquireOrAppend( _pmdEDUCB *eduCB,
                                const dpsTransLockId &lockId,
                                DPS_TRANSLOCK_TYPE lockType,
                                BOOLEAN appendHead = FALSE );

      BOOLEAN hasWait( const dpsTransLockId &lockId );

      INT32 waitLockX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );


   private:
      INT32 appendToRun( _pmdEDUCB *eduCB, DPS_TRANSLOCK_TYPE lockType,
                         dpsTransLockUnit *pLockUnit );

      void appendToWait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                         dpsTransLockUnit *pLockUnit );

      void appendHeadToWait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                             dpsTransLockUnit *pLockUnit );

      void removeFromWait( _pmdEDUCB *eduCB,
                           dpsTransLockUnit *pLockUnit,
                           const dpsTransLockId &lockId );

      void removeFromRun( _pmdEDUCB *eduCB,
                          dpsTransLockUnit *pLockUnit );

      void wakeUp( _pmdEDUCB *eduCB );

      BOOLEAN isLockCompatible( DPS_TRANSLOCK_TYPE first,
                                DPS_TRANSLOCK_TYPE second );

      BOOLEAN checkCompatible( _pmdEDUCB *eduCB,
                               DPS_TRANSLOCK_TYPE lockType,
                               dpsTransLockUnit *pLockUnit );

      INT32 waitLock( _pmdEDUCB *eduCB );


   private:
      dpsTransLockUnitList       _lockLst;
      ossSpinXLatch              _lstMutex;
      static ossSpinXLatch       _initMutex;
      static UINT32              _lockTimeout;  // The variable is shared by all lock-buckets
   };
}

#endif // DPSTRANSLOCKBUCKET_HPP_
