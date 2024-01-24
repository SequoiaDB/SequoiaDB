/******************************************************************************
 * @Description   : seqDB-23115:createDataSource接口参数验证
 *                  seqDB-23117:dropDataSource接口参数验证 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// CI环境不支持运行数据源串行用例，暂时屏蔽
// try
// {
//    main( test );
// }
// finally
// {
//    try
//    {
//       datasrcDB.dropUsr( userName, passwd );
//    }
//    finally
//    {
//       datasrcDB.close();
//    }
// }

function test ()
{
   var type = "SequoiaDB";

   datasrcDB.createUsr( userName, passwd );
   var coordArr = getCoordUrl( datasrcDB );
   var dataSrcSize = 1;
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName );
   clearDataSource( "nocs", dataSrcName );

   // 数据源名称127个字节
   var dataSrcSize = 127;
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, coordArr[0], userName, passwd );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName );
   clearDataSource( "nocs", dataSrcName );

   // 创建数据源，指定AccessMode参数
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, type, { "AccessMode": "READ" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName, "READ" );
   clearDataSource( "nocs", dataSrcName );

   // 创建数据源，指定ErrorFilterMask参数
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, type, { "ErrorFilterMask": "WRITE" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName, "READ|WRITE", "WRITE" );
   clearDataSource( "nocs", dataSrcName );

   // 创建数据源，指定ErrorControlLevel参数
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, type, { "ErrorControlLevel": "Low" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName, "READ|WRITE", "NONE", "Low" );
   clearDataSource( "nocs", dataSrcName );

   // 数据源名称为""
   var dataSrcName = "";
   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd );
   clearDataSource( "nocs", dataSrcName );

   // 数据源名称以 $ 开头
   var dataSrcName = createDataSrcName( 20 );
   dataSrcName = "$" + dataSrcName;
   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd );
   clearDataSource( "nocs", dataSrcName );

   // 数据源名称以 SYS 开头
   var dataSrcName = createDataSrcName( 20 );
   dataSrcName = "SYS" + dataSrcName;
   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd );
   clearDataSource( "nocs", dataSrcName );

   // 数据源名称包含 128个字节
   var dataSrcSize = 128;
   var dataSrcName = createDataSrcName( dataSrcSize );
   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd );
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      db.dropDataSource( dataSrcName );
   } );

   // 创建数据源，Address 为 IP
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName = createDataSrcName( dataSrcSize );
   CreateDataSource( dataSrcName, datasrcIp, userName, passwd );
   clearDataSource( "nocs", dataSrcName );

   assert.tryThrow( [SDB_AUTH_AUTHORITY_FORBIDDEN], function() 
   {
      db.createDataSource( dataSrcName, datasrcUrl, "test", passwd );
   } );
   clearDataSource( "nocs", dataSrcName );

   assert.tryThrow( [SDB_AUTH_AUTHORITY_FORBIDDEN], function() 
   {
      db.createDataSource( dataSrcName, datasrcUrl, userName, "test" );
   } );
   clearDataSource( "nocs", dataSrcName );

   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd, "mysql" );
   clearDataSource( "nocs", dataSrcName );

   CreateDataSource( dataSrcName, datasrcUrl, userName, passwd, type, "ftslownodethreshold" );
   clearDataSource( "nocs", dataSrcName );
}

function createDataSrcName ( dataSrcSize )
{
   var str = '';
   for( var i = dataSrcSize; i > 0; i-- )
   {
      str += String.fromCharCode( ( Math.floor( Math.random() * 80 ) + 47 ) );
   }
   if( str.slice( 0, 3 ) == "SYS" )
   {
      str = createDataSrcName( dataSrcSize );
   }
   return str;
}

function CreateDataSource ( dataSrcName, datasrcUrl, userName, passwd, trpe, options )
{
   if( trpe == undefined ) { trpe = ""; }
   if( options == undefined ) { options = ""; }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, trpe, options );
   } );
}

function checkExplain ( explainObj, name, address, user, accessModeDesc, errorFilterMaskDesc, errorControlLevel )
{
   if( typeof ( accessModeDesc ) == "undefined" ) { accessModeDesc = "READ|WRITE"; }
   if( typeof ( errorFilterMaskDesc ) == "undefined" ) { errorFilterMaskDesc = "NONE"; }
   if( typeof ( errorControlLevel ) == "undefined" ) { errorControlLevel = "High"; }
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   assert.equal( explainObj.current().toObj().User, user );
   assert.equal( explainObj.current().toObj().AccessModeDesc, accessModeDesc );
   assert.equal( explainObj.current().toObj().ErrorFilterMaskDesc, errorFilterMaskDesc );
   assert.equal( explainObj.current().toObj().ErrorControlLevel, errorControlLevel );
   explainObj.close();
}