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

#ifndef CASERUNNER_HPP_
#define CASERUNNER_HPP_

#include <vector>
#include <boost/thread.hpp>

#include "core.hpp"
#include "plan.hpp"
#include "job.hpp"
#include "statistics.hpp"
#include "ossLatch.hpp"

using namespace std;

namespace sdbclient
{
   class sdb;
}

class caseRunner
{
   public:
      caseRunner();
      ~caseRunner();

   public:
      static void active(caseRunner *runner);

   public:
      INT32 run(executionPlan &plan);

      void join();

   private:
      inline UINT64 _range()
      {
         return _plan._insert + _plan._update +
                _plan._delete + _plan._query;
      }

      inline JOB_TYPE _type(UINT64 rand)
      {
         if (rand < _plan._insert)
         {
            --_plan._insert;
            return JOB_TYPE_INSERT;
         }
         else if (rand < (_plan._insert + _plan._delete))
         {
            --_plan._delete;
            return JOB_TYPE_DELETE;
         }
         else if (rand <
                  (_plan._insert + _plan._delete + _plan._update))
         {
            --_plan._update;
            return JOB_TYPE_UPDATE;
         }
         else if (rand <
                  (_plan._insert + _plan._delete +
                   _plan._update + _plan._query))
         {
            --_plan._query;
            return JOB_TYPE_QUERY;
         }
         else
         {
            return JOB_TYPE_QUIT;
         }
      }

   private:
      INT32 _init(executionPlan &plan);

      void _crun();

      void _getJob(job &j);

      INT32 _insert(const job &j, sdbclient::sdb &conn);

      INT32 _drop(const job &j, sdbclient::sdb &conn);

      INT32 _update(const job &j, sdbclient::sdb &conn);

      INT32 _query(const job &j, sdbclient::sdb &conn);

   private:
      vector<boost::thread *> _consumers;
      executionPlan _plan;
      _ossSpinXLatch _mtx;
      statistics _statistics;
};

#endif

