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

   Source File Name = utilRecycleBinConf.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_RECYCLEBIN_CONF_HPP__
#define UTIL_RECYCLEBIN_CONF_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   // default value for Enable field
   #define UTIL_RECYCLEBIN_DFT_ENABLE        ( TRUE )
   // default value for AutoDrop field
   #define UTIL_RECYCLEBIN_DFT_AUTODROP      ( FALSE )
   // default value for ExpireTime field, in minutes
   #define UTIL_RECYCLEBIN_DFT_EXPIRETIME    ( 4320 )
   // default value for MaxItemNum
   #define UTIL_RECYCLEBIN_DFT_MAXITEMNUM    ( 100 )
   // default value for MaxVersionNum
   #define UTIL_RECYCLEBIN_DFT_MAXVERNUM     ( 2 )

   /*
      _utilRecycleBinConf define
    */
   // configuration options of recycle bin
   // stored in SYSINFO.SYSDCBASE
   class _utilRecycleBinConf : public SDBObject
   {
   public:
      _utilRecycleBinConf() ;
      ~_utilRecycleBinConf() ;

      void reset() ;

      OSS_INLINE void setEnable( BOOLEAN enable )
      {
         _enable = enable ;
      }

      OSS_INLINE void setExpireTime( INT32 expireTime )
      {
         _expireTime = expireTime ;
      }

      OSS_INLINE void setMaxItemNum( INT32 maxItemNum )
      {
         _maxItemNum = maxItemNum ;
      }

      OSS_INLINE void setMaxVersionNum( INT32 maxVersionNum )
      {
         _maxVersionNum = maxVersionNum ;
      }

      OSS_INLINE void setAutoDrop( BOOLEAN autoDrop )
      {
         _autoDrop = autoDrop ;
      }

      OSS_INLINE BOOLEAN getEnable() const
      {
         return _enable ;
      }

      OSS_INLINE INT32 getExpireTime() const
      {
         return _expireTime ;
      }

      OSS_INLINE UINT64 getExpireTimeInMS() const
      {
         return (UINT64)_expireTime * 60 * OSS_ONE_SEC ;
      }

      OSS_INLINE INT32 getMaxItemNum() const
      {
         return _maxItemNum ;
      }

      OSS_INLINE INT32 getMaxVersionNum() const
      {
         return _maxVersionNum ;
      }

      OSS_INLINE BOOLEAN getAutoDrop() const
      {
         return _autoDrop ;
      }

      OSS_INLINE BOOLEAN isCapacityUnlimited() const
      {
         return _maxItemNum < 0 ;
      }

      OSS_INLINE BOOLEAN isVersionUnlimited() const
      {
         return _maxVersionNum < 0 ;
      }

      OSS_INLINE BOOLEAN isTimeUnlimited() const
      {
         return _expireTime < 0 ;
      }

      OSS_INLINE BOOLEAN isEnabled() const
      {
         return ( _enable ) &&
                ( 0 != _expireTime ) &&
                ( 0 != _maxItemNum ) &&
                ( 0 != _maxVersionNum ) ;
      }

      // from BSON object from "RecycleBin" field
      // e.g. { RecycleBin : { Enable : .... } }
      INT32 fromBSON( const bson::BSONObj &object ) ;
      // build BSON object with or without "RecycleBin" field
      // NOTE: if `needRecycleBinField` is TRUE, object will be built with
      //       "RecycleBin" field, e.g. { RecycleBin : { Enable : .... } }
      //       if `needRecycleBinField` is TRUE, object will be built without
      //       "RecycleBin" field, e.g. { Enable : .... }
      INT32 toBSON( bson::BSONObj &object, BOOLEAN needRecycleBinField ) const ;

      // update options
      // NOTE: the options should without "RecycleBin" field
      //       e.g. { Enable : .... }
      INT32 updateOptions( const bson::BSONObj &object ) ;

      _utilRecycleBinConf &operator =( const _utilRecycleBinConf &conf ) ;

   protected:
      INT32 _toBSON( bson::BSONObjBuilder &builder ) const ;

   protected:
      // indicates whether recycle bin is enable
      BOOLEAN  _enable ;
      // expire time of recycle items, in minutes
      INT32    _expireTime ;
      // maximum number of recycle items
      INT32    _maxItemNum ;
      // maximum version number of recycle items with the same
      // name or unique ID
      INT32    _maxVersionNum ;
      // indicates whether to automatically drop oldest items
      // when recycle bin is full
      BOOLEAN  _autoDrop ;
   } ;

   typedef class _utilRecycleBinConf utilRecycleBinConf ;

}

#endif // UTIL_RECYCLEBIN_CONF_HPP__
