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

   Source File Name = utilRecycleItem.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle item.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_RECYCLE_ITEM_HPP__
#define UTIL_RECYCLE_ITEM_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"
#include "utilGlobalID.hpp"
#include "dms.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   // format of name of recycle item
   // SYSRECYCLE_<recycleID>_<originID>
   #define UTIL_RECYCLE_PREFIX      "SYSRECYCLE_"
   #define UTIL_RECYCLE_FORMAT      UTIL_RECYCLE_PREFIX"%llu_%llu"
   #define UTIL_RECYCLE_PREFIX_SZ   ( sizeof( UTIL_RECYCLE_PREFIX ) - 1 )

   // maximum size of recycle name
   #define UTIL_RECYCLE_NAME_SZ  OSS_MIN( ( DMS_COLLECTION_SPACE_NAME_SZ ), \
                                          ( DMS_COLLECTION_NAME_SZ ) )
   // maximum size of origin name
   #define UTIL_ORIGIN_NAME_SZ   DMS_COLLECTION_FULL_NAME_SZ

   /*
      UTIL_RECYCLE_TYPE define
    */
   // type of recycle object
   enum UTIL_RECYCLE_TYPE
   {
      UTIL_RECYCLE_UNKNOWN,
      // recycle for collection space
      UTIL_RECYCLE_CS,
      // recycle for collection
      UTIL_RECYCLE_CL,
      // recycle for sequence
      UTIL_RECYCLE_SEQ,
      // recycle for index
      UTIL_RECYCLE_IDX
   } ;

   // names of recycle types
   #define UTIL_RECYCLE_CS_NAME     "CollectionSpace"
   #define UTIL_RECYCLE_CL_NAME     "Collection"
   #define UTIL_RECYCLE_SEQ_NAME    "Sequence"
   #define UTIL_RECYCLE_IDX_NAME    "Index"

   /*
      UTIL_RECYCLE_OPTYPE define
    */
   // operator type of recycle item ( which creates this recycle item )
   enum UTIL_RECYCLE_OPTYPE
   {
      UTIL_RECYCLE_OP_UNKNOWN,
      // drop operator to create this recycle item
      UTIL_RECYCLE_OP_DROP,
      // truncate operator to create this recycle item
      UTIL_RECYCLE_OP_TRUNCATE
   } ;

   // names of recycle operator types
   #define UTIL_RECYCLE_OP_DROP_NAME      "Drop"
   #define UTIL_RECYCLE_OP_TRUNCATE_NAME  "Truncate"

   // convert between recycle types to names
   const CHAR *utilGetRecycleTypeName( UTIL_RECYCLE_TYPE type ) ;
   UTIL_RECYCLE_TYPE utilGetRecycleType( const CHAR *typeName ) ;

   // convert between recycle operator types to names
   const CHAR *utilGetRecycleOpTypeName( UTIL_RECYCLE_OPTYPE type ) ;
   UTIL_RECYCLE_OPTYPE utilGetRecycleOpType( const CHAR *typeName ) ;

   INT32 utilGetRecyCLsInCSBounds( const CHAR *fieldName,
                                   utilCSUniqueID csUniqueID,
                                   bson::BSONObj &matcher ) ;

   /*
      _utilRecycleItem define
    */
   class _utilRecycleItem : public _utilPooledObject
   {
   public:
      _utilRecycleItem() ;
      _utilRecycleItem( const _utilRecycleItem &item ) ;
      _utilRecycleItem( UTIL_RECYCLE_TYPE type,
                        UTIL_RECYCLE_OPTYPE opType ) ;
      ~_utilRecycleItem() ;

      void inherit( const _utilRecycleItem &item,
                    const CHAR *originName,
                    utilCLUniqueID originID ) ;
      void init( utilRecycleID recycleID ) ;
      void reset() ;

      OSS_INLINE BOOLEAN isValid() const
      {
         return UTIL_RECYCLEID_NULL != _recycleID ;
      }

      OSS_INLINE utilRecycleID getRecycleID() const
      {
         return _recycleID ;
      }

      OSS_INLINE void setRecycleID( utilRecycleID recycleUID )
      {
         _recycleID = recycleUID ;
      }

      OSS_INLINE const CHAR *getRecycleName() const
      {
         return _recycleName ;
      }

      OSS_INLINE void setRecycleName( const CHAR *recycleName )
      {
         ossStrncpy( _recycleName, recycleName, UTIL_RECYCLE_NAME_SZ ) ;
         _recycleName[ UTIL_RECYCLE_NAME_SZ ] = '\0' ;
      }

      OSS_INLINE void setOriginID( utilCLUniqueID originID )
      {
         _originID = originID ;
      }

      OSS_INLINE utilCLUniqueID getOriginID() const
      {
         return _originID ;
      }

      OSS_INLINE const CHAR *getOriginName() const
      {
         return _originName ;
      }

      OSS_INLINE void setOriginName( const CHAR *originName )
      {
         ossStrncpy( _originName, originName, UTIL_ORIGIN_NAME_SZ ) ;
         _originName[ UTIL_ORIGIN_NAME_SZ ] = '\0' ;
      }

      OSS_INLINE UTIL_RECYCLE_TYPE getType() const
      {
         return _type ;
      }

      OSS_INLINE void setType( UTIL_RECYCLE_TYPE type )
      {
         _type = type ;
      }

      OSS_INLINE UTIL_RECYCLE_OPTYPE getOpType() const
      {
         return _opType ;
      }

      OSS_INLINE void setOpType( UTIL_RECYCLE_OPTYPE opType )
      {
         _opType = opType ;
      }

      OSS_INLINE UINT64 getRecycleTime() const
      {
         return _recycleTime ;
      }

      OSS_INLINE void setRecycleTime( UINT64 recycleTime )
      {
         _recycleTime = recycleTime ;
      }

      OSS_INLINE BOOLEAN isMainCL() const
      {
         return _isMainCL ;
      }

      OSS_INLINE void setMainCL( BOOLEAN isMainCL )
      {
         _isMainCL = isMainCL ;
      }

      OSS_INLINE BOOLEAN isCSRecycled() const
      {
         return _isCSRecycled ;
      }

      OSS_INLINE void setCSRecycled( BOOLEAN isCSRecycled )
      {
         _isCSRecycled = isCSRecycled ;
      }

      OSS_INLINE BOOLEAN isDrop() const
      {
         return UTIL_RECYCLE_OP_DROP == _opType ;
      }

      OSS_INLINE BOOLEAN isTruncate() const
      {
         return UTIL_RECYCLE_OP_TRUNCATE == _opType ;
      }

      INT32 toBSON( bson::BSONObj &object ) const ;
      INT32 toBSON( bson::BSONObjBuilder &builder ) const ;
      INT32 toBSON( bson::BSONObjBuilder &builder,
                    const CHAR *fieldName ) const ;
      INT32 fromBSON( const bson::BSONObj &object ) ;
      INT32 fromBSON( const bson::BSONObj &object, const CHAR *fieldName ) ;

      INT32 fromRecycleName( const CHAR *recycleName ) ;

      _utilRecycleItem &operator =( const _utilRecycleItem &item ) ;
      BOOLEAN operator ==( const _utilRecycleItem &item ) const ;

   protected:
      void _setRecycleName( utilRecycleID recycleID, utilCLUniqueID originID ) ;

   protected:
      // unique ID of recycle item
      utilRecycleID        _recycleID ;
      // name of recycle item
      CHAR                 _recycleName[ UTIL_RECYCLE_NAME_SZ + 1 ] ;
      // origin unique ID of recycle item
      utilCLUniqueID       _originID ;
      // origin name of recycle item
      CHAR                 _originName[ UTIL_ORIGIN_NAME_SZ + 1 ] ;
      // type of recycle item
      UTIL_RECYCLE_TYPE    _type ;
      // operator type of recycle item ( which creates this recycle item )
      UTIL_RECYCLE_OPTYPE  _opType ;
      // time of recycle item
      UINT64               _recycleTime ;
      // indicates whether this recycle item is from main-collection
      BOOLEAN              _isMainCL ;
      // indicates whether this recycle item is invalid
      // especially in the case, a recycled collection in a recycled
      // collection space
      BOOLEAN              _isCSRecycled ;
   } ;

   typedef class _utilRecycleItem utilRecycleItem ;

   typedef ossPoolList< utilRecycleItem > UTIL_RECY_ITEM_LIST ;
   typedef UTIL_RECY_ITEM_LIST::iterator UTIL_RECY_ITEM_LIST_IT ;
   typedef UTIL_RECY_ITEM_LIST::const_iterator UTIL_RECY_ITEM_LIST_CIT ;

   typedef ossPoolList< ossPoolString > UTIL_RECY_ITEM_NAME_LIST ;
   typedef UTIL_RECY_ITEM_NAME_LIST::iterator UTIL_RECY_ITEM_NAME_LIST_IT ;
   typedef UTIL_RECY_ITEM_NAME_LIST::const_iterator UTIL_RECY_ITEM_NAME_LIST_CIT ;

   // index for recycle name
   #define UTIL_RECYCLEBIN_NAME_INDEX_NAME      "SYSRECYBINNAMEIDX"
   #define UTIL_RECYCLEBIN_ITEM_NAME_INDEX \
               "{ name : \"" UTIL_RECYCLEBIN_NAME_INDEX_NAME "\", " \
               "key : { \"" FIELD_NAME_RECYCLE_NAME "\" : 1 }, " \
               "unique : true, enforced : true }"

   // index for origin name
   #define UTIL_RECYCLEBIN_ORIGNAME_INDEX_NAME  "SYSRECYBINORIGNAMEIDX"
   #define UTIL_RECYCLEBIN_ORIGNAME_INDEX \
               "{ name : \"" UTIL_RECYCLEBIN_ORIGNAME_INDEX_NAME "\", " \
               "key : { \"" FIELD_NAME_ORIGIN_NAME "\" : 1 } }"

   // index for origin ID
   // NOTE: truncate will create duplicated items with the same origin ID,
   //       so should not have unique
   #define UTIL_RECYCLEBIN_ORIGID_INDEX_NAME    "SYSRECYBINORIGIDIDX"
   #define UTIL_RECYCLEBIN_ORIGID_INDEX \
               "{ name : \"" UTIL_RECYCLEBIN_ORIGID_INDEX_NAME "\", " \
               "key : { \"" FIELD_NAME_ORIGIN_ID "\" : 1 } }"

   // index for unique ID of recycle item
   // locate recycle objects from the same recycle item
   #define UTIL_RECYCLEBIN_RECYID_INDEX_NAME    "SYSRECYBINRECYIDX"
   #define UTIL_RECYCLEBIN_RECYID_INDEX \
               "{ name : \"" UTIL_RECYCLEBIN_RECYID_INDEX_NAME "\", " \
               "key : { \"" FIELD_NAME_RECYCLE_ID "\" : 1 } }"

   // index for unique ID of object
   // locate recycle objects from its own unique ID
   #define UTIL_RECYCLEBIN_UID_INDEX_NAME       "SYSRECYBINUIDIDX"
   #define UTIL_RECYCLEBIN_UID_INDEX \
               "{ name : \"" UTIL_RECYCLEBIN_UID_INDEX_NAME "\", " \
               "key : { \"" FIELD_NAME_UNIQUEID "\" : 1 } }"

}

#endif // UTIL_RECYCLE_ITEM_HPP__
