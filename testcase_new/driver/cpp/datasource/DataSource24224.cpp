/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-24224:创建/修改/获取/列举/删除数据源 
                   seqDB-24225:createDataSource参数校验
                   seqDB-24228:getDataSource参数校验
                   seqDB-24229:dropDataSource参数校验
 * @Author:        liuxiaoxuan
 *                 2021-05-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

TEST( datasourceTest, ds_24224 )
{
   INT32 rc = SDB_OK ;

   sdb db ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   if ( isStandalone( db ) )
   {
      return ;
   }

   const char *dataSourceName = "ds_24224" ;
   const char *addr = ARGS->dsCoordUrl() ;

   // create datasource
   sdbDataSource datasource ;
   rc = db.createDataSource( datasource, dataSourceName, addr ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // list datasources
   sdbCursor cursor ;
   BSONObj cond = BSON( "Name" << dataSourceName ) ;
   rc = db.listDataSources( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 

   BSONObj obj ;
   SINT64 count = 0 ;
   while ( cursor.next(obj) == 0 ) 
   { 
      count ++;
      ASSERT_EQ( addr, obj.getField( "Address" ).String() ) << "actual obj: " << obj.toString() ;  
      ASSERT_EQ( "READ|WRITE", obj.getField( "AccessModeDesc" ).String() ) << "actual obj: " << obj.toString() ;  
   }   
   cursor.close() ;
   ASSERT_EQ( 1, count ) ;

   // get name of datasource
   const char *expectDSName = dataSourceName ;
   const char *actualDSName = datasource.getName() ;
   ASSERT_STREQ( expectDSName, actualDSName ) ;

   // alter datasource
   BSONObj option = BSON( "AccessMode" << "READ" ) ;
   rc = datasource.alterDataSource( option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get dataSource
   sdbDataSource datasource1 ;
   rc = db.getDataSource( dataSourceName, datasource1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cond = BSON( "Name" << datasource1.getName() ) ; 
   rc = db.listDataSources( cursor, cond ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   count = 0 ; 
   while ( cursor.next(obj) == 0 ) 
   {   
      count ++; 
      ASSERT_EQ( "READ", obj.getField( "AccessModeDesc" ).String() ) << "actual obj: " << obj.toString() ;   
   }   
   cursor.close() ;
   ASSERT_EQ( 1, count ) ; 
                                                      
   // drop datasource
   rc = db.dropDataSource( dataSourceName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get datasource
   rc = db.getDataSource( dataSourceName, datasource1 ) ;
   ASSERT_EQ( SDB_CAT_DATASOURCE_NOTEXIST, rc ) ;

   db.disconnect() ;
}

TEST( datasourceTest, ds_24225_24228_24229 )
{
   INT32 rc = SDB_OK ;
   sdb db ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 

   if ( isStandalone( db ) )
   {
      return ;
   }

   const char *dataSourceName = "ds_24225_24228_24229" ;
   const char *addr = ARGS->dsCoordUrl() ;

   // create datasource
   sdbDataSource datasource ;
   rc = db.createDataSource( datasource, NULL, addr ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ; 
   rc = db.createDataSource( datasource, dataSourceName, NULL ) ;            
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.createDataSource( datasource, dataSourceName, "sdbserver1" ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // get datasource
   rc = db.getDataSource( NULL, datasource ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // drop datasource
   rc = db.dropDataSource( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   db.disconnect() ;
}
