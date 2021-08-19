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
      // fifo queue, reserve operation always reserve the first one
      queue<UINT16>        _freeCollections ;
      // a reserved temp table will be stored in this map, with their EDU ID
      map<UINT16, UINT64>  _occupiedCollections ;

   public :
      _dmsTempSUMgr ( _SDB_DMSCB *dmsCB ) ;

      // this function verify whether SYSTEMP collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      INT32 init() ;
      void  fini() ;

      INT32 release ( _dmsMBContext *&context ) ;

      INT32 reserve ( _dmsMBContext **ppContext, UINT64 eduID ) ;

   private:
      INT32       _cleanTmpPath() ;
      void        _removeTmpSu() ;
      INT32       _removeATmpFile( const CHAR *pPath,
                                   const CHAR *pPosix ) ;

   } ;
   typedef class _dmsTempSUMgr dmsTempSUMgr ;

}

#endif //DMSTMPSUMGR_HPP__

