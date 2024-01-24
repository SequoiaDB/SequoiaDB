/******************************************************************************
 * @Description   : seqDB-28729:设置会话访问备节点，执行analyze插入大量数据后，不再执行analyze
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.21
 * @LastEditTime  : 2023.01.12
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28729";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   // 设置会话属性是访问备节点
   db.setSessionAttr( { PreferredInstance: "S" } );

   db.analyze( { "Collection": COMMCSNAME + "." + testConf.clName } );

   // 插入大量数据使缓存过期
   var recsNum = 500000;
   insertData( cl, recsNum );
   var isDefault = false;
   var isExpired = true;
   var avgNumFields = 10;
   var sampleRecords = 0;
   var totalRecords = 0;
   var totalDataPages = 0;
   var totalDataSize = 0;

   // 接口返回统计信息实际值
   checkCollectionStat( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize );
}