/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
