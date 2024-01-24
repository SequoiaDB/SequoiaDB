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

   Source File Name = sequoiaFSDataCache.hpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSDATACACHE_HPP__
#define __SEQUOIAFSDATACACHE_HPP__

#include "ossTypes.hpp"
#include "utilStr.hpp"
#include "sequoiaFSDao.hpp"

#include "ossRWMutex.hpp"

//#define FS_CACHE_LEN 8388608     
//#define FS_CACHE_OFFSETBIT 23    //8M占位23位
#define FS_CACHE_LEN 4194304     
#define FS_CACHE_OFFSETBIT 22    //4M占位22位
//#define FS_CACHE_LEN 1048576
//#define FS_CACHE_OFFSETBIT 20    //1M占位20位
//#define FS_CACHE_LEN 524288
//#define FS_CACHE_OFFSETBIT 19    //512K占位19位
//#define FS_CACHE_LEN 262144
//#define FS_CACHE_OFFSETBIT 18    //256K占位18位
//#define FS_CACHE_LEN 131072
//#define FS_CACHE_OFFSETBIT 17    //128K占位17位

#define SCOPE_SIZE  32 

using namespace engine;

namespace sequoiafs
{
   struct cachePiece
   {
      INT64 first ;
      INT64 last ;

      cachePiece( INT64 start, INT64 end )
         : first( start ),
           last( end )
      {
      }

      cachePiece()
         : first( 0 ),
           last( 0 )
      {
      }

      OSS_INLINE bool contain( INT64 offset ) const
      {
         return ( offset >= first && offset <= last ) ;
      }

      OSS_INLINE bool contains( INT64 size, INT64 offset ) const
      {
         return ( offset >= first && offset + size - 1 <= last ) ;
      }

      void operator=( const cachePiece& pieces )
      {
         first = pieces.first ;
         last = pieces.last ;
      }

      BOOLEAN operator==( const cachePiece& pieces )
      {
         if(first != pieces.first)
         {
            return false;
         }
         if(last != pieces.last)
         {
            return false;
         }

         return true;
      }
   } ;

   struct dataCache
   {
      INT32 _flId;
      INT64 _offset;
      UINT64 _time;
      BOOLEAN _dirty;
      BOOLEAN _expired;
      BOOLEAN _isReaded;  //是否曾有请求读过这个块
      UINT32  _tryTime;
      UINT32  _refCount;
      UINT32  _downloadCount;
      ossRWMutex* _rwMutex;
      dataCache*  _bucketPre;
      dataCache*  _bucketNext;
      dataCache*  _filePre;
      dataCache*  _fileNext;
      cachePiece _validScope[SCOPE_SIZE];
      INT32     _scopeLen;
      CHAR       data[0]; 

      void init(INT32 flId, INT64 offset);
      INT32 cacheWrite(const CHAR *buf, INT64 size, INT64 offset);
      INT32 cacheRead(CHAR *buf, INT64 size, INT64 offset, INT32 *len);
      INT32 cacheDownLoad(fsConnectionDao *db, const CHAR *clFullName, const OID &oid);
      INT32 addPiece(INT64 size, INT64 offset); 
      void clearPiece();
      BOOLEAN matchPiece(INT64 size, INT64 offset);
      BOOLEAN canFlush();
      BOOLEAN isFull();
      void setExpired(){_expired = TRUE;}
      void insertPiece(INT32 index, cachePiece piece);
      void erasePiece(INT32 start, INT32 end);
      INT32 lockW();
      BOOLEAN tryLockW();
      INT32 unLockW();
      INT32 lockR();
      INT32 unLockR();
   };
}
#endif
