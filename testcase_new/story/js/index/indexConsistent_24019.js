/******************************************************************************
 * @Description   : seqDB-24019:copyIndex接口验证
 * @Author        : liuli
 * @CreateTime    : 2021.08.05
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_maincl_24019";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( args )
{
   var indexName1 = "index_24019_1";
   var indexName2 = "index_24019_2";
   var clName = testConf.clName;
   var subCLName1 = "subcl_24019_1";
   var subCLName2 = "subcl_24019_2";
   var maincl = args.testCL;

   // 创建索引
   maincl.createIndex( indexName1, { b: 1 } );
   maincl.createIndex( indexName2, { c: 1 } );

   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   insertBulkData( maincl, 1000 );

   // 非法参数校验
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      maincl.copyIndex( "nocl" );
   } );

   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      maincl.copyIndex( COMMCSNAME + "." + subCLName1, "noindex" );
   } );

   // 复制索引
   maincl.copyIndex( COMMCSNAME + "." + subCLName1, indexName1 );
   checkCopyTaskForOrder( COMMCSNAME, clName, indexName1, COMMCSNAME + "." + subCLName1, 0, 1 );

   maincl.copyIndex( COMMCSNAME + "." + subCLName1 );
   checkCopyTaskForOrder( COMMCSNAME, clName, indexName2, COMMCSNAME + "." + subCLName1, 0, 2 );

   maincl.copyIndex();
   checkCopyTaskForOrder( COMMCSNAME, clName, [indexName1, indexName2], COMMCSNAME + "." + subCLName2, 0, 3 );

   // 校验任务和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, subCLName1, [indexName1, indexName2], 0 );
   checkIndexTask( "Create index", COMMCSNAME, subCLName1, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName2, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName2, true );
}

// 按BeginTimestamp排序校验copy索引任务
function checkCopyTaskForOrder ( csName, mainclName, indexNames, subclNames, resultCode, beginOrder )
{
   if( undefined == resultCode ) { resultCode = 0; }
   if( beginOrder == undefined ) { beginOrder = 1; }
   if( typeof ( indexNames ) == "string" ) { indexNames = [indexNames]; }
   if( typeof ( subclNames ) == "string" ) { subclNames = [subclNames]; }

   var cursor = db.listTasks( { "Name": csName + '.' + mainclName, TaskTypeDesc: "Copy index" }, {}, { BeginTimestamp: 1 } );
   var taskInfo;
   var order = 0;
   while( cursor.next() )
   {
      order++;
      if( order = beginOrder )
      {
         taskInfo = cursor.current().toObj();
      }
   }
   cursor.close();

   //校验索引名
   if( !commCompareObject( taskInfo.IndexNames.sort(), indexNames.sort() ) ) 
   {
      throw new Error( "check indexNames error! " + JSON.stringify( taskInfo ) );
   }

   //校验结果状态码
   if( taskInfo.ResultCode != resultCode )
   {
      throw new Error( "check resultCode error! " + JSON.stringify( taskInfo ) );
   }

   //校验copy子表信息
   if( !commCompareObject( taskInfo.CopyTo.sort(), subclNames.sort() ) )
   {
      throw new Error( "check copyto subclNames error! " + JSON.stringify( taskInfo ) );
   }

   //校验子表组信息   
   var actGroupNames = [];
   var groups = taskInfo.Groups;
   for( var i = 0; i < groups.length; i++ )
   {
      actGroupNames.push( groups[i].GroupName );
   }

   var clGroupNames = [];
   for( var i = 0; i < subclNames.length; i++ )
   {
      var subclName = subclNames[i];
      var groupNames = commGetCLGroups( db, subclName );
      clGroupNames = clGroupNames.concat( groupNames );
   }

   //去掉重复的组名     
   clGroupNames.sort();
   var expGroupNames = [clGroupNames[0]];
   for( var i = 1; i < clGroupNames.length; i++ )
   {
      if( clGroupNames[i] !== clGroupNames[i - 1] )
      {
         expGroupNames.push( clGroupNames[i] )
      }
   }
   if( !commCompareObject( actGroupNames.sort(), expGroupNames ) )
   {
      throw new Error( "check groupNames error! expGroupNames=" + JSON.stringify( expGroupNames ) + "\ntask=" + JSON.stringify( taskInfo ) );
   }
}