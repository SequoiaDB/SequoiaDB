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

   Source File Name = dmsIndexWriteGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_INDEX_WRITE_GUARD_HPP_
#define SDB_DMS_INDEX_WRITE_GUARD_HPP_

#include "ossUtil.hpp"
#include "dmsIndexBuildGuard.hpp"

namespace engine
{

   // forward declaration
   class _pmdEDUCB ;
   class _dmsStorageDataCommon ;
   class _dmsMBContext ;

   /*
      _dmsIndexWriteGuard define
    */
   class _dmsIndexWriteGuard : public SDBObject
   {
   public:
      _dmsIndexWriteGuard() ;
      _dmsIndexWriteGuard( _pmdEDUCB *cb,
                           BOOLEAN isEnabled = TRUE ) ;
      ~_dmsIndexWriteGuard() ;

      INT32 lock( const dmsIdxMetadataKey &metadataKey,
                  UINT32 indexID,
                  const ixmIndexCB &indexCB,
                  const dmsRecordID &rid,
                  dmsIndexBuildGuardPtr &guardPtr ) ;

      INT32 begin() ;
      INT32 commit() ;
      INT32 abort( BOOLEAN isForced = FALSE ) ;

      BOOLEAN isEnabled() const
      {
         return _isEnabled ;
      }

      BOOLEAN checkNeedProcess( UINT32 indexID )
      {
         return _processMap.testBit( indexID ) ;
      }

      void setNeedProcess( UINT32 indexID )
      {
         _processMap.setBit( indexID ) ;
      }

      BOOLEAN isSet( const dmsIdxMetadataKey &metadataKey,
                     const dmsRecordID &rid ) ;

   protected:
      _pmdEDUCB *_eduCB ;
      BOOLEAN _isEnabled ;
      dmsRecordID _rid ;
      typedef ossPoolMap<dmsIdxMetadataKey,
                         dmsIndexBuildGuardPtr> _dmsIdxBuildGuardRIDMap ;
      typedef _dmsIdxBuildGuardRIDMap::iterator _dmsRIDIdxBuildGuardMapIter ;
      _dmsIdxBuildGuardRIDMap _guards ;
      _utilStackBitmap<DMS_COLLECTION_MAX_INDEX> _processMap ;
   } ;

   typedef class _dmsIndexWriteGuard dmsIndexWriteGuard ;

}

#endif // SDB_DMS_INDEX_WRITE_GUARD_HPP_
