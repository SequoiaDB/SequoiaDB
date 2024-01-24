/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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
#include <iostream>

#include "optStatUnit.hpp"

using namespace std;
using namespace engine;
using namespace bson ;

TEST(optIndexPathTest, empty)
{
   optIndexPathEncoder encoder ;
   std::cout << encoder.getPath() << std::endl ;
   ASSERT_TRUE( encoder.getPath() == ossPoolString( "" ) ) ;
}

TEST(optIndexPathTest, prefix_1)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", TRUE ) ;
   encoderL.append( "a", FALSE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() == encoderL.getPath() ) ;
}

TEST(optIndexPathTest, prefix_2)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", TRUE ) ;
   encoderL.append( "a", FALSE ) ;
   encoderL.append( "c", TRUE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() == encoderL.getPath() ) ;
}

TEST(optIndexPathTest, name_with_underline)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", FALSE ) ;
   encoderL.append( "a_b", FALSE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() != encoderL.getPath() ) ;
}
