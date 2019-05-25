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

   Source File Name = dmsTempSUMgr.hpp

   Descriptive Name = DMS Temp Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSTMPSUMGR_HPP__
#define DMSTMPSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "dms.hpp"
#include "dmsSysSUMgr.hpp"
#include <queue>
#include <map>

using namespace std ;

namespace engine
{

   /*
      _dmsTempSUMgr define
   */
   class _dmsTempSUMgr : public _dmsSysSUMgr
   {
   private :
      queue<UINT16>        _freeCollections ;
      map<UINT16, UINT64>  _occupiedCollections ;

   public :
      _dmsTempSUMgr ( _SDB_DMSCB *dmsCB ) ;

      INT32 init() ;

      INT32 release ( _dmsMBContext *&context ) ;

      INT32 reserve ( _dmsMBContext **ppContext, UINT64 eduID ) ;

   private:
      INT32 _initTmpPath() ;

   } ;
   typedef class _dmsTempSUMgr dmsTempSUMgr ;

}

#endif //DMSTMPSUMGR_HPP__

