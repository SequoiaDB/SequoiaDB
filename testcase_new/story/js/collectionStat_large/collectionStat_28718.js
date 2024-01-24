/******************************************************************************
 * @Description   : seqDB-28718:主表插入数据，不执行/执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.20
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ()
{
   var csName = "cs_28718";
   var mainCLName = "mainCL_28718";
   var subCLName1 = "subCL_28718_1";
   var subCLName2 = "subCL_28718_2";
   var groupName = testPara.groups[0][0].GroupName;

   commDropCS( db, csName );

   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   // 插入数据
   var recsNum = 10000;
   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   insertData( mainCL, recsNum );
   var isDefault = true;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 400;
   var totalRecords = 400;
   var totalDataPages = 2;
   var totalDataSize = 160000;

   // 未做analyze,显示集合统计信息的默认值
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": csName + "." + mainCLName } );
   isDefault = false;
   var sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + subCLName1, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages1 = tmpcur.TotalDataPages;
   cursor.close();

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + subCLName2, NodeName: nodeName } );
   tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages2 = tmpcur.TotalDataPages;
   totalDataPages = totalDataPages1 + totalDataPages2;
   cursor.close();
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, undefined );

   commDropCS( db, csName );
}