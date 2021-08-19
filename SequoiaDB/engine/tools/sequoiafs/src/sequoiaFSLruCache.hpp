/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = sequoiaFSLruCache.cpp

   Descriptive Name = sequoiafs fuse file lru cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
       03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSLRUCACHE_HPP__
#define __SEQUOIAFSLRUCACHE_HPP__

#include "sequoiaFS.hpp"
#include <iostream>
#include <map>

namespace sequoiafs
{
   class Node 
   {
   public:
      Node(int k, const struct dirMetaNode* v): key(k), prev(NULL), next(NULL)
      {
         node.name= v->name;
         node.mode= v->mode;
         node.uid = v->uid;
         node.gid = v->gid;
         node.pid= v->pid;
         node.nLink = v->nLink;
         node.id= v->id;
         node.size= v->size;
         node.atime= v->atime;
         node.ctime= v->ctime;
         node.mtime= v->mtime;
         node.symLink= v->symLink;
      }
   public:
      int key;
      struct dirMetaNode node;
      Node *prev, *next;
   };


   class DoublyLinkedList : public SDBObject
   {
      public:
         DoublyLinkedList(): front(NULL), rear(NULL){}
         Node* getRearDir()
         {
            return rear;
         }
         void removeRearDir();
         void moveDirToHead(Node *Dir);
         Node* addDirToHead(int key,const struct dirMetaNode* value);
         bool isEmpty()
         {
            return rear == NULL;
         }
      private:
         Node *front, *rear;
   };

   class LRUCache : public SDBObject
   {
      public:
         LRUCache(int capacity)
         {
            this->capacity = capacity;
            size = 0;
            dirList = new DoublyLinkedList();
            DirMap = map<int, Node*>();
         }

         ~LRUCache()
         {
            map<int, Node*>::iterator i1;
            for(i1=DirMap.begin();i1!=DirMap.end();i1++)
            {
               delete i1->second;
            }
            delete dirList;
         }
         int initMutex();
         void put(int key,const struct dirMetaNode * value);
         struct dirMetaNode * get(int key) ;

      private:
         int capacity, size;
         DoublyLinkedList *dirList;
         map<int, Node*> DirMap;

         pthread_mutex_t mutex;
         pthread_mutexattr_t attr;
   };

}

#endif
