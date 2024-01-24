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

   Source File Name = dmsPersistUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_PERSIST_UNIT_HPP_
#define SDB_DMS_PERSIST_UNIT_HPP_

#include "dmsDef.hpp"
#include "interface/IPersistUnit.hpp"
#include "utilUniqueID.hpp"

namespace engine
{

   // forward declaration
   class _dmsStorageDataCommon ;
   class _dmsMBStatInfo ;

   /*
      dmsPersistUnitState define
    */
   enum class _dmsPersistUnitState
   {
      INACTIVE,
      ACTIVE,
      ACTIVE_IN_TRANS,
      PREPARED,
   } ;
   typedef enum _dmsPersistUnitState dmsPersistUnitState ;

   /*
      _dmsStatPersistUnit define
    */
   class _dmsStatPersistUnit : public IStatPersistUnit
   {
   public:
      _dmsStatPersistUnit( utilCLUniqueID clUID,
                           _dmsStorageDataCommon *su,
                           _dmsMBStatInfo *mbStat ) ;
      virtual ~_dmsStatPersistUnit() ;
      _dmsStatPersistUnit( const _dmsStatPersistUnit &o ) = delete ;
      _dmsStatPersistUnit &operator =( const _dmsStatPersistUnit & ) = delete ;

   public:
      virtual INT32 commitUnit( IExecutor *executor ) ;
      virtual INT32 abortUnit( IExecutor *executor ) ;

      utilCLUniqueID getCLUniqueID() const
      {
         return _clUID ;
      }

      void incRecordCount( UINT64 recordCount )
      {
         _recordCountIncDelta += recordCount ;
      }

      void decRecordCount( UINT64 recordCount )
      {
         _recordCountDecDelta += recordCount ;
      }

      void incDataLen( UINT64 dataLen )
      {
         _dataLenIncDelta += dataLen ;
      }

      void decDataLen( UINT64 dataLen )
      {
         _dataLenDecDelta += dataLen ;
      }

      void incOrgDataLen( UINT64 orgDataLen )
      {
         _orgDataLenIncDelta += orgDataLen ;
      }

      void decOrgDataLen( UINT64 orgDataLen )
      {
         _orgDataLenDecDelta += orgDataLen ;
      }

      static utilThreadLocalPtr<_dmsStatPersistUnit> makeThreadLocalPtr(
                                                      utilCLUniqueID clUID,
                                                      _dmsStorageDataCommon *su,
                                                      _dmsMBStatInfo *mbStat ) ;

   protected:
      utilCLUniqueID _clUID ;
      _dmsStorageDataCommon *_su = nullptr ;
      _dmsMBStatInfo *_mbStat = nullptr ;
      UINT64 _recordCountIncDelta = 0 ;
      UINT64 _recordCountDecDelta = 0 ;
      UINT64 _dataLenIncDelta = 0 ;
      UINT64 _dataLenDecDelta = 0 ;
      UINT64 _orgDataLenIncDelta = 0 ;
      UINT64 _orgDataLenDecDelta = 0 ;
   } ;

   typedef class _dmsStatPersistUnit dmsStatPersistUnit ;

   /*
      _dmsPersistUnit define
    */
   class _dmsPersistUnit : public IPersistUnit
   {
   public:
      _dmsPersistUnit() ;
      virtual ~_dmsPersistUnit() ;
      _dmsPersistUnit( const _dmsPersistUnit &o ) = delete ;
      _dmsPersistUnit &operator =( const _dmsPersistUnit & ) = delete ;

   public:
      virtual INT32 beginUnit( IExecutor *executor,
                               BOOLEAN isTrans ) ;
      virtual INT32 prepareUnit( IExecutor *executor,
                                 BOOLEAN isTrans ) ;
      virtual INT32 commitUnit( IExecutor *executor,
                                BOOLEAN isTrans ) ;
      virtual INT32 abortUnit( IExecutor *executor,
                               BOOLEAN isTrans,
                               BOOLEAN isForced ) ;

      virtual INT32 registerStatUnit( utilThreadLocalPtr<IStatPersistUnit> &statUnitPtr ) ;

   protected:
      virtual INT32 _beginUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _prepareUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _commitUnit( IExecutor *executor ) = 0 ;
      virtual INT32 _abortUnit( IExecutor *executor ) = 0 ;

      virtual BOOLEAN _isTransSupported() const = 0 ;

   protected:
      dmsPersistUnitState _state = dmsPersistUnitState::INACTIVE ;
      UINT32 _activeLevel = 0 ;

      typedef ossPoolMap<utilCLUniqueID, utilThreadLocalPtr<IStatPersistUnit>> _dmsStatPersistUnitMap ;
      _dmsStatPersistUnitMap _statMap ;
   } ;

   typedef class _dmsPersistUnit dmsPersistUnit ;

}

#endif // SDB_DMS_PERSIST_UNIT_HPP_