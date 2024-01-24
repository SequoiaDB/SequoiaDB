/******************************************************************************
 * @Description   : seqDB-6656:批量更新已压缩和未压缩的记录
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6656";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 800000;
   var checkRecsNum = 3;

   // insert   
   insertRecs2( cl, insertRecsNum );

   waitDictionary( db, csName, clName );

   // findAndUpdateRecs
   var rc = cl.find( { $and: [{ INNER_NO: { $gte: 200000 } }, { INNER_NO: { $lt: 700000 } }] } )
      .update( { $inc: { SA_ACCT_NO: 1 }, $set: { IVC_NAME: "<DRW_NME>徐凤明</DRW_NME>" } } );
   while( rc.next() );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum, checkRecsNum );
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );
         // 检查数据总数
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, insertRecsNum );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * insertRecsNum );

            if( i < 200000 || i >= 700000 )
            {  // 检查更新前记录
               var recsCnt = nodeCL.find( {
                  INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
                  IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行"
               } ).count();
            }
            else
            {  // 检查更新后记录
               var recsCnt = nodeCL.find( {
                  INNER_NO: i, SA_ACCT_NO: i + 1, EVT_ID: "lwy20120702" + i,
                  IVC_NAME: "<DRW_NME>徐凤明</DRW_NME>"
               } ).count();
            }
            assert.equal( recsCnt, 1 );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}