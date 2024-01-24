/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = lobMetaMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2023  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REVERT_LOB_MATE_MGR_HPP_
#define REVERT_LOB_MATE_MGR_HPP_

#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "dpsDef.hpp"
#include "ossLatch.hpp"
#include <map>
#include <vector>

using namespace std ;

namespace sdbrevert
{
   class lobMeta : public SDBObject
   {
      public:
         lobMeta( const bson::OID &oid, UINT32 sequence ) ;
         ~lobMeta() ;
         BOOLEAN operator<( const lobMeta& other ) const ;

      private:
         bson::OID _oid ;
         UINT32    _sequence ;
   } ;

   class lobMetaMgr : public SDBObject
   {
      public:
         lobMetaMgr() ;
         ~lobMetaMgr() ;
         BOOLEAN checkLSN( const bson::OID &oid, UINT32 sequence, DPS_LSN_OFFSET lsn ) ;

      private:
         map<lobMeta, DPS_LSN_OFFSET>   _lobMetaMap ;
         ossSpinXLatch                  _mutex ;
   } ;

   class lobLockMap : public SDBObject
   {
      public:
         lobLockMap( UINT32 size ) ;
         ~lobLockMap() ;
         INT32 getLobLock( const bson::OID &oid, ossSpinXLatch *&lobLock ) ;

      private:
         UINT32                  _size ;
         vector<ossSpinXLatch*>  _lobLockVec ;
   } ;
}

#endif /* REVERT_LOB_MATE_MGR_HPP_ */
