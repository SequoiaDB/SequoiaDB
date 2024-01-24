/*******************************************************************
* @Description : test export with -r random ascii
*                seqDB-13511:自定义任意16进制ascii码为记录分隔符，
*                            导出到json文件
*                seqDB-13512:自定义任意16进制ascii码为记录分隔符，
*                            导出到csv文件           
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13511";
var clname1 = COMMCLNAME + "_sdbimprt13511";
var docs = [{ a: "testa" }, { a: "testb" }];
var expRecs = ["{\"a\":\"testa\"}", "{\"a\":\"testb\"}"];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   testExprtImprtJson();
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cl1.truncate();

   testExprtImprtCsv();
   cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13511.json";
   cmd.run( "rm -rf " + jsonfile );
   var randNum = getRandomInt( 0, 128 );
   // avoid '"' and ','
   if( randNum == 34 || randNum == 44 ) randNum++;
   var asc = "\\" + randNum;
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " -r " + asc +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + jsonfile +
      " --type json " +
      " -r " + asc +
      " --fields='a string'";
   testRunCommand( command );

   cmd.run( "rm -rf " + jsonfile );
}

function testExprtImprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13512.csv";
   cmd.run( "rm -rf " + csvfile );
   var asc = "0xab";
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " -r " + asc +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " -r " + "'\\10b'" +
      " --headerline true" +
      " --fields='a string'";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function getRandomInt ( m, n )
{
   var range = n - m;
   var ret = m + parseInt( Math.random() * range );
   return ret;
}