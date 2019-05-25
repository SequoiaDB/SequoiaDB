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
#include <iostream>
#include "runner.hpp"
#include "../../bson/bson.h"
#include "../../client/client.hpp"
#include "ossUtil.hpp"
#include "createBson.hpp"
#include "ossMem.hpp"
using namespace std;
using namespace bson ;

#define JUDGE(rc) if (SDB_OK!=rc){goto error;}

#define JUDGE_INFO(rc, type)\
        if (SDB_OK != rc){_statistics.incRecord(1, 0, type);}\
        else {_statistics.incRecord(1, 1, type);}

caseRunner::caseRunner()
{

}

caseRunner::~caseRunner()
{
   join();
   for (UINT32 i=0; i<_consumers.size(); i++)
   {
      delete _consumers.at(i);
   }
   _consumers.clear();
}

void caseRunner::active(caseRunner *runner)
{
   runner->_crun();
   return ;
}

INT32 caseRunner::run(executionPlan &plan)
{
   INT32 rc = SDB_OK;

   rc = _init(plan);
   JUDGE(rc)

done:
   return rc;
error:
   goto done;
}

void caseRunner::join()
{
   for (UINT32 i=0; i<_consumers.size(); i++)
   {
      _consumers.at(i)->join();
   }
   return;
}

INT32 caseRunner::_init(executionPlan &plan)
{
   INT32 rc = SDB_OK;
   _plan = plan;
   rc = _statistics.init();
   JUDGE(rc)

   for (UINT32 i=0; i<plan._thread; i++)
   {
      boost::thread *thread = new boost::thread(active, this);
      _consumers.push_back(thread);
   }

done:
   return rc;
error:
   goto done;

}

void caseRunner::_crun()
{
   sdbclient::sdb connection ;
   if (SDB_OK != connection.connect(_plan._host.c_str(), _plan._port))
   {
      std::cerr << "connect " << _plan._host << ":" <<
      _plan._port << " failed" << endl;
      return;
   }
   job j;
   INT32 rc = SDB_OK;
   while (TRUE)
   {
      _getJob(j);
      if (JOB_TYPE_QUIT == j._type)
      {
         break;
      }
      else if (JOB_TYPE_INSERT == j._type)
      {
        rc = _insert(j, connection);
        JUDGE_INFO(rc, JOB_TYPE_INSERT)
      }
      else if (JOB_TYPE_DELETE == j._type)
      {
         rc = _drop(j, connection);
         JUDGE_INFO(rc, JOB_TYPE_DELETE)
      }
      else if (JOB_TYPE_UPDATE == j._type)
      {
         rc = _update(j, connection);
         JUDGE_INFO(rc, JOB_TYPE_UPDATE)
      }
      else
      {
         rc = _query(j, connection);
         JUDGE_INFO(rc, JOB_TYPE_QUERY)
      }
   }

   return;
}

void caseRunner::_getJob(job &j)
{
   JOB_TYPE type = JOB_TYPE_QUIT;
   if (0 == _range())
   {
      j._type = type;
      return ;
   }

   UINT32 rand = ossRand();
   UINT32 rand2 = rand;
   _mtx.get();

   rand %= _range();
   type = _type(rand);

   _mtx.release();

   j._type = type;
   j._cs = _plan._cs + string("_") +
           boost::lexical_cast<string>(rand2%_plan._csNum);
   j._collection = _plan._collection + string("_") +
                  boost::lexical_cast<string>(rand2%_plan._collectionNum);
}


INT32 caseRunner::_insert(const job &j, sdbclient::sdb &conn)
{
   INT32 rc = SDB_OK;
   sdbclient::sdbCollectionSpace cs ;
   sdbclient::sdbCollection collection ;
   CHAR *data = NULL;
   CHAR index[50];
   BSONObj obj ;
   BSONObj obj2 ;
   rc = conn.getCollectionSpace ( j._cs.c_str(), cs ) ;
   if ( SDB_DMS_CS_NOTEXIST == rc)
   {
      rc = conn.createCollectionSpace(j._cs.c_str(), SDB_PAGESIZE_DEFAULT, cs);
      JUDGE(rc)
   }
   else if (SDB_OK != rc)
   {
      goto error;
   }
   else
   {
   }

   rc = cs.getCollection(j._collection.c_str(), collection);
   if (SDB_DMS_NOTEXIST == rc)
   {
      rc = cs.createCollection ( j._collection.c_str(), collection ) ;
      JUDGE(rc)
      getObjIndexKey(index);
      obj2.init ( index ) ;
      rc = collection.createIndex(obj2, "sdbtestindex", FALSE, FALSE);
      JUDGE(rc)
   }

   rc = getObjData((j._cs + j._collection).c_str(), data);
   JUDGE(rc)
   obj.init ( data ) ;
   JUDGE(rc)
   STA_TIMER(rc = collection.insert(obj),
             _statistics,
             JOB_TYPE_INSERT)
   JUDGE(rc)
done:
   if ( NULL != data )
      SDB_OSS_FREE(data);
   return rc;
error:
   goto done;
}

INT32 caseRunner::_drop(const job &j, sdbclient::sdb &conn)
{
   INT32 rc = SDB_OK;
   sdbclient::sdbCollectionSpace cs ;
   sdbclient::sdbCollection collection ;
   CHAR data[50] = {0};
   BSONObj obj ;
   UINT32 rand;
   rc = conn.getCollectionSpace ( j._cs.c_str(), cs ) ;
   if ( SDB_OK != rc)
   {
      goto error;
   }

   rc = cs.getCollection(j._collection.c_str(), collection);
   if (SDB_OK != rc)
   {
      goto error;
   }

   rand = ossRand();
   rand %= SCALE_BASE;
   if (rand < _plan._scale)
   {
      getObjIndexDelKey(data);
   }
   else
   {
      getObjDelKey(data);
   }

   obj.init ( data ) ;
   JUDGE(rc)
   STA_TIMER(rc = collection.del(obj),
             _statistics,
             JOB_TYPE_DELETE)
   JUDGE(rc)
done:
   return rc;
error:
   goto done;
}

INT32 caseRunner::_update(const job &j, sdbclient::sdb &conn)
{
   INT32 rc = SDB_OK;
   sdbclient::sdbCollectionSpace cs ;
   sdbclient::sdbCollection collection ;
   CHAR data[50] = {0};
   CHAR condi[50] = {0};
   BSONObj obj ;
   BSONObj obj2 ;
   UINT32 rand = 0;

   rc = conn.getCollectionSpace ( j._cs.c_str(), cs ) ;
   if ( SDB_OK != rc)
   {
      goto error;
   }

   rc = cs.getCollection(j._collection.c_str(), collection);
   if (SDB_OK != rc)
   {
      goto error;
   }

   rand = ossRand();
   rand %= SCALE_BASE;
   if (rand < _plan._scale)
   {
      getObjIndexKey(condi);
   }
   else
   {
      getObjKey(condi);
   }
   obj2.init ( condi ) ;
   getObjRule(data);
   obj.init ( data ) ;
   JUDGE(rc)
   STA_TIMER(rc = collection.update(obj, obj2),
             _statistics,
             JOB_TYPE_UPDATE)
   JUDGE(rc)

done:
   return rc;
error:
   goto done;
}

INT32 caseRunner::_query(const job &j, sdbclient::sdb &conn)
{
   INT32 rc = SDB_OK;
   sdbclient::sdbCollectionSpace cs ;
   sdbclient::sdbCollection collection ;
   sdbclient::sdbCursor cursor;
   CHAR data[50] = {0};
   BSONObj obj ;
   UINT32 rand = 0;
   rc = conn.getCollectionSpace ( j._cs.c_str(), cs ) ;
   if ( SDB_OK != rc)
   {
      goto error;
   }

   rc = cs.getCollection(j._collection.c_str(), collection);
   if (SDB_OK != rc)
   {
      goto error;
   }

   rand = ossRand();
   rand %= SCALE_BASE;
   if (rand < _plan._scale)
   {
      getObjIndexKey(data);
   }
   else
   {
      getObjKey(data);
   }
   obj.init ( data ) ;
   JUDGE(rc)
   STA_TIMER(rc = collection.query(cursor, obj),
             _statistics,
             JOB_TYPE_QUERY)
   JUDGE(rc)
done:
   return rc;
error:
   goto done;
}
