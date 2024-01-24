/*******************************************************************
* @Description : seqDB-13540:指定fields并强制导出数据
* @author      : Liang XueWang 
*******************************************************************/
main( test );

function test ()
{
   var clName1 = COMMCLNAME + "_sdbexprt_13540";
   var clName2 = COMMCLNAME + "_sdbimprt_13540";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { "_id": 1, "a": 1 } );

   var csvFile = tmpFileDir + "sdbexprt13540.csv";
   cmd.run( "rm -rf " + csvFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --file " + csvFile +
      " --type csv" +
      " --force true " +
      " --fields a";
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

   cmd.run( "rm -rf " + csvFile );
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
