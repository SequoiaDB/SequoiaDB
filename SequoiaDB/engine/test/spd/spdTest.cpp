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
#include "spdFMPMgr.hpp"
#include "spdFMP.hpp"

using namespace engine ;

TEST(spdTest, spdTest_1)
{
   INT32 rc = SDB_OK ;
   spdFMPMgr fmpMgr ;
   rc = fmpMgr.init() ;
   ASSERT_TRUE( rc == SDB_OK ) ;

   spdFMP *fmp = NULL ;
   rc = fmpMgr.getFMP( fmp ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   getchar () ;
}
