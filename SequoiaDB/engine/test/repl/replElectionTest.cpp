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
#include "replCongressman.hpp"
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>


using namespace std;
using namespace engine;

const UINT32 PROPOSERNUM = 5;
const UINT32 ACCEPTORNUM = 5;

#define LOOP_OP(a, b)\
        for(UINT32 i=0; i<a; i++)\
        {\
           b;\
        }

void init_proposer(_replEleMotion &motion)
{
   vector<_replElePartner> acceptors;
   for (UINT32 i=0; i<ACCEPTORNUM; i++)
   {
      _replElePartner p ;
      p._id = i;
      p._sockid = i;
      acceptors.push_back(p);
   }

   _replCongressman::proposeMotion(acceptors,
                                   0,
                                   PROPOSERNUM,
                                   motion);
   return ;
}

void init_accept(_replEleMotion &motion)
{
   _replCongressman::acceptMotion(motion);
}

TEST(replCongressmanTest, base_1)
{
   _replCongressman proposer ;
   _replEleMotion motion;
   _MsgEleMsg msg;
   init_proposer(motion);

   proposer.init(motion);
   ASSERT_TRUE(REPL_ELE_STATE_PREPAREPEND == proposer.state());
   ASSERT_TRUE(REPL_ELE_PROPOSER == proposer.role());

   msg._header.opCode = REPL_ELE_MSG_ACCEPT;
   msg._ballot._id = 5;
   msg._ballot._proposerid = 0;
   LOOP_OP(ACCEPTORNUM, proposer.handleInput(&msg))
   ASSERT_TRUE(REPL_ELE_STATE_PROPOSEPEND == proposer.state());

   msg._header.opCode = REPL_ELE_MSG_ADOPT;
   LOOP_OP(ACCEPTORNUM, proposer.handleInput(&msg))
   ASSERT_TRUE(REPL_ELE_STATE_LEADER == proposer.state());
   ASSERT_TRUE(REPL_ELE_LEADER == proposer.role());

   LOOP_OP(REPL_ELE_LEAD_TIME /2, proposer.timeout(1))
   ASSERT_TRUE(REPL_ELE_STATE_CONTINUEPEND == proposer.state());
   ASSERT_TRUE(REPL_ELE_LEADER == proposer.role());

   LOOP_OP(REPL_ELE_LEAD_TIME - REPL_ELE_LEAD_TIME /2, proposer.timeout(1))
   ASSERT_TRUE(REPL_ELE_STATE_PREPARE == proposer.state());
   ASSERT_TRUE(REPL_ELE_PROPOSER == proposer.role());

   LOOP_OP(REPL_ELE_LOOP_INTERVAL_SEC, proposer.timeout(1))
   ASSERT_TRUE(REPL_ELE_STATE_PREPAREPEND == proposer.state());

}

TEST(replCongressmanTest, base_2)
{
   _replCongressman acceptor;
   _replEleMotion motion;
   _MsgEleMsg msg;
   init_accept(motion);

   acceptor.init(motion);
   ASSERT_TRUE(REPL_ELE_STATE_BEFORE_ACC == acceptor.state());
   ASSERT_TRUE(REPL_ELE_ACCEPTOR == acceptor.role());

   LOOP_OP(REPL_ELE_SILENT_TIME, acceptor.timeout(1))
   ASSERT_TRUE(REPL_ELE_STATE_WAIT_ACC == acceptor.state());

   msg._header.opCode = REPL_ELE_MSG_PREPARE;
   msg._ballot._id = 5;
   msg._ballot._proposerid = 0;
   acceptor.handleInput(&msg);
   ASSERT_TRUE(REPL_ELE_STATE_WAIT_ACC == acceptor.state());

   msg._header.opCode = REPL_ELE_MSG_PROPOSE;
   acceptor.handleInput(&msg);
   ASSERT_TRUE(REPL_ELE_STATE_COMPLETE_ACC == acceptor.state());

   LOOP_OP(REPL_ELE_LEAD_TIME, acceptor.timeout(1))
   ASSERT_TRUE(REPL_ELE_STATE_WAIT_ACC == acceptor.state());
}
