/******************************************************************************
 * @Description   : seqDB-6626:创建CL，不开压缩，对CL做CRUD 
                     (不开压缩，CRUD操作在其他其他用例都要覆盖，此用例只验证CL属性及插入数据正确性)
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6626";
testConf.clOpt = { Compressed: false, ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 50000;
   var checkRecsNum = 3;

   // 检查编目集合属性
   var clInfo = db.snapshot( 8, { Name: csName + "." + clName } ).toArray();
   var details = JSON.parse( clInfo[0] );
   assert.equal( details.Attribute, 0, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.AttributeDesc, "", "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionType, undefined, "clInfo = " + JSON.stringify( clInfo ) );
   assert.equal( details.CompressionTypeDesc, undefined, "clInfo = " + JSON.stringify( clInfo ) );

   // 插入数据
   insertRecs1( cl, insertRecsNum );

   // 检查结果，检查组内每个节点数据正确性
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );

         // 检查CL属性正确性
         var clInfo = nodeDB.snapshot( 4, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         assert.equal( details.Attribute, "", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CompressionType, "", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.DictionaryCreated, false, "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.DictionaryVersion, 0, "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CurrentCompressionRatio, 1, "clInfo = " + JSON.stringify( clInfo ) );

         // 检查总数据量正确性
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, insertRecsNum );

         // 检查随机n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * insertRecsNum );
            var recsCnt = cl.find( {
               atest: i, btest: i, ctest: "test" + i,
               dtest: "abcdefg890abcdefg890abcdefg890"
            } ).count();
            var expctCnt = 1;
            assert.equal( recsCnt, expctCnt );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}