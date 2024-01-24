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

   Source File Name = dmsMetadata.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DMS_METADATA_HPP_
#define SDB_DMS_METADATA_HPP_

#include "dms.hpp"
#include "dmsEngineDef.hpp"
#include "msgDef.h"
#include "utilCompression.hpp"
#include "utilRecycleItem.hpp"
#include "utilRenameLogger.hpp"
#include "utilResult.hpp"
#include "../bson/bson.hpp"
#include "../bson/ordering.h"

namespace engine
{

   // forward declaration
   class _dmsSUDescriptor ;
   class _dmsMetadataBlock ;
   class _dmsMBStatInfo ;
   class _ixmIndexCB ;

   /*
      _dmsCLMetadataKey define
    */
   class _dmsCLMetadataKey : public SDBObject
   {
   public:
      _dmsCLMetadataKey() = default ;
      ~_dmsCLMetadataKey() = default ;
      _dmsCLMetadataKey( const _dmsCLMetadataKey &o ) = default ;
      _dmsCLMetadataKey &operator =( const _dmsCLMetadataKey & ) = default ;

      _dmsCLMetadataKey( utilCLUniqueID clUID,
                         UINT32 clLID )
      : _clOrigUID( clUID ),
        _clOrigLID( clLID )
      {
      }

      _dmsCLMetadataKey( const _dmsMetadataBlock *mb ) ;

      BOOLEAN operator ==( const _dmsCLMetadataKey &o ) const
      {
         return _clOrigUID == o._clOrigUID && _clOrigLID == o._clOrigLID ;
      }

      BOOLEAN operator <( const _dmsCLMetadataKey &o ) const
      {
         if ( _clOrigUID < o._clOrigUID )
         {
            return TRUE ;
         }
         else if ( _clOrigUID > o._clOrigUID )
         {
            return FALSE ;
         }
         return _clOrigLID < o._clOrigLID ;
      }

      void init( const _dmsMetadataBlock *mb ) ;

      utilCLUniqueID getCLOrigUID() const
      {
         return _clOrigUID ;
      }

      void setCLOrigUID( utilCLUniqueID origUID )
      {
         _clOrigUID = origUID ;
      }

      UINT32 getCLOrigLID() const
      {
         return _clOrigLID ;
      }

      void setCLOrigLID( UINT32 origLID )
      {
         _clOrigLID = origLID ;
      }

      BOOLEAN isValid() const
      {
         return UTIL_UNIQUEID_NULL != _clOrigUID &&
                DMS_INVALID_LOGICCLID != _clOrigLID ;
      }

   protected:
      utilCLUniqueID _clOrigUID = UTIL_UNIQUEID_NULL ;
      UINT32 _clOrigLID = DMS_INVALID_LOGICCLID ;
   } ;

   typedef class _dmsCLMetadataKey dmsCLMetadataKey ;

   /*
      _dmsIdxMetadataKey define
    */
   class _dmsIdxMetadataKey : public _dmsCLMetadataKey
   {
   public:
      _dmsIdxMetadataKey() = default ;
      ~_dmsIdxMetadataKey() = default ;
      _dmsIdxMetadataKey( const _dmsIdxMetadataKey &o ) = default ;
      _dmsIdxMetadataKey &operator =( const _dmsIdxMetadataKey & ) = default ;

      _dmsIdxMetadataKey( utilCLUniqueID clUID,
                          UINT32 clLID,
                          utilIdxInnerID idxInnerID )
      : _dmsCLMetadataKey( clUID, clLID ),
         _idxInnerID( idxInnerID )
      {
      }

      _dmsIdxMetadataKey( const _dmsMetadataBlock *mb,
                          const _ixmIndexCB *idxCB ) ;

      BOOLEAN operator ==( const _dmsIdxMetadataKey &o ) const
      {
         return ( _dmsCLMetadataKey::operator ==( o ) ) &&
                ( _idxInnerID == o._idxInnerID ) ;
      }

      BOOLEAN operator <( const _dmsIdxMetadataKey &o ) const
      {
         if ( _dmsCLMetadataKey::operator <( o ) )
         {
            return TRUE ;
         }
         else if ( _dmsCLMetadataKey::operator ==( o ) )
         {
            return _idxInnerID < o._idxInnerID ;
         }
         return FALSE ;
      }

      void init( const _dmsMetadataBlock *mb,
                 const _ixmIndexCB *idxCB ) ;

      utilIdxInnerID getIdxInnerID() const
      {
         return _idxInnerID ;
      }

      void setIdxInnerID( utilIdxInnerID idxInnerID )
      {
         _idxInnerID = idxInnerID ;
      }

      BOOLEAN isValid() const
      {
         return _dmsCLMetadataKey::isValid() &&
                UTIL_UNIQUEID_NULL != _idxInnerID ;
      }

   protected:
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL ;
   } ;

   typedef class _dmsIdxMetadataKey dmsIdxMetadataKey ;

   /*
      _dmsCSMetadata define
    */
   class _dmsCSMetadata : public SDBObject
   {
   public:
      _dmsCSMetadata() = delete ;
      virtual ~_dmsCSMetadata() = default ;
      _dmsCSMetadata( const _dmsCSMetadata &o ) = default ;
      _dmsCSMetadata &operator =( const _dmsCSMetadata & ) = default ;

      _dmsCSMetadata( _dmsSUDescriptor *su ) ;

      utilCSUniqueID getCSUID() const
      {
         return _csUID ;
      }

      _dmsSUDescriptor *getSU() const
      {
         return _su ;
      }

   protected:
      mutable _dmsSUDescriptor *_su = nullptr ;
      utilCSUniqueID _csUID = UTIL_UNIQUEID_NULL ;
   } ;

   typedef class _dmsCSMetadata dmsCSMetadata ;

   /*
      _dmsCLMetadata define
    */
   class _dmsCLMetadata : public _dmsCSMetadata
   {
   public:
      _dmsCLMetadata() = delete ;
      virtual ~_dmsCLMetadata() = default ;
      _dmsCLMetadata( const _dmsCLMetadata &o ) = default ;
      _dmsCLMetadata &operator =( const _dmsCLMetadata & ) = delete ;

      _dmsCLMetadata( _dmsSUDescriptor *su,
                      _dmsMetadataBlock *mb,
                      _dmsMBStatInfo *mbStat ) ;

      utilCLInnerID getCLInnerID() const
      {
         return _clInnerID ;
      }

      UINT32 getCLLID() const
      {
         return _clLID ;
      }

      utilCLInnerID getCLOrigInnerID() const
      {
         return _clOrigInnerID ;
      }

      UINT32 getCLOrigLID() const
      {
         return _clOrigLID ;
      }

      utilCLUniqueID getCLUID() const
      {
         return utilBuildCLUniqueID( getCSUID(), getCLInnerID() ) ;
      }

      utilCLUniqueID getOrigUID() const
      {
         return utilBuildCLUniqueID( getCSUID(), getCLOrigInnerID() ) ;
      }

      _dmsMetadataBlock *getMB() const
      {
         return _mb ;
      }

      _dmsMBStatInfo *getMBStat() const
      {
         return _mbStat ;
      }

      dmsCLMetadataKey getCLKey() const
      {
         return dmsCLMetadataKey( getOrigUID(), getCLOrigLID() ) ;
      }

      UINT64 fetchSnapshotID() ;

   protected:
      mutable _dmsMetadataBlock *_mb = nullptr ;
      mutable _dmsMBStatInfo *_mbStat = nullptr ;
      utilCLInnerID _clInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _clLID = DMS_INVALID_LOGICCLID ;
      utilCLInnerID _clOrigInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _clOrigLID = DMS_INVALID_LOGICCLID ;
   } ;

   typedef class _dmsCLMetadata dmsCLMetadata ;

   /*
      _dmsIdxMetadata define
    */
   class _dmsIdxMetadata : public _dmsCLMetadata
   {
   public:
      _dmsIdxMetadata() = delete ;
      virtual ~_dmsIdxMetadata() = default ;
      _dmsIdxMetadata( const _dmsIdxMetadata &o ) = default ;
      _dmsIdxMetadata &operator =( const _dmsIdxMetadata & ) = delete ;

      _dmsIdxMetadata( _dmsSUDescriptor *su,
                       _dmsMetadataBlock *mb,
                       _dmsMBStatInfo *mbStat,
                       _ixmIndexCB *indexCB ) ;

      _dmsIdxMetadata( const _dmsIdxMetadata &o, BOOLEAN getOwned ) ;

      utilIdxInnerID getIdxInnerID() const
      {
         return _idxInnerID ;
      }

      UINT32 getIdxLID() const
      {
         return _idxLID ;
      }

      utilIdxUniqueID getIdxUID() const
      {
         return utilBuildIdxUniqueID( getCSUID(), getIdxInnerID() ) ;
      }

      dmsIdxMetadataKey getIdxKey() const
      {
         return dmsIdxMetadataKey( getOrigUID(), getCLOrigLID(), getIdxInnerID() ) ;
      }

      const bson::BSONObj &getKeyPattern() const
      {
         return _keyPattern ;
      }

      const bson::Ordering &getOrdering() const
      {
         return _ordering ;
      }

      BOOLEAN isUnique() const
      {
         return _isUnique ;
      }

      BOOLEAN isStrictUnique() const
      {
         return _isStrictUnique ;
      }

      BOOLEAN isEnforced() const
      {
         return _isEnforced ;
      }

      BOOLEAN isNotNull() const
      {
         return _isNotNull ;
      }

      BOOLEAN isNotArray() const
      {
         return _isNotArray ;
      }

   protected:
      utilIdxInnerID _idxInnerID = UTIL_UNIQUEID_NULL ;
      UINT32 _idxLID = DMS_INVALID_EXTENT ;
      bson::BSONObj _keyPattern ;
      bson::Ordering _ordering ;
      BOOLEAN _isUnique = FALSE ;
      BOOLEAN _isStrictUnique = FALSE ;
      BOOLEAN _isEnforced = FALSE ;
      BOOLEAN _isNotNull = FALSE ;
      BOOLEAN _isNotArray = FALSE ;
   } ;

   typedef class _dmsIdxMetadata dmsIdxMetadata ;

}

#endif // SDB_DMS_METADATA_HPP_
