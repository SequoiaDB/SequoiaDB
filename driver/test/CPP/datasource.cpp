/*******************************************************************************
*@Description : Test data source C++ driver, include createDataSource/dropDataSource /
*               /getDataSource/listDataSources/alterDataSource
*@Modify List :
*               2021-5-11   QinCheng Yang
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST( datasource, createDSTest )
{
   INT32 rc = SDB_OK ;
   sdb db ;
   sdbCursor cursor ;
   sdbDataSource ds ;

   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: create ds with name and address
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropDataSource( DATASOURCENAME );
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 2: create ds with urls
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEURLS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropDataSource( DATASOURCENAME );
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 3: create ds with error type
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS,
                             NULL, NULL, "errorDSType" ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 4: create ds with invalid options
   BSONObjBuilder optBuild4 ;
   optBuild4.append( "a", 1) ;
   BSONObj options4 = optBuild4.obj() ;
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS,
                             NULL, NULL, NULL, &options4 ) ;
   ASSERT_EQ( SDB_OPTION_NOT_SUPPORT, rc ) ;

   // case 5: create ds with valid options
   BSONObjBuilder optBuild5 ;
   optBuild5.append( "AccessMode", "READ") ;
   optBuild5.append( "ErrorFilterMask", "READ") ;
   optBuild5.append( "ErrorControlLevel", "Low") ;
   BSONObj options5 = optBuild5.obj() ;
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS,
                             NULL, NULL, NULL, &options5 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObjBuilder matcher ;
   matcher.append( "Name", DATASOURCENAME ) ;
   rc = db.listDataSources( cursor, matcher.obj() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_STREQ( "READ", obj.getStringField( "AccessModeDesc" ) ) ;
   ASSERT_STREQ( "READ", obj.getStringField( "ErrorFilterMaskDesc" ) ) ;
   ASSERT_STREQ( "Low", obj.getStringField( "ErrorControlLevel" ) ) ;

   rc = db.dropDataSource( DATASOURCENAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   db.disconnect();
}

TEST( datasource, dropDSTest )
{
   INT32 rc               = SDB_OK ;
   sdb db ;

   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR *pDataSourceName = "errorDataSourceName" ;
   rc = db.dropDataSource( pDataSourceName ) ;
   ASSERT_EQ( SDB_CAT_DATASOURCE_NOTEXIST, rc ) ;

   db.disconnect();
}

TEST( datasource, getDSTest )
{
   INT32 rc               = SDB_OK ;
   sdb db ;
   sdbDataSource ds ;

   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: get valid data source
   rc = db.getDataSource( DATASOURCENAME, ds ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 2: get invalid data source
   rc = db.getDataSource( "invalidDSTest", ds ) ;
   ASSERT_EQ( SDB_CAT_DATASOURCE_NOTEXIST, rc ) ;

   // clean
   db.dropDataSource( DATASOURCENAME );
   db.disconnect();
}

TEST( datasource, alterDSTest )
{
   INT32 rc               = SDB_OK ;
   sdb db ;
   sdbCursor cursor ;
   sdbDataSource ds ;

   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.createDataSource( ds, DATASOURCENAME, DATASOURCEADDRESS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // case 1: empty option
   const BSONObj option1;
   rc = ds.alterDataSource( option1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 2: invalid option
   BSONObjBuilder options2 ;
   options2.append( "Alter", 1) ;
   rc = ds.alterDataSource( options2.obj() ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // case 3: valid option
   const CHAR * dsNewName = "dataSourceNewNameC++Test" ;
   BSONObjBuilder options3 ;
   options3.append( "Name", dsNewName ) ;
   options3.append( "AccessMode", "READ" ) ;
   options3.append( "ErrorFilterMask", "READ" ) ;
   options3.append( "ErrorControlLevel", "Low" ) ;
   rc = ds.alterDataSource( options3.obj() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR *dsActualName = ds.getName();
   ASSERT_STREQ( dsNewName, dsActualName ) ;

   BSONObjBuilder matcher ;
   matcher.append( "Name", dsNewName ) ;
   rc = db.listDataSources( cursor, matcher.obj() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_STREQ( "READ", obj.getStringField( "AccessModeDesc" ) ) ;
   ASSERT_STREQ( "READ", obj.getStringField( "ErrorFilterMaskDesc" ) ) ;
   ASSERT_STREQ( "Low", obj.getStringField( "ErrorControlLevel" ) ) ;

   // clean
   db.dropDataSource( dsNewName ) ;
   db.disconnect();
}
