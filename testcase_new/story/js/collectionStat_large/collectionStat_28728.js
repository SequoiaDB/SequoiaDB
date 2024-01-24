/******************************************************************************
 * @Description   : seqDB-28728:设置会话访问备节点，插入数据后，执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28728";
testConf.clOpt = { ReplSize: 0 };
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   // 设置会话属性是访问备节点
   db.setSessionAttr( { PreferredInstance: "S" } );

   var recsNum = 10000;
   insertData( cl, recsNum );

   // analyze，更新缓存中的集合统计信息
   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );
   // 接口返回统计信息实际值
   var isDefault = false;
   var isExpired = false;
   var avgNumFields = 10;
   var sampleRecords = 200;
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: COMMCSNAME + "." + testConf.clName } );
   var tmpcur = cursor.current().toObj()["Details"][0]["Group"][0];
   var totalDataPages = tmpcur.TotalDataPages;
   cursor.close();

   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, recsNum, totalDataPages, undefined );
}