/******************************************************************************
 * @Description   : seqDB-28719:主表插入数据，部分子表执行analyze，部分子表不执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ()
{
   var csName = "cs_28719";
   var mainCLName = "mainCL_28719";
   var subCLName1 = "subCL_28719_1";
   var subCLName2 = "subCL_28719_2";
   var groupName = testPara.groups[0][0].GroupName;

   commDropCS( db, csName );

   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();

   // 插入数据
   var recsNum = 10000;
   insertData( mainCL, recsNum );
   var isDefault = true;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 400;
   var totalRecords = 200 + recsNum;
   var totalDataPages = 9;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + subCLName1, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];

   // 表1的实际totalDataPages与表2的默认值1
   var totalDataPages = tmpcur.TotalDataPages + 1;
   cursor.close();

   // analyze后，该字段值是从磁盘获取，快照和接口不能得出准确值，所以在用例中写固定值
   var totalDataSize = 370000;

   // 表1执行analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": csName + "." + subCLName1 } );

   // 统计信息默认值与实际值的和
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   commDropCS( db, csName );
}