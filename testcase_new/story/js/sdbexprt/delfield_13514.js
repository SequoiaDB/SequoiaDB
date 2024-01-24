/*******************************************************************
* @Description : test export with -e random ascii
*                seqDB-13514:自定义任意16进制ascii码为字段分隔符
*                seqDB-9657:delfield参数指定16进制              
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13514";
var clname1 = COMMCLNAME + "_sdbimprt13514";
var docs = [{ a: "1", b: "1" }, { a: "2", b: "2" }];
var expRecs = ["{\"a\":\"1\",\"b\":\"1\"}",
   "{\"a\":\"2\",\"b\":\"2\"}"];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   testExprtImprt();

   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprt ()
{
   var csvfile = tmpFileDir + "sdbexprt13514.csv";
   cmd.run( "rm -rf " + csvfile );
   var asc = "0x23";
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " -e " + asc +
      " --sort '{ _id: 1 }'" +
      " --fields a,b";
   testRunCommand( command );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " -e " + "'\\35'" +
      " --fields='a string,b string'" +
      " --headerline true" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}