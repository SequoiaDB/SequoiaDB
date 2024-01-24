/***************************************************
 * @Description : test getRGName 
 *                seqDB-22056: 获取分区组名称 
 * @Modify      : liuxiaoxuan
 *                2020-04-08
 ***************************************************/

#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class replicaGroupTest22056 : public testBase 
{
   protected:
      const CHAR *pGroupName ;
      sdbReplicaGroupHandle rg ;
   
      void SetUp()  
      {
         testBase::SetUp() ;
         INT32 rc = SDB_OK ;
         if( isStandalone( db ) )
         {
            printf( "Run mode is standalone\n" ) ;
            return ;
         }

         // create rg
         pGroupName = "testGroupInCpp" ;
         rc = sdbCreateReplicaGroup( db, pGroupName, &rg ) ;
         ASSERT_EQ( SDB_OK, rc ) << "Error: Failed to create replica group[%s]" << pGroupName ;
      }

      void TearDown()
      {
         INT32 rc = SDB_OK ;
         if ( !isStandalone( db ) )
         {   
            rc = sdbRemoveReplicaGroup( db, pGroupName ) ; 
            ASSERT_EQ( SDB_OK, rc ) << "Error: Failed to remove replica group[%s]" << pGroupName ; 
         }   
         testBase::TearDown() ;
      }  
} ;


TEST_F( replicaGroupTest22056, getRGName )
{

   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   CHAR pBuffer[ NAME_LEN ] = { 0 } ;

   rc = sdbGetRGName( rg, pBuffer, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strncmp( pBuffer, pGroupName, strlen(pGroupName) ) ) ;

   rc = sdbGetRGName( rg, NULL, 1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}
