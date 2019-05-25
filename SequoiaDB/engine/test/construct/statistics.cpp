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
#include <boost/lexical_cast.hpp>

#include "statistics.hpp"
using namespace std;
#define JUDGE(rc) if (SDB_OK != rc){goto error;}

const UINT16 RUN = 0;
const UINT16 QUIT = 1;
const string TOTAL_INFO = "total info";
statistics::statistics():_thread(NULL),
                         _updatePoint(0),
                         _logPoint(0),
                         _quitFlag(QUIT)
{
}

statistics::~statistics()
{
   _quitFlag = QUIT;
   if (NULL != _thread)
   {
      _thread->join();
      delete _thread;
   }
   _details.clear();
   _detailsBuf.clear();
   ossClose(_file);
}

void statistics::active(statistics *s)
{
   s->run();
   return ;
}

INT32 statistics::init()
{
   INT32 rc = SDB_OK;

   rc = ossOpen( "./statistics.log", OSS_REPLACE | OSS_READWRITE
                 , OSS_RU | OSS_WU | OSS_RG, _file );
   JUDGE(rc)

   _quitFlag = RUN;
   _thread = new boost::thread(active, this);

done:
   return rc;
error:
   goto done;
}

void statistics::time(const ossTimestamp &begin, const JOB_TYPE &type)
{
   if (JOB_TYPE_QUIT == type)
   {
      return;
   }
   ossTimestamp tstamp;
   ossGetCurrentTime(tstamp);
   UINT64 bMicrS = begin.time * 1000000 + begin.microtm;
   UINT64 eMicrS = tstamp.time * 1000000 + tstamp.microtm;
   if ( eMicrS <= bMicrS )
   {
      return;
   }
   boost::thread::id tid= boost::this_thread::get_id();
   _mtx.get();
   if (JOB_TYPE_INSERT == type)
   {
      _details[tid]._insertT += eMicrS - bMicrS;
   }
   else if (JOB_TYPE_UPDATE == type)
   {
       _details[tid]._updateT += eMicrS - bMicrS;
   }
   else if (JOB_TYPE_QUERY == type)
   {
      _details[tid]._queryT += eMicrS - bMicrS;
   }
   else
   {
      _details[tid]._dropT += eMicrS - bMicrS;
   }
   ++_updatePoint;
   _mtx.release();

   return;
}

void statistics::incRecord(const UINT32 &total, const UINT32 &suc,
                           const JOB_TYPE &type)
{
   if (JOB_TYPE_QUIT == type)
   {
      return;
   }

   boost::thread::id tid= boost::this_thread::get_id();
   _mtx.get();
   unitInfo &info = _details[tid];
   if (JOB_TYPE_INSERT == type)
   {
      info._insert += total;
      info._insertSuc += suc;
   }
   else if (JOB_TYPE_UPDATE == type)
   {
       info._update += total;
       info._updateSuc += suc;
   }
   else if (JOB_TYPE_QUERY == type)
   {
      info._query += total;
      info._querySuc += suc;
   }
   else
   {
      info._drop += total;
      info._dropSuc += suc;
   }
   ++_updatePoint;
   _mtx.release();

   return;
}

void statistics::run()
{
   while (RUN == _quitFlag)
   {
      ossSleepsecs(30);
      _mtx.get();
      if (_updatePoint == _logPoint)
      {
         _mtx.release();
         continue;
      }
      else
      {
         _updateLogs();
         _logPoint = _updatePoint;
      }
      _mtx.release();
      _log2File();
   }
   _mtx.get();
   if (_updatePoint == _logPoint)
   {
      _mtx.release();
      return;
   }
   else
   {
      _updateLogs();
      _logPoint = _updatePoint;
   }
   _mtx.release();
   _log2File();
   return;
}

void statistics::_log2File()
{
   SINT64 len = 0;
   map<boost::thread::id, string>::const_iterator itr =
                  _detailsBuf.begin();
   for (; itr!=_detailsBuf.end(); itr++)
   {
      ossWrite(&_file, itr->second.c_str(), itr->second.size(), &len);
   }
   ossWrite(&_file, _collectLog.c_str(), _collectLog.size(), &len);
   return;
}

void statistics::_updateLogs()
{
   _collect.clear();
   map<boost::thread::id, unitInfo>::const_iterator itr =
                            _details.begin();
   for (; itr!=_details.end(); itr++)
   {
      _updateLog(boost::lexical_cast<string>(itr->first),
                 itr->second,
                 _detailsBuf[itr->first]);
      _collect.collect(itr->second);
   }

   _updateLog(TOTAL_INFO, _collect, _collectLog, TRUE);
   return;
}

void statistics::_updateLog(const string &id, const unitInfo &info,
                            string &log, BOOLEAN isTotal)
{
   log.clear();
   log.append("id: ");
   log.append(id);
   log.append( "\n");
   log.append("insert: ");
   log.append(boost::lexical_cast<string>(info._insert));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._insertSuc));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._insertT));
   log.append("\n");
   log.append("update: ");
   log.append(boost::lexical_cast<string>(info._update));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._updateSuc));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._updateT));
   log.append("\n");
   log.append("query: ");
   log.append(boost::lexical_cast<string>(info._query));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._querySuc));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._queryT));
   log.append("\n");
   log.append("drop: ");
   log.append(boost::lexical_cast<string>(info._drop));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._dropSuc));
   log.append("  ");
   log.append(boost::lexical_cast<string>(info._dropT));
   log.append("\n\n");
   if (isTotal)
      log.append("======================================\n");
}

