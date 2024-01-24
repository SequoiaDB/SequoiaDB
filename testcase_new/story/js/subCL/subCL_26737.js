/******************************************************************************
 * @Description   : seqDB-26737:创建主表range切分，子表hash切分，插入数据
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.07.19
 * @LastEditTime  : 2022.08.03
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ( testPara )
{
   var mainClName = CHANGEDPREFIX + "mainClName_26737";
   var subClName = CHANGEDPREFIX + "subClName_26737";
   commDropCL( db, COMMCSNAME, subClName + "1", true, true );
   commDropCL( db, COMMCSNAME, subClName + "2", true, true );
   commDropCL( db, COMMCSNAME, mainClName, true, true );
   var cs = testPara.testCS;
   var mainCL = cs.createCL( mainClName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCL1 = cs.createCL( subClName + "1", { ShardingKey: { a: 1 } } );
   var subCL2 = cs.createCL( subClName + "2", { ShardingKey: { a: 1 } } );
   mainCL.attachCL( COMMCSNAME + "." + subClName + "1", { LowBound: { a: { $minKey: 1 } }, UpBound: { a: 50 } } );
   mainCL.attachCL( COMMCSNAME + "." + subClName + "2", { LowBound: { a: 50 }, UpBound: { a: { $maxKey: 1 } } } );
   var subCL = [];
   subCL.push( subCL1 );
   subCL.push( subCL2 );
   for( var i = 0; i < subCL.length; ++i )
   {
      // 获取源数据组、目标数据组和partition
      var sourceDataGroupName = getSourceGroupNameAlone( COMMCSNAME, subClName + ( i + 1 ) );

      var desDataGroupName = getOtherDataGroups( sourceDataGroupName );

      var partition = getPartition( COMMCSNAME, subClName + ( i + 1 ) );
      // 对两个子表进行分区切分
      subCLSplitHash( subCL[i], sourceDataGroupName, desDataGroupName, partition );
   }
   var insertNum = 100;
   var docs = [];
   var expResult = [];
   for( var i = 0; i < insertNum; ++i )
   {
      docs.push( { a: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市" } } );
      expResult.push( { a: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市", street: "人民路12号" } } );
   }
   mainCL.insert( docs );
   mainCL.update( { $set: { "localtion.street": "人民路12号" } } );
   var cursor = mainCL.find().sort( { a: 1 } );
   commCompareResults( cursor, expResult );

   mainCL.truncate();
   var docs = [];
   docs.push( { a: 0, salary: 100 } );
   docs.push( { a: 1, salary: 10, name: "Tom" } );
   docs.push( { a: 1, salary: 10, name: "Mike", age: 20 } );
   mainCL.insert( docs );
   mainCL.update( { $set: { age: 25 } }, { age: { $exists: 1 } } );
   var expResult = [];
   expResult.push( { a: 1, salary: 10, name: "Mike", age: 25 } );
   var cursor = mainCL.find( { age: 25 } ).sort( { a: 1 } );
   commCompareResults( cursor, expResult );

   mainCL.truncate();
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 90; ++i )
   {
      docs.push( { a: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市" } } );
   }
   for( var i = 90; i < 100; i++ )
   {
      docs.push( { a: i, mineName: "北京矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "北京", city: "北京市" } } );
      expResult.push( { a: i, mineName: "北京矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "北京", city: "北京市", street: "人民路12号" } } );
   }
   mainCL.insert( docs );
   mainCL.upsert( { $set: { "localtion.street": "人民路12号" } }, { mineName: "北京矿场" } );
   var cursor = mainCL.find( { mineName: "北京矿场" } ).sort( { a: 1 } );
   commCompareResults( cursor, expResult );

   mainCL.truncate();
   var docs = [];
   var expResult = [];
   for( var i = 0; i < 100; ++i )
   {
      docs.push( { a: i, mineName: "上海矿场", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市" } } );
   }
   mainCL.insert( docs );
   mainCL.upsert( { $set: { "localtion.street": "人民路12号" } }, { mineTime: "2013-06-14" } );
   var cursor = mainCL.find( { localtion: { $elemMatch: { street: "人民路12号" } } } ).sort( { a: 1 } );
   expResult.push( { "localtion": { "street": "人民路12号" }, "mineTime": "2013-06-14" } );
   commCompareResults( cursor, expResult );
   commDropCL( db, COMMCSNAME, mainClName, true, true );
}

function getSourceGroupNameAlone ( COMMCSNAME, clName )
{
   var fullName = COMMCSNAME + "." + clName;
   var GroupName = "";
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: fullName } );
   while( cursor.next() )
   {
      GroupName = cursor.current().toObj()["CataInfo"][0]["GroupName"];
   }
   cursor.close();
   return GroupName;
}

function getOtherDataGroups ( SourceGroupName )
{
   var cursor = db.listReplicaGroups();
   var Groups = [];
   while( cursor.next() )
   {
      if( cursor.current().toObj()["Role"] == 0 )
      {
         if( cursor.current().toObj()["GroupName"] != SourceGroupName )
         {
            Groups.push( cursor.current().toObj()["GroupName"] )
         }
      }
   }
   cursor.close();
   return Groups;
}

function getPartition ( COMMCSNAME, clName )
{
   var fullName = COMMCSNAME + "." + clName;
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: fullName } );
   var partition = "";
   while( cursor.next() )
   {
      partition = cursor.current().toObj()["Partition"];
   }
   cursor.close();
   return partition;
}

function subCLSplitHash ( subcl, SourceGroupName, OtherDataGroups, Partition )
{
   var partitionPerGroup = Partition / ( OtherDataGroups.length + 1 );
   for( var i = 0; i < OtherDataGroups.length; ++i )
   {
      var startPartition = Math.round( partitionPerGroup * i );
      var endPartition = Math.round( partitionPerGroup * ( i + 1 ) );
      subcl.split( SourceGroupName, OtherDataGroups[i], { Partition: startPartition }, { Partition: endPartition } );
   }
}