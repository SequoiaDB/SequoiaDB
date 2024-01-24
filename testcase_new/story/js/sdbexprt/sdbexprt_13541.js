/*******************************************************************
* @Description : seqDB-13541:生成配置文件时包含_id和不包含_id               
* @author      : Liang XueWang 
*******************************************************************/
main( test );

function test ()
{
   testWithId();
   testWithoutId();
}

function testWithId ()
{
   var clName1 = COMMCLNAME + "_sdbexprt13541";
   var clName2 = COMMCLNAME + "_sdbimprt13541";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { _id: 1, a: 1 } );

   var confFile = tmpFileDir + "sdbexprt13541.conf";
   cmd.run( "rm -rf " + confFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --genconf " + confFile +
      " --type csv" +
      " --withid true" +
      " --force true";
   testRunCommand( command );

   var csvFile = tmpFileDir + "sdbexprt13541.csv";
   cmd.run( "rm -rf " + csvFile );
   command = installPath + "bin/sdbexprt" +
      " --conf " + confFile +
      " --file " + csvFile;
   testRunCommand( command );

   var content = "_id,a\n1,1\n";
   checkFileContent( csvFile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + csvFile +
      " --type csv " +
      " --headerline true" +
      " --fields='_id int,a int'";
   testRunCommand( command );

   var expRecords = [{ "_id": 1, "a": 1 }];
   var cursor = cl1.find();
   commCompareResults( cursor, expRecords, false );

   cmd.run( "rm -rf " + confFile );
   cmd.run( "rm -rf " + csvFile );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}

function testWithoutId ()
{
   var clName1 = COMMCLNAME + "_sdbexprt13541";
   var clName2 = COMMCLNAME + "_sdbimprt13541";
   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2 );
   cl1.insert( { _id: 1, a: 1 } );

   var confFile = tmpFileDir + "sdbexprt13541.conf";
   cmd.run( "rm -rf " + confFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName1 +
      " --genconf " + confFile +
      " --type json" +
      " --withid false" +
      " --force true";
   testRunCommand( command );

   var jsonFile = tmpFileDir + "sdbexprt13541.json";
   cmd.run( "rm -rf " + jsonFile );
   command = installPath + "bin/sdbexprt" +
      " --conf " + confFile +
      " --file " + jsonFile;
   testRunCommand( command );

   var content = "{ \"a\": 1 }\n";
   checkFileContent( jsonFile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName2 +
      " --file " + jsonFile +
      " --type json " +
      " --headerline true" +
      " --fields='a int'";;
   testRunCommand( command );

   var expRecords = [{ "a": 1 }];
   var cursor = cl1.find();
   commCompareResults( cursor, expRecords );

   cmd.run( "rm -rf " + confFile );
   cmd.run( "rm -rf " + jsonFile );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}
