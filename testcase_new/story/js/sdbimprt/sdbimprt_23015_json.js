/************************************
*@Description: seqDB-23015:replacekeydup（默认为false）和allowkeydup（默认为true）均为默认值，导入csv/json数据
*              seqDB-23016:replacekeydup为true，allowkeydup为默认值（默认为true），导入csv/json数据
*              seqDB-23017:replacekeydup和allowkeydup均为true，导入数据
*              seqDB-23018:replacekeydup为true，allowkeydup为false，导入数据
*              seqDB-23019:replacekeydup为默认值（false），allowkeydup为false，导入数据
*@Author:      2020/11/07  liuli
**************************************/
testConf.clName = COMMCLNAME + "_23015";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var indexName = "index_23015";
   var indexDef = { "a": 1 };
   var insertRecs = [{ "a": 1, "b": "sss" }, { "b": "eee" }];
   var indexOption1 = { Unique: true };
   var indexOption2 = { Unique: true, Enforced: true };
   var importData1 = '{"b":"eee"}\n' + '{"a":1,"b":"aaa"}\n' + '{"a":2,"b":"bbb"}\n' + '{"a":3,"b":"ccc"}\n';
   var importData2 = '{"b":"ttt"}\n' + '{"a":1,"b":"aaa"}\n' + '{"a":2,"b":"bbb"}\n' + '{"a":3,"b":"ccc"}\n';

   // 指定强制唯一索引，replacekeydup 和 allowkeydup 均为默认值
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption2 );
   var expectResult = [{ "b": "eee" }, { "a": 1, "b": "sss" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   testImprt( cl, importData2, sdbimprtOption, expectResult );

   // 指定唯一索引，replacekeydup 为 true, allowkeydup 为默认值
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var sdbimprtOption = "--replacekeydup true";
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   testImprt( cl, importData1, sdbimprtOption, expectResult );
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   testImprt( cl, importData1, sdbimprtOption, expectResult );

   // replacekeydup 和 allowkeydup 均为 true
   var sdbimprtOption = "--replacekeydup true --allowkeydup true";
   try
   {
      testImprt( cl, importData1, sdbimprtOption, expectResult );
      throw new Error( sdbimprtOption + "is a mistake, expected failure but actual success!!!" );
   } catch( e )
   {
      if( e.message != 127 )
      {
         throw e;
      }
   }

   // 指定强制唯一索引，replacekeydup 为 true, allowkeydup 为 false
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption2 );
   var sdbimprtOption = "--replacekeydup true --allowkeydup false";
   var expectResult = [{ "b": "ttt" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   testImprt( cl, importData2, sdbimprtOption, expectResult );

   // 指定唯一索引，replacekeydup 为 true, allowkeydup 为 false
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var sdbimprtOption = "--replacekeydup true --allowkeydup false";
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "aaa" }, { "a": 2, "b": "bbb" }, { "a": 3, "b": "ccc" }];
   testImprt( cl, importData1, sdbimprtOption, expectResult );

   // 指定唯一索引，replacekeydup 为默认值, allowkeydup 为 false
   prepareIndexAndDataInCL( cl, indexName, indexDef, insertRecs, indexOption1 );
   var sdbimprtOption = "--allowkeydup false";
   var expectResult = [{ "b": "eee" }, { "b": "eee" }, { "a": 1, "b": "sss" }];
   testImprt( cl, importData1, sdbimprtOption, expectResult );

   cmd.run( "rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec" );
}

function prepareIndexAndDataInCL ( cl, indexName, indexDef, insertRecs, indexOption )
{
   cl.remove();
   commDropIndex( cl, indexName, true );
   commCreateIndex( cl, indexName, indexDef, indexOption );
   cl.insert( insertRecs );
}

function testImprt ( cl, importData, sdbimprtOption, expectResult )
{
   var filename = tmpFileDir + "23015.json";
   var file = fileInit( filename );
   file.write( importData );
   file.close();

   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --file " + filename +
      " --type json " +
      " -j 1 --parsers 1 " +
      sdbimprtOption;

   cmd.run( command );
   commCompareResults( cl.find().sort( { "a": 1 } ), expectResult );
   cmd.run( "rm -rf " + filename );
}