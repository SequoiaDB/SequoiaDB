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

   Source File Name = dmsPersistGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_PERSIST_GUARD_HPP_
#define SDB_DMS_PERSIST_GUARD_HPP_

#include "dmsPersistUnit.hpp"
#include "ossUtil.hpp"
#include "interface/IStorageService.hpp"
#include "dms.hpp"
#include "dmsMetadata.hpp"
#include "pmdDummySession.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsPersistGuard define
    */
   class _dmsPersistGuard : public SDBObject
   {
   public:
      _dmsPersistGuard() ;
      _dmsPersistGuard( IStorageService *service,
                        _dmsStorageDataCommon *su,
                        _dmsMBContext *mbContext,
                        _pmdEDUCB *cb,
                        BOOLEAN isEnabled = TRUE ) ;
      ~_dmsPersistGuard() ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

      INT32 init() ;
      INT32 fini() ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      void incRecordCount( UINT64 count = 1 ) ;
      void decRecordCount( UINT64 count = 1 ) ;
      void incDataLen( UINT64 dataLen ) ;
      void decDataLen( UINT64 dataLen ) ;
      void incOrgDataLen( UINT64 orgDataLen ) ;
      void decOrgDataLen( UINT64 orgDataLen ) ;

   protected:
      IStorageService *_service = nullptr ;
      IPersistUnit *_persistUnit = nullptr ;
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      pmdDummySession _dummySession ;
      utilCLUniqueID _clUniqueID = UTIL_UNIQUEID_NULL ;
      utilThreadLocalPtr<dmsStatPersistUnit> _statUnitPtr ;
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      BOOLEAN _hasBegin ;
   } ;

   typedef class _dmsPersistGuard dmsPersistGuard ;

}

#endif // SDB_DMS_PERSIST_GUARD_HPP_
