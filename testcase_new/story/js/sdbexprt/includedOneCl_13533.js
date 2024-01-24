/*******************************************************************
* @Description : test export with --included
*                seqDB-13533:导出一个集合的多个字段名到文件首行               
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13533";
var doc = { a: 1, b: 2, c: 3 };
var csvContent = "a,b,c\n1,2,3\n";

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
   var csvfile = tmpFileDir + "sdbexprt13533.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --included true " +
      " --fields a,b,c";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}