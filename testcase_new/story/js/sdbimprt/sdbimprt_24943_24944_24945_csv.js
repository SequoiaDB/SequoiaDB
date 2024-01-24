/******************************************************************************
 * @Description   : seqDB-24944:冲突处理为替换冲突记录，查看导入数据结果
 *                  seqDB-24943:冲突处理为跳过冲突记录，查看导入数据结果
 *                  seqDB-24945:导入数据中包含唯一索引，查看导入数据结果  
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.01.10
 * @LastEditTime  : 2022.01.26
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24944";

main( test );
function test ( args )
{
   var cl = args.testCL;
   var indexName = "index_24944";
   var indexDef = { "a": 1 };
   var insertRecs = [{ "a": 1, "b": "sss" }, { "b": "eee" }, { "a": 3, "b": "ddd" }];
   var indexOption1 = { Unique: true };
   // 准备导入数据{ "b": "eee" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }
   var importData1 = ",eee\n" + "1,aaa\n" + "2,bbb\n" + "3,ccc\n";
   var expDuplicatedRecords = "Duplicated records: 2";

   // seqDB-24944:冲突处理为替换冲突记录，查看导入数据结果
   // 指定唯一索引，replacekeydup为true
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var sdbimprtOption = "--replacekeydup true";
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   imprtAndCheck( cl, importData1, sdbimprtOption, expectResult, expDuplicatedRecords );

   // seqDB-24943:冲突处理为跳过冲突记录，查看导入数据结果
   // 指定唯一索引，allowkeydup为true
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var sdbimprtOption = "--allowkeydup true";
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "sss" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ddd" }];
   imprtAndCheck( cl, importData1, sdbimprtOption, expectResult, expDuplicatedRecords );

   // seqDB-24945:导入数据中包含唯一索引，查看导入数据结果  
   // 场景a：不存在索引冲突
   var importData1 = ",eee\n" + "4,ccc\n";
   var expDuplicatedRecords = "Duplicated records: 0";
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "sss" }, { "a": 3, "b": "ddd" }, { "a": 4, "b": "ccc" }];
   imprtAndCheck( cl, importData1, "", expectResult, expDuplicatedRecords );

   // 场景b：存在1条索引冲突
   var importData1 = ",eee\n" + "3,ccc\n";
   var expDuplicatedRecords = "Duplicated records: 1";
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "sss" }, { "a": 3, "b": "ddd" }];
   imprtAndCheck( cl, importData1, "", expectResult, expDuplicatedRecords );

   // 场景c：所有数据存在索引冲突
   var importData1 = "1,eee\n" + "3,ccc\n";
   var expDuplicatedRecords = "Duplicated records: 2";
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var expectResult = [{ "b": "eee" }, { "a": 1, "b": "sss" }, { "a": 3, "b": "ddd" }];
   imprtAndCheck( cl, importData1, "", expectResult, expDuplicatedRecords );

   cmd.run( "rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec" );
}

function prepareIndexAndDataInCL ( cl, indexName, indexDef, insertRecs, indexOption )
{
   cl.remove();
   commDropIndex( cl, indexName, true );
   commCreateIndex( cl, indexName, indexDef, indexOption );
   cl.insert( insertRecs );
}

function imprtAndCheck ( cl, importData, sdbimprtOption, expectResult, expDuplicatedRecords )
{
   var filename = tmpFileDir + "24943_24944_24945.csv";
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
      " --fields ' a, b' " +
      " -j 1 --parsers 1 " +
      sdbimprtOption;

   var rc = cmd.run( command );
   var rcObj = rc.split( "\n" );
   var actDuplicatedRecords = rcObj[6];

   assert.equal( actDuplicatedRecords, expDuplicatedRecords );
   commCompareResults( cl.find().sort( { "a": 1 } ), expectResult );
   cmd.run( "rm -rf " + filename );
}