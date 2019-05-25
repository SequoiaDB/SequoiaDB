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

#include "ossTypes.h"
#include <gtest/gtest.h>

#include "pmdEDU.hpp"
#include "ossErr.h"
#include "pmdEDUMgr.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"

using namespace engine;

TEST(catalogMainControllerTest, init)
{
   pmdEDUMgr *pEduMgr = pmdGetKRCB()->getEDUMgr();
   EDUID agentEDU = PMD_INVALID_EDUID;
   INT32 rc = pEduMgr->startEDU(EDU_TYPE_CATMGR, NULL, &agentEDU);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_NE(PMD_INVALID_EDUID, agentEDU);
   boost::xtime sleepTime;
   boost::xtime_get(&sleepTime, boost::TIME_UTC_);
   sleepTime.sec += 1;
   boost::thread::sleep(sleepTime);
   pmdGetKRCB()->destroy();
}
