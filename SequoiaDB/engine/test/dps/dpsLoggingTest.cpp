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
#include <gtest/gtest.h>

#include "core.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "utilBsongen.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "dpsOp2Record.hpp"
#include "dpsLogRecordDef.hpp"

#include <stdio.h>
#include <string>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>

using namespace engine;
using namespace std;

const UINT32 MAX_FIELD_NUM = 20;
const UINT32 MAX_FIELD_NAME_LEN = 10;
const UINT32 MAX_STRING_LEN = 10;
const UINT32 MAX_DEPTH = 5;

#define CRE_OBJ(a) \
        cout << "create bsonobj begin" << endl;\
        genRandomRecord(MAX_FIELD_NUM, MAX_FIELD_NAME_LEN, MAX_STRING_LEN, MAX_DEPTH, a);\
        cout << "create bsonobj end" << endl;


void mySleep(UINT32 sec=10)
{
   boost::xtime xt;
   boost::xtime_get(&xt, boost::TIME_UTC_);
   xt.sec += sec;
   boost::thread::sleep(xt);
}

volatile INT32 FLAG = 0;
boost::thread *THREAD = NULL;

void active(_dpsLogWrapper *wrapper)
{
   while (0 == FLAG)
   {
      wrapper->run( NULL );
   }

   wrapper->tearDown();

   return;
}

void deActiveThread()
{
   FLAG = -1;
   THREAD->join();
   delete THREAD;
   THREAD = NULL;
   FLAG = 0;
   return;
}

void deleteFiles()
{
   for (UINT32 i=0; i<20; i++ )
   {
      string file("sequoiadbLog.");
      file += boost::lexical_cast<string>(i) ;
      ossDelete(file.c_str()) ;
   }
}

#define ACTIVE_THREAD(a) THREAD = new boost::thread(active, a);
#define DEACTIVE_THREAD deActiveThread();

TEST(logWrapperTest, recordInsert_1)
{
   INT32 rc = SDB_OK ;
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.getLogMgr()->init ( ".", DPS_DFT_LOG_BUF_SZ,
                                                  NULL ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordInsert.collection");
   BSONObj obj;
   CRE_OBJ(obj)

   dpsMergeInfo mergeInfo ;
   dpsLogRecord &record = mergeInfo.getMergeBlock().record();
   rc = dpsInsert2Record( name.c_str(),
                          obj, DPS_INVALID_TRANS_ID,
                          DPS_INVALID_LSN_OFFSET,
                          DPS_INVALID_LSN_OFFSET,
                          record ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;

   rc = wrapper.prepare( mergeInfo ) ;
   wrapper.writeData(mergeInfo) ;


   DEACTIVE_THREAD
   _OSS_FILE file;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[record.alignedLen()];

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, record.alignedLen() , &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == record.alignedLen());
   dpsLogRecord rowRecord ;
   rc = rowRecord.load( buf ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   cout << rowRecord.head()._lsn << " " << rowRecord.head()._length << " " << rowRecord.head()._type ;


   ASSERT_TRUE(rowRecord.head()._lsn == 0);
   ASSERT_TRUE(rowRecord.head()._length == record.alignedLen() );
   ASSERT_TRUE(rowRecord.head()._type == LOG_TYPE_DATA_INSERT);

   dpsLogRecord::iterator itr = rowRecord.find( DPS_LOG_PUBLIC_FULLNAME ) ;
   ASSERT_TRUE( itr.valid() ) ;

   rc = ossStrncmp(name.c_str(), itr.value(), name.size() );
   ASSERT_TRUE( SDB_OK == rc );

   itr = rowRecord.find( DPS_LOG_INSERT_OBJ ) ;
   ASSERT_TRUE( itr.valid() ) ;
   BSONObj objData(itr.value());
   ASSERT_TRUE(0 == obj.woCompare(objData));
   ossClose(file);
   deleteFiles() ;
   delete []buf;
}

/**
TEST(logWrapperTest, recordUpdate_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordUpdate");
   string clname("collection");

   BSONObj oldMatch, newMatch ;
   BSONObj obj;
   CRE_OBJ(obj)
   BSONObj obj1 ;
   CRE_OBJ(obj1)
   UINT64 tmplen = sizeof(dpsLogInfo) + strlen(name.c_str())+1 +
                   strlen(clname.c_str())+1+oldMatch.objsize()+ obj.objsize() +
                   newMatch.objsize() + obj1.objsize();
   UINT64 len = ossRoundUpToMultipleX(tmplen, sizeof(SINT32));
   cout << "len:" << len << endl;

   dpsMergeInfo mergeInfo ;
   wrapper.recordUpdate(name.c_str(), clname.c_str(), oldMatch, obj,
                        newMatch, obj1, mergeInfo);
   wrapper.writeData(mergeInfo) ;

   DEACTIVE_THREAD
   _OSS_FILE file;
   INT32 rc = SDB_OK;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[len];

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, len, &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == (SINT64)len);
   _dpsLogInfo *info = (_dpsLogInfo *)buf;

   ASSERT_TRUE(info->_lsn == 0);
   ASSERT_TRUE(info->_length == ossRoundUpToMultipleX(len, sizeof(SINT32) ) );
   ASSERT_TRUE(info->_type == LOG_TYPE_DATA_UPDATE);
   ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);

   char *data = buf + sizeof(dpsLogInfo) ;

   rc = ossStrncmp(name.c_str(), data, strlen(name.c_str()) );
   ASSERT_TRUE( SDB_OK == rc );
   rc = ossStrncmp(clname.c_str(), &data[strlen(name.c_str())+1], strlen(clname.c_str())+1) ;
   ASSERT_TRUE( SDB_OK == rc );
   data += strlen(name.c_str())+1+strlen(clname.c_str())+1;

   BSONObj objDataMatch(data);
   data += objDataMatch.objsize() ;
   BSONObj objData(data);
   ASSERT_TRUE(0 == obj.woCompare(objData));
   data += objData.objsize() ;
   BSONObj newobjDataMatch(data) ;
   data += newobjDataMatch.objsize() ;
   BSONObj newobjData(data) ;
   ASSERT_TRUE(0 == obj1.woCompare(newobjData)) ;

   ossClose(file);
   delete []buf;
   deleteFiles() ;
}


TEST(logWrapperTest, recordDelete_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordDelete");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   BSONElement ele = obj.getField ( "_id" );
   UINT64 len = sizeof(dpsLogInfo) + strlen(name.c_str())+1 + strlen(clname.c_str())+1+obj.objsize();
   cout << "record end:" << len << endl;

   dpsMergeInfo mergeInfo ;
   wrapper.recordDelete(name.c_str(), clname.c_str(), obj, mergeInfo);
   wrapper.writeData( mergeInfo ) ;
   DEACTIVE_THREAD
   _OSS_FILE file;
   INT32 rc = SDB_OK;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[len];

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, len, &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == (SINT64)len);
   _dpsLogInfo *info = (_dpsLogInfo *)buf;

   ASSERT_TRUE(info->_lsn == 0);
   ASSERT_TRUE(info->_length == ossRoundUpToMultipleX(len, sizeof(SINT32) ) );
   ASSERT_TRUE(info->_type == LOG_TYPE_DATA_DELETE);
   ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);

   char *data = buf + sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name.c_str(), data, strlen(name.c_str()) );
   ASSERT_TRUE( SDB_OK == rc );
   rc = ossStrncmp(clname.c_str(), &data[strlen(name.c_str())+1], strlen(clname.c_str())+1) ;
   ASSERT_TRUE( SDB_OK == rc );
   data += strlen(name.c_str())+1+strlen(clname.c_str())+1;

   ossClose(file);
   delete []buf;
   deleteFiles() ;

}

TEST(logWrapperTest, recordCScrt_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordScrt_1");

   UINT64 len = sizeof(dpsLogInfo) + strlen(name.c_str())+1 +sizeof (INT32);
   cout << "record end:" << len << endl;

   dpsMergeInfo mergeInfo ;
   wrapper.recordCScrt(name.c_str(), 4096, mergeInfo );
   wrapper.writeData( mergeInfo ) ;

   DEACTIVE_THREAD
   _OSS_FILE file;
   INT32 rc = SDB_OK;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[len];

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, len, &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == (SINT64)len);
   _dpsLogInfo *info = (_dpsLogInfo *)buf;
   cout << info->_lsn << " " << info->_length << " " << info->_type << " "
        << info->_namelen << endl;


   ASSERT_TRUE(info->_lsn == 0);
   ASSERT_TRUE(info->_length == ossRoundUpToMultipleX(len, sizeof(SINT32) ) );
   ASSERT_TRUE(info->_type == LOG_TYPE_CS_CRT);
   ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1);

   char *data = buf + sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name.c_str(), data, strlen(name.c_str())+1 );
   ASSERT_TRUE( SDB_OK == rc );
   ossClose(file);
   delete []buf;
   deleteFiles() ;
}


TEST(logWrapperTest, recordCSdel_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordSdel_1");

   UINT64 len = sizeof(dpsLogInfo) + strlen(name.c_str())+1 ;
   cout << "record end:" << len << endl;

   dpsMergeInfo mergeInfo ;
   wrapper.recordCSdel(name.c_str(), mergeInfo);
   wrapper.writeData( mergeInfo ) ;

   DEACTIVE_THREAD
   _OSS_FILE file;
   INT32 rc = SDB_OK;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[len];

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, len, &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == (SINT64)len);
   _dpsLogInfo *info = (_dpsLogInfo *)buf;
   cout << info->_lsn << " " << info->_length << " " << info->_type << " "
        << info->_namelen << endl;


   ASSERT_TRUE(info->_lsn == 0);
   ASSERT_TRUE(info->_length == ossRoundUpToMultipleX(len, sizeof(SINT32) ) );
   ASSERT_TRUE(info->_type == LOG_TYPE_CS_DELETE);
   ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1);

   char *data = buf + sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name.c_str(), data, strlen(name.c_str())+1 );
   ASSERT_TRUE( SDB_OK == rc );
   ossClose(file);
   delete []buf;
   deleteFiles() ;
}


TEST(logWrapperTest, recordAll_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   UINT64 len = 0;

   dpsMergeInfo mergeInfo ;

   string name1("recordInsert");
   string clname1("collection");
   BSONObj obj1;
   CRE_OBJ(obj1)
   wrapper.recordInsert(name1.c_str(), clname1.c_str(),obj1, mergeInfo);
   wrapper.writeData(mergeInfo) ;

   string name2("recordUpdate");
   string clname2("collection");
   BSONObj oldMatch, newMatch ;
   BSONObj obj2;
   CRE_OBJ(obj2)
   BSONObj obj2_1 ;
   CRE_OBJ(obj2_1)
   wrapper.recordUpdate(name2.c_str(), clname2.c_str(),
                        oldMatch, obj2, newMatch, obj2_1, mergeInfo);
   wrapper.writeData(mergeInfo) ;

   string name3("recordDelete");
   string clname3("collection");
   BSONObj obj3;
   CRE_OBJ(obj3)
   wrapper.recordDelete(name3.c_str(), clname3.c_str(),
                        obj3, mergeInfo);
   wrapper.writeData( mergeInfo ) ;

   string name4("recordScrt_1");
   wrapper.recordCScrt(name4.c_str(), 4096, mergeInfo );
   wrapper.writeData( mergeInfo ) ;

   string name5("recordSdel_1");
   wrapper.recordCSdel(name5.c_str(), mergeInfo);
   wrapper.writeData( mergeInfo ) ;

   UINT64 len1 = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name1.c_str())+1 +strlen(clname1.c_str())+1+ obj1.objsize(), sizeof(SINT32));
   UINT64 len2 = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name2.c_str())+1 +strlen(clname2.c_str())+1+
                                       oldMatch.objsize()+obj2.objsize()+newMatch.objsize()+obj2_1.objsize(), sizeof(SINT32));
   UINT64 len3 = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name3.c_str())+1 +strlen(clname3.c_str())+1+ obj3.objsize(), sizeof(SINT32));
   UINT64 len4 = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name4.c_str())+1+sizeof(SINT32), sizeof(SINT32));
   UINT64 len5 = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name5.c_str())+1, sizeof(SINT32));

   len += len1 + len2 + len3 + len4 + len5;

   DEACTIVE_THREAD
   _OSS_FILE file;
   INT32 rc = SDB_OK;
   rc = ossOpen("sequoiadbLog.0", OSS_DEFAULT | OSS_READWRITE | OSS_SHAREWRITE,
                OSS_RU | OSS_WU | OSS_RG, file);

   ASSERT_TRUE(SDB_OK == rc);

   CHAR *buf = new CHAR[len];
   CHAR *start = buf;

   SINT64 read = 0;
   rc = ossSeekAndRead(&file, 64*1024, buf, len, &read);
   ASSERT_TRUE(SDB_OK == rc);
   ASSERT_TRUE(read == (SINT64)len);


   _dpsLogInfo *info = (_dpsLogInfo *)buf;
   ASSERT_TRUE(info->_lsn == 0);
   ASSERT_TRUE(info->_length == len1 );
   ASSERT_TRUE(info->_type == LOG_TYPE_DATA_INSERT);
   ASSERT_TRUE(info->_namelen == strlen(name1.c_str())+1+strlen(clname1.c_str())+1);

   buf += sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name1.c_str(), buf, strlen(name1.c_str()) );
   ASSERT_TRUE( SDB_OK == rc );
   rc = ossStrncmp(clname1.c_str(), &buf[strlen(name1.c_str())+1], strlen(clname1.c_str())+1) ;
   ASSERT_TRUE( SDB_OK == rc );
   buf += strlen(name1.c_str())+1+strlen(clname1.c_str())+1;
   BSONObj objData1(buf);
   ASSERT_TRUE(0 == obj1.woCompare(objData1));

   buf = start + len1;
   info = (_dpsLogInfo *)buf;
   ASSERT_TRUE(info->_lsn == len1); cout << info->_lsn << " " << len1 << endl; 
   ASSERT_TRUE(info->_length == len2 );
   ASSERT_TRUE(info->_type == LOG_TYPE_DATA_UPDATE);
   ASSERT_TRUE(info->_namelen == strlen(name2.c_str())+1+strlen(clname2.c_str())+1);

   buf += sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name2.c_str(), buf, strlen(name2.c_str()) );
   ASSERT_TRUE( SDB_OK == rc );
   rc = ossStrncmp(clname2.c_str(), &buf[strlen(name2.c_str())+1], strlen(clname2.c_str())+1) ;
   ASSERT_TRUE( SDB_OK == rc );
   buf += strlen(name2.c_str())+1+strlen(clname2.c_str())+1;

   BSONObj objData2Match(buf) ;
   buf += objData2Match.objsize() ;
   BSONObj objData2(buf);
   ASSERT_TRUE(0 == obj2.woCompare(objData2));

   buf = start + len1 + len2;
   info = (_dpsLogInfo *)buf;
   ASSERT_TRUE(info->_lsn == len1+len2);
   ASSERT_TRUE(info->_length == len3 );
   ASSERT_TRUE(info->_type == LOG_TYPE_DATA_DELETE);
   ASSERT_TRUE(info->_namelen == strlen(name3.c_str())+1+strlen(clname3.c_str())+1);

   buf += sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name3.c_str(), buf, strlen(name3.c_str()) );
   ASSERT_TRUE( SDB_OK == rc );
   rc = ossStrncmp(clname3.c_str(), &buf[strlen(name3.c_str())+1], strlen(clname3.c_str())+1) ;
   ASSERT_TRUE( SDB_OK == rc );
   buf += strlen(name3.c_str())+1+strlen(clname3.c_str())+1;

   buf = start + len1 + len2 + len3;
   info = (_dpsLogInfo *)buf;
   ASSERT_TRUE(info->_lsn == len1+len2+len3);
   ASSERT_TRUE(info->_length == len4 );
   ASSERT_TRUE(info->_type == LOG_TYPE_CS_CRT);
   ASSERT_TRUE(info->_namelen == strlen(name4.c_str())+1);

   buf += sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name4.c_str(), buf, strlen(name4.c_str())+1 );
   ASSERT_TRUE( SDB_OK == rc );

   buf = start + len1 + len2 + len3 + len4;
   info = (_dpsLogInfo *)buf;
   ASSERT_TRUE(info->_lsn == len1+len2+len3+len4);
   ASSERT_TRUE(info->_length == len5 );
   ASSERT_TRUE(info->_type == LOG_TYPE_CS_DELETE);
   ASSERT_TRUE(info->_namelen == strlen(name5.c_str())+1);

   buf += sizeof(dpsLogInfo) ;
   rc = ossStrncmp(name5.c_str(), buf, strlen(name5.c_str())+1 );
   ASSERT_TRUE( SDB_OK == rc );

   ossClose(file);
   delete []start;
   deleteFiles() ;


}

TEST(dpsReplicaLogMgrTest, search_index_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 50 * 1024);
   UINT64 total = 100;
   cout << "len:" << len  << " total records:" << total << endl;
   wrapper.incVersion() ;
   for (UINT64 i=0; i<total; i++)
   {
      dpsMergeInfo info ;
      ASSERT_TRUE(SDB_OK == wrapper.recordInsert(name.c_str(), clname.c_str(), obj, info));
      wrapper.writeData(info) ;
   }
   wrapper.tearDown();
   cout << "insert done" << endl;

   DPS_LSN lsn = wrapper.getStartLsn ( TRUE ) ;
   ASSERT_TRUE(lsn.offset == 0);
   ASSERT_TRUE(lsn.version == 1);
   INT32 count = 0 ;
   INT32 rc = SDB_OK ;
   while ( TRUE )
   {
      _dpsMessageBlock mb(len);
      rc = wrapper.search(lsn, &mb, DPS_SEARCH_MEM);
      if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         break ;
      _dpsLogInfo *info = (_dpsLogInfo *)(mb.offset(0));
      ASSERT_TRUE(SDB_OK == rc);
      ASSERT_TRUE(info->_lsn == lsn.offset);
      ASSERT_TRUE(info->_version == lsn.version );
      if ( info->_type != LOG_TYPE_DUMMY )
      {
         ASSERT_TRUE(info->_length >= len ) ;
         ASSERT_TRUE(info->_type == LOG_TYPE_DATA_INSERT );
         ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);
      }
      lsn.offset += info->_length ;
      ++count ;
   }
   ASSERT_TRUE ( 0 != count ) ;
   ASSERT_TRUE ( SDB_DPS_LSN_OUTOFRANGE == rc ) ;
   deleteFiles() ;
}

TEST(dpsReplicaLogMgrTest, search_index_2)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 50 * 1024);
   UINT64 total = 64*1024*200 / len * 5  + 100;
   cout << "len:" << len  << " total records:" << total << endl;
   wrapper.incVersion();
   for (UINT64 i=0; i<total; i++)
   {
      dpsMergeInfo mergeInfo ;
      while ( TRUE )
      {
         if (SDB_OK != wrapper.recordInsert(name.c_str(),clname.c_str(), obj, mergeInfo))
         {
            mySleep(1) ;
         }
         else
         {
            wrapper.writeData(mergeInfo) ;
            break;
         }
      }
   }
   DEACTIVE_THREAD
   cout << "insert done" << endl;
   DPS_LSN lsn = wrapper.getStartLsn ( TRUE ) ;
   INT32 count = 0 ;
   INT32 rc = SDB_OK ;
   while ( TRUE )
   {
      _dpsMessageBlock mb(len);
      rc = wrapper.search(lsn, &mb, DPS_SEARCH_MEM);
      if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         break ;
      _dpsLogInfo *info = (_dpsLogInfo *)(mb.offset(0));
      ASSERT_TRUE(SDB_OK == rc);
      ASSERT_TRUE(info->_lsn == lsn.offset);
      ASSERT_TRUE(info->_version == lsn.version);
      if ( info->_type != LOG_TYPE_DUMMY )
      {
         ASSERT_TRUE(info->_length >= len ) ;
         ASSERT_TRUE(info->_type == LOG_TYPE_DATA_INSERT );
         ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);
      }
      lsn.offset += info->_length ;
      ++count ;
   }
   ASSERT_TRUE ( 0 != count ) ;
   ASSERT_TRUE ( SDB_DPS_LSN_OUTOFRANGE == rc ) ;
   deleteFiles() ;
}



TEST(dpsReplicaLogMgrTest, search_file_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 64 * 1024);

   UINT64 total = 32 * 1024 * 1024 / len * 10;

   cout << "len:" << len  << " total records:" << total << endl;
   wrapper.incVersion() ;
   for (UINT64 i=0; i<total; i++)
   {
      dpsMergeInfo mergeInfo ;
      while ( TRUE )
      {
         if (SDB_OK != wrapper.recordInsert(name.c_str(), clname.c_str(), obj, mergeInfo))
         {
            wrapper.tearDown();
         }
         else
         {
            wrapper.writeData(mergeInfo) ;
           break;
         }
      }
   }
   DEACTIVE_THREAD
   cout << "insert done" << endl;
   _dpsMessageBlock mb(len);
   DPS_LSN lsn;
   lsn.offset = 0 ;
   lsn.version = 1 ;
   INT32 rc = SDB_OK ;
   INT32 count = 0 ;
   while ( TRUE )
   {
      rc = wrapper.search(lsn, &mb, DPS_SEARCH_FILE);
      if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         break ;
      _dpsLogInfo *info = (_dpsLogInfo *)(mb.offset(0));
      ASSERT_TRUE(SDB_OK == rc);
      ASSERT_TRUE(info->_lsn == lsn.offset);
      ASSERT_TRUE(info->_version == lsn.version);
      if ( info->_type != LOG_TYPE_DUMMY )
      {
         ASSERT_TRUE(info->_length >= len ) ;
         ASSERT_TRUE(info->_type == LOG_TYPE_DATA_INSERT );
         ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);
      }
      mb.clear();
      lsn.offset += info->_length ;
      ++count ;
   }
   ASSERT_TRUE ( 0 != count ) ;
   ASSERT_TRUE ( SDB_DPS_LSN_OUTOFRANGE == rc ) ;
   deleteFiles() ;
}
**/

/*
TEST(dpsLogFileMgrTest, search_file_2)
{
   deleteFiles();
   _dpsLogWrapper wrapper;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   ACTIVE_THREAD(&wrapper)
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 64 * 1024);

   UINT64 total = 32 * 1024 * 1024 / len * 20 * 2;

   cout << "len:" << len  << " total records:" << total << endl;
   wrapper.incVersion() ;
   for (UINT64 i=0; i<total; i++)
   {
      while ( TRUE )
      {
         if (SDB_OK != wrapper.recordInsert(name.c_str(), clname.c_str(),obj))
         {
            mySleep(1) ;
         }
         else
         {
           break;
         }
      }
   }
   DEACTIVE_THREAD
   cout << "insert done" << endl;
   _dpsMessageBlock mb(len);
   DPS_LSN lsn = wrapper.getStartLsn () ;
   INT32 count = 0 ;
   INT32 rc = SDB_OK ;
   while ( TRUE )
   {
      rc = wrapper.search(lsn, &mb, DPS_SEARCH_FILE);
      if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         break ;
      _dpsLogInfo *info = (_dpsLogInfo *)(mb.offset(0));
      ASSERT_TRUE(SDB_OK == rc);
      ASSERT_TRUE(info->_lsn == lsn.offset);
      ASSERT_TRUE(info->_version == lsn.version);
      if ( info->_type != LOG_TYPE_DUMMY )
      {
         ASSERT_TRUE(info->_length >= len ) ;
         ASSERT_TRUE(info->_type == LOG_TYPE_DATA_INSERT );
         ASSERT_TRUE(info->_namelen == strlen(name.c_str())+1+strlen(clname.c_str())+1);
      }
      mb.clear();
      lsn.offset += info->_length ;
      ++count ;
   }
   ASSERT_TRUE ( 0 != count ) ;
   ASSERT_TRUE ( SDB_DPS_LSN_OUTOFRANGE == rc ) ;
}
*/

/**
TEST(logWrapperTest, move_1)
{
   deleteFiles();
   _dpsLogWrapper wrapper ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   wrapper.incVersion() ;
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 50 * 1024);
   UINT64 total = 100;
   cout << "len:" << len  << " total records:" << total << endl;
   wrapper.incVersion() ;
   for (UINT64 i=0; i<total; i++)
   {
      dpsMergeInfo mergeInfo ;
      ASSERT_TRUE(SDB_OK == wrapper.recordInsert(name.c_str(), clname.c_str(), obj, mergeInfo));
      wrapper.writeData( mergeInfo ) ;
   }
   wrapper.tearDown();
   cout << "insert done" << endl;
   ASSERT_TRUE( (total-1) * len == wrapper.getCurrentLsn().offset );

   DPS_LSN lsn ;
   lsn.version = 1;
   lsn.offset = (total-1) * len;
   _dpsMessageBlock mb(len) ;
   ASSERT_TRUE(SDB_OK == wrapper.search(lsn, &mb, DPS_SEARCH_MEM));

   wrapper.move( total / 2 * len, 1 ) ;
   ASSERT_TRUE( (total/2)*len == wrapper.expectLsn().offset);
   ASSERT_TRUE(SDB_OK !=wrapper.search(lsn, &mb, DPS_SEARCH_MEM));
   lsn.offset = (total / 2 ) * len - len ;
   ASSERT_TRUE(SDB_OK == wrapper.search(lsn, &mb, DPS_SEARCH_MEM));
   deleteFiles() ;
}
**/
/*
TEST(logWrapperTest, move_2)
{
   deleteFiles();
   _dpsLogWrapper wrapper;
   wrapper.incVersion() ;
   ASSERT_TRUE ( 0 == wrapper.init ( "." ) ) ;
   string name("recordInsert");
   string clname("collection");
   BSONObj obj;
   CRE_OBJ(obj)
   UINT32 len = ossRoundUpToMultipleX(sizeof(dpsLogInfo) + strlen(name.c_str())+1 +strlen(clname.c_str())+1+ obj.objsize(), sizeof(SINT32));
   ASSERT_TRUE(len < 50 * 1024);
   UINT64 total = 64*1024*200 / len + 1;
   cout << "len:" << len  << " total records:" << total << endl;
   for (UINT64 i=0; i<total; i++)
   {
      while ( TRUE )
      {
         if (SDB_OK != wrapper.recordInsert(name.c_str(),clname.c_str(), obj))
         {
            wrapper.tearDown();
         }
         else
         {
            break;
         }
      }
   }
   wrapper.tearDown();
   cout << "insert done" << endl;

   DPS_LSN lsn ;
   lsn.version = 1;
   lsn.offset = (total-1) * len;
   _dpsMessageBlock mb(len) ;
   ASSERT_TRUE(SDB_OK == wrapper.search(lsn, &mb, DPS_SEARCH_MEM));
   wrapper.move( total / 2 * len, 1 ) ;
   ASSERT_TRUE( (total / 2 ) * len == wrapper.getCurrentLsn().offset );
   ASSERT_TRUE( (total/2)*len + len == wrapper.expectLsn().offset);
   ASSERT_TRUE(SDB_OK !=wrapper.search(lsn, &mb, DPS_SEARCH_MEM));
   lsn.offset = (total / 2 ) * len;
   ASSERT_TRUE(SDB_OK == wrapper.search(lsn, &mb, DPS_SEARCH_MEM));
}
*/
