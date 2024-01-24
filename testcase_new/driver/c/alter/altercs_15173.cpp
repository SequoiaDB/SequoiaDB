/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-15173, seqDB-15172:使用sdbGetQueryMeta获取查询元数据     
 * @Modify:        wenjing wang Init
 *                 2018-04-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char *DOMAIN = "Domain";
const char *NAME = "name" ;
const char *AUTOSPLIT = "AutoSplit" ;
const char *GROUPS = "Groups" ;

class alterCSTest : public testBase
{
protected:
   void SetUp()
   {
      sdbDomainHandle domain = SDB_INVALID_HANDLE ;
      INT32 rc = SDB_OK ;
      bson opt ;
      std::vector< std::string> groupNames ;
      testBase::SetUp() ;
      csName = "altercs_15172" ;
      domainName = "altercs_15172" ;
      
      cs = SDB_INVALID_HANDLE ;
      rc = sdbGetCollectionSpace( db, csName.c_str(), &cs) ;
      if ( rc == -34 )
      {
         rc = sdbCreateCollectionSpaceV2( db, csName.c_str(), NULL, &cs );
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get cs " << csName ;
      if ( isStandalone( db ) )
      {
         goto done ;
      }
      
      rc = getGroups( db, groupNames ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get all data groupnames " ;
      
      bson_init( &opt ) ;
      bson_append_bool( &opt, AUTOSPLIT, 0 ) ;
      bson_append_start_array( &opt, GROUPS ) ;
      for ( int  i = 0; i < groupNames.size(); ++i )
      {
         char seq[2] = {0};
         snprintf(seq, sizeof(seq), "%d", i );
         bson_append_string( &opt, seq, groupNames[i].c_str() ) ;
      }
      bson_append_finish_array( &opt ) ;
      bson_finish( &opt ) ;
      
      rc = sdbGetDomain( db, domainName.c_str() , &domain ) ;
      if ( rc == -214 )
      {
         rc = sdbCreateDomain( db, domainName.c_str(), &opt, &domain ) ;
         bson_destroy( &opt ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create domain " << domainName ;
      }
     
done: 
      if ( SDB_INVALID_HANDLE != domain )
      {
         sdbReleaseDomain( domain );
      }
   }
   
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() && cs != SDB_INVALID_HANDLE )
      {
         rc = sdbDropCollectionSpace( db, csName.c_str() ) ; 
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
         if ( !isStandalone( db ) )
         {
            rc = sdbDropDomain( db, "altercs_15172" ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName ;
         }        
         sdbReleaseCS( cs ) ;
      } 
      testBase::TearDown() ;
   }
protected:
   BOOLEAN checkCSInDomain();
   INT32 getCSType() ;
   sdbCSHandle cs ;
   std::string csName ; 
   std::string domainName ;  
};

BOOLEAN alterCSTest::checkCSInDomain()
{
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   INT32 rc = SDB_OK ;
   bson ret ;
   bson_init( &ret ) ;
   bson_finish( &ret ) ;
   rc = sdbGetDomain( db, domainName.c_str(), &domain ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "sdbGetDomain (" << domainName 
                << ")" << "failed, rc=" << rc ;
      goto err ;
   }
   
   rc = sdbListCollectionSpacesInDomain( domain, &cursor ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "sdbListCollectionSpacesInDomain (" << domainName 
                << ")" << "failed, rc=" << rc ;
      goto err ;
   }
   
   rc = SDB_DMS_EOC ;
   while ( SDB_DMS_EOC != sdbNext( cursor, &ret ) )
   {
      bson_iterator it ;
      rc = SDB_DMS_EOC ;
      bson_type type = bson_find( &it, &ret, "Name" ) ;
      if ( csName == bson_iterator_string ( &it ) )
      {
         rc = SDB_OK ;
         break ;
      }
      bson_destroy( &ret ) ;
      bson_init( &ret ) ;
   }
done:
   bson_destroy( &ret ) ;
   if ( domain != SDB_INVALID_HANDLE ) 
   {
      sdbReleaseDomain( domain ) ;
   }
   
   if ( cursor != SDB_INVALID_HANDLE ) 
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
   return rc == SDB_OK ;
err:
   goto done;
}

INT32 alterCSTest::getCSType() 
{
   INT32 rc = SDB_OK ;
   sdbReplicaGroupHandle group = SDB_INVALID_HANDLE ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   sdbConnectionHandle sdb = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   const char *hostName = NULL ;
   const char *svcName = NULL ;
   int csType = -1 ;
   bson cond, ret;
   bson_init( &ret );
   bson_init( &cond );
   bson_append_string( &cond, "Name", csName.c_str() );
   bson_finish( &cond );
   
   rc = sdbGetReplicaGroup1( db, 1, &group ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbGetReplicaGroup1 rc = " <<  rc << std::endl;
      goto err;
   }
   rc = sdbGetNodeMaster( group, &node );
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbGetNodeMaster rc = " <<  rc << std::endl;
      goto err;
   }
   
   sdbGetNodeAddr( node, &hostName, &svcName, NULL, NULL ) ;
   rc = sdbConnect( hostName, svcName, ARGS->user(), ARGS->passwd(), &sdb ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbConnect(" << hostName << "," 
                << svcName << ") rc = " <<  rc << std::endl;
      goto err;
   }
   rc = sdbGetCollection( sdb, "SYSCAT.SYSCOLLECTIONSPACES" , &cl ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbGetCollection(\"SYSCAT.SYSCOLLECTIONSPACES\"),rc = " 
                <<  rc << std::endl;
      goto err;
   }
   
   rc = sdbQuery( cl, &cond, NULL, NULL, NULL, 0, -1, &cursor );
   if ( rc != SDB_OK )
   {
      std::cout << "fail to sdbQuery,rc = " 
                <<  rc << std::endl;
      goto err;
   }
   
   while ( SDB_DMS_EOC != sdbNext( cursor, &ret ) )
   {
      bson_iterator it ;
      bson_type type = bson_find( &it, &ret, "Type" ) ;
      if ( type == BSON_INT )
      {
         csType = bson_iterator_int( &it ) ;
         break;
      }
      bson_destroy( &ret ) ;
      bson_init( &ret ) ;
   }
done:   
   return csType ;
err:
   bson_destroy( &ret ) ;
   bson_destroy( &cond ) ;
   if ( node != SDB_INVALID_HANDLE ) 
   {
      sdbReleaseNode( node ) ;
   }
   
   if ( group != SDB_INVALID_HANDLE ) 
   {
      sdbReleaseReplicaGroup( group ) ;
   }
   
   if ( cl != SDB_INVALID_HANDLE ) 
   {
      sdbReleaseCollection( cl ) ;
   }
   
   if ( cursor != SDB_INVALID_HANDLE ) 
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
   
   if ( sdb != SDB_INVALID_HANDLE )
   {
      sdbDisconnect( sdb ) ;
      sdbReleaseConnection( sdb ) ;
   }
}

TEST_F( alterCSTest, SetDomain )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   bson opt ;
   bson_init( &opt );
   bson_append_string( &opt, DOMAIN, domainName.c_str() );
   bson_finish( &opt );
   
   rc = sdbCSSetDomain( cs, &opt ) ;
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSSetDomain " << csName ;
  
   BOOLEAN ret = checkCSInDomain() ;
   ASSERT_EQ( ret, TRUE ) << "fail to sdbCSSetDomain " << csName ;
   
   rc = sdbCSRemoveDomain( cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSRemoveDomain " << csName ;
   
   ret = checkCSInDomain() ;
   ASSERT_EQ( ret, FALSE ) << "fail to sdbCSSetDomain " << csName ;
}

TEST_F( alterCSTest, EnableCapped  )
{
   INT32 rc = SDB_OK ;
   
   if ( isStandalone( db ) )
   {
      return ;
   }
   rc = sdbCSEnableCapped( cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSEnableCapped " << csName ;
  
   int type = getCSType() ;
   ASSERT_EQ( type, 1 ) << "fail to sdbCSEnableCapped " << csName ;
   
   rc = sdbCSDisableCapped( cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSDisableCapped " << csName ;
   
   type = getCSType() ;
   ASSERT_EQ( type, 0 ) << "fail to sdbCSSetDomain " << csName ;
}

TEST_F( alterCSTest, SetAttributes  )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   bson opt ;
   bson_init( &opt );
   bson_append_string( &opt, DOMAIN, domainName.c_str() );
   bson_finish( &opt );
   
   rc = sdbCSSetAttributes( cs, &opt ) ;
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSSetDomain " << csName ;
  
   BOOLEAN ret = checkCSInDomain() ;
   ASSERT_EQ( ret, TRUE ) << "fail to sdbCSSetDomain " << csName ;
  
}

TEST_F( alterCSTest, alter  )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   bson opt ;
   bson_init( &opt );
   bson_append_string( &opt, DOMAIN, domainName.c_str() );
   bson_finish( &opt );
   
   rc = sdbAlterCollectionSpace( cs, &opt ) ;
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbCSSetDomain " << csName ;
  
   BOOLEAN ret = checkCSInDomain() ;
   ASSERT_EQ( ret, TRUE ) << "fail to sdbCSSetDomain " << csName ;
}
