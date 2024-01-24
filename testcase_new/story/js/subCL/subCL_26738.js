/******************************************************************************
 * @Description   : seqDB-26738:主表range，子表hash，插入数据查看错误码是否正确
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.07.19
 * @LastEditTime  : 2022.08.04
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
main( test );

function test ( testPara )
{
   var mainClName = CHANGEDPREFIX + "mainClName_26738";
   var subClName = CHANGEDPREFIX + "subClName_26738";
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
   mainCL.insert( { a: 0, b: [1, 2], salary: 100 } );
   mainCL.insert( { a: 1, b: [1, 2], salary: 100 } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.update( { $addtoset: { c: 2 } } );
   } );
   mainCL.update( { $pull: { "b.0": 1 } } );
   mainCL.update( { $push: { salary: 1 } } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.update( { $pull_all: { b: 3 } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.update( { $push_all: { b: 2 } } );
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.update( { $pop: { b: [2] } } );
   } );
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