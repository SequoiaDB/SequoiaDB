/******************************************************************************
 * @Description   : seqDB-30387:_id重复处理为跳过重复记录，查看导入数据结果
 *                : seqDB-30388:_id重复处理为替换重复记录，查看导入数据结果
 *                : seqDB-30406:导入工具指定keydup相关参数冲突
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.03.08
 * @LastEditTime  : 2023.03.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_30387_30388_csv";

main( test );
function test ( args )
{
   var cl = args.testCL;
   var insertRecs = [{ "_id": 1, "b": "sss" }, { "_id": 2, "b": "eee" }, { "_id": 3, "b": "ddd" }];
   // 准备导入数据{ "_id": 1, "b": "aaa" }, { "_id": 4, "b": "fff" }
   var importData1 = "1,aaa\n" + "2,bbb\n" + "4,fff\n";
   var expDuplicatedRecords = "Duplicated records: 2";

   // seqDB-30388:_id重复处理为替换重复记录，查看导入数据结果
   // 指定唯一索引，replaceidkeydup为true
   cl.insert( insertRecs );
   var sdbimprtOption = "--replaceidkeydup true";
   var expectResult = [{ "_id": 1, "b": "aaa" }, { "_id": 2, "b": "bbb" }, { "_id": 3, "b": "ddd" }, { "_id": 4, "b": "fff" }];
   imprtAndCheck( cl, importData1, sdbimprtOption, expectResult, expDuplicatedRecords );

   // seqDB-30387:_id重复处理为跳过重复记录，查看导入数据结果
   // 指定唯一索引，allowidkeydup为true
   cl.truncate();
   cl.insert( insertRecs );
   var sdbimprtOption = "--allowidkeydup true";
   var expectResult = [{ "_id": 1, "b": "sss" }, { "_id": 2, "b": "eee" }, { "_id": 3, "b": "ddd" }, { "_id": 4, "b": "fff" }];
   imprtAndCheck( cl, importData1, sdbimprtOption, expectResult, expDuplicatedRecords );

   // seqDB-30406:导入工具指定keydup相关参数冲突
   cl.truncate();
   cl.insert( insertRecs );
   var sdbimprtOption = " --allowidkeydup true --allowkeydup true";
   assert.tryThrow( 127, function()
   {
      imprtAndCheck( cl, importData1, sdbimprtOption );
   } );

   var sdbimprtOption = " --allowidkeydup true --replacekeydup true";
   assert.tryThrow( 127, function()
   {
      imprtAndCheck( cl, importData1, sdbimprtOption );
   } );

   var sdbimprtOption = " --allowidkeydup true --replaceidkeydup true";
   assert.tryThrow( 127, function()
   {
      imprtAndCheck( cl, importData1, sdbimprtOption );
   } );

   var sdbimprtOption = " --replaceidkeydup true --allowkeydup true";
   assert.tryThrow( 127, function()
   {
      imprtAndCheck( cl, importData1, sdbimprtOption );
   } );

   var sdbimprtOption = " --replaceidkeydup true --replacekeydup true";
   assert.tryThrow( 127, function()
   {
      imprtAndCheck( cl, importData1, sdbimprtOption );
   } );

   cmd.run( "rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec" );
}

function imprtAndCheck ( cl, importData, sdbimprtOption, expectResult, expDuplicatedRecords )
{
   var filename = tmpFileDir + "30387_30388.csv";
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   file.write( importData );
   file.close();

   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      " --file " + filename +
      " --fields ' _id, b' " +
      " -j 1 --parsers 1 " +
      sdbimprtOption;

   var rc = cmd.run( command );
   var rcObj = rc.split( "\n" );
   assert.notEqual( rcObj.indexOf( expDuplicatedRecords ), -1, rcObj );

   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult, false );
   cmd.run( "rm -rf " + filename );
}