/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = ossTimeTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/21/2022  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossUtil.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>

using namespace std ;

void checkOssMkTime( std::string time_str, time_t expected )
{
   struct tm temp_tm = { 0 };
   time_t actual     = 0 ;
   INT32 year        = 0 ;
   INT32 month       = 0 ;
   INT32 day         = 0 ;
   INT32 hour        = 0 ;
   INT32 minute      = 0 ;
   INT32 second      = 0 ;

   sscanf ( time_str.c_str(),
           "%d-%d-%d %d:%d:%d",
            &year   ,
            &month  ,
            &day    ,
            &hour   ,
            &minute ,
            &second ) ;

   temp_tm.tm_year  = year - 1900 ;
   temp_tm.tm_mon   = month - 1 ;
   temp_tm.tm_mday  = day    ;
   temp_tm.tm_hour  = hour   ;
   temp_tm.tm_min   = minute ;
   temp_tm.tm_sec   = second ;

   actual = ossMkTime( &temp_tm ) ;
   ASSERT_EQ( actual, expected ) ;
}

TEST( ossTimeTest, ossMkTime )
{
   ASSERT_EQ( ossMkTime( NULL ), -1 ) ;

   // CST
   checkOssMkTime( "1988-04-17 01:30:00", 577215000 ) ;
   checkOssMkTime( "1988-04-17 01:59:59", 577216799 ) ;
   // CST to CDT
   checkOssMkTime( "1988-04-17 02:00:00", 577216800 ) ;
   checkOssMkTime( "1988-04-17 02:30:00", 577218600 ) ;
   checkOssMkTime( "1988-04-17 02:59:59", 577220399 ) ;
   // CDT
   checkOssMkTime( "1988-04-17 03:00:00", 577216800 ) ;
   checkOssMkTime( "1988-04-17 03:30:00", 577218600 ) ;
   checkOssMkTime( "1988-09-11 00:30:00", 589908600 ) ;
   // CDT to CST
   checkOssMkTime( "1988-09-11 01:00:00", 589914000 ) ;
   checkOssMkTime( "1988-09-11 01:30:00", 589915800 ) ;
   checkOssMkTime( "1988-09-11 01:59:59", 589917599 ) ;
   // CST
   checkOssMkTime( "1988-09-11 02:00:00", 589917600 ) ;
   checkOssMkTime( "1988-09-11 02:30:00", 589919400 ) ;
   checkOssMkTime( "1988-09-11 03:00:00", 589921200 ) ;
}
/*
// TODO Reopen after lob support DST(daylight saving time).
void checkToUTCTime( std::string time_str, time_t expected )
{
   struct tm temp_tm = { 0 } ;
   time_t utc = 0 ;
   time_t local = 0 ;

   strptime(time_str.c_str(), "%Y-%m-%d %H:%M:%S", &temp_tm);
   local = ossMkTime( &temp_tm ) ;
   ossTimeLocalToUTCInSameDate( local, utc ) ;
   ASSERT_EQ( utc, expected ) ;
}

TEST( ossTimeTest, ossTimeLocalToUTCInSameDate )
{
   checkToUTCTime( "1988-04-17 01:30:00", 577243800 ) ;
   checkToUTCTime( "1988-04-17 01:59:59", 577245599 ) ;
   checkToUTCTime( "1988-04-17 02:00:00", 577249200 ) ;
   checkToUTCTime( "1988-04-17 02:30:00", 577251000 ) ;
   checkToUTCTime( "1988-04-17 02:59:59", 577252799 ) ;
   checkToUTCTime( "1988-04-17 03:00:00", 577249200 ) ;
   checkToUTCTime( "1988-04-17 03:30:00", 577251000 ) ;
   checkToUTCTime( "1988-09-11 00:30:00", 589941000 ) ;
   checkToUTCTime( "1988-09-11 01:00:00", 589942800 ) ;
   checkToUTCTime( "1988-09-11 01:30:00", 589944600 ) ;
   checkToUTCTime( "1988-09-11 01:59:59", 589946399 ) ;
   checkToUTCTime( "1988-09-11 02:00:00", 589946400 ) ;
   checkToUTCTime( "1988-09-11 02:30:00", 589948200 ) ;
   checkToUTCTime( "1988-09-11 03:00:00", 589950000 ) ;
}
*/