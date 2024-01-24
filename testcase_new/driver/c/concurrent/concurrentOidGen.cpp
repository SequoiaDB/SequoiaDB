/*************************************************************
 * @Description: test case for c driver
 *               concurrent test bson_oid_gen
 * @Modify:      wenjing wang Init
 *				     2018-03-10
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum ( 10 )
#define LEN_OID ( 12 )
#define LEN_PART1_OID ( 4 )
#define LEN_PART_MACHINE ( 3 )
#define LEN_PART_PID ( 2 )
#define LEN_PART2_OID ( LEN_PART_MACHINE + LEN_PART_PID )
#define LEN_PART3_OID ( 3 )

bson_oid_t g_oids[ ThreadNum ] ;
class ThreadArg : public WorkerArgs
{
public:
   INT32 index ;
} ;

void thrdFunc( ThreadArg *arg )
{
   INT32 index = arg->index ;
   bson_oid_t oid ;
   bson_oid_gen( &oid );
   g_oids[index] = oid ;
}

class concurrentOidGenTest : public testBase
{
protected:
   Worker* _workers[ ThreadNum ] ;
   ThreadArg _args[ ThreadNum ] ;
   
   void SetUp()
   {
//      testBase::SetUp() ;
      for( INT32 i = 0; i < ThreadNum; ++i )
      {
         _args[i].index = i ;
         _workers[i] = new Worker( (WorkerRoutine)thrdFunc, &_args[i], false ) ;
      }
   }
   
   void TearDown()
   {
      INT32 rc = SDB_OK ;
  //    testBase::TearDown() ;
      for( INT32 i = 0; i < ThreadNum; ++i )
      {
         delete _workers[i] ;
      }
   }
} ;

TEST_F( concurrentOidGenTest, collectionspace )
{
   INT32 rc = SDB_OK ;
   bson_oid_t prevOid ;
   INT32 oidPos[10] = {0};
   BOOLEAN isFirst = TRUE ;
   for( INT32 i = 0; i < ThreadNum; ++i )
   {
      _workers[i]->start() ;
   }
   
   for( INT32 i = 0; i < ThreadNum; ++i )
   {
      _workers[i]->waitStop();
   }
   
   for ( INT32 i = 0; i < ThreadNum; ++i )
   {
      char *orig = (char*)&g_oids[i]  ;
      if ( isFirst )
      {
         prevOid = g_oids[i] ;
         isFirst = FALSE ;
      }
      else
      {
         char *dest = orig + LEN_PART1_OID ;
         char *src = (char*)&prevOid + LEN_PART1_OID ;
         ASSERT_EQ( memcmp( src, dest , LEN_PART2_OID ), 0 ) << "oid part2 is not equal" ;
      }
      
      SINT16 pid = *( (SINT16*)( orig + LEN_PART1_OID + LEN_PART_MACHINE ) );
      ASSERT_NE( pid, 0 ) << "pid is zero" ;
      int sequence = 0 ;
      char *p = (char*)&sequence ;
      p[0] = *( (char*)orig + LEN_OID - 1 );
      p[1] = *( (char*)orig + LEN_OID - 2 );
      p[2] = *( (char*)orig + LEN_OID - 3 );
      if ( sequence >= 0 && sequence < 10 )
      {
          oidPos[ sequence ] = 1;
      }
      printf("%d\n", sequence);
      char str[25] = {0};
      bson_oid_to_string( &g_oids[i], str);
      printf("%s\n", str);
   }
   
   for ( INT32 i = 0; i < ThreadNum; ++i )
   {
      ASSERT_EQ( oidPos[i], 1 ) << "oid part3 is not sequence" ;
   }
}

