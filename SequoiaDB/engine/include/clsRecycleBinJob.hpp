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

   Source File Name = clsRecycleBinJob.hpp

   Descriptive Name = Recycle Bin Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RECYCLEBIN_JOB_HPP__
#define CLS_RECYCLEBIN_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "utilLightJobBase.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "monDMS.hpp"
#include "clsMgr.hpp"
#include "rtnCB.hpp"

using namespace std ;

namespace engine
{

   /*
      _clsDropRecycleBinItemJob define
    */
   class _clsDropRecycleBinJob : public _utilLightJob
   {
   public:
      _clsDropRecycleBinJob() ;
      virtual ~_clsDropRecycleBinJob() ;

      INT32 initDropItems( const UTIL_RECY_ITEM_LIST &recycleItems ) ;
      INT32 initDropAll() ;
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

      virtual const CHAR *name() const
      {
         return _isDropAll ? "DropRecycleBinAll" : "DropRecycleBinItem" ;
      }

      INT32 dropItem( const utilRecycleItem &recycleItem,
                      _pmdEDUCB *cb,
                      BOOLEAN checkCatalog ) ;
      INT32 dropItems( const UTIL_RECY_ITEM_LIST &recycleItems,
                       _pmdEDUCB *cb,
                       BOOLEAN checkCatalog ) ;
      INT32 dropAll( _pmdEDUCB *cb, BOOLEAN checkCatalog ) ;

   protected:
      clsRecycleBinManager *  _recycleBinMgr ;
      BOOLEAN                 _isDropAll ;
      UTIL_RECY_ITEM_LIST     _recycleItems ;
   } ;

   typedef class _clsDropRecycleBinJob clsDropRecycleBinJob ;

   INT32 clsStartDropRecycleBinItemJob( const UTIL_RECY_ITEM_LIST &recycleItems ) ;
   INT32 clsStartDropRecycleBinAllJob() ;

}

#endif // CLS_RECYCLEBIN_JOB_HPP__
