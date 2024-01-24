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

   Source File Name = sequoiaFSLRUList.cpp

   Descriptive Name = sequoiafs fuse file lru cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj   Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSLRULIST_HPP__
#define __SEQUOIAFSLRULIST_HPP__

#include "sequoiaFSCommon.hpp"

#define  REDUCE_LEVEL 2
#define  HOT_LEVEL 5

namespace sequoiafs
{
   class dirMetaNode 
   {
      public:
         dirMetaNode(int k, dirMeta* v)
            : key(k), 
              isHot(FALSE),
              count(0), 
              linkPre(NULL), 
              linkNext(NULL), 
              bucketPre(NULL), 
              bucketNext(NULL)
         {
            meta.setName(v->name());
            meta.setMode(v->mode());
            meta.setUid(v->uid());
            meta.setGid(v->gid());
            meta.setPid(v->pid());
            meta.setNLink(v->nLink());
            meta.setId(v->id());
            meta.setSize(v->size());
            meta.setAtime(v->atime());
            meta.setCtime(v->ctime());
            meta.setMtime(v->mtime());
            meta.setSymLink(v->symLink());
         }

         INT32 getKey(){return key;}
         
      public:
         INT32 key;
         bool isHot;
         INT32 count;
         dirMeta meta;
         dirMetaNode *linkPre;
         dirMetaNode *linkNext;
         dirMetaNode *bucketPre;
         dirMetaNode *bucketNext;
   };

   class LRUList : public SDBObject
   {
      public:
         LRUList();
         ~LRUList();
         void init(INT32 capa){_capacity = capa;}
         INT32 addCold(dirMetaNode* node);
         dirMetaNode* findReduceCold();
         INT32 del(dirMetaNode* node);

         INT32 getColdSize(){return _coldSize;}
         INT32 getHotSize(){return _hotSize;}
         
      private:   
         INT32 _coldToHot(dirMetaNode* node);
         INT32 _coldToCold(dirMetaNode* node);
         void _reduceHot();

      private:
         INT32 _capacity;
         INT32 _hotSize;
         INT32 _coldSize;
         dirMetaNode* _hotHead;
         dirMetaNode* _hotTail;
         dirMetaNode* _coldHead;
         dirMetaNode* _coldTail;
         ossSpinXLatch _mutex;  

         friend class dirMetaCache;
   };

}

#endif
