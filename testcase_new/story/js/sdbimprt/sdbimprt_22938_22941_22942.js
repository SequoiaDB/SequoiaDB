/******************************************************************************
 * @Description   : seqDB-22938:导入字符串类型的数据，指定字符串最小长度<最大长度
 *                  seqDB-22941:导入字符串类型的数据，指定字符串最小长度=最大长度
 *                  seqDB-22942:导入字符串类型的数据，指定字符串最小长度>最大长度
 * @Author        : liuli
 * @CreateTime    : 2020.11.02
 * @LastEditTime  : 2021.03.29
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_22938";
var filename = tmpFileDir + "22938.csv";

main( test );

function test ( args )
{
   var file = fileInit( filename );
   file.write( "1,abc\n" + "2,abcd\n" + "3,abcde\n" + "4,abcdef\n" + "5,abcdefg\n" + "6,''" );
   file.close();
   var cl = args.testCL;

   // min < max, not specify default
   var fields = "a string(4,6)";
   var expectResult = [{ "a": null }, { "a": "abcd" }, { "a": "abcde" }, { "a": "abcdef" }, { "a": "abcdef" }, { "a": null }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // default < min < max
   var fields = "a string(4,6) default r*3";
   var expectResult = [];
   assert.tryThrow( 127, function()
   {
      testImprt( fields );
   } );

   // min < default < max
   var fields = "a string(4,6) default s*345";
   var expectResult = [{ "a": "s*345" }, { "a": "abcd" }, { "a": "abcde" }, { "a": "abcdef" }, { "a": "abcdef" }, { "a": "s*345" }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min < max < default
   var fields = "a string(4,6) default t*34567";
   var expectResult = [{ "a": "t*3456" }, { "a": "abcd" }, { "a": "abcde" }, { "a": "abcdef" }, { "a": "abcdef" }, { "a": "t*3456" }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min = max, not specify default
   var fields = "a string(5,5)";
   var expectResult = [{ "a": null }, { "a": null }, { "a": "abcde" }, { "a": "abcde" }, { "a": "abcde" }, { "a": null }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min = max = default
   var fields = "a string(5,5) default s*345";
   var expectResult = [{ "a": "s*345" }, { "a": "s*345" }, { "a": "abcde" }, { "a": "abcde" }, { "a": "abcde" }, { "a": "s*345" }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min = max < default
   var fields = "a string(5,5) default t*34567";
   var expectResult = [{ "a": "t*345" }, { "a": "t*345" }, { "a": "abcde" }, { "a": "abcde" }, { "a": "abcde" }, { "a": "t*345" }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min > max, max != 0, not specify default
   var fields = "a string(6,4)";
   var expectResult = [];
   assert.tryThrow( 127, function()
   {
      testImprt( fields );
   } );

   // min > max, max = 0, not specify default
   var fields = "a string(6,0)";
   var expectResult = [{ "a": null }, { "a": null }, { "a": null }, { "a": "abcdef" }, { "a": "abcdefg" }, { "a": null }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // min > max, max = 0, default >min
   var fields = "a string(6,0) default t*34567";
   var expectResult = [{ "a": "t*34567" }, { "a": "t*34567" }, { "a": "t*34567" }, { "a": "abcdef" }, { "a": "abcdefg" }, { "a": "t*34567" }];
   testImprt( fields );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();
}

function testImprt ( fields )
{
   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields '_id int, " + fields +
      "' --file " + filename;

   cmd.run( command );
}