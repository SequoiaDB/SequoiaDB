/***************************************************
 * @Description : test domain  
 *                seqDB-10401: 创建/获取/删除domain 
 * @Modify      : liuxiaoxuan
 *                2020-04-08
 ***************************************************/

#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class domain10401: public testBase
{
protected:
    const CHAR *pDomainName ; 

    void SetUp()  
    {
       testBase::SetUp() ;
    }
    void TearDown()
    {
       testBase::TearDown() ;
    }
} ;

TEST_F( domain10401, domain )
{
   if( isStandalone( db ) ) 
   {   
      printf( "Run mode is standalone\n" ) ; 
      return ;
   }   

   INT32 rc                       = SDB_OK ;
   pDomainName  = "DomainNameNormalRunAll10401";
   sdbDomainHandle dom            = 0 ;
   sdbCSHandle cs                 = 0 ; 
   sdbCollectionHandle cl         = 0 ; 
   sdbCursorHandle cursor         = 0 ;
   const CHAR* csName             = "domain_collectionspace_10401" ;
   const CHAR* clName             = "domain_collection_10401" ;
   CHAR  bson_itValue[5][1000] ;
   bson  domObj ;
   bson  altObj ;
   bson matcher ;
   bson obj ;
   bson csOption ;
   bson_iterator it ;

   // Domain option bson
   bson_init( &domObj ) ;
   bson_append_bool( &domObj, "AutoSplit", true ) ;
   bson_finish( &domObj ) ;

   // Create domain
   rc = sdbCreateDomain( db, pDomainName, &domObj, &dom ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create domain, rc = " << rc ;

   // List domains
   bson_init( &obj ) ;
   bson_init( &matcher ) ; 
   bson_append_string( &matcher, "Name", pDomainName ) ; 
   bson_finish( &matcher ) ; 
   rc = sdbListDomains( db, &matcher, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to list domains, rc = " << rc ;
   // Check list result
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   ASSERT_TRUE( bson_find( &it, &obj, "AutoSplit" ) ) ;
   bson_destroy( &obj ) ;
   sdbCloseCursor ( cursor ) ;

   // Get domain
   rc = sdbGetDomain( db, pDomainName, &dom ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to get domain, rc = " << rc ;

   // Alter domain
   std::vector<std::string> groupNames ;
   rc = getGroups( db, groupNames ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get all groups " ;

   bson_init( &altObj ) ;
   bson_append_start_array( &altObj, "Groups" ) ;
   bson_append_string( &altObj, "0", groupNames[0].c_str() ) ; 
   bson_append_finish_array( &altObj ) ;
   bson_finish( &altObj ) ;
   bson_print( &altObj ) ;
   rc = sdbAlterDomain( dom, &altObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to alter domain, rc =" << rc ;
   // Check result after alter domain
   bson_init( &obj ) ; 
   rc = sdbListDomains( db, &matcher, NULL, NULL, &cursor ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "Failed to list domains, rc = " << rc ;
   rc = sdbNext( cursor, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   ASSERT_EQ( BSON_ARRAY, bson_find( &it, &obj, "Groups" ) ) ;
   bson_destroy( &obj ) ; 
   sdbCloseCursor ( cursor ) ; 

   // Create cs and cl within domain
   bson_init( &csOption ) ; 
   bson_append_string( &csOption, "Domain", pDomainName ) ;
   bson_finish( &csOption ) ;
   
   rc = sdbCreateCollectionSpaceV2( db, csName, &csOption, &cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ; 
 
   // List collection space
   rc = sdbListCollectionSpacesInDomain( dom, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to list CS, rc = " << rc ;
   // Check list result
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_print( &obj ) ;
   ASSERT_EQ( BSON_STRING, bson_find( &it, &obj, "Name" ) ) ; 
   bson_destroy( &obj ) ;
   sdbCloseCursor ( cursor ) ;

   // List collection
   rc = sdbListCollectionsInDomain( dom, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to list CL, rc = " << rc ;
   // Check list result
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   bson_print( &obj ) ;
   ASSERT_EQ( BSON_STRING, bson_find( &it, &obj, "Name" ) ) ;
   bson_destroy( &obj ) ;
   sdbCloseCursor ( cursor ) ;

   // Clear the environment
   rc = sdbDropCollectionSpace( db, csName ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   rc = sdbDropDomain( db, pDomainName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to clear domain in SDB, rc = " << rc ;

   bson_destroy( &domObj ) ;
   bson_destroy( &altObj ) ;
   bson_destroy( &matcher ) ;
   bson_destroy( &csOption ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseDomain( dom );
   sdbReleaseCS( cs ) ;
   sdbReleaseCollection( cl ) ;
}
