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

   Source File Name = pmdTransExecutor.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_TRANS_EXECUTOR_HPP__
#define PMD_TRANS_EXECUTOR_HPP__

#include "dpsTransExecutor.hpp"

namespace engine
{

   class _pmdEDUCB ;

   /*
      _pmdTransExecutor define
   */
   class _pmdTransExecutor : public _dpsTransExecutor
   {
      public:
         _pmdTransExecutor( _pmdEDUCB *cb, monMonitorManager *monMgr ) ;
         virtual ~_pmdTransExecutor() ;

      public:
         /*
            Interface
         */
         virtual EDUID        getEDUID() const ;
         virtual UINT32       getTID() const ;
         virtual void         wakeup( INT32 wakeupRC ) ;
         virtual INT32        wait( INT64 timeout ) ;
         virtual IExecutor*   getExecutor() ;
         virtual BOOLEAN      isInterrupted () ;

      protected:
         _pmdEDUCB            *_pEDUCB ;

   } ;
   typedef _pmdTransExecutor pmdTransExecutor ;

}

#endif // PMD_TRANS_EXECUTOR_HPP__

