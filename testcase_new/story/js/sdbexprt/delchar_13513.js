/*******************************************************************
* @Description : test export with -a random ascii
*                seqDB-13513:自定义任意16进制ascii码为字符分隔符              
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13513";
var clname1 = COMMCLNAME + "_sdbimprt13513";
var doc = { c: 1, d: "exprtTest" };
var expRecs = ["{\"c\":1,\"d\":\"exprtTest\"}"];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( doc );

   testExprtImprt();
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprt ()
{
   var csvfile = tmpFileDir + "sdbexprt13513.csv";
   cmd.run( "rm -rf " + csvfile );
   var asc = "0x12";
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " -a " + asc +
      " --fields c,d";
   testRunCommand( command );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " -a " + "'\\18'" +
      " --headerline true " +
      " --fields='c int,d string'";
   testRunCommand( command );
   cmd.run( "rm -rf " + csvfile );
}