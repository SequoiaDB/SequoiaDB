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

   Source File Name = dmsSysSUMgr.hpp

   Descriptive Name = Data Management Service SYS Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS System Storage Unit management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMSSYSSUMGR_HPP__
#define DMSSYSSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "dms.hpp"

using namespace std ;

namespace engine
{

   class _SDB_DMSCB ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;

   /*
      _dmsSysSUMgr define
   */
   class _dmsSysSUMgr : public SDBObject
   {
      public :
         _dmsSysSUMgr ( _SDB_DMSCB *dmsCB )
         : _dmsCB( dmsCB )
         {
            _su = NULL ;
         }

         virtual ~_dmsSysSUMgr () {}

         _dmsStorageUnit *getSU ()
         {
            return _su ;
         }

      protected :
         #define DMSSYSSUMGR_XLOCK() ossScopedLock _lock(&_mutex, EXCLUSIVE)
         #define DMSSYSSUMGR_SLOCK() ossScopedLock _lock(&_mutex, SHARED)

         ossSpinSLatch        _mutex ;
         _dmsStorageUnit      *_su ;
         _SDB_DMSCB           *_dmsCB ;
   } ;


}

#endif //DMSSYSSUMGR_HPP__

