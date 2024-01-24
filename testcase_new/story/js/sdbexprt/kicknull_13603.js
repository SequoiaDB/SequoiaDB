/*******************************************************************
* @Description : test export with --kicknull
*                seqDB-13603:剔除null
*                seqDB-13604:不剔除null
*                seqDB-10941:指定kicknull参数导出数据
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13603";
var docs = [{ a: 1, b: 1 }, { a: 2, b: null }];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtKicknull1();    // test kicknull true
   testExprtKicknull2();    // test kicknull false

   commDropCL( db, csname, clname );
}

function testExprtKicknull1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13603.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --kicknull true" +
      " --fields a,b" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n1,1\n2,\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtKicknull2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13603.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --kicknull false" +
      " --fields a,b" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n1,1\n2,null\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}