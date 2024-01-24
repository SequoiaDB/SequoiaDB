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

   Source File Name = sequoiaFSDataCache.cpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2020  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSDataCache.hpp"
#include "ossUtil.h"

using namespace engine;
using namespace sequoiafs;

void dataCache::init(INT32 flId, INT64 offset)
{
   _flId = flId;
   _offset = offset;
   _dirty = FALSE;
   _expired = FALSE;
   _refCount = 0;
   _downloadCount = 0;
   _tryTime = 0;
   _time = ossGetCurrentMilliseconds();
   _bucketNext = NULL;
   _bucketPre = NULL;
   _fileNext = NULL;
   _filePre = NULL;
   ossMemset(&(_validScope), 0, sizeof(cachePiece)*SCOPE_SIZE);
   _scopeLen = 0;
   _rwMutex= SDB_OSS_NEW _ossRWMutex();
}

INT32 dataCache::cacheWrite(const CHAR *buf, INT64 size, INT64 offset)
{
   INT32 rc = SDB_OK;
   
   SDB_ASSERT(offset >= _offset, "offset >= _offset");
   SDB_ASSERT(offset+size <=  _offset + FS_CACHE_LEN, 
             "offset+size <=  _offset + FS_CACHE_LEN");

   ossMemcpy(data + offset - _offset, buf, size);
   _dirty = true;
   
   rc = addPiece(size, offset);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to addPiece, size:%ld, offset:%ld. rc=%d", size, offset, rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 dataCache::cacheRead(CHAR *buf, INT64 size, INT64 offset, INT32 *len)
{
   INT32 rc = SDB_OK;
   INT64 begin = 0;
   INT64 end = 0;
   BOOLEAN isFindPiece = FALSE;
   
   _isReaded = TRUE;
   
   *len = 0;
   for(INT32 i = 0; i < _scopeLen; i++)
   {
      cachePiece p = _validScope[i];
      if(p.contain(offset))
      {
         begin = offset;
         end = min64(offset+size-1, p.last);
         *len = (UINT32(end - begin + 1));
         ossMemcpy(buf, data + begin - _offset, 
                   end - begin + 1);
         isFindPiece = TRUE;
         goto done;
      }
      
   }

   if(!isFindPiece)
   {
      rc = SDB_DMS_RECORD_NOTEXIST;
   }

done: 
   return rc;
}

INT32 dataCache::cacheDownLoad(fsConnectionDao *db,
                               const CHAR *clFullName, const OID &oid)
{
   INT32 rc = SDB_OK;
   INT32 readLen = 0;
   INT64 newLobSize = 0;
 
   if(_dirty)
   {
      for(INT32 i = 0; i < _scopeLen; i++)
      { 
         cachePiece p = _validScope[i];
         rc = db->writeLob(data + p.first - _offset, 
                  clFullName, 
                  oid, 
                  p.first, 
                  (UINT32)(p.last - p.first + 1),
                  &newLobSize);
         if(rc != SDB_OK)
         {
            goto error;
         }
      }
      clearPiece();
   }

   _downloadCount++;
   if(_downloadCount > 1)
   {
      PD_LOG(PDDEBUG, "downloadCount=%d, oid=%s, _offset=%d",
             _downloadCount, oid.toString().c_str(), _offset);
   }
   
   rc = db->readLob(clFullName, oid, _offset, FS_CACHE_LEN, data, &readLen);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to readLob, rc=%d", rc);
      goto error;
   }
   addPiece(readLen, _offset);
   _expired = FALSE;
   _isReaded = FALSE;

done:
   return rc;
   
error:
   goto done;    
}

INT32 dataCache::addPiece(INT64 size, INT64 offset)
{
   INT32 rc = SDB_OK;
   INT64 first = offset;
   INT64 last = offset+size-1;
   INT32 index = 0;
   INT32 overIndex = 0;
   BOOLEAN match = false;
   BOOLEAN over  = false;

   for (; index < _scopeLen; index++)
   {
      //example: first=3, _validScope:[{5,6}, {9,10}]
      if(first <= _validScope[index].first)
      {
         //example: first=3, last=4  _validScope:[{5,6}, {9,10}]
         if(last >= _validScope[index].first-1)
         {
            match = true;
            _validScope[index].first = first;
            //example: first=3, last=7  _validScope:[{5,6}, {9,10}]
            if(last > _validScope[index].last)
            {
               over = true;
               _validScope[index].last = last;
            }
            break;
         }
         //example: first=3, last=3  _validScope:[{5,6}, {9,10}]
         break;
      }
      //example: first=7  _validScope:[{5,6}, {9,10}]
      else if(first <= _validScope[index].last+1)
      {
         match = true;
         //example: first=7, last=8, _validScope:[{5,6}, {9,10}]
         if(last > _validScope[index].last)
         {
            over = true;
            _validScope[index].last = last;
         }
         break;
      }
   }
   
   if(!match)
   {
      if(SCOPE_SIZE == _scopeLen)
      {
         PD_LOG(PDERROR, "Too many piece in the file.");
         rc = SDB_NOSPC;
         goto error;
      }
      if(index == _scopeLen)
      {
         _validScope[index] = cachePiece(first, last);
      }
      else
      {
         insertPiece(index, cachePiece(first, last));
      }
      _scopeLen++;
   }
   if(over && index < _scopeLen - 1)
   {
      for(INT32 i = index; i < _scopeLen - 1; i++)
      {
         if(_validScope[index].last >= _validScope[i+1].last)
         {
            overIndex = i + 1;
            continue;
         }
         else if(_validScope[index].last >= _validScope[i+1].first - 1)
         {
            overIndex = i + 1;
            _validScope[index].last = _validScope[i+1].last;
            break;
         }
         else
         {
            break;
         }
      }

      if(overIndex > index)
      {
         erasePiece(index+1, overIndex);
         _scopeLen = _scopeLen - (overIndex - index);
      }
   }

done:
   return rc;
   
error:
   goto done;
}

void dataCache::clearPiece()
{
   _dirty = FALSE;

   ossMemset(&(_validScope), 0, sizeof(cachePiece)*SCOPE_SIZE);
   _scopeLen = 0;
}

BOOLEAN dataCache::matchPiece(INT64 size, INT64 offset)
{
   BOOLEAN find = false;
   cachePiece nullPiece(0,0);

   if(_expired)
   {
      goto error;
   }
   
   for(INT32 i = 0; i < _scopeLen; i++)
   {
      if(nullPiece == _validScope[i])
      {
         break;
      }
      if(_validScope[i].contains(size, offset))
      {
         find = true;
         break;
      }
   }

done:
   return find; 

error:
   goto done;
}

BOOLEAN dataCache::canFlush()
{
   BOOLEAN canFlush = false;

   if(0 == _refCount)
   {
      if (_scopeLen > SCOPE_SIZE / 8) 
      {
         canFlush = true;
      }
      else if(isFull()) 
      {
         canFlush = true;
      }
      else
      {
         UINT64 nowTime = ossGetCurrentMilliseconds();
         if(nowTime - _time > 3000)
         {
            canFlush = true;
         }
      }
   }
   
   return canFlush;
}

BOOLEAN dataCache::isFull()
{
   BOOLEAN isFull = FALSE;
   if(1 == _scopeLen
      && _validScope[0].first == _offset
      && _validScope[0].last == _offset + FS_CACHE_LEN - 1)
   {
      isFull = TRUE;
   }
   return isFull;
}

void dataCache::insertPiece(INT32 index, cachePiece piece)
{
   ossMemmove(&_validScope[index+1], &_validScope[index], (_scopeLen - index) * sizeof(cachePiece));
   _validScope[index] = piece;
}

void dataCache::erasePiece(INT32 start, INT32 last)
{
   ossMemmove(&_validScope[start], &_validScope[last+1], _scopeLen - 1 - last);
}

INT32 dataCache::lockW()
{
   return _rwMutex->lock_w();
}

BOOLEAN dataCache::tryLockW()
{
   return _rwMutex->try_lock_w();
}
   
INT32 dataCache::unLockW()
{
   return _rwMutex->release_w();
}

INT32 dataCache::lockR()
{
   return _rwMutex->lock_r();
}

INT32 dataCache::unLockR()
{
   return _rwMutex->release_r();
}

