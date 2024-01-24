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

   Source File Name = dmsDataWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_DATA_WRITE_GUARD_HPP_
#define SDB_DMS_DATA_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "dms.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;
   class _dmsMBStatInfo ;

   /*
      _dmsDataWriteGuard define
    */
   class _dmsDataWriteGuard : public SDBObject
   {
   public:
      _dmsDataWriteGuard() ;
      _dmsDataWriteGuard( _dmsStorageDataCommon *su,
                          _dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          BOOLEAN isEnabled = TRUE ) ;
      ~_dmsDataWriteGuard() ;

      void beforeWrite() ;
      void afterWrite() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

   protected:
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      UINT16 _mbID ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      BOOLEAN _isInWrite ;
   } ;

   typedef class _dmsDataWriteGuard dmsDataWriteGuard ;

}

#endif // SDB_DMS_DATA_WRITE_GUARD_HPP_
