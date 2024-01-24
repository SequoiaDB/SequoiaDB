/*******************************************************************
* @Description : seqDB-13539:不指定fields，强制导出数据，不包含_id
* @author      : Liang XueWang 
*******************************************************************/
main( test );

function test ()
{
   var clName1 = COMMCLNAME + "_sdbexprt13539";
   var clName2 = COMMCLNAME + "_sdbimprt13539";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { _id: 1, a: 1 } );

   var csvFile = tmpFileDir + "sdbexprt13539.csv";
   cmd.run( "rm -rf " + csvFile );

   //指定withid为false，force为true，导出到csv
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + csvFile +
      " --type csv" +
      " --withid false " +
      " --force true";
   testRunCommand( command );

   var expectResult = "a\n1\n";
   checkFileContent( csvFile, expectResult );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + csvFile +
      " --type csv " +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );

   var expRecords = [{ "a": 1 }];
   var cursor = cl2.find( {}, { _id: { $include: 0 } } );
   commCompareResults( cursor, expRecords, false );

   cl2.remove();
   cmd.run( "rm -rf " + csvFile );

   //不指定withid，指定force为true，导出到csv
   command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + csvFile +
      " --type csv" +
      " --force true";
   testRunCommand( command );
   checkFileContent( csvFile, expectResult );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + csvFile +
      " --type csv " +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );
   expRecords = [{ "a": 1 }];
   cursor = cl2.find( {}, { _id: { $include: 0 } } );
   commCompareResults( cursor, expRecords, false );

   cl2.remove();
   cmd.run( "rm -rf " + csvFile );

   var jsonFile = tmpFileDir + "sdbexprt13539.json";
   cmd.run( "rm -rf " + jsonFile );

   //指定withid为false，导出到json
   command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + jsonFile +
      " --type json" +
      " --withid false" +
      " --sort '{ _id: 1 }'";
   testRunCommand( command );

   expResult = "{ \"a\": 1 }\n";
   checkFileContent( jsonFile, expResult );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + jsonFile +
      " --type json";
   testRunCommand( command );

   expRecords = [{ "a": 1 }];
   cursor = cl2.find();
   commCompareResults( cursor, expRecords );

   cmd.run( "rm -rf " + jsonFile );
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
