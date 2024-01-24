/******************************************************************************
 * @Description   : seqDB-28721:执行analyz,向主表插入大量数据，使主表部分过期执行analyze/全部过期执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test )
function test ()
{
   var csName = "cs_28721";
   var mainCLName = "mainCL_28721";
   var subCLName1 = "subCL_28721_1";
   var subCLName2 = "subCL_28721_2";
   var groupName = testPara.groups[0][0].GroupName;

   commDropCS( db, csName );

   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 300000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 300000 }, UpBound: { a: 500000 } } );

   var node = db.getRG( groupName ).getMaster();
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   db.analyze( { "Collection": csName + "." + mainCLName } );
   // 插入大量数据，使主表部分过期
   var docs1 = [];
   var recsNum1 = 300000;
   for( var i = 0; i < recsNum1; i++ )
   {
      docs1.push( { a: i } );
   }
   mainCL.insert( docs1 );
   db.analyze( { "Collection": csName + "." + mainCLName } );
   // 返回实际值
   var isDefault = false;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + subCLName1, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages1 = tmpcur.TotalDataPages;
   cursor.close();
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, recsNum1, totalDataPages1, undefined );
   // 再次插入大量数据，使主表全部过期
   var docs2 = [];
   var startValue = 300000;
   var recsNum2 = 200000;
   for( var i = startValue; i < ( startValue + recsNum2 ); i++ )
   {
      docs2.push( { a: i } );
   }
   mainCL.insert( docs2 );
   db.analyze( { "Collection": csName + "." + mainCLName } );
   var sampleRecords = 400;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + subCLName2, NodeName: nodeName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages2 = tmpcur.TotalDataPages;
   var totalDataPages = totalDataPages1 + totalDataPages2;
   var recsNum = recsNum1 + recsNum2;
   cursor.close();
   // 返回实际值
   checkCollectionStat( mainCL, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, undefined );

   commDropCS( db, csName );
}