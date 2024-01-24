/*************************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1068
 *               SEQUOIADBMAINSTREAM-3133
 *               seqDB-12219:构造sdbDate数据并使用toString方法转为字符串
 *               seqDB-12220:构造Timestamp数据并使用toString方法转为字符串
 *               seqDB-14702:接口Date_t.toString()测试
 *               seqDB-14703:接口BSONObjBuilder::appendDate校验
 *               seqDB-14704:接口BSONObjBuilder::appendTimestamp校验
 * @Modify:      Liang xuewang Init
 *				     2017-07-20
 *************************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "testcommon.hpp" 

using namespace sdbclient ;
using namespace bson ;
using namespace std ;


BOOLEAN setTzEnv()
{
   INT32 rc = SDB_OK ;
   const char* envtz = "TZ" ;
   const char* UTC = "UTC" ;
   char *tz = getenv(envtz);
	
   if ( tz != NULL && strcmp(tz,UTC) !=0 )
   {
      return TRUE ;
   }
   else
   {
      rc = setenv(envtz, UTC, 1);
   }
   
   return rc == SDB_OK ;
}

// test appendDate(const StringData& fieldName, Date_t dt)
TEST( dateTest12219, appendDate )
{
   SINT64 mills[] = {
      -62167248352000,   // 0000-01-01 00:00:00
      253402271999000,   // 9999-12-31 23:59:59
      -62167248353000,   // < 0000-01-01 00:00:00 
      253402272000000,   // > 9999-12-31 23:59:59
      -9223372036854775808,  // -2^63
      9223372036854775807    // 2^63-1
   } ;
   INT32 size = sizeof( mills ) / sizeof( mills[0] ) ;

   if (!setTzEnv())
   {
      return ;
   }
   BSONObjBuilder builder ;
   for( INT32 i = 0;i < size;i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "date", i ) ;
      Date_t dt( mills[i] ) ;
      builder.appendDate( key, dt ) ;
   }
   string real = builder.obj().toString() ;

   // 夏令时导致可能出现两种结果
   string expect1 = "{ \"date0\": {\"$date\": \"0000-01-01\"}, "
                      "\"date1\": {\"$date\": \"9999-12-31\"}, "
                      "\"date2\": { \"$date\": -62167248353000 }, "
                      "\"date3\": { \"$date\": 253402272000000 }, "
                      "\"date4\": { \"$date\": -9223372036854775808 }, "
                      "\"date5\": { \"$date\": 9223372036854775807 } }" ;
   string expect2 = "{ \"date0\": { \"$date\": -62167248352000 }, "
                    "\"date1\": {\"$date\": \"9999-12-31\"}, "
                    "\"date2\": { \"$date\": -62167248353000 }, "
                    "\"date3\": {\"$date\": \"9999-12-31\"}, "
                    "\"date4\": { \"$date\": -9223372036854775808 }, "
                    "\"date5\": { \"$date\": 9223372036854775807 } }";
   
   ASSERT_TRUE( ( expect1 == real ) || ( expect2 == real ) ) 
                << "fail to check date\n expect1 = " << expect1 
                << "\n expect2 = " << expect2 << "\n real = " << real ; 
}

// test appendTimestamp( const StringData& fieldName , long long time , unsigned int inc )
TEST( timestampTest12220, appendTimestamp )
{
   SINT64 secs[] = {
      -2145945600,   	// 1902-01-01 00:00:00.000000
      -1325491200,      // 1928-01-01 00:00:00.000000
      2145887999     	// 2037-12-31 23:59:59.999999
   } ;
   INT32 micros[] = {
      0,  			   // micro seconds for 1902-01-01 00:00:00.000000
      0,             // micro seconds for 1928-01-01 00:00:00.000000
      999999 			// micro seconds for 2037-12-31 23:59:59.999999
   } ;

   if (!setTzEnv())
   {
      return ;
   }
   INT32 size = sizeof(secs) / sizeof(secs[0]) ;
   BSONObjBuilder builder ;
   for( INT32 i = 0;i < size;i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "time", i ) ;
      builder.appendTimestamp( key, secs[i]*1000, micros[i] ) ;
   }
   string real = builder.obj().toString() ;

   // 夏令时导致可能出现两种结果
   string expect1 = "{ \"time0\": {\"$timestamp\": \"1902-01-01-00.05.52.000000\"}, "
                      "\"time1\": {\"$timestamp\": \"1928-01-01-00.00.00.000000\"}, "
                      "\"time2\": {\"$timestamp\": \"2037-12-31-23.59.59.999999\"} }" ;
   /*string expect2 = "{ \"time0\": {\"$timestamp\": \"1902-01-01-00.00.00.000000\"}, "
                      "\"time1\": {\"$timestamp\": \"1928-01-01-00.00.00.000000\"}, "
                      "\"time2\": {\"$timestamp\": \"2037-12-31-23.59.59.999999\"} }" ;*/
   string expect2 = "{ \"time0\": {\"$timestamp\": \"1901-12-31-16.00.00.000000\"}, "
                     "\"time1\": {\"$timestamp\": \"1927-12-31-16.00.00.000000\"}, "
		     "\"time2\": {\"$timestamp\": \"2037-12-31-15.59.59.999999\"} }" ;
   ASSERT_TRUE( ( expect1 == real ) || ( expect2 == real ) ) 
                << "fail to check timestamp\n expect1 = " << expect1 
                << "\n expect2 = " << expect2 << "\n real = " << real ;
}

// test Date_t.toString()
TEST( dateTest14702, toString )
{
   SINT64 mills[] = {
      -62167248352000,    // 0000-01-01 00:00:00
      253402271999000,    // 9999-12-31 23:59:59
   } ;
   string dates[] = {
      "Fri Dec 31 15:54:08 -1",
      "Fri Dec 31 15:59:59 9999"
   } ;
   
   if (!setTzEnv())
   {
      return ;
   }
   INT32 size = sizeof( mills ) / sizeof( mills[0] ) ;
   for( INT32 i = 0;i < size;i++ )
   {
      Date_t dt( mills[i] ) ;
      ASSERT_EQ( dates[i], dt.toString() ) << "fail to check Date_t" ;
   }

   Date_t dt ;
   string begin = "Thu Jan  1 00:00:00 1970" ;
   ASSERT_EQ( begin, dt.toString() ) << "fail to check Date_t" ;
}

// test append(const StringData& fieldName, Date_t dt)
TEST( dateTest14703, append )
{
   SINT64 mills[] = {
      -62167248352000,    // 0000-01-01 00:00:00
      253402271999000,    // 9999-12-31 23:59:59
   } ;
   string dates1[] = {
      "{ \"date\": {\"$date\": \"0000-01-01\"} }",
      "{ \"date\": {\"$date\": \"9999-12-31\"} }"
   } ;
   string dates2[] = {
      "{ \"date\": { \"$date\": -62167248352000 } }",
      "{ \"date\": {\"$date\": \"9999-12-31\"} }"
   } ;

   if (!setTzEnv())
   {
      return ;
   }
   INT32 size = sizeof( mills ) / sizeof( mills[0] ) ;
   for( INT32 i = 0;i < size;i++ )
   {
      Date_t dt( mills[i] ) ;
      BSONObjBuilder builder ;
      builder.append( "date", dt ) ;
      string str = builder.obj().toString() ;
      ASSERT_TRUE( ( dates1[i] == str ) || ( dates2[i] == str ) )
         << "fail to check date " << str ;
   }

   Date_t dt ;
   BSONObjBuilder builder ;
   builder.append( "date", dt ) ;
   string str = builder.obj().toString() ;
   string date = "{ \"date\": {\"$date\": \"1970-01-01\"} }" ;
   ASSERT_EQ( date, str ) << "fail to check date" ;
}

// test appendTimestamp( const StringData& fieldName )
// test appendTimestamp( const StringData& fieldName , long long val )
TEST( timestampTest14704, appendTimestamp )
{
   BSONObjBuilder builder ;
   builder.appendTimestamp( "time" ) ;
   string real = builder.obj().toString() ;
   string expect = "{ \"time\": {\"$timestamp\": \"1970-01-01-00.00.00.000000\"} }" ;
   ASSERT_EQ( expect, real ) << "fail to check timestamp" ;

   SINT64 secs[] = {
      -2145945600,   	// 1902-01-01 00:00:00.000000
      2145887999     	// 2037-12-31 23:59:59.999999
   } ;
   INT32 micros[] = {
      0,  			   // micro seconds for 1902-01-01 00:00:00.000000
      999999 			// micro seconds for 2037-12-31 23:59:59.999999
   } ;
   
   if (!setTzEnv())
   {
      return ;
   }
   INT32 size = sizeof( secs ) / sizeof( secs[0] ) ;

   // 夏令时导致可能出现两种结果
   string expect1[] = {
      "{ \"time\": {\"$timestamp\": \"1902-01-01-00.05.52.000000\"} }",
      "{ \"time\": {\"$timestamp\": \"2037-12-31-23.59.59.999999\"} }"
   } ;
   string expect2[] = {
      "{ \"time\": {\"$timestamp\": \"1901-12-31-16.00.00.000000\"} }",
      "{ \"time\": {\"$timestamp\": \"2037-12-31-15.59.59.999999\"} }" 
   } ;
   
   for( INT32 i = 0;i < size;i++ )
   {
      OpTime t( secs[i], micros[i] ) ;
      BSONObjBuilder builder ;
      builder.appendTimestamp( "time", t.asDate() ) ;
      string real = builder.obj().toString() ;
      ASSERT_TRUE( ( expect1[i] == real ) || ( expect2[i] == real ) )
         << "fail to check timestamp " << real ;
   }
}
