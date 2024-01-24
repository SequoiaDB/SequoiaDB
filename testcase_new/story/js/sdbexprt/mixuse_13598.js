/*******************************************************************
* @Description : test export with --select --filter --sort --skip
*                                 --limit
*                seqDB-13598:所有过滤条件组合使用导出数据
*                seqDB-11100:指定skip/limit参数导出数据
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13598";

main( test );

function test ()
{
   var docs = [{ a: 1, b: "a" }, { a: 3, b: "b" }, { a: 2 },
   { a: 4 }, { a: 7, b: "c" }, { a: 4, b: "d" },
   { a: 10 }, { a: 11, b: "k" }, { a: 15 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprt();

   commDropCL( db, csname, clname );
}

function testExprt ()
{
   var csvfile = tmpFileDir + "sdbexprt13598.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$gt: 2 } }'" +
      " --select '{ a: \"\", b: { \\$default: \"dft\"}}'" +
      " --sort '{ a: 1 }'" +
      " --skip 2" +
      " --limit 4" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n" +
      "4,\"d\"\n" +
      "7,\"c\"\n" +
      "10,\"dft\"\n" +
      "11,\"k\"\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}
