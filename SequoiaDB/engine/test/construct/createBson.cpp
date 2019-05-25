/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

*******************************************************************************/

#include "ossTypes.hpp"
#include <map>
#include "createBson.hpp"
#include "utilBsongen.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "ossLatch.hpp"
#include "mthModifier.hpp"
using namespace std;

const UINT32 HASH_SIZE = 100;
map<string, UINT32> sequences[HASH_SIZE];
_ossSpinSLatch locks[HASH_SIZE];

#define OBJKEY "$set"
#define OBJRULE "$inc"
#define OBJKEYK "sdbtest"
#define OBJKEYI "sdbtestindex"
#define OBJKEYU "sdbtestupdate"

const UINT32 MAX_FIELD_NUM = 10;
const UINT32 MAX_FIELD_NAME_LEN = 10;
const UINT32 MAX_STR_LEN = 10;
const UINT32 MAX_DEPTH = 5;

#define CREATE_BSON(obj) \
        genRandomRecord(MAX_FIELD_NUM, MAX_FIELD_NAME_LEN, MAX_STR_LEN, MAX_DEPTH, obj, TRUE )

INT32 getObjData(const CHAR *key, CHAR *&str)
{
   engine::_mthModifier modifier;
   UINT32 hash = ossHash(key);
   hash %= HASH_SIZE;
   UINT32 id = 0;
   locks[hash].get();
   id = ++((sequences[hash])[string(key)]);
   locks[hash].release();
   BSONObjBuilder ob;
   BSONObj obj ;
   BSONObjBuilder ob1 ;
   BSONObjBuilder ob2 ;
   BSONObjBuilder ob3 ;

   ob1.append ( OBJKEYK, id ) ;
   ob2.append ( OBJKEYI, id ) ;
   ob3.append ( OBJKEYU, id ) ;
   ob.append(OBJKEY, ob1.obj());
   ob.append(OBJKEY, ob2.obj());
   ob.append(OBJKEY, ob3.obj());
   obj = ob.obj();
   if (SDB_OK != modifier.loadPattern(obj))
   {
      return -1;
   }

   BSONObj obj2;
   BSONObj obj3;
   CREATE_BSON(obj2);
   modifier.modify(obj2, obj3);
   str = (CHAR *)SDB_OSS_MALLOC(obj3.objsize());
   ossMemcpy(str, obj3.objdata(), obj3.objsize());
   return 0;
}

void getObjKey(CHAR *str)
{
   BSONObjBuilder ob;
   BSONObj obj ;
   ob.append(OBJKEYK, 1);
   obj = ob.obj();
   ossMemcpy(str, obj.objdata(), obj.objsize());
   return;
}

void getObjIndexKey(CHAR *str)
{
   BSONObjBuilder ob;
   BSONObj obj ;
   ob.append(OBJKEYI, 1);
   obj = ob.obj();
   ossMemcpy(str, obj.objdata(), obj.objsize());
   return;
}

void getObjRule(CHAR *str)
{
   BSONObjBuilder ob1;
   BSONObjBuilder ob2;
   BSONObj obj;
   ob1.append(OBJKEYU, 1);
   ob2.append(OBJRULE, ob1.obj());
   obj = ob2.obj();
   ossMemcpy(str, obj.objdata(), obj.objsize());
   return ;
}

void getObjDelKey(CHAR *str)
{
   static UINT32 i = 1;
   BSONObjBuilder ob;
   BSONObj obj ;
   ob.append(OBJKEYK, i++);
   obj = ob.obj();
   ossMemcpy(str, obj.objdata(), obj.objsize());
   return;
}

void getObjIndexDelKey(CHAR *str)
{
   static UINT32 i = 1;
   BSONObjBuilder ob;
   BSONObj obj ;
   ob.append(OBJKEYI, i++);
   obj = ob.obj();
   ossMemcpy(str, obj.objdata(), obj.objsize());
   return;
}
