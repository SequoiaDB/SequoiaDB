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

#include "sequoiaFSLRUList.hpp"

using namespace sequoiafs;

LRUList::LRUList()
:_hotSize(0),
 _coldSize(0),
 _hotHead(NULL),
 _hotTail(NULL),
 _coldHead(NULL),
 _coldTail(NULL)
{
}

LRUList::~LRUList()
{
}

INT32 LRUList::addCold(dirMetaNode* node)
{
   INT32 rc = SDB_OK;

   if(NULL == node)
   {
      goto error;
   }

   if(NULL == _coldHead)
   {
      _coldHead = node;
      _coldTail = node;
   }
   else
   {
      node->linkPre = _coldHead->linkPre;
      if(_coldHead->linkPre != NULL)
      {
         _coldHead->linkPre->linkNext = node;
      }
      node->linkNext = _coldHead;
      _coldHead->linkPre = node;
      _coldHead = node;
   }
   
   node->count= 1;
   ++_coldSize;  //TODO:xx++ 改成++xx

done:
   return rc;
   
error:
   goto done;
}

dirMetaNode* LRUList::findReduceCold()
{
   dirMetaNode* cur = NULL;
   dirMetaNode* del = NULL;
   
   if(NULL == _coldTail)
   {
      goto done;
   }

   if(_coldSize < _capacity/2)  //TODO:sizeof   dirMetaNode计算一下大小
   {
      goto done;
   }

   cur = _coldTail;

   while(_coldSize >= _capacity/2 && cur != _coldHead && cur != NULL)
   {
      dirMetaNode* pre = cur->linkPre;
      if(cur->count > HOT_LEVEL)
      {
         _coldToHot(cur);
      }
      else if(cur->count > REDUCE_LEVEL)
      {
         _coldToCold(cur);
      }
      else
      {
         del = cur;
         break;
      }
      cur = pre;
      _coldTail = pre;
   }

done:
   return del;
}

INT32 LRUList::del(dirMetaNode* node)
{
   INT32 rc = SDB_OK;
   
   if(NULL == node)
   {
      goto done;
   }

   if(node->linkNext!= NULL)
   {
      node->linkNext->linkPre = node->linkPre;
   }
   if(node->linkPre != NULL)
   {
      node->linkPre->linkNext = node->linkNext;
   }

   if(node->isHot)
   {
      --_hotSize;
      if(node == _hotHead)
      {
         _hotHead = node->linkNext;
      }
      if(node == _hotTail)
      {
         _hotTail = node->linkPre;
      }
   }
   else
   {
      --_coldSize;
      if(node == _coldHead)
      {
         _coldHead = node->linkNext;
      }
      if(node == _coldTail)
      {
         _coldTail = node->linkPre;
      }
   }
   
done:
   return rc;
}

INT32 LRUList::_coldToHot(dirMetaNode* node)
{
   INT32 rc = SDB_OK;
   
   if(NULL == node)
   {
      goto done;
   }

   //del cold
   if(node->linkNext!= NULL)
   {
      node->linkNext->linkPre = node->linkPre;
   }
   if(node->linkPre != NULL)
   {
      node->linkPre->linkNext = node->linkNext;
   }

   //add hot
   if(_hotHead != NULL)
   {
      node->linkNext = _hotHead;
      _hotHead->linkPre = node;
   }
   else
   {
      node->linkNext = _coldHead;
      _coldHead->linkPre = node;
      _hotTail = node;
   }
   node->linkPre = NULL;

   _hotHead = node;
   _hotHead->isHot = TRUE;
   _hotHead->count = 0;

   --_coldSize;
   ++_hotSize;

   if(_hotSize >= _capacity/2)
   {
      _reduceHot();
   }

done:
   return rc;
}

void LRUList::_reduceHot()
{
   _hotTail->isHot = FALSE;
   _coldHead = _hotTail;
   _hotTail = _hotTail->linkPre;
   _coldHead->count = 0;

   --_hotSize;
   ++_coldSize;
}

INT32 LRUList::_coldToCold(dirMetaNode* node)
{
   INT32 rc = SDB_OK;
   
   if(NULL == node)
   {
      goto done;
   }
   
   if(node->linkNext!= NULL)
   {
      node->linkNext->linkPre = node->linkPre;
   }
   if(node->linkPre != NULL)
   {
      node->linkPre->linkNext = node->linkNext;
   }

   node->linkNext = _coldHead;
   node->linkPre = _coldHead->linkPre;

   _coldHead = node;
   _coldHead->count = 0;

done:
   return rc;
}
