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

#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_

#include <map>

#include <boost/thread.hpp>

#include "core.hpp"
#include "ossLatch.hpp"
#include "job.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"

using namespace std;

struct unitInfo
{
   unitInfo()
   {
      _insert = 0;
      _insertSuc = 0;
      _insertT = 0;
      _update = 0;
      _updateSuc = 0;
      _updateT = 0;
      _query = 0;
      _querySuc = 0;
      _queryT = 0;
      _drop = 0;
      _dropSuc = 0;
      _dropT = 0;
   }

   void collect(const unitInfo &info)
   {
      _insert += info._insert;
      _insertSuc += info._insertSuc;
      _insertT += info._insertT;
      _update += info._update;
      _updateSuc += info._updateSuc;
      _updateT += info._updateT;
      _query += info._query;
      _querySuc += info._querySuc;
      _queryT += info._queryT;
      _drop += info._drop;
      _dropSuc += info._dropSuc;
      _dropT += info._dropT;
   }

   void clear()
   {
      _insert = 0;
      _insertSuc = 0;
      _insertT = 0;
      _update = 0;
      _updateSuc = 0;
      _updateT = 0;
      _query = 0;
      _querySuc = 0;
      _queryT = 0;
      _drop = 0;
      _dropSuc = 0;
      _dropT = 0;
   }

   UINT64 _insert;
   UINT64 _insertSuc;
   UINT64 _insertT;
   UINT64 _update;
   UINT64 _updateSuc;
   UINT64 _updateT;
   UINT64 _query;
   UINT64 _querySuc;
   UINT64 _queryT;
   UINT64 _drop;
   UINT64 _dropSuc;
   UINT64 _dropT;
};

#define STA_TIMER(code, sta, type)\
        {\
           ossTimestamp tstamp;\
           ossGetCurrentTime(tstamp);\
           {code;}\
           sta.time(tstamp, type);\
        }

class statistics
{
   public:
      statistics();
      ~statistics();

   public:
      static void active(statistics *s);

   public:
      INT32 init();

      void time(const ossTimestamp &begin, const JOB_TYPE &type);

      void incRecord(const UINT32 &total, const UINT32 &suc,
                     const JOB_TYPE &type);
      void run();

   private:
      void _updateLog(const string &id, const unitInfo &info,
                      string &log, BOOLEAN isTotal=FALSE);

      void _updateLogs();

      void _log2File();

   private:
      map<boost::thread::id, unitInfo> _details;
      map<boost::thread::id, string> _detailsBuf;
      unitInfo _collect;
      string _collectLog;
      boost::thread *_thread;
      _ossSpinSLatch _mtx;
      OSSFILE _file;
      UINT64 _updatePoint;
      UINT64 _logPoint;
      volatile UINT16 _quitFlag;
};


#endif


