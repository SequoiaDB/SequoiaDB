/******************************************************************************
 * @Description   : seqDB-6628:指定压缩类型为snappy，创建CL
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6628";
testConf.clOpt = { Compressed: true, CompressionType: "snappy", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 50000;
   var checkRecsNum = 3;

   // 检查集合属性
   var clInfo = db.snapshot( 8, { Name: csName + "." + clName } ).toArray();
   var details = JSON.parse( clInfo[0] );
   assert.equal( details.Attribute, 1, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.AttributeDesc, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionType, 0, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionTypeDesc, "snappy", "clInfo = " + JSON.stringify( clInfo ) );

   // 增删改查数据，并检视结果
   insertRecs1( cl, insertRecsNum );
   checkRecs( cl, insertRecsNum, checkRecsNum, "insert" );

   updateRecs( cl );
   checkRecs( cl, insertRecsNum, checkRecsNum, "update" );

   findAndRemoveRecs( cl );
   checkRecs( cl, insertRecsNum, checkRecsNum, "findAndRemove" );
}

function updateRecs ( cl )
{
   cl.update( { $set: { dtest: "电子银行业务回单(付款)" } } );
}

function findAndRemoveRecs ( cl )
{
   var rc = cl.find( { $and: [{ atest: { $lt: 10000 } }, { dtest: { $et: "电子银行业务回单(付款)" } }] } )
      .remove();
   while( rc.next() );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum, operType )
{
   // 检查总数据量
   var totalCnt = cl.count();
   if( operType == "insert" || operType == "update" )
   {
      var expctCnt = insertRecsNum;
   }
   else if( operType == "findAndRemove" )
   {
      var expctCnt = insertRecsNum - 10000;
   }
   assert.equal( totalCnt, expctCnt );

   // 随机检查n条记录
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      if( operType == "insert" )
      {
         var recsCnt = cl.find( {
            atest: i, btest: i, ctest: "test" + i,
            dtest: "abcdefg890abcdefg890abcdefg890"
         } ).count();
         var expctCnt = 1;
      }
      else if( operType == "update" )
      {
         var recsCnt = cl.find( {
            atest: i, btest: i, ctest: "test" + i,
            dtest: "电子银行业务回单(付款)"
         } ).count();
         var expctCnt = 1;
      }
      else if( operType == "findAndRemove" )
      {
         var recsCnt = cl.find( {
            atest: i, btest: i, ctest: "test" + i,
            dtest: "电子银行业务回单(付款)"
         } ).count();
         if( i < 10000 )
         {
            var expctCnt = 0;
         }
         else
         {
            var expctCnt = 1;
         }
      }
      assert.equal( recsCnt, expctCnt );
   }
}