/******************************************************************************
 * @Description   : seqDB-23849:集合上有回收站项目，切分
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.06.24
 * @LastEditTime  : 2022.06.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = CHANGEDPREFIX + "_cs_23849";
testConf.clName = COMMCLNAME + "_23849";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "hash" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   var csName = testConf.csName;
   var clName = testConf.clName;
   var clFullName = csName + "." + clName;
   var cl = testPara.testCL;

   cleanRecycleBin( db, csName );

   var docs = [{ "a": 1 }, { "a": 100 }];
   cl.insert( docs );
   // 生成truncate回收站项目
   cl.truncate();
   var recycleName = getOneRecycleName( db, clFullName, "Truncate" );
   assert.notEqual( recycleName, undefined );

   // 存在回收站项目时split
   cl.split( srcGroupName, dstGroupName, 50 );
   assert.equal( commGetCLGroups( db, clFullName ), [srcGroupName, dstGroupName] );
   assert.equal( cl.count(), 0 );

   // 恢复回收站项目，并检查恢复后数据正确性
   db.getRecycleBin().returnItem( recycleName, { "Enforced": true } );
   assert.equal( getRecycleName( db, clFullName, "Truncate" ), [] );

   assert.equal( commGetCLGroups( db, clFullName ), [srcGroupName] );
   commCompareResults( cl.find(), docs );
}