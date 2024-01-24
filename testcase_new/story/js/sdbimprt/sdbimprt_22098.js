/***************************************************************************
@Description : seqDB-22098:sdbimprt 增加支持“null”导入为data/timestamp类型
@Modify list :
2020-04-22  chensiqin  Create
****************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_22098";

main( test );

function test ( testPara )
{

   cl = testPara.testCL;
   clName = testConf.clName
   testImprtCsv1( clName, cl );
   testImprtCsv2( clName, cl );
}

function testImprtCsv1 ( clName, cl )
{
   var filename = tmpFileDir + "22098_1.csv";
   var file = fileInit( filename );
   file.write( "1,null,null,null,null,null,null,null,null,null\n" );
   file.write( "2,, , , , , , , ,1\n" );
   file.write( "3,1,2,3,4,5,6,7,8,9\n" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--file " + filename;

   cl.remove();
   cmd.run( command + " --fields='_id int, type1 int, type2 long, type3 double, type4 decimal, type5 number, type6 bool, type7 string, type8 null, type9 int' " );
   var expectResult = [{ "type1": null, "type2": null, "type3": null, "type4": null, "type5": null, "type6": null, "type7": "null", "type8": null, "type9": null }, { "type9": 1 }, { "type1": 1, "type2": 2, "type3": 3, "type4": { "$decimal": "4" }, "type5": 5, "type6": true, "type7": "7", "type8": null, "type9": 9 }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();
   cmd.run( command + " --fields='_id int, type1 , type2 , type3 , type4 , type5 , type6 , type7 , type8, type9 ' " );
   var expectResult = [{ "type1": null, "type2": null, "type3": null, "type4": null, "type5": null, "type6": null, "type7": null, "type8": null, "type9": null }, { "type9": 1 }, { "type1": 1, "type2": 2, "type3": 3, "type4": 4, "type5": 5, "type6": 6, "type7": 7, "type8": 8, "type9": 9 }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
}

function testImprtCsv2 ( clName, cl )
{

   var filename = tmpFileDir + "22098_2.csv";
   var file = fileInit( filename );
   file.write( '1,,null,2020-04-22\n' );
   file.write( '2,null,,2020-04-22 \n' );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields='_id int, type1 timestamp, type2 date,date1 date' " +
      "--file " + filename;

   cl.remove();
   cmd.run( command );
   var expectResult = [{ "type2": null, "date1": { "$date": "2020-04-22" } }, { "type1": null, "date1": { "$date": "2020-04-22" } }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

}
