/**************************************************************************
 * @Description:   test case for C driver
 *  
 * @Modify:        wenjing wang Init
 *                 2018-09-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
#include <map>
#include <string>

class snapshotTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
      bson retObj ;
      testBase::SetUp() ;
      bson_init( &retObj ) ;
      isStandAlone = FALSE ;
      rc = sdbGetList(db, SDB_LIST_GROUPS, NULL, NULL, NULL, &cursor ) ;
      if ( rc == -159 )
      {
         isStandAlone = TRUE ;
         return ;
      } 
      
      ASSERT_EQ( SDB_OK, rc ) << "Get replica groups list failed, rc = "<< rc ;
      while( (rc = sdbNext(cursor, &retObj )) == SDB_OK &&
             rc != SDB_DMS_EOC )
      {
         bson_iterator it, sub ;
         bson_find( &it, &retObj, "Group" ) ;
         bson_iterator_subiterator( &it, &sub ) ;
         bson_iterator_next( &sub ) ;
         while( bson_iterator_more( &sub ) )
         {
            bson tmp1 ;
            bson_init( &tmp1 ) ;
            bson_iterator_subobject( &sub, &tmp1 ) ;
            bson_iterator i1 ;
            bson_find( &i1, &tmp1, "HostName" ) ;
            std::string hostName = bson_iterator_string( &i1 ) ;
            
            bson_find( &i1, &tmp1, "Service" ) ;
            bson tmp2 ;
            bson_init( &tmp2 ) ;
            bson_iterator_subobject( &i1, &tmp2 ) ;
            bson_iterator i2 ;
            bson_iterator_init( &i2, &tmp2 ) ;
            bson_iterator_next( &i2 ) ;
            bson tmp3 ;
            bson_init( &tmp3 ) ;
            bson_iterator_subobject( &i2, &tmp3 ) ;
            bson_iterator i3 ;
            bson_find( &i3, &tmp3, "Name" ) ;
            std::string svcName = bson_iterator_string( &i3 ) ;
            
            bson_destroy( &tmp3 ) ;
            bson_destroy( &tmp2 ) ;
            bson_destroy( &tmp1 ) ;
            bson_iterator_next( &sub ) ;
            hostName2Svcname.insert( std::pair<std::string,std::string>(hostName, svcName ));
         }
         bson_destroy( &retObj ) ;
      }
      
      if ( cursor != SDB_INVALID_HANDLE )
      {
         sdbCloseCursor( cursor ) ;
         sdbReleaseCursor( cursor ) ;
      }
   }
   
   void TearDown()
   {
      testBase::TearDown() ;
   }
protected:
   std::map<std::string, std::string> hostName2Svcname ;
   BOOLEAN isStandAlone ; 
};

TEST_F( snapshotTest, validParameters )
{
 
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;  
   bson cond, sel, orderBy, hint, ret; 
   
   if ( isStandAlone ) return ;
   if ( hostName2Svcname.empty() ) return ;
   std::map<std::string, std::string>::const_iterator iter = hostName2Svcname.begin();
   
   bson_init( &cond );
   bson_append_string( &cond, "svcname", iter->second.c_str() );
   bson_finish( &cond );
   
   bson_init( &sel );
   bson_append_string( &sel, "NodeName", "" );
   bson_finish( &sel );
   
   bson_init( &orderBy );
   bson_append_int( &orderBy, "NodeName", 1 );
   bson_finish( &orderBy ) ;
   
   bson_init( &hint );
   bson_append_start_object( &hint, "$Options" ) ;
   bson_append_string( &hint, "mode", "local" );
   bson_append_finish_object( &hint );
   bson_finish( &hint );
   
   rc = sdbGetSnapshot1( db, SDB_SNAP_CONFIGS, &cond, &sel, &orderBy, &hint ,0, -1, &cursor);
   bson_destroy( &cond );
   bson_destroy( &sel );
   bson_destroy( &orderBy );
   bson_destroy( &hint ) ;
   ASSERT_EQ( rc, SDB_OK ) << " Get snapshot of node configurations failed, ret=" << rc ;
   
   bson_init( &ret );
   std::string prevNodeName ;
   while( (rc = sdbNext( cursor, &ret )) == SDB_OK &&
          rc != SDB_DMS_EOC )
   {
      bson_iterator it ;
      bson_find( &it, &ret, "NodeName" ) ;
     
      std::string curNodeName = bson_iterator_string( &it );
      if ( prevNodeName.empty() )
      {
         prevNodeName = curNodeName ;
      }
      else
      {
         ASSERT_GT(curNodeName, prevNodeName) << prevNodeName << ">" << curNodeName ;
         prevNodeName = curNodeName ;
      }

      std::string::size_type pos = curNodeName.find(":");
      std::string hostName = curNodeName.substr(0, pos );      
      std::map<std::string, std::string>::const_iterator mit = hostName2Svcname.find( hostName ) ;

      ASSERT_EQ( mit->second, iter->second ) << mit->second<< "not equal" << iter->second ;
      
   }
   bson_destroy( &ret );
   
   if ( cursor != SDB_INVALID_HANDLE )
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
     

}
