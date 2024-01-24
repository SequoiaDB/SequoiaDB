/*******************************************************************
* @Description : test export with -r -a -e default
*                seqDB-13510:导出数据到csv，默认分隔符
*                seqDB-13527:导出一个集合的多个字段               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13510";
var doc = { a: 1, b: "123", c: "456" };
var csvContent = "a,b,c\n1,\"123\",\"456\"\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );
   testExprtCsv();
   commDropCL( db, csname, clname );
}

function testExprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13510.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a,b,c";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}