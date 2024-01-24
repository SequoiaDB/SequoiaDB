/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13543:后面记录比第一条记录字字段数多，
*                            强制导出到json
*                seqDB-13544:后面记录比第一条记录字字段数多，
*                            强制导出到csv             
* @author      : Liang XueWang 
*
*******************************************************************/
main( test );

function test ()
{
   testWithIdCsv();
   testWithIdJson();
}

function testWithIdCsv ()
{
   var clName1 = COMMCLNAME + "_sdbexprt13544";
   var clName2 = COMMCLNAME + "_sdbimprt13544";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { _id: 1, a: 1 } );
   cl1.insert( { _id: 2, a: 2, b: 1 } );
   cl1.insert( { _id: 3, a: 3, b: 2, c: 1 } );

   var csvFile = tmpFileDir + "sdbexprt13544.csv";
   cmd.run( "rm -rf " + csvFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + csvFile +
      " --type csv" +
      " --withid true" +
      " --sort '{ _id: 1 }'" +
      " --force true";
   testRunCommand( command );

   var expResult = "_id,a\n1,1\n2,2\n3,3\n";
   checkFileContent( csvFile, expResult );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + csvFile +
      " --type csv " +
      " --fields='_id int,a int'" +
      " --headerline true" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecords = [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }];
   var cursor = cl2.find();
   commCompareResults( cursor, expRecords, false );

   cmd.run( "rm -rf " + csvFile );
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}

function testWithIdJson ()
{
   var clName1 = COMMCLNAME + "_sdbexprt13543";
   var clName2 = COMMCLNAME + "_sdbimprt13543";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { _id: 1, a: 1 } );
   cl1.insert( { _id: 2, a: 2, b: 1 } );
   cl1.insert( { _id: 3, a: 3, b: 2, c: 1 } );

   var jsonFile = tmpFileDir + "sdbexprt13543.json";
   cmd.run( "rm -rf " + jsonFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + jsonFile +
      " --type json" +
      " --withid true" +
      " --sort '{ _id: 1 }'";
   testRunCommand( command );

   var expResult = "{ \"_id\": 1, \"a\": 1 }\n" +
      "{ \"_id\": 2, \"a\": 2, \"b\": 1 }\n" +
      "{ \"_id\": 3, \"a\": 3, \"b\": 2, \"c\": 1 }\n";
   checkFileContent( jsonFile, expResult );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + jsonFile +
      " --type json" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecords = [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2, "b": 1 }, { "_id": 3, "a": 3, "b": 2, "c": 1 }];
   var cursor = cl2.find();
   commCompareResults( cursor, expRecords, false );

   cmd.run( "rm -rf " + jsonFile );
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
