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

   Source File Name = utilLocationTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2022  HYQ  Initial Draft

   Last Changed =

******************************************************************************/

#include"ossTypes.hpp"
#include<gtest/gtest.h>

#include "utilLocation.hpp"
#include <iostream>


namespace engine
{

    /*
   Name: src_select_test1
   Description:
      location affinity test
   Expected Result:
      affinitive in one of below cases:
      1. location1 prefix == location2 prefix
      2. location1 prefix == location2
      3. location1 == location2
   */


   TEST(utilLocationTest, utilAffinity_1)
   {
      ASSERT_TRUE ( FALSE == utilCalAffinity( NULL, NULL ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( NULL, "A.b") ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", NULL ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "", "" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", "" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "", "A.C" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.a", ".a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( ".a", "A.a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( ".a", ".b" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.a", "....." ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A.a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", "B.a" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A.a.b.c" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "AB.b", "A.b" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A", "A.b" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "Adafa", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A", "Adadafa" ) ) ;
   }

}