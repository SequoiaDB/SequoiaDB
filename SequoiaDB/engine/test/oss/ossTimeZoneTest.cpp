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

   Source File Name = ossTimeZoneTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/02/2022  Yang Qincheng  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossTimeZone.hpp"
#include "ossMem.h"
#include "ossUtil.h"
#include <gtest/gtest.h>
#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#if defined(_LINUX)
#include <pthread.h>
#endif

using namespace std ;

#if defined(_LINUX)

struct timeZone
{
   INT32 len ;
   CHAR * pName ;
} ;

class ossTimeZoneTest : public testing::Test
{
protected:
    static timeZone tz ;
    static CHAR *originalTZ ;

   static void SetUpTestCase()
   {
      tz.len = OSS_TIMEZONE_MAX_LEN + 1 ;
      tz.pName = (CHAR *)SDB_OSS_MALLOC( tz.len ) ;
      originalTZ = getenv( "TZ" ) ;
   }

   static void TearDownTestCase()
   {
      if ( NULL != tz.pName )
      {
        SDB_OSS_FREE( tz.pName ) ;
      }
      // Restore the environment
      if ( NULL == originalTZ )
      {
         unsetenv( "TZ" ) ;
      }
      else
      {
         setenv( "TZ", originalTZ, 1 ) ;
      }
   }

   void SetUp()
   {
      ossMemset( tz.pName, 0, tz.len ) ;
   }
   void TearDown()
   {
   }
};

timeZone ossTimeZoneTest::tz = { 0 } ;
CHAR * ossTimeZoneTest::originalTZ = NULL ;

TEST_F( ossTimeZoneTest, ossGetTZEnvTest )
{
   INT32 rc  = SDB_OK ;
   INT32 ret = 0 ;
   string value ;

   // case 1: null
   rc = ossGetTZEnv( NULL, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 2: error len
   rc = ossGetTZEnv( tz.pName, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 3: TZ no exists
   rc = ossGetTZEnv( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 4: TZ is empty
   ret = setenv( "TZ", "", 1 ) ;
   ASSERT_EQ( 0, ret ) << "Failed to set TZ as empty, erron = " << errno ;
   rc = ossGetTZEnv( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 5: normal value
   ret = setenv( "TZ", "Asia/Shanghai", 1 ) ;
   ASSERT_EQ( 0, ret ) << "Failed to set TZ as Asia/Shanghai, erron = " << errno ;
   rc = ossGetTZEnv( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;

   // case 6: different len
   rc = ossGetTZEnv( tz.pName, 3 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;

   rc = ossGetTZEnv( tz.pName, 14 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;
}

TEST_F( ossTimeZoneTest, ossGetSysTimeZoneTest )
{
   INT32 rc = SDB_OK ;
   string value ;

   // case 1: null
   rc = ossGetSysTimeZone( NULL, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 2: error len
   rc = ossGetSysTimeZone( tz.pName, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 3: normal value. Make sure time zone is Asia/Shanghai
   rc = ossGetSysTimeZone( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;

   // case 4: different len
   rc = ossGetSysTimeZone( tz.pName, 3 ) ;
   ASSERT_EQ( SDB_INVALIDSIZE, rc ) ;

   rc = ossGetSysTimeZone( tz.pName, 14 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;
}

void *getTZEnvByThread( void *args )
{
   timeZone * tz = ( timeZone * )args ;

   ossGetTZEnv( tz->pName, tz->len ) ;
   return 0 ;
}

TEST_F( ossTimeZoneTest, ossInitTZEnvTest )
{
   INT32 rc  = SDB_OK ;
   INT32 ret = 0 ;
   pthread_t tid ;
   string value ;

   // case 1: TZ no exists. Make sure time zone is Asia/Shanghai
   ret = unsetenv( "TZ" ) ;
   ASSERT_EQ( 0, ret ) << "Failed to unset TZ, erron = " << errno ;

   rc = ossInitTZEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = ossGetTZEnv( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;

   // case 2: TZ is UTC
   ret = setenv( "TZ", "UTC", 1 ) ;
   ASSERT_EQ( 0, ret ) << "Failed to set TZ as UTC, erron = " << errno ;

   rc = ossInitTZEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = ossGetTZEnv( tz.pName, tz.len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   value = tz.pName ;
   ASSERT_EQ( "UTC", value ) ;

   // case 3: check TZ in the child thread
   ret = setenv( "TZ", "Asia/Shanghai", 1 ) ;
   ASSERT_EQ( 0, ret ) << "Failed to set TZ as Asia/Shanghai, erron = " << errno ;

   ret = pthread_create( &tid, NULL, getTZEnvByThread, &tz ) ;
   ASSERT_EQ( 0, ret ) ;

   ret = pthread_join( tid, NULL ) ;
   ASSERT_EQ( 0, ret ) ;
   value = tz.pName ;
   ASSERT_EQ( "Asia/Shanghai", value ) ;
}

#endif  // _LINUX