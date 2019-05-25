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
#include "ossUtil.hpp"
#include "netRouteAgent.hpp"
#include "netMsgHandler.hpp"
#include "ossSocket.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <boost/thread.hpp>

using namespace std;
using namespace engine;

struct testMsg
{
   UINT32 a;
   UINT64 b;
};

struct myMsg
{
   _MsgHeader header;
   testMsg body;
};

_MsgRouteID MY_ID_1;

_MsgRouteID MY_ID_2;

const CHAR *HOST = "localhost";
const CHAR *SERVICE = "12345";
const UINT32 MSG_TYPE1= 1;
const UINT32 MSG_REQUESTID = 10;
const UINT32 MSG_A = 5000;
const UINT64 MSG_B = 100000;
#define INIT_HEADER(msg, i)\
        {UINT32 base = i;\
         msg.header.messageLength = sizeof(myMsg);\
         msg.header.opCode = base;\
         msg.header.requestID = base+1; \
         msg.header.routeID.columns.groupID = base+2 ;\
         msg.header.routeID.columns.nodeID = base+2 ;\
         msg.header.routeID.columns.serviceID = base+2 ;\
         msg.body.a = base+3;\
         msg.body.b = base+4; }

void run(_netFrame *frame)
{
   ASSERT_TRUE( SDB_OK == frame->listen(HOST, SERVICE));
   frame->run();
}

void run2(_netFrame *frame, const CHAR *service)
{
   ASSERT_TRUE( SDB_OK == frame->listen(HOST, service));
   frame->run();
   cout << "run end:" << service << endl ;
}

class myHandler : public _netMsgHandler
{
   public:
      myHandler(){_flag = FALSE;_needReply = FALSE ;}
      virtual ~myHandler(){}
   public:
      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg )
      {
         _header = *header ;
         myMsg *m = (myMsg *)msg;
         _msg.a = m->body.a;
         _msg.b = m->body.b;
         _flag = TRUE;
         if (_needReply)
         {
            INT32 rc = _agent->syncSend(handle, m ) ;
            cout << "reply res:" << rc << endl ;
          }
         return 0;
      }
      void wait()
      {
         while (!_flag){ossSleepmillis(100);}
         _flag = FALSE;
      }
   public:
      _MsgHeader _header ;
      testMsg _msg ;
      _netRouteAgent *_agent;
      BOOLEAN _needReply ;
      volatile BOOLEAN _flag;
};

class myTimeoutHandler : public _netTimeoutHandler
{
   private:
      virtual ~myTimeoutHandler(){}
   public:
      myTimeoutHandler():_i(0){}
      virtual void handleTimeout( const UINT32 &millisec,
                                  const UINT32 &id )
      {
         _i += millisec ;
      }
   public:
      UINT32 _i ;
};

TEST(netTest, listen_connect_1)
{
   myHandler handler;
   _netFrame acceptor(&handler);
   boost::thread t(run, &acceptor);

   ossSleepsecs(1);
   _netFrame connector(NULL);
   MY_ID_1.columns.groupID = 1;
   MY_ID_1.columns.nodeID = 1;
   ASSERT_TRUE(SDB_OK == connector.syncConnect(HOST, SERVICE, MY_ID_1));
   myMsg msg;
   msg.header.messageLength = sizeof( myMsg ) ;
   boost::posix_time::ptime total ;
   for (UINT32 i=0; i<10; i++)
   {
      INIT_HEADER(msg, i)
      ASSERT_TRUE(SDB_OK == connector.syncSend(MY_ID_1, &msg));
      handler.wait();
      ASSERT_TRUE(handler._header.messageLength == msg.header.messageLength);
      ASSERT_TRUE(handler._header.opCode ==  msg.header.opCode);
      ASSERT_TRUE(handler._header.TID == msg.header.TID);
      ASSERT_TRUE(handler._header.routeID.value == msg.header.routeID.value);
      ASSERT_TRUE(handler._header.requestID == msg.header.requestID);
      ASSERT_TRUE(handler._msg.a == msg.body.a);
      ASSERT_TRUE(handler._msg.b == msg.body.b);
   }
   acceptor.stop();
   t.join();
}

TEST(netTest, timer_1)
{
   _netTimeoutHandler *h1 = NULL;
   _netTimeoutHandler *h2 = NULL;
   myTimeoutHandler *my1 = new myTimeoutHandler();
   myTimeoutHandler *my2 = new myTimeoutHandler();
   h1 = my1;
   h2 = my2;
   myHandler handler;
   _netFrame frame(&handler);
   boost::thread t(run, &frame);
   ossSleepsecs(1);
   UINT32 id1 = 0;
   UINT32 id2 = 0;
   UINT32 timeout1 = 456;
   UINT32 timeout2 = 1024;
   frame.addTimer(timeout1, h1, id1 );
   frame.addTimer(timeout2, h2, id2 );
   ASSERT_TRUE(id1 != id2);
   ossSleepsecs(5);
   frame.stop();
   t.join();
   ASSERT_TRUE(0 != my1->_i);
   ASSERT_TRUE(0 == my1->_i % timeout1);
   ASSERT_TRUE(0 != my2->_i);
   ASSERT_TRUE(0 == my2->_i % timeout2);
}

void run_agent_1( _netRouteAgent *agent, _MsgRouteID *id )
{
   ASSERT_TRUE(SDB_OK == agent->listen(*id)) ;
   agent->run() ;
}

/*
TEST(netTest, multi_instance_1)
{
   myHandler handler1;
   myHandler handler2;
   _netFrame frame1(&handler1);
   _netFrame frame2(&handler2);
   UINT32 id ;
   _netTimeoutHandler *t = new myTimeoutHandler();
   frame1.addTimer( 5, t, id) ;
   boost::thread t1(run2, &frame1, "6000");
   boost::thread t2(run2, &frame2, "6001");
  t1.join() ;
}*/


TEST(netTest, agent_1)
{
   myHandler handler;
   _netRouteAgent agent(&handler);
   handler._agent = &agent ;
   handler._needReply = TRUE ;

   agent.updateRoute(MY_ID_1, HOST, "50000");
   agent.updateRoute(MY_ID_2, HOST, "50001");
   boost::thread t(run_agent_1, &agent, &MY_ID_1);

   myHandler handler2;
   ossSleepsecs(1);
   _netRouteAgent sender(&handler2);
   sender.updateRoute(MY_ID_1, HOST, "50000");
   sender.updateRoute(MY_ID_2, HOST, "50001");
   boost::thread t2(run_agent_1, &sender, &MY_ID_2);
   ossSleepsecs(1);
   myMsg msg;
   for (UINT32 i=0; i<10; i++)
   {
      INIT_HEADER(msg, i)
      ASSERT_TRUE(SDB_OK == sender.syncSend(MY_ID_1, &msg ));
      handler.wait();
      ASSERT_TRUE(handler._header.messageLength == msg.header.messageLength);
      ASSERT_TRUE(handler._header.opCode ==  msg.header.opCode);
      ASSERT_TRUE(handler._header.TID == msg.header.TID);
      ASSERT_TRUE(handler._header.routeID.value == msg.header.routeID.value);
      ASSERT_TRUE(handler._header.requestID == msg.header.requestID);
      ASSERT_TRUE(handler._msg.a == msg.body.a);
      ASSERT_TRUE(handler._msg.b == msg.body.b);
      handler2.wait() ;
      ASSERT_TRUE(handler2._header.messageLength == msg.header.messageLength);
      ASSERT_TRUE(handler2._header.opCode ==  msg.header.opCode);
      ASSERT_TRUE(handler2._header.TID == msg.header.TID);
      ASSERT_TRUE(handler2._header.routeID.value == msg.header.routeID.value);
      ASSERT_TRUE(handler2._header.requestID == msg.header.requestID);
      ASSERT_TRUE(handler2._msg.a == msg.body.a);
      ASSERT_TRUE(handler2._msg.b == msg.body.b);
   }

   sender.close(MY_ID_1) ;
   ossSleepsecs(1);
   agent.stop();
   sender.stop() ;
   t.join();
   t2.join() ;
}

TEST(netTest, myTest)
{
   myHandler handler;
   _netFrame acceptor(&handler);
   INT32 sentLen = 0 ;
   boost::thread t(run, &acceptor);

   ossSleepsecs(1);
   MY_ID_1.columns.groupID = 1;
   MY_ID_1.columns.nodeID = 1;
   _ossSocket sock( HOST, 12345) ;
   sock.initSocket() ;
   ASSERT_TRUE( SDB_OK == sock.connect() ) ;
   sock.disableNagle() ;
   myMsg msg;
   msg.header.messageLength = sizeof( myMsg ) ;
   UINT64 total = 0 ;
   INT32 rc = 0 ;
   for (UINT32 i=0; i<1000000; i++)
   {
      INIT_HEADER(msg, i)
      ossTimestamp start;
      ossGetCurrentTime(start);
      rc = sock.send((const CHAR *)&msg, sizeof(msg), sentLen, -1) ;
      ossTimestamp end ;
      ossGetCurrentTime( end ) ;
      total += end.time * 1000000 + end.microtm - start.time*1000000 - start.microtm ;
      ASSERT_TRUE( rc == SDB_OK ) ;
   }
   acceptor.stop();
   t.join();
   cout << total << endl ;
}
