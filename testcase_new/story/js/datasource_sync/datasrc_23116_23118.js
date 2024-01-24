/******************************************************************************
 * @Description   : seqDB-23116:alterDataSource接口参数验证
 *                  seqDB-23118:getDataSource接口参数验证 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.04.01
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
   datasrcDB.createUsr( userName, passwd );
   var coordArr = getCoordUrl( datasrcDB );
   var dataSrcSize = 1;
   var dataSrcName = createDataSrcName( dataSrcSize );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 修改数据源名称为1个字节
   var dataSource = db.getDataSource( dataSrcName );
   var dataSrcName = createDataSrcName( dataSrcSize )
   dataSource.alter( { "Name": dataSrcName } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName );

   // 修改数据源名称为127个字节
   var dataSrcSize = 127;
   var dataSource = db.getDataSource( dataSrcName );
   var dataSrcName = createDataSrcName( dataSrcSize );
   dataSource.alter( { "Name": dataSrcName } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName );

   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSource = db.getDataSource( dataSrcName );
   var dataSrcName = createDataSrcName( dataSrcSize );
   dataSource.alter( { "Name": dataSrcName } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName );

   // 修改Address为HostName:scvName
   var dataSource = db.getDataSource( dataSrcName );
   dataSource.alter( { "Address": coordArr[0] } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName );

   // 修改AccessMode
   dataSource.alter( { "AccessMode": "READ" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName, "READ" );

   // 修改ErrorFilterMask
   dataSource.alter( { "ErrorFilterMask": "WRITE" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName, "READ", "WRITE" );

   // 修改ErrorControlLevel
   dataSource.alter( { "ErrorControlLevel": "Low" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName, "READ", "WRITE", "Low" );

   // 修改Address为IP
   var options = { "Address": datasrcIp };
   Alter( dataSource, options );

   // 修改User
   dataSource.alter( { "Address": coordArr[0] } );
   var options = { "User": "test" };
   Alter( dataSource, options, SDB_AUTH_AUTHORITY_FORBIDDEN );

   // 修改Password
   var options = { "Password": "test" };
   Alter( dataSource, options, SDB_AUTH_AUTHORITY_FORBIDDEN );

   // 修改参数不合法
   var options = { "ErrorFilterMask": "sssssss" };
   Alter( dataSource, options );

   var options = { "ErrorControlLevel": "sssssss" };
   Alter( dataSource, options );

   // 数据源名称以SYS开头
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName1 = "SYS" + createDataSrcName( dataSrcSize );
   var options = { "Name": dataSrcName1 };
   Alter( dataSource, options );

   // 数据源名称以$开头
   var dataSrcSize = Math.random() * ( 127 - 1 );
   var dataSrcName1 = "$" + createDataSrcName( dataSrcSize );
   var options = { "Name": dataSrcName1 };
   Alter( dataSource, options );

   // 数据源名称中包含 . 
   var dataSrcName1 = createDataSrcName( 3 ) + "." + createDataSrcName( 3 );
   var options = { "Name": dataSrcName1 };
   Alter( dataSource, options );

   // 数据源名称有128个字节
   var dataSrcSize = 128;
   var dataSrcName1 = createDataSrcName( dataSrcSize );
   var options = { "Name": dataSrcName1 };
   Alter( dataSource, options );

   clearDataSource( "nocs", dataSrcName );

   try
   {
      db.dropDataSource( dataSrcName1 );
   }
   catch( e )
   {
      if( e != SDB_INVALIDARG )
      {
         throw new Error( e );
      }
   }
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

function Alter ( dataSource, options, errorCode )
{
   if( typeof ( errorCode ) == "undefined" ) { errorCode = SDB_INVALIDARG; }
   assert.tryThrow( [errorCode], function() 
   {
      dataSource.alter( options );
   } );
}