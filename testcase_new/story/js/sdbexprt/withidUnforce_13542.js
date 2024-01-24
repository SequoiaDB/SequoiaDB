/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13542:非强制导出，指定包含_id               
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;

main( test );

function test ()
{
   testWithIdCsv1();  // test withid true, fields no id
   testWithIdCsv2();  // test withid true, fields with id
   testWithIdJson();  // test withid true
}

function testWithIdCsv1 ()
{
   var clname = COMMCLNAME + "_sdbexprt13542";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( { _id: 1, a: 1 } );

   var csvfile = tmpFileDir + "sdbexprt13542.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a" +
      " --withid true " +
      " --force false ";
   testRunCommand( command );

   var expect = "a\n1\n";
   checkFileContent( csvfile, expect );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}

function testWithIdCsv2 ()
{
   var clname = COMMCLNAME + "_sdbexprt13542";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( { _id: 1, a: 1 } );

   var csvfile = tmpFileDir + "sdbexprt13542.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields _id,a" +
      " --withid true " +
      " --force false ";
   testRunCommand( command );

   var expect = "_id,a\n1,1\n";
   checkFileContent( csvfile, expect );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
}

function testWithIdJson ()
{
   var clname = COMMCLNAME + "_sdbexprt13542";
   var cl = commCreateCL( db, csname, clname );
   cl.insert( { _id: 1, a: 1 } );

   var jsonfile = tmpFileDir + "sdbexprt13542.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " --withid true " +
      " --force false ";
   testRunCommand( command );

   var expect = "{ \"_id\": 1, \"a\": 1 }\n";
   checkFileContent( jsonfile, expect );

   cmd.run( "rm -rf " + jsonfile );
   commDropCL( db, csname, clname );
}
