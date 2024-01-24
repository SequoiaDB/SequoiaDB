/*******************************************************************
* @Description : test export with --sort
*                seqDB-13586:--sort指定的排序字段未被导出
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13586";

main( test );

function test ()
{
   var docs = [{ a: 1, b: 1 }, { a: 3, b: 3 },
   { a: 2, b: 2 }, { a: 4, b: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSort();    // test sort with { a: 1 }, fields with b

   commDropCL( db, csname, clname );
}

function testExprtSort ()
{
   var csvfile = tmpFileDir + "sdbexprt13586.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1 }'" +
      " --fields b" +
      " --type csv";
   testRunCommand( command );

   var content = "b\n1\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}