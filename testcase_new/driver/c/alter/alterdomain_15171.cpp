/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-15171, seqDB-15223:使用sdbGetQueryMeta获取查询元数据     
 * @Modify:        wenjing wang Init
 *                 2018-04-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char *AUTOSPLIT = "AutoSplit" ;
const char *GROUPS =  "Groups" ;
const char *NAME = "Name" ;
const char *GROUPNAME = "GroupName" ;
const char *GROUPID = "GroupID" ;
#define GROUPNUM (3)

typedef struct struGroupAttr
{
   char *name ;
   int id ;
   struGroupAttr()
   {
      name = NULL ;
      id = 0 ;
   }
}groupAttr;

typedef struct struDomainAttr{
   char *name;
   BOOLEAN autoSplit;
   groupAttr groups[GROUPNUM];
}domainAttr;

class alterDomainTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      bson opt ;
      domainName = "alterDomain_15173" ;
      testBase::SetUp() ;
      
      domain = SDB_INVALID_HANDLE ;
      if ( isStandalone( db ) )
      {
         goto done ;
      }
      
      rc = getGroups( db, groupNames ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get all data groupnames " ;
      
      bson_init( &opt ) ;
      bson_append_bool( &opt, AUTOSPLIT, 1 ) ;
      bson_finish( &opt ) ;
      
      rc = sdbGetDomain( db, domainName , &domain ) ;
      if ( rc == -214 )
      {
         rc = sdbCreateDomain( db, domainName, &opt, &domain ) ;
         bson_destroy( &opt ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create domain " << domainName ;
      }
   done:
      return ;
   }
   
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( !isStandalone( db ) && shouldClear() )
      {
         rc = sdbDropDomain( db, domainName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName ;
      } 
      testBase::TearDown() ;
   }
protected:
   INT32 getDomainAttr( domainAttr *attr) ;
   INT32 resotreAttr() ;
   void checkResult( int groupnum = 1 ) ;
   sdbDomainHandle domain ;
   const char *domainName ;
   std::vector<std::string> groupNames ;
};

INT32 alterDomainTest::resotreAttr()
{
   INT32 rc = SDB_OK ;
   bson opt ;
   bson_init( &opt ) ;
   bson_append_bool( &opt, AUTOSPLIT, 1 ) ;
   bson_finish( &opt ) ;

   rc = sdbDomainSetAttributes( domain, &opt );
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   return rc ;
}

INT32 alterDomainTest::getDomainAttr( domainAttr *attr )
{
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   INT32 rc = SDB_OK ;
   bson ret ;
   bson_init( &ret ) ;
   bson cond ;
   bson_init( &cond ) ;
   bson_append_string( &cond, NAME, domainName ) ;
   bson_finish( &cond ) ;

   rc = sdbListDomains( db, &cond, NULL, NULL, &cursor );
   while ( rc == SDB_OK && SDB_DMS_EOC != sdbNext( cursor, &ret ) ){
      bson_iterator it;
      bson_type type = bson_find( &it, &ret, NAME );
      if ( type == BSON_STRING )
      {
         attr->name = strdup( bson_iterator_string(&it) ) ;
      }

      type = bson_find( &it, &ret, AUTOSPLIT );
      if ( type == BSON_BOOL )
      {
         attr->autoSplit = bson_iterator_bool( &it ) ;
      }

      type = bson_find( &it, &ret, GROUPS );
      if ( type != BSON_ARRAY )
      {
         goto done ;
      }
      
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      bson_iterator_from_buffer ( &i, groupList ) ;
      while ( bson_iterator_next ( &i ) )
      {
         int index = 0 ;
         bson_iterator sit ;
         bson sub ;
         bson_init( &sub ) ;
         bson_init_finished_data ( &sub,
                                   (CHAR*)bson_iterator_value ( &i ) ) ;
         bson_iterator_init( &sit, &sub ) ;
         while ( BSON_EOO != bson_iterator_more( &sit ) )
         {
            const CHAR *pKey = bson_iterator_key( &sit ) ;
            if ( 0 == strncmp( pKey, GROUPNAME, strlen( GROUPNAME ) ) )
            {
               attr->groups[index].name = strdup(bson_iterator_string( &sit )) ;
            }else if (0 == strncmp( pKey, GROUPID, strlen( GROUPID ) )){
               attr->groups[index].id = bson_iterator_int( &sit ) ;
            }
            bson_iterator_next( &sit ) ;
         }
         bson_destroy( &sub );
         index++;
         if ( index >= GROUPNUM )
         {
            break ;
         } 
      }
   }
   
done:
   bson_destroy( &cond );
   bson_destroy( &ret );
   if ( cursor != SDB_INVALID_HANDLE )
   {
      sdbReleaseCursor( cursor ) ;
   }
   return rc ;
}

void alterDomainTest::checkResult( int groupnum  )
{
   domainAttr attr ;
   INT32 rc = SDB_OK ;
   int i = 0;
   rc = getDomainAttr( &attr ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to listDomain" ;
   ASSERT_EQ( TRUE, attr.autoSplit ) << "fail to set AutoSplit" ;
   for (; i < groupnum; ++i )
   {
      ASSERT_EQ( groupNames[i], attr.groups[i].name ) << "fail to set Groups" ;
   }
   for ( ; i < GROUPNUM; ++i )
   {
      ASSERT_EQ( NULL, attr.groups[i].name ) << "fail to set Groups" ;
   }
}

TEST_F( alterDomainTest, SetAttributes )
{
   INT32 rc = SDB_OK ;
   bson opt ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;      
   }
   
   bson_init( &opt ) ;
   bson_append_bool( &opt, AUTOSPLIT, 1 ) ;
   bson_append_start_array( &opt, GROUPS ) ;
   bson_append_string( &opt, "0", groupNames[0].c_str() ) ;
   bson_append_finish_array( &opt ) ;
   bson_finish( &opt ) ;

   rc = sdbDomainSetAttributes( domain, &opt );
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDomainSetAttributes " 
         << domainName  ;
   
   checkResult();
  
   rc = resotreAttr() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to resotreAttr" ;

}

TEST_F( alterDomainTest, SetGroups )
{
   bson opt ;
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;      
   }
   
   bson_init( &opt );
   bson_append_start_array( &opt, GROUPS ) ;
   bson_append_string( &opt, "0", groupNames[0].c_str() ) ;
   bson_append_finish_array( &opt ) ;
   bson_finish( &opt );  
   rc = sdbDomainSetGroups(domain, &opt);
   bson_print( &opt ) ;
   bson_destroy( &opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDomainSetGroups " 
         << domainName  ;
         
   checkResult();
  
   rc = resotreAttr() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to resotreAttr" ;
}

TEST_F( alterDomainTest, AddGroups )
{
   bson opt ;
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;      
   }
   
   bson_init( &opt );
   bson_append_start_array( &opt, GROUPS ) ;
   bson_append_string( &opt, "0", groupNames[0].c_str() ) ;
   bson_append_finish_array( &opt ) ;
   bson_finish( &opt );  
   rc = sdbDomainAddGroups( domain, &opt );
   bson_print( &opt ) ;
   
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDomainSetGroups " 
         << domainName  ;
         
   checkResult();
   
   rc = sdbDomainRemoveGroups( domain, &opt );
   bson_print( &opt ) ;
   
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDomainRemoveGroups " 
         << domainName  ;
   checkResult( 0 );
   
   rc = resotreAttr() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to resotreAttr" ;
}


TEST_F( alterDomainTest, sdbAlterDomain )
{
   bson opt ;
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;      
   }
   
   bson_init( &opt );
   bson_append_start_array( &opt, GROUPS ) ;
   bson_append_string( &opt, "0", groupNames[0].c_str() ) ;
   bson_append_finish_array( &opt ) ;
   bson_finish( &opt );  
   rc = sdbAlterDomain( domain, &opt );
   bson_print( &opt ) ;
   
   ASSERT_EQ( SDB_OK, rc ) << "fail to sdbDomainSetGroups " 
         << domainName  ;
         
   checkResult();
  
   rc = resotreAttr() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to resotreAttr" ;
}
