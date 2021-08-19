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

   Source File Name = dpsTransDef.hpp 

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/28/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPS_TRANS_DEF_HPP__
#define DPS_TRANS_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"

namespace engine
{

   /*
      TRANS_ISOLATION_LEVEL define
   */
   enum TRANS_ISOLATION_LEVEL
   {
      TRANS_ISOLATION_RU = 0, // READ UNCOMMITTED
      TRANS_ISOLATION_RC = 1, // READ COMMITTED
      TRANS_ISOLATION_RS = 2, // READ STABILITY
    //TRANS_ISOLATION_RR = 3, // REPEATABLE READ

      TRANS_ISOLATION_MAX
   } ;

   /*
      DPS_TRANSLOCK_OP_MODE_TYPE define
   */
   enum DPS_TRANSLOCK_OP_MODE_TYPE
   {
      DPS_TRANSLOCK_OP_MODE_TRY = 0,
      DPS_TRANSLOCK_OP_MODE_ACQUIRE,
      DPS_TRANSLOCK_OP_MODE_TEST,
      DPS_TRANSLOCK_OP_MODE_TEST_PREEMPT
   } ;

   /*
      DPS_TRANS_STATUS define
   */
   enum DPS_TRANS_STATUS
   {
      // transaction is doing
      DPS_TRANS_DOING         = 1,
      // transaction is waiting for commit
      // ( have processed pre-commit request )
      DPS_TRANS_WAIT_COMMIT   = 2,
      // transaction is committed
      DPS_TRANS_COMMIT        = 3,
      // transaction is rollbacked
      DPS_TRANS_ROLLBACK      = 4,
      // transaction is doing but interrupted (it is going to rollback)
      DPS_TRANS_DOING_INTERRUPT = 5,
      // transaction is not found in history map ( cleared by gc )
      DPS_TRANS_UNKNOWN       = 99
   } ;

   #define DPS_TRANS_ISOLATION_DFT        TRANS_ISOLATION_RU
   #define DPS_TRANS_DFT_TIMEOUT          (60)  /* 1 minute */
   #define DPS_TRANS_LOCKWAIT_DFT         FALSE
   #define DPS_TRANS_AUTOCOMMIT_DFT       FALSE
   #define DPS_TRANS_AUTOROLLBACK_DFT     TRUE
   #define DPS_TRANS_USE_RBS_DFT          TRUE
   #define DPS_TRANS_RCCOUNT_DFT          TRUE

   /*
      TRANS CONFIG MASK
   */
   #define TRANS_CONF_MASK_ISOLATION         0x00000001
   #define TRANS_CONF_MASK_TIMEOUT           0x00000002
   #define TRANS_CONF_MASK_WAITLOCK          0x00000004
   #define TRANS_CONF_MASK_USERBS            0x00000008
   #define TRANS_CONF_MASK_AUTOCOMMIT        0x00000010
   #define TRANS_CONF_MASK_AUTOROLLBACK      0x00000020
   #define TRANS_CONF_MASK_RCCOUNT           0x00000040

   /*
      _dpsTransEvent define
   */
   class _dpsTransEvent
   {
      public:
         _dpsTransEvent() {}
         virtual ~_dpsTransEvent() {}

         virtual INT32  onRollbackAll() = 0 ;
   } ;
   typedef _dpsTransEvent dpsTransEvent ;

}

#endif // DPS_TRANS_DEF_HPP__

