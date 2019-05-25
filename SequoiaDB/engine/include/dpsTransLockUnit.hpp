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

   Source File Name = dpsTransLockUnit.hpp

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

#ifndef DPSTRANSLOCKUNIT_HPP_
#define DPSTRANSLOCKUNIT_HPP_

#include "pmdEDU.hpp"
#include <map>

namespace engine
{
   typedef std::map< UINT32, DPS_TRANSLOCK_TYPE >    dpsTransLockRunList;
   class dpsTransLockUnit : public SDBObject
   {
   public:
      dpsTransLockUnit()
      {
         _pWaitCB = NULL ;
      }
      ~dpsTransLockUnit(){};
   public:
      _pmdEDUCB   *_pWaitCB;
      dpsTransLockRunList  _runList;
   };
}

#endif // DPSTRANSLOCKUNIT_HPP_
