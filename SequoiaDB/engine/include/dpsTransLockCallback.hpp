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

   Source File Name = dpsTransLockCallback.hpp

   Descriptive Name = DPS Transaction Lock Callback Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPS_TRANS_LOCK_CALLBACK_HPP__
#define DPS_TRANS_LOCK_CALLBACK_HPP__

#include "oss.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "dpsTransLRB.hpp"

namespace engine
{

   /*
      _dpsITransLockCallback interface
   */
   class _dpsITransLockCallback : public SDBObject
   {
   public:
      _dpsITransLockCallback()
      {
      }

      virtual ~_dpsITransLockCallback()
      {
      }

      /// Interface
      virtual void afterLockAcquire( const dpsTransLockId &lockId,
                                     INT32 irc,
                                     DPS_TRANSLOCK_TYPE requestLockMode,
                                     UINT32 refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     const dpsTransLRBHeader *pLRBHeader,
                                     dpsLRBExtData *pExtData ) = 0 ;

      virtual void beforeLockRelease( const dpsTransLockId &lockId,
                                      DPS_TRANSLOCK_TYPE lockMode,
                                      UINT32 refCounter,
                                      const dpsTransLRBHeader *pLRBHeader,
                                      dpsLRBExtData *pExtData ) = 0 ;

   } ;

}

#endif // DPS_TRANS_LOCK_CALLBACK_HPP__

